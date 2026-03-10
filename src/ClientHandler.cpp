#include "ClientHandler.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>  // std::atoi
#include <cstring>
#include <iostream>

#include "CgiHandler.hpp"
#include "CgiInputHandler.hpp"
#include "CgiResponseHandler.hpp"
#include "Config.hpp"
#include "MonitoredFdHandler.hpp"
#include "Parser.hpp"
#include "RequestProcessor.hpp"
#include "Server.hpp"
#include "pollfd_utils.hpp"

ClientHandler::ClientHandler(int client_fd, const std::string& addr,
                             const std::string& port,
                             const std::string& client_addr, Server& server,
                             Config& config)
    : client_fd_(client_fd),
      addr_(addr),
      port_(port),
      client_addr_(client_addr),
      server_(server),
      config_(config),
      bytes_sent_(0),
      state_(kReceiving) {}

ClientHandler::~ClientHandler() {
  if (client_fd_ != -1 && close(client_fd_) == -1) {
    std::cerr << "Error: ~ClientHandler(): close() failed\n";
  }
}

HandlerStatus ClientHandler::handle_input() {
  if (state_ == kExecutingCgi) {
    return kHandlerContinue;
  }

  ssize_t num_read = recv(client_fd_, buffer_, buf_size, 0);
  if (num_read == -1) {
    return kHandlerClosed;
  }
  if (num_read == 0) {  // Connection closed
    return kHandlerClosed;
  }

  ParserStatus status = parser_.parse_request(buffer_, num_read);
  if (status == kParseContinue) {
    return kHandlerContinue;
  }

  const Request& req = parser_.get_request();
  std::string host_name = "";

  // map::find を使って、const 安全に値を探す
  std::map<std::string, std::string>::const_iterator it = req.headers.find("host");
  if (it != req.headers.end()) {
    host_name = it->second;
  }
  const ServerContext& target_config = config_.get_config(std::atoi(port_.c_str()), host_name);
  ProcessorResult result = RequestProcessor::process(status, req, target_config);

  internal_redirect_count_ = 0;

  if (result.next_action == ProcessorResult::kExecuteCgi) {
    if (!do_cgi(current_request_, result.script_path)) {
      return kHandlerReceived;
    }
    return kHandlerContinue;
  }

  // Normal response
  state_ = kSendingResponse;
  response_ = result.response;  // Maybe this copy is too heavy
  response_str_ = response_.serialize();
  bytes_sent_ = 0;
  return kHandlerReceived;
}

HandlerStatus ClientHandler::handle_output() {
  if (state_ == kExecutingCgi) {
    return kHandlerContinue;
  }
  if (state_ != kSendingResponse) {
    return kHandlerContinue;
  }

  ssize_t remaining = response_str_.size() - bytes_sent_;
  ssize_t num_sent =
      send(client_fd_, response_str_.data() + bytes_sent_, remaining, 0);

  if (num_sent == -1) {
    return kHandlerClosed;
  }

  bytes_sent_ += num_sent;

  if (bytes_sent_ < response_str_.size()) {
    return kHandlerContinue;
  }

  return kHandlerSent;
}

bool ClientHandler::do_cgi(const Request& request,
                                           const std::string& script_path) {
  state_ = kExecutingCgi;

  std::string server_name;
  std::string remote_addr;
  setup_cgi_(server_name, remote_addr);

  CgiHandler cgi(request, server_name, port_, remote_addr);
  if (cgi.execute_cgi(script_path) == -1) {
    const Request& req = parser_.get_request();
    std::string host_name = "";
    std::map<std::string, std::string>::const_iterator it = req.headers.find("host");
    if (it != req.headers.end()) {
    host_name = it->second;
    }
    const ServerContext& target_config = config_.get_config(std::atoi(port_.c_str()), host_name);
    response_.prepare_error_response(kInternalServerError, RequestProcessor::get_error_page_path(target_config, kInternalServerError));
    response_str_ = response_.serialize();
    state_ = kSendingResponse;
    server_.set_fd_events(client_fd_, POLLOUT);
    return false;
  }

  server_.set_fd_events(client_fd_, 0);

  server_.register_fd(cgi.get_pipe_in_fd(),
                      new CgiInputHandler(cgi.get_pipe_in_fd(),
                                          cgi.get_cgi_pid(),
                                          request.body),
                      POLLOUT);

  server_.register_fd(cgi.get_pipe_out_fd(),
                      new CgiResponseHandler(cgi.get_pipe_out_fd(),
                                             cgi.get_cgi_pid(),
                                             server_,
                                             client_fd_),
                      POLLIN);
  return true;
}

void ClientHandler::setup_cgi_(std::string& server_name,
                               std::string& remote_addr) const {
  server_name = "localhost";

  int listen_port = std::atoi(port_.c_str());
  const ServerContext& sc = config_.get_config(listen_port, "");
  for (std::size_t i = 0; i < sc.server_names.size(); ++i) {
    if (!sc.server_names[i].empty()) {
      server_name = sc.server_names[i];
      break;
    }
  }
  remote_addr = client_addr_;
}

void ClientHandler::cgi_response_ready(const std::string& response) {
  response_str_ = response;
  bytes_sent_ = 0;
  state_ = kSendingResponse;

  server_.set_fd_events(client_fd_, POLLOUT);
}

void ClientHandler::cgi_local_redirect_ready(const std::string& location) {
  if (location.empty() || location[0] != '/') {
    cgi_response_ready("HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n");
    return;
  }

  ++internal_redirect_count_;
      const Request& req = parser_.get_request();
      std::string host_name = "";
      std::map<std::string, std::string>::const_iterator it = req.headers.find("host");
      if (it != req.headers.end()) {
      host_name = it->second;
      }
      const ServerContext& target_config = config_.get_config(std::atoi(port_.c_str()), host_name);
  if (internal_redirect_count_ > kMaxInternalRedirects) {

    response_.prepare_error_response(kInternalServerError, RequestProcessor::get_error_page_path(target_config, kInternalServerError));
    response_str_ = response_.serialize();
    bytes_sent_ = 0;
    state_ = kSendingResponse;
    server_.set_fd_events(client_fd_, POLLOUT);
    return;
  }

  Request redirected = current_request_;
  redirected.target = location;
  current_request_ = redirected;

  ProcessorResult result = RequestProcessor::process(kParseFinished, current_request_, target_config);
  if (result.next_action == ProcessorResult::kExecuteCgi) {
    if (!do_cgi(current_request_, result.script_path)) {
      return;
    }
    return;
  }

  response_ = result.response;
  response_str_ = response_.serialize();
  bytes_sent_ = 0;
  state_ = kSendingResponse;
  server_.set_fd_events(client_fd_, POLLOUT);
}
