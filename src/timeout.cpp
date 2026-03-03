#include "Server.hpp"
#include "timeout.hpp"

#include <poll.h>
#include <signal.h>
#include <ctime>
#include <cstddef>
#include <iostream>
#include <map>

#include "ClientHandler.hpp"
#include "MonitoredFdHandler.hpp"
#include "SystemError.hpp"

namespace timeout {

int64_t now_time() {
  return static_cast<int64_t>(std::time(NULL));
}

int poll_timeout_ms(int64_t ms, int max_ms) {
  if (ms < 0) return 0;
  if (ms > max_ms) return max_ms;
  return static_cast<int>(ms);
}
}

// --- poll timeout 計算 + poll 呼び出し + timeout発火処理 ---
bool Server::poll_with_deadlines_(int& poll_ret) {
  const int64_t now = timeout::now_time();

  bool has_deadline = false;
  int64_t nearest_deadline = 0;

  for (std::size_t i = 0; i < poll_fds_.size(); ++i) {
    int fd = poll_fds_[i].fd;
    std::map<int, MonitoredFdHandler*>::iterator it =
        monitored_fd_to_handler_.find(fd);
    if (it == monitored_fd_to_handler_.end()) continue;

    MonitoredFdHandler* h = it->second;
    if (h == NULL) continue;
    if (!h->has_deadline()) continue;

    int64_t dl = h->deadline_ms();
    if (!has_deadline || dl < nearest_deadline) {
      has_deadline = true;
      nearest_deadline = dl;
    }
  }

  int timeout_ms = -1;
  if (has_deadline) {
    timeout_ms = timeout::poll_timeout_ms(nearest_deadline - now, 1000);
  }

  poll_ret = poll(&poll_fds_[0], poll_fds_.size(), timeout_ms);
  if (poll_ret == -1) {
    throw SystemError("poll");
  }

  if (poll_ret == 0) {
    // timeout 発火処理を実行し、イベントなしとして false を返す
    (void)handle_timeouts_();
    return false;
  }

  return true;
}

bool Server::handle_timeouts_() {
  const int64_t now = timeout::now_time();

  for (std::size_t i = 0; i < poll_fds_.size(); ++i) {
    int fd = poll_fds_[i].fd;

    std::map<int, MonitoredFdHandler*>::iterator it =
        monitored_fd_to_handler_.find(fd);
    if (it == monitored_fd_to_handler_.end()) continue;

    MonitoredFdHandler* h = it->second;
    if (h == NULL) continue;

    if (!h->has_deadline()) continue;
    if (now < h->deadline_ms()) continue;

    HandlerStatus st = h->handle_timeout();

    if (st == kHandlerFatalError) {
      std::cerr << "Fatal error occurred. Stopping server.\n";
      return false;
    }

    if (st == kCgiInputDone || st == kHandlerClosed) {
      if (find_client_handler(poll_fds_[i].fd) != NULL) {
        remove_client(i);
      } else {
        remove_fd(i);
      }
      --i;
      continue;
    }
  }
  return true;
}