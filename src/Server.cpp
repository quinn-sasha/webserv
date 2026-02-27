#include "Server.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/poll.h>

#include <cstddef>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "AcceptHandler.hpp"
#include "ClientHandler.hpp"
#include "ListenSocket.hpp"
#include "MonitoredFdHandler.hpp"
#include "SystemError.hpp"
#include "pollfd_utils.hpp"
#include "string_utils.hpp"

Server::Server(const std::string& config_file) {
  config_.load_file(config_file);
  for (std::size_t i = 0; i < config_.get_configs().size(); i++) {
    const ServerContext& sc = config_.get_configs()[i];
    for (std::size_t j = 0; j < sc.listens.size(); j++) {
      std::string addr = sc.listens[j].address;
      std::string port = int_to_string(sc.listens[j].port);
      ListenSocket* listen_sock =
          new ListenSocket(addr, port, max_connections_);
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
    delete iter->second;  // Close every fd except listen socket
  }
}

// accept(), send(), recv() という異なるシステムコールに対して
// ソケットを監視しなければいけない.
// 監視するソケットの種類はListen socket, accepted socket.
void Server::run() {
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    throw SystemError("signal()");
  }
  while (true) {
    if (poll_fds_.empty()) {
      throw std::runtime_error("Error: poll_fds_ must not be empty");
    }
    // CAUTION: temporary block infinitely (temporal)
    if (poll(&poll_fds_.front(), poll_fds_.size(), -1) == -1) {
      throw SystemError("poll()");
    }
    for (std::size_t i = 0; i < poll_fds_.size(); i++) {
      HandlerStatus status = handle_fd_event(i);
      if (status == kHandlerContinue) {
        continue;
      }
      if (status == kHandlerFatalError) {
        throw std::runtime_error("Fatal error on monitored fd");
      }
      if (status == kHandlerClosed) {
        remove_client(i);
        i--;
      }
    }
  }
}

// Returns kFatalError, kClosed or kContinue
HandlerStatus Server::handle_fd_event(int pollfd_index) {
  struct pollfd& poll_fd = poll_fds_[pollfd_index];
  MonitoredFdHandler* handler = monitored_fd_to_handler_[poll_fd.fd];

  if (poll_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    return handler->handle_poll_error();
  }
  if (!(poll_fd.revents & POLLIN) && !(poll_fd.revents & POLLOUT)) {
    return kHandlerContinue;
  }
  if (poll_fd.revents & POLLIN) {
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
  int fd = poll_fds_[pollfd_index].fd;
  delete monitored_fd_to_handler_[fd];  // close(fd)
  monitored_fd_to_handler_.erase(fd);

  poll_fds_[pollfd_index] = poll_fds_.back();
  poll_fds_.pop_back();
}

void Server::register_new_client(int client_fd, const std::string& addr,
                                 const std::string& port) {
  struct pollfd tmp;
  set_pollfd_in(tmp, client_fd);
  poll_fds_.push_back(tmp);
  MonitoredFdHandler* handler =
      new ClientHandler(client_fd, addr, port, *this, config_);
  monitored_fd_to_handler_.insert(std::make_pair(client_fd, handler));
}
