#include "CgiResponseHandler.hpp"
#include "Response.hpp"

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
#include "string_utils.hpp"

static int64_t now_time_cgi_out() {
  return static_cast<int64_t>(std::time(NULL)); // 秒
}

CgiResponseHandler::CgiResponseHandler(int out_fd, pid_t cgi_pid, Server& server, int client_fd)
    : out_fd_(out_fd),
      cgi_pid_(cgi_pid),
      server_(server),
      client_fd_(client_fd),
      cgi_output_(),
      finished_(false),
      start_sec_(now_time_cgi_out()),
      last_activity_sec_(start_sec_) {
  deadline_sec_ = start_sec_ + kCgiTimeoutSec;
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
int64_t CgiResponseHandler::deadline_ms() const { return deadline_sec_; }

void CgiResponseHandler::extend_deadline_on_activity_() {
  last_activity_sec_ = now_time_cgi_out();
  deadline_sec_ = last_activity_sec_ + kCgiTimeoutSec;
}

static bool parse_status_code(const std::string& status_line, int& status_code) {
  if (status_line.size() < 3) {
    return false;
  }

  std::string code_str = status_line.substr(0, 3);
  if (convert_to_integer(status_code, code_str, 10) == -1) {
    return false;
  }

  return status_code >= 200 && status_code <= 599;
}

static bool iequal_key(const std::string& lhs, const std::string& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
        std::tolower(static_cast<unsigned char>(rhs[i]))) {
      return false;
    }
  }
  return true;
}

static std::string first_header_value(
    const std::vector<std::pair<std::string, std::string> >& headers,
    const std::string& key) {
  for (std::size_t i = 0; i < headers.size(); ++i) {
    if (iequal_key(headers[i].first, key)) {
      return headers[i].second;
    }
  }
  return "";
}

static bool has_header_pair(
    const std::vector<std::pair<std::string, std::string> >& headers,
    const std::string& key) {
  for (std::size_t i = 0; i < headers.size(); ++i) {
    if (iequal_key(headers[i].first, key)) {
      return true;
    }
  }
  return false;
}

static Response build_response_from_parsed(
    const CgiResponseHandler::ParsedCgiOutput& parsed) {
  Response response;      
  if (!parsed.is_valid) {
    response.prepare_error_response(kBadGateway, "");
    return response;
  }

  response.set_status_code(parsed.status_code);

  for (std::size_t i = 0; i < parsed.headers.size(); ++i) {
    response.add_header(parsed.headers[i].first, parsed.headers[i].second);
  }

  response.set_body(parsed.body);

  if (!has_header_pair(parsed.headers, "content-length") &&
      !has_header_pair(parsed.headers, "transfer-encoding")) {
    response.ensure_content_length();
  }

  return response;
}

CgiResponseHandler::ParsedCgiOutput
CgiResponseHandler::parse_cgi_output_(const std::string& cgi_output) {
  ParsedCgiOutput result;

  std::size_t body_start = cgi_output.find("\r\n\r\n");
  if (body_start == std::string::npos) {
    body_start = cgi_output.find("\n\n");
    if (body_start == std::string::npos) {
      return result;
    }
    body_start += 2;
  } else {
    body_start += 4;
  }

  std::string headers_str = cgi_output.substr(0, body_start);
  result.body = cgi_output.substr(body_start);

  std::istringstream headers_stream(headers_str);
  std::string line;
  std::string status_line;

  while (std::getline(headers_stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }
    if (line.empty()) {
      continue;
    }

    std::size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, colon_pos);
    std::string value = trim(line.substr(colon_pos + 1), " \t");

    if (iequal_key(key, "Status")) {
      status_line = value;
    } else {
      result.headers.push_back(std::make_pair(key, value));
    }
  }

  const bool has_content_type = has_header_pair(result.headers, "content-type");
  const bool has_location = has_header_pair(result.headers, "location");

  if (!has_content_type && !has_location) {
    return result;
  }

  if (has_location && status_line.empty()) {
    const std::string loc = first_header_value(result.headers, "location");
    if (!loc.empty() && loc[0] == '/') {
      result.is_local_redirect = true;
      result.local_location = loc;
      result.is_valid = true;
      return result;
    }
  }

  if (status_line.empty()) {
    result.status_code = has_location ? 302 : 200;
  } else {
    if (!parse_status_code(status_line, result.status_code)) {
      return ParsedCgiOutput();
    }
  }

  result.is_valid = true;
  return result;
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
    Response response;
    response.prepare_error_response(kGatewayTimeout, "");
    ch->cgi_response_ready(response.serialize());
  }

  return kCgiInputDone;
}

HandlerStatus CgiResponseHandler::handle_input() {
  if (finished_) return kCgiInputDone;

  char buf[kReadBufSize];
  ssize_t n = read(out_fd_, buf, sizeof(buf));

  if (n == -1) {
    return handle_poll_error();
  }

  if (n == 0) {
    finished_ = true;

    bool cgi_error = false;
    if (cgi_pid_ > 0) {
      int status;
      pid_t wpid = waitpid(cgi_pid_, &status, 0);
      if (wpid == cgi_pid_) {
        if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
            WIFSIGNALED(status)) {
          cgi_error = true;
        }
      }
      cgi_pid_ = -1;
    }

    const ParsedCgiOutput parsed = parse_cgi_output_(cgi_output_);
    ClientHandler* ch = server_.find_client_handler(client_fd_);
    if (ch != NULL) {
      if (parsed.is_local_redirect) {
        ch->cgi_local_redirect_ready(parsed.local_location);
      } else {
        Response response;
        if (cgi_error) {
          response.prepare_error_response(kBadGateway, "");
        } else {
          response = build_response_from_parsed(parsed);
        }
        ch->cgi_response_ready(response.serialize());
      }
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
    Response response;
    response.prepare_error_response(kBadGateway, "");
    ch->cgi_response_ready(response.serialize());
  }

  return kCgiInputDone;
}
