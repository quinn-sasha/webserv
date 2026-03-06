#ifndef INCLUDE_RESPONSE_HPP_
#define INCLUDE_RESPONSE_HPP_

#include <map>
#include <string>

#include "Parser.hpp"

class Response {
  static const HttpVersion version_ = kHttp11;
  std::string status_code_;
  std::string reason_phrase_;
  std::map<std::string, std::string> headers_;
  std::string body_;

 public:
  void prepare_error_response(ParserStatus status, const std::string& path);
  void prepare_success_response(ParserStatus status);  // TODO: add arguments
  void prepare_redirect_response(int status, const std::string& redirect_url);
  void set_body(const std::string& body);
  void add_header(const std::string& key, const std::string& value);
  std::string get_reason_phrase(int code);
  bool fill_from_file(const std::string& path);
  std::string get_mime_type(const std::string& path);
  std::string serialize() const;
};

#endif  // INCLUDE_RESPONSE_HPP_
