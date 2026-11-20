#!/usr/bin/env bash
# Self-tests for the tools/cfw helper scripts (matrix.py, flags.sh, conflicts.sh, reuse.sh).
# Pure shell + git + python3; no firmware toolchain needed. Run: tools/cfw/tests/run.sh
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; cfw="$(dirname "$here")"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "  ok   $1"; }
bad() { fail=$((fail+1)); echo "  FAIL $1"; }
check() { local name="$1"; shift; if "$@"; then ok "$name"; else bad "$name"; fi; }
names() { python3 -c 'import json,sys;print(" ".join(sorted({i["name"] for i in json.load(sys.stdin)["include"]})))'; }
count() { python3 -c 'import json,sys;print(len(json.load(sys.stdin)["include"]))'; }

# ---- fixture registry ------------------------------------------------------------------------
printf 'feature\tflags\tpr\tbranch\tbase_branch\tdepends\tin_main\n' > "$tmp/features.tsv"
printf 'basefeat\tMOD_BASE\t1\tfeature/base\tmain\t-\ty\n'              >> "$tmp/features.tsv"
printf 'parent\tMOD_PARENT\t2\tfeature/parent\tmain\t-\tn\n'             >> "$tmp/features.tsv"
printf 'child\tMOD_CHILD1,MOD_CHILD2\t3\tfeature/child\tfeature/parent\tparent\tn\n' >> "$tmp/features.tsv"
printf 'noflags\t\t4\tfeature/noflags\tmain\t-\tn\n'                       >> "$tmp/features.tsv"
printf '# comment\ninfra/*\n' > "$tmp/infra.txt"
export CFW_FEATURES="$tmp/features.tsv" CFW_INFRA="$tmp/infra.txt"
m() { python3 "$cfw/matrix.py" "$@"; }

echo "flags.sh"
check "dependency closure + sorted flags" test "$("$cfw/flags.sh" child)" = "-DMOD_CHILD1 -DMOD_CHILD2 -DMOD_PARENT"
check "--merged lists only in_main features" test "$("$cfw/flags.sh" --merged)" = "-DMOD_BASE"
check "--stock is empty" test -z "$("$cfw/flags.sh" --stock)"
check "unknown feature fails" bash -c "! '$cfw/flags.sh' nosuch >/dev/null 2>&1"

echo "matrix.py (PR validation)"
out="$(m --pr feature/child 2>/dev/null)"; rc=$?
check "1. registered feature PR succeeds"                     test $rc -eq 0
check "1. ... gets stock, merged, feature, merged+feature"    test "$(echo "$out" | names)" = "child merged merged+child stock"
check "1. ... 4 configs x 3 devices = 12 builds"              test "$(echo "$out" | count)" -eq 12
check "1. ... feature config really carries the feature flags" bash -c "echo '$out' | grep -q '\"name\":\"child\",\"flags\":\"-DMOD_CHILD1 -DMOD_CHILD2 -DMOD_PARENT\"'"
check "1. ... merged+feature = merged flags plus feature"     bash -c "echo '$out' | grep -q '\"merged+child\",\"flags\":\"-DMOD_BASE -DMOD_CHILD1 -DMOD_CHILD2 -DMOD_PARENT\"'"
out="$(m --pr feature/unregistered 2>"$tmp/err")"; rc=$?
check "2. unregistered feature PR FAILS"                       test $rc -ne 0
check "2. ... and emits no matrix (nothing can build green)"   test -z "$out"
check "2. ... with a clear message"                            grep -q "not registered" "$tmp/err"
out="$(m --pr infra/some-tooling 2>/dev/null)"; rc=$?
check "3. infrastructure PR succeeds"                          test $rc -eq 0
check "3. ... gets stock + merged only"                        test "$(echo "$out" | names)" = "merged stock"
check "3. ... 2 configs x 3 devices = 6 builds"                test "$(echo "$out" | count)" -eq 6
check "3. a look-alike prefix is not infra (infrastructure/x)" bash -c "! python3 '$cfw/matrix.py' --pr infrastructure/x >/dev/null 2>&1"
check "registered branch already in main FAILS"                bash -c "! python3 '$cfw/matrix.py' --pr feature/base >/dev/null 2>&1"
check "registered feature with no flags FAILS"                 bash -c "! python3 '$cfw/matrix.py' --pr feature/noflags >/dev/null 2>&1"
check "empty branch name FAILS"                                bash -c "! python3 '$cfw/matrix.py' --pr '' >/dev/null 2>&1"

