#ifndef INCLUDE_STRING_UTILS_HPP_
#define INCLUDE_STRING_UTILS_HPP_

#include <list>
#include <string>

std::string to_lower(std::string s);

std::list<std::string> split_string(const std::string& target,
                                    const std::string& delimiter);

std::string trim(const std::string& target, std::string to_delete);

#endif  // INCLUDE_STRING_UTILS_HPP_
