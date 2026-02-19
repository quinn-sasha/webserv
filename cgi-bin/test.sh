#!/bin/sh

printf "Content-Type: text/plain\r\n"
printf "\r\n"
printf "=== Shell CGI ===\n"
printf "Method : %s\n" "$REQUEST_METHOD"
printf "Path   : %s\n" "$PATH_INFO"
printf "Query  : %s\n" "$QUERY_STRING"