# Co-op checklist — what MAP1 / MAP2 taught us (2026-10-17)

Use this for every other map. Each line is a mechanism that works (with where it lives) or a pitfall that cost a test session. "Host" runs the real game; clients mirror. Code: `mod/src/coop/`.

## Story flow
- **Trigger events** (`cObjTriggerEvent`, GameDll vtable `0x10afa0`): arm `0x68eb0`, fire `0x68130` (runs the script `OnActivate`). Host decides; a client's own arm/fire becomes a request (one per trigger per 3 s), the host runs it once and records it (`g_ran_fire`), everyone applies the commit by entity id + name (`story.cpp`). Pitfall: the `+0x1fc` "fired" flag can be set without `OnActivate` having run — decide by "ran here", not the flag.
- **Cutscenes**: started by a fire (`ExecuteScript("cScrCutSceneN")`). Clients play them; a fire that arrives during a client's cutscene camera is deferred. The host's profile may auto-skip cutscenes inside `OnActivate` — not forwarded; only a skip outside `OnActivate` (the key) is (`kCutsceneSkip`, trigger skip `0x6b350`, cutscene vtable `0x110998`). A late joiner skips cutscenes of fires older than 3 s. When a client's cutscene ends, a client 20 m+ from the host is moved to the host.
- **Paired cutscene animations** `PlaySA(anim, partner)` (MAP2A bridge) and prop `SetAnim` (`kPlaySA`, `kPropAnim`) are replayed. Pose writes pause while the host actor stays in `CutScene`.
- **Mission end**: every success/fail through `cMission` message `0x91c20` is the host's and is replayed (`kMissionEnd`). Pitfall: `cObjGuide` death writes fail code 6 itself, not through `0x91c20`.
- **Map change**: `exit_*` tokens; host `kMapStart`, clients follow with `spmap`; a client that reached the map itself is not reloaded. Same-map `spmap` (replay) crashes at the next teardown — avoid.
- **Late join / sync**: `kSyncRequest` → the journal replay to that client only; live commits only go to synced clients.

## Actors
- **Animals**: `cScrAnimal` (GameDll vtable `0xff470`) and mission cobras `cObjCobra` (`0x10e920`) are mirrored by entity id (BSP-placed, same id everywhere; script-spawned ones too when spawn order matches). Client brains frozen; position, yaw, body-state name, health from the host. Deaths: host `kAnimalDied`, client presents once. Client hits → `AnimalHit` to the host. Pitfalls: a client must never present a death twice (cobra kill count can fail the mission); body-state names that the client's model cannot play raise the engine's fatal "Could not forward animation" (MAP12C yeti: open).
- **Animals attack every player** (`combat.cpp`): puppets are attack targets with a non-colliding physics body; the host retargets an animal to the nearest player (3 m margin); the attack's "are you the player" tests (vt `+0xFC`) answer yes for a living puppet; hits/knockdowns on a puppet go to that player's own hunter (`kPlayerHurt`, `kPlayerKnock`). Story animals that flee to a flee point are parked at `(-10000,...)` by design (MAP2A lion ambush).
- **Story NPCs** (`/npc/` models, `cObjNPC`/`cObjGuide`): mirrored with `kNpc` (`CharacterBrainFreeze`, position, leg clips; arms/gun not driven). Health is the profile float (`+0x1F8` → `+0x54`), mirrored; client `cObjNPC::OnHurt` (`0x5eeb0`) blocked; NPCs never send `AnimalHit`. A client NPC stuck in a sync attack is ended with the base `OnEndSA`. Guides/companions follow the host only, unless `[Story] NpcFollowNearest=1` (built, untested).
- **Ambient snakes** `cObjSnake` (`0x10e470`) are not mirrored (no brain; not a mission target).

## World objects
- **Shootable targets** `cObjTarget` (vtable `0x1017c8`): bottles/cans count for `TARGET_CAN`, stumps add a profile tally. Shared (`props.cpp`, `kPropHit`, journaled).
- Not yet looked at: vehicles (`cMap4Truck`), mines (`cScrMine`), helicopter (`cScrHelicopter`), traps (`cScrTrap`), cages, pickups/items, doors/gates, destructibles other than `cObjTarget`, fire/smoke FX timers.

## Players
- Puppet = mountain-guide NPC body, `obj_humanEx`. Weapon on the hand bone, gun clips only names that exist on that mesh (a missing clip leaves bind pose with the muzzle-flame bone visible; a prefixed name can raise a fatal dialog). Weapon kinds: 1 handgun3, 2 shotgun1, 3 rifle1 (third-person mesh, no working clip: flame visible, open), 5 rifle2 bolt-action, 6 handgun5, 4 other (knife etc.: no model).
- Client death: F9 revive next to the host; host death ends the level. Players knock each other over (shot/melee).

## Evidence habits
- Logs: `<Bin>\dh2coop.log` (or `%LOCALAPPDATA%\dh2coop` under Program Files), previous runs `dh2coop.1/.2.log`, engine `debug.log` (`debug.1/.2.log`). Grep `story:`, `events:`, `animals:`, `combat:`, `props:`, `exception`, `Could not forward`.
- Test commands (console, or one line per entry in `<Bin>\dh2coop.cmd`): `coop_trigger <name>` (arm, then fire), `coop_tp x:y:z`, `coop_tohost`, `coop_kill <id>[:dmg]`, `coop_claim <id>`, `coop_prophit <id>`, `coop_knock <player>`, `coop_down`, `coop_respawn`.
- `run-coop-local.ps1 -Map <MAP> [-Players N] [-AddClient N] [-Seconds S] [-Capture]`; input reaches only the window in front.
