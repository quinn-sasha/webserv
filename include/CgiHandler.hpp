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

  CgiHandler(const CgiHandler&);
  CgiHandler& operator=(const CgiHandler&);
};

#endif  // INCLUDE_CGIHANDLER_HPP_