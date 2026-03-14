#include "Response.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>

#include "Parser.hpp"
#include "string_utils.hpp"
#include "Config.hpp"

namespace http_error_constants {
  const char* kErrorHtmlStart = "<html><head><title>";
  const char* kErrorTitleEnd  = "</title></head><body><center><h1>";
  const char* kErrorHeaderEnd = "</h1></center><hr><center>webserv</center></body></html>";
}

namespace http_redirect_constants {
  const char* kRedirectHtmlStart  = "<html><head><title>Moved</title></head><body><h1>";
  const char* kRedirectBodyMiddle = "</h1><p>The document has moved <a href=\"";
  const char* kRedirectBodyEnd    = "\">here</a>.</p></body></html>";
}

std::string Response::get_reason_phrase(int code) {
  switch (code) {
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "NoContent";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Content Too Large";
    case 414: return "URI Too Long";
    case 431: return "RequestHeaderFieldsTooLarge";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    default:  return "Internal Server Error";
  }
}

void Response::set_status_code(int code) {
  status_code_ = int_to_string(code);
  reason_phrase_ = get_reason_phrase(code);
}

void Response::ensure_content_length() {
  if (headers_.find("Content-Length") != headers_.end()) {
    return;
  }
  if (headers_.find("Transfer-Encoding") != headers_.end()) {
    return;
  }

  std::stringstream ss;
  ss << body_.size();
  add_header("Content-Length", ss.str());
}

void Response::prepare_error_response(ParserStatus status, const std::string& path) {
  int code = static_cast<int>(status);
  set_status_code(code);

  if (!path.empty() && access(path.c_str(), R_OK) == 0) {
    fill_from_file(path);
  } else {
    std::string message = status_code_ + " " + reason_phrase_;

    std::string html = http_error_constants::kErrorHtmlStart;
    html.append(message);
    html.append(http_error_constants::kErrorTitleEnd);
    html.append(message);
    html.append(http_error_constants::kErrorHeaderEnd);

    set_body_and_content_length(html);
  }
  add_header("Content-Type", "text/html");
}

void Response::prepare_success_response(ParserStatus status) {
  int code = static_cast<int>(status);
  set_status_code(code);

  if (code == kNoContent) {
    body_.clear();
    add_header("Content-Length", "0");
  } else {
    ensure_content_length();
  }
}

void Response::prepare_redirect_response(int status, const std::string& redirect_url) {
  set_status_code(status);

  add_header("Location", redirect_url);
  add_header("Content-Type", "text/html");

  std::string html = http_redirect_constants::kRedirectHtmlStart;
  html.append(status_code_);
  html.append(" ");
  html.append(reason_phrase_);
  html.append(http_redirect_constants::kRedirectBodyMiddle);
  html.append(redirect_url);
  html.append(http_redirect_constants::kRedirectBodyEnd);

  set_body_and_content_length(html);
}

std::string Response::serialize() const {
  std::string response;
  if (version_ == kHttp10) {
    response.append("HTTP/1.0 ");
  } else if (version_ == kHttp11) {
    response.append("HTTP/1.1 ");
  }
  response.append(status_code_);
  response.append(" ");
  response.append(reason_phrase_);
  response.append("\r\n");
  // TODO:write header into string
  for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
      it != headers_.end(); ++it) {
    response.append(it->first);
    response.append(": ");
    response.append(it->second);
    response.append("\r\n");
  }
  response.append("\r\n");  // End of header
  // write body into string
  response.append(body_);
  return response;
}

void Response::set_body(const std::string& body) {
  body_ = body;
}

void Response::set_body_and_content_length(const std::string& body) {
  set_body(body);
  ensure_content_length();
}

static std::string normalize_header_name(const std::string& key) {
  std::string normalized;
  normalized.reserve(key.size());

  bool capitalize = true;
  for (std::size_t i = 0; i < key.size(); ++i) {
    unsigned char ch = static_cast<unsigned char>(key[i]);
    if (ch == '-') {
      normalized.push_back('-');
      capitalize = true;
      continue;
    }
    if (capitalize) {
      normalized.push_back(static_cast<char>(std::toupper(ch)));
      capitalize = false;
    } else {
      normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
  }
  return normalized;
}

void Response::add_header(const std::string& key, const std::string& value) {
  headers_[normalize_header_name(key)] = value;
}

bool Response::fill_from_file(const std::string& path) {
  std::ifstream ifs(path.c_str(), std::ios::binary);
  if (!ifs) {
    return false;
  }

  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  set_body_and_content_length(content);
  return true;
}

std::string Response::get_mime_type(const std::string& path) {
  size_t pos = path.find_last_of('.');
  if (pos == std::string::npos) {
    return "application/octet-stream";
  }

  std::string extension = path.substr(pos + 1);
  if (extension == "html" || extension == "htm") return "text/html";
  if (extension == "css")  return "text/css";
  if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
  if (extension == "png")  return "image/png";
  if (extension == "txt" || extension == "py")  return "text/plain";

  return "application/octet-stream";
}


