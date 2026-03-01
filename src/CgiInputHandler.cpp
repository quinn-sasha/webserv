#include "CgiInputHandler.hpp"

#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <ctime>
#include <signal.h>
#include <sys/wait.h>

static std::int64_t now_ms_cgi_in() {
  return static_cast<std::int64_t>(std::time(NULL)) * 1000;
}

CgiInputHandler::CgiInputHandler(int pipe_in_fd, pid_t cgi_pid,
                                 const std::string& body)
    : pipe_in_fd_(pipe_in_fd),
      cgi_pid_(cgi_pid),
      body_(body),
      bytes_written_(0),
      start_ms_(now_ms_cgi_in()),
      last_activity_ms_(start_ms_) {
  deadline_ms_ = start_ms_ + kCgiInputTimeoutMs;
}

CgiInputHandler::~CgiInputHandler() {
  if (pipe_in_fd_ != -1) {
    close(pipe_in_fd_);
  }
}

bool CgiInputHandler::has_deadline() const { return true; }
std::int64_t CgiInputHandler::deadline_ms() const { return deadline_ms_; }

void CgiInputHandler::extend_deadline_on_activity_() {
  last_activity_ms_ = now_ms_cgi_in();
  deadline_ms_ = last_activity_ms_ + kCgiInputTimeoutMs;
}

HandlerStatus CgiInputHandler::handle_timeout() {
  if (pipe_in_fd_ != -1) {
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
  }
  return kCgiInputDone;
}

HandlerStatus CgiInputHandler::handle_input() {
  return kHandlerContinue;
}

HandlerStatus CgiInputHandler::handle_output() {
  // ボディが空 or 書き込み完了
  if (body_.empty() || bytes_written_ >= body_.size()) {
    if (pipe_in_fd_ != -1) {
      close(pipe_in_fd_);
      pipe_in_fd_ = -1;
    }
    return kCgiInputDone;
  }

  ssize_t n = write(pipe_in_fd_, body_.data() + bytes_written_,
                    body_.size() - bytes_written_);
  if (n == -1) {
    // EPIPE (Broken Pipe) 等は Fatal にせず単に閉じる
    std::cerr << "Note: write() to CGI failed (CGI likely closed stdin): "
              << strerror(errno) << "\n";
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
    return kCgiInputDone;
  }

  if (n > 0) {
    bytes_written_ += static_cast<std::size_t>(n);
    extend_deadline_on_activity_();
  }

  if (bytes_written_ >= body_.size()) {
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
    return kCgiInputDone;
  }

  return kHandlerContinue;
}

HandlerStatus CgiInputHandler::handle_poll_error() {
  std::cerr << "Note: poll error on CGI pipe_in (CGI likely closed stdin)\n";
  if (pipe_in_fd_ != -1) {
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
  }
  return kCgiInputDone;
}