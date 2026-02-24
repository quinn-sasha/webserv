#include "Config.hpp"
#include "config_utils.hpp"
#include "parse_config_location.hpp"
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
		lc.allow_methods.push_back("GET");
	}
}

static void finalize_server_context(ServerContext& sc, LocationContext& lc) {
	if (sc.listens.empty()) {
		ListenContext default_listen;
		default_listen.address = "0.0.0.0";
		default_listen.port = 8080;
		sc.listens.push_back(default_listen);
	}

	if (sc.locations.empty()) {
		finalize_location_context(sc, lc);
	}

	if (sc.server_names.empty()) {
		sc.server_names.push_back("");
	}
}

void parse_server(std::vector<std::string>& tokens, size_t& i) {
	if (i>= tokens.size() || tokens[i++] != "{") {
		error_exit("Expected '{' after server");
	}

	ServerContext sc;
	static std::map<std::string, ServerParser> s_parsers;

	if (s_parsers.empty()) {
		s_parsers["listen"] 							= parse_listen_directive;
		s_parsers["server_name"] 					= parse_server_name_directive;
		s_parsers["client_max_body_size"] = parse_client_max_body_size_directive;
		s_parsers["root"] 								= parse_server_root_directive;
		s_parsers["index"] 								= parse_server_index_directive;
		s_parsers["location"] 						= parse_location_directive;
		s_parsers["error_page"]						= parse_error_pages_directive;
	}

	while (i < tokens.size() && tokens[i] != "}") {
		std::string key = tokens[i++];
		if (s_parsers.count(key)) {
			s_parsers[key](tokens, i, sc);
		} else {
			error_exit("Unknown directive: " + key);
		}
	}

	if (i >= tokens.size() || tokens[i] != "}") {
		error_exit("Unexpected end of file: missing '}' in server block");
	}
	// TODO: ここでデフォルト値を補完する ディレクティブがないときとか
	finalize_server_context(ServerContext& sc);
	servers_.push_back(sc);
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

// TODO: 設定ファイルにない項目のためにコンストラクタでデフォルト値を入れておく。
// TODO: 実際にリクエストからLocationContextを検索する関数の実装。
