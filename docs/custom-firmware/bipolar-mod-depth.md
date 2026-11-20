# Bipolar Modulation Depth (signed envelope Amount) - Implementation

**Flag:** `-DMOD_BIPOLAR_MOD_DEPTH` (all targets)
**Prerequisite:** `-DMOD_LFO_SIGNED_DEPTH` (pitch-lfo-extensions Phase B; hard `#error` in `mtEnvelopeGenerator.h` if missing). Uses the UI/CC conventions of signed LFO depth; Phase B's canonical LFO output stage is unchanged and not otherwise required. No dependency on `MOD_INSTRUMENT_EXT`.
**Base:** `b61193b` (patch applies after filter-rename; no overlap) · branch `mod/recorder-filter-bipolar`, commit `9beac19` · spec: "Bug Report and Notes" 3.2, PENDING_REQUIREMENTS #2

## Source files modified
| File | Change |
|---|---|
| `core/source/mtEnvelopeGenerator.cpp` | ADSR attack-phase clamp made sign-aware (the actual blocker for negative amounts) |
| `core/source/mtEnvelopeGenerator.h` | `#error` if flag set without `MOD_LFO_SIGNED_DEPTH` |
| `core/interface/instrumentEditor/fIinstrEdit.cpp` | `changeEnvAmount()` signed; MIDI CC law for envelope Amount; CC dispatch of loop 2 |
| `core/interface/instrumentEditor/gInstrEdit.cpp` | `showEnvAmount()` centred bar + signed text; `showLfoAmount()` restores plain bar for Amp |

Shared with other features: all four files are also touched by pitch-lfo-extensions (generator output stage, editor Amount code); `gInstrEdit.cpp` also by sample-instrument-fx and filter-rename. Changes are in separate hunks. (`files/gInstrEdit.cpp` here is the tree after both filter-rename and this feature.)

## Summary / description
The envelope **Amount** (loop 0 ADSR and loop 2 WT envelope) becomes bipolar, -100..+100 %, like the signed LFO depth. Positive amounts behave exactly as before.

## Audit result (why this is small)
* The generator is linear in `amount` in every phase (hold, decay, sustain, release, WT env), so a negative amount already produced negative output **except in the attack phase**, where `clamp(x, killOutput, +1)` forced the output to >= 0. Only that clamp was wrong. With a negative amount (flag on) the clamp is mirrored (`[-1, min(killOutput,0)]`); amount >= 0 keeps the stock clamp unchanged. Phase transitions do not depend on the sign.
* Engine consumers (cutoff, pan, wavetable position, granular position/length, finetune/pitch) already accept negative modulation values because LFOs have always been bipolar; they clamp the final value. So both directions reach the destination. Nothing changed in mtAudioEngine/mtAudioSequencer/mtAudioUpdateHandle.
* **Storage: no change needed.** `envelope[].amount` is a float already serialized as-is, so a negative value persists in the existing field; no use of the extension block, no version bump, `INSTRUMENT_FILE_VERSION` stays as in the tree. Old files only contain amounts >= 0 and load/sound unchanged. The loop-3 mirror (`envelope[].amount = lfo[].amount`, clamped >= 0) is untouched.
* I did **not** use the LFO "canonical stage" (unit-amount generator + signed depth at output) for ADSR: it is unnecessary because ADSR amount scaling is inherent and the generator is already sign-clean; it would also have changed the kill/retrigger behaviour.
* MOD export (`fileManagerExportToMod.cpp`) only reads `lfo[].amount`, never the envelope amount: not affected.

## Editor / CC behaviour (flag on)
* Amount (non-Amp, loop 0 and 2): encoder/up/down steps whole percent, clamps at +-100, lands exactly on 0; centred bar (`controlStyleValueLeftRight_100_100`), text `0`, `+50`, `-100`; Delete resets to default (+100 %).
* **Amp destination stays unsigned (0..100):** its envelope is a fixed-point gain stage (`getMostSignificantAmount()`/`effect_envelope`); negative gain has no defined meaning. Same decision as signed LFO depth. The Amp bar gets an explicit plain style so it never inherits a centred bar.
* MIDI CC for envelope Amount (non-Amp): 0 -> -100 %, 64 -> 0 %, 127 -> +100 % (same law as signed LFO depth). **Compatibility change:** existing CC assignments to envelope Amount now centre at 64. The CC dispatcher now sends loop 2 (WT env) to the envelope Amount (stock sent it to the LFO amount, which WT env never reads); flag off keeps the stock routing.

## Known limitations
* Instruments with negative envelope amounts, played by a build **without** this flag, behave wrongly in attack (stuck at 0 until decay). Back up / keep flags consistent.
* Retriggering a negative-amount envelope mid-release starts from `killOutput` as stock; the mirrored clamp pins the attack at killOutput if the previous output was further from zero than the target (mirror of the stock quirk).
* Other envelope-type Amount-like parameters (pitch depth, etc.) are out of scope.

## Verified (compile only)
`mtEnvelopeGenerator.cpp`, `fIinstrEdit.cpp`, `gInstrEdit.cpp` (+ mtAudioSequencer/mtAudioEngine for consistency) on original/plus/mini with {none; `MOD_LFO_SIGNED_DEPTH MOD_INSTRUMENT_EXT`; + `MOD_BIPOLAR_MOD_DEPTH`; all mods on}: all pass. Flag without `MOD_LFO_SIGNED_DEPTH` gives the intended `#error`.
## NOT verified
Everything audible/visual: negative attack/decay/sustain/release on each destination, bar rendering, CC from a controller, saved/reloaded negative amounts, retrigger behaviour. No host test of the generator was written.
