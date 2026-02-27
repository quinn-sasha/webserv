#ifndef INCLUDE_PARSE_SERVER_CONFIG
#define INCLUDE_PARSE_SERVER_CONFIG

#include <vector>

#include "Config.hpp"

typedef void (*ServerParser)(const std::vector<std::string>&, size_t,
                             ServerContext&);

void parse_listen_directive(const std::vector<std::string>& tokens,
                            size_t token_index, ServerContext& sc);

void parse_server_name_directive(const std::vector<std::string>& tokens,
                                 size_t token_index, ServerContext& sc);

void parse_client_max_body_size_directive(
    const std::vector<std::string>& tokens, size_t token_index,
    ServerContext& sc);

void parse_server_root_directive(const std::vector<std::string>& tokens,
                                 size_t token_index, ServerContext& sc);

void parse_server_index_directive(const std::vector<std::string>& tokens,
                                  size_t token_index, ServerContext& sc);

void parse_error_page_directive(const std::vector<std::string>& tokens,
                                size_t token_index, ServerContext& sc);

void parse_location_directive(const std::vector<std::string>& tokens,
                              size_t token_index, ServerContext& sc);

#endif
