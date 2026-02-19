#ifndef INCLUDE_CGIHANDLER_HPP_
#define INCLUDE_CGIHANDLER_HPP_

#include <sys/types.h>
#include <string>
#include <vector>

#include "HttpRequest.hpp"

class CgiHandler {
 public:
  explicit CgiHandler(const HttpRequest& request);
  int execute_async(const std::string& script_path,
                    pid_t& out_pid,
                    int& out_pipe_out_fd);

 private:
  const HttpRequest& request_;

  std::vector<std::string> create_env_vars(const std::string& script_path);
  char** create_envp(const std::vector<std::string>& env_vars);
  void free_envp(char** envp);

  CgiHandler(const CgiHandler&);
  CgiHandler& operator=(const CgiHandler&);
};

#endif  // INCLUDE_CGIHANDLER_HPP_