# Pattern BPM + Tempo FX / FX Off semantics — Implementation & Progress

**Build flag:** `-DMOD_PATTERN_BPM`. **Prerequisite:** `MOD_ACTION_PAGE4` for its control. **Pattern file v8.**
**Baseline:** polyend/tracker-ce `ed513015` (work was NOT rebased onto upstream `6d5dbc9`; see BUILD.md) · **Branch:** `mod/seq-fixes` (on top of the merged earlier mods `b61193b`)
**Verified:** compile-checked Original/Plus/Mini, flag on/off and in subsets; full link in the merged tree. **Not run on hardware.**

## Source files modified (exact)
| File | Change |
|---|---|
| `core/source/mtStructs.h` | `PATTERN_FILE_VERSION` 8 under the flag (7 with Master Multiplier only, 6 stock) |
| `core/FileManager/fileManagerPattern.cpp` | migration `<8`: BPM bytes = OFF |
| `core/source/mtSequencer.h` | `getPatternBpm/setPatternBpm` (stored in `rezerwa2`/`rezerwa3`, little-endian, 0 = OFF, 10–400) |
| `core/source/mtSequencer.cpp` | `getCurrentTempo()` fallback chain; FX Off cancels Tempo FX; Tempo FX cleared on pattern change |
| `core/interface/patternEditor/*` | control and display — see action-page4 |

## Behaviour
Tempo priority: Tempo FX (while active) → Pattern BPM of the playing pattern → Project BPM. The pattern stores only OFF or a value. FX Off (`!`) clears an active Tempo FX; the starting tempo returns automatically; nothing saved is modified. A change of pattern clears the previous pattern's Tempo FX and the new pattern's starting tempo applies at once. Looping the same pattern keeps the stock Tempo-FX persistence. External clock sync ignores Pattern BPM (as it ignores Project BPM). Encoder: OFF → first increment starts at the Project BPM; decrement below 10 = OFF.
**Old pattern files** load as OFF; v8 files will not load on firmware without this flag... as with the Multiplier's v7, stock firmware cannot read them — keep backups. Tempo FX *stop* is not cancelled by FX Off (only the tempo).
