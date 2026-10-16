# Job: P4a — animal messages in the net layer (mechanical; no game launch, no Ghidra)

Read first: `re/jobs/_rules.md`, `STATUS.md`, `re/notes/lead-animals.md` (design), `docs/net-protocol.md`,
`mod/src/net/protocol.h`, `mod/src/net/protocol.cpp`, `mod/src/net/session.h`, `mod/src/net/session.cpp`, and
`mod/net-test/main.cpp` (existing tests, 932/932 passing).

## Goal
Add the two Phase 4 messages, end to end: encode/decode, session send/receive, unit tests.

## Messages (protocol version stays 1; append new type ids after the existing ones)
1. `AnimalStates`: host → all clients, **unreliable** channel, sent at 20 Hz. Fields:
   - `uint16_t seq` (host counter), `uint32_t tickMs` (host clock)
   - `std::vector<AnimalState> animals`, count encoded as u8, at most 32 entries
2. `AnimalState`, one per entry:
   - `uint16_t id`: entity id, e.g. 120–124
   - `float pos[3]`
   - `float yaw`: degrees, 0 = +z, 90 = +x
   - `float health`
   - `uint8_t flags`: bit0 dead, bit1 parked/inactive, bit2 frozen-on-host (reserved)
   - `std::string state`: body-state name. Encode as a length-prefixed u8 length (max 31 chars, truncate longer).
3. `AnimalHit`: client → host, **reliable**. Fields: `uint8_t shooterId`, `uint16_t animalId`, `float damage`,
   `float hitPos[3]` (may be zeros).

## Session API (mirror `SendPlayerState`)
- `bool SendAnimalStates(const AnimalStates&)`: host only; broadcast unreliable. Returns false on a client.
- `bool SendAnimalHit(const AnimalHit&)`: client only; reliable to the host. Returns false on the host.
- Incoming messages reach the handler/event queue the same way `PlayerState` does: add `EventType::AnimalStates` and `EventType::AnimalHit` with the payload in `Event`. Fill `shooterId` on the host from the sending peer's player id, not from the payload. Look at how the existing code maps a peer to a player id.

## Tests (`mod/net-test/main.cpp`)
- Encode→decode round trip for each message, including an empty animal list, 32 entries, a 40-char state name (truncated to 31), and unicode-free ASCII only.
- Decode rejects truncated buffers and a count byte larger than the remaining payload.
- Session loopback test, if the existing tests already have a host+client loopback: the host sends `AnimalStates`, the client receives an equal payload; the client sends `AnimalHit`, the host receives it with the right `shooterId`.
- All existing tests still pass. Report the new total.

## Rules
- Edit only `mod/src/net/*`, `mod/net-test/*` and `docs/net-protocol.md` (document both messages: field table and type ids).
- Do not touch `mod/src/coop/*` (the lead or P4b wires it in).
- Build with `mod\net-test\build.ps1` and run the test exe. Then build the mod with `mod\build.ps1` to prove it still compiles. If `Main7.exe` is running, the install step fails. That is fine: report it, don't kill the game.

## Deliverables
The code, the tests passing (paste the summary line), the `docs/net-protocol.md` update, and `re/notes/p4a-net-animals.md` with what changed and `## Proposed MEMORY facts` (none expected).
