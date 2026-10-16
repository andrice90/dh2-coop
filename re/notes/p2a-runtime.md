# P2a runtime notes — mod framework

**Progress:** docs and code not runtime-proven yet. Sections below fill in as each launch finishes.

Scaffolding job only. No command registration, slow-motion hook, or player tracker.

## Build

Pending `mod\build.ps1 -Clean`.

## Console Exec proof

Pending. Plan: one instance, `[Probe] Enabled=1`, `[Console] ExecAfterMap=cam_fov 70`, `+mod data -nointro -developper -cheats -spmap HUNTMAP4`.

Expect `console::Exec('cam_fov 70')` in `dh2coop.log`, the probe's `cConsole::Exec` line, and no `symbol 'cam_fov' not found` in `debug.log`.

## Heartbeat proof

Pending. `heartbeat` lines every 10 s of `Present` time in `dh2coop.log`.

## Two instances

Pending `run-coop-local.ps1`. Both `debug.log` files should contain `map 'HUNTMAP4' loaded succesfully`. Both `dh2coop.log` files should contain `heartbeat`.

## Problems

None yet.

## Open questions

- Does a clean shutdown rewrite `Data\Cfg\console.cfg` to `cam_fov = 70` after `Exec`? That is the "took effect" check beyond the Exec log line.
- Does `CreateMutexA` renaming actually let the second process reach the map without `-join`? The decompile says the second process exits when the mutex already exists and neither `-join` nor `-dedicated` is set. Runtime not run yet.
- Crash minidump path is implemented and not yet forced.

## Proposed MEMORY facts

Filled after the runs, each with a log line or export name.
