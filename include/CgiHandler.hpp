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

  explicit CgiHandler(const Request& request,
                      const std::string& query_string,
                      const std::string& script_uri,
                      const std::string& server_name,
                      const std::string& server_port,
                      const std::string& remote_addr);
  ~CgiHandler();

  int execute_cgi(const std::string& script_path, const std::string& cgi_path);

  int get_pipe_in_fd() const { return pipe_in_fd_; }
  int get_pipe_out_fd() const { return pipe_out_fd_; }
  pid_t get_cgi_pid() const { return cgi_pid_; }

 private:
  const Request& request_;
  int pipe_in_fd_;
  int pipe_out_fd_;
  pid_t cgi_pid_;
  std::string query_string_;
  std::string script_uri_;
  std::string server_name_;
  std::string server_port_;
  std::string remote_addr_;


  void exec_cgi_child(int pipe_in[2], int pipe_out[2], const std::string& script_path, const std::string& cgi_path);

  CgiHandler(const CgiHandler&);
  CgiHandler& operator=(const CgiHandler&);
};

#endif  // INCLUDE_CGIHANDLER_HPP_
