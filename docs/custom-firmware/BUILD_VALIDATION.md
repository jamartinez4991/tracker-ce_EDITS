# Build validation

How custom-firmware builds are configured, recorded and reused. Helper scripts live in `tools/cfw/`; the
machine-readable record is [`build-results.tsv`](build-results.tsv); feature status is in
[`INTEGRATION_LEDGER.md`](INTEGRATION_LEDGER.md).

## What counts as validation

Four kinds of evidence are kept separate everywhere in this repo. A feature is never "validated" by the first one alone.

| Kind | Meaning | Where recorded |
|---|---|---|
| **Compiled** | `make <target>` links with the given flags, 0 errors | `build-results.tsv`, ledger |
| **Host test** | `tests/**.cpp` pass. These are host-side models of isolated logic; they do not run firmware code | ledger |
| **Source review** | Diff and shared-code review by a human or Claude | ledger / PR report |
| **Hardware** | Run on a real Tracker. Not done unless the ledger says so | ledger |

## Device targets

| Device | `make` target | Notes |
|---|---|---|
| Tracker+ | `plus` | IMX platform, has synth support (`FEATURE_SYNTH`) |
| Original Tracker | `original` | MK66 platform; by far the most warnings (stock ≈ 2.7k) |
| Tracker Mini | `mini` | IMX platform |

`make all` also builds `psyn` (synth plugins); `make ptfx` builds the sample-editor plugins. The custom flags only
affect the three firmware targets, so the validation matrix builds those.

## Configuring flags

Feature flags are plain `-D` defines passed through `CUSTOM_DEFINES`:

```
make plus CUSTOM_DEFINES="-DMOD_MASTER_MULTIPLIER -DMOD_STEP_JUMP"
```

The makefiles do not track flag changes, so **always `make clean` when the flags change** (`tools/cfw/build.sh` does).
`tools/cfw/features.tsv` is the single list of features, flags, branches and dependencies:

```
tools/cfw/flags.sh --merged            # every feature already in main
tools/cfw/flags.sh step-jump           # one feature + its dependencies
tools/cfw/flags.sh --stock             # empty: stock configuration
```

## Toolchain

GNU Arm Embedded Toolchain 10.3-2021.10 (GCC 10.3.1), the same as `build.yml`; the Makefile uses `agt/10.3` when
present. Set `TOOLCHAIN=/path/to/arm-none-eabi-` to use another prefix. Results from a different compiler build are
not interchangeable (warning counts differ), so the toolchain version string is part of every record.

## Building and recording

```
export TOOLCHAIN=...                                   # optional
export CFW_RESULTS=docs/custom-firmware/build-results.tsv
export CFW_LOG_DIR=/tmp/cfwlogs                        # optional: keep full logs
tools/cfw/build.sh plus "$(tools/cfw/flags.sh step-jump)" step-jump
```

`build.sh` does a clean build of one device and prints/appends one record:
`date  commit  srchash  target  flags  toolchain  result  errors  warnings  config-name`.
`tools/cfw/report.sh build-results.tsv` renders the records as a Markdown table.

Self-tests for the helper scripts: `tools/cfw/tests/run.sh` (no toolchain needed).

Host tests: `tools/cfw/hosttests.sh` compiles and runs every `tests/*.cpp` and `tests/*/*.cpp`.

## Which configurations to build

Chosen from `features.tsv`, not by enumerating flag combinations (`tools/cfw/matrix.py`):

| Config | Flags | Why |
|---|---|---|
| `stock` | none | Custom code must compile out completely |
| `merged` | every feature already in `main` | The integrated configuration; exercises shared files |
| `<feature>` | the feature plus its declared dependencies | The feature on its own |
| `merged+<feature>` | `merged` plus the PR's feature | Interaction with everything already merged |

Each is built for `plus`, `original` and `mini`. A PR branch must be registered in `features.tsv` (or be an
`infra/*` branch) or validation fails; see "Feature registration and CI" in `PR_WORKFLOW.md`. When a PR touches code that two unmerged features share (see the
shared-code table in the ledger), add the specific pair by hand and say why in the PR report.

## Stock regression

With every custom flag off the firmware must compile exactly like the upstream baseline
(`UPSTREAM_BASE` in `tools/cfw/baseline.env`). Warning counts alone prove nothing, so the check is on the
warning *contents*: `tools/cfw/newwarn.sh stock.log other.log` lists warnings (file + message, line numbers
stripped) present in `other.log` but not in `stock.log`. CI runs it twice:

* stock build of the PR vs stock build of the upstream baseline: must list nothing;
* each flag-on build vs the stock build of the same commit: lists the warnings the custom code introduces.

A higher raw warning count with *no* new distinct warnings means existing warnings were repeated by an additional
translation unit. Example (first `merged` build of `main`, Plus): 397 vs 394 stock, and `newwarn.sh` lists nothing — the
three extra lines are the stock `interfaceMenuPopup.h` unused-parameter warnings, emitted again because the new
`mtMidiSlotTrigger.cpp` includes that header.

Known stock counts with the local xPack GCC 10.3.1 (informational; recompute when the toolchain changes):
Plus 394, Original 2726, Mini 301.

## When a previous result can be reused

`tools/cfw/reuse.sh <target> "<flags>"` answers this. A record is reusable only if **all** of these match:

1. `srchash` – hash of the **committed** firmware source tree (uncommitted edits are not seen — commit before building) (`tools/cfw/srchash.sh`: file hashes under `core platforms synth
   editor-fx-sdk synth-plugin-sdk util Makefile version.mk`). Docs, tests, workflows and tools are excluded, so a
   docs-only commit does not invalidate builds.
2. target,
3. flag set (order-insensitive),
4. toolchain version string, taken from the same compiler `build.sh` would use (`$TOOLCHAIN`, else `agt/10.3`,
   else `arm-none-eabi-gcc` on `PATH`). If the compiler cannot be run, the version is unverifiable and **nothing is
   reused**,
5. result was `OK`.

Anything else — a rebase, a new commit touching firmware paths, a different toolchain — means the old record no longer
applies. Do not carry results across a rebase by assumption: records are keyed to the tree they were built from, and
the ledger marks results on an older tree as *historical*.

## Recording results

* Append records with `CFW_RESULTS` (above) and commit `build-results.tsv` with the ledger update.
* CI records the same fields in each job's step summary and uploads the build logs as artifacts (14 days).
* Hardware results are written by hand in the ledger with the tester, date, device and firmware commit.
