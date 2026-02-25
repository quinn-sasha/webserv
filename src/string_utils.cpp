#include <cstddef>
#include <list>
#include <string>

std::string to_lower(std::string s) {
  for (size_t i = 0; i < s.length(); ++i) {
    s[i] = std::tolower(static_cast<unsigned char>(s[i]));
  }
  return s;
}

std::list<std::string> split_string(const std::string& target,
                                    const std::string& delimiter) {
  std::list<std::string> result;
  if (target.empty()) {
    return result;
  }
  std::size_t start = 0;
  while (true) {
    std::size_t word_end = target.find(delimiter, start);
    if (word_end == std::string::npos) {
      result.push_back(target.substr(start));
      return result;
    }
    result.push_back(target.substr(start, word_end - start));
    start = word_end + delimiter.size();
  }
}

std::string trim(const std::string& target, std::string to_delete) {
  if (target.empty()) {
    return "";
  }
  std::size_t start = target.find_first_not_of(to_delete);
  std::size_t last = target.find_last_not_of(to_delete);
  if (start == std::string::npos) {
    return "";
  }
  return target.substr(start, last - start + 1);
}
