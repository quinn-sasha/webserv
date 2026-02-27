#ifndef INCLUDE_CONFIG_HPP_
#define INCLUDE_CONFIG_HPP_

#include <map>
#include <vector>

struct ConfigLimits {
  static const long kPortMin = 0;
  static const long kPortMax = 65535;
  static const long kClientMaxBodyDefault = 1000000;
  static const long kRedirectCodeMin = 300;
  static const long kRedirectCodeMax = 399;
  static const long kMovedPermanently = 301;
  static const long kFound = 302;
  static const long kTemporaryRedirect = 307;
  static const long kPermanentRedirect = 308;
};

struct LocationContext {
  std::string path;
  std::string root;
  std::vector<std::string> allow_methods;
  std::vector<std::string> index;
  bool is_exact_match;
  bool autoindex;
  int redirect_status_code;
  std::string redirect_url;
  std::string upload_store;
  std::string cgi_extension;
  std::string cgi_path;

  LocationContext()
      : path("/"),
        root("./html"),
        is_exact_match(false),
        autoindex(false),
        redirect_status_code(-1) {}
};

struct ListenConfig {
  std::string address;
  int port;
};

struct ServerContext {
  std::vector<ListenConfig> listens;
  std::vector<std::string> server_names;
  long client_max_body_size;
  std::string server_root;
  std::vector<std::string> server_index;
  std::map<int, std::string> error_pages;
  std::vector<LocationContext> locations;

  ServerContext() : client_max_body_size(ConfigLimits::kClientMaxBodyDefault) {}
  const LocationContext& get_matching_location(const std::string& uri) const;
};

class Config {
  std::vector<ServerContext> servers_;
  std::string read_file(const std::string& filepath);
  std::vector<std::string> tokenize(const std::string& content);
  void parse_server(const std::vector<std::string>& tokens, size_t token_index);

 public:
  void load_file(const std::string& filepath);
  const std::vector<ServerContext>& get_configs() { return servers_; }
  const ServerContext& get_config(int port, const std::string& host) const;
};

#endif
