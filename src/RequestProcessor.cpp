#include "RequestProcessor.hpp"

#include "Parser.hpp"
#include "Response.hpp"

Response RequestProcessor::make_error_response(
    ParserStatus status /*, request */) {
  Response response;
  switch (status) {
    case kBadRequest:
      response.status_code = "400";
      break;
    case kNotImplemented:
      response.status_code = "501";
      break;
    case kUriTooLong:
      response.status_code = "414";
      break;
    case kVersionNotSupported:
      response.status_code = "505";
      break;
    // Method Not Allowed isn't checked here
    default:
      response.status_code = "500";
  }
  // TODO: write headers
  // TODO: write body
  return response;
}

// エラーかどうかだけを判定して、status code を書き込み返す（一時的な実装）
ProcesseorResult RequestProcessor::process(
    ParserStatus status, const Request& request /*, const Config& config */) {
  ProcesseorResult result;
  if (status != kParseFinished) {
    result.response = make_error_response(status);
    result.next_action = ProcesseorResult::kSendResponse;
    return result;
  }
  // temporal code
  result.response.status_code = "200";
  result.next_action = ProcesseorResult::kSendResponse;
  return result;
  // construct URI
  // method allowed?
  // CGI?
}
