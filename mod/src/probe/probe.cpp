// Runtime probe (Track E). Pass-through logging only - nothing here changes game behaviour.
//
// What it records (all to dh2coop.log):
//   * module load order / DLL load events (d3d8.dll, d3d9.dll, EngineDll7/CommonDll7/GameDll7 bases)
//   * Direct3DCreate8/9 calls (caller), IDirect3D*::CreateDevice params, device Reset/Present/EndScene
//   * resolved export addresses and engine static state (cObjManager::m_nDriverType, cRenderDX::m_d3d[d] ...)
//   * cNetServer / cNetClient connect-like calls, map-load calls, console / command-line queries
#include "probe/probe.h"

#include <windows.h>

#include <d3d9.h>  // D3DPRESENT_PARAMETERS (D3D9 only; D3D8 headers are gone from the Windows SDK)
#include <intrin.h>
#include <tlhelp32.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <set>
#include <string>

#include <safetyhook.hpp>

#include "core/config.h"
#include "core/exports.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/d3d8_hooks.h"

namespace dh2::probe {

using safetyhook::InlineHook;
using dh2::core::describe_address;
using dh2::core::find_export;
using dh2::core::safe_cstr;
using dh2::core::safe_read;

namespace {

core::Config g_cfg;
bool g_active = false;
int g_state_samples = 0;
ULONGLONG g_next_sample = 0;
ULONGLONG g_last_change_log = 0;
std::string g_last_state;
bool g_d3d9_logged = false;

inline uintptr_t A(const void* p) { return reinterpret_cast<uintptr_t>(p); }

// ---------------------------------------------------------------------------------------------
// time
// ---------------------------------------------------------------------------------------------
ULONGLONG ft_to_u64(const FILETIME& f) { return (static_cast<ULONGLONG>(f.dwHighDateTime) << 32) | f.dwLowDateTime; }

// Milliseconds since the process was created.
long long uptime_ms() {
    static ULONGLONG created = [] {
        FILETIME c, e, k, u;
        GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u);
        return ft_to_u64(c);
    }();
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    return static_cast<long long>((ft_to_u64(now) - created) / 10000ULL);
}

// ---------------------------------------------------------------------------------------------
// hook plumbing
// ---------------------------------------------------------------------------------------------
// Hooks are intentionally leaked: never unhook at process exit (other threads / modules may be gone).
InlineHook* mk(const char* label, void* target, void* detour) {
    if (!target) {
        spdlog::warn("hook {}: target not resolved, skipped", label);
        return nullptr;
    }
    auto r = InlineHook::create(target, detour);
    if (!r) {
        spdlog::error("hook {}: create failed (safetyhook error type {}) target={}", label,
                      static_cast<int>(r.error().type), describe_address(target));
        return nullptr;
    }
    auto* h = new InlineHook(std::move(*r));
    spdlog::info("hook {}: installed at {} ({:#x}), trampoline {:#x}", label, describe_address(target), A(target),
                 A(h->trampoline().data()));
    return h;
}

// Logs the first `limit` calls, then every call whose counter is a power of two.
bool should_log(std::atomic<uint32_t>& counter, uint32_t limit) {
    const uint32_t n = ++counter;
    return n <= limit || (n & (n - 1)) == 0;
}

// ---------------------------------------------------------------------------------------------
// D3D
// ---------------------------------------------------------------------------------------------
// D3D8 D3DPRESENT_PARAMETERS (13 fields). d3d8.h is not in the Windows 10/11 SDK, so declare it here.
struct D3D8Params {
    UINT BackBufferWidth, BackBufferHeight;
    UINT BackBufferFormat;
    UINT BackBufferCount;
    UINT MultiSampleType;
    UINT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    UINT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT FullScreen_PresentationInterval;
};
static_assert(sizeof(D3D8Params) == 52);

std::string fmt_params(int api, const void* pp) {
    if (!pp) return "<null>";
    if (api == 0) {
        D3D8Params p{};
        if (!safe_read(pp, p)) return "<unreadable>";
        return fmt::format(
            "{}x{} fmt={} bb={} ms={} swap={} hwnd={:#x} windowed={} autoDS={} dsfmt={} flags={:#x} hz={} pi={:#x}",
            p.BackBufferWidth, p.BackBufferHeight, p.BackBufferFormat, p.BackBufferCount, p.MultiSampleType,
            p.SwapEffect, A(p.hDeviceWindow), p.Windowed, p.EnableAutoDepthStencil, p.AutoDepthStencilFormat,
            p.Flags, p.FullScreen_RefreshRateInHz, p.FullScreen_PresentationInterval);
    }
    D3DPRESENT_PARAMETERS p{};
    if (!safe_read(pp, p)) return "<unreadable>";
    return fmt::format(
        "{}x{} fmt={} bb={} ms={} msq={} swap={} hwnd={:#x} windowed={} autoDS={} dsfmt={} flags={:#x} hz={} pi={:#x}",
        p.BackBufferWidth, p.BackBufferHeight, static_cast<int>(p.BackBufferFormat), p.BackBufferCount,
        static_cast<int>(p.MultiSampleType), p.MultiSampleQuality, static_cast<int>(p.SwapEffect),
        A(p.hDeviceWindow), p.Windowed, p.EnableAutoDepthStencil, static_cast<int>(p.AutoDepthStencilFormat),
        p.Flags, p.FullScreen_RefreshRateInHz, p.PresentationInterval);
}

struct DevApi {
    const char* name;
    int vt_createdevice;  // IDirect3D{8,9}::CreateDevice
    int vt_reset, vt_present, vt_endscene;  // IDirect3DDevice{8,9}
    InlineHook* createdevice = nullptr;
    InlineHook* reset = nullptr;
    InlineHook* present = nullptr;
    InlineHook* endscene = nullptr;
    std::atomic<uint32_t> n_create{0};   // Direct3DCreateN calls
    std::atomic<uint32_t> n_cdev{0};     // CreateDevice calls
    std::atomic<uint32_t> n_reset{0};
    std::atomic<uint64_t> n_present{0};
    std::atomic<uint64_t> n_endscene{0};
    std::atomic<void*> device{nullptr};
    std::atomic<bool> d3d_hooked{false};
    std::atomic<bool> dev_hooked{false};
    InlineHook* create = nullptr;        // Direct3DCreateN export in d3dN.dll
};

DevApi g_api[2] = {
    {"D3D8", /*CreateDevice*/ 15, /*Reset*/ 14, /*Present*/ 15, /*EndScene*/ 35},
    {"D3D9", /*CreateDevice*/ 16, /*Reset*/ 16, /*Present*/ 17, /*EndScene*/ 42},
};

template <int I>
HRESULT __stdcall hk_Reset(void* self, void* pp) {
    DevApi& a = g_api[I];
    const uint32_t n = ++a.n_reset;
    spdlog::info("{} Device::Reset #{} dev={:#x} params: {}", a.name, n, A(self), fmt_params(I, pp));
    return a.reset->unsafe_stdcall<HRESULT>(self, pp);
}

template <int I>
HRESULT __stdcall hk_Present(void* self, const RECT* src, const RECT* dst, HWND wnd, const void* dirty) {
    DevApi& a = g_api[I];
    const uint64_t n = ++a.n_present;
    if (n <= 3) {
        spdlog::info("{} Device::Present #{} dev={:#x} hwnd={:#x} caller={} (+{}ms)", a.name, n, A(self), A(wnd),
                     describe_address(_ReturnAddress()), uptime_ms());
    }
    return a.present->unsafe_stdcall<HRESULT>(self, src, dst, wnd, dirty);
}

template <int I>
HRESULT __stdcall hk_EndScene(void* self) {
    DevApi& a = g_api[I];
    const uint64_t n = ++a.n_endscene;
    if (n <= 3) {
        spdlog::info("{} Device::EndScene #{} dev={:#x} caller={}", a.name, n, A(self),
                     describe_address(_ReturnAddress()));
    }
    return a.endscene->unsafe_stdcall<HRESULT>(self);
}

void hook_device(int api, void* dev, const char* how) {
    if (api != 1) return;  // D3D8 Present/Reset live in dh2::d3d8
    DevApi& a = g_api[api];
    bool expected = false;
    if (!a.dev_hooked.compare_exchange_strong(expected, true)) return;
    a.device = dev;
    void** vt = *reinterpret_cast<void***>(dev);
    spdlog::info("{} device {:#x} vtable {:#x} ({}) [{}]: Reset={} Present={} EndScene={}", a.name, A(dev), A(vt),
                 describe_address(vt), how, describe_address(vt[a.vt_reset]), describe_address(vt[a.vt_present]),
                 describe_address(vt[a.vt_endscene]));
    a.reset = mk("D3D9 Device::Reset", vt[a.vt_reset], reinterpret_cast<void*>(&hk_Reset<1>));
    a.present = mk("D3D9 Device::Present", vt[a.vt_present], reinterpret_cast<void*>(&hk_Present<1>));
    a.endscene = mk("D3D9 Device::EndScene", vt[a.vt_endscene], reinterpret_cast<void*>(&hk_EndScene<1>));
}

template <int I>
HRESULT __stdcall hk_CreateDevice(void* self, UINT adapter, UINT type, HWND focus, DWORD flags, void* pp,
                                  void** out) {
    DevApi& a = g_api[I];
    const uint32_t n = ++a.n_cdev;
    spdlog::info("{} IDirect3D::CreateDevice #{} (+{}ms) this={:#x} adapter={} devtype={} focus={:#x} behavior={:#x} "
                 "caller={}",
                 a.name, n, uptime_ms(), A(self), adapter, type, A(focus), flags, describe_address(_ReturnAddress()));
    spdlog::info("{}   params in : {}", a.name, fmt_params(I, pp));
    HRESULT hr = a.createdevice->unsafe_stdcall<HRESULT>(self, adapter, type, focus, flags, pp, out);
    spdlog::info("{}   -> hr={:#010x} device={:#x}", a.name, static_cast<uint32_t>(hr), out ? A(*out) : 0);
    spdlog::info("{}   params out: {}", a.name, fmt_params(I, pp));
    if (SUCCEEDED(hr) && out && *out) hook_device(I, *out, "CreateDevice");
    return hr;
}

void hook_d3d_object(int api, void* obj) {
    if (api != 1) return;  // D3D8 CreateDevice lives in dh2::d3d8
    DevApi& a = g_api[api];
    bool expected = false;
    if (!a.d3d_hooked.compare_exchange_strong(expected, true)) return;
    void** vt = *reinterpret_cast<void***>(obj);
    spdlog::info("{} IDirect3D object {:#x} vtable {:#x} ({}) CreateDevice={}", a.name, A(obj), A(vt),
                 describe_address(vt), describe_address(vt[a.vt_createdevice]));
    a.createdevice = mk("IDirect3D9::CreateDevice", vt[a.vt_createdevice], reinterpret_cast<void*>(&hk_CreateDevice<1>));
}

template <int I>
void* __stdcall hk_Direct3DCreate(UINT sdk) {
    DevApi& a = g_api[I];
    void* ra = _ReturnAddress();
    const uint32_t n = ++a.n_create;
    void* obj = a.create->unsafe_stdcall<void*>(sdk);
    spdlog::info("Direct3DCreate{}(sdk={:#x}) #{} (+{}ms) -> {:#x} caller={} (thread {})", I == 0 ? 8 : 9, sdk, n,
                 uptime_ms(), A(obj), describe_address(ra), GetCurrentThreadId());
    if (obj) hook_d3d_object(I, obj);
    return obj;
}

void try_hook_d3d_create(int api) {
    if (api != 1) return;  // Direct3DCreate8 lives in dh2::d3d8
    DevApi& a = g_api[api];
    if (a.create) return;
    HMODULE m = GetModuleHandleA("d3d9.dll");
    if (!m) return;
    void* target = find_export(m, "Direct3DCreate9");
    a.create = mk("Direct3DCreate9", target, reinterpret_cast<void*>(&hk_Direct3DCreate<1>));
    if (!a.create) a.create = reinterpret_cast<InlineHook*>(-1);  // do not retry forever (never dereferenced)
}

// ---------------------------------------------------------------------------------------------
// engine hooks (EngineDll7.dll). All __thiscall exports -> __fastcall(this, edx, args...) detours.
// ---------------------------------------------------------------------------------------------
struct NetCounters {
    std::atomic<uint64_t> srv_rxtx{0}, srv_dispatch{0}, cli_rxtx{0}, cli_send{0}, cli_recv{0};
} g_net;

struct EngineHooks {
    InlineHook *srv_create{}, *srv_check_new{}, *srv_disconnect{}, *srv_shutdown{}, *srv_ignore{}, *srv_cl_disc{};
    InlineHook *srv_rxtx{}, *srv_dispatch{}, *cli_rxtx{}, *cli_send{}, *cli_recv{};
    InlineHook *cli_init{}, *cli_tryconnect{}, *cli_connsucc{}, *cli_resetlan{}, *cli_stoplisten{};
    InlineHook *bsp_load{}, *levelmap_load{};
    InlineHook *con_exec{}, *con_cheats{}, *con_dev{}, *cmd_search{}, *app_init{}, *app_mainloop{}, *app_params{};
    InlineHook *rdx_init{}, *rdx_createdevice{};
    InlineHook *hp_map_to_start{}, *hp_lastmap{}, *hp_setlastmap{}, *hp_setmission{}, *hp_mapcomplete{};
} H;

#define THIS_CALL __fastcall
#define CALLER describe_address(_ReturnAddress())

// ---- cNetServer ----
int THIS_CALL hk_NetServer_Create(void* self, void*, int a, int b, int c) {
    spdlog::info("cNetServer::Create(this={:#x}, {}, {}, {}) ENTER (+{}ms) caller={}", A(self), a, b, c, uptime_ms(),
                 CALLER);
    int r = H.srv_create->unsafe_thiscall<int>(self, a, b, c);
    spdlog::info("cNetServer::Create(this={:#x}) -> {}", A(self), r);
    return r;
}
int THIS_CALL hk_NetServer_CheckNewClients(void* self, void*) {
    int r = H.srv_check_new->unsafe_thiscall<int>(self);
    static std::atomic<int> last{0};
    if (r != 0 && last.exchange(r) != r)
        spdlog::info("cNetServer::CheckNewClients(this={:#x}) -> {} (+{}ms)", A(self), r, uptime_ms());
    return r;
}
int THIS_CALL hk_NetServer_DisconnectClient(void* self, void*, int id) {
    spdlog::info("cNetServer::DisconnectClient(this={:#x}, id={}) caller={}", A(self), id, CALLER);
    return H.srv_disconnect->unsafe_thiscall<int>(self, id);
}
int THIS_CALL hk_NetServer_ShutdownServer(void* self, void*) {
    spdlog::info("cNetServer::ShutdownServer(this={:#x}) caller={}", A(self), CALLER);
    return H.srv_shutdown->unsafe_thiscall<int>(self);
}
int THIS_CALL hk_NetServer_IgnoreRemotes(void* self, void*, int v) {
    spdlog::info("cNetServer::IgnoreRemotes(this={:#x}, {}) caller={}", A(self), v, CALLER);
    return H.srv_ignore->unsafe_thiscall<int>(self, v);
}
void THIS_CALL hk_NetServer_ClientDisconnected(void* self, void*, int id) {
    spdlog::info("cNetServer::ClientDisconnected(this={:#x}, id={})", A(self), id);
    H.srv_cl_disc->unsafe_thiscall<void>(self, id);
}
void THIS_CALL hk_NetServer_RxTx(void* self, void*) {
    ++g_net.srv_rxtx;
    H.srv_rxtx->unsafe_thiscall<void>(self);
}
void THIS_CALL hk_NetServer_DispatchMessages(void* self, void*) {
    ++g_net.srv_dispatch;
    H.srv_dispatch->unsafe_thiscall<void>(self);
}

// ---- cNetClient ----
int THIS_CALL hk_NetClient_Init(void* self, void*, int a, const char* host, int port) {
    spdlog::info("cNetClient::Init(this={:#x}, {}, host='{}', port/arg={}) ENTER (+{}ms) caller={}", A(self), a,
                 safe_cstr(host), port, uptime_ms(), CALLER);
    int r = H.cli_init->unsafe_thiscall<int>(self, a, host, port);
    spdlog::info("cNetClient::Init(this={:#x}) -> {}", A(self), r);
    return r;
}
int THIS_CALL hk_NetClient_TryConnect(void* self, void*) {
    int r = H.cli_tryconnect->unsafe_thiscall<int>(self);
    static std::atomic<uint32_t> n{0};
    if (should_log(n, 8)) spdlog::info("cNetClient::TryConnect(this={:#x}) -> {} caller={}", A(self), r, CALLER);
    return r;
}
int THIS_CALL hk_NetClient_ConnectionSucceeded(void* self, void*) {
    int r = H.cli_connsucc->unsafe_thiscall<int>(self);
    static std::atomic<int> last{-12345};
    if (last.exchange(r) != r) spdlog::info("cNetClient::ConnectionSucceeded(this={:#x}) -> {}", A(self), r);
    return r;
}
int THIS_CALL hk_NetClient_ResetLanDrv(void* self, void*) {
    spdlog::info("cNetClient::ResetLanDrv(this={:#x}) caller={}", A(self), CALLER);
    return H.cli_resetlan->unsafe_thiscall<int>(self);
}
void THIS_CALL hk_NetClient_StopListening(void* self, void*) {
    spdlog::info("cNetClient::StopListening(this={:#x}) caller={}", A(self), CALLER);
    H.cli_stoplisten->unsafe_thiscall<void>(self);
}
void THIS_CALL hk_NetClient_RxTx(void* self, void*) {
    ++g_net.cli_rxtx;
    H.cli_rxtx->unsafe_thiscall<void>(self);
}
void THIS_CALL hk_NetClient_SendMessages(void* self, void*) {
    ++g_net.cli_send;
    H.cli_send->unsafe_thiscall<void>(self);
}
void THIS_CALL hk_NetClient_ReceiveServerMessages(void* self, void*, int a) {
    ++g_net.cli_recv;
    H.cli_recv->unsafe_thiscall<void>(self, a);
}

// ---- map load ----
int THIS_CALL hk_BspTree_Load(void* self, void*, char* name, unsigned flags, int b) {
    const ULONGLONG t0 = GetTickCount64();
    spdlog::info("cBspTree::Load(this={:#x}, name='{}', {:#x}, {}) ENTER (+{}ms) caller={}", A(self), safe_cstr(name),
                 flags, b, uptime_ms(), CALLER);
    int r = H.bsp_load->unsafe_thiscall<int>(self, name, flags, b);
    spdlog::info("cBspTree::Load -> {} in {}ms", r, GetTickCount64() - t0);
    return r;
}
int THIS_CALL hk_LevelMap_Load(void* self, void*, char* name, unsigned char b) {
    spdlog::info("cLevelMap::Load(this={:#x}, name='{}', {}) (+{}ms) caller={}", A(self), safe_cstr(name), b,
                 uptime_ms(), CALLER);
    int r = H.levelmap_load->unsafe_thiscall<int>(self, name, b);
    spdlog::info("cLevelMap::Load -> {}", r);
    return r;
}

// ---- GameDll7: cHunterProfile map bookkeeping ----
char* THIS_CALL hk_HP_GetMapToBeStarted(void* self, void*) {
    char* r = H.hp_map_to_start->unsafe_thiscall<char*>(self);
    spdlog::info("cHunterProfile::GetMapToBeStarted(this={:#x}) -> '{}' (+{}ms) caller={}", A(self), safe_cstr(r),
                 uptime_ms(), CALLER);
    return r;
}
char* THIS_CALL hk_HP_GetLastmap(void* self, void*) {
    char* r = H.hp_lastmap->unsafe_thiscall<char*>(self);
    static std::atomic<uint32_t> n{0};
    if (should_log(n, 4)) spdlog::info("cHunterProfile::GetLastmap -> '{}' caller={}", safe_cstr(r), CALLER);
    return r;
}
void THIS_CALL hk_HP_SetLastMap(void* self, void*, const char* m) {
    spdlog::info("cHunterProfile::SetLastMap('{}') caller={}", safe_cstr(m), CALLER);
    H.hp_setlastmap->unsafe_thiscall<void>(self, m);
}
void THIS_CALL hk_HP_SetCurrentMission(void* self, void*, unsigned char m) {
    spdlog::info("cHunterProfile::SetCurrentMission({}) caller={}", m, CALLER);
    H.hp_setmission->unsafe_thiscall<void>(self, m);
}
void THIS_CALL hk_HP_SetMapIsComplete(void* self, void*) {
    spdlog::info("cHunterProfile::SetMapIsComplete() caller={}", CALLER);
    H.hp_mapcomplete->unsafe_thiscall<void>(self);
}

// ---- console / command line / application ----
int THIS_CALL hk_Console_Exec(void* self, void*, char* line, int flag) {
    static std::atomic<uint32_t> n{0};
    if (++n <= 400) spdlog::info("cConsole::Exec('{}', {}) caller={}", safe_cstr(line, 200), flag, CALLER);
    return H.con_exec->unsafe_thiscall<int>(self, line, flag);
}
void THIS_CALL hk_Console_SetCheats(void* self, void*, int v) {
    spdlog::info("cConsole::SetCheats({}) (+{}ms) caller={}", v, uptime_ms(), CALLER);
    H.con_cheats->unsafe_thiscall<void>(self, v);
}
void THIS_CALL hk_Console_SetDeveloper(void* self, void*, int v) {
    spdlog::info("cConsole::SetDeveloper({}) (+{}ms) caller={}", v, uptime_ms(), CALLER);
    H.con_dev->unsafe_thiscall<void>(self, v);
}
int THIS_CALL hk_CmdLine_Search(void* self, void*, char* s, int n) {
    int r = H.cmd_search->unsafe_thiscall<int>(self, s, n);
    static std::mutex m;
    static std::set<std::string> seen;
    std::string key = safe_cstr(s, 64) + "/" + std::to_string(n) + "=" + std::to_string(r);
    bool fresh;
    {
        std::lock_guard lk(m);
        fresh = seen.insert(key).second;
    }
    if (fresh) spdlog::info("cCmdLine::Search('{}', {}) -> {} caller={}", safe_cstr(s, 64), n, r, CALLER);
    return r;
}
void THIS_CALL hk_App_Init(void* self, void*, HINSTANCE a, HINSTANCE b, char* cmd, int c, int d, void* icon) {
    spdlog::info("cApplication::Init(this={:#x}, hInst={:#x}, {:#x}, cmd='{}', {}, {}, icon={:#x}) (+{}ms)", A(self),
                 A(a), A(b), safe_cstr(cmd, 300), c, d, A(icon), uptime_ms());
    H.app_init->unsafe_thiscall<void>(self, a, b, cmd, c, d, icon);
    spdlog::info("cApplication::Init returned (+{}ms)", uptime_ms());
}
void THIS_CALL hk_App_ProcesParameters(void* self, void*) {
    spdlog::info("cApplication::ProcesParameters ENTER (+{}ms)", uptime_ms());
    H.app_params->unsafe_thiscall<void>(self);
    spdlog::info("cApplication::ProcesParameters done");
}
int THIS_CALL hk_App_MainLoop(void* self, void*) {
    spdlog::info("cApplication::MainLoop ENTER (+{}ms)", uptime_ms());
    int r = H.app_mainloop->unsafe_thiscall<int>(self);
    spdlog::info("cApplication::MainLoop returned {}", r);
    return r;
}

// ---- renderer (engine side) ----
int THIS_CALL hk_RenderDX_Init(void* self, void*, HWND hwnd, const void* param) {
    spdlog::info("cRenderDX::Init(this={:#x}, hwnd={:#x}, tRenderParam={:#x}) ENTER (+{}ms) caller={}", A(self),
                 A(hwnd), A(param), uptime_ms(), CALLER);
    spdlog::info("  tRenderParam[0..96]: {}", core::hex_dump(param, 96));
    int r = H.rdx_init->unsafe_thiscall<int>(self, hwnd, param);
    spdlog::info("cRenderDX::Init -> {}", r);
    return r;
}
int THIS_CALL hk_RenderDX_CreateDevice(void* self, void*, void* d3d8, HWND hwnd, const void* param, void** dev) {
    spdlog::info("cRenderDX::CreateDevice(this={:#x}, IDirect3D8={:#x}, hwnd={:#x}, param={:#x}) ENTER caller={}",
                 A(self), A(d3d8), A(hwnd), A(param), CALLER);
    int r = H.rdx_createdevice->unsafe_thiscall<int>(self, d3d8, hwnd, param, dev);
    spdlog::info("cRenderDX::CreateDevice -> {} device={:#x}", r, dev ? A(*dev) : 0);
    return r;
}

// ---------------------------------------------------------------------------------------------
// engine static state
// ---------------------------------------------------------------------------------------------
struct EngineStatics {
    int* driver_type = nullptr;
    void** d3d = nullptr;
    void** d3dd = nullptr;
    HWND* hwnd = nullptr;
    int* windowed = nullptr;
    HINSTANCE* hinst = nullptr;
    char* app_path = nullptr;
    int* cur_class_id = nullptr;
} S;

template <class T>
T rd(const T* p, T def = T{}) {
    T v = def;
    if (p) safe_read(p, v);
    return v;
}

std::string state_line() {
    const d3d8::Stats d3 = d3d8::GetStats();
    return fmt::format(
        "driverType={} m_d3d={:#x} m_d3dd={:#x} m_hWnd={:#x} windowed={} curClassId={} | create8={} create9={} "
        "cdev8={} cdev9={} present8={} reset8={} present9={} endscene9={} | srv(rxtx={} dispatch={}) "
        "cli(rxtx={} send={} recv={})",
        rd(S.driver_type, -9999), A(rd(S.d3d)), A(rd(S.d3dd)), A(rd(S.hwnd)), rd(S.windowed, -9999),
        rd(S.cur_class_id, -9999), d3.create_calls, g_api[1].n_create.load(), d3.device_calls, g_api[1].n_cdev.load(),
        d3.present_calls, d3.reset_calls, g_api[1].n_present.load(), g_api[1].n_endscene.load(),
        g_net.srv_rxtx.load(), g_net.srv_dispatch.load(), g_net.cli_rxtx.load(), g_net.cli_send.load(),
        g_net.cli_recv.load());
}

void resolve_statics(HMODULE eng) {
    S.driver_type = core::find_export_as<int*>(eng, "?m_nDriverType@cObjManager@@0HA");
    S.cur_class_id = core::find_export_as<int*>(eng, "?m_nCurClassId@cObjManager@@0HA");
    S.d3d = core::find_export_as<void**>(eng, "?m_d3d@cRenderDX@@2PAUIDirect3D8@@A");
    S.d3dd = core::find_export_as<void**>(eng, "?m_d3dd@cRenderDX@@2PAUIDirect3DDevice8@@A");
    S.hwnd = core::find_export_as<HWND*>(eng, "?m_hWnd@cApplication@@0PAUHWND__@@A");
    S.windowed = core::find_export_as<int*>(eng, "?m_bWindowed@cApplication@@0HA");
    S.hinst = core::find_export_as<HINSTANCE*>(eng, "?m_hInst@cApplication@@0PAUHINSTANCE__@@A");
    S.app_path = core::find_export_as<char*>(eng, "?m_applicationPath@cApplication@@0PADA");

    const struct {
        const char* label;
        void* p;
    } rows[] = {
        {"cObjManager::m_nDriverType", S.driver_type},     {"cObjManager::m_nCurClassId", S.cur_class_id},
        {"cRenderDX::m_d3d", S.d3d},                       {"cRenderDX::m_d3dd", S.d3dd},
        {"cApplication::m_hWnd", S.hwnd},                  {"cApplication::m_bWindowed", S.windowed},
        {"cApplication::m_hInst", S.hinst},                {"cApplication::m_applicationPath", S.app_path},
    };
    for (auto& r : rows)
        spdlog::info("static {} @ {:#x} ({})", r.label, A(r.p), describe_address(r.p));

    if (S.app_path) {
        // MSVC mangles both `char*` and `char[N]` as PAD: show both interpretations.
        char* deref = nullptr;
        safe_read(S.app_path, deref);
        spdlog::info("m_applicationPath as array: '{}' | as pointer: '{}'", safe_cstr(S.app_path),
                     safe_cstr(reinterpret_cast<const char*>(A(deref) > 0x10000 ? deref : nullptr)));
    }
    spdlog::info("initial {}", state_line());
}

// A handful of exports whose addresses we just want on record (used by later tracks).
void log_export_addresses(HMODULE eng) {
    static const char* names[] = {
        "?Create@cNetServer@@QAEHHHH@Z",
        "?Init@cNetClient@@QAEHHPBDH@Z",
        "?TryConnect@cNetClient@@QAEHXZ",
        "?CheckNewClients@cNetServer@@UAEHXZ",
        "?AddEntity@cObjManager@@UAEHPBDH@Z",
        "?Load@cBspTree@@QAEHPADIH@Z",
        "?MainLoop@cApplication@@UAEHXZ",
        "?CreateDevice@cRenderDX@@IAEHPAUIDirect3D8@@PAUHWND__@@ABUtRenderParam@@AAPAUIDirect3DDevice8@@@Z",
        "?EngLuaExecuteScript@@YAHPBD@Z",
    };
    for (const char* n : names) {
        void* p = GetProcAddress(eng, n);
        if (p)
            spdlog::info("export {} = {:#x} ({})", n, A(p), describe_address(p));
        else
            spdlog::info("export {} : NOT FOUND", n);
    }
}

void install_engine_hooks(HMODULE eng) {
    auto R = [&](const char* n) { return find_export(eng, n); };
    auto V = [](auto* f) { return reinterpret_cast<void*>(f); };

    if (g_cfg.probe_hook_net) {
        H.srv_create = mk("cNetServer::Create", R("?Create@cNetServer@@QAEHHHH@Z"), V(hk_NetServer_Create));
        H.srv_check_new = mk("cNetServer::CheckNewClients", R("?CheckNewClients@cNetServer@@UAEHXZ"),
                             V(hk_NetServer_CheckNewClients));
        H.srv_disconnect = mk("cNetServer::DisconnectClient", R("?DisconnectClient@cNetServer@@QAEHH@Z"),
                              V(hk_NetServer_DisconnectClient));
        H.srv_shutdown =
            mk("cNetServer::ShutdownServer", R("?ShutdownServer@cNetServer@@QAEHXZ"), V(hk_NetServer_ShutdownServer));
        H.srv_ignore =
            mk("cNetServer::IgnoreRemotes", R("?IgnoreRemotes@cNetServer@@QAEHH@Z"), V(hk_NetServer_IgnoreRemotes));
        H.srv_cl_disc = mk("cNetServer::ClientDisconnected", R("?ClientDisconnected@cNetServer@@MAEXH@Z"),
                           V(hk_NetServer_ClientDisconnected));
        H.srv_rxtx = mk("cNetServer::RxTx", R("?RxTx@cNetServer@@QAEXXZ"), V(hk_NetServer_RxTx));
        H.srv_dispatch =
            mk("cNetServer::DispatchMessages", R("?DispatchMessages@cNetServer@@QAEXXZ"), V(hk_NetServer_DispatchMessages));

        H.cli_init = mk("cNetClient::Init", R("?Init@cNetClient@@QAEHHPBDH@Z"), V(hk_NetClient_Init));
        H.cli_tryconnect = mk("cNetClient::TryConnect", R("?TryConnect@cNetClient@@QAEHXZ"), V(hk_NetClient_TryConnect));
        H.cli_connsucc = mk("cNetClient::ConnectionSucceeded", R("?ConnectionSucceeded@cNetClient@@QAEHXZ"),
                            V(hk_NetClient_ConnectionSucceeded));
        H.cli_resetlan = mk("cNetClient::ResetLanDrv", R("?ResetLanDrv@cNetClient@@QAEHXZ"), V(hk_NetClient_ResetLanDrv));
        H.cli_stoplisten =
            mk("cNetClient::StopListening", R("?StopListening@cNetClient@@QAEXXZ"), V(hk_NetClient_StopListening));
        H.cli_rxtx = mk("cNetClient::RxTx", R("?RxTx@cNetClient@@QAEXXZ"), V(hk_NetClient_RxTx));
        H.cli_send = mk("cNetClient::SendMessages", R("?SendMessages@cNetClient@@QAEXXZ"), V(hk_NetClient_SendMessages));
        H.cli_recv = mk("cNetClient::ReceiveServerMessages", R("?ReceiveServerMessages@cNetClient@@QAEXH@Z"),
                        V(hk_NetClient_ReceiveServerMessages));
    }
    if (g_cfg.probe_hook_map) {
        H.bsp_load = mk("cBspTree::Load", R("?Load@cBspTree@@QAEHPADIH@Z"), V(hk_BspTree_Load));
        H.levelmap_load = mk("cLevelMap::Load", R("?Load@cLevelMap@@UAEHPADE@Z"), V(hk_LevelMap_Load));
    }
    if (g_cfg.probe_hook_console) {
        H.con_exec = mk("cConsole::Exec", R("?Exec@cConsole@@QAEHPADH@Z"), V(hk_Console_Exec));
        H.con_cheats = mk("cConsole::SetCheats", R("?SetCheats@cConsole@@QAEXH@Z"), V(hk_Console_SetCheats));
        H.con_dev = mk("cConsole::SetDeveloper", R("?SetDeveloper@cConsole@@QAEXH@Z"), V(hk_Console_SetDeveloper));
        H.cmd_search = mk("cCmdLine::Search", R("?Search@cCmdLine@@UAEHPADH@Z"), V(hk_CmdLine_Search));
        H.app_init = mk("cApplication::Init", R("?Init@cApplication@@UAEXPAUHINSTANCE__@@0PADHHPAUHICON__@@@Z"),
                        V(hk_App_Init));
        H.app_params = mk("cApplication::ProcesParameters", R("?ProcesParameters@cApplication@@UAEXXZ"),
                          V(hk_App_ProcesParameters));
        H.app_mainloop = mk("cApplication::MainLoop", R("?MainLoop@cApplication@@UAEHXZ"), V(hk_App_MainLoop));
    }
    if (g_cfg.probe_hook_d3d) {
        H.rdx_init = mk("cRenderDX::Init", R("?Init@cRenderDX@@UAEHPAUHWND__@@ABUtRenderParam@@@Z"), V(hk_RenderDX_Init));
        H.rdx_createdevice = mk("cRenderDX::CreateDevice",
                                R("?CreateDevice@cRenderDX@@IAEHPAUIDirect3D8@@PAUHWND__@@ABUtRenderParam@@AAPAUIDirect3DDevice8@@@Z"),
                                V(hk_RenderDX_CreateDevice));
    }
}

void install_game_hooks(HMODULE game) {
    auto R = [&](const char* n) { return find_export(game, n); };
    auto V = [](auto* f) { return reinterpret_cast<void*>(f); };
    if (!g_cfg.probe_hook_map) return;
    H.hp_map_to_start = mk("cHunterProfile::GetMapToBeStarted", R("?GetMapToBeStarted@cHunterProfile@@QAEPADXZ"),
                           V(hk_HP_GetMapToBeStarted));
    H.hp_lastmap = mk("cHunterProfile::GetLastmap", R("?GetLastmap@cHunterProfile@@QAEPADXZ"), V(hk_HP_GetLastmap));
    H.hp_setlastmap =
        mk("cHunterProfile::SetLastMap", R("?SetLastMap@cHunterProfile@@QAEXPBD@Z"), V(hk_HP_SetLastMap));
    H.hp_setmission =
        mk("cHunterProfile::SetCurrentMission", R("?SetCurrentMission@cHunterProfile@@QAEXE@Z"), V(hk_HP_SetCurrentMission));
    H.hp_mapcomplete =
        mk("cHunterProfile::SetMapIsComplete", R("?SetMapIsComplete@cHunterProfile@@QAEXXZ"), V(hk_HP_SetMapIsComplete));
}

// ---------------------------------------------------------------------------------------------
// module tracking
// ---------------------------------------------------------------------------------------------
struct LdrStr {
    USHORT Length, MaximumLength;
    PWSTR Buffer;
};
struct LdrNotifData {
    ULONG Flags;
    const LdrStr* FullDllName;
    const LdrStr* BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
};
using LdrNotifFn = VOID(NTAPI*)(ULONG reason, const LdrNotifData* data, PVOID ctx);
using LdrRegisterFn = LONG(NTAPI*)(ULONG flags, LdrNotifFn fn, PVOID ctx, PVOID* cookie);

// Driver probing makes d3d8 load/unload the same GPU driver DLLs repeatedly: log each path once, and
// only log unloads of modules we care about.
bool ldr_interesting(const std::string& lower) {
    for (const char* k : {"d3d", "dll7", "binkw32", "ws2_32", "dinput", "dxgi", "dgvoodoo", ".asi", "msacm", "dsound"})
        if (lower.find(k) != std::string::npos) return true;
    return false;
}

VOID NTAPI on_dll_event(ULONG reason, const LdrNotifData* d, PVOID) {
    if ((reason != 1 && reason != 2) || !d || !d->FullDllName) return;
    char name[MAX_PATH] = {};
    WideCharToMultiByte(CP_ACP, 0, d->FullDllName->Buffer, d->FullDllName->Length / 2, name, MAX_PATH - 1, nullptr,
                        nullptr);
    std::string lower = name;
    for (auto& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    static std::mutex m;
    static std::set<std::string> seen;
    bool log = false;
    {
        std::lock_guard lk(m);
        if (reason == 1)
            log = seen.insert(lower).second;
        else
            log = ldr_interesting(lower);
    }
    if (!log) return;
    spdlog::info("[ldr] {} {} base={:#x} size={:#x} (+{}ms)", reason == 1 ? "LOADED  " : "UNLOADED", name,
                 A(d->DllBase), d->SizeOfImage, uptime_ms());
}

void log_loaded_modules(const char* when) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) {
        spdlog::warn("module snapshot failed ({})", GetLastError());
        return;
    }
    MODULEENTRY32 e{};
    e.dwSize = sizeof e;
    int i = 0;
    spdlog::info("--- loaded modules ({}), load order ---", when);
    if (Module32First(snap, &e)) {
        do {
            spdlog::info("  [{:2}] base={:#010x} size={:#09x} {}", i++, A(e.modBaseAddr), e.modBaseSize, e.szExePath);
        } while (Module32Next(snap, &e));
    }
    CloseHandle(snap);
    spdlog::info("--- end of module list ---");
}

