#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "Server.hpp"
#include "config_utils.hpp"
#include "signal_utils.hpp"

int main(int argc, char* argv[]) {
  std::string config_path;

  if (argc == 2) {
    config_path = argv[1];
  } else if (argc == 1) {
    config_path = "config/default.conf";
  } else {
    error_exit("Usage: ./webserv configuration_file");
  }
  set_signal_handler(SIGINT, turn_off_running_status);
  set_signal_handler(SIGTERM, turn_off_running_status);
  set_signal_handler(SIGTSTP, turn_off_running_status);
  try {
    std::vector<ListenConfig> listen_configs;
    Server server(config_path);
    server.run();
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
    return EXIT_FAILURE;
  }
}

