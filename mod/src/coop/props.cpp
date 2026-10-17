// Shootable world props (bottles and stumps, cObjTarget). A client's bullet does not run OnHit.
// The host breaks the prop, counts a bottle toward the mission, and commits kPropHit. Every client
// plays Boom and, for a stump, increments its own profile tally. Design: re/notes/props-research.md.
#include <map>
#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_set>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "coop/events.h"
#include "coop/player_tracker.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"

namespace dh2::coop::props {
namespace {

// GameDll7 v1.0 build 434. re/notes/props-research.md, re/notes/props-target.c.
constexpr uintptr_t kRvaOnHit = 0x277f0;             // __thiscall(cObjTarget*, cObjEntity* other)
constexpr uintptr_t kRvaBoom = 0x27040;              // __thiscall(cObjTarget*)
constexpr uintptr_t kRvaUpdateMission = 0x8390;      // __cdecl(int action, int value)
constexpr uintptr_t kRvaProfileAt = 0x4e20;          // __thiscall(table*, int index) -> cHunterProfile*
constexpr uintptr_t kRvaProfileTable = 0x165888;     // DAT_10165888, the table OnHit passes with index 0
constexpr uintptr_t kRvaMissionGate = 0x1657f0;      // DAT_101657f0, bottle count runs only when this is non-zero
constexpr uintptr_t kRvaTargetVt = 0x1017c8;         // cObjTarget vtable
constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kBroken = 0x208;                 // 0 intact, 1 broken
constexpr uintptr_t kStump = 0x21c;                  // generic/cheat: 1 stump, 0 bottle
constexpr uintptr_t kTally = 0x90;                   // tMissionStatus stump count
constexpr int kTargetCan = 0x25;

using BoomFn = void(__thiscall*)(void* self);
using UpdateMissionFn = void(__cdecl*)(int action, int value);
using ProfileAtFn = void*(__thiscall*)(void* table, int index);
using MissionStatusFn = void*(__thiscall*)(void* profile);
using KindOfFn = int(__thiscall*)(void* self, const char* name);

HMODULE g_game = nullptr;
safetyhook::InlineHook* g_hit_hook = nullptr;
BoomFn g_boom = nullptr;
UpdateMissionFn g_update_mission = nullptr;
ProfileAtFn g_profile_at = nullptr;
MissionStatusFn g_mission_status = nullptr;
KindOfFn g_kind_of = nullptr;
bool g_in_apply = false;
bool g_targets_logged = false;
int g_scan_tries = 0;
float g_scan_wait = 0.f;
std::unordered_set<uint16_t> g_pending;

void* CheckedRva(uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    if (!g_game) return nullptr;
    void* p = reinterpret_cast<char*>(g_game) + rva;
    uint8_t got[5] = {};
    if (!core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("props: {} at GameDll7+{:#x} has unexpected bytes; not used", label, rva);
        return nullptr;
    }
    return p;
}

int ReadInt(void* ent, uintptr_t off) {
    int v = 0;
    if (!ent || !core::safe_read(static_cast<char*>(ent) + off, v)) return 0;
    return v;
}

bool WriteInt(void* ent, uintptr_t off, int v) {
    if (!ent) return false;
    __try {
        *reinterpret_cast<int*>(static_cast<char*>(ent) + off) = v;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int IdOf(void* ent) {
    int16_t id = -1;
    if (!ent || !core::safe_read(static_cast<char*>(ent) + kEntId, id)) return -1;
    return id;
}

bool IsTarget(void* ent) {
    void* vt = nullptr;
    if (!ent || !g_game || !core::safe_read(ent, vt)) return false;
    return vt == reinterpret_cast<char*>(g_game) + kRvaTargetVt;
}

bool IsStump(void* ent) { return ReadInt(ent, kStump) == 1; }

bool IsBullet(void* other) {
    if (!other || !g_kind_of) return false;
    __try {
        return g_kind_of(other, "mod_obj_bullet") != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void CallOriginal(void* self, void* other) {
    if (!g_hit_hook || !self) return;
    __try {
        g_hit_hook->unsafe_thiscall<void>(self, other);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        spdlog::error("props: OnHit faulted id={}", IdOf(self));
    }
}

bool CallBoom(void* self) {
    if (!g_boom || !self) return false;
    __try {
        g_boom(self);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        spdlog::error("props: Boom faulted id={}", IdOf(self));
        return false;
    }
}

// Same profile OnHit increments: FUN_10004e20(&DAT_10165888, 0) then cHunterProfile::GetMissionStatus().
// The stump count is tMissionStatus +0x90 (props-target.c).
void* LocalMissionStatus() {
    if (!g_game || !g_profile_at || !g_mission_status) return nullptr;
    void* table = reinterpret_cast<char*>(g_game) + kRvaProfileTable;
    void* profile = nullptr;
    __try {
        profile = g_profile_at(table, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        profile = nullptr;
    }
    if (!profile) return nullptr;
    // GetMissionStatus() indexes profile+0x11e8 by the byte at +0x23d8. Out of range still returns a pointer.
    unsigned char mission = 0xFF;
    if (!core::safe_read(static_cast<char*>(profile) + 0x23d8, mission) || mission >= 0x1c) {
        spdlog::warn("props: profile mission index {} out of range", mission);
        return nullptr;
    }
    void* status = nullptr;
    __try {
        status = g_mission_status(profile);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = nullptr;
    }
    return status;
}

void AddStumpTally(int id) {
    void* status = LocalMissionStatus();
    int before = 0;
    if (!status || !core::safe_read(static_cast<char*>(status) + kTally, before) ||
        !WriteInt(status, kTally, before + 1)) {
        spdlog::warn("props: stump tally id={} not written", id);
        return;
    }
    const int after = ReadInt(status, kTally);
    spdlog::info("props: stump tally id={} {} -> {}", id, before, after);
}

void CountBottle(int id) {
    int gate = 0;
    if (!g_game || !core::safe_read(reinterpret_cast<char*>(g_game) + kRvaMissionGate, gate) || gate == 0) {
        spdlog::info("props: UpdateMissionStatus skipped id={} DAT_101657f0={}", id, gate);
        return;
    }
    if (!g_update_mission) {
        spdlog::warn("props: UpdateMissionStatus unavailable id={}", id);
        return;
    }
    __try {
        g_update_mission(1, kTargetCan);
        spdlog::info("props: UpdateMissionStatus(1, 0x25) id={}", id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        spdlog::error("props: UpdateMissionStatus faulted id={}", id);
    }
}

// Boom, then the broken flag. mission_count is host-only (a bottle). A stump tallies on this process.
bool BreakIntact(void* ent, bool mission_count, const char* via) {
    if (!IsTarget(ent) || ReadInt(ent, kBroken) != 0) return false;
    const int id = IdOf(ent);
    const bool stump = IsStump(ent);
    g_in_apply = true;
    const bool boom = CallBoom(ent);
    const bool flagged = WriteInt(ent, kBroken, 1);
    g_in_apply = false;
    spdlog::info("props: break id={} stump={} boom={} flag={} via={}", id, stump ? 1 : 0, boom, flagged, via);
    if (stump) AddStumpTally(id);
    else if (mission_count) CountBottle(id);
    return true;
}

void CommitHit(void* ent, uint8_t actor) {
    const int id = IdOf(ent);
    if (id < 0 || !NetIsHost()) return;
    events::Commit(events::kPropHit, static_cast<uint16_t>(id), IsStump(ent) ? 1u : 0u, 0.f, "", actor);
}

void RequestHit(void* ent) {
    const int id = IdOf(ent);
    if (id < 0) return;
    if (ReadInt(ent, kBroken) != 0) {
        spdlog::info("props: id={} already broken", id);
        return;
    }
    // OnHit runs every frame while the bullet touches the target: one request, no log per frame.
    if (g_pending.count(static_cast<uint16_t>(id)) != 0) return;
    g_pending.insert(static_cast<uint16_t>(id));
    const uint32_t stump = IsStump(ent) ? 1u : 0u;
    events::Request(events::kPropHit, static_cast<uint16_t>(id), stump, 0.f, "");
    spdlog::info("props: client hit id={} stump={} requested", id, stump);
}

void __fastcall hk_OnHit(void* self, void*, void* other) {
    if (g_in_apply || !NetIsOnline()) {
        CallOriginal(self, other);
        return;
    }
    if (NetIsHost()) {
        const int before = ReadInt(self, kBroken);
        CallOriginal(self, other);
        const int after = ReadInt(self, kBroken);
        // Diagnostic for the user's report (host shots not reaching the client, 2026-10-17 16:05): one line per
        // target per state change, whatever the outcome.
        static std::map<int, int> logged;
        const int id = IdOf(self);
        if (logged[id] != (before + 1) * 10 + after) {
            logged[id] = (before + 1) * 10 + after;
            spdlog::info("props: host OnHit id={} target={} bullet={} broken {}->{}", id, IsTarget(self),
                         IsBullet(other), before, after);
        }
        if (before == 0 && after != 0 && IsTarget(self)) {
            spdlog::info("props: host hit id={} stump={} broke", IdOf(self), IsStump(self) ? 1 : 0);
            CommitHit(self, 0);
        }
        return;
    }
    if (!IsBullet(other)) {
        CallOriginal(self, other);
        return;
    }
    if (!IsTarget(self)) return;
    RequestHit(self);
}

void OnPropRequest(const net::GameEvent& ev) {
    void* ent = EntityById(ev.subjectId);
    if (!IsTarget(ent)) {
        spdlog::warn("props: request id={} is not a cObjTarget", ev.subjectId);
        return;
    }
    if (!BreakIntact(ent, true, "request")) {
        spdlog::info("props: request id={} already broken", ev.subjectId);
        return;
    }
    CommitHit(ent, ev.actorId);
}

void OnPropCommit(const net::GameEvent& ev) {
    g_pending.erase(ev.subjectId);
    void* ent = EntityById(ev.subjectId);
    if (!IsTarget(ent)) {
        spdlog::warn("props: commit id={} is not a cObjTarget here", ev.subjectId);
        return;
    }
    if (!BreakIntact(ent, false, "commit")) {
        spdlog::info("props: commit id={} already broken", ev.subjectId);
    }
}

void LogTargets() {
    if (g_targets_logged) return;
    int found = 0;
    bool any_entity = false;
    for (int id = 0; id < 1024; ++id) {
        void* ent = EntityById(id);
        if (!ent) continue;
        any_entity = true;
        if (!IsTarget(ent)) continue;
        float pos[3] = {};
        core::safe_copy(static_cast<char*>(ent) + kEntPos, pos, sizeof pos);
        spdlog::info("props: target id={} pos=({:.1f},{:.1f},{:.1f}) {}", id, pos[0], pos[1], pos[2],
                     IsStump(ent) ? "stump" : "bottle");
        ++found;
    }
    if (!any_entity && g_scan_tries < 20) return;
    if (found == 0) spdlog::info("props: no cObjTarget on this map");
    g_targets_logged = true;
}

void __cdecl cmd_prophit(const char* arg) {
    const int id = std::atoi(core::safe_cstr(arg, 32).c_str());
    if (id < 0) {
        spdlog::warn("props: usage coop_prophit <entity id>");
        return;
    }
    void* ent = EntityById(id);
    if (!IsTarget(ent)) {
        spdlog::warn("props: coop_prophit {} is not a cObjTarget", id);
        return;
    }
    spdlog::info("props: coop_prophit id={} stump={}", id, IsStump(ent) ? 1 : 0);
    if (!NetIsOnline() || NetIsHost()) {
        if (!BreakIntact(ent, true, "coop_prophit")) {
            spdlog::info("props: coop_prophit id={} already broken", id);
            return;
        }
        CommitHit(ent, 0);
        return;
    }
    RequestHit(ent);
}

class PropsModule : public dh2::Module {
public:
    const char* Name() const override { return "props"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        events::SetHandler(events::kPropHit, &OnPropRequest, &OnPropCommit);
    }

    void OnEngineReady(HMODULE engine) override {
        if (!engine) return;
        g_kind_of = core::find_export_as<KindOfFn>(engine, "?IsEntityKindOf@cObjEntity@@QBEHPBD@Z");
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = game;
        g_boom = reinterpret_cast<BoomFn>(CheckedRva(kRvaBoom, "cObjTarget::Boom"));
        g_update_mission = reinterpret_cast<UpdateMissionFn>(CheckedRva(kRvaUpdateMission, "UpdateMissionStatus"));
        g_profile_at = reinterpret_cast<ProfileAtFn>(CheckedRva(kRvaProfileAt, "profile table"));
        g_mission_status = core::find_export_as<MissionStatusFn>(
            game, "?GetMissionStatus@cHunterProfile@@QAEPAUtMissionStatus@@XZ");
        g_hit_hook = core::install_hook("cObjTarget::OnHit (0x277f0)", CheckedRva(kRvaOnHit, "cObjTarget::OnHit"),
                                        reinterpret_cast<void*>(&hk_OnHit));
    }

    void OnMapLoaded(const char*) override {
        g_pending.clear();
        g_targets_logged = false;
        g_scan_tries = 0;
        g_scan_wait = 0.f;
        LogTargets();
    }

    void OnFrame(float dt) override {
        if (!g_cmd && console_cmds::Ready()) g_cmd = console_cmds::AddCommandStr("coop_prophit", &cmd_prophit);
        if (g_targets_logged) return;
        g_scan_wait += dt;
        if (g_scan_wait < 0.5f) return;
        g_scan_wait = 0.f;
        ++g_scan_tries;
        LogTargets();
    }

    bool g_cmd = false;
};

DH2_MODULE(PropsModule)

}  // namespace
}  // namespace dh2::coop::props
