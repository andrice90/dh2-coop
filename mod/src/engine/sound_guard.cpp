// Sound driver diagnostics for the same-map replay crash (re/notes/replay-crash.md).
// EngineDll7 cSndDrvDS (vtable RVA 0x445160) keeps
//   waves  at driver +0x14 (data) / +0x18 (count): cWave { +4 bytes, +0xc name, +0x10 refcount, +0x14 length }
//   sounds at driver +0x0c (data) / +0x10 count:   record { +0 wave index, +0x14 channel, ... } (0x74 bytes)
// WaveRelease (0x3882a0) drops one reference and frees the wave at 0. A release on an empty slot is the crash.
// [Sound] Trace=1 logs every wave free and new load plus anomalies; Trace=2 also logs each reference change.
// The replay crash was such a wave: freed by EndPrecache while the respawned hunter used it. The fix is in
// core/lifecycle.cpp ([Coop] FullReloadOnReplay); this file only reports.
#include <windows.h>

#include <cstdint>
#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"

namespace dh2::sound {
namespace {

int g_trace = 0;
HMODULE g_engine = nullptr;
int g_in_bulk = 0;          // inside Begin/EndPrecache or ReleaseAllSounds
const char* g_bulk_name = "";
int g_releasing_sound = -1;  // driver sound being released (its own record still names the wave)
char g_trace_wave[64] = {};  // [Sound] TraceWave: name substring whose events also log a backtrace

safetyhook::InlineHook* g_wave_release = nullptr;
safetyhook::InlineHook* g_wave_load = nullptr;
safetyhook::InlineHook* g_begin = nullptr;
safetyhook::InlineHook* g_end = nullptr;
safetyhook::InlineHook* g_release_all = nullptr;
safetyhook::InlineHook* g_new_sound = nullptr;
safetyhook::InlineHook* g_release_sound = nullptr;
safetyhook::InlineHook* g_release_sample = nullptr;
safetyhook::InlineHook* g_proto_ref = nullptr;
safetyhook::InlineHook* g_mgr_release = nullptr;

struct Array {
    void** data;
    int count;
};

Array waves(void* drv) {
    Array a{};
    core::safe_read(static_cast<char*>(drv) + 0x14, a.data);
    core::safe_read(static_cast<char*>(drv) + 0x18, a.count);
    return a;
}

Array sounds(void* drv) {
    Array a{};
    core::safe_read(static_cast<char*>(drv) + 0x0c, a.data);
    core::safe_read(static_cast<char*>(drv) + 0x10, a.count);
    return a;
}

void* wave_at(void* drv, int idx) {
    const Array a = waves(drv);
    void* w = nullptr;
    if (a.data && idx >= 0 && idx < a.count) core::safe_read(a.data + idx, w);
    return w;
}

int wave_ref(void* wave) {
    int r = -999;
    if (wave) core::safe_read(static_cast<char*>(wave) + 0x10, r);
    return r;
}

std::string wave_name(void* wave) {
    if (!wave) return "<empty>";
    const char* n = nullptr;
    core::safe_read(static_cast<char*>(wave) + 0xc, n);
    return core::safe_cstr(n, 96);
}

// Driver sounds whose wave index is idx.
int users_of(void* drv, int idx) {
    const Array a = sounds(drv);
    int n = 0;
    for (int i = 0; a.data && i < a.count && i < 4096; ++i) {
        if (i == g_releasing_sound) continue;
        void* rec = nullptr;
        if (!core::safe_read(a.data + i, rec) || !rec) continue;
        int w = -1;
        if (core::safe_read(rec, w) && w == idx) ++n;
    }
    return n;
}

std::string backtrace(int skip);

std::string stack_if(const std::string& name) {
    if (!g_trace_wave[0] || name.find(g_trace_wave) == std::string::npos) return {};
    return " stack: " + backtrace(2);
}

std::string backtrace(int skip) {
    void* frames[14] = {};
    const USHORT n = RtlCaptureStackBackTrace(static_cast<DWORD>(skip), 14, frames, nullptr);
    std::string s;
    for (USHORT i = 0; i < n; ++i) {
        if (!s.empty()) s += " < ";
        s += core::describe_address(frames[i]);
    }
    return s;
}

void __fastcall hk_wave_release(void* drv, void*, int idx) {
    void* w = wave_at(drv, idx);
    if (!w) {
        spdlog::error("snd: WaveRelease({}) on an empty slot ({}) caller={} stack: {}", idx,
                      g_in_bulk ? g_bulk_name : "direct", core::describe_address(_ReturnAddress()), backtrace(1));
    } else if (g_trace) {
        const int ref = wave_ref(w);
        if (ref <= 1) {
            const int users = users_of(drv, idx);
            if (users > 0) {
                spdlog::error("snd: wave {} '{}' freed by {} while {} driver sound(s) still use it; caller={} stack: {}",
                              idx, wave_name(w), g_in_bulk ? g_bulk_name : "WaveRelease", users,
                              core::describe_address(_ReturnAddress()), backtrace(1));
            } else {
                spdlog::info("snd: wave {} '{}' freed ({}) caller={}", idx, wave_name(w), g_in_bulk ? g_bulk_name : "direct",
                             core::describe_address(_ReturnAddress()));
            }
        } else if (g_trace >= 2 && !g_in_bulk) {
            spdlog::info("snd: wave {} '{}' ref {}->{} caller={}", idx, wave_name(w), ref, ref - 1,
                         core::describe_address(_ReturnAddress()));
        }
    }
    g_wave_release->unsafe_thiscall<void>(drv, idx);
}

int __fastcall hk_wave_load(void* drv, void*, const char* name) {
    const int before = waves(drv).count;
    const int r = g_wave_load->unsafe_thiscall<int>(drv, name);
    if (g_trace) {
        void* w = r >= 0 ? wave_at(drv, r) : nullptr;
        const int ref = wave_ref(w);
        if (ref == 1 || g_trace >= 2) {
            spdlog::info("snd: wave {} '{}' {} ref={} (table {}->{}) caller={}", r, core::safe_cstr(name, 96),
                         ref == 1 ? "loaded" : "found", ref, before, waves(drv).count,
                         core::describe_address(_ReturnAddress()));
        }
    }
    return r;
}

int count_live(const Array& a) {
    int n = 0;
    for (int i = 0; a.data && i < a.count && i < 8192; ++i) {
        void* p = nullptr;
        if (core::safe_read(a.data + i, p) && p) ++n;
    }
    return n;
}

void bulk(safetyhook::InlineHook* h, void* drv, const char* label) {
    if (g_trace) {
        spdlog::info("snd: {} begin waves={} sounds={} caller={}", label, count_live(waves(drv)), count_live(sounds(drv)),
                     core::describe_address(_ReturnAddress()));
    }
    ++g_in_bulk;
    g_bulk_name = label;
    h->unsafe_thiscall<void>(drv);
    --g_in_bulk;
    if (g_trace) spdlog::info("snd: {} end waves={} sounds={}", label, count_live(waves(drv)), count_live(sounds(drv)));
}

void __fastcall hk_begin(void* drv, void*) { bulk(g_begin, drv, "BeginPrecache"); }
void __fastcall hk_end(void* drv, void*) { bulk(g_end, drv, "EndPrecache"); }
void __fastcall hk_release_all(void* drv, void*) { bulk(g_release_all, drv, "ReleaseAllSounds"); }

int __fastcall hk_new_sound(void* drv, void*, const char* name, int a, int b, int c) {
    const int r = g_new_sound->unsafe_thiscall<int>(drv, name, a, b, c);
    if (g_trace >= 2) {
        const std::string n = core::safe_cstr(name, 96);
        spdlog::info("snd: NewSound('{}') -> {} caller={}{}", n, r, core::describe_address(_ReturnAddress()), stack_if(n));
    }
    return r;
}

void __fastcall hk_release_sound(void* drv, void*, int sidx) {
    if (g_trace >= 2) {
        const Array a = sounds(drv);
        void* rec = nullptr;
        int w = -1;
        if (a.data && sidx >= 0 && sidx < a.count && core::safe_read(a.data + sidx, rec) && rec) core::safe_read(rec, w);
        spdlog::info("snd: ReleaseSound({}) wave={} '{}' caller={}", sidx, w, wave_name(wave_at(drv, w)),
                     core::describe_address(_ReturnAddress()));
    }
    const int prev = g_releasing_sound;
    g_releasing_sound = sidx;
    g_release_sound->unsafe_thiscall<void>(drv, sidx);
    g_releasing_sound = prev;
}

void __fastcall hk_release_sample(void* drv, void*, const char* name) {
    if (g_trace >= 2) {
        const std::string n = core::safe_cstr(name, 96);
        spdlog::info("snd: ReleaseSample('{}') caller={}{}", n, core::describe_address(_ReturnAddress()), stack_if(n));
    }
    g_release_sample->unsafe_thiscall<void>(drv, name);
}

float __fastcall hk_proto_ref(void* drv, void*, const char* name, int flag) {
    const float r = g_proto_ref->unsafe_thiscall<float>(drv, name, flag);
    if (g_trace >= 2) {
        const std::string n = core::safe_cstr(name, 96);
        spdlog::info("snd: SampleRef('{}', {}) -> {:.3f} caller={}{}", n, flag, r, core::describe_address(_ReturnAddress()),
                     stack_if(n));
    }
    return r;
}

// cSndManager::ReleaseSound(protoIndex) (EngineDll7 0x149fe0): protos at manager +0x64 (data) / +0x68 (count),
// proto +0x2c name. An index outside the table only logs "Invalid sound index for release" (SoundLoad.cpp 1166)
// and returns: every run has two of those at the first map load.
void __fastcall hk_mgr_release(void* mgr, void*, int idx) {
    int count = 0;
    core::safe_read(static_cast<char*>(mgr) + 0x68, count);
    if (idx < 0 || idx >= count) {
        spdlog::info("snd: cSndManager::ReleaseSound({}) outside the proto table ({}); engine no-op. caller={} stack: {}",
                     idx, count, core::describe_address(_ReturnAddress()), backtrace(1));
    } else if (g_trace >= 2) {
        void** data = nullptr;
        void* proto = nullptr;
        const char* name = nullptr;
        core::safe_read(static_cast<char*>(mgr) + 0x64, data);
        if (data && core::safe_read(data + idx, proto) && proto) core::safe_read(static_cast<char*>(proto) + 0x2c, name);
        const std::string n = core::safe_cstr(name, 96);
        spdlog::info("snd: proto release {} '{}' caller={}{}", idx, n, core::describe_address(_ReturnAddress()), stack_if(n));
    }
    g_mgr_release->unsafe_thiscall<void>(mgr, idx);
}

void* checked(uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    auto* target = reinterpret_cast<uint8_t*>(g_engine) + rva;
    uint8_t got[5] = {};
    if (!core::safe_copy(target, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("snd: unexpected bytes at EngineDll7+{:#x} ({}); not hooked", rva, label);
        return nullptr;
    }
    return target;
}

class SoundModule : public dh2::Module {
public:
    const char* Name() const override { return "sound"; }
    int Priority() const override { return 5; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_trace = GetPrivateProfileIntA("Sound", "Trace", 0, ini.c_str());
        GetPrivateProfileStringA("Sound", "TraceWave", "", g_trace_wave, sizeof g_trace_wave, ini.c_str());
    }

    void OnEngineReady(HMODULE engine) override {
        g_engine = engine;
        // The empty-slot check runs always (one log line before the engine faults); the rest only with Trace.
        g_wave_release = core::install_hook("cSndDrvDS::WaveRelease", checked(0x3882a0, "WaveRelease"),
                                            reinterpret_cast<void*>(&hk_wave_release));
        if (!g_trace) return;
        g_wave_load = core::install_hook("cSndDrvDS::LoadWave", checked(0x385790, "LoadWave"),
                                         reinterpret_cast<void*>(&hk_wave_load));
        g_begin = core::install_hook("cSndDrvDS::BeginPrecache", checked(0x385650, "BeginPrecache"),
                                     reinterpret_cast<void*>(&hk_begin));
        g_end = core::install_hook("cSndDrvDS::EndPrecache", checked(0x3887d0, "EndPrecache"),
                                   reinterpret_cast<void*>(&hk_end));
        g_release_all = core::install_hook("cSndDrvDS::ReleaseAllSounds", checked(0x3891a0, "ReleaseAllSounds"),
                                           reinterpret_cast<void*>(&hk_release_all));
        g_new_sound = core::install_hook("cSndDrvDS::NewSound", checked(0x388f70, "NewSound"),
                                         reinterpret_cast<void*>(&hk_new_sound));
        g_release_sound = core::install_hook("cSndDrvDS::ReleaseSound", checked(0x38a6a0, "ReleaseSound"),
                                             reinterpret_cast<void*>(&hk_release_sound));
        g_release_sample = core::install_hook("cSndDrvDS::ReleaseSample", checked(0x388560, "ReleaseSample"),
                                              reinterpret_cast<void*>(&hk_release_sample));
        g_proto_ref = core::install_hook("cSndDrvDS::SampleRef", checked(0x388430, "SampleRef"),
                                         reinterpret_cast<void*>(&hk_proto_ref));
        g_mgr_release = core::install_hook("cSndManager::ReleaseSound", checked(0x149fe0, "cSndManager::ReleaseSound"),
                                           reinterpret_cast<void*>(&hk_mgr_release));
        spdlog::info("snd: trace level {} wave '{}'", g_trace, g_trace_wave);
    }
};

DH2_MODULE(SoundModule)

}  // namespace
}  // namespace dh2::sound
