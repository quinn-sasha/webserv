#!/usr/bin/env python3
import os
import sys

path = "../test_root/sample.png"
size = os.path.getsize(path)

sys.stdout.write("Content-Type: image/png\r\n")
sys.stdout.write("Content-Length: %d\r\n" % size)
sys.stdout.write("\r\n")
sys.stdout.flush()

with open(path, "rb") as f:
    sys.stdout.buffer.write(f.read())
    sys.stdout.buffer.flush()