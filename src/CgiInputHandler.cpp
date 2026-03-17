#include "CgiInputHandler.hpp"
#include "Server.hpp"

#include <unistd.h>
#include <cstring>
#include <iostream>
#include <ctime>
#include <signal.h>
#include <sys/wait.h>

static int64_t now_time_cgi_in() {
  return static_cast<int64_t>(std::time(NULL));
}

CgiInputHandler::CgiInputHandler(int pipe_in_fd, pid_t cgi_pid,
                                 const std::string& body, Server& server,
                                 int client_fd)
    : pipe_in_fd_(pipe_in_fd),
      cgi_pid_(cgi_pid),
      body_(body),
      bytes_written_(0),
      client_fd_(client_fd),
      server_(server),
      start_sec_(now_time_cgi_in()),
      last_activity_sec_(start_sec_) {
  deadline_sec_ = start_sec_ + kCgiInputTimeoutSec;
}

CgiInputHandler::~CgiInputHandler() {
  close_in_fd_();
}

void CgiInputHandler::close_in_fd_() {
  if (pipe_in_fd_ != -1) {
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
  }
}

bool CgiInputHandler::has_deadline() const { return true; }
int64_t CgiInputHandler::deadline_sec() const { return deadline_sec_; }

void CgiInputHandler::update_deadline_() {
  last_activity_sec_ = now_time_cgi_in();
  deadline_sec_ = last_activity_sec_ + kCgiInputTimeoutSec;
  server_.update_timeout(pipe_in_fd_);
  // Keep the parent connection alive while CGI stdin is still flowing.
  if (client_fd_ >= 0) {
    server_.update_timeout(client_fd_);
  }
}

HandlerStatus CgiInputHandler::handle_timeout() {
  close_in_fd_();
  return kCgiInputDone;
}

HandlerStatus CgiInputHandler::handle_input() {
  return kHandlerContinue;
}

HandlerStatus CgiInputHandler::handle_output() {
  if (body_.empty() || bytes_written_ >= body_.size()) {
    close_in_fd_();
    return kCgiInputDone;
  }

  ssize_t n = write(pipe_in_fd_, body_.data() + bytes_written_,
                    body_.size() - bytes_written_);
  if (n == -1) {
    handle_poll_error();
  }

  if (n > 0) {
    bytes_written_ += static_cast<std::size_t>(n);
    update_deadline_();
  }

  if (bytes_written_ >= body_.size()) {
    close_in_fd_();
    return kCgiInputDone;
  }

  return kHandlerContinue;
}

HandlerStatus CgiInputHandler::handle_poll_error() {
  std::cerr << "Note: poll error on CGI pipe_in (CGI likely closed stdin)\n";
  close_in_fd_();
  return kCgiInputDone;
}
