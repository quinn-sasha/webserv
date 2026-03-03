#ifndef INCLUDE_CGIINPUTHANDLER_HPP_
#define INCLUDE_CGIINPUTHANDLER_HPP_

#include <string>
#include <sys/types.h>
#include <stdint.h>

#include "MonitoredFdHandler.hpp"

class CgiInputHandler : public MonitoredFdHandler {
 public:
  CgiInputHandler(int pipe_in_fd, pid_t cgi_pid, const std::string& body);
  ~CgiInputHandler();

  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error();

  // timeout support
  virtual bool has_deadline() const;
  virtual int64_t deadline_ms() const;
  virtual HandlerStatus handle_timeout();

 private:
  static const int64_t kCgiInputTimeoutMs = 10000;  // 10s

  void extend_deadline_on_activity_();

  int         pipe_in_fd_;
  pid_t       cgi_pid_;
  std::string body_;
  std::size_t bytes_written_;

  // deadline state
  int64_t start_ms_;
  int64_t last_activity_ms_;
  int64_t deadline_ms_;

  CgiInputHandler(const CgiInputHandler&);
  CgiInputHandler& operator=(const CgiInputHandler&);
};

#endif  // INCLUDE_CGIINPUTHANDLER_HPP_