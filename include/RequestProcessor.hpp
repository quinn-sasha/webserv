#ifndef INCLUDE_REQUESTPROCESSOR_HPP_
#define INCLUDE_REQUESTPROCESSOR_HPP_

#include "Parser.hpp"
#include "Response.hpp"

struct ProcesseorResult {
  enum Action {
    kSendResponse,
    kExecuteCgi,
    // Maybe redirect?
  };
  Action next_action;
  Response response;  // Needed if normal operation or error
  // Something CGI needs
};

class RequestProcessor {
 public:
  static ProcesseorResult process(
      ParserStatus status /*, const Request& request , const Config& config */);
};

#endif  // INCLUDE_REQUESTPROCESSOR_HPP_
