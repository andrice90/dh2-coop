# Snake sync — mission cobras (`cObjCobra`)

**Progress:** done. MAP3B two-window test passed on the final build (host pid 2600, client pid 3944, 16:25).

Recipe: `re/notes/snake.md` (`## Proposed change`). Worktree `snake-sync`.

## What changed

- `player_tracker.cpp`: `Animals()` includes GameDll7 vtable `+0x10e920` (`cObjCobra`) next to `cScrAnimal` `+0xff470`. `cObjSnake` `+0x10e470` is not included. The tracker logs `tracker: cobras at map load ids=` when that id list changes.
- Client mirror: `FreezeBrain` on a cobra returns false (the `+0x200` brain is not `cAIHuntBrain_Animal`). The call is not retried. `+0x728` is cleared so `cObjCobra::Update` returns before the bite and the death timer. Host body-state names applied for a cobra are only `Rise`, `Idle`, `Attack`, `TurnLeft`, `TurnRight`, `Death`, via `FUN_10012cc0` (GameDll7 `0x12cc0`, prologue `55 8B EC 6A FF`). `Dead` is not written. `SetBodyState` does not interrupt `Rise`.
- Death: host `AnimalHit` and host `coop_kill` do not subtract health and do not call slot `+0x154`. They set body state `Death` through `FUN_10012cc0` and set `+0x738 = 50.0`. The host `Update` calls `+0x154` (`FUN_10080320`) once when that timer crosses 0, which zeroes `+0x19C`. The health snapshot then commits `kAnimalDied`. The client calls that slot once in `PresentDeathOnce`. No `ClaimCallback`. The client brain stays frozen.
- A client's `coop_kill` still subtracts local health so the mirror sends `AnimalHit`.
- Host bite retarget is behind `[Animals] CobraBitesNearest=1` (default 0, not enabled in the test). It would point `+0x728` at the nearest living puppet or the host hunter.

## Verified in the research dumps

`D:\scary hunt\dh2coop\re\notes\snake-funcs.c` (gitignored) and `snake-vt.txt`:

- Vtable `0x10e920` slot `+0x8c` is `FUN_10080440`. On a connecting bullet it stores `0x42480000` at `+0x738` and calls `FUN_10012cc0(this, "Death")`. `0x42480000` is float `50.0`.
- `FUN_10081620` (`Update`) returns immediately when `+0x728` is null. While `+0x738 > 0` it subtracts `dt * 20` and calls slot `+0x154` when the result crosses below 0.
- Slot `+0x154` is `FUN_10080320`: `+0x19C = 0` and `FUN_10091e80(mission, 1, +0x434)`. It does not set a body state. A second call when the remaining count is already 0 takes the `MISSION FAILED!` branch.

File bytes at GameDll7 RVA `0x12cc0`: `55 8B EC 6A FF`.

## Test

`.\run-coop-local.ps1 -Map MAP3B`. `cScrTask01` fired on its own at map start. `CobraBitesNearest` stayed 0.

Final run, host `game\Bin\dh2coop.log`:

```
[2026-10-17 16:25:05.741] tracker: cobras at map load ids=49,50,51
[2026-10-17 16:25:27.591] animals: cobra id=50 coop_kill -> Death helper=true state='Death' timer=50
[2026-10-17 16:25:30.082] animals: death commit id=50 hp=0.0 state='Death'
[2026-10-17 16:25:47.721] animals: cobra id=49 AnimalHit -> Death helper=true state='Death' timer=50
[2026-10-17 16:25:47.721] animals: AnimalHit cobra id=49 shooter=1 dmg=10.00 (no health subtract)
[2026-10-17 16:25:50.253] animals: death commit id=49 hp=0.0 state='Death'
```

No `PresentDeath` on the host. No `MISSION FAILED` or `MISSION SUCCESS` after both kills.

Client `game2\Bin\dh2coop.log`:

```
[2026-10-17 16:25:07.225] animals: id=50 state='Rise'
[2026-10-17 16:25:07.273] animals: id=50 state='Idle'
[2026-10-17 16:25:09.242] animals: sync role=client id=50 pos=(34.49,-0.64,-111.06) state='Idle' hp=100.0 yaw=315.0 target=(34.49,-0.64,-111.06) drift=0.000
[2026-10-17 16:25:27.693] animals: id=50 state='Death' pos=(34.5,-0.6,-111.1) hp=100
[2026-10-17 16:25:30.101] animals: present death id=50 ok=true unfrozen=false pos=(34.5,-0.6,-111.1) state='Death'
[2026-10-17 16:25:47.711] animals: coop_kill cobra id=49 dmg=10.0 hp 100.0->90.0 ok=true (client reports the hit)
[2026-10-17 16:25:47.804] animals: id=49 state='Death' pos=(-14.0,-0.4,-107.9) hp=100
[2026-10-17 16:25:50.277] animals: present death id=49 ok=true unfrozen=false pos=(-14.0,-0.4,-107.9) state='Death'
```

Bite targets cleared once each (`cobra id=49/50/51 bite target cleared`). `FreezeBrain` logged `ok=false` for those three ids. One `present death` per cobra. Host `coop_kill 50` and client `coop_kill 49:10` each showed `Death` on both windows, then the kill about 2.5 s later. An earlier build (16:20, before the body helper was used on the client) left a client cobra in `Rise` through present-death; this build leaves `Rise` for `Idle` and then `Death`.

Screenshots after `coop_tp` beside cobra 49 (16:20 pair): `re/notes/coop-local/snake-sync-host.png`, `re/notes/coop-local/snake-sync-client.png`. Host hunter about `(-12.1,-0.5,-108.0)`, client about `(-16.5,-0.5,-107.7)`, cobra 49 at `(-13.98,-0.44,-107.90)`. Both overlays say `animals 5`. The cobra is in the grass at their feet.

## Proposed MEMORY facts

- MAP3B mission cobras are entity ids 49, 50, and 51 on both processes in one `AddAnimal` order. Evidence: host and client `tracker: cobras at map load ids=49,50,51` (16:19:58 and 16:25:05).
- `FreezeBrain` (`cAIHuntBrain_Animal::FreezeBrain` on entity `+0x200`) returns false for `cObjCobra`. Evidence: client `mirror freeze id=49 ok=false` (16:20:00.245 and 16:25). Clearing `+0x728` is what stops `Update`.
- A cobra kill that sets `Death` and `+0x738 = 50` reaches `+0x19C = 0` about 2.5 s later without a host `PresentDeath` call, and the client `PresentDeath` runs once. Two such kills on MAP3B did not log a mission failure. Evidence: host lines 16:25:27.591 and 16:25:50.253; client `present death id=50` and `id=49` once each.
- `FUN_10012cc0` at GameDll7 RVA `0x12cc0` prologue `55 8B EC 6A FF` is the body-state write `cObjCobra::OnHit` uses. `SetBodyState` does not interrupt `Rise`. Evidence: file bytes; client id 50 stayed `Rise` at 16:20 and moved `Rise` to `Idle` at 16:25:07.273 after this call.

## Open questions

- `[Animals] CobraBitesNearest` was not turned on. A bite calls the target's slot `+0x14c` with damage `0.0`. Whether that reaches the player through `combat::ForwardPuppetHurt` is UNVERIFIED.
- The screenshots show both hunters at the cobra's coordinates. The cobra mesh itself is in the grass and is not a clear read of the model. Position sync is the `drift=0.000` lines.
