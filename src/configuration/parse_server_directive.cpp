/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_server_directive.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 14:48:42 by ikota             #+#    #+#             */
/*   Updated: 2026/02/24 16:20:11 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "config_utils.hpp"

typedef void (*ServerParser)(std::vector<std::string>&, size_t&, ServerContext&);

void parse_listen_directive(std::vector<std::string>& tokens, size_t& i, ServerContext& sc) {
  if (i >= tokens.size() || tokens[i] == ";") {
    error_exit("Empty listen");
  }

	ListenContext lc;
  std::string val = tokens[i];
  size_t colon_pos = val.find(':');

  if (colon_pos != std::string::npos) {
    // IP:PORT
    lc.address = val.substr(0, colon_pos);
    if (lc.address == "localhost") {
        lc.address = "127.0.0.1";
    } else if (!is_valid_ip(lc.address)) {
        error_exit("Invalid IP");
    }
		std::string port_str = val.substr(colon_pos + 1);
		if (!is_valid_port(port_str)) {
			error_exit("Invalid port");
		}
		lc.port = std::atoi(port_str.c_str());
  } else {
		// PORT only
			lc.address = "0.0.0.0";
			if (!is_valid_port(val)) {
				error_exit("Invalid port");
			}
		lc.port = std::atoi(val.c_str());
  }

	sc.listens.push_back(lc);
  i++;

  if (i >= tokens.size() || tokens[i] != ";") {
    error_exit("Expected ';'");
  }

	i++;
}

void parse_server_name_directive(std::vector<std::string>& tokens, size_t&i, ServerContext& sc) {
	set_vector_string(tokens, i, sc.server_names, "server_name");
}

void parse_client_max_body_size_directive(std::vector<std::string>&tokens,
				size_t& i, ServerContext& sc) {
	if (i >= tokens.size() || tokens[i] == ";") {
		error_exit("client_max_body_size_directive must have at least one value");
	}

	std::string val = tokens[i];
	char* endptr;
	errno = 0;
	long size = std::strtol(val.c_str(), &endptr, 10);

	if (errno == ERANGE || *endptr != '\0' || size < 0) {
		error_exit("Invalid client_max_body_size: " + val);
	}

	sc.client_max_body_size = size;
	i++;

	if (i >= tokens.size() || tokens[i] != ";") {
		error_exit("Expected ';' after client_max_body_size values");
	}
	i++;
}

void parse_server_root_directive(std::vector<std::string>& t, size_t& i, ServerContext& sc) {
  set_single_string(t, i, sc.server_root, "server root");
}

void parse_server_index_directive(std::vector<std::string>& tokens, size_t& i, ServerContext& sc) {
	set_vector_string(tokens, i, sc.server_index, "index");
}

typedef void (*LocationParser)(std::vector<std::string>&, size_t&, LocationContext&);

void parse_location_directive(std::vector<std::string>& tokens, size_t& i, ServerContext& sc) {
	// 完全一致、優先前方一致、通常前方一致

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
		parsers["root"] 				 = parse_root_directive;
		parsers["upload_store"]  = parse_upload_store_directive;
		parsers["index"] 				 = parse_index_directive;
		parsers["allow_methods"] = parse_allow_methods_directive;
		parsers["autoindex"] 		 = parse_autoindex_directive;
		parsers["return"] 			 = parse_return_directive;
		parsers["cgi_extension"] = parse_cgi_extension_directive;
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

//TODO: void parse_error_page_directive() {
// }
