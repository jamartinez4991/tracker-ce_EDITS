# Master Timing Loop Multiplier — Implementation & Progress

**Build flag:** `-DMOD_MASTER_MULTIPLIER` (opt-in; all targets)
**Spec:** `FOCUS PeTracker CE - Master Multiplier Handoff v1.txt`
**Baseline:** polyend/tracker-ce `ed513015e96945829ad367e4367947b978cfd41a` · **Branch:** `mod/master-multiplier`
**Status:** implemented; compile-verified Original/Plus/Mini × flag on/off; boundary decision logic host-tested (`tests/boundary.cpp`);
full firmware (all ten mod flags) linked for all three targets. **Not run on hardware.**
**File-format change:** `PATTERN_FILE_VERSION` 6 → 7 under this flag (structure size unchanged, 16,432 bytes).

## Source files modified (exact)

| File | Change |
|---|---|
| `core/source/mtStructs.h` | `PATTERN_FILE_VERSION` = 7 under the flag |
| `core/source/mtSequencer.h` | multiplier helpers (`getPatternMultiplier`, `setPatternMultiplier`, `decodeMasterMultiplier`); `masterStep` / `masterLength` / `masterActiveMult` in `strPlayer`; 3 method declarations |
| `core/source/mtSequencer.cpp` | `masterTimingStart/Stop`, `masterRestartTracks`; init in `play()`, `playSelection()`, `stop()`; boundary decision and re-latch in `switchRowStep()` |
| `core/FileManager/fileManagerPattern.cpp` | explicit v6→v7 stage in `checkVersionAndTranslate()` |
| `core/interface/patternEditor/patternEditor.h` | `changeActualPatternMultiplier`, `setActualPatternMultiplier`, `showMultiplier`, display state |
| `core/interface/patternEditor/fPatternEdit.cpp` | edit helpers; Page 3 button 3 callback; `selectedPlace == 3` in encoder / left / right / up / down / pads / pad lighting |
| `core/interface/patternEditor/gPatternEdit.cpp` | Page 3 "Multiplier" label and `x1..x8` value; `showMultiplier()` (refreshed from `showLength()`) |

**Shared with other features:** `mtSequencer.h/.cpp` (Step Jump, Modified Chance, Lazy Chop, Live Recording Toggles);
`fPatternEdit.cpp` / `gPatternEdit.cpp` (Live Recording Toggles uses Page 3 slots 4–5, this feature slot 3 — no overlap, the page-3 style
condition and button list were merged by hand); `mtStructs.h` (shared instrument block). **Prerequisites: none.**
Optional integration (each side compiled only when the other flag is on): `resetCondChanceState()` (Cond. Chance) and
`discardPendingStepJumps()` (Step Jump) are called at every master boundary.

## Behaviour

* **Storage:** `strPattern::rezerwa1` = 0..3 for x1..x8. Anything else reads as x1. All reads/writes go through the helpers in `mtSequencer.h`.
  `clearPattern()` already zeroes the byte, so new/cleared/missing patterns are x1. Saved, duplicated and undone with the pattern (whole-struct copies).
* **Migration:** files older than v7 always get `rezerwa1 = 0` after the existing staged translations and before the version bytes are
  rewritten, so no historical byte is trusted. Validation needed no change (v7 uses the v6 sizes).
  *Downgrade note:* stock/flag-off firmware rewrites the version as 6, so a v7 multiplier is read back as x1 by a flag-on build.
* **UI:** Pattern Editor Page 3, slot 3 (`interfaceButton2`, `label[2]`): "Multiplier" with `x1..x8` beneath it, always shown. Press selects
  (`selectedPlace = 3`), encoder / left / right / up / down step by one and clamp at the ends, pads 1–8 select x1–x8 directly (other pads ignored,
  the selected value's pad lights). Marks the pattern struct changed (Pattern Length convention, no explicit undo entry).
* **Runtime state:** `masterStep`, `masterLength` (latched pattern steps × multiplier), `masterActiveMult` (latched). Independent of
  `globalPos`, `actual_pos`, `actual_pos_absolute`, `passCount`, mutes and jumps.
* **Boundary decision (final track, in `switchRowStep()`, after the track advanced):** only in ordinary playback (not Performance Mode, not
  selection playback — those force `masterActiveMult = 1`, i.e. stock). With latched multiplier > 1 the counter advances once per global step
  (the final track's step boundary happens exactly once per global step, muted or not) and `endOfPattern` becomes "counter reached the latched
  length" instead of "last track wrapped". Pattern Break keeps its stock early `endOfPattern`. With multiplier 1 nothing is changed.
* **At every ordinary `endOfPattern` (including x1):** all tracks are set to step 1 (`actual_pos` and `actual_pos_absolute`; `passCount` untouched),
  pending Step Jumps are discarded, Modified Chance tables are cleared — then the **unchanged** stock transition block runs (sequential switch,
  Song Mode, callbacks, song end). Afterwards the multiplier of the pattern now playing is latched and the counter restarts, so live multiplier
  edits and the destination pattern's own value take effect at the next boundary only.
* **Transport:** `play(fromPos)` latches and starts the counter at `fromPos` (first boundary keeps the right distance); `stop()` and
  `playSelection()` put it in the x1 state. MIDI clock goes through the same tick path, so no separate timer exists.
* **Pre-loading:** untouched. `isLastPatternStep()` keeps its meaning, so the sequential-switch arming guard just retries on every physical last step.

## Decisions / deviations to review

1. **Counter advances in `switchRowStep()` of the final track**, not in `inc_uStepGlobal()` plus a pending flag. Same cadence (once per global
   step) and it makes "finish the step before committing" automatic, so no pending flag was needed.
2. **Duration is latched** per cycle. If Pattern Length is changed mid-cycle at ×2..×4 the cycle still ends at the latched count (physical wraps
   no longer end it). At ×1 the stock behaviour (end at the next wrap) is unchanged.
3. **x1 boundary also restarts tracks / clears Modified Chance / discards jumps** (the spec treats the x1 wrap as the master boundary). For stock
   patterns without Jump/Chance this is a no-op because all ordinary tracks wrap together.
4. Switching from Performance Mode back to ordinary playback stays at x1 until the next ordinary boundary, then latches. No Performance-specific reset.
5. MIDI Song Position Pointer: not separately audited; the counter starts from whatever `fromPos` `play()` receives.

## Tests

* `tests/boundary.cpp`: boundaries at 16/32/48/64 steps, mid-pattern start, live ×2→×4 change (32 then 64), Pattern Break immediate, decode of invalid values. Passes.
* Not done: any run on hardware; Song Mode / sequential switching at ×2..×4; v1–v6 file loading with real files; undo/redo of the value.

## v4: maximum raised ×4 → ×8
`kMasterMultiplierMax = 8` (mtSequencer.h); raw storage 0..7 (old files with 0..3 stay valid). Pad handlers (Page 3 and Page 4), clamp, display and `decodeMasterMultiplier` all follow the constant. `masterLength` is `uint16_t`: 192 steps × 8 = 1536, no overflow. Condition FX state handling is unchanged (counters persist across the repeated cycles and reset at the multiplier boundary). `tests/boundary.cpp` now checks ×1..×8 boundaries (incl. 192×8 = 1536, mid-pattern start) and decode of 0..7 / invalid. ALL OK.
