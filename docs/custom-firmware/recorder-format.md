# Recorder Format (Stereo/Mono selector made visible) - Implementation

**Flag:** `-DMOD_RECORDER_FORMAT` (Tracker+ and Mini only; no-op on the original Tracker)
**Prerequisite:** `-DMOD_MONO_SAMPLING` (hard `#error` if missing). Requires nothing else.
**Base:** merged tree `b61193b` · branch `mod/recorder-filter-bipolar`, commit `efa4662` · spec: "Bug Report and Notes" 3.1

## Source files modified
| File | Change |
|---|---|
| `core/interface/sampleRecorder/sampleRecorder.h` | `MOD_RECORDER_FORMAT_ACTIVE` derivation + `#error` without MOD_MONO_SAMPLING |
| `core/interface/sampleRecorder/gSampleRec.cpp` | Format label made two-line; `showFormat()` shows effective value and shows the control; `showSource()` refreshes it |

Shared with other features: both files are also touched by **mono-sampling** (this feature is a correction on top of it; the Format selector, input handling and recorder wiring in `fSampleRec.cpp`/`fProjEdit.cpp` are unchanged).

## Summary / description
The Format button (Level column, bottom button) showed its caption but no value. Cause: `showDefaultScreen()` set the label's layout value to 0 (`display.setControlValue(label[LEVEL_COL], 0)`), which in `cLabel::update()` is the single-line layout (`lines_step_y = 0`), so the second line (text2: "Stereo"/"Mono") was drawn on top of / not beneath the caption. Gain's label uses value 1 (two-line layout). With the flag on, the Level column label now uses value 1, identical to Gain: caption "Format" on top, "Stereo"/"Mono" beneath. `showFormat()` also calls `setControlShow()` like `showGain()`.
Mic sources are mono-only in hardware, so the displayed value is the **effective** format ("Mono" for the mic sources); the stored selection (default Stereo) is not changed, and `showSource()` refreshes the label when the source changes.

## Is the setting really consumed? (traced)
1. `functActionRecord()` (fSampleRec.cpp ~1601): for non-mic sources `destinationChannels = recordMono ? 1 : 2`, `sourceChannels = 2`; mics are 1/1.
2. `Recorder::setConfig()` allocates one or two destination channels; `Recorder::update()` averages (L+R)/2 into the single channel when destination is mono / source stereo.
3. Save: every `saveRecordedSound()` call passes `getAddress(kLeft)` and `getAddress(kRight)`; `getAddress(kRight)` returns the left buffer when only one destination channel exists, and `RecordingSave::saveRecordedSound()` writes one channel when `left == right` (`header.numChannels = soundAddress.size()`). So Mono => 1-channel WAV, Stereo => 2.
4. Default Stereo (`recordMono=false` member default) and reset in `cProjectEditor::loadProjectValues()`.
Result: no code change was needed in the recording/saving path; only the display was broken.

## Behaviour
Flag off: identical to mono-sampling alone. Flag on: Format value visible beneath its label; navigation/editing unchanged (press Format, encoder/up/down toggles, Delete = Stereo).

## Verified (compile only)
`gSampleRec.cpp`, `fSampleRec.cpp`, `fProjEdit.cpp` compiled (GCC 10.3.1, `-fsyntax-only`) on original/plus/mini with: no flags, `-DMOD_MONO_SAMPLING`, `-DMOD_MONO_SAMPLING -DMOD_RECORDER_FORMAT`: all pass (only pre-existing warnings). `-DMOD_RECORDER_FORMAT` alone fails with the intended `#error`.
## NOT verified (hardware)
That value 1 gives the intended on-screen layout/fit of "Format"+"Stereo"/"Mono" (conclusion drawn from reading `cLabel::update()`), mono WAV channel count/content on a real device, project-switch reset.
