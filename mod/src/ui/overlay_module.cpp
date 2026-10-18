// Co-op overlay: the lobby window (F8, opens by itself at the main menu), player markers (F7) and on-screen notices.
// Drawn from the D3D8 Present callback on the game thread. While the lobby is open it takes the mouse and keyboard
// (Win32 messages through ImGui's Win32 backend) and the game's DirectInput reads see nothing pressed.
#include "core/module.h"
#include "core/log.h"
#include "core/safe_mem.h"
#include "display/display.h"
#include "coop/coop.h"
#include "coop/flow.h"
#include "coop/player_tracker.h"
#include "coop/puppet_models.h"
#include "engine/d3d8_hooks.h"
#include "engine/dinput_hooks.h"
#include "ui/imgui_impl_dx8.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <d3d8.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace {

struct LobbyFields {
    bool loaded = false;
    char name[32] = {};
    int model = 0;
    char join_text[640] = {};  // a join code (up to 300 characters), or IP / IP:port
    int join_port = 27420;     // the port for an IP typed without one
    int host_port = 27420;
    std::string error;  // input check before HOST / JOIN
    std::string info;   // what the JOIN field holds, or what the last Copy did
};

struct OverlayState {
    bool inited = false;
    bool visible = true;  // the lobby window
    bool auto_open = true;  // [Overlay] Enabled: open the lobby by itself at the main menu
    bool at_menu = false;
    bool win32 = false;
    HWND hwnd = nullptr;
    WNDPROC old_proc = nullptr;
    LobbyFields lobby;
    bool markers = true;
    bool marker_test = false;
    bool keys_ready = false;
    bool f8_down = false;
    bool f7_down = false;
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

// The old F8 status panel, now a "Details" section at the bottom of the lobby window.
void draw_details(const dh2::coop::SessionView& view) {
    ImGui::Text("role %s", view.role.empty() ? "offline" : view.role.c_str());
    if (view.role == "client") ImGui::TextUnformatted("F9 stand up at the host");
    if (view.localId == 0xFF)
        ImGui::TextUnformatted("local id -");
    else
        ImGui::Text("local id %u", static_cast<unsigned>(view.localId));
    ImGui::Text("port %u", static_cast<unsigned>(view.port));
    if (!view.tailnetIp.empty())
        ImGui::Text("tailnet %s", view.tailnetIp.c_str());
    else
        ImGui::TextUnformatted("tailnet -  (Tailscale not connected)");
    if (!view.otherIps.empty()) ImGui::Text("lan %s", view.otherIps.c_str());

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
}

// ---- lobby window ----

void load_lobby_fields() {
    LobbyFields& f = g_ov.lobby;
    if (f.loaded) return;
    const dh2::coop::LobbyPrefs p = dh2::coop::LoadLobbyPrefs();
    std::snprintf(f.name, sizeof f.name, "%s", p.name.c_str());
    f.model = p.model;
    std::snprintf(f.join_text, sizeof f.join_text, "%s", p.join_host.c_str());
    f.join_port = p.join_port;
    f.host_port = p.host_port;
    f.loaded = true;
}

void save_lobby_fields() {
    const LobbyFields& f = g_ov.lobby;
    dh2::coop::LobbyPrefs p;
    p.name = f.name;
    p.model = f.model;
    p.join_host = f.join_text;
    p.join_port = static_cast<uint16_t>(f.join_port);
    p.host_port = static_cast<uint16_t>(f.host_port);
    dh2::coop::SaveLobbyPrefs(p);
}

std::string trimmed(const char* s) {
    std::string t = s ? s : "";
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back()))) t.pop_back();
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.front()))) t.erase(t.begin());
    return t;
}

const char* role_name(uint8_t id) { return id == 0 ? "host" : "player"; }

ImVec4 state_color(dh2::coop::LobbyState s) {
    using dh2::coop::LobbyState;
    switch (s) {
        case LobbyState::Hosting:
        case LobbyState::Connected: return ImVec4(0.45f, 0.90f, 0.45f, 1.f);
        case LobbyState::Connecting: return ImVec4(0.95f, 0.85f, 0.35f, 1.f);
        case LobbyState::Failed: return ImVec4(1.f, 0.45f, 0.40f, 1.f);
        default: return ImVec4(0.80f, 0.80f, 0.80f, 1.f);
    }
}

