# Mission end and trophy claim: engine map (lead, 2026-10-16 evening)

How a map ends, from the level scripts down to the results screen. This is what co-op event sync hooks.
Sources: `re\extracted\SCRIPTS\LEVELS\HUNTMAPBASE.SCR`, and Ghidra decompiles (gitignored):
- `re/notes/lead-natives.c`: the native registration `FUN_1000ba30`.
- `re/notes/lead-mission*.c`, `lead-claim*.c`
All addresses are GameDll7 v1.0 RVAs (image base `0x10000000`).

## Hunt map flow (scripts, `HUNTMAPBASE.SCR`)

1. **Death.** `cScrAnimalEventManager` state `routine` reads the animal's health every update. At ≤ 0 it goes to `dead`:
   - It shows the hint "Go claim your trophy!".
   - It sets `gbTrophyDowned`.
   - It waits until `"hunter"` is within `fDistToTrophy` (15 m), then calls `SpawnPredator`. The predator spawns at the nearest `predator` hunt-area point to the corpse, and only within 70 m of the hunter.
2. **Claim.** `SetClaim` calls `animal.SetCanBeClaimed(TRUE)` and `SetScrClaimCallback(controllerIdx, "ClaimCallback")` when the manager is created.
   - The engine shows the claim prompt ("Press F to get meat") on a dead claimable animal. Pressing it runs `ClaimCallback`, which calls `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1)` and sets `::globals.gbCanEndHunt = 1`.
3. **End.** `cScrHuntMapGlobals::Update` sees `gbCanEndHunt` and calls `GlobalsCheckMissionStatus()` once any of these holds:
   - the predator is invalid or off;
   - `the_predator.CanBeRespawned()`;
   - the hunter is 25 m from the predator.
4. Every `"hunter"` in these scripts is the local player only (`GetDistBetween2Ais("hunter", …)`, `::globals.player`). On the host, a client walking up to a deer or the trophy does not count. Open item.

## Natives (registered in `FUN_1000ba30`, `cExportKey::ExportFnc` + `s_ExportStaticFunction`)

| Script name | Function | Body |
|---|---|---|
| `GlobalsCheckMissionStatus()` | `0x98f0` | `cMission::CheckMissionStatus(g_mission)` |
| `UpdateMissionStatus(action, value)` | `0x8390` | `0x91e80(g_mission, action, value)` |
| `CompleteTask(id)` | `0x9080` | `0x95840(g_mission, id)` |
| `ActivateTrigger(name)` | `0x84a0` | find entity by name, `0x68eb0` |
| `GetDistBetween2Ais(a, b)` | `0x9200` | (not decompiled yet) |

`g_mission` = `*(cMission**)(GameDll7 + 0x1869e8)`. `cScrTriggerEvent::CheckMissionStatus` (`0x6a1a0`) also calls `0x94e50`.

## cMission

| RVA | Convention | What |
|---|---|---|
| `0x94e50` | thiscall(mission) | **CheckMissionStatus.** Status `+0x254` 2 = success when every task in the list at `+0x1f8` is done (`task+0x5c != 0` or `task+0x68 == 6`). Otherwise 1. On success: `SetMapIsComplete(*(hunter+0x1f8))` where hunter = `*(mission+0x258)`; then `0x50760(hunter, 0)`; then `0x91c20(5, "MISSION SUCCESSFUL!")`. |
| `0x965e0` | thiscall(mission, text) | **Fail.** Status 3, `0x95aa0`, `0x91c20(6, text)`. |
| `0x91e80` | thiscall(mission, action, value) | **UpdateMissionStatus.** Stores action/value at `+0x1dc`/`+0x1e0`. If `+0x1f0` is set, value == `+0x1e8` and action == 1, it decrements `+0x1e4`; at 0 it calls `0x91c20(6, "MISSION FAILED!")` (protected kill). The animal death vfunc `+0x154` (`0x1e8b0`) calls it. |
| `0x990b0` | thiscall(task, …) | Task update. Task state 5 calls vfunc `+0xc`; `+0x5c == 0` → `0x91c20(6, "MISSION FAILED!")`. |
| `0x95960` | fastcall(mission) | Marks every task state 6, then CheckMissionStatus (complete-all). |
| `0x91c20` | **thiscall(mission, char code, const char* text)** | **Mission message.** Writes server message `0x87` (code, text) into the in-process stream `DAT_10185ea8`. `MOD_SvParseMessage` (`0x3140`) turns it into the HUD text, and code 5/6 brings up the results screen. **Every mission end on every map passes here** (4 callers: `0x94e50`, `0x91e80`, `0x965e0`, `0x990b0`). The call sites are `push text; push code; mov ecx, mission; call`, and the callee cleans up. Ghidra showed it as stdcall because `this` is unused in the body. |

