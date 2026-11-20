// MOD_SAMPLE_INSTRUMENT_FX - DJ Filter (see features/sample-instrument-fx)
//
// Bipolar, non-resonant 2-pole (12 dB/oct) Butterworth biquad.
//   value  -100 .. -1 : low-pass,  cutoff 20 kHz (at -1) .. 20 Hz (at -100), logarithmic
//   value  0          : exact bypass (block passed through untouched, no filter state touched)
//   value  +1 .. +100 : high-pass, cutoff 20 Hz (at +1) .. 20 kHz (at +100), logarithmic
// One instance serves a whole voice: 1 channel (original Tracker) or 2 (i.MX L/R). Coefficients are
// shared, state is per channel.
#ifndef LIBS_AUDIO_FILTER_DJ_H_
#define LIBS_AUDIO_FILTER_DJ_H_
#ifdef MOD_SAMPLE_INSTRUMENT_FX
#include "AudioStream.h"

class AudioFilterDJ : public AudioStream
{
  public:
    static constexpr uint8_t kMaxChannels = 2;
    static constexpr float kMinCutoffHz = 20.0f;
    static constexpr float kMaxCutoffHz = 20000.0f;

    explicit AudioFilterDJ(uint8_t channelCount)
      : AudioStream(channelCount, inputQueueArray)
      , channels(channelCount)
    {
    }
    virtual void update(void);

    void setValue(int8_t value); // clamps to -100..100; recalculates coefficients here, not per sample
    void reset();                // clears filter state

    // Cutoff in Hz for a non-zero value (LP for <0, HP for >0).
    static float cutoffHz(int8_t value);

  private:
    audio_block_t* inputQueueArray[kMaxChannels];
    const uint8_t channels;
    bool active = false;
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; // normalised (a0 = 1)
    float z1[kMaxChannels] = { 0.0f, 0.0f };                       // transposed direct form II
    float z2[kMaxChannels] = { 0.0f, 0.0f };
};

#endif /* MOD_SAMPLE_INSTRUMENT_FX */
#endif /* LIBS_AUDIO_FILTER_DJ_H_ */
