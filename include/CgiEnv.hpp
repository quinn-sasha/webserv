#ifndef INCLUDE_CGIENV_HPP_
#define INCLUDE_CGIENV_HPP_

#include <map>
#include <string>

class HttpRequest;  // forward declaration

class CgiEnv {
 public:
  enum MetaVar {
    kAuthType,
    kContentLength,
    kContentType,
    kGatewayInterface,
    kPathInfo,
    kQueryString,
    kRemoteAddr,
    kRequestMethod,
    kScriptName,
    kScriptFilename,
    kServerName,
    kServerPort,
    kServerProtocol,
    kServerSoftware
  };

  CgiEnv();
  ~CgiEnv();

  CgiEnv(const CgiEnv& other);
  CgiEnv& operator=(const CgiEnv& other);

  // 低レベルAPI
  void set(const std::string& key, const std::string& value);
  std::string get(const std::string& key) const;
  bool has(const std::string& key) const;

  // MetaVar API（RFC/CGIっぽい名前で扱う）
  void set(MetaVar var, const std::string& value);
  std::string get(MetaVar var) const;

  // Request からまとめて構築（ここに create_env を移す）
  static CgiEnv from_request(const HttpRequest& request,
                             const std::string& script_path);

  // execve() 用
  char** to_envp() const;
  static void free_envp(char** envp);

 private:
  static const char* key_of(MetaVar var);

  std::map<std::string, std::string> env_;
};

#endif  // INCLUDE_CGIENV_HPP_