# DH2 co-op protocol v1

Progress: spec and code done, including `AnimalStates` (id 9), `AnimalHit` (id 10), and `GameEvent` (id 48). `dh2net_test.exe` passed twice on 2026-10-16: 3406 checks, 0 failures (was 3294 before `GameEvent`). Details in `re/notes/p2c-net.md`, `re/notes/p4a-net-animals.md`, and `re/notes/p4c-events.md`.

This is the wire contract for `net::Session` (ENet, host-authoritative, star topology). The engine's own sockets are not used. The game never calls `WSAStartup`; `enet_initialize` does that, exactly once per process while any session is alive.

## Port and channels

| Item | Value |
|---|---|
| Default UDP port | **27420** (`net::kDefaultPort`) |
| Engine `sv_port` | 27318, dead. Do not use it. |
| Address family | IPv4 only (ENet 1.3.18) |
| Channel 0 | Reliable control: Hello, Welcome, Reject, PlayerJoined, PlayerLeft, Ping, Pong |
| Channel 1 | Unreliable sequenced snapshots: PlayerState, AnimalStates. No `UNSEQUENCED` flag, so ENet keeps order and may drop a stale snapshot. |
| Channel 2 | Reliable events: AnimalHit, GameEvent. |

`Join` takes an IPv4 literal or a name that resolves to IPv4. Use `127.0.0.1` for same-PC tests.

## Topology

One host, up to three clients (**4 players total**). Clients talk only to the host. The host relays each client's `PlayerState` to the other clients after overwriting `playerId` with the id it assigned that peer. A client cannot spoof another player's id, including the host's id 0.

The host's own `PlayerState` is sent directly to every client. Senders do not receive their own state back.

Lobby cap is enforced in the handshake, not by hoping ENet runs out of slots. The host allocates `maxClients + 4` ENet peers so one more connection can still finish the handshake and receive `Reject`. Connections past that allocation are refused by ENet and get no `Reject` packet.

## Version policy

`Hello.protoVersion` must be `1` and `Hello.gameBuild` must be `434`. Anything else is `Reject` reason `VersionMismatch`, then the link is closed. Version is checked before the lobby-full check. A new build bumps `gameBuild` and does not reuse v1 ids.

Fixed messages have **exact** sizes. `AnimalStates` varies with the animal count and each name length, and the buffer must be consumed exactly. Truncation, a count above 32, a name length above 31, and trailing bytes are malformed. Unknown ids are ignored by the session (logged) so a newer sender fails closed on an old host only at Hello, not by crashing.

## Byte order

Every multi-byte integer is little-endian. Floats are IEEE-754 binary32, written as a little-endian uint32 of the raw bits. Player names are 32 bytes, zero-padded. A player name longer than 31 bytes is truncated and byte 31 is set to 0, so the field is always a C string. An animal body-state name is a `u8` length (0..31) followed by that many bytes and no trailing NUL. Encode keeps the first 31 bytes of a longer name.

`net::ByteReader` reads only when the remaining length covers the field. A short buffer returns failure and does not touch memory past the span.

## Message ids

| Id | Name | Dir | Channel | Bytes | Payload after the id byte |
|---|---|---|---|---|---|
| 1 | Hello | C→H | 0 reliable | 36 | `u8 protoVersion`, `u16 gameBuild`, `name[32]` |
| 2 | Welcome | H→C | 0 reliable | 7 | `u8 playerId`, `u32 hostTick`, `u8 maxPlayers` |
| 3 | Reject | H→C | 0 reliable | 2 | `u8 reason` |
| 4 | PlayerJoined | H→C | 0 reliable | 34 | `u8 playerId`, `name[32]` |
| 5 | PlayerLeft | H→C | 0 reliable | 2 | `u8 playerId` |
| 6 | PlayerState | both, relayed | 1 unreliable sequenced | 44 | see below |
| 7 | Ping | both | 0 reliable | 5 | `u32 token` |
| 8 | Pong | both | 0 reliable | 5 | `u32 token` (echo) |
| 9 | AnimalStates | H→C | 1 unreliable sequenced | 8 + entries | `u16 seq`, `u32 tickMs`, `u8 count`, then `count` entries |
| 10 | AnimalHit | C→H | 2 reliable | 20 | `u8 shooterId`, `u16 animalId`, `f32 damage`, `f32 hitPos[3]` |
| 48 | GameEvent | request C→H, commit H→C | 2 reliable | 18 + key | see below |

