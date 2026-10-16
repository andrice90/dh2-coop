#pragma once
// Phase 3 ghost mode: one third-person human entity per remote player, driven by interpolated PlayerState.
// Design: re/notes/lead-puppets.md. Game thread only.
#include <cstdint>

#include "net/protocol.h"

namespace dh2::coop::puppets {

// Feed every received remote PlayerState (localTimeMs = receiver's steady clock at arrival).
void OnRemoteState(uint8_t playerId, const net::PlayerState& state, uint32_t localTimeMs);
// Remote player left: remove the puppet.
void OnRemoteLeft(uint8_t playerId);
// Session ended: remove all puppets.
void RemoveAll();
int Count();

}  // namespace dh2::coop::puppets
