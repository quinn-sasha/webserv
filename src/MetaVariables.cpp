#include "MetaVariables.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <vector>

MetaVariables::MetaVariables() {}
MetaVariables::~MetaVariables() {}

MetaVariables::MetaVariables(const MetaVariables& other)
    : meta_variables_(other.meta_variables_),
      http_headers_(other.http_headers_) {}

MetaVariables& MetaVariables::operator=(const MetaVariables& other) {
  if (this != &other) {
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

static std::string set_up_full_path(const Request& request) {
  std::string path = request.target;
  std::size_t pos = path.find('?');
  if (pos != std::string::npos) 
    path = path.substr(0, pos);
  return path;
}

static std::string set_up_http_version(const Request& request) {
  if (request.version == kHttp10)
    return "HTTP/1.0";
  return "HTTP/1.1";
}

static std::string set_up_script_filename(const std::string& script_name) {
  std::string filename = script_name;
  std::size_t slash_pos = filename.find_last_of('/');
  if (slash_pos != std::string::npos) 
    filename = filename.substr(slash_pos + 1);
  return filename;
}

void MetaVariables::set_content_meta_(const Request& request) {
  const bool has_content_length = request.headers.count("content-length") != 0;
  const bool has_transfer_encoding = request.headers.count("transfer-encoding") != 0;

  if (has_content_length && has_transfer_encoding && 
      request.headers.count("content-type")) {
    meta_variables_["CONTENT_TYPE"] = request.headers.at("content-type");
  }

  if (has_content_length) {
    meta_variables_["CONTENT_LENGTH"] = request.headers.at("content-length");
  } else if (has_transfer_encoding) {
    std::ostringstream oss;
    oss << request.body.size();
    meta_variables_["CONTENT_LENGTH"] = oss.str();
  }
}

void MetaVariables::set_http_headers_(const Request& request) {
  for (std::map<std::string, std::string>::const_iterator it =
           request.headers.begin(); it != request.headers.end(); ++it) {
    const std::string& k = it->first;
    const std::string& v = it->second;
    if (k.empty() || k == "content-type" || k == "content-length") {
      continue;
    }
    http_headers_[to_upper_http_env_key(k)] = v;
  }
}

MetaVariables MetaVariables::from_request(const Request& request,
                                          const std::string& script_uri,
                                          const std::string& query_string,
                                          const std::string& server_name,
                                          const std::string& server_port,
                                          const std::string& remote_addr) {
  MetaVariables env;

  std::string full_path = set_up_full_path(request);

  std::string script_name = full_path;
  std::string path_info = "";
  if (!script_uri.empty() && full_path.size() >= script_uri.size() &&
      full_path.compare(0, script_uri.size(), script_uri) == 0) {
    script_name = script_uri;
    path_info = full_path.substr(script_uri.size());
  }

  std::string http_version = set_up_http_version(request);
  std::string script_filename = set_up_script_filename(script_name);

  env.meta_variables_["REQUEST_METHOD"] = method_to_str(request.method);
  env.meta_variables_["QUERY_STRING"] = query_string;
  env.meta_variables_["SCRIPT_NAME"] = script_name;
  env.meta_variables_["PATH_INFO"] = path_info;
  env.meta_variables_["SERVER_PROTOCOL"] = http_version;
  env.meta_variables_["GATEWAY_INTERFACE"] = "CGI/1.1";
  env.meta_variables_["SERVER_SOFTWARE"] = "webserv/1.0";
  env.meta_variables_["SERVER_NAME"] = server_name;
  env.meta_variables_["SERVER_PORT"] = server_port;
  env.meta_variables_["REMOTE_ADDR"] = remote_addr;
  env.meta_variables_["SCRIPT_FILENAME"] = script_filename;
  env.meta_variables_["REDIRECT_STATUS"] = "200";

  env.set_content_meta_(request);
  env.set_http_headers_(request);
  return env;
}

void MetaVariables::add_to_list(std::vector<std::string>& list,
                                const std::string& key,
                                const std::string& value) const {
  if (!value.empty() || key == "QUERY_STRING") {
    list.push_back(key + "=" + value);
  }
}

char** MetaVariables::build_envp() const {
  std::vector<std::string> env_list;

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
