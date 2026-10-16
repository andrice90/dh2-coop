# P4c — host-authoritative world events

## Why this exists

User, after the position sync looked right (2026-10-16, play session 16:08–16:13):

> i shot at it and killed it as the server but the client still saw it standing there. then i approached it as the client and got a prompt to "Press F to get meat" which ended the game in success. Although the client sees a success screen and the server it still in game and does not see that success.

The follow-up: this is not a one-off for that deer. Client/server event sync should scale to other maps and modes.

Positions stay on the 20 Hz snapshot. Things that happen once (died, harvested, map complete, and later an objective, a pickup, a spawn) are reliable events. The host is the only peer that commits an outcome. A client sends a request. Another map adds a **kind** and a handler. It does not add a message id.

## What the 16:09 logs already showed

Host `game\Bin\dh2coop.log`, client `game2\Bin\dh2coop.log`. No `AnimalHit` and no `FuncInflictDamage` in either file. The kill was the host's own shot.

Host id 121:

- `16:09:31.865` `state='CutScene' … hp=27`
- `16:09:32.407` `state='PostPlayAnim' … hp=0`
- `16:09:32.416` `state='Die' … hp=0`
- `16:09:32.426` `state='Dead' pos=(-25.8,2.2,49.5) hp=0`
- from `16:09:34` the sync line stays `state='Dead' hp=0.0` near `(-26.68, 1.26, 50.81)` yaw 291.6 through `16:12:58`

Client id 121:

- `16:09:31.155` `state='Hurt' … hp=27`
- `16:09:31.965` `state='CutScene' … hp=27`
- `16:09:32.505` `state='Dead' pos=(-25.8,1.1,49.6) hp=0` (no `Die` line)
- matching sync lines, `drift=0.000`, through `16:12:58`

Health 0 and the name `Dead` did replicate. `SetBodyState("Dead")` did not play the fall, and per-frame `HandleKey` kept driving the root. Both logs still show a live `coop: remote` line at `16:13:00`, so the host was still in the match when the client had already finished. The meat prompt and the success screen are engine UI. They are not in `dh2coop.log`.

## Wire

`GameEvent`, message id **48**, channel 2, reliable. Layout in `docs/net-protocol.md`.

| Field | Role |
|---|---|
| flags bit0 | commit (host → every client) or request (client → host) |
| actorId | host overwrites this on a request, same rule as `AnimalHit.shooterId` |
| kind | game registry. The session does not interpret it. Unknown kinds decode and are ignored. |
| subjectId | entity id, or 0 for a map/objective |
| i0, f0, key | kind-specific. `key` is up to 31 bytes so an objective name does not need a new packet. |

The host keeps a journal of the last 64 commits and resends it when a player joins, so a late client still hears "this animal died" and "this map completed". Repeating a commit for the same kind/subject/key resends the stored one.

`dh2net_test.exe` 2026-10-16: 3406 checks, 0 failures, two runs, including request actor rewrite, commit fan-out, and an unknown kind (`99`, key `objective.escort`).

## Kinds now

| Kind | Value | Who commits | Apply |
|---|---|---|---|
| AnimalDied | 1 | Host, the first time an animal's health is ≤ 0 and it is not parked (`y < -1000`). | Client calls `cScrAnimal` vfunc `+0x154` (`entity::PresentDeath`) once, after one place at the corpse. Later frames do not `HandleKey` or `SetBodyState`. |
| MapComplete | 2 | Host, when `cHunterProfile::SetMapIsComplete` runs, or when a client request is accepted. | Client hook swallows the local call and sends a request. The host runs the real call, then broadcasts. The client runs it when the commit arrives. |

Handlers are `events::SetHandler(kind, onRequest, onCommit)`. `AnimalDied` requests are ignored: the host simulates death. A client bullet still uses `AnimalHit` → `FuncInflictDamage`, and the death commit follows if that drops health to 0.

`SetMapIsComplete` is an export hook. The profile pointer comes from `GetMapToBeStarted` (and from `SetMapIsComplete`'s `this`). Offline, `SetMapIsComplete` still calls the original. That solo path is UNVERIFIED this session.

`GetLastmap` was hooked in the 16:31 build and removed after the 16:36 exit. It is `lea eax, [ecx+0x23D9]; ret` (GameDll RVA `0x11c0`, 7 bytes). The probe installed that hook on 2026-10-16 and never logged a call (`re/notes/runtime-spmap-probe.txt`). `GetMapToBeStarted` (RVA `0xeacd0`) did run in that probe: `this=0xd985888` → `'map1'`.

## 16:36 both windows exited

User, after `.\run-coop-local.ps1` (Seconds=0, host pid 23124, client pid 6228): "both crashed".

No `Main7.exe` left. No `dh2coop-crash.dmp` on either side. No Application Error, no new WER report, no System event at 16:36. The vectored handler never logged a first-chance exception, so this was not an SEH the filter can see (process exit, or a failure that never reached the handler).

Host `game\Bin\dh2coop.log` last lines (map load `16:36:02.313`, still hosting, no peer):

- `16:36:04.514` hunter yaw moved `151.87` → `150.59`
- `16:36:04.694` animals 120/122/123 `PostPlayAnim` hp 45
- `16:36:04.705` those three back to `CutScene`. No `AnimalHit`, no `SetMapIsComplete`, no death line.

Host `debug.log` shows `CM_YES` (CHOOSE ITEM), then `Could not set anim 'idletorun'` on `reddeer.skin`, then one more fps line at `time=3.011998` (`fn=229`, 99 fps, 2.7M tris) and the file stops.

Client `game2\Bin\dh2coop.log` starts `16:36:04.584`. Map load `16:36:05.275` while `role=offline`. Last line `16:36:05.386` `coop_status: role=offline`. No `coop: join start`, no welcome. `debug.log` stops at `SERVER loading time : 203ms`.

The 16:08 session (build `b8c4032`, no profile hooks) stayed in the match for minutes. The 16:36 binary is `c10dc47` plus the event hooks. Animal mirroring was not active: the host had no peer, and the client never joined. `events::Pump` was a no-op. The new code that had run was the two hooks sitting in GameDll.

## UNVERIFIED until the next play

- Whether `PresentDeath` on a `FreezeBrain`'d client animal actually plays the fall. The vfunc also calls brain `+0x94(1)` and `AddDeadAnimal`. `+0x94` being `Die` is still UNVERIFIED (lead note). If the deer stays standing, the next try is to unfreeze after the call, not to decompile the skeleton.
- Whether both windows show the success screen after the client presses F. The host should leave the match too. A second success popup on the client is possible if the prompt's caller shows UI after `SetMapIsComplete` returns.
- `AnimalHit` from a client shot. Still never seen in a log.
- Whether "Press F to get meat" writes anything besides `SetMapIsComplete`. If the client still completes alone, the log line `client SetMapIsComplete swallowed` tells us the hook saw it. If that line is absent, the success screen is a different function. That is lead work. Do not start Ghidra for it.

## Proposed MEMORY facts

- `SetBodyState("Dead")` plus health 0 does not play an animal's death. Host id 121 logged `Die` then `Dead` at 16:09:32; the client logged `Dead` only and the user still saw it standing while sync drift was 0.000.
- Death presentation remains animal vfunc `+0x154`. The coop client now calls it once per death. Whether that plays the fall on a frozen animal is UNVERIFIED.
- `cHunterProfile::SetMapIsComplete` (export `?SetMapIsComplete@cHunterProfile@@QAEXXZ`) is the call a client harvest reached. It completed only that process until the MapComplete event. The in-game result of routing it through the host is UNVERIFIED.