void log_header() {
    char exe[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    spdlog::info("process: {} pid={} cmdline: {}", exe, GetCurrentProcessId(), GetCommandLineA());
    spdlog::info("probe thread started +{}ms after process creation", uptime_ms());
    for (const char* n : {"d3d8.dll", "d3d9.dll", "dinput8.dll", "EngineDll7.dll", "CommonDll7.dll", "GameDll7.dll",
                          "ws2_32.dll", "binkw32.dll", "dgVoodooD3D8.dll"}) {
        HMODULE m = GetModuleHandleA(n);
        char path[MAX_PATH] = {};
        if (m) GetModuleFileNameA(m, path, MAX_PATH);
        spdlog::info("  {:<16} {}", n, m ? fmt::format("loaded base={:#x} {}", A(m), path) : std::string("not loaded"));
    }
}

void probe_on_present(void* device, const RECT*, const RECT*, HWND wnd, const void*) {
    static std::atomic<uint64_t> n{0};
    const uint64_t i = ++n;
    if (i <= 3) {
        spdlog::info("probe D3D8 Present #{} dev={:#x} hwnd={:#x} caller={}", i, A(device), A(wnd),
                     describe_address(_ReturnAddress()));
    }
}

void sample_state() {
    if (!g_active) return;
    const ULONGLONG now = GetTickCount64();
    if (g_state_samples < g_cfg.probe_state_seconds) {
        if (g_next_sample == 0) g_next_sample = now;
        if (now < g_next_sample) return;
        spdlog::info("state t={}s {}", g_state_samples + 1, state_line());
        ++g_state_samples;
        g_next_sample += 1000;
        return;
    }
    if (now - g_last_change_log <= 2000) return;
    std::string s = state_line();
    if (s == g_last_state) return;
    spdlog::info("state(change) {}", s);
    g_last_state = std::move(s);
    g_last_change_log = now;
}

}  // namespace