bool model_combo(const char* label, int* model) {
    const int n = dh2::coop::PuppetModelCount();
    if (*model < 0 || *model >= n) *model = 0;
    bool changed = false;
    if (ImGui::BeginCombo(label, dh2::coop::PuppetModelAt(*model).label)) {
        for (int i = 0; i < n; ++i) {
            const bool selected = i == *model;
            if (ImGui::Selectable(dh2::coop::PuppetModelAt(i).label, selected)) {
                *model = i;
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

// What the JOIN field holds, as the line under it: a code's host, or why the text cannot be used.
void describe_join_text(LobbyFields& f) {
    f.error.clear();
    f.info.clear();
    const dh2::coop::JoinInput in = dh2::coop::ParseJoinInput(f.join_text, static_cast<uint16_t>(f.join_port));
    if (in.kind == dh2::coop::JoinInput::Kind::Code) f.info = in.message;
    else if (in.kind == dh2::coop::JoinInput::Kind::Bad) f.error = in.message;
}

// Where the address column starts (window padding + the widest label).
float label_column() {
    const ImGuiStyle& s = ImGui::GetStyle();
    return s.WindowPadding.x + ImGui::CalcTextSize("Join code").x + s.ItemSpacing.x * 2.f;
}

// One "how friends reach you" row on the host: label, address:port, Copy. An empty address shows `why` instead.
void address_row(const char* label, const std::string& ip, uint16_t port, const std::string& why, const char* id) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(label_column());
    if (ip.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", why.c_str());
        ImGui::PopStyleColor();
        return;
    }
    const std::string text = ip + ":" + std::to_string(port);
    ImGui::TextUnformatted(text.c_str());
    ImGui::SameLine();
    ImGui::PushID(id);
    if (ImGui::SmallButton("Copy")) {
        ImGui::SetClipboardText(text.c_str());
        g_ov.lobby.error.clear();
        g_ov.lobby.info = std::string(label) + " address copied: " + text;
    }
    ImGui::PopID();
}

// HOST: how friends reach this PC, best first: the internet (UPnP port forward, with the join code), the tailnet
// (Tailscale, set up by hand), the LAN.
void draw_host_addresses(const dh2::coop::LobbyView& lobby) {
    LobbyFields& f = g_ov.lobby;
    ImGui::TextUnformatted("Friends join with:");
    if (lobby.internetOpen) {
        address_row("Internet", lobby.internetIp, lobby.internetPort, {}, "inet");
        const std::string name = trimmed(f.name);
        const dh2::coop::JoinCodeResult code = dh2::coop::MakeJoinCode(name.empty() ? std::string("Hunter") : name);
        if (!code.code.empty()) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Join code");
            ImGui::SameLine(label_column());
            ImGui::TextUnformatted(code.code.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Copy##code")) {
                std::string msg;
                f.error.clear();
                f.info.clear();
                if (dh2::coop::CopyJoinCode(name.empty() ? std::string("Hunter") : name, msg)) f.info = msg;
                else f.error = msg;
            }
        }
    } else {
        address_row("Internet", {}, 0,
                    lobby.internetWorking ? std::string("asking your router to open the port (UPnP) ...")
                    : lobby.internetMessage.empty() ? std::string("off")
                                                    : lobby.internetMessage,
                    "inet");
    }
    address_row("Tailnet", lobby.tailnetIp, lobby.port, "- (Tailscale is not running on this PC)", "tail");
    std::string lan = lobby.otherIps;
    const size_t comma = lan.find(',');
    if (comma != std::string::npos) lan.resize(comma);
    address_row("LAN", lan, lobby.port, "- (no LAN address)", "lan");
}

void draw_lobby(const dh2::coop::SessionView& view, ImVec2 disp) {
    using dh2::coop::LobbyState;
    load_lobby_fields();
    LobbyFields& f = g_ov.lobby;
    const dh2::coop::LobbyView lobby = dh2::coop::Lobby();
    const bool busy = lobby.state == LobbyState::Hosting || lobby.state == LobbyState::Connected ||
                      lobby.state == LobbyState::Connecting;

    const float scale = std::clamp(disp.y / 900.f, 1.f, 2.f);
    ImGui::SetNextWindowPos(ImVec2(disp.x * 0.5f, disp.y * 0.18f), ImGuiCond_Always, ImVec2(0.5f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(480.f * scale, 0.f), ImVec2(640.f * scale, disp.y * 0.8f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("Co-op lobby  (F8 shows / hides)", nullptr, flags);
    ImGui::SetWindowFontScale(scale);

    ImGui::PushStyleColor(ImGuiCol_Text, state_color(lobby.state));
    ImGui::TextWrapped("%s", lobby.message.c_str());
    ImGui::PopStyleColor();
    if (!f.error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, state_color(LobbyState::Failed));
        ImGui::TextWrapped("%s", f.error.c_str());
        ImGui::PopStyleColor();
    }
    if (!f.info.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.85f, 1.f, 1.f));
        ImGui::TextWrapped("%s", f.info.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::Separator();

    // Test twin of a paste into the JOIN field (console: coop_joinfield <join code>).
    std::string pasted;
    if (dh2::coop::TakePendingJoinPaste(pasted)) {
        std::snprintf(f.join_text, sizeof f.join_text, "%s", pasted.c_str());
        describe_join_text(f);
    }

    // Your player: name and body are sent when the session starts, so they are fixed while in one.
    if (!busy) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Your name");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(190.f * scale);
        ImGui::InputText("##name", f.name, sizeof f.name);
        ImGui::SameLine();
        ImGui::TextUnformatted("Model");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.f * scale);
        model_combo("##model", &f.model);
    } else {
        for (const auto& p : lobby.players) {
            if (p.you) ImGui::Text("You are %s, seen as %s.", p.name.c_str(), dh2::coop::PuppetModelAt(p.model).label);
        }
    }
    ImGui::Separator();

    if (!busy) {
        ImGui::TextUnformatted("Host a game");
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Port");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.f * scale);
        ImGui::InputInt("##hostport", &f.host_port, 0, 0);
        ImGui::SameLine();
        if (ImGui::Button("HOST", ImVec2(110.f * scale, 0.f))) {
            f.error.clear();
            if (trimmed(f.name).empty()) {
                f.error = "Type a player name first.";
            } else if (f.host_port < 1 || f.host_port > 65535) {
                f.error = "The port must be 1 to 65535 (default 27420).";
            } else {
                save_lobby_fields();
                dh2::coop::LobbyHost(trimmed(f.name), f.model, static_cast<uint16_t>(f.host_port));
            }
        }
        ImGui::Spacing();
        ImGui::TextUnformatted("Join a friend");
        ImGui::SetNextItemWidth(330.f * scale);
        if (ImGui::InputTextWithHint("##join", "join code, or IP / IP:port", f.join_text, sizeof f.join_text))
            describe_join_text(f);
        ImGui::SameLine();
        if (ImGui::Button("JOIN", ImVec2(110.f * scale, 0.f))) {
            const dh2::coop::JoinInput in =
                dh2::coop::ParseJoinInput(f.join_text, static_cast<uint16_t>(std::clamp(f.join_port, 1, 65535)));
            f.error.clear();
            if (trimmed(f.name).empty()) {
                f.error = "Type a player name first.";
            } else if (in.kind == dh2::coop::JoinInput::Kind::Empty) {
                f.error = "Paste the host's join code, or type their IP (IP:port when it is not 27420).";
            } else if (in.kind == dh2::coop::JoinInput::Kind::Bad) {
                f.error = in.message;
            } else {
                save_lobby_fields();
                dh2::coop::LobbyJoin(trimmed(f.name), f.model, in.host, in.port, in.pass);
            }
        }
    } else {
        if (lobby.state == LobbyState::Hosting) {
            draw_host_addresses(lobby);
            ImGui::Spacing();
        }
        ImGui::Text("Players (%d of 4)", static_cast<int>(lobby.players.size()));
        if (ImGui::BeginTable("players", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
                                                ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Role");
            ImGui::TableSetupColumn("Model");
            ImGui::TableSetupColumn("Ping");
            ImGui::TableHeadersRow();
            for (const auto& p : lobby.players) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s%s", p.name.c_str(), p.you ? "  (you)" : "");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(role_name(p.id));
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(dh2::coop::PuppetModelAt(p.model).label);
                ImGui::TableSetColumnIndex(3);
                if (p.you)
                    ImGui::TextUnformatted("-");
                else
                    ImGui::Text("%u ms", p.rttMs);
            }
            ImGui::EndTable();
        }
        ImGui::Spacing();
        const char* hint = nullptr;
        if (lobby.state == LobbyState::Hosting && lobby.atMenu)
            hint = "Close this window (F8), then start a New Game or Load a save. Everyone follows you.";
        else if (lobby.state == LobbyState::Connected && lobby.atMenu)
            hint = "Stay in the main menu. When the host starts a game you follow automatically.";
        else if (lobby.state == LobbyState::Connecting)
            hint = "Waiting for the host to answer (a few seconds).";
        if (hint) ImGui::TextWrapped("%s", hint);
        if (lobby.state == LobbyState::Hosting && !lobby.atMenu && !lobby.map.empty()) {
            // Level recovery for everyone: the first click arms (on-screen "click again"), the second does it.
            ImGui::Text("Level %s", lobby.map.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Restart level (F5)")) dh2::coop::HostRestartLevel();
            const std::string next = dh2::coop::NextStoryMap(lobby.map);
            if (!next.empty()) {
                ImGui::SameLine();
                const std::string label = "Next level: " + next + " (F6)";
                if (ImGui::Button(label.c_str())) dh2::coop::HostNextLevel();
            }
        }
        const char* leave = lobby.state == LobbyState::Hosting   ? "Stop hosting"
                            : lobby.state == LobbyState::Connecting ? "Cancel"
                                                                    : "Leave";
        if (ImGui::Button(leave, ImVec2(140.f * scale, 0.f))) {
            f.error.clear();
            dh2::coop::LobbyLeave();
        }
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Details")) draw_details(view);
    ImGui::End();
}

// Game window messages while the lobby is open go to ImGui. Keys and characters stop there (the game reads keys
// through DirectInput, which SetUiCapture blanks); everything else, Alt+F4 included, still reaches the game.
LRESULT CALLBACK lobby_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (g_ov.visible && g_ov.hwnd && ImGui::GetCurrentContext()) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
        if (msg == WM_CHAR || msg == WM_KEYDOWN || msg == WM_KEYUP) return 0;
    }
    return CallWindowProcA(g_ov.old_proc, hwnd, msg, wparam, lparam);
}

void ensure_win32(HWND hwnd) {
    if (g_ov.win32 || !hwnd) return;
    if (!ImGui_ImplWin32_Init(hwnd)) {
        spdlog::error("overlay: ImGui_ImplWin32_Init failed");
        g_ov.win32 = true;  // do not retry every frame; the lobby stays mouse-less
        return;
    }
    g_ov.hwnd = hwnd;
    g_ov.old_proc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&lobby_wndproc)));
    g_ov.win32 = true;
    spdlog::info("overlay: lobby input on window {:#x} (previous proc {:#x})", reinterpret_cast<uintptr_t>(hwnd),
                 reinterpret_cast<uintptr_t>(g_ov.old_proc));
}

