# STATUS — resume here

**Last updated:** 2026-10-18 by the lead (Claude). Earlier: Grok removed DLSS 5 from the retail Bin; the co-op mod stayed.

## Wave "playthrough" (2026-10-18): clean co-op run MAP1 → MAP12C
User: extend the MAP4A lessons (movers, riders, NPC/animal sync) and the earlier map work to every story map; research each map, cross-reference, implement and test targeted fixes; full end-to-end runs are not feasible, test sections. Brief: `re/jobs/_wave-playthrough.md` (MAP4A lessons, roster, pre-allocated event kinds / msg ids / flags, test conventions). The old audits (`re/notes/audit-maps-a/b/c.md`) are the starting gap lists.

| Agent (model) | Worktree / branch | Port / slot | Scope |
|---|---|---|---|
| research-A (Opus) | main, `re/notes/playthrough-a.md` | – / A | MAP1–MAP3C delta audit, routing |
| research-B (Opus) | main, `re/notes/playthrough-b.md` | – / B | MAP4A–MAP8 |
| research-C (Opus) | main, `re/notes/playthrough-c.md` | – / C | MAP9–MAP12C + game end |
| triggers (Opus) | `dh2coop-trig` / `triggers` (moved from `dh2coop-scale`) | 27610 / T | `coop_tasks`, OnHit/BeginCutScene replication (`kTriggerHit` 18), host fire guards, journal serials, spawner volumes |
| life (Opus) | `dh2coop-life` / `journal-life` | 27620 / J | kill credit for client kills, second lives (`kAnimalRevive` 20), scale interplay, host scale settings win |
| beasts (Opus) | `dh2coop-beasts` / `beasts` | 27630 / K | client tranq hits (MAP6/MAP7), water crocodiles, claim side effects, clip coverage, cobra bites |
| flow (Opus) | `dh2coop-flow` / `mission-flow` (old WIP → `mission-flow-wip`) | 27640 / D | host-decided exits, guide death, ammo tasks, results/retry, game end |
| vehicles (Opus) | `dh2coop-truck` / `trucks` | 27650 / N | MAP5 truck, mover sweep, MAP4A riders as targets / late join |
| pilot (Opus) | `dh2coop-pilot` / `pilot-maps` | 27660 / P | MAP11A/11C `cObjPilot`, host-driven helicopter |
| hunter (Opus) | `dh2coop-hunter` / `hunter-nearest` (rebase) | 27670 / M | hunter = nearest, companions follow nearest, `SetCanBeAttacked` for puppets |
| bosses (Opus) | `dh2coop-yeti` / `yeti` (rebase) | 27680 / W | yeti, Hogzilla, Big Grimm, NPC PlayAnim beats |
| smoke (Sonnet) | `dh2coop-smoke` / `smoke-pass` | 27690 / G | baseline 2-window smoke on all 21 story maps, F5/F6, 3 players |

Lead: review + cherry-pick every branch into main; integration test; route research findings to owners.

Merged by the lead 09:4x:
- `13cf306`, `a5558d9` (branch `replay-crash`): **same-map replay crash fixed**. A replay (results-screen retry, F5, `spmap` of the loaded map) now runs Main7's map-change cleanup (`0xf8f0`) before the loader (`0x101d0`), so it is a full load (`[Coop] FullReloadOnReplay=1`). `e38f9dc`: the `cSndDrvDS::WaveRelease` hook also skips a release on an empty slot. Verified `replay-coop-test.ps1` MAP1 → MAP1 → MAP1 → MAP2A → MAP1 on two windows: RESULT ok, no crash. Handled first-chance AVs in `display.cpp` `gameplay_icon_scale` (guarded `safe_copy` on the loading thread) are harmless.
- `038afc0`..`a1e9212` (branch `puppet-polish`): `coop_fake*` test sender, no first-chance AV on weapon unlink on story maps (scripted state machine hidden from `SetSyncWith(nullptr)`), a leaving player's puppet is parked (hidden, frozen, y = -10000) and reused by the next joiner with the **same model** (lead change: also on a model swap, collision restored if parked while riding). Verified MAP2A: model swap, leave, rejoin, swap again: bodies reused, no exceptions.
- Routed review M1/M2 (NPC mirror death / thaw with the animal FreezeBrain) to `life`.

## Pivot to real testing (2026-10-17 late night)
User: pause the wide fan-out; make install and host/join very easy; start real testing over Tailscale; implement player-count scaling; fix the compass; then a lobby overlay (one DH2 COOP shortcut → main menu → lobby with HOST / JOIN).

