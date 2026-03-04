#include "Response.hpp"

#include <string>
#include <sstream>
#include <fstream>

#include "Parser.hpp"
#include "string_utils.hpp"
#include "Config.hpp"

void Response::prepare_error_response(ParserStatus status, const std::string& path) {
  switch (status) {
    case kBadRequest:
      status_code_ = "400";
    case kNotFound:
      status_code_ = "404";
      break;
    case kNotImplemented:
      status_code_ = "501";
      break;
    case kUriTooLong:
      status_code_ = "414";
      break;
    case kVersionNotSupported:
      status_code_ = "505";
      break;
    // Method Not Allowed isn't checked here
    default:
      status_code_ = "500";
  }
  // TODO: write headers
  // TODO: write body
}

// TODO: add arguments
void Response::prepare_success_response() { status_code_ = "200"; }

void Response::prepare_redirect_response(int status, const std::string& redirect_url) {
  status_code_ = int_to_string(status);

  headers_["Location"] = redirect_url;

  //TODO: write body
}

std::string Response::serialize() const {
  std::string response;
  if (version_ == kHttp10) {
    response.append("HTTP/1.0 ");
  } else if (version_ == kHttp11) {
    response.append("HTTP/1.1 ");
  }
  response.append(status_code_);
  response.append(" \r\n");  // No reason-phrase
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

  std::stringstream ss;
  ss << body_.size();
  add_header("Content-Length", ss.str());
}

void Response::add_header(const std::string& key, const std::string& value) {
  headers_[key] = value;
}

void Response::fill_from_file(const std::string& path) {
  std::ifstream ifs(path.c_str(), std::ios::binary);
  if (!ifs) {
    //開けなかったときのエラーハンドリング
    return;
  }

  std::string content((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
  set_body(content);
}

std::string Response::get_mime_type(const std::string& path) {
  size_t pos = path.find_last_of('.');
  if (pos == std::string::npos) {
    return "application/octet-stream";
  }

  std::string extension = path.substr(pos + 1);
  if (extension == "html" || extension == "htm") return "text/html";
  if (extension == "css")  return "text/css";
  if (extension == "js")   return "text/javascript";
  if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
  if (extension == "png")  return "image/png";
  if (extension == "txt")  return "text/plain";

  return "application/octet-stream";
}
