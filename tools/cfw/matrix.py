#!/usr/bin/env python3
"""matrix.py --integrated | --pr <head-branch>   -> GitHub Actions matrix JSON on stdout.

Configurations come from features.tsv, not from enumerating every flag combination:
  stock            no custom flags (stock regression)
  merged           every feature already in main (the integrated configuration)
  <feature>        the PR's own feature plus its declared dependencies   (PR runs only)
  merged+<feature> everything in main plus the PR's feature              (PR runs only)
"""
import csv, json, subprocess, sys, os
here = os.path.dirname(os.path.abspath(__file__))
TARGETS = ["plus", "original", "mini"]

def flags(*args):
    return subprocess.check_output([os.path.join(here, "flags.sh"), *args], text=True).strip()

rows = list(csv.DictReader(open(os.path.join(here, "features.tsv")), delimiter="\t"))
configs = [("stock", flags("--stock")), ("merged", flags("--merged"))]
if len(sys.argv) >= 3 and sys.argv[1] == "--pr":
    branch = sys.argv[2]
    feat = next((r for r in rows if r["branch"] == branch), None)
    if feat and feat["in_main"] != "y":
        name = feat["feature"]
        configs.append((name, flags(name)))
        merged = [r["feature"] for r in rows if r["in_main"] == "y"]
        configs.append(("merged+" + name, flags(*merged, name)))
elif "--integrated" not in sys.argv:
    sys.exit(__doc__)

seen, include = set(), []
for name, fl in configs:
    if fl in seen and name != "stock":
        continue
    seen.add(fl)
    for t in TARGETS:
        include.append({"name": name, "flags": fl, "target": t})
print(json.dumps({"include": include}, separators=(",", ":")))
