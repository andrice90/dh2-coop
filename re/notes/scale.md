# Player-count difficulty scaling (job `scale`, 2026-10-17)

Built from `re/notes/player-scale.md`. Code: `mod/src/coop/scale.cpp|h` (new module, priority 49), plus small edits in
`coop.cpp|h` (`ConnectedPlayers()`), `events.h` (`kScaleMax = 40`), `animals.cpp` (one-shot log when the snapshot is
full) and `mod/dh2coop.ini` (`[Scale]`). Branch `scale`, port 27520, Ghidra slot S.

`extra = ConnectedPlayers() - 1`, counted on the host only. Solo, and a host with no client, play the retail fight.

## What was built

1. **One body, more health.** A `MakeBoss(TRUE)` animal (yeti, hogzilla, big grim) or a large animal that starts a
   scripted attack alone gets `+0x4C0 = base * (1 + 0.25 * extra)` and `+0x19C` scaled by the same factor. `base` is
   `+0x4C0` before the first scaling (only spawn writes `+0x4C0`). A join or leave rescales max and current health by
   `newFactor / oldFactor`: no refill, no heal. The host commits every new max (`kScaleMax`, journaled, key
   `"<factor> #<serial>"`); a client writes that max into its own `+0x4C0`, so its boss bar divides by the same max.
   A later `SetHealth` on a scaled animal gets the same factor on every peer.
2. **Extra bodies.** Right after the map script's `AddAnimalOnMap` returns (GameDll `0x8cab0`), every peer calls the
   game's own `AddAnimal` for `Reserves` (3) parked copies of each pool species of that map, named
   `coop_<SPECIES>_<n>`. Same order on every peer, so the ids match. The host keeps `extra` of them in the fight while
   a script animal of that species family is on its `AIHuntTask_Attack*` task: it tops up when the family's attack
   begins and when a later trigger sends in a fresh attacker (not a retarget). A release is the script `MoveToPos`
   path (entity vt `+0x158(3,0)`, vt `+0xA4(position)`, angle) next to an attacker, then that attacker's
   `CustomProps`, `StartTask` and `SetTaskParam` calls (round-robin over the attackers). Reserves keep their species
   health. When the script `Kill()`s the last up animal of a family, its reserves are parked too. Reserve deaths are
   kept out of the mission (`0x91e80` skipped while a reserve's present-death `0x1e8b0` runs) on every peer.
   Clients never release: the snapshot carries the reserve, and the client gives it the `MoveToPos` body reset once
   (collision back on) and freezes its brain again.
3. **Skips.** Hunt maps (`HUNTMAP*`) and the menu: nothing at all. Bosses: health only. Crocodiles: never bodies.

## Corrections to the research note

- **Order of a trigger's calls is BeginCutScene first, OnActivate after** (`0x68800`: state 4 runs `+0x210` and
  releases it; `ActivateHUD`/`DeactivateHUD` move 5 → 3 → 2; state 2 calls `0x68130`). `re/notes/audit-maps-b.md` 0.1
  has it right; `player-scale.md` has it backwards. Consequences:
  - MAP2B `cScrTask03`: the cutscene `MoveToPos` restores the grizzly to 350 first, then `OnActivate` runs
    `Make_Scared` (`SetHealth(10000)`, attack `time 5`). The MAP2B grizzly is a scare, not a fight: with 2 players its
    max is 437.5 and its current health 12500 (retail: 350 / 10000). The real grizzly fight is MAP2C `cScrTask08`
    (437.5 / 437.5 with 2 players).
  - MAP12C: `MoveToPos` (cutscene) runs before `MakeBoss` (`OnActivate`).
  - So "inactive just before StartTask" cannot detect a wave (MAP2A wolf1 is placed by `BeginCutScene` before its
    `StartTask`). A wave is the attack `StartTask`s of one frame, evaluated after the frame; an animal "is attacking"
    while its brain still runs that task (brain vt `+0x40` = `IsTaskActive(name)`, the call `StartTask` itself uses).
