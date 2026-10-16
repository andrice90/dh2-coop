# Slow motion — lead analysis (2026-10-16)

Source: `re/notes/lead-slowmo-decomp.c` (GameDll7, slot L), bytes via pefile/capstone.

## Code
- `cHUDSlowMotion::Enable` = GameDll7 RVA `0xca690` (`void __fastcall Enable(cHUDSlowMotion* this)`; `this` in ECX, no stack args, plain `ret`).
  - Bails if input/flag query `(*DAT_101657ec)->vfunc[+0x60](0x20000)` is nonzero.
  - If not already active (`+0x78 == 0`) and `+0xe4 == 0`: `SetFrameTime(this, DAT_10164d38)` (the slow-mo frametime, cvar `mod_slowmotionFrameTime`, default 0.35), stores meter params, starts a render effect at `render+0x47c`.
  - Sets `+0xd8 = 1`, `+0x78 = 1` (active).
- `cHUDSlowMotion::SetFrameTime` = RVA `0xca400` (`__thiscall(this, float t)`): `RealExec("frametime %.5f")`, `this+0xd0 = t`, sound pitch `sound->vfunc[+0x68](0x14, t)`, global `DAT_101608a4 = t`.
- So slow motion = the global console var `frametime` (Track D: default 1.0, range 0.001–100) set per client.

## Co-op policy (user: slow-mo off for everyone)
1. Hook `Enable` and return immediately while co-op is active (no time change, no HUD effect, no sound pitch).
2. At every map load in co-op, `Exec("frametime 1")` to undo any stale value.

## Signature check (GameDll7 v1.0 only, hash in MEMORY.md)
At `base + 0xca690`: `55 8B EC 6A FF 68 <imm32>` with `imm32 == base + 0xf62d0` (relocated SEH scope table pointer). If the check fails, log and leave slow-mo alone (fall back to `frametime 1` on map load only).
`SetFrameTime` at `base + 0xca400`: `55 8B EC 6A FF 68 <imm32 == base + 0xf62b0>`.
