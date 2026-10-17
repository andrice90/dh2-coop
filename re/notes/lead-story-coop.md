# Story mode in co-op: lead design draft (2026-10-16 20:30)

Inputs:
- `re/notes/story-inventory.md` (worker, 2026-10-16, gitignored because it quotes ~2000 script lines). Regenerate it with `re\scripts\story_inventory.py`.
- `re/notes/lead-mission.md` (mission end, claim).
- `re/notes/lead-combat.md` (attack list).

## What the story levels are (inventory facts)

- **Maps.** 25 story/bonus maps. Each `MAIN.SPJ` pulls a world script, a trigger script, a mission script and cutscene scripts. Script source is plain text (`SCRIPTS\TRIGGERS\TRIGGERMISSIONxx.SCR`, `MISSIONS\MISSIONxx.SCR`, `CUTSCENE\MAPxx\*.SCR`).
- **Missions.**
  - Tasks are `m_pMission.CreateTask` blocks. Scripts advance them with `UpdateMissionStatus`, `CompleteTask`, `ActivateTriggerOnEnd` / `ActivateTriggerOnFail`.
  - The map ends through cMission. This is already co-op through the mission-message hook (`kMissionEnd`).
  - No script loads a map or a checkpoint. Progression is C++ (results screen → menu/next map).
- **Cutscenes.**
  - 74 referenced, all started from trigger scripts with `ExecuteScript("cScrCutSceneN")`.
  - Inside one: `player.SetPFlags(PLAYER_INCUTSCENE)`, `ActivateCutSceneMode`, camera `BeginCamera`/`EndCamera`, HUD off/on, NPC tasks (`StartTask`/`MoveToPos`/`WaitForTask`), `PlayMessage` subtitles, `Wait`.
  - Every cutscene has a `Skip` state.
- **No QTE/struggle code in the scripts.** There are knife/`Pumafight` anim states, so melee is C++ or animation-driven.
- **NPCs.** 19 of 25 maps have an NPC/custom-character instance (guide, companions).
- **"hunter" / player: 1581 script lines.**
  - player flags 419;
  - task target 341 (`SetTaskParam("target","hunter")`);
  - trigger tests on the player 317;
  - camera/cutscene 222;
  - bind player 98; player methods 98; inventory/weapon 61; damage/health 13; distance 8; teleport 4.
- Simplest first maps by count (no/few cutscenes, NPCs, player lines): **MAP3B**, MAP12B, MAP12C, MAP4A, MAP11B. Hardest: MAP8, MAP6, MAP2A, MAP10, MAP12A.

## Model: "host decides, everyone runs the scripts"

Re-simulating the whole world on clients from snapshots (every NPC, prop, camera and HUD state) is too much surface. Instead:
- **Every process runs the same level scripts** (HUD, hints, cutscene cameras, local player flags, inventory all stay local and correct).
- **Only the host makes decisions.** A decision is anything that branches the script flow:
  - which trigger fired;
  - task completed or failed;
  - mission end;
  - an animal/NPC died;
  - trophy claimed.
- **Clients replay the host's decisions** through `GameEvent` kinds. Their own decision points are swallowed while online.

| Decision point | Host | Client | Kind |
|---|---|---|---|
| Mission end (message `0x91c20`) | runs, commits | swallowed; applies commit | `kMissionEnd` ✅ |
| `CheckMissionStatus` (`0x94e50`) | runs | skipped | — ✅ |
| Trophy claim (`0x1cd40`) | runs; runs clients' requests | request | `kTrophyClaim` ✅ |
| Map start | announces | follows (`spmap`) | `kMapStart` (built, untested) |
| **Trigger activation** (`cScrTriggerEvent`; `ActivateTrigger` native `0x84a0` → `0x68eb0`) | fires when **any** player satisfies it; commits the name | own player-driven activations swallowed; activates by name on commit | `kTrigger` (new) |
| Task complete/fail (`CompleteTask` `0x95840`, task state 5/6) | runs, commits | swallowed; applies | `kTask` (new; maybe not needed if triggers replay) |
| Cutscene skip | host's skip ends it for everyone (or each player watches fully; decide) | — | `kCutsceneSkip` (later) |
| AI entity death | host sim | present death | `kAnimalDied` ✅ (extend to NPCs) |

**Continuous state stays on snapshots:** players (puppets ✅), animals (mirror ✅), plus NPCs: generalize the animal mirror to every brain-driven character (guide, companions). The same recipe applies: freeze brain, pre-render HandleKey, body-state names.

## Trigger layers (static analysis 20:35)

- **L1 volumes: `cObjTrigger` (CommonDll7).**
  - `CheckPhysics` is empty; entry detection comes from the engine's physics overlap.
  - `cObjTrigger::OnActivate(trigger, activator)` `0x4dee0` (virtual; `cObjTriggerHitCont::OnActivate` `0x4f450` dedups activators and calls it):
    - if the trigger's state name is not `"Activated"`, and mode `+0x1d8` is 1, or mode is 2 and `activator` vt `+0xfc` is non-zero;
    - then it runs the script callback (`g_pClassManager` vt `+0x28(+0x1dc script idx, +0x1b4, activator +0x1b4)`) and sets state `"Activated"` (vt `+0x17c`). One-shot.
