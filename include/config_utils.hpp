#ifndef INCLUDE_CONFIG_UTILS_HPP_
#define INCLUDE_CONFIG_UTILS_HPP_

#include <vector>

void error_exit(const std::string& msg);
void check_ip_format(const std::string& ip);
long safe_strtol(const std::string& str, long min_val, long max_val);
void set_single_string(std::vector<std::string>& tokens, size_t& i,
                       std::string& field, const std::string& directive_name);
void set_vector_string(std::vector<std::string>& tokens, size_t& i,
                       std::vector<std::string> field,
                       const std::string& directive_name);

#endif
