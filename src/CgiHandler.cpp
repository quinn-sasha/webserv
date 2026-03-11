#include "CgiHandler.hpp"
#include "MetaVariables.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>   
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <fstream>

#include "Server.hpp"
#include "CgiResponseHandler.hpp"
#include "pollfd_utils.hpp"
#include "string_utils.hpp"

CgiHandler::CgiHandler(const Request& request,
                       const std::string& query_string,
                       const std::string& server_name,
                       const std::string& server_port,
                       const std::string& remote_addr)
    : request_(request),
      pipe_in_fd_(-1),
      pipe_out_fd_(-1),
      cgi_pid_(-1),
      query_string_(query_string),
      server_name_(server_name),
      server_port_(server_port),
      remote_addr_(remote_addr) {}

CgiHandler::~CgiHandler() {
}

static void cgi_error_exit(const char* func_name, int code) {
  std::cerr << "Error: " << func_name << " " << strerror(errno) << "\n";
  std::exit(code);
}

static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    return -1;
  }
  return 0;
}

static bool is_executable_regular_file(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }  
  if (!S_ISREG(st.st_mode)) {
    return false;
  }
  if ((st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
    return false;
  }
  return true;
}

// static std::vector<std::string> split_ws(const std::string& s) {
//   std::istringstream iss(s);
//   std::vector<std::string> toks;
//   std::string t;
//   while (iss >> t) {
//     toks.push_back(t);
//   }
//   return toks;
// }

// static std::vector<std::string> shebang_tokens(const std::string& script_path) {
//   std::ifstream ifs(script_path.c_str());
//   if (!ifs.is_open()) return std::vector<std::string>();

//   std::string line;
//   std::getline(ifs, line);
//   line = trim(line, "\t \r");

//   if (line.size() >= 2 && line[0] == '#' && line[1] == '!') {
//     std::string rest = line.substr(2);
//     while (!rest.empty() && (rest[0] == ' ' || rest[0] == '\t')) {
//       rest.erase(0, 1); 
//     }
//     return split_ws(rest);
//   }
//   return std::vector<std::string>();
// }



static CgiHandler::ExecArgv build_exec_argv(const std::string& script_name,
                                            const std::string& cgi_path) {
  CgiHandler::ExecArgv execargv;
  if (!is_executable_regular_file(script_name)) 
  return execargv;

  // if (!cgi_path.empty()) {
    execargv.file = cgi_path;
    execargv.argv.push_back(execargv.file);
    execargv.argv.push_back(script_name);
    return execargv;
  //}

  // std::vector<std::string> shebang = shebang_tokens(script_name);
  // if (!shebang.empty()) {
  //   execargv.file = shebang[0];
  //   execargv.argv = shebang;
  //   execargv.argv.push_back(script_name);
  //   return execargv;
  // }

  // execargv.file = "./" + script_name;
  // execargv.argv.push_back(execargv.file);
  //return execargv;
}

static std::string prepare_script_name(const std::string& script_path) {
  std::size_t slash = script_path.rfind('/');
  if (slash == std::string::npos) {
    return script_path;
  }
  std::string script_dir = script_path.substr(0, slash);
  if (chdir(script_dir.c_str()) == -1) {
    cgi_error_exit("chdir", -1);
  }
  return script_path.substr(slash + 1);
}

void CgiHandler::exec_cgi_child(int pipe_in[2], int pipe_out[2], const std::string& script_path, const std::string& cgi_path) {
  close(pipe_in[1]);
  if (dup2(pipe_in[0], STDIN_FILENO) == -1) {
    cgi_error_exit("dup2(stdin)", -1);
  }
  close(pipe_in[0]);

  close(pipe_out[0]);
  if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
    cgi_error_exit("dup2(stdout)", -1);
  }
  close(pipe_out[1]);
  
  std::string script_name = prepare_script_name(script_path);

  MetaVariables env = \
  MetaVariables::from_request(request_, script_name, query_string_,
                                  server_name_, server_port_, remote_addr_);

  ExecArgv eargv = build_exec_argv(script_name, cgi_path);

  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(eargv.argv.size() + 1);
  for (std::size_t i = 0; i < eargv.argv.size(); ++i) {
    argv_ptrs.push_back(const_cast<char*>(eargv.argv[i].c_str()));
  }
  argv_ptrs.push_back(NULL);

  execve(eargv.file.c_str(), &argv_ptrs[0], env.build_envp());

  cgi_error_exit("execve", 127);
}

int CgiHandler::execute_cgi(const std::string& script_path, const std::string& cgi_path) {  
  int pipe_in[2];
  int pipe_out[2];

  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    std::cerr << "Error: pipe " << strerror(errno) << "\n";
    return -1;
  }

  cgi_pid_ = fork();

  if (cgi_pid_ == -1) {
    std::cerr << "Error: fork " << strerror(errno) << "\n";
    close(pipe_in[0]);  
    close(pipe_in[1]);
    close(pipe_out[0]); 
    close(pipe_out[1]);
    return -1;
  }

  if (cgi_pid_ == 0) {
    exec_cgi_child(pipe_in, pipe_out, script_path, cgi_path);
  }

  close(pipe_in[0]);
  close(pipe_out[1]);

  if (set_nonblocking(pipe_in[1]) == -1 || set_nonblocking(pipe_out[0]) == -1) {
    std::cerr << "Error: fcntl " << strerror(errno) << "\n";
    return -1;
  }

  pipe_in_fd_ = pipe_in[1];
  pipe_out_fd_ = pipe_out[0];
  
  return 0; // 成功
}


