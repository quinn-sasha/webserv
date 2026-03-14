#!/bin/bash

# カラー設定
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${YELLOW}===============================================${NC}"
echo -e "${YELLOW}   Webserv Complete Functionality Test         ${NC}"
echo -e "${YELLOW}===============================================${NC}"

# --- Port 8080: 正常系 (GET, POST, DELETE, Autoindex) ---
echo -e "\n${BLUE}[PORT 8080] Normal Operations${NC}"

# 1. GET
echo -ne "1. GET /get/index.html:      "
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/get/index.html

# 2. POST (Upload)
echo -ne "2. POST /upload/test.txt:    "
curl -s -o /dev/null -w "%{http_code}\n" -X POST -d "Hello Webserv" http://localhost:8080/upload/test.txt

# 3. DELETE
echo -ne "3. DELETE /uploads/test.txt: "
curl -s -o /dev/null -w "%{http_code}\n" -X DELETE http://localhost:8080/uploads/test.txt

# 4. Autoindex
echo -ne "4. GET /images/ (Autoindex): "
curl -s http://localhost:8080/images/ | grep -q "Index of" && echo -e "${GREEN}200 OK (HTML confirmed)${NC}" || echo -e "${RED}Failed${NC}"


# --- Port 8081: Redirect ---
echo -e "\n${BLUE}[PORT 8081] Redirection${NC}"
echo -ne "1. Redirect / -> Google:     "
curl -s -o /dev/null -w "Status: %{http_code}, Location: %{redirect_url}\n" http://localhost:8081/


# --- Port 8082: Error Cases (400-505) ---
echo -e "\n${BLUE}[PORT 8082] Error Handling${NC}"

# 400 Bad Request
echo -ne "400 (Space in URI):          "
echo -e "GET /path with space HTTP/1.1\r\nHost: localhost:8082\r\n\r\n" | nc localhost 8082 | head -n 1 | tr -d '\r'
echo -ne "400 (No leading slash):      "
echo -e "GET hoge HTTP/1.1\r\nHost: localhost:8082\r\n\r\n" | nc localhost 8082 | head -n 1 | tr -d '\r'

# 403 Forbidden (事前準備: chmod 000 ./docs/get/forbid.txt)
echo -ne "403 (Forbidden file):        "
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8082/get/forbid.txt

# 404 Not Found
echo -ne "404 (Not Found):             "
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8082/not_found_path

# 405 Method Not Allowed
echo -ne "405 (Method Not Allowed):    "
curl -s -o /dev/null -w "%{http_code}\n" -X POST http://localhost:8082/get

# 413 Content Too Large
echo -ne "413 (Payload Too Large):     "
curl -s -o /dev/null -w "%{http_code}\n" -d "OverLimitString" http://localhost:8082/get/index.html

# 414 URI Too Long
echo -ne "414 (URI Too Long):          "
curl -s -o /dev/null -w "%{http_code}\n" "http://localhost:8082/$(python3 -c 'print("a"*2100)')"

# 431 Request Header Too Large
echo -ne "431 (Header Too Large):      "
echo -e "GET / HTTP/1.1\r\nHost: localhost:8082\r\nLarge-Header: $(python3 -c 'print("a"*10000)')\r\n\r\n" | nc localhost 8082 | head -n 1 | tr -d '\r'

# 501 Not Implemented
echo -ne "501 (Unknown Method):        "
curl -s -o /dev/null -w "%{http_code}\n" -X UNKNOWN http://localhost:8082/

# 505 Version Not Supported
echo -ne "505 (HTTP/2.0):              "
echo -e "GET / HTTP/2.0\r\nHost: localhost:8082\r\n\r\n" | nc localhost 8082 | head -n 1 | tr -d '\r'

echo -e "\n${YELLOW}===============================================${NC}"
