#include "Config.hpp"
#include "config_utils.hpp"
#include <sstream>
#include <fstream>

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
				tokens.push_back(current_word); // token
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
		}
		else if (c == '{' || c == '}' || c == ';') {
			if (!current_word.empty()) {
				tokens.push_back(current_word);
				current_word.clear();
			}
			tokens.push_back(std::string(1, c));
		}
		else {
			current_word += c;
		}
		if (!current_word.empty()) {
			tokens.push_back(current_word);
		}
	}
	return tokens;
}

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
	if (i >= tokens.size() || tokens[i] == ";") {
		error_exit("server_name directive must have at least one value");
	}

	while (i < tokens.size() && tokens[i] != ";") {
		sc.server_names.push_back(tokens[i]);
		i++;
	}

	if (i >= tokens.size() || tokens[i] != ";") {
		error_exit("Expected ';' after server_name values");
	}

	i++;
}

void parse_client_max_body_size_directive(std::vector<std::string>tokens,
				size_t i, ServerContext& sc) {
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

typedef void (*LocationParser)(std::vector<std::string>&, size_t&, LocationContext&);

void parse_location_directive(std::vector<std::string>& tokens, size_t i, ServerContext& sc) {
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
		parsers["root"] = parse_root_wrapper;
		parsers["upload_store"] = parse_upload_store_wrapper;
		parsers["index"] = parse_index_wrapper;
		parsers["allow_methods"] = parse_allow_methods_wrapper;
	}

	while (i < tokens.size() && tokens[i] != "}") {
		std::string key = tokens[i++];
		if (parsers.count(key)) {
			parsers[key](tokens, i, lc);
		} else {
			error_exit("Unknown location directive: " + key);
		}
	}

	// while (i < tokens.size() && tokens[i] != "}") {
	// 	if (tokens[i] == "root") {
	// 		i++;
	// 		if (i >= tokens.size() || tokens[i] == ";")
	// 			error_exit("root needs a value");
	// 		lc.root  = tokens[i];
	// 		i++;
	// 		if (i >= tokens.size() || tokens[i] != ";") {
	// 			error_exit("Expected ';' after root path");
	// 		}
	// 		i++;
	// 	} else if (tokens[i] == "index") {
	// 			i++;
	// 			if (i >= tokens.size() || tokens[i] == ";")
	// 				error_exit("index needs a value");
	// 			while (i < tokens.size() && tokens[i] != ";") {
	// 				lc.index.push_back(tokens[i]);
	// 				i++;
	// 			}
	// 			if (i >= tokens.size() || tokens[i] != ";") {
	// 				error_exit("Expected ';' after index values");
	// 			}
	// 			i++;
	// 	} else if (tokens[i] == "allow_methods") {
	// 			i++;
	// 			if (i >= tokens.size() || tokens[i] == ";")
	// 				error_exit("allow_methods needs a value");
	// 			while (i < tokens.size() && tokens[i] != ";") {
	// 				lc.allow_methods.push_back(tokens[i]);
	// 				i++;
	// 			}
	// 			if (i >= tokens.size() || tokens[i] != ";") {
	// 				error_exit("Expected ';' after allow_methods values");
	// 			}
	// 			i++;
	// 	} else if (tokens[i] == "autoindex") {
	// 			i++;
	// 			if (i >= tokens.size() || tokens[i] == ";")
	// 				error_exit("autoindex needs a value");
	// 			if (tokens[i] != "on" && tokens[i] != "off")
  //         error_exit("autoindex must be 'on' or 'off'");
	// 			lc.autoindex = (tokens[i] == "on");
	// 			i++;
	// 			if (i >= tokens.size() || tokens[i] != ";") {
	// 				error_exit("Expected ';' after autoindex values");
	// 			}
	// 			i++;
	// 	} else if (tokens[i] == "return") {
	// 			i++;
	// 			if (i >= tokens.size() || tokens[i] == ";")
	// 				error_exit("return needs a value");
	// 			char* endptr;
	// 			errno = 0;
	// 			long val = std::strtol(tokens[i].c_str(), &endptr, 10);
	// 			if (errno == ERANGE || *endptr != '\0'
	// 				|| (val != 301 || val != 302 || val != 307 || val != 308)) {
	// 				error_exit("");
	// 			}
	// 			lc.redirect_status_code = val;
	// 			i++;

	// 			if (i < tokens.size() && tokens[i] != ";") {
	// 				lc.redirect_url = tokens[i];
	// 				i++;
	// 			}

	// 			if (i >= tokens.size() || tokens[i] != ";") {
	// 				error_exit("Expected ';' after return values");
	// 			}
	// 			i++;
	// 	} else if (tokens[i] == "upload_store") {
	// 			i++;

	// 			if (i < tokens.size() && tokens[i] != ";") {
	// 				lc.upload_store = tokens[i];
	// 				i++;
	// 			}

	// 			if (i >= tokens.size() || tokens[i] != ";") {
	// 				error_exit("Expected ';' after upload_store values");
	// 			}
	// 			i++;
	// 	} else {
	// 		error_exit("Unknown location directive: " + tokens[i]);
	// 	}
	// }

	// if (i >= tokens.size() || tokens[i] != "}") {
	// 	error_exit("Misssing '}' in location block");
	// }
	// i++;

	// sc.locations.push_back(lc);
}

//リストを順番に読んで、ServerContextに値を代入していく
void parse_server(std::vector<std::string>& tokens, size_t& i) {
	if (tokens[i] != "{") {
		error_exit("Expected '{' after server");
	}
	i++;

	ServerContext sc;

	while (i < tokens.size() && tokens[i] != "}") {
		const std::string& key = tokens[i++];
		if (key == "listen") {
    	parse_listen_directive(tokens, i, sc);
			continue;
		}
		if (tokens[i] == "server_name") {
			parse_server_name_directive(tokens, i, sc);
			continue;
		}
		if (tokens[i] == "client_max_body_size") {
			parse_client_max_body_size_directive(tokens, i, sc);
			continue;
		}
		if (tokens[i] == "location") {
			parse_location_directive(tokens, i, sc);
			continue;
		}

		error_exit("Unknown directive: " + tokens[i]);
	}
	if (i >= tokens.size() || tokens[i] != "}") {
		error_exit("Unexpected end of file: missing '}' in server block");
	}
	// TODO: ここでデフォルト値を補完する ディレクティブがないときとか
	// servers_.push_back(sc);
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

// TODO: if else をたくさんつかっているところを修正する。
// TODO: 設定ファイルにない項目のためにコンストラクタでデフォルト値を入れておく。
// TODO: 実際にリクエストからLocationContextを検索する関数の実装。
