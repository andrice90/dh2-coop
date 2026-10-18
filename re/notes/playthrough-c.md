# Playthrough research C: MAP9 → MAP12C and the game end (job `research-C`, wave "playthrough", 2026-10-18)

Static research only: no build, no game run, no git writes. Sources: `re/extracted/SCRIPTS` (TRIGGERS, MISSIONS, LEVELS, CUTSCENE, MODELS/ANIMALS, INTERFACE/SCRIPTS/GUI*), BSP entity lumps (dumps in `re/notes/playthrough-c-ents-MAP11A..12C.txt`; MAP9/MAP10 in the older `auditb-ents-MAP9/10.txt`), clip lists (`re/scripts/ap_list.py` on `.AP`; a scratch scan of `ModelAnimBone` records for animal `.MODEL` files), Bink headers in `game/Data/Movies`, Ghidra slot C (outputs `re/notes/playthrough-c-endgame.c`, `-setmapcomplete.c`, `-movie.c`, `-ghidra1.txt`, `-ghidra2.txt`). Code refs are `mod/src/coop/<file>` on main `a3322d1` (function names, line numbers approximate). Unmerged branches read: `dh2coop-yeti` (`83656f1` + dirty), `dh2coop-hunter` (`3d2c561`, `2a27878`). Neither `yeti-bosses.md` nor `hunter-nearest.md` exists in those worktrees; their commit messages were used.

Legend: **OK** covered by main · **RISK** covered with a weak spot · **GAP** not handled. Old gap ids: `b-Gn` = `audit-maps-b.md`, `c-Gn` = `audit-maps-c.md`. New items: `PC-n`. Every claim VERIFIED (seen in code/data/decompile) or UNVERIFIED (inferred).

---

## 0. Cross-map mechanisms (drive most items below)

### 0.1 Kill credit is per process, not "client kills never count" (PC-1)
- `+0x420` (the `MISSION_ACTION_*` a death reports) lives on each process's own copy of the animal. Set to 1 by the animal's bullet/melee `OnHit` (`0x1a680`, `auditb-animal-onhit-tranq.c:65/81`); reset to 0 by the `MoveToPos` respawn `0x14610` (`park-flag.c:131`) and by present-death `0x1e8b0` right after it reports `0x91e80(mission, +0x420, +0x434)` (`objmodel-decomp-game-animal.c:1060-1063`). VERIFIED.
- Host: a client's lethal `AnimalHit` → `InflictDamage` + `PresentDeath` (`animals.cpp` `OnAnimalHit`) reports the host copy's `+0x420`: 0 unless **the host hit that animal earlier in the same life** (then 1, it lingers until the death). VERIFIED (code + decompile).
- Client: the client's own lethal bullet runs the normal death on its local copy (`OnHurt` at 0 → `0x1e8b0`, `+0x420 = 1` from its own `OnHit`), so **the client's mission counts the client's kills**; a host kill presented on the client (`PresentDeathOnce`) reports the client copy's `+0x420`, normally 0. UNVERIFIED at runtime, follows from the code paths.
- So each mission counts mostly its own player's kills. A kill task ends on whichever process reaches the count. A client's task end fires its `ActivateTriggerOnEnd` trigger through the forced OnHit (`0x96da0`), whose fire becomes a request; the host's `OnFireRequest` runs `Fire()` = `OnActivate` directly, **bypassing the host's `WaitTask` check**. So a map whose end trigger's `OnActivate` calls `CompleteTask(n)` or exits recovers from split credit; a map whose end needs the **host** mission to have every kill task done (`CheckMissionStatus` without `CompleteTask`) soft-locks. In my range: **MAP11C (BLOCKS)**, **MAP12C (BLOCKS when the host never hit the yeti in its last life)**. Resilient: MAP9, MAP10, MAP11A, MAP11B, MAP12A, MAP12B (table in section 9).
- Split credit also means the client's mission can end a task *before* the host's (its local kills of "phantom" second lives, 0.3) and push the story ahead of the host's own mission.

### 0.2 BeginCutScene-only spawns (PC-2; root cause b-G1 / c-G3, still open)
`OnHit` and `BeginCutScene` still run only on the process whose own player touched the trigger (`story.cpp` hooks arm `0x68eb0`, fire `0x68130`; nothing hooks `0x6b140`). A client-first touch makes the host run `OnActivate` alone (`OnFireRequest` → `Fire`). Every `AddAnimal` animal starts parked at -10000 (scale agent, VERIFIED). When the **only** `MoveToPos` that brings an animal in sits in `BeginCutScene` and `OnActivate` then `StartTask`s it, the host's animal stays parked and its kill task can never finish:
- **MAP11B `cScrTask10`** (Hogzilla, task 5 → `cScrTask09` `WaitTask(5)` → exit): BLOCKS.
- **MAP12A `cScrTask08`** (pbear, task 8 → OnEnd `cScrTask13` → exit): BLOCKS.
- **MAP12A `cScrTask05`** (wolf1, task 5 → OnEnd `cScrTask10`): BLOCKS when wolf1 died in task 4 (its `OnActivate` restarts wolf1 without `MoveToPos`).
- DESYNC only (later `CompleteTask` covers the task): MAP9 `cScrTask03` (dingo1), MAP10 `cScrTask22` (puma4), `cScrTask21` (boars 5/6), `cScrTask09` (puma4), MAP12A `cScrTask03` (pbear). MAP11C `cScrTask10` (wave t4 boars 1/2) is heli-activated: BLOCKS only if a client's heli touches first (rare, 1.4).
Map-start triggers (`Activate(); ActivateAfter(0)` in `Spawn`) run `BeginCutScene` on every process: MAP12B/MAP12C boss spawns are safe.

### 0.3 Respawn = new life is still open (b-G2 / c-G2 parts 2-3); what main fixed
- FIXED since the audits: a replayed `MoveToPos` no longer wakes a **live** frozen mirror (`hk_MoveToPos` re-freezes after `0x14610`; per-frame `brain+0x79` check in `ApplyMirror`). VERIFIED code.
- Still open: after the first death the client mirror is terminal (`death_presented` → hp pinned 0, no placement, `ApplyMirror`), and `PresentDeathOnce` **unfroze** the brain (`[Animals] UnfreezeDead=1`). The next replayed `MoveToPos` revives that local copy (health, dead bits, `+0x600` cleared) and `hk_MoveToPos` does not re-freeze it (`m.frozen == false`). The script's `StartTask(...)`/`SetTaskParam("target","hunter")` then runs on the client's own AI: a **phantom** that attacks the client's own hunter locally, can be "killed" locally (counts on the client mission, 0.1), and is never sent to the host. Meanwhile the host's real second life is invisible on the client and hurts the client's puppet (`kPlayerHurt`). Host side `announced_` and the journal key `(kAnimalDied, id, "")` swallow every later death. VERIFIED code; runtime UNVERIFIED.
- Lives per map in my range: MAP9 dingo1 ×7, dingo2 ×6; MAP10 wildboar1/2 ×5, puma4 ×4, puma1 ×3; MAP11A boars 1-3 ×2-3; MAP11B boars 1-5 ×2; MAP11C boars 1/2 ×4; MAP12A wolf1 ×4, wolf2 ×3, pbear ×2.

