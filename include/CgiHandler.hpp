#ifndef INCLUDE_CGIHANDLER_HPP_
#define INCLUDE_CGIHANDLER_HPP_

#include <sys/types.h>
#include <string>
#include <vector>

#include "Parser.hpp"

class CgiHandler {
 public:  
 struct ExecArgv {
    std::string file;              
    std::vector<std::string> argv;  
  };
  explicit CgiHandler(const Request& request);
  ~CgiHandler();

  int execute_cgi(const std::string& script_path);

  int get_pipe_in_fd() const { return pipe_in_fd_; }
  int get_pipe_out_fd() const { return pipe_out_fd_; }
  pid_t get_cgi_pid() const { return cgi_pid_; }

 private:

  const Request& request_;
  int pipe_in_fd_;
  int pipe_out_fd_;
  pid_t cgi_pid_;

  void exec_cgi_child(int pipe_in[2], int pipe_out[2], const std::string& script_path);

  CgiHandler(const CgiHandler&);
  CgiHandler& operator=(const CgiHandler&);
};

#endif  // INCLUDE_CGIHANDLER_HPP_
