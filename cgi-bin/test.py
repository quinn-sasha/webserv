#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html\r")
print("\r")
print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>CGI is working!</h1>")
print("<h2>Environment Variables:</h2>")
print("<ul>")
print(f"<li>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}</li>")
print(f"<li>SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'N/A')}</li>")
print(f"<li>QUERY_STRING: {os.environ.get('QUERY_STRING', 'N/A')}</li>")
print(f"<li>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'N/A')}</li>")
print("</ul>")

# POSTデータ表示
if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print(f"<h2>POST Data:</h2>")
        print(f"<pre>{post_data}</pre>")

print("</body>")
print("</html>")