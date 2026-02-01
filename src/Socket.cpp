#include "Socket.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <string>

int inet_connect(const std::string& host, const std::string& service,
                 int socktype) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = socktype;
  struct addrinfo* result_info;
  if (getaddrinfo(host.c_str(), service.c_str(), &hints, &result_info) == -1) {
    return -1;
  }
  struct addrinfo* node;
  int sfd;
  bool succeeds = false;
  for (node = result_info; node != NULL; node = node->ai_next) {
    sfd = socket(node->ai_family, node->ai_socktype, node->ai_flags);
    if (sfd == -1) {
      continue;
    }
    if (connect(sfd, node->ai_addr, node->ai_addrlen) == 0) {
      succeeds = true;
      break;
    }
    close(sfd);
  }
  freeaddrinfo(result_info);
  if (!succeeds) {
    return -1;
  }
  return sfd;
}

int inet_listen(const std::string& service, int backlog) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo* result_info;
  if (getaddrinfo(NULL, service.c_str(), &hints, &result_info) != 0) {
    return -1;
  }
  struct addrinfo* node;
  int sfd;
  bool succeeds = false;
  for (node = result_info; node != NULL; node = node->ai_next) {
    sfd = socket(node->ai_family, node->ai_socktype, node->ai_flags);
    if (sfd == -1) {
      continue;
    }
    int optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) ==
        -1) {
      close(sfd);
      freeaddrinfo(result_info);
      return -1;
    }
    if (bind(sfd, node->ai_addr, node->ai_addrlen) == 0) {
      succeeds = true;
      break;
    }
    close(sfd);
  }
  freeaddrinfo(result_info);
  if (!succeeds) {
    return -1;
  }
  if (listen(sfd, backlog) == -1) {
    close(sfd);
    return -1;
  }
  return sfd;
}
