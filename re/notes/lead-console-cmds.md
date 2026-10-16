# Console command registration — lead analysis (2026-10-16)

Source: lead decompile `re/notes/lead-console-decomp.c`, `lead-console-decomp2.c` (EngineDll7, slot L). Supersedes the registration notes in `console.md` where they differ.

## `cConsole::AddStaticFuncf` is **__cdecl**, not __thiscall
- Export `?AddStaticFuncf@cConsole@@QAAPAVcConsoleFunc@@IPADP6AXXZZZ` (RVA `0x166bd0`). `QAA` = public member, **__cdecl**: MSVC compiles variadic member functions as cdecl with `this` pushed as the first stack argument. Ghidra labels it `__thiscall`, which shifts every parameter by one in its output (`AddSlot((cConsole*)param_1, ...)` is really `this`).
- Real signature: `cConsoleFunc* __cdecl AddStaticFuncf(cConsole* self, unsigned flags, char* name, void (__cdecl* fn)(void), int typeTag..., -1)`.
- Body: `malloc(0x40)` → `cConsoleFunc(name)`; `func+0x10 |= flags`; `SetFunction(&fn)` (stores fn at `+0x2c`, clears `+0x34` object ptr); for each tag until `-1`: `AddParam(FUN_10165d50(0, tag))`; `AddSlot(self, func)`; `TestFunction`.
- Calling it as `__thiscall` (console in ECX) would put garbage in every argument and crash.

## Type tags (`FUN_10165d50`, RVA `0x165d50`)
`1`,`2` → 0x24-byte vars; `3` → 0x28; `4` int, `5`, `6` float (0x30 each); `7` string (0x30, default `DAT_10407c9d`). Tags 4/6/7 per Track D; 1–3, 5 unverified names.

## Dispatch (`cConsoleFunc::CallFunction`, RVA `0x169cf0`)
1. `this->vfunc[+0xc](line)` = `Scan` parses the argument text into the param vars.
2. Flags maintained by `AddParam` (`0x1696b0`): `+0x38` "all params are float" (cleared by any non-float param), `+0x3c` "no float params" (cleared by any float param). Both start at 1.
3. Static function (`+0x34 == 0`):
   - all-float (`+0x38 != 0 && +0x3c == 0`): handler at `+0x2c` called with each param's `GetNumber()` (vfunc `+0x20`) as `float` args.
   - otherwise: handler called with each param's raw 32-bit value from vfunc `+0x50` (string var → `char*`, int var → int). Param array: `func+0x10` base, count via `+0x04` (+1).
   - Handler call is a plain cdecl call with N args (caller cleans).
4. Member function (`+0x34 != 0`): same shapes through `+0x30` with object `+0x34`.

## How we register (implemented in `mod/src/engine/console_cmds.*`)
```cpp
using AddStaticFuncf_t = void* (__cdecl*)(void* self, unsigned flags, const char* name, void (__cdecl* fn)(), ...);
add(console, 0, "coop_host", reinterpret_cast<void(__cdecl*)()>(&CoopHost), 7, -1);   // void __cdecl CoopHost(const char* arg)
add(console, 0, "coop_join", reinterpret_cast<void(__cdecl*)()>(&CoopJoin), 7, -1);   // arg "ip[:port]"
add(console, 0, "coop_status", reinterpret_cast<void(__cdecl*)()>(&CoopStatus), -1);  // no args
```
- One string param per command; multi-value input is packed (`ip:port`) because whether a string param consumes the whole rest of the line is **UNVERIFIED** (Scan not traced). Handlers must tolerate `nullptr`/empty strings.
- Flag `0` = visible without `-developper`.
- Register after `*(&console) != nullptr` (InitConsole done), once.

## Runtime proof required
`Console::Exec("coop_status")` and `Exec("coop_join 127.0.0.1:27420")` from our own code → handler logs the received string. If the handler receives garbage, re-check vfunc `+0x50` for the string var type (`FUN_1016cab0` vtable).
