# Live Recording Toggles (Note Off Detection · Microtiming) — Implementation & Progress

**Build flag:** `-DMOD_LIVE_REC_TOGGLES` (opt-in; all targets)
**Spec:** `FOCUS PeTracker CE - Live Recording Toggles v1.txt`
**Baseline:** polyend/tracker-ce `ed513015e96945829ad367e4367947b978cfd41a` · **Branch:** `mod/live-recording-toggles`
**Status:** implemented; compile-verified Original/Plus/Mini × flag on/off; **not run on hardware; spec acceptance tests A–F not executed.**

## Source files modified (exact)

| File | Change |
|---|---|
| `core/source/mtSequencer.h` | per-track captured settings in `strPlayerTrack` (`recCapMicrotiming`, `recCapNoteOff`); global runtime options; `recMicroFor()/recNoteOffFor()` accessors; public get/set for the UI |
| `core/source/mtSequencer.cpp` | `getLiveRecMicrotiming()`; capture in `handleNoteOn()` (live-rec branch); captured values used in both `handleNoteOff()` branches; Note Off insertion gated |
| `core/interface/patternEditor/fPatternEdit.cpp` | Page 3 slots 3 and 4: press handlers |
| `core/interface/patternEditor/gPatternEdit.cpp` | Page 3 labels `Note Off ON/OFF` and `Microtiming ON/OFF` |

**Shared files:** `fPatternEdit.cpp` / `gPatternEdit.cpp` (Page 3) are also touched by the *deferred* Master Timing Multiplier
(Page 3 slot 2, reserved, untouched here) and Step Jump. `mtSequencer.cpp/.h` are touched by those deferred features too.
No overlap with the other in-scope features. Prerequisites: none.

## Architecture (spec §5)

*Global controls decide the next recording; captured per-track state decides an active one.*

* Globals (RAM only, nothing saved, no file-format change): `liveRecNoteOffDetection` (default ON) and
  `liveRecMicrotimingOverride` (−1 = follow the config's `recOptions` microtiming component, 0/1 once the user toggles).
  Lazy "follow" means no init hook is needed and later config edits never overwrite a user's toggle (spec item 7).
* Captured state lives on `strPlayerTrack` beside `recOpen`/`recNoteOnStepIdx`, written **only** at the point where the
  existing code sets `recOpen = 1` (i.e. the note really enters the recording path on an armed track, not pre-roll
  preview, not audition, not rejected notes). It is read only for tracks with `recOpen`, so it needs no separate clearing:
  every existing path that clears `recOpen` already ends the lifecycle (spec item 3).
* Release paths use `recMicroFor(track)` / `recNoteOffFor(track)` — never the globals.
* Only the pattern Note Off *insertion* (`step->isNoteEmpty()` block, including its micromove write) is gated. Preview
  release (`sendMidiNoteOff`, `noteOff()`, `noteOpen=0`, held-note bookkeeping, `recOpen=0`) runs unchanged.
* Microtiming controls both the destination step (`getDestRecordingStep(tr, !micro)`) and the `micromove` write, for Note On
  and Note Off. `recVelocity()` and `recOptions` are untouched; the existing config menu is untouched. Playback of existing
  micromove is untouched.

## Source-verification items (spec §12)

1. **Held-source group lifecycle** — single armed track: a new Note On while `liveRecHeldNotesCount > 0` and `recOpen` is
   treated as *joining* the open group (`joinsOpenGroup`); it uses the group's captured settings for destination selection
   and **does not overwrite** them. A Note On with no held sources (or on a multi-armed layout) starts a fresh lifecycle
   and captures the current globals. (Note: in stock code a joining press still writes its own note step and moves
   `recNoteOnStepIdx`; that behaviour is preserved.)
2. **State ownership** — per track, next to `recOpen`; no second lifecycle, `liveRecHeldNotes` untouched.
3. **Cleanup** — see above; stop/disable/`stopOpenNoteOnTrack` paths only clear `recOpen`, which invalidates the capture.
4. **Multi-armed release matching** — unchanged (`stepSent.note == note`); captured values follow the matched track.
5. **Timing source** — `player.track[].uStep` (destination) vs `player.uStep` (micromove value) left exactly as stock.
6. **UI slots** — Page 3 slots 3 and 4 were unassigned; slot 2 left free for the Master Multiplier.
7. **Runtime init** — lazy, as described.

## Deviations / open points

* **Editing style:** the two controls are *press-to-toggle* function buttons on Page 3 (label shows ON/OFF), not a
  select-then-encoder parameter. Page 1's select/encoder model hangs off the pattern grid's `selectedPlace` machinery
  (encoder, up/down, left/right, delete handlers); wiring two more places in there was judged too invasive for this
  pass. Behaviour required by the spec (editable while recording / while notes are held) holds. Say the word if you want
  encoder editing added.
* In the single-armed join case the group's captured Microtiming also decides the destination of the joining note.

## Verification

| Check | Result |
|---|---|
| Compile 3 targets × flag on/off (`mtSequencer.cpp`, `fPatternEdit.cpp`, `gPatternEdit.cpp`) | Pass |
| Flag off ⇒ code paths equal stock (`recMicroFor`→`recMicrotiming()`, `recNoteOffFor`→`true`) | By inspection |
| Acceptance tests A–F (single/multi armed, mid-note toggle, pre-roll, pad-instrument-edit, collisions, velocity independence, UI) | **Not run — need hardware/simulator** |