Status `+0x254`: 1 running, 2 success, 3 failed.

## Claim

- `cObjAnimal::ClaimCallback` `0x1cd40`, `__cdecl(cObjAnimal*)`. It is registered as a callback in `0x1ea90` and calls `0x15550` (fastcall animal):
  - `animal+0x404 = 0` (claimable flag off);
  - if `+0x408` (script object index) is set, `g_pClassManager` vfunc `+0x28(+0x408, +0x40c)`, which runs the script function named at `+0x40c` (`"ClaimCallback"`).
- Running `0x1cd40(animal)` on the host runs the host's script claim exactly like a local key press. Verified 20:04 (below).

## Co-op design now in `mod/src/coop/events.cpp` (verified 2026-10-16 20:04)

- **Mission message hook (`0x91c20`).**
  - Host: run it, then commit `kMissionEnd` (i0 = code, key = text).
  - Client: swallow (the client's scripts must not end its map).
  - Client apply: for code 5, status 2 + `SetMapIsComplete` on the client's own profile + `0x50760(hunter, 0)`, then the message. For code 6, status 3 + the message.
- **CheckMissionStatus hook (`0x94e50`).** Client only: skipped while online, so a client's own claim or tasks cannot run the success branch. Host and offline: unchanged.
- **Claim hook (`0x1cd40`).**
  - Client: clear `+0x404` locally and send a `kTrophyClaim` request.
  - Host on request: check the animal is dead and claimable, run the original, commit `kTrophyClaim`. Clients clear `+0x404`.
  - Host local claim: run it and commit.
- The hooks check the `55 8b ec 6a ff` prologue first and refuse other builds.
- Removed: the `SetMapIsComplete` / `GetMapToBeStarted` hooks (Grok, P4c). A client's success screen did not come from `SetMapIsComplete`; it came from `0x91c20`.

### Test 20:03–20:04 (`tools-src\event-test.ps1 -Keep`, HUNTMAP4)

Host `game\Bin\dh2coop.log`:
```
20:04:11.070 animals: coop_kill id=120 dmg=1000.0 hp 45.0->-955.0 ok=true
20:04:11.081 events: commit kind=1 subject=120 ...
20:04:16.308 events: claim id=120 by player 1 ran on host ok=true
20:04:16.327 events: host mission message code=5 text='MISSION SUCCESSFUL!' caller=GAMEDLL7.DLL+0x94f13
20:04:16.327 events: commit kind=2 subject=0 actor=0 i0=5 key='MISSION SUCCESSFUL!' seq=3 sent=true
```
Client `game2\Bin\dh2coop.log`:
```
20:04:11.100 animals: present death id=120 ok=true pos=(-0.9,2.5,63.1)
20:04:16.303 events: client claim id=120 sent to host
20:04:16.325 events: trophy id=120 claimed by player 1
20:04:16.335 events: applied host mission end code=5 text='MISSION SUCCESSFUL!' ok=true
```
- Screenshots `re/notes/coop-local/event-200320-*.png`. The client showed the "Go claim your trophy!" hint after the host kill (`client-1killed`). Both windows show the MISSION SUCCESSFUL results screen (`host-2claim1`, `client-2claim1`).
- The host's script ended the hunt on a client's claim within 20 ms. Its hunter never went near the trophy, so no predator spawned.

## Open

- **Death fall on the client** (`PresentDeath` ok=1): visual check pending. The deer was off-screen in the test.
- **Real claim key** on the client: the test called the hook directly. A real F press should reach the same hook (same callback).
- **Results screen buttons:** ESC replays, RETURN continues. These are per-process today. The host's choice should drive the clients (map flow sync).
- **"hunter" means the local player in scripts** (spook distance, trophy distance, predator target and safe distance). Host-side option: hook `GetDistBetween2Ais` (`0x9200`) so `"hunter"` resolves to the nearest player.
- **Failure paths** (`0x965e0`, `0x990b0`, `0x91e80`) go through the same message hook, but none has been seen in co-op yet.
