#include "AcceptHandler.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>

#include "MonitoredFdHandler.hpp"
#include "Server.hpp"

AcceptHandler::AcceptHandler(int listen_fd, Server& server,
                             const std::string& addr, const std::string& port)
    : listen_fd_(listen_fd), server_(server), addr_(addr), port_(port) {}

// もしaccept()がノンブロックで実行できるなら、
// この処理が呼び出される
HandlerStatus AcceptHandler::handle_input() {
  int client_fd = accept(listen_fd_, NULL, NULL);
  if (client_fd == -1) {
    return kHandlerContinue;
  }
  if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
    return kHandlerFatalError;
  }
  server_.register_new_client(client_fd, addr_, port_);
  return kHandlerAccepted;
}
