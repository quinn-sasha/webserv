#include "ClientHandler.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "Config.hpp"
#include "MonitoredFdHandler.hpp"
#include "Parser.hpp"
#include "RequestProcessor.hpp"
#include "CgiHandler.hpp"
#include "CgiInputHandler.hpp"
#include "CgiResponseHandler.hpp" 
#include "Server.hpp"
#include "pollfd_utils.hpp"


ClientHandler::ClientHandler(int client_fd, const std::string& addr,
                             const std::string& port, Server& server,
                             Config& config)
    : client_fd_(client_fd),
      addr_(addr),
      port_(port),
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

  // kParseFinished or some error status
  ProcesseorResult result = RequestProcessor::process(status, parser_.get_request());

  if (result.next_action == ProcesseorResult::kExecuteCgi) {
    state_ = kExecutingCgi;

    CgiHandler cgi(parser_.get_request());
    if (cgi.execute_cgi(result.script_path) == -1) {
      response_.prepare_error_response(kInternalServerError);
      response_str_ = response_.serialize();
      state_ = kSendingResponse;
      return kHandlerReceived;
    }

    // client_fd は保持したまま、一旦 poll 監視を止める（読みも書きもしない）
    server_.set_fd_events(client_fd_, 0);

    // pipe だけ別ハンドラで監視
    server_.register_fd(cgi.get_pipe_in_fd(),
                        new CgiInputHandler(cgi.get_pipe_in_fd(),
                                            cgi.get_cgi_pid(),
                                            parser_.get_request().body),
                        POLLOUT);

    server_.register_fd(cgi.get_pipe_out_fd(),
                        new CgiResponseHandler(cgi.get_pipe_out_fd(),
                                           cgi.get_cgi_pid(),
                                           server_,
                                           client_fd_),
                        POLLIN);

    return kHandlerContinue;
  }

  // Normal response
  state_ = kSendingResponse;
  response_ = result.response; // Maybe this copy is too heavy 
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

void ClientHandler::cgi_response_ready(const std::string& response) {
  response_str_ = response;
  bytes_sent_ = 0;
  state_ = kSendingResponse;

  server_.set_fd_events(client_fd_, POLLOUT);
}