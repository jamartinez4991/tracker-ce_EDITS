# Pitch and Modulation Extensions — implementation notes

Spec: `FOCUS PeTrackdr CE - Pitch and Modulation Extensions v1.txt` · baseline `ed513015` · branch `mod/pitch-lfo-extensions`
Status: **implemented, compile-checked only** (arm-none-eabi-gcc 15.2 `-fsyntax-only`, original/plus/mini). Not linked, not run on hardware.

## Source files modified (see CHANGED_FILES.txt / pitch-lfo-extensions.patch)
| File | Phase | Change |
|---|---|---|
| core/libs/Audio/play_memory.h / .cpp | A | `PM_PITCH_HI/LO` macros; modulation state; `update()` wrapper; `setPitchModulationCents()`; reset in `play()` |
| core/libs/Audio/play_memory_{single_shot,loop_forward,loop_backward,loop_pingpong,granular,wavetable}.cpp | A | glide clamps use `PM_PITCH_HI/LO(table)` (mechanical; identical expansion when flag is off) |
| core/AudioEngine/mtAudioEngine.h / .cpp | A, B/C | `modPitchModulation()`; canonical LFO setup in `calcLfoBasedEnvelope` |
| core/AudioEngine/mtAudioUpdateHandle.cpp | A | pitch refresh sends float cents instead of the integer fine-tune contribution |
| core/AudioEngine/mtAudioSequencer.cpp | B/C | `initEnvelopesParameters` passes signed depth + polarity to the generator |
| core/source/mtEnvelopeGenerator.h / .cpp | B/C | output stage: `(unipolar ? (x+1)/2 : x) * signedDepth` for loop modes 1 and 3 |
| core/source/mtStructs.h | all | shared flag derivations, shared 8-byte instrument extension block, version 10, size assert 380, defaults |
| core/FileManager/fileManagerInstrument.cpp, mtFileManagerOldVersionRecovery.cpp | A, C | defaults + `recoveryInstrumentVersion9` |
| core/interface/instrumentEditor/instrumentEditor.h, fIinstrEdit.cpp, gInstrEdit.cpp | all | signed Amount, CC mapping, Amount-column sub-parameters, display |

## Opt-in flags (independent; `CUSTOM_DEFINES`)
* `-DMOD_PITCH_RANGE` (Phase A) · `-DMOD_LFO_SIGNED_DEPTH` (Phase B) · `-DMOD_LFO_POLARITY` (Phase C).
* Derived in mtStructs.h: `MOD_INSTRUMENT_EXT` (A or C or Sample FX), `MOD_AMOUNT_SUBPARAMS` (A or C), `MOD_LFO_CANONICAL` (B or C).
* B and C share the canonical LFO output stage (prerequisite); A is independent of B/C. All flags off = byte-identical behaviour to baseline.

## Phase A — Extended pitch range
* Depth = `pitchModSemitones*100 + pitchModCents` cents (−24..+24 st, −100..+100 ct, independent, additive). Default 0 st / +100 ct = the stock ±1 st range, so existing instruments sound unchanged.
* `modulation_cents = envelope/LFO output × combined depth` (output already includes the existing Amount). Clamped to ±2500 cents.
* The float cents go to `AudioPlayMemory::setPitchModulationCents()`, which caches `pitchModulationRatio = 2^(cents/1200)` (computed on change, never per sample). The integer fine-tune path is bypassed for modulation only; ordinary fine-tune, tune, sequencer fine-tune FX unchanged.
* `update()` swaps the effective pitch (`pitchControl × ratio`, clamped to note-table ends ×0.25 / ×4) in for the block, runs the existing mode code unchanged, then restores the base. Glide step is scaled by the same ratio, and any glide advance in the block is converted back to the base domain — so modulation never accumulates into `pitchControl`. Covers single shot, forward/backward/ping-pong loops, wavetable, granular, slice and beat-slice (they read `pitchControl` at call time).
* Glide clamps inside the loops compare against the table ends × the bound scales while modulating, so glide + modulation is not truncated.
* Pitch-consumer inventory: every `pitchControl` read is in `play_memory*.cpp` (list in the session audit); nothing outside that class reads it.
* UI: on the Pitch destination, press the selected **Amount** button again to cycle Amount → [Polarity] → Pitch st → Pitch ct. The value text shows the combined result (`+12.50 st`, `0.00 st`); the bar shows the control being edited. Combined display is read-only.