- **No `MoveToPos` (`0x14610`) hook is needed.** It copies `+0x4C0` into `+0x19C`, and `+0x4C0` already holds the
  scaled max. VERIFIED: MAP2C bear 437.5 after `OnActivate`'s `MoveToPos`; MAP12C yeti max survives. So there is no
  overlap with `journal-life`'s planned `0x14610` hook.
- **SetHealth** keeps retail meaning (current health only); a scaled animal's new value gets the factor. The research
  note wanted `SetHealth` to rewrite `+0x4C0` and `baseMax`; that would make `Make_Scared`'s 10000 the max that a later
  `MoveToPos` restores, which retail never does.
- **Reserves are created from `0x8cab0`, not `OnMapLoaded`.** `OnMapLoaded` fires when `cBspTree::Load` returns,
  before any entity exists; `0x100067d0` then runs `AddAnimalOnMap` (`0x8cab0`), `AddMapEvent`, `AddMatFile`,
  `InitGlobals`. Hooking the end of `0x8cab0` puts the reserves right after the script's own animals on every peer.
- **Script natives.** `StartTask` = CommonDll thunk `0xe000` (`jmp [vt+0x15c]`), `SetTaskParam` = `0xe0c0`
  (`+0x16c`), `StopTask` `0xe060` (`+0x164`), `StopAllTasks` `0xe080` (`+0x168`). The animal script object
  (GameDll vtable `0xff010`) reaches the CommonDll bodies through GameDll import thunks (`+0xeca88`, `+0xeca70`); the
  module hooks the exported bodies (`?StartTask@cScrCharacterEx@@UAEXPAD@Z`, `?SetTaskParam@...`) and checks the
  slots at load (`hooked bodies: true/true`).
- **`CustomProps` (GameDll `0x16300`) must be replayed.** MAP4A lions get `CustomProps("MakeChaser")` (brain
  `AIHuntBrain_Chaser`) before `AttackChasers`; without it the reserve's `StartTask` raises "Could not activate task:
  AIHuntTask_AttackChasers for entity coop_LIONESSE_0" (debug.log, run 8). `Make_Scared` is never copied.
- **Lion and lioness are one pack.** MAP4A/MAP4B start one `LION` and one `LIONESSE` together. Grouped by species
  they would be two lone large animals (health, no body); they are grouped as one family (pack of two).
