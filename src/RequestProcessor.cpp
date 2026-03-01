#include "RequestProcessor.hpp"

#include "Parser.hpp"
#include "Response.hpp"
#include "string_utils.hpp"

// エラーかどうかだけを判定して、status code を書き込み返す（一時的な実装）
ProcesseorResult RequestProcessor::process(
    ParserStatus status, const Request& request, const ServerContext& target_config) {
  ProcesseorResult result;
  if (status != kParseFinished) {
    result.response.prepare_error_response(status /*, request */);
    result.next_action = ProcesseorResult::kSendResponse;
    return result;
  }
  // construct URI
try {
  //最長一致するLocationContextを取得
  const LocationContext& lc = target_config.get_matching_location(request.target);

  // リダイレクト処理
  if (lc.redirect_status_code != -1) {
    result.response.prepare_redirect_response(lc.redirect_status_code, lc.redirect_url);
    result.next_action = ProcesseorResult::kSendResponse;
    return result;
  }

  // 物理パスの構築
  std::string physical_path = lc.root + request.target;

  bool method_allowed = false;
  std::string request_to_str = method_to_str(request.method);
  for (size_t i = 0; i < lc.allow_methods.size(); ++i) {
    if (request_to_str == lc.allow_methods[i]) {
      method_allowed = true;
      break;
    }
  }

  if (!method_allowed) {
    // error_pageのパスを渡す処理も必要！
    result.response.prepare_error_response(kMethodNotAllowed);
    result.next_action = ProcesseorResult::kSendResponse;
    return result;
  }

  // CGI?
  if (request.target.find("/cgi-bin/") != std::string::npos) {
    result.next_action = ProcesseorResult::kExecuteCgi;
    result.script_path = request.target.substr(1); // /path/cgi-bin/script.py -> path/cgi-bin/script.py
    return result;
  }

  // 通常ファイルの処理
  // TODO: physical_pathを使ってファイルを開き、レスポンスを作成する。
  // ファイルの存在確認　physical_pathが指すファイルが実際に存在するか？stat()
  // ディレクトリの場合　lc.indexを探すか、lc.autoindexがonであればファイルリストを作成する処理が必要
  // ファイル読み込み　std::ifstreamなどでファイル内容を読み込みresult.responseのbodyにセットする
  result.response.prepare_success_response();
  result.next_action = ProcesseorResult::kSendResponse;
 } catch (const std::exception& e) {
// if not CGI, do some process like fetching file
// ServerContextにはerror_pageの設定で、指定したファイルを読み込んで返すロジックが必要
  result.response.prepare_error_response(kBadRequest);
  result.next_action = ProcesseorResult::kSendResponse;
 }

  return result;
}
