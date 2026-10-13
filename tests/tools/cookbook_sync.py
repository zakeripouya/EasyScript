#!/usr/bin/env python3
"""Checks that every cookbook recipe page shows its program, its data and its
output exactly as the files the tests run (docs/cookbook/SECTOR/NAME.md beside
NAME.es, NAME.out and the csv files NAME.es reads). Prints one line per
problem and exits 1 if there are any."""
import os, re, sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "cookbook")
problems = []
for sector in sorted(os.listdir(root)):
    d = os.path.join(root, sector)
    if not os.path.isdir(d):
        continue
    for f in sorted(os.listdir(d)):
        if not f.endswith(".es"):
            continue
        name = f[:-3]
        page = os.path.join(d, name + ".md")
        if not os.path.exists(page):
            problems.append(f"{sector}/{name}: no {name}.md")
            continue
        blocks = re.findall(r"```\n(.*?)```", open(page).read(), re.S)
        es = open(os.path.join(d, f)).read()
        wanted = [(f, es), (name + ".out", open(os.path.join(d, name + ".out")).read())]
        wanted += [(c, open(os.path.join(d, c)).read()) for c in sorted(os.listdir(d))
                   if c.endswith(".csv") and f'"{c}"' in es]
        shown = [b.rstrip("\n") for b in blocks]
        for what, text in wanted:
            if text and text.rstrip("\n") not in shown:
                problems.append(f"{sector}/{name}.md doesn't show {what} exactly")
        index = os.path.join(d, "README.md")
        if not os.path.exists(index) or f"({name}.md)" not in open(index).read():
            problems.append(f"{sector}/README.md doesn't list {name}.md")
for p in problems:
    print(p)
sys.exit(1 if problems else 0)
