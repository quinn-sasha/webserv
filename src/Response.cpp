#include "Response.hpp"

#include <string>

#include "Parser.hpp"

// TODO: make_error_responseをResponseクラスのメンバー関数にする
void Response::prepare_error_response(ParserStatus status /*, request */) {
  switch (status) {
    case kBadRequest:
      status_code_ = "400";
      break;
    case kNotImplemented:
      status_code_ = "501";
      break;
    case kUriTooLong:
      status_code_ = "414";
      break;
    case kVersionNotSupported:
      status_code_ = "505";
      break;
    // Method Not Allowed isn't checked here
    default:
      status_code_ = "500";
  }
  // TODO: write headers
  // TODO: write body
}

// TODO: add arguments
void Response::prepare_success_response() { status_code_ = "200"; }

std::string Response::serialize() const {
  std::string response;
  if (version_ == kHttp10) {
    response.append("HTTP/1.0 ");
  } else if (version_ == kHttp11) {
    response.append("HTTP/1.1 ");
  }
  response.append(status_code_);
  response.append(" \r\n");  // No reason-phrase
  // TODO:write header into string
  response.append("\r\n");  // End of header
  // write body into string
  return response;
}
