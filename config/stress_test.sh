#!/bin/bash

# カラー設定
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m'

TARGET_URL="http://localhost:8080/cgi-bin/test.py"
CONCURRENCY=50
DURATION="10s"

echo -e "${CYAN}===============================================${NC}"
echo -e "${CYAN}   Webserv Stress & Reliability Test           ${NC}"
echo -e "${CYAN}===============================================${NC}"

# 1. サーバーのPIDを取得 (webservという名前のプロセスを探す)
SERVER_PID=$(pgrep webserv)

if [ -z "$SERVER_PID" ]; then
    echo -e "${YELLOW}Error: webserv process not found. Please start the server first.${NC}"
    exit 1
fi

echo -e "${YELLOW}[Step 1] Initial Memory Usage${NC}"
ps -o rss,vsz,command -p $SERVER_PID | sed 's/^/  /'

echo -e "\n${YELLOW}[Step 2] Launching Siege for $DURATION ($CONCURRENCY users)...${NC}"
#
sleep 5

siege -c $CONCURRENCY -b -t$DURATION $TARGET_URL

echo -e "\n${YELLOW}[Step 3] Post-Stress Memory Usage${NC}"
ps -o rss,vsz,command -p $SERVER_PID | sed 's/^/  /'

echo -e "\n${YELLOW}[Step 4] Checking for Hanging Connections (ESTABLISHED)${NC}"
STUCK_CONNS=$(netstat -an | grep 8080 | grep ESTABLISHED | wc -l)
if [ "$STUCK_CONNS" -eq 0 ]; then
    echo -e "${GREEN}  Perfect: 0 hanging connections found.${NC}"
else
    echo -e "${YELLOW}  Warning: $STUCK_CONNS connections still established.${NC}"
fi

echo -e "\n${CYAN}===============================================${NC}"
echo -e "${CYAN}   Stress Test Completed                       ${NC}"
echo -e "${CYAN}===============================================${NC}"
