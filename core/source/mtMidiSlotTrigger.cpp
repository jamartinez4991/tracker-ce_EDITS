#ifdef MOD_MIDI_SLOT_TRIGGER

#include "mtMidiSlotTrigger.h"

#include "mtSequencer.h"
#include "mtStructs.h"

#include "core/interface.h"
#include "patternEditor/patternEditor.h"

#include <cstdio>

extern cPatternEditor patternEditor;

bool midiSlotTriggerNoteInProgress = false;

// raw MIDI notes whose note-on was accepted by the slot mapper. A note-off for one of these is
// always passed on, even if settings / column changed while it was held.
static uint32_t heldRaw[4] = { 0, 0, 0, 0 };

static inline bool isHeld(uint8_t n)
{
    return (heldRaw[n >> 5] >> (n & 31)) & 1u;
}
static inline void setHeld(uint8_t n, bool v)
{
    if (v)
        heldRaw[n >> 5] |= (1u << (n & 31));
    else
        heldRaw[n >> 5] &= ~(1u << (n & 31));
}

void midiSlotTriggerResetConfig()
{
    mtConfig.midi.slotTrigEnabled = 0;
    mtConfig.midi.slotTrigMode = kSlotTrigModeDrum;
    mtConfig.midi.slotTrigKeys = kSlotTrigKeysChromatic;
    mtConfig.midi.slotTrigStartNote = kSlotTrigDefaultStartNote;
    mtConfig.midi.slotTrigStartOctave = kSlotTrigDefaultOctave;
    mtConfig.midi.slotTrigMagic = kSlotTrigConfigMagic;
}

void midiSlotTriggerCheckConfig()
{
    // configs saved by firmware without this feature never contained these bytes: no magic -> defaults
    if (mtConfig.midi.slotTrigMagic != kSlotTrigConfigMagic)
    {
        midiSlotTriggerResetConfig();
        return;
    }
    if (mtConfig.midi.slotTrigEnabled > 1)
        mtConfig.midi.slotTrigEnabled = 0;
    if (mtConfig.midi.slotTrigMode >= kSlotTrigModeCount)
        mtConfig.midi.slotTrigMode = kSlotTrigModeDrum;
    if (mtConfig.midi.slotTrigKeys >= kSlotTrigKeysCount)
        mtConfig.midi.slotTrigKeys = kSlotTrigKeysChromatic;
    if (mtConfig.midi.slotTrigStartNote > 127)
        mtConfig.midi.slotTrigStartNote = kSlotTrigDefaultStartNote;
    if (mtConfig.midi.slotTrigStartOctave >= kSlotTrigOctaveCount)
        mtConfig.midi.slotTrigStartOctave = kSlotTrigDefaultOctave;
}

bool midiSlotTriggerContextActive()
{
    return mtConfig.midi.slotTrigEnabled && mtInterface.isPatternScreenActive() &&
           patternEditor.editParam == cPatternEditor::EditInstr;
}

int16_t midiSlotTriggerMapNote(uint8_t rawNote)
{
    int16_t slot;

    if (mtConfig.midi.slotTrigMode == kSlotTrigModeKeyboard &&
        mtConfig.midi.slotTrigKeys == kSlotTrigKeysWhite)
    {
        static const int8_t whiteIdx[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
        int base = 12 * mtConfig.midi.slotTrigStartOctave;
        if (rawNote < base)
            return -1;
        int k = rawNote - base;
        int w = whiteIdx[k % 12];
        if (w < 0)
            return -1; // black key
        slot = (k / 12) * 7 + w;
    }
    else // Drum, or Keyboard + Chromatic
    {
        if (rawNote < mtConfig.midi.slotTrigStartNote)
            return -1;
        slot = rawNote - mtConfig.midi.slotTrigStartNote;
    }

    if (slot >= kTotalInstrumentsCount)
        return -1;
    return slot;
}

bool midiSlotTriggerNoteOn(uint8_t rawNote)
{
    if (rawNote > 127 || !midiSlotTriggerContextActive())
        return true; // not our context: existing behaviour

    int16_t slot = midiSlotTriggerMapNote(rawNote);
    if (slot < 0)
        return false; // unmapped: ignored completely, no fallback to the selected instrument

    // single remembered-instrument state, updated whether or not recording is on
    mtProject.values.lastUsedInstrument = (uint8_t)slot;
    setHeld(rawNote, true);
    midiSlotTriggerNoteInProgress = true; // cleared by mtMidi.cpp after handleNoteOn
    return true;
}

bool midiSlotTriggerNoteOff(uint8_t rawNote)
{
    if (rawNote > 127)
        return true;

    if (isHeld(rawNote))
    {
        setHeld(rawNote, false);
        return true; // release what was triggered, regardless of current settings
    }

    if (midiSlotTriggerContextActive() && midiSlotTriggerMapNote(rawNote) < 0)
        return false; // its note-on was ignored, so ignore the release too

    return true;
}

//--------------------------------------------------------------------
// labels
static char noteLabelBuf[128][12];

const char* const kMidiSlotNoteLabels[128] = {
#define L(i) noteLabelBuf[i]
#define L8(i) L(i), L(i + 1), L(i + 2), L(i + 3), L(i + 4), L(i + 5), L(i + 6), L(i + 7)
    L8(0),   L8(8),   L8(16),  L8(24),  L8(32),  L8(40),  L8(48),  L8(56),
    L8(64),  L8(72),  L8(80),  L8(88),  L8(96),  L8(104), L8(112), L8(120),
#undef L8
#undef L
};

const char* const kMidiSlotOctaveLabels[kSlotTrigOctaveCount] = {
    noteLabelBuf[0],  noteLabelBuf[12], noteLabelBuf[24], noteLabelBuf[36],
    noteLabelBuf[48], noteLabelBuf[60], noteLabelBuf[72], noteLabelBuf[84],
    noteLabelBuf[96], noteLabelBuf[108], noteLabelBuf[120],
};

void midiSlotTriggerRefreshLabels()
{
    // Name = what the Tracker shows for the pitch the sequencer receives for this raw note,
    // i.e. after the Middle C shift (same formula as mtMidi.cpp).
    int transp = 12 * ((int)mtConfig.midi.middleC - 2);
    for (int n = 0; n < 128; n++)
    {
        int internal = n + transp;
        internal = internal < 0 ? 0 : (internal > 127 ? 127 : internal);
        snprintf(noteLabelBuf[n], sizeof(noteLabelBuf[n]), "%d - %s", n, mtNotes[internal]);
    }
}

#endif // MOD_MIDI_SLOT_TRIGGER
