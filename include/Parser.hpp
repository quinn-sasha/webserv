#ifndef INCLUDE_PARSER_HPP_
#define INCLUDE_PARSER_HPP_

#include <map>
#include <string>

enum ParserStatus {
  kBadRequest,
  kNotImplemented,
  kUriTooLong,
  kVersionNotSupported,
  kMethodNotAllowed,
  // Custom status used in Parser class
  kParseContinue,
  kParseFinished,
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

enum ParserState {
  kParsingRequestLine,
  kParsingHeaders,
  kParsingBody,
  kParsedState,
};

struct Request {
  HttpMethod method;
  std::string target;
  HttpVersion version;
  std::map<std::string, std::string> headers;
};

class Parser {
  static const int kMaxUriLength = 2000;
  static const int kMaxRequestLine = 8100;
  std::string buffer_;
  ParserState state_;
  Request request_;

  // Helpers
  ParserStatus parse_method_name(const std::string& method);
  ParserStatus parse_request_target(const std::string& target);
  ParserStatus parse_http_version(const std::string& version);
  // Prohibit copy and assignment
  Parser(const Parser& ohter);
  Parser& operator=(const Parser& ohter);

 public:
  Parser() : state_(kParsingRequestLine) {}
  ParserStatus parse_request_line(const std::string& start_line);
  ParserStatus parse_filed_line(const std::string& filed_line);
  ParserStatus determine_next_action() const;
  ParserStatus parse_request(const char* message, ssize_t num_read);
  const Request& get_request() const { return request_; }
};

#endif  // INCLUDE_PARSER_HPP_
