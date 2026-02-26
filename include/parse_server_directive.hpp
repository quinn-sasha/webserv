#ifndef INCLUDE_PARSE_SERVER_CONFIG
#define INCLUDE_PARSE_SERVER_CONFIG

#include <vector>

#include "Config.hpp"

typedef void (*ServerParser)(std::vector<std::string>&, size_t&,
                             ServerContext&);

void parse_listen_directive(std::vector<std::string>& tokens, size_t& i,
                            ServerContext& sc);

void parse_server_name_directive(std::vector<std::string>& tokens, size_t& i,
                                 ServerContext& sc);

void parse_client_max_body_size_directive(std::vector<std::string>& tokens,
                                          size_t& i, ServerContext& sc);

void parse_server_root_directive(std::vector<std::string>& t, size_t& i,
                                 ServerContext& sc);

void parse_server_index_directive(std::vector<std::string>& tokens, size_t& i,
                                  ServerContext& sc);

void parse_error_page_directive(std::vector<std::string>& tokens, size_t& i,
                                ServerContext& sc);

void parse_location_directive(std::vector<std::string>& tokens, size_t& i,
                              ServerContext& sc);

#endif
