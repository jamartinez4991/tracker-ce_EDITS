#ifdef MOD_SAMPLE_INSTRUMENT_FX
#include "effect_sampleRateReduction.h"
#include <math.h>

float AudioSampleRateReduction::effectiveRateHz(uint8_t value)
{
    const float x = (float)value / 255.0f;
    return kMaxRateHz * expf(x * x * logf(kMinRateHz / kMaxRateHz));
}

AUDIO_CORE_FLASH void AudioSampleRateReduction::setReduction(uint8_t value)
{
    uint32_t inc = 0;
    if (value != 0)
    {
        // ratio = effective rate / output rate, strictly < 1 for value > 0.
        const float ratio = effectiveRateHz(value) / kMaxRateHz;
        const float scaled = ratio * 4294967296.0f;
        inc = scaled >= 4294967040.0f ? 0xFFFFFF00u : (uint32_t)scaled;
        if (inc == 0)
            inc = 1;
    }
    if (phaseIncrement == 0 && inc != 0)
        captureNext = true; // leaving bypass: never output stale held data
    phaseIncrement = inc;   // phase is deliberately preserved
}

AUDIO_CORE_FLASH void AudioSampleRateReduction::reset()
{
    phase = 0;
    captureNext = true;
    held[0] = 0;
    held[1] = 0;
}

AUDIO_CORE_RAM void AudioSampleRateReduction::update(void)
{
    audio_block_t* in[kMaxChannels] = { nullptr, nullptr };
    bool any = false;

    // The upstream audio graph leaves silent channels (e.g. the right side of a mono sample)
    // without a block, so each channel is processed independently when present.
    for (uint8_t ch = 0; ch < channels; ch++)
    {
        in[ch] = (phaseIncrement == 0) ? receiveReadOnly(ch) : receiveWritable(ch);
        if (in[ch])
            any = true;
    }

    if (!any)
        return; // nothing arrived: advance no state

    if (phaseIncrement != 0)
    {
        uint32_t ph = phase;
        const uint32_t inc = phaseIncrement;
        bool first = captureNext;
        int16_t h0 = held[0];
        int16_t h1 = held[1];

        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
        {
            const uint32_t prev = ph;
            ph += inc;
            if (first || ph < prev) // wrap = capture event, shared by both channels
            {
                if (in[0])
                    h0 = in[0]->data[i];
                if (in[1])
                    h1 = in[1]->data[i];
                first = false;
            }
            if (in[0])
                in[0]->data[i] = h0;
            if (in[1])
                in[1]->data[i] = h1;
        }

        phase = ph;
        captureNext = first;
        held[0] = h0;
        held[1] = h1;
    }

    for (uint8_t ch = 0; ch < channels; ch++)
    {
        if (!in[ch])
            continue;
        transmit(in[ch], ch);
        release(in[ch]);
    }
}
#endif /* MOD_SAMPLE_INSTRUMENT_FX */
