# Wave "playthrough" (2026-10-18): clean co-op story run MAP1 → MAP12C

User goal: 2–4 players finish the story from MAP1 to the end without a soft lock, a crash, or a desync that stops someone. Last session's MAP4A work (truck chase) taught us new classes of problems; this wave looks for the same classes on every story map, fixes them, and tests what can be tested. Testing every mission end to end is not feasible: research specific sections, then run them.

Read first: `re/jobs/_claude-rules.md` (ground rules, they all still apply), `re/notes/coop-checklist.md`, `STATUS.md` top sections, `MEMORY.md` (especially "Vehicles and riders", "Main menu", "Story triggers"), and the old audits `re/notes/audit-maps-a.md` (MAP2B–4B), `audit-maps-b.md` (MAP5–10), `audit-maps-c.md` (MAP11A–12C). The audits were written before the MAP4A work: some gaps are fixed now (below), most are still open.

Story order: MAP1, MAP2A, MAP2B, MAP2C, MAP3A, MAP3B, MAP3C, MAP4A, MAP4B, MAP5, MAP6, MAP7, MAP8, MAP9, MAP10, MAP11A, MAP11B, MAP11C, MAP12A, MAP12B, MAP12C.

## What MAP4A taught us (look for these everywhere)
1. **Per-process simulation of moving world objects diverges.** Each process drove its own truck; the host's derailed while the client's stayed on track. Fix pattern (`mod/src/coop/vehicles.cpp`, protocol v4 `VehicleStates` msg 11): the host streams the chassis pose at ~30 Hz; clients `Freeze` their copy, clear the drive bit, follow the pose with 100 ms interpolation on the host clock, and carry child bodies (wheels, both extra body states at `*(+0xc0)`/`*(+0xc4)`). Any scripted mover that is not an animal (vehicles, helicopters, boats, cable cars, cages, falling trees, rolling rocks) needs the same question: who drives it, and does the copy stay in step?
2. **Attached entities.** A seated hunter has `+0xA4` = vehicle id. `LocalPlayer().pos` lags 2.5–5 m at speed; use the entity's `+0xE8`. Riders' puppets are seated in the *local* copy (`kFlagRiding`, collision/gravity/velocity held at zero every frame).
3. **Shared spots break player-vs-player rules.** In one truck every player reported the same seat of their own copy, so every shot "hit" someone (100% knock-over). Knock-over is now off while riding.
4. **Replayed `MoveToPos` wakes frozen client brains** (`0x14610` clears brain `+0x79`). Fixed in `animals.cpp`: a hook on `0x14610` re-freezes at once and a per-frame wake check backs it up. Check that this covers every animal class you meet (crocodiles, cobras, NPC `MoveToPos` thunks) and that a respawn is a *new life* (still open: audit-a G4, audit-b G2, audit-c G2).
5. **Forced body states the model lacks kill the script VM.** The MAP4A lioness has no `idleattack1`; hundreds of failing `SetBodyState` per second ended in `cVMachine::SysMain` faulting. Now each failing state is tried 3 times and then left (the copy can stand in its last pose). Other models lack clips too (yeti, `indiangirl`, `poacher`, `ausranger3`, girl/fiancée NPCs).
6. **Client trigger state-2 spin** (~6000 FIRE lines per ride): fixed (`story.cpp` sets `+0x1fc = 1` when this process already ran the fire).
7. **Who the animals go for.** Riders are not attack targets on MAP4A (a lion tackling a puppet in the truck caused the stuck lioness), so every lion goes for the host; an unattended host died twice (host death = mission failed). Player-count scaling (`scale.cpp`, `[Scale]`) adds pack animals per extra player, so aggro distribution matters more.
8. **Kill credit.** Client kills reach the host's mission as "action 0", so kill tasks never count a client's kill (found by the scale agent; proposed fix `+0x420 = 1` before the death in `animals.cpp` `OnAnimalHit`, untested). If a client kills the last required animal, the task never completes: soft lock.
9. **Recovery tools exist.** F5 twice = host restarts the level through the main menu, F6 twice = next story level, clients follow (`coop.cpp` `HostRestartLevel` / `HostNextLevel`). There are no checkpoints inside a level.

## Already in main (do not redo)
Trigger arm/fire sync (`story.cpp`, kinds 5/6) with "ran here" decisions; cutscene play, deferral during a client cutscene, late-join skip, gather after a cutscene; `PlaySA` and prop `SetAnim` replay (7/8); mission end through `0x91c20` (2); trophy claim (3); map start and follow, also from the main menu (4); late-join catch-up (9/10); combat: animals attack every player, nearest-player retarget, `kPlayerHurt`/`kPlayerKnock`; NPC mirror (`kNpc`, `CharacterBrainFreeze`, profile health); cobras mirrored; shootable props (16); host notice (17); scaling (40); MAP4A truck sync (msg 11), riders, `MoveToPos` re-freeze, failing-state limit, state-2 spin fix; lobby; F5/F6.

