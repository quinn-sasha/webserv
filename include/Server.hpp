#ifndef INCLUDE_SERVER_HPP_
#define INCLUDE_SERVER_HPP_

#include <poll.h>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Config.hpp"
#include "ListenSocket.hpp"
#include "MonitoredFdHandler.hpp"

#include "TimeoutManager.hpp"

class ClientHandler;

class Server {
  static const std::size_t kMaxClients = 4096;
  std::size_t num_clients_;
  std::vector<ListenSocket*> listen_sockets_;
  std::vector<struct pollfd> poll_fds_;
  std::map<int, MonitoredFdHandler*> monitored_fd_to_handler_;
  Config config_;
  TimeoutManager timeout_manager_;

  bool handle_timeouts_();
  
  // 指定したFDのインデックスを返す。見つからなければ -1
  int find_pollfd_index_(int fd) const;

 public:
  Server(const std::string& config_file);
  ~Server();
  void run();
  HandlerStatus handle_fd_event(int pollfd_index);

  int register_new_client(int client_fd, const std::string& addr,
                          const std::string& client_addr,
                          const std::string& port);

  void remove_client(int pollfd_index);
  void remove_fd(int pollfd_index);

  void register_fd(int fd, MonitoredFdHandler* handler, short events);
  void set_fd_events(int fd, short events);
  void update_timeout(int fd);

  ClientHandler* find_client_handler(int client_fd);
};

#endif  // INCLUDE_SERVER_HPP_
