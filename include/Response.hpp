#ifndef INCLUDE_RESPONSE_HPP_
#define INCLUDE_RESPONSE_HPP_

#include <map>
#include <string>

#include "Parser.hpp"

class Response {
 public:
  static const HttpVersion version = kHttp11;
  std::string status_code;
  std::map<std::string, std::string> headers;
  std::string body;

  std::string serialize() const;
};

#endif  // INCLUDE_RESPONSE_HPP_
