#pragma once
// Host-authoritative discrete events. Snapshots stay continuous (position, health, anim name).
// One wire envelope (net::GameEvent, id 48). A new map or mode adds a kind and a handler.
// Clients send requests. The host commits. Clients apply commits and do not decide outcomes.
// Engine side (GameDll7 v1.0 RVAs, re/notes/lead-mission.md): every mission end on every map goes
// through cMission's message call (0x91c20, "MISSION SUCCESSFUL!" / "MISSION FAILED!"), and a hunt
// trophy claim goes through cObjAnimal::ClaimCallback (0x1cd40).
#include "net/protocol.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace dh2::coop::events {

constexpr uint16_t kAnimalDied = 1;   // commit. subject = entity id. Client plays death once.
constexpr uint16_t kMissionEnd = 2;   // commit. i0 = message code (5 success, 6 failed), key = text.
constexpr uint16_t kTrophyClaim = 3;  // request (client pressed the claim key) and commit. subject = entity id.
constexpr uint16_t kMapStart = 4;     // commit. key = map name (e.g. HUNTMAP4, MENUMAP), subject = host map serial.
                                      // Clients follow with spmap/menumap; a same-name commit is a replay.

// onRequest runs on the host. onCommit runs on a client. Either may be null.
using Handler = void (*)(const net::GameEvent&);
void SetHandler(uint16_t kind, Handler onRequest, Handler onCommit);

// Host only. Records the commit and broadcasts it. A repeat of the same kind/subject/key resends
// the stored commit and does not append another. No-op on a client.
void Commit(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key, uint8_t actor);

// Client only. Reliable request. The host stamps actorId. No-op on the host.
void Request(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key);

void OnNet(const net::GameEvent& ev);
// Retries a mission end that arrived before this process had a cMission.
void Pump();
// Drops the journal. Call on map load and when this process leaves the session.
void Reset();
// Host: send the journal to current peers (a client who joined after the outcome).
void Replay();

// Test hook (console coop_claim <id>): run the claim path for an animal as if the engine called it.
void DebugClaim(uint16_t animalId);

// This process's current map name (upper case, no path/extension), "" before the first map.
const std::string& CurrentMap();
// Client: true when the host's last announced map is the map this process is in (or none announced yet).
// Entity-id based sync (animals) must not run across different maps.
bool SameMapAsHost();

}  // namespace dh2::coop::events
