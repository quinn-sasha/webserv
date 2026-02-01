#ifndef INCLUDE_SYSTEMERROR_HPP_
#define INCLUDE_SYSTEMERROR_HPP_

#include <exception>
#include <string>

class SystemError : public std::exception {
  std::string message_;

 public:
  explicit SystemError(const std::string& location) {
    message_ = location + ": " + strerror(errno);
  }
  virtual ~SystemError() throw() {}  // Need this to suppress error
  virtual const char* what() const throw() { return message_.c_str(); }
};

#endif  // INCLUDE_SYSTEMERROR_HPP_
