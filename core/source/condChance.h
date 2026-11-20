// Conditional Chance FX ("Cond. Chance") - pure value decoding and decision logic.
// Header-only and free of firmware dependencies so it can be host-tested (tests/cond-chance).
//
// FX value (1 byte, 0..172) - three formats only, mirroring the stock Chance descriptions:
//   0..100     probability in percent                         "Cond: xx%"
//   101..136   Play n then Skip m  (n,m = 1..6, idx = (n-1)*6+(m-1))   "Cond: Play n / Skip m"
//   137..172   Skip n then Play m  (same idx)                           "Cond: Skip n / Play m"
// A counted condition owns a per-track, per-step encounter phase (0 .. n+m-1). Every encounter of the
// step advances it exactly once, whether the step played or not.
#pragma once
#include <stdint.h>

namespace condChance
{
constexpr uint8_t kPctMax = 100;
constexpr uint8_t kPlaySkipStart = 101;
constexpr uint8_t kSkipPlayStart = 137;
constexpr uint8_t kMaxValue = 172;
constexpr uint8_t kDefaultValue = 50; // 50 %

struct Decoded
{
    bool counted;   // false = pure probability
    uint8_t pct;    // probability 0..100 (counted conditions always 100)
    bool playFirst; // counted only: true = Play N then Skip M
    uint8_t n;      // counted only: 1..6
    uint8_t m;      // counted only: 1..6
};

inline Decoded decode(uint8_t value)
{
    Decoded d{ false, 100, true, 1, 1 };
    if (value > kMaxValue)
        value = kMaxValue;
    if (value <= kPctMax)
    {
        d.pct = value;
        return d;
    }
    const uint8_t idx = value - kPlaySkipStart; // 0..71
    d.counted = true;
    d.playFirst = idx < 36;
    d.n = 1 + (idx % 36) / 6;
    d.m = 1 + (idx % 36) % 6;
    return d;
}

inline uint8_t encodeCounted(bool playFirst, uint8_t n, uint8_t m)
{
    return (playFirst ? kPlaySkipStart : kSkipPlayStart) + (n - 1) * 6 + (m - 1);
}

// Counting part for the current encounter. 'phase' is advanced in place (wraps at n+m).
inline bool countedPasses(const Decoded& d, uint8_t& phase)
{
    const uint8_t cycle = d.n + d.m;
    if (phase >= cycle)
        phase = 0;
    const bool pass = d.playFirst ? (phase < d.n) : (phase >= d.n);
    phase = (phase + 1) % cycle;
    return pass;
}

// 'roll' = uniform random in 0..99 (the caller supplies it so tests are deterministic).
inline bool gatePasses(const Decoded& d, uint8_t roll)
{
    return d.pct >= 100 || roll < d.pct;
}
} // namespace condChance
