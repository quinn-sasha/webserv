#include "Parser.hpp"

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <vector>

#include "string_utils.hpp"

bool uses_obsolete_line_folding(const std::string& request,
                                std::size_t crlf_pos) {
  if (crlf_pos + 2 >= request.size()) {
    return false;
  }
  if (request[crlf_pos + 2] == ' ' || request[crlf_pos + 2] == '\t') {
    return true;
  }
  return false;
}

namespace {
std::list<std::string> make_canonicalized_codings(
    const std::string& field_value) {
  std::list<std::string> result = split_string(field_value, ",");
  std::list<std::string>::iterator iter;
  for (iter = result.begin(); iter != result.end(); iter++) {
    *iter = ::trim(*iter, " \t");
    *iter = ::to_lower(*iter);
  }
  return result;
}
}  // namespace

// request line だけ見て判断すると結果は OK or 501(Not implemented)
// 405(Method not allowed)かどうかはrequest-targetとセットで判別できる
ParserStatus Parser::parse_method_name(const std::string& method) {
  request_.method = kUnknownMethod;
  if (method.size() > kMaxMethodLength) {
    return kNotImplemented;
  }
  if (method.find(' ') != std::string::npos) {
    return kBadRequest;
  }
  if (method == "GET") {
    request_.method = kGet;
    return kParseContinue;
  }
  return kNotImplemented;
}

ParserStatus Parser::parse_request_target(const std::string& target) {
  if (target.size() > kMaxUriLength) {
    return kUriTooLong;
  }
  if (target.find(' ') != std::string::npos) {
    return kBadRequest;
  }
  if (target.front() != '/') {
    return kBadRequest;
  }
  request_.target = target;
  return kParseContinue;
}

// HTTP-version  = HTTP-name "/" DIGIT(0 ~ 9) "." DIGIT
ParserStatus Parser::parse_http_version(const std::string& version) {
  request_.version = kUnknownVersion;
  if (version.find(' ') != std::string::npos) {
    return kBadRequest;
  }
  if (version.compare(0, 4, "HTTP") != 0) {
    return kBadRequest;
  }
  std::size_t delimiter_pos = version.find('/');
  if (delimiter_pos == std::string::npos) {
    return kBadRequest;
  }
  std::string numbering = version.substr(delimiter_pos + 1);
  if (numbering == "1.0") {
    request_.version = kHttp10;
    return kParseContinue;
  }
  if (numbering == "1.1") {
    request_.version = kHttp11;
    return kParseContinue;
  }
  if (numbering == "2.0" || numbering == "3.0") {
    return kVersionNotSupported;
  }
  return kBadRequest;
}

// CAUTION: request_line は改行を含まない
ParserStatus Parser::parse_request_line(const std::string& request_line) {
  if (request_line.size() > kMaxLineLength) {
    return kContentTooLarge;
  }
  std::size_t word_start = 0;
  std::vector<std::string> tokens;
  while (true) {
    std::size_t word_end = request_line.find(' ', word_start);
    if (word_end == std::string::npos) {
      tokens.push_back(request_line.substr(word_start));
      break;
    }
    std::string word = request_line.substr(word_start, word_end - word_start);
    tokens.push_back(word);
    word_start = word_end + 1;
  }
  if (tokens.size() != 3) {
    return kBadRequest;
  }
  ParserStatus status;
  if ((status = parse_method_name(tokens[0])) != kParseContinue) {
    return status;
  }
  if ((status = parse_request_target(tokens[1])) != kParseContinue) {
    return status;
  }
  if ((status = parse_http_version(tokens[2])) != kParseContinue) {
    return status;
  }
  return kParseContinue;
}

