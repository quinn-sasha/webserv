#include "Server.hpp"

#include <exception>
#include <iostream>
#include <string>

#include "ListenSocket.hpp"
#include "SystemError.hpp"

Server::Server(const std::string& service, int maxpending)
    : listen_socket_(service, maxpending) {}

// Have responsible for freeing client_fd
void Server::handle_echo_request(int client_fd) const {
  char buffer[buffer_size];
  while (true) {
    ssize_t num_read = recv(client_fd, buffer, buffer_size, 0);
    if (num_read == -1) {
      close(client_fd);
      throw SystemError("recv()");
    }
    if (num_read == 0) {
      return;
    }
    if (send(client_fd, buffer, num_read, 0) != num_read) {
      close(client_fd);
      throw SystemError("send()");
    }
  }
}

void Server::run() const {
  while (true) {
    try {
      int client_fd = accept(listen_socket_.fd(), NULL, NULL);
      if (client_fd == -1) {
        throw SystemError("accept()");
      }
      handle_echo_request(client_fd);
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }
}
