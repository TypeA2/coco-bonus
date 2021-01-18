#!/usr/bin/env python3

import sys
import os
import json
import matplotlib.pyplot as plt

assert len(sys.argv) == 4, "Must supply data file paths and output path"

datafiles = [ sys.argv[1], sys.argv[2] ]

for df in datafiles:
    assert df and os.path.isfile(df), f"Executable path must be a file, got {df}"

outfile = sys.argv[3]

assert outfile and not os.path.isfile(outfile), \
    f"Output file {outfile} must not exist"

for df in datafiles:
    data = None
    with open(df, "r") as f:
        data = json.load(f)

    iterations = list(map(int, data[list(data.keys())[0]].keys()))

    min_iterations = min(iterations)
    max_iterations = max(iterations)

    print("\nLaTeX output:\n")

    allocators = data.keys()
    print(f"% {df}: " + ", ".join(allocators))

    print("Iterations ", end="")
    for a in allocators:
        print(f"& Mean ({a}, $ns$) & SD ({a}, $ns$) ", end="")

    print("\\\\\n\\hline")

    i = min_iterations
    while i <= max_iterations:
        print(f"{i} ", end="")

        for alloc in allocators:
            mean, sd = data[alloc][str(i)]

            print(f"& {mean} & {sd} ", end="")

        print("\\\\")
        i *= 2
