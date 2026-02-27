#include <cerrno>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "ListenSocket.hpp"
#include "Server.hpp"

int main() {
  try {
    // TODO: temporal setting. Refer to Cofig object
    std::vector<ListenConfig> listen_configs;
    struct ListenConfig config1 = {"localhost", "8888"};
    listen_configs.push_back(config1);
    Server server(listen_configs, 100);
    server.run();
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
