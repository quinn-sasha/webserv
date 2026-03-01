#ifndef INCLUDE_CGIRESPONSEHANDLER_HPP_
#define INCLUDE_CGIRESPONSEHANDLER_HPP_

#include <sys/types.h>
#include <stdint.h>
#include <string>
#include "MonitoredFdHandler.hpp"

class CgiResponseHandler : public MonitoredFdHandler {
 public:
  CgiResponseHandler(int cgi_fd, int client_fd, pid_t cgi_pid);
  ~CgiResponseHandler();

  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error();

  // timeout
  bool has_deadline() const;
  int64_t deadline_ms() const;
  HandlerStatus handle_timeout();

  int get_client_fd() const { return client_fd_; }

  std::string parse_cgi_output(const std::string& cgi_output);

 private:
  static const int64_t kCgiTimeoutMs = 30000;   // 30s
  static const std::size_t  kReadBufSize  = 4096;

  void extend_deadline_on_activity_();

  int cgi_fd_;
  int client_fd_;
  pid_t cgi_pid_;

  std::string cgi_output_;
  std::string send_buffer_;
  ssize_t bytes_sent_;
  bool cgi_finished_;

  int64_t start_ms_;
  int64_t last_activity_ms_;
  int64_t deadline_ms_;
};

#endif  // INCLUDE_CGIRESPONSEHANDLER_HPP_
