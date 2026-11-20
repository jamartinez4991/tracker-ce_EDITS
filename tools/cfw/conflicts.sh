#!/usr/bin/env bash
# conflicts.sh [--strict]
# Read-only. For every feature not yet in main (features.tsv) report:
#   - whether its branch exists on $REMOTE (default origin)
#   - whether its declared base is an ancestor of the branch (i.e. it is rebased on its base)
#   - the files that would conflict if it were merged into the current main (git merge-tree, no checkout)
# A clean result is NOT proof the combined features work: see PR_WORKFLOW.md (integration glue).
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
remote="${REMOTE:-origin}"; main="${MAIN:-$remote/main}"; rc=0
printf '%-30s %-6s %-14s %s\n' FEATURE PR "ON-BASE" "CONFLICTS-WITH-MAIN"
tail -n +2 "$here/features.tsv" | while IFS=$'\t' read -r feat flags pr branch base dep inmain; do
  [ "$inmain" = y ] && continue
  ref="$remote/$branch"; bref="$remote/$base"
  if ! git rev-parse -q --verify "$ref" >/dev/null; then printf '%-30s %-6s %-14s %s\n' "$feat" "#$pr" "-" "branch missing"; continue; fi
  if git merge-base --is-ancestor "$bref" "$ref" 2>/dev/null; then onbase=yes; else onbase="NO(stale)"; fi
  out="$(git merge-tree --write-tree --name-only "$main" "$ref" 2>&1)"; mrc=$?
  if [ $mrc -eq 0 ]; then c="none"; else c="$(echo "$out" | sed -n 's/^CONFLICT (content): Merge conflict in //p' | tr '\n' ' ')"; [ -z "$c" ] && c="$(echo "$out" | tail -1)"; rc=1; fi
  printf '%-30s %-6s %-14s %s\n' "$feat" "#$pr" "$onbase" "$c"
done
[ "${1:-}" = "--strict" ] && exit $rc || exit 0
