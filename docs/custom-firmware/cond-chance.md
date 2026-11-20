# Cond. Chance (new step FX) — Implementation & Progress

**Build flag:** `-DMOD_COND_CHANCE`. **Prerequisites:** none. Optional hooks: Step Jump (a failed Cond. Chance suppresses the jump), Master Multiplier (phases reset at the multiplier boundary).
**Supersedes `MOD_MODIFIED_CHANCE`** (values 175–246 inside stock Chance). Do not build both: with only `MOD_COND_CHANCE` the stock Chance is exactly stock.
**Baseline:** polyend/tracker-ce `ed513015` (work was NOT rebased onto upstream `6d5dbc9`; see BUILD.md) · **Branch:** `mod/seq-fixes` (on top of the merged earlier mods `b61193b`)
**Verified:** compile-checked Original/Plus/Mini, flag on/off and in subsets; full link in the merged tree. **Not run on hardware.**

## Source files modified (exact)
| File | Change |
|---|---|
| `core/source/condChance.h` (new) | header-only value decode, counting and gate logic (host-tested) |
| `core/source/mtSequencer.h` | `FX_TYPE_COND_CHANCE = 52` (fixed id), per-track `condPhase[128]`, `condDecided/condFired`, `stepSerial/condEvalSerial`, 3 declarations |
| `core/source/mtSequencer.cpp` | `evaluateCondChanceControl` (before the mute return), `resetCondChanceState`, FX switch case, resets (play init, pattern change, track replace, master boundary), step-serial bump in `switchRowStep`, failed-gate handling in the Step Jump control pass |
| `core/source/mtSequencer_patternEdit.cpp` | min/max/default, label formatting, random-FX exclusion, id clamp |
| `core/source/mtStructs.h` | `FX_COUNT` 46/47/48 by flag combination |
| `core/interface/core/interfaceDefs.h`, `interfaceFxDesc.h` | FX tables (`o - Cond. Chance`) and description (rewritten in v4) |
| `core/interface/patternEditor/gPatternEdit.cpp` | v4: title-bar text `Cond: ...` for FX 52 (+ `condChance.h` include) |
| `tests/cond-chance/test.cpp` | host test |

## Stock Chance
Untouched by this flag. The modified-chance branch still exists but is superseded.

## FX value encoding (one byte, 0–172) — v4: Gate removed
Three formats only, with the stock Chance wording and the prefix `Cond`:
* **0–100** — probability in percent. Title bar `Cond: xx%`, grid label `xx`.
* **101–136** — Play x / Skip y (x,y = 1..6). Title bar `Cond: Play x / Skip y`, grid label `x:y` (same as stock Chance).
* **137–172** — Skip y / Play x. Title bar `Cond: Skip y / Play x`, grid label `Syx` (same as stock Chance).
There is no Gate, no 75 % / 50 % tier and no P/Q/R/S/T/U lettering any more. Default for a new FX: 50 %. Random-FX generation uses 0–100 only.
**File compatibility:** values 21–236 written by v3 firmware are re-read under the new layout (101+ now means counted Play/Skip). v3 was never released beyond testing; re-enter any Cond. Chance values placed with v3.

## Semantics
Each step keeps its own encounter counter per track. Every pass over the step advances it **exactly once** (per-step serial guard), whether the step played or not. A plain percentage rolls once per encounter and has no counter. Muted tracks keep counting. Counters reset on playback start, pattern change, track replacement and the Master Multiplier boundary; they persist across ordinary pattern loops. Stock Chance and Cond. Chance on one step must both pass.

## Why the old Play-X-Skip-Y combinations misbehaved
Not conclusively reproduced (no hardware here). Findings: the old scheme had two different Play/Skip systems under one Chance command (stock 103–174 counted whole-pattern passes; 175–246 counted per-step encounters) and the labels overlapped (`P`), and the old evaluation had no guard against being run more than once per step. The new FX has one counting system, one evaluator, an idempotence guard, and a full simulation test.

## Tests
`tests/cond-chance/test.cpp` (host, v4): all 173 values decode (no gaps, no gate), all 72 counted combinations × 4 cycles in both arrangements, round-trip, percent thresholds and statistics. **ALL OK.** Not tested on hardware.