- `coop_kill` reports action 0 (`+0x420` is only set to `MISSION_ACTION_KILL` = 1 by the animal's bullet `OnHit`),
  and kill tasks ignore action 0. Quota tests need `coop_killshot <id>` (writes `+0x420 = 1`, then the same kill).

## Pool table (which species get reserves)

From every `StartTask("AIHuntTask_Attack*")` in the trigger, cutscene and world scripts (scratch script, not
committed), per map: species that attack in a pack, and a large species only where one script function starts two or
more of it. Only species that really attack get reserves (MAP2C/MAP12A boars and the cobras never `StartTask` an
attack).

| Map | Pool | Map | Pool |
|---|---|---|---|
| MAP2A, MAP2B, MAP2C, MAP12A | GWOLF | MAP6, MAP7, MAPISLAND | BTIGER |
| MAP3A | HYENA, LEOPARD | MAP8 | SCRUBBULL (stampede) |
| MAP3B | LEOPARD | MAP9 | DINGO |
| MAP3C | LIONESSE | MAP10 | PUMA, WILDBOAR3 |
| MAP4A, MAP4B | LION, LIONESSE | MAP11A/B/C | WILDBOAR |
| MAP5 | HYENA, ELEPHANT (stampede) | MAPARENA | GWOLF, LEOPARD, SCRUBBULL, LION |

None: MAP1, MAP12B, MAP12C, MAPTSAVO, hunt maps. Largest snapshot: MAPARENA 11 + 12 = 23 animals, under
`kMaxAnimals` (32); the cap was not raised. `animals.cpp` now logs once per map if animals and NPCs exceed it.

Large (lone → health): GBEAR, BBEAR, ABEAR, PBEAR, LION, LIONESSE, MOUNTAINLION, COUGAR, PUMA, BTIGER, RHINO, ELEPHANT,
HIPO, SCRUBBULL. Species ids are the `ANIMAL_*` constants in `SCRIPTER.EXP` (GWOLF 0 … IMPALA 36).

## ini (`[Scale]`, documented in `mod/dh2coop.ini`)

| Key | Default | Meaning |
|---|---|---|
| `Health` | 1 (VERIFIED) | Boss / lone large health scaling. |
| `Bodies` | 1 (VERIFIED) | Reserve pool and releases. Must match on every player (entity ids). |
| `Reserves` | 3 | Reserves per pool species (0–3). Must match on every player. |
| `ClientRespawn` | 1 (VERIFIED state) | Client gives a released reserve the `MoveToPos` body reset, then re-freezes it. |
| `ReuseKilled` | 0 | Release a reserve again after it died. Off until respawn mirroring exists (G4). |
| `Log` | 1 | `scale:` lines. Commands: `coop_scale` (state dump), `coop_killshot <id>`. |

## Evidence (logs in `re/notes/coop-local/scale/`, gitignored)

| Test | Result | Verdict |
|---|---|---|
| Reserve ids, MAP2A, 2/3/4 players | Every process: `reserves on 'MAP2A': coop_GWOLF_0=160 coop_GWOLF_1=161 coop_GWOLF_2=162` (run1–3, run7). MAP2B 179–181, MAP2C 102–104, MAP4A 90–95. Never `waiting for a local entity`. | VERIFIED |
| MAP2A opening (`coop_tp` into `cScrTask02`, real BeginCutScene → OnActivate) | 2p: `GWOLF attack begins: 2 script attacker(s) … extra 1 -> release 1`, `release coop_GWOLF_0 id=160 like 'wolf1' … target=hunter`. 3p: release 160 + 161. 4p: release 160, 161, 162 (run7 23:24:14). Pack on screen 3/4/5. | VERIFIED |
| Client mirror of reserves | 4p: all three clients `client sees coop_GWOLF_0 id=160 up at (-139.4,37.0,115.8) … respawn=true frozen=true coll=0x7/0x7/0xfffffff8 brain79=1`; sync lines `drift=0.000` on all clients 17 s later. | VERIFIED |
| Retarget does not add bodies | MAP2A `cScrTask03`/`cScrTask06` restart wolves already attacking: `wave GWOLF x2 … -> pack`, no release (run12). | VERIFIED |
| Reserves kept out of kill tasks | MAP2A task 5 ("kill 3 wolves", end trigger `cScrTask07`): killshot reserve 160, wolf1, wolf2 → `reserve death kept out of the mission (action 1, species GWOLF)`, two `mission report action 1 value 0`, no `cScrTask07`; wolf3 wave released 161; killshot 161 (kept out), wolf3 → `trigger FIRE … 'cScrTask07'` at the third script wolf (run3 23:17:18, run12 23:36:07). Same on clients. MAP2B siege reserves killed by Hugh also kept out. | VERIFIED |
| Single wolf gains a body | MAP2B `cScrTask02` (one wolf2): release 1 (run6 23:21:28; run11 as `new attackers join the attack` while the opening wolf3 still attacked). | VERIFIED |
| Siege does not climb | MAP2B `cScrTask08`: `release 0` while a reserve is up; after it died the next attack released the next fresh one; after three deaths `no GWOLF reserve left`. Never more than `extra` reserves up. | VERIFIED |
| Script park | MAP2B `coop_tp` into `cScrTask03`: BeginCutScene `MoveToPos`+`Kill` on wolves 1–3 → `script parked every GWOLF; parking coop_GWOLF_0 id=179 ok=true`; client mirrors the park (run11). | VERIFIED |
| Lone large, MAP2C grizzly | `wave GBEAR x1 [gbear] … -> one body, health`, `lone large 'gbear' id=100 max 350.0 -> 437.5 hp 350.0 -> 437.5 (factor 1.25, players 2)`; client `host max 'gbear' id=100 350.0 -> 437.5`; host and client sync `hp=437.5`. No GBEAR reserve exists. | VERIFIED |
| MAP2B grizzly | max 437.5, hp 12500 (`Make_Scared` 10000 × 1.25); after the 3rd player joined: `max 437.5 -> 525.0 hp 12500.0 -> 15000.0`, late joiner replays both commits (run6). | VERIFIED |
| MAP12C yeti, 3 players | `boss 'yeti' id=24 max 2500.0 -> 3750.0`; both clients `host max 'yeti' id=24 2500.0 -> 3750.0`. After `coop_kill 24:750`: host 3000/3750, client 3000/3750 (`mark … base=2500.0 factor=1.50 max=3750.0 hp=3000.0`). Leaves: `3750 -> 3125 hp 3000 -> 2500` then `3125 -> 2500 hp 2500 -> 2000` (ratio kept). Host boss bar at 80 % (`run5-map12c-host.png`). | VERIFIED (numbers); client bar not seen on screen (client still in the opening cutscene, see open issues) |
| Lion family + chaser, MAP4A | `wave LION x2 [lion1,lion2] … -> pack`, `release coop_LIONESSE_0 id=93 like 'lion1' … props: MakeChaser task=AIHuntTask_AttackChasers ok=true running=true` (run9). | VERIFIED |
| Hunt map untouched | HUNTMAP4: `(scaling off on this map)`, no reserves, animal ids 120–124 as before. | VERIFIED |
| Late join, no body for a running wave | MAP2B: client joined after `cScrTask01` started (`extra 0 -> release 0`); no top-up until a new trigger. | VERIFIED |
| Other pool maps (MAP3A/B/C, MAP4B, MAP5–MAP11C, MAPARENA, MAPISLAND) | Not run. Table from scripts only. | UNVERIFIED |
| Real bullet hits on a client's reserve | Not tested (no input allowed). Client reserve body state equals a client `MoveToPos`'d wolf (`coll=0x7/0x7`). | UNVERIFIED |

A/B for client crashes seen during these runs (`ab-main-map12c-*.log`, lead's main ASI, same steps): MAP12C P3
crashed at `EngineDll7+0x28b337` (the known yeti fault) and P2 faulted at `EngineDll7+0x29a8bf`
(`cVMachine::SysMain`, "trigger OnActivate faulted id=22" while applying the deferred `cScrTask01`) exactly as with
the scale build. The MAP4A client in run 8 died with the same `0x29a8bf` VM fault two seconds after a script lion's
sync attack knocked it down. Pre-existing, not caused by this module.

## Open issues

1. **Reused reserves (G4).** A reserve that died stays dead on clients; with `ReuseKilled=1` the host would bring it
   back and clients would not see it (run3: `release coop_GWOLF_0 … (reused after a death)`, clients kept
   `pos=(-10000…) state='Dead'`). Default `ReuseKilled=0`: each map has 3 extra bodies per pool species in total, so
   long sieges (MAP2B kill-10, MAP3A 9 hyenas) run out of extras. Turn it on once respawn mirroring lands.
2. **Client kills do not count on the host (pre-existing, not this job).** A client's lethal `AnimalHit` runs
   `FuncInflictDamage` + `PresentDeath` on the host with `+0x420 = 0`, so the host mission gets action 0 and kill
   tasks ignore it. Writing `+0x420 = 1` before `PresentDeath` in `animals.cpp` `OnAnimalHit` would fix it.
   UNVERIFIED in game (no input), follows from the decompile and the `coop_kill` reports above.
3. **Reserve health vs script `SetHealth`.** Reserves keep the species health (spec). MAP4A lions get
   `SetHealth(100)` from the script while their reserve keeps 170; MAP2A wolf3 gets 600 while its reserve keeps 50.
   Decision for the lead: copy the template's current health ratio instead?
4. **Release position.** Reserves appear 1.5 m (then 3 m) beside their template on the world x/z axes, at the
   template's height. On slopes or next to rocks one may spawn inside geometry; not seen in the tested maps.
5. **ini must match.** `Bodies` and `Reserves` change entity ids. Nothing enforces equal values between players (the
   kit ships one ini).
6. **Hook overlap.** `hunter-nearest` (not merged) also hooks `cScrCharacterEx::SetTaskParam`. Both are named
   per-hook functions (`scale::AfterSetTaskParam`); fold them into one hook when merging. Reserves call the exported
   `SetTaskParam`, so the hunter-nearest rewrite of `"hunter"` would apply to them too.
7. MAP12C client boss bar: the client stays in the opening cutscene (pre-existing) and never shows the bar; its
   `+0x4C0` / `+0x19C` match the host's.

## Proposed MEMORY facts

- Trigger event order: `OnHit` → `ActivateAfter` (state 4 when a `BeginCutScene` exists) → Update case 4 runs
  `BeginCutScene` (`+0x210`, then released) → cutscene → state 3 → 2 → fire `0x68130` runs `OnActivate`. So
  `BeginCutScene` runs before `OnActivate`. — GameDll decompile `0x68800`; MAP2B log: `script parked` (BeginCutScene
  Kill) 15 s before `FIRE id=144` (OnActivate).
- GameDll `0x100067d0` (world start) runs `0x8cab0` (`AddAnimalOnMap`), `0x90100`, `0x83430` (`AddMapEvent`),
  `0x917f0`, then `AddMatFile` and `InitGlobals` — all after `cBspTree::Load` returns. Calling `AddAnimal`
  (`0x8d970`, cdecl(species, 1, name)) at the end of `0x8cab0` gives identical entity ids on every peer (MAP2A
  160–162 on 4 processes). — scale run logs.
- `ANIMAL_*` ids: GWOLF 0, BBEAR 1, COUGAR 2, GBEAR 3, HYENA 4, LEOPARD 5, MCARIBOU 6, MOUNTAINLION 7, PBEAR 8, RHINO 9,
  WILDBOAR 10, ZEBRA 11, BANTENG 12, CAMEL 13, CAPEBUFFALO 14, DINGO 15, HIPO 16, SCRUBBULL 17, WILDDOG 18, COBRA 19,
  ABEAR 20, BTIGER 21, PUMA 22, ELEPHANT 23, CROCODILE 24, YETI 25, BIGGRIM 26, LION 27, LIONESSE 28, HOGZILLA 29,
  WHITETAIL 30, WATERCROCODILE 31, PANTELOPE 32, WILDBOAR3 33, REDDEER 34, AXIS 35, IMPALA 36. `MISSION_ACTION_*`:
  NONE 0, KILL 1, KILL_PROTECTED 2, REACHED_POSITION 3, FIND_SOMETHING 4, LOSE_TARGET 5. — `SCRIPTER.EXP`.
- Kill tasks count only deaths reported with action 1: present-death sends `0x91e80(mission, +0x420, +0x434)` and
  only the animal's bullet `OnHit` sets `+0x420 = 1`. `FuncInflictDamage` + `PresentDeath` (our `coop_kill`, the
  host's handling of a client `AnimalHit`) reports action 0. — scale run logs (`mission report action 0` vs 1).
- Script natives `StartTask`/`StopTask`/`StopAllTasks`/`SetTaskParam` are CommonDll vcall thunks `0xe000`/`0xe060`/
  `0xe080`/`0xe0c0` (`jmp [vt+0x15c/+0x164/+0x168/+0x16c]`); on the animal script object (GameDll vtable `0xff010`)
  those slots reach the exported `cScrCharacterEx` bodies through GameDll import thunks. Brain vt `+0x40` is
  `IsTaskActive(name)`. — CommonDll `cScrCharacterEx::ScrExport` decompile, runtime slot check.
- `CustomProps(name)` (GameDll `0x16300`, thiscall(script object, char*)) runs that method of the animal's script
  class; `MakeChaser` (LION/LIONESSE) switches to `AIHuntBrain_Chaser`, and `AIHuntTask_AttackChasers` cannot start
  without it ("Could not activate task"). — decompile, MAP4A run.
- A parked animal's physics body keeps collision words `0xfffffff8/0xfffffff8/0xfffffff8`; `MoveToPos`
  (vt `+0x158(3,0)`) restores `7/7/0xfffffff8`. A client wolf placed only by the host's `BeginCutScene` keeps the
  parked words. — `coop_scale` dumps (run2).
