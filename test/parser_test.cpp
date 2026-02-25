#include "Parser.hpp"

#include <gtest/gtest.h>

#include <string>

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
  EXPECT_EQ(parser.parse_request_line("GET / HTTP/1.0"), kParseContinue);
  EXPECT_EQ(parser.get_request().method, kGet);
  EXPECT_EQ(parser.get_request().target, "/");
  EXPECT_EQ(parser.get_request().version, kHttp10);

  EXPECT_EQ(parser.parse_request_line("GET / HTTP/1.1"), kParseContinue);
  EXPECT_EQ(parser.get_request().version, kHttp11);
}

class HeadersParseTest : public testing::Test {
 protected:
  Parser parser;

  void SetUp() override {
    std::string first_line = "GET / HTTP/1.1\r\n";
    parser.parse_request(first_line.c_str(), first_line.size());
  }
};

TEST_F(HeadersParseTest, ParseValidHeader) {
  std::string str = "Host: webserv\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kParseContinue);
  auto iter = parser.get_request().headers.find("host");
  EXPECT_EQ(iter->second, "webserv");
}

TEST_F(HeadersParseTest, ParseValidHeaders) {
  std::string str = "Host: webserv\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "Content-Length: 100\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "Accept-Language: ja\r\nAccept: image/gif, image/jpeg, */*\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "\r\n";  // end of header
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kParseFinished);
}

TEST_F(HeadersParseTest, InvalidMultipleHosts) {
  std::string str = "Host: webserv\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "HOST: nvidia\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(HeadersParseTest, InvalidMultipleLenthes) {
  std::string str = "Content-Length: 100\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "Content-Length: 42\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(HeadersParseTest, InvalidHeaderName) {
  std::string str = "H ost : example.com\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(HeadersParseTest, InvalidEmptyHeaderName) {
  std::string str = ": example.com\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(HeadersParseTest, ObsoleteLineFolding) {
  std::string str =
      "Subject: This is a subject field\r\n which spans multiple lines.";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

class BodyLengthParse : public testing::Test {
 protected:
  Parser parser;

  void SetUp() override {
    std::string str = "GET / HTTP/1.1\r\n";
    parser.parse_request(str.c_str(), str.size());
    str = "Host: example.com\r\n";
    parser.parse_request(str.c_str(), str.size());
  }
};

TEST(VersionNotSupportEncodings, VersionNotSupportEncodings) {
  Parser parser;
  std::string str = "GET / HTTP/1.0\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "Transfer-Encoding: chunked\r\n";  // Valid
  parser.parse_request(str.c_str(), str.size());
  str = "\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(BodyLengthParse, InvalidEncodingsOrder) {
  ParserStatus status;
  std::string str = "Transfer-Encoding: chunked, gzip\r\n\r\n";
  status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(BodyLengthParse, EmptyEncodings) {
  ParserStatus status;
  std::string str = "Transfer-Encoding:\r\n\r\n";
  status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(BodyLengthParse, NotImplementedEncodings) {
  ParserStatus status;
  std::string str = "Transfer-Encoding: gzip, chunked\r\n\r\n";
  status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kNotImplemented);
}
