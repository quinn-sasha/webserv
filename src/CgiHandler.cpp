#include "CgiHandler.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cerrno>

#include "Server.hpp"
#include "CgiResponseHandler.hpp"
#include "pollfd_utils.hpp"

CgiHandler::CgiHandler(const HttpRequest& request) 
    : request_(request) {}

std::vector<std::string> CgiHandler::create_env_vars(const std::string& script_path) {
  std::vector<std::string> env_vars;
  
  // 必須CGI環境変数（RFC 3875）
  env_vars.push_back("REQUEST_METHOD=" + request_.method());
  env_vars.push_back("SCRIPT_NAME=" + request_.path());
  env_vars.push_back("SCRIPT_FILENAME=" + script_path);
  env_vars.push_back("PATH_INFO=");
  env_vars.push_back("QUERY_STRING=" + request_.query_string());
  env_vars.push_back("SERVER_SOFTWARE=webserv/1.0");
  env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
  env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
  
  // Content-Type, Content-Length (POSTの場合)
  const std::string& content_type = request_.header("Content-Type");
  if (!content_type.empty()) {
    env_vars.push_back("CONTENT_TYPE=" + content_type);
  }
  
  const std::string& content_length = request_.header("Content-Length");
  if (!content_length.empty()) {
    env_vars.push_back("CONTENT_LENGTH=" + content_length);
  }
  
  // Host
  const std::string& host = request_.header("Host");
  if (!host.empty()) {
    env_vars.push_back("HTTP_HOST=" + host);
  }
  
  // PATH（インタプリタ実行に必要）
  env_vars.push_back("PATH=/usr/local/bin:/usr/bin:/bin");
  
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

int CgiHandler::execute_async(const std::string& script_path, 
                               pid_t& out_pid) {
  int pipe_in[2];
  int pipe_out[2];
  
  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    std::cerr << "Error: pipe() failed: " << strerror(errno) << "\n";
    return -1;
  }
  
  pid_t pid = fork();
  
  if (pid == -1) {
    std::cerr << "Error: fork() failed: " << strerror(errno) << "\n";
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);
    return -1;
  }
  
  if (pid == 0) {
    // 子プロセス
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
    
    char* argv[3];
    argv[0] = const_cast<char*>("python3");
    argv[1] = const_cast<char*>(script_path.c_str());
    argv[2] = NULL;
    
    execve("/usr/bin/python3", argv, envp);
    
    std::cerr << "Error: execve() failed: " << strerror(errno) << "\n";
    free_envp(envp);
    exit(1);
  }
  
  // 親プロセス
  close(pipe_in[0]);
  
  // POSTデータを送信
  if (!request_.body().empty()) {
    ssize_t written = write(pipe_in[1], request_.body().data(), 
                            request_.body().size());
    if (written == -1) {
      std::cerr << "Error: write() failed: " << strerror(errno) << "\n";
    }
  }
  close(pipe_in[1]);
  
  // 出力パイプをノンブロッキングに
  close(pipe_out[1]);
  int flags = fcntl(pipe_out[0], F_GETFL, 0);
  if (fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK) == -1) {
    std::cerr << "Error: fcntl(O_NONBLOCK) failed: " << strerror(errno) << "\n";
    close(pipe_out[0]);
    return -1;
  }
  
  out_pid = pid;
  return pipe_out[0];  // 読み込み側のfdを返す
}