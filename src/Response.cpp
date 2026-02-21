#include "Response.hpp"

#include <string>

#include "Parser.hpp"

std::string Response::serialize() const {
  std::string response;
  if (version == kHttp10) {
    response.append("HTTP/1.0 ");
  } else if (version == kHttp11) {
    response.append("HTTP/1.1 ");
  }
  response.append(status_code);
  response.append(" \n");  // No reason-phrase
  // write header into string
  // write body into string
  return response;
}