class ProbeModule : public dh2::Module {
public:
    const char* Name() const override { return "probe"; }
    int Priority() const override { return 10; }

    void OnInit() override {
        g_cfg = core::config();
        if (!g_cfg.probe_enabled) {
            spdlog::info("probe module off ([Probe] Enabled=0)");
            return;
        }
        g_active = true;
        spdlog::info("=== runtime probe start ===");
        log_header();
        log_loaded_modules("probe start");
        if (HMODULE nt = GetModuleHandleA("ntdll.dll")) {
            auto reg = reinterpret_cast<LdrRegisterFn>(GetProcAddress(nt, "LdrRegisterDllNotification"));
            PVOID cookie = nullptr;
            if (reg) {
                const LONG st = reg(0, on_dll_event, nullptr, &cookie);
                spdlog::info("probe LdrRegisterDllNotification -> {:#x}", static_cast<uint32_t>(st));
            }
        }
        if (g_cfg.probe_hook_d3d) d3d8::AddPresentCallback(&probe_on_present);
    }

    void OnEngineReady(HMODULE engine) override {
        if (!g_active || !engine) return;
        log_export_addresses(engine);
        resolve_statics(engine);
        install_engine_hooks(engine);
        g_next_sample = GetTickCount64();
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll")) {
            spdlog::info("CommonDll7.dll base = {:#x} (+{}ms)", A(common), uptime_ms());
        }
    }

    void OnGameDllLoaded(HMODULE game) override {
        if (!g_active || !game) return;
        install_game_hooks(game);
    }

    void OnFrame(float) override { sample_state(); }
};

DH2_MODULE(ProbeModule)

void tick() {
    if (!g_active || !g_cfg.probe_hook_d3d || g_d3d9_logged) return;
    try_hook_d3d_create(1);
    if (g_api[1].create) {
        g_d3d9_logged = true;
        spdlog::info("d3d9.dll loaded; Direct3DCreate9 hook ready (+{}ms)", uptime_ms());
    }
}

}  // namespace dh2::probe
