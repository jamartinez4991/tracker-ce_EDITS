#ifdef MOD_SAMPLE_INSTRUMENT_FX
#include "filter_dj.h"
#include <math.h>

float AudioFilterDJ::cutoffHz(int8_t value)
{
    const float t = (float)(value < 0 ? -(int)value : (int)value) / 100.0f; // 0..1
    const float ratio = kMaxCutoffHz / kMinCutoffHz;                        // 1000
    if (value < 0)
        return kMaxCutoffHz * powf(1.0f / ratio, t); // 20 kHz -> 20 Hz
    return kMinCutoffHz * powf(ratio, t);            // 20 Hz  -> 20 kHz
}

AUDIO_CORE_FLASH void AudioFilterDJ::reset()
{
    for (uint8_t ch = 0; ch < kMaxChannels; ch++)
    {
        z1[ch] = 0.0f;
        z2[ch] = 0.0f;
    }
}

AUDIO_CORE_FLASH void AudioFilterDJ::setValue(int8_t value)
{
    if (value > 100)
        value = 100;
    if (value < -100)
        value = -100;

    if (value == 0)
    {
        active = false;
        return;
    }

    // Keep the cutoff safely below Nyquist so the bilinear prewarp stays well conditioned.
    float fc = cutoffHz(value);
    const float fcLimit = 0.49f * AUDIO_SAMPLE_RATE_EXACT;
    if (fc > fcLimit)
        fc = fcLimit;

    const float w0 = 2.0f * (float)M_PI * fc / AUDIO_SAMPLE_RATE_EXACT;
    const float cosw = cosf(w0);
    const float alpha = sinf(w0) * 0.70710678f; // sin(w0) / (2Q), Q = 1/sqrt(2): Butterworth

    float nb0, nb1, nb2;
    if (value < 0) // low-pass
    {
        nb0 = (1.0f - cosw) * 0.5f;
        nb1 = 1.0f - cosw;
        nb2 = nb0;
    }
    else // high-pass
    {
        nb0 = (1.0f + cosw) * 0.5f;
        nb1 = -(1.0f + cosw);
        nb2 = nb0;
    }
    const float a0 = 1.0f + alpha;
    const float na1 = -2.0f * cosw;
    const float na2 = 1.0f - alpha;

    if (!(a0 > 0.0f) || !isfinite(nb0) || !isfinite(na1) || !isfinite(na2)) // invalid -> bypass
    {
        active = false;
        return;
    }

    const bool wasActive = active;
    const float inv = 1.0f / a0;
    b0 = nb0 * inv;
    b1 = nb1 * inv;
    b2 = nb2 * inv;
    a1 = na1 * inv;
    a2 = na2 * inv;
    if (!wasActive)
        reset(); // leaving bypass: start from silence, no stale state
    active = true;
}

AUDIO_CORE_RAM void AudioFilterDJ::update(void)
{
    audio_block_t* in[kMaxChannels] = { nullptr, nullptr };
    bool any = false;
    const bool process = active;

    // Silent channels (no block) are skipped individually, see AudioSampleRateReduction::update.
    for (uint8_t ch = 0; ch < channels; ch++)
    {
        in[ch] = process ? receiveWritable(ch) : receiveReadOnly(ch);
        if (in[ch])
            any = true;
    }

    if (!any)
        return;

    if (process)
    {
        const float cb0 = b0, cb1 = b1, cb2 = b2, ca1 = a1, ca2 = a2;
        for (uint8_t ch = 0; ch < channels; ch++)
        {
            if (!in[ch])
                continue;
            float s1 = z1[ch];
            float s2 = z2[ch];
            int16_t* d = in[ch]->data;
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
            {
                const float x = (float)d[i];
                const float y = cb0 * x + s1;
                s1 = cb1 * x - ca1 * y + s2;
                s2 = cb2 * x - ca2 * y;
                float o = y;
                if (o > 32767.0f)
                    o = 32767.0f;
                else if (o < -32768.0f)
                    o = -32768.0f;
                d[i] = (int16_t)o;
            }
            z1[ch] = s1;
            z2[ch] = s2;
        }
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
