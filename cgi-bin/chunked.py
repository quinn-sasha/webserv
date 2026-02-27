#!/usr/bin/env python3
import sys
import time

# chunked レスポンスを返すCGI
sys.stdout.write("Transfer-Encoding: chunked\r\n")
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()

chunks = ["Hello, ", "World!", " This", " is", " chunked", " response!"]

for chunk in chunks:
    size = len(chunk)
    sys.stdout.write(f"{size:x}\r\n")  # チャンクサイズ（16進数）
    sys.stdout.write(chunk)
    sys.stdout.write("\r\n")
    sys.stdout.flush()
    time.sleep(0.1)

# 終端チャンク
sys.stdout.write("0\r\n\r\n")
sys.stdout.flush()