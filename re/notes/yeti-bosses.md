# Boss fights and scripted beats (bosses agent, branch `yeti`)

Scope: MAP12C yeti (sync attacks, throws), MAP12B Big Grimm (falling trees), MAP11B Hogzilla, MAP12A Reggie / Wirake
knockdown beats, MAP2B Hugh `hurtdown` / `syncguidegetup`. Code: `mod/src/coop/bosses.cpp` (+ `.h`), the sync-attack parts of
`animals.cpp` and `combat.cpp`, `entity_api` helpers. Test script: `tools-src/boss-test.ps1`. Scratch decompiles (not
for commit reading): `re/notes/yeti-*.c|txt`. Event kinds 34-37 (`events.h`) belong to this work; protocol message id 14 is
reserved and still unused (everything rides the event journal). Addresses are module-relative (GameDll7 / CommonDll7 /
EngineDll7 v1.0).

## 1. The MAP12C client crash (root cause, fixed in `f9572eb`)

A sync attack (`PlaySA`, `cAIHuntTask_PlaySA::Begin` CommonDll7 `0xd19d0`, attack task `TryPlaySA` `0xcadf0`) ties the
victim's model parts to the attacker's part 0: victim part 0 `SetSyncWith(animal part 0)`, victim part 1
`SetSyncWith(victim part 0)`. `cModelPart` layout: `+0xf4` first follower, `+0x100` the part this one follows, `+0x10c` next
follower; each link is a `cModelPartLink {vt, owner entity, part index}`. `cModelPart::SetAnim(int)` (EngineDll7 `0x1e73b0`)
replays every clip on all followers and raises "Could not forward animation" when a follower cannot play it. Only the
task's End (task vt `+0x14`) undoes the links and runs `OnEndSA` on both characters.

On the client, the yeti mirror was still inside its own opening-cutscene `AIHuntTask_PlaySA` with the guide. Each frame the
mirror's `SetBodyState` ran `CutScene.End -> SetAnim("idleattack1")`, replayed on the guide's legs, which have no such clip:
732 engine exceptions in 7.5 s. `cVMachine::ExecuteDirect` pushes a `cState` frame and pops only on a normal return, so
each exception leaked VM state until `cVMStack::Push` faulted (reported EngineDll7 `+0x28b337`, real AV `+0x24b12e`) and
the next script call was fatal.

Fix rules (all in `animals.cpp`, client side):
- A mirror never forwards a clip to another character: `ForeignSyncCount` / `DetachForeignSync` (part vt `+0x38`
  `SetSyncWith(nullptr)` on each foreign link) before any state or clip write.
- A cutscene `AIHuntTask_PlaySA` that ran on the client's copy gets its own finish (CommonDll7 `0xcf950`, task `+4` = 2) so the
  waiting cutscene resumes and stops the task itself (StopTask made the opening cutscene wait for ever). Other stale local
  sync attacks end with unlink + the base `OnEndSA` (the virtual faults on the frozen yeti). Thaw only under the local
  cutscene camera (`story::LocalCutsceneCamera`).
- A `SetBodyState` name that faulted is never tried again; after 3 faults the mirror gets positions and health only
  (`failed_states`, `body_disabled`). Main's failing-state limit (3 non-fault failures) stays beside it.
- NPC partners are re-frozen after `OnEndSA` (it clears the character-brain freeze bit, `IsCharacterBrainFrozen`).

Host to client: the animal flag `kSyncAttack` (`1<<4`) plus the clip name in `arms` (`PartClipName(part 0)`). The mirror plays
the clip by name (`SetPartsState("CutScene")` + `PlayClip`) when its model has it and holds the pose otherwise. A scripted
clip in `CutScene` that is not a cutscene `PlayAnim` (the yeti's throw) is sent the same way.

Host side (`combat.cpp`): a puppet hit by a sync attack is unlinked from the attacker before the animal's `PlaySA` (the guide body
has no sync clips: a forward error would be fatal on the host); the client's knockdown falls back to the map's `pl_*` clip
when its hunter lacks the animal's clip (yeti_syncattack1..4 live only in the NPC `MAP12C.AP`). The nearest-player retarget
skips an animal in a throw (attack sub-state `+0x18` = `0x8000`) or sync attack (`0x2000`).

## 2. `bosses.cpp`

