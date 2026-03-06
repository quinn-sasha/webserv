// #include <gtest/gtest.h>
#include <iostream>
#include <cassert>
#include "RequestProcessor.hpp"
#include "Parser.hpp"
#include "Config.hpp"

// int main(int argc, char** argv) {
//   ::testing::InitGoogleTest(&argc, argv);
//   return RUN_ALL_TESTS();
// }

// int main() {
//   ServerContext config;
//   LocationContext loc;
//   loc.path = "/";
//   loc.root = "./test_root";
//   loc.allow_methods.push_back("get");
//   loc.index.push_back("index.html");
//   config.locations.push_back(loc);

//   RequestProcessor processor;

//   // normal.ver
//   std::cout << "--- Test 1: GET /index.html---" << std::endl;
//   Request req1;
//   req1.method = kGet;
//   req1.target = "/index.html";
//   req1.version = kHttp11;

//   ProcessorResult res1 = processor.process(kParseFinished, req1, config);
//   std::cout << res1.response.serialize() << std::endl;

//   // error.ver
//   std::cout << "\n--- Test 2: GET /non_existent.html ---" << std::endl;
//   Request req2;
//   req2.method = kGet;
//   req2.target = "/non_existent.html";
//   req2.version = kHttp11;

//   ProcessorResult res2 = processor.process(kParseFinished, req2, config);
//   std::cout << res2.response.serialize() << std::endl;

//   // redirect.ver
//   std::cout << "\n--- Test 3: GET /old-path (Redirect) ---" << std::endl;

//   LocationContext redirect_loc;
//   redirect_loc.path = "/old-path";
//   redirect_loc.redirect_status_code = 301;
//   redirect_loc.redirect_url = "http://google.com";

//   config.locations.push_back(redirect_loc);

//   Request req3;
//   req3.method = kGet;
//   req3.target = "/old-path";
//   req3.version = kHttp11;

//   ProcessorResult res3 = processor.process(kParseFinished, req3, config);
//   std::cout << res3.response.serialize() << std::endl;

//   return 0;

//   // autoindex.ver
//   std::cout << "\n--- Test 4: GET /images/ (Autoindex) ---" << std::endl;

//   LocationContext autoindex_loc;
//   autoindex_loc.path = "/images/";
//   autoindex_loc.root = "./test_root";
//   autoindex_loc.autoindex = true;
//   autoindex_loc.allow_methods.push_back("get");

//   config.locations.push_back(autoindex_loc);

//   Request req4;
//   req4.method = kGet;
//   req4.target = "/images/";
//   req4.version = kHttp11;

//   ProcessorResult res4 = processor.process(kParseFinished, req4, config);
//   std::cout << res4.response.serialize() << std::endl;
// }

int main(int argc, char **argv) {
  (void)argc;
  Config config_parser;
  config_parser.load_file(argv[1]);
  RequestProcessor processor;

  // --- Case 1: 正常系 (Port 8080) ---
  {
    std::cout << "\n--- Test: Normal GET (Port 8080) --- " << std::endl;
    const ServerContext& sc = config_parser.get_config(8080, "localhost");
    Request req;
    req.method = kGet;
    req.target = "/index.html";
    std::cout << processor.process(kParseFinished, req, sc).response.serialize() << std::endl;
  }

  // --- Case 2: リダイレクト (Port 8081) ---
  {
    std::cout << "\n--- Test: Redirect (Port 8081) ---" << std::endl;
    const ServerContext& sc = config_parser.get_config(8081, "redirect.test");
    Request req;
    req.method = kGet;
    req.target = "/old-path";
    std::cout << processor.process(kParseFinished, req, sc).response.serialize() << std::endl;
  }

  // --- Case 3: 404エラー (Port 8082) ---
  {
    std::cout << "\n--- Test: 404 Not Found (Port 8082) ---" << std::endl;
    const ServerContext& sc = config_parser.get_config(8082, "error.test");
    Request req;
    req.method = kGet;
    req.target = "/nothing";
    std::cout << processor.process(kParseFinished, req, sc).response.serialize() << std::endl;
  }

  return 0;
}
