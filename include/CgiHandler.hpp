#ifndef INCLUDE_CGIHANDLER_HPP_
#define INCLUDE_CGIHANDLER_HPP_

#include <sys/types.h>
#include <string>
#include <vector>
#include "HttpRequest.hpp"

//class Server;  // 前方宣言

class CgiHandler {
 public:
  explicit CgiHandler(const HttpRequest& request);
  
  // ノンブロッキング版（fdを返す）
  int execute_async(const std::string& script_path, 
                    pid_t& out_pid);
                    // (const std::string& script_path, 
                    // pid_t& out_pid, 
                    // Server& server);

 private:
  const HttpRequest& request_;
  
  std::vector<std::string> create_env_vars(const std::string& script_path);
  char** create_envp(const std::vector<std::string>& env_vars);
  void free_envp(char** envp);

  // これらのメソッドは削除（使われていない）
  // std::string read_from_pipe(int fd);
  // std::string parse_cgi_output(const std::string& cgi_output);
};

#endif  // INCLUDE_CGIHANDLER_HPP_