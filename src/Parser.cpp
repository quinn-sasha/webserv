#include "Parser.hpp"

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
int convert_to_size(std::size_t& result, const std::string& input, int base) {
  int tmp_res;
  if (convert_to_integer(tmp_res, input, base) == -1) {
    return -1;
  }
  if (tmp_res < 0) {
    return -1;
  }
  result = static_cast<std::size_t>(tmp_res);
  return 0;
}

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
    return kRequestHeaderFieldsTooLarge;
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

ParserStatus Parser::parse_transfer_encodings(const std::string& field_value) {
  std::list<std::string> encodings = make_canonicalized_codings(field_value);
  if (encodings.empty()) {
    return kBadRequest;
  }
  if (encodings.back() != "chunked") {
    return kBadRequest;
  }
  std::list<std::string>::iterator iter;
  for (iter = encodings.begin(); iter != encodings.end(); iter++) {
    // Managing implemented codings in set would be better
    if (*iter != "chunked") {
      return kNotImplemented;
    }
  }
  request_.body_parse_info.is_chunked = true;
  request_.body_parse_info.encodings = encodings;
  return kParseContinue;
}

ParserStatus Parser::determine_next_action() {
  if (request_.headers.find("host") == request_.headers.end()) {
    return kBadRequest;
  }
  if (!request_.headers.count("transfer-encoding") &&
      !request_.headers.count("content-length")) {
    return kParseFinished;
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
  std::string value = request_.headers.at("content-length");
  std::size_t length;
  if (convert_to_size(length, value, 10) == -1) {
    return kBadRequest;
  }
  if (length > kMaxBodySize) {
    return kContentTooLarge;
  }
  request_.body_parse_info.content_length = length;
  request_.body_parse_info.is_chunked = false;
  return kParseContinue;
}

ParserStatus Parser::parse_chunked_size_section() {
  std::size_t crlf_pos = chunked_data_.tmp_buf.find("\r\n");
  if (crlf_pos == std::string::npos) {
    return kParseContinue;
  }
  std::size_t word_end = crlf_pos;
  std::size_t delimiter_pos = chunked_data_.tmp_buf.find(";");
  if (delimiter_pos != std::string::npos && delimiter_pos < word_end) {
    word_end = delimiter_pos;
  }
  std::string size_str = chunked_data_.tmp_buf.substr(0, word_end);
  if (convert_to_size(chunked_data_.remaining_size, size_str, 16) == -1) {
    return kBadRequest;
  }
  if (chunked_data_.remaining_size == 0) {  // last chunk
    chunked_data_.state = kParsingTrailer;
    chunked_data_.tmp_buf.erase(0, crlf_pos + 2);  // Discard chunk extension
    return kKeepParsingChunked;
  }
  if (delimiter_pos == std::string::npos) {
    chunked_data_.state = kParsingData;
    chunked_data_.tmp_buf.erase(0, crlf_pos + 2);
  } else {
    chunked_data_.state = kParsingExtension;
    chunked_data_.tmp_buf.erase(0, delimiter_pos + 1);
  }
  return kKeepParsingChunked;
}

ParserStatus Parser::parse_chunked_body(const std::string& body) {
  chunked_data_.tmp_buf.append(body);
  while (true) {
    if (chunked_data_.tmp_buf.size() > kMaxBodySize) {
      return kContentTooLarge;
    }
    if (request_.body.size() > kMaxBodySize) {
      return kContentTooLarge;
    }
    if (chunked_data_.state == kParsingSize) {
      ParserStatus status = parse_chunked_size_section();
      if (status != kKeepParsingChunked) {
        return status;
      }
    }
    if (chunked_data_.state == kParsingTrailer) {
      break;
    }
    if (chunked_data_.state == kParsingExtension) {
      // Just discard
      std::size_t crlf_pos = chunked_data_.tmp_buf.find("\r\n");
      chunked_data_.tmp_buf.erase(0, crlf_pos + 2);
      chunked_data_.state = kParsingData;
    }
    if (chunked_data_.state == kParsingData) {
      std::size_t size_read = chunked_data_.tmp_buf.size();
      std::size_t remain = chunked_data_.remaining_size;
      if (size_read < remain) {
        request_.body.append(chunked_data_.tmp_buf);
        chunked_data_.tmp_buf.clear();
        chunked_data_.remaining_size -= size_read;
        return kParseContinue;
      }
      request_.body.append(chunked_data_.tmp_buf.substr(0, remain));
      chunked_data_.tmp_buf.erase(0, remain);
      chunked_data_.remaining_size = 0;
      chunked_data_.state = kParsingCrlf;
    }
    if (chunked_data_.state == kParsingCrlf) {
      if (chunked_data_.tmp_buf.size() < 2) {
        return kParseContinue;
      }
      std::size_t crlf_pos = chunked_data_.tmp_buf.find("\r\n");
      if (crlf_pos == std::string::npos) {
        return kBadRequest;
      }
      if (crlf_pos != 0) {
        return kBadRequest;
      }
      chunked_data_.state = kParsingSize;
      chunked_data_.tmp_buf.erase(0, 2);
      continue;
    }
  }
  // after last chunk
  while (true) {
    if (chunked_data_.tmp_buf.size() > kMaxLineLength) {
      return kContentTooLarge;
    }
    std::size_t crlf_pos = chunked_data_.tmp_buf.find("\r\n");
    if (crlf_pos == std::string::npos) {
      return kParseContinue;
    }
    if (crlf_pos == 0) {
      return kParseFinished;
    }
    chunked_data_.tmp_buf.erase(0, crlf_pos + 2);  // Just discard
  }
}

// Request Pipelining is not supported
ParserStatus Parser::parse_content_length_body(const std::string& body) {
  std::size_t remaining =
      request_.body_parse_info.content_length - request_.body.size();
  if (body.size() < remaining) {
    request_.body.append(body);
    return kParseContinue;
  }
  request_.body.append(body.substr(0, remaining));
  return kParseFinished;
}

ParserStatus Parser::parse_body(const std::string& body) {
  if (request_.body_parse_info.is_chunked) {
    return parse_chunked_body(body);
  }
  return parse_content_length_body(body);
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
      return kParseFinished;
    }
    if (status != kParseContinue) {
      return status;
    }
    state_ = kParsingBody;
  }
  if (state_ == kParsingBody) {
    ParserStatus status = parse_body(buffer_);
    buffer_.clear();
    return status;
  }
  return kParseContinue;
}
