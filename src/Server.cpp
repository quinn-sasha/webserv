#include "Server.hpp"
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
  while (true) {
    int num_events = poll(poll_fds_.data(), poll_fds_.size(), -1);
    
    if (num_events == -1) {
      if (errno == EINTR) continue;
      throw std::runtime_error("poll() failed");
    }
    
    for (std::size_t i = 0; i < poll_fds_.size() && num_events > 0; ++i) {
      if (poll_fds_[i].revents == 0) continue;
      
      --num_events;
      HandlerStatus status = handle_fd_event(i);
      
      if (status == kClosed || status == kAllSent) {
        remove_client(i);
        --i;
      }
    }
  }
}

HandlerStatus Server::handle_fd_event(int pollfd_index) {
  struct pollfd& poll_fd = poll_fds_[pollfd_index];
  MonitoredFdHandler* handler = monitored_fd_to_handler_[poll_fd.fd];

  // POLLERRとPOLLNVALのみエラー扱い
  if (poll_fd.revents & (POLLERR | POLLNVAL)) {
    std::cerr << "POLLERR or POLLNVAL on fd " << poll_fd.fd << "\n";
    return handler->handle_poll_error();
  }
  
  // POLLINまたはPOLLHUP
  if (poll_fd.revents & (POLLIN | POLLHUP)) {
    HandlerStatus status = handler->handle_input();
    
    if (status == kFatalError) {
      return kFatalError;
    }
    if (status == kClosed) {
      return kClosed;
    }
    
    // ✅ 通常のHTTPレスポンス
    if (status == kReceived) {
      set_pollfd_out(poll_fd, poll_fd.fd);
      return kReceived;
    }
    
    // ✅ CGIレスポンス
    if (status == kCgiReceived) {
      // CgiResponseHandlerであることが保証されている
      CgiResponseHandler* cgi_handler = static_cast<CgiResponseHandler*>(handler);
      
      int old_fd = poll_fd.fd;          // cgi_fd
      int new_fd = cgi_handler->get_client_fd();  // client_fd
      
      // poll_fds_を更新
      poll_fd.fd = new_fd;
      set_pollfd_out(poll_fd, new_fd);
      
      // monitored_fd_to_handler_を更新
      monitored_fd_to_handler_.erase(old_fd);
      monitored_fd_to_handler_.insert(std::make_pair(new_fd, handler));
      
      return kCgiReceived;
    }
    
    if (status == kAccepted || status == kContinue) {
      return kContinue;
    }
  }
  
  // POLLOUT
  if (poll_fd.revents & POLLOUT) {
    HandlerStatus status = handler->handle_output();
    
    if (status == kFatalError) {
      return kFatalError;
    }
    if (status == kClosed) {
      return kClosed;
    }
    if (status == kAllSent) {
      return kAllSent;
    }
    if (status == kContinue) {
      return kContinue;
    }
  }
  
  return kContinue;
}

void Server::remove_client(int pollfd_index) {
  int fd = poll_fds_[pollfd_index].fd;
  delete monitored_fd_to_handler_[fd];
  monitored_fd_to_handler_.erase(fd);

  poll_fds_[pollfd_index] = poll_fds_.back();
  poll_fds_.pop_back();
}

void Server::register_new_client(int client_fd) {
  struct pollfd tmp;
  set_pollfd_in(tmp, client_fd);
  poll_fds_.push_back(tmp);
  MonitoredFdHandler* handler = new ClientHandler(client_fd, *this);
  monitored_fd_to_handler_.insert(std::make_pair(client_fd, handler));
}

void Server::register_cgi_response(int cgi_fd, int client_fd, pid_t cgi_pid) {
  struct pollfd tmp;
  set_pollfd_in(tmp, cgi_fd);
  poll_fds_.push_back(tmp);
  
  MonitoredFdHandler* handler = new CgiResponseHandler(cgi_fd, client_fd, cgi_pid);
  monitored_fd_to_handler_.insert(std::make_pair(cgi_fd, handler));
}
