#pragma once
// Phase 3 ghost mode: one third-person human entity per remote player, driven by interpolated PlayerState.
// Design: re/notes/lead-puppets.md. Game thread only.
#include <cstdint>

#include "net/protocol.h"

namespace dh2::coop::puppets {

// Feed every received remote PlayerState (localTimeMs = receiver's steady clock at arrival).
void OnRemoteState(uint8_t playerId, const net::PlayerState& state, uint32_t localTimeMs);
// The body (coop/puppet_models.h index) this player picked. Before the first state it only chooses the spawn;
// later it replaces the puppet.
void SetModel(uint8_t playerId, int model);
// Remote player left: remove the puppet.
void OnRemoteLeft(uint8_t playerId);
// Session ended: remove all puppets.
void RemoveAll();
int Count();
// True for a puppet or weapon entity this module spawned. Those use /npc/ models and must not be mirrored as world NPCs.
bool Owns(void* ent);
// The remote player this puppet body stands for, or net::kInvalidPlayerId (weapons and other entities too).
uint8_t PlayerOf(void* ent);
// That player's puppet entity, or null.
void* EntityOf(uint8_t playerId);
// True for a puppet body whose player is alive (not kFlagDead).
bool IsLivingPuppet(void* ent);
// The living puppet nearest to `pos` (world metres) and its squared distance, or null when none is closer than
// `best_sq` on entry. Dead players (kFlagDead) are skipped.
void* NearestLivingPuppet(const float pos[3], float& best_sq);

}  // namespace dh2::coop::puppets