// CRLF was removed from filed_line.
// Return kParseContinue or kBadRequest.
ParserStatus Parser::parse_field_line(const std::string& filed_line) {
  if (filed_line.size() > kMaxLineLength) {
    return kContentTooLarge;
  }
  std::size_t delimiter_pos = filed_line.find(':');
  if (delimiter_pos == std::string::npos) {
    return kBadRequest;
  }
  std::string name = filed_line.substr(0, delimiter_pos);
  std::string value = ::trim(filed_line.substr(delimiter_pos + 1), " \t");
  if (name == "") {
    return kBadRequest;
  }
  if (name.find_first_of(" \t\r\n") != std::string::npos) {
    return kBadRequest;
  }
  if (value.find_first_of("\r\n") != std::string::npos) {
    return kBadRequest;
  }
  name = ::to_lower(name);  // unifiy filed-name lowercase
  if (request_.headers.find(name) == request_.headers.end()) {
    request_.headers[name] = value;
    return kParseContinue;
  }
  // Set-Cookie mustn't use list syntax, and it needs other way to store.
  // However, we will just ignore Set-Cookie for currnet implementation.
  if (name == "host") {
    return kBadRequest;
  }
  if (name == "content-length") {  // Error even if length is equal
    return kBadRequest;
  }
  request_.headers[name].append(", " + value);
  return kParseContinue;
}

ParserStatus Parser::parse_transfer_encodings(
    const std::string& field_value) const {
  std::list<std::string> codings = make_canonicalized_codings(field_value);
  if (codings.empty()) {
    return kBadRequest;
  }
  if (codings.back() != "chunked") {
    return kBadRequest;
  }
  std::list<std::string>::iterator iter;
  for (iter = codings.begin(); iter != codings.end(); iter++) {
    // Managing implemented codings in set would be better
    if (*iter != "chunked") {
      return kNotImplemented;
    }
  }
  return kParseContinue;
}

ParserStatus Parser::determine_next_action() const {
  if (request_.headers.find("host") == request_.headers.end()) {
    return kBadRequest;
  }
  if (request_.headers.count("transfer-encoding") &&
      request_.headers.count("content-length")) {
    return kBadRequest;
  }
  if (request_.headers.count("transfer-encoding")) {
    if (request_.version != kHttp11) {
      return kBadRequest;
    }
    return parse_transfer_encodings(request_.headers.at("transfer-encoding"));
  }
  if (request_.headers.count("content-length")) {
    std::string value = request_.headers.at("content-length");
    char* endptr;
    long int length = strtol(value.c_str(), &endptr, 10);
    if (*endptr != '\0') {
      return kBadRequest;
    }
    if (length < 0 || length > LONG_MAX) {
      return kBadRequest;
    }
    if (static_cast<std::size_t>(length) > kMaxBodySize) {
      return kBadRequest;
    }
    return kParseContinue;
  }
  return kParseFinished;
}

// Called by ClientHandler
// It determines if parse is failed, continuing or finished
// and update parser state
ParserStatus Parser::parse_request(const char* message, ssize_t num_read) {
  buffer_.append(std::string(message, num_read));
  if (buffer_.size() > kMaxRequestSize) {
    return kContentTooLarge;
  }
  if (state_ == kParsingRequestLine) {
    std::size_t crlf_pos = buffer_.find("\r\n");
    if (crlf_pos == std::string::npos) {
      return kParseContinue;
    }
    ParserStatus status = parse_request_line(buffer_.substr(0, crlf_pos));
    if (status != kParseContinue) {
      return status;
    }
    state_ = kParsingHeaders;
    buffer_.erase(0, crlf_pos + 2);
  }
  if (state_ == kParsingHeaders) {
    while (true) {
      std::size_t crlf_pos = buffer_.find("\r\n");
      if (crlf_pos == std::string::npos) {
        return kParseContinue;
      }
      if (crlf_pos == 0) {  // empty line
        buffer_.erase(0, 2);
        break;
      }
      if (uses_obsolete_line_folding(buffer_, crlf_pos)) {
        return kBadRequest;  // or other status to send message
      }
      ParserStatus status = parse_field_line(buffer_.substr(0, crlf_pos));
      if (status != kParseContinue) {
        return status;
      }
      buffer_.erase(0, crlf_pos + 2);
    }
    ParserStatus status = determine_next_action();
    if (status == kParseFinished) {
      state_ = kParsedState;
      return kParseFinished;
    }
    if (status != kParseContinue) {
      return status;
    }
    state_ = kParsingBody;
  }
  if (state_ == kParsingBody) {
    // TODO: implement parsing body
    // Clear buffer after parse finished
    return kParseContinue;
  }
  buffer_.clear();
  return kParseFinished;
}
