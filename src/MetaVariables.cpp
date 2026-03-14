#include "MetaVariables.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <vector>

MetaVariables::MetaVariables() {}
MetaVariables::~MetaVariables() {}

MetaVariables::MetaVariables(const MetaVariables& other)
    : request_method_(other.request_method_),
      query_string_(other.query_string_),
      content_length_(other.content_length_),
      content_type_(other.content_type_),
      meta_variables_(other.meta_variables_),
      http_headers_(other.http_headers_) {}

MetaVariables& MetaVariables::operator=(const MetaVariables& other) {
  if (this != &other) {
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
                                          const std::string& script_uri,
                                          const std::string& query_string,
                                          const std::string& server_name,
                                          const std::string& server_port,
                                          const std::string& remote_addr) {
  MetaVariables env;

  env.request_method_ = method_to_str(request.method);
  env.query_string_ = query_string;

  std::string full_path = request.target;
  std::size_t q_pos = full_path.find('?');
  if (q_pos != std::string::npos) {
    full_path = full_path.substr(0, q_pos);
  }

  std::string script_name = full_path;
  std::string path_info = "";

  if (!script_uri.empty() &&
      full_path.size() >= script_uri.size() &&
      full_path.compare(0, script_uri.size(), script_uri) == 0) {
    script_name = script_uri;
    path_info = full_path.substr(script_uri.size());
  }

  if (request.headers.count("content-type")) {
    env.content_type_ = request.headers.at("content-type");
  }

  if (request.headers.count("content-length")) {
    env.content_length_ = request.headers.at("content-length");
  }

  env.meta_variables_["SCRIPT_NAME"] = script_name;
  env.meta_variables_["PATH_INFO"] = path_info;
  env.meta_variables_["SERVER_PROTOCOL"] = "HTTP/1.1";
  env.meta_variables_["GATEWAY_INTERFACE"] = "CGI/1.1";
  env.meta_variables_["SERVER_SOFTWARE"] = "webserv/1.0";
  env.meta_variables_["SERVER_NAME"] = server_name;
  env.meta_variables_["SERVER_PORT"] = server_port;
  if (!remote_addr.empty()) {
    env.meta_variables_["REMOTE_ADDR"] = remote_addr;
  }
  env.meta_variables_["REDIRECT_STATUS"] = "200";

  for (std::map<std::string, std::string>::const_iterator it =
           request.headers.begin(); it != request.headers.end(); ++it) {
    const std::string& k = it->first;
    const std::string& v = it->second;
    if (k.empty()) {
      continue;
    }
    if (k == "content-type" || k == "content-length") {
      continue;
    }
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

  add_to_list(env_list, "REQUEST_METHOD", request_method_);
  add_to_list(env_list, "QUERY_STRING", query_string_);
  add_to_list(env_list, "CONTENT_LENGTH", content_length_);
  add_to_list(env_list, "CONTENT_TYPE", content_type_);

  for (std::map<std::string, std::string>::const_iterator it = meta_variables_.begin();
       it != meta_variables_.end(); ++it) {
    add_to_list(env_list, it->first, it->second);
  }

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
