#ifndef INCLUDE_PARSE_LOCATION_DIRECTIVE
#define INCLUDE_PARSE_LOCATION_DIRECTIVE

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "Config.hpp"

typedef void (*ServerParser)(const std::vector<std::string>&, size_t&,
                             ServerContext&);

void parse_location_root_directive(const std::vector<std::string>& token,
                                   size_t& token_index, LocationContext& lc);
void parse_upload_store_directive(const std::vector<std::string>& tokens,
                                  size_t& token_index, LocationContext& lc);
void parse_location_index_directive(const std::vector<std::string>& tokens,
                                    size_t& token_index, LocationContext& lc);
void parse_allow_methods_directive(const std::vector<std::string>& tokens,
                                   size_t& token_index, LocationContext& lc);
void parse_location_client_max_body_size_directive(const std::vector<std::string>& tokens,
                                   size_t& token_index, LocationContext& lc);
void parse_autoindex_directive(const std::vector<std::string>& tokens,
                               size_t& token_index, LocationContext& lc);
void parse_return_directive(const std::vector<std::string>& tokens,
                            size_t& token_index, LocationContext& lc);

void parse_cgi_handlers_directive(const std::vector<std::string>& tokens,
                            size_t& token_index, LocationContext& lc);

#endif
