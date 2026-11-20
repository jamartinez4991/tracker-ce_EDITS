#!/usr/bin/env bash
# report.sh <results.tsv>  -> Markdown table of build records (for $GITHUB_STEP_SUMMARY or the ledger).
echo '| Date | Commit | Source hash | Target | Flags | Toolchain | Result | Err | Warn | Config |'
echo '|---|---|---|---|---|---|---|---|---|---|'
grep -v '^#' "$1" | while IFS=$'\t' read -r d c h t f tc r e w n; do
  tcs="$(echo "$tc" | sed -E 's/.*\((.*)\) ([0-9.]+ [0-9]+).*/\1 \2/')"
  echo "| $d | \`$c\` | \`$h\` | $t | \`${f}\` | ${tcs} | $r | $e | $w | $n |"
done
