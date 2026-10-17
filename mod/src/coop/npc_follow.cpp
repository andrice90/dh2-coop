// Story NPCs (guides, companions) treat the nearest living player as "the player" on the host.
// Research: re/notes/npc-follow.md (Cursor worker, 2026-10-17). cAIBrain_NPC (GameDll7 vtable 0x112d30) asks for the
// player once per update in its slot +0x7c (FUN_100ad050): `call 0x6ae0` at 0xad093 returns the host hunter, and the
// function only uses that entity's position (+0xE8). A mid hook on the next instruction (0xad098, mov ebx,eax)
// swaps the result for the closest of the host hunter and the living puppets. Every other "hunter" lookup (mission,
// weapons, stealth, script FindEntScrObject) stays the host's. Client NPC brains are frozen mirrors and never run this.
// The companion's far-target field (NPC +0xb68, read only in that same function) is kept on the same entity, and
// only while it points at a player, so a removed puppet never stays there.
#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <unordered_map>

#include <safetyhook.hpp>
#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "coop/player_tracker.h"
#include "coop/puppets.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"

namespace dh2::coop::npc_follow {
namespace {

constexpr uintptr_t kRvaCall = 0xad093;      // E8 rel32 -> 0x6ae0 (player getter)
constexpr uintptr_t kRvaAfter = 0xad098;     // 8B D8 8B CE 89 5D E4 (mov ebx,eax / mov ecx,esi / mov [ebp-1c],ebx)
constexpr uintptr_t kRvaPlayerGetter = 0x6ae0;
constexpr uintptr_t kRvaBrainVt = 0x112d30;  // cAIBrain_NPC
constexpr uintptr_t kBrainNpc = 0x190;       // brain -> NPC entity
constexpr uintptr_t kNpcAiFlags = 0xb60;     // bit 0 companion
constexpr uintptr_t kNpcFarTarget = 0xb68;   // companion's stored target
constexpr uintptr_t kEntPos = 0xE8;

bool g_enabled = false;  // [Story] NpcFollowNearest (default off until tested)
uintptr_t g_base = 0;
safetyhook::MidHook g_hook;
std::unordered_map<void*, void*> g_written;  // NPC -> the far target we last wrote
int g_bad_brain_logs = 0;
uint32_t g_switch_logs = 0;

float dist_sq(const float a[3], const float b[3]) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

void on_player_lookup(safetyhook::Context& ctx) {
    if (!g_enabled || !NetIsHost() || !NetIsOnline()) return;
    void* hunter = reinterpret_cast<void*>(ctx.eax);
    void* brain = reinterpret_cast<void*>(ctx.esi);
    uintptr_t vt = 0;
    if (!hunter || !brain || !core::safe_read(brain, vt) || vt != g_base + kRvaBrainVt) {
        if (brain && g_bad_brain_logs++ < 3)
            spdlog::warn("npc_follow: esi {:#x} is not a cAIBrain_NPC (vt {:#x}); left alone",
                         reinterpret_cast<uintptr_t>(brain), vt);
        return;
    }
    char* npc = nullptr;
    float np[3] = {}, hp[3] = {};
    if (!core::safe_read(static_cast<char*>(brain) + kBrainNpc, npc) || !npc ||
        !core::safe_copy(npc + kEntPos, np, sizeof np))
        return;
    float best = std::numeric_limits<float>::max();
    int dead = 0;
    core::safe_read(static_cast<char*>(hunter) + 0x6CC, dead);
    if (!dead && core::safe_copy(static_cast<char*>(hunter) + kEntPos, hp, sizeof hp)) best = dist_sq(np, hp);
    void* puppet = puppets::NearestLivingPuppet(np, best);
    void* pick = puppet ? puppet : hunter;
    if (puppet) {
        ctx.eax = reinterpret_cast<uintptr_t>(puppet);
        if ((g_switch_logs++ % 600) == 0) {
            int16_t id = -1;
            core::safe_read(npc + 0x04, id);
            spdlog::info("npc_follow: NPC id {} follows player {} ({:.1f} m)", id, puppets::PlayerOf(puppet),
                         std::sqrt(best));
        }
    }
    // Companion far target: only touch it while it holds a player (the host hunter, or what we wrote last time).
    uint32_t flags = 0;
    void* held = nullptr;
    if (core::safe_read(npc + kNpcAiFlags, flags) && (flags & 1) && core::safe_read(npc + kNpcFarTarget, held)) {
        const auto it = g_written.find(npc);
        const bool ours = held == hunter || (it != g_written.end() && it->second == held);
        if (ours && held != pick) {
            std::memcpy(npc + kNpcFarTarget, &pick, sizeof pick);
            g_written[npc] = pick;
        }
    }
}

class NpcFollowModule : public dh2::Module {
public:
    const char* Name() const override { return "npc_follow"; }
    int Priority() const override { return 48; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_enabled = GetPrivateProfileIntA("Story", "NpcFollowNearest", 0, ini.c_str()) != 0;
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_base = reinterpret_cast<uintptr_t>(game);
        if (!g_enabled) return;
        uint8_t call[5] = {}, after[7] = {};
        static const uint8_t kAfter[7] = {0x8b, 0xd8, 0x8b, 0xce, 0x89, 0x5d, 0xe4};
        int32_t rel = 0;
        const bool ok = core::safe_copy(reinterpret_cast<void*>(g_base + kRvaCall), call, sizeof call) &&
                        core::safe_copy(reinterpret_cast<void*>(g_base + kRvaAfter), after, sizeof after);
        if (ok) std::memcpy(&rel, call + 1, sizeof rel);
        if (!ok || call[0] != 0xe8 || kRvaAfter + rel != kRvaPlayerGetter || std::memcmp(after, kAfter, sizeof after) != 0) {
            spdlog::error("npc_follow: GameDll7+{:#x} is not the expected player lookup; not hooked (wrong build?)",
                          kRvaCall);
            return;
        }
        auto created = safetyhook::MidHook::create(reinterpret_cast<void*>(g_base + kRvaAfter), &on_player_lookup);
        if (!created) {
            spdlog::error("npc_follow: mid hook failed (safetyhook error {})", static_cast<int>(created.error().type));
            return;
        }
        g_hook = std::move(*created);
        spdlog::info("npc_follow: NPC brains follow the nearest player (GameDll7+{:#x})", kRvaAfter);
    }

    void OnMapUnloading() override { g_written.clear(); }
};

DH2_MODULE(NpcFollowModule)

}  // namespace
}  // namespace dh2::coop::npc_follow
