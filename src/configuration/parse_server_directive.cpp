/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_server_directive.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 14:48:42 by ikota             #+#    #+#             */
/*   Updated: 2026/02/26 16:27:30 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "config_utils.hpp"
#include "parse_location_directive.hpp"

void parse_listen_directive(std::vector<std::string>& tokens, size_t& i,
                            ServerContext& sc) {
  if (i >= tokens.size() || tokens[i] == ";") {
    error_exit("Empty listen");
  }

  ListenConfig lc;
  std::string val = tokens[i];
  size_t colon_pos = val.find(':');

  if (colon_pos != std::string::npos) {
    // IP:PORT
    lc.address = val.substr(0, colon_pos);
    check_ip_format(lc.address);
    if (lc.address == "localhost") {
      lc.address = "127.0.0.1";
      std::string port_str = val.substr(colon_pos + 1);
      lc.port = static_cast<int>(safe_strtol(port_str, ConfigLimits::kPortMin,
                                             ConfigLimits::kPortMax));
    } else {
      // PORT only
      lc.address = "0.0.0.0";
      lc.port = static_cast<int>(
          safe_strtol(val, ConfigLimits::kPortMin, ConfigLimits::kPortMax));
    }

    sc.listens.push_back(lc);
    i++;

    if (i >= tokens.size() || tokens[i++] != ";") {
      error_exit("Expected ';'");
    }
  }
}

void parse_server_name_directive(std::vector<std::string>& tokens, size_t& i,
                                 ServerContext& sc) {
  set_vector_string(tokens, i, sc.server_names, "server_name");
}

void parse_client_max_body_size_directive(std::vector<std::string>& tokens,
                                          size_t& i, ServerContext& sc) {
  if (i >= tokens.size() || tokens[i] == ";") {
    error_exit("client_max_body_size_directive must have at least one value");
  }

  sc.client_max_body_size = safe_strtol(tokens[i++], 0, __LONG_MAX__);

  if (i >= tokens.size() || tokens[i++] != ";") {
    error_exit("Expected ';' after client_max_body_size values");
  }
}

void parse_server_root_directive(std::vector<std::string>& t, size_t& i,
                                 ServerContext& sc) {
  set_single_string(t, i, sc.server_root, "server root");
}

void parse_server_index_directive(std::vector<std::string>& tokens, size_t& i,
                                  ServerContext& sc) {
  set_vector_string(tokens, i, sc.server_index, "index");
}

void parse_error_page_directive(std::vector<std::string>& tokens, size_t& i,
                                ServerContext& sc) {
  std::vector<int> codes;

  if (i >= tokens.size() || tokens[i] == ";")
    error_exit("error_page directive needs values");

  while (i < tokens.size() && tokens[i] != ";") {
    codes.push_back(std::atoi(tokens[i].c_str()));
    i++;
  }
  if (codes.size() < 2) {
    error_exit("error_page needs at least one code and a path");
  }

  std::string path = tokens[i - 1];
  codes.pop_back();

  for (size_t j = 0; j < codes.size(); ++j) {
    sc.error_pages[codes[j]] = path;
  }

  if (i >= tokens.size() || tokens[i++] != ";") {
    error_exit("Expected ';' after error_page values");
  }
}

typedef void (*LocationParser)(std::vector<std::string>&, size_t&,
                               LocationContext&);

void parse_location_directive(std::vector<std::string>& tokens, size_t& i,
                              ServerContext& sc) {
  LocationContext lc;
  lc.is_exact_match = false;

  if (tokens[i] == "=") {
    lc.is_exact_match = true;
    i++;
  } else if (tokens[i] == "^~") {
    lc.is_exact_match = false;
    i++;
  }

  if (i >= tokens.size() || tokens[i] == "{") {
    error_exit("Location path is missing");
  }
  lc.path = tokens[i];
  i++;

  if (i >= tokens.size() || tokens[i] != "{") {
    error_exit("Expected '{' after location path");
  }
  i++;

  static std::map<std::string, LocationParser> parsers;
  if (parsers.empty()) {
    parsers["root"] = parse_location_root_directive;
    parsers["upload_store"] = parse_upload_store_directive;
    parsers["index"] = parse_location_index_directive;
    parsers["allow_methods"] = parse_allow_methods_directive;
    parsers["autoindex"] = parse_autoindex_directive;
    parsers["return"] = parse_return_directive;
    // parsers["cgi_extension"] = parse_cgi_extension_directive;
  }

  while (i < tokens.size() && tokens[i] != "}") {
    std::string key = tokens[i++];
    if (parsers.count(key)) {
      parsers[key](tokens, i, lc);
    } else {
      error_exit("Unknown location directive: " + key);
    }
  }
}

const LocationContext& ServerContext::get_matching_location(
    const std::string& uri) const {
  const LocationContext* best_match = NULL;
  size_t longest_len = 0;

  for (size_t i = 0; i < locations.size(); ++i) {
    const std::string& path = locations[i].path;
    if (uri.find(path) == 0) {
      bool is_border = false;
      if (uri.length() == path.length()) {
        is_border = true;
      } else if (path[path.length() - 1] == '/') {
        is_border = true;
      } else if (uri[path.length()] == '/') {
        is_border = true;
      }

      if (is_border) {
        if (path.length() > longest_len) {
          longest_len = path.length();
          best_match = &locations[i];
        }
      }
    }
  }
  if (best_match) return *best_match;
}
