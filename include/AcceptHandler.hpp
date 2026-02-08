#ifndef INCLUDE_ACCEPTHANDLER_HPP_
#define INCLUDE_ACCEPTHANDLER_HPP_

#include "MonitoredFdHandler.hpp"

class Server;

class AcceptHandler : public MonitoredFdHandler {
  int listen_fd_;
  Server& server_;

  AcceptHandler(const AcceptHandler&);
  AcceptHandler& operator=(const AcceptHandler&);

 public:
  AcceptHandler(int listen_fd, Server& server);
  ~AcceptHandler() {}  // Do nothing because ListenSocket will close fd
  HandlerStatus handle_input();  // Return kContinue or kFatal
  HandlerStatus handle_output() { return kFatal; }  // Must not call this
};

#endif  // INCLUDE_ACCEPTHANDLER_HPP_
