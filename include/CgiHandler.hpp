#ifndef INCLUDE_CGIHANDLER_HPP_
#define INCLUDE_CGIHANDLER_HPP_

#include <string>
#include <vector>
#include "HttpRequest.hpp"

class CgiHandler {
 public:
  explicit CgiHandler(const HttpRequest& request);
  
  std::string execute(const std::string& script_path);

 private:
  const HttpRequest& request_;
  
  std::vector<std::string> create_env_vars(const std::string& script_path);
  char** create_envp(const std::vector<std::string>& env_vars);
  void free_envp(char** envp);
  std::string read_from_pipe(int fd);
  std::string parse_cgi_output(const std::string& cgi_output);
};

#endif  // INCLUDE_CGIHANDLER_HPP_