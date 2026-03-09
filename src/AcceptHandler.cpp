#include "AcceptHandler.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <string>

#include "MonitoredFdHandler.hpp"
#include "Server.hpp"

namespace {
std::string translate_newtwork_addr(const struct sockaddr_in& network_addr) {
  uint32_t addr_int = ntohl(network_addr.sin_addr.s_addr);
  uint8_t octet1 = (addr_int >> 24) & 0xFF;
  uint8_t octet2 = (addr_int >> 16) & 0xFF;
  uint8_t octet3 = (addr_int >> 8) & 0xFF;
  uint8_t octet4 = addr_int & 0xFF;
  char result[16];  // Size of 255.255.255.255 plus null terminated is 16
  std::snprintf(result, sizeof(result), "%u.%u.%u.%u", octet1, octet2, octet3,
                octet4);
  return std::string(result);
}
}  // namespace

AcceptHandler::AcceptHandler(int listen_fd, Server& server,
                             const std::string& addr, const std::string& port)
    : listen_fd_(listen_fd), addr_(addr), port_(port), server_(server) {}

// もしaccept()がノンブロックで実行できるなら、
// この処理が呼び出される
HandlerStatus AcceptHandler::handle_input() {
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &addr_len);
  if (client_fd == -1) {
    return kHandlerContinue;
  }
  if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
    return kHandlerFatalError;
  }  
  std::string client_ip_addr = translate_newtwork_addr(client_addr);
  if (server_.register_new_client(client_fd, addr_, client_ip_addr, port_) ==
      -1) {
    close(client_fd);
    return kHandlerContinue;
  }
  return kHandlerAccepted;
}
