// Story NPCs (guides, companions) treat the nearest living player as "the player" on the host.
// Research: re/notes/npc-follow.md (Cursor worker, 2026-10-17), re/notes/hunter-nearest.md. cAIBrain_NPC (GameDll7
// vtable 0x112d30) asks for the player once per update in its slot +0x7c (FUN_100ad050): `call 0x6ae0` at 0xad093
// returns the host hunter, and the function only uses that entity's position (+0xE8). A mid hook on the next
// instruction (0xad098, mov ebx,eax) swaps the result for the closest of the host hunter and the living puppets.
// Every other "hunter" lookup (mission, weapons, stealth, script FindEntScrObject) stays the host's. Client NPC
// brains are frozen mirrors and never run this.
// The companion's far-target field (NPC +0xb68, read only in that same function) is kept on the same entity, and
// only while it points at a player, so a removed puppet never stays there.
// The brain copies the picked pointer into the command it queues (FUN_100aae00, command +0x1c), and a queued
// command lives until its task ends. Before a puppet is destroyed, Forget() clears the queue of every NPC whose
// last pick was that puppet (cAIBrain_NPC::ClearAllInfoTask 0xac4e0, what the script StopAllTask calls).
#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>

#include <safetyhook.hpp>
#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "coop/hunter_nearest.h"
#include "coop/player_tracker.h"
#include "coop/puppets.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"

