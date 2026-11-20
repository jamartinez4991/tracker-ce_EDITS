# Realtime Recording Quantize — Implementation & Progress

**Build flag:** `-DMOD_REALTIME_QUANTIZE`. **Prerequisites:** `MOD_LIVE_REC_TOGGLES` (hard #error) and `MOD_ACTION_PAGE4` for its controls (without Page 4 there is no UI and it stays at defaults).
**Baseline:** polyend/tracker-ce `ed513015` (work was NOT rebased onto upstream `6d5dbc9`; see BUILD.md) · **Branch:** `mod/seq-fixes` (on top of the merged earlier mods `b61193b`)
**Verified:** compile-checked Original/Plus/Mini, flag on/off and in subsets; full link in the merged tree. **Not run on hardware.**

## Source files modified (exact)
| File | Change |
|---|---|
| `core/source/rtQuantize.h` (new) | intervals, nearest grid, strength interpolation, micromove mapping (host-tested) |
| `core/source/mtSequencer.h` | per-track `recQuantApplied/recQuantAdj`; session settings + accessors; 2 declarations |
| `core/source/mtSequencer.cpp` | `quantizedNoteOnPosition`, `quantizedNoteOffPosition`; Note-On and both Note-Off paths in `handleNoteOn/handleNoteOff` |
| `core/FileManager/WorkspaceProjectLoad.cpp` | session reset on project load / reload |
| `core/interface/patternEditor/*` | controls — see action-page4 |
| `tests/rt-quantize/test.cpp` | host test |

## Behaviour (per the brief)
Divisions 1/2, 1/4, 1/4T, 1/8, 1/8T, 1/16 = 384/192/128/96/64/48 microsteps, anchored to the pattern (measure) start. `Pq = D·floor(P/D+0.5)` (ties forward), `Pr = P + round(S/100·(Pq−P))` (half away from zero). Applies only while Microtiming Detection is ON (otherwise the stock non-microtimed path is untouched); the two controls stay visible and dim, values retained, edits ignored. The adjustment is captured at Note-On and re-applied to Note-Off, so duration is preserved; the same-step Note-Off guard moves the off to the next step (micromove cleared). Note Off Detection OFF writes no pattern note-off. Velocity untouched. Session settings are RAM only, never in the project; reset on project load/reload and power-up (defaults 1/16, 100 %).
**Heads-up:** with the defaults (1/16, 100 %) Microtiming Detection ON now snaps notes to step starts; set Q.Str to 0 % for the old raw-microtiming behaviour. Pre-roll/anticipated first note keeps its stock placement.
Select-then-adjust (button need not stay held) was in the older brief; the newer Page 4 handoff specifies hold-to-edit, which is what is built.

## Tests
Host: all intervals divide the measure; nearest-grid and tie rules; measure-boundary behaviour; for every position 0–1535 and every division, strength 0 = identity, 100 = grid, monotone in between; rounding cases; micromove range. **ALL OK.** Not tested on hardware (multiple simultaneously held notes, pre-roll and undo in particular).
