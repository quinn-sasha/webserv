#include "ClientHandler.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "MonitoredFdHandler.hpp"
#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include "CgiResponseHandler.hpp"
#include "Server.hpp"
#include "pollfd_utils.hpp"

ClientHandler::ClientHandler(int client_fd, Server& server)
    : client_fd_(client_fd),
      server_(server),
      bytes_sent_(0),
      cgi_mode_(false) {}

ClientHandler::~ClientHandler() {
  if (client_fd_ != -1 && close(client_fd_) == -1) {
    std::cerr << "Error: ~ClientHandler(): close() failed\n";
  }
}

HandlerStatus ClientHandler::handle_input() {
  ssize_t num_read = recv(client_fd_, recv_buffer_, buf_size, 0);
  
  if (num_read <= 0) {
    return kClosed;
  }

  HttpRequest::ParseResult result = request_.parse(recv_buffer_, num_read);
  
  if (result == HttpRequest::kError) {
    HttpResponse response;
    response.set_status(400, "Bad Request");
    response.set_body("<h1>400 Bad Request</h1>");
    send_buffer_ = response.to_string();
    bytes_sent_ = 0;
    return kReceived;
  }
  
  if (result == HttpRequest::kIncomplete) {
    return kContinue;
  }
  
  std::cout << "Method: " << request_.method() << "\n";
  std::cout << "Path: " << request_.path() << "\n";
  std::cout << "Query: " << request_.query_string() << "\n";  // デバッグ用
  
  generate_response();
  
  // CGIモードの場合はこのハンドラを削除
  if (cgi_mode_) {
    return kClosed;
  }
  
  return kReceived;
}

HandlerStatus ClientHandler::handle_output() {
  if (send_buffer_.empty()) {
    std::cerr << "Error: handle_output() called but no data to send\n";
    return kClosed;
  }
  
  ssize_t remaining = send_buffer_.size() - bytes_sent_;
  ssize_t num_sent = send(client_fd_, send_buffer_.data() + bytes_sent_, 
                          remaining, 0);
  
  if (num_sent == -1) {
    return kClosed;
  }
  
  bytes_sent_ += num_sent;
  
  if (bytes_sent_ < static_cast<ssize_t>(send_buffer_.size())) {
    return kContinue;
  }
  
  return kAllSent;
}

void ClientHandler::generate_response() {
  HttpResponse response;
  
  if (request_.path() == "/") {
    response.set_status(200, "OK");
    response.add_header("Content-Type", "text/html");
    response.set_body("<h1>Hello, World!</h1>");
    send_buffer_ = response.to_string();
    bytes_sent_ = 0;
  } else if (request_.is_cgi()) {
    CgiHandler cgi(request_);
    std::string script_path = request_.path().substr(1);
    
    std::cout << "Starting CGI (async): " << script_path << "\n";
    
    pid_t cgi_pid;
    int cgi_fd = cgi.execute_async(script_path, cgi_pid);
    
    if (cgi_fd == -1) {
      response.set_status(500, "Internal Server Error");
      response.set_body("<h1>500 Internal Server Error</h1>");
      send_buffer_ = response.to_string();
      bytes_sent_ = 0;
      return;
    }
    
    // CgiResponseHandlerを登録
    server_.register_cgi_response(cgi_fd, client_fd_, cgi_pid);
    
    // client_fd_の所有権をCgiResponseHandlerに移譲
    client_fd_ = -1;
    cgi_mode_ = true;  // CGIモードフラグをセット
    
  } else {
    response.set_status(404, "Not Found");
    response.add_header("Content-Type", "text/html");
    response.set_body("<h1>404 Not Found</h1>");
    send_buffer_ = response.to_string();
    bytes_sent_ = 0;
  }
}