## Phase B — Signed LFO depth (not Amp)
* Editor range −100..+100 % in whole-percent steps (exact zero), centred bar, explicit sign. Loop 3 keeps `envelope[].amount` non-negative; signed value lives in `lfo[].amount`.
* Engine runs the generator at unit amount (canonical −1..+1; random S&H draws 0..1 → 2r−1, timing unchanged) and applies depth once at the output. Old positive amounts keep their exact meaning.
* MIDI CC for LFO amount (loop 1/3, non-Amp): 0 → −100 %, 64 → 0 %, 127 → +100 %, monotonic. **Compatibility change:** existing CC assignments to LFO Amount now centre at 64 (stock: 0..127 → 0..100 %). No compatibility mode, per spec.
* Export: `fileManagerExportToMod.cpp` clamps amounts to ≥0 already; negative depth exports as 0 (internal value untouched). Not modified.
* Automation: only MIDI CC edits LFO amount (no sequencer FX targets it); audited, no other clamp found.

## Phase C — Polarity
* `lfoPolarityMask` (bit n = envelope slot n Unipolar) in the shared extension block — chosen over packing a bit into `shape`/`wtSust` because ≥30 raw `shape` consumers (note-on, sequencer, engine, update handle, editor) would need masking. Default 0 = Bipolar, set explicitly by defaults and by the v9→v10 recovery.
* Applied in the output stage only: no restart, no phase change, no new random sample. Works for both standard and wavetable LFOs.
* UI: Amount column cycles to **Polarity** (Bipolar/Unipolar); any encoder movement toggles. Placed there because the standard LFO page has free columns but the wavetable LFO page has none.

## Shared storage change
`strInstrument` 372 → 380 bytes, `INSTRUMENT_FILE_VERSION` 9 → 10 whenever any of Sample FX / Pitch Range / Polarity is enabled. Block layout is fixed (`sampleRateReduction, djFilter, pitchModSemitones, pitchModCents, lfoPolarityMask, reserved[3]`), so files move freely between mod builds. **Not readable by stock firmware.** See `../CROSS_FEATURE.md`.

## Known limitations / deviations (be explicit)
1. **Amp destination excluded from B and C.** Its LFO is a fixed-point gain stage (`effect_envelope.cpp`); negative or unipolar-shifted depth has no defined meaning there. Needs a design decision.
2. Polarity/pitch-depth are reached by **press-again cycling of the Amount button**, not dedicated columns (no free slot on the WT-LFO page). Delete-to-default does not cover the sub-parameters.
3. No modulation smoothing was added (spec §4.3: only if stepping is audible) — untested on hardware.
4. Effective-pitch bounds (×0.25 / ×4 of the note table) are a design choice, not measured; high ratios on granular/wavetable need on-device CPU/aliasing checks.
5. MOD export clamps negative depth to 0 (see above).

## Verification actually run
* Compile (`-fsyntax-only`, all three targets) for: no flags; each flag alone; A+B+C together; plus a trial merge of all five features with every flag on (28 changed .cpp files, no errors; only pre-existing warnings).
* `tests/formulas.cpp` (host): CC 0/64/127 mapping + monotonic; additive examples (+12.50, +11.50, 0.00, ±25 st); unipolar = linear map; signed-depth truth table; glide/base restore algebra. These test copies of the expressions, not the firmware build.
## Not run (needs hardware)
Acceptance plan §9 in full: audible pitch accuracy per playback mode, glide + modulation, polarity phase continuity, random S&H, CC from a controller, instrument save/reload, v9 → v10 migration load.
