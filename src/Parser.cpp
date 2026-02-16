#include "Parser.hpp"

#include <cstddef>
#include <string>
#include <vector>

// request line だけ見て判断すると結果は OK or 501(Not implemented)
// 405(Method not allowed)かどうかはrequest-targetとセットで判別できる
ParserStatus Parser::parse_method_name(const std::string& method) {
  request_line_.method = kUnknownMethod;
  if (method.size() > kMaxUriLength) {
    return kNotImplemented;
  }
  if (method.find(' ') != std::string::npos) {
    return kBadRequest;
  }
  if (method == "GET") {
    request_line_.method = kGet;
    return kContinue;
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
  request_line_.target = target;
  return kContinue;
}

// HTTP-version  = HTTP-name "/" DIGIT(0 ~ 9) "." DIGIT
ParserStatus Parser::parse_http_version(const std::string& version) {
  request_line_.version = kUnknownVersion;
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
    request_line_.version = kHttp10;
    return kContinue;
  }
  if (numbering == "1.1") {
    request_line_.version = kHttp11;
    return kContinue;
  }
  if (numbering == "2.0" || numbering == "3.0") {
    return kVersionNotSupported;
  }
  return kBadRequest;
}

// まだrequest_lineの解析が完了していない
// かつrequestに改行が初めて発見された時に呼ばれる
// CAUTION: request_line は改行を含まない
// 引数に request_line を受け取るが、これは仮のインターフェイス
// クラスのメンバ変数として受け取ってもいい
ParserStatus Parser::parse_request_line(const std::string& request_line) {
  if (request_line.size() > kMaxRequestLine) {
    return kNotImplemented;
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
  if ((status = parse_method_name(tokens[0])) != kContinue) {
    return status;
  }
  if ((status = parse_request_target(tokens[1])) != kContinue) {
    return status;
  }
  if ((status = parse_http_version(tokens[2])) != kContinue) {
    return status;
  }
  return kContinue;
}

const RequestLine Parser::get_request_line() const { return request_line_; }
