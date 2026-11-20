# Pattern Editor Action Button Page 4 (+ Master Multiplier editing fix) — Implementation & Progress

**Build flag:** `-DMOD_ACTION_PAGE4` — the framework. Each control appears only if its own feature flag is built: Note Off / Microtiming (`MOD_LIVE_REC_TOGGLES`), Quant + Q.Str (`MOD_REALTIME_QUANTIZE`), SQnt (`MOD_STEP_ENTRY_DIVISION`), Multiplier (`MOD_MASTER_MULTIPLIER`), Pat. BPM (`MOD_PATTERN_BPM`). With the flag on, the recording toggles and the Multiplier **move off Page 3** (Page 3 is then unchanged from stock).
**Baseline:** polyend/tracker-ce `ed513015` (work was NOT rebased onto upstream `6d5dbc9`; see BUILD.md) · **Branch:** `mod/seq-fixes` (on top of the merged earlier mods `b61193b`)
**Verified:** compile-checked Original/Plus/Mini, flag on/off and in subsets; full link in the merged tree. **Not run on hardware.**

## Source files modified (exact)
| File | Change |
|---|---|
| `core/interface/patternEditor/patternEditor.h` | page count 4, Page 4 declarations, `MOD_P3_*` helper macros, #error for quantize without live-rec toggles, step-entry state |
| `core/interface/patternEditor/fPatternEdit.cpp` | `setActionButtonPage4Funct`, held-button handler template, encoder / Left / Right / Up / Down / pad / pad-lighting dispatch, Page 3 registrations guarded, `refreshActualActionButtonPage` case 3, SQnt cursor move |
| `core/interface/patternEditor/gPatternEdit.cpp` | `showActionButtonPage4Labels`, `showPage4Values`, 4 page dots, Page 3 labels guarded, `showLength`/`showMultiplier` refresh Page 4 |

## Layout (Page 4)
0 Note Off (toggle) · 1 Microtiming (toggle) · 2 Quant · 3 Q.Str · 4 SQnt · 5 Multiplier · 6 Pat. BPM · 7 More. Shift and Shift+Rec layers are untouched.

## Interaction (same model as Pattern / Length / Step Jump)
Hold the button → focus leaves the pattern, label frame highlights; encoder and Left/Down (−) Right/Up (+) edit; release → focus returns. Key repeat/acceleration is whatever the input layer provides (no new timer). Pads select directly, press only:
| Control | Pads |
|---|---|
| Quant, SQnt | pads 1–6 = the six divisions |
| Q.Str | 48 pads scaled to 0–100 % |
| Multiplier | pads 1–8 = x1–x8 |
| Pat. BPM | pad 1 = OFF, pads 2–48 scaled to 10–400 (encoder is 1 BPM) |
The selected value's pad is lit.

## Master Multiplier fix
Cause: on Page 3 the button was registered for **press only** and used a wrong `selectedPlace`, so the held-parameter lifecycle (release → refocus) and the highlight frame were broken. Now: Page 4 button 5, press **and** release registered through the shared template, `selectedPlace` = button index (what the frame uses). Range x1–x8, pads 1–8.

## Notes
`selectedPlace` equals the button index, so Page 4 places 5/6 would collide with Page 1's undo/redo; dispatch is therefore gated on `actualActionButtonsPage == 3`, which cannot change while a button is held.

## v4 fix — Shift+Rec "Live REC" showed Page 4 values
**Files:** `core/interface/patternEditor/patternEditor.h` (`recArmingShown`), `core/interface/patternEditor/gPatternEdit.cpp` (`showRecordArmingButtons`, `showPage4Values`, `showActionButtonPage4Labels`, `showDefaultScreen`).
**Cause:** `showRecordArmingButtons` wrote "Live REC" / Armed / Unarmed, but `showPage4Values()` (called from `showLength`, pattern refreshes, toggles) rewrote the value line of the same label controls afterwards. Now a flag set by the arm interface makes `showPage4Values()` do nothing until the interface is left (Shift release → full Page 4 redraw, or the default screen). Arm functionality is untouched. Multiplier pads on Page 4 are now 1–8.
