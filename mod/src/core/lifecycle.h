#pragma once
// Process-lifetime services owned by the ASI init thread: mutex rename, crash dump, DLL watch.
#include "core/config.h"

namespace dh2::core {

// Call from DllMain before the init thread starts. CreateMutexA runs on the game thread
// immediately after InitEngineDll returns, which is too soon to wait for the init thread.
void install_mutex_hook();
void flush_mutex_log();

void install_crash_handlers();
void request_shutdown();

// Blocks the ASI init thread until process detach. Dispatches module events.
void run_lifecycle();

}  // namespace dh2::core