// fov at +0xD0 is degrees and horizontal: EngineDll7 UpdatePerspectiveProjectionFrustum
// multiplies it by pi/360 and the X screen scale is (width/2) / tan(half). Same scale on Y
// because SetAspect stores height/width, which cancels in the Y multiplier.
float marker_scale(float fov_deg, float width) {
    float tan_half = std::tan(fov_deg * 0.008726646f);  // pi/360
    if (tan_half < 0.01f) tan_half = 0.01f;
    return (width * 0.5f) / tan_half;
}

float dot3(const dh2::coop::Vec3& a, const dh2::coop::Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

ImU32 marker_color(int id, int alpha) {
    static const int rgb[4][3] = {{255, 200, 60}, {90, 170, 255}, {110, 220, 110}, {255, 110, 100}};
    const int* c = rgb[(id < 0 ? 0 : id) & 3];
    return IM_COL32(c[0], c[1], c[2], alpha);
}

// Alpha stays at or above 50%. Full at the camera, half by ~80 m.
int marker_alpha(float dist_m) {
    float t = 1.f - dist_m / 160.f;
    if (t < 0.5f) t = 0.5f;
    if (t > 1.f) t = 1.f;
    return static_cast<int>(t * 255.f);
}

void draw_diamond(ImDrawList* dl, ImVec2 c, ImU32 col, ImU32 outline) {
    const float r = 5.f;  // 10 px tip to tip
    const ImVec2 p[4] = {ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y)};
    dl->AddQuadFilled(p[0], p[1], p[2], p[3], col);
    dl->AddQuad(p[0], p[1], p[2], p[3], outline, 1.5f);
}

