#pragma once
// Vehicles (cObjVehicle: the MAP4A and MAP5 truck "car1"). Every machine used to drive its own copy along the
// waypoints, so the copies drifted apart; mirrored lions, teleported into a copy that was somewhere else, flipped it.
// Now the host's copy drives and every client's copy follows the host's chassis pose (frozen physics, no
// self-driving). Riders and the driver are attached, so they come along. [Coop] VehicleSync=0 turns it off.
// Seats: each player sits in its own copy (place_1). The host's NPC seats (the driver) are mirrored (kVehicleSeat);
// a client whose host rides is seated in its copy when its own script did not do it (late join, a boarding cutscene
// it never ran, F9, a catch-up teleport); a client whose own script boarded first makes the host board (kVehicleBoard).
// Re/notes/vehicles.md.
#include <cstdint>

#include "net/protocol.h"

namespace dh2::coop::vehicles {

// Client: a host snapshot arrived (already checked for the current map).
void OnStates(const net::VehicleStates& states, uint32_t localMs);

// Client, instead of putting the hunter next to the host (TeleportToHost, F9 revive): when the local hunter already
// sits in a vehicle, leave it there; when the host rides and we do not, seat ours in our copy of the host's vehicle.
// True when the local hunter sits in a vehicle afterwards (the caller then must not move it).
bool RideWithHost(const char* why);
// Client: the host's hunter sits in a vehicle (its last PlayerState had kFlagRiding).
bool HostRides();
// Host: riders (kFlagRiding puppets) stay on the animals' attack-target list ([Vehicles] RiderTargets).
bool RidersAreTargets();

}  // namespace dh2::coop::vehicles
