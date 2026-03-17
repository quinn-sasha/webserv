#include "CgiResponseHandler.hpp"
#include "Response.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <map>
#include <cctype>
#include <ctime>

#include "Server.hpp"
#include "ClientHandler.hpp"
#include "string_utils.hpp"

namespace cgi {
static int64_t now_time_cgi_out() {
  return static_cast<int64_t>(std::time(NULL));
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

static std::string normalize_header_key(const std::string& key) {
  std::string out;
  out.reserve(key.size());
  for (std::size_t i = 0; i < key.size(); ++i) {
    out.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(key[i]))));
  }
  return out;
}

static bool has_header_terminator(const std::string& data) {
  return data.find("\r\n\r\n") != std::string::npos ||
         data.find("\n\n") != std::string::npos;
}

static bool is_valid_header_name(const std::string& key) {
  if (key.empty()) {
    return false;
  }
  for (std::size_t i = 0; i < key.size(); ++i) {
    unsigned char ch = static_cast<unsigned char>(key[i]);
    if (ch <= 31 || ch == 127 || ch == ' ' || ch == '\t' || ch == ':') {
      return false;
    }
  }
  return true;
}

static Response build_response_from_parsed(
    const CgiResponseHandler::ParsedCgiOutput& parsed) {
  Response response;
  if (!parsed.is_valid) {
    response.prepare_error_response(kBadGateway, "");
    return response;
  }

  response.set_status_code(parsed.status_code);

  for (std::map<std::string, std::string>::const_iterator it =
           parsed.headers.begin();
       it != parsed.headers.end(); ++it) {
    response.add_header(it->first, it->second);
  }

  response.set_body(parsed.body);

  if (parsed.headers.find("content-length") == parsed.headers.end() &&
      parsed.headers.find("transfer-encoding") == parsed.headers.end()) {
    response.ensure_content_length();
  }

  return response;
}

static bool parse_header_line(const std::string& line, bool& seen_status,
                              std::string& status_line,
                              std::map<std::string, std::string>& headers) {
  std::size_t colon_pos = line.find(':');
  if (line[0] == ' ' || line[0] == '\t' ||
      colon_pos == std::string::npos) {
    return false;
  }

  std::string key = line.substr(0, colon_pos);
  if (!cgi::is_valid_header_name(key)) {
    return false;
  }

  std::string normalized_key = normalize_header_key(key);
  std::string value = trim(line.substr(colon_pos + 1), " \t");

  if (normalized_key == "status") {
    if (seen_status || value.empty()) {
      return false;
    }
    seen_status = true;
    status_line = value;
    return true;
  }

  headers[normalized_key] = value;
  return true;
}

static bool apply_content_type_location_rules(
    CgiResponseHandler::ParsedCgiOutput& result,
    const std::string& status_line) {
  const std::map<std::string, std::string>::const_iterator content_type_it =
      result.headers.find("content-type");
  const std::map<std::string, std::string>::const_iterator location_it =
      result.headers.find("location");

  const bool has_content_type = content_type_it != result.headers.end();
  const bool has_location = location_it != result.headers.end();

  if (!has_content_type && !has_location) {
    return false; // not valid
  }

  if (has_location && status_line.empty()) {
    const std::string& loc = location_it->second;
    if (!loc.empty() && loc[0] == '/') {
      result.is_local_redirect = true;
      result.local_location = loc;
      result.is_valid = true;
      return true;
    }
  }

  return true;
}
} //namespace

bool CgiResponseHandler::has_deadline() const { return true; }
int64_t CgiResponseHandler::deadline_sec() const { return deadline_sec_; }

void CgiResponseHandler::update_deadline_() {
  last_activity_sec_ = cgi::now_time_cgi_out();
  deadline_sec_ = last_activity_sec_ + kCgiTimeoutSec;
  server_.update_timeout(out_fd_);
  // Keep the parent connection alive while CGI stdout is still flowing.
  if (client_fd_ >= 0) {
    server_.update_timeout(client_fd_);
  }
}

CgiResponseHandler::CgiResponseHandler(int out_fd, pid_t cgi_pid, Server& server, int client_fd)
    : out_fd_(out_fd),
      cgi_pid_(cgi_pid),
      server_(server),
      client_fd_(client_fd),
      cgi_output_(),
      finished_(false),
      start_sec_(cgi::now_time_cgi_out()),
      last_activity_sec_(start_sec_) {
  deadline_sec_ = start_sec_ + kCgiTimeoutSec;
}

