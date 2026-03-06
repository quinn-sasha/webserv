#include "RequestProcessor.hpp"

#include "Parser.hpp"
#include "Response.hpp"
#include "string_utils.hpp"
#include <sys/stat.h>
#include <dirent.h>

namespace http_constants {
  const char* kHtmlStart    = "<html><head><title>Index of ";
  const char* kTitleEnd     = "</title></head><body><h1>Index of ";
  const char* kHeaderEnd    = "</h1><hr><pre>";
  const char* kHtmlEnd      = "</pre><hr></body></html>";
}

ProcessorResult RequestProcessor::handle_error(ParserStatus status,
                                    const ServerContext& target_config) {
  ProcessorResult result;
  std::string error_page_path = "";

  if (target_config.error_pages.count(status)) {
    error_page_path = target_config.error_pages.at(status);
  }

  result.response.prepare_error_response(status, error_page_path);
  result.next_action = ProcessorResult::kSendResponse;

  return result;
}

ProcessorResult RequestProcessor::handle_redirect(const LocationContext& lc) {
  ProcessorResult result;

  result.response.prepare_redirect_response(lc.redirect_status_code, lc.redirect_url);
  result.next_action = ProcessorResult::kSendResponse;
  return result;
}

bool RequestProcessor::is_method_allowed(HttpMethod method, const LocationContext& lc) {
  std::string request_method = method_to_str(method);
  for (size_t i = 0; i < lc.allow_methods.size(); ++i) {
    if (request_method == lc.allow_methods[i]) {
      return true;
    }
  }
  return false;
}

ProcessorResult RequestProcessor::handle_cgi(const Request& request) {
  ProcessorResult result;
  result.next_action = ProcessorResult::kExecuteCgi;
  if (!request.target.empty() && request.target[0] == '/') {
    result.script_path = request.target.substr(1); // /path/cgi-bin/script.py -> path/cgi-bin/script.py
  }
  else {
    result.script_path = request.target;
  }
  return result;
}

ParserStatus RequestProcessor::errno_to_status(int err_num) {
  switch (err_num) {
    case ENOENT:
    case ENOTDIR:
      return kNotFound;

    case EACCES:
      return kForbidden;

    case ENAMETOOLONG:
      return kUriTooLong;

    default:
      return kInternalServerError;
  }
}

std::string RequestProcessor::find_index_file(const std::string& directory_path, const LocationContext& lc) {
  for (std::vector<std::string>::const_iterator it = lc.index.begin();
        it != lc.index.end(); ++it) {
  // physical_pathの末尾に/が付いているかどうか
    std::string test_path = directory_path;
    if (!test_path.empty() && test_path[test_path.length() - 1] != '/' &&
        !it->empty() && (*it)[0] != '/') {
      test_path.append("/");
    }
    test_path.append(*it);

    struct stat test_s;
    if (stat(test_path.c_str(), &test_s) == 0 && S_ISREG(test_s.st_mode)) {
      return test_path;
    }
  }
  return "";
}

ProcessorResult RequestProcessor::create_autoindex_response(
  const std::string& path, const std::string& target) {

  ProcessorResult result;
  DIR* dir = opendir(path.c_str());
  if (dir == NULL) {
    result.response.prepare_error_response(kForbidden, "");
    result.next_action = ProcessorResult::kSendResponse;
    return result;
  }

  std::string body = http_constants::kHtmlStart;
  body.append(target);
  body.append(http_constants::kTitleEnd);
  body.append(target);
  body.append(http_constants::kHeaderEnd);

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == ".") {
      continue;
    }
    std::string link = name;
    struct stat ent_s;

    std::string full_ent_path = path;
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

  body += http_constants::kHtmlEnd;

  result.response.set_body(body);
  result.response.prepare_success_response(kOk);
  result.next_action = ProcessorResult::kSendResponse;
  return result;
}

ProcessorResult RequestProcessor::handle_directory(const std::string& path, const Request& request,
                                 const LocationContext& lc, const ServerContext& target_config) {
  std::string index_file_path = find_index_file(path, lc);

  if (!index_file_path.empty()) {
    return handle_file(index_file_path, target_config);
  }

  if (lc.autoindex) {
    return create_autoindex_response(path, request.target);
  }

  return handle_error(kForbidden, target_config);
}

ProcessorResult RequestProcessor::handle_file(const std::string& path, const ServerContext& target_config) {
  ProcessorResult result;
  if (!result.response.fill_from_file(path)) {
    if (errno == ENOENT) {
      return handle_error(kNotFound, target_config);
    } else if (errno == EACCES) {
      return handle_error(kForbidden, target_config);
    }
    return handle_error(kInternalServerError, target_config);
  }
  std::string mime = result.response.get_mime_type(path);
  result.response.add_header("Content-Type", mime);
  result.response.prepare_success_response(kOk);
  result.next_action = ProcessorResult::kSendResponse;
  return result;
}

ProcessorResult RequestProcessor::handle_static_file(const Request& request,
                  const LocationContext& lc, const ServerContext& target_config) {
  std::string physical_path = lc.root + request.target;
  std::cout << "DEBUG: Trying to open file: [" << physical_path << "]" << std::endl;
  struct stat s;

  if (stat(physical_path.c_str(), &s) == -1) {
    return handle_error(errno_to_status(errno), target_config);
  }
  if (S_ISDIR(s.st_mode)) {
    return handle_directory(physical_path, request, lc, target_config);
  } else if (S_ISREG(s.st_mode)) {
    return handle_file(physical_path, target_config);
  }

  return handle_error(kForbidden, target_config);
}

// エラーかどうかだけを判定して、status code を書き込み返す（一時的な実装）
ProcessorResult RequestProcessor::process(
    ParserStatus status, const Request& request, const ServerContext& target_config) {
  ProcessorResult result;
  if (status != kParseFinished) {
    return handle_error(status, target_config);
  }
  // construct URI
try {
  //最長一致するLocationContextを取得
  const LocationContext& lc = target_config.get_matching_location(request.target);

  // リダイレクト処理
  if (lc.redirect_status_code != -1) {
    return handle_redirect(lc);
  }

  if (!is_method_allowed(request.method, lc)) {
    return handle_error(kMethodNotAllowed, target_config);
  }

  // CGI? 拡張子の判定する
  if (request.target.find("/cgi-bin/") != std::string::npos) {
    return handle_cgi(request);
  }
  return handle_static_file(request, lc, target_config);
 } catch (const std::exception& e) {
// if not CGI, do some process like fetching file
  return handle_error(kNotFound, target_config);
 }
}
