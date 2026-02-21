#include <string>

std::string to_lower(std::string s) {
  for (size_t i = 0; i < s.length(); ++i) {
    s[i] = std::tolower(static_cast<unsigned char>(s[i]));
  }
  return s;
}
