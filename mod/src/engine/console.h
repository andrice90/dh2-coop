#pragma once
// Thin wrapper over the exported cConsole. Does not register commands.
#include <windows.h>

namespace dh2::console {

// Resolve exports. The global cConsole* stays null until InitConsole runs.
bool Bind(HMODULE engine);

// True when the global pointer is non-null and Exec is resolved.
bool Ready();

// Queues the line via cConsole::Exec(line, 0). Returns 1 queued, 0 queue full, -1 not ready.
// Lines of 127 characters or more are rejected (the engine slot is 0x80 bytes).
int Exec(const char* line);

// Runs the line immediately via cConsole::RealExec. Same return convention, except the
// engine's own return value is passed through when the call is made.
int RealExec(const char* line);

// Called after a successful cBspTree::Load. Executes [Console] ExecAfterMap once.
void OnMapLoaded(const char* bspName);

}  // namespace dh2::console
