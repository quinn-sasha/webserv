#ifndef INCLUDE_CLIENTHANDLER_HPP_
#define INCLUDE_CLIENTHANDLER_HPP_

#include <sys/socket.h>

#include <cstddef>

#include "MonitoredFdHandler.hpp"

class Server;

class ClientHandler : public MonitoredFdHandler {
  int client_fd_;
  Server& server_;
  ssize_t bytes_to_send_;
  ssize_t bytes_sent_;
  static const std::size_t buf_size = SO_RCVBUF;
  char buffer_[buf_size];

  ClientHandler(const ClientHandler& other);
  ClientHandler operator=(const ClientHandler& other);

 public:
  ClientHandler(int client_fd, Server& server);
  ~ClientHandler();
  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error() { return kClosed; }
};

#endif  // INCLUDE_CLIENTHANDLER_HPP_
