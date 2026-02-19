#!/usr/bin/perl
use strict;
use warnings;

my $body = "";
my $cl = $ENV{CONTENT_LENGTH} || 0;
if ($cl > 0) {
  read(STDIN, $body, $cl);
}

print "Content-Type: text/plain\r\n";
print "\r\n";
print "=== Perl CGI ===\n";
print "Method : $ENV{REQUEST_METHOD}\n";
print "Path   : $ENV{PATH_INFO}\n";
print "Query  : $ENV{QUERY_STRING}\n";
print "Body   : $body\n";