#include "CgiResponseHandler.hpp"

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

#include "Server.hpp"

CgiResponseHandler::CgiResponseHandler(int cgi_fd, int client_fd, 
                                       pid_t cgi_pid)
    : cgi_fd_(cgi_fd),
      client_fd_(client_fd),
      cgi_pid_(cgi_pid),
      bytes_sent_(0),
      cgi_finished_(false)
      {}

CgiResponseHandler::~CgiResponseHandler() {
  if (cgi_fd_ != -1) {
    close(cgi_fd_);
  }
  
  // CGIプロセスが生きていたら終了待機
  int status;
  if (waitpid(cgi_pid_, &status, WNOHANG) == 0) {
    waitpid(cgi_pid_, &status, 0);
  }
  
  if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    std::cerr << "CGI exited with status " << WEXITSTATUS(status) << "\n";
  }
  if (WIFSIGNALED(status)) {
    std::cerr << "CGI killed by signal " << WTERMSIG(status) << "\n";
  }
}

HandlerStatus CgiResponseHandler::handle_input() {
  char buffer[4096];
  ssize_t n = read(cgi_fd_, buffer, sizeof(buffer));
  
  if (n == -1) {
    return kClosed;
  }
  
  if (n == 0) {
    // CGI出力受信完了
    cgi_finished_ = true;
    send_buffer_ = parse_cgi_output(cgi_output_);
    bytes_sent_ = 0;
    return kCgiReceived;  // ✅ kCgiReceivedを返す
  }
  
  cgi_output_.append(buffer, n);
  return kContinue;
}

HandlerStatus CgiResponseHandler::handle_output() {
  if (cgi_fd_ != -1) {
    close(cgi_fd_);
    cgi_fd_ = -1;
  }
  
  if (send_buffer_.empty()) {
    return kClosed;
  }
  
  ssize_t remaining = send_buffer_.size() - bytes_sent_;
  ssize_t n = send(client_fd_, send_buffer_.data() + bytes_sent_, 
                   remaining, 0);
  
  if (n == -1) {
    return kClosed;
  }
  
  bytes_sent_ += n;
  
  if (bytes_sent_ < static_cast<ssize_t>(send_buffer_.size())) {
    return kContinue;
  }
  
  return kAllSent;
}

HandlerStatus CgiResponseHandler::handle_poll_error() {
  std::cerr << "Error: poll error on CGI fd\n";
  return kClosed;
}

std::string CgiResponseHandler::parse_cgi_output(const std::string& cgi_output) {
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
  
  std::ostringstream response;
  
  // Statusヘッダー確認
  std::size_t status_pos = headers.find("Status:");
  if (status_pos != std::string::npos) {
    std::size_t line_end = headers.find('\n', status_pos);
    std::string status_line = headers.substr(status_pos + 8, line_end - status_pos - 8);
    
    if (!status_line.empty() && status_line[status_line.size() - 1] == '\r') {
      status_line = status_line.substr(0, status_line.size() - 1);
    }
    
    response << "HTTP/1.1 " << status_line << "\r\n";
    headers.erase(status_pos, line_end - status_pos + 1);
  } else {
    response << "HTTP/1.1 200 OK\r\n";
  }
  
  // Transfer-Encoding: chunked の場合は Content-Length を付けない
  if (headers.find("Transfer-Encoding") != std::string::npos &&
      headers.find("chunked") != std::string::npos) {
  } else {
    // 通常レスポンス → Content-Length
    response << "Content-Length: " << body.size() << "\r\n";
  }
  
  response << headers << body;
  return response.str();
}