#include "CgiResponseHandler.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <map>
#include <cctype>
#include <ctime>

#include "Server.hpp"
#include "ClientHandler.hpp"

static int64_t now_ms_cgi_out() {
  return static_cast<int64_t>(std::time(NULL)) * 1000;
}

CgiResponseHandler::CgiResponseHandler(int out_fd, pid_t cgi_pid, Server& server, int client_fd)
    : out_fd_(out_fd),
      cgi_pid_(cgi_pid),
      server_(server),
      client_fd_(client_fd),
      cgi_output_(),
      finished_(false),
      start_ms_(now_ms_cgi_out()),
      last_activity_ms_(start_ms_) {
  deadline_ms_ = start_ms_ + kCgiTimeoutMs;
}

CgiResponseHandler::~CgiResponseHandler() {
  if (out_fd_ != -1) {
    close(out_fd_);
    out_fd_ = -1;
  }

  if (cgi_pid_ > 0) {
    int status;
    waitpid(cgi_pid_, &status, WNOHANG);
    cgi_pid_ = -1;
  }
}

bool CgiResponseHandler::has_deadline() const { return true; }
int64_t CgiResponseHandler::deadline_ms() const { return deadline_ms_; }

void CgiResponseHandler::extend_deadline_on_activity_() {
  last_activity_ms_ = now_ms_cgi_out();
  deadline_ms_ = last_activity_ms_ + kCgiTimeoutMs;
}

std::string CgiResponseHandler::make_504_response_() {
  const char* body = "<h1>504 Gateway Timeout</h1>";
  std::ostringstream oss;
  oss << "HTTP/1.1 504 Gateway Timeout\r\n"
      << "Content-Type: text/html\r\n"
      << "Content-Length: " << std::strlen(body) << "\r\n"
      << "\r\n"
      << body;
  return oss.str();
}

std::string CgiResponseHandler::make_502_response_() {
  const char* body = "<h1>502 Bad Gateway</h1>";
  std::ostringstream oss;
  oss << "HTTP/1.1 502 Bad Gateway\r\n"
      << "Content-Type: text/html\r\n"
      << "Content-Length: " << std::strlen(body) << "\r\n"
      << "\r\n"
      << body;
  return oss.str();
}

static std::string key_to_lower_local(const std::string& str) {
  std::string lower = str;
  for (std::size_t i = 0; i < lower.size(); ++i) {
    lower[i] = std::tolower(static_cast<unsigned char>(lower[i]));
  }
  return lower;
}

std::string CgiResponseHandler::parse_cgi_output_(const std::string& cgi_output) {
  std::size_t body_start = cgi_output.find("\r\n\r\n");
  if (body_start == std::string::npos) {
    body_start = cgi_output.find("\n\n");
    if (body_start == std::string::npos) {
      return make_502_response_();
    }
    body_start += 2;
  } else {
    body_start += 4;
  }

  std::string headers_str = cgi_output.substr(0, body_start);
  std::string body = cgi_output.substr(body_start);

  std::map<std::string, std::string> headers_map;
  std::istringstream headers_stream(headers_str);
  std::string line;
  std::string status_line;

  while (std::getline(headers_stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }
    if (line.empty()) continue;

    std::size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) continue;

    std::string key = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);

    std::size_t value_start = value.find_first_not_of(" \t");
    if (value_start != std::string::npos) value = value.substr(value_start);
    else value = "";

    std::string lower_key = key_to_lower_local(key);
    if (lower_key == "status") status_line = value;
    else headers_map[lower_key] = value;
  }

  const bool has_content_type = headers_map.count("content-type") != 0;
  const bool has_location = headers_map.count("location") != 0;

  if (!has_content_type && !has_location) {
    std::cerr << "CGI Error: Missing both Content-Type and Location headers.\n";
    return make_502_response_();
  }

  if (status_line.empty()) {
    status_line = has_location ? "302 Found" : "200 OK";
  }

  std::ostringstream resp;
  resp << "HTTP/1.1 " << status_line << "\r\n";

  for (std::map<std::string, std::string>::iterator it = headers_map.begin();
       it != headers_map.end(); ++it) {
    std::string key = it->first;
    if (!key.empty()) {
      key[0] = std::toupper(static_cast<unsigned char>(key[0]));
      for (std::size_t i = 1; i < key.size(); ++i) {
        if (key[i - 1] == '-') key[i] = std::toupper(static_cast<unsigned char>(key[i]));
      }
    }
    resp << key << ": " << it->second << "\r\n";
  }

  if (headers_map.find("content-length") == headers_map.end() &&
      headers_map.find("transfer-encoding") == headers_map.end()) {
    resp << "Content-Length: " << body.size() << "\r\n";
  }

  resp << "\r\n" << body;
  return resp.str();
}

HandlerStatus CgiResponseHandler::handle_timeout() {
  if (finished_) return kCgiInputDone;
  finished_ = true;

  if (cgi_pid_ > 0) {
    kill(cgi_pid_, SIGKILL);
    int status;
    waitpid(cgi_pid_, &status, 0);
    cgi_pid_ = -1;
  }

  if (out_fd_ != -1) {
    close(out_fd_);
    out_fd_ = -1;
  }

  ClientHandler* ch = server_.find_client_handler(client_fd_);
  if (ch != NULL) {
    ch->cgi_response_ready(make_504_response_());
  }

  return kCgiInputDone;
}

HandlerStatus CgiResponseHandler::handle_input() {
  if (finished_) return kCgiInputDone;

  char buf[kReadBufSize];
  ssize_t n = read(out_fd_, buf, sizeof(buf));

  if (n == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return kHandlerContinue;
    return kHandlerClosed;
  }

  if (n == 0) {
    finished_ = true;

    bool cgi_error = false;
    if (cgi_pid_ > 0) {
      int status;
      pid_t wpid = waitpid(cgi_pid_, &status, 0);
      if (wpid == cgi_pid_) {
        if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) || WIFSIGNALED(status)) {
          cgi_error = true;
        }
      }
      cgi_pid_ = -1;
    }

    const std::string resp = cgi_error ? make_502_response_()
                                       : parse_cgi_output_(cgi_output_);

    ClientHandler* ch = server_.find_client_handler(client_fd_);
    if (ch != NULL) {
      ch->cgi_response_ready(resp);
    }

    return kCgiInputDone;
  }

  extend_deadline_on_activity_();
  cgi_output_.append(buf, n);
  return kHandlerContinue;
}

HandlerStatus CgiResponseHandler::handle_output() {
  return kHandlerContinue;
}

HandlerStatus CgiResponseHandler::handle_poll_error() {
  std::cerr << "Error : CgiResponseHandler's poll" << std::endl;
  return kHandlerClosed;
}