// Realtime Recording Quantize / Step Entry Division - pure arithmetic (header-only, host-testable).
// 48 microsteps per Tracker step, 16 steps per measure -> 768 microsteps per measure.
#pragma once
#include <stdint.h>

namespace rtq
{
constexpr int kDivisionCount = 6;
constexpr int kDefaultDivision = 5; // Sixteenth note
constexpr int kDefaultStrength = 100;
constexpr int32_t kMicrostepsPerStep = 48;

// 0 Half, 1 Quarter, 2 Quarter triplet, 3 Eighth, 4 Eighth triplet, 5 Sixteenth
constexpr int32_t kIntervalMicro[kDivisionCount] = { 384, 192, 128, 96, 64, 48 };
constexpr const char* kDivisionLabel[kDivisionCount] = { "1/2", "1/4", "1/4T", "1/8", "1/8T", "1/16" };

inline int32_t interval(int division)
{
    return kIntervalMicro[(division < 0 || division >= kDivisionCount) ? kDefaultDivision : division];
}

// P_q = D * floor(P/D + 0.5); exact halfway positions move forward. P >= 0.
inline int32_t nearestGrid(int32_t P, int32_t D)
{
    return D * ((2 * P + D) / (2 * D));
}

// P_result = P + round(S/100 * (P_q - P)), halves rounded away from zero.
inline int32_t quantize(int32_t P, int division, int strength)
{
    if (strength < 0)
        strength = 0;
    if (strength > 100)
        strength = 100;
    const int32_t delta = nearestGrid(P, interval(division)) - P;
    const int32_t num = strength * delta;
    const int32_t r = num >= 0 ? (num + 50) / 100 : -((-num + 50) / 100);
    return P + r;
}

// micro offset inside a step (0..47) -> step micromove value (0..100), the inverse of the existing
// mapClamped(uStep, 1, 48, 0, 100) used by live recording
inline int toMicromove(int32_t microInStep)
{
    return (int)((microInStep * 100 + 23) / 47);
}

// Step Entry Division: advance a cursor given in microsteps.
// Returns the new absolute microstep position (grid point strictly after 'micro') and guarantees the
// step index advances by at least one.
inline int32_t nextEntryPosition(int32_t micro, int division, int32_t currentStep)
{
    const int32_t D = interval(division);
    int32_t next = (micro / D + 1) * D;
    while (next / kMicrostepsPerStep <= currentStep)
        next += D;
    return next;
}
} // namespace rtq
