#pragma once
// Phase 4 shared animals. Game thread only. Design: re/notes/lead-animals.md.
#include "net/protocol.h"

#include <cstdint>

namespace dh2::coop::animals {

// Host, 20 Hz: build AnimalStates from Animals() and broadcast. No-op until 50 ms have passed.
void HostTick(uint32_t nowMs);
// Client: append one snapshot. localMs is this process's steady clock at arrival.
void OnAnimalStates(const net::AnimalStates& states, uint32_t localMs);
// Host: apply a client's hit. cScrAnimal subtracts health (FuncInflictDamage) after doing what the animal's bullet
// OnHit (GameDll 0x1a680) does for a hit: the kill counts for the mission (+0x420), a protected animal fails it,
// a tranquilizer dart starts the host's tranq timer. hit.hitPos[0] carries the client's hit kind (kHit* below).
// A cobra takes the bullet OnHit path instead: body state Death and +0x738 = 50, and its own Update counts the kill once.
void OnAnimalHit(const net::AnimalHit& hit);
// AnimalHit.hitPos is not a position on the wire: hitPos[0] is the kind of hit the client's OnHit saw.
constexpr float kHitUnknown = 0.f;  // local damage without a bullet seen (coop_kill)
constexpr float kHitBullet = 1.f;   // a player's bullet that hit a bone
constexpr float kHitTranq = 2.f;    // a player's tranquilizer dart (no damage on most species)
constexpr float kHitMelee = 3.f;    // a player's knife / kick (bullet kind 2: no protected-animal fail)
// Client: a host AnimalDied commit (life, mission report, fled). Presents death once, then stops driving that animal.
void OnAnimalDied(const net::GameEvent& ev);
// Client: a paired cutscene anim (PlaySA) just started. Leave this mirrored actor's pose alone
// until the host body-state name changes. `name` is the partner's target name, held too when we mirror it.
void HoldCutscene(uint16_t id);
void HoldCutsceneNamed(const char* name);
// Drop buffers and unfreeze any animal this process was driving.
void Reset();
// `active` is a host with at least one peer, or a connected client. The solo spike stays off while active.
void SetSessionActive(bool active, bool hosting);
// The animal entity with this id (int16 at +0x04) in the tracker list, or null.
void* Find(uint16_t id);
// Entity id of an animal pointer, or -1.
int IdOf(void* animal);
// Test hook (console coop_kill <id>). cScrAnimal: FuncInflictDamage. A host cobra starts the death
// timer; a client cobra only drops local health so the mirror can report AnimalHit.
bool DebugDamage(uint16_t id, float damage);

}  // namespace dh2::coop::animals
