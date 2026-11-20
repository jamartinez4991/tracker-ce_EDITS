// Host model of the master boundary decision in Sequencer::switchRowStep() (final track).
#include <cassert>
#include <cstdio>
struct S { int steps, mult, activeMult, masterStep = 0, masterLength; bool breakPattern = false; };
static int raw2mult(int raw) { return raw < 8 ? raw + 1 : 1; }
// returns true when the transition block would run for this global step
static bool step(S& s, int physicalPos /*0-based pos of last track after its wrap logic*/, bool wrapped)
{
    bool endOfPattern = wrapped || s.breakPattern;
    if (s.activeMult > 1) {
        s.masterStep++;
        if (!s.breakPattern) endOfPattern = (s.masterStep >= s.masterLength);
    }
    if (endOfPattern) { s.activeMult = s.mult; s.masterLength = s.steps * s.activeMult; s.masterStep = 0; }
    (void)physicalPos;
    return endOfPattern;
}
static int firstBoundary(int steps, int mult, int start = 0)
{
    S s; s.steps = steps; s.mult = mult; s.activeMult = mult; s.masterLength = steps * mult; s.masterStep = start;
    int pos = start;
    for (int n = 1; n < 100000; n++) {
        bool wrapped = (++pos >= steps); if (wrapped) pos = 0;
        if (step(s, pos, wrapped)) return n;
    }
    return -1;
}
int main()
{
    for (int m = 1; m <= 8; m++) assert(firstBoundary(16, m) == 16 * m);
    assert(firstBoundary(16, 2, 8) == 24); assert(firstBoundary(192, 8) == 1536); assert(firstBoundary(16, 8, 100) == 28);            // start at step 9 of a x2 pattern: 8 + 16 left
    assert(raw2mult(0) == 1 && raw2mult(7) == 8 && raw2mult(8) == 1 && raw2mult(255) == 1);
    // live change: x2 running, pattern now says x4 -> current cycle still ends at 32, next lasts 64
    S s; s.steps = 16; s.mult = 2; s.activeMult = 2; s.masterLength = 32; int pos = 0, count = 0, ends[3], e = 0;
    for (int n = 1; n <= 100 && e < 2; n++) {
        if (n == 10) s.mult = 4;
        bool w = (++pos >= 16); if (w) pos = 0; count++;
        if (step(s, pos, w)) { ends[e++] = count; count = 0; }
    }
    assert(ends[0] == 32 && ends[1] == 64);
    // break pattern ends the cycle immediately
    S b; b.steps = 16; b.mult = 4; b.activeMult = 4; b.masterLength = 64; b.masterStep = 3; b.breakPattern = true;
    assert(step(b, 3, false) == true);
    puts("boundary OK");
}
