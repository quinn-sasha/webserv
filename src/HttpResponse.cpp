#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse() 
    : status_code_(200), status_message_("OK") {}

void HttpResponse::set_status(int code, const std::string& message) {
  status_code_ = code;
  status_message_ = message;
}

void HttpResponse::add_header(const std::string& key, const std::string& value) {
  headers_[key] = value;
}

void HttpResponse::set_body(const std::string& body) {
  body_ = body;
  
  // Content-Length自動設定
  std::ostringstream oss;
  oss << body_.size();
  headers_["Content-Length"] = oss.str();
}

std::string HttpResponse::to_string() const {
  std::ostringstream oss;
  
  // ステータスライン
  oss << "HTTP/1.1 " << status_code_ << " " << status_message_ << "\r\n";
  
  // ヘッダー
  for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
       it != headers_.end(); ++it) {
    oss << it->first << ": " << it->second << "\r\n";
  }
  
  oss << "\r\n" << body_;
  return oss.str();
}