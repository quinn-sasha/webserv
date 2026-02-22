#ifndef INCLUDE_METAVARIABLES_HPP_
#define INCLUDE_METAVARIABLES_HPP_

#include <map>
#include <string>

class HttpRequest;

class MetaVariables {
 public:
  enum MetaVar {
    CgiAuthType,
    CgiContentLength,
    CgiContentType,
    CgiGatewayInterface,
    CgiPathInfo,
    CgiQueryString,
    CgiRemoteAddr,
    CgiRequestMethod,
    CgiScriptName,
    CgiScriptFilename,
    CgiServerName,
    CgiServerPort,
    CgiServerProtocol,
    CgiServerSoftware
  };

  MetaVariables();
  ~MetaVariables();

  MetaVariables(const MetaVariables& other);
  MetaVariables& operator=(const MetaVariables& other);

  // 任意キー操作（内部用）
  void set_raw(const std::string& key, const std::string& value);
  std::string get_raw(const std::string& key) const;
  bool contains_raw(const std::string& key) const;

  // CGIメタ変数操作（通常はこちらを使う）
  void set_meta(MetaVar var, const std::string& value);
  std::string get_meta(MetaVar var) const;

  static MetaVariables from_request(const HttpRequest& request,
                                    const std::string& script_path);

  // execve 用 envp を生成・破棄
  char** build_envp() const;
  static void destroy_envp(char** envp);

 private:
  static const char* meta_name(MetaVar var);

  std::map<std::string, std::string> env_;
};

#endif  // INCLUDE_METAVARIABLES_HPP_