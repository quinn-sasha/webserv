#ifndef INCLUDE_POLLFD_UTILS_HPP_
#define INCLUDE_POLLFD_UTILS_HPP_

#include <poll.h>

void set_pollfd_in(struct pollfd& poll_fd, int fd);
void set_pollfd_out(struct pollfd& poll_fd, int fd);

#endif  // INCLUDE_POLLFD_UTILS_HPP_