### 0.4 Client-local cutscenes that wait on a frozen animal (PC-3, NEW)
A cutscene that `WaitForTask`s an **animal** task (`AIHuntTask_GoToEntity`, `AIHuntTask_PlaySA`) never ends on a client whose copy is a frozen mirror, unless the host's animal is in `CutScene` at that moment (the only thaw, `ApplyMirror`: host state `CutScene` and local state `CutScene`). While the camera is on the rail, `PlayerInCutscene()` defers every host fire (`OnFireCommit`), and the client's own trigger stays in state 5 (no fire request). The client is out of the story until it presses the cutscene **skip key** (works locally; `hk_TriggerSkip` only forwards the host's). Cases (all `BeginCutScene`, so only a client-first touch today): MAP10 cs2 (`cScrTask21`, boars `GoToEntity` → never `CutScene` on the host either), MAP10 cs4 (`cScrTask09`, puma4 PlaySA), MAP12A cs2 (`cScrTask05`, wolf1 PlaySA), MAP12A cs4 (`cScrTask08`, pbear PlaySA). MAP12C cs1 (map start, yeti PlaySA) is covered by the thaw when both load together, and by the late-join skip otherwise. Cutscenes that wait on **NPC** `AITaskNPC_PlayAnim` (MAP9 cs1-3, MAP10 cs1/4/5/6, MAP11B cs1/2, MAP12A cs1/3/5/6) are assumed to finish on clients (MAP2A precedent; mechanism UNVERIFIED). VERIFIED script + code; hang UNVERIFIED at runtime.

### 0.5 "Could not forward animation" is a part-sync partner problem, not a missing clip (PC-4, corrects MEMORY)
Clip scan of the `.MODEL` files: LIONESSE and LION **do** contain `idleattack1` (so do DINGO, PUMA, WILDBOAR, WILDBOAR3, GWOLF, PBEAR, BIGGRIM, HOGZILLA, YETI). The engine message is "forward … from Legs to Legs model X": a clip set on an animal is replayed on the parts linked to it by a sync attack (`SetSyncWith`), and the **partner** lacks it. That is the yeti branch's root cause (`83656f1`), and it explains the MAP4A lioness too (linked to a puppet). Partners in my range:
- Animal ↔ **puppet** on the host: the puppet body loads `game.ap,shotgun1.ap,handgun3.ap` only, no map pak and no `pl_*` clips. Every player sync attack on a puppet links it: MAP10 pumas (`pl_pm_*`), MAP11A/B/C boars (`pl_wb_syncattack1`), MAP12A wolves (`pl_gw_syncattack1`). Yeti branch: unlink the puppet in `combat.cpp` `hk_PlaySA` ("fatal on the host").
- Client's own knockdown clip (`kPlayerKnock` key = the animal's clip): `TEENM\BODY\MAP10.AP` has `pl_p_*` but MAP10 pumas use `pl_pm_*` (`PUMA.SCR:94-101`, `:262-403`); MAP12C yeti uses `yeti_syncattack1..4` (teen pak has `pl_pb_syncattack1` only). Yeti branch falls back to the map's `kMapClips` front clip. MAP11 boars and MAP12A wolves match their teen paks. MAP9 dingoes and MAP12A/B bears have no player sync attack (`AddSA` commented out). Big Grimm's only SA is `faltree` (a tree), so audit-c's `pl_bg_syncattack1` note is moot.
- Animal ↔ **NPC** cutscene pairs whose clips exist on both (verified `cs04`, `cs05`, `cs02a`, `yeti_syncattack1` in the NPC and animal map paks): the danger is the mirror writing body states while the client copy is still linked (MAP12C 1151 lines of `idleattack1` forwarded to the guide). Main ends stale links only for NPC mirrors; the yeti branch drops links on animals before any state write.
Each failure is an engine error raised through the script VM; per the yeti branch, every one leaks a `cVMachine` state frame until the VM stack faults (`EngineDll7+0x28b337` / `+0x29a8bf`). Main's "3 tries per state name" caps the count per name, not the leak.

### 0.6 Host runs `OnActivate` twice (b-G5, still open)
`OnFireRequest` zeroes a pending `+0x1fc` (states 2-5) and calls `Fire`; the host's own sequence later reaches state 2 and `hk_TriggerFire` (host path) runs `OnActivate` again. Matters most on the pilot maps (1.4): `cScrTask01` `BSMGoToState("Ani1")` twice restarts the host's helicopter path. VERIFIED code.

### 0.7 The game end and chapter movies (PC-5, NEW)
- `cHunterProfile::SetMapIsComplete` (`0xec160`) advances the profile's map index `+0x23d8` on **every** call (cap `0x19` = 25 entries) and sets `bShowEndCredits` (`0x186cd0`) when the index reaches 25 and `bGameCompleted` (`0x186ccc`) is 0. VERIFIED decompile. Clients call it once per map: `ApplyMissionEnd` (code 5) and their own trigger-level exits (`0x6c420`).
- Results → Continue (`GAMEMENU.SCR` `CM_NEXT_MISSION`, `:990-1047`): `UpdateSave`, autosave dialog, then if `!bGameCompleted && bShowEndCredits && nCurrentMap >= NR_MISSION-3 && nCurrentMission > NR_MAP-4`: Quit to the main menu and `ConsoleExec("mod_playendfmv")`; else `GameLoadLevel()`. `mod_playendfmv` = `0xbe480` (registered at `0xbd841`): plays `Movies\omap12c` in-game and sets `bShowEndCredits = 1`; `MAINMENU.SCR:43-48` opens the credits page when the flag is set. VERIFIED script + decompile.
- `0xc0080` (the level starter registered next to `mod_startlevel`, `0xbd797`) plays `Movies\o<previous map>` and `Movies\i<next map>` by the mission table type, then `spmap <GetMapToBeStarted>`. Movies in my range: `imap11a` 54.8 s, `omap12a` 35.8 s, `imap12c` 23.0 s, `omap12c` 64.1 s (Bink headers). Every process plays them through its own menu path; a client following by `spmap`/`mod_spmap` does not.
- The player `EngIoToolPlayMovieInGame` → `0x343a10` is a modal loop with its own `PeekMessage` pump (Esc on key-up ends it) that renders each frame through `render` vt `+0x84/+0x88/+0x90`. If `+0x90` is `cRenderDX::Flip` (likely), the mod's Flip/Present pumps keep ENet alive (timeouts 10-45 s, `session.cpp:70-71`); else a 55-64 s movie drops the session. UNVERIFIED which slot; test R9.
- A client's credits need its own profile index at 25, i.e. a profile that completed the same 25 progression entries. A client that joined mid-campaign gets `GameLoadLevel` (its own next map) until the host's `kMapStart(MENUMAP)` pulls it back. COSMETIC.

