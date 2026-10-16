#pragma once
// Registers our own console commands through the engine's cConsole::AddStaticFuncf.
// ABI notes (re/notes/lead-console-cmds.md): AddStaticFuncf is variadic, so MSVC compiled it __cdecl
// with `this` as the first stack argument (mangled QAA). Handlers are plain __cdecl; a string
// parameter (type tag 7) arrives as `const char*`.
#include <windows.h>

namespace dh2::console_cmds {

using Handler0 = void(__cdecl*)();
using HandlerStr = void(__cdecl*)(const char* arg);

// Resolves AddStaticFuncf and the console global. Safe to call from any thread.
bool Bind(HMODULE engine);
// True once the engine's console object exists (InitConsole ran).
bool Ready();
// Must be called on the game thread after Ready(). `name` must have static storage duration.
bool AddCommand0(const char* name, Handler0 fn, unsigned flags = 0);
bool AddCommandStr(const char* name, HandlerStr fn, unsigned flags = 0);

}  // namespace dh2::console_cmds
