#ifndef INCLUDE_PARSER_HPP_
#define INCLUDE_PARSER_HPP_

#include <list>
#include <map>
#include <string>

enum ParserStatus {
  kBadRequest,
  kNotImplemented,
  kUriTooLong,
  kVersionNotSupported,
  kMethodNotAllowed,
  kContentTooLarge,
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

  struct BodyParseInfo {
    bool is_chunked;
    std::list<std::string> encodings;
    std::size_t content_length;
  } body_parse_info;

  std::string body;
};

class Parser {
  static const std::size_t kMaxUriLength = 2000;
  static const std::size_t kMaxMethodLength = 10;  // temporal
  static const std::size_t kMaxLineLength = 8100;
  static const std::size_t kMaxBodySize = 1000 * 1024ul;
  static const std::size_t kMaxRequestSize = 1ul * 1024 * 1024;
  std::string buffer_;
  ParserState state_;
  Request request_;

  // Helpers
  ParserStatus parse_method_name(const std::string& method);
  ParserStatus parse_request_target(const std::string& target);
  ParserStatus parse_http_version(const std::string& version);
  ParserStatus parse_transfer_encodings(const std::string& field_value);
  ParserStatus parse_request_line(const std::string& start_line);
  ParserStatus parse_field_line(const std::string& field_line);
  ParserStatus determine_next_action();
  // Prohibit copy and assignment
  Parser(const Parser& ohter);
  Parser& operator=(const Parser& ohter);

 public:
  Parser() : state_(kParsingRequestLine) {}
  ParserStatus parse_request(const char* message, ssize_t num_read);
  const Request& get_request() const { return request_; }
};

#endif  // INCLUDE_PARSER_HPP_
