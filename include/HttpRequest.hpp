#ifndef INCLUDE_HTTPREQUEST_HPP_
#define INCLUDE_HTTPREQUEST_HPP_

#include <map>
#include <string>

class HttpRequest {
 public:
  enum ParseResult {
    kIncomplete,  // まだ受信中
    kComplete,    // 解析完了
    kError        // 不正なリクエスト
  };

  HttpRequest();
  
  // リクエストを追加で解析
  ParseResult parse(const char* data, std::size_t len);
  
  // Getter
  const std::string& method() const { return method_; }
  const std::string& path() const { return path_; }
  const std::string& version() const { return version_; }
  const std::string& header(const std::string& key) const;
  const std::string& body() const { return body_; }
  const std::string& query_string() const { return query_string_; }  // 追加確認
  bool is_cgi() const;  // CGI判定
  bool is_chunked() const { return is_chunked_; }  // ✅ 追加

 private:
  std::string raw_request_;
  std::string method_;
  std::string path_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;
  std::string query_string_;  // 追加確認
  bool header_parsed_;
  bool is_chunked_;        // ✅ 追加
  std::size_t content_length_;

  bool parse_request_line();
  bool parse_headers();
  bool parse_body();
  ParseResult decode_chunked(const std::string& raw_body);  // ✅ 追加
};

#endif  // INCLUDE_HTTPREQUEST_HPP_