#!/usr/bin/env bash
# reuse.sh <target> "<flags>" [results.tsv] [rev]
# Exit 0 and print the matching record if a previous OK result exists for the SAME firmware source
# tree (srchash), target, flag set and toolchain. Otherwise exit 1 (build needed).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
target="${1:?}"; flags="${2-}"; file="${3:-$here/../../docs/custom-firmware/build-results.tsv}"; rev="${4:-HEAD}"
[ -f "$file" ] || { echo "NEEDS_BUILD (no results file)"; exit 1; }
norm() { tr ' ' '\n' <<<"$1" | grep -v '^$' | sort | tr '\n' ' ' | sed 's/ $//'; }
want="$(norm "${flags:-STOCK}")"; [ "$want" = "" ] && want=STOCK
h="$("$here/srchash.sh" "$rev")"
gcc="${TOOLCHAIN:-arm-none-eabi-}gcc"; tcver="$("$gcc" --version 2>/dev/null | head -1 || echo unknown)"
while IFS=$'\t' read -r date commit hash tgt fl tc res err warn name; do
  [ "$hash" = "$h" ] && [ "$tgt" = "$target" ] && [ "$res" = OK ] || continue
  [ "$(norm "$fl")" = "$want" ] || continue
  [ "$tc" = "$tcver" ] || continue
  printf 'REUSE\t%s\n' "$date $commit $hash $tgt $fl warnings=$warn"; exit 0
done < <(grep -v '^#' "$file")
echo "NEEDS_BUILD (no identical result for srchash=$h target=$target flags=$want toolchain=$tcver)"; exit 1
