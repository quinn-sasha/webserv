#ifndef INCLUDE_SOCKET_HPP_
#define INCLUDE_SOCKET_HPP_

#include <netdb.h>

#include <string>

int inet_connect(const std::string& host, const std::string& service,
                 int socktype);
int inet_listen(const std::string& service, int backlog);

#endif  // INCLUDE_SOCKET_HPP_
