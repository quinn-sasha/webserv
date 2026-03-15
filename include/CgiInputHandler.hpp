#ifndef INCLUDE_CGIINPUTHANDLER_HPP_
#define INCLUDE_CGIINPUTHANDLER_HPP_

#include <string>
#include <sys/types.h>
#include <stdint.h>

#include "MonitoredFdHandler.hpp"

class Server;

class CgiInputHandler : public MonitoredFdHandler {
 public:
  CgiInputHandler(int pipe_in_fd, pid_t cgi_pid, const std::string& body, Server& server);
  ~CgiInputHandler();

  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error();

  virtual bool has_deadline() const;
  virtual int64_t deadline_sec() const;
  virtual HandlerStatus handle_timeout();

 private:
  static const int64_t kCgiInputTimeoutSec = 10;  // 10s

  void update_deadline_();
  void close_in_fd_();

  int         pipe_in_fd_;
  pid_t       cgi_pid_;
  std::string body_;
  std::size_t bytes_written_;
  Server&     server_;

  // deadline state
  int64_t start_sec_;
  int64_t last_activity_sec_;
  int64_t deadline_sec_;

  CgiInputHandler(const CgiInputHandler&);
  CgiInputHandler& operator=(const CgiInputHandler&);
};

#endif  // INCLUDE_CGIINPUTHANDLER_HPP_
