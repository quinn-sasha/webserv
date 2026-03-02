#ifndef INCLUDE_CGIRESPONSEHANDLER_HPP_
#define INCLUDE_CGIRESPONSEHANDLER_HPP_

#include "ClientHandler.hpp"
#include "MonitoredFdHandler.hpp"
#include <string>
#include <sys/types.h>
#include <stdint.h>

class Server;  
class ClientHandler;

class CgiResponseHandler : public MonitoredFdHandler {
 public:
  CgiResponseHandler(int out_fd, pid_t cgi_pid, ClientHandler* owner);
  CgiResponseHandler(int out_fd, pid_t cgi_pid, Server& server, int client_fd);
  ~CgiResponseHandler();

  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_timeout();
  HandlerStatus handle_poll_error();

  // timeout support
  bool has_deadline() const;
  int64_t deadline_ms() const;

 private:
  static const int64_t kCgiTimeoutMs = 30000;   // 30s
  static const std::size_t kReadBufSize = 4096;

  void extend_deadline_on_activity_();

  static std::string make_504_response_();
  static std::string make_502_response_();
  static std::string parse_cgi_output_(const std::string& cgi_output);

  int out_fd_;
  pid_t cgi_pid_;
  ClientHandler* owner_;
  Server& server_;
  int client_fd_;
  std::string cgi_output_;

  bool finished_;
  int64_t start_ms_;
  int64_t last_activity_ms_;
  int64_t deadline_ms_;

  CgiResponseHandler(const CgiResponseHandler&);
  CgiResponseHandler& operator=(const CgiResponseHandler&);
};

#endif  // INCLUDE_CGIRESPONSEHANDLER_HPP_
