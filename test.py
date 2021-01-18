#!/usr/bin/env python3

import sys
import os
import json
import subprocess

assert len(sys.argv) == 3, "Must supply executable path and output path"

executable = sys.argv[1]

assert executable and os.path.isfile(executable), \
    f"Executable path must be a file, got {executable}"

outfile = sys.argv[2]

assert outfile and not os.path.isfile(outfile), \
    f"Output file {outfile} must not exist"

proc = subprocess.Popen(executable, stdout=subprocess.PIPE, encoding="utf8")

data = {}

current_iterations = None
current_allocator = None
min_iterations = None
for line in proc.stdout:
    line = line.rstrip("\r\n")

    if not line or not line.startswith("!"):
        continue
    
    name, value = line[1:].split("=")

    if name != "name":
        value = float(value)

    if name == "iterations":
        current_iterations = int(value)
        print(f"Iterations: {current_iterations}")

        if not min_iterations:
            min_iterations = current_iterations
    elif name == "mean":
        data[current_allocator][current_iterations][0] = round(value)
    elif name == "sd":
        data[current_allocator][current_iterations][1] = round(value)
    else:
        current_allocator = value

        if not current_allocator in data:
            data[current_allocator] = {}

        data[current_allocator] |= {
            current_iterations: [None, None]
        }

max_iterations = current_iterations

with open(outfile, "w") as f:
    json.dump(data, f)

