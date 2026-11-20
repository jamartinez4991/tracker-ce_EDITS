// MOD_SAMPLE_INSTRUMENT_FX - Sample-Rate Reduction (see features/sample-instrument-fx)
//
// Sample-and-hold decimator. Captures the input at a reduced effective rate and holds each captured
// value until the next capture event. No interpolation and no anti-alias filter (aliasing is the
// intended sound). The engine's real output rate, pitch, speed and duration are untouched.
//
// One instance serves a whole voice: 1 channel on the original Tracker, 2 channels (L/R) on i.MX.
// Both channels share ONE phase accumulator (capture events are simultaneous); only the held values
// are per channel.
#ifndef LIBS_AUDIO_EFFECT_SAMPLERATEREDUCTION_H_
#define LIBS_AUDIO_EFFECT_SAMPLERATEREDUCTION_H_
#ifdef MOD_SAMPLE_INSTRUMENT_FX
#include "AudioStream.h"

class AudioSampleRateReduction : public AudioStream
{
  public:
    static constexpr uint8_t kMaxChannels = 2;
    static constexpr float kMaxRateHz = AUDIO_SAMPLE_RATE_EXACT; // parameter 0 (bypass)
    static constexpr float kMinRateHz = 100.0f;                  // parameter 255

    explicit AudioSampleRateReduction(uint8_t channelCount)
      : AudioStream(channelCount, inputQueueArray)
      , channels(channelCount)
    {
    }
    virtual void update(void);

    // 0 = exact bypass, 255 = 100 Hz. Does not reset the sampling phase.
    void setReduction(uint8_t value);
    // Deterministic voice-start state: first sample is captured, no stale held data.
    void reset();

    // Effective sampling frequency for a parameter value: fmax * (fmin/fmax)^(x^2), x = value/255.
    static float effectiveRateHz(uint8_t value);

  private:
    audio_block_t* inputQueueArray[kMaxChannels];
    const uint8_t channels;
    uint32_t phaseIncrement = 0; // Q0.32 fraction of an output sample per capture; 0 = bypass
    uint32_t phase = 0;
    bool captureNext = true;
    int16_t held[kMaxChannels] = { 0, 0 };
};

#endif /* MOD_SAMPLE_INSTRUMENT_FX */
#endif /* LIBS_AUDIO_EFFECT_SAMPLERATEREDUCTION_H_ */
