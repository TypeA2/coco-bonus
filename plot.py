#!/usr/bin/env python3

import sys
import os
import json
import matplotlib
import matplotlib.axes
import matplotlib.pyplot as plt

assert len(sys.argv) == 4, "Must supply data file paths and output path"

datafiles = [ sys.argv[1], sys.argv[2] ]

for df in datafiles:
    assert df and os.path.isfile(df), f"Executable path must be a file, got {df}"

outfile = sys.argv[3]

assert outfile and not os.path.isfile(outfile), \
    f"Output file {outfile} must not exist"

avg_plot: matplotlib.axes.Axes
sd_plot: matplotlib.axes.Axes
fig, (avg_plot, sd_plot) = plt.subplots(2, 1)

iterations = None
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

    for a, results in data.items():
        averages = []
        sdevs = []

        for avg, sd in results.values():
            averages.append(avg)
            sdevs.append(sd)

        avg_plot.plot(iterations, averages, label=f"{df.split('.')[0]} - {a}")
        sd_plot.plot(iterations, sdevs, label=f"{df.split('.')[0]} - {a}")


def setup_plot(p: matplotlib.axes.Axes, title: str):
    p.set_xlabel("# of iterations")
    p.set_xscale("log", base=2)
    p.set_xticks(iterations)
    
    p.set_ylabel("Duration (ns)")
    p.set_xlim((iterations[0], iterations[-1]))

    p.legend()
    p.grid()

    p.set_title(title)

setup_plot(avg_plot, "Average duration")
setup_plot(sd_plot, "Standard deviation")

plt.subplots_adjust(hspace=0.35)

plt.show()