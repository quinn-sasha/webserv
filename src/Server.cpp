#include "Server.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/poll.h>

#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <ctime>
#include <climits>
#include <stdint.h>

#include "AcceptHandler.hpp"
#include "CgiInputHandler.hpp"
#include "CgiResponseHandler.hpp"
#include "ClientHandler.hpp"
#include "ListenSocket.hpp"
#include "MonitoredFdHandler.hpp"
#include "SystemError.hpp"
#include "pollfd_utils.hpp"
#include "string_utils.hpp"

static int64_t now_ms_server() {
  return static_cast<int64_t>(std::time(NULL)) * 1000;
}

static int clamp_timeout_ms(int64_t ms) {
  if (ms < 0) return 0;
  if (ms > 1000) return 1000;
  return static_cast<int>(ms);
}

Server::Server(const std::string& config_file) : num_clients_(0) {
  config_.load_file(config_file);
  for (std::size_t i = 0; i < config_.get_configs().size(); i++) {
    const ServerContext& sc = config_.get_configs()[i];
    for (std::size_t j = 0; j < sc.listens.size(); j++) {
      std::string addr = sc.listens[j].address;
      std::string port = int_to_string(sc.listens[j].port);
      ListenSocket* listen_sock = new ListenSocket(addr, port, kMaxClients);
      listen_sockets_.push_back(listen_sock);
      struct pollfd tmp;
      set_pollfd_in(tmp, listen_sock->fd());
      poll_fds_.push_back(tmp);
      MonitoredFdHandler* handler =
          new AcceptHandler(listen_sock->fd(), *this, addr, port);
      monitored_fd_to_handler_[listen_sock->fd()] = handler;
    }
  }
}

Server::~Server() {
  for (std::size_t i = 0; i < listen_sockets_.size(); i++) {
    delete listen_sockets_[i];
  }
  std::map<int, MonitoredFdHandler*>::iterator iter;
  for (iter = monitored_fd_to_handler_.begin();
       iter != monitored_fd_to_handler_.end(); iter++) {
    delete iter->second;
  }
}

void Server::run() {
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    throw SystemError("signal()");
  }
  while (true) {
    if (poll_fds_.empty()) {
      throw std::runtime_error("Error: poll_fds_ must not be empty");
    }

    // 次のdeadlineまでの時間を poll の timeout にする
    int64_t now = now_ms_server();
    bool has_deadline = false;
    int64_t nearest_deadline = 0;

    for (std::size_t i = 0; i < poll_fds_.size(); ++i) {
      int fd = poll_fds_[i].fd;
      std::map<int, MonitoredFdHandler*>::iterator it =
          monitored_fd_to_handler_.find(fd);
      if (it == monitored_fd_to_handler_.end()) continue;

      MonitoredFdHandler* h = it->second;
      if (!h->has_deadline()) continue;

      int64_t dl = h->deadline_ms();
      if (!has_deadline || dl < nearest_deadline) {
        has_deadline = true;
        nearest_deadline = dl;
      }
    }

    int timeout_ms = -1;
    if (has_deadline) {
      timeout_ms = clamp_timeout_ms(nearest_deadline - now);
    }

    int poll_ret = poll(&poll_fds_[0], poll_fds_.size(), timeout_ms);
    if (poll_ret == -1) {
      throw SystemError("poll");
    }

    if (poll_ret == 0) {
      // timeout 発火: deadline切れハンドラを処理
      now = now_ms_server();
      for (std::size_t i = 0; i < poll_fds_.size(); ++i) {
        int fd = poll_fds_[i].fd;
        std::map<int, MonitoredFdHandler*>::iterator it =
            monitored_fd_to_handler_.find(fd);
        if (it == monitored_fd_to_handler_.end()) continue;

        MonitoredFdHandler* h = it->second;
        if (!h->has_deadline()) continue;
        if (now < h->deadline_ms()) continue;

        HandlerStatus st = h->handle_timeout();

        // CGI timeout で kCgiReceived を返した場合、通常の差し替え処理を再利用
        if (st == kCgiReceived) {
          CgiResponseHandler* cgi_handler =
              static_cast<CgiResponseHandler*>(h);

          int old_fd = poll_fds_[i].fd;
          int new_fd = cgi_handler->get_client_fd();

          poll_fds_[i].fd = new_fd;
          set_pollfd_out(poll_fds_[i], new_fd);

          monitored_fd_to_handler_.erase(old_fd);
          monitored_fd_to_handler_.insert(std::make_pair(new_fd, h));
          continue;
        }

        if (st == kCgiInputDone) {
          poll_fds_.erase(poll_fds_.begin() + i);
          monitored_fd_to_handler_.erase(fd);
          delete h;
          --i;
          continue;
        }

        if (st == kHandlerClosed) {
          remove_client(i);
          --i;
          continue;
        }

        if (st == kHandlerFatalError) {
          std::cerr << "Fatal error occurred. Stopping server.\n";
          return;
        }
      }
      continue;
    }

    for (std::size_t i = 0; i < poll_fds_.size(); ++i) {
      if (poll_fds_[i].revents == 0) {
        continue;
      }
      HandlerStatus status = handle_fd_event(i);
      if (status == kHandlerContinue || status == kCgiReceived) {
        continue;
      }
      if (status == kCgiInputDone) {
        --i;
        continue;
      }
      if (status == kHandlerFatalError) {
        std::cerr << "Fatal error occurred. Stopping server.\n";
        return;
      }
      if (status == kHandlerClosed) {
        remove_client(i);
        --i;
      }
    }
  }
}