`PlayerState` layout (44 bytes including the id):

| Offset | Type | Field |
|---|---|---|
| 0 | u8 | msg id `6` |
| 1 | u8 | playerId |
| 2 | u16 | seq |
| 4 | u32 | tickMs (sender clock, milliseconds) |
| 8 | f32[3] | pos |
| 20 | f32 | yaw, degrees |
| 24 | f32 | pitch, degrees |
| 28 | f32[3] | velocity, world units per second |
| 40 | u16 | flags. Bit 0 crouched, 1 reloading, 2 firing, 4 dead (hunter `+0x6CC`), 5 knocked down (hunter `+0x368`, inside a sync attack), bits 8-11 shot counter. |
| 42 | u8 | weaponId |
| 43 | u8 | health |

Flag bits in the `u16`: bit 0 crouch, bit 1 prone, bit 2 aiming, bit 3 firing, bit 4 dead. Bits 5–15 are reserved and must be sent as 0 until assigned.

`AnimalStates` layout. The caller sends it at 20 Hz (`ConsumeSendTick`). `Session` does not throttle the call. Count is 0..32. Encode keeps the first 32 entries. An empty list is an 8-byte packet.

| Offset | Type | Field |
|---|---|---|
| 0 | u8 | msg id `9` |
| 1 | u16 | seq (host counter) |
| 3 | u32 | tickMs (host clock, milliseconds) |
| 7 | u8 | count |
| 8 | entries | `count` times the entry below |

`AnimalState` entry. Offsets are from the start of that entry. The name is raw bytes, not a C string.

| Offset | Type | Field |
|---|---|---|
| 0 | u16 | id (entity id, for example 120–124) |
| 2 | f32[3] | pos |
| 14 | f32 | yaw, degrees. 0 faces +z, 90 faces +x |
| 18 | f32 | health |
| 22 | u8 | flags |
| 23 | u8 | state length, 0..31 |
| 24 | bytes | state, the body-state name |

An entry with an empty name is 24 bytes. Flag bits: bit 0 dead, bit 1 parked/inactive, bit 2 frozen-on-host. Bits 3–7 are reserved. The wire keeps whatever the sender put in those bits.

`AnimalHit` layout (20 bytes including the id). `hitPos` may be all zeros. The host sets `shooterId` from the sending peer's player id before queueing the event.

| Offset | Type | Field |
|---|---|---|
| 0 | u8 | msg id `10` |
| 1 | u8 | shooterId |
| 2 | u16 | animalId |
| 4 | f32 | damage |
| 8 | f32[3] | hitPos |

`GameEvent` is the envelope for discrete world outcomes (an animal died, a map completed, a later objective or pickup). Continuous fields stay on `AnimalStates` / `PlayerState`. The session does not interpret `kind`. An unknown kind still decodes; the game layer ignores a kind it has no handler for. That is how another map adds an outcome without a new message id.

| Offset | Type | Field |
|---|---|---|
| 0 | u8 | msg id `48` |
| 1 | u8 | flags. Bit 0 = commit (host → clients). Clear = request (client → host). |
| 2 | u8 | actorId. On a request the host overwrites this with the sending peer's player id. |
| 3 | u16 | seq. The host assigns it on a commit. |
| 5 | u16 | kind. Game-layer registry, full list in `mod/src/coop/events.h`. `1` AnimalDied, `2` MissionEnd (i0 = message code 5/6, key = text), `3` TrophyClaim, `4` MapStart, `5` TriggerEvent, `6` TriggerFire, `7` PropAnim, `8` PlaySA, `9` SyncRequest, `10` SyncDone, `11` CutsceneSkip, `12` PlayerHurt, `13` PlayerKnock, `14` PlayerKnockEnd, `15` KnockRequest. Other values are legal on the wire. |
| 7 | u16 | subjectId. Entity id, or 0 when the subject is the map or an objective. |
| 9 | u32 | i0. Kind-specific integer. |
| 13 | f32 | f0. Kind-specific float. |
| 17 | u8 | key length, 0..31 |
| 18 | bytes | key. A short name (objective, item class). Not a C string. |

