#pragma once
// Helicopters on the pilot maps (MAP11A / MAP11C): the player is a cObjPilot attached to the entity "helicopter"
// (GameDll cObjHelicopter). Its flight is a baked clip on the fly model (cBodyHelicopter Ani1 -> Loop1 -> Ani2 ...),
// advanced from Loop to the next flight segment by the script's CompleteTask. Every machine used to fly its own copy
// from replayed trigger fires, so a client started late and waited in loops of its own. Now the host's helicopter is
// the one that flies: the host streams its body state, clip and clip time (HeliStates, ~20 Hz) and every client plays
// its copy at the same state, clip and time. The clip moves the "masscenter" bone, the entity follows the bone and
// the pilot is attached to the entity, so the clip time is the whole pose. [Coop] HeliSync=0 only measures the drift.
#include <cstdint>

#include "net/protocol.h"

namespace dh2::coop::heli {

// Client: a host snapshot arrived (already checked for the current map).
void OnStates(const net::HeliStates& states, uint32_t localMs);

}  // namespace dh2::coop::heli
