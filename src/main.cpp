#include <cerrno>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "Server.hpp"

int main() {
  try {
    while (true) {
      Server server("8888", 100);  // Temporal setting
      server.run();
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
