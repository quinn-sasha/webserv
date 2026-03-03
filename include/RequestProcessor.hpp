#ifndef INCLUDE_REQUESTPROCESSOR_HPP_
#define INCLUDE_REQUESTPROCESSOR_HPP_

#include "Parser.hpp"
#include "Response.hpp"
#include "Config.hpp"

struct ProcesseorResult {
  enum Action {
    kSendResponse,
    kExecuteCgi,
  };
  Action next_action;
  Response response;  // Needed if normal operation or error
  // Something CGI needs
  std::string script_path; // CGIスクリプトのパスを保持
};

class RequestProcessor {
  int status_to_int(ParserStatus status);
  void handle_error(ProcesseorResult& result, ParserStatus status,
                    const ServerContext& target_config);
public:
  ProcesseorResult process(
      ParserStatus status, const Request& request, const ServerContext& target_config);
};

#endif  // INCLUDE_REQUESTPROCESSOR_HPP_
