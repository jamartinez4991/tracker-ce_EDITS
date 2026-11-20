#!/usr/bin/env bash
# hosttests.sh - compile and run every standalone host test under tests/ (tests/*.cpp, tests/*/test.cpp).
# NOTE: these are host-side models/unit tests of isolated logic. They do not execute firmware code.
set -uo pipefail
root="$(git rev-parse --show-toplevel)"; cd "$root"
fail=0; n=0
for t in $(find tests -name '*.cpp' | sort); do
  n=$((n+1)); exe="$(mktemp)"
  if g++ -std=c++20 -Wall -Icore/source -Icore -o "$exe" "$t" 2>/tmp/hosttest.err && "$exe" >/tmp/hosttest.out 2>&1; then
    echo "PASS $t: $(tail -1 /tmp/hosttest.out)"
  else
    echo "FAIL $t"; head -20 /tmp/hosttest.err; tail -5 /tmp/hosttest.out; fail=1
  fi
  rm -f "$exe"
done
echo "host tests: $n run, $([ $fail = 0 ] && echo all passed || echo FAILURES)"
exit $fail
