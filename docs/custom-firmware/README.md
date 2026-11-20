# Custom firmware overview

Integrated branch: `custom-firmware`. Based on upstream `922bba0` (polyend/tracker-ce main). Every feature is opt-in with a `-DMOD_*` flag (`make plus CUSTOM_DEFINES="-DMOD_..."`). With no flags the build is stock behaviour.

## Features, flags and dependencies

| PR branch | Flags | Depends on |
|---|---|---|
| feature/mono-sampling | MOD_MONO_SAMPLING | - |
| feature/recorder-format | MOD_RECORDER_FORMAT | mono-sampling |
| feature/sample-instrument-fx | MOD_SAMPLE_INSTRUMENT_FX (instrument ext block v10) | - |
| feature/filter-rename | MOD_FILTER_RENAME | sample-instrument-fx |
| feature/pitch-lfo-extensions | MOD_PITCH_RANGE, MOD_LFO_SIGNED_DEPTH, MOD_LFO_POLARITY | sample-instrument-fx |
| feature/bipolar-mod-depth | MOD_BIPOLAR_MOD_DEPTH | pitch-lfo-extensions |
| feature/lazy-chop | MOD_LAZY_CHOP | - |
| feature/live-recording-toggles | MOD_LIVE_REC_TOGGLES | - |
| feature/midi-slot-trigger | MOD_MIDI_SLOT_TRIGGER | - |
| feature/step-jump | MOD_STEP_JUMP (FX id 51) | - |
| feature/cond-chance | MOD_COND_CHANCE (FX id 52) | step-jump |
| feature/master-multiplier | MOD_MASTER_MULTIPLIER (pattern v7) | - |
| feature/pattern-progression | MOD_PATTERN_PROGRESSION | master-multiplier |
| feature/pattern-bpm | MOD_PATTERN_BPM (pattern v8) | master-multiplier |
| feature/action-page4 | MOD_ACTION_PAGE4 | pattern-bpm, live-recording-toggles (via `stack/page4-base`) |
| feature/realtime-quantize-step-entry | MOD_REALTIME_QUANTIZE, MOD_STEP_ENTRY_DIVISION | action-page4 |

Modified Chance was discarded (replaced by Cond. Chance).

## Migration notes
- Stage branches are derived from the integrated tree by stripping the preprocessor blocks of excluded features, so each stage compiles on its own.
- Sample Instrument FX (SRR and DJ filter) is added to the new unified per-channel voice. Both effects now process each present channel independently, because the upstream null-silence graph leaves one side of mono samples without a block.
- Mono sampling remains Plus/Mini only; the Original now supports stereo recording upstream (open decision).
- Lazy Chop interacts with the new VoiceStreamManager slice caches: builds, runtime behaviour not verified.
- Hardware testing: none performed.
