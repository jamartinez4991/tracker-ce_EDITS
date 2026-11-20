# Sample Instrument FX — Implementation & Progress

**Build flag:** `-DMOD_SAMPLE_INSTRUMENT_FX` (opt-in; all three targets)
**Spec:** `FOCUS PeTracker CE  Sample Instrument FX.txt`
**Baseline:** polyend/tracker-ce `ed513015e96945829ad367e4367947b978cfd41a` · **Branch:** `mod/sample-instrument-fx`
**Status:** implemented; compiles on Original / Tracker+ / Mini with flag on and off; DSP unit-tested on host;
**not linked, not run on hardware.**

## Source files modified (exact)

| File | Change |
|---|---|
| `core/libs/Audio/effect_sampleRateReduction.h/.cpp` **(new)** | `AudioSampleRateReduction`: sample-and-hold decimator, 1 or 2 channels, one shared phase |
| `core/libs/Audio/filter_dj.h/.cpp` **(new)** | `AudioFilterDJ`: bipolar Butterworth biquad, 1 or 2 channels, shared coefficients, per-channel state |
| `core/libs/Audio/Audio.h` | includes the two new headers (flag-guarded) |
| `core/AudioEngine/AudioVoice.h/.cpp` | members declared between SVF and amplifier (construction = update order); connections; getters |
| `core/AudioEngine/mtAudioEngine.h/.cpp` | pointers, `modSampleRateReduction/modDjFilter`, **reverb+delay send taps moved to DJ-filter outputs**, neutral defaults on engine reset |
| `core/AudioEngine/mtAudioNoteOnHandle.cpp` | `handleNoteOnInstrumentFx()` called on both note-on paths (reset state + apply instrument values) |
| `core/AudioEngine/mtAudioUpdateHandle.cpp` | refresh handlers for the two new masks (active-voice updates, no retrigger) |
| `core/source/mtStructs.h` | two fields appended to `strInstrument`; two refresh masks; `INSTRUMENT_FILE_VERSION` 9→10; size assert; defaults |
| `core/FileManager/mtFileManagerOldVersionRecovery.cpp` | `recoveryInstrumentVersion9()` (explicit neutral values), registered |
| `core/FileManager/fileManagerInstrument.cpp` | neutral values in the instrument default/reset path |
| `core/interface/instrumentEditor/instrumentEditor.h` | declarations |
| `core/interface/instrumentEditor/fIinstrEdit.cpp` | Effects page slots 5/6: select buttons, encoder/up/down, delete-to-default, left/right skip slot 4 |
| `core/interface/instrumentEditor/gInstrEdit.cpp` | labels, bars, value display, frame for slots 5/6 |

**Shared with other features (no conflict expected, flagged per project rules):**
`mtStructs.h`, `mtAudioEngine.h/.cpp`, `mtAudioNoteOnHandle.cpp`, `mtAudioUpdateHandle.cpp`, `AudioVoice.*`,
`fIinstrEdit.cpp`, `gInstrEdit.cpp`, `instrumentEditor.h`, `mtFileManagerOldVersionRecovery.cpp` are also
candidates for Pitch/Modulation Extensions and Lazy Chop. **Both mods use the shared 8-byte instrument
extension block (`MOD_INSTRUMENT_EXT`, one version bump, one recovery function)** — see `../CROSS_FEATURE.md`.

## What it does

* **Sample Rate** (`uint8_t sampleRateReduction`, 0–255, 0 = exact bypass). Effective rate
  `f = 44117.647 · (100/44117.647)^(x²)`, `x = value/255` (255 → exactly 100 Hz). Q0.32 phase accumulator,
  capture on wrap, first sample after reset always captured, phase never reset on parameter change,
  no interpolation, no anti-alias filter. Bypass passes the very same block through.
* **DJ Filter** (`int8_t djFilter`, −100..+100, 0 = exact bypass). Negative = low-pass 20 kHz→20 Hz,
  positive = high-pass 20 Hz→20 kHz, logarithmic. RBJ Butterworth biquad (Q=1/√2), float transposed DF-II,
  coefficients recalculated only in `setValue()`, cutoff clamped to 0.49·fs, non-finite coefficients fall
  back to bypass, state cleared when leaving bypass, output saturated to int16.
* **Signal path** `PlayMemory → BitDepth → SVF → SampleRateReduction → DJFilter → Amplifier → …`;
  reverb/delay taps now start at the DJ filter outputs (L/R on i.MX).
* **i.MX stereo:** one paired processor per voice (2 inputs/2 outputs): shared sampling clock, independent
  held values and filter state. If either input block is missing the pair is dropped: nothing is advanced
  and the received block is released.
* **Editor (Instrument → Effects page):** slot 5 *Sample rate* (shows `Off`, `NNN Hz`, `NN.Nk`),
  slot 6 *DJ filter* (shows `Off`, `LP n`, `HP n`). Slot 4 (Instrument/MIDI Mapping) is still button-only;
  left/right skip it.
* Not modulatable, not in Performance Mode, not in the sequencer FX list, no MIDI-CC mapping (all per spec).

## Deviations / notes (spec §12 item 11)

1. `sizeof(strInstrument)` becomes **380** (372 + the shared 8-byte block that also reserves the Pitch Range /
   Polarity fields; this supersedes the earlier 376-byte layout). The `== 372` assertion is flag-conditional.
2. `INSTRUMENT_FILE_VERSION` is 10 **only when a struct-extending mod flag is set**; the recovery function
   `recoveryInstrumentVersion9` sets every field of the shared block. **Instruments saved with the mod are not readable by stock firmware.**
3. Note-on initialisation is attached to *both* note-on paths (the plain one beside
   `handleNoteOnBitCrusher()` and the sequencer-FX one beside `handleFxNoteOnBitDepth()`).
4. Left/right in the Effects page skip slot 4; the frame highlights slots 0–3, 5, 6.
5. `fileManagerProject.cpp` was *not* touched (its `bitDepth` is the master value, not the instrument's).

## Verification

| Check | Result |
|---|---|
| Compile (syntax-only, firmware flags) Original/Plus/Mini × flag on/off, 15 files | Pass |
| `static_assert` size on all targets | Pass |
| Host unit tests (`tests/test.cpp`, stubbed AudioStream, ASan+UBSan) | **ALL PASS** |
| – mapping endpoints/monotonic (p=255 → 100.00 Hz), capture spacing 441/442 samples at p=255, phase across blocks, stereo capture at identical indices, held values independent, parameter change keeps phase | Pass |
| – DJ: −3.0 dB at fc, −12.3 dB @2fc, −24 dB @4fc (≈12 dB/oct) for LP and HP; ±1..±100 stable on full-scale square/Nyquist input; R silent when only L driven; exact bypass pointer-identical; missing-partner drops pair w/o leak | Pass |
| Full firmware link, CPU/memory on device, audible quality at extremes, i.MX stereo on hardware, `.pti` v9→v10 round trip on device, editor visuals | **Not done** |

Concerns to check on device: 20 Hz low-pass in single-precision DF-II (noise/denormals), CPU with 8+ voices
(float biquad ×2 channels per voice only when the filter is engaged), whether the frame rectangles for slots
5/6 exist in `framesPlaces` as assumed.

## Dependencies
Requires nothing. Required by nothing.
