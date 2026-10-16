// Slow motion off for everyone in co-op (user decision 2026-10-15).
// Evidence: re/notes/lead-slowmo.md. cHUDSlowMotion::Enable (GameDll7 RVA 0xca690) is the only path
// that applies the slow-motion frametime; we detour it to a no-op and reset `frametime 1` per map.
#include <windows.h>

#include <cstdint>
#include <cstring>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "core/hook.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console.h"

namespace dh2::coop {
namespace {

constexpr uintptr_t kEnableRva = 0xca690;
constexpr uintptr_t kEnableScopeTableRva = 0xf62d0;  // relocated imm32 of `push` at Enable+5

safetyhook::InlineHook* g_enable_hook = nullptr;
std::uint32_t g_blocked = 0;

void __fastcall hk_Enable(void* self) {
    if (Settings().disable_slowmo) {
        if (g_blocked++ < 5) spdlog::info("slowmo: cHUDSlowMotion::Enable blocked (co-op policy)");
        return;
    }
    g_enable_hook->unsafe_fastcall<void>(self);
}

// 55 8B EC 6A FF 68 <imm32 == base + 0xf62d0>  (push ebp; mov ebp,esp; push -1; push scopetable)
bool signature_ok(uintptr_t base) {
    std::uint8_t b[10] = {};
    if (!core::safe_copy(reinterpret_cast<void*>(base + kEnableRva), b, sizeof b)) return false;
    static const std::uint8_t prologue[6] = {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68};
    if (std::memcmp(b, prologue, sizeof prologue) != 0) return false;
    std::uint32_t imm = 0;
    std::memcpy(&imm, b + 6, 4);
    return imm == base + kEnableScopeTableRva;
}

class SlowMoModule : public dh2::Module {
public:
    const char* Name() const override { return "slowmo"; }
    int Priority() const override { return 60; }

    void OnGameDllLoaded(HMODULE game) override {
        if (!Settings().disable_slowmo) {
            spdlog::info("slowmo: DisableSlowMo=0, leaving slow motion alone");
            return;
        }
        const uintptr_t base = reinterpret_cast<uintptr_t>(game);
        if (!signature_ok(base)) {
            spdlog::warn("slowmo: Enable signature mismatch at GameDll7+{:#x}; hook skipped (frametime reset only)",
                         kEnableRva);
            return;
        }
        g_enable_hook = core::install_hook("cHUDSlowMotion::Enable", reinterpret_cast<void*>(base + kEnableRva),
                                           reinterpret_cast<void*>(&hk_Enable));
    }

    void OnMapLoaded(const char*) override {
        if (!Settings().disable_slowmo) return;
        const int r = console::Exec("frametime 1");
        spdlog::info("slowmo: queued 'frametime 1' -> {}", r);
    }
};

DH2_MODULE(SlowMoModule)

}  // namespace
}  // namespace dh2::coop
