#ifndef INCLUDE_METAVARIABLES_HPP_
#define INCLUDE_METAVARIABLES_HPP_

#include <map>
#include <string>
#include <vector>

#include "Parser.hpp"

class MetaVariables {
 public:
  MetaVariables();
  ~MetaVariables();

  MetaVariables(const MetaVariables& other);
  MetaVariables& operator=(const MetaVariables& other);

  static MetaVariables from_request(const Request& request,
                                    const std::string& script_path);

  char** build_envp() const;
  static void destroy_envp(char** envp);

 private:
  // --- Special Variables (Core Identity) ---
  std::string script_filename_;
  std::string request_method_;
  std::string query_string_;
  std::string content_length_;
  std::string content_type_;

  // --- CGI Standard Meta-Variables (Map) ---
  // e.g., SERVER_PROTOCOL, REMOTE_ADDR, SERVER_PORT
  std::map<std::string, std::string> meta_variables_;

  // --- HTTP Headers (Map, prefixed with HTTP_) ---
  // e.g., HTTP_USER_AGENT, HTTP_ACCEPT
  std::map<std::string, std::string> http_headers_;

  // Helper to add strings to the environment vector
  void add_to_list(std::vector<std::string>& list, const std::string& key,
                   const std::string& value) const;
};

#endif  // INCLUDE_METAVARIABLES_HPP_