An empty key is 18 bytes. Encode keeps the first 31 bytes of a longer key. A key length that does not consume the rest of the packet is malformed. The host does not relay a request. A client cannot send a commit (`SendGameEvent` returns false). A commit from a client is dropped if it is somehow received. The host does not apply its own commit; it already ran the outcome.

**Who gets a commit (2026-10-17, `re/notes/lead-dropin.md`).** The host keeps the current map's commits in a journal.
- A client sends `SyncRequest` (key = its map) once its map is loaded and its hunter exists. The host answers that client only (`Session::SendGameEventTo`):
  - its `MapStart`;
  - if the maps match, the journal in order (a replayed `TriggerFire` carries its age in seconds in f0), then `SyncDone`.
- Live commits go only to clients that got `SyncDone` for the host's current map. `MapStart` goes to every client.
- `PlayerHurt` / `PlayerKnock` / `PlayerKnockEnd` are sent the same way but are not journaled, so a late joiner never replays a hit.

`Reject.reason`: 0 none, 1 `VersionMismatch`, 2 `LobbyFull`, 3 `BadHello`.

`Welcome.playerId` is 0 for the host and 1..maxClients for clients. The lowest free id is reused after a leave. `hostTick` is milliseconds since the host called `StartHost` (steady clock). `maxPlayers` counts the host.

`Ping`/`Pong` are application messages. `Session` answers `Ping` with `Pong` on the same peer. The game should not send its own `Pong`. RTT for the overlay is ENet's per-peer `roundTripTime`, not this token. Until ENet has a sample, that field stays at ENet's default of 500 ms.

## Reserved id ranges

Ids 9 and 10 are assigned above. Do not send the ids in this table. They stay free so a later message can be added without shifting v1 ids.

| Ids | Status |
|---|---|
| 11–15 | unused |
| 16–31 | unused, reserved for later lobby / clock messages (channel 0) |
| 32–47 | unused, reserved |
| 48 | assigned: `GameEvent` (channel 2). Kinds inside it cover spawn, death, and mission outcomes. |
| 49–63 | unused, reserved for another event envelope if one packet is no longer enough |
| 64–79 | unused, reserved for later hit messages (channel 2) |
| 80–95 | unused, reserved for later damage / kill messages (channel 2) |
| 96–255 | unused |

## Link close codes

These are the `enet_uint32` passed to `enet_peer_disconnect`, not a v1 message. ENet 1.3.18 delivers that value to the **remote** peer. A timeout is delivered as data `0` (`enet_protocol_notify_disconnect` forces 0). The side that initiated the close also sees local data 0, so `Session` remembers that it initiated the close and reports `Disconnected` rather than `TimedOut`.

| Code | Meaning |
|---|---|
| 0 | Timeout, or the initiator's local completion event |
| 1 | Graceful client leave |
| 2 | Rejected (a `Reject` was sent first and acknowledged) |
| 3 | Host shutdown |
| 4 | Hello did not arrive within 2 s |

## Handshake

1. Host `StartHost`. Local id is 0. ENet peer slots = `maxClients + 4` (default maxClients 3).
2. Client `Join`. On the ENet connect event it sends `Hello` on channel 0.
3. Host rejects a bad version, a truncated hello, or a full lobby (`active clients >= maxClients`). `Reject` is queued, then `enet_peer_disconnect_later` so the reject is acknowledged before the close.
4. Otherwise the host assigns an id, sends `Welcome`, then `PlayerJoined` for the host and for every player already in, then tells the other clients `PlayerJoined` for the newcomer.
5. `PlayerJoined` is never sent for the recipient themselves. `Welcome` is how a client learns its own id.

`Connected` means this session is in the lobby (host at `StartHost`, client when `Welcome` arrives, and the host again for each accepted remote). `PlayerLeft` is the roster message. `Disconnected` / `TimedOut` describe a link. Other clients hear a leave only as `PlayerLeft`. They hear a dead host as `Disconnected` or `TimedOut`.

## Interpolation

`net::SnapshotBuffer` is one remote player's history. Keep one per remote id.

| Parameter | Default |
|---|---|
| History | 1000 ms |
| Render delay | 100 ms |
| Extrapolation cap | 250 ms |
| Snapshot rate | 20 Hz (`kStateIntervalMs` = 50) |

