#include "MetaVariables.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

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

static std::string to_upper_http_env_key(const std::string& header_key) {
  // "User-Agent" -> "HTTP_USER_AGENT"
  std::string out = "HTTP_";
  for (std::size_t i = 0; i < header_key.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(header_key[i]);
    if (c == '-') out.push_back('_');
    else out.push_back(static_cast<char>(std::toupper(c)));
  }
  return out;
}

static std::string method_to_str(HttpMethod method) {
  switch (method) {
    case kGet:    return "GET";
    case kPost:   return "POST";
    case kDelete: return "DELETE";
    default:      return "UNKNOWN";
  }
}

MetaVariables MetaVariables::from_request(const Request& request,
                                          const std::string& script_path) {
  MetaVariables env;

  env.set_meta(CgiRequestMethod, method_to_str(request.method));

  // target = path [ "?" query ]
  std::string path = request.target;
  std::string query = "";
  std::size_t q_pos = path.find('?');
  if (q_pos != std::string::npos) {
    query = path.substr(q_pos + 1);
    path = path.substr(0, q_pos);
  }

  env.set_meta(CgiScriptName, path);
  env.set_meta(CgiPathInfo, path);

  env.set_meta(CgiQueryString, query);
  env.set_meta(CgiServerProtocol, "HTTP/1.1");
  env.set_meta(CgiGatewayInterface, "CGI/1.1");
  env.set_meta(CgiScriptFilename, script_path);

  // Content-* は CGI では特別扱い
  if (request.headers.count("content-type")) {
    env.set_meta(CgiContentType, request.headers.at("content-type"));
  }
  
  {
    std::ostringstream oss;
    oss << request.body.size();
    env.set_meta(CgiContentLength, oss.str());
  }

  // HTTPヘッダを CGI の HTTP_* へ変換して渡す
  for (std::map<std::string, std::string>::const_iterator it = request.headers.begin();
       it != request.headers.end(); ++it) {
    const std::string& k = it->first;
    const std::string& v = it->second;

    if (k.empty()) continue;

    // CGIでは CONTENT_TYPE/CONTENT_LENGTH を使うので HTTP_ 版には入れない
    if (k == "content-type" || k == "content-length") continue;

    env.set_raw(to_upper_http_env_key(k), v);
  }

  return env;
}