echo "matrix.py (manual dispatch / integration)"
out="$(m --dispatch 2>/dev/null)"; rc=$?
check "4. dispatch without feature succeeds"                   test $rc -eq 0
check "4. ... stock + merged only (documented behaviour)"      test "$(echo "$out" | names)" = "merged stock"
out="$(m --dispatch parent 2>/dev/null)"; rc=$?
check "4. dispatch with registered feature builds it"          bash -c "[ $rc -eq 0 ] && [ \"$(echo "$out" | names)\" = 'merged merged+parent parent stock' ]"
check "4. dispatch with unknown feature FAILS"                 bash -c "! python3 '$cfw/matrix.py' --dispatch nosuch >/dev/null 2>&1"
check "--integrated is stock + merged"                         test "$(m --integrated | names)" = "merged stock"

echo "matrix.py (the real registry)"
unset CFW_FEATURES CFW_INFRA
check "real registry: this branch is an infrastructure PR"     bash -c "python3 '$cfw/matrix.py' --pr infra/dev-workflow >/dev/null 2>&1"
bad_rows=0
while IFS=$'\t' read -r feat flags pr branch base dep inmain; do
  [ "$inmain" = y ] && continue
  n="$(python3 "$cfw/matrix.py" --pr "$branch" 2>/dev/null | count)"; [ "$n" = 12 ] || { echo "    $branch -> $n"; bad_rows=1; }
done < <(tail -n +2 "$cfw/features.tsv")
check "real registry: every unmerged feature branch gets 12 builds" test $bad_rows -eq 0
check "real registry: unregistered branch fails"               bash -c "! python3 '$cfw/matrix.py' --pr feature/not-in-registry >/dev/null 2>&1"

echo "conflicts.sh (strict exit status)"
g="$tmp/repo"; git init -q "$g"; cd "$g"; git config user.email t@t; git config user.name t
printf 'one\ntwo\nthree\n' > a.txt; printf 'x\n' > b.txt; git add .; git commit -qm base
base=$(git rev-parse HEAD)
printf 'one\nMAIN\nthree\n' > a.txt; git commit -qam "main change"; mainc=$(git rev-parse HEAD)
git checkout -q -b conflicting "$base"; printf 'one\nFEATURE\nthree\n' > a.txt; git commit -qam conflicting; conf=$(git rev-parse HEAD)
git checkout -q -b clean "$base"; printf 'y\n' > b.txt; git commit -qam clean; cln=$(git rev-parse HEAD)
git update-ref refs/remotes/origin/main "$mainc"; git update-ref refs/remotes/origin/feature/conf "$conf"; git update-ref refs/remotes/origin/feature/clean "$cln"
reg() { printf 'feature\tflags\tpr\tbranch\tbase_branch\tdepends\tin_main\n' > "$tmp/c.tsv"; for r in "$@"; do printf '%s\n' "$r" >> "$tmp/c.tsv"; done; }
conf_row=$'conf\tMOD_C\t1\tfeature/conf\tmain\t-\tn'; clean_row=$'clean\tMOD_K\t2\tfeature/clean\tmain\t-\tn'; miss_row=$'gone\tMOD_G\t3\tfeature/gone\tmain\t-\tn'
export CFW_FEATURES="$tmp/c.tsv"
reg "$clean_row"; "$cfw/conflicts.sh" --strict >"$tmp/o1" 2>&1; rc=$?
check "strict: no conflicts -> exit 0"                         test $rc -eq 0
reg "$conf_row" "$clean_row"; "$cfw/conflicts.sh" --strict >"$tmp/o2" 2>&1; rc=$?
check "strict: a real conflict -> exit 1 (not lost in a subshell)" test $rc -eq 1
check "strict: report names the conflicting file and branch"   bash -c "grep -q 'conf .*a.txt' '$tmp/o2' && grep -q 'clean .*none' '$tmp/o2'"
"$cfw/conflicts.sh" >"$tmp/o3" 2>&1; rc=$?
check "default mode: same conflict still exits 0"              test $rc -eq 0
check "default mode: report still lists every feature"         bash -c "grep -c -E '^(conf|clean) ' '$tmp/o3' | grep -qx 2"
reg "$clean_row" "$miss_row"; "$cfw/conflicts.sh" --strict >"$tmp/o4" 2>&1; rc=$?
check "strict: a missing branch is a failure, not omitted"     test $rc -eq 1
check "strict: missing branch is reported"                     grep -q "BRANCH MISSING" "$tmp/o4"
reg "$conf_row"; "$cfw/conflicts.sh" --strict >/dev/null 2>&1; rc=$?
check "strict works when run from a subdirectory"              bash -c "mkdir -p '$g/sub' && cd '$g/sub' && ! '$cfw/conflicts.sh' --strict >/dev/null 2>&1"
unset CFW_FEATURES

