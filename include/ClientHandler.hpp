#ifndef INCLUDE_CLIENTHANDLER_HPP_
#define INCLUDE_CLIENTHANDLER_HPP_

#include <sys/socket.h>
#include <cstddef>
#include <string>

#include "MonitoredFdHandler.hpp"
#include "Parser.hpp"
#include "Response.hpp"

class Server;

class ClientHandler : public MonitoredFdHandler {
  int client_fd_;
  Server& server_;
  static const std::size_t buf_size = 4096; // Adjust if needed
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
  ClientHandler(int client_fd, Server& server);
  ~ClientHandler();
  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error() { return kHandlerClosed; }
};

#endif  // INCLUDE_CLIENTHANDLER_HPP_