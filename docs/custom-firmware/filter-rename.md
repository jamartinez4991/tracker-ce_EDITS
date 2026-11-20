# Filter rename ("DJ filter" -> "Filter") - Implementation

**Flag:** `-DMOD_FILTER_RENAME` (all targets) · **Prerequisite:** none at compile level; the label only exists when `-DMOD_SAMPLE_INSTRUMENT_FX` is on (otherwise the flag is a harmless no-op)
**Base:** `b61193b` · branch `mod/recorder-filter-bipolar`, commit `9d8c984` · spec: "Bug Report and Notes" 3.3

## Source files modified
| File | Change |
|---|---|
| `core/interface/instrumentEditor/gInstrEdit.cpp` | Effects page slot 6 caption `"DJ filter"` -> `"Filter"` under `#ifdef MOD_FILTER_RENAME` |

Shared with other features: `gInstrEdit.cpp` is also edited by sample-instrument-fx, pitch-lfo-extensions and bipolar-mod-depth (different lines, no conflict).

## Summary / description
Label text only. Retains parameter values, range (-100..+100), LP/HP behaviour, storage (`strInstrument::djFilter`), engine (`AudioFilterDJ`) and the value readout (`Off` / `LP n` / `HP n`). A grep of all `.cpp/.h` (case-insensitive "dj filter/djfilter/dj_filter", and any string literal containing "DJ") found exactly **one** user-visible string; every other hit is an internal identifier (`djFilter`, `AudioFilterDJ`, `filter_dj.*`, `showParamsDjFilter`, ...), deliberately left unchanged. No help/menu text or docs inside the repo mention it. The mod docs (sample-instrument-fx IMPLEMENTATION.md) still call it "DJ filter" as a design name.

## Behaviour
Flag off: "DJ filter". Flag on: "Filter". Nothing else changes.

## Verified (compile only)
`gInstrEdit.cpp` compiled on original/plus/mini with {none, `-DMOD_SAMPLE_INSTRUMENT_FX`, both, `-DMOD_FILTER_RENAME` only}: all pass.
## NOT verified (hardware)
Label fit on screen ("Filter" is shorter than "DJ filter", so no overflow expected).
