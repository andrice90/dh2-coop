# P2c — ENet transport and protocol v1

## Progress

Done. `mod/net-test/build.ps1` built `dh2net_test.exe` (Win32, `/MT`, ENet v1.3.18, spdlog v1.17.0) and the exe exited 0. The suite runs twice in one process. No game launch. No Ghidra.

## Deliverables

- `mod/src/net/protocol.h`, `protocol.cpp` — v1 messages, little-endian reader/writer
- `mod/src/net/session.h`, `session.cpp` — `net::Session`
- `mod/src/net/interp.h`, `interp.cpp` — `net::SnapshotBuffer`
- `mod/net-test/` — own CMake project and `build.ps1`
- `docs/net-protocol.md` — layout, channels, handshake, integration sketch

`mod/CMakeLists.txt` already globs `src/*.cpp` (parallel job). These files were not added to that list by hand. They compiled here with the same `/W4 /permissive- /std:c++20 /MT` bar the ASI uses.

## Test results

Command: `.\mod\net-test\build.ps1` on 2026-10-16. Exit 0. 932 checks, 0 failures. Both passes printed the same relay matrix and the same per-peer byte counters.

```
check                      run1   run2
codec round-trip           pass   pass
truncated buffers          pass   pass
interp + yaw wrap          pass   pass
handshake ids 0..3         pass   pass
PlayerJoined fan-out       pass   pass
PlayerState relay          pass   pass
reject version             pass   pass
reject full                pass   pass
PlayerLeft                 pass   pass
host shutdown              pass   pass
enet_initialize once       pass   pass
digest match               pass
```

Relay counts (receiver rows, sender columns host / p1 / p2 / p3), both runs:

| | host | p1 | p2 | p3 |
|---|---:|---:|---:|---:|
| host | 0 | 60 | 60 | 60 |
| p1 | 60 | 0 | 60 | 60 |
| p2 | 60 | 60 | 0 | 60 |
| p3 | 60 | 60 | 60 | 0 |

That is one host and three clients, each sending 60 `PlayerState`s at 20 Hz for 3 s (seq 0..59, `tickMs = seq * 50`). Every client received every other player's states, including the host, through the host. Senders did not get their own state back. A state sent by p1 with `playerId` 9 was delivered as player 1.

Host `Peers()` after the handshake, both runs:

| player | rtt | loss | bytes in | bytes out |
|---|---:|---:|---:|---:|
| 1 | 1 ms | 0 | 41 | 114 |
| 2 | 1 ms | 0 | 36 | 109 |
| 3 | 1 ms | 0 | 36 | 109 |

Those sizes match the v1 layouts. Player 1 in = Hello 36 + Ping 5. Players 2 and 3 in = Hello 36. Out for player 1 = Welcome 7 + three `PlayerJoined` (34 × 3) + Pong 5 = 114. Out for the others = Welcome 7 + three `PlayerJoined` = 109 (no pong). RTT is ENet's sample, not the 500 ms default (`ENET_PEER_DEFAULT_ROUND_TRIP_TIME` in ENet 1.3.18 `enet.h`).

`Session::InitializeCount` rose by 1 for the whole overlapping set (host, three clients, the version-mismatch client, and the full-lobby client) and did not rise again until every session had been destroyed.

Also covered, without sockets: exact-size round trip of every v1 message, 31-byte name truncation, bit-exact floats, 2000 deterministic random buffers plus every truncated prefix and a trailing byte, reserved ids 16/32/47/48/63/64/79/80/95/96/255 rejected, linear motion at a 100 ms delay, yaw shortest-arc both directions across ±180°, pitch lerp, flag step, 1 s history trim (21 samples, oldest at newest−1000 ms), extrapolation of 100 ms and a cap at 250 ms with yaw/pitch held.

Loopback bound UDP **27420** (`BoundPort() == 27420`).

## How a timeout is told from a leave

Verified from ENet 1.3.18 `protocol.c` (the tree FetchContent put in `mod/net-test/build/_deps/enet-src`, tag v1.3.18):

- `enet_protocol_notify_disconnect` writes `event->data = 0` for a timeout and for the side that called `enet_peer_disconnect` (`protocol.c` around the notify helper and `enet_protocol_check_timeouts`).
- `enet_protocol_handle_disconnect` stores the remote's disconnect word in `peer->eventData` before the disconnect event is dispatched.

`Session` sends a non-zero code (1 graceful, 2 reject, 3 shutdown, 4 hello timeout) and remembers when it started the close. The test's leave and host-shutdown paths produced `Disconnected` and `PlayerLeft`, and zero `TimedOut` events. A real ENet timeout (silent peer, 5–30 s) was not run.

## Open questions

- Hunter yaw, pitch, crouch/prone/aim/fire bits, and weapon id are not pinned to fields. Wire health is a u8; the hunter's real health is profile attribute 9 (MEMORY). The coop module has to quantize. **UNVERIFIED** which engine fields feed `PlayerState`.
- Hello-timeout (2 s) and ENet's 5–30 s peer timeout are implemented and not exercised. **UNVERIFIED** at runtime.
- The host keeps `maxClients + 4` ENet slots so a full lobby can still send `Reject`. A connection past those slots is refused by ENet with no `Reject`. **UNVERIFIED** what that client observes (expected: connect timeout, data 0).
- On a lossy link, channel 1 can drop a `PlayerState`. The localhost test kept one seq in flight, so it does not measure loss. Interpolation is what covers a gap.
- NAT is out of scope. v1 is direct IPv4 UDP to 27420. No hole punching.
- `mod/CMakeLists.txt` was not built here (this job builds only `mod/net-test`). The ASI glob will compile `src/net/*.cpp` on the next ASI build. **UNVERIFIED** that the ASI link is clean, though the same sources already built against the same ENet and spdlog tags.

## Proposed MEMORY facts

No new engine facts. These confirm an existing one:

- The game still does not need to call `WSAStartup`. `net::Session` calls `enet_initialize` once while any session is alive, and that is the WSAStartup. Evidence: `dh2net_test` initDelta=1 across five overlapping sockets; ENet 1.3.18 `win32.c` `enet_initialize` calls `WSAStartup`. Matches MEMORY "No binary imports WSAStartup".
