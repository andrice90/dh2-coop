# P4b — shared animals: host broadcast + client apply

## Progress

Done and tested with two instances (2026-10-16 16:05). Hit application is implemented and not yet seen in a log: no deer was in view to shoot. Visual check is with the user.

## What it does

Host, while at least one client is connected, every 50 ms (`net::kStateIntervalMs`):

- Reads each animal from `Animals()`: id `+0x04`, position `+0xE8`, health `+0x19C`, body-state name.
- Yaw starts as `+0xF4.y`. After that it is `atan2(dx, dz)` in degrees from the move since the last send, and it updates only when that step is longer than 0.05 m.
- Flags: bit0 dead when health ≤ 0, bit1 parked when `y < -1000`.
- `Session::SendAnimalStates` (unreliable).

Client:

- On the first snapshot for an id: `FreezeBrain(true)`. Physics stays linked.
- Each frame, in the pre-render callback: 100 ms interpolation (position lerp, shortest-path yaw, state/health/flags from the newer snapshot), then `HandleKey` position in centimetres and angle, world-box refresh, `BspUpdate`.
- The first pose is applied for 3 frames (lead workaround for a big teleport). Whether that stops the model vanishing is UNVERIFIED: no deer was in either capture.
- If the live body-state name is not the sampled name, `SetBodyState`. Needed because `FreezeBrain` does not hold the state machine.
- Before writing host health to `+0x19C`: a local drop of more than 0.01 is `AnimalHit`. The host calls `entity::InflictDamage` (`FuncInflictDamage`).

`[Animals] Spike` does not run while a session is active. New ini keys are in `mod/dh2coop.ini`.

## Test

`.\tools-src\move-test.ps1`, two runs. Logs: `game\Bin\dh2coop.log` (host), `game2\Bin\dh2coop.log` (client).

Both runs: host logged `animals: host snapshot 5 animals seq=1`, client logged `client received 5 animals seq=1`, then `mirror freeze id=… ok=true` for 120–124. The client's first place moved deer off their own spawns. Example from 16:04:43:

| id | client spawn | first target (host) |
|---|---|---|
| 120 | (-0.9, 2.7, 63.0) | (-51.6, 7.9, -65.9) |
| 122 | (-62.2, 14.1, -80.8) | (56.8, 2.0, -46.1) |
| 123 | (-10.4, 2.2, -35.1) | (108.1, 0.6, 58.7) |
| 124 | (-10000, -10000, -10000) | same (parked puma) |

The 2 s sync line in the same OnFrame as the first packet is still the spawn (pre-render has not run yet). Every later pair, matched to the nearest host line (about 20 ms apart):

**Run 2 (shipped code, 16:04–16:05), 28 pairs per id after that first line**

| id | max position error | max yaw error | state mismatches |
|---|---|---|---|
| 120 | 0.000 m | 0 | 0 |
| 121 | 0.010 m | 0 | 0 |
| 122 | 0.000 m | 0 | 0 |
| 123 | 0.000 m | 0 | 0 |
| 124 | 0.000 m | 0 | 0 |

**Run 1 (before the re-assert fix), deer 122 actually walked.** At 16:02:06 and 16:02:08 both sides had `Walk`, yaw 171.4, position error 0.113 m. Drift of the client entity against the pose we had just written was 0.000, so physics did not pull it back. Deer 121's live state became `IdleAttack` for about 4 s (16:01:44–16:01:48) while every host sample was `CutScene`. Run 2 does not show that leak.

Sustained names follow. `CutScene` and the parked puma's `IdleAttack` matched. `Walk` on run 1 matched on the next 2 s sample (well under 300 ms; the change and the sample are 1 s apart in the log, and both sides already agreed at 16:02:06).

`PostPlayAnim` on the host lasts about 10 ms (`16:01:48.425` then `CutScene` 10 ms later, same pattern every ~3.3 s). The snapshot period is 50 ms, so most of those blips are never in a packet. When one is, the client holds it for about one period (run 2: client `PostPlayAnim` for 50 ms at 16:05:24, host blip was ~10 ms). That is a sampling limit, not a stuck state.

Screenshots `re/notes/coop-local/move-160438-game-2walked.png` and `move-160438-game2-2walked.png`: no deer in frame. The client view shows the host hunter as the mountain-guide puppet, walking. Deer on this map were tens of metres from the spawn the hunter walked toward.

No `AnimalHit` line. A scripted shot was not aimed at a deer.

Puppet replication still ran (client log: `handgun3/walkforwardalert`, `handgun3/run`).

## Problems

- First sync log line of a session is the pre-move spawn. The place log is the one that shows the teleport.
- Grazing `PostPlayAnim` pulses are shorter than the send interval.
- Model visibility after the first teleport was not checked (nothing in view).
- Remote damage, flee, and death are not exercised.

## Proposed MEMORY facts

- Client animal drive that keeps the deer drawn: `FreezeBrain(true)`, physics kept, pre-render `HandleKey("position")` / `HandleKey("angle")`, world AABB refresh, `BspUpdate`. Evidence: `game2\Bin\dh2coop.log` 16:04:43 `mirror freeze` / `mirror first place`, and later `animals: sync` pairs with position error ≤ 0.010 m (run 2) and 0.113 m while walking (run 1, id 122). `drift=0.000` against the written target.
- Movement yaw `atan2(dx, dz)` degrees matched on both sides when a deer moved (id 122, yaw 171.4, 16:02:06, both logs).
- `FreezeBrain` does not pin the body-state name. Run 1 client id 121 logged `IdleAttack` for ~4 s while the host stayed `CutScene`. Re-applying `SetBodyState` whenever the live name differs removed that on run 2.
- Host grazing `PostPlayAnim` lasts ~10 ms (host log 16:01:48.425, 10 ms until `CutScene`). A 50 ms snapshot usually misses it.