Push with the **local** receive time, not `PlayerState.tickMs`. The game runs a variable frame (`sv_fps` default 100) and the two PCs do not share a clock. Sample at `renderTimeMs - delayMs`:

- Between two snapshots: linear position, linear pitch, linear velocity, shortest-arc yaw in degrees.
- Discrete fields (flags, weapon, health) stay on the older snapshot until the sample time reaches the newer one.
- Past the newest snapshot: position += velocity * dt, dt capped at 250 ms. Yaw and pitch hold. `extrapolated` is set.
- Older than the buffer: clamp to the oldest snapshot.

`Session::ConsumeSendTick(nowMs)` is true at most once per 50 ms. Drive it from a steady clock, not from the scaled sim clock.

## Stats

`Session::Peers()` is one row per remote lobby member:

- `rttMs` — ENet `roundTripTime` (reliable RTT). Default 500 until a sample exists.
- `packetLoss` — ENet reliable loss as a fraction (`packetLoss / 65536`). It does not count unreliable snapshots ENet chose to drop.
- `bytesIn` / `bytesOut` — v1 payload bytes this session has seen for that peer, not UDP header bytes.

## How the game should call Session

The coop module lives outside `mod/src/net/` and is not part of this job. One session, no thread of its own, called from the game thread (the Present hook until a sim-tick hook exists).

```cpp
// Host button / coop_host
net::Session session;
net::Session::Identity me;
me.name = playerName;                 // max 31 chars
me.protoVersion = net::kProtocolVersion;
me.gameBuild = net::kGameBuild;       // 434
session.StartHost(net::kDefaultPort, /*maxClients=*/3, me);

// Join button / coop_join <ip>
session.Join(ip, net::kDefaultPort, me);

// Once per frame, on the game thread:
session.Service();
for (const net::Event& ev : session.DrainEvents()) {
    if (ev.type == net::EventType::PlayerState)
        history[ev.state.playerId].Push(WallMs(), ev.state);
    if (ev.type == net::EventType::PlayerLeft)
        history[ev.playerId].Clear();
    // PlayerJoined → roster; Reject → show reason; Disconnected/TimedOut → drop link
}

if (session.IsConnected() && session.ConsumeSendTick(WallMs())) {
    net::PlayerState st = ReadLocalHunter(); // pos from entity +0xE8; quantize health to u8
    st.tickMs = WallMs();
    st.seq = nextSeq++;
    session.SendPlayerState(st);
}

// Host, on the same 20 Hz tick:
net::AnimalStates animals;
animals.seq = nextAnimalSeq++;
animals.tickMs = WallMs();
session.SendAnimalStates(animals);  // false when this process is not the host

// Client, when a local shot should count on the host:
net::AnimalHit hit;
hit.animalId = animalId;
hit.damage = dropped;
session.SendAnimalHit(hit);  // false on the host; EventType::AnimalHit arrives with shooterId set

// Client request / host commit. See re/notes/p4c-events.md. The host stamps actorId on a request.
net::GameEvent ev;
ev.kind = 2;                  // MissionEnd, as one example. The session does not care.
session.SendGameEvent(ev);   // false on the host when flags bit0 is clear
ev.flags = net::game_event_flag::kCommit;
session.SendGameEvent(ev);   // false on a client; host fans the commit out, and does not queue it locally

// Render, after the sim step, with the same WallMs clock:
net::InterpSample sample;
if (history[id].Sample(WallMs(), sample))
    DrawRemote(id, sample.pos, sample.yaw, sample.pitch);
```

`ReadLocalHunter` is future work. Position is the entity `tVect3` at `cObjEntity + 0xE8` (already in MEMORY). Yaw, pitch, crouch bits, weapon id, and the health byte are **not** pinned to fields yet; the coop module owns that mapping. Health on the hunter is a profile attribute, so the u8 on the wire is a quantized value.

Shutdown: `session.Disconnect()` from the game thread. It flushes an ENet disconnect and releases the socket. After the last session is destroyed, `enet_deinitialize` runs.

Do not call `enet_initialize` anywhere else. Do not call `Service` from inside an event handler; queue the work for the next frame. `Session` is single-threaded.
