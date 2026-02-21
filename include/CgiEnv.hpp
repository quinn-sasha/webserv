#ifndef INCLUDE_CGIENV_HPP_
#define INCLUDE_CGIENV_HPP_

#include <map>
#include <string>

class CgiEnv {
 public:
  CgiEnv();
  ~CgiEnv();

  CgiEnv(const CgiEnv& other);             
  CgiEnv& operator=(const CgiEnv& other);  

  void set(const std::string& key, const std::string& value);
  std::string get(const std::string& key) const;
  bool has(const std::string& key) const;

  // execve() に渡す char** を生成（呼び出し側が free_envp() で解放）
  char** to_envp() const;
  static void free_envp(char** envp);

 private:
  std::map<std::string, std::string> env_;  
};

#endif  // INCLUDE_CGIENV_HPP_