#ifndef INCLUDE_PARSER_HPP_
#define INCLUDE_PARSER_HPP_

#include <string>

enum ParserStatus {
  kContinue,
  kBadRequest,
  kNotImplemented,
  kUriTooLong,
  kVersionNotSupported,
};

enum HttpMethod {
  kGet,
  kPost,
  kDelete,
  kUnknownMethod,
};

enum HttpVersion {
  kHttp10,
  kHttp11,
  kUnknownVersion,
};

struct RequestLine {
  HttpMethod method;
  std::string target;
  HttpVersion version;
};

class Parser {
  static const int kMaxUriLength = 2000;
  static const int kMaxRequestLine = 8100;
  RequestLine request_line_;
  // Helpers
  ParserStatus parse_method_name(const std::string& method);
  ParserStatus parse_request_target(const std::string& target);
  ParserStatus parse_http_version(const std::string& version);

 public:
  ParserStatus parse_request_line(const std::string& start_line);
  const RequestLine get_request_line() const;
};

#endif  // INCLUDE_PARSER_HPP_
