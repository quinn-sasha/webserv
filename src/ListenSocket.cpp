#include "ListenSocket.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "SystemError.hpp"

ListenSocket::ListenSocket(const std::string& addr, const std::string& port,
                           int maxpending)
    : fd_(-1) {
  struct addrinfo hints;
  std::memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo* result_info;
  int status = getaddrinfo(addr.c_str(), port.c_str(), &hints, &result_info);
  if (status != 0) {
    std::string msg = "getaddrinfo(): " + std::string(gai_strerror(status));
    throw std::runtime_error(msg);
  }
  struct addrinfo* node;
  int sfd;
  bool succeeds = false;
  for (node = result_info; node != NULL; node = node->ai_next) {
    sfd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
    if (sfd == -1) {
      continue;
    }
    if (fcntl(sfd, F_SETFL, O_NONBLOCK) == -1) {
      close(sfd);
      freeaddrinfo(result_info);
      throw SystemError("fcntl()");
    }
    int optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) ==
        -1) {
      close(sfd);
      freeaddrinfo(result_info);
      throw SystemError("setsockopt()");
    }
    if (bind(sfd, node->ai_addr, node->ai_addrlen) == 0) {
      succeeds = true;
      break;
    }
    close(sfd);
  }
  freeaddrinfo(result_info);
  if (!succeeds) {
    throw std::runtime_error("Coludn't bind socket to any address\n");
  }
  if (listen(sfd, maxpending) == -1) {
    close(sfd);
    throw SystemError("listen()");
  }
  fd_ = sfd;
}

ListenSocket::~ListenSocket() {
  if (fd_ == -1) {
    std::cerr << "Tried to destroy uninitialized socket\n";
    return;
  }
  if (close(fd_) == -1) {
    std::cerr << "Error: ~ListenSocket(): close()\n";
  }
}
