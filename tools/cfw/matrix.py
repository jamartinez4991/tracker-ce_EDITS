#!/usr/bin/env python3
"""matrix.py --integrated | --pr <head-branch> | --dispatch [<feature>]  -> Actions matrix JSON on stdout.

Configurations come from features.tsv (the authoritative registry), not from enumerating flag combinations:
  stock            no custom flags (stock regression)
  merged           every feature already in main (the integrated configuration)
  <feature>        the feature plus its declared dependencies
  merged+<feature> everything in main plus the feature

Modes (exit status 1 with a message on stderr = validation must FAIL):
  --integrated           stock + merged.
  --pr BRANCH            BRANCH registered in features.tsv and not yet in main -> stock, merged, feature, merged+feature.
                         BRANCH matches a pattern in infra-branches.txt        -> stock, merged only (infrastructure PR).
                         BRANCH registered but already in main                 -> FAIL (stale registry / wrong branch).
                         anything else                                         -> FAIL (unregistered feature branch).
  --dispatch [FEATURE]   manual run. With FEATURE: must be registered and not in main, same configs as --pr.
                         Without FEATURE: stock + merged only (an integration/infrastructure check).
Environment: CFW_FEATURES / CFW_INFRA override the registry files (used by the self-tests).
"""
import csv, fnmatch, json, os, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
FEATURES = os.environ.get("CFW_FEATURES", os.path.join(here, "features.tsv"))
INFRA = os.environ.get("CFW_INFRA", os.path.join(here, "infra-branches.txt"))
TARGETS = ["plus", "original", "mini"]


def die(msg):
    print(("::error title=CFW feature registration::" if os.environ.get("GITHUB_ACTIONS") else "") + "matrix.py: " + msg,
          file=sys.stderr)
    sys.exit(1)


def flags(*args):
    return subprocess.check_output([os.path.join(here, "flags.sh"), *args], text=True).strip()


def infra_patterns():
    if not os.path.exists(INFRA):
        return []
    return [l.strip() for l in open(INFRA) if l.strip() and not l.lstrip().startswith("#")]


def feature_configs(rows, feat):
    name = feat["feature"]
    if feat["in_main"] == "y":
        die(f"feature '{name}' is already marked in_main=y in features.tsv; a PR for it cannot be validated "
            "as a new feature. Fix the registry or use the right branch.")
    if not flags(name):
        die(f"feature '{name}' resolves to no -D flags; refusing to validate it as a stock build")
    merged = [r["feature"] for r in rows if r["in_main"] == "y"]
    return [(name, flags(name)), ("merged+" + name, flags(*merged, name))]


def main():
    argv = sys.argv[1:]
    if not argv:
        sys.exit(__doc__)
    rows = list(csv.DictReader(open(FEATURES), delimiter="\t"))
    configs = [("stock", flags("--stock")),
               ("merged", flags("--merged"))]
    mode = argv[0]
    if mode == "--integrated":
        pass
    elif mode == "--pr":
        branch = argv[1] if len(argv) > 1 else ""
        if not branch:
            die("--pr needs the head branch name")
        feat = next((r for r in rows if r["branch"] == branch), None)
        if feat:
            configs += feature_configs(rows, feat)
        elif any(fnmatch.fnmatch(branch, p) for p in infra_patterns()):
            print(f"matrix.py: '{branch}' is an infrastructure branch (infra-branches.txt): stock + merged only",
                  file=sys.stderr)
        else:
            die(f"branch '{branch}' is not registered in tools/cfw/features.tsv and is not an infrastructure "
                "branch (tools/cfw/infra-branches.txt). A firmware feature PR must be registered so its own flags "
                "are compiled; add the feature to features.tsv (reviewed) or name the branch infra/...")
    elif mode == "--dispatch":
        name = argv[1] if len(argv) > 1 else ""
        if name:
            feat = next((r for r in rows if r["feature"] == name), None)
            if not feat:
                die(f"feature '{name}' is not registered in tools/cfw/features.tsv")
            configs += feature_configs(rows, feat)
        else:
            print("matrix.py: manual dispatch without a feature: stock + merged only", file=sys.stderr)
    else:
        sys.exit(__doc__)

    seen, include = set(), []
    for name, fl in configs:
        if fl in seen and name != "stock":
            continue
        seen.add(fl)
        for t in TARGETS:
            include.append({"name": name, "flags": fl, "target": t})
    print(json.dumps({"include": include}, separators=(",", ":")))


main()
