#include "RequestProcessor.hpp"

#include "Parser.hpp"
#include "Response.hpp"
#include "string_utils.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>

namespace http_constants {
  const char* kHtmlStart    = "<html><head><title>Index of ";
  const char* kTitleEnd     = "</title></head><body><h1>Index of ";
  const char* kHeaderEnd    = "</h1><hr><pre>";
  const char* kHtmlEnd      = "</pre><hr></body></html>";
}

ProcessorResult RequestProcessor::handle_error(ParserStatus status,
                                    const ServerContext& target_config) {
  ProcessorResult result;
  std::string error_page_full_path = "";

  if (target_config.error_pages.count(status)) {
    std::string error_uri = target_config.error_pages.at(status);
    std::string root = "";
    const LocationContext& error_lc = target_config.get_matching_location(error_uri);
    if (error_lc.root.empty()) {
      root = target_config.server_root;
    } else {
      root = error_lc.root;
    }

    if (error_uri[0] != '/') {
      error_uri.insert(0, "/");
    }
    error_page_full_path = root + error_uri;
  }
  
  result.response.prepare_error_response(status, error_page_full_path);
  result.next_action = ProcessorResult::kSendResponse;

  return result;
}

ProcessorResult RequestProcessor::handle_redirect(const LocationContext& lc) {
  ProcessorResult result;

  result.response.prepare_redirect_response(lc.redirect_status_code, lc.redirect_url);
  result.next_action = ProcessorResult::kSendResponse;
  return result;
}

