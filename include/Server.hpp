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

class ClientHandler; 

class Server {
  static const std::size_t kMaxClients = 4096;
  std::size_t num_clients_;
  std::vector<ListenSocket*> listen_sockets_;
  std::vector<struct pollfd> poll_fds_;
  std::map<int, MonitoredFdHandler*> monitored_fd_to_handler_;
  Config config_;

 public:
  Server(const std::string& config_file);
  ~Server();
  void run();
  HandlerStatus handle_fd_event(int pollfd_index);

  int register_new_client(int client_fd, const std::string& addr,
                          const std::string& port);

  void remove_client(int pollfd_index);  
  void remove_fd(int pollfd_index);

  void register_fd(int fd, MonitoredFdHandler* handler, short events);
  void set_fd_events(int fd, short events);
  ClientHandler* find_client_handler(int client_fd); // cgi -> client_fd のチェック

};

#endif  // INCLUDE_SERVER_HPP_
