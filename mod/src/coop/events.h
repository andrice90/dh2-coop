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

constexpr uint16_t kAnimalDied = 1;   // commit. subject = entity id, key = "life <n>" (one per life), f0 = species.
                                      // i0 = life | report << 16 | fled << 24; report = 1 + the MISSION_ACTION_* this end
                                      // gave the host's mission (0 = none), fled = reached a flee point (parked, no
                                      // corpse). The client presents the death once and gives its mission the same report.
constexpr uint16_t kMissionEnd = 2;   // commit. i0 = message code (5 success, 6 failed, 7 level exit), key = text.
constexpr uint16_t kTrophyClaim = 3;  // request (client pressed the claim key) and commit. subject = entity id.
constexpr uint16_t kMapStart = 4;     // commit. key = map name (e.g. HUNTMAP4, MENUMAP), subject = host map serial.
                                      // Clients follow with spmap/menumap; a same-name commit is a replay.
constexpr uint16_t kTriggerEvent = 5; // request and commit. subject = cObjTriggerEvent entity id, key = its name.
                                      // Arms a trigger (script trigger_N.Activate(), GameDll 0x68eb0).
                                      // i0 (commit) = the host's arm serial of that trigger (1, 2, ... per map).
constexpr uint16_t kTriggerFire = 6;  // request and commit. Same subject/key. The armed trigger fired: a player entered
                                      // it and its script OnActivate() ran (GameDll 0x68130). The main story branch.
                                      // i0 (commit) = the host's fire serial of that trigger.
constexpr uint16_t kPropAnim = 7;     // commit. subject = world prop entity id, key = anim name, i0 = serial per prop.
                                      // Script SetAnim on a prop (MAP2A fallbridge "broken"). Not hunters, puppets, or mirrored actors.
constexpr uint16_t kPlaySA = 8;       // commit. subject = character entity id, key = "anim|partner", i0 = serial.
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
constexpr uint16_t kTriggerHit = 18;     // request and commit (story.cpp). subject/key = cObjTriggerEvent id/name, i0
                                         // (commit) = the host's OnHit serial. The trigger's script OnHit ran on the
                                         // host (GameDll 0x6b140): a client's touch is a request, clients apply the
                                         // commit and their own trigger then plays BeginCutScene and the cutscene.
                                         // Journaled before the fire it leads to. Kind 19 is reserved for story.cpp.
constexpr uint16_t kTranqHit = 22;       // request (beasts.cpp). subject = animal entity id: a client's tranquilizer dart
                                         // hit the hit-bone list of its mirror. The host runs cScrAnimal::OnHit's tranq
                                         // branch (sleep timer, kill credit, overdose fail); the sleep is the usual
                                         // kAnimalDied. Darts do no health damage, so AnimalHit never carried them.
constexpr uint16_t kAnimalRevive = 20;   // commit (animals.cpp). subject = animal id, i0 = its new life (2, 3, ...),
                                         // key = "life <n>". Script MoveToPos (GameDll 0x14610) brought back an animal
                                         // that had died or fled; the client resets that mirror and re-freezes it.
constexpr uint16_t kScaleConfig = 21;    // commit, not journaled (scale.cpp), sent to each client once per connection.
                                         // i0 = Bodies | Reserves << 8: the host's [Scale] values decide entity ids.
// Mission flow (flow.cpp, re/notes/flow.md).
constexpr uint16_t kLevelExit = 24;      // request. subject = cObjTriggerLevel entity id, key = its name ("exit_1"),
                                         // i0 = 1 fast path (ActivateTriggerLevel). A client reached a level exit; the
                                         // host runs it for its own hunter when it is armed there (one exit per map).
constexpr uint16_t kAmmoState = 25;      // request. i0 = 1 while this client's ammo check (GameDll 0x2f530) passes.
                                         // The host fails an ammo-gated task only when no living player has ammo.
constexpr uint16_t kLevelNext = 26;      // commit (Send, not journaled). The host left its results/pause menu: i0 = 1
                                         // continue (key = "map|spawn", the profile's next map and the exit's spawn
                                         // point), 2 end-of-game movie, 3 retry (key = map), 4 main menu.

// Vehicles (vehicles.cpp). Every process keeps its own copy of the truck; the host's copy drives (VehicleStates).
constexpr uint16_t kVehicleSeat = 28;    // commit (journaled). subject = vehicle id, i0 = place | occupant id << 16
                                         // (0xFFFF = empty), key = "p<place>=<id>#<serial>". An NPC (the driver) got
                                         // in or out on the host; clients seat their copy of it the same way.
constexpr uint16_t kVehicleBoard = 29;   // request. subject = vehicle id, i0 = driver NPC id (0xFFFF none), key = the
                                         // waypoint the client's script drove to. A client's own script seated it while
                                         // the host's hunter is not seated: the host boards too and drives.

// Bosses and scripted beats (bosses.cpp, re/notes/yeti-bosses.md).
constexpr uint16_t kNpcAnim = 34;        // commit. subject = NPC entity id, key = "<clip>#<n>" (n counts per NPC, so the
                                         // journal does not swallow a repeated clip). An AITaskNPC_PlayAnim clip without
                                         // a partner (MAP12A Reggie "hurtdown", MAP2B Hugh "syncguidegetup").
constexpr uint16_t kPropSA = 35;         // commit. subject = cObjAIHuntMoveObject id (MAP12B falling tree), key = clip.
constexpr uint16_t kPropSAEnd = 36;      // commit. Same subject and key; f0 = seconds since the start on the host.
constexpr uint16_t kBossThrow = 37;      // commit (Send). subject = thrower id, key = "x y z vx vy vz" of the rock as it
                                         // left the hand: clients throw a harmless copy (MAP12C yeti).
constexpr uint16_t kScaleMax = 40;       // commit (scale.cpp). subject = animal id, f0 = its new max health (+0x4C0),
                                         // key = "<factor> #<serial>". Player-count scaling of a boss / lone large animal.

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

// Host: the HUD showed a mission message (5 success, 6 failed, 7 checkpoint). One that did not pass cMission's
// 0x91c20 (a guide's death, a level exit, the task status report 0x96750) is committed as kMissionEnd here.
void CommitHudMissionMessage(int code, const char* text);

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
