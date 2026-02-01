#ifndef INCLUDE_LISTENSOCKET_HPP_
#define INCLUDE_LISTENSOCKET_HPP_

#include <netdb.h>
#include <unistd.h>

#include <string>

class ListenSocket {
  int fd_;
  // Prohibit this 2 operations to prevent double close
  ListenSocket(const ListenSocket&);
  ListenSocket& operator=(const ListenSocket&);

 public:
  ListenSocket(const std::string& service, int backlog);
  ~ListenSocket();
  int fd() const { return fd_; };
};

#endif  // INCLUDE_LISTENSOCKET_HPP_
