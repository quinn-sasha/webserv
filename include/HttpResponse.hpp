#ifndef INCLUDE_HTTPRESPONSE_HPP_
#define INCLUDE_HTTPRESPONSE_HPP_

#include <map>
#include <string>

class HttpResponse {
 public:
  HttpResponse();

  void set_status(int code, const std::string& message);
  void add_header(const std::string& key, const std::string& value);
  void set_body(const std::string& body);
  
  std::string to_string() const;

 private:
  int status_code_;
  std::string status_message_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

#endif  // INCLUDE_HTTPRESPONSE_HPP_