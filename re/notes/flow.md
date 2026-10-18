# flow: mission flow for every player (job `flow`, 2026-10-18)

Goal: every player leaves each level together to the same next map, fails together, retries together, and reaches the end of the game together. Worktree `D:\scary hunt\dh2coop-flow`, branch `mission-flow` (based on main `7b2af06`, with the same-map replay fix). Port 27640, Ghidra slot D. Stopped early: the user's session limit was nearly used up.

## Branches and commits
- `mission-flow-wip` `381f226`: the earlier agent's uncommitted work, kept as-is (old base `f45f1a3`). It had pause sync (kHostPause) and a host movie relay (kMovie). I did not port those (see "Not ported").
- `mission-flow` `5ae6824` "WIP: mission flow: host-decided exits and continue (MAP3A, MAP3B verified)": all code below.

## What the code does (`mod/src/coop/flow.cpp`, kinds in `events.h`)
| Part | Mechanism | Status |
|---|---|---|
| Level exits | Hooks `cObjTriggerLevel` walk-in `0x6c420` and immediate path `0x6c610`. Online client: never runs its own exit, sends `kLevelExit` (24: subject = trigger id, key = name, i0 = fast), 5 s throttle per exit. Host: runs a requested exit with the mission's hunter (`cMission +0x258`) only if its copy is armed (`+0x1e8`) and no exit ran on this map yet. An exit "ran" when its cooldown `+0x1e4` turned to -5. | VERIFIED MAP3A, MAP3B |
| Exit message and completion | Host HUD hook `0xd8db0` commits code 7 "Checkpoint reached" as `kMissionEnd` (it bypasses `0x91c20`). Client `ApplyMissionEnd` code 7: `SetMapIsComplete` on its profile, `0x50760(hunter, 2.0f)`, message. The client sees the game's autosave (save slot) menu and can save. | VERIFIED |
| HUD gate | Client: mission messages 5/6/7 are shown only when `ApplyMissionEnd` allowed that code (`flow::AllowMissionMessage`); its own guide death (`0x5fde0` code 6), exit, and `0x96750` status report are dropped. Host: any HUD mission message not yet journaled is committed (`events::CommitHudMissionMessage`, dedup by text). | code 7 VERIFIED; code 6 from a guide UNVERIFIED |
| Profile map sync | On every story map load, `0xa800(rules, "map3b")` when `cHunterProfile::GetMapToBeStarted` differs. A `-spmap` start leaves the profile at `map1`, so an exit went to MAP2A (the old "MAP3B exit_1 loaded MAP2A"). `SetMapIsComplete` increments the profile's mission index (`profile +0x23d8`); `mod_newlevel` loads `GetMapToBeStarted`. | VERIFIED (MAP3B exit -> MAP3C) |
| Host continue | Hook on console `mod_newlevel` (`0xc0080`). Host sends `kLevelNext` (26, i0 = 1, key = "map3b\|human2": profile next map and rules `+0xbc` spawn) and runs it. Client: sets its profile map and spawn, queues `closemainmenu`, `closegamemenu`, `mod_newlevel` (what `GameLoadLevel` `0xbea20` does), so it plays the same chapter movies (outro of index-1 / intro of index, by table type byte `+0xb`) and loads the same map. A later `kMapStart` for that map is skipped (`flow::AlreadyFollowing`) or seen as "got here on its own". | VERIFIED MAP3A->MAP3B, MAP3B->MAP3C (both arrive, same spawn, host 0 m) |
| Client menu choice | Hooks the game-menu natives (`GameLoadLevel` `0xbea20`, `GameLoadHunt` `0xbeb60`, `GameLoadMission` `0xbeca0`, `GameLoadBonus` `0xbedf0`, `RestartMission` `0xbe6a0`, `Restart` `0xc0770`, `Quit` `0xbef40`). Online client: not run, banner "Waiting for the host to choose"; on a results screen the stats page is posted back (`cGuiEvent::PostMessageA` 13134/13137). Host: runs them; retry / main menu / another map send `kLevelNext` 3/4/5 for the banner, the map change reaches clients as `kMapStart`. | client wait VERIFIED (MAP3B); host retry/quit UNVERIFIED |
| End of game | Hook on `mod_playendfmv` (`0xbe480`, plays `Movies\omap12c`). Host sends `kLevelNext` 2; client queues its own `mod_playendfmv`. Then the host's `Quit` (sv_stop) -> `kMapStart MENUMAP`. Credits are commented out in `GAMEMENU.SCR`, so there are none. | UNVERIFIED |
| Ammo-gated tasks | Hook on `0x2f530` (fastcall, called by task `CanBeCompleted` `0x97440`/`0x979f0` for `NEED_AMMO_FOR_COMPLETE`). Online client: always 1. Host: own ammo, else 1 while any living remote (`Snapshot()` flags without `kFlagDead`) reported ammo (`kAmmoState` 25 request, on change and every 30 s; no report = counts as armed). Test: `coop_ammofake 0|1|-1`, `coop_ammocheck`. Did not use a `PlayerState` field or flag. | UNVERIFIED |
| Client death + F9 | Poll: local hunter `+0x6cc` 1 -> 0 while the client's mission status is 3 and no host end was applied: status back to 1 (`0x50350` -> `0x965e0` set it; `0x95aa0` only stops task music, `0x96bb0`). The `0x96750` re-emit is dropped by the HUD gate. | UNVERIFIED |
| Follow via kMapStart | `events.cpp` `OnMapStartCommit`: `BeforeFollow` (clear HUD menu code `+0xd0`, queue `closegamemenu`), then `mod_spmap <lower map>` from a level too (was `spmap`): it closes the menus at once and sets the profile map. Menu map: `menumap MENUMAP`. | UNVERIFIED (F5/F6 and retry paths now use it) |
| coop.cpp (lead file) | `TeleportToHost` returns false while `!SameMapAsHost()` (+4 lines). | compiled only |
| UI | Overlay banner line (`flow::Banner()`), `coop_shot <name>` back-buffer screenshots (`ui/shot_module.cpp`), `tools-src/flow-lib.ps1` (Start-Flow/Send-Cmd/Shot/Save-FlowLogs/Stop-Flow, port 27640). | used in tests |