### NPC PlayAnim beats (`kNpcAnim` = 34)
`AITaskNPC_PlayAnim::Update` (GameDll7 `0xb5b10`, thiscall(task, dt)): task `+0xa0` clip pending, `+0xb8` the NPC, `+0xc0` clip
name, `+0x100` play_sa flag, `+0x104` partner. When it starts the clip it clears `+0xa0`, holsters (`0x55430(npc,0)`,
`0x54f80(npc)`), sets the NPC state `NoChange` (`0x5de00`) and then plays the clip: paired -> `PlaySA(clip, partner)`
(`story.cpp` `kPlaySA` already commits that), unpaired -> `SetAnim(clip)` (vt `+0x174`) on the active item (`0x552f0`) or the NPC.
- Host: hook on Update; on the tick `+0xa0` goes 1 -> 0 and the clip is unpaired, commit `kNpcAnim` (subject NPC entity id,
  key `<clip>#<n>`, n counts per NPC so the journal keeps repeats).
- Client: `OnNpcAnimCommit` replays the task's calls on its frozen copy (`guarded_npc_clip`; fallback `SetBodyState NoChange` +
  `PlayClip`). `NpcAnimHold(id, hostState)` (called from the NPC mirror in `animals.cpp`) holds the pose and skips the leg-clip
  and body-state writes until the host NPC leaves `NoChange` (3 s grace for samples older than the commit).
- Beats: MAP12A Reggie / Wirake `hurtdown`, MAP2B Hugh `hurtsit` / `hurtdown` / `syncguidegetup`, cutscene `cs03` / `cs04`.

### Boss props: MAP12B falling trees (`kPropSA` = 35, `kPropSAEnd` = 36)
`cObjAIHuntMoveObject` (CommonDll7 vtable `0x1342c0`, BSP `obj_move_object`, `msn12faltree`) is a character. Big Grimm's
`AttackObjectMovers::TryMoveObject` (`0xc9ea0`) builds a basis from the tree's angles (`0x15230`, thiscall(m4x4, a, b, c)) and
position and calls `PlaySA(clip, basis)` on the tree (override `0xb5ff0`) and on the bear. The tree's `OnEndSA` override
(`0xb6130`) moves it to its fallen pose.
- Host: hooks on both; commit `kPropSA` (key = clip, e.g. `faltree`) at the start and `kPropSAEnd` (f0 = seconds since the start).
- Client: `OnPropSACommit` builds the same basis from the local tree and calls the original `PlaySA`; `PumpMovers` runs `OnEndSA` at
  the host's moment in the clip (a late joiner gets both commits at once and lets the clip play first).

### Yeti throws (`kBossThrow` = 37)
`AttackThrowers::ProcessThrow` (CommonDll7 `0xc9740`, fastcall(task)): creates `obj_throw_object` (`rock1.skin`) in the yeti's left
hand, detaches it (task `+0x17c` = 1, rock at `+0x180`), `Throw` (`0xb4c10`: `+0x1f0` target, `+0x1e4` damage, `+0x1e8` impulse)
and an impulse. The rock's physics callback `0xb43b0` (cdecl(rock, other)) hurts the target once on contact (`+0x1dc` armed).
Task `+8 -> +0x68` is the yeti; its brain `+0x254` is the target id.
- Host: hook on ProcessThrow; on the tick the rock is thrown, send (not journaled) `kBossThrow` with position, velocity
  (packed). `PumpRockHits`: an armed rock inside a puppet's 0.7 m x (-0.3..2.0 m) capsule runs the engine's own hit callback, which
  reaches the puppet's `OnHurt` and goes to that player (`kPlayerHurt`), because the puppet body does not collide.
- Client: `OnBossThrowCommit` spawns a harmless `obj_throw_object` (not armed, target itself) with the host's velocity.
- Diagnostic hook on the rock callback logs touches.

### ini and commands
`[Bosses]` `NpcAnims`, `PropSA`, `ThrowCopies`, `ThrowHitsPuppets` (all default 1, documented in `mod/dh2coop.ini`).
Console: `coop_hurtname <name>:<damage>` (hurt an animal by script name, with kill credit on the host/solo; on a client it is a hit on the
mirror and goes to the host as `AnimalHit`). `coop_animalknock <player 1-3>:<clip>|end` (host, combat.cpp): sends that player the
knock-down an animal's sync attack would (`kPlayerKnock` with the animal's clip name), or the stand-up.

