# MIDI Instrument Slot Triggering - Implementation & Progress

**Build flag:** `-DMOD_MIDI_SLOT_TRIGGER` (opt-in; all targets)
**Spec:** `261116 Polyend Tracker CE - MIDI Instrument Slot Trigger.txt`
**Baseline:** polyend/tracker-ce `ed513015e96945829ad367e4367947b978cfd41a` - **Branch:** `mod/midi-slot-trigger`
**Status:** implemented; compile-verified Original/Plus/Mini x flag on/off (syntax-only, not linked). **Not run on hardware, no host tests.**

## Source files modified (exact)

| File | Change |
|---|---|
| `core/source/mtMidiSlotTrigger.h` (new) | API, enums, extern labels. Whole content inside `#ifdef MOD_MIDI_SLOT_TRIGGER` |
| `core/source/mtMidiSlotTrigger.cpp` (new) | context check, note-to-slot mapping, held-note bitmask, config validation/defaults, menu label table. Whole file inside the `#ifdef` (picked up by the makefile `find`) |
| `core/source/mtMidi.cpp` | USB + jack note-on/off: keep `rawPitch` before Middle C shift, call `midiSlotTriggerNoteOn/Off` before `sendSelection()` in the `midiInVoiceMode_sequencer` branch; clear in-progress flag after `handleNoteOn` |
| `core/source/mtSequencer.cpp` | one block in the edit-mode / chord-entry branch of `handleNoteOn()`: write `micromove` for slot-triggered notes |
| `core/source/mtStructs.h` | six bytes appended to `strMtConfig::strMIDIValues` (`slotTrigEnabled/Mode/Keys/StartNote/StartOctave/Magic`) |
| `core/source/mtConfig.cpp` | `checkConfig()` validates, `resetConfig()` defaults, old-format recovery zeroes the magic |
| `core/interface/configEditor/configMenu.cpp` | MIDI group 11 -> 16 items; five new items; Middle C item becomes a "with action" item (refreshes note labels) |
| `core/interface/configEditor/configMenuDefs.h` | Drum/Keyboard and Chromatic/White keys text lists |
| `core/interface/configEditor/fConfigEdit.cpp` | `cConfigEditor::start()` rebuilds the note labels |

## Summary

An external pad controller or keyboard can trigger Tracker instrument slots. When the Pattern Editor is on screen with the **Instrument column** selected and the feature is enabled, incoming notes are mapped to slots instead of being played as pitches on the selected instrument. All other contexts keep stock behaviour. No new playback or recording engine: the mapped slot is stored in `mtProject.values.lastUsedInstrument` and the existing `Sequencer::handleNoteOn/Off` paths do the rest.

## Settings (global config, MIDI menu, not project data)

| Menu item | Values | Default |
|---|---|---|
| Slot trigger | Off / On | Off |
| Slot trig mode | Drum / Keyboard | Drum |
| Slot trig keys | Chromatic / White keys (Keyboard mode only) | Chromatic |
| Slot trig note | 0-127, shown as `36 - C3` | 36 |
| Slot trig octave | 11 octaves (note 0,12..120) shown as `36 - C3` | note 36 |

All five items are always listed (the menu group has a fixed item count, no conditional visibility); Note applies to Drum and Keyboard/Chromatic, Octave to Keyboard/White keys. Values are retained when the feature is off.

## Behaviour

* **Mapping** uses the raw MIDI note as sent by the controller, **before** the Middle C shift, so a pad keeps its slot if Middle C is changed. The pitch handed to the sequencer is still the calibrated one (shift applied exactly once, as before).
* **Drum / Chromatic:** `slot = raw - startNote`. **White keys:** C of the start octave = slot 1, C-D-E-F-G-A-B = slots 1-7, next C = 8...; black keys ignored.
* Slot = index in the pattern instrument space (samples 0-47, MIDI 48-63, synths 64-66 on FEATURE_SYNTH builds), limit `kTotalInstrumentsCount`. Notes below the start, beyond the last slot or black keys are dropped entirely (no fallback to the selected instrument). Empty slots are still triggered (existing engine behaviour).
* `lastUsedInstrument` is set on every valid mapped note-on whether recording is on or off; no separate MIDI variable. Because `handleNoteOn` runs synchronously right after, the recorded instrument is the mapped slot.
* Velocity, recording, pattern-write, held-note tracking (`liveRecHeldNotes`, keyed by note) are untouched stock code, so Recording Options still govern velocity/microtiming.
* **Note-off:** a 128-bit mask remembers raw notes whose note-on was accepted. Their note-off is always forwarded (so changing start note, mode or column while held cannot strand a note). A note-off for an ignored note is also ignored (so it cannot move the edit cursor). No new voice logic; the stock release finds the voice by pitch.
* **Microtiming (spec 6.3):** the live-rec path already writes `micromove`; edit-mode MIDI entry did not. Added for slot-triggered notes only: when Microtiming Detection is on, the sequencer is playing and the cursor step is the step being played, `micromove` is written with the same formula as live recording. With Microtiming off, or stopped, nothing is written.
* **Config compatibility:** the size-prefixed `midi` struct grows from 14 to 20 bytes. Older files (smaller size prefix) load only their bytes; `slotTrigMagic` (0x5A) is then missing so defaults are applied. `CONFIG_STRUCT_VER` unchanged. Flag-off firmware reading a flag-on file ignores the extra bytes and rewrites the smaller struct (settings lost on next save).

## Prerequisites

None. Works alone.

## Files shared with other features

Checked against `features/*/CHANGED_FILES.txt` and the `_build` tree.

| File | Also edited by |
|---|---|
| `core/source/mtStructs.h` | sample-instrument-fx, pitch-lfo-extensions, step-jump, master-multiplier (different regions: this one is the MIDI config struct) |
| `core/source/mtSequencer.cpp` | lazy-chop, live-recording-toggles, modified-chance, step-jump, master-multiplier (this change is a small `#ifdef` block in `handleNoteOn()`'s edit/chord branch; live-recording-toggles also works around live recording, so check that region on merge) |

`mtMidi.cpp`, `mtConfig.cpp`, `configMenu.cpp`, `configMenuDefs.h`, `fConfigEdit.cpp` are touched by no other feature. Use the same flags for every translation unit (the flag changes `strMtConfig`).

## Verified / not verified

Verified: syntax-only compile of all 6 touched .cpp + the new file on original/plus/mini, flag on and off (only pre-existing warning at mtSequencer.cpp:1109).

NOT verified: linking, any hardware behaviour, config round-trip from an old EEPROM image, menu rendering of 16 items and of a 128-entry list, label content, edit-mode microtiming, multi-note release, USB/jack parity, whether the sequencer-voice routing also applies on screens other than the Pattern Editor (guarded by `isPatternScreenActive()`).

## Spec ambiguities and choices

* Spec's example names (`60 - C4`) assume MIDI 60 = C4; the Tracker's own convention (Middle C setting, default C-5, `mtNotes`) names the note the sequencer receives. Labels use the Tracker convention, so with the default Middle C note 60 reads `60 - C5`, 36 reads `36 - C3`. ASCII `-` used instead of an em dash, `#` instead of the sharp glyph (display font).
* Mapping is on the raw (pre-calibration) note; the number shown is therefore the controller's MIDI note number.
* Edit-mode microtiming only applies while playing with the cursor on the playing step; outside that there is no meaningful timing to capture.
* Conditional menu visibility was not available, so all settings are always shown.
