# beasts — every species and weapon a client uses counts like the host's (2026-10-18)

Worktree `D:\scary hunt\dh2coop-beasts`, branch `beasts`, port 27630, Ghidra slot K. The session stopped early (usage limit): item 1 is done and verified on MAP6; items 2–4 are coded as a WIP commit and untested; item 5 not started.

## Commits
- `4ebdc36` Client tranquilizer darts reach the host (MAP6 verified): `beasts.cpp/.h`, `events.h` (`kTranqHit = 22`), `dh2coop.ini` keys.
- `f92caf9` WIP: water crocodiles mirrored, client claim script, mirror clip checks: `player_tracker.cpp/.h`, `events.cpp`, `animals.cpp` (2 call sites).

Edits in `animals.cpp` (owned by `life`), both in `ApplyMirror`: `#include "coop/beasts.h"`; NPC branch `NpcLegClip(state)` → `beasts::NpcLegClipFor(ent, NpcLegClip(state))`; animal branch `if (!beasts::MirrorStateOk(ent, sample.state)) continue;` before `SetBodyState`.

## 1. Tranquilizer darts — VERIFIED (MAP6, two windows)
Engine (GameDll7 decompiles, slot K):
- `cScrAnimal::OnHit` `0x1a680` (vt `+0x8c`, thiscall(animal, bullet, tVect3* pos, tVect3* dir)). Needs hit-bone index `+0x3d8 >= 0` (set by the bullet ray, reset to -1). It writes `+0x420 = 1` (the kill action the death reports), then for `mod_obj_tranq_bullet`: `+0x4f0 == 2 && +0x604 == 1` → `+0x4f0 = 3`, `0x965e0(mission, "…overdose…")` (status 3 → `0x91c20(6)`); else `+0x4f0 = 2`. Hurt is vt `+0x228` = `0x13d80(dmg, bullet)` with `+0x4f8` (tranq hurt). Last step: brain vt `+0x88(shooter)`.
- `cScrAnimal::Update` `0x1f3c0`: while `+0x4f0 == 2`, `+0x4f4 -= dt`; at ≤ 0 (and `+0x600 == 0`) calls the death slot `+0x154` (`0x1e8b0`: "Dead", `+0x4f0 = 1`, `UpdateMissionStatus(+0x420, +0x434)`). `+0x4fc` (time before limp) → brain `+0x84 |= 0x10000`.
- `+0x4f4` = `SetTranqTime` (MoveToPos default 5.0), `+0x4f8` = `SetTranqHurt` (default 0), `+0x604` = `IsProtected()` (BTIGER, ELEPHANT). BTIGER: tranq 2.0 s, protected; ABEAR/RHINO: default 5.0 s.
- `UpdateMissionStatus` = `0x91e80(mission, action, value)`: stores `+0x1dc/+0x1e0`; only action 1 counts a kill. A protected-species allowance lives at `+0x1f0/+0x1e8/+0x1e4`.
- Bullets: `cMOD_ObjBullet` (ctor `0x2b590`, Start `0x2b0b0`: `+0x1b8` = firing weapon item, `+0x1bc` = its parent hunter; `+0x1b4` type, 2 = no hit bone / no tranq branch). Tranq bullet = `cMOD_ObjTranqBullet` (ctor `0x32f30`).

Mod: client hook runs OnHit for local effects with `+0x4f8 = 0`, restores `+0x4f0`, then `Request(kTranqHit, id)`; host `HostDart` = the branch above (credit, state 2, overdose, hurt, `combat::OnClientHitAnimal`). Host's own darts run the real OnHit and are logged (`beasts: host dart`).

Evidence (`re/notes/coop-local/beasts-map6-run2-game*-dh2coop.log`):
- client `beasts: client dart id=194 bone=0 … -> host`; host `beasts: host ran player 1's dart on id=194: tranq 0->2 sleeps in 2.0 s protected=1`, 2.0 s later `state='Die'`/`'Dead'`, `scale: mission report action 1 value 21`, `death commit id=194`; client `present death id=194 ok=true`.
- `coop_tasks` on the host: task 2 state 6 after the client dart on 194; task 3 state 6 after host dart 196 + client dart 195; task 4 state 6 after client dart on the bear 198 (5.0 s, `value 20`). Client dart on scale reserve 199 also slept.
- Host's own dart (`coop_tranq 196` on the host, real branch): `tranq 0->2 sleeps in 2.0 s action=1` — same as the emulation.
- Run 1: the host (no god mode) was killed by the ambient `EVENT_ANIMAL` BTIGER (id 202) → MISSION FAILED. Use `godMode 1` on the host in tests.
- UNVERIFIED: the overdose path from a client (two client darts on one tiger within its 2 s), MAP7 (tigers, rhino), visual limp/sleep on the client screen (screens taken were too late / blocked: `beasts-map6-*.png`).

