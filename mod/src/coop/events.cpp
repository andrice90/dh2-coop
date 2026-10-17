// Discrete world events. Design: re/notes/p4c-events.md, engine map: re/notes/lead-mission.md.
// GameEvent is the only packet. Kinds are a registry so another map does not need a new message id.
#include "coop/events.h"

#include <windows.h>
#include <intrin.h>

#include <cctype>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/animals.h"
#include "coop/coop.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console.h"

namespace dh2::coop::events {
namespace {

constexpr size_t kJournalCap = 64;

// GameDll7 v1.0 (build 434). Every hooked body starts `55 8b ec 6a ff` (SEH frame); checked before hooking.
constexpr uintptr_t kRvaMission = 0x1869e8;         // cMission* singleton (scripts' GlobalsCheckMissionStatus reads it)
constexpr uintptr_t kRvaMissionMessage = 0x91c20;   // __thiscall(cMission*, char code, const char* text): msg 0x87
constexpr uintptr_t kRvaCheckMission = 0x94e50;     // __thiscall(cMission*): all tasks done -> success branch
constexpr uintptr_t kRvaHunterMissionEnd = 0x50760; // __thiscall(hunter, int 0): called in the success branch
constexpr uintptr_t kRvaClaimCallback = 0x1cd40;    // __cdecl(cObjAnimal*): clears +0x404, runs the script callback
constexpr uintptr_t kMissionStatus = 0x254;         // 1 running, 2 success, 3 failed
constexpr uintptr_t kMissionHunter = 0x258;
constexpr uintptr_t kHunterProfile = 0x1f8;
constexpr uintptr_t kAnimalClaimable = 0x404;
constexpr uintptr_t kAnimalHealth = 0x19C;
constexpr char kMsgSuccess = 5;
constexpr char kMsgFailed = 6;

struct Handlers {
    Handler request = nullptr;
    Handler commit = nullptr;
};

HMODULE g_game = nullptr;
safetyhook::InlineHook* g_msg_hook = nullptr;
safetyhook::InlineHook* g_check_hook = nullptr;
safetyhook::InlineHook* g_claim_hook = nullptr;
using SetMapIsCompleteFn = void(__thiscall*)(void* profile);
using HunterEndFn = void(__thiscall*)(void* hunter, int arg);
SetMapIsCompleteFn g_set_map_complete = nullptr;
HunterEndFn g_hunter_end = nullptr;
bool g_in_apply = false;
bool g_pending_end = false;
net::GameEvent g_pending_ev;
uint32_t g_check_swallowed = 0;
uint16_t g_seq = 0;
// Map flow (kMapStart). g_map is this process's map. The host announces each start once it is hosting.
std::string g_map;
uint16_t g_map_serial = 0;
bool g_map_committed = false;
std::string g_host_map;         // client: last map the host announced
bool g_client_seen_map = false;  // client: first announcement after joining only loads on a different map
std::vector<net::GameEvent> g_journal;
std::map<uint16_t, Handlers> g_handlers;
std::map<uint16_t, bool> g_unknown_logged;

void* Mission() {
    if (!g_game) return nullptr;
    void* m = nullptr;
    core::safe_read(reinterpret_cast<char*>(g_game) + kRvaMission, m);
    return m;
}

void* Rva(uintptr_t rva) { return g_game ? reinterpret_cast<char*>(g_game) + rva : nullptr; }

// The RVAs are for one retail build. Refuse to hook anything that does not look like the expected prologue.
void* CheckedRva(uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    void* p = Rva(rva);
    uint8_t got[5] = {};
    if (!p || !core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("events: {} at GameDll7+{:#x} has unexpected bytes; not hooked (wrong game build?)", label, rva);
        return nullptr;
    }
    return p;
}

bool guarded_message(void* mission, char code, const char* text) {
    __try {
        g_msg_hook->unsafe_thiscall<void>(mission, code, text);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_success_side_effects(void* mission) {
    __try {
        *reinterpret_cast<int*>(static_cast<char*>(mission) + kMissionStatus) = 2;
        void* hunter = *reinterpret_cast<void**>(static_cast<char*>(mission) + kMissionHunter);
        if (hunter) {
            void* profile = *reinterpret_cast<void**>(static_cast<char*>(hunter) + kHunterProfile);
            if (profile && g_set_map_complete) g_set_map_complete(profile);
            if (g_hunter_end) g_hunter_end(hunter, 0);
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_claim(void* animal) {
    __try {
        g_claim_hook->unsafe_ccall<void>(animal);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int FindJournal(uint16_t kind, uint16_t subject, const std::string& key) {
    for (size_t i = 0; i < g_journal.size(); ++i) {
        const net::GameEvent& ev = g_journal[i];
        if (ev.kind == kind && ev.subjectId == subject && ev.key == key) return static_cast<int>(i);
    }
    return -1;
}

void NoteUnknown(uint16_t kind, const char* dir) {
    if (g_unknown_logged[kind]) return;
    g_unknown_logged[kind] = true;
    spdlog::warn("events: no {} handler for kind {} (ignored)", dir, kind);
}

// ---- kMissionEnd ----

// Client: replay what the host's mission did. Success mirrors cMission::CheckMissionStatus's success branch
// (status 2, SetMapIsComplete on this player's profile, the hunter call), then the message that ends the map.
bool ApplyMissionEnd(const net::GameEvent& ev) {
    void* mission = Mission();
    if (!mission || !g_msg_hook) return false;
    const char code = static_cast<char>(ev.i0);
    g_in_apply = true;
    bool ok = true;
    if (code == kMsgSuccess) ok = guarded_success_side_effects(mission);
    else if (code == kMsgFailed) *reinterpret_cast<int*>(static_cast<char*>(mission) + kMissionStatus) = 3;
    ok = guarded_message(mission, code, ev.key.c_str()) && ok;
    g_in_apply = false;
    spdlog::info("events: applied host mission end code={} text='{}' ok={}", static_cast<int>(code), ev.key, ok);
    return true;
}

void OnMissionEndCommit(const net::GameEvent& ev) {
    if (ApplyMissionEnd(ev)) return;
    g_pending_end = true;
    g_pending_ev = ev;
    spdlog::warn("events: mission end code={} waiting for cMission", ev.i0);
}

// ---- kTrophyClaim ----

void OnClaimRequest(const net::GameEvent& ev) {
    void* animal = animals::Find(ev.subjectId);
    if (!animal) {
        spdlog::warn("events: claim request id={} from player {}: no animal", ev.subjectId, ev.actorId);
        return;
    }
    int claimable = 0;
    float hp = 1.f;
    core::safe_read(static_cast<char*>(animal) + kAnimalClaimable, claimable);
    core::safe_read(static_cast<char*>(animal) + kAnimalHealth, hp);
    if (FindJournal(kTrophyClaim, ev.subjectId, "") >= 0) {
        spdlog::info("events: claim request id={} from player {}: already claimed", ev.subjectId, ev.actorId);
        Commit(kTrophyClaim, ev.subjectId, 0, 0.f, "", ev.actorId);
        return;
    }
    if (claimable == 0 || hp > 0.f) {
        spdlog::warn("events: claim request id={} from player {} rejected (claimable={} hp={:.1f})", ev.subjectId,
                     ev.actorId, claimable, hp);
        return;
    }
    g_in_apply = true;
    const bool ok = guarded_claim(animal);
    g_in_apply = false;
    spdlog::info("events: claim id={} by player {} ran on host ok={}", ev.subjectId, ev.actorId, ok);
    if (ok) Commit(kTrophyClaim, ev.subjectId, 0, 0.f, "", ev.actorId);
}

void OnClaimCommit(const net::GameEvent& ev) {
    // The host ran the claim script. Here only the prompt goes away; the client's own script must not end the map.
    if (void* animal = animals::Find(ev.subjectId)) {
        *reinterpret_cast<int*>(static_cast<char*>(animal) + kAnimalClaimable) = 0;
    }
    spdlog::info("events: trophy id={} claimed by player {}", ev.subjectId, ev.actorId);
}

// ---- kMapStart ----

std::string MapNameFromLabel(const char* label) {
    std::string s = label ? label : "";
    const size_t slash = s.find_last_of("\\/");
    if (slash != std::string::npos) s.erase(0, slash + 1);
    const size_t dot = s.rfind('.');
    if (dot != std::string::npos) s.resize(dot);
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool IsMenuMap(const std::string& name) { return name == "MENUMAP"; }

void OnMapStartCommit(const net::GameEvent& ev) {
    const std::string want = MapNameFromLabel(ev.key.c_str());
    const bool first = !g_client_seen_map;
    g_client_seen_map = true;
    g_host_map = want;
    if (want.empty()) return;
    if (first && want == g_map) {
        spdlog::info("events: host map '{}' (serial {}) is already loaded here", want, ev.subjectId);
        return;
    }
    // spmap also closes the main and in-game menus; menumap is how the game itself enters the menu map.
    const std::string cmd = (IsMenuMap(want) ? "menumap " : "spmap ") + want;
    const int r = console::Exec(cmd.c_str());
    spdlog::info("events: following host to '{}' (serial {}, here '{}'): {} -> {}", want, ev.subjectId, g_map, cmd, r);
}

// ---- kAnimalDied ----

void OnAnimalDiedCommit(const net::GameEvent& ev) { animals::OnAnimalDied(ev.subjectId); }

void OnAnimalDiedRequest(const net::GameEvent& ev) {
    static bool logged = false;
    if (logged) return;
    logged = true;
    spdlog::info("events: AnimalDied is host-simulated; ignored request subject={} actor={}", ev.subjectId,
                 ev.actorId);
}

// Per map: the journal only holds outcomes of the current map.
void ResetMap() {
    g_journal.clear();
    g_seq = 0;
    g_pending_end = false;
    g_check_swallowed = 0;
}

// ---- hooks ----

void __fastcall hk_MissionMessage(void* self, void* /*edx*/, char code, const char* text) {
    const std::string t = core::safe_cstr(text, 64);
    if (g_in_apply || !NetIsOnline()) {
        g_msg_hook->unsafe_thiscall<void>(self, code, text);
        return;
    }
    const std::string caller = core::describe_address(_ReturnAddress());
    if (NetIsHost()) {
        spdlog::info("events: host mission message code={} text='{}' caller={}", static_cast<int>(code), t, caller);
        g_msg_hook->unsafe_thiscall<void>(self, code, text);
        Commit(kMissionEnd, 0, static_cast<uint8_t>(code), 0.f, t, 0);
        return;
    }
    spdlog::info("events: client mission message swallowed code={} text='{}' caller={}", static_cast<int>(code), t,
                 caller);
}

void __fastcall hk_CheckMission(void* self, void* /*edx*/) {
    if (g_in_apply || !NetIsOnline() || NetIsHost()) {
        g_check_hook->unsafe_thiscall<void>(self);
        return;
    }
    // Client: the host's mission decides. Scripts call this every frame once a hunt can end.
    if ((g_check_swallowed++ % 300) == 0) {
        spdlog::info("events: client CheckMissionStatus skipped (x{}) caller={}", g_check_swallowed,
                     core::describe_address(_ReturnAddress()));
    }
}

void __cdecl hk_ClaimCallback(void* animal) {
    if (g_in_apply || !NetIsOnline()) {
        g_claim_hook->unsafe_ccall<void>(animal);
        return;
    }
    const int id = animals::IdOf(animal);
    if (NetIsHost()) {
        g_claim_hook->unsafe_ccall<void>(animal);
        spdlog::info("events: host claimed trophy id={}", id);
        if (id >= 0) Commit(kTrophyClaim, static_cast<uint16_t>(id), 0, 0.f, "", 0);
        return;
    }
    if (animal) *reinterpret_cast<int*>(static_cast<char*>(animal) + kAnimalClaimable) = 0;
    spdlog::info("events: client claim id={} sent to host", id);
    if (id >= 0) Request(kTrophyClaim, static_cast<uint16_t>(id), 0, 0.f, "");
}

class EventsModule : public dh2::Module {
public:
    const char* Name() const override { return "events"; }
    int Priority() const override { return 41; }

    void OnInit() override {
        SetHandler(kAnimalDied, &OnAnimalDiedRequest, &OnAnimalDiedCommit);
        SetHandler(kMissionEnd, nullptr, &OnMissionEndCommit);
        SetHandler(kTrophyClaim, &OnClaimRequest, &OnClaimCommit);
        SetHandler(kMapStart, nullptr, &OnMapStartCommit);
        spdlog::info("events: registry AnimalDied={} MissionEnd={} TrophyClaim={} MapStart={}", kAnimalDied,
                     kMissionEnd, kTrophyClaim, kMapStart);
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = game;
        g_set_map_complete =
            reinterpret_cast<SetMapIsCompleteFn>(core::find_export(game, "?SetMapIsComplete@cHunterProfile@@QAEXXZ"));
        g_hunter_end = reinterpret_cast<HunterEndFn>(CheckedRva(kRvaHunterMissionEnd, "hunter mission end"));
        g_msg_hook = core::install_hook("cMission message (0x91c20)", CheckedRva(kRvaMissionMessage, "mission message"),
                                        reinterpret_cast<void*>(&hk_MissionMessage));
        g_check_hook = core::install_hook("cMission::CheckMissionStatus (0x94e50)",
                                          CheckedRva(kRvaCheckMission, "check mission"),
                                          reinterpret_cast<void*>(&hk_CheckMission));
        g_claim_hook = core::install_hook("cObjAnimal::ClaimCallback (0x1cd40)",
                                          CheckedRva(kRvaClaimCallback, "claim callback"),
                                          reinterpret_cast<void*>(&hk_ClaimCallback));
    }

    void OnMapLoaded(const char* bspName) override {
        ResetMap();
        g_map = MapNameFromLabel(bspName);
        ++g_map_serial;
        g_map_committed = false;
        spdlog::info("events: journal cleared for map '{}' (serial {})", g_map, g_map_serial);
    }
};

DH2_MODULE(EventsModule)

}  // namespace

void SetHandler(uint16_t kind, Handler onRequest, Handler onCommit) {
    Handlers& row = g_handlers[kind];
    if (row.request || row.commit) {
        spdlog::info("events: replacing handler for kind {}", kind);
    }
    row.request = onRequest;
    row.commit = onCommit;
}

void Commit(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key, uint8_t actor) {
    if (!NetIsHost()) return;
    const std::string trimmed(key.substr(0, net::kMaxGameEventKeyChars));
    const int existing = FindJournal(kind, subject, trimmed);
    if (existing >= 0) {
        SendGameEvent(g_journal[static_cast<size_t>(existing)]);
        return;
    }
    if (g_journal.size() >= kJournalCap) {
        spdlog::warn("events: journal full, dropped kind={} subject={}", kind, subject);
        return;
    }
    net::GameEvent ev;
    ev.flags = net::game_event_flag::kCommit;
    ev.actorId = actor;
    ev.seq = ++g_seq;
    ev.kind = kind;
    ev.subjectId = subject;
    ev.i0 = i0;
    ev.f0 = f0;
    ev.key = trimmed;
    g_journal.push_back(ev);
    const bool sent = SendGameEvent(ev);
    spdlog::info("events: commit kind={} subject={} actor={} i0={} key='{}' seq={} sent={} journal={}", kind, subject,
                 actor, i0, trimmed, ev.seq, sent, g_journal.size());
}

void Request(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key) {
    if (NetIsHost() || !NetIsOnline()) return;
    net::GameEvent ev;
    ev.kind = kind;
    ev.subjectId = subject;
    ev.i0 = i0;
    ev.f0 = f0;
    ev.key.assign(key.data(), key.size() > net::kMaxGameEventKeyChars ? net::kMaxGameEventKeyChars : key.size());
    const bool sent = SendGameEvent(ev);
    spdlog::info("events: request kind={} subject={} key='{}' sent={}", kind, subject, ev.key, sent);
}

void OnNet(const net::GameEvent& ev) {
    if (ev.commit()) {
        if (NetIsHost()) return;
        const auto it = g_handlers.find(ev.kind);
        if (it == g_handlers.end() || !it->second.commit) {
            NoteUnknown(ev.kind, "commit");
            return;
        }
        spdlog::info("events: apply commit kind={} subject={} actor={} i0={} key='{}'", ev.kind, ev.subjectId,
                     ev.actorId, ev.i0, ev.key);
        it->second.commit(ev);
        return;
    }
    if (!NetIsHost()) return;
    const auto it = g_handlers.find(ev.kind);
    if (it == g_handlers.end() || !it->second.request) {
        NoteUnknown(ev.kind, "request");
        return;
    }
    it->second.request(ev);
}

void Pump() {
    if (g_pending_end && !NetIsHost() && NetIsOnline() && ApplyMissionEnd(g_pending_ev)) g_pending_end = false;
    // The first map's load happens before hosting starts (auto-host runs on the next frame), so announce here.
    if (NetIsHost() && !g_map_committed && !g_map.empty()) {
        g_map_committed = true;
        Commit(kMapStart, g_map_serial, IsMenuMap(g_map) ? 1u : 0u, 0.f, g_map, 0);
    }
}

void Reset() {
    ResetMap();
    g_map_committed = false;
    g_host_map.clear();
    g_client_seen_map = false;
}

const std::string& CurrentMap() { return g_map; }

bool SameMapAsHost() {
    if (NetIsHost() || !NetIsOnline() || g_host_map.empty()) return true;
    return g_host_map == g_map;
}

void Replay() {
    if (!NetIsHost() || g_journal.empty()) return;
    for (const net::GameEvent& ev : g_journal) SendGameEvent(ev);
    spdlog::info("events: replayed {} commits", g_journal.size());
}

void DebugClaim(uint16_t animalId) {
    void* animal = animals::Find(animalId);
    spdlog::info("events: coop_claim id={} animal={:#x}", animalId, reinterpret_cast<uintptr_t>(animal));
    if (animal && g_claim_hook) hk_ClaimCallback(animal);
}

}  // namespace dh2::coop::events