static std::string method_to_str(HttpMethod method) {
  switch (method) {
    case kGet:    return "get";
    case kPost:   return "post";
    case kDelete: return "delete";
    default:      return "unknown";
  }
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

static bool is_cgi_handler(const LocationContext& lc,std::string& path_only,
                            std::string& cgi_path,std::string& script_uri) {

  if (lc.cgi_handlers.empty()) {
    return false;
  }

  for (size_t i = 0; i < lc.cgi_handlers.size(); ++i) {
    const std::string& extension = lc.cgi_handlers[i].extension;
    if (extension.empty()) {
      continue;
    }

    std::size_t pos = path_only.find(extension);
    while (pos != std::string::npos) {
      std::size_t end = pos + extension.size();
      if (end == path_only.size() || path_only[end] == '/') {
        cgi_path = lc.cgi_handlers[i].binary_path;
        script_uri = path_only.substr(0, end);
        return true;
      }
      pos = path_only.find(extension, pos + 1);
    }
  }
  return false;
}

ProcessorResult RequestProcessor::handle_cgi(const std::string& path_only,
                                             const std::string& query_string,
                                             const std::string& cgi_path,
                                             const LocationContext& lc,
                                             const ServerContext& target_config) {
  ProcessorResult result;
  result.next_action = ProcessorResult::kExecuteCgi;
  result.script_uri = path_only;
  result.query_string = query_string;
  result.cgi_path = cgi_path;

  std::string root;
  if (!lc.root.empty()) {
    root = lc.root;
  } else if (!target_config.server_root.empty()) {
    root = target_config.server_root;
  } else {
    root = "./html";
  }

  std::string script_uri = path_only;
  if (!script_uri.empty() && script_uri[0] == '/') {
    script_uri = script_uri.substr(1);
  }

  result.script_path = root + "/" + script_uri;

  if (result.cgi_path.empty()) {
    return handle_error(kInternalServerError, target_config);
  }

  if (access(result.script_path.c_str(), R_OK) != 0) {
    return handle_error(errno_to_status(errno), target_config);
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
  result.response.add_header("Content-Type", "text/html");
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

ProcessorResult RequestProcessor::handle_upload(const Request& request, const std::string& path_only,
  const LocationContext& lc, const ServerContext& target_config) {

  ProcessorResult result;
  std::string save_path;

  if (!lc.upload_store.empty()) {
    size_t last_slash = path_only.find_last_of('/');
    std::string filename;
    if (last_slash == std::string::npos) {
      filename = path_only;
    } else {
      filename = path_only.substr(last_slash + 1);
    }
    save_path = lc.upload_store;
    if (!save_path.empty() && save_path[save_path.length() - 1] != '/') {
      save_path.append("/");
    }
    save_path.append(filename);
  } else {
    save_path = lc.root + path_only;
  }

  std::ofstream ofs(save_path.c_str(), std::ios::binary);

  if (!ofs) {
    return handle_error(errno_to_status(errno), target_config);
  }
  if (!request.body.empty()) {
    ofs.write(request.body.data(), request.body.size());
  }
  ofs.close();

  result.response.prepare_success_response(kCreated);
  result.next_action = ProcessorResult::kSendResponse;
  return result;
}

ProcessorResult RequestProcessor::handle_delete(std::string& path,
  const ServerContext& target_config) {

  ProcessorResult result;

  if (std::remove(path.c_str()) != 0) {
    return handle_error(errno_to_status(errno), target_config);
  }

  result.response.prepare_success_response(kNoContent);
  result.next_action = ProcessorResult::kSendResponse;
  return result;
}

ProcessorResult RequestProcessor::handle_static_file(const Request& request, const std::string& path,
                  const LocationContext& lc, const ServerContext& target_config) {
  std::string physical_path = lc.root + path;
  struct stat s;

  if (stat(physical_path.c_str(), &s) == -1) {
    return handle_error(errno_to_status(errno), target_config);
  }

  if (request.method == kGet) {
    if (S_ISDIR(s.st_mode)) {
    return handle_directory(physical_path, request, lc, target_config);
  } else if (S_ISREG(s.st_mode)) {
      return handle_file(physical_path, target_config);
    }
  }
  else if (request.method == kDelete) {
    if (S_ISDIR(s.st_mode)) {
      return handle_error(kForbidden, target_config);
    } else {
      return handle_delete(physical_path, target_config);
    }
  }
  return handle_error(kForbidden, target_config);
}

ProcessorResult RequestProcessor::process(
    ParserStatus status, const Request& request, const ServerContext& target_config) {
  ProcessorResult result;
  if (status != kParseFinished) {
    return handle_error(status, target_config);
  }
  // construct URI
  const LocationContext& lc = target_config.get_matching_location(request.target);

  if (lc.path == "__NOT_FOUND__") {
    return handle_error(kNotFound, target_config);
  }

  if (lc.redirect_status_code != -1) {
    return handle_redirect(lc);
  }

  if (!is_method_allowed(request.method, lc)) {
    return handle_error(kMethodNotAllowed, target_config);
  }

  long client_max_body_size;
  if (lc.client_max_body_size != -1) {
    client_max_body_size = lc.client_max_body_size;
  } else {
    client_max_body_size = target_config.client_max_body_size;
  }

  if (static_cast<long>(request.body.size()) > client_max_body_size) {
   return handle_error(kContentTooLarge, target_config);
  }

  std::string path_only = request.target;
  std::string query_string = "";
  size_t q_pos = path_only.find("?");
  if (q_pos != std::string::npos) {
    query_string = path_only.substr(q_pos + 1);
    path_only = path_only.substr(0, q_pos);
  }

  std::string cgi_path;
  std::string script_uri;
  if (is_cgi_handler(lc, path_only, cgi_path, script_uri)) {
    return handle_cgi(script_uri, query_string, cgi_path, lc, target_config);
  }

  if (request.method == kPost) {
    return handle_upload(request, path_only, lc, target_config);
  }

  return handle_static_file(request, path_only, lc, target_config);
}

std::string RequestProcessor::get_error_page_path(
    const ServerContext& target_config, ParserStatus status) {

  std::map<int, std::string>::const_iterator it = target_config.error_pages.find(status);
  if (it == target_config.error_pages.end()) {
    return "";
  }

  std::string error_uri = it->second;

  const LocationContext& error_lc = target_config.get_matching_location(error_uri);

  std::string root;

  if (error_lc.path != "__NOT_FOUND__" && !error_lc.root.empty()) {
    root = error_lc.root;
  } else {
    root = target_config.server_root;
  }

  if (!error_uri.empty() && error_uri[0] != '/') {
    root += "/";
  }

  return root + error_uri;
}
