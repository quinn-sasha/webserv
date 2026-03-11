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
  if (num_read == -1 || num_read == 0) {
    return kHandlerClosed;
  }

  ParserStatus status = parser_.parse_request(buffer_, num_read);
  if (status == kParseContinue) {
    return kHandlerContinue;
  }

  refresh_current_request_();
  const ServerContext& target_config = set_up_target_config_();
  ProcessorResult result =
      RequestProcessor::process(status, current_request_, target_config);

  internal_redirect_count_ = 0;

  if (result.next_action == ProcessorResult::kExecuteCgi) {
    if (!do_cgi_(current_request_, result.script_path,
                result.cgi_path, result.query_string)) {
      return kHandlerReceived;
    }
    return kHandlerContinue;
  }

  state_ = kSendingResponse;
  response_ = result.response;
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

bool ClientHandler::do_cgi_(const Request& request,
                           const std::string& script_path,
                           const std::string& cgi_path,
                           const std::string& query_string) {
  state_ = kExecutingCgi;

  std::string server_name;
  std::string remote_addr;
  setup_cgi_(server_name, remote_addr);

  CgiHandler cgi(request, query_string, server_name, port_, remote_addr);
  if (cgi.execute_cgi(script_path, cgi_path) == -1) {
    send_error_response_(kInternalServerError);
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
    send_error_response_(kBadGateway);
    return;
  }

  ++internal_redirect_count_;
  refresh_current_request_();
  const ServerContext& target_config = set_up_target_config_();
  if (internal_redirect_count_ > kMaxInternalRedirects) {
    send_error_response_(kInternalServerError);
    return;
  }

  current_request_.target = location;

  ProcessorResult result =
      RequestProcessor::process(kParseFinished, current_request_, target_config);
  if (result.next_action == ProcessorResult::kExecuteCgi) {
    if (!do_cgi_(current_request_, result.script_path,
                result.cgi_path, result.query_string)) {
      return;
    }
    return;
  }

  response_ = result.response;
  send_prepared_response_();
}

void ClientHandler::send_prepared_response_() {
  response_str_ = response_.serialize();
  bytes_sent_ = 0;
  state_ = kSendingResponse;
  server_.set_fd_events(client_fd_, POLLOUT);
}

void ClientHandler::send_error_response_(ParserStatus status) {
  refresh_current_request_();
  const ServerContext& target_config = set_up_target_config_();
  response_.prepare_error_response(
      status,
      RequestProcessor::get_error_page_path(target_config, status));
  send_prepared_response_();
}

void ClientHandler::refresh_current_request_() {
  current_request_ = parser_.get_request();
}

const ServerContext& ClientHandler::set_up_target_config_() const {
  std::string host_name = "";
  std::map<std::string, std::string>::const_iterator it =
      current_request_.headers.find("host");
  if (it != current_request_.headers.end()) {
    host_name = it->second;
  }
  return config_.get_config(std::atoi(port_.c_str()), host_name);
}
