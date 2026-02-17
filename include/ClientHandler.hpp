#ifndef INCLUDE_CLIENTHANDLER_HPP_
#define INCLUDE_CLIENTHANDLER_HPP_

#include <sys/socket.h>
#include <cstddef>
#include <string>

#include "MonitoredFdHandler.hpp"
#include "HttpRequest.hpp"  

class Server;

class ClientHandler : public MonitoredFdHandler {
  int client_fd_;
  Server& server_;
  
  // 受信バッファ
  static const std::size_t buf_size = 4096; //SO_RCVBUF;
  char recv_buffer_[buf_size];
  
  // HTTPリクエスト
  HttpRequest request_;  
  
  // 送信バッファ
  std::string send_buffer_;  
  ssize_t bytes_sent_;

  bool cgi_mode_; 

  ClientHandler(const ClientHandler& other);
  ClientHandler operator=(const ClientHandler& other);
  
  void generate_response();  

 public:
  ClientHandler(int client_fd, Server& server);
  ~ClientHandler();
  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error() { return kClosed; }
};

#endif  // INCLUDE_CLIENTHANDLER_HPP_