void draw_arrow(ImDrawList* dl, ImVec2 at, float dx, float dy, ImU32 col, ImU32 outline) {
    const float len = std::sqrt(dx * dx + dy * dy);
    float nx = 0.f, ny = 1.f;
    if (len > 0.001f) {
        nx = dx / len;
        ny = dy / len;
    }
    const float px = -ny, py = nx;
    const ImVec2 tip(at.x + nx * 7.f, at.y + ny * 7.f);
    const ImVec2 a(at.x - nx * 7.f + px * 5.f, at.y - ny * 7.f + py * 5.f);
    const ImVec2 b(at.x - nx * 7.f - px * 5.f, at.y - ny * 7.f - py * 5.f);
    dl->AddTriangleFilled(tip, a, b, col);
    dl->AddTriangle(tip, a, b, outline, 1.5f);
}

// Ray from the screen centre hits the inset rectangle. dir is screen space, +y down.
ImVec2 edge_point(ImVec2 disp, float dx, float dy) {
    const float inset = 24.f;
    const float hx = disp.x * 0.5f - inset;
    const float hy = disp.y * 0.5f - inset;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) {
        dx = 0.f;
        dy = 1.f;  // straight behind, no sideways offset: bottom edge
        len = 1.f;
    }
    dx /= len;
    dy /= len;
    float t = 1.0e9f;
    if (std::fabs(dx) > 1.0e-5f) {
        const float tx = (dx > 0.f ? hx : -hx) / dx;
        if (tx > 0.f) t = tx;
    }
    if (std::fabs(dy) > 1.0e-5f) {
        const float ty = (dy > 0.f ? hy : -hy) / dy;
        if (ty > 0.f && ty < t) t = ty;
    }
    return ImVec2(disp.x * 0.5f + dx * t, disp.y * 0.5f + dy * t);
}

