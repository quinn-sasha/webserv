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
                                    const std::string& script_uri,
                                    const std::string& query_string,
                                    const std::string& server_name,
                                    const std::string& server_port,
                                    const std::string& remote_addr);

  char** build_envp() const;
  static void destroy_envp(char** envp);

 private:
  std::string request_method_;
  std::string query_string_;
  std::string content_length_;
  std::string content_type_;

  std::map<std::string, std::string> meta_variables_;

  std::map<std::string, std::string> http_headers_;

  void add_to_list(std::vector<std::string>& list, const std::string& key,
                   const std::string& value) const;
};

#endif  // INCLUDE_METAVARIABLES_HPP_
