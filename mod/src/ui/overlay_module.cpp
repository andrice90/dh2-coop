// Read-only co-op status overlay. Drawn from the D3D8 Present callback on the game thread.
#include "core/module.h"
#include "core/log.h"
#include "coop/coop.h"
#include "engine/d3d8_hooks.h"
#include "ui/imgui_impl_dx8.h"

#include <imgui.h>
#include <d3d8.h>

#include <spdlog/spdlog.h>

#include <cstdint>
#include <string>

namespace {

struct OverlayState {
    bool inited = false;
    bool visible = true;
    bool keys_ready = false;
    bool f8_down = false;
    bool logged_begin_fail = false;
    LARGE_INTEGER qpc_freq{};
    LARGE_INTEGER qpc_last{};
};

OverlayState g_ov;

float overlay_dt() {
    if (g_ov.qpc_freq.QuadPart == 0) QueryPerformanceFrequency(&g_ov.qpc_freq);
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    float dt = 1.0f / 60.0f;
    if (g_ov.qpc_last.QuadPart != 0 && g_ov.qpc_freq.QuadPart != 0) {
        dt = static_cast<float>(static_cast<double>(now.QuadPart - g_ov.qpc_last.QuadPart) /
                                static_cast<double>(g_ov.qpc_freq.QuadPart));
    }
    g_ov.qpc_last = now;
    if (dt < 0.000001f) dt = 0.000001f;
    if (dt > 0.25f) dt = 0.25f;
    return dt;
}

HWND game_window(IDirect3DDevice8* dev, HWND passed) {
    if (passed) return passed;
    D3DDEVICE_CREATION_PARAMETERS cp{};
    if (SUCCEEDED(dev->GetCreationParameters(&cp))) return cp.hFocusWindow;
    return nullptr;
}

bool is_foreground(HWND hwnd) {
    if (!hwnd) return false;
    const HWND fg = GetForegroundWindow();
    if (!fg) return false;
    if (fg == hwnd) return true;
    const HWND root = GetAncestor(hwnd, GA_ROOT);
    return root != nullptr && fg == root;
}

ImVec2 display_size(IDirect3DDevice8* dev, HWND window) {
    IDirect3DSurface8* back = nullptr;
    if (SUCCEEDED(dev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back)) && back) {
        D3DSURFACE_DESC desc{};
        const HRESULT hr = back->GetDesc(&desc);
        back->Release();
        if (SUCCEEDED(hr) && desc.Width > 0 && desc.Height > 0)
            return ImVec2(static_cast<float>(desc.Width), static_cast<float>(desc.Height));
    }
    if (window) {
        RECT rc{};
        if (GetClientRect(window, &rc) && rc.right > 0 && rc.bottom > 0)
            return ImVec2(static_cast<float>(rc.right), static_cast<float>(rc.bottom));
    }
    return ImVec2(640.0f, 480.0f);
}

