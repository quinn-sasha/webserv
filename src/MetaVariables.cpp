#include "MetaVariables.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <vector>

MetaVariables::MetaVariables() {}
MetaVariables::~MetaVariables() {}

MetaVariables::MetaVariables(const MetaVariables& other)
    : script_filename_(other.script_filename_),
      request_method_(other.request_method_),
      query_string_(other.query_string_),
      content_length_(other.content_length_),
      content_type_(other.content_type_),
      meta_variables_(other.meta_variables_),
      http_headers_(other.http_headers_) {}

MetaVariables& MetaVariables::operator=(const MetaVariables& other) {
  if (this != &other) {
    script_filename_ = other.script_filename_;
    request_method_ = other.request_method_;
    query_string_ = other.query_string_;
    content_length_ = other.content_length_;
    content_type_ = other.content_type_;
    meta_variables_ = other.meta_variables_;
    http_headers_ = other.http_headers_;
  }
  return *this;
}

static std::string to_upper_http_env_key(const std::string& header_key) {
  // "User-Agent" -> "HTTP_USER_AGENT"
  std::string out = "HTTP_";
  for (std::size_t i = 0; i < header_key.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(header_key[i]);
    if (c == '-')
      out.push_back('_');
    else
      out.push_back(static_cast<char>(std::toupper(c)));
  }
  return out;
}

static std::string method_to_str(HttpMethod method) {
  switch (method) {
    case kGet:
      return "GET";
    case kPost:
      return "POST";
    case kDelete:
      return "DELETE";
    default:
      return "UNKNOWN";
  }
}

MetaVariables MetaVariables::from_request(const Request& request,
                                          const std::string& script_path) {
  MetaVariables env;

  env.request_method_ = method_to_str(request.method);
  env.script_filename_ = script_path;

  std::string full_path = request.target;
  std::size_t q_pos = full_path.find('?');
  if (q_pos != std::string::npos) {
    env.query_string_ = full_path.substr(q_pos + 1);
    full_path = full_path.substr(0, q_pos);
  } else {
    env.query_string_ = "";
  }

  std::string script_name = full_path;
  std::string path_info = "";
  std::string cgi_prefix = "/cgi-bin/";
  std::size_t cgi_pos = full_path.find(cgi_prefix);
  
  if (cgi_pos != std::string::npos) {
    std::size_t start_search = cgi_pos + cgi_prefix.length();
    std::size_t next_slash = full_path.find('/', start_search);
    if (next_slash != std::string::npos) {
      script_name = full_path.substr(0, next_slash);
      path_info = full_path.substr(next_slash);
    }
  }

  if (request.headers.count("content-type")) {
    env.content_type_ = request.headers.at("content-type");
  }

  {
    std::ostringstream oss;
    oss << request.body.size();
    env.content_length_ = oss.str();
  }

  // --- ② Meta Variables (CGI Standard) ---
  env.meta_variables_["SCRIPT_NAME"] = script_name;
  env.meta_variables_["PATH_INFO"] = path_info;
  env.meta_variables_["SERVER_PROTOCOL"] = "HTTP/1.1";
  env.meta_variables_["GATEWAY_INTERFACE"] = "CGI/1.1";
  env.meta_variables_["SERVER_SOFTWARE"] = "webserv/1.0";
  env.meta_variables_["SERVER_NAME"] = "localhost"; //TODO:correct NAME
  env.meta_variables_["SERVER_PORT"] = "8888"; //TODO:correct PORT
  //env.meta_variables_["REMOTE_ADDR"] = "..."

  for (std::map<std::string, std::string>::const_iterator it =
           request.headers.begin();
       it != request.headers.end(); ++it) {
    const std::string& k = it->first;
    const std::string& v = it->second;

    if (k.empty()) continue;

    if (k == "content-type" || k == "content-length") continue;

    env.http_headers_[to_upper_http_env_key(k)] = v;
  }

  return env;
}

void MetaVariables::add_to_list(std::vector<std::string>& list,
                                const std::string& key,
                                const std::string& value) const {
  if (!value.empty() || key == "QUERY_STRING" || key == "CONTENT_LENGTH") {
    list.push_back(key + "=" + value);
  }
}

char** MetaVariables::build_envp() const {
  std::vector<std::string> env_list;

  // --- Add Special Variables ---
  add_to_list(env_list, "REQUEST_METHOD", request_method_);
  add_to_list(env_list, "SCRIPT_FILENAME", script_filename_);
  add_to_list(env_list, "QUERY_STRING", query_string_);
  add_to_list(env_list, "CONTENT_LENGTH", content_length_);
  add_to_list(env_list, "CONTENT_TYPE", content_type_);

  // --- Add Meta Variables ---
  for (std::map<std::string, std::string>::const_iterator it = meta_variables_.begin();
       it != meta_variables_.end(); ++it) {
    add_to_list(env_list, it->first, it->second);
  }

  // --- Add HTTP Headers ---
  for (std::map<std::string, std::string>::const_iterator it =
           http_headers_.begin();
       it != http_headers_.end(); ++it) {
    add_to_list(env_list, it->first, it->second);
  }

  char** envp = new char*[env_list.size() + 1];
  for (std::size_t i = 0; i < env_list.size(); ++i) {
    envp[i] = new char[env_list[i].size() + 1];
    std::strcpy(envp[i], env_list[i].c_str());
  }
  envp[env_list.size()] = NULL;
  return envp;
}

void MetaVariables::destroy_envp(char** envp) {
  if (!envp) return;
  for (std::size_t i = 0; envp[i] != NULL; ++i) {
    delete[] envp[i];
  }
  delete[] envp;
}
