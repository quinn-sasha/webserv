#ifndef INCLUDE_REQUESTPROCESSOR_HPP_
#define INCLUDE_REQUESTPROCESSOR_HPP_

#include "Parser.hpp"
#include "Response.hpp"
#include "Config.hpp"

#include <cerrno>
#include <iostream>

struct ProcessorResult {
  enum Action {
    kSendResponse,
    kExecuteCgi,
  };
  Action next_action;
  Response response;  // Needed if normal operation or error
  // Something CGI needs
  std::string script_path; // CGIスクリプトのパスを保持
  std::string query_string;
  std::string cgi_path;
  std::string request_body;
};

class RequestProcessor {
  static ProcessorResult handle_error(ParserStatus status,
                    const ServerContext& target_config);
  static ProcessorResult handle_redirect(const LocationContext& lc);
  static ProcessorResult handle_cgi(const std::string& path_only, const std::string& query_string, const std::string& cgi_path,
                                    const Request& request, const LocationContext& lc, const ServerContext& target_config);
  static std::string find_index_file(const std::string& directory_path, const LocationContext& lc);
  static ProcessorResult create_autoindex_response(const std::string& path,
                                                            const std::string& target);
  static ProcessorResult handle_directory(const std::string& path, const Request& request,
                                 const LocationContext& lc, const ServerContext& target_config);
  static ProcessorResult handle_file(const std::string& path, const ServerContext& target_config);
  static ProcessorResult handle_upload(const Request& request, const std::string& path_only,
                                        const LocationContext& lc, const ServerContext& target_config);
  static ProcessorResult handle_delete(std::string& path, const ServerContext& target_config);
  static ProcessorResult handle_static_file(const Request& request,
                                            const std::string& path,
                                            const LocationContext& lc,
                                            const ServerContext& target_config);
  static bool is_method_allowed(HttpMethod method, const LocationContext& lc);
  static int status_to_int(ParserStatus status);
  static ParserStatus errno_to_status(int err_num);
public:
  static ProcessorResult process(
      ParserStatus status, const Request& request, const ServerContext& target_config);
  static std::string get_error_page_path(const ServerContext& target_config, ParserStatus status);
};

#endif  // INCLUDE_REQUESTPROCESSOR_HPP_
