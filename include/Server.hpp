#ifndef INCLUDE_SERVER_HPP_
#define INCLUDE_SERVER_HPP_

#include <map>
#include <string>
#include <vector>
#include <poll.h>

#include "Config.hpp"
#include "ListenSocket.hpp"
#include "MonitoredFdHandler.hpp"

class Server {
  static const std::size_t max_connections_ = 4096;
  std::vector<ListenSocket*> listen_sockets_;
  std::vector<struct pollfd> poll_fds_;
  std::map<int, MonitoredFdHandler*> monitored_fd_to_handler_;
  Config config_;

 public:
  Server(const std::string& config_file);
  ~Server();
  void run();
  HandlerStatus handle_fd_event(int pollfd_index);
  void register_cgi_fd(int pipe_in_fd, int pipe_out_fd,
                           pid_t cgi_pid, const std::string& body,
                           int client_fd);
  void register_new_client(int client_fd, const std::string& addr,
                           const std::string& port);
  void remove_client(int pollfd_index);
};

#endif  // INCLUDE_SERVER_HPP_