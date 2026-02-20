#ifndef INCLUDE_CONFIG_HPP_
#define INCLUDE_CONFIG_HPP_

#include <iostream>
#include <vector>
#include <map>

struct LocationContext {
	std::string path;
	std::string root; //物理パス(/tmp/www)
	std::vector<std::string> allow_methods; //許可メソッド
	std::string index; //デフォルトファイル(index.html)
	bool autoindex; //ディレクトリ一覧を表示するか
	std::string redirect_url; //リダイレクト先
	std::string upload_store; //ファイルアップロード先
	std::string cgi_extension; //CGI実行対象の拡張子
	std::string cgi_path; //CGI実行ファイルのパス
};

struct ListenContext {
	std::string address;
	int port;
};

struct ServerContext {
	std::vector<ListenContext> listens;
	std::vector<std::string> server_names;
	long client_max_body_size;
	std::map<int, std::string> error_pages; //エラーコードとパスの対応
	std::vector<LocationContext> locations; //このサーバー内のlocationリスト

	ServerContext()
        : client_max_body_size(1048576) // 1MB
  {}
	const LocationContext& get_matching_location(const std::string& uri) const;
};

class Config {
	std::vector<ServerContext> servers_;
	std::string read_file(const std::string& filepath);
	std::vector<std::string> tokenize(const std::string& content);
	void parse_server(std::vector<std::string>& tokens, size_t& i);
public:
	//設定ファイルを読み込んで、上記の構造体に値を詰めていく
	void load_file(const std::string& filepath);
	//実行部がこのポートに届いたリクエストはどのserver設定を使えばいい？？のためのゲッター
	const ServerContext& get_server_config(int port, const std::string& host_header) const;
};

#endif
