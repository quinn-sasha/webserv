#!/usr/bin/env python3
import os
import sys

sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")

sys.stdout.write("<html>\n")
sys.stdout.write("<head><title>CGI Test</title></head>\n")
sys.stdout.write("<body>\n")
sys.stdout.write("<h1>CGI is working!</h1>\n")
sys.stdout.write("<h2>Environment Variables:</h2>\n")
sys.stdout.write("<ul>\n")
sys.stdout.write(f"<li>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}</li>\n")
sys.stdout.write(f"<li>SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'N/A')}</li>\n")
sys.stdout.write(f"<li>QUERY_STRING: {os.environ.get('QUERY_STRING', 'N/A')}</li>\n")
sys.stdout.write(f"<li>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'N/A')}</li>\n")
sys.stdout.write("</ul>\n")

if os.environ.get("REQUEST_METHOD") == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        sys.stdout.write("<h2>POST Data:</h2>\n")
        sys.stdout.write(f"<pre>{post_data}</pre>\n")

sys.stdout.write("</body>\n</html>\n")
sys.stdout.flush()