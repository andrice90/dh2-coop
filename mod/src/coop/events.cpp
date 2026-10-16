// Discrete world events. Design: re/notes/p4c-events.md.
// GameEvent is the only packet. Kinds are a registry so another map does not need a new message id.
#include "coop/events.h"

#include <windows.h>
#include <intrin.h>

#include <cstdint>
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

namespace dh2::coop::events {
namespace {

constexpr size_t kJournalCap = 64;

struct Handlers {
    Handler request = nullptr;
    Handler commit = nullptr;
};

safetyhook::InlineHook* g_map_hook = nullptr;
safetyhook::InlineHook* g_start_hook = nullptr;
void* g_profile = nullptr;
bool g_in_apply = false;
bool g_host_completed = false;
bool g_client_completed = false;
bool g_pending_host = false;
bool g_pending_client = false;
uint8_t g_pending_actor = 0;
uint16_t g_seq = 0;
std::vector<net::GameEvent> g_journal;
std::map<uint16_t, Handlers> g_handlers;
std::map<uint16_t, bool> g_unknown_logged;

bool call_complete(safetyhook::InlineHook* hook, void* self) {
    __try {
        hook->unsafe_thiscall<void>(self);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallOriginal(void* self) {
    if (!g_map_hook || !self) return false;
    g_in_apply = true;
    const bool ok = call_complete(g_map_hook, self);
    g_in_apply = false;
    if (!ok) spdlog::warn("events: SetMapIsComplete faulted");
    return ok;
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

void FinishHostComplete(uint8_t actor) {
    g_host_completed = true;
    g_pending_host = false;
    Commit(kMapComplete, 0, actor, 0.f, "", actor);
}

void OnMapRequest(const net::GameEvent& ev) {
    spdlog::info("events: map-complete request from player {}", ev.actorId);
    if (g_host_completed) {
        Commit(kMapComplete, 0, ev.actorId, 0.f, "", ev.actorId);
        return;
    }
    if (!CallOriginal(g_profile)) {
        g_pending_host = true;
        g_pending_actor = ev.actorId;
        spdlog::warn("events: map-complete waiting for cHunterProfile");
        return;
    }
    FinishHostComplete(ev.actorId);
}

void OnMapCommit(const net::GameEvent& ev) {
    if (g_client_completed) return;
    if (!CallOriginal(g_profile)) {
        g_pending_client = true;
        spdlog::warn("events: map-complete commit waiting for cHunterProfile (actor {})", ev.actorId);
        return;
    }
    g_client_completed = true;
    g_pending_client = false;
    spdlog::info("events: applied host map-complete (actor {})", ev.actorId);
}

void OnAnimalDiedCommit(const net::GameEvent& ev) { animals::OnAnimalDied(ev.subjectId); }

void OnAnimalDiedRequest(const net::GameEvent& ev) {
    static bool logged = false;
    if (logged) return;
    logged = true;
    spdlog::info("events: AnimalDied is host-simulated; ignored request subject={} actor={}", ev.subjectId,
                 ev.actorId);
}

// GetLastmap is `lea eax,[ecx+0x23D9]; ret` (7 bytes) and the probe never logged a call.
// GetMapToBeStarted is a normal function. The spmap probe called it and got a profile pointer.
char* __fastcall hk_GetMapToBeStarted(void* self, void* /*edx*/) {
    if (self && !g_profile) {
        g_profile = self;
        spdlog::info("events: profile captured this={:#x}", reinterpret_cast<uintptr_t>(self));
    }
    if (!g_start_hook) return nullptr;
    return g_start_hook->unsafe_thiscall<char*>(self);
}

void __fastcall hk_SetMapIsComplete(void* self, void* /*edx*/) {
    g_profile = self;
    static int seen = 0;
    if (seen < 8) {
        ++seen;
        spdlog::info("events: SetMapIsComplete enter this={:#x} online={} host={}", reinterpret_cast<uintptr_t>(self),
                     NetIsOnline() ? 1 : 0, NetIsHost() ? 1 : 0);
    }
    if (g_in_apply || !NetIsOnline()) {
        if (g_map_hook) g_map_hook->unsafe_thiscall<void>(self);
        return;
    }
    const std::string caller = core::describe_address(_ReturnAddress());
    if (NetIsHost()) {
        spdlog::info("events: host SetMapIsComplete caller={}", caller);
        if (!g_host_completed) {
            if (CallOriginal(self)) FinishHostComplete(0);
            else spdlog::warn("events: host SetMapIsComplete did not run");
        } else {
            Commit(kMapComplete, 0, 0, 0.f, "", 0);
        }
        return;
    }
    spdlog::info("events: client SetMapIsComplete swallowed caller={}", caller);
    Request(kMapComplete, 0, 0, 0.f, "");
}

class EventsModule : public dh2::Module {
public:
    const char* Name() const override { return "events"; }
    int Priority() const override { return 41; }

    void OnInit() override {
        SetHandler(kAnimalDied, &OnAnimalDiedRequest, &OnAnimalDiedCommit);
        SetHandler(kMapComplete, &OnMapRequest, &OnMapCommit);
        spdlog::info("events: registry AnimalDied={} MapComplete={}", kAnimalDied, kMapComplete);
    }

    void OnGameDllLoaded(HMODULE game) override {
        void* complete = core::find_export(game, "?SetMapIsComplete@cHunterProfile@@QAEXXZ");
        void* start = core::find_export(game, "?GetMapToBeStarted@cHunterProfile@@QAEPADXZ");
        g_map_hook = core::install_hook("cHunterProfile::SetMapIsComplete", complete,
                                        reinterpret_cast<void*>(&hk_SetMapIsComplete));
        g_start_hook = core::install_hook("cHunterProfile::GetMapToBeStarted", start,
                                          reinterpret_cast<void*>(&hk_GetMapToBeStarted));
    }

    void OnMapLoaded(const char* bspName) override {
        Reset();
        spdlog::info("events: journal cleared for map '{}'", bspName ? bspName : "");
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
    spdlog::info("events: commit kind={} subject={} actor={} key='{}' seq={} sent={} journal={}", kind, subject, actor,
                 trimmed, ev.seq, sent, g_journal.size());
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
        spdlog::info("events: apply commit kind={} subject={} actor={} key='{}'", ev.kind, ev.subjectId, ev.actorId,
                     ev.key);
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
    if (g_pending_host && NetIsHost() && CallOriginal(g_profile)) FinishHostComplete(g_pending_actor);
    if (g_pending_client && !NetIsHost() && NetIsOnline() && CallOriginal(g_profile)) {
        g_client_completed = true;
        g_pending_client = false;
        spdlog::info("events: applied queued host map-complete");
    }
}

void Reset() {
    g_journal.clear();
    g_seq = 0;
    g_host_completed = false;
    g_client_completed = false;
    g_pending_host = false;
    g_pending_client = false;
    g_pending_actor = 0;
}

void Replay() {
    if (!NetIsHost() || g_journal.empty()) return;
    for (const net::GameEvent& ev : g_journal) SendGameEvent(ev);
    spdlog::info("events: replayed {} commits", g_journal.size());
}

}  // namespace dh2::coop::events