## 2. Water crocodiles — WIP, UNTESTED
- `cObjCrocodile` vtable `0xffa50` (ctor `0x1d720` = cScrAnimal ctor + croctrail FX `+0x71c`, `+0x728 = 1.0`). Differs from `0xff470` in: dtor, HandleKey, Draw, Update `+0x44` `0x1ff80` (cScrAnimal Update + trail + fade `+0x450` from `+0x728` while the brain task's `+0x18 == 0x4000` or `+0x590 == 1`), Precache `0x17040`, Spawn `0x1f270`, `+0x84`, MoveToPos `+0x158` `0x17270` (calls `0x14610` first → the existing re-freeze hook covers it), Kill `+0x234` `0x1dd40` (calls `0x1be20`). OnHit, death `+0x154`, hurt are cScrAnimal's (so darts work too).
- Brain `AIHuntBrain_Floater` (`cAIHuntBrain_Floater`, CommonDll strings): FreezeBrain unverified.
- `CROCODILE_W.MODEL` clips: defaultpose intro idle hurt attack1 run rundisabled + sync clips (no walk/idleattack1). `ANIMAL_CROCODILE` (MAP8 `crocodile1..3`) is a plain obj_animal, already mirrored.
- Risk: a frozen client croc keeps its task state, so its fade may differ from the host's (invisible or always surfaced).
- Next: `-Map MAP8`, host `coop_tp 85.0:-4:143.1`, wait 10 s; host log `tracker:   vt GAMEDLL7.DLL+0xffa50 x3`; client `animals: mirror freeze id=… ok=true` for those ids; `-Capture`. MAP3A: host `coop_trigger cScrTask10`, croc ids ~101–105.

## 3. Claim callback on clients — WIP, UNTESTED
`0x1cd40` → `0x15550`: `+0x404 = 0`, then if `+0x408` (script object) is set, class manager vt `+0x28(+0x408, +0x40c)`. Client `OnClaimCommit` now runs the original once per id per map (`[Animals] ClientClaimScript=1`), not under `g_in_apply`. MAP2C callback: `DisableStealth`, objective, `StartTask 5/6`, `cave_meat.Activate()`, `setstate(overanddone)`; hunt maps: `UpdateMissionStatus(REACHED,1)`, `gbCanEndHunt = 1` (client mission checks stay swallowed).
Next: audit-a G9 recipe (`-Map MAP2C`, host `coop_trigger cScrTask05`, `coop_kill <boar>`, client `coop_claim <id>`; client log `claim script ran here`, no later boar respawn on the client; client can use cave_meat).

## 4. Clip coverage — WIP, UNTESTED (and a correction)
- **Correction to MEMORY "MAP4A lioness has no idleattack1":** `LIONESSE.MODEL` has `idleattack1/2/3`. "Could not forward animation" is raised only in `cModelPart::SetAnim(int,float,float)` (EngineDll7 `0x1e73b0`) when the part is linked (`+0xf4` ring, next link at partner `+0x10c`) and a partner has neither `"<part +0x5c prefix>/<clip>"` nor `"<clip>"`. The lioness copy was still linked to a hunter body after a tackle. A clip missing on the part itself is not forwarded: `SetAnim(name)` (`0x1e7820`) logs "Could not set anim" and stops the part (vt `+0x5c`) → bind pose.
- `beasts::MirrorStateOk`: only when part 0 is linked; checks the literal clips of the cAnimalLegs state (table from `ANIMALLEGS.SCR`) plus the brain-param clip (`idleanim`/`stalkanim`/`attackanim` via `cObjCharacterEx::GetBrainParam`) against every linked part. Also logs once per model which cAnimalLegs clips it lacks (seen: btiger 18/36, abear 14/36 — host and client equal, soft failures only).
- `beasts::NpcLegClipFor`: preferred clip if playable, else fallbacks (`idlenormalneutral`→`idle`/`idlenormalgestures`/`coveridle`/`defaultpose`; side/back walks, `run/forward`, `walk/forwardlimp`→`walk/forward`; `fall`→`hurtdown`; `liedead`→`liedeadidle`; `hurt`→`hit`; `coveridle`→`cover`), else no SetAnim. Seen MAP6: `ausranger3` has no `followme`.
- NPC packs (ap_list): indiangirl/teengirl/nephew `idle walk/forward run/forward turn* point conversation crying jump`; fiance `defaultpose cover coveridle uncover hit scare hurtdown`; ausranger3 has `idle idlenormalneutral walk/forward run/forward liedead liedeadidle`, no side walks; poacher GAME.AP only `blink`.
- Open: a linked (stale sync ring) animal mirror still forwards its current state's Update clips; the real fix is ending the stale sync attack on animal mirrors (`life`'s `ApplyMirror` does it for NPCs only).
- Next: MAP7 `-Capture`, host `coop_tp 158:3:-45` (girl): client log `beasts: npc model '…indiangirl…'` lines and screenshot.

