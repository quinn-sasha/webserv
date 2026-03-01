#ifndef INCLUDE_MONITOREDFDHANDLER_HPP_
#define INCLUDE_MONITOREDFDHANDLER_HPP_

#include <stdint.h> 

enum HandlerStatus {
  kHandlerContinue,    // ClientHandler, AcceptHandler
  kHandlerReceived,    // ClientHandler
  kHandlerSent,        // ClientHandler
  kHandlerAccepted,    // AcceptHandler
  kHandlerClosed,      // ClientHandler
  kHandlerFatalError,  // AcceptHandler, ClientHandler
  kCgiReceived,        // cgi
  kCgiInputDone        // cgi
};

// If we got kFatalError, Server should be stopped
class MonitoredFdHandler {
 public:
  virtual ~MonitoredFdHandler() {}
  virtual HandlerStatus handle_input() = 0;
  virtual HandlerStatus handle_output() = 0;
  virtual HandlerStatus handle_poll_error() = 0;

  // timeout API (monotonic ms)
  virtual bool has_deadline() const { return false; }
  virtual int64_t deadline_ms() const { return 0; }
  virtual HandlerStatus handle_timeout() { return kHandlerClosed; }
};

#endif  // INCLUDE_MONITOREDFDHANDLER_HPP_