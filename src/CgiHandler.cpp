#include "CgiHandler.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cerrno>

CgiHandler::CgiHandler(const HttpRequest& request) 
    : request_(request) {}

std::vector<std::string> CgiHandler::create_env_vars(const std::string& script_path) {
  std::vector<std::string> env_vars;
  
  // 必須CGI環境変数（RFC 3875）
  env_vars.push_back("REQUEST_METHOD=" + request_.method());
  env_vars.push_back("SCRIPT_NAME=" + request_.path());
  env_vars.push_back("SCRIPT_FILENAME=" + script_path);
  env_vars.push_back("PATH_INFO=");
  env_vars.push_back("QUERY_STRING=");  // TODO: クエリ文字列パース
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

std::string CgiHandler::read_from_pipe(int fd) {
  std::string output;
  char buffer[4096];
  ssize_t n;
  
  while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
    output.append(buffer, n);
  }
  
  return output;
}

std::string CgiHandler::parse_cgi_output(const std::string& cgi_output) {
  // CGI出力を解析してHTTPレスポンスに変換
  
  // ヘッダーとボディを分離
  std::size_t body_start = cgi_output.find("\r\n\r\n");
  if (body_start == std::string::npos) {
    body_start = cgi_output.find("\n\n");
    if (body_start == std::string::npos) {
      return "HTTP/1.1 500 Internal Server Error\r\n\r\nCGI output error";
    }
    body_start += 2;
  } else {
    body_start += 4;
  }
  
  std::string headers = cgi_output.substr(0, body_start);
  std::string body = cgi_output.substr(body_start);
  
  // HTTPステータスラインを追加
  std::ostringstream response;
  
  // Statusヘッダーを確認
  std::size_t status_pos = headers.find("Status:");
  if (status_pos != std::string::npos) {
    // Status: 200 OK を HTTP/1.1 200 OK に変換
    std::size_t line_end = headers.find('\n', status_pos);
    std::string status_line = headers.substr(status_pos + 8, line_end - status_pos - 8);
    
    // \r を削除
    if (!status_line.empty() && status_line[status_line.size() - 1] == '\r') {
      status_line = status_line.substr(0, status_line.size() - 1);
    }
    
    response << "HTTP/1.1 " << status_line << "\r\n";
    
    // Statusヘッダーを削除
    headers.erase(status_pos, line_end - status_pos + 1);
  } else {
    // デフォルトは 200 OK
    response << "HTTP/1.1 200 OK\r\n";
  }
  
  // Content-Lengthを追加（なければ）
  if (headers.find("Content-Length:") == std::string::npos) {
    response << "Content-Length: " << body.size() << "\r\n";
  }
  
  response << headers << body;
  return response.str();
}

std::string CgiHandler::execute(const std::string& script_path) {
  int pipe_in[2];   // CGIへの入力（POSTデータ）
  int pipe_out[2];  // CGIからの出力
  
  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    std::cerr << "Error: pipe() failed: " << strerror(errno) << "\n";
    return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
  }
  
  pid_t pid = fork();
  
  if (pid == -1) {
    std::cerr << "Error: fork() failed: " << strerror(errno) << "\n";
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);
    return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
  }
  
  if (pid == 0) {
    // 子プロセス（CGIスクリプト実行）
    
    // 標準入力をpipe_inから
    close(pipe_in[1]);
    if (dup2(pipe_in[0], STDIN_FILENO) == -1) {
      std::cerr << "Error: dup2(stdin) failed: " << strerror(errno) << "\n";
      exit(1);
    }
    close(pipe_in[0]);
    
    // 標準出力をpipe_outへ
    close(pipe_out[0]);
    if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
      std::cerr << "Error: dup2(stdout) failed: " << strerror(errno) << "\n";
      exit(1);
    }
    close(pipe_out[1]);
    
    // 環境変数作成
    std::vector<std::string> env_vars = create_env_vars(script_path);
    char** envp = create_envp(env_vars);
    
    // argv作成
    char* argv[3];
    argv[0] = const_cast<char*>("python3");
    argv[1] = const_cast<char*>(script_path.c_str());
    argv[2] = NULL;
    
    // CGIスクリプト実行
    execve("/usr/bin/python3", argv, envp);
    
    // execve失敗（ここに到達したらエラー）
    std::cerr << "Error: execve() failed: " << strerror(errno) << "\n";
    
    // メモリ解放（実際には到達しない）
    free_envp(envp);
    exit(1);
  }
  
  // 親プロセス
  
  // POSTデータをCGIに送信
  close(pipe_in[0]);
  if (!request_.body().empty()) {
    ssize_t written = write(pipe_in[1], request_.body().data(), request_.body().size());
    if (written == -1) {
      std::cerr << "Error: write() failed: " << strerror(errno) << "\n";
    }
  }
  close(pipe_in[1]);
  
  // CGIの出力を読み取り
  close(pipe_out[1]);
  std::string cgi_output = read_from_pipe(pipe_out[0]);
  close(pipe_out[0]);
  
  // 子プロセス終了待機
  int status;
  if (waitpid(pid, &status, 0) == -1) {
    std::cerr << "Error: waitpid() failed: " << strerror(errno) << "\n";
    return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
  }
  
  if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    std::cerr << "CGI script exited with status " << WEXITSTATUS(status) << "\n";
    return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
  }
  
  if (WIFSIGNALED(status)) {
    std::cerr << "CGI script killed by signal " << WTERMSIG(status) << "\n";
    return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
  }
  
  // CGI出力をHTTPレスポンスに変換
  return parse_cgi_output(cgi_output);
}