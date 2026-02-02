#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "ListenSocket.hpp"
#include "SystemError.hpp"

#define BUF_SIZE 4096

void handle_echo_request(int client_fd) {
  char buffer[BUF_SIZE];
  while (true) {
    ssize_t num_read = recv(client_fd, buffer, BUF_SIZE, 0);
    if (num_read == -1) {
      throw SystemError("recv()");
    }
    if (num_read == 0) {
      return;
    }
    if (send(client_fd, buffer, num_read, 0) != num_read) {
      throw SystemError("send()");
    }
  }
}

int main() {
  try {
    // Temporal port number and backlog
    ListenSocket listen_socket("8888", 100);
    while (true) {
      int client_fd = accept(listen_socket.fd(), NULL, NULL);
      if (client_fd == -1) {
        throw SystemError("accept()");
      }
      handle_echo_request(client_fd);
      close(client_fd);
    }
    // TODO: cannot distinguish system error from other errors
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
