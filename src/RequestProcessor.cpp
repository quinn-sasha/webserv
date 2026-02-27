#include "RequestProcessor.hpp"

#include "Parser.hpp"
#include "Response.hpp"

// エラーかどうかだけを判定して、status code を書き込み返す（一時的な実装）
ProcesseorResult RequestProcessor::process(
    ParserStatus status, const Request& request /*, const Config& config */) {
  ProcesseorResult result;
  if (status != kParseFinished) {
    result.response.prepare_error_response(status /*, request */);
    result.next_action = ProcesseorResult::kSendResponse;
    return result;
  }
  // construct URI
  // method allowed?
  // CGI?
  if (request.target.find("/cgi-bin/") != std::string::npos) {
    result.next_action = ProcesseorResult::kExecuteCgi;
    result.script_path = request.target.substr(1); // /path/cgi-bin/script.py -> path/cgi-bin/script.py
    return result;
  }
  // if not CGI, do some process like fetching file
  result.response.prepare_success_response();
  result.next_action = ProcesseorResult::kSendResponse;
  return result;
}
