#ifndef INCLUDE_CGIINPUTHANDLER_HPP_
#define INCLUDE_CGIINPUTHANDLER_HPP_

#include <string>
#include <sys/types.h>

#include "MonitoredFdHandler.hpp"

class CgiInputHandler : public MonitoredFdHandler {
 public:
  CgiInputHandler(int pipe_in_fd, pid_t cgi_pid, const std::string& body);
  ~CgiInputHandler();

  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error();

 private:
  int         pipe_in_fd_;
  pid_t       cgi_pid_;
  std::string body_;
  std::size_t bytes_written_;

  CgiInputHandler(const CgiInputHandler&);
  CgiInputHandler& operator=(const CgiInputHandler&);
};

#endif  // INCLUDE_CGIINPUTHANDLER_HPP_