## 3. Rebase onto main (7a27fdb) and what changed

- `animals.cpp`: main's MoveToPos re-freeze hook, per-frame wake check, failing-state limit and kill credit stay; the earlier
  `IsHuntBrainFrozen` re-freeze was already removed from the branch. The client thaw rule is now main's
  `beasts::NoCutSceneThaw` (`[Animals] ThawNeedsCamera=1`: thaw for the host's CutScene only while this client's camera is on a
  cutscene rail) plus this branch's two conditions (`m.sa_clip.empty()`, `m.task_finished_ms == 0`: not while the mirror plays the
  host's sync-attack clip or has just finished its own cutscene task). The branch's `story::LocalCutsceneCamera` helper is gone
  (same test as `NoCutSceneThaw`).
- `events.h`: kinds 34-37 kept. `combat.cpp`: the "not in a throw (`0x8000`) or sync attack (`0x2000`)" guard moved into main's new
  load-aware `RetargetToNearestPlayer`.
- `mod\build.ps1` and `mod\net-test\build.ps1` pass (3568 checks).

## 4. Results (two windows, `tools-src/boss-test.ps1`, godMode on both, 2026-10-18)

All runs: `Could not forward animation` 0, access violations 0, both games alive at the end (about 14 runs on the rebased and
the earlier build). Screenshots and copies of both logs: `re/notes/coop-local/bosses-*` (gitignored).

| Map | Verdict | Decisive lines |
|---|---|---|
| MAP12C yeti: client survives, sync attacks, throws | VERIFIED | client `mirror id=24 plays the host's sync attack 'yeti_syncattack1' states=3 parts=3`, `... 'throwup' ...`; `bosses: throw copy by id=24 rock id=29 ... vel=(...)`; 5 runs, 0 forward errors, client alive |
| MAP12C rocks aimed at the client | VERIFIED after a fix | before: 3 of 4 rocks missed (the pump steps 1-2 m per frame at 10-20 fps: `dropped ... closest 0.97 m`). After the swept test: `rock id=31 reached player 1 after 688 ms: hit ok=true` and client `took 30.0 from animal id 31 (type 1)`; 4 of 4 hit |
| MAP12C yeti death, mission end on both | VERIFIED (client kill and host kill) | client `coop_kill 24:5000`: host `mission report action=1 species=25`, `host mission message code=5 text='MISSION SUCCESSFUL!'`, client `applied host mission end code=5 ... ok=true`, `applied host fire 'cScrTask02'`. Host `coop_hurtname yeti:5000`: same. Host `coop_kill` gives `action=0` and no end (test artifact: it never sets `+0x420`, a bullet does) |
| MAP12C opening: client `defer host fire 'cScrTask01'` | VERIFIED short | the defer line is followed within 30-110 ms by `mirror id=24 finished its local cutscene task` and `applied host fire 'cScrTask01'`: normal join, late join 6 s, late join 20 s (`will skip the cutscene of trigger id 22`) |
| MAP12B Big Grimm falling trees | VERIFIED | host `mover id=14 PlaySA 'faltree'`, client `applied prop SA id=14 'faltree' basis=true ok=true`; end pose host (58.2,4.2,-0.1) vs client (58.2,4.3,-0.5); tree 2 (33.2,11.2,-59.7) on both; `bosses-12b-b-152544-p0-3-game2.png` shows the fallen branches |
| MAP12B Grimm death, exit | VERIFIED | `coop_hurtname pbear:100000`: host `death commit id=55`, client `present death id=55 ok=true`, `applied host fire 'cScrTask02'` |
| MAP11B Hogzilla | PARTLY | Reggie `cs04` beat: host `[host] npc PlayAnim id=60 'cs04'`, client `applied npc anim id=60 'cs04' ... r=1`, released when the host leaves NoChange. Hogzilla spawns through the real `cScrTask10` flow; its `wb_syncattack1` attacks (on Reggie, about every 10 s) show on the client mirror. Death: `death commit id=88`, client `present death id=88 ok=true`. Knockdown of the client by the real animal UNVERIFIED (below); synthetic `coop_animalknock 1:wb_syncattack1`: `no clip 'wb_syncattack1' on our hunter; knockdown uses 'pl_wb_syncattack1'`, `knocked down ... ok=true`, stands up by the 8 s timeout |
| MAP12A Reggie / Wirake `hurtdown` | VERIFIED in the log, pose UNVERIFIED | real trigger flow (tp into the volume, not `coop_trigger`): host `npc PlayAnim id=137 'hurtdown' ... item=true`, client `applied npc anim id=137 'hurtdown' has=true item=true r=1`; Wirake (cScrTask08): `id=138 'hurtdown' item=false`, client `clip0='hurtdown'`, held while the host stays NoChange |
| MAP2B Hugh | VERIFIED in the log, pose UNVERIFIED | `cScrTask04` Update sequence: host `'hurtsit'`, then `'hurtdown'`, later `cScrTask14` `'syncguidegetup'`; client `applied npc anim id=128 'hurtdown' ... clip0='hurtdown'`, `'syncguidegetup'` released at host state Idle 2.3 s later |

