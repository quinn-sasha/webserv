#ifndef INCLUDE_CONFIG_UTILS_HPP_
#define INCLUDE_CONFIG_UTILS_HPP_

#include <vector>
#include <string>

void error_exit(const std::string& msg);
void check_ip_format(const std::string& ip);
long safe_strtol(const std::string& str, long min_val, long max_val);
void set_single_string(const std::vector<std::string>& tokens,
                       size_t token_index, std::string& field,
                       const std::string& directive_name);
void set_vector_string(const std::vector<std::string>& tokens,
                       size_t token_index, std::vector<std::string>& field,
                       const std::string& directive_name);
std::string to_string_long(long v);

#endif
