#include "pollfd_utils.hpp"

#include <gtest/gtest.h>

TEST(PollfdUtilsTest, SetIn) {
  struct pollfd poll_fd;
  set_pollfd_in(poll_fd, 42);
  EXPECT_EQ(poll_fd.fd, 42);
  EXPECT_EQ(poll_fd.events, POLLIN);
}

TEST(PollfdUtilsTest, SetOut) {
  struct pollfd poll_fd;
  set_pollfd_out(poll_fd, 42);
  EXPECT_EQ(poll_fd.fd, 42);
  EXPECT_EQ(poll_fd.events, POLLOUT);
}