Ini keys `[Flow]` (default 1): `ClientExitAsksHost`, `AmmoAnyPlayer`, `SyncProfileMap`, `FollowHostMenu`.

## Evidence
- MAP3A (`re/notes/coop-local/flow-m3a-*`): host `coop_trigger cScrTask14` x2, client `coop_tp -130.0:12:-11.3`. Client `flow: client's own level exit 'exit_2' (id 93, target 'map3b,human2') not run; asked the host`; host `flow: level exit 'exit_2' ... ran for the host's hunter: true; next map 'map3b', spawn 'human2'`, `commit kind=2 ... i0=7`; client `applied host mission end code=7` and the save menu with the banner (`flow-m3a-exit-game2.png`); host `coop_results continue` -> `host continues to 'map3b' (spawn 'human2')`; client `following the host's continue`, `client mod_newlevel with the host`, `StartGame('map3b')`, `host map 'MAP3B' (serial 2) is already loaded here (this client got here on its own)`; screenshot `flow-m3b-arrive-game2.png` shows MAP3B with "Host 0m".
- MAP3B (`flow-m3b-*`): host `coop_trigger cScrTask05` x2 -> host `level exit 'exit_1' (id 43, target 'map3c,human') ran; next map 'map3c'`; the client's replayed fire `asked the host` -> host `this map's exit already ran`; client `coop_results continue` first -> `waiting for the host's choice`, banner "Waiting for the host to choose"; host continue -> both `StartGame('map3c')`.
- In-game movies present through D3D at 30 fps (host heartbeat `window_frames=300` during `mod_playendfmv`, screenshot `flow-movie-host.png`): OnFrame and the network keep running, no timeout over ~60 s.
- Engine facts used (decompiles in slot D, not committed): `0x6c420`/`0x6c610`, `0x6a4d0`/`0x6a610`, `0x67b0`, `0x5fde0`, `0x96750`, `0x979f0`/`0x97440`/`0x2f530`, `0x50350`/`0x965e0`/`0x95aa0`, `0xbe480`, `0x3140`, `0xd8db0`/`0xc9ca0`, `0xbea20`/`0xbe6a0`/`0xc0770`/`0xbef40`, console registration `0xbd6d0` (handlers: `mod_newlevel 0xc0080`, `mod_restartmission 0xc3cb0`, `mod_startlevel 0xe8360`, `mod_spmap 0xbd4b0`, `mod_playendfmv 0xbe480`), `0xa800`, `GetMapToBeStarted 0xeacd0`, `SetMapIsComplete 0xec160`; Main7 `sv_restart` `0x20df0` sets server `+0x1e1c` (restart request), `spmap` `0x209b0`.

