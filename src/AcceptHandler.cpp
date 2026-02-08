#include "AcceptHandler.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

#include "MonitoredFdHandler.hpp"
#include "Server.hpp"

AcceptHandler::AcceptHandler(int listen_fd, Server& server)
    : listen_fd_(listen_fd), server_(server) {}

// もしaccept()がノンブロックで実行できるなら、
// この処理が呼び出される
HandlerStatus AcceptHandler::handle_input() {
  int client_fd = accept(listen_fd_, NULL, NULL);
  if (client_fd == -1) {
    if (errno == ENFILE || errno == EMFILE) {
      std::cerr << "Server busy (file descriptors limit reached)\n";
      return kContinue;
    }
    if (errno == ECONNABORTED) {  // QUESTION: should I add EINTR here?
      return kContinue;
    }
    return kFatal;
  }
  if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
    close(client_fd);
  }
  server_.register_new_client(client_fd);
  return kContinue;
}
