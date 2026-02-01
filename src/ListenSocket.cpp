#include "ListenSocket.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

ListenSocket::ListenSocket(const std::string& service, int backlog) : fd_(-1) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo* result_info;
  int status = getaddrinfo(NULL, service.c_str(), &hints, &result_info);
  if (status != 0) {
    std::cerr << "getaddrinfo(): " << gai_strerror(status) << "\n";
    return;
  }
  struct addrinfo* node;
  int sfd;
  bool succeeds = false;
  for (node = result_info; node != NULL; node = node->ai_next) {
    sfd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
    if (sfd == -1) {
      continue;
    }
    int optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) ==
        -1) {
      close(sfd);
      freeaddrinfo(result_info);
      std::cerr << "setsockopt(): " << strerror(errno) << "\n";
      return;
    }
    if (bind(sfd, node->ai_addr, node->ai_addrlen) == 0) {
      succeeds = true;
      break;
    }
    close(sfd);
  }
  freeaddrinfo(result_info);
  if (!succeeds) {
    std::cerr << "Coludn't set socket to any address\n";
    return;
  }
  if (listen(sfd, backlog) == -1) {
    close(sfd);
    std::cerr << "listen(): " << strerror(errno) << "\n";
    return;
  }
  fd_ = sfd;
}

ListenSocket::~ListenSocket() {
  if (fd_ == -1) {
    std::cerr << "~ListenSocket(): tried to destroy uninitialized socket\n";
  }
  if (close(fd_) == -1) {
    std::cerr << "~ListenSocket close(): " << strerror(errno) << "\n";
  }
}
