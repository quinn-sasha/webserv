#include "ClientHandler.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "MonitoredFdHandler.hpp"

ClientHandler::ClientHandler(int client_fd, Server& server)
    : client_fd_(client_fd),
      server_(server),
      bytes_to_send_(0),
      bytes_sent_(0) {}

ClientHandler::~ClientHandler() {
  if (close(client_fd_) == -1) {
    std::cerr << "Error: ~ClientHandler(): close()\n";
  }
}

HandlerStatus ClientHandler::handle_input() {
  ssize_t num_read = recv(client_fd_, buffer_, buf_size, 0);
  if (num_read == -1) {
    return kClosed;
  }
  if (num_read == 0) {  // Connection closed
    return kClosed;
  }
  bytes_to_send_ = num_read;
  return kReceived;
}

HandlerStatus ClientHandler::handle_output() {
  if (bytes_to_send_ <= 0) {
    std::cerr << "Error: handle_output() called, but no data to send\n";
    return kContinue;
  }
  ssize_t remaining = bytes_to_send_ - bytes_sent_;
  ssize_t num_sent = send(client_fd_, buffer_ + bytes_sent_, remaining, 0);
  if (num_sent == -1) {
    return kClosed;
  }
  bytes_sent_ += num_sent;
  if (bytes_sent_ < bytes_to_send_) {
    return kContinue;
  }
  bytes_sent_ = 0;
  bytes_to_send_ = 0;
  return kAllSent;
}
