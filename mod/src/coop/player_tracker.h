#pragma once
// Finds the live entity manager and the local hunter; samples transforms on the game thread.
// Design + evidence: re/notes/lead-player-tracker.md.
#include <cstdint>
#include <vector>

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
    // World render camera of the last frame (cRender::SetCamera hook): live view direction even while standing
    // (the hunter's +0xF4 heading only follows the view while moving; cObjScriptedEntity::m_pActiveCamera is null
    // during free hunt).
    bool viewValid = false;
    float viewYaw = 0.f;    // degrees, atan2(z.x, z.z) — same convention as the hunter heading
    float viewPitch = 0.f;  // degrees, asin(z.y): + = looking up (verified: mouse up -> +16)
    Vec3 viewPos;           // camera (eye) position
    // Actions (per-frame poll in the tracker). Crouch = eye height above the feet < 1.40 m (standing 1.71,
    // crouched ~1.20; the cObjCharacterEx +0x204 crouch bit is not used by the hunter). Weapon state names come
    // from cObjItemEx::GetCurrentState on the hunter's active item: Idle/Walk/Run/Fire/Reload.
    bool crouched = false;
    uint8_t weapon = 0;      // net::WeaponKind, from the active item's model name
    bool firing = false;
    bool reloading = false;
    uint8_t shots = 0;       // increments on every transition into "Fire"
};

// Latest sample of the local hunter (game thread only). valid=false until found.
const LocalPlayerSample& LocalPlayer();
// Re-reads the local hunter's transform right now (game thread). False if no hunter is known.
bool SampleLocalNow(LocalPlayerSample& out);
// Number of animals seen in the last scan.
int AnimalCount();
// Animal entities (cScrAnimal, GameDll7 vtable +0xff470) from the last 1 s scan, in manager index order.
const std::vector<void*>& Animals();
// Called once per frame on the game thread from the first cRender::SetCamera of the frame: after the sim/physics
// step, before anything is drawn. Place remote-driven entities here so the drawn pose is ours, not physics'.
void SetPreRenderCallback(void (*fn)());
// Live cObjManager (captured from AddEntity), nullptr until the first spawn.
void* EntityManager();
// The local hunter's model_name key (e.g. "Objects/Characters/Teenm/Body/teenmm.skin,game.ap,HUNTMAP4.ap"); "" if unknown.
const char* HunterModelName();

}  // namespace dh2::coop
