#include "Server.hpp"
#include "CgiInputHandler.hpp"   
#include "CgiResponseHandler.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/poll.h>

#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <errno.h>
#include <iostream>

#include "AcceptHandler.hpp"
#include "ClientHandler.hpp"
#include "ListenSocket.hpp"
#include "MonitoredFdHandler.hpp"
#include "SystemError.hpp"
#include "pollfd_utils.hpp"

Server::Server(const std::vector<ListenConfig>& listen_configs,
               int maxpending) {
  for (std::size_t i = 0; i < listen_configs.size(); i++) {
    listen_sockets_.push_back(new ListenSocket(
        listen_configs[i].addr, listen_configs[i].port, maxpending));
    int listen_fd = listen_sockets_[i]->fd();
    struct pollfd tmp;
    set_pollfd_in(tmp, listen_fd);
    poll_fds_.push_back(tmp);
    MonitoredFdHandler* handler = new AcceptHandler(listen_fd, *this);
    monitored_fd_to_handler_.insert(std::make_pair(listen_fd, handler));
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
    throw SystemError("signal()")
;  }
  
  while (true) {
    if (poll_fds_.empty()) {
      throw std::runtime_error("Error: poll_fds_ must not be empty");
    }
    
    int num_events = poll(&poll_fds_[0], poll_fds_.size(), -1);
    
    if (num_events == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw SystemError("poll");
    }
    
    for (std::size_t i = 0; i < poll_fds_.size() && num_events > 0; ++i) {
      if (poll_fds_[i].revents == 0) {
        continue;
      }
      
      --num_events;
      
      HandlerStatus status = handle_fd_event(i);
      
      if (status == kHandlerContinue || status == kCgiReceived) {
        continue;
      }

      if (status == kCgiInputDone) {
        // handle_fd_event 側で既に erase されているので、
        // インデックスを調整して続行
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
      CgiResponseHandler* cgi_handler = static_cast<CgiResponseHandler*>(handler);
      
      int old_fd = poll_fd.fd;
      int new_fd = cgi_handler->get_client_fd();
      
      poll_fd.fd = new_fd;
      set_pollfd_out(poll_fd, new_fd);
      
      monitored_fd_to_handler_.erase(old_fd);
      monitored_fd_to_handler_.insert(std::make_pair(new_fd, handler));
      
      return kCgiReceived;
    }
    if (status != kHandlerContinue) {
      return status;
    }
  }
  
  if (poll_fd.revents & POLLOUT) {
    HandlerStatus status = handler->handle_output();
    
    if (status == kCgiInputDone) {
      int fd = poll_fd.fd;
      poll_fds_.erase(poll_fds_.begin() + pollfd_index);
      monitored_fd_to_handler_.erase(fd);
      delete handler;
      return kCgiInputDone;
    }
    return status;
  }
  
  return kHandlerContinue;
}

void Server::remove_client(int pollfd_index) {
  int fd = poll_fds_[pollfd_index].fd;
  delete monitored_fd_to_handler_[fd];
  monitored_fd_to_handler_.erase(fd);

  poll_fds_.erase(poll_fds_.begin() + pollfd_index);
}

void Server::register_new_client(int client_fd) {
  struct pollfd tmp;
  set_pollfd_in(tmp, client_fd);
  poll_fds_.push_back(tmp);
  MonitoredFdHandler* handler = new ClientHandler(client_fd, *this);
  monitored_fd_to_handler_.insert(std::make_pair(client_fd, handler));
}

void Server::register_cgi_fd(int pipe_in_fd, int pipe_out_fd,
                                 pid_t cgi_pid, const std::string& body,
                                 int client_fd) {
  {
    struct pollfd tmp;
    set_pollfd_out(tmp, pipe_in_fd);
    poll_fds_.push_back(tmp);
    monitored_fd_to_handler_.insert(std::make_pair(pipe_in_fd,
        new CgiInputHandler(pipe_in_fd, cgi_pid, body)));
  }
  {
    struct pollfd tmp;
    set_pollfd_in(tmp, pipe_out_fd);
    poll_fds_.push_back(tmp);
    monitored_fd_to_handler_.insert(std::make_pair(pipe_out_fd,
        new CgiResponseHandler(pipe_out_fd, client_fd, cgi_pid)));
  }
}
