# Mono Sampling — Implementation & Progress

**Build flag:** `-DMOD_MONO_SAMPLING` (opt-in; effective on Tracker+ and Tracker Mini only)
**Spec:** `FOCUS PeTracker CE - Mono Sampling.txt`
**Baseline:** polyend/tracker-ce `ed513015e96945829ad367e4367947b978cfd41a`
**Branch:** `mod/mono-sampling` · **Status:** implemented, compile-verified (3 targets), **not run on hardware**

## Source files modified

| File | Change |
|---|---|
| `core/interface/sampleRecorder/sampleRecorder.h` | `recordMono` runtime field in `strRecorderConfig`; `FORMAT_COL`, availability/selection helpers, `showFormat/changeFormatSelection/setDefaultFormat` declarations |
| `core/interface/sampleRecorder/fSampleRec.cpp` | Destination channel count honours `recordMono`; Format button/encoder/up/down/left/right/delete handling; state functions |
| `core/interface/sampleRecorder/gSampleRec.cpp` | Draws the Format caption on the Level column button; spectrum shows one channel when Mono |
| `core/interface/projectEditor/fProjEdit.cpp` | Resets Format to Stereo in `cProjectEditor::loadProjectValues()` |

Not modified (by design): `core/source/Recorder.cpp`, `core/source/mtStructs.h`, any save/import code.

**Shared with other features:** none of the five in-scope FOCUS features touch these files.
(Deferred "sample recorder monitoring" work would also touch `fSampleRec.cpp` / `gSampleRec.cpp`.)

## What it does

Adds a Stereo/Mono choice to the Sample Recorder config screen. Stereo is the default.
Mono records a stereo-capable source (Line In, Radio, USB) into **one** destination channel using the
recorder's existing `(L + R) / 2` average in `Recorder::update()`. No new DSP was written.

* Stereo-capable source: `sourceChannels = 2`, `destinationChannels = recordMono ? 1 : 2`.
* Mono-only source (mics): unchanged, always 1/1. Selecting Stereo does not duplicate the channel.
* Setting is read once in `functActionRecord()` when a recording is configured, so it cannot change an
  active recording.
* Runtime only — not in `mtProject.values`, no project/config version bump.
* Reset to Stereo on power-up (member default) and in `loadProjectValues()` (project load).
* Original Tracker: no change at all (already mono-only; code is compiled out).
* Mono destinations allocate one buffer, so max duration roughly doubles in the same sample memory.
* Mono recordings use the existing one-channel save path (`getAddress(kRight)` returns the left buffer
  when only one destination channel exists) — the same path the mic sources already use.

## UI placement (spec §3.6: reachable for every source, no existing control displaced)

The Format selector uses the **Level column's bottom button** (Tracker+ col 4, Mini col 1). That column
was a display-only meter with a "Level" caption and was previously not selectable, and Radio never uses
it, so the control is reachable for Line In, Radio, USB and both mics, and no existing control is
overwritten. The level bar is unchanged; only its caption becomes `Format` with `Stereo`/`Mono` below.

* Navigation order (left/right): Source · [Radio controls] · **Format** · Gain · Monitor.
* Press to select, press again / encoder / up / down to change, Delete resets to Stereo.
* The choice is never touched by source changes.
* Trade-off: the caption "Level" is no longer drawn (the meter itself still is). **On-device visual
  check needed** (text2 line under the caption, label fit).

Superseded earlier design (Format in the first Radio column, hidden while Radio was the source) was
replaced because it violated the spec.

## Dependencies

* Requires: nothing.
* Required by: nothing. (The deferred SampleTrak "recording grade" work depends on Mono/Stereo existing.)

## Verification

| Check | Result |
|---|---|
| Compiles with `-DMOD_MONO_SAMPLING`: original, Tracker+, Mini (3 touched .cpp files) | Pass |
| Compiles with flag off | Pass |
| New symbols present only for Plus/Mini with flag on | Pass |
| Diff vs baseline is minimal, CRLF/LF line endings preserved | Pass (+140/−0) |
| Full firmware link | **Not done** (no `agt` 10.3 toolchain here; used GCC 15.2 syntax-compile) |
| On-device: Format UI navigation, mono WAV channel count, averaged content, project-switch reset, new-project reset, duration | **Not tested** |

To verify on device: record L-only/R-only test tones in Mono and confirm the sample is the average and
one channel; switch projects and confirm Stereo returns; confirm *New Project* also goes through
`loadProjectValues()` (only `WorkspaceProjectLoad.cpp` was found calling it).