- **L2 script trigger events: `cObjTriggerEvent` (GameDll7 class name at `0x162c88`), the scripts' `cScrTriggerEvent trigger_N`.**
  - Activation is `0x68eb0` (fastcall trigger): `+0x1e4 = 1`, clears `+0x1fc`/`+0x1e8`, registers with the mission (`0x92100(g_mission, trigger)`).
  - Reached from the `ActivateTrigger(name)` native (`0x84a0`, which looks up by target name `+0x198` via `0x6d80`) and from the tasks' `ActivateTriggerOnEnd`/`OnFail`.
- **Proposed sync: "same inputs → same script flow".**
  - L1 is the input from player positions. Sync it.
    - A client's local `OnActivate` (its own hunter entered) is swallowed and sent as a request (`kVolume`, subject = trigger entity id; BSP entity ids match across instances like the animals).
    - The host runs `OnActivate(trigger, hostHunter)`. Pass the host's hunter so script checks on "the player" pass.
    - The host commits every L1 activation, its own included. Clients run the original `OnActivate(trigger, localHunter)`, so the same scripts run everywhere.
  - L2 then follows from the scripts on each process.
  - Fallback if L2 diverges: also commit/swallow `0x68eb0` the same way.
  - Runtime check first: log L1 and L2 activations on a solo MAP3B play-through (user plays, we log) to see the real sequence.

## S1 results (2026-10-16 21:18–21:30, MAP3B, two instances)

- **Baseline (21:18).** Both instances load MAP3B, join, see each other's puppet, mirror its 2 animals (leopards, entity ids 47/48), and arm the same map-start trigger events.
- **First user play (21:20), before trigger sync.**
  - The host walked into trigger zones: `cScrTask03`/`04` were armed on the host only. The leopards came in on the host.
  - On the client they were positioned correctly (mirror drift 0) but invisible, because the client's script branch never ran.
  - The host then crashed when a leopard attacked the client's puppet (`ProcessAttackStuck`, puppet has no physics). Puppet attack targets are now off by default.
  - The knife showed as a shotgun on the puppet (kind "other"). It now shows unarmed.
- **Trigger sync (21:28, commit `a6698de`).**
  - Arming (`0x68eb0`) and firing (`0x68130`) are host-decided. The host commits `kTriggerEvent` (5) / `kTriggerFire` (6) with the entity id and name. Clients replay them by id; a client's own arm/fire becomes a request.
  - Log: host fires `cScrTask02` → arms `cScrTask03` → fires `cScrTask03` → arms `cScrTask04`. The client applied each within 4 ms.
  - **User: "leopards aggroed and interacted with both players."**
- **Not handled yet:**
  - Cutscenes (`ExecuteScript("cScrCutScene…")` runs in fired OnActivate on both sides, so they should play on both; skip/timing not synced).
  - `CompleteTask` / task HUD (runs on both via the same scripts; host-only completion not enforced).
  - Player damage from host animals to client players (clients are effectively invulnerable).
  - NPC mirror.
  - Player death policy.
  - Map transition after a story mission success.

## Open engineering questions (next RE, lead)

1. **Trigger activation path.**
   - How does a `cScrTriggerEvent` volume test "player inside"?
   - Where is the single activation function to hook? Candidates: `0x68eb0` (from `ActivateTrigger`), `cScrTriggerEvent::CheckMissionStatus` `0x6a1a0`.
   - Needs a hook that lets the host count puppets as players, and clients suppress local activations.
2. **Cutscene timing.** Scripts use `Wait`/`WaitForTask`. If both processes run the same cutscene from the same trigger commit, they stay roughly in step. A skip on one side diverges. Policy needed.
3. **Player death** in story: what the engine does, and the co-op policy (fail for all vs. revive). User decision.
4. **NPC mirror.** Do guide/companion brains tolerate `FreezeBrain` on clients? Do scripted NPC dialogue (`PlayMessage`) and tasks run fine with a frozen brain?
5. **Starting a story map.** Does `spmap MAP3B` set the mission context correctly? Mission scripts call `SetMission(n)` themselves (MAP1: `SetMission(1)`), which suggests yes. Check that the client's results/profile progress (`SetMapIsComplete`) unlocks the next map in the client's own campaign.

## Plan

- **S0 (now):** finish hunt maps (map flow test, P5 combat), because they share all of the infrastructure.
- **S1:** RE the trigger activation path (lead). Then a spike on **MAP3B** with two instances: does the client follow, do triggers mirror, does the mission end on both?
- **S2:** NPC mirror (generalize animals).
- **S3:** cutscene policy, death policy, then the harder maps.
