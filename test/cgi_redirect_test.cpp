#include <gtest/gtest.h>
#include <string>
#include "CgiResponseHandler.hpp"

class CgiRedirectTest : public ::testing::Test {
protected:
    CgiResponseHandler handler;

    CgiRedirectTest() : handler(-1, -1, -1) {}
};

TEST_F(CgiRedirectTest, BasicRedirect) {
    std::string cgi_output = "Location: http://example.com\r\n\r\n";
    std::string response = handler.parse_cgi_output(cgi_output);
    
    EXPECT_NE(response.find("HTTP/1.1 302 Found"), std::string::npos);
    EXPECT_NE(response.find("Location: http://example.com"), std::string::npos);
}

TEST_F(CgiRedirectTest, RedirectWithStatus) {
    std::string cgi_output = "Status: 301 Moved Permanently\r\nLocation: /new-path\r\n\r\n";
    std::string response = handler.parse_cgi_output(cgi_output);
    
    EXPECT_NE(response.find("HTTP/1.1 301 Moved Permanently"), std::string::npos);
    EXPECT_NE(response.find("Location: /new-path"), std::string::npos);
}

TEST_F(CgiRedirectTest, RedirectWithBody) {
    std::string cgi_output = "Location: /internal\r\n\r\nRedirecting...";
    std::string response = handler.parse_cgi_output(cgi_output);
    
    EXPECT_NE(response.find("HTTP/1.1 302 Found"), std::string::npos);
    EXPECT_NE(response.find("Location: /internal"), std::string::npos);
    EXPECT_NE(response.find("Redirecting..."), std::string::npos);
    EXPECT_NE(response.find("Content-Length: 14"), std::string::npos);
}

TEST_F(CgiRedirectTest, InvalidCgiOutput) {
    // Missing both Content-Type and Location
    std::string cgi_output = "X-Header: value\r\n\r\nBody";
    std::string response = handler.parse_cgi_output(cgi_output);
    
    EXPECT_NE(response.find("HTTP/1.1 502 Bad Gateway"), std::string::npos);
}
