#ifndef INCLUDE_REQUESTPROCESSOR_HPP_
#define INCLUDE_REQUESTPROCESSOR_HPP_

#include "Parser.hpp"
#include "Response.hpp"

struct ProcesseorResult {
  enum Action {
    kSendResponse,
    kExecuteCgi,
    // maybe redirect?
  };
  Action next_action;
  Response response;  // needed if normal operation or error
  // something CGI needs
};

class RequestProcessor {
 public:
  static Response make_error_response(ParserStatus status);
  static ProcesseorResult process(
      ParserStatus status, const Request& request /*, const Config& config */);
};

#endif  // INCLUDE_REQUESTPROCESSOR_HPP_
