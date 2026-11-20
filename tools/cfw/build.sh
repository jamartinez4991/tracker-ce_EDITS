#!/usr/bin/env bash
# build.sh <plus|original|mini> "<flags>" [name]
# Clean build of one device with CUSTOM_DEFINES=<flags>. Prints and (optionally) appends one TSV record:
#   date commit srchash target flags toolchain result errors warnings name
# Env: TOOLCHAIN (prefix, default: Makefile default), CFW_RESULTS (file to append to),
#      CFW_LOG_DIR (keep the full build log there), JOBS (default nproc)
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
target="${1:?target}"; flags="${2-}"; name="${3:-custom}"
case "$target" in plus|original|mini) ;; *) echo "bad target $target" >&2; exit 2;; esac
root="$(git rev-parse --show-toplevel)"; cd "$root"
tc_args=(); [ -n "${TOOLCHAIN:-}" ] && tc_args=("TOOLCHAIN=$TOOLCHAIN")
gcc="${TOOLCHAIN:-$( [ -x agt/10.3/bin/arm-none-eabi-gcc ] && echo "$root/agt/10.3/bin/arm-none-eabi-" || echo arm-none-eabi-)}gcc"
tcver="$("$gcc" --version 2>/dev/null | head -1 || true)"; [ -n "$tcver" ] || tcver=unknown
log="$(mktemp)"
make clean >/dev/null 2>&1 || true
if make "$target" "${tc_args[@]}" CUSTOM_DEFINES="$flags" -j"${JOBS:-$(nproc)}" >"$log" 2>&1; then res=OK; else res=FAIL; fi
errors=$(grep -cE ' error |\*\*\* ' "$log" || true)
warnings=$(grep -c 'warning:' "$log" || true)
rec="$(date -u +%F)\t$(git rev-parse --short HEAD)\t$("$here/srchash.sh")\t$target\t${flags:-STOCK}\t$tcver\t$res\t$errors\t$warnings\t$name"
printf "$rec\n"
[ -n "${CFW_RESULTS:-}" ] && printf "$rec\n" >> "$CFW_RESULTS"
if [ -n "${CFW_LOG_DIR:-}" ]; then mkdir -p "$CFW_LOG_DIR"; cp "$log" "$CFW_LOG_DIR/$target-$name.log"; fi
[ "$res" = OK ] || { echo "---- last errors ----" >&2; grep -E ' error |\*\*\* ' "$log" | head -20 >&2; }
rm -f "$log"
[ "$res" = OK ]