void draw_one_marker(ImDrawList* dl, const dh2::coop::ViewCamera& cam, float scale, ImVec2 disp,
                     const dh2::coop::Vec3& world, const char* label, int metres, int alpha, ImU32 col) {
    const dh2::coop::Vec3 d{world.x - cam.pos.x, world.y - cam.pos.y, world.z - cam.pos.z};
    const float cx = dot3(d, cam.x);
    const float cy = dot3(d, cam.y);
    const float cz = dot3(d, cam.z);
    const bool behind = cz <= 0.1f;
    float sx = 0.f, sy = 0.f;
    bool on_screen = false;
    if (!behind) {
        sx = disp.x * 0.5f + (cx / cz) * scale;
        sy = disp.y * 0.5f - (cy / cz) * scale;
        on_screen = sx >= 0.f && sy >= 0.f && sx <= disp.x && sy <= disp.y;
    }
    const ImU32 outline = IM_COL32(20, 20, 20, alpha);
    char text[80];
    if (metres >= 0)
        std::snprintf(text, sizeof(text), "%s  %dm", label, metres);
    else
        std::snprintf(text, sizeof(text), "%s", label);
    const ImVec2 ts = ImGui::CalcTextSize(text);
    if (on_screen) {
        draw_diamond(dl, ImVec2(sx, sy), col, outline);
        dl->AddText(ImVec2(sx - ts.x * 0.5f, sy + 8.f), col, text);
        return;
    }
    // Behind: flip both axes so the arrow stays on the side the player actually is,
    // then screen Y flips again (-cy -> +cy). Straight behind and slightly above lands on the bottom.
    const float vx = behind ? -cx : cx;
    const float vy = behind ? -cy : cy;
    const float dirx = vx;
    const float diry = -vy;
    const ImVec2 at = edge_point(disp, dirx, diry);
    float ndx = dirx, ndy = diry;
    const float nlen = std::sqrt(ndx * ndx + ndy * ndy);
    if (nlen > 0.001f) {
        ndx /= nlen;
        ndy /= nlen;
    } else {
        ndx = 0.f;
        ndy = 1.f;
    }
    draw_arrow(dl, at, ndx, ndy, col, outline);
    // Name sits just inside the arrow so it stays on the back buffer.
    float tx = at.x - ndx * (ts.x * 0.5f + 12.f) - ts.x * 0.5f;
    float ty = at.y - ndy * 16.f - ts.y * 0.5f;
    if (tx < 4.f) tx = 4.f;
    if (ty < 4.f) ty = 4.f;
    if (tx + ts.x > disp.x - 4.f) tx = disp.x - 4.f - ts.x;
    if (ty + ts.y > disp.y - 4.f) ty = disp.y - 4.f - ts.y;
    dl->AddText(ImVec2(tx, ty), col, text);
}

