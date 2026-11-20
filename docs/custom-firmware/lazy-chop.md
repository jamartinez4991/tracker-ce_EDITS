# Lazy Chop — Implementation & Progress

**Build flag:** `-DMOD_LAZY_CHOP` (opt-in; all targets)
**Spec:** `FOCUS PeTracker CE - Lazy Chop Implementation.txt`
**Baseline:** polyend/tracker-ce `ed513015e96945829ad367e4367947b978cfd41a` · **Branch:** `mod/lazy-chop`
**Status:** implemented; compile-verified Original/Plus/Mini × flag on/off; batch-insert unit-tested on host;
**not linked, not run on hardware.**

## Source files modified (exact)

| File | Change |
|---|---|
| `core/interface/samplePlayback/samplePlayback.h` | `LazyChopSession` struct + member functions |
| `core/interface/samplePlayback/fSamplePlay.cpp` | session start/capture/finalize; hooks in note-on/off, Add, play-mode change, layout toggle, transport, `stop()`, `update()` |
| `core/source/mtSliceManager.h/.cpp` | new `insertSlicesAtPositions()` (existing `addSlice()` untouched) |
| `core/AudioEngine/VoiceStreamManager.h/.cpp` | deferred `requestInstrumentSliceCacheRefresh()` serviced from `update()` |
| `core/AudioEngine/mtAudioEngine.h/.cpp` | read-only `isSamplePlaying()` / `isSampleReversed()` on `playerEngine` |
| `core/libs/Audio/play_memory.h` | read-only `isReverse()` |
| `core/source/mtSequencer.h/.cpp` | read-only `getManualNoteBinding(track, source, instrument)` |

**Shared with other features:** `mtAudioEngine.h/.cpp` (also Sample Instrument FX, Pitch/LFO), `mtSequencer.h/.cpp`
(also Live Recording Toggles; deferred Master Multiplier/Step Jump). All edits are additive and flag-guarded, in
different regions. Prerequisites: none.

## Behaviour

* **Start:** after `sequencer.handleNoteOn()` in `cSamplePlayback::noteOnHandle()`, only if: slice or beat-slice mode, normal
  slice layout, no session active, `getLastUsedVoice()` valid **and** the sequencer track really holds an open manual note
  whose source and instrument match **and** the voice is playing forward. Otherwise nothing changes.
* **Capture:** Button 2 / Insert (both go through `functAddSlice()`) while a session is active store
  `getWavePosition()` (absolute normalised position) in a pending list — no slice-table change, no playback change, no cache work.
  Exact duplicates ignored; capacity (existing incl. zero boundary + pending ≤ 48) respected; reversed voice ⇒ press consumed but nothing
  captured. If the binding has become invalid the session is finalised and the press falls through to ordinary midpoint Add.
  No session ⇒ ordinary Add, unchanged. Alternate layout unchanged (session cannot start there; toggling layout finalises).
* **Finalize (idempotent, `finalizing` guard):** initiating source released; voice stopped (natural end/cut/replacement, detected per voice
  via `isPlaying()` and the sequencer binding in `update()`); play-mode change; layout toggle; transport play (before voices are cut);
  module `stop()` (before `stopManualNotes()`); instrument change (`update()`). Always commits to the instrument captured at start;
  invalid instrument index ⇒ discarded safely. UI refresh only if that instrument is still the one on screen.
* **Commit:** `insertSlicesAtPositions()` sorts, creates the zero boundary only if something is inserted, drops duplicates (pending and
  existing), rejects position 65535 (sample end), never exceeds 48, keeps existing boundaries, selects the last inserted boundary, leaves the
  selection alone when nothing was inserted.
* **Streamed samples:** the commit never rebuilds the cache itself. It calls `requestInstrumentSliceCacheRefresh()`; `VoiceStreamManager::update()`
  runs the refresh only when no streamed job is in progress, `fs->isAsyncReadBusy()` is false and no active voice uses that instrument.
  (Conservative: any in-progress job blocks every instrument.) Ordinary Add/Remove still refresh immediately, as before.

## Spec §14 verification items

1. Voice mapping: sequencer track index == `instrumentPlayer[]` index in the manual-note branch (`setLastUsedVoice(trackIdx)`); verified in source and checked at runtime via the binding.
2. Track state: manual preview sets `noteOpen`, `noteSource`, `stepSent.instrument` in `handleNoteOn()`'s play-mode branch; used as-is.
3. Natural end: direct per-voice `isPlaying()` polled in `update()` (no dependence on `interfacePlayingEndFlag`).
4. Endpoint: `getPosition()` can return 65535 — rejected at commit. Very short samples untested.
5. Instrument lifetime: index only; range-checked at commit.
6. Stream scheduler: condition above; the DMA completion path clears `currentJob.inProgress`.
7. UI refresh: replicates the non-cache part of `functAddSlice()` (zoom position, select/adjust values, slice points).
8. Context: all lazy-chop code runs in the UI/main loop; the engine side only exposes read accessors. Slice-table writes are not synchronised with the audio ISR beyond what ordinary Add already does.

## Limitations / not verified

* Notes triggered by an external MIDI keyboard that bypass `cSamplePlayback::noteOnHandle()` do not start a session.
* Another gesture while a session runs neither replaces nor joins it (as specified).
* Pending markers are not drawn on the waveform until commit (spec: not visible as committed boundaries).
* No hardware/simulator run; regression and streaming tests in spec §13 not executed.
* Host test (`tests/test.cpp`): ordering, duplicates, existing-duplicate, zero boundary, 65535 reject, 48 cap, selection — **pass**.
