// dh2coop.asi entry point. Loaded by Ultimate ASI Loader (Bin\dinput8.dll) from the game's Bin folder.
// DllMain installs the single-instance mutex hook, then spawns the init thread. The mutex is created
// on the game thread as soon as InitEngineDll returns, so that hook cannot wait for the init thread.
#include <windows.h>

#include "core/config.h"
#include "core/lifecycle.h"
#include "core/log.h"
#include "core/module.h"
#include "engine/dinput_hooks.h"

namespace {

DWORD WINAPI init_thread(LPVOID) {
    using namespace dh2::core;
    const Config cfg = load_config((self_dir() / "dh2coop.ini").string());
    set_config(cfg);
    log_init(cfg.log_level);
    spdlog::info("dh2coop {} loaded from {}", "0.2.0-framework", self_path().string());
    spdlog::info("cmdline: {}", GetCommandLineA());
    flush_mutex_log();
    install_crash_handlers();
    dh2::modules::Start();
    dh2::modules::DispatchInit();
    run_lifecycle();
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        dh2::core::set_self_module(self);
        dh2::core::install_mutex_hook();
        // Before this DllMain returns, EngineDll7 has not called DirectInput8Create yet.
        dh2::dinput::TryInstall();
        if (HANDLE t = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr)) CloseHandle(t);
    } else if (reason == DLL_PROCESS_DETACH) {
        dh2::core::request_shutdown();
    }
    return TRUE;
}
