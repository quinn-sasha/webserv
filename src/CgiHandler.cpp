#include "CgiHandler.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <fstream>

#include "Server.hpp"
#include "CgiResponseHandler.hpp"
#include "pollfd_utils.hpp"

CgiHandler::CgiHandler(const HttpRequest& request) 
    : request_(request) {}

std::vector<std::string> CgiHandler::create_env_vars(const std::string& script_path) {
  std::vector<std::string> env_vars;

  env_vars.push_back("REQUEST_METHOD=" + request_.method());
  env_vars.push_back("SCRIPT_NAME=" + request_.path());
  env_vars.push_back("PATH_INFO=" + request_.path());
  env_vars.push_back("QUERY_STRING=" + request_.query_string());
  env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
  env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
  env_vars.push_back("SCRIPT_FILENAME=" + script_path);

  // Content-Type転送
  const std::string& content_type = request_.header("Content-Type");
  if (!content_type.empty()) {
    env_vars.push_back("CONTENT_TYPE=" + content_type);
  }

  // ✅ Content-Length（デコード済みのbody_のサイズ）
  const std::string& body = request_.body();
  if (!body.empty()) {
    std::ostringstream oss;
    oss << body.size();
    env_vars.push_back("CONTENT_LENGTH=" + oss.str());
  }

  return env_vars;
}

char** CgiHandler::create_envp(const std::vector<std::string>& env_vars) {
  // char*配列を動的確保
  char** envp = new char*[env_vars.size() + 1];
  
  for (std::size_t i = 0; i < env_vars.size(); ++i) {
    envp[i] = new char[env_vars[i].size() + 1];
    std::strcpy(envp[i], env_vars[i].c_str());
  }
  envp[env_vars.size()] = NULL;
  
  return envp;
}

void CgiHandler::free_envp(char** envp) {
  for (std::size_t i = 0; envp[i] != NULL; ++i) {
    delete[] envp[i];
  }
  delete[] envp;
}

static std::string get_interpreter_from_shebang(const std::string& script_path) {
  std::ifstream f(script_path.c_str());
  if (!f.is_open()) return "";

  std::string first_line;
  std::getline(f, first_line);

  if (first_line.size() >= 2 && first_line[0] == '#' && first_line[1] == '!') {
    std::string interp = first_line.substr(2);
    while (!interp.empty() && (interp[interp.size()-1] == '\r' 
                             || interp[interp.size()-1] == ' ')) {
      interp.erase(interp.size()-1);
    }
    return interp;
  }

  return "";
}

static std::vector<std::string> get_interpreter(const std::string& script_path) {
  std::vector<std::string> args;

  // まずshebangを確認
  std::string shebang = get_interpreter_from_shebang(script_path);
  if (!shebang.empty()) {
    std::istringstream iss(shebang);
    std::string token;
    while (iss >> token) {
      args.push_back(token);
    }
    args.push_back(script_path);
    return args;
  }

  // 次に拡張子で判断
  std::size_t dot = script_path.rfind('.');
  if (dot != std::string::npos) {
    std::string ext = script_path.substr(dot);
    if (ext == ".py")       args.push_back("/usr/bin/python3");
    else if (ext == ".rb")  args.push_back("/usr/bin/ruby");
    else if (ext == ".pl")  args.push_back("/usr/bin/perl");
    else if (ext == ".sh")  args.push_back("/bin/sh");
  }

  args.push_back(script_path);
  return args;
}

int CgiHandler::execute_async(const std::string& script_path, 
                               pid_t& out_pid,
                               int& out_pipe_out_fd) {  // ✅ 引数追加
  int pipe_in[2];
  int pipe_out[2];

  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    std::cerr << "Error: pipe() failed: " << strerror(errno) << "\n";
    return -1;
  }

  pid_t pid = fork();

  if (pid == -1) {
    std::cerr << "Error: fork() failed: " << strerror(errno) << "\n";
    close(pipe_in[0]);  close(pipe_in[1]);
    close(pipe_out[0]); close(pipe_out[1]);
    return -1;
  }

  if (pid == 0) {
    close(pipe_in[1]);
    if (dup2(pipe_in[0], STDIN_FILENO) == -1) {
      std::cerr << "Error: dup2(stdin) failed: " << strerror(errno) << "\n";
      exit(1);
    }
    close(pipe_in[0]);

    close(pipe_out[0]);
    if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
      std::cerr << "Error: dup2(stdout) failed: " << strerror(errno) << "\n";
      exit(1);
    }
    close(pipe_out[1]);

    std::vector<std::string> env_vars = create_env_vars(script_path);
    char** envp = create_envp(env_vars);

    std::vector<std::string> argv_strs = get_interpreter(script_path);
    std::vector<char*> argv_ptrs;
    for (std::size_t i = 0; i < argv_strs.size(); ++i)
      argv_ptrs.push_back(const_cast<char*>(argv_strs[i].c_str()));
    argv_ptrs.push_back(NULL);

    execve(argv_ptrs[0], &argv_ptrs[0], envp);

    std::cerr << "Error: execve() failed: " << strerror(errno) << "\n";
    free_envp(envp);
    exit(1);
  }

  // 親プロセス
  close(pipe_in[0]);
  close(pipe_out[1]);

  // pipe_in をノンブロッキング（write側）
  int flags = fcntl(pipe_in[1], F_GETFL, 0);
  fcntl(pipe_in[1], F_SETFL, flags | O_NONBLOCK);

  // pipe_out をノンブロッキング（read側）
  flags = fcntl(pipe_out[0], F_GETFL, 0);
  fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);

  // ✅ write() はしない（pollに任せる）
  out_pid = pid;
  out_pipe_out_fd = pipe_out[0];  // ✅ 追加
  return pipe_in[1];
}