### Unverified / open
- **Visual pose of Reggie / Hugh on the client.** With an item (a weapon) the task sets the clip on the item and the host's NPC state stays
  `Idle`, so `NpcAnimHold` never sees `NoChange` and lets go after its 3 s grace; what the client's NPC then shows (idle or the
  hurt pose) was not seen. Two screenshot attempts ended in the ice fog of the MAP12A start area (no usable camera).
- **A real Hogzilla attack on a client.** Hogzilla's task target is Reggie (`AttackChargers`), and `RetargetToNearestPlayer` only moves
  animals that already chase a player. After Reggie died (about 90 s into the fight) Hogzilla stood in `Run` at
  (753.2,157.2,-551.8) for 100 s and ignored the client's hits (`animal id 88 hurt by player 1: brain told ok=true`). Reggie's
  death fails the mission in real play, so this only matters for a test.
- A yeti sync attack on a puppet: in these runs the yeti attacked the guide, the host hunter and melee-hit the client, never a sync
  attack on the client. `hk_PlaySA` unlink + `kPlayerKnock` with the yeti clips is verified only through `coop_animalknock`.
- `coop_trigger` fire is not a real run of the trigger on the host (the host's PlayAnim never started in the first MAP12A run);
  use a volume (`coop_tp` into it).
- 3 or 4 players, a real two-PC run, a late joiner during a tree fall.

### What made things unstable
Nothing in this branch's runs: no crash, no forward error, no AV. Main's thaw crash class (MAP8 crocodile, MAP7 tiger and Reginald: a copy
thawed for the host's sync attack) is what `beasts` fixed. This branch's mirror of a host sync attack sets CutScene on the parts by name
with no partner link and never thaws, and a stale local sync attack is ended before any state write.

## 5. Proposed MEMORY facts
- A client mirror linked to another character by a sync attack (`cModelPart` followers) replays every clip it plays on that partner;
  a clip the partner lacks raises "Could not forward animation" through the script VM, each exception leaks a `cState` frame, and about
  700 of them end the process. Unlink (`SetSyncWith(nullptr)`) before any clip or state write on a mirror.
- The MAP12C opening cutscene `WaitForTask`s the yeti's `AIHuntTask_PlaySA`. The frozen mirror never finishes it; calling the task's own
  finish (CommonDll7 `0xcf950`) lets the cutscene resume and release the camera in about 0.1 s. Without it the host's fire of
  `cScrTask01` is deferred for ever (the lead's "80+ s defer").
- A thrown rock moves about 20 m/s; per-frame sampling at 10-20 fps steps past a 1.4 m capsule. Test the path, not the point.
- `coop_kill <id>:<dmg>` on the host has no kill credit (`+0x420`); `coop_hurtname <script name>:<dmg>` sets it. A client's `coop_kill` reaches the host as a real hit.
- MAP12B: Big Grimm starts `AttackObjectMovers` at map start; a tree falls when a player stands near it and Grimm near the tree (tree 1
  (56.1,3.9,0.4), tree 2 (34.9,11.3,-59.1) m); both players near the tree is the reliable test.
- BSP `entity("trigger_event")` records carry `script class name` and the position in cm: MAP11B `cScrTask10` (749.65,159.06,-562.0) m,
  MAP12A `cScrTask10` (-47.2,8.8,101.6), `cScrTask08` (-126.0,7.6,-27.0), MAP2B `cScrTask14` (-77.9,9.8,-123.2).