namespace dh2::coop::npc_follow {
namespace {

constexpr uintptr_t kRvaCall = 0xad093;      // E8 rel32 -> 0x6ae0 (player getter)
constexpr uintptr_t kRvaAfter = 0xad098;     // 8B D8 8B CE 89 5D E4 (mov ebx,eax / mov ecx,esi / mov [ebp-1c],ebx)
constexpr uintptr_t kRvaPlayerGetter = 0x6ae0;
constexpr uintptr_t kRvaBrainVt = 0x112d30;  // cAIBrain_NPC
constexpr uintptr_t kRvaClearAllInfoTask = 0xac4e0;  // fastcall(cAIBrain_NPC*)
constexpr uintptr_t kBrainNpc = 0x190;       // brain -> NPC entity
constexpr uintptr_t kNpcBrain = 0xc48;       // NPC -> cAIBrain_NPC
constexpr uintptr_t kNpcAiFlags = 0xb60;     // bit 0 companion
constexpr uintptr_t kNpcFarTarget = 0xb68;   // companion's stored target
constexpr uintptr_t kEntPos = 0xE8;

bool g_enabled = false;  // [Story] NpcFollowNearest (default: the [Story] HunterNearest value)
uintptr_t g_base = 0;
safetyhook::MidHook g_hook;
std::unordered_map<void*, void*> g_written;    // NPC -> the far target we last wrote
std::unordered_map<void*, void*> g_last_pick;  // NPC -> the puppet it was last pointed at
bool g_clear_ok = false;                       // ClearAllInfoTask prologue checked
int g_bad_brain_logs = 0;
uint32_t g_switch_logs = 0;

using ClearFn = void(__fastcall*)(void* brain);

bool guarded_clear(void* brain) {
    __try {
        reinterpret_cast<ClearFn>(g_base + kRvaClearAllInfoTask)(brain);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
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
    float np[3] = {};
    if (!core::safe_read(static_cast<char*>(brain) + kBrainNpc, npc) || !npc ||
        !core::safe_copy(npc + kEntPos, np, sizeof np))
        return;
    float best = 0.f;
    void* pick = hunter_nearest::NearestLivingPlayer(np, &best);
    if (!pick) pick = hunter;
    const bool is_puppet = puppets::PlayerOf(pick) != net::kInvalidPlayerId;
    if (pick != hunter) {
        ctx.eax = reinterpret_cast<uintptr_t>(pick);
        if (is_puppet) NoteTarget(npc, pick);
        if ((g_switch_logs++ % 600) == 0) {
            int16_t id = -1;
            core::safe_read(npc + 0x04, id);
            spdlog::info("npc_follow: NPC id {} follows player {} ({:.1f} m)", id, puppets::PlayerOf(pick),
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

// Test: coop_npcinfo <entity id> logs what the brain tick works with: AI type flags, the companion's far target,
// hello/follow distances, the allowed-task mask and the command queue length.
void __cdecl cmd_npcinfo(const char* arg) {
    const int id = std::atoi(core::safe_cstr(arg, 16).c_str());
    char* npc = static_cast<char*>(EntityById(id));
    char* brain = nullptr;
    uintptr_t vt = 0;
    if (!npc || !core::safe_read(npc + kNpcBrain, brain) || !brain || !core::safe_read(brain, vt) ||
        vt != g_base + kRvaBrainVt) {
        spdlog::warn("coop_npcinfo: id {} is not an NPC with a cAIBrain_NPC", id);
        return;
    }
    uint32_t flags = 0, mask = 0, state = 0;
    void* far_t = nullptr;
    float hello = 0.f, follow = 0.f;
    char* queue = nullptr;
    int count = -1;
    core::safe_read(npc + kNpcAiFlags, flags);
    core::safe_read(npc + kNpcFarTarget, far_t);
    core::safe_read(brain + 0x130, hello);
    core::safe_read(brain + 0x134, follow);
    core::safe_read(brain + 0x78, mask);
    core::safe_read(brain + 0x1a4, state);
    if (core::safe_read(brain + 0xc8, queue) && queue) core::safe_read(queue + 8, count);
    int16_t far_id = -1;
    if (far_t) core::safe_read(static_cast<char*>(far_t) + 0x04, far_id);
    // Queue entries (layout assumed: cList, element array at +4; each entry a copy of the command FUN_100aae00 got:
    // +4 priority, +8 task mask, +0x1c target entity, +0x30 "keep" flag).
    std::string items;
    char** arr = nullptr;
    if (queue && count > 0 && count < 16 && core::safe_read(queue + 4, arr) && arr) {
        for (int i = 0; i < count; ++i) {
            char* it = nullptr;
            int prio = 0, tmask = 0, keep = 0;
            void* tgt = nullptr;
            if (!core::safe_read(arr + i, it) || !it) continue;
            core::safe_read(it + 4, prio);
            core::safe_read(it + 8, tmask);
            core::safe_read(it + 0x1c, tgt);
            core::safe_read(it + 0x30, keep);
            int16_t tid = -1;
            if (tgt) core::safe_read(static_cast<char*>(tgt) + 0x04, tid);
            items += fmt::format(" [prio {} mask {:#x} target id {} keep {}]", prio, tmask, tid, keep);
        }
    }
    spdlog::info("coop_npcinfo: id {} flags={:#x} far={:#x} (id {}, player {}) hello={:.1f} follow={:.1f} mask={:#x} "
                 "state={} queue={}{}",
                 id, flags, reinterpret_cast<uintptr_t>(far_t), far_id, far_t ? puppets::PlayerOf(far_t) : 0xFF, hello, follow,
                 mask, state, count, items);
}

// Test: coop_npcclear <entity id> drops that NPC's queued commands (the script StopAllTask), so the next brain tick
// picks a player again.
void __cdecl cmd_npcclear(const char* arg) {
    const int id = std::atoi(core::safe_cstr(arg, 16).c_str());
    char* npc = static_cast<char*>(EntityById(id));
    char* brain = nullptr;
    uintptr_t vt = 0;
    const bool ok = g_clear_ok && npc && core::safe_read(npc + kNpcBrain, brain) && brain &&
                    core::safe_read(brain, vt) && vt == g_base + kRvaBrainVt && guarded_clear(brain);
    spdlog::info("coop_npcclear: id {} cleared={}", id, ok);
}

class NpcFollowModule : public dh2::Module {
public:
    const char* Name() const override { return "npc_follow"; }
    int Priority() const override { return 48; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        const int def = GetPrivateProfileIntA("Story", "HunterNearest", 0, ini.c_str());
        g_enabled = GetPrivateProfileIntA("Story", "NpcFollowNearest", def, ini.c_str()) != 0;
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_base = reinterpret_cast<uintptr_t>(game);
        if (!g_enabled) return;
        uint8_t call[5] = {}, after[7] = {}, clear[5] = {};
        static const uint8_t kAfter[7] = {0x8b, 0xd8, 0x8b, 0xce, 0x89, 0x5d, 0xe4};
        static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
        int32_t rel = 0;
        const bool ok = core::safe_copy(reinterpret_cast<void*>(g_base + kRvaCall), call, sizeof call) &&
                        core::safe_copy(reinterpret_cast<void*>(g_base + kRvaAfter), after, sizeof after);
        if (ok) std::memcpy(&rel, call + 1, sizeof rel);
        if (!ok || call[0] != 0xe8 || kRvaAfter + rel != kRvaPlayerGetter || std::memcmp(after, kAfter, sizeof after) != 0) {
            spdlog::error("npc_follow: GameDll7+{:#x} is not the expected player lookup; not hooked (wrong build?)",
                          kRvaCall);
            return;
        }
        g_clear_ok = core::safe_copy(reinterpret_cast<void*>(g_base + kRvaClearAllInfoTask), clear, sizeof clear) &&
                     std::memcmp(clear, kPrologue, sizeof clear) == 0;
        if (!g_clear_ok) spdlog::warn("npc_follow: ClearAllInfoTask at GameDll7+{:#x} has unexpected bytes", kRvaClearAllInfoTask);
        auto created = safetyhook::MidHook::create(reinterpret_cast<void*>(g_base + kRvaAfter), &on_player_lookup);
        if (!created) {
            spdlog::error("npc_follow: mid hook failed (safetyhook error {})", static_cast<int>(created.error().type));
            return;
        }
        g_hook = std::move(*created);
        spdlog::info("npc_follow: NPC brains follow the nearest player (GameDll7+{:#x})", kRvaAfter);
    }

    void OnMapUnloading() override {
        g_written.clear();
        g_last_pick.clear();
    }

    void OnFrame(float) override {
        if (!cmd_registered_ && console_cmds::Ready()) {
            cmd_registered_ = console_cmds::AddCommandStr("coop_npcinfo", &cmd_npcinfo);
            console_cmds::AddCommandStr("coop_npcclear", &cmd_npcclear);
        }
    }

private:
    bool cmd_registered_ = false;
};

DH2_MODULE(NpcFollowModule)

}  // namespace

void NoteTarget(void* npc, void* target) {
    if (npc && target) g_last_pick[npc] = target;
}

void Forget(void* ent) {
    if (!ent) return;
    void* hunter = LocalPlayer().entity;
    for (auto it = g_last_pick.begin(); it != g_last_pick.end();) {
        if (it->second != ent) {
            ++it;
            continue;
        }
        char* npc = static_cast<char*>(it->first);
        void* brain = nullptr;
        uintptr_t vt = 0;
        bool cleared = false;
        if (g_clear_ok && core::safe_read(npc + kNpcBrain, brain) && brain && core::safe_read(brain, vt) &&
            vt == g_base + kRvaBrainVt)
            cleared = guarded_clear(brain);
        void* held = nullptr;
        if (core::safe_read(npc + kNpcFarTarget, held) && held == ent) std::memcpy(npc + kNpcFarTarget, &hunter, sizeof hunter);
        int16_t id = -1;
        core::safe_read(npc + 0x04, id);
        spdlog::info("npc_follow: NPC id {} dropped its commands for a removed player (cleared={})", id, cleared);
        it = g_last_pick.erase(it);
    }
    for (auto it = g_written.begin(); it != g_written.end();) {
        if (it->second == ent) it = g_written.erase(it);
        else ++it;
    }
}

}  // namespace dh2::coop::npc_follow
