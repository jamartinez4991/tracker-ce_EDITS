# Pattern Progression Independent of Track Jumps — Implementation & Progress

**Build flag:** `-DMOD_PATTERN_PROGRESSION` (bug fix; opt-in like everything else). **Prerequisites:** none.
**Baseline:** polyend/tracker-ce `ed513015` (work was NOT rebased onto upstream `6d5dbc9`; see BUILD.md) · **Branch:** `mod/seq-fixes` (on top of the merged earlier mods `b61193b`)
**Verified:** compile-checked Original/Plus/Mini, flag on/off and in subsets; full link in the merged tree. **Not run on hardware.**

## Source files modified (exact)
| File | Change |
|---|---|
| `core/source/mtSequencer.cpp` | next-pattern preload request (`isLastStepInPattern` and the song-mode branch in `play_microStep`) now keyed to the pattern clock instead of Track 1's jumpable playhead |

**Shared with:** every sequencer feature touching `mtSequencer.cpp` (see CROSS_FEATURE.md). Edit is a single region.

## Root cause (audited)
The end-of-pattern *wrap* was already independent of jumps: the last track's `actual_pos_absolute` counter (never changed by a jump) raises `player.endOfPattern`. But the request to **preload the next pattern** (sequential switch / song mode) fired only when `track[0].actual_pos == length`. `actual_pos` is what Step Jump rewrites. A Track 1 Jump loop therefore never reached "last step", the next pattern was never loaded/armed, and the transition could not happen.

## Fix
Preload now triggers from the pattern clock: `actual_pos_absolute == length` of row 0 (immune to jumps), or `masterStep + 1 >= masterLength` when the Master Multiplier is above x1. Selection-loop and performance mode keep the original test. Track-local looping is untouched: tracks still loop with Jump; only the pattern-level request moved.

## Regression test (required by the bug report)
Hardware, not yet run: put an unresolved Jump loop on Track 1, other tracks normal, Song/sequential switch armed — the next pattern must start on the next pattern boundary. Also repeat with the loop on the last track and with Multiplier x2. Step Jump behaviour itself is unchanged.
