# tools/cfw — custom-firmware helper scripts

Small bash/python helpers used by the CI workflows and by hand. See `docs/custom-firmware/BUILD_VALIDATION.md`.

| Script | Purpose |
|---|---|
| `features.tsv` | Single list of features: flags, PR, branch, base, dependencies, whether in `main`. **Update when a feature merges.** |
| `infra-branches.txt` | Glob patterns identifying infrastructure-only PR branches (`infra/*`); everything else must be in `features.tsv` |
| `baseline.env` | Upstream baseline commit and the source paths that decide build-result reuse |
| `flags.sh` | Print `-D` flags for features (+ dependencies), `--merged`, or `--stock` |
| `matrix.py` | Actions matrix JSON: `--integrated`, `--pr <branch>` (fails for unregistered feature branches), `--dispatch [feature]` |
| `build.sh` | Clean build of one device; emits/appends a result record |
| `srchash.sh`, `reuse.sh` | Source-tree hash; decide whether an earlier result is reusable |
| `newwarn.sh` | Warnings in one build log that are absent from another (stock regression) |
| `report.sh` | Render `build-results.tsv` as Markdown |
| `hosttests.sh` | Compile and run host tests under `tests/` |
| `conflicts.sh` | Read-only dependency / merge-conflict report for open feature branches (`REMOTE=fork` locally); `--strict` exits 1 on any missing, unmergeable or conflicting branch |
| `tests/run.sh` | Self-tests for the scripts above (runs in CI) |

Nothing here merges, rebases, pushes or edits firmware sources.
