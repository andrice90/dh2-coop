#pragma once
// Vehicles (cObjVehicle: the MAP4A and MAP5 truck "car1"). Every machine used to drive its own copy along the
// waypoints, so the copies drifted apart; mirrored lions, teleported into a copy that was somewhere else, flipped it.
// Now the host's copy drives and every client's copy follows the host's chassis pose (frozen physics, no
// self-driving). Riders and the driver are attached to the truck, so they come along. [Coop] VehicleSync=0 turns it off.
#include <cstdint>

#include "net/protocol.h"

namespace dh2::coop::vehicles {

// Client: a host snapshot arrived (already checked for the current map).
void OnStates(const net::VehicleStates& states, uint32_t localMs);

}  // namespace dh2::coop::vehicles
