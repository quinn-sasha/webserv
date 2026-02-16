#include "HttpRequest.hpp"
#include <sstream>
#include <cstdlib>

HttpRequest::HttpRequest() 
    : header_parsed_(false), content_length_(0) {}

HttpRequest::ParseResult HttpRequest::parse(const char* data, std::size_t len) {
  raw_request_.append(data, len);

  // ヘッダー未解析
  if (!header_parsed_) {
    std::size_t header_end = raw_request_.find("\r\n\r\n");
    if (header_end == std::string::npos) {
      return kIncomplete;  // まだヘッダー受信中
    }

    if (!parse_request_line()) return kError;
    if (!parse_headers()) return kError;
    
    header_parsed_ = true;

    // Content-Length取得
    const std::string& cl = header("Content-Length");
    if (!cl.empty()) {
      content_length_ = std::atoi(cl.c_str());
    }
  }

  // ボディ受信確認
  std::size_t header_end = raw_request_.find("\r\n\r\n");
  std::size_t body_start = header_end + 4;
  std::size_t body_len = raw_request_.size() - body_start;

  if (body_len < content_length_) {
    return kIncomplete;  // まだボディ受信中
  }

  body_ = raw_request_.substr(body_start, content_length_);
  return kComplete;
}

bool HttpRequest::parse_request_line() {
  std::size_t line_end = raw_request_.find("\r\n");
  if (line_end == std::string::npos) return false;

  std::string line = raw_request_.substr(0, line_end);
  std::istringstream iss(line);
  
  std::string full_path;
  iss >> method_ >> full_path >> version_;

  // クエリ文字列を分離
  std::size_t query_pos = full_path.find('?');
  if (query_pos != std::string::npos) {
    query_string_ = full_path.substr(query_pos + 1);
    path_ = full_path.substr(0, query_pos);
  } else {
    path_ = full_path;
    query_string_ = "";
  }

  return !method_.empty() && !path_.empty() && !version_.empty();
}

bool HttpRequest::parse_headers() {
  std::size_t pos = raw_request_.find("\r\n") + 2;
  
  while (true) {
    std::size_t line_end = raw_request_.find("\r\n", pos);
    if (line_end == std::string::npos) break;
    
    std::string line = raw_request_.substr(pos, line_end - pos);
    if (line.empty()) break;  // 空行（ヘッダー終了）

    std::size_t colon = line.find(':');
    if (colon != std::string::npos) {
      std::string key = line.substr(0, colon);
      std::string value = line.substr(colon + 2);  // ": "をスキップ
      headers_[key] = value;
    }

    pos = line_end + 2;
  }
  return true;
}

const std::string& HttpRequest::header(const std::string& key) const {
  static const std::string empty;
  std::map<std::string, std::string>::const_iterator it = headers_.find(key);
  return (it != headers_.end()) ? it->second : empty;
}

bool HttpRequest::is_cgi() const {
  // 例: /cgi-bin/ で始まるパス
  return path_.find("/cgi-bin/") == 0;
}