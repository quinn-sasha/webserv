#ifndef INCLUDE_CLIENTHANDLER_HPP_
#define INCLUDE_CLIENTHANDLER_HPP_

#include <sys/socket.h>
#include <cstddef>
#include <string>

#include "Config.hpp"
#include "MonitoredFdHandler.hpp"
#include "Parser.hpp"
#include "Response.hpp"

class Server;

class ClientHandler : public MonitoredFdHandler {
  int client_fd_;
  std::string addr_;
  std::string port_;
  Server& server_;
  const Config& config_;
  static const std::size_t buf_size = SO_RCVBUF;
  char buffer_[buf_size];
  Parser parser_;
  Response response_;
  std::size_t bytes_sent_;
  std::string response_str_;

  enum State {
    kReceiving,
    kExecutingCgi,
    kSendingResponse,
  };
  State state_;

  ClientHandler(const ClientHandler& other);
  ClientHandler& operator=(const ClientHandler& other);
  
 public:
  ClientHandler(int client_fd, const std::string& addr, const std::string& port,
                Server& server, Config& config);
  ~ClientHandler();
  void cgi_response_ready(const std::string& response);

  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error() { return kHandlerClosed; }
};

#endif  // INCLUDE_CLIENTHANDLER_HPP_