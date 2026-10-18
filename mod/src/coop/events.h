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
constexpr uint16_t kTriggerEvent = 5; // request and commit. subject = cObjTriggerEvent entity id, key = its name.
                                      // Arms a trigger (script trigger_N.Activate(), GameDll 0x68eb0).
constexpr uint16_t kTriggerFire = 6;  // request and commit. Same subject/key. The armed trigger fired: a player entered
                                      // it and its script OnActivate() ran (GameDll 0x68130). The main story branch.
constexpr uint16_t kPropAnim = 7;     // commit. subject = world prop entity id, key = anim name.
                                      // Script SetAnim on a prop (MAP2A fallbridge "broken"). Not hunters, puppets, or mirrored actors.
constexpr uint16_t kPlaySA = 8;       // commit. subject = character entity id, key = "anim|partner".
                                      // cObjCharacterEx::PlaySA. MAP2A's bridge mesh is the partner of Hugh's "cs02".
constexpr uint16_t kSyncRequest = 9;  // request. key = the client's map, subject = its map serial. Sent once the client's
                                      // map is loaded and its hunter exists. The host answers that client only: its
                                      // kMapStart, then (same map) every journal commit and kSyncDone.
constexpr uint16_t kSyncDone = 10;    // commit, to one client. i0 = commits replayed, key = map. Live commits follow.
constexpr uint16_t kCutsceneSkip = 11;  // commit. subject/key = the cObjTriggerEvent whose cutscene the host skipped.
// Phase 5 (combat.cpp). Sent with Send, not journaled: a late joiner must not replay a hit.
constexpr uint16_t kPlayerHurt = 12;     // commit. subject = player id, f0 = damage, i0 = attacker entity id | type << 16.
constexpr uint16_t kPlayerKnock = 13;    // commit. subject = player id, key = sync-attack clip, f0 = facing yaw (deg).
constexpr uint16_t kPlayerKnockEnd = 14; // commit. subject = player id: the sync attack finished, stand up.
                                         // kPlayerKnock with an empty key is a player's hit: i0 = attacker player id,
                                         // the victim picks its map's clip and stands up on its own timer.
constexpr uint16_t kKnockRequest = 15;   // request. subject = victim player id, i0 = 1 shot / 2 melee.
constexpr uint16_t kPropHit = 16;        // request and commit. subject = cObjTarget entity id, i0 = 1 stump / 0 bottle.
                                         // Journaled, so a late joiner breaks the same props during catch-up.
                                         // Not sent with Send: a broken bottle or stump stays broken.
constexpr uint16_t kHostNotice = 17;     // commit (Send, not journaled). key = a short on-screen line from the host
                                         // ("Host restarts MAP3B"); the map change itself is the usual kMapStart.

// onRequest runs on the host. onCommit runs on a client. Either may be null.
using Handler = void (*)(const net::GameEvent&);
void SetHandler(uint16_t kind, Handler onRequest, Handler onCommit);

// Host only. Records the commit and sends it to the clients that have synced this map (kSyncDone sent);
// kMapStart goes to every client. A client that is still loading gets it in its catch-up instead.
// A repeat of the same kind/subject/key is not appended; the stored commit goes back to `actor` only (a client whose
// request found it already decided), and nowhere when the host itself repeats it. No-op on a client.
void Commit(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key, uint8_t actor);

// Host only. Like Commit but not journaled: momentary effects (a hit, a knockdown) that a late joiner must not replay.
void Send(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key, uint8_t actor);

// Client only. Reliable request. The host stamps actorId. No-op on the host.
void Request(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key);

void OnNet(const net::GameEvent& ev);
// Retries a mission end that arrived before this process had a cMission.
void Pump();
// Drops the journal. Call on map load and when this process leaves the session.
void Reset();
// Host: a client left. It gets the journal again when it asks (kSyncRequest).
void OnPeerLeft(uint8_t playerId);
// Client: between its kSyncRequest and the end of the catch-up (replayed commits applied, cutscenes
// skipped, hunter moved to the host). Story code skips cutscenes while this is true.
bool CatchingUp();

// Test hook (console coop_claim <id>): run the claim path for an animal as if the engine called it.
void DebugClaim(uint16_t animalId);

// This process's current map name (upper case, no path/extension), "" before the first map.
const std::string& CurrentMap();
// Client: true when the host's last announced map is the map this process is in (or none announced yet).
// Entity-id based sync (animals) must not run across different maps.
bool SameMapAsHost();
// This process's map serial (the kMapStart subject the host announces); stamped into AnimalStates.
uint16_t LocalMapSerial();
// Client: a snapshot stamped with this host map serial belongs to the map this process is on now. False before
// the host announced any map. Always true on the host.
bool SnapshotIsCurrent(uint16_t hostMapSerial);

}  // namespace dh2::coop::events
