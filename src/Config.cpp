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

}

void parse_location_directive(std::vector<std::string>& tokens, size_t i, ServerContext& sc) {

}

//リストを順番に読んで、ServerContextに値を代入していく
void parse_server(std::vector<std::string>& tokens, size_t& i) {
	if (tokens[i] != "{") {
		error_exit("Expected '{' after server");
	}
	i++;

	ServerContext sc;

	while (i < tokens.size() && tokens[i] != "}") {
		if (tokens[i] == "listen") {
    	i++;
    	parse_listen_directive(tokens, i, sc);
		} else if (tokens[i] == "server_name") {
				i++;
			parse_server_name_directive(tokens, i, sc);
			// TODO: parse server_name
		}	else if (tokens[i] == "location") {
				i++;
			parse_location_directive(tokens, i, sc);
			// TODO parse location
		}
	}
	if (i >= tokens.size() || tokens[i] != "}") {
		error_exit("Unexpected end of file: missing '}' in server block");
	}
	// servers.push_back(sc);
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


//TODO: 設定ファイルにない項目のためにコンストラクタでデフォルト値を入れておく。
//TODO: 文字列をトークンに分ける。
// ポートが指定されてない場合は80番ポートが使用、ポートだけなら
