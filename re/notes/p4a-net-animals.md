# P4a — animal messages in the net layer

## Progress

Done. Tests passed (3294 checks, 0 failed, two runs). `dh2coop.asi` compiled. Install did not run: `Main7.exe` was open.

## What changed

Protocol version stays 1. New ids are the next free values after Pong (8):

| Id | Message | Direction | Channel |
|---|---|---|---|
| 9 | `AnimalStates` | host → clients | 1, unreliable sequenced |
| 10 | `AnimalHit` | client → host | 2, reliable |

`AnimalStates` carries `u16 seq`, `u32 tickMs`, and a `u8` count (0..32) of `AnimalState` entries. Each entry is `u16 id`, `f32 pos[3]`, `f32 yaw`, `f32 health`, `u8 flags`, then a `u8` length and that many state-name bytes (max 31). Encode truncates a longer name and drops entries past 32. Decode rejects a short buffer, a count above 32, a count larger than the bytes still in the buffer, a name length above 31, and any trailing byte.

`AnimalHit` is a fixed 20-byte payload: `u8 shooterId`, `u16 animalId`, `f32 damage`, `f32 hitPos[3]`.

Session:

- `SendAnimalStates` broadcasts on the host only. A client call returns false.
- `SendAnimalHit` sends reliable to the host. A host call returns false.
- The host copies `AnimalHit.shooterId` from the sending peer's player id (`Remote::playerId`, the same field `PlayerState` uses) before queueing `EventType::AnimalHit`.
- `Event` carries `animalStates` and `animalHit`.

Files: `mod/src/net/protocol.h`, `mod/src/net/protocol.cpp`, `mod/src/net/session.h`, `mod/src/net/session.cpp`, `mod/net-test/main.cpp`, `docs/net-protocol.md`. `mod/src/coop/*` is unchanged. Its event switch already has a `default`, so the new event types compile there and are ignored until P4b.

## Wire notes

Header of `AnimalStates` is 8 bytes. One entry with an empty name is 24 bytes, plus the name bytes. A full snapshot of 32 names of 31 bytes is 8 + 32 * 55 = 1768 bytes.

ENet's default MTU in the vendored header is 1392 (`ENET_HOST_DEFAULT_MTU` in `mod/net-test/build/_deps/enet-src/include/enet/enet.h`). `enet_peer_send` in that tree's `peer.c` turns an unreliable packet larger than the fragment length into reliable fragments unless `ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT` is set. This send path does not set that flag (same flags as `PlayerState`: unreliable, sequenced). A HUNTMAP4 snapshot is 5 animals, far under the MTU. A 32-by-31 snapshot is over it.

Flag bits: bit0 dead, bit1 parked, bit2 frozen-on-host. Other bits are passed through.

## Tests

`mod\net-test\build.ps1` on 2026-10-16. Both runs passed, including the new `animal loopback` row. Summary line:

```
checks passed 3294 failed 0
```

Previous total was 932. The new checks cover an empty animal list, 32 entries, a 40-character state name truncated to 31, ASCII names, truncated buffers, a count byte larger than the remaining payload, count 33, and a name length of 32. The loopback host broadcast an `AnimalStates` payload that all three clients decoded equal to the send. Client player 1 sent `AnimalHit` with `shooterId` 9. The host event had `shooterId` 1. A client `SendAnimalStates` and a host `SendAnimalHit` both returned false.

Host peer byte counters after that exchange, before the player-state wave: player 1 in=61 out=206, players 2 and 3 in=36 out=201. The +25 in and +5 out on player 1 match that client's Ping (5) plus AnimalHit (20), and the Pong (5) the host sent back. `AnimalStates` is in the shared out count.

`mod\build.ps1` linked `mod\build\dh2coop.asi` (1424 KB). The script then stopped with `Main7.exe is running; close it before installing the ASI.` The new ASI was not copied into `game\Bin`. The game was left running.

## Proposed MEMORY facts

None. These are mod protocol fields. No new engine export, RVA, or runtime log.

## Open questions

- Whether a 1768-byte `AnimalStates` stays unreliable on a real link, or ENet delivers it as reliable fragments. The loopback test covered a 3-animal snapshot, not a full 32-by-31 one.
- P4b owns when the host applies `AnimalHit` and how clients consume `AnimalStates`. This job does not rate-limit sends; the caller uses the existing 20 Hz `ConsumeSendTick`.
