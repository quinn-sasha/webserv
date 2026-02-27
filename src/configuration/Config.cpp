#include "Config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "config_utils.hpp"
#include "parse_server_directive.hpp"

std::string Config::read_file(const std::string& filepath) {
  std::ifstream ifs(filepath.c_str());
  if (!ifs) {
    error_exit("Could not open file: " + filepath);
  }
  std::stringstream ss;
  ss << ifs.rdbuf();
  return ss.str();
}

std::vector<std::string> Config::tokenize(const std::string& content) {
  std::vector<std::string> tokens;
  std::string current_word;
  for (size_t i = 0; i < content.size(); ++i) {
    char c = content[i];
    if (c == '#') {
      if (!current_word.empty()) {
        tokens.push_back(current_word);
        current_word.clear();
      }
      while (i < content.size() && content[i] != '\n') {
        i++;
      }
      continue;
    }

    if (std::isspace(c)) {
      if (!current_word.empty()) {
        tokens.push_back(current_word);
        current_word.clear();
      }
    } else if (c == '{' || c == '}' || c == ';') {
      if (!current_word.empty()) {
        tokens.push_back(current_word);
        current_word.clear();
      }
      tokens.push_back(std::string(1, c));
    } else {
      current_word.append(1, c);
    }
  }
  if (!current_word.empty()) {
    tokens.push_back(current_word);
  }
  return tokens;
}

static void finalize_location_context(ServerContext& sc, LocationContext& lc) {
  if (lc.root.empty()) {
    lc.root = sc.server_root;
  }

  if (lc.index.empty()) {
    if (!sc.server_index.empty()) {
      lc.index = sc.server_index;
    } else {
      lc.index.push_back("index.html");
    }
  }

  if (lc.allow_methods.empty()) {
    lc.allow_methods.push_back("get");
    lc.allow_methods.push_back("post");
  }
}

static void finalize_server_context(ServerContext& sc) {
  if (sc.listens.empty()) {
    ListenConfig default_listen;
    default_listen.address = "0.0.0.0";
    default_listen.port = 8080;
    sc.listens.push_back(default_listen);
  }

  if (sc.server_names.empty()) {
    sc.server_names.push_back("");
  }

  for (size_t j = 0; j < sc.locations.size(); ++j) {
    finalize_location_context(sc, sc.locations[j]);
  }

  if (sc.locations.empty()) {
    LocationContext default_lc;
    finalize_location_context(sc, default_lc);
    sc.locations.push_back(default_lc);
  }
}

void Config::parse_server(const std::vector<std::string>& tokens, size_t i) {
  if (i >= tokens.size() || tokens[i++] != "{") {
    error_exit("Expected '{' after server");
  }

  ServerContext sc;
  static std::map<std::string, ServerParser> s_parsers;

  if (s_parsers.empty()) {
    s_parsers["listen"] = parse_listen_directive;
    s_parsers["server_name"] = parse_server_name_directive;
    s_parsers["client_max_body_size"] = parse_client_max_body_size_directive;
    s_parsers["root"] = parse_server_root_directive;
    s_parsers["index"] = parse_server_index_directive;
    s_parsers["location"] = parse_location_directive;
    s_parsers["error_page"] = parse_error_page_directive;
  }

  while (i < tokens.size() && tokens[i] != "}") {
    std::string key = tokens[i++];
    if (s_parsers.count(key)) {
      s_parsers[key](tokens, i, sc);
    } else {
      error_exit("Unknown directive: " + key);
    }
  }

  if (i >= tokens.size() || tokens[i++] != "}") {
    error_exit("Unexpected end of file: missing '}' in server block");
  }

  finalize_server_context(sc);
  servers_.push_back(sc);
}

const ServerContext& Config::get_config(int port,
                                        const std::string& host) const {
  std::vector<const ServerContext*> candidates;
  for (size_t i = 0; i < servers_.size(); ++i) {
    for (size_t j = 0; j < servers_[i].listens.size(); ++j) {
      if (servers_[i].listens[j].port == port) {
        candidates.push_back(&servers_[i]);
      }
    }
  }
  if (candidates.empty()) {
    throw std::runtime_error("get_config(): no matching port");
  }
  for (size_t i = 0; i < candidates.size(); ++i) {
    for (size_t j = 0; j < candidates[i]->server_names.size(); ++j) {
      if (candidates[i]->server_names[j] == host) {
        return *candidates[i];
      }
    }
  }
  return *candidates[0];
}

void Config::load_file(const std::string& filepath) {
  std::string content = read_file(filepath);
  std::vector<std::string> tokens = tokenize(content);

  for (size_t i = 0; i < tokens.size(); ++i) {
    if (tokens[i] == "server") {
      i++;
      parse_server(tokens, i);
    }
  }
}
