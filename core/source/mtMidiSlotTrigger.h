#ifndef MT_MIDI_SLOT_TRIGGER_H_
#define MT_MIDI_SLOT_TRIGGER_H_

// MOD_MIDI_SLOT_TRIGGER - MIDI Instrument Slot Triggering.
// Maps incoming MIDI notes to instrument slots, only while the Pattern Editor is on screen with
// the Instrument column selected. Everything in this module is compiled out without the flag.

#ifdef MOD_MIDI_SLOT_TRIGGER

#include <stdint.h>

// config value ranges / enums (strMtConfig::midi.slotTrig*)
enum
{
    kSlotTrigModeDrum = 0,
    kSlotTrigModeKeyboard = 1,
    kSlotTrigModeCount = 2,

    kSlotTrigKeysChromatic = 0,
    kSlotTrigKeysWhite = 1,
    kSlotTrigKeysCount = 2,

    kSlotTrigOctaveCount = 11, // C0..C10 in MIDI numbering: notes 0,12,...,120
    kSlotTrigConfigMagic = 0x5A,
    kSlotTrigDefaultStartNote = 36,
    kSlotTrigDefaultOctave = 3, // MIDI note 36
};

// validates / defaults the settings (called from checkConfig and resetConfig)
void midiSlotTriggerCheckConfig();
void midiSlotTriggerResetConfig();

// true when the feature would intercept MIDI notes right now (enabled + Pattern Editor on screen +
// Instrument column selected)
bool midiSlotTriggerContextActive();

// Pure mapping: raw incoming MIDI note (before Middle C calibration) -> instrument slot index
// (0 based, same index space as the pattern's instrument field), -1 if the note maps to nothing.
int16_t midiSlotTriggerMapNote(uint8_t rawNote);

// Shared USB/jack hooks. rawNote is the note as received (before Middle C calibration).
// Return false when the event must be dropped.
bool midiSlotTriggerNoteOn(uint8_t rawNote);
bool midiSlotTriggerNoteOff(uint8_t rawNote);

// true only while a slot-triggered note-on is being handed to the sequencer
extern bool midiSlotTriggerNoteInProgress;

// menu labels "<note number> - <note name>", rebuilt for the current Middle C setting
extern const char* const kMidiSlotNoteLabels[128];
extern const char* const kMidiSlotOctaveLabels[kSlotTrigOctaveCount];
void midiSlotTriggerRefreshLabels();

#endif // MOD_MIDI_SLOT_TRIGGER
#endif
