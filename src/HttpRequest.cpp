#include "HttpRequest.hpp"
#include <sstream>
#include <cstdlib>

HttpRequest::HttpRequest() 
    : header_parsed_(false), is_chunked_(false), content_length_(0) {}

HttpRequest::ParseResult HttpRequest::parse(const char* data, std::size_t len) {
  raw_request_.append(data, len);

  if (!header_parsed_) {
    std::size_t header_end = raw_request_.find("\r\n\r\n");
    if (header_end == std::string::npos) {
      return kIncomplete;
    }

    if (!parse_request_line()) return kError;
    if (!parse_headers()) return kError;
    
    header_parsed_ = true;

    const std::string& cl = header("Content-Length");
    if (!cl.empty()) {
      content_length_ = static_cast<std::size_t>(std::atoi(cl.c_str()));
    }
    
    const std::string& te = header("Transfer-Encoding");
    is_chunked_ = (te == "chunked");
  }

  std::size_t header_end = raw_request_.find("\r\n\r\n");
  std::size_t body_start = header_end + 4;

  // ボディがまだない場合
  if (body_start > raw_request_.size()) {
    return kIncomplete;
  }

  std::string raw_body = raw_request_.substr(body_start);

  if (is_chunked_) {
    return decode_chunked(raw_body);
  }

  // ボディなし（GET等）
  if (content_length_ == 0) {
    return kComplete;
  }

  // Content-Length固定長
  if (raw_body.size() < content_length_) {
    return kIncomplete;
  }

  body_ = raw_body.substr(0, content_length_);
  return kComplete;
}

HttpRequest::ParseResult HttpRequest::decode_chunked(const std::string& raw_body) {
  std::string decoded;
  std::size_t pos = 0;

  while (pos < raw_body.size()) {
    // チャンクサイズ行を探す
    std::size_t crlf = raw_body.find("\r\n", pos);
    if (crlf == std::string::npos) {
      return kIncomplete;
    }

    std::string size_str = raw_body.substr(pos, crlf - pos);

    // チャンク拡張（";"以降）を無視
    std::size_t semicolon = size_str.find(';');
    if (semicolon != std::string::npos) {
      size_str = size_str.substr(0, semicolon);
    }

    // 空文字チェック
    if (size_str.empty()) {
      return kError;
    }

    char* end_ptr;
    std::size_t chunk_size = std::strtoul(size_str.c_str(), &end_ptr, 16);
    if (end_ptr == size_str.c_str()) {
      return kError;
    }

    pos = crlf + 2;

    // 最終チャンク（0\r\n\r\n）
    if (chunk_size == 0) {
      body_ = decoded;
      // Content-Lengthをデコード済みサイズに更新
      std::ostringstream oss;
      oss << body_.size();
      headers_["Content-Length"] = oss.str();
      return kComplete;
    }

    // チャンクデータが届いているか確認（データ + \r\n）
    if (pos + chunk_size + 2 > raw_body.size()) {
      return kIncomplete;
    }

    decoded.append(raw_body, pos, chunk_size);
    pos += chunk_size + 2;
  }

  return kIncomplete;
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