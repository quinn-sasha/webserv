#include "Parser.hpp"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "string_utils.hpp"

namespace {
std::string extract_field_value(const std::string& filed_line,
                                std::size_t delimiter_pos) {
  std::string spaces = " \t";
  std::size_t start = filed_line.find_first_not_of(spaces, delimiter_pos + 1);
  if (start == std::string::npos) {
    return "";
  }
  std::size_t last = filed_line.find_last_not_of(spaces);
  return filed_line.substr(start, last - start + 1);
}

bool uses_obsolete_line_folding(const std::string& request,
                                std::size_t crlf_pos) {
  if (crlf_pos + 1 >= request.size()) {
    return false;
  }
  if (request[crlf_pos + 1] == ' ' || request[crlf_pos + 1] == '\t') {
    return true;
  }
  return false;
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
ParserStatus Parser::parse_filed_line(const std::string& filed_line) {
  if (filed_line.size() > kMaxLineLength) {
    return kContentTooLarge;
  }
  std::size_t delimiter_pos = filed_line.find(':');
  if (delimiter_pos == std::string::npos) {
    return kBadRequest;
  }
  std::string name = filed_line.substr(0, delimiter_pos);
  std::string value = extract_field_value(filed_line, delimiter_pos);
  if (name.find_first_of(" \t\r\n") != std::string::npos) {
    return kBadRequest;
  }
  if (value.find_first_of("\r\n") != std::string::npos) {
    return kBadRequest;
  }
  name = to_lower(name);  // unifiy filed-name lowercase
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

ParserStatus Parser::determine_next_action() const {
  if (request_.headers.find("host") == request_.headers.end()) {
    return kBadRequest;
  }
  // TODO:
  // * implement POST and DELETE
  if (request_.method == kGet || request_.method == kDelete) {
    return kParseFinished;
  }
  return kParseContinue;
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
      ParserStatus status = parse_filed_line(buffer_.substr(0, crlf_pos));
      if (status != kParseContinue) {
        return status;
      }
      buffer_.erase(0, crlf_pos + 2);
    }
    ParserStatus status = determine_next_action();
    if (status != kParseFinished || status != kParseContinue) {
      return status;
    }
    if (status == kParseFinished) {
      state_ = kParsedState;
      return kParseFinished;
    }
    state_ = kParsingBody;
  }
  if (state_ == kParsingBody) {
    // TODO: implement parsing body
  }
  return kParseFinished;
}
