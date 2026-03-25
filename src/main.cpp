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
  try {
    std::vector<ListenConfig> listen_configs;
    Server server(config_path);
    server.run();
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
    return EXIT_FAILURE;
  }
}

// #include "RequestProcessor.hpp"

// int main(int argc, char **argv) {
//   (void)argc;
//   Config config_parser;
//   config_parser.load_file(argv[1]);
//   RequestProcessor processor;

//   // --- Case 1: 正常系 (Port 8080) ---
//   {
//     std::cout << "\n--- Test: Normal GET (Port 8080) --- " << std::endl;
//     const ServerContext& sc = config_parser.get_config(8080, "localhost");
//     Request req;
//     req.method = kGet;
//     req.target = "/index.html";
//     std::cout << processor.process(kParseFinished, req, sc).response.serialize() << std::endl;
//   }

//   // --- Case 2: リダイレクト (Port 8081) ---
//   {
//     std::cout << "\n--- Test: Redirect (Port 8081) ---" << std::endl;
//     const ServerContext& sc = config_parser.get_config(8081, "redirect.test");
//     Request req;
//     req.method = kGet;
//     req.target = "/old-path";
//     std::cout << processor.process(kParseFinished, req, sc).response.serialize() << std::endl;
//   }

//   // --- Case 3: 404エラー (Port 8082) ---
//   {
//     std::cout << "\n--- Test: 404 Not Found (Port 8082) ---" << std::endl;
//     const ServerContext& sc = config_parser.get_config(8082, "error.test");
//     Request req;
//     req.method = kGet;
//     req.target = "/nothing";
//     std::cout << processor.process(kParseFinished, req, sc).response.serialize() << std::endl;
//   }

//   // --- Case 3: error_page (Port 8083) ---
//   {
//     std::cout << "\n--- Test: 404 error_page redirect (Port 8083) ---" << std::endl;
//     const ServerContext& sc = config_parser.get_config(8083, "error.test");
//     Request req;
//     req.method = kGet;
//     req.target = "/nothing";
//     std::cout << processor.process(kParseFinished, req, sc).response.serialize() << std::endl;
//   }

//   return 0;
// }
