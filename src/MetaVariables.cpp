#include "MetaVariables.hpp"

#include <cstring>
#include <sstream>

#include "HttpRequest.hpp"

MetaVariables::MetaVariables() {}
MetaVariables::~MetaVariables() {}

MetaVariables::MetaVariables(const MetaVariables& other) : env_(other.env_) {}

MetaVariables& MetaVariables::operator=(const MetaVariables& other) {
  if (this != &other) {
    env_ = other.env_;
  }
  return *this;
}

void MetaVariables::set_raw(const std::string& key, const std::string& value) {
  env_[key] = value;
}

std::string MetaVariables::get_raw(const std::string& key) const {
  std::map<std::string, std::string>::const_iterator it = env_.find(key);
  if (it == env_.end()) return "";
  return it->second;
}

bool MetaVariables::contains_raw(const std::string& key) const {
  return env_.find(key) != env_.end();
}

char** MetaVariables::build_envp() const {
  char** envp = new char*[env_.size() + 1];
  std::size_t i = 0;

  for (std::map<std::string, std::string>::const_iterator it = env_.begin();
       it != env_.end(); ++it) {
    std::string entry = it->first + "=" + it->second;
    envp[i] = new char[entry.size() + 1];
    std::strcpy(envp[i], entry.c_str());
    ++i;
  }
  envp[i] = NULL;
  return envp;
}

void MetaVariables::destroy_envp(char** envp) {
  for (std::size_t i = 0; envp[i] != NULL; ++i) {
    delete[] envp[i];
  }
  delete[] envp;
}

const char* MetaVariables::meta_name(MetaVar var) {
  switch (var) {
    case CgiAuthType:         
        return "AUTH_TYPE";
    case CgiContentLength:    
        return "CONTENT_LENGTH";
    case CgiContentType:      
        return "CONTENT_TYPE";
    case CgiGatewayInterface: 
        return "GATEWAY_INTERFACE";
    case CgiPathInfo:         
        return "PATH_INFO";
    case CgiQueryString:      
        return "QUERY_STRING";
    case CgiRemoteAddr:       
        return "REMOTE_ADDR";
    case CgiRequestMethod:    
        return "REQUEST_METHOD";
    case CgiScriptName:       
        return "SCRIPT_NAME";
    case CgiScriptFilename:   
        return "SCRIPT_FILENAME";
    case CgiServerName:       
        return "SERVER_NAME";
    case CgiServerPort:       
        return "SERVER_PORT";
    case CgiServerProtocol:   
        return "SERVER_PROTOCOL";
    case CgiServerSoftware:   
        return "SERVER_SOFTWARE";
  }
  return "";
}

void MetaVariables::set_meta(MetaVar var, const std::string& value) {
  set_raw(std::string(meta_name(var)), value);
}

std::string MetaVariables::get_meta(MetaVar var) const {
  return get_raw(std::string(meta_name(var)));
}

MetaVariables MetaVariables::from_request(const HttpRequest& request,
                                          const std::string& script_path) {
  MetaVariables env;

  env.set_meta(CgiRequestMethod, request.method());
  env.set_meta(CgiScriptName, request.path());
  env.set_meta(CgiPathInfo, request.path());
  env.set_meta(CgiQueryString, request.query_string());
  env.set_meta(CgiServerProtocol, "HTTP/1.1");
  env.set_meta(CgiGatewayInterface, "CGI/1.1");
  env.set_meta(CgiScriptFilename, script_path);

  const std::string& content_type = request.header("Content-Type");
  if (!content_type.empty()) {
    env.set_meta(CgiContentType, content_type);
  }

  {
    const std::string& body = request.body();
    std::ostringstream oss;
    oss << body.size();
    env.set_meta(CgiContentLength, oss.str());
  }

  return env;
}