#include "pollfd_utils.hpp"

#include <cstring>

void set_pollfd_in(struct pollfd& poll_fd, int fd) {
  std::memset(&poll_fd, 0, sizeof(struct pollfd));
  poll_fd.fd = fd;
  poll_fd.events = POLLIN;
}

void set_pollfd_out(struct pollfd& poll_fd, int fd) {
  std::memset(&poll_fd, 0, sizeof(struct pollfd));
  poll_fd.fd = fd;
  poll_fd.events = POLLOUT;
}
