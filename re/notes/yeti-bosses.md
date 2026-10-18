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
`[Bosses]` `NpcAnims`, `PropSA`, `ThrowCopies`, `ThrowHitsPuppets` (all default 1; none is in `mod/dh2coop.ini` yet).
Console: `coop_hurtname <name>:<damage>` (hurt an animal by script name, with kill credit on the host/solo; on a client it is a hit on the
mirror and goes to the host as `AnimalHit`).

## 3. Test log

(filled in as the runs finish; see the section "Results" below)
