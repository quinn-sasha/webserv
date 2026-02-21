#include <gtest/gtest.h>

#include "MonitoredFdHandler.hpp"
#include "Parser.hpp"

TEST(RequestLineParse, InvalidRequestLine) {
  Parser parser;
  EXPECT_EQ(parser.parse_request_line("GET/t/ HTTP/1.1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET   / HTTP/1.1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET GET / HTTP/1.1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line(""), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET"), kBadRequest);
}

TEST(RequestLineParse, InvalidMethod) {
  Parser parser;
  EXPECT_EQ(parser.parse_request_line("Get / HTTP/1.1"), kNotImplemented);
  EXPECT_EQ(parser.parse_request_line("UNKNOWN / HTTP/1.1"), kNotImplemented);
}

TEST(RequestLineParse, InvalidRequestTarget) {
  Parser parser;
  EXPECT_EQ(parser.parse_request_line("GET index.html HTTP/1.1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET abc/ HTTP/1.1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET /index.html / HTTP/1.1"),
            kBadRequest);
}

TEST(RequestLineParse, InvalidVersion) {
  Parser parser;
  EXPECT_EQ(parser.parse_request_line("GET / http/1.1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET / 1.1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET / HTTP/1.+1"), kBadRequest);
  EXPECT_EQ(parser.parse_request_line("GET / HTTP/2.0"), kVersionNotSupported);
}

TEST(RequestLineParse, PositiveTest) {
  Parser parser;
  EXPECT_EQ(parser.parse_request_line("GET / HTTP/1.0"), kHandlerContinue);
  EXPECT_EQ(parser.get_request().method, kGet);
  EXPECT_EQ(parser.get_request().target, "/");
  EXPECT_EQ(parser.get_request().version, kHttp10);

  EXPECT_EQ(parser.parse_request_line("GET / HTTP/1.1"), kHandlerContinue);
  EXPECT_EQ(parser.get_request().version, kHttp11);
}
