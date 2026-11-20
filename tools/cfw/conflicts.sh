#!/usr/bin/env bash
# conflicts.sh [--strict]
# Read-only. For every feature not yet in main (features.tsv) report:
#   - whether its branch exists on $REMOTE (default origin)
#   - whether its declared base is an ancestor of the branch (i.e. it is rebased on its base)
#   - the files that would conflict if it were merged into the current main (git merge-tree, no checkout)
# Default: always prints the full report and exits 0.
# --strict: exits 1 if ANY open feature branch is missing, cannot be merge-tested, or conflicts with main.
# A clean result is NOT proof the combined features work: see PR_WORKFLOW.md (integration glue).
# Env: REMOTE (default origin), MAIN (default $REMOTE/main), CFW_FEATURES (default tools/cfw/features.tsv)
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
tsv="${CFW_FEATURES:-$here/features.tsv}"
cd "$(git rev-parse --show-toplevel)" || exit 2   # merge-tree prints paths relative to cwd
remote="${REMOTE:-origin}"; main="${MAIN:-$remote/main}"
strict=0; [ "${1:-}" = "--strict" ] && strict=1
problems=0
printf '%-30s %-6s %-14s %s\n' FEATURE PR "ON-BASE" "CONFLICTS-WITH-MAIN"
# Process substitution (not a pipe) so $problems is set in this shell, not a subshell.
while IFS=$'\t' read -r feat flags pr branch base dep inmain; do
  [ "$inmain" = y ] && continue
  ref="$remote/$branch"; bref="$remote/$base"
  if ! git rev-parse -q --verify "$ref" >/dev/null; then
    printf '%-30s %-6s %-14s %s\n' "$feat" "#$pr" "-" "BRANCH MISSING ($ref)"; problems=$((problems+1)); continue
  fi
  if git merge-base --is-ancestor "$bref" "$ref" 2>/dev/null; then onbase=yes; else onbase="NO(stale)"; fi
  out="$(git merge-tree --write-tree --name-only "$main" "$ref" 2>&1)"; mrc=$?
  if [ $mrc -eq 0 ]; then
    c="none"
  elif [ $mrc -eq 1 ]; then
    c="$(printf '%s\n' "$out" | awk 'NR>1 && NF==0 {exit} NR>1 {printf "%s ", $0}')"; [ -z "$c" ] && c="conflict (see git merge-tree)"
    problems=$((problems+1))
  else
    c="MERGE TEST FAILED: $(printf '%s\n' "$out" | tail -1)"; problems=$((problems+1))
  fi
  printf '%-30s %-6s %-14s %s\n' "$feat" "#$pr" "$onbase" "$c"
done < <(tail -n +2 "$tsv")
echo "problems: $problems"
if [ $strict -eq 1 ] && [ $problems -gt 0 ]; then exit 1; fi
exit 0
