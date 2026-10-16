#pragma once
// Finds the live entity manager and the local hunter; samples transforms on the game thread.
// Design + evidence: re/notes/lead-player-tracker.md.
#include <cstdint>

namespace dh2::coop {

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
};

struct LocalPlayerSample {
    bool valid = false;
    void* entity = nullptr;
    int16_t id = -1;
    Vec3 pos;     // cObjEntity +0xE8 (world)
    Vec3 angle;   // cObjEntity +0xF4 (world Euler; component order/units being established)
};

// Latest sample of the local hunter (game thread only). valid=false until found.
const LocalPlayerSample& LocalPlayer();
// Re-reads the local hunter's transform right now (game thread). False if no hunter is known.
bool SampleLocalNow(LocalPlayerSample& out);
// Number of animals seen in the last scan.
int AnimalCount();

}  // namespace dh2::coop
