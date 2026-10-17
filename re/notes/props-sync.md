# Shootable props sync (`cObjTarget`)

**Progress:** done. MAP1 two-window tests and a late join ran. The broken mesh was not in frame (see Screenshots).

## What changed

`kPropHit` (16) is a journaled request and commit. Subject is the target entity id. `i0` is 1 for a stump (`+0x21c == 1`) and 0 for a bottle.

`mod/src/coop/props.cpp` hooks `cObjTarget::OnHit` (GameDll `0x277f0`) after the `55 8b ec 6a ff` prologue check. Offline, or while a remote break is being applied, the original runs. The host lets the original run and commits only when `+0x208` goes from 0 to 1. A client whose `other` is a `mod_obj_bullet` does not run the original; it requests once per id until the commit arrives.

The host's request handler resolves the id, requires the `cObjTarget` vtable (`0x1017c8`), and if `+0x208` is 0 calls `Boom` (`0x27040`), sets `+0x208 = 1`, counts a bottle with `UpdateMissionStatus(1, 0x25)` when `DAT_101657f0` is set, and adds 1 to the stump tally. It then commits with the requester as actor. A client commit does the Boom and the flag, and the stump tally on its own profile. It never calls `UpdateMissionStatus`.

The stump tally is the same pointer `OnHit` increments: `FUN_10004e20(&DAT_10165888, 0)` (GameDll `0x4e20`, index 0) then `cHunterProfile::GetMissionStatus()`, field `+0x90`. `re/notes/props-target.c`.

`coop_prophit <id>` is the client request path on a client and the host break path on the host. Map load logs every `cObjTarget` once (id, position, bottle or stump) and clears the pending set.

## Tests

MAP1, `run-coop-local.ps1 -Map MAP1` in this worktree. The same five targets on the host and the client:

```
props: target id=83 pos=(-66.5,60.4,7.8) bottle
props: target id=84 pos=(-70.8,60.7,8.2) bottle
props: target id=85 pos=(-65.9,60.5,8.4) bottle
props: target id=86 pos=(-71.1,60.0,1.1) bottle
props: target id=87 pos=(-46.4,32.7,-170.5) stump
```

Game3 logged the same five lines at 16:07:07.

**Client `coop_prophit 83` (16:02:49).** Host:

```
props: break id=83 stump=0 boom=true flag=true via=request
props: UpdateMissionStatus(1, 0x25) id=83
events: commit kind=16 subject=83 actor=1 i0=0 key='' seq=9 sent=true journal=9
```

Client:

```
props: coop_prophit id=83 stump=0
events: request kind=16 subject=83 key='' sent=true
events: apply commit kind=16 subject=83 actor=1 i0=0 key=''
props: break id=83 stump=0 boom=true flag=true via=commit
```

The client log had no `UpdateMissionStatus` line.

**Real client bullet on 85 (16:03:08),** not a console command. Host `UpdateMissionStatus(1, 0x25) id=85` and `events: commit kind=16 subject=85`. Client `props: break id=85 ... via=commit` and no mission-count line.

**Client `coop_prophit 87` (stump, 16:06:34).** Host `props: stump tally id=87 0 -> 1` and `events: commit kind=16 subject=87 actor=1 i0=1`. Client `props: stump tally id=87 0 -> 1` after `via=commit`. No mission count on the client.

**Host `coop_prophit 84` (16:08:17), fresh pair.** Host:

```
props: break id=84 stump=0 boom=true flag=true via=coop_prophit
props: UpdateMissionStatus(1, 0x25) id=84
events: commit kind=16 subject=84 actor=0 i0=0 key='' seq=3 sent=true journal=3
```

Client: `events: apply commit kind=16 subject=84` then `props: break id=84 stump=0 boom=true flag=true via=commit`. No `UpdateMissionStatus` on the client.

**Repeat.** Two seconds later the host ran `coop_prophit 84` again: `props: coop_prophit id=84 already broken`. The client log has one apply for 84.

**Late join.** `run-coop-local.ps1 -AddClient 3 -Map MAP1`. Game3 during catch-up (`game3/Bin/dh2coop.log`):

```
events: apply commit kind=16 subject=83 actor=1 i0=0 key=''
props: break id=83 stump=0 boom=true flag=true via=commit
events: apply commit kind=16 subject=85 actor=1 i0=0 key=''
props: break id=85 stump=0 boom=true flag=true via=commit
events: apply commit kind=16 subject=87 actor=1 i0=1 key=''
props: break id=87 stump=1 boom=true flag=true via=commit
props: stump tally id=87 0 -> 1
events: synced with host on 'MAP1' (13 commits replayed)
```

No `UpdateMissionStatus` on game3. 84 and 86 were not in the journal, so they were not broken.

### Screenshots

`re/notes/coop-local/props-bottle83-host.png`, `props-bottle83-client.png`, `props-bottle84-host.png`, `props-bottle84-client.png`, `props-latejoin-game3.png`.

`coop_tp` logged the requested point, and the next tracker sample was still somewhere else. After `coop_tp: hunter to (-68.0,60.7,8.2)` the client tracker read `hunter pos=(-57.62,58.78,-2.18)`. The shots show the two sessions in the forest. They do not show the bottle mesh. Whether the mesh hid and the break FX played is UNVERIFIED from the pictures. `Boom` returned true and `+0x208` was set in the logs above.

The first session's `dh2coop.log` was replaced when the pair was relaunched. The 16:02–16:06 lines above were read from that log while it was open. Game3's log and the 16:08 host/client logs are still on disk.

## Proposed MEMORY facts

- MAP1 `cObjTarget` ids match across processes: 83, 84, 85, 86 bottles and 87 stump, at the positions in the log lines above. Evidence: host, client, and game3 `props: target` lines at map load.
- `cObjTarget::OnHit` at GameDll `0x277f0` has prologue `55 8b ec 6a ff` on this build and is the bullet entry. Evidence: `hook cObjTarget::OnHit (0x277f0): installed at GAMEDLL7.DLL+0x277f0` in the client log, and a real `Fire` shot (client `tracker: action weapon state 'Fire'`) produced `props: client hit id=84`.
- The local stump tally OnHit uses is profile index 0 in the table at GameDll `0x165888`, then `GetMissionStatus()` `+0x90`. Evidence: both the host and the client logged `props: stump tally id=87 0 -> 1` from that write, and `props-target.c` shows `FUN_10004e20(&DAT_10165888,0)` plus `GetMissionStatus` at the `+0x90` increment.
- A bottle count is host-only `UpdateMissionStatus(1, 0x25)` and it ran once per break (`id=83`, `id=85`, `id=84`). Clients applied `Boom` with no such line.

## Open questions

- The mission task wants four cans. Only two host counts were observed. Whether the fourth count completes the task was not played.
- After the client started refiring `cScrTask01` every frame, `EntityById` stopped returning those `cObjTarget`s (`coop_prophit 86 is not a cObjTarget` on the host, then the stump too). A fresh pair resolved the same ids. Cause UNVERIFIED.
- `coop_tp` does not leave the hunter at the logged coordinates, so a screenshot next to a broken bottle is still open.
