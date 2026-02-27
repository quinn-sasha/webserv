#include "Parser.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(RequestLineParse, InvalidRequestLine) {
  Parser parser;
  std::string str;
  str = "GET/t/ HTTP/1.1\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  str = "GET   / HTTP/1.1\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  str = "GET GET / HTTP/1.1\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  str = "\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  str = "GET\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
}

TEST(RequestLineParse, InvalidMethod) {
  Parser parser;
  std::string str;
  str = "Get / HTTP/1.1\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kNotImplemented);
  str = "UNKNOWN / HTTP/1.1\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kNotImplemented);
}

TEST(RequestLineParse, InvalidRequestTarget) {
  {
    Parser parser;
    std::string str = "GET index.html HTTP/1.1\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  }
  {
    Parser parser;
    std::string str = "GET abc/ HTTP/1.1\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  }
  {
    Parser parser;
    std::string str = "GET /index.html / HTTP/1.1\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  }
}

TEST(RequestLineParse, InvalidVersion) {
  {
    Parser parser;
    std::string str = "GET / http/1.1\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  }
  {
    Parser parser;
    std::string str = "GET / 1.1\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  }
  {
    Parser parser;
    std::string str = "GET / HTTP/1.+1\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kBadRequest);
  }
  {
    Parser parser;
    std::string str = "GET / HTTP/2.0\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()),
              kVersionNotSupported);
  }
}

TEST(RequestLineParse, PositiveTest) {
  {
    Parser parser;
    std::string str = "GET / HTTP/1.0\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseContinue);
    EXPECT_EQ(parser.get_request().method, kGet);
    EXPECT_EQ(parser.get_request().target, "/");
    EXPECT_EQ(parser.get_request().version, kHttp10);
  }
  {
    Parser parser;
    std::string str = "GET / HTTP/1.1\r\n";
    EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseContinue);
    EXPECT_EQ(parser.get_request().method, kGet);
    EXPECT_EQ(parser.get_request().target, "/");
    EXPECT_EQ(parser.get_request().version, kHttp11);
  }
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

TEST_F(BodyLengthParse, ContentLengthNotNumber) {
  ParserStatus status;
  std::string str = "Content-Length: abc\r\n\r\n";
  status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(BodyLengthParse, ContentLengthTooBig) {
  ParserStatus status;
  std::string str = "Content-Length: 2147483647000\r\n\r\n";
  status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(BodyLengthParse, ContentLengthNegative) {
  ParserStatus status;
  std::string str = "Content-Length: -42\r\n\r\n";
  status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

class ParseBody : public testing::Test {
 protected:
  Parser parser;

  void SetUp() override {
    std::string str = "GET / HTTP/1.1\r\n";
    parser.parse_request(str.c_str(), str.size());
    str = "Host: example.com\r\n";
    parser.parse_request(str.c_str(), str.size());
  }
};

TEST_F(ParseBody, SmallContentLength) {
  std::string str;
  str = "Content-Length: 5\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello";
  parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseFinished);
  EXPECT_EQ(parser.get_request().body, "hello");
}

// TODO: Need to implement client timeout for the case: Content-Length >
// total body size since server will be waiting forever.
TEST_F(ParseBody, ContentLengthBiggerThanBody) {
  std::string str;
  str = "Content-Length: 10\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseContinue);
  EXPECT_EQ(parser.get_request().body, "hello");
}

TEST_F(ParseBody, ContentLengthSmallerThanBody) {
  std::string str;
  str = "Content-Length: 2\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseFinished);
  EXPECT_EQ(parser.get_request().body, "he");
}

TEST_F(ParseBody, ContentLengthZero) {
  std::string str;
  str = "Content-Length: 0\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseFinished);
  EXPECT_EQ(parser.get_request().body, "");
}

TEST_F(ParseBody, NormalChunked) {
  std::string str;
  str = "Transfer-Encoding: chunked\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "5\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "5\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "0\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseFinished);
  EXPECT_EQ(parser.get_request().body, "hellohello");
}

TEST_F(ParseBody, ChunkedSizeNotNumber) {
  std::string str;
  str = "Transfer-Encoding: chunked\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "xgh\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
  EXPECT_EQ(parser.get_request().body, "");
}

TEST_F(ParseBody, ChunkedSizeOverflow) {
  std::string str;
  str = "Transfer-Encoding: chunked\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "429496729500\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
  EXPECT_EQ(parser.get_request().body, "");
}

TEST_F(ParseBody, ChunkedBodyNoFollowingCrlf) {
  std::string str;
  str = "Transfer-Encoding: chunked\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "5\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello";
  parser.parse_request(str.c_str(), str.size());
  str = "Not Crlf";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kBadRequest);
}

TEST_F(ParseBody, ChunkedBodyTooLarge) {
  std::string str;
  str = "Transfer-Encoding: chunked\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  while (true) {
    str = "10\r\n";
    if (parser.parse_request(str.c_str(), str.size()) == kContentTooLarge) {
      SUCCEED();
      return;
    }
    str = "hellohello\r\n";
    if (parser.parse_request(str.c_str(), str.size()) == kContentTooLarge) {
      SUCCEED();
      return;
    }
  }
  FAIL();
}

TEST_F(ParseBody, ChunkedExtensionDiscarded) {
  std::string str;
  str = "Transfer-Encoding: chunked\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "5;topic=teapot\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "0\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kParseFinished);
  EXPECT_EQ(parser.get_request().body, "hello");
}

TEST_F(ParseBody, ChunkedTrailerDiscarded) {
  std::string str;
  str = "Transfer-Encoding: chunked\r\n\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "5\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "hello\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "0\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "topic=teapot\r\n";
  parser.parse_request(str.c_str(), str.size());
  str = "animal=cat\r\n";
  EXPECT_EQ(parser.parse_request(str.c_str(), str.size()), kParseContinue);
  str = "\r\n";
  ParserStatus status = parser.parse_request(str.c_str(), str.size());
  EXPECT_EQ(status, kParseFinished);
  EXPECT_EQ(parser.get_request().body, "hello");
}
