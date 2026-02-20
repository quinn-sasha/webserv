#include "Config.hpp"
#include "config_utils.hpp"
#include <sstream>
#include <fstream>

// fileの中身を文字列としてすべて読み取る。
std::string read_file(const std::string& filepath) {
	std::ifstream ifs(filepath.c_str());
	if (!ifs) {
		error_exit("Could not open file: " + filepath);
	}
	std::stringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

//ファイルを読み込んで単語のリストにする
std::vector<std::string> tokenize(const std::string& content) {
	std::vector<std::string> tokens;
	std::string current_word;
	//一文字ずつループ
	for (size_t i = 0; i < content.size(); ++i) {
		char c = content[i];
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
	}
	return tokens;
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
			if (i >= tokens.size()) {
				error_exit("Unexpected EOF after listen");
			}
		std::string val = tokens[i];
		size_t colon_pos = val.find(':');
		if (colon_pos != std::string::npos) {
			// TODO:IPアドレスが正しいフォーマットかどうかの確認
			sc.host = val.substr(0, colon_pos);
			std::string ip = val.substr(colon_pos + 1);
		} else {
			sc.port = std::atoi(tokens[i].c_str());
			//TODO: std::strtol()を用いてオーバフローの対策とportの範囲をチェックする
		}
		}
		else if (tokens[i] == "server_name") {
			i++;
			// TODO: parse server_name
		}
		else if (tokens[i] == "location") {
			i++;
			// TODO parse location
		}
		i++;
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
			parse_server(tokens, i);
		}
	}
}


//TODO: 設定ファイルにない項目のためにコンストラクタでデフォルト値を入れておく。
//TODO: 文字列をトークンに分ける。
// IPアドレスのバリデーション　ドットで分割 .が３つあるか　各セグメントが空でないか、数字だけか　数値が0~255かどうか
// ポートの範囲でウェルノウンポート(0 ~ 1023), 登録済みポート(1024~49151), ダイナミック/プライベートポート
// configはどのポートを使えるのか？？
