#ifndef INCLUDE_PARSE_LOCATION_DIRECTIVE
#define INCLUDE_PARSE_LOCATION_DIRECTIVE

#include <vector>

#include "Config.hpp"

typedef void (*ServerParser)(std::vector<std::string>&, size_t&,
                             ServerContext&);

void parse_location_root_directive(std::vector<std::string>& token, size_t& i,
                                   LocationContext& lc);
void parse_upload_store_directive(std::vector<std::string>& tokens, size_t& i,
                                  LocationContext& lc);
void parse_location_index_directive(std::vector<std::string>& tokens, size_t& i,
                                    LocationContext& lc);
void parse_allow_methods_directive(std::vector<std::string>& tokens, size_t& i,
                                   LocationContext& lc);
void parse_autoindex_directive(std::vector<std::string>& tokens, size_t& i,
                               LocationContext& lc);
void parse_return_directive(std::vector<std::string>& tokens, size_t& i,
                            LocationContext& lc);
// void parse_cgi_extension_directive(std::vector<std::string>& tokens,
// size_t&i, LocationContext& lc);

#endif
