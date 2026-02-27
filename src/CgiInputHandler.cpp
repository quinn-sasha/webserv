#include "CgiInputHandler.hpp"

#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

CgiInputHandler::CgiInputHandler(int pipe_in_fd, pid_t cgi_pid,
                                  const std::string& body)
    : pipe_in_fd_(pipe_in_fd),
      cgi_pid_(cgi_pid),
      body_(body),
      bytes_written_(0) {}

CgiInputHandler::~CgiInputHandler() {
  if (pipe_in_fd_ != -1) {
    close(pipe_in_fd_);
  }
}

HandlerStatus CgiInputHandler::handle_input() {
  return kHandlerContinue;
}

HandlerStatus CgiInputHandler::handle_output() {
  // ボディが空 or 書き込み完了
  if (body_.empty() || bytes_written_ >= body_.size()) {
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
    return kCgiInputDone;
  }

  ssize_t n = write(pipe_in_fd_,
                    body_.data() + bytes_written_,
                    body_.size() - bytes_written_);
  if (n == -1) {
    // EPIPE (Broken Pipe) 等は Fatal にせず単に閉じる
    std::cerr << "Note: write() to CGI failed (CGI likely closed stdin): " << strerror(errno) << "\n";
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
    return kCgiInputDone;
  }

  bytes_written_ += static_cast<std::size_t>(n);

  if (bytes_written_ >= body_.size()) {
    close(pipe_in_fd_);
    pipe_in_fd_ = -1;
    return kCgiInputDone;
  }

  return kHandlerContinue;
}

HandlerStatus CgiInputHandler::handle_poll_error() {
  // pollエラー時もFatalにせず、入力を諦めて終了させる
  std::cerr << "Note: poll error on CGI pipe_in (CGI likely closed stdin)\n";
  close(pipe_in_fd_);
  pipe_in_fd_ = -1;
  return kCgiInputDone;
}