HandlerStatus Server::handle_fd_event(int pollfd_index) {
  struct pollfd& poll_fd = poll_fds_[pollfd_index];
  MonitoredFdHandler* handler = monitored_fd_to_handler_[poll_fd.fd];

  if (poll_fd.revents & (POLLERR | POLLNVAL)) {
    return handler->handle_poll_error();
  }

  if (poll_fd.revents & (POLLIN | POLLHUP)) {
    HandlerStatus status = handler->handle_input();

    if (status == kCgiReceived) {
      CgiResponseHandler* cgi_handler =
          static_cast<CgiResponseHandler*>(handler);

      int old_fd = poll_fd.fd;
      int new_fd = cgi_handler->get_client_fd();

      poll_fd.fd = new_fd;
      set_pollfd_out(poll_fd, new_fd);

      monitored_fd_to_handler_.erase(old_fd);
      monitored_fd_to_handler_.insert(std::make_pair(new_fd, handler));

      return kCgiReceived;
    }
    if (status == kHandlerFatalError) {
      return kHandlerFatalError;
    }
    if (status == kHandlerClosed) {
      return kHandlerClosed;
    }
    if (status == kHandlerAccepted || status == kHandlerContinue) {
      return kHandlerContinue;
    }
    if (status == kHandlerReceived) {
      set_pollfd_out(poll_fd, poll_fd.fd);
    }
  }
  HandlerStatus status = handler->handle_output();
  if (status == kCgiInputDone) {
    int fd = poll_fd.fd;
    poll_fds_.erase(poll_fds_.begin() + pollfd_index);
    monitored_fd_to_handler_.erase(fd);
    delete handler;
    return kCgiInputDone;
  }

  if (status == kHandlerFatalError) {
    return kHandlerFatalError;
  }
  if (status == kHandlerClosed) {
    return kHandlerClosed;
  }

  if (status == kHandlerSent) {
    return kHandlerClosed;  // TODO: implement keep-alive (optional)
  }

  return kHandlerContinue;
}

void Server::remove_client(int pollfd_index) {
  --num_clients_;
  int fd = poll_fds_[pollfd_index].fd;
  delete monitored_fd_to_handler_[fd];  // close(fd)
  monitored_fd_to_handler_.erase(fd);

  poll_fds_.erase(poll_fds_.begin() + pollfd_index);
}

// Returns 0 if success, otherwise -1
int Server::register_new_client(int client_fd, const std::string& addr,
                                const std::string& port) {
  if (num_clients_ >= kMaxClients) {
    std::cerr << "Number of clients exceeded the limit " << kMaxClients << "\n";
    return -1;
  }
  ++num_clients_;
  struct pollfd tmp;
  set_pollfd_in(tmp, client_fd);
  poll_fds_.push_back(tmp);
  MonitoredFdHandler* handler =
      new ClientHandler(client_fd, addr, port, *this, config_);
  monitored_fd_to_handler_.insert(std::make_pair(client_fd, handler));
  return 0;
}

void Server::register_cgi_fd(int pipe_in_fd, int pipe_out_fd, pid_t cgi_pid,
                             const std::string& body, int client_fd) {
  {
    struct pollfd tmp;
    set_pollfd_out(tmp, pipe_in_fd);
    poll_fds_.push_back(tmp);
    monitored_fd_to_handler_.insert(std::make_pair(
        pipe_in_fd, new CgiInputHandler(pipe_in_fd, cgi_pid, body)));
  }
  {
    struct pollfd tmp;
    set_pollfd_in(tmp, pipe_out_fd);
    poll_fds_.push_back(tmp);
    monitored_fd_to_handler_.insert(std::make_pair(
        pipe_out_fd, new CgiResponseHandler(pipe_out_fd, client_fd, cgi_pid)));
  }
}
