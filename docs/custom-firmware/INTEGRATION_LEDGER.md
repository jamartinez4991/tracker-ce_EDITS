# Integration ledger

Authoritative record of custom-firmware feature integration. Update it whenever authorized integration work changes a
feature's state. Machine-readable feature list (flags, branches, dependencies): [`tools/cfw/features.tsv`](../../tools/cfw/features.tsv).
Build rules and reuse: [`BUILD_VALIDATION.md`](BUILD_VALIDATION.md). Procedure: [`PR_WORKFLOW.md`](PR_WORKFLOW.md).

**Repository:** `jamartinez4991/tracker-ce_EDITS` (personal fork; separate from any upstream-contribution work)
**`main`:** `a18b8ad` (merge of PR #10) — last updated 2026-11-20
**Upstream baseline:** `polyend/tracker-ce` `922bba0` (the fork's `main` was branched from it; not re-checked for newer upstream commits — we deliberately stay on it)
**Integrated reference branch:** `custom-firmware` (all features except the discarded Modified Chance; used only for comparison, not a merge target)
**Default branch:** `main`

## Validation legend

| Mark | Meaning |
|---|---|
| **C** | Compiled (0 errors) — see `build-results.tsv` or the notes below |
| **T** | Host test passed (models of isolated logic, not firmware code) |
| **R** | Source / diff review done |
| **H** | Hardware validated — **none of the migrated code has been run on hardware** |

Hardware testing done earlier on the pre-migration baseline (`ed513015`) does not carry over to the migrated code.

## Status summary

Statuses: Proposed · In Development · Awaiting Rebase · Rebased · Awaiting Validation · Validation Complete · Awaiting Review · Approved for Merge · Merged · Blocked.

| # | Feature | Flags | Branch (head) | In `main`? | Status | Depends on | Needed by |
|---|---|---|---|---|---|---|---|
| 1 | Mono sampling | `MOD_MONO_SAMPLING` | `feature/mono-sampling` (`78643d1`) | **yes** | Merged | – | #2 |
| 2 | Recorder format display | `MOD_RECORDER_FORMAT` | `feature/recorder-format` (`e924009`) | **no** (see note 1) | Awaiting Rebase | #1 | – |
| 3 | Sample instrument FX (SRR, DJ filter) + shared instrument ext block v10 | `MOD_SAMPLE_INSTRUMENT_FX` | `feature/sample-instrument-fx` (`87dad36`) | **yes** | Merged | – | #4, #5 |
| 4 | Filter rename | `MOD_FILTER_RENAME` | `feature/filter-rename` (`70f3a68`) | **no** (note 1) | Awaiting Rebase | #3 | – |
| 5 | Pitch range, signed LFO depth, LFO polarity | `MOD_PITCH_RANGE` `MOD_LFO_SIGNED_DEPTH` `MOD_LFO_POLARITY` | `integrate/pitch-lfo-extensions` (original `feature/pitch-lfo-extensions` commit `17ca6bc` + merge of `main`) | **no** (note 1) | Awaiting Review (integration step 1, replacement PR) | #3 | #6 |
| 6 | Bipolar modulation depth | `MOD_BIPOLAR_MOD_DEPTH` | `feature/bipolar-mod-depth` (`45fef02`) | **no** (note 1) | Awaiting Rebase | #5 | – |
| 7 | Lazy Chop | `MOD_LAZY_CHOP` | `feature/lazy-chop` (`1a9a26e`) | **yes** | Merged | – | – |
| 8 | Live recording toggles (Note Off, Microtiming) | `MOD_LIVE_REC_TOGGLES` | `feature/live-recording-toggles` (`b02b749`) | **yes** | Merged | – | #15, #16 |
| 9 | MIDI instrument slot trigger | `MOD_MIDI_SLOT_TRIGGER` | `feature/midi-slot-trigger` (`bc46af7`) | **yes** | Merged | – | – |
| 10 | Track-local Step Jump (FX id 51) | `MOD_STEP_JUMP` | `feature/step-jump` (`d827799`) | **yes** | Merged | – | #11 |
| 11 | Cond. Chance (FX id 52; replaces Modified Chance) | `MOD_COND_CHANCE` | `feature/cond-chance` (`771190f`) | no | Awaiting Rebase (base `feature/step-jump` has moved; conflicts with `main`; glue pending) | #10 | – |
| 12 | Master Timing Loop Multiplier ×1..×8, pattern v7 | `MOD_MASTER_MULTIPLIER` | `feature/master-multiplier` (`d1d45d1`) | **yes** | Merged | – | #13, #14 |
| 13 | Pattern progression | `MOD_PATTERN_PROGRESSION` | `feature/pattern-progression` (`93f413a`) | no | Awaiting Rebase (conflicts with `main`) | #12 | – |
| 14 | Pattern BPM, pattern v8 | `MOD_PATTERN_BPM` | `feature/pattern-bpm` (`623666d`) | no | Awaiting Rebase (conflicts with `main`) | #12 | #15 |
| 15 | Action Button Page 4 | `MOD_ACTION_PAGE4` | `feature/action-page4` (`fc953c2`) | no | Awaiting Rebase (base `stack/page4-base` is stale; conflicts with `main`) | #14, #8 (via `stack/page4-base`) | #16 |
| 16 | Realtime Quantize + Step Entry Division | `MOD_REALTIME_QUANTIZE` `MOD_STEP_ENTRY_DIVISION` | `feature/realtime-quantize-step-entry` (`29b9581`) | no | Awaiting Rebase (waits for #15) | #15 (+#8) | – |

Discarded: **Modified Chance** (`MOD_MODIFIED_CHANCE`) — replaced by Cond. Chance (#11); never combine them.

**Note 1 — merged into the stack branch only.** PRs #2, #4, #5 and #6 were stacked on `feature/mono-sampling`,
`feature/sample-instrument-fx` and `feature/pitch-lfo-extensions`. GitHub reports them as merged, but each merged into
its *parent branch*, after (or independent of) the parent's merge into `main`. Their code is **not in `main`**
(checked: no `MOD_FILTER_RENAME`, `MOD_LFO_SIGNED_DEPTH` or `MOD_BIPOLAR_MOD_DEPTH` in `main`; the only trace of #2 and #5
is the shared hooks that #1 and #3 carry). `tools/cfw/conflicts.sh` shows them as "NO(stale)" because their declared base
branch has moved. They still need to land in `main` through new PRs from their original commits (dry-run: no conflicts
with `main`). Treat them as **not integrated**.

## Integration order

Already in `main`: #1, #3, #7, #8, #9, #10, #12.

Approved plan (2026-11-20): one PR per step, each from a fresh `integrate/*` branch; the original PR branches are kept
untouched as history (no force-push, no rewriting). Missing integration glue goes into the PR that needs it.
Mono sampling stays Plus/Mini only.

| Step | Content | Branch | Status |
|---|---|---|---|
| 1 | #5 Pitch range / signed LFO depth / LFO polarity | `integrate/pitch-lfo-extensions` | Awaiting Review |
| 2 | #6 Bipolar modulation depth (after #5) | `integrate/bipolar-mod-depth` | Not started |
| 3 | #4 Filter rename, #2 Recorder format | `integrate/filter-rename`, `integrate/recorder-format` | Not started |
| 4 | #11 Cond. Chance (+ `resetCondChanceState()` glue) | `integrate/cond-chance` | Not started |
| 5 | #14 Pattern BPM, then #13 Pattern progression | `integrate/pattern-bpm`, `integrate/pattern-progression` | Not started |
| 6 | #15 Action Page 4 | `integrate/action-page4` | Not started |
| 7 | #16 Realtime Quantize + Step Entry Division | `integrate/realtime-quantize-step-entry` | Not started |

Final audit: compare `main` with `custom-firmware`; build every flag on all three devices.

## Shared code and integration points

| File / function | Features | Notes |
|---|---|---|
| `mtSequencer.cpp` `switchRowStep()` | #10 (`applyPendingStepJump`), #12 (master boundary), #11 (`stepSerial`), #7/#8 (blank-line residue) | Order: Step Jump block, then the master-boundary block, then `masterTimingStart(0)` after the transition |
| `mtSequencer.cpp` `masterRestartTracks()` | #12 (owner), #10 (`discardPendingStepJumps()`), #11 (`resetCondChanceState()`) | **Glue lives in the later feature's PR**; neither side shows a conflict |
| `mtSequencer.cpp` `handleNoteOn()` | #9 (micromove block), #8, #16 (quantized destination) | #9's block sits after the `recVelocity()` block |
| `mtSequencer.cpp` `resetChanceState()`, `reset_actual_pos()` | #10 (discard points), #11 | |
| `mtSequencer.cpp` play/Chance pass | #10, #11 | |
| `mtSequencer_patternEdit.cpp`, `interfaceDefs.h`, `interfaceFxDesc.h` | #10, #11 | FX ids 51 / 52; `FX_COUNT` in `mtStructs.h` |
| `mtStructs.h` | #3 (instrument ext block v10), #5, #9 (`strMtConfig.midi`), #10/#11 (`FX_COUNT`), #12 (`PATTERN_FILE_VERSION` 7), #14 (8) | Pattern file v6 stock → v7 multiplier → v8 pattern BPM |
| `patternEditor.h`, `fPatternEdit.cpp`, `gPatternEdit.cpp` | #8 (Page 3 slots), #12 (Page 3 slot), #15/#16/#14 (Page 4), #11 (title bar) | `MOD_P3_TOGGLES` / `MOD_P3_MULTIPLIER` macros; Page 4 hides them |
| `fileManagerPattern.cpp` | #12, #14 | v7/v8 load |
| `AudioVoice`, `mtAudioEngine.cpp`, `play_memory_*` | #3 (SRR/DJ per channel), #5, #7 | Built on upstream's unified per-channel voice |
| `gSampleRec.cpp`, `sampleRecorder.h` | #1, #2 | |
| `configMenu*.cpp`, `mtConfig.cpp`, `mtMidi.cpp` | #9 only | midi config block grew 14→20 bytes with a magic byte |

## Validation record

**Current evidence** (see `build-results.tsv` for exact records; `merged` = the six flags of the features in `main`):

<!-- RESULTS_START -->
`main` `e6e4fca` (all six merged features together, plus stock), built 2026-11-20 with the local xPack GCC 10.3.1. This is the first build of the combined `main`. **Compiled (C) only**; no host test beyond `tests/boundary.cpp` (model) was affected.

| Date | Commit | Source hash | Target | Flags | Toolchain | Result | Err | Warn | Config |
|---|---|---|---|---|---|---|---|---|---|
| 2026-11-20 | `e6e4fca` | `3007edf9ff8c` | plus | `-DMOD_LAZY_CHOP -DMOD_LIVE_REC_TOGGLES -DMOD_MASTER_MULTIPLIER -DMOD_MIDI_SLOT_TRIGGER -DMOD_MONO_SAMPLING -DMOD_SAMPLE_INSTRUMENT_FX` | xPack GNU Arm Embedded GCC x86_64 10.3.1 20210824 | OK | 0 | 397 | merged |
| 2026-11-20 | `e6e4fca` | `3007edf9ff8c` | plus | `STOCK` | xPack GNU Arm Embedded GCC x86_64 10.3.1 20210824 | OK | 0 | 394 | stock |
| 2026-11-20 | `e6e4fca` | `3007edf9ff8c` | original | `-DMOD_LAZY_CHOP -DMOD_LIVE_REC_TOGGLES -DMOD_MASTER_MULTIPLIER -DMOD_MIDI_SLOT_TRIGGER -DMOD_MONO_SAMPLING -DMOD_SAMPLE_INSTRUMENT_FX` | xPack GNU Arm Embedded GCC x86_64 10.3.1 20210824 | OK | 0 | 2729 | merged |
| 2026-11-20 | `e6e4fca` | `3007edf9ff8c` | original | `STOCK` | xPack GNU Arm Embedded GCC x86_64 10.3.1 20210824 | OK | 0 | 2726 | stock |
| 2026-11-20 | `e6e4fca` | `3007edf9ff8c` | mini | `-DMOD_LAZY_CHOP -DMOD_LIVE_REC_TOGGLES -DMOD_MASTER_MULTIPLIER -DMOD_MIDI_SLOT_TRIGGER -DMOD_MONO_SAMPLING -DMOD_SAMPLE_INSTRUMENT_FX` | xPack GNU Arm Embedded GCC x86_64 10.3.1 20210824 | OK | 0 | 301 | merged |
| 2026-11-20 | `e6e4fca` | `3007edf9ff8c` | mini | `STOCK` | xPack GNU Arm Embedded GCC x86_64 10.3.1 20210824 | OK | 0 | 301 | stock |

New distinct warnings with the merged flags compared with the stock build: **none** on Plus, Original and Mini (`tools/cfw/newwarn.sh`); the +3 on Plus and Original are existing `interfaceMenuPopup.h` warnings repeated by the new `mtMidiSlotTrigger.cpp` translation unit.
<!-- RESULTS_END -->

**Earlier results, on older source trees (historical — not reusable under the rules in `BUILD_VALIDATION.md`):**

| Feature | Evidence | Tree it applied to |
|---|---|---|
| #1, #3, #4, #5, #6, #7, #8 | Stage builds from the 2026-11 migration, 0 errors on Plus (mono-sampling through bipolar-mod-depth also on Original); integrated `custom-firmware` built on Plus, Original, Mini with and without flags | migration branches on `922bba0` before they were merged; **the combined `main` was not built until the records above** |
| #9 | Plus, Original, Mini × flag on/off, all 0 errors (warnings 397/2729/301 on, 394/2726/301 off); source review (see its PR); no host test | `bc46af7` on `e17f8d9` |
| #12 | Plus, Original, Mini × flag on/off, 0 errors; Plus with `MOD_LIVE_REC_TOGGLES`; `tests/boundary.cpp` passes (host model) | `d1d45d1` on `e17f8d9`. The merge with #9 was a clean dry-run; the combination was not built before merging |
| #10 | Plus, Original, Mini × {`step-jump`, `step-jump+master-multiplier`, `step-jump+master-multiplier+midi-slot-trigger+live-rec-toggles`, stock}, all 0 errors, stock warnings 394/2726/301; `tests/boundary.cpp` (does not cover Step Jump) | `d827799` on `e6e4fca` |
| #11, #13, #14, #15, #16 | Plus builds and host tests on the pre-merge stage branches | **invalidated** — all will be rebased onto the new `main` |

Stock regression (Plus only, local): `main` with every flag off produces the same 394 warnings, with identical messages, as the upstream baseline `922bba0` (`tools/cfw/newwarn.sh`; source hashes differ as expected). Original and Mini were checked only by warning count (2726 / 301, equal to the earlier stock counts).

Not done for any feature: hardware validation, audio-thread timing measurement, memory-map comparison against upstream.

## Outstanding hardware tests

* #1/#2 mono recording on Plus/Mini; mono sampling is **Plus/Mini only** (Original now records stereo upstream — open question whether to enable).
* #3 SRR/DJ filter on mono and stereo samples (per-channel processing was reworked for upstream's null-silence graph); audio-thread load on Original (two channels now); instrument v10 load/save with older instruments.
* #7 Lazy Chop against upstream's `VoiceStreamManager` slice caches.
* #8, #9 Note Off / Microtiming toggles; slot trigger via Jack and USB MIDI in each mode, held notes across column changes, chord entry, old config load.
* #12 ×1..×8 boundaries, pattern chaining, Performance Mode, MIDI clock; loading v6 and v7 pattern files.
* #10 jump pending at a ×N boundary when the pattern loops without switching; jump across a pattern switch; with slot-triggered entry and Live Recording Toggles.
* #11, #13–#16 everything (not integrated yet). #14: pattern v8 load vs upstream's pattern-load validation; Pattern BPM against `InternalTickClock`.

## Known limitations / unresolved

* Slot-triggered edit-mode notes (#9) follow the global Microtiming Detection setting, not #8's Page 3 toggle. Same as `custom-firmware`; left as is.
* Pattern files saved by flag-on builds are v7/v8 and unreadable by stock; instrument v10 likewise.
* `tests/boundary.cpp` is a host model of the master boundary decision, not an execution of firmware code.
* Records in `build-results.tsv` currently come from a local xPack GCC 10.3.1; CI uses the ARM 10.3-2021.10 toolchain (same GCC 10.3.1), so counts may differ slightly — compare within one toolchain.
* The existing `build.yml` (inherited from upstream, unchanged here, with its own write permissions and PR comments) fails PRs whose commits carry an AI `Co-authored-by` trailer.

## Decisions

| Decision | Detail |
|---|---|
| Modified Chance discarded | Cond. Chance (#11) replaces it |
| `main` stays default; stay on upstream `922bba0` | Even if upstream moves |
| Never PR to `polyend/tracker-ce` | All PRs in this fork only |
| Merge method | Merge commit, only on explicit authorization |
| Order #9 → #12 → #10 → #11 | #12 must merge before #10 is rebased; never rebase onto a PR branch |
| Step Jump / Master Multiplier glue | `#ifdef MOD_STEP_JUMP discardPendingStepJumps();` at the end of `masterRestartTracks()`, in #10 |
| Cond. Chance glue | `#ifdef MOD_COND_CHANCE resetCondChanceState();` in the same function, in #11 |
| Filter rename depends on Sample instrument FX | It lives inside that feature's code |
| `stack/page4-base` | Auxiliary merge of #14 and #8 used as #15's base; obsolete once those are in `main` |
| Mono sampling Plus/Mini only | Open: enable on Original? |
| SRR/DJ per-channel processing | Needed for upstream's null-silence graph |
| Slot-trigger Microtiming | Follows the global setting (see limitations) |
| `features.tsv` is the authoritative registry | A feature PR branch must be registered or validation fails; infrastructure branches use `infra/*` (`tools/cfw/infra-branches.txt`) |
| A clean Git merge is not proof of correctness | Compare against `custom-firmware` for missing glue |

## Integration step 1 validation (#5 Pitch/LFO, branch `integrate/pitch-lfo-extensions`)

- Content: original #5 commit `17ca6bc` plus a clean merge of `main` (`a18b8ad`); no conflicts, no glue needed.
  Compared with `custom-firmware`, the only differences in the touched files are #4 (Filter rename) and #6 (Bipolar depth).
- **C (Plus only, by decision):** stock 394, Pitch/LFO + deps 394, merged 397, merged + Pitch/LFO 397, Pitch range alone 395,
  Polarity alone 394, Signed depth alone 394 — all 0 errors (rows in `build-results.tsv`).
- **Original / Mini: not built for this step** (Plus-only validation chosen to save build time). A partial Original run
  (stock 2726, Pitch/LFO + deps 2726, 0 errors) is also recorded. Mini is untested.
- **T:** host tests pass (`tests/boundary.cpp`; these do not exercise Pitch/LFO).
- **H:** no hardware validation.
