#ifndef SOURCE_MTSLICEMANAGER_H_
#define SOURCE_MTSLICEMANAGER_H_
#include "mtStructs.h"

class mtSliceManager
{
  public:
    void addSlice(strInstrument* instr);
#ifdef MOD_LAZY_CHOP
    // Explicit-position batch insert (normalised 0..65534 positions). Sorts, drops exact duplicates
    // (inside the batch and against existing boundaries), honours MAX_SLICE_NUMBER, keeps all existing
    // boundaries. Selects the last inserted boundary. Returns the number inserted (0 = instrument
    // untouched, including no zero boundary created).
    uint8_t insertSlicesAtPositions(strInstrument* instr, const uint16_t* positions, uint8_t count);
#endif
    void removeSlice(strInstrument* instr);
    void adjustSlice(strInstrument* instr, int32_t val);
    void autoSlice(strInstrument* instr);
    void equalSlice(strInstrument* instr, uint8_t n);

    uint8_t getAutoSliceProgress();
    uint8_t getAutoSliceState();
};

extern mtSliceManager sliceManager;

#endif /* SOURCE_MTSLICEMANAGER_H_ */
