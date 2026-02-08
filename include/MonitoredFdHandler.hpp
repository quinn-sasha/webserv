#ifndef INCLUDE_MONITOREDFDHANDLER_HPP_
#define INCLUDE_MONITOREDFDHANDLER_HPP_

enum HandlerStatus {
  kContinue,  // AcceptHandler, ClientHandler
  kClosed,    // ClientHandler
  kFatal      // AcceptHandler, ClientHandler
};
// If we got kFatal, Server should be stopped

class MonitoredFdHandler {
 public:
  virtual ~MonitoredFdHandler() {}
  virtual HandlerStatus handle_input() = 0;
  virtual HandlerStatus handle_output() = 0;
};

#endif  // INCLUDE_MONITOREDFDHANDLER_HPP_
