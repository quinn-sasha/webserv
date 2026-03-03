#include "RequestProcessor.hpp"

#include "Parser.hpp"
#include "Response.hpp"
#include "string_utils.hpp"
#include <sys/stat.h>
#include <dirent.h>

// エラーかどうかだけを判定して、status code を書き込み返す（一時的な実装）
ProcesseorResult RequestProcessor::process(
    ParserStatus status, const Request& request, const ServerContext& target_config) {
  ProcesseorResult result;
  if (status != kParseFinished) {
    handle_error(result, status, target_config);
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
    handle_error(result, kMethodNotAllowed, target_config);
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
  // 物理パスの構築
  std::string physical_path = lc.root + request.target;
  struct stat s;
  if (stat(physical_path.c_str(), &s) == -1) {
    if (errno == ENOENT) {
      handle_error(result, kNotFound, target_config);
    } else if (errno == EACCES) {
      handle_error(result, kForbidden, target_config);
    } else {
      handle_error(result, kBadRequest, target_config);
    }
    return result;
  }

  if (S_ISDIR(s.st_mode)) {
    bool index_found = false;
    // ファイルを探す処理 見つかればphysical_pathを更新して続行、なければautoindex確認
    for (std::vector<std::string>::const_iterator it = lc.index.begin();
          it != lc.index.end(); ++it) {
            // physical_pathの末尾に/が付いているかどうか
      std::string test_path = physical_path;
      if (!test_path.empty() && test_path[test_path.length() - 1] != '/' &&
          !it->empty() && (*it)[0] != '/') {
        test_path.append("/");
      }
      test_path.append(*it);

      struct stat test_s;
      if (stat(test_path.c_str(), &test_s) == 0 && S_ISREG(test_s.st_mode)) {
        physical_path = test_path;
        index_found = true;
        break;
      }
    }

    if (!index_found) {
      if (!lc.autoindex) {
        handle_error(result, kForbidden, target_config);
        return result;
      }
      //ディレクトリ内のファイルをすべて表示する
      //ディレクトリを開いてopendir()中身を取得readdir()して、HTMLを組み立てる 生成した文字列をresult.response.set_body()
      //Response::set_bodyの実装
      //Content_type: autoindexの結果はHTML　レスポンスヘッダーにContent-Type: text/htmlを含める
      DIR* dir = opendir(physical_path.c_str());
      if (dir == NULL) {
        handle_error(result, kForbidden, target_config);
        return result;
      }
      std::string body = "<html><head><title>Index of " + request.target + "</title></head><body>";
      body += "<h1>Index of " + request.target + "</h1><hr><pre>";

      struct dirent* entry;
      while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".") {
          continue;
        }
        std::string link = name;
        struct stat ent_s;
        std::string full_ent_path = physical_path;
        if (!full_ent_path.empty() && full_ent_path[full_ent_path.length() - 1] != '/') {
          full_ent_path += "/";
        }
        full_ent_path += name;
        if (stat(full_ent_path.c_str(), &ent_s) == 0 && S_ISDIR(ent_s.st_mode)) {
          link += "/";
        }
        body += "<a href=\"" + link + "\">" + link + "</a>\n";
      }
      closedir(dir);
      body += "</pre><hr></body></html>";
      result.response.set_body(body);
      result.response.prepare_success_response();
      result.next_action = ProcesseorResult::kSendResponse;
      return result;
    }
  }
  //　最終的なphysical_pathを読み込む
  result.response.fill_from_file(physical_path);
  // 本来はファイルの種類に応じてContent_Typeをセットする
  std::string mime = get_mime_type(physical_path);
  result.response.add_header("Content-Type", mime);
  result.response.prepare_success_response();
  result.next_action = ProcesseorResult::kSendResponse;
  return result;
 } catch (const std::exception& e) {
// if not CGI, do some process like fetching file
  handle_error(result, kBadRequest, target_config);
  return result;
 }
}

int RequestProcessor::status_to_int(ParserStatus status) {
  switch (status) {
    case kBadRequest:                 return 400;
    case kForbidden:                  return 403;
    case kNotFound:                   return 404;
    case kUriTooLong:                 return 414;
    case kVersionNotSupported:        return 505;
    case kNotImplemented:             return 501;
    case kContentTooLarge:            return 413;
    case kRequestHeaderFieldsTooLarge: return 431;
    case kMethodNotAllowed:           return 405;
    case kInternalServerError:        return 500;
    // default:                          return ;
  }
}

void RequestProcessor::handle_error(ProcesseorResult& result, ParserStatus status,
                                    const ServerContext& target_config) {
  int code = status_to_int(status);
  std::string error_page_path = "";

  if (target_config.error_pages.count(code)) {
    error_page_path = target_config.error_pages.at(code);
  }

  result.response.prepare_error_response(status, error_page_path);
  result.next_action = ProcesseorResult::kSendResponse;
}