Done in main:
- `0178722`: `run-coop-local` waits for `coop: hosting on UDP <p> -> true` and stops on a failed bind; the host shows a notice. (The 21:53 "different mod build" reject was the user's retail game hosting on 27420; the reject path itself worked.)
- `3732bd1`: start from the main menu (clients follow with `mod_spmap`, see MEMORY "Main menu"), menu banner, `-Map MENU`; F8 shows the tailnet IP; compass ring round at every heading (`[Display] CompassFix=1`). The two-window MAP1 check of protocol v2 passed: same build id, welcome in 1.7 s, animals mirrored; windowed minimize keeps frames running (the Flip pump only matters for fullscreen, still unverified).

- `8ea801f`: **co-op lobby** (F8; opens by itself at the main menu): name, puppet model, HOST, JOIN (IP + port), status line (no answer after 8 s, reject reasons), tailnet IP with Copy, player list with roles, models and ping. Protocol v3 carries the model. Ten puppet bodies verified on MAP3B. Knife / machete / branch now drawn on puppets (machete verified; held flat, no orientation offset). Kit: one **DH2 COOP** shortcut; `dist\dh2coop-kit.zip` built 23:1x.
- Verified on two local windows (port 27425): lobby host/join, failed join message, model shown to the other player, host map start followed from the menu.

- `d14e897`: **host level recovery**: F5 twice = restart the level (through the main menu), F6 twice = next story level; lobby buttons; clients follow and get an on-screen line (event kind 17). Verified MAP2B → MAP2C → restart on two windows. Kit zip refreshed.

**For the user (real test):** run `install.bat` from `dist\dh2coop-kit.zip` on both PCs (Tailscale on both), DH2 COOP, HOST on one, JOIN with the host's tailnet IP on the other, host closes the lobby (F8) and starts New Game or Load. Still unverified: clicking the lobby with a real mouse (all automated runs used `coop_lobby`), New Game / Load through the real menu (automated runs used `mod_spmap`), a remote PC over Tailscale.

- `e9f1d75`: **MAP4A truck** (user watched the scale agent's runs: host truck derailed, puppets floating ahead, client crashes). One host-driven truck (protocol v4 `VehicleStates`, `vehicles.cpp`: clients freeze their copy and follow the chassis pose at ~30 Hz, wheels carried, 100 ms interpolation on the host clock; user: "smoothness is good now, sync looks good"); riders (`kFlagRiding`) seated in the local truck bed from the live seat position with collision/gravity/velocity held at zero (user: "puppet sync looks good now"); no player knock-over while riding; client lions: `MoveToPos` (0x14610) hook re-freezes the brain at once, per-frame wake check, failing body states tried 3 times then left (fixed the client script-VM crash); client trigger state-2 spin set to 1 (the ~6000-line FIRE flood).
- `a838e7b`..`8467e8f`: **player-count scaling merged** from the scale agent (`scale.cpp`, `[Scale]`, `re/notes/scale.md`). MAP4A: reserves `coop_LION_0..2` / `coop_LIONESSE_0..2` ids 90-95 on both peers; the first wave released `coop_LIONESSE_0` for 2 players.
- Open on MAP4A: riders are not lion targets (every lion goes for the host; an unattended host died in two test runs, mission failed); the client's lioness copy can stand in one pose after a tackle (states it cannot enter are no longer forced).
- Scale agent decisions waiting for the user (`re/notes/scale.md`): reuse dead reserves (off), reserves copy the script animal's health (they keep species health now); **client kills report "action 0" to the mission, so kill tasks never count client kills** (proposed fix `+0x420 = 1` before the death in `animals.cpp` OnAnimalHit, untested); `Bodies` / `Reserves` must match on every peer (not enforced); its `SetTaskParam` hook overlaps the unmerged `hunter-nearest` branch.

### Paused for the next phase (agents stopped 22:0x; worktrees kept, resume each with its context)
| Job | Worktree / branch | State when paused |
|---|---|---|
| journal-life (animal respawn mirroring, journal dedupe M5, H1) | `dh2coop-life` / `journal-life` | nothing committed |
| mission-flow (results screen, host-only exits, guide death `0x5fde0`, fail paths) | `dh2coop-flow` / `mission-flow` | 8 uncommitted files incl. new `flow.cpp` |
| hunter-nearest | `dh2coop-hunter` / `hunter-nearest` | 2 commits + 1 dirty file |
| yeti / bosses | `dh2coop-yeti` / `yeti` | 1 commit + 5 dirty files |
| puppet-polish | `dh2coop-polish` / `puppet-polish` | 4 commits |
| npc-arms | `dh2coop-npcarms` / `npc-arms` | 2 commits + 3 dirty |
| weapons (dropped by the user: no more weapon alignment) | `dh2coop-weapons` / `weapons` | 1 commit + 3 dirty; do not resume |
| replay-crash | `dh2coop-replay` / `replay-crash` | 2 commits (fix + tests) |
| kit-fixes (installer) | `dh2coop-kit` / `kit-fixes` | 3 dirty (big `install.ps1` rewrite); DLSS parts obsolete |
| smoke-all-maps | `dh2coop-smoke` / `smoke-pass` | 3 new scripts, uncommitted |

Empty worktrees made for the next wave: `dh2coop-pilot` (`pilot-maps`), `dh2coop-beasts` (`beasts`), `dh2coop-truck` (`trucks`).

Next-phase plan (from `re/notes/audit-maps-a/b/c.md` gap lists; the goal is a full multi-player story playthrough):
- Lead (main thread): trigger `OnHit` / `BeginCutScene` replication (`kTriggerHit`, audit-b G1: blocks MAP5/7/8/9, MAP3A find task), host fire guards (audit-b G5 pending states 2-5, audit-a G6 not armed, audit-b G10 `+0x1fc=1` on swallowed client fire), O(1) `EntityById` + list pruning (review M3/L11).
- journal-life: respawn = new life (audit-b G2, audit-c G2/G4), brain re-freeze every frame, journal serials for re-armed triggers (audit-c G8).
- mission-flow: host-only exits with spawn point (audit-a G2, audit-c G7), guide death commit (audit-b G9), need-ammo on any player (audit-c G6), client mission status after death (audit-c G13).
- hunter-nearest: spawner volumes for puppets (audit-b G8), `NpcFollowNearest` test (audit-b G7).
- pilot-maps: `cObjPilot` vt `0x109d48` in tracker/combat/puppets (audit-c G1), heli lag.
- beasts: water crocodiles `0xffa50` (audit-a G7, audit-b G6), tranq hits `0x1a680` (audit-b G3), claim callback on clients (audit-a G9), cobra bites nearest.
- trucks: MAP5 `car1` and MAP4A trucks (audit-b G4, audit-a G10), after `kTriggerHit`.
- yeti, npc-arms (death/PlayAnim beats only, audit-c G9; no more arm cosmetics), puppet-polish (`SetCanBeAttacked` for puppets, audit-a G8), replay-crash, smoke: finish and report.

## Launchers (2026-10-17 night)
`host.bat` and `join.bat` still only start `Main7.exe +mod data`. Desktop resolution and the wide FOV are the mod's `[Display]` block. The retail Bin has the 9:38 PM `dh2coop.asi` and `dh2coop.ini` with `[Display]` (`PlayerName=panjypoo` kept).

DLSS 5 was removed from that Bin: ReShade (`dxgi.dll`, ini, preset, shaders), the feeder and `host64`, both NVIDIA NGX DLLs, dgVoodoo `D3D8.dll` / `D3D9.dll` / `dgVoodoo.conf`, `host_dlss5.bat`, and the `DH2 HOST DLSS` shortcut. `dinput8.dll`, `host.bat`, and `join.bat` stayed. `tools-src\kit\host_dlss5.bat` is deleted, and the kit installer no longer copies it or creates that shortcut. Uninstall still deletes a leftover copy and shortcut.

## Helicopter arena (scoped only, 2026-10-17)
Not built. `re/notes/heli-arena.md`: copied MAPARENA, host-moved helicopter orbiting the nearest player, bombs via `kPlayerHurt`, full-auto gun is a new `autogun_1st` that reuses the handgun 3 viewmodel. Story insert waits until `-spmap` playtests pass.

## Display (2026-10-17 evening)
`[Display]` in `dh2coop.ini` sets the back buffer (0 = desktop work area, windowed) and keeps the HUD in a centered 4:3 rectangle. Wider screens keep the 4:3 vertical field of view. The standalone FOV-fix ASI is renamed `.asi.off` in this checkout's game bins so it does not widen the view twice. Detail: `re/notes/display.md`. Local multi-window launches tile when Width is 0.

MAP1 is running for the user (pid 25408, `run-game.ps1 -spmap MAP1`). Log: `display: CreateRender 3424x1352 windowed=1`, `display: hud 1803x1352 at (811,0) inside 3424x1352`, `Bsp\MAP1.bsp` loaded. Co-op hooks are unchanged. Not committed.

## Player-count scaling (research only, 2026-10-17)
Method is written, not built: `re/notes/player-scale.md`. Bosses (`MakeBoss` → entity `+0x3C0`: yeti, hogzilla, big grim) and a wave of one large animal (MAP2B/MAP2C grizzly, MAP2B cougar) get `+25%` max health per extra player, on `+0x19C` and `+0x4C0`, so the cutscene `MoveToPos` keeps it. They stay one body. Wolves, hyenas, dingoes, leopards, and boars get one extra body per extra player and no health scale, even when the wave is a single wolf. MAP2A's two wolves become 3 / 4 / 5. Reserves must not count toward `SetTaskAnimalToShoot`. Retail `SetDiffProps(HEALTH_ANIMAL, 10, 11, 12)` is `×1.0 / 1.1 / 1.2` on every `SetHealth` (`re/notes/scale-decomp.c`).

## Fan-out 2026-10-17 evening: rest of the game + re-opened items (user moved to Claude Max)
User: extend the MAP1/MAP2 fixes to every other map, all weapon models; Grok/cursor workers replaced by Claude subagents (Opus 5.5 / Sonnet 5.5 / Haiku 5.5); "fan out", "go in depth", "re-open any old closed items". Shared brief: `re/jobs/_claude-rules.md`; lessons list: `re/notes/coop-checklist.md`. Tooling made parallel-safe (`f45f1a3`): `run-coop-local.ps1 -Port`, `build.ps1` and the launcher only look at their own checkout's games.

| Job | Agent | Where (port / Ghidra slot) | Deliverable |
|---|---|---|---|
| Map audit MAP2B–4B | Opus, research | main (– / A) | `re/notes/audit-maps-a.md` |
| Map audit MAP5–10 | Opus, research | main (– / B) | `re/notes/audit-maps-b.md` |
| Map audit MAP11A–12C + bonus | Opus, research | main (– / C) | `re/notes/audit-maps-c.md` |
| Full code review (crashes, threads, netcode, kit) | Opus, research | main (– / E) | `re/notes/review-2026-10-17.md` |
| Log sweep (every log on the PC) | Haiku, research | main | `re/notes/log-sweep.md` |
| Smoke + trigger walk on every map, exit destinations, 3P late join | Sonnet | `dh2coop-smoke` `smoke-pass` (27430 / G) | `re/notes/smoke-all-maps.md` |
| All weapon models on puppets, rifle1 flame | Sonnet | `dh2coop-weapons` `weapons` (27440 / F) | `re/notes/weapons.md` |
| Same-map replay crash (`cSndDrvDS::WaveRelease`) — re-opened | Opus | `dh2coop-replay` `replay-crash` (27450 / V) | `re/notes/replay-crash.md` |
| MAP12C yeti + all bosses / sync-attack animals on clients | Opus | `dh2coop-yeti` `yeti` (27460 / W) | `re/notes/yeti-bosses.md` |
| NPC mirror arms, gun, firing, death — re-opened | Opus | `dh2coop-npcarms` `npc-arms` (27480 / X) | `re/notes/npc-arms.md` |
| Results screen follows host, chapter ends, pause, follow race, fail paths — re-opened | Opus | `dh2coop-flow` `mission-flow` (27490 / D) | `re/notes/mission-flow.md` |
| Drop-out AV, drop-in damage, puppet pitch / feet / crouch / hands / weapon sounds — re-opened | Sonnet | `dh2coop-polish` `puppet-polish` (27500 / H) | `re/notes/puppet-polish.md` |
| `"hunter"` = nearest player on the host (spook, claim, chases, guides, `NpcFollowNearest`) — re-opened | Opus | `dh2coop-hunter` `hunter-nearest` (27470 / M) | `re/notes/hunter-nearest.md` |

**Lead progress (usage limit hit mid-session, resume here):**
- Done in main:
  - `ad8b0b4`: debug.log copy from DllMain; prop anims only for map entities.
  - `6b40deb`: protocol v2 (build id, password, snapshot map serial), Flip net pump, ENet timeouts. Builds and net-test passes; **not run in game yet**.
- Lead to-do next:
  - Run two windows on MAP1 with `6b40deb` (welcome, snapshots, minimize a window → log `network from cRenderDX::Flip`).
  - Review M3/L11 and the smaller review items: O(1) `EntityById`; per-frame validation of `Animals()`, `Npcs()` and `items_`; L3, L6, L7, L10, L13.
- Reports in:
  - log-sweep and code-review are handled; findings went to their owners.
  - New jobs: `journal-life` (Opus: H1/M5/M6/M8/L8) and `kit-fixes` (Sonnet: L5).
  - audit-maps-a and audit-maps-c have landed.
- Their top new gaps:
  - Pilot maps MAP11A/11C: the player is a `cObjPilot` (vt GameDll `0x109d48`), and the tracker only knows the hunter.
  - Trigger `OnHit` (`0x6b140`) runs on one machine only, so MAP3A's "find 3" can soft-lock.
  - Level exits (`0x6c420`/`0x6c610`) are per-client and come from the profile.
  - `MoveToPos` is a respawn that unfreezes client brains.
  - Water crocodiles (`cObjCrocodile`) are not mirrored.
  - Guide death (`0x5fde0`) bypasses `0x91c20`.
  - `SetCanBeAttacked` is host-only.
- Still running when the limit hit: audit-b, smoke, weapons, replay, yeti, npc-arms, flow, polish, hunter, journal-life, kit-fixes. Check `git -C D:\scary hunt\dh2coop-<job> log` and `re/notes/<job>.md` in each worktree.

Lead: review each deliverable, cherry-pick into `main`, integration-test, then a second wave from the audit gap lists (vehicles, mines, helicopter, traps, pickups, timers…). Still for the user: bottle host real-bullet retest; two-PC run after merges.

## Lead session 2026-10-17 (user goals: untested objective types, 4 players, drop-in/drop-out, player markers; stretch: knock other players over)
User report at session start: a story map played with NPC + animal sync, scripted events and objectives on both, client death + F9 revive, host death = game over, and map-to-map progression in unison.

| Track | State |
|---|---|
| **4 players** (`run-coop-local -Players 4`, 2x2 grid) | Verified 10:12: each client sees the other three, no reloads, no crashes. |
| **Drop-in** (`kSyncRequest`/`kSyncDone`; per-client replay; old cutscenes skipped; teleport if more than 20 m from the host) | Verified MAP12C 10:21 (`tools-src\dropin-test.ps1`): late joiner synced in 1 s, skipped the yeti cutscene, moved next to the host. Design `re/notes/lead-dropin.md`. |
| **Drop-out** | Puppet removed on leave. A guarded first-chance AV in `UnlinkPart` (`BipLHand`) on leave is harmless; open. |
| **Player markers** (F7, diamond + name + metres, edge arrows) | Cursor worker, merged `56a01b0`; seen in the knock screenshots. |
| **Animals fight every player** (`combat.cpp`, puppets with a non-colliding physics body, nearest-player retarget, hits forwarded with `kPlayerHurt`) | **User-verified 11:14 on MAP3B.** Now the default (`[Puppets] Physics=1 AttackTarget=1`). `re/notes/lead-combat.md`. |
| **Players knock each other over** (shot or melee; that map's `pl_*_syncattack` clip, then get-up; `coop_knock <id>` for tests) | Verified both directions 10:25 (`tools-src\knock-test.ps1`). |
| **Objective types** | Worker report `re/notes/story-objectives.md`: guide/companion NPCs and animal chases read the host's `"hunter"`. Checklist maps: MAP3A, MAP9, MAP2B, MAP7, MAP8, MAP4A, MAP5, MAP11A (user to play). |
| Fixed along the way | 3+ player journal re-broadcast (existing clients reloaded the map); trigger fire request flood (~100/s) that crashed a client on MAP12C; stuck NPC sync attack on clients. |
| **Open: MAP12C yeti fight crashes clients** | Pre-existing: client exits ~10 s after the yeti starts attacking (first-chance `EngineDll7+0x28b337`, a dynamic-array push). The client yeti stays in a sync attack that started before the join; ending it (`OnEndSA`) faults on the yeti. |
| **NPC follow target** | Research `re/notes/npc-follow.md`. Lead built `npc_follow.cpp` (`ea6219f`): mid hook after `cAIBrain_NPC`'s player lookup. **Off** until tested (`[Story] NpcFollowNearest=1`). |

### 16:30 state after the two-PC test fixes (main `be53813`, kit refreshed)
Merged and smoke-tested (MAP1 pair, all hooks up, no errors):
- per-window input (`a7ad257`, `[Coop] InputOnlyWhenFocused=1`);
- shared bottles/stumps (`1691190` + diagnostics `8431527`; host real-bullet path still unverified: one retest, then park if it fails, per the user);
- shared cobras (`be53813`);
- NPC profile health (`72f4d42`);
- animal attack treats living puppets as players (`8226705`; the MAP2A lion's vanish is its scripted flee: `reached flee_point`);
- host runs client-requested fires once and records them (`8aa991d`);
- auto-skip recognized inside OnActivate (`8aa991d`);
- mission-end "arrived alone" (`a3dc2c6`);
- rifle2 / handgun5 puppet models (`9f7fcd9` cherry-pick).

`npc_follow` (guides/companions follow the nearest player) is built but **off** (`[Story] NpcFollowNearest=1`), not yet tested.

Next user test: MAP1 (bottles: host shot → client; cutscenes on the client), MAP2A (Hugh's bar equal on both; lion), MAP3B (cobras), mission end → next map on both. Still open: the two-PC client crash at the lion (did not reproduce); the client following from its results screen (needs a clean two-window test now that input is per window); `rifle1` third-person flame.

### Two-PC user test 2026-10-17 11:34–12:07 (host = this PC; notes in the user's vault "DH2 Test.md")
Host log: `D:\scary hunt\Activision Value\Magic Wand\DNGH2\Bin\dh2coop.log`. The client PC's logs were lost (each launch truncated them, and Program Files redirects writes).

| Finding | Cause (from the host log) | State |
|---|---|---|
| Client set off a cutscene, host never saw it, both stuck | Host's `OnFireRequest` skipped `Fire` when `+0x1fc` was set but the host had never run the trigger (`cScrTask12` 11:40:15) | Fixed `0d960f1` (decide by "ran here") |
| Clients never saw cutscenes | Host profile auto-skips cutscenes inside `OnActivate`, and `kCutsceneSkip` forwarded that | Fixed `0d960f1`: only the skip key (`0x6b600`) is forwarded |
| User request: cutscene = gather point | — | `0d960f1`: when a client's cutscene ends, a client 20 m+ away is moved to the host |
| Hit floods (403 / 880 reports) | Client NPC mirrors (Uncle Hugh, ids 82/75/128) sent `AnimalHit` every frame | Fixed `0d960f1` (no NPC hits; host ignores them; 150 ms limit) |
| Fell through the map after teleport / drop-in | Teleport went 1.5 m to the side at the host's height | Fixed `0d960f1` (host's spot + 0.25 m) |
| No client logs | Truncated per run; VirtualStore under Program Files | Fixed `04adb0a` (`%LOCALAPPDATA%\dh2coop` under Program Files, `.1/.2` rotation) |
| Mountain lion pounce → client crash (MAP2A) | Lion switched to the client 11:57:56; client gone 11:58:07; nothing forwarded | Repro: Cursor `session-repro` |
| Mission end: client stuck on the "pause" screen | Host frames and network kept running in menus; client left 50 s before the host's next map | Repro: `session-repro` |
| Host pause shows a pause screen on the client | Unknown | Repro: `session-repro` |
| Drop-in player: no damage, no cutscenes, no bear | The cutscene part is the forwarded auto-skip; damage unknown (no animal hits from that session, only the Hugh flood) | Repro: `session-repro` |
| Bolt-action on puppets misaligned, frozen muzzle flash | Puppet uses `rifle1.skin` for every rifle; the local gun is `rifle2` | Cursor `rifle-puppet` (worktree `D:\scary hunt\dh2coop-rifle`) |
| Bottles / stumps only shootable by the host | — | **User decision 15:45:** a client's shot on any world interactable (stump, bottle, snake, ...) propagates to every session, and goal items count toward the goal for everyone. Research: Cursor `props-research`; snakes need the animal mirror for `cObjSnake`/`cObjCobra`. |
| "Pause" screen on the client | User 15:45: only at mission end, so it is the client's results screen (then stuck when the host continued) | Repro: `session-repro` task 2 |
| MAP2A Uncle Hugh's protect-objective health differs on host and client, and drops on both at different rates | The mirror copies `+0x19C`, but the host's `+0x19C` is rewritten from another field every frame; the client's own scripts also damage him | Research: Cursor `npc-health-re` (Ghidra slot V) |

Found while reading the code: with 3+ players, every join re-sent the host's journal to *all* clients, so the existing clients `spmap`ed their current map (the replay crash) and replayed `PlaySA`/mission end. Fixed by the per-client sync.

## Cursor worker log (lead away)
- **21:35** Previous MAP3B pair had already shut down cleanly. That session killed both leopards and never logged a mission end.
- **21:35** `tools-src\story-digest.ps1` written. It reads logs with `FileShare.ReadWrite` because the game keeps `dh2coop.log` open.
- **21:35** Fresh pair: host pid 16668, client pid 8932, MAP3B. Client welcomed as player 1, `animals: client received 2`. Both armed `cScrTask01` (fired), `02`, `06`, `07` before join.
- **21:47** User finished. Report and digest: `re/notes/story-s1-map3b.md`. Shots: `re/notes/coop-local/story-s1-map3b-start-*.png`, `story-s1-map3b-end-*.png`.
  - First attempt: mission failed (code 6) synced, then a same-map `spmap MAP3B` replay.
  - Second attempt: triggers followed through `cScrTask05`. No `CompleteTask` line and no mission-success message. Host loaded `map2a.bsp` itself and committed `kMapStart`. Client ran `spmap MAP2A` and crashed in `cSndDrvDS::WaveRelease` (the known post-replay teardown). Client window stuck on LOADING. Host is in the forest, overlay `animals 7`, `no remotes`.
  - Snakes were not in the 2-animal `cScrAnimal` mirror.
  - In-game pause menus are not synced. Not a small fix; user accepts the desync. See Next steps.
- **21:56** Closed that pair and smoke-tested MAP12B, MAP12C, MAP4A, MAP11B (30 s each). All four clients were welcomed. Animals received: 1, 1, 6, 8. MAP11B's post-join `cScrTask02` arm was applied on the client. MAP12C's client crashed in `cObjNPC::Update` (guide model `mountainguide.skin`, missing `cBodyShotgun1npc`); no client screenshot. The other three had no exception in the kept lines. Shots in `re/notes/coop-local/story-s1-map*.png`.
- **00:03** User: F9 is working. Client death no longer sticks. The key stands them up at the host (`parts='Idle,Idle'`), clears hunter `+0x6CC` and the dead bit at `+0xC1`, and the view pitch comes back off the sky (log: pitch `-4.6` then `-13.5` while the hunter moved). Host death still ends the level.
- **23:53** Client respawn test. `coop_down` put the client hunter in `Dying`. F9 logged `respawn at (-144.9,38.3,136.9) health 100.0 ok=true state='Idle'` while the host stood at `(-143.4,38.3,136.6)`. Both processes stayed up. Host death still ends the level.
- **23:46** Animal death during a cutscene pose. Client wolf 154 stayed in `CutScene` after the host had left it, and the mirror skipped that actor entirely, so the host death commit never called present-death (hp stayed 50). The skip now applies only while the host sample is also `CutScene`.
- **23:44** Paired-anim hold. Every `PlaySA` (any map) now pauses pose writes on the caller and, when we mirror them, the partner, until the host body-state name changes. That covers Hugh and the other cutscene actors, not only MAP2A.
- **23:38** World-model sync. Trigger scripts already change the objective on both sides. The bridge mesh is `PlaySA(anim, partnerName)`: the host plays it on Hugh and on `fallbridge`, and a frozen client brain never starts that task. Clients now replay the same `PlaySA` (partner looked up by name), which is the cutscene mesh path on the other maps too. `SetAnim` on named props stays the other path (rocks, traps, the skip-to-broken clip).
- **23:33** Client crash. Forcing Hugh's handgun into `NormalToAlert` made `cModelPart::SetAnim` forward `handgun3/normaltoalert` onto the gun mesh and the engine raised its fatal dialog. The client no longer drives the NPC's weapon. Position and leg clips stay.
- **23:31** MAP2A bridge (`cScrTask03` / `fallbridge`). A test command armed and fired that trigger at 23:29:08, before anyone reached the bridge, and the client applied the same fire. It is one-shot, so the collapse did not play again on either window. Fresh pair relaunched with the trigger unfired. Prop `SetAnim` sync (`kPropAnim`) is in this build and was not what consumed the trigger.
- **23:16** NPC mirror. World NPCs (`/npc/` model, not puppets) ride the animal snapshot with flag `kNpc`. Clients call `cObjCharacterEx::CharacterBrainFreeze`, not the animal `FreezeBrain`, then copy position and yaw and reapply the host body-state name. MAP2A test 23:16: Hugh id 75, `CharacterBrainFreeze ok=true`. While the host walked, both logs showed him move together (`drift=0.000`) through `IdleToWalk` and `WalkForward` at the same times. Both processes stayed up. Pair left running on MAP2A.
- **23:07** MAP3B `exit_1` loaded MAP2A on the host (`cBspTree::Load('Bsp\map2a.bsp')` 23:04:54). Client followed (`following host to 'MAP2A'`) and loaded it too. User: the map switch worked. On MAP2A, Uncle Hugh (`obj_guide`, model `objects/npc/oldhugh/oldhugh.skin`, script `cScrHugs2a`, brain `cScrBrain_NPC`) follows each window's own hunter. The animal mirror only copies `cScrAnimal` (vt `GAMEDLL7.DLL+0xff470`, 7 of them on this map). Hugh is not in that set. Next: NPC mirror. Do not call the animal `FreezeBrain` on his brain until that is checked.
- **23:00** Puppet arms and gun. Legs already played `walk/forward`. Arms and the shotgun stayed in the idle pose because entity `SetAnim("shotgun1/...")` is the call that creates the missing `cBodyShotgun1npc`. Arm clips are now `cModelPart::SetAnim` on every part except the legs, and the gun mesh gets the matching short clip. 23:00 client log: `arms 'shotgun1/walkforwardalert' parts=3`, `weapon anim 'idlealertneutral' parts=1`. User: "animations, weapon model, all look good now!"
- **22:53** MAP12C client controllable. The opening cutscene waits on the yeti's `AIHuntTask_PlaySA`. Freezing that brain meant `EndCamera` / `DeactivateCutSceneMode` never ran. The client now leaves the yeti alone while its body state is `CutScene`, and holds the host's `cScrTask01` fire until the camera is back on the hunter. 22:52 client log: weapon state left `CutScene` and became `Fire`; hunter moved off the spawn with the view camera on the body. The running pair is this build. A follow-up (not installed yet) ignores duplicate copies of that deferred fire.
- **22:49** Client cutscene camera: `cScrTask01` had `+0x1fc` flag 5 without `0x68130` ever running, so the host fire was skipped. `OnFireCommit` now runs OnActivate when this process has not. Relaunched 22:48. Client log: `flag=5 but OnActivate did not run; clearing it` then `applied host fire 'cScrTask01'`. Host hunter left the spawn and fired. Client view yaw stopped moving at that same moment (cutscene camera released; the client window had no input).
- **22:46** MAP12C user test: client joined ~8s after the host, after `cScrTask01` had already fired (22:42:16 host, 22:42:18 client welcome). Client hunter stayed in `CutScene` and could not play. Yeti position synced (drift 0) but local body state stayed `CutScene` while `SetBodyState` of `Attack`/`Run`/`IdleAttack`/`Hurt` returned -1, so it slid without an animation. `run-coop-local.ps1` now starts the client when the host logs `coop: host start`, not at engine time 8s. Relaunched 22:45: client welcomed in the same second as the host, still inside the yeti's `CutScene`; the host fired `cScrTask01` 5s later. The client applied the arm of `cScrTask02` and did not log `applied host fire` for `cScrTask01` (the trigger was already marked fired). Yeti `SetBodyState` still returns -1 after that.
- **22:17** MAP12C client crash fixed in `puppets.cpp`. Solo MAP12C does not log `cBodyShotgun1npc`. Co-op did, because the puppet body played `shotgun1/idlealertneutral` and the gun mesh played `idlealertneutral`; both ask for `cBodyShotgun1npc`, which this map does not register. Body clips are unarmed `GAME.AP` names again. The gun mesh stays attached and is not `SetAnim`'d. Retest: `run-coop-local.ps1 -Map MAP12C -Seconds 25` exited 0 (client alive the whole window). One `cBodyShotgun1npc` line remains at weapon spawn. The client yeti (`id=24`) logs `SetBodyState 'Attack'/'IdleAttack' -> -1` and does not take the process down. Shutdown still logs the usual nvgpu first-chance.

> **Lead away (~80% of the 5-hour limit). Worker handoff: `docs/handoff-cursor-story.md`** (Cursor CLI, Grok 4.7 Medium Fast; job `re/jobs/story-s1-playtest.md`).
> **Story S1 works on MAP3B:**
> - host-decided trigger arming/firing (`kTriggerEvent` 5 / `kTriggerFire` 6, `mod/src/coop/story.cpp`);
> - the user saw leopards aggro and interact with both players (21:28).
>
> See "PIVOT" below and `re/notes/lead-story-coop.md`.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap (**primary goal: co-op story mode**, hunt maps first), `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail. Grok's Phase 4 write-up: `GROK_p4-session.md`.

## Current phase
**Phase 4 (shared animals + world events): core done, verified with two instances 20:04.**
- **P4b (Grok, `b8c4032`):** clients mirror the host's animals: position, movement yaw, body-state name, health. User-confirmed 16:13: "position looked good".
- **Event sync (lead rework of Grok's P4c `926d351`, 20:04):**
  - Engine map: `re/notes/lead-mission.md`.
  - Every mission end on every map goes through cMission's message call (GameDll `0x91c20`, "MISSION SUCCESSFUL!"/"MISSION FAILED!"). Hooked:
    - The host runs it and commits `kMissionEnd`.
    - A client swallows its own call, and its `CheckMissionStatus` (`0x94e50`) is skipped while online.
    - The client applies the host's commit (status, `SetMapIsComplete` on its own profile, then the message that brings up the results screen).
  - Trophy claim (`cObjAnimal::ClaimCallback` `0x1cd40`): a client's claim becomes a `kTrophyClaim` request. The host runs the real claim, so the host's script ends the hunt.
  - **Test 20:04** (`tools-src\event-test.ps1`):
    - `coop_kill 120` on the host → client `present death ok=true` and the "Go claim your trophy!" hint.
    - `coop_claim 120` on the client → host claim → host mission message → **MISSION SUCCESSFUL results screen in both windows** (`re/notes/coop-local/event-200320-*-2claim1.png`).
- The 16:36/16:53 "both crashed" runs were the Grok Build shell's job object killing the launched games, not the mod. `run-coop-local.ps1` now starts games with `Win32_Process.Create`.
- **Phase 3 (ghost mode): DONE**, user-confirmed 15:30.

### Later results (20:09–20:35)
- **Client hit (verified 20:11):** `coop_kill 120:20` on the client hits the local mirror → `AnimalHit` → host hp 45→25 → host deer state `Run` (flees).
- **Built, compiled, NOT runtime-tested yet** (the user's test windows were open, so no install):
  - **Map flow.**
    - Main7 `cSpServer::StartGame` (`0x1ed80`) hook dispatches `OnMapLoaded` on same-map replays (no `cBspTree::Load` then).
    - `StopServer` (`0x1b060`) hook adds a new module callback `OnMapUnloading` (entities still valid).
    - Host commits `kMapStart` (4); clients follow with `spmap`/`menumap`. A same-name commit means replay.
    - Animal mirror is gated on same map.
    - Test: `tools-src\mapflow-test.ps1`.
  - **Friends UX.** `[Coop] AutoRole=off|host|join`, `JoinAddress`, `JoinRetrySeconds` (retry while offline; `coop_leave` stops it).
  - **P5a.** On the host, puppets are added to the animals' attack-target list (`cAIHuntAnimalInterface::AddAttackTarget`, CommonDll singleton `0x1aea7c`) and removed before teardown. Animals natively choose the closest target. `[Puppets] AttackTarget=1`. Plan: `re/notes/lead-combat.md`.
- **Story prep:**
  - Worker inventory `re/notes/story-inventory.md` (cursor-agent Grok 4.7, 15 min; gitignored: quotes ~2000 script lines; regenerate with `re\scripts\story_inventory.py`).
  - Lead design `re/notes/lead-story-coop.md`: host decides, everyone runs the scripts; sync trigger-volume activations (`cObjTrigger::OnActivate` CommonDll `0x4dee0`). First story map: MAP3B.

## In flight
| Job | Owner | Log | Deliverable |
|---|---|---|---|
| (none) | | | story-inventory finished 20:23, lead-reviewed |

Worker jobs now run through `tools-src\cursorjob.ps1` (Cursor CLI, Grok 4.7 Medium Fast). User decision 2026-10-16: Grok Build performance not good enough; `grokjob.ps1` retired.

## PIVOT 2026-10-16 21:20 (user): story mode first
User: "the real goal is singleplayer… switch to getting story mode missions to sync". Hunt-map map-change polish is parked.
- **Story S1 started.**
  - `run-coop-local.ps1 -Map MAP3B` loads the first-choice story map on both instances. They join, see each other's puppet and mirror its 2 animals (screens `re/notes/coop-local/story-3b-*-0.png`).
  - New module `mod/src/coop/story.cpp` logs every:
    - L1 trigger-volume activation (`cObjTrigger::OnActivate`);
    - L2 script trigger event (`0x68eb0`);
    - `CompleteTask`.
  - At map start, both sides activate the same L2 events: `cScrTask01/02/06/07`.
  - Next: the user plays MAP3B co-op; read the logs to see which decisions diverge, then build L1 sync (`re/notes/lead-story-coop.md`).
- **Hunt-map state at the pivot:**
  - Kill, death fall, client hit, claim and mission end all work in automated two-window tests (20:38–21:16).
  - Map following works for A→B→A.
  - **Known issue:** after a same-map replay (`spmap` of the current map), the *next* world teardown crashes in `cSndDrvDS::WaveRelease` (both windows).
    - It is not the puppet-weapon refcount ring (fixed, balanced), not the BSP reset path (a forced full reload still crashes), and not the stale command file.
    - The "weapons off" controls were invalid (`mod/dh2coop.ini` has no `Weapons` key), so whether puppet weapons are involved is still unknown.
    - Workaround: don't replay the same map; restart the game instead.

## Next steps — pick up here (2026-10-17 00:05)

Handoff for the next agent: `docs/handoff-story-resume.md`.

**Playing now:** `.\run-coop-local.ps1 -Map MAP2A`. Left window is the host, right is the client. F8 hides the overlay. **F9** on a client stands that hunter up at the host. Do not write `coop_trigger` into a session the user is playing; it spends one-shot story triggers.

**User-confirmed this session:**
- Puppet legs, arms, and gun look right.
- MAP3B `exit_1` loads MAP2A on both sides.
- Uncle Hugh's position stays matched for the map. Leg clips follow. His gun and arms do not (driving the handgun body script raises a fatal `cModelPart::SetAnim` dialog).
- Story objectives sync. Paired cutscene meshes sync (`PlaySA`, MAP2A bridge). The client cutscene camera plays. Pose writes pause for every `PlaySA` caller and mirrored partner until the host body-state name changes.
- Animal deaths sync, including a wolf that was still in `CutScene` on the client.
- **F9 works** (00:03). Client death does not end the level. Host death still does.

**Lead (next session), in order:**
1. Play the next story maps (MAP2B onward) with `.\run-coop-local.ps1 -Map <NAME>`. Watch objectives, `PlaySA` meshes, NPC position, and animal deaths. F9 if the client dies.
2. **NPC arms and gun.** Safe path is still open. Do not `SetBodyState` / `SetAnim` the NPC weapon entity, and do not `SetAnimOnParts` with a `handgun3/` prefix onto it (client crash 23:32, `normaltoalert` forwarded onto the gun mesh).
3. **Snakes.** Still one per instance. `cObjSnake` / `cObjCobra`, not `cScrAnimal`.
4. **Host animals hurting client players.** Puppets must not be attack targets (`ProcessAttackStuck`). Plan: `re/notes/lead-combat.md`.
5. **MAP12C yeti.** Client no longer crashes, and the opening cutscene releases. `SetBodyState` of `Attack` / `IdleAttack` / `Run` still returns -1. Those names are the deer set.
6. **Pause overlays.** Host pause freezes client animal animation because snapshots stop. A client pause pauses nobody. User accepts this until there is a real menu-open signal. The frame hook does not run inside the modal.
7. **Same-map replay.** `spmap` of the already-loaded map, then the next teardown, crashes in `cSndDrvDS::WaveRelease`. Restart the process instead of replaying.
8. **Story map follow.** MAP3B → MAP2A worked when it was a fresh load. A follow that arrives before the client finishes `spmap` logs `no such trigger here`. Commit `kMapStart` only after the client has the map.

**Hunt-map leftovers (lower priority now):**
0. Map flow beyond A→B→A and the P5a puma test are parked.
1. **User play check (hunt map, two windows `.\run-coop-local.ps1`):**
   - Host kills a deer: does the client see it **fall**? Log: client `animals: present death id=… ok=true`. If it stays standing, try unfreezing the brain after `PresentDeath`.
   - Client walks to the corpse and presses **F** (real key): both windows should show MISSION SUCCESSFUL. Log: client `events: client claim id=…`.
   - Client **shoots** a deer: host log `animals: AnimalHit id=… hp a->b`. Does the host deer flee? Death should follow on lethal damage. Never seen in a log yet.
2. **Hunt-map gaps (lead):**
   - Scripts' `"hunter"` = local player only (spook distance, trophy distance, predator target / safe distance). Option: a host-side `GetDistBetween2Ais` (`0x9200`) hook that resolves `"hunter"` to the nearest player.
   - The animal AI's own perception of the hunter (C++ brain) sees only the host hunter. That is Phase 5.
   - **Results screen:** ESC replay / RETURN continue are per-process. The host's choice should drive the clients.
   - **Map flow:** the host's menu map choice should load the same map on the clients. Today both use `-spmap HUNTMAP4` on the command line. Join UX for friends (ini or overlay, not command line).
   - Other hunt maps: check animal ids and counts per map (HUNTMAP1–n).
3. **Story-mode prep:**
   - Inventory the story map scripts (natives, `"hunter"` uses, cutscenes/QTE, triggers, companions, spawns). That is a worker job → `re/jobs/story-inventory.md` via `cursorjob.ps1`.
   - Then the lead writes the co-op story architecture: host runs scripts, clients mirror entities, outcome natives host-only.
4. Leftover Phase 3 checks (low): shotgun/rifle in the left hand, crouch-pose verdict, crouch key.
5. Polish (low): feet slide, puppet pitch, remote weapon sounds.
6. Cleanup: the Draw diagnostic hook and spike code (solo only, off by default).

## Done
- **Phase 4:**
  - P4a net messages (`14250e0`).
  - P4b animal mirror (`b8c4032`).
  - Event envelope `GameEvent` id 48 (`926d351`).
  - Mission end + trophy claim sync (lead, 2026-10-16 20:04).
  - Automation channel `Bin\dh2coop.cmd` + `coop_kill` / `coop_claim`.
- **Phase 3:**
  - Puppet spawn recipe + physics unlink.
  - NPC body.
  - Directional clips.
  - Render-camera view yaw/pitch.
  - Action flags.
  - Armed clip sets.
  - Weapon model attach.
  - Test tooling (`game-input.ps1`, `move-test.ps1`, `focus-test.ps1`, `capture-window.ps1 -NoFocus`, `event-test.ps1`).
- **P2b overlay** (F8), **Phase 2 core**, **Phase 1** (Route E), **Phase 0**: see MEMORY.md / PLAN.md.

## Blockers / risks
- Delegation (user, 2026-10-17 evening, Claude Max): workers are Claude subagents (Opus 5.5 expert, Sonnet 5.5 defined jobs, Haiku 5.5 mechanical). Grok / `tools-src\cursorjob.ps1` retired. Parallel game runs need a distinct `-Port` and Ghidra slot per worktree.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; the REA MCP is registered for this folder only). Ghidra headless also works through `re\scripts\ghrun.ps1` / `decomp.ps1` (slot L = lead).
2. Read this file, then the `PLAN.md` decisions, then skim `MEMORY.md`, then `re/notes/lead-mission.md` and `re/notes/lead-animals.md`.
3. Check jobs:
   - For each in-flight log `re\jobs\<job>.out.log`, look for the `=== end` line.
   - `Get-Process node` with `cursor-agent` in the command line means a worker is running.
4. Run `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Build and test:
   - Build: `.\mod\build.ps1` (fails to install while `Main7.exe` runs).
   - Solo test: `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'` (no `-Seconds` when the user plays).
   - Two windows for the user: `.\run-coop-local.ps1`. Close CHOOSE ITEM with Enter, Left, Enter.
   - Automated: `.\tools-src\event-test.ps1` (kill + claim + mission end), `.\tools-src\move-test.ps1`, `.\tools-src\focus-test.ps1`.
   - Drive a running game: write console lines to `game\Bin\dh2coop.cmd` / `game2\Bin\dh2coop.cmd`.
   - Logs: `game\Bin\dh2coop.log`, `game2\Bin\dh2coop.log`. A new launch truncates them: quote what you need first.
6. Continue with **Next steps** above. Update this file after every step and commit (timestamps from `Get-Date`). Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content`.