## Roster (one owner per file; a second agent touching a file keeps the edit small and says so in its report)
| Agent | Worktree / branch | Port | Ghidra slot | Owns |
|---|---|---|---|---|
| research-A (MAP1–MAP3C) | main checkout, notes only | – | A | `re/notes/playthrough-a.md` |
| research-B (MAP4A–MAP8) | main checkout, notes only | – | B | `re/notes/playthrough-b.md` |
| research-C (MAP9–MAP12C) | main checkout, notes only | – | C | `re/notes/playthrough-c.md` |
| triggers | `D:\scary hunt\dh2coop-trig` / `triggers` | 27610 | T | `story.cpp`, the journal in `events.cpp` |
| life | `D:\scary hunt\dh2coop-life` / `journal-life` | 27620 | J | `animals.cpp` (host tick, mirror, deaths), kill credit, `scale.cpp` interplay |
| beasts | `D:\scary hunt\dh2coop-beasts` / `beasts` | 27630 | K | new `beasts.cpp`, `Animals()` filter in `player_tracker.cpp`, claim commit in `events.cpp` |
| flow | `D:\scary hunt\dh2coop-flow` / `mission-flow` | 27640 | D | new `flow.cpp`, map start in `events.cpp`, mission/exit hooks |
| vehicles | `D:\scary hunt\dh2coop-truck` / `trucks` | 27650 | N | `vehicles.cpp`, riding in `puppets.cpp` |
| pilot | `D:\scary hunt\dh2coop-pilot` / `pilot-maps` | 27660 | P | local-player detection in `player_tracker.cpp`, new `heli.cpp` |
| hunter | `D:\scary hunt\dh2coop-hunter` / `hunter-nearest` | 27670 | M | `npc_follow.cpp`, `hunter_nearest*`, attack-target rules in `combat.cpp` |
| bosses | `D:\scary hunt\dh2coop-yeti` / `yeti` | 27680 | W | boss code, NPC PlayAnim beats |
| smoke | `D:\scary hunt\dh2coop-smoke` / `smoke-pass` | 27690 | G | smoke scripts, `re/notes/smoke-all-maps.md` |

The lead (main thread) owns `coop.cpp`, `protocol.*`, `session.*`, docs, merges, and uses port 27425 / slot L.

## Numbers (pre-allocated so branches do not collide)
- `events.h` kinds: triggers 18–19 (`kTriggerHit` = 18), life 20–21 (`kAnimalRevive` = 20), beasts 22–23, flow 24–27 (`kLevelExit` = 24), vehicles 28–29, pilot 30–31, hunter 32–33, bosses 34–37. `kScaleMax` stays 40.
- Protocol message ids: pilot 12, vehicles 13, bosses 14, flow 15. Do not bump `kProtocolVersion` (the lead bumps it at merge; the build id check already refuses mixed builds).
- `PlayerState` flags in main: `1<<0` crouched, `1<<1` reloading, `1<<2` firing, `1<<3` riding, `1<<4` dead, `1<<5` knocked. Free bits start at `1<<6` (corrected 2026-10-18; pilot used none). New `PlayerState` fields: only flow (ammo), appended at the end.
- Console test commands: prefix `coop_`; check `grep -rn AddCommand mod/src` for name clashes.

## Testing conventions
- Build `.\mod\build.ps1`, two windows `.\run-coop-local.ps1 -Map <MAP> -Port <yours>` (`game` = host, `game2` = client, created by the script). `-Players 3/4`, `-LateJoin <s>`, `-Seconds <s>`, `-Capture` exist; `-Lobby` starts at the main menu.
- Drive with one console line per write into `<tree>\Bin\dh2coop.cmd`, ~1.2 s apart. `coop_trigger X` arms, or fires an armed trigger, and **skips `OnHit` / `BeginCutScene` even on the host**; use `coop_tp x:y:z` into the volume for real flows. Positions in BSP are cm (÷100).
- MAP1 tutorial hint pages are modal and pause that process until RETURN (`coop_key enter`); after teleporting into `cScrTask03` expect page 3 after the cutscene, then page 2.
- Real keyboard/mouse input only reaches the window in front; do not send it. Add a `coop_*` command when a test needs an action.
- `game\Bin\dh2coop.log` / `game2\Bin\dh2coop.log`, `debug.log`; each launch rotates (`.1/.2`).
- Close only your own games by full path. At most one co-op session of yours at a time.
- Ghidra: run `D:\scary hunt\dh2coop\re\scripts\ghrun.ps1 -Slot <yours>` / `decomp.ps1` from the main checkout (worktrees have no Ghidra project); extracted scripts are in `D:\scary hunt\dh2coop\re\extracted\`.

## Report
Note `re/notes/<agent>.md` (research agents: `playthrough-a/b/c.md` in the main checkout): per map what you checked, what you fixed, evidence (log lines, screenshots under `re/notes/coop-local/<agent>-*`), VERIFIED / UNVERIFIED, open questions, "Proposed MEMORY facts". Final message to the lead: commits (hash + subject), per-map verdict table, what needs the user (a real two-PC test, a visual verdict, a design decision).
