// Story mode, step S1: observe the decision points of a story map (re/notes/lead-story-coop.md).
// Logs L1 trigger-volume activations (cObjTrigger::OnActivate), L2 script trigger events (GameDll 0x68eb0)
// and mission task completion (cMission::CompleteTask 0x95840) on every process. No behavior change yet.
#include <windows.h>
#include <intrin.h>

#include <cstdint>
#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"

namespace dh2::coop::story {
namespace {

constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntTargetName = 0x198;   // cObjBaseEntity target name (char*), as FindEntScrObject compares
constexpr uintptr_t kRvaTriggerEvent = 0x68eb0;  // GameDll7: fastcall(cObjTriggerEvent*) activation
constexpr uintptr_t kRvaCompleteTask = 0x95840;  // GameDll7: thiscall(cMission*, int task)

safetyhook::InlineHook* g_volume_hook = nullptr;
safetyhook::InlineHook* g_event_hook = nullptr;
safetyhook::InlineHook* g_task_hook = nullptr;
bool g_log = true;

std::string Describe(void* ent) {
    if (!ent) return "null";
    int16_t id = -1;
    char* name = nullptr;
    core::safe_read(static_cast<char*>(ent) + kEntId, id);
    core::safe_read(static_cast<char*>(ent) + kEntTargetName, name);
    void* vt = nullptr;
    core::safe_read(ent, vt);
    return fmt::format("id={} name='{}' vt={}", id, name ? core::safe_cstr(name, 48) : std::string(),
                       core::describe_address(vt));
}

const char* Role() { return NetIsHost() ? "host" : (NetIsOnline() ? "client" : "solo"); }

void __fastcall hk_VolumeActivate(void* self, void*, void* activator) {
    if (g_log) {
        spdlog::info("story: [{}] volume activate {} by {} caller={}", Role(), Describe(self), Describe(activator),
                     core::describe_address(_ReturnAddress()));
    }
    g_volume_hook->unsafe_thiscall<void>(self, activator);
}

void __fastcall hk_TriggerEvent(void* self, void*) {
    int active = -1;
    core::safe_read(static_cast<char*>(self) + 0x1e4, active);
    if (g_log && active != 1) {
        spdlog::info("story: [{}] trigger event activate {} caller={}", Role(), Describe(self),
                     core::describe_address(_ReturnAddress()));
    }
    g_event_hook->unsafe_fastcall<void>(self);
}

void __fastcall hk_CompleteTask(void* self, void*, int task) {
    if (g_log) {
        spdlog::info("story: [{}] CompleteTask {} caller={}", Role(), task, core::describe_address(_ReturnAddress()));
    }
    g_task_hook->unsafe_thiscall<void>(self, task);
}

void* CheckedGameRva(HMODULE game, uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    uint8_t got[5] = {};
    void* p = reinterpret_cast<char*>(game) + rva;
    if (!core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("story: {} at GameDll7+{:#x} has unexpected bytes; not hooked", label, rva);
        return nullptr;
    }
    return p;
}

class StoryModule : public dh2::Module {
public:
    const char* Name() const override { return "story"; }
    int Priority() const override { return 47; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_log = GetPrivateProfileIntA("Story", "LogTriggers", 1, ini.c_str()) != 0;
    }

    void OnEngineReady(HMODULE) override {
        if (HMODULE common = core::wait_for_module("CommonDll7.dll", 10000)) {
            g_volume_hook = core::install_hook(
                "cObjTrigger::OnActivate (story log)",
                core::find_export(common, "?OnActivate@cObjTrigger@@UAEXPAVcObjScriptedEntity@@@Z"),
                reinterpret_cast<void*>(&hk_VolumeActivate));
        }
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_event_hook = core::install_hook("cObjTriggerEvent activate (0x68eb0)",
                                          CheckedGameRva(game, kRvaTriggerEvent, "trigger event"),
                                          reinterpret_cast<void*>(&hk_TriggerEvent));
        g_task_hook = core::install_hook("cMission::CompleteTask (0x95840)",
                                         CheckedGameRva(game, kRvaCompleteTask, "complete task"),
                                         reinterpret_cast<void*>(&hk_CompleteTask));
    }
};

DH2_MODULE(StoryModule)

}  // namespace
}  // namespace dh2::coop::story
