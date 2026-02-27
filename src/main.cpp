#include <cerrno>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "Server.hpp"
#include "config_utils.hpp"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    error_exit("Usage: ./webserv configuration_file");
  }
  try {
    std::vector<ListenConfig> listen_configs;
    Server server(argv[1]);
    server.run();
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