## Per-map verdicts
| Test | Result |
|---|---|
| MAP3A exits (audit-a G2) | VERIFIED: client exit_2 -> host runs it -> both MAP3B at human2 |
| MAP3B exit_1 | VERIFIED: both MAP3C (was MAP2A) |
| MAP11A exit (audit-c G7) | NOT RUN (pilot maps need the pilot agent's tracker fix; the exit hook does not depend on it) |
| MAP2B guide death (audit-a G12) | NOT RUN (`coop_npcdie <hugh id>` on the host is the test command) |
| Forced failure then retry | NOT RUN (`coop_down` on the host, then `coop_results restart` on the host) |
| MAP2C -> MAP3A chapter change | NOT RUN (expect `imap3a` on both via each side's `mod_newlevel`) |
| MAP12C end | NOT RUN (`coop_results continue` on the host after the yeti; expect `mod_playendfmv` on both, then MENUMAP) |

## Open items / exact next steps
1. Retry: `.\run-coop-local.ps1 -Map MAP2B -Port 27640`; host `coop_down` (host death = MISSION FAILED on both); host `coop_results restart` (fail menu `Restart` 0xc0770 -> `sv_restart`); check both reload MAP2B (client `following host ... mod_spmap map2b`), then F6 or another map change to prove the next teardown does not crash. If `sv_restart` misbehaves, route the host's `Restart`/`RestartMission` to the lead's main-menu hop.
2. Guide death: MAP2B, host `coop_npcdie <id of hugs2b>` (ids from `animals:` NPC lines). Expect host `host HUD mission message code=6 ... committing it`, client `applied host mission end code=6`, and no `client's own mission message` shown.
3. MAP2C -> MAP3A: host `coop_trigger` to the end or `CheckMissionStatus` path, host `coop_results continue`; check both play `imap3a` and arrive.
4. MAP12C end and MAP11A/11C (ammo with `coop_ammofake 0` on the host and the client at 1/0) once the pilot tracker lands.
5. Client death: `coop_down` on the client, `coop_respawn`, expect `flow: client revived; its mission status 3 -> 1`.
6. A first-chance AV at `dh2coop.asi+0x1144fd` (`memcpy`, so a `safe_read`) on the host at each `coop_trigger cScrTask14` on MAP3A: caught, not from flow (it happens inside the trigger command path); worth a look by the triggers owner.
7. Not ported from `mission-flow-wip`: host pause sync (`kHostPause`, `Game_Pause 0xf2e0` / `cSpServer +0x1f2c`) and the host movie relay (`kMovie`); the continue path makes the client play its own movies instead.

## Numbers used
Kinds 24 `kLevelExit`, 25 `kAmmoState`, 26 `kLevelNext`; 27 unused. Protocol message 15 and the `PlayerState` ammo field/flags are unused. Note: main already has `kFlagKnocked = 1 << 5` and `kFlagDead = 1 << 4`, which collide with the brief's flow (`1<<5`) and pilot (`1<<4`) allocations.

## Proposed MEMORY facts
- Level exits (`cObjTriggerLevel` `0x6c420` walk-in, `0x6c610` immediate) take the next map from the profile, not the BSP: `SetMapIsComplete` (`0xec160`) does `SetLastMap(GetMapToBeStarted)` and `profile +0x23d8 += 1` (story mode), and `mod_newlevel` (`0xc0080`) spmaps `GetMapToBeStarted` (mission table entry `+0x23d8`, names lower case, 21 story maps then huntmap1-4 and bonus maps). A `-spmap` start leaves `+0x23d8` at 0 (map1). `0xa800(rules, "map3b")` (what `mod_spmap` and the menus call) sets it. Verified MAP3B exit -> MAP3C after the sync (2026-10-18).
- The exit's code 7 opens `gameAutosave` (save slot menu), which then calls `GameLoadLevel` (`0xbea20`: closemainmenu, closegamemenu, `mod_newlevel`). `mod_newlevel` plays `Movies\o<prev>` when the previous table entry's type byte `+0xb` is 2/3 and `Movies\i<next>` when the next is 1/3.
- In-game movies (`EngIoToolPlayMovieInGame`) present through D3D at 30 fps: our OnFrame and ENet keep running during a 60 s movie (2026-10-18 10:00).
- Console handlers (GameDll `0xbd6d0`): `mod_spmap 0xbd4b0` (clearcache 1, then `0x36b0`), `mod_newlevel 0xc0080`, `mod_newhunt 0xc03f0`, `mod_startlevel 0xe8360`, `mod_restartmission 0xc3cb0`, `mod_gameloadmission 0xc36b0`, `mod_gameloadbonus 0xc39b0`, `mod_playendfmv 0xbe480`.
- Game-menu natives (registered `0xc2d40`): `Restart 0xc0770` (stats/mission/rules reset, `sv_restart` = Main7 `0x20df0`, server `+0x1e1c = 1`), `GameLoadLevel 0xbea20`, `GameLoadHunt 0xbeb60`, `GameLoadMission 0xbeca0`, `GameLoadBonus 0xbedf0`, `RestartMission 0xbe6a0`, `Quit 0xbef40` (closegamemenu, sv_stop).
- Task ammo condition: `0x2f530` (fastcall, global `GameDll+0x186314`) = local player's `GetAmmoCount`; callers `0x97440` and `0x979f0` only.
