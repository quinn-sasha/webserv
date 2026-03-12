#ifndef INCLUDE_CGIRESPONSEHANDLER_HPP_
#define INCLUDE_CGIRESPONSEHANDLER_HPP_

#include "ClientHandler.hpp"
#include "MonitoredFdHandler.hpp"
#include <map>
#include <string>
#include <sys/types.h>
#include <stdint.h>

class Server;  
class ClientHandler;

class CgiResponseHandler : public MonitoredFdHandler {
 public:
   struct ParsedCgiOutput {
    bool is_local_redirect;
    bool is_valid;
    int status_code;
    std::string local_location;
    std::map<std::string, std::string> headers;
    std::string body;

    ParsedCgiOutput()
        : is_local_redirect(false),
          is_valid(false),
          status_code(-1),
          local_location(),
          headers(),
          body() {}
  };

  CgiResponseHandler(int out_fd, pid_t cgi_pid, ClientHandler* owner);
  CgiResponseHandler(int out_fd, pid_t cgi_pid, Server& server, int client_fd);
  ~CgiResponseHandler();

  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error();

  // timeout support
  virtual bool has_deadline() const;
  virtual int64_t deadline_ms() const;
  virtual HandlerStatus handle_timeout();

 private:
  static const int64_t kCgiTimeoutMs = 10000;   // 10s
  static const int64_t kCgiTimeoutSec = 10;   // 10s
  static const std::size_t kReadBufSize = 4096;
  static const std::size_t kMaxCgiHeaderBytes = 16 * 1024;
  static const std::size_t kMaxCgiOutputBytes = 8 * 1024 * 1024;
  
  CgiResponseHandler(const CgiResponseHandler&);
  CgiResponseHandler& operator=(const CgiResponseHandler&);
  void extend_deadline_on_activity_();
  static ParsedCgiOutput parse_cgi_output_(const std::string& cgi_output);
  HandlerStatus fail_with_bad_gateway_();
  int out_fd_;
  pid_t cgi_pid_;
  ClientHandler* owner_;
  Server& server_;
  int client_fd_;
  std::string cgi_output_;

  bool finished_;
  int64_t start_sec_;
  int64_t last_activity_sec_;
  int64_t deadline_sec_;

};


#endif  // INCLUDE_CGIRESPONSEHANDLER_HPP_
