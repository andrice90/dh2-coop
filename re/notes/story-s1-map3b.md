# Story S1 — MAP3B play-through (2026-10-16 21:35–21:47)

Pair: `run-coop-local.ps1 -Map MAP3B`. Host pid 16668 (`game\`), client pid 8932 (`game2\`).
Digest tool: `tools-src\story-digest.ps1` (204 data rows). Screenshots under `re/notes/coop-local/` (gitignored):
`story-s1-map3b-start-host.png`, `story-s1-map3b-start-client.png`, `story-s1-map3b-end-host.png`, `story-s1-map3b-end-client.png`.
User's error shot: `C:\Users\andri\OneDrive\Pictures\Screenshots\dh2_error.png`.

## What the user said

> Overall sync was good. When the host and client had an overlay that paused the game, it would not pause the in game events. When the host had an overlay that paused the game, the animals would stop animating for the clients. If this is not an easy fix to sync in game overlays with pauses, I am OK with this being a little unsynced. But if you can find a way to sync in game overlays where it pauses for all clients when the host is in an overlay, and then does not pause when clients are in an overlay, that would probably be the best solution. The snake enemies specifically were not synced their status, I killed them as the client and the host and saw one per game instance, not synced across both. I played through to the end of the mission and hit the cutscene for the next mission, then the host entered the next mission but the client is stuck in the loading screen and an enormous error message popped up.

## What the windows showed at the end

- Host: in the forest (MAP2A). Overlay `role host`, `animals 7`, `no remotes`, pistol. Position about `(-148, 39, 145)`.
- Client: stuck on the title card `LOADING...`. The user's screenshot is the engine log filling the screen, including the access violation below.

## Timeline (from the logs)

1. **21:35:20** Both load MAP3B. Before join, each side arms `cScrTask01` and fires it, then arms `cScrTask02`, `cScrTask06`, `cScrTask07`. Client `coop: welcomed as player 1`. Client `animals: client received 2`. Tracker: exactly 2 entities with animal vt `GAMEDLL7.DLL+0xff470` (ids 47 and 48, hp 150, parked at -10000).
2. **21:36:38** `cScrTask02` fires, arms `cScrTask03`, which fires and arms `cScrTask04`. Client applies each within a few milliseconds. The client's own fire of task 02 is a request the host also runs.
3. **21:36:43** Client fires `cScrTask06` four times. Host commits it once (`seq=6`, actor 1) and the `[host] trigger FIRE` line appears once, but the host also logs `fired by player 1` three more times with no extra commit. Client applies the fire four times.
4. **21:37:02** Same pattern for `cScrTask07` (three client requests, one commit, three `fired by player 1` lines).
5. **21:37:24 / 21:37:45** Host commits animal death id 47, then 48. Client applies both. These are the two leopards.
6. **21:37:58** Host mission message code 6, text `Mission failed` / `Your hunt has ended!` (caller `GAMEDLL7.DLL+0x96636`). Client applies it. No `CompleteTask` line anywhere in this session.
7. **21:38:05** Same-map replay. Host `cSpServer::StartGame('MAP3B') -> 1 (bsp reset, same map)`. Client `following host to 'MAP3B' (serial 2, here 'MAP3B'): spmap MAP3B -> 1`. Both sides then re-arm tasks 01/02/06/07 and play the map again. Leopards die again (ids 47 at 21:38:28, 48 at 21:38:33).
8. **21:40:33** Host fires `cScrTask04`, arms `cScrTask05`. Client applies both.
9. **21:41:47** Client fires `cScrTask05` (three requests, one commit). Host also arms `cScrTask06` again with the log suffix `(host commit)`.
10. **21:42:10–21:43:00** Both heartbeats drop to `avg_dt≈0.033` (about 30 fps). Hunter positions stop changing. This is the stretch the user described as the next-mission cutscene.
11. **21:43:03** Host loads `Bsp\map2a.bsp` (63 ms) with no mission-success message. It arms MAP2A `cScrTask01` id 117 and commits `kMapStart` key `MAP2A`. Client applies the task-01 arm first: `story: trigger commit id=117 'cScrTask01': no such trigger here`, then `console::Exec('spmap MAP2A') -> 1`.
12. **21:43:07** Client dies during that teardown. Host continues into MAP2A (174 entities, 7× animal vt `+0xff470`) and at 21:43:27 fires `cScrTask01` and arms `cScrTask02/08/12/13`. Client log stops at the exception. Overlay on the host: `no remotes`.

## Crash text

Client `game2\Bin\debug.log` (the line is one long stack; this is the function chain):

```
Access violation while 'writting to' address 0x10 at address 0x10382e7
caught in function cSndDrvDS::WaveRelease<-cSndDrvDS::ReleaseSound<-cSource::~cSource<-cSndManager::DeleteSource<-cSoundSource::Unload<-cSoundSource::~cSoundSource<-cObjHunter::Destroy<-cObjEntity::InternalRelease<-cModelPart::Destroy<-cModelMesh::~cModelMesh<-cModelMesh::Destroy<-cModelDef::RemoveInstance<-cModelManager::RemoveModel<-cObjBaseEntity::Release<-cObjScriptedEntity::Release<-cObjManager::RemoveEntity1<-cObjManager::RemoveEntity2<-cSpServer::RemoveEntity<-cObjManager::DeleteAllObjects<-cSpServer::DeleteAllObjects<-cSpServer::StopServer<-cSpServer::ConSpMap<-cConsoleFunc::CallFunction<-cConsole::RealExec<-cConsole::Update<-cGameApp::UpdateInput<-cGameManagerSp::Update<-cGameApp::HostFrame<-cGameApp::MainLoop<-cApplication::Run<-Main
```

Matching mod log: `first-chance exception 0xc0000005 at EngineDll7.dll+0x3882e7`.

Earlier first-chance `0xc0000005` at `nvgpucomp32.dll+0x107554` appears on both sides several times (map loads and the replay). Host `debug.log` attributes those to `cModelPart::SetSyncWith<-cPartHumanTorso::SetSyncWith`. The host survived them and is still in MAP2A. They are not the crash that stuck the client.

## Divergences

### Pause overlays

What: an in-game overlay that pauses the local game does not pause the other side's events. A host pause stops animal animation on the client. A client pause does not stop the host.

When: user report for the whole play-through. The mod log does not name the menu. `cHUDSlowMotion::Enable` was blocked five times on each side (`slowmo: cHUDSlowMotion::Enable blocked`), which is the existing co-op policy, not a pause menu.

Hypothesis UNVERIFIED for the exact menu call: the host sim is the source of animal snapshots, so a host modal that stops the host frame also stops new animal poses, and the client copies that freeze. A client modal does not stop the host, so commits keep arriving. There is no pause message in the mod. Doing "host pause pauses everyone, client pause pauses nobody" means detecting menu-open and adding a message. The frame hook does not run while the game thread is inside the modal, so this is not a small change. Left for the lead. The user accepts the desync.

### Snakes

What: each window had its own snake. Killing one did not kill the other.

When: user report during MAP3B. The animal mirror never listed more than ids 47 and 48.

Hypothesis UNVERIFIED for the snake vtable: those two ids are the leopards (`cScrAnimal`, vt `+0xff470`), and their deaths synced (kind=1 commits above). `cObjSnake` and `cObjCobra` are different classes in the class catalog. The mirror does not follow them, so each process simulates its own.

### Mission fail, then same-map replay

What: both sides got `Mission failed`. The follow-up was `spmap MAP3B` while MAP3B was already loaded (`bsp reset, same map`).

That replay is the known setup for the next teardown crash. The MAP2A load was that next teardown.

### Success path is not the mission message

What: the second attempt reached the next mission with no `events: host mission message` and no `story: CompleteTask`. The host loaded `map2a.bsp` itself. The client followed `kMapStart` and crashed.

Hypothesis UNVERIFIED: a cutscene or script calls the map change directly, so `kMissionEnd` never runs on a MAP3B success. The results-screen path in `lead-story-coop.md` is not what this play hit.

### New-map trigger applied before the map exists

What: client log at 21:43:03.869 applies `cScrTask01` id 117, warns `no such trigger here`, then runs `spmap MAP2A`.

Hypothesis UNVERIFIED: the host commits the new map's first trigger arm in the same burst as `kMapStart`, and the client handles the arm while it is still on MAP3B.

### Repeated fires

What: one client entry into `cScrTask06` / `07` / `05` produced several `client fire ... sent to host` lines and several host `fired by player 1` lines, with a single commit.

Hypothesis UNVERIFIED: the client's fire hook runs, the host's replay fires the trigger, and that replay trips the hook again. The script may have run more than once.

## Digest

`kind=apply` on an `events: commit kind=1` line is an animal death (AnimalDied). Mission text is split because the second line of the message has no timestamp; the full text is `Mission failed` / `Your hunt has ended!`.

```
time | role | kind | detail
21:35:18.998 | host | mission | events: registry AnimalDied=1 MissionEnd=2 TrophyClaim=3 MapStart=4
21:35:19.911 | host | mission | hook cMission message (0x91c20): installed at GAMEDLL7.DLL+0x91c20 (0xd1b1c20)
21:35:19.911 | host | mission | hook cMission::CheckMissionStatus (0x94e50): installed at GAMEDLL7.DLL+0x94e50 (0xd1b4e50)
21:35:19.911 | host | mission | hook cMission::CompleteTask (0x95840): installed at GAMEDLL7.DLL+0x95840 (0xd1b5840)
21:35:20.033 | host | apply | events: journal cleared for map 'MAP3B' (serial 1)
21:35:20.052 | host | arm | story: [solo] trigger event activate id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:20.176 | host | fire | story: [solo] trigger FIRE id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:35:20.176 | host | arm | story: [solo] trigger event activate id=44 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:20.176 | host | arm | story: [solo] trigger event activate id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:20.176 | host | arm | story: [solo] trigger event activate id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:20.268 | host | mission | events: commit kind=4 subject=1 actor=0 i0=0 key='MAP3B' seq=1 sent=true journal=1
21:35:27.481 | client | mission | events: registry AnimalDied=1 MissionEnd=2 TrophyClaim=3 MapStart=4
21:35:28.352 | client | mission | hook cMission message (0x91c20): installed at GAMEDLL7.DLL+0x91c20 (0xd741c20)
21:35:28.352 | client | mission | hook cMission::CheckMissionStatus (0x94e50): installed at GAMEDLL7.DLL+0x94e50 (0xd744e50)
21:35:28.352 | client | mission | hook cMission::CompleteTask (0x95840): installed at GAMEDLL7.DLL+0x95840 (0xd745840)
21:35:28.439 | client | apply | events: journal cleared for map 'MAP3B' (serial 1)
21:35:28.457 | client | arm | story: [solo] trigger event activate id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:28.584 | client | fire | story: [solo] trigger FIRE id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:35:28.584 | client | arm | story: [solo] trigger event activate id=44 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:28.584 | client | arm | story: [solo] trigger event activate id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:28.584 | client | arm | story: [solo] trigger event activate id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:35:28.718 | host | apply | events: replayed 1 commits
21:35:28.735 | client | mission | events: apply commit kind=4 subject=1 actor=0 i0=0 key='MAP3B'
21:35:28.735 | client | mission | events: host map 'MAP3B' (serial 1) is already loaded here
21:36:38.132 | host | fire | story: [host] trigger FIRE id=44 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:38.132 | host | arm | story: [host] trigger event activate id=40 name='cScrTask03' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:36:38.132 | host | arm | events: commit kind=5 subject=40 actor=0 i0=0 key='cScrTask03' seq=2 sent=true journal=2
21:36:38.132 | host | fire | events: commit kind=6 subject=44 actor=0 i0=0 key='cScrTask02' seq=3 sent=true journal=3
21:36:38.134 | client | fire | story: [client] trigger FIRE id=44 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:38.134 | client | request | events: request kind=6 subject=44 key='cScrTask02' sent=true
21:36:38.134 | client | request | story: client fire 'cScrTask02' (id 44) sent to host
21:36:38.136 | client | arm | events: apply commit kind=5 subject=40 actor=0 i0=0 key='cScrTask03'
21:36:38.136 | client | arm | story: applied host trigger 'cScrTask03' (id 40)
21:36:38.136 | client | fire | events: apply commit kind=6 subject=44 actor=0 i0=0 key='cScrTask02'
21:36:38.136 | client | fire | story: applied host fire 'cScrTask02' (id 44)
21:36:38.150 | host | fire | story: trigger 'cScrTask02' (id 44) fired by player 1
21:36:38.162 | host | fire | story: [host] trigger FIRE id=40 name='cScrTask03' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:38.162 | host | arm | story: [host] trigger event activate id=41 name='cScrTask04' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:36:38.162 | host | arm | events: commit kind=5 subject=41 actor=0 i0=0 key='cScrTask04' seq=4 sent=true journal=4
21:36:38.163 | host | fire | events: commit kind=6 subject=40 actor=0 i0=0 key='cScrTask03' seq=5 sent=true journal=5
21:36:38.164 | client | fire | events: apply commit kind=6 subject=44 actor=0 i0=0 key='cScrTask02'
21:36:38.166 | client | arm | events: apply commit kind=5 subject=41 actor=0 i0=0 key='cScrTask04'
21:36:38.166 | client | arm | story: applied host trigger 'cScrTask04' (id 41)
21:36:38.166 | client | fire | events: apply commit kind=6 subject=40 actor=0 i0=0 key='cScrTask03'
21:36:38.166 | client | fire | story: applied host fire 'cScrTask03' (id 40)
21:36:43.673 | client | fire | story: [client] trigger FIRE id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:43.673 | client | request | events: request kind=6 subject=46 key='cScrTask06' sent=true
21:36:43.673 | client | request | story: client fire 'cScrTask06' (id 46) sent to host
21:36:43.676 | host | fire | story: trigger 'cScrTask06' (id 46) fired by player 1
21:36:43.676 | host | fire | events: commit kind=6 subject=46 actor=1 i0=0 key='cScrTask06' seq=6 sent=true journal=6
21:36:43.683 | client | fire | story: [client] trigger FIRE id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:43.683 | client | request | events: request kind=6 subject=46 key='cScrTask06' sent=true
21:36:43.683 | client | request | story: client fire 'cScrTask06' (id 46) sent to host
21:36:43.685 | host | fire | story: [host] trigger FIRE id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:43.693 | client | fire | story: [client] trigger FIRE id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:43.693 | client | request | events: request kind=6 subject=46 key='cScrTask06' sent=true
21:36:43.693 | client | request | story: client fire 'cScrTask06' (id 46) sent to host
21:36:43.700 | host | fire | story: trigger 'cScrTask06' (id 46) fired by player 1
21:36:43.700 | host | fire | story: trigger 'cScrTask06' (id 46) fired by player 1
21:36:43.703 | client | fire | story: [client] trigger FIRE id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:36:43.703 | client | request | events: request kind=6 subject=46 key='cScrTask06' sent=true
21:36:43.703 | client | request | story: client fire 'cScrTask06' (id 46) sent to host
21:36:43.705 | client | fire | events: apply commit kind=6 subject=46 actor=1 i0=0 key='cScrTask06'
21:36:43.712 | host | fire | story: trigger 'cScrTask06' (id 46) fired by player 1
21:36:43.716 | client | fire | story: applied host fire 'cScrTask06' (id 46)
21:36:43.716 | client | fire | events: apply commit kind=6 subject=46 actor=1 i0=0 key='cScrTask06'
21:36:43.716 | client | fire | events: apply commit kind=6 subject=46 actor=1 i0=0 key='cScrTask06'
21:36:43.716 | client | fire | events: apply commit kind=6 subject=46 actor=1 i0=0 key='cScrTask06'
21:36:43.722 | client | fire | events: apply commit kind=6 subject=46 actor=1 i0=0 key='cScrTask06'
21:37:02.614 | client | fire | story: [client] trigger FIRE id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:37:02.614 | client | request | events: request kind=6 subject=45 key='cScrTask07' sent=true
21:37:02.614 | client | request | story: client fire 'cScrTask07' (id 45) sent to host
21:37:02.624 | client | fire | story: [client] trigger FIRE id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:37:02.624 | client | request | events: request kind=6 subject=45 key='cScrTask07' sent=true
21:37:02.624 | client | request | story: client fire 'cScrTask07' (id 45) sent to host
21:37:02.626 | host | fire | story: trigger 'cScrTask07' (id 45) fired by player 1
21:37:02.626 | host | fire | events: commit kind=6 subject=45 actor=1 i0=0 key='cScrTask07' seq=7 sent=true journal=7
21:37:02.628 | host | fire | story: trigger 'cScrTask07' (id 45) fired by player 1
21:37:02.634 | client | fire | story: [client] trigger FIRE id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:37:02.634 | client | request | events: request kind=6 subject=45 key='cScrTask07' sent=true
21:37:02.634 | client | request | story: client fire 'cScrTask07' (id 45) sent to host
21:37:02.635 | client | fire | events: apply commit kind=6 subject=45 actor=1 i0=0 key='cScrTask07'
21:37:02.637 | host | fire | story: trigger 'cScrTask07' (id 45) fired by player 1
21:37:02.646 | client | fire | story: applied host fire 'cScrTask07' (id 45)
21:37:02.647 | client | fire | events: apply commit kind=6 subject=45 actor=1 i0=0 key='cScrTask07'
21:37:02.657 | client | fire | events: apply commit kind=6 subject=45 actor=1 i0=0 key='cScrTask07'
21:37:24.645 | host | apply | events: commit kind=1 subject=47 actor=0 i0=0 key='' seq=8 sent=true journal=8
21:37:24.664 | client | apply | events: apply commit kind=1 subject=47 actor=0 i0=0 key=''
21:37:45.235 | host | apply | events: commit kind=1 subject=48 actor=0 i0=0 key='' seq=9 sent=true journal=9
21:37:45.247 | client | apply | events: apply commit kind=1 subject=48 actor=0 i0=0 key=''
21:37:58.477 | host | mission | events: host mission message code=6 text='Mission failed
21:37:58.477 | host | mission | events: commit kind=2 subject=0 actor=0 i0=6 key='Mission failed
21:37:58.483 | client | mission | events: apply commit kind=2 subject=0 actor=0 i0=6 key='Mission failed
21:37:58.483 | client | mission | events: applied host mission end code=6 text='Mission failed
21:38:05.340 | host | arm | story: [host] trigger event activate id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:38:05.340 | host | arm | events: commit kind=5 subject=39 actor=0 i0=0 key='cScrTask01' seq=11 sent=true journal=11
21:38:05.355 | host | apply | events: journal cleared for map 'MAP3B' (serial 2)
21:38:05.438 | host | mission | events: commit kind=4 subject=2 actor=0 i0=0 key='MAP3B' seq=1 sent=true journal=1
21:38:05.442 | client | arm | events: apply commit kind=5 subject=39 actor=0 i0=0 key='cScrTask01'
21:38:05.452 | client | mission | events: apply commit kind=4 subject=2 actor=0 i0=0 key='MAP3B'
21:38:05.452 | client | mission | events: following host to 'MAP3B' (serial 2, here 'MAP3B'): spmap MAP3B -> 1
21:38:05.468 | client | exception | first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554
21:38:05.477 | host | fire | story: [host] trigger FIRE id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:38:05.477 | host | arm | story: [host] trigger event activate id=44 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:38:05.477 | host | arm | events: commit kind=5 subject=44 actor=0 i0=0 key='cScrTask02' seq=2 sent=true journal=2
21:38:05.477 | host | arm | story: [host] trigger event activate id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:38:05.477 | host | arm | events: commit kind=5 subject=46 actor=0 i0=0 key='cScrTask06' seq=3 sent=true journal=3
21:38:05.477 | host | arm | story: [host] trigger event activate id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:38:05.477 | host | arm | events: commit kind=5 subject=45 actor=0 i0=0 key='cScrTask07' seq=4 sent=true journal=4
21:38:05.477 | host | fire | events: commit kind=6 subject=39 actor=0 i0=0 key='cScrTask01' seq=5 sent=true journal=5
21:38:05.485 | client | arm | story: [client] trigger event activate id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:38:05.485 | client | request | events: request kind=5 subject=39 key='cScrTask01' sent=true
21:38:05.485 | client | request | story: client trigger 'cScrTask01' (id 39) sent to host
21:38:05.541 | client | apply | events: journal cleared for map 'MAP3B' (serial 2)
21:38:05.574 | client | arm | events: apply commit kind=5 subject=44 actor=0 i0=0 key='cScrTask02'
21:38:05.574 | client | arm | story: applied host trigger 'cScrTask02' (id 44)
21:38:05.574 | client | arm | events: apply commit kind=5 subject=46 actor=0 i0=0 key='cScrTask06'
21:38:05.574 | client | arm | story: applied host trigger 'cScrTask06' (id 46)
21:38:05.574 | client | arm | events: apply commit kind=5 subject=45 actor=0 i0=0 key='cScrTask07'
21:38:05.574 | client | arm | story: applied host trigger 'cScrTask07' (id 45)
21:38:05.574 | client | fire | events: apply commit kind=6 subject=39 actor=0 i0=0 key='cScrTask01'
21:38:05.574 | client | fire | story: applied host fire 'cScrTask01' (id 39)
21:38:05.578 | host | arm | story: trigger 'cScrTask01' (id 39) requested by player 1 -> active
21:38:05.578 | host | arm | events: commit kind=5 subject=39 actor=1 i0=0 key='cScrTask01' seq=6 sent=true journal=6
21:38:05.612 | client | arm | events: apply commit kind=5 subject=39 actor=1 i0=0 key='cScrTask01'
21:38:05.681 | client | exception | first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554
21:38:05.748 | host | exception | first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554
21:38:09.097 | host | fire | story: [host] trigger FIRE id=44 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:38:09.097 | host | arm | story: [host] trigger event activate id=40 name='cScrTask03' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:38:09.097 | host | arm | events: commit kind=5 subject=40 actor=0 i0=0 key='cScrTask03' seq=7 sent=true journal=7
21:38:09.098 | host | fire | events: commit kind=6 subject=44 actor=0 i0=0 key='cScrTask02' seq=8 sent=true journal=8
21:38:09.104 | client | arm | events: apply commit kind=5 subject=40 actor=0 i0=0 key='cScrTask03'
21:38:09.104 | client | arm | story: applied host trigger 'cScrTask03' (id 40)
21:38:09.104 | client | fire | events: apply commit kind=6 subject=44 actor=0 i0=0 key='cScrTask02'
21:38:09.104 | client | fire | story: applied host fire 'cScrTask02' (id 44)
21:38:09.127 | host | fire | story: [host] trigger FIRE id=40 name='cScrTask03' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:38:09.127 | host | arm | story: [host] trigger event activate id=41 name='cScrTask04' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:38:09.127 | host | arm | events: commit kind=5 subject=41 actor=0 i0=0 key='cScrTask04' seq=9 sent=true journal=9
21:38:09.127 | host | fire | events: commit kind=6 subject=40 actor=0 i0=0 key='cScrTask03' seq=10 sent=true journal=10
21:38:09.135 | client | arm | events: apply commit kind=5 subject=41 actor=0 i0=0 key='cScrTask04'
21:38:09.135 | client | arm | story: applied host trigger 'cScrTask04' (id 41)
21:38:09.135 | client | fire | events: apply commit kind=6 subject=40 actor=0 i0=0 key='cScrTask03'
21:38:09.135 | client | fire | story: applied host fire 'cScrTask03' (id 40)
21:38:12.841 | host | fire | story: [host] trigger FIRE id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:38:12.852 | host | fire | events: commit kind=6 subject=46 actor=0 i0=0 key='cScrTask06' seq=11 sent=true journal=11
21:38:12.859 | client | fire | events: apply commit kind=6 subject=46 actor=0 i0=0 key='cScrTask06'
21:38:12.870 | client | fire | story: applied host fire 'cScrTask06' (id 46)
21:38:28.376 | host | apply | events: commit kind=1 subject=47 actor=0 i0=0 key='' seq=12 sent=true journal=12
21:38:28.388 | client | apply | events: apply commit kind=1 subject=47 actor=0 i0=0 key=''
21:38:33.564 | host | apply | events: commit kind=1 subject=48 actor=0 i0=0 key='' seq=13 sent=true journal=13
21:38:33.578 | client | apply | events: apply commit kind=1 subject=48 actor=0 i0=0 key=''
21:38:47.938 | host | fire | story: [host] trigger FIRE id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:38:47.938 | host | fire | events: commit kind=6 subject=45 actor=0 i0=0 key='cScrTask07' seq=14 sent=true journal=14
21:38:47.946 | client | fire | events: apply commit kind=6 subject=45 actor=0 i0=0 key='cScrTask07'
21:38:47.946 | client | fire | story: applied host fire 'cScrTask07' (id 45)
21:40:17.767 | client | exception | first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554
21:40:33.286 | host | fire | story: [host] trigger FIRE id=41 name='cScrTask04' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:40:33.286 | host | arm | story: [host] trigger event activate id=42 name='cScrTask05' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:40:33.286 | host | arm | events: commit kind=5 subject=42 actor=0 i0=0 key='cScrTask05' seq=15 sent=true journal=15
21:40:33.286 | host | fire | events: commit kind=6 subject=41 actor=0 i0=0 key='cScrTask04' seq=16 sent=true journal=16
21:40:33.297 | client | arm | events: apply commit kind=5 subject=42 actor=0 i0=0 key='cScrTask05'
21:40:33.297 | client | arm | story: applied host trigger 'cScrTask05' (id 42)
21:40:33.297 | client | fire | events: apply commit kind=6 subject=41 actor=0 i0=0 key='cScrTask04'
21:40:33.297 | client | fire | story: applied host fire 'cScrTask04' (id 41)
21:41:47.803 | client | fire | story: [client] trigger FIRE id=42 name='cScrTask05' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:41:47.803 | client | request | events: request kind=6 subject=42 key='cScrTask05' sent=true
21:41:47.803 | client | request | story: client fire 'cScrTask05' (id 42) sent to host
21:41:47.812 | host | arm | story: [host] trigger event activate id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7 (host commit)
21:41:47.812 | host | fire | story: trigger 'cScrTask05' (id 42) fired by player 1
21:41:47.812 | host | fire | events: commit kind=6 subject=42 actor=1 i0=0 key='cScrTask05' seq=17 sent=true journal=17
21:41:47.813 | client | fire | story: [client] trigger FIRE id=42 name='cScrTask05' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:41:47.813 | client | request | events: request kind=6 subject=42 key='cScrTask05' sent=true
21:41:47.813 | client | request | story: client fire 'cScrTask05' (id 42) sent to host
21:41:47.822 | host | fire | story: trigger 'cScrTask05' (id 42) fired by player 1
21:41:47.823 | client | fire | story: [client] trigger FIRE id=42 name='cScrTask05' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:41:47.823 | client | request | events: request kind=6 subject=42 key='cScrTask05' sent=true
21:41:47.823 | client | request | story: client fire 'cScrTask05' (id 42) sent to host
21:41:47.824 | client | fire | events: apply commit kind=6 subject=42 actor=1 i0=0 key='cScrTask05'
21:41:47.824 | client | fire | story: applied host fire 'cScrTask05' (id 42)
21:41:47.832 | host | fire | story: trigger 'cScrTask05' (id 42) fired by player 1
21:41:47.834 | client | fire | events: apply commit kind=6 subject=42 actor=1 i0=0 key='cScrTask05'
21:41:47.844 | client | fire | events: apply commit kind=6 subject=42 actor=1 i0=0 key='cScrTask05'
21:43:03.514 | host | exception | first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554
21:43:03.598 | host | apply | events: journal cleared for map 'MAP2A' (serial 3)
21:43:03.663 | host | arm | story: [host] trigger event activate id=117 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:43:03.663 | host | arm | events: commit kind=5 subject=117 actor=0 i0=0 key='cScrTask01' seq=1 sent=true journal=1
21:43:03.838 | host | mission | events: commit kind=4 subject=3 actor=0 i0=0 key='MAP2A' seq=2 sent=true journal=2
21:43:03.869 | client | arm | events: apply commit kind=5 subject=117 actor=0 i0=0 key='cScrTask01'
21:43:03.869 | client | arm | story: trigger commit id=117 'cScrTask01': no such trigger here
21:43:03.869 | client | mission | events: apply commit kind=4 subject=3 actor=0 i0=0 key='MAP2A'
21:43:03.869 | client | mission | events: following host to 'MAP2A' (serial 3, here 'MAP3B'): spmap MAP2A -> 1
21:43:07.381 | client | exception | first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554
21:43:07.384 | client | exception | first-chance exception 0xc0000005 at EngineDll7.dll+0x3882e7 eip=0x10382e7
21:43:13.076 | host | exception | first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554
21:43:27.945 | host | fire | story: [host] trigger FIRE id=117 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 activator=id=172 name='hunter' vt=GAMEDLL7.DLL+0x106ff0
21:43:27.945 | host | arm | story: [host] trigger event activate id=118 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:43:27.945 | host | arm | events: commit kind=5 subject=118 actor=0 i0=0 key='cScrTask02' seq=3 sent=true journal=3
21:43:27.945 | host | arm | story: [host] trigger event activate id=123 name='cScrTask08' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:43:27.945 | host | arm | events: commit kind=5 subject=123 actor=0 i0=0 key='cScrTask08' seq=4 sent=true journal=4
21:43:27.945 | host | arm | story: [host] trigger event activate id=150 name='cScrTask12' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:43:27.945 | host | arm | events: commit kind=5 subject=150 actor=0 i0=0 key='cScrTask12' seq=5 sent=true journal=5
21:43:27.945 | host | arm | story: [host] trigger event activate id=151 name='cScrTask13' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7
21:43:27.945 | host | arm | events: commit kind=5 subject=151 actor=0 i0=0 key='cScrTask13' seq=6 sent=true journal=6
21:43:27.946 | host | fire | events: commit kind=6 subject=117 actor=0 i0=0 key='cScrTask01' seq=7 sent=true journal=7
rows=204
```

## Smoke test

Not run yet. The MAP3B pair is still open (client stuck on LOADING, host in MAP2A).

## Proposed MEMORY facts

- MAP3B's successful exit in this play did not call `cMission`'s message hook. The host loaded `Bsp\map2a.bsp` and then committed `kMapStart` `MAP2A`. Evidence: host `dh2coop.log` 21:43:03 has `cBspTree::Load('Bsp\map2a.bsp'` and `events: commit kind=4 ... key='MAP2A'`, and no `host mission message` between the `cScrTask05` fire and that load.
- A failed MAP3B does call it. Evidence: 21:37:58 `events: host mission message code=6 text='Mission failed` / `Your hunt has ended!'`, client `applied host mission end code=6`.
- The animal mirror on MAP3B followed 2 `cScrAnimal` entities (vt `GAMEDLL7.DLL+0xff470`, ids 47 and 48). Their `kAnimalDied` commits were applied on the client. No other animal id was snapshotted.
- After `spmap MAP3B` while MAP3B was loaded, the next `spmap MAP2A` on the client crashed in `cSndDrvDS::WaveRelease` inside `cSpServer::StopServer` (`EngineDll7.dll+0x3882e7`). Same stack the known same-map-replay bug names.
