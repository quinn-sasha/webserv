#include "Server.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <sys/poll.h>
#include <unistd.h>

#include <climits>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "AcceptHandler.hpp"
#include "CgiInputHandler.hpp"
#include "ClientHandler.hpp"
#include "ListenSocket.hpp"
#include "MonitoredFdHandler.hpp"
#include "SystemError.hpp"
#include "pollfd_utils.hpp"
#include "string_utils.hpp"

volatile sig_atomic_t g_running = true;

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
      timeout_manager_.add_timeout(listen_sock->fd(), handler);
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

  while (g_running) {
    if (poll_fds_.empty()) {
      throw std::runtime_error("Error: poll_fds_ must not be empty");
    }

    int timeout_ms = timeout_manager_.get_next_timeout_ms();

    int poll_ret = poll(&poll_fds_[0], poll_fds_.size(), timeout_ms);
    if (poll_ret == -1) {
      if (errno != EINTR) {
        throw SystemError("poll");
      }
      continue;
    }

    bool timeout_ok = handle_timeouts_();
    if (!timeout_ok) {
      break;
    }

    if (poll_ret == 0) {
      continue;
    }
    for (std::size_t i = 0; i < poll_fds_.size(); ++i) {
      if (poll_fds_[i].revents == 0) {
        continue;
      }

      HandlerStatus status = handle_fd_event(i);

      if (status == kHandlerContinue) {
        continue;
      }
      if (status == kHandlerFatalError) {
        std::cerr << "Fatal error occurred. Stopping server.\n";
        return;
      }
      if (status == kHandlerClosed || status == kCgiInputDone) {
        if (find_client_handler(poll_fds_[i].fd) != NULL) {
          remove_client(i);
        } else {
          remove_fd(i);
        }
        --i;
      }
    }
  }
}

HandlerStatus Server::handle_fd_event(int pollfd_index) {
  struct pollfd& poll_fd = poll_fds_[pollfd_index];

  timeout_manager_.update_timeout(poll_fd.fd);

  std::map<int, MonitoredFdHandler*>::iterator hit =
      monitored_fd_to_handler_.find(poll_fd.fd);
  if (hit == monitored_fd_to_handler_.end() || hit->second == NULL) {
    return kHandlerFatalError;
  }
  MonitoredFdHandler* handler = hit->second;

  if (poll_fd.revents & (POLLERR | POLLNVAL)) {
    return handler->handle_poll_error();
  }

  if (poll_fd.revents & (POLLIN | POLLHUP)) {
    HandlerStatus status = handler->handle_input();

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

void Server::remove_fd(int pollfd_index) {
  int fd = poll_fds_[pollfd_index].fd;
  timeout_manager_.remove_timeout(fd);
  delete monitored_fd_to_handler_[fd];
  monitored_fd_to_handler_.erase(fd);
  poll_fds_.erase(poll_fds_.begin() + pollfd_index);
}

void Server::remove_client(int pollfd_index) {
  if (num_clients_ > 0) {
    --num_clients_;
  }
  remove_fd(pollfd_index);
}

// Returns 0 if success, otherwise -1
int Server::register_new_client(int client_fd, const std::string& addr,
                                const std::string& client_addr,
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
      new ClientHandler(client_fd, addr, port, client_addr, *this, config_);
  monitored_fd_to_handler_.insert(std::make_pair(client_fd, handler));
  timeout_manager_.add_timeout(client_fd, handler);
  return 0;
}

void Server::register_fd(int fd, MonitoredFdHandler* handler, short events) {
  struct pollfd p;
  p.fd = fd;
  p.events = events;
  p.revents = 0;
  poll_fds_.push_back(p);
  monitored_fd_to_handler_.insert(std::make_pair(fd, handler));
  timeout_manager_.add_timeout(fd, handler);
}

void Server::set_fd_events(int fd, short events) {
  int idx = find_pollfd_index_(fd);
  if (idx != -1) {
    poll_fds_[idx].events = events;
  }
}

void Server::update_timeout(int fd) {
  timeout_manager_.update_timeout(fd);
}

int Server::find_pollfd_index_(int fd) const {
  for (std::size_t i = 0; i < poll_fds_.size(); ++i) {
    if (poll_fds_[i].fd == fd) 
      return static_cast<int>(i);
  }
  return -1;
}

ClientHandler* Server::find_client_handler(int client_fd) {
  std::map<int, MonitoredFdHandler*>::iterator it =
      monitored_fd_to_handler_.find(client_fd);
  if (it == monitored_fd_to_handler_.end() || it->second == NULL) {
    return NULL;
  }
  return dynamic_cast<ClientHandler*>(it->second);
}

bool Server::handle_timeouts_() {
  std::vector<int> timeout_fd = timeout_manager_.get_timedout_fds();

  for (std::size_t i = 0; i < timeout_fd.size(); ++i) {
    int fd = timeout_fd[i];
    
    std::map<int, MonitoredFdHandler*>::iterator it = monitored_fd_to_handler_.find(fd);
    if (it == monitored_fd_to_handler_.end()) continue;

    MonitoredFdHandler* handler = it->second;
    HandlerStatus status = handler->handle_timeout();

    if (status == kHandlerFatalError) {
      std::cerr << "Fatal error occurred. Stopping server.\n";
      return false;
    }

    int idx = find_pollfd_index_(fd);
    if (idx != -1) {
      if (find_client_handler(fd) != NULL) {
        remove_client(idx);
      } else {
        remove_fd(idx);
      }
    }
  }
  return true;
}
