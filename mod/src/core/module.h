#pragma once
// Static-registration module list. Dispatch order is Priority() ascending, then Name().
// A module that throws is logged and skipped from then on. Modules do not edit dllmain.cpp.
#include <windows.h>

namespace dh2 {

class Module {
public:
    virtual ~Module() = default;
    virtual const char* Name() const = 0;
    virtual int Priority() const { return 100; }

    virtual void OnInit() {}
    virtual void OnEngineReady(HMODULE engine) { (void)engine; }
    virtual void OnGameDllLoaded(HMODULE game) { (void)game; }
    virtual void OnMapLoaded(const char* bspName) { (void)bspName; }
    virtual void OnFrame(float dtSeconds) { (void)dtSeconds; }
    virtual void OnShutdown() {}
};

namespace detail {
void register_module(Module* (*create)());
}

// Registers ClassName during static initialization. ClassName must derive from dh2::Module.
#define DH2_MODULE(ClassName)                                                          \
    static ::dh2::Module* dh2_factory_##ClassName() { return new ClassName(); }        \
    namespace {                                                                        \
    struct dh2_registrar_##ClassName {                                                 \
        dh2_registrar_##ClassName() { ::dh2::detail::register_module(&dh2_factory_##ClassName); } \
    } dh2_registrar_inst_##ClassName;                                                  \
    }

namespace modules {
void Start();  // construct and sort; call once from the ASI init thread
void DispatchInit();
void DispatchEngineReady(HMODULE engine);
void DispatchGameDll(HMODULE game);
void DispatchMapLoaded(const char* bspName);
void DispatchFrame(float dtSeconds);
void DispatchShutdown();
}  // namespace modules

}  // namespace dh2
