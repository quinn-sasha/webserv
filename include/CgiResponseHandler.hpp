#ifndef INCLUDE_CGIRESPONSEHANDLER_HPP_
#define INCLUDE_CGIRESPONSEHANDLER_HPP_

#include <sys/types.h>
#include <string>
#include "MonitoredFdHandler.hpp"

class CgiResponseHandler : public MonitoredFdHandler {
 public:
  CgiResponseHandler(int cgi_fd, int client_fd, pid_t cgi_pid);
  ~CgiResponseHandler();
  
  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error();
  
  int get_client_fd() const { return client_fd_; }

 private:
  int cgi_fd_;
  int client_fd_;
  pid_t cgi_pid_;
  
  std::string cgi_output_;
  std::string send_buffer_;
  ssize_t bytes_sent_;
  bool cgi_finished_;
  
  std::string parse_cgi_output(const std::string& cgi_output);
};

#endif  // INCLUDE_CGIRESPONSEHANDLER_HPP_