### 0.8 The pilot after G1 rides its own helicopter (input for the `pilot` owner)
- `cObjPilot::Spawn` `0x65370` (re-decompiled): `FindEntity("helicopter","obj_helicopter")`, `Attach(pilot, heli id, GetTagIndex("masscenter"), 1, 0)`, body state `Heli`, `RemoveStimulus` + `RemoveAttackTarget`. VERIFIED.
- So once the tracker accepts vt `0x109d48`, the pilot's `+0xA4` is the heli id and `SendLocalState` sets `kFlagRiding` automatically (`coop.cpp` `parent >= 0`). Remote pilots' puppets are then unregistered as attack targets (matches the pilot's own `RemoveAttackTarget`), collision/gravity off, and seated beside the local pilot in **the local heli copy** (`puppets.cpp` `riding_spot`, heading from seat motion); `PickVictim` returns nobody for an attached local player, so no PvP knock in the shared cockpit spot. This is the right picture: every player is the pilot of their own heli copy and sees the others in it. VERIFIED code.
- Two things break for an attached player: (a) `TeleportToHost` (after-cutscene gather, join catch-up, `coop_tohost`) has no parent check and would `place_near` the attached pilot; `DistanceToHost` uses `LocalPlayer().pos`, sampled once a second, which lags tens of metres at heli speed (PC-6). (b) `PlayerInCutscene()` (camera > 4 m from `+0xE8`) must hold for the pilot camera, or every host fire defers forever (c-G1 open question).
- The helicopter (`cScrHelicopterEnt`, `cBodyHelicopter` in `MODELS/WORLD/HELICOPTER/HELICOPTER.SCR`) is animation-driven: `Ani1 → Loop1 → Ani2 → Loop2 → Ani3a/Ani3b → Loop3 → Ani4`. Each Loop waits for `IsTaskCompleted()` (set by `helicopter.CompleteTask()` in the task-end triggers' `OnActivate`), so every copy re-synchronises at each gate within one commit latency. Unlike the MAP4A truck (physics, diverged), drift is bounded; no pose streaming needed first. Two exceptions: `Ani3a` never leaves its state (`SetAnim("ani3b")` only); `Loop3` is reached only through cutscene 2's `BSMGoToState("Ani3b")` (and `Ani4` through cutscene 3), both in `BeginCutScene`s of heli-touched triggers. A process that misses those cutscenes keeps its heli parked at the end of `ani3b` (c-G14, COSMETIC unless the next trigger lies past it). VERIFIED script.

---

## 1. MAP9 (Reggie, dingoes, ice; `SetMission(9)`; end = `CheckMissionStatus`)

Sequence (`TRIGGERMISSION09.SCR`): `cScrTask01` at spawn (task 1 race, arms 02-08, 12-15, 100; Reggie `SetCanBeAttacked(0)`) → `02` (BCS cutscene 1: Reggie health 20000/recover 1000; OnActivate Reggie companion of `"hunter"`, `AddAIToBeProtected`) → `03` (BCS `dingo1.MoveToPos` + cs4 `ForceBrainActivate`; OnActivate dingo2 respawn, both `AttackDogs` → `"hunter"`, task 2) → `04` (BCS cs2; dingoes 1/2 respawn, task 3) → `05` (dingoes 1/2 respawn, no task) → `06` (respawn, task 4) → `07` (Reggie guide + `AITaskNPC_MoveToPos c3_1`) → `08` (BCS cs3; arms 09, task 5, dingoes 1-3 respawn) → `09` (state: arms 10, Reggie walk/radiotalk/look with `WaitForTask`) → `10` (arms 11, task 6, dingoes 1/2 respawn) → **`11`: OnHit `UpdateMissionStatus(REACHED_POSITION,1)` (race task 1); state: `heli1.SetAnim("m9_rescue")`, `CompleteTask(2..6)`, Reggie look/talk, `CheckMissionStatus()`**. Mission: race 1; dingo kills 2/2/2/3/2 (tasks 2-6), no OnEnd triggers. Ids (BSP order, UNVERIFIED): triggers 98-112 (`cScrTask11` = 108), dingoes 118-120, reserves `coop_DINGO_0..2` after them.

| Beat | Main today | Verdict |
|---|---|---|
| Touch triggers, arm/fire replay | `story.cpp` | OK |
| **Final race completion only in `cScrTask11` OnHit** | client-first: client OnHit completes race 1 locally; host runs OnActivate only; host `CheckMissionStatus` fails once (state machine goes Inactive); host's own later touch refused (`+0x1fc = 1`) | **BLOCKS** (b-G1, open). F5 only |
| BeginCutScene cutscenes 1/2/3/4 (+ dingo1 spawn in 03) | only on the toucher | DESYNC (b-G1); PC-2 DESYNC only (tasks completed by `cScrTask11`) |
| Cutscene 4 `ForceBrainActivate(0/1)` on dingo1 (client-run) | if it clears brain `+0x79`, the per-frame wake check re-freezes (only while `m.frozen`) | OK (UNVERIFIED that the brain call touches `+0x79`) |
| Reggie health 20000 in cs1 only | host Reggie keeps 100 when the client ran cs1; he is `SetCanBeAttacked(0)` from `cScrTask01` | OK |
| Dingoes: 3 bodies, ~13 lives | live re-freeze OK; second lives phantom/invisible on clients (0.3) | **DESYNC, severe** (b-G2 open): invisible dingoes hurt the client, a local phantom attacks it |
| Kill credit | tasks 2-6 closed by `CompleteTask` in `cScrTask11` | OK (resilient) |
| Scaling: DINGO pool, 3 reserves | reserves kept out of the mission (scale VERIFIED on MAP2A) | OK; reserves do not respawn (`ReuseKilled=0`) |
| Dingo `target "hunter"` | host only; `RetargetToNearestPlayer` switches animals already chasing a player (3 m margin) | RISK (hunter owner; `HunterNearest` unmerged) |
| Reggie companion / guide of `"hunter"` | follows host (`NpcFollowNearest=0`) | DESYNC (b-G7) |
| `cScrTask09`/`11` states `WaitForTask(reggie9, …)` on clients | replayed OnActivate; NPC tasks assumed to finish (MAP2A) | RISK, UNVERIFIED; `CheckMissionStatus` is host-only anyway |
| Ice ×3 over `trigger_hurt 100000`, 4 hurt volumes | per process; host falling in = level failed | COSMETIC (c-G17); host death ends the level |
| Knife start, `handgun2_1st`/`mbranch_1st` pickups | per-process copies, no `next trigger` | OK |
| EVENT_ANIMAL dingo, bats/birds/cavedust | event respawn goes through `0x14610` → inline hook re-freezes | COSMETIC |
| Clips | dingo has no player SA (`AddSA` commented); teen pak `pl_do_syncattack1` unused | OK |
| `heli1.SetAnim("m9_rescue")` | own native, runs in both OnActivate replays | OK |
| F5 | menu hop; nothing map-specific | OK |

Cross-ref: the OnHit-only race completion repeats on MAP10 `cScrTask16` BCS / `cScrTask17` OnHit (resilient: `CompleteTask(1..11)`), MAP11B `05`/`08` (HUD only), MAP11C `02` (BLOCKS if a client heli leads, 3.x), MAP12A none (race completed in `cScrTask09` state).

## 2. MAP10 (Wirake, Reggie, pumas, boars, foot traps; `SetMission(10)`; end = `cScrTask17`)

Sequence: `cScrTask01` at spawn (BCS cs1 `wirake10 play_sa reggie10`; OnActivate protect Wirake, traps 1-3 to `mine_t1_*`, Reggie/Wirake unattackable, Reggie companion `"hunter"`, task 1, arms 02-12 and 14-22) → `22` (BCS puma4 spawn + cs3; puma4 → hunter, task 2) → `02`/`03` (`CompleteTask(2)`; puma1/puma2 spawn+attack; task 3) → `21` (BCS boars 5/6 spawn + cs2 `GoToEntity`+`WaitForTask`; `CompleteTask(3)`, boars attack) → `04`…`08` (traps moved, boar/puma waves, `CompleteTask(4..7)`, tasks 4-8) → `18`/`15` (puma3 surprise, state 2048) → `09` (BCS puma4 spawn + cs4 puma4 `PlaySA` with reggie10, `Kill`; OnActivate `CompleteTask(8)`, Reggie guide `MoveToPos c5_1`, Wirake companion, task 9, arms 10) → `10`/`11`/`12`/`14` (waves; 14 sends wildboar2 at `wirake10`) → `13` (`CompleteTask(9)`) → `16` (BCS `UpdateMissionStatus(3,9)`, boar `time 1`, puma1 spawn + cs5 `reggie10 play_sa puma1`; OnActivate puma1/puma2 respawn + attack, **task 11**) → task 11 end (2 pumas) → **`cScrTask17`** (OnHit `UpdateMissionStatus(3,1)` + `ActivateAfter(2)`; OnActivate `CompleteTask(1..11)` + `CheckMissionStatus`). Ids (UNVERIFIED): triggers 193-214, pumas 215-218, boars 219-224, reserves PUMA/WILDBOAR3 after them, then traps.

| Beat | Main today | Verdict |
|---|---|---|
| Final via task 11 OnEnd `cScrTask17` (forced, no touch) | any mission that counts 2 pumas fires it; host runs `CompleteTask(1..11)` + `CheckMissionStatus` | OK (resilient to split credit, 0.1) |
| Kill credit, task 11 | split 1/1 between host and client: neither mission ends task 11 | **BLOCKS if the two puma kills are split and neither player hit both** (PC-1). Rare but real with 2+ shooters; puma3/puma4 kills during task 11 also count |
| BCS spawns 22/21/09 | client-first: host animals stay parked | DESYNC (PC-2), tasks closed later |
| cs2 boars `GoToEntity` + `WaitForTask` (client-run) | frozen mirror never finishes; host never in `CutScene` | **BLOCKS that client** until it presses skip (PC-3) |
| cs4 puma4 `PlaySA` + `WaitForTask` (client-run) | as above; plus puma4 ↔ Reggie part link on the client | **BLOCKS that client** (PC-3), crash risk (PC-4) |
| cs5 `reggie10 play_sa puma1` (host-run → `kPlaySA` → client `PlaySA` + hold) | hold ends when the host state changes; NPC side ended by base `OnEndSA`, animal side stays linked | RISK (PC-4; yeti branch) |
| Puma sync attack on a puppet (host) | puppet linked to the puma; puppet lacks `pl_pm_*` | **crash risk on the host** (PC-4; yeti branch) |
| Puma knock on a client (`kPlayerKnock 'pl_pm_syncattack1'`) | client teen `MAP10.AP` has only `pl_p_*` | **crash risk on the client** (PC-4; yeti branch fallback) |
| Wirake/Reggie protected, unattackable, guide/companion of `"hunter"` | host decides; follow host only | DESYNC (b-G7) |
| `cScrTask14` boar → `wirake10` while Wirake is unattackable | — | OK |
| "Mines" = `EVENT_TRAP` traps moved by replayed `MoveToPos` | per process; puppets never close traps (`obj_player` only) | COSMETIC (b-G13) |
| Respawns (boars ×5, pumas ×3-4) | phantom/invisible second lives | DESYNC (b-G2) |
| Scaling: PUMA + WILDBOAR3 pools | reserves have no `map10.ap` (only `wildboar5/6`, `puma1/4` get it) but copy attack tasks only | OK |
| `AddAnimPakForAI` on 4 animals | same on every process | OK |
| Ammo pickups, medkits | private copies | OK |
| `dbrifle1_1st` on puppets | rifle1 mesh, flame bone | COSMETIC (b-G15) |
| F5 | — | OK |

Cross-ref: animal `WaitForTask` in a client-run cutscene: MAP12A cs2/cs4 (same pattern), MAP12C cs1 (covered). NPC+animal `play_sa`: MAP12A cs2/cs4, MAP12C cs1.

## 3. MAP11A (helicopter rail shooter, player = `cObjPilot`; `SetMission(11)`; end = exit `exit_1` → MAP11B)

Sequence (`TRIGGERMISSION11A.SCR`): `cScrTask01` at spawn (`ActivateBy("helicopter")`; BCS cutscene 1 (cutscene heli `heli1.SetAnim`); OnActivate `helicopter.BSMGoToState("Ani1")`, cable car `tele.SetAnim("idle1")`, task 1 race, arms 09 and `cScrFailed`, `SetMaxAttackerNo(4)`) → `09` (heli touch; OnActivate `UpdateMissionStatus(3,1)` = race done on both, wildboar2 respawn `AttackChargers` → `siberian2`, task 2, arms 06) → task 2 end → `06` (forced, `WaitTask(2)`; `helicopter.CompleteTask()`, arms 03) → `03` (heli; boars 1-3 → `siberian3`, task 3) → task 3 end → `07` (`CompleteTask`, arms 10) → `10` (heli; BCS `tele.SetAnim("ani1")` + cutscene 2 = `helicopter.BSMGoToState("Ani3b_frozen")` … `"Ani3b"`; arms 04) → `04` (heli; boars 1-3 → `siberian4`, task 4) → task 4 end → `08` (forced, `ActivateAfter(3)`; BCS cutscene 3 = `Ani4_frozen` … `Ani4`; OnActivate `ActivateTriggerLevel("exit_1")`). Mission: race 1; boar kills 1/3/3 with `NEED_AMMO_FOR_COMPLETE`, OnEnd 06/07/08, OnFail `cScrFailed`. Ids (BSP order, UNVERIFIED): siberians 30-33, `heli1` 34, `helicopter` 35, `tele` 36, triggers 44-55 (01=44, 03=46, 04=47, 06=49, 07=50, 08=51, 09=52, 10=53, Failed=55), boars 56-61, reserves 62-64. BSP `cScrTask02`/`05` have no script class (inert).

| Beat | Main today | Verdict |
|---|---|---|
| **Player is `cObjPilot` (vt `0x109d48`)** | tracker accepts only `0x106ff0` (`player_tracker.cpp` `kVtHunterRva`); `LocalPlayer().valid` false → no `kSyncRequest` (`PumpClientSync`), no mirror callback, no `SendLocalState`, no puppets; commits go only to synced peers | **BLOCKS the client** (c-G1, open): its heli never starts (its `cScrTask01` fire is a request; the host's commit never reaches an unsynced peer), it sees frozen boars, gets no cutscenes. The host can finish alone; `kMapStart` is broadcast, so the client follows to MAP11B |
| Heli = per-process animation machine, gated by `CompleteTask` | replayed fires (once G1 works) | OK/RISK (0.8): bounded drift; a client's heli trails by its load delay until the first Loop gate |
| Heli-touched triggers 01/09/03/10/04 | each heli touches its own copy; host leads (its `CompleteTask` comes first) | OK. If a client heli leads, the host misses that trigger's BCS (cutscene 2 → host heli stuck in `Ani3a`) |
| Host `OnActivate` twice (`cScrTask01`) | a client whose cutscene 1 ends before the host's → `OnFireRequest` fires during the host's state 5; host's own state 2 fires again | **RISK** (b-G5, 0.6): host heli restarts `Ani1`, client heli ahead for the whole map → client-first touches (see MAP11C for why that blocks there) |
| Task-end triggers 06/07/08, `cScrFailed` | forced OnHit on whichever mission ends the task; fire request → host `Fire` | OK; split kill credit only delays (0.1) |
| Kill credit | exit does not check the mission | OK (resilient) |
| Respawns: wildboar2 ×3 lives, boars 1/3 ×2 | phantoms/invisible on clients | DESYNC (b-G2/c-G2) |
| Siberians 1-4 (`cObjGuide`, boar targets) | mirrored NPCs; host death = `0x5fde0` code 6 outside `0x91c20` | RISK (c-G5/b-G9); client fails only via its mirrored `OnDie` |
| Scaling: WILDBOAR pool | reserves copy `target siberianN` → up to 3 extra boars per wave on one siberian (100 hp) | **design decision** (PC-7): more players = more boars on the NPC the players must protect |
| `NEED_AMMO_FOR_COMPLETE` | host's ammo only (`0x979f0` → `0x2f530`) | RISK (c-G6) |
| Pilot puppets, PvP, attack targets after G1 | `kFlagRiding` auto (parent = heli id): seated in the local heli, unregistered, no PvP knock | OK by design (0.8) |
| After-cutscene gather / join teleport of an attached pilot | `TeleportToHost` has no parent check; `DistanceToHost` uses the 1 s tracker sample | **RISK after G1** (PC-6): pilot pulled out of its heli after cutscenes 1-3 |
| `PlayerInCutscene()` with the pilot camera | > 4 m camera-to-`+0xE8` = "in cutscene" | UNVERIFIED (c-G1 question): if the gun camera sits > 4 m from the mass centre, every host fire defers forever |
| Cutscenes 1/2/3 | started by the leading heli's own OnHit/BCS; cutscene heli `heli1` uses its own `SetAnim` (no commit needed) | OK on processes whose heli touches; c-G14 cosmetic otherwise |
| `tele` cable car `SetAnim` (`cScrEntity`) | `idle1` in OnActivate (replayed + `kPropAnim`); `ani1` in BCS (host commit if the host's heli touched) | OK |
| Exit `exit_1` | each process runs `ActivateTriggerLevel` from the replayed OnActivate: own `SetMapIsComplete` + code 7 → own load; host `kMapStart` may arrive during the client's own load | RISK (c-G7): a queued `spmap MAP11B` after the client's own load is the same-map replay that crashes at the next teardown (UNVERIFIED timing) |
| Weapon `rifle3_1st` | puppets riding (no gun shown matters little) | COSMETIC |
| Intro movie `imap11a` (54.8 s) on the host's menu path | modal Bink loop | UNVERIFIED (PC-5, R9) |
| F5 | menu hop; G1 still blocks the client after it | OK for the host |

## 4. MAP11B (on foot, Reggie protected, Hogzilla; no `SetMission`; end = exit via `cScrTask09`)

Sequence: `cScrTask01` at spawn (BCS cutscene 1 Reggie `PlayAnim cs03`; OnActivate ammo −35/−10 on each player, Reggie to `start1`, protect Reggie, task 1, arms 02) → `02` (task 2, arms 03; boars 1-3 respawn, wildboar2 → `"hunter"`, 1/3 → Reggie) → `03` (boars 4-6, wildboar5 → `"hunter"`, task 3, arms 04) → `04` (voice only) → **`cScrTask05` armed only by picking up `ammo_rifle2_1st`** (BSP `next trigger`) → `05` (OnHit `UpdateMissionStatus(3,1)`; OnActivate Reggie to `start2`, task 4, arms 10) → **`10` (BCS: escape2, boar timers, `hogzilla1.MoveToPos("hogzilla_1")` + cutscene 2; OnActivate arms 11/06, task 5, `MakeBoss`, Hogzilla `AttackChargers` → Reggie)** → `06` (boars 1-3 respawn, task 6) → `07` (boars 4-5, task 7) → `08` (OnHit `UpdateMissionStatus(3,4)`; arms 09) → `09` (touch near the start, `WaitTask(5)`; `ActivateTriggerLevel("exit_1")`). Mission: race 1, boars 3/3, race 4, Hogzilla 1 (OnEnd `cScrTask11`, empty), boars 3/2; no `CheckMissionStatus`. Ids (VERIFIED in `story-s1-smoke.txt`: `cScrTask01`=80, `cScrTask10`=78; BSP order otherwise): triggers 70-80, boars 82-87, Hogzilla 88, reserves 89-91.

| Beat | Main today | Verdict |
|---|---|---|
| **Hogzilla spawn only in `cScrTask10` BCS** | client-first touch: host runs OnActivate only → host Hogzilla stays parked → task 5 never ends → `cScrTask09` `WaitTask(5)` never passes on any mission | **BLOCKS** (PC-2). F5 only |
| `cScrTask09` `WaitTask(5)` checked on the toucher's mission | client whose mission counted Hogzilla can trigger the exit (request → host `Fire`) | OK/RISK: the host walking in first is refused silently; a player whose mission did not count must let the other walk in |
| Kill credit for Hogzilla | boss fight: the host almost always hit it in that life (`+0x420` lingers) | RISK (PC-1) |
| OnHit-only race completions (05, 08) | HUD only (no `CheckMissionStatus`) | COSMETIC (c-G3) |
| `ammo_rifle2_1st` pickup arms 05 | client pickup → arm request → host arms + commits | OK |
| Reggie protected, targeted by boars and Hogzilla | protected-kill fail via `0x91c20` (hooked) + `cObjGuide` code 6 | OK on the host; RISK on clients (c-G5) |
| Scaling: WILDBOAR reserves copy `target reggie11b` round-robin; Hogzilla health ×1.25/extra | up to 2 extra boars on Reggie at 4 players | **design decision** (PC-7) |
| Hogzilla sync attacks (`wb_`/`back_wb_`/`left_wb_`/`right_wb_syncattack1`) on puppets / clients | host: puppet link (PC-4); client knock clip not in `TEENM\BODY\MAP11B.AP` | RISK (c-G16, PC-4; yeti branch) |
| Hogzilla body states on clients | same model; clips present | OK |
| Cutscene 2: Hogzilla `IdleWait` + `ForceBrainActivate`, Reggie `PlayAnim cs04` `WaitForTask(reggie)` | no animal wait | OK |
| Boar waves t6/t7 reuse boars 1-5 | phantom/invisible second lives | DESYNC (b-G2) |
| `"hunter"` targets (wildboar2/5) | host; retarget covers chases | RISK (hunter owner) |
| Exit | as MAP11A | RISK (c-G7) |
| Stumps, pickups, escape points | props / private / replayed | OK |
| F5 | — | OK |

## 5. MAP11C (helicopter rail shooter; no `SetMission`; end = `CheckMissionStatus` → results)

Sequence: `cScrTask01` (heli; BCS cutscene 1; `Ani1`, task 1 race, arms 02) → **`02` (heli; OnHit `UpdateMissionStatus(3,1)` = race 1)**; OnActivate boars 1-3 → siberians 1/2, task 2, arms 06 → task 2 end → `06` (`CompleteTask`, arms 03) → `03` (boars 1-3 → siberian3, task 3) → task 3 end → `07` (`CompleteTask`, arms 10) → **`10` (heli; BCS boars 1/2 respawn → siberian4 + cutscene 2; OnActivate task 4 (4 boars), arms 04)** → `04` (heli; boars 3/4 → siberian4, arms 09) → task 4 end → `09` (forced; boars 1/2 again, task 5, arms 08) → task 5 end → `08` (forced; BCS cutscene 3 incl. `helicopter.CompleteTask()`; OnActivate **`CheckMissionStatus()`**). Kill tasks 3/3/4/2, all `NEED_AMMO_FOR_COMPLETE`; `cScrFailed` never armed; BSP `exit_1` never used. Ids (UNVERIFIED): siberians 34-37, triggers 40-51 (02=41, 08=47, 10=49), boars 52-55, reserves 56-58.

| Beat | Main today | Verdict |
|---|---|---|
| Pilot player | as MAP11A; `kMissionEnd` goes only to synced peers → an unsynced client never sees the results and never runs `SetMapIsComplete` | **BLOCKS the client** (c-G1); its profile index lags one entry, so no end credits later (0.7) |
| **End needs the host mission to have tasks 1-5 done** (`CheckMissionStatus` without `CompleteTask`) | client kills not counted on the host unless the host hit that boar | **BLOCKS** (PC-1): 12 kills spread over host and clients; any boar the host never touched is missing on the host |
| Race task 1 only in `cScrTask02` OnHit | each heli touches; a client heli that leads makes the host `Fire` it → host OnHit refused → race never done on the host → end fails | **BLOCKS if a client heli leads** (b-G1); b-G5 (0.6) is the way a client heli gets ahead |
| `cScrTask10` BCS respawns boars 1/2 for task 4 (needs 4) | if a client heli leads, host spawns only boars 3/4 | **BLOCKS if a client heli leads** (PC-2) |
| Phantom second lives counted on the client mission | client's forced task-end triggers fire early on the host (heli advances) while the host's task stays open | DESYNC, feeds the end block above (0.1, 0.3) |
| Siberians, scaling, ammo, cutscenes, puppets | as MAP11A | as MAP11A |
| Results screen → next map | host continues; clients get `kMissionEnd` (if synced) and follow `kMapStart` | OK/RISK (flow) |
| F5 | — | OK for the host |

## 6. MAP12A (Wirake protected, Reggie companion, wolves, polar bear, stealth, traps, ice; `SetMission(12)`; end = exit)

Compact (time limit; analysis done, tables shortened). Ids (UNVERIFIED): triggers 158-172 (`05`=162, `08`=165, `09`=166, `10`=167, `11`=168, `12`=169, `13`=170), wolves 176-181, pbear 182, boars 183-184, GWOLF reserves 185-187.
- **`cScrTask08` BCS is the only pbear spawn** (OnActivate only `StartTask`, task 8 → OnEnd `cScrTask13` → `cScrTask09` exit): client-first touch → host bear parked → **BLOCKS** (PC-2). Same BCS sets Wirake `SetCanBeAttacked(0)` and health 10000 (cutscene 4) and parks wolves 3-6/boars: host keeps them when the client went first (DESYNC, Wirake protected → fail risk).
- **`cScrTask05` BCS is the only wolf1 respawn** (task 5 → OnEnd `cScrTask10`): BLOCKS when wolf1 died in task 4 (PC-2).
- Cutscene 2 (wolf1 PlaySA, `cScrTask05`) and cutscene 4 (pbear PlaySA, `cScrTask08`) wait on the animal: client-run → client camera stuck (PC-3) + part links (PC-4).
- OnHit-only `DisableStimulus`/`EnableStimulus` (`cScrTask11`/`12`): DESYNC (c-G3). Stealth meter local: OK.
- Kill credit: tasks 5/8 end through OnEnd triggers whose OnActivate `CompleteTask`s; flee = kill counted on the host only: resilient (PC-1).
- Respawns wolf1 ×4, wolf2 ×3, pbear ×2: DESYNC (b-G2). Wolf sync attacks on puppets (`pl_gw_syncattack1`): host link (PC-4).
- Reserves copy `target wirake12a` (wolf2 template): extra wolves on protected Wirake (PC-7, decision).
- Reggie companion of host (c-G10); Reggie/Wirake `hurtdown` PlayAnim not shown on clients (c-G9); Reggie `shotgun1,map12a` body machine missing on both (log-sweep #21).
- Traps ×10, ice ×8, `trigger_hurt` ×2: per process (c-G17/G15). Cutscene 6 `player.MoveToPos("c5_2")` only where it runs: OK. Exit: c-G7; outro `omap12a` 35.8 s (PC-5). F5 OK.

## 7. MAP12B (Big Grimm; no `SetMission`; end = exit `exit_2`)
- `cScrTask01` is a map-start trigger: BCS bear spawn + cutscene 1 on every process: OK. `MakeBoss` → health ×1.25/extra: OK.
- Task 1 (Big Grimm, flee = kill) → forced `cScrTask02` (`WaitTask(1)`, 4 s) → exit: any mission ending task 1 works (resilient, PC-1). Test R4 uses this map.
- Big Grimm's only SA is `faltree` (trees `obj_move_object` ×2, host only: c-G12 UNVERIFIED); no player sync attack (`bg_` lines commented).
- Players start unarmed; weapons are per-process pickups (each player must find their own). Traps ×5, fall `trigger_hurt`: per process. Target `"hunter"`: host (hunter owner). F5 OK.

## 8. MAP12C (yeti; end = `CheckMissionStatus`) and the game end
- Map-start BCS yeti spawn + cutscene 1 (yeti PlaySA with the guide) on every process: covered by the CutScene thaw / late-join skip on main; clients were still seen stuck (scale.md open 7). Yeti branch fixes it.
- **Yeti fight kills clients on main** (`EngineDll7+0x28b337` / `+0x29a8bf`, scale A/B logs); fixed and verified (2.5 min fight) in `dh2coop-yeti` `83656f1`, unmerged: **BLOCKS** (bosses).
- **End needs the host mission to count the yeti** (`cScrTask02` → `CheckMissionStatus`): BLOCKS if a client lands the kill and the host never hit the yeti in its last life (PC-1).
- Guide is `obj_redshirt`: his death does not fail. Yeti throws host-only (c-G12). Default puppet body = the guide's model (COSMETIC).
- Game end per process (0.7): results → Continue → (profile index 25) Quit + `mod_playendfmv` (64 s) → credits; host `kMapStart(MENUMAP)` can hit a client on its results page or inside the movie (`menumap` from the game menu: UNVERIFIED safe). Clients with a different progression get no credits.

## 9. Ranked list (what stops a 2-4 player run)

BLOCKS
1. **B2 OnHit/BeginCutScene not replicated** (triggers; b-G1/c-G3 + PC-2): MAP9 end (`cScrTask11`), MAP11B Hogzilla, MAP12A tasks 5/8, MAP11C race/wave t4 when a client heli leads. Fix: audit-b G1 design — hook `0x6b140` (prologue `55 8b ec 6a ff` VERIFIED by audit-b), client → `kTriggerHit` (18) request at its OnHit; host runs `0x6b140(trigger, host hunter)` (fallback: disarm `0x68fd0` + script OnHit `+0x208`, `+0x1e8 = 1`) so its own BCS/cutscene/OnActivate run; do it together with b-G5. Tests R1 (MAP9: host `coop_trigger cScrTask03/04/06/08/09/10`, client `coop_tp -2.4:17.5:-182.6`; bug = host `ran 'cScrTask11'` but no `events: host mission message code=5`), R2 (MAP11B: host `coop_trigger cScrTask05` ×2, client `coop_tp 749.7:161.5:-562.0`; bug = host `animals: sync role=host id=88 pos=(-10000…` after `ran 'cScrTask10'`), R3 (MAP12A: host `coop_trigger cScrTask10` ×2, client `coop_tp -126.0:7.6:-27.0`; bug = pbear id 182 parked, client stuck in cutscene 4).
2. **B1 pilot not tracked** (pilot; c-G1): clients excluded from MAP11A/11C, no results on MAP11C, profile lags (no credits). Fix: accept GameDll7 vt `0x109d48` in `player_tracker.cpp` (ctor `0x64660`, Spawn `0x65370` re-verified), keep `combat.cpp` `LocalHunter` as is (pilots are not targets), plus PC-6 and the camera check. Test R5: `-Map MAP11A`; today the client never logs `tracker: local hunter entity` nor `events: synced with host on 'MAP11A'`.
3. **B3 kill credit** (life; PC-1): MAP11C and MAP12C ends; MAP10 task 11 when split. Fix: in `animals.cpp` `OnAnimalHit`, write `int 1` to the host copy's `+0x420` for every client hit with dmg > 0, before `InflictDamage`/`PresentDeath` (as bullet `OnHit` `0x1a680` does); reserves stay excluded by `scale.cpp`. Test R4: `-Map MAP12B`, client `coop_kill 55:100000`; bug = host `scale: mission report action 0 value 26`, no FIRE `cScrTask02`; fix = `action 1`, FIRE after 4 s.
4. **B4 yeti/partner-forward crashes** (bosses): merge `dh2coop-yeti` `83656f1` (MAP12C fight, puppet unlink in `hk_PlaySA`, knock clip fallback for MAP10 `pl_pm_*` / MAP12C `yeti_syncattack*`, stale links dropped before state writes).
5. **B5 client cutscene hang on frozen animals** (triggers + bosses; PC-3): MAP10 cs2/cs4, MAP12A cs2/cs4. Workaround: client presses the cutscene skip key. Fix: deferral cap in `story.cpp` (deferred host fire older than ~20 s while the local camera is on a rail → skip the local trigger's cutscene via `0x6b350`), plus the yeti branch's `FinishPlaySATask` extended to `GoToEntity`/`IdleWait`.

DESYNC
- D1 respawn = new life (life; b-G2/c-G2): phantom local animals + invisible real ones, worst MAP9. Fix: audit-c G2 parts 2-3, and in `hk_MoveToPos` re-freeze/reset a `death_presented` mirror too. Test R6 (MAP9: host `coop_trigger cScrTask04`, `coop_kill 118`, `coop_trigger cScrTask05`; bug = client `hp=0.0` while host `hp=30.0`).
- D2 host `OnActivate` twice (triggers; b-G5): restarts the MAP11A/C heli.
- D3 `TeleportToHost` on attached players (pilot/vehicles; PC-6): skip while `+0xA4 >= 0`, use `SampleLocalNow` in `DistanceToHost`. Test R8: after B1, client `coop_tohost` on MAP11A.
- D4 exits per process + `kMapStart` race (flow; c-G7): MAP11A, 11B, 12A, 12B.
- D5 `cObjGuide` deaths outside `0x91c20` (flow; c-G5/b-G9): siberians, Reggie, Wirake. D6 ammo-gated tasks (flow; c-G6): MAP11A/C.
- D7 `"hunter"` targets/companions (hunter; b-G7/c-G10): merge `hunter-nearest`.
- D8 reserves copying protected-NPC targets (life/scale; PC-7): decision for the user.
- D9 chapter/end movies vs ENet and client game end (flow; PC-5). Test R9: any map, host cmd `mod_playendfmv`; bug = `net: peer timed out` within 64 s.

COSMETIC: NPC PlayAnim beats (c-G9), heli drift / `Ani3a` (c-G14), per-process hazards (c-G15/G17), trees/throws (c-G12), puppet weapons (b-G15), client credits with another progression (PC-5).

## Routing
- **triggers**: B2 (OnHit/BCS replication, MAP9/11B/12A/11C), B5 deferral cap, D2 (b-G5).
- **life**: B3 (`+0x420 = 1` on client hits), D1 (new life + dead-mirror re-freeze), D8 scale interplay (reserves on protected NPCs).
- **beasts**: nothing new in MAP9-12C (no crocs, cobras or tranq here).
- **flow**: D4 exits, D5 guide deaths, D6 ammo, D9 movies + MAP12C game end/credits on clients.
- **vehicles**: D3 (riders share `TeleportToHost`).
- **pilot**: B1 (vt `0x109d48`), D3 for pilots, pilot camera vs `PlayerInCutscene`, heli drift check (c-G14).
- **hunter**: D7 (merge `hunter-nearest`; MAP9/10/12A companions, `"hunter"` targets).
- **bosses**: B4 (merge yeti branch), B5 PlaySA finish, NPC PlayAnim beats (c-G9), trees/throws (c-G12).

## Proposed MEMORY facts
- Kill credit is per process: animal `+0x420` is set to 1 by `cScrAnimal::OnHit` `0x1a680`, reset by the respawn `0x14610` and by present-death `0x1e8b0` after it reports `0x91e80(mission, +0x420, +0x434)`; a host-side `InflictDamage`+`PresentDeath` counts only if the host hit that animal in that life. A client's task end reaches the host as a fire request that bypasses the host's `WaitTask`.
- `cHunterProfile::SetMapIsComplete` `0xec160` advances the profile index `+0x23d8` on every call (cap 25) and sets `bShowEndCredits` (`0x186cd0`) at 25 when `bGameCompleted` (`0x186ccc`) is 0. `mod_playendfmv` = `0xbe480` (plays `Movies\omap12c`, sets the flag); `MAINMENU.SCR` opens the credits when the flag is set; `GAMEMENU.SCR` `CM_NEXT_MISSION` runs Quit + `mod_playendfmv` at the end.
- `EngIoToolPlayMovieInGame` (EngineDll7 `0xaa2b0`) → `0x343a10`: modal Bink loop with its own `PeekMessage` pump, Esc key-up ends it, renders through `render` vt `+0x84/+0x88/+0x90`. Movie lengths: `imap11a` 54.8 s, `omap12a` 35.8 s, `imap12c` 23.0 s, `omap12c` 64.1 s.
- Correction to the MAP4A lioness line: LION, LIONESSE, YETI and every MAP9-12C animal model contain `idleattack1`; "Could not forward animation X from Legs to Legs model Y" is a clip forwarded to a sync-linked partner Y that lacks it (yeti branch `83656f1`).
- MAP10 pumas sync-attack players with `pl_pm_*`, which `TEENM\BODY\MAP10.AP` lacks (it has `pl_p_*`); NPC map paks hold the NPC side of animal sync attacks under the animal's clip name (`pm_`, `wb_`, `yeti_syncattack1..4`).
- `cBodyHelicopter`: `Ani1 → Loop1 → Ani2 → Loop2 → Ani3a/Ani3b → Loop3 → Ani4`; Loops wait for `IsTaskCompleted`; `Ani3a` never leaves its state; `Loop3`/`Ani4` are entered only from cutscenes 2/3 (`BSMGoToState`).
- Map-start triggers (`Activate(); ActivateAfter(0)` in `Spawn`) run `BeginCutScene` on every process; other triggers run it only where their own player touched.
- BSP-placed entity ids equal the BSP lump index (MAP11B `cScrTask10`=78, MAP12B `cScrTask01`=51, MAP12C `cScrTask01`=22); `AddAnimal` animals follow in script order (MAP12C yeti = 24).

Not done in this session (usage limit): runtime tests R1-R10, per-beat line numbers for MAP12A-12C, verification of `render` vt `+0x90` = Flip.

