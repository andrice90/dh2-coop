# hunter-nearest: "the player" means every player (job `hunter`, wave playthrough, 2026-10-18)

Worktree `D:\scary hunt\dh2coop-hunter`, branch `hunter-nearest`, port 27670, Ghidra slot M. Stopped early at the
lead's request (usage limit): sections "Open" and "Next steps" say exactly where to resume.

## Commits (on main `a1e9212`)

| Hash | Subject |
|---|---|
| `aa726ad` | Host: "the hunter" is the nearest living player for scripts and AI ([Story] HunterNearest, default off) |
| `a282923` | hunter_nearest: keep "no attack" when a script made the player unattackable; coop_npcinfo |
| `46217d4` | npc_follow: coop_npcinfo lists the queued commands; coop_npcclear <id> drops them (the earlier agent's uncommitted change) |
| `11533a9` | Host: puppets follow the host hunter's SetCanBeAttacked; puppets get the hunter's sync-attack deny time |
| `dfff32a` | Host: spread attackers over the players (load-aware nearest-player retarget) |

Rebase: the brief said `a3322d1`; main moved four times while I worked, so the branch sits on `a1e9212` (puppet
parking). Conflicts: `dh2coop.ini` (kept `[Sound]` and `[Story]`), `coop.cpp` (both command lists), `dinput_hooks.*`
(kept `SetUiCapture` and `QueueKeys`), `run-coop-local.ps1` (took main's: it already has `-Set`), `puppets.cpp`
(main's park/reuse kept; riding + posture/shots both kept). No conflict in `combat.cpp` or `scale.cpp`. With
parking: `park_puppet` now calls `hunter_nearest::OnPuppetRemoving` (stimuli, NPC commands), and a reused parked
body is renamed with `NamePuppet` for its new player.

## What the code does now

- `[Story] HunterNearest` (default **0**, unchanged): `SetTaskParam("target"/"spooker","hunter")` on animals/NPCs
  names the nearest living player (`coop_p<id>`); `GetDistBetween2Ais` nearest on HUNTMAP*; puppets get presence /
  gunshot stimuli. `NpcFollowNearest` defaults to the same value (mid hook `GameDll+0xad098`).
- `[Combat] FollowHostAttackable=1` (new, combat.cpp + one line in puppets.cpp): the host reads its hunter's `+0x644`
  (script `player.SetCanBeAttacked`, GameDll `0x49ec0`) every frame; puppets are added to / removed from the attack
  list with it, late joiners included. Pilot (MAP11A/11C, other vtable) is not read: always attackable there.
- `[Combat] PuppetDenySA=1` (new): hook on `cObjCharacterEx::CanPlaySA` (CommonDll `0x14a60`, returns 1). After a
  puppet's sync attack ends, the host refuses a new one for the host hunter's deny time (`+0x67c`, written by
  `SetDenySATime` `0x4bf60`; spawn value 15 s). Facts: hunter `OnEndSA` `0x53cd0` copies `+0x67c` to the countdown
  `+0x680`; the hunter's own CanPlaySA is vt `+0x218` = GameDll `0x485e0` (0 while `+0x680 > 0`).
- `[Combat] AttackerSpreadMetres=4` (new): retarget every 0.5 s scores each player in the attack list as
  distance + 4 m per other animal on that player; switch when 3 m better (own entry not counted, 2 s rest).
- Test commands: `coop_targets` (each animal's target, count per player, attack list size, host attackable),
  `coop_npcs` (NPC id/name/pos/flags/queue, host distance, nearest player). npc_follow logs every change of an
  NPC's player (`npc_follow: NPC id N 'name' now follows player P`).
- `SetTaskParam` is hooked twice at CommonDll `0x1f190`: hunter_nearest (OnEngineReady, inner) and scale
  (OnGameDllLoaded, outer). Scale records the script's raw `"hunter"`; a reserve's replay goes through the
  substitution at the reserve's own position (log `animal id 160 task 'target' = player 1`). Works as is.

## Results

| Map / test | Evidence (`re/notes/coop-local/hunter-*`, gitignored) | Verdict |
|---|---|---|
| MAP3C Abby defense (G8), 2p | host `cScrTask02` fire 10:01:57 → `puppets: attack target remove … (list 1)`, `combat: the host's hunter cannot be attacked now`; `coop_targets`: lion1, lion2, `coop_LIONESSE_0` all → `abby3c`; no `hurt player 1` with the client 3 m from Abby (`hunter-m3c-g8-*`) | VERIFIED |
| MAP3C late joiner | client joined 20 s after the fire: puppet named, never `attack target add`; list stays without it | VERIFIED |
| MAP3C Abby dies ~5 s after the fire with nobody shooting | same with `Scale.Bodies=0` (2 lions, no reserve): Abby `Liedead` 6 s after the fire. Retail behaviour, not scaling. Mission end after Abby's death is G12 (flow) | VERIFIED (A/B) |
| MAP2B wolf siege (G8), 2p | `cScrTask08` fire 10:06:51 → puppet removed (list 1), all wolves + reserve on `hugs2b`; 10 `coop_killshot` deaths later the script's `NoChange` → 10:08:08 `attack target add` (list 3), `can be attacked now` (`hunter-m2b-siege-*`) | VERIFIED both directions |
| MAP2A wolves, 3p side by side (aggro + scaling) | wolf1 `target hunter` → player 1 (nearest, 5.3 m); reserves 160/161 released; spread `switches to player 2 … from player 1`; final `coop_targets` host 1, player 1 1, player 2 1, Hugh 1 (`hunter-m2a-3p-aggro-*`). Clients took hits (`combat: took …`) | VERIFIED |
| MAP2A Hugh (guide) | npc_follow switches Hugh's player (`now follows player 1/2/0`), but no visible movement test done (host 35 m away, client 8 m: Hugh stood at PathNode2, queue=1) | UNVERIFIED |
| MAP8 tribesmen, MAP9/10 Reggie, MAP12A Reggie companion | not run. The earlier agent's MAP9 run: Reggie "follows player 1 (38.5 m)" for a minute without moving (distance unchanged), queue=1 | UNVERIFIED / likely not working |
| PuppetDenySA | hook installed; `combat: puppet CanPlaySA (vt +0x218) is CommonDll7.dll+0x14a60 (the hooked base: deny time works)`; no sync attack on a puppet happened in these runs | UNVERIFIED (effect) |
| HUNTMAP4 (earlier agent, before the rebase) | client spook, trophy walk-up, puma attacks the client, claim, success | VERIFIED then; not re-run after the rebase |

Side findings for other owners:
- MAP2A: a client entered `cScrTask02` first (`ran 'cScrTask02' for player 2`), then the host walked in and ran its
  own `BeginCutScene` + a **second `OnActivate`** 1.1 s later (`[host] trigger FIRE id=118`): wolf1's target set
  twice, scale saw a second wave. Owner triggers (a-G6 / X1).
- A reserve released "like wolf2" (`target hugs2a`) ended on a player: the engine's own stuck retarget
  (`ProcessAttackStuck` → closest list entry) moves animals off NPC targets; not our code.

## Defaults

Unchanged: `HunterNearest=0` (so `NpcFollowNearest=0`). Recommendation for the lead: the task substitution is
safe (MAP2A/MAP3C/MAP2B, no fault), but companions are not shown to work, and `NpcFollowNearest` follows
`HunterNearest`. If you want the task part on, set `HunterNearest=1` **and** `NpcFollowNearest=0` explicitly.
New `[Combat]` keys default on: `FollowHostAttackable=1` and `AttackerSpreadMetres=4` (verified),
`PuppetDenySA=1` (low risk, effect unverified; set 0 if in doubt).

## Open

1. **Companion follow does not move the NPC (likely).** `cAIBrain_NPC` (vt `0x112d30`, tick `0xad050`) only
   queues a new follow when its command queue is empty, or holds one entry with `+0x30 == 0`. Queue layout
   (decompiled `0xa5910`/`0xa5f40`): count = `*(q+4) + 1`, entries `0x88` bytes at `*(q+0x10)`; entry `+4`
   priority, `+8` task mask, `+0x18` task object (task `+0x3c` = mask id, task `+0x10` = target entity), `+0x1c`
   target, `+0x30` keep. `coop_npcinfo`'s item dump assumed `*(q+4)` = array: wrong, fix it to the layout above.
   A companion keeps its current follow command (toward the host) until that task ends; the far branch
   (`+0xb68`, 8 m) is only read with an empty queue. Fix idea: when the picked player changes (with a margin),
   write the new entity into the follow task (`entry+0x18` → task `+0x10`) and `entry+0x1c`, or drop the brain's
   own entries (`FUN_100ab7c0(brain, nullptr, oldTarget)` clears entries that point at an entity).
2. Guides (Hugh MAP2A/2B, Abby MAP3A): hello/wait uses the substituted player; check with the host far behind and
   a client at the guide (watch `animals: id=<npc> state=` for a hello/wave state and the path progress).
3. `GetDistBetween2Ais` stays HUNTMAP-only (story maps use it for each process's own stealth HUD).
4. Fold the two `SetTaskParam` hooks into one if the lead prefers (order today is deterministic).

## Next steps (for whoever resumes)

1. Fix `coop_npcinfo` items to the `0x88` layout; run `-Map MAP12A -Set 'Story.HunterNearest=1'`, client
   `coop_tp` 30 m from Reggie (`coop_npcs` gives his id), `coop_npcinfo <id>` every few s: does the follow entry's
   target stay on the host? Then implement idea 1 and re-test MAP12A (audit-c G10), MAP8 (`coop_trigger cScrTask08`
   then client 40 m away, audit-b G7), MAP9.
2. MAP2A Hugh with the host at spawn and a client walking the path (PathNode1 -142.3:38.6:129.4 →
   PathNode2 -141.3:38:120.5 → PathNode3 -140.9:37.9:112.7).
3. PuppetDenySA: MAP2A wolves on a client until `combat: sync attack 'pl_gw_syncattack1' on player 1`, then look
   for `player 1 is not pounced again yet` within 25 s (MAP2A sets `SetDenySATime(25)`).
4. Re-run HUNTMAP4 once after the rebase.

Helpers used: scratch `hn.ps1` (start/cmd/grep/stop by full path), BSP entity lister (tr + awk over
`re/extracted/BSP/<MAP>.BSP`, `property("position", …)` lines, cm ÷ 100).

## Proposed MEMORY facts

- `player.SetCanBeAttacked(n)` = GameDll `0x4bf80` → `0x49ec0`: hunter `+0x644 = n`, then
  Add/RemoveAttackTarget on that hunter only. Puppets now follow `+0x644` on the host (VERIFIED MAP3C, MAP2B).
- `SetDenySATime(t)` = GameDll `0x4bf60`: hunter `+0x67c = t` (ctor 15.0). Hunter `OnEndSA` `0x53cd0` copies it to
  `+0x680`; the hunter's CanPlaySA (vt `+0x218`, GameDll `0x485e0`) returns 0 while `+0x680 > 0` (and while
  `+0x648 == 0`). Base `cObjCharacterEx::CanPlaySA` (CommonDll `0x14a60`) returns 1; puppets use it (runtime
  vt check). `SetMaxAttackerNo(n)` (`0x7a20`) writes byte `+0x34` of the attack interface.
- NPC brain command queue: count `*(q+4)+1`, entries `0x88` bytes at `*(q+0x10)` (`0xa5910`), entry `+0x18` task
  object (task `+0x3c` = mask id), `+0x30` keep; `0xa9ed0` finds a kept entry by mask, `0xa85f0` the task by mask.
- MAP3C Abby (30000 profile health, lions `maxdmgval 20000`) dies ~5 s after `cScrTask02` when nobody shoots, with
  or without the scale reserve.
