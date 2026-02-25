#include "ClientHandler.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "MonitoredFdHandler.hpp"
#include "Parser.hpp"
#include "RequestProcessor.hpp"

ClientHandler::ClientHandler(int client_fd, Server& server)
    : client_fd_(client_fd),
      server_(server),
      bytes_sent_(0),
      state_(kReceiving) {}

ClientHandler::~ClientHandler() {
  if (close(client_fd_) == -1) {
    std::cerr << "Error: ~ClientHandler(): close()\n";
  }
}

HandlerStatus ClientHandler::handle_input() {
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
  ProcesseorResult result =
      RequestProcessor::process(status /*, parser_.get_request() */);
  if (result.next_action == ProcesseorResult::kExecuteCgi) {
    state_ = kExecutingCgi;
    // set up CGI
    // return
  }
  state_ = kSendingResponse;
  response_ = result.response;  // Maybe this copy is too heavy
  response_str_ = response_.serialize();
  return kHandlerReceived;
}

HandlerStatus ClientHandler::handle_output() {
  if (state_ == kExecutingCgi) {
    return kHandlerContinue;
  }
  if (state_ != kSendingResponse) {
    std::cerr << "handle_output() was called even if data isn't ready\n";
    return kHandlerClosed;
  }
  ssize_t remaining = response_str_.size() - bytes_sent_;
  ssize_t num_sent =
      send(client_fd_, response_str_.c_str() + bytes_sent_, remaining, 0);
  if (num_sent == -1) {
    return kHandlerClosed;
  }
  bytes_sent_ += num_sent;
  if (bytes_sent_ < response_str_.size()) {
    return kHandlerContinue;
  }
  return kHandlerSent;
}
