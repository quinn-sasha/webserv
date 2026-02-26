#include "parse_location_directive.hpp"

#include "config_utils.hpp"

void parse_location_root_directive(std::vector<std::string>& tokens, size_t& i,
                                   LocationContext& lc) {
  set_single_string(tokens, i, lc.root, "root");
}

void parse_upload_store_directive(std::vector<std::string>& tokens, size_t& i,
                                  LocationContext& lc) {
  set_single_string(tokens, i, lc.upload_store, "upload_store");
}

void parse_location_index_directive(std::vector<std::string>& tokens, size_t& i,
                                    LocationContext& lc) {
  set_vector_string(tokens, i, lc.index, "index");
}

void parse_allow_methods_directive(std::vector<std::string>& tokens, size_t& i,
                                   LocationContext& lc) {
  set_vector_string(tokens, i, lc.allow_methods, "allow_methods");
  for (size_t i = 0; i < lc.allow_methods.size(); ++i) {
    std::transform(lc.allow_methods[i].begin(), lc.allow_methods[i].end(),
                   lc.allow_methods[i].begin(), ::tolower);
    std::string method = lc.allow_methods[i];
    if (method != "get" && method != "post" && method != "delete")
      error_exit("Invalid method " + method + " in allow_methods");
  }
}

void parse_autoindex_directive(std::vector<std::string>& tokens, size_t& i,
                               LocationContext& lc) {
  if (i >= tokens.size() || tokens[i] == ";") {
    error_exit("autoindex needs a value (on/off)");
  }

  if (tokens[i] != "on" && tokens[i] != "off") {
    error_exit("autoindex must be 'on' or 'off'");
  }
  lc.autoindex = (tokens[i] == "on");
  i++;
  if (i >= tokens.size() || tokens[i++] != ";") {
    error_exit("Expected ';' after autoindex values");
  }
}

void parse_return_directive(std::vector<std::string>& tokens, size_t& i,
                            LocationContext& lc) {
  if (i >= tokens.size() || tokens[i] == ";")
    error_exit("return directive needs a status code");

  long val = safe_strtol(tokens[i++], ConfigLimits::kRedirectCodeMin,
                         ConfigLimits::kRedirectCodeMax);

  if (val != ConfigLimits::kMovedPermanently && val != ConfigLimits::kFound &&
      val != ConfigLimits::kTemporaryRedirect &&
      val != ConfigLimits::kPermanentRedirect) {
    error_exit("Unsupported redirect status: " + std::to_string(val));
  }
  lc.redirect_status_code = static_cast<int>(val);

  if (i < tokens.size() && tokens[i] != ";") {
    lc.redirect_url = tokens[i];
    i++;
  }

  if (lc.redirect_url.empty()) {
    error_exit("Return 3xx directive requires a redirection URL");
  }

  if (i >= tokens.size() || tokens[i++] != ";") {
    error_exit("Expected ';' after return values");
  }
}
