# Step Entry Division — Implementation & Progress

**Build flag:** `-DMOD_STEP_ENTRY_DIVISION`. **Prerequisite:** `MOD_ACTION_PAGE4` for its control.
**Baseline:** polyend/tracker-ce `ed513015` (work was NOT rebased onto upstream `6d5dbc9`; see BUILD.md) · **Branch:** `mod/seq-fixes` (on top of the merged earlier mods `b61193b`)
**Verified:** compile-checked Original/Plus/Mini, flag on/off and in subsets; full link in the merged tree. **Not run on hardware.**

## Source files modified (exact)
| File | Change |
|---|---|
| `core/source/rtQuantize.h` | `nextEntryPosition` (shared with Realtime Quantize header) |
| `core/source/mtSequencer.h` | session value + accessors (shared block with Realtime Quantize) |
| `core/interface/patternEditor/fPatternEdit.cpp` | `stepEntryDelta`; `moveCursorByStep` and `moveCursorByStepParallel` |
| `core/interface/patternEditor/patternEditor.h`, `gPatternEdit.cpp` | state and control (see action-page4) |
| `core/FileManager/WorkspaceProjectLoad.cpp` | session reset |

## Behaviour
Six divisions as Quantize. 1/16 (default) = stock Step Jump (the stored Step Jump value is never changed). Any other division replaces the Step Jump interval for the cursor advance after note entry (keyboard/pad entry and edit-mode MIDI). Triplet divisions are tracked in microsteps so the cursor alternates (1/4T: steps 2,5,8,10,13,16 …); moving the cursor by hand resyncs. Wraps at the pattern end. Independent of Realtime Quantize. Paste-height moves keep the stock behaviour. Host-tested (`tests/rt-quantize`). Not hardware-tested.

## v4 fix — notes entered without microtiming
**Files:** `core/source/mtSequencer.h` (`stepEntryFracStep/Micro`, `setStepEntryFraction`, `stepEntryMicromoveFor`; cleared on division change and session reset), `core/source/mtSequencer.cpp` (`handleNoteOn` edit branch), `core/source/mtSequencer_patternEdit.cpp` (`setSelectionNote`), `core/interface/patternEditor/fPatternEdit.cpp` (`stepEntryDelta` publishes the fraction; fixes below), `tests/step-entry/test.cpp`.
**Cause:** the fractional position was tracked in `stepEntryMicro` but never written to the step; the note writers only set the note. Now both note-write paths set `micromove = (frac*100+23)/47` for the step the cursor landed on (nothing for 1/16 or an on-grid position). The advancement logic is unchanged.
**Also fixed (v3 regression):** `moveCursorByStepParallel()` contained `int stepSize = stepSize;` (uninitialised read at 1/16 on the MIDI/parallel path). It now reads `mtProject.values.patternEditStep`.
**Test:** `tests/step-entry` walks 24 entries on each of 1/2, 1/4, 1/4T, 1/8, 1/8T (1/16 = stock): step index and micromove match the ideal grid positions (e.g. 1/4T: 0, 2.34, 5.68… ). ALL OK. Not run on hardware.
