# Grok session handoff — Phase 4 animals and world events

**Written:** 2026-10-16 16:32 (America/Chicago) by Grok 4.7, lead away. **HEAD:** `926d351` on `master`. No git remote, so nothing was pushed. Author on the Grok commits is `andri <andrice90@gmail.com>` with trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`.

Read this, then `STATUS.md`, then `re/notes/p4c-events.md`. Do not edit `MEMORY.md`. Proposed facts are at the bottom of the P4b and P4c notes.

## Where the work stopped

The 16:36 pair is **not running**. The user said both crashed. Details and log quotes: `re/notes/p4c-events.md` section "16:36 both windows exited". No dump, no SEH line. Do not claim the death animation or the shared success screen works.

`GetLastmap` (7-byte getter, never observed by the probe) is no longer hooked. The profile pointer is captured from `GetMapToBeStarted`, which the spmap probe did call. `SetMapIsComplete` stays hooked and now logs on entry. `coop: join start` is logged before `Session::Join`. That build had not been played yet when this paragraph was written.

CHOOSE ITEM is still Enter, Left, Enter. Host is the left window (`game\`, `-coophost`). Client is the right (`game2\`, `-coopjoin 127.0.0.1`). Ask before closing windows the user is playing. `mod\build.ps1` cannot install while `Main7.exe` is running.

## User reports (verbatim)

1. Started from `docs/handoff-grok-p4.md` and continued Phase 4.
2. "position looked good. i shot at it and killed it as the server but the client still saw it standing there. then i approached it as the client and got a prompt to "Press F to get meat" which ended the game in success. Although the client sees a success screen and the server it still in game and does not see that success."
3. "more than just this exact scenario, I think client-server event sync should be looked at next and built in a way that can scale to other maps and modes."
4. Write this file when done.

## Commits this session

| Commit | What |
|---|---|
| `b8c4032` | P4b. Clients mirror the host's animals (position, yaw, body-state name, health). |
| `926d351` | P4c. `GameEvent` id 48. Animal death presentation and host-authoritative map completion. |

Parent of the session is the lead handoff `8f90a05`. P4a (`14250e0`, `AnimalStates` / `AnimalHit`) was already committed. Do not edit `mod/src/net/*` again unless the wire contract is wrong. `GameEvent` was an explicit user request, which is why the net layer changed.

## What P4b does (user-confirmed positions)

Host, while at least one client is connected, every 50 ms: read `Animals()`, send `AnimalStates` on channel 1. Yaw is movement yaw (`atan2(dx, dz)`), not `+0xF4`. Dead flag when health ≤ 0. Parked flag when `y < -1000` (the puma, id 124, stays at -10000).

Client, in the pre-render callback (not OnFrame): `FreezeBrain(true)`, keep physics, `HandleKey("position")` / `HandleKey("angle")`, refresh the world AABB, `BspUpdate`. First placement repeats 3 frames. Re-apply `SetBodyState` whenever the live name differs from the sample. `FreezeBrain` does not pin the state machine. A local health drop of more than 0.01 sends `AnimalHit`. The host calls `FuncInflictDamage`.

Automated run 2 (16:05, the shipped re-apply build): after the first pre-apply sample, 28 pairs per id, max position error 0.010 m, yaw error 0, 0 state mismatches. Grazing `PostPlayAnim` on the host lasts ~10 ms and usually misses the 50 ms snapshot. That is not a client bug.

`[Animals] Spike=1` stays solo-only and is off. UnlinkPhysics hides animals. Do not use it on the sync path.

## What the kill / harvest showed (build `b8c4032`, before events)

Play 16:08–16:13. Those log files were truncated by the 16:31 relaunch. The lines below are copied into `re/notes/p4c-events.md`. No `AnimalHit` and no `FuncInflictDamage`. The user shot as the host.

Host deer 121: `Hurt` hp 27 → `PostPlayAnim` hp 0 → `Die` hp 0 → `Dead` hp 0 at 16:09:32. Sync then sits near `(-26.68, 1.26, 50.81)`, yaw 291.6, hp 0, through 16:12:58.

Client deer 121: `Hurt` hp 27, then `Dead` hp 0 at 16:09:32.505. It never logged `Die`. Sync drift 0.000 against the host corpse. The user still saw it standing. `SetBodyState("Dead")` is not the fall. Per-frame `HandleKey` was still driving the root.

The client then got "Press F to get meat" and a success screen. The host was still logging `coop: remote` at 16:13:00. The meat prompt and the success screen are engine UI. They do not appear in `dh2coop.log`. The known completion call is `cHunterProfile::SetMapIsComplete` (`?SetMapIsComplete@cHunterProfile@@QAEXXZ`, RVA `0xec160`).

## Event sync (commit `926d351`)

Continuous state stays on snapshots. One-shot outcomes use one reliable envelope, `net::GameEvent`, message id **48**, channel 2. Layout is in `docs/net-protocol.md`.

- A **request** is client → host. `Session` overwrites `actorId` from the peer, same rule as `AnimalHit.shooterId`. The host does not relay the request.
- A **commit** (flags bit 0) is host → every client. The host does not apply its own commit. A client `SendGameEvent` of a commit returns false.
- `kind` is a `u16` the session does not interpret. Unknown kinds still decode. The game layer logs once and ignores a kind with no handler. A new map adds a handler, not a packet.
- `subjectId` is an entity id, or 0 for the map. `key` is up to 31 bytes (an objective or item name).
- The host journal holds 64 commits and resends them on `PlayerJoined`, so a late client hears deaths and map completion. Same kind + subject + key resends the stored commit.

`dh2net_test.exe` after this change: **3406 checks, 0 failures, two runs.** New section `game event loopback` covers the actor rewrite, commit fan-out, and kind `99` with key `objective.escort`. Ids 49–63 stay reserved.

Registry (`mod/src/coop/events.h`):

| Kind | Value | Commit | Apply |
|---|---|---|---|
| `kAnimalDied` | 1 | Host, first time health ≤ 0 and the animal is not parked. | Client places once at the corpse, calls `entity::PresentDeath` (animal vfunc `+0x154`), then stops `HandleKey` and `SetBodyState` for that id. |
| `kMapComplete` | 2 | Host, when `SetMapIsComplete` runs, or when a client request is accepted. | Client hook swallows the local call and sends a request. Host runs the real function and broadcasts. Client runs it when the commit arrives. |

`events::SetHandler(kind, onRequest, onCommit)` is the extension point. Either callback may be null. `AnimalDied` requests are ignored because the host simulates death. A client bullet is still `AnimalHit` → `FuncInflictDamage`. The death commit follows if health hits 0.

The profile pointer comes from `GetMapToBeStarted` (proven call in the spmap probe) and from `SetMapIsComplete`'s `this`. There is no singleton export. If a map-complete arrives before that pointer exists, `events::Pump` retries. Offline, `SetMapIsComplete` calls the original. That solo path was not played this session. Do not hook `GetLastmap` again: it is 7 bytes and the 16:36 exit is the reason it was removed.

Module priority: coop 40, events 41, animals 46. `coop.cpp` drains `GameEvent` into `events::OnNet`, calls `events::Replay` on `PlayerJoined`, `events::Reset` on map load / leave / client disconnect, and `events::Pump` at the end of the frame. The host does not clear the journal when a remote leaves.

## Files

- `mod/src/coop/events.h`, `events.cpp` — registry, journal, the two hooks, the two kinds.
- `mod/src/coop/animals.cpp` — death commit on the host. On the client, present once and stop driving. `OnAnimalDied`.
- `mod/src/coop/coop.cpp`, `coop.h` — `SendGameEvent`, `NetIsHost`, `NetIsOnline`.
- `mod/src/engine/entity_api.cpp` — `PresentDeath` (vfunc `+0x154`, SEH-guarded).
- `mod/src/net/protocol.*`, `session.*` — `MsgId::GameEvent = 48`. `mod/net-test/main.cpp`.
- `docs/net-protocol.md`, `PLAN.md` (user decision 2026-10-16), `STATUS.md`, `mod/dh2coop.ini` (comment only, no new key).
- `re/notes/p4b-animal-sync.md`, `re/notes/p4c-events.md`.

## Discoveries

- Client drive that stays visible: `FreezeBrain(true)`, physics kept, pre-render `HandleKey` position and angle, world AABB, `BspUpdate`. Evidence: P4b run 2, drift 0.000, user said positions looked good.
- `FreezeBrain` does not hold the body-state name. Run 1 deer 121 showed `IdleAttack` for ~4 s while the host stayed `CutScene`. Re-apply when the live name differs. Run 2 had 0 mismatches.
- Movement yaw matched on both sides when a deer walked (id 122, yaw 171.4).
- `SetBodyState("Dead")` plus health 0 does not play the fall. The host logged `Die` then `Dead`. The client logged `Dead` only, drift 0.000, and the user saw it standing. Death presentation is vfunc `+0x154` (`FUN_1001e8b0`): early-out if `+0x600 != 0` or byte `+0xC1` mask `0x4`; otherwise props `| 0x480`, health 0, brain vfunc `+0x94(1)`, `AddDeadAnimal`. Whether `+0x94` is `cAIHuntBrain_Animal::Die` is still UNVERIFIED. Do not start Ghidra. That is lead work.
- The meat prompt appeared from health 0 / `Dead` alone, without `+0x154`. Pressing F completed only the client. Treat mission completion as a host commit.
- Animal ids 120–124 match across instances on HUNTMAP4. Spawn positions do not. Network id = entity id.
- Launching `run-coop-local.ps1` truncates both `dh2coop.log` files. Quote anything you need into a note before the next launch.
- Parallel `mod\build.ps1` and `mod\net-test\build.ps1` tripped MSVC `C1090` (PDB server, `0x6BA`). Build them one at a time.
- `RTK.md` from a user rule is not in this repo.

## UNVERIFIED — the next session's job

1. Host kills a deer. Client should fall, not stay standing. Expect `animals: death commit` on the host and `animals: present death id=… ok=1` on the client. If `ok=1` and it is still standing, `FreezeBrain` may be blocking the fall. Next try is unfreezing after `PresentDeath`. Do not dig into skeleton culling.
2. Client presses F on "Press F to get meat". Both windows should get the success screen. Expect `client SetMapIsComplete swallowed`, then `map-complete request`, then the host runs it and `applied host map-complete`. If the client still finishes alone and that swallowed line is absent, the success screen is a different function. Lead work.
3. A second success popup on the client is possible if the prompt's caller shows UI after `SetMapIsComplete` returns.
4. Shoot from the **client**. `AnimalHit` has never been seen in a log. If the host deer does not flee, write that down. Do not decompile the hurt path.
5. Solo `SetMapIsComplete` (no session) was not played. The hook calls the original when `NetIsOnline()` is false.
6. The 3-frame teleport versus model culling is still open. The user could see the deer, so the first teleport did not make it permanently invisible.
7. A client who joins after the host already completed the map gets the journal replay and may complete immediately. That is intended and untested.

## Do not

- Write to `D:\scary hunt\Activision Value\...`.
- Commit `game\`, `game2\`, `tools\`, `re\ghidra\`, binaries, or `MEMORY.md`.
- Close the user's game windows without asking.
- Edit `mod/src/net/*` for a new outcome. Add a kind in `events.cpp` instead.
- Call `PresentDeath` on the host. The host's own shot already played it.
- Drive a dead client animal with `HandleKey` or `SetBodyState` after the one-shot present. That is what pinned the standing pose.
