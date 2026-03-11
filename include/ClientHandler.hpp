#ifndef INCLUDE_CLIENTHANDLER_HPP_
#define INCLUDE_CLIENTHANDLER_HPP_

#include <sys/socket.h>

#include <cstddef>
#include <string>

#include "Config.hpp"
#include "MonitoredFdHandler.hpp"
#include "Parser.hpp"
#include "Response.hpp"

class Server;

class ClientHandler : public MonitoredFdHandler {
  int client_fd_;
  std::string addr_;
  std::string port_;
  std::string client_addr_;
  Server& server_;
  const Config& config_;
  static const std::size_t buf_size = SO_RCVBUF;
  char buffer_[buf_size];
  Parser parser_;
  Response response_;
  std::size_t bytes_sent_;
  std::string response_str_;
  Request current_request_;
  int internal_redirect_count_;
  static const int kMaxInternalRedirects = 5;

  enum State {
    kReceiving,
    kExecutingCgi,
    kSendingResponse,
  };
  State state_;

  ClientHandler(const ClientHandler& other);
  ClientHandler& operator=(const ClientHandler& other);

 public:
  ClientHandler(int client_fd, const std::string& addr, const std::string& port,
                const std::string& client_addr, Server& server, Config& config);
  ~ClientHandler();
  void cgi_response_ready(const std::string& response);
  void cgi_local_redirect_ready(const std::string& location);
  void setup_cgi_(std::string& server_name, std::string& remote_addr) const;
  HandlerStatus handle_input();
  HandlerStatus handle_output();
  HandlerStatus handle_poll_error() { return kHandlerClosed; }

  // private:
  void refresh_current_request_();
  const ServerContext& set_up_target_config_() const;
  bool do_cgi_(const Request& request, const std::string& script_path,
              const std::string& cgi_path,
              const std::string& query_string);
  void send_prepared_response_();
  void send_error_response_(ParserStatus status);


};

#endif  // INCLUDE_CLIENTHANDLER_HPP_