CgiResponseHandler::~CgiResponseHandler() {
  cleanup_cgi_();
}

void CgiResponseHandler::cleanup_cgi_() {
  if (out_fd_ != -1) {
    close(out_fd_);
    out_fd_ = -1;
  }

  if (cgi_pid_ > 0) {
    kill(cgi_pid_, SIGKILL);
    int status;
    waitpid(cgi_pid_, &status, 0);
    cgi_pid_ = -1;
  }
}

bool CgiResponseHandler::is_cgi_error_() {
  if (cgi_pid_ <= 0) {
    return false;
  }

  int status;
  pid_t wpid = waitpid(cgi_pid_, &status, 0);

  if (wpid != cgi_pid_) {
    return false;
  }

  return (WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
         WIFSIGNALED(status);
}

void CgiResponseHandler::handle_cgi_completion_(bool cgi_error) {
  const ParsedCgiOutput parsed = parse_cgi_output_(cgi_output_);
  ClientHandler* ch = server_.find_client_handler(client_fd_);
  if (ch == NULL) {
    return;
  }

  if (parsed.is_local_redirect) {
    ch->cgi_local_redirect_ready(parsed.local_location);
    return;
  }

  Response response;
  if (cgi_error || !parsed.is_valid) {
    response.prepare_error_response(kBadGateway, "");
  } else {
    response = cgi::build_response_from_parsed(parsed);
  }
  ch->cgi_response_ready(response.serialize());
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
  bool seen_status = false;

  while (std::getline(headers_stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }
    if (line.empty()) {
      continue;
    }
    if (!cgi::parse_header_line(line, seen_status, status_line, result.headers)) {
      return ParsedCgiOutput();
    }
  }

  if (!cgi::apply_content_type_location_rules(result, status_line)) {
    return ParsedCgiOutput();  
  }
  if (result.is_local_redirect) {
    return result;
  }

  const bool has_location =
      result.headers.find("location") != result.headers.end();

  if (status_line.empty()) {
    result.status_code = has_location ? 302 : 200;
  } else {
    if (!cgi::parse_status_code(status_line, result.status_code)) {
      return ParsedCgiOutput();
    }
  }

  result.is_valid = true;
  return result;
}

HandlerStatus CgiResponseHandler::handle_timeout() {
  if (finished_){ 
    return kCgiInputDone;
  }
  finished_ = true;
  cleanup_cgi_();
  ClientHandler* ch = server_.find_client_handler(client_fd_);
  if (ch != NULL) {
    Response response;
    response.prepare_error_response(kGatewayTimeout, "");
    ch->cgi_response_ready(response.serialize());
  }

  return kCgiInputDone;
}

HandlerStatus CgiResponseHandler::handle_input() {
  if (finished_) {
    return kCgiInputDone;
  }
  char buf[kReadBufSize];
  ssize_t n = read(out_fd_, buf, sizeof(buf));

  if (n == -1) {
    return handle_poll_error();
  }

  if (n == 0) {
    finished_ = true;
    bool cgi_error = is_cgi_error_();
    cgi_pid_ = -1;

    handle_cgi_completion_(cgi_error);
    return kCgiInputDone;
  }

  update_deadline_();

  if (cgi_output_.size() + static_cast<std::size_t>(n) > kMaxCgiOutputBytes) {
    return fail_with_bad_gateway_();
  }

  cgi_output_.append(buf, n);

  if (!cgi::has_header_terminator(cgi_output_) &&
      cgi_output_.size() > kMaxCgiHeaderBytes) {
    return fail_with_bad_gateway_();
  }

  return kHandlerContinue;
}

HandlerStatus CgiResponseHandler::handle_output() {
  return kHandlerContinue;
}

HandlerStatus CgiResponseHandler::handle_poll_error() {
  std::cerr << "Error : CgiResponseHandler's poll" << std::endl;
  return fail_with_bad_gateway_();
}

HandlerStatus CgiResponseHandler::fail_with_bad_gateway_() {
  finished_ = true;

  cleanup_cgi_();

  ClientHandler* ch = server_.find_client_handler(client_fd_);
  if (ch != NULL) {
    Response response;
    response.prepare_error_response(kBadGateway, "");
    ch->cgi_response_ready(response.serialize());
  }

  return kCgiInputDone;
}
