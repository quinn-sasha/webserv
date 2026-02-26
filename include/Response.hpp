#ifndef INCLUDE_RESPONSE_HPP_
#define INCLUDE_RESPONSE_HPP_

#include <map>
#include <string>

#include "Parser.hpp"

class Response {
  static const HttpVersion version_ = kHttp11;
  std::string status_code_;
  std::map<std::string, std::string> headers_;
  std::string body_;

 public:
  void prepare_error_response(ParserStatus status /*, request */);
  void prepare_success_response();  // TODO: add arguments
  std::string serialize() const;
};

#endif  // INCLUDE_RESPONSE_HPP_