void draw_markers(const dh2::coop::SessionView& view, ImVec2 disp) {
    const dh2::coop::ViewCamera cam = dh2::coop::LastViewCamera();
    if (!cam.valid || cam.proj != 0) return;
    const float scale = marker_scale(dh2::display::EffectiveHorizontalFov(cam.fov), disp.x);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const dh2::coop::Vec3 hunter{view.hunterPos[0], view.hunterPos[1], view.hunterPos[2]};
    for (int i = 0; i < view.remoteCount && i < 4; ++i) {
        const dh2::coop::RemoteView& r = view.remotes[i];
        if (r.packets == 0) continue;
        const dh2::coop::Vec3 feet{r.pos[0], r.pos[1], r.pos[2]};
        const dh2::coop::Vec3 head{feet.x, feet.y + 2.f, feet.z};
        float dist = 0.f;
        if (view.hunterFound) {
            const float dx = feet.x - hunter.x, dy = feet.y - hunter.y, dz = feet.z - hunter.z;
            dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        const int metres = static_cast<int>(dist + 0.5f);
        const int alpha = marker_alpha(dist);
        draw_one_marker(dl, cam, scale, disp, head, r.name.c_str(), metres, alpha, marker_color(r.id, alpha));
    }
    if (!g_ov.marker_test) return;
    const auto& animals = dh2::coop::Animals();
    for (int i = 0; i < static_cast<int>(animals.size()); ++i) {
        dh2::coop::Vec3 feet{};
        if (!dh2::core::safe_read(static_cast<char*>(animals[i]) + 0xE8, feet)) continue;
        // Parked animals sit at the sentinel (-10000,-10000,-10000). A marker there is not a deer.
        if (feet.x < -5000.f && feet.z < -5000.f) continue;
        const dh2::coop::Vec3 head{feet.x, feet.y + 2.f, feet.z};
        float dist = 0.f;
        if (view.hunterFound) {
            const float dx = feet.x - hunter.x, dy = feet.y - hunter.y, dz = feet.z - hunter.z;
            dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        char label[32];
        std::snprintf(label, sizeof(label), "animal %d", i);
        const int alpha = marker_alpha(dist);
        draw_one_marker(dl, cam, scale, disp, head, label, static_cast<int>(dist + 0.5f), alpha,
                        IM_COL32(180, 180, 180, alpha));
    }
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
    const bool f8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    const bool f7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
    if (!g_ov.keys_ready) {
        g_ov.f8_down = f8;
        g_ov.f7_down = f7;
        g_ov.keys_ready = true;
    } else if (is_foreground(hwnd)) {
        if (f8 && !g_ov.f8_down) {
            g_ov.visible = !g_ov.visible;
            spdlog::info("overlay: lobby {}", g_ov.visible ? "opened (F8)" : "closed (F8)");
        }
        if (f7 && !g_ov.f7_down) {
            g_ov.markers = !g_ov.markers;
            spdlog::info("overlay: markers={}", g_ov.markers ? 1 : 0);
        }
    }
    if (g_ov.keys_ready) {
        g_ov.f8_down = f8;
        g_ov.f7_down = f7;
    }
    // The lobby opens by itself when the main menu loads and closes when a map loads.
    const bool at_menu = dh2::coop::Lobby().atMenu;
    if (at_menu != g_ov.at_menu) {
        g_ov.at_menu = at_menu;
        const bool want = at_menu && g_ov.auto_open;
        if (want != g_ov.visible) {
            g_ov.visible = want;
            spdlog::info("overlay: lobby {} ({})", want ? "opened" : "closed", at_menu ? "main menu" : "map loaded");
        }
    }
    ensure_win32(hwnd);
    dh2::dinput::SetUiCapture(g_ov.visible);

    ImGuiIO& io = ImGui::GetIO();
    if (g_ov.hwnd) ImGui_ImplWin32_NewFrame();
    io.DisplaySize = display_size(dev, hwnd);
    io.DeltaTime = overlay_dt();
    if (g_ov.visible && g_ov.hwnd) {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        io.MouseDrawCursor = true;
        // The back buffer can differ from the client area (a resized window): put the cursor in back-buffer pixels.
        POINT pt{};
        RECT rc{};
        if (GetCursorPos(&pt) && ScreenToClient(hwnd, &pt) && GetClientRect(hwnd, &rc) && rc.right > 0 && rc.bottom > 0)
            io.AddMousePosEvent(static_cast<float>(pt.x) * io.DisplaySize.x / static_cast<float>(rc.right),
                                static_cast<float>(pt.y) * io.DisplaySize.y / static_cast<float>(rc.bottom));
    } else {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        io.MouseDrawCursor = false;
    }

    static bool logged_size = false;
    if (!logged_size) {
        logged_size = true;
        spdlog::info("overlay: display {:.0f}x{:.0f}", static_cast<double>(io.DisplaySize.x),
                     static_cast<double>(io.DisplaySize.y));
    }

    ImGui_ImplDX8_NewFrame();
    ImGui::NewFrame();
    const dh2::coop::SessionView view = dh2::coop::Snapshot();
    if (g_ov.visible) draw_lobby(view, io.DisplaySize);
    if (g_ov.markers) draw_markers(view, io.DisplaySize);
    if (!view.notice.empty() && !(g_ov.visible && view.noticeIsBanner)) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 40.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::Begin("##coop_notice", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted(view.notice.c_str());
        ImGui::End();
    }
    // Mission flow line for a client (waiting for the host's choice, following it): coop/flow.cpp.
    const std::string flow_line = dh2::coop::flow::Banner();
    if (!flow_line.empty() && !g_ov.visible) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 8.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.70f);
        ImGui::Begin("##coop_flow", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.35f, 1.0f), "%s", flow_line.c_str());
        ImGui::End();
    }
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
        g_ov.auto_open = GetPrivateProfileIntA("Overlay", "Enabled", 1, ini.c_str()) != 0;
        g_ov.visible = false;
        g_ov.markers = GetPrivateProfileIntA("Overlay", "Markers", 1, ini.c_str()) != 0;
        g_ov.marker_test = GetPrivateProfileIntA("Overlay", "MarkerTest", 0, ini.c_str()) != 0;
        spdlog::info("overlay: ini Enabled={} Markers={} MarkerTest={}", g_ov.visible ? 1 : 0, g_ov.markers ? 1 : 0,
                     g_ov.marker_test ? 1 : 0);
        dh2::d3d8::AddPresentCallback(&on_present);
        dh2::d3d8::AddResetCallback(&on_reset);
    }

    // OnShutdown runs on the ASI init thread, not the device thread. The present
    // callback AddRefs the device; process exit drops that ref. Do not call D3D here.
    void OnShutdown() override {}
};

DH2_MODULE(OverlayModule)

}  // namespace
