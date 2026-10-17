#include "core/module.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

namespace dh2 {
namespace {

struct Entry {
    Module* (*create)();
};

struct Inst {
    Module* mod = nullptr;
    bool disabled = false;
};

std::mutex& mu() {
    static std::mutex m;
    return m;
}

std::vector<Entry>& registry() {
    static std::vector<Entry> r;
    return r;
}

std::vector<Inst>& instances() {
    static std::vector<Inst> v;
    return v;
}

template <class Fn>
void each(const char* hook, Fn fn) {
    std::lock_guard lock(mu());
    for (Inst& inst : instances()) {
        if (inst.disabled || !inst.mod) continue;
        try {
            fn(*inst.mod);
        } catch (const std::exception& ex) {
            spdlog::error("module '{}' threw in {}: {}; disabling", inst.mod->Name(), hook, ex.what());
            inst.disabled = true;
        } catch (...) {
            spdlog::error("module '{}' threw in {}: unknown exception; disabling", inst.mod->Name(), hook);
            inst.disabled = true;
        }
    }
}

}  // namespace

namespace detail {
void register_module(Module* (*create)()) {
    if (!create) return;
    registry().push_back(Entry{create});
}
}  // namespace detail

namespace modules {

void Start() {
    std::lock_guard lock(mu());
    auto& inst = instances();
    if (!inst.empty()) return;
    for (const Entry& e : registry()) {
        try {
            Module* m = e.create();
            if (!m || !m->Name()) {
                spdlog::error("module factory returned an empty module");
                delete m;
                continue;
            }
            inst.push_back(Inst{m, false});
        } catch (const std::exception& ex) {
            spdlog::error("module factory threw: {}", ex.what());
        } catch (...) {
            spdlog::error("module factory threw an unknown exception");
        }
    }
    std::sort(inst.begin(), inst.end(), [](const Inst& a, const Inst& b) {
        if (a.mod->Priority() != b.mod->Priority()) return a.mod->Priority() < b.mod->Priority();
        return std::strcmp(a.mod->Name(), b.mod->Name()) < 0;
    });
    std::string list;
    for (const Inst& i : inst) {
        if (!list.empty()) list += ", ";
        list += fmt::format("{}({})", i.mod->Name(), i.mod->Priority());
    }
    spdlog::info("modules: {} registered [{}]", inst.size(), list);
}

void DispatchInit() { each("OnInit", [](Module& m) { m.OnInit(); }); }
void DispatchEngineReady(HMODULE engine) {
    each("OnEngineReady", [engine](Module& m) { m.OnEngineReady(engine); });
}
void DispatchGameDll(HMODULE game) {
    each("OnGameDllLoaded", [game](Module& m) { m.OnGameDllLoaded(game); });
}
void DispatchMapLoaded(const char* bspName) {
    each("OnMapLoaded", [bspName](Module& m) { m.OnMapLoaded(bspName); });
}
void DispatchMapUnloading() { each("OnMapUnloading", [](Module& m) { m.OnMapUnloading(); }); }
void DispatchFrame(float dtSeconds) {
    each("OnFrame", [dtSeconds](Module& m) { m.OnFrame(dtSeconds); });
}
void DispatchShutdown() { each("OnShutdown", [](Module& m) { m.OnShutdown(); }); }

}  // namespace modules
}  // namespace dh2