void draw_overlay(const dh2::coop::SessionView& view) {
    ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.62f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 0.0f), ImVec2(340.0f, 460.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.07f, 0.09f, 0.62f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.10f, 0.12f, 0.16f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.12f, 0.16f, 0.22f, 0.80f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs;
    ImGui::Begin("co-op    F8 hide", nullptr, flags);

    ImGui::Text("role %s", view.role.empty() ? "offline" : view.role.c_str());
    if (view.role == "client") ImGui::TextUnformatted("F9 stand up at the host");
    if (view.localId == 0xFF)
        ImGui::TextUnformatted("local id -");
    else
        ImGui::Text("local id %u", static_cast<unsigned>(view.localId));
    ImGui::Text("port %u", static_cast<unsigned>(view.port));

    if (view.hunterFound) {
        ImGui::Text("hunter %.1f %.1f %.1f  yaw %.1f", static_cast<double>(view.hunterPos[0]),
                    static_cast<double>(view.hunterPos[1]), static_cast<double>(view.hunterPos[2]),
                    static_cast<double>(view.hunterYaw));
    } else {
        ImGui::TextUnformatted("hunter not found");
    }

    ImGui::Separator();
    if (ImGui::BeginTable("remotes", 6, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("id");
        ImGui::TableSetupColumn("name");
        ImGui::TableSetupColumn("pos");
        ImGui::TableSetupColumn("yaw");
        ImGui::TableSetupColumn("pkt");
        ImGui::TableSetupColumn("rtt");
        ImGui::TableHeadersRow();
        for (int i = 0; i < view.remoteCount && i < 4; ++i) {
            const dh2::coop::RemoteView& r = view.remotes[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", static_cast<unsigned>(r.id));
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(r.name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.1f %.1f %.1f", static_cast<double>(r.pos[0]), static_cast<double>(r.pos[1]),
                        static_cast<double>(r.pos[2]));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.1f", static_cast<double>(r.yaw));
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%u", static_cast<unsigned>(r.packets));
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%u", static_cast<unsigned>(r.rttMs));
        }
        ImGui::EndTable();
    }
    if (view.remoteCount <= 0) ImGui::TextUnformatted("no remotes");

    ImGui::Separator();
    ImGui::Text("animals %d", view.animals);
    ImGui::Text("fps %.0f", static_cast<double>(ImGui::GetIO().Framerate));
    ImGui::End();
    ImGui::PopStyleColor(3);
}

bool ensure_imgui(IDirect3DDevice8* dev) {
    if (g_ov.inited) return true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplDX8_Init(dev)) {
        spdlog::error("overlay: ImGui_ImplDX8_Init failed");
        ImGui::DestroyContext();
        return false;
    }
    g_ov.inited = true;
    spdlog::info("overlay: imgui dx8 initialized");
    return true;
}

void on_present(void* device, const RECT* src, const RECT* dst, HWND window, const void* dirty) {
    (void)src;
    (void)dst;
    (void)dirty;
    auto* dev = static_cast<IDirect3DDevice8*>(device);
    if (!dev || !ensure_imgui(dev)) return;

    const HWND hwnd = game_window(dev, window);
    const bool down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (!g_ov.keys_ready) {
        g_ov.f8_down = down;
        g_ov.keys_ready = true;
    } else {
        if (is_foreground(hwnd) && down && !g_ov.f8_down) {
            g_ov.visible = !g_ov.visible;
            spdlog::info("overlay: visible={}", g_ov.visible ? 1 : 0);
        }
        g_ov.f8_down = down;
    }
    if (!g_ov.visible) return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = display_size(dev, hwnd);
    io.DeltaTime = overlay_dt();
    io.MousePos = ImVec2(-100000.0f, -100000.0f);
    io.MouseDown[0] = io.MouseDown[1] = io.MouseDown[2] = false;

    static bool logged_size = false;
    if (!logged_size) {
        logged_size = true;
        spdlog::info("overlay: display {:.0f}x{:.0f}", static_cast<double>(io.DisplaySize.x),
                     static_cast<double>(io.DisplaySize.y));
    }

    ImGui_ImplDX8_NewFrame();
    ImGui::NewFrame();
    draw_overlay(dh2::coop::Snapshot());
    ImGui::Render();

    ImDrawData* dd = ImGui::GetDrawData();
    // PrepareDraw uploads the font (LockRect) and captures D3DSBT_ALL before the scene.
    // The game has already ended its scene by the time Present runs.
    if (!ImGui_ImplDX8_PrepareDraw(dd)) return;
    const HRESULT hr = dev->BeginScene();
    if (FAILED(hr)) {
        if (!g_ov.logged_begin_fail) {
            g_ov.logged_begin_fail = true;
            spdlog::error("overlay: BeginScene hr={:#010x}", static_cast<std::uint32_t>(hr));
        }
        ImGui_ImplDX8_FinishDraw();
        return;
    }
    ImGui_ImplDX8_RenderPrepared(dd);
    dev->EndScene();
    ImGui_ImplDX8_FinishDraw();
}

void on_reset(void* device, void* present_params, int phase) {
    (void)device;
    (void)present_params;
    if (!g_ov.inited || !ImGui::GetCurrentContext()) return;
    if (phase == 0) {
        ImGui_ImplDX8_InvalidateDeviceObjects();
        spdlog::info("overlay: reset phase 0, device objects invalidated");
    } else {
        ImGui_ImplDX8_CreateDeviceObjects();
        spdlog::info("overlay: reset phase 1");
    }
}

class OverlayModule : public dh2::Module {
public:
    const char* Name() const override { return "overlay"; }
    int Priority() const override { return 200; }

    void OnInit() override {
        const std::string ini = (dh2::core::self_dir() / "dh2coop.ini").string();
        g_ov.visible = GetPrivateProfileIntA("Overlay", "Enabled", 1, ini.c_str()) != 0;
        spdlog::info("overlay: ini Enabled={}", g_ov.visible ? 1 : 0);
        dh2::d3d8::AddPresentCallback(&on_present);
        dh2::d3d8::AddResetCallback(&on_reset);
    }

    // OnShutdown runs on the ASI init thread, not the device thread. The present
    // callback AddRefs the device; process exit drops that ref. Do not call D3D here.
    void OnShutdown() override {}
};

DH2_MODULE(OverlayModule)

}  // namespace
