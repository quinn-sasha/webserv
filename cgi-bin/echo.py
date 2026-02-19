#!/usr/bin/env python3
import os
import sys

cl = int(os.environ.get("CONTENT_LENGTH", "0"))
body = sys.stdin.read(cl) if cl > 0 else ""

print("Content-Type: text/plain\r")
print(f"X-Received-Bytes: {len(body)}\r")
print("\r")
print(f"Received {len(body)} bytes")
print(f"First 100 bytes: {body[:100]}")
print(f"Last  100 bytes: {body[-100:]}")