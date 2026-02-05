#ifndef INCLUDE_SERVER_HPP_
#define INCLUDE_SERVER_HPP_

#include <cstddef>
#include <string>

#include "ListenSocket.hpp"

class Server {
  ListenSocket listen_socket_;
  static const std::size_t buffer_size = 4096;

 public:
  Server(const std::string& addr, const std::string& port, int maxpending);
  void run() const;
  void handle_echo_request(int client_fd) const;  // Temporal member function
};

#endif  // INCLUDE_SERVER_HPP_