## 5. Cobra — NOT STARTED (hook coded)
- Found: a client's real bullet on a mirrored cobra was lost (cobra OnHit `0x80440` sets Death + `+0x738 = 50` locally, no health drop, the mirror resets the state). `hk_CobraOnHit` now sends `AnimalHit` (host `StartCobraDeath`) when `+0x738` becomes 50. UNTESTED: `coop_tranq <cobra id>` on the client (MAP3B ids 49–51 / MAP7 `cobra1`).
- `CobraBitesNearest` default decision not made: run audit-b G12 recipe (MAP7, host `coop_trigger cScrTask07` (twice if armed only), client `coop_tp 156.3:2:-41.2`, host `animals: cobra id=… bite target -> puppet` with the key on).

## Other findings
- Life's `OnAnimalHit` (bullets) never runs the protected-animal fail of OnHit (`+0x604` → `0x965e0` "killed a protected animal"), and reports action 0 unless `+0x420 = 1` is set (its open item).
- MAP6 reused leopards (Task05/07 `MoveToPos`) stay dead on clients (audit G4, `life`): a client cannot see them, though `coop_tranq` still reaches the host.
- Tranq gun model `tranqgun1st/tranqgun.skin` maps to weapon kind 4 (no puppet weapon).

## Proposed MEMORY facts
- Tranq darts: `cScrAnimal::OnHit` (GameDll7 `0x1a680`) on a `mod_obj_tranq_bullet` with hit bone `+0x3d8 >= 0`: `+0x420 = 1`, `+0x4f0 = 2` (or overdose `+0x4f0 = 3` + `0x965e0` when already 2 and `+0x604 == 1`), hurt `0x13d80` by `+0x4f8`; `Update` `0x1f3c0` counts `+0x4f4` down and runs the death `+0x154` (sleep = "Dead", kill action `+0x420`). BTIGER tranq 2.0 s protected, default 5.0 s, tranq hurt 0. — decompile + MAP6 run 2026-10-18.
- `UpdateMissionStatus` `0x91e80(mission, action, value)` only counts action 1 as a kill; MAP6 BTIGER value 21, ABEAR 20. Mission task list `+0x1f8` (count `*(+4)+1`, pointers `*(+0x10)`), task id `+0x64`, state `+0x68` (6 done). — decompile + `coop_tasks`.
- `cObjCrocodile` vtable GameDll7 `0xffa50`: MoveToPos `0x17270` calls `0x14610`; Update `0x1ff80`; OnHit/death are cScrAnimal's. — vtable diff.
- "Could not forward animation" is only raised for linked parts (`cModelPart::SetAnim(int,float,float)` EngineDll7 `0x1e73b0`); `cModelPart::GetAnimIndex(const char*)` `0x1e69b0` returns -1 when missing; the MAP4A lioness model has `idleattack1`. — decompile + `.MODEL` clip list.
- `cObjCobra::OnHit` `0x80440`: a client bullet on a mirror never reached the host before `hk_CobraOnHit`. — decompile.
