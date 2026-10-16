#pragma once
// Host-authoritative discrete events. Snapshots stay continuous (position, health, anim name).
// One wire envelope (net::GameEvent, id 48). A new map or mode adds a kind and a handler.
// Clients send requests. The host commits. Clients apply commits and do not decide outcomes.
#include "net/protocol.h"

#include <cstdint>
#include <string_view>

namespace dh2::coop::events {

constexpr uint16_t kAnimalDied = 1;    // commit. subject = entity id. Client plays death once.
constexpr uint16_t kMapComplete = 2;   // request and commit. cHunterProfile::SetMapIsComplete.

// onRequest runs on the host. onCommit runs on a client. Either may be null.
using Handler = void (*)(const net::GameEvent&);
void SetHandler(uint16_t kind, Handler onRequest, Handler onCommit);

// Host only. Records the commit and broadcasts it. A repeat of the same kind/subject/key resends
// the stored commit and does not append another. No-op on a client.
void Commit(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key, uint8_t actor);

// Client only. Reliable request. The host stamps actorId. No-op on the host.
void Request(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key);

void OnNet(const net::GameEvent& ev);
// Retries a map-complete that arrived before the profile pointer was known.
void Pump();
// Drops the journal. Call on map load and when this process leaves the session.
void Reset();
// Host: send the journal to current peers (a client who joined after the outcome).
void Replay();

}  // namespace dh2::coop::events
