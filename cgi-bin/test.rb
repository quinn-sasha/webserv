#!/usr/bin/env ruby

body = ""
cl = (ENV["CONTENT_LENGTH"] || "0").to_i
body = STDIN.read(cl) if cl > 0

print "Content-Type: text/plain\r\n"
print "\r\n"
print "=== Ruby CGI ===\n"
print "Method : #{ENV['REQUEST_METHOD']}\n"
print "Path   : #{ENV['PATH_INFO']}\n"
print "Query  : #{ENV['QUERY_STRING']}\n"
print "Body   : #{body}\n"