# Track Local Step Jump — Implementation & Progress

**Build flag:** `-DMOD_STEP_JUMP` (opt-in; all targets)
**Spec:** `FOCUS PeTracker CE - Jump + Chance Refinement v2.txt` (§§2–8, 12–14, 16–18)
**Baseline:** polyend/tracker-ce `ed513015e96945829ad367e4367947b978cfd41a` · **Branch:** `mod/step-jump`
**Status:** implemented; compile-verified Original/Plus/Mini × flag on/off; target-resolution arithmetic host-tested
(`tests/resolve.cpp`). **Not linked in isolation, not run on hardware.**

## Source files modified (exact)

| File | Change |
|---|---|
| `core/source/mtSequencer.h` | `FX_TYPE_STEP_JUMP` appended after `FX_TYPE_R9`; `pendingJumpValid` / `pendingJumpTarget` in `strPlayerTrack`; 3 method declarations |
| `core/source/mtSequencer.cpp` | control pass before the mute return; explicit FX handlers; Chance-failure cancel; override in `switchRowStep()`; discard points; 3 new functions |
| `core/source/mtSequencer_patternEdit.cpp` | `getFxMin/Max/Default` (1 / 128 / 1); `generateRandomFx()` bound + exclusion; the hard-coded `0, 47` type clamp |
| `core/source/mtStructs.h` | `FX_COUNT` 46 → 47 under the flag (`FX_MAX` = 49 left alone, see below) |
| `core/interface/core/interfaceDefs.h` | `fxIDs`, `fxNames` (`u - Step Jump`), `fxShortNames`, both pointer tables — inserted **before** the hidden Break Pattern entry |
| `core/interface/core/interfaceFxDesc.h` | description in both the `FEATURE_SYNTH` and non-synth arrays |

`interfaceConstData.cpp`, `fPatternEdit.cpp`, `gPatternEdit.cpp`, `interfacePopups.cpp`, `fProjEdit.cpp` were audited and need no edit:
they size everything from `FX_COUNT` / `FX_COUNT_HIDDEN_FXes`.

**Shared with other features:** `mtSequencer.h/.cpp` and `mtSequencer_patternEdit.cpp` (also Modified Chance, Master Multiplier, and earlier
Live Recording Toggles / Lazy Chop); `mtStructs.h` (also the shared instrument extension block). All edits are in different regions or
`#ifdef`-wrapped. **Prerequisites: none** (works alone). Optional hooks to Modified Chance / Master Multiplier are listed in `CROSS_FEATURE.md`.

## Behaviour

* **FX:** one new visible step FX "Step Jump" (symbol `u`), value 1–128, default 1, in either column. Internal ID 51 (appended; no existing ID moves).
* **Record (uStep == 1, on the `stepToSend` snapshot):** `evaluateStepJumpControl()` runs after the `!isActive` return and **before** the mute
  return, so a muted track still jumps. Inactive tracks (selection playback) keep stock behaviour: no evaluation. Column 2 (index 1) is scanned
  first and column 1 (index 0) overwrites, so **column 1 wins**. Live edits take effect on the next evaluation because only the snapshot is read.
* **Chance gating, no double evaluation:** unmuted — the existing Chance pass is the only evaluation and clears the pending jump when it fails.
  Muted — the stock pass is behind the mute return, so the control pass evaluates the first Chance found (same scan order) itself and
  returns without recording on failure. *Side effect:* on a muted track, a step that holds **both** Chance and Step Jump now advances
  `lastCondFired` (Group With Last / Not Last state). Steps without Step Jump are untouched.
* **Commit (deferred):** `applyPendingStepJump()` is called at the end of the mode handling in `switchRowStep()`, i.e. after forward / backward /
  random / custom have done their own position and `passCount` accounting. It only overwrites `actual_pos`. Nothing runs inside the step, no
  notes, gates, voices or microstep timing are touched; Step Jump is consumed explicitly so it is never sent to an instrument engine.
* **Resolution:** `D = ((T-1) % L) + 1`, `L = lastPos - startPos + 1` using the **same** bounds as forward mode: whole effective length
  (`trackLength + 1`, honouring performance length overrides) or, in looped selection playback, `[firstStep, min(lastStep, trackLength)]`
  with the selection start as base. `T` is stored as entered and resolved when consumed.
* **Cycle accounting:** untouched. `passCount` and `actual_pos_absolute` follow the native rule of each mode (forward: increments when the last
  step finishes even if the jump picks another step).
* **Custom play modes:** one-step override. The cursor (`actual_pos_absolute` → `playOrderWithFx`) keeps advancing, so the order resumes
  after the jumped-to step. The custom FX injected for the *natural* next order slot (`applyRowCustomFx`) is applied to the jump destination.
* **Discard points:** global Break Pattern (checked at consumption); performance-track replacement (`switchPerformanceTrackNow`);
  pattern/song transition (`reset_actual_pos()` — all tracks); `play()`, `playSelection()`, `stop()`, pre-roll (`resetChanceState()`);
  every step boundary consumes the flag, so it can never outlive the step that set it.
* **Self-targeting / chains:** no limits, no loop detection.

## Items the spec asked to be checked

* **Ping-pong:** `PLAYMODE_PINGPONG` is declared and appears only in `reset_actual_pos(row)`. Nothing assigns it to `performancePlayMode`, so it is
  **unreachable** in this tree; Step Jump makes no claim about it.
* **`FX_MAX`:** compared against the *list index* (`fxIdToName`) in `fPatternEdit.cpp` / `gPatternEdit.cpp` / `fProjEdit.cpp`, not the internal ID.
  Highest index is now 46 < 49, so it is unchanged. The `0, 47` clamp in `mtSequencer_patternEdit.cpp` clamped the internal *ID* and would have
  truncated 51, so it is widened under the flag (`fxNameToId` only returns valid IDs).
* **Random FX:** `generateRandomFx()` picks numeric IDs in `[2, FX_COUNT - hidden + 1)`; the bound is held at its old value under the flag and
  Step Jump is excluded explicitly. Random-value FX (`FX_TYPE_RANDOM_VALUE`) can still target it through the generic clamp, like any FX.
* **Pattern files:** `strFx` is two bytes, no size change, no validation of FX IDs exists in `patternFileValidation.h`. A pattern saved with Step
  Jump and loaded by firmware without it shows an unknown FX (ID 51) that the stock sequencer ignores — not tested on hardware.
* **"Master-loop restart":** in a firmware without Master Multiplier the only global boundary is a pattern/song transition; a plain per-track wrap
  (including the last track looping the same pattern) is *not* one, so a jump on the last step still takes effect there. With
  `MOD_MASTER_MULTIPLIER` the multiplier boundary also discards pending jumps.

## Tests

* `tests/resolve.cpp` — spec table (16-step pattern), selection 9–24 target 40, 128-step pattern, over-long selection. Passes.
* Not done: any run of the sequencer itself. The behavioural matrix in spec §17 (modes, selection, break, performance replacement, recording)
  needs hardware.