echo "reuse.sh (toolchain detection and strictness)"
r="$tmp/rrepo"; git init -q "$r"; cd "$r"; git config user.email t@t; git config user.name t
mkdir -p core agt/10.3/bin; echo 'int x;' > core/a.cpp; git add core; git commit -qm c
printf '#!/bin/sh\necho "fake-gcc 1.0 (test)"\n' > agt/10.3/bin/arm-none-eabi-gcc; chmod +x agt/10.3/bin/arm-none-eabi-gcc
mkdir -p "$tmp/tc2" "$tmp/tc3"; printf '#!/bin/sh\necho "fake-gcc 2.0 (other)"\n' > "$tmp/tc2/arm-none-eabi-gcc"; chmod +x "$tmp/tc2/arm-none-eabi-gcc"
h="$("$cfw/srchash.sh")"
res="$tmp/res.tsv"
printf '2026-01-01\tabc\t%s\tplus\t-DMOD_A -DMOD_B\tfake-gcc 1.0 (test)\tOK\t0\t5\tcfg\n' "$h" > "$res"
printf '2026-01-01\tabc\t%s\tmini\t-DMOD_A\tunknown\tOK\t0\t5\tcfg\n' "$h" >> "$res"
printf '2026-01-01\tabc\t%s\tplus\tSTOCK\tfake-gcc 1.0 (test)\tFAIL\t1\t5\tcfg\n' "$h" >> "$res"
P="/usr/bin:/bin"
ru() { env -u TOOLCHAIN PATH="$P" "$cfw/reuse.sh" "$@" >"$tmp/ru" 2>&1; [ -n "${CFW_TEST_DEBUG:-}" ] && cat "$tmp/ru"; true; }
ru plus "-DMOD_B -DMOD_A" "$res";                          check "agt/10.3 compiler auto-detected (same as build.sh) -> reuse, flag order ignored" grep -q ^REUSE "$tmp/ru"
ru plus "-DMOD_A" "$res";                                  check "different flags -> rebuild" grep -q NEEDS_BUILD "$tmp/ru"
ru original "-DMOD_A -DMOD_B" "$res";                      check "different target -> rebuild" grep -q NEEDS_BUILD "$tmp/ru"
ru plus "" "$res";                                         check "only a FAILED record exists -> rebuild" grep -q NEEDS_BUILD "$tmp/ru"
env -u TOOLCHAIN PATH="$P" TOOLCHAIN="$tmp/tc2/arm-none-eabi-" "$cfw/reuse.sh" plus "-DMOD_A -DMOD_B" "$res" >"$tmp/ru" 2>&1
check "different compiler version -> rebuild" grep -q NEEDS_BUILD "$tmp/ru"
echo 'int y;' >> core/a.cpp; git commit -qam 'firmware change'
ru plus "-DMOD_A -DMOD_B" "$res";                          check "firmware source changed -> rebuild" grep -q NEEDS_BUILD "$tmp/ru"
git checkout -q HEAD~1 -- core/a.cpp; git commit -qam 'revert firmware change'; echo doc > README.md; git add README.md; git commit -qm doc
ru plus "-DMOD_A -DMOD_B" "$res";                          check "docs-only commit does not invalidate" grep -q ^REUSE "$tmp/ru"
mv agt/10.3/bin/arm-none-eabi-gcc agt/10.3/bin/gone
ru mini "-DMOD_A" "$res";                                  check "compiler not found -> never reuse, even if the record says 'unknown'" grep -q "cannot be verified" "$tmp/ru"
env -u TOOLCHAIN PATH="$P" TOOLCHAIN="$tmp/tc3/arm-none-eabi-" "$cfw/reuse.sh" mini "-DMOD_A" "$res" >"$tmp/ru" 2>&1
check "bogus TOOLCHAIN -> never reuse" grep -q "cannot be verified" "$tmp/ru"

echo
echo "cfw self-tests: $pass passed, $fail failed"
[ $fail -eq 0 ]
