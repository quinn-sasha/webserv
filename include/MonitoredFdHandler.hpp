#ifndef INCLUDE_MONITOREDFDHANDLER_HPP_
#define INCLUDE_MONITOREDFDHANDLER_HPP_

enum HandlerStatus {
  kContinue,      // ClientHandler, AcceptHandler
  kAllSent,       // ClientHandler(temporal): sent all data in buffer
  kAccepted,      // AcceptHandler
  kReceived,      // ClientHandler (通常のHTTP)
  kCgiReceived,   // CgiResponseHandler (CGI専用) ✅ 追加
  kCgiInputDone,
  kClosed,        // ClientHandler
  kFatalError     // AcceptHandler, ClientHandler
};
// If we got kFatalError, Server should be stopped

class MonitoredFdHandler {
 public:
  virtual ~MonitoredFdHandler() {}
  virtual HandlerStatus handle_input() = 0;
  virtual HandlerStatus handle_output() = 0;
  virtual HandlerStatus handle_poll_error() = 0;
};

#endif  // INCLUDE_MONITOREDFDHANDLER_HPP_
