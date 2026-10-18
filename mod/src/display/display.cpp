// Modern resolution and a 4:3 HUD on wider screens.
//
// The back buffer size is the cRenderInterface cvars copied by CreateRender (EngineDll7 0xa3d40):
// width at +0x1fc, height at +0x200, windowed at +0x208 (RegisterConsole, re/notes/loop-decomp-conregs-EngineDll7.c).
// The world camera already stores aspect as height/width from the viewport (SetAspect at EngineDll7 0x357128)
// and a horizontal FOV at cRenderCamera+0xD0. Leaving that FOV alone on a wide buffer crops the vertical view.
// UpdatePerspectiveProjectionFrustum (0x2fdb0) is wrapped so the projection uses a wider horizontal angle
// that keeps the 4:3 vertical field; the stored FOV is put back before the call returns.
//
// HUD sprites live in a virtual 640x480. GuiSetDisplay2D (CommonDll7 0xe07b0) sets
// x scale = pixelWidth/640 (global RVA 0x1ac7dc) and y scale = pixelHeight/480 (0x1ac7e0).
// Those differ whenever the buffer is not 4:3, which stretches the HUD. After that call the scales
// are replaced with one uniform scale and the D3D viewport is moved to the centered 4:3 rectangle,
// so a virtual (320, 240) crosshair stays at the middle of the screen. GetMouseEvent (0xe6810)
// maps cursor pixels with the full buffer (pixel / width * 640); the same offset is applied there.
// The viewport is restored on Present, before the next frame's world pass.
#include "display/display.h"

#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include <spdlog/spdlog.h>

#include <intrin.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace {

constexpr float kVirtualW = 640.f;
constexpr float kVirtualH = 480.f;
constexpr float kFourThree = 4.f / 3.f;
// Preferred image base 0x10000000, so 0x101ac7d4 is RVA 0x1ac7d4 (not 0xac7d4).
constexpr uintptr_t kRvaPixelW = 0x1ac7d4;
constexpr uintptr_t kRvaPixelH = 0x1ac7d8;
constexpr uintptr_t kRvaScaleX = 0x1ac7dc;
constexpr uintptr_t kRvaScaleY = 0x1ac7e0;
constexpr uintptr_t kRvaMouseX = 0x1b35ac;
constexpr uintptr_t kRvaMouseY = 0x1b35b0;

bool g_enabled = true;
bool g_windowed = true;
bool g_hor_plus = true;
bool g_lock_hud = true;
bool g_compass_fix = true;  // [Display] CompassFix
const char* g_dpi_result = "not tried";  // MakeDpiAware (DllMain), logged by choose_resolution
int g_raw_hud = 0;          // > 0 while a HUD element that lays itself out in back-buffer pixels draws (scope)
int g_width = 0;
int g_height = 0;

float g_aspect = 0.75f;
float g_hud_scale = 1.f;
float g_hud_off_x = 0.f;
float g_hud_off_y = 0.f;
float g_view_h = 0.f;
float g_elem_scale = 0.5f;  // gameplay icons only; menus stay at the 4:3 fit
int g_elem_min_h = 1080;
bool g_hud_active = false;
bool g_hud_logged = false;
bool g_elem_logged = false;

safetyhook::InlineHook* g_create_render = nullptr;
safetyhook::InlineHook* g_frustum = nullptr;
safetyhook::InlineHook* g_gui = nullptr;
safetyhook::InlineHook* g_mouse = nullptr;
safetyhook::InlineHook* g_cursor = nullptr;
safetyhook::InlineHook* g_sprite = nullptr;
safetyhook::InlineHook* g_sprite_uv = nullptr;
safetyhook::InlineHook* g_sprite_angle = nullptr;
safetyhook::InlineHook* g_text = nullptr;
safetyhook::InlineHook* g_rsprite = nullptr;
safetyhook::InlineHook* g_rbar = nullptr;
safetyhook::InlineHook* g_rtri = nullptr;
safetyhook::InlineHook* g_rtri3 = nullptr;
safetyhook::InlineHook* g_rline = nullptr;
safetyhook::InlineHook* g_rline2 = nullptr;
safetyhook::InlineHook* g_print = nullptr;
safetyhook::InlineHook* g_char = nullptr;
bool g_hud_sprite_logged = false;

HMODULE g_common = nullptr;

float expand_fov(float stored, float aspect_hw) {
    if (!g_hor_plus || stored < 1.f || stored > 170.f) return stored;
    if (!(aspect_hw > 0.05f && aspect_hw < 0.745f)) return stored;
    const float factor = (1.f / aspect_hw) / kFourThree;
    if (factor < 1.001f) return stored;
    const float half = stored * 0.5f * (3.14159265f / 180.f);
    float widened = std::atan(std::tan(half) * factor) * 2.f * (180.f / 3.14159265f);
    if (widened < stored) widened = stored;
    if (widened > 150.f) widened = 150.f;
    return widened;
}

template <class T>
T* common_ptr(uintptr_t rva) {
    if (!g_common) return nullptr;
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(g_common) + rva);
}

bool read_common(uintptr_t rva, float& out) {
    float* p = common_ptr<float>(rva);
    return p && dh2::core::safe_read(p, out);
}

void write_common(uintptr_t rva, float v) {
    if (float* p = common_ptr<float>(rva)) *p = v;
}

// Pause and the main menu push a modal (manager+0x88, count at +0x108) or set +0x7c.
// The compass and ammo are the screen control under that, drawn with the same sprites.
bool menu_is_open() {
    void** slot = common_ptr<void*>(0x1b35b8);  // cGuiManager::pInstance
    void* mgr = nullptr;
    if (!slot || !dh2::core::safe_read(slot, mgr) || !mgr) return false;
    auto* b = static_cast<uint8_t*>(mgr);
    int kind = 0, modals = 0;
    dh2::core::safe_read(b + 0x7c, kind);
    dh2::core::safe_read(b + 0x108, modals);
    return kind == 1 || modals > 0;
}

float gameplay_icon_scale() {
    if (!(g_elem_scale > 0.f) || g_elem_scale == 1.f || g_view_h < static_cast<float>(g_elem_min_h)) return 1.f;
    if (menu_is_open()) return 1.f;
    if (!g_elem_logged) {
        g_elem_logged = true;
        spdlog::info("display: gameplay icons {:.0f}% (menus stay full size, height {:.0f})", g_elem_scale * 100.f,
                     g_view_h);
    }
    return g_elem_scale;
}

// DrawMouseCursor maps the pointer with pixel * 640/width across the whole buffer.
// Widgets live in the centered 4:3 block, so the graphic lags on X and crosses the highlight.
int* mouse_axis(int offset) {
    void** slot = common_ptr<void*>(0x11f878);
    void* obj = nullptr;
    if (!slot || !dh2::core::safe_read(slot, obj) || !obj) return nullptr;
    return reinterpret_cast<int*>(static_cast<uint8_t*>(obj) + offset);
}

// GameDll turns virtual HUD units into pixels with width*(1/640) and height*(1/480), so a
// circle becomes a wide oval on 21:9. Point both constants at one pixels-per-unit value.
void patch_layout_constants(float view_w, float view_h) {
    HMODULE game = GetModuleHandleA("GameDll7.dll");
    if (!game || view_w < 2.f || view_h < 2.f) return;
    const float fit = std::min(view_w / kVirtualW, view_h / kVirtualH);
    auto* base = reinterpret_cast<uint8_t*>(game);
    DWORD old = 0;
    if (!VirtualProtect(base + 0x111ff8, 16, PAGE_EXECUTE_READWRITE, &old)) return;
    *reinterpret_cast<float*>(base + 0x111ff8) = fit / view_w;
    *reinterpret_cast<float*>(base + 0x112000) = fit / view_h;
    DWORD ignored = 0;
    VirtualProtect(base + 0x111ff8, 16, old, &ignored);
}

void apply_hud_viewport(float view_w, float view_h) {
    const float sx = view_w / kVirtualW;
    const float sy = view_h / kVirtualH;
    const float scale = std::min(sx, sy);
    if (!(scale > 0.01f)) return;
    g_view_h = view_h;
    const float ui_w = kVirtualW * scale;
    const float ui_h = kVirtualH * scale;
    const float off_x = (view_w - ui_w) * 0.5f;
    const float off_y = (view_h - ui_h) * 0.5f;
    // Sprites are positioned in back-buffer pixels. A viewport inset does not move them;
    // it clips, and it squashes a circle into a tall oval. Centering is done on the coordinates.
    const bool inset = std::fabs(off_x) >= 1.f || std::fabs(off_y) >= 1.f;
    g_hud_scale = scale;
    g_hud_off_x = off_x;
    g_hud_off_y = off_y;
    g_hud_active = inset;
    patch_layout_constants(view_w, view_h);
    write_common(kRvaScaleX, scale);
    write_common(kRvaScaleY, scale);
    if (!g_hud_logged) {
        g_hud_logged = true;
        spdlog::info("display: hud {:.0f}x{:.0f} at ({:.0f},{:.0f}) inside {:.0f}x{:.0f}", ui_w, ui_h, off_x, off_y,
                     view_w, view_h);
    }
}

int __fastcall hk_CreateRender(void* self, void*, HWND hwnd) {
    auto* base = static_cast<uint8_t*>(self);
    int* width = reinterpret_cast<int*>(base + 0x1fc);
    int* height = reinterpret_cast<int*>(base + 0x200);
    int* windowed = reinterpret_cast<int*>(base + 0x208);
    int w = g_width;
    int h = g_height;
    if (w <= 0 || h <= 0) {
        RECT work{};
        if (g_windowed && SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0)) {
            w = work.right - work.left;
            h = work.bottom - work.top;
        } else {
            w = GetSystemMetrics(SM_CXSCREEN);
            h = GetSystemMetrics(SM_CYSCREEN);
        }
        if (g_windowed) {
            // SetWindowSize inflates the outer window by the caption. Keep the frame on the desktop.
            w -= 16;
            h -= 40;
        }
    }
    if (w < 640) w = 640;
    if (h < 480) h = 480;
    int prev_w = 0, prev_h = 0, prev_win = 0;
    dh2::core::safe_read(width, prev_w);
    dh2::core::safe_read(height, prev_h);
    dh2::core::safe_read(windowed, prev_win);
    *width = w;
    *height = h;
    *windowed = g_windowed ? 1 : 0;
    spdlog::info("display: CreateRender {}x{} windowed={} (was {}x{} windowed={})", w, h, g_windowed ? 1 : 0,
                 prev_w, prev_h, prev_win);
    return g_create_render->unsafe_thiscall<int>(self, hwnd);
}

void __fastcall hk_Frustum(void* cam, void*) {
    if (g_lock_hud && g_common) {
        void** slot = common_ptr<void*>(0x11f2d4);
        void* render = nullptr;
        if (slot && dh2::core::safe_read(slot, render) && render) {
            float left = 0, top = 0, right = 0, bottom = 0;
            auto* b = static_cast<uint8_t*>(render);
            if (dh2::core::safe_read(b + 0x1a8, left) && dh2::core::safe_read(b + 0x1b0, right) &&
                dh2::core::safe_read(b + 0x1ac, top) && dh2::core::safe_read(b + 0x1b4, bottom))
                patch_layout_constants(right - left, bottom - top);
        }
    }
    float fov = 0.f, aspect = 0.75f;
    auto* bytes = static_cast<uint8_t*>(cam);
    const bool have = dh2::core::safe_read(bytes + 0xD0, fov) && dh2::core::safe_read(bytes + 0xD4, aspect);
    if (have && aspect > 0.05f) g_aspect = aspect;
    const float use = have ? expand_fov(fov, aspect) : fov;
    if (have && use != fov) *reinterpret_cast<float*>(bytes + 0xD0) = use;
    g_frustum->unsafe_thiscall<void>(cam);
    if (have && use != fov) *reinterpret_cast<float*>(bytes + 0xD0) = fov;
}

void __cdecl hk_GuiSetDisplay2D() {
    if (g_lock_hud && g_common) {
        void** slot = common_ptr<void*>(0x11f2d4);
        void* render = nullptr;
        if (slot && dh2::core::safe_read(slot, render) && render) {
            float left = 0, top = 0, right = 0, bottom = 0;
            auto* b = static_cast<uint8_t*>(render);
            if (dh2::core::safe_read(b + 0x1a8, left) && dh2::core::safe_read(b + 0x1b0, right) &&
                dh2::core::safe_read(b + 0x1ac, top) && dh2::core::safe_read(b + 0x1b4, bottom))
                patch_layout_constants(right - left, bottom - top);
        }
    }
    g_gui->unsafe_ccall<void>();
    if (!g_lock_hud) return;
    float view_w = 0.f, view_h = 0.f;
    if (!read_common(kRvaPixelW, view_w) || !read_common(kRvaPixelH, view_h)) return;
    if (view_w < 2.f || view_h < 2.f) return;
    apply_hud_viewport(view_w, view_h);
}

void __cdecl hk_GetMouseEvent(int* a, int* b, int* x, int* y, int* e) {
    int before_x = 0, before_y = 0;
    if (int* px = common_ptr<int>(kRvaMouseX)) dh2::core::safe_read(px, before_x);
    if (int* py = common_ptr<int>(kRvaMouseY)) dh2::core::safe_read(py, before_y);
    g_mouse->unsafe_ccall<void>(a, b, x, y, e);
    if (!g_lock_hud || !g_hud_active || !(g_hud_scale > 0.01f) || !x || !y) return;
    int after_x = before_x, after_y = before_y;
    if (int* px = common_ptr<int>(kRvaMouseX)) dh2::core::safe_read(px, after_x);
    if (int* py = common_ptr<int>(kRvaMouseY)) dh2::core::safe_read(py, after_y);
    if (after_x == before_x && after_y == before_y) return;
    *x = static_cast<int>(std::lround((static_cast<float>(after_x) - g_hud_off_x) / g_hud_scale));
    *y = static_cast<int>(std::lround((static_cast<float>(after_y) - g_hud_off_y) / g_hud_scale));
}

void __fastcall hk_DrawMouseCursor(void* self, void*) {
    int* mx = mouse_axis(0x0c);
    int* my = mouse_axis(0x10);
    int saved_x = 0, saved_y = 0;
    bool spoofed = false;
    float pw = 0.f, ph = 0.f;
    if (mx && my && g_hud_active && g_hud_scale > 0.01f && read_common(kRvaPixelW, pw) && read_common(kRvaPixelH, ph) &&
        pw > 2.f && ph > 2.f && dh2::core::safe_read(mx, saved_x) && dh2::core::safe_read(my, saved_y)) {
        // Feed the cursor the pixel that its full-buffer formula turns into (pixel - inset) / scale.
        *mx = static_cast<int>(std::lround((static_cast<float>(saved_x) - g_hud_off_x) / g_hud_scale * pw / kVirtualW));
        *my = static_cast<int>(std::lround((static_cast<float>(saved_y) - g_hud_off_y) / g_hud_scale * ph / kVirtualH));
        spoofed = true;
    }
    g_cursor->unsafe_thiscall<void>(self);
    if (spoofed) {
        *mx = saved_x;
        *my = saved_y;
    }
}

void scale_rect(float* r, float s) {
    const float cx = (r[0] + r[2]) * 0.5f;
    const float cy = (r[1] + r[3]) * 0.5f;
    r[0] = cx + (r[0] - cx) * s;
    r[2] = cx + (r[2] - cx) * s;
    r[1] = cy + (r[1] - cy) * s;
    r[3] = cy + (r[3] - cy) * s;
}

// Gameplay icons share the menu's virtual 640x480, so at 1080p/1440p they fill too much of the screen.
// Shrink each sprite around its own center. Menus (a modal is up) keep the full 4:3 size.
template <class Draw>
void draw_scaled(float* dest, Draw&& draw) {
    if (!dest || !g_lock_hud || !(g_hud_scale > 0.01f) || g_view_h < 2.f) {
        draw();
        return;
    }
    const float saved[4] = {dest[0], dest[1], dest[2], dest[3]};
    // Gui text stays in 640x480 and the engine multiplies by fit afterwards. Do the same
    // centering and half-size scale the weapon sprites get, then hand virtual units back.
    const float fit = g_hud_scale;
    const float view_w = g_hud_off_x * 2.f + kVirtualW * fit;
    const float icon = gameplay_icon_scale();
    const float top = g_hud_off_y - (g_view_h - kVirtualH * fit) * 0.5f;
    const float mid_y = top + g_view_h * 0.5f;
    const float bottom = top + g_view_h;
    auto place = [&](float v, bool vertical) {
        float s = v * fit + (vertical ? g_hud_off_y : g_hud_off_x);
        if (icon == 1.f) return s / fit;
        if (!vertical) return (view_w * 0.5f + (s - view_w * 0.5f) * icon) / fit;
        const float anchor = (v > kVirtualH * 0.68f) ? bottom : mid_y;
        return (anchor + (s - anchor) * icon) / fit;
    };
    dest[0] = place(saved[0], false);
    dest[2] = place(saved[2], false);
    dest[1] = place(saved[1], true);
    dest[3] = place(saved[3], true);
    draw();
    dest[0] = saved[0];
    dest[1] = saved[1];
    dest[2] = saved[2];
    dest[3] = saved[3];
}

void __cdecl hk_GuiDrawSprite(float* dest, float* uv, int tex, unsigned color) {
    draw_scaled(dest, [&] { g_sprite->unsafe_ccall<void>(dest, uv, tex, color); });
}

void __cdecl hk_GuiDrawSpriteUv(float* dest, float* uv, int tex, void* pixel_rect) {
    draw_scaled(dest, [&] { g_sprite_uv->unsafe_ccall<void>(dest, uv, tex, pixel_rect); });
}

void __cdecl hk_GuiDrawSpriteAngle(float* dest, float* uv, int tex, unsigned color, float angle) {
    draw_scaled(dest, [&] { g_sprite_angle->unsafe_ccall<void>(dest, uv, tex, color, angle); });
}

void __cdecl hk_GuiDrawText(const char* text, float* dest) {
    draw_scaled(dest, [&] { g_text->unsafe_ccall<void>(text, dest); });
}

// Compass, ammo and the aim reticle are cRender::DrawSprite / DrawBar / DrawTriangle2D from GameDll,
// not GuiDrawSprite. Callers either pass 640x480 virtual rects or pixels already stretched with
// width/640 and height/480. Both become a centered 4:3 layout; icons under ~500x300 virtual shrink.
// cFont::Char draws the ammo digits with DrawSprite. The caller is EngineDll, so the
// GameDll/CommonDll check would leave the "10" at full size beside the weapon box.
bool caller_is_font_glyph(void* ret) {
    HMODULE engine = GetModuleHandleA("EngineDll7.dll");
    HMODULE owner = nullptr;
    if (!engine || !ret) return false;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(ret), &owner) ||
        owner != engine)
        return false;
    const auto rva = static_cast<uintptr_t>(static_cast<uint8_t*>(ret) - reinterpret_cast<uint8_t*>(engine));
    return (rva >= 0x11680 && rva < 0x11c00) || (rva >= 0xe3c0 && rva < 0xe800);
}

bool caller_is_game(void* ret) {
    HMODULE game = GetModuleHandleA("GameDll7.dll");
    HMODULE owner = nullptr;
    if (!game || !ret) return false;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(ret), &owner))
        return false;
    return owner == game;
}

bool caller_is_hud(void* ret) {
    HMODULE owner = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(ret), &owner))
        return false;
    // GameDll draws the compass, reticle and ammo. CommonDll draws the compass arc and bars
    // through the same 2D calls, after multiplying by the GUI scale.
    return owner == GetModuleHandleA("GameDll7.dll") || owner == GetModuleHandleA("CommonDll7.dll");
}

struct HudPlace {
    float w, h, fit, off_x, off_y, icon;
    bool ok;
};

HudPlace hud_place(void* render) {
    HudPlace p{};
    float left = 0, top = 0, right = 0, bottom = 0;
    auto* b = static_cast<uint8_t*>(render);
    if (!dh2::core::safe_read(b + 0x1a8, left) || !dh2::core::safe_read(b + 0x1ac, top) ||
        !dh2::core::safe_read(b + 0x1b0, right) || !dh2::core::safe_read(b + 0x1b4, bottom))
        return p;
    p.w = right - left;
    p.h = bottom - top;
    if (p.w < 2.f || p.h < 2.f) return p;
    p.fit = std::min(p.w / kVirtualW, p.h / kVirtualH);
    p.off_x = (p.w - kVirtualW * p.fit) * 0.5f + left;
    p.off_y = (p.h - kVirtualH * p.fit) * 0.5f + top;
    g_view_h = p.h;
    p.icon = gameplay_icon_scale();
    p.ok = g_lock_hud && g_raw_hud == 0;
    return p;
}

void* current_render() {
    void** slot = common_ptr<void*>(0x11f2d4);
    void* render = nullptr;
    if (!slot || !dh2::core::safe_read(slot, render)) return nullptr;
    return render;
}

bool rect_is_virtual(const float* r) {
    for (int i = 0; i < 4; ++i)
        if (r[i] < -2.f || r[i] > 648.f) return false;
    return true;
}

float y_anchor(const HudPlace& p, float y_after_offset);
float place_x(const HudPlace& p, float x);
float place_y(const HudPlace& p, float y, float anchor_y);

void remap_hud_rect(void* render, float* r) {
    if (!r) return;
    const HudPlace p = hud_place(render);
    if (!p.ok) return;
    float x0 = r[0], y0 = r[1], x1 = r[2], y1 = r[3];
    // Design-time rects are still 0..640. Everything else was already multiplied by the
    // patched constant and is in uniform left-aligned pixels.
    if (rect_is_virtual(r)) {
        x0 *= p.fit;
        y0 *= p.fit;
        x1 *= p.fit;
        y1 *= p.fit;
    }
    const float pw = std::fabs(x1 - x0);
    const float ph = std::fabs(y1 - y0);
    if (pw > p.w * 0.7f && ph > p.h * 0.5f) return;
    const float anchor_y = y_anchor(p, (y0 + y1) * 0.5f + p.off_y);
    r[0] = place_x(p, x0);
    r[2] = place_x(p, x1);
    r[1] = place_y(p, y0, anchor_y);
    r[3] = place_y(p, y1, anchor_y);
    if (!g_hud_sprite_logged) {
        g_hud_sprite_logged = true;
        spdlog::info("display: hud sprites use one centered layout, icon scale {:.0f}%", p.icon * 100.f);
    }
}

void __fastcall hk_RenderSprite(void* self, void*, float* dest, float* uv, int tex, unsigned color) {
    float local[4] = {};
    float* use = dest;
    void* ret = _ReturnAddress();
    // CommonDll sprites are GUI; draw_scaled already placed those. Engine font glyphs are the ammo count.
    if ((caller_is_game(ret) || caller_is_font_glyph(ret)) && dest && dh2::core::safe_copy(dest, local, sizeof local)) {
        remap_hud_rect(self, local);
        use = local;
    }
    g_rsprite->unsafe_thiscall<void>(self, use, uv, tex, color);
}

void __fastcall hk_RenderBar(void* self, void*, float* dest, unsigned color) {
    float local[4] = {};
    float* use = dest;
    if (caller_is_hud(_ReturnAddress()) && dest && dh2::core::safe_copy(dest, local, sizeof local)) {
        remap_hud_rect(self, local);
        use = local;
    }
    g_rbar->unsafe_thiscall<void>(self, use, color);
}

// Half-size icons shrink toward the screen center, which lifts the compass and the weapon
// into the middle of the view. Widgets that already sit in the lower part of the frame
// shrink toward the bottom edge instead, so the group stays low and each shape keeps one anchor.
float y_anchor(const HudPlace& p, float y_after_offset) {
    const float ui_h = kVirtualH * p.fit;
    const float top = p.off_y - (p.h - ui_h) * 0.5f;
    const float mid = top + p.h * 0.5f;
    const float bottom = top + p.h;
    return (y_after_offset > top + p.h * 0.68f) ? bottom : mid;
}

float place_x(const HudPlace& p, float x) {
    x += p.off_x;
    if (p.icon != 1.f) x = p.w * 0.5f + (x - p.w * 0.5f) * p.icon;
    return x;
}

float place_y(const HudPlace& p, float y, float anchor_y) {
    y += p.off_y;
    if (p.icon != 1.f) y = anchor_y + (y - anchor_y) * p.icon;
    return y;
}

// One transform for every point of a widget. Virtual 640x480 values are scaled by fit first;
// values already in pixels (the patched width/640 constant) are not. Mixing the two inside one
// shape is what pulled the compass arc off its ring.
void map_points(const HudPlace& p, float* pts, int n) {
    bool virt = true;
    for (int i = 0; i < n; ++i) {
        const float x = pts[i * 2];
        const float y = pts[i * 2 + 1];
        if (x < -2.f || x > 648.f || y < -2.f || y > 488.f) virt = false;
    }
    float y_sum = 0.f;
    for (int i = 0; i < n; ++i) {
        if (virt) {
            pts[i * 2] *= p.fit;
            pts[i * 2 + 1] *= p.fit;
        }
        y_sum += pts[i * 2 + 1] + p.off_y;
    }
    const float anchor_y = y_anchor(p, y_sum / static_cast<float>(n));
    for (int i = 0; i < n; ++i) {
        pts[i * 2] = place_x(p, pts[i * 2]);
        pts[i * 2 + 1] = place_y(p, pts[i * 2 + 1], anchor_y);
    }
}

// The N/E/S/W dial turns in GameDll 0xc6be0 (stdcall(rect, uv, tex, color, unused, centered, virtual)). It takes the
// quad's corners from pixels into camera space (cRender::Transform2dTo3dCameraSpace), rotates them there, and
// projects them back (Transform3dTo2dCameraSpace). Those use tan(fovX/2) at cRender+0x4a4 and tan(fovY/2) at +0x4a8
// (ortho extents +0x4ac / +0x4b0), so the round trip is a true rotation only when tanX/tanY equals width/height.
// The camera keeps the 4:3 values on a wider buffer: the dial was round with North forward and a horizontal oval
// (width/height / (4/3) wide) with East forward. During that one call the vertical value is set to match the
// buffer's shape, then restored. Nothing else in the call reads it.
// DrawTriangle2D itself also goes through those tangents (2D vertices are placed in camera space and drawn with the
// real projection), so hk_RenderTri puts the real values back around each triangle the dial draws.
safetyhook::InlineHook* g_rot_sprite = nullptr;
// The rifle scope is cHUDZoom::Draw (GameDll 0xdb810, fastcall(this)). It already lays itself out for any buffer
// from the cRender viewport (+0x1a8..+0x1b4): black bars of (width - height) / 2 on both sides and the lens as four
// quarter sprites in the centered height x height square (the first through cRender::DrawSprite, the other three
// through the HUD drawer slots +0x18 / +0x1c / +0x20). Remapping those pieces like 640x480 icons shrank each quarter
// toward its own anchor and split the scope in two (user screenshot 2026-10-18). Nothing is remapped while it runs.
safetyhook::InlineHook* g_zoom_draw = nullptr;

void __fastcall hk_ZoomDraw(void* self, void*) {
    ++g_raw_hud;
    g_zoom_draw->unsafe_fastcall<void>(self);
    --g_raw_hud;
}

void** g_engine_render = nullptr;  // EngineDll7 ?render@@3PAVcRender@@A, the cRender GameDll draws through
struct DialTangents {
    uint8_t* render = nullptr;  // non-null while the dial call runs with patched tangents
    float real_y = 0.f, real_oy = 0.f, dial_y = 0.f, dial_oy = 0.f;
} g_dial;

void __stdcall hk_RotatedSprite(float* rect, void* uv, int tex, unsigned color, int unused, int centered, int virt) {
    void* render_obj = nullptr;
    if (g_engine_render) dh2::core::safe_read(g_engine_render, render_obj);
    auto* render = static_cast<uint8_t*>(render_obj);
    float l = 0, t = 0, r = 0, b = 0, tx = 0, ty = 0, ox = 0, oy = 0;
    const bool fix = g_compass_fix && g_lock_hud && render && dh2::core::safe_read(render + 0x1a8, l) &&
                     dh2::core::safe_read(render + 0x1ac, t) && dh2::core::safe_read(render + 0x1b0, r) &&
                     dh2::core::safe_read(render + 0x1b4, b) && dh2::core::safe_read(render + 0x4a4, tx) &&
                     dh2::core::safe_read(render + 0x4a8, ty) && dh2::core::safe_read(render + 0x4ac, ox) &&
                     dh2::core::safe_read(render + 0x4b0, oy) && r - l > 2.f && b - t > 2.f;
    if (fix) {
        const float hw = (b - t) / (r - l);
        g_dial = {render, ty, oy, tx * hw, ox * hw};
        *reinterpret_cast<float*>(render + 0x4a8) = g_dial.dial_y;
        *reinterpret_cast<float*>(render + 0x4b0) = g_dial.dial_oy;
        static bool logged = false;
        if (!logged) {
            logged = true;
            spdlog::info("display: compass dial turns in screen space (tanY {:.3f} -> {:.3f} for {:.0f}x{:.0f})", ty,
                         tx * hw, r - l, b - t);
        }
    }
    g_rot_sprite->unsafe_stdcall<void>(rect, uv, tex, color, unused, centered, virt);
    if (fix) {
        g_dial.render = nullptr;
        *reinterpret_cast<float*>(render + 0x4a8) = ty;
        *reinterpret_cast<float*>(render + 0x4b0) = oy;
    }
}

// Around a triangle the dial draws: the real tangents for DrawTriangle2D, then the dial's again.
struct RealTangentsScope {
    uint8_t* render = g_dial.render;
    RealTangentsScope() {
        if (!render) return;
        *reinterpret_cast<float*>(render + 0x4a8) = g_dial.real_y;
        *reinterpret_cast<float*>(render + 0x4b0) = g_dial.real_oy;
    }
    ~RealTangentsScope() {
        if (!render) return;
        *reinterpret_cast<float*>(render + 0x4a8) = g_dial.dial_y;
        *reinterpret_cast<float*>(render + 0x4b0) = g_dial.dial_oy;
    }
};

void __fastcall hk_RenderTri(void* self, void*, float* a, float* b, float* c, float* u0, float* u1, float* u2, int mode,
                            unsigned color) {
    float pa[2], pb[2], pc[2];
    float *ua = a, *ub = b, *uc = c;
    if (a && b && c && caller_is_hud(_ReturnAddress()) && dh2::core::safe_copy(a, pa, sizeof pa) && dh2::core::safe_copy(b, pb, sizeof pb) &&
        dh2::core::safe_copy(c, pc, sizeof pc)) {
        const HudPlace p = hud_place(self);
        if (p.ok) {
            float pts[6] = {pa[0], pa[1], pb[0], pb[1], pc[0], pc[1]};
            map_points(p, pts, 3);
            pa[0] = pts[0];
            pa[1] = pts[1];
            pb[0] = pts[2];
            pb[1] = pts[3];
            pc[0] = pts[4];
            pc[1] = pts[5];
            ua = pa;
            ub = pb;
            uc = pc;
            if (!g_hud_sprite_logged) {
                g_hud_sprite_logged = true;
                spdlog::info("display: compass/reticle sprites centered, icon scale {:.0f}%", p.icon * 100.f);
            }
        }
    }
    RealTangentsScope real;
    g_rtri->unsafe_thiscall<void>(self, ua, ub, uc, u0, u1, u2, mode, color);
}

void map_line(void* self, void* ret, const float* a, const float* b, float* pa, float* pb, const float*& ua, const float*& ub) {
    if (!a || !b || !caller_is_hud(ret)) return;
    if (!dh2::core::safe_copy(a, pa, sizeof(float) * 2) || !dh2::core::safe_copy(b, pb, sizeof(float) * 2)) return;
    const HudPlace p = hud_place(self);
    if (!p.ok) return;
    float pts[4] = {pa[0], pa[1], pb[0], pb[1]};
    map_points(p, pts, 2);
    pa[0] = pts[0];
    pa[1] = pts[1];
    pb[0] = pts[2];
    pb[1] = pts[3];
    ua = pa;
    ub = pb;
}

void __fastcall hk_RenderLine(void* self, void*, const float* a, const float* b, unsigned color) {
    float pa[2], pb[2];
    const float *ua = a, *ub = b;
    map_line(self, _ReturnAddress(), a, b, pa, pb, ua, ub);
    g_rline->unsafe_thiscall<void>(self, ua, ub, color);
}

void __fastcall hk_RenderLine2(void* self, void*, const float* a, const float* b, unsigned c0, unsigned c1) {
    float pa[2], pb[2];
    const float *ua = a, *ub = b;
    map_line(self, _ReturnAddress(), a, b, pa, pb, ua, ub);
    g_rline2->unsafe_thiscall<void>(self, ua, ub, c0, c1);
}

void __fastcall hk_RenderTri3(void* self, void*, const float* a, const float* b, const float* c, unsigned color) {
    float pa[2], pb[2], pc[2];
    const float *ua = a, *ub = b, *uc = c;
    if (a && b && c && caller_is_hud(_ReturnAddress()) && dh2::core::safe_copy(a, pa, sizeof pa) &&
        dh2::core::safe_copy(b, pb, sizeof pb) && dh2::core::safe_copy(c, pc, sizeof pc)) {
        const HudPlace p = hud_place(self);
        if (p.ok) {
            float pts[6] = {pa[0], pa[1], pb[0], pb[1], pc[0], pc[1]};
            map_points(p, pts, 3);
            pa[0] = pts[0];
            pa[1] = pts[1];
            pb[0] = pts[2];
            pb[1] = pts[3];
            pc[0] = pts[4];
            pc[1] = pts[5];
            ua = pa;
            ub = pb;
            uc = pc;
        }
    }
    g_rtri3->unsafe_thiscall<void>(self, ua, ub, uc, color);
}

void place_text(void* ret, float& x, float& y) {
    if (!caller_is_hud(ret)) return;
    void* render = current_render();
    if (!render) return;
    const HudPlace p = hud_place(render);
    if (!p.ok) return;
    float pts[2] = {x, y};
    if (!(pts[0] > -4000.f && pts[0] < 8000.f && pts[1] > -4000.f && pts[1] < 8000.f)) return;
    map_points(p, pts, 1);
    x = pts[0];
    y = pts[1];
}

// The ammo count is cFont::Print (GameDll 0xdc7eb), already in the same pixel space as the weapon box.
void __fastcall hk_FontPrint(void* self, void*, float x, float y, const char* text) {
    place_text(_ReturnAddress(), x, y);
    g_print->unsafe_thiscall<void>(self, x, y, text);
}

// cFont::Printf is thiscall varargs, so a detour would drop the extra arguments. x,y sit on the stack.
void on_font_printf(safetyhook::Context& ctx) {
    if (!g_lock_hud || !ctx.esp) return;
    auto* ret = *reinterpret_cast<void**>(ctx.esp);
    auto* xy = reinterpret_cast<float*>(ctx.esp + 8);
    place_text(ret, xy[0], xy[1]);
}

void choose_resolution() {
    const std::string ini = (dh2::core::self_dir() / "dh2coop.ini").string();
    g_enabled = GetPrivateProfileIntA("Display", "Enabled", 1, ini.c_str()) != 0;
    g_windowed = GetPrivateProfileIntA("Display", "Windowed", 1, ini.c_str()) != 0;
    g_hor_plus = GetPrivateProfileIntA("Display", "HorPlus", 1, ini.c_str()) != 0;
    g_lock_hud = GetPrivateProfileIntA("Display", "LockHud", 1, ini.c_str()) != 0;
    g_compass_fix = GetPrivateProfileIntA("Display", "CompassFix", 1, ini.c_str()) != 0;
    g_width = static_cast<int>(GetPrivateProfileIntA("Display", "Width", 0, ini.c_str()));
    g_height = static_cast<int>(GetPrivateProfileIntA("Display", "Height", 0, ini.c_str()));
    g_elem_min_h = static_cast<int>(GetPrivateProfileIntA("Display", "HudScaleMinHeight", 1080, ini.c_str()));
    const int pct = static_cast<int>(GetPrivateProfileIntA("Display", "HudScalePercent", 50, ini.c_str()));
    g_elem_scale = static_cast<float>(pct) / 100.f;
    if (g_elem_scale < 0.05f) g_elem_scale = 0.05f;
    if (g_elem_scale > 1.f) g_elem_scale = 1.f;
    spdlog::info("display: Enabled={} {}x{} Windowed={} HorPlus={} LockHud={} HudScale={}% from height {}",
                 g_enabled ? 1 : 0, g_width, g_height, g_windowed ? 1 : 0, g_hor_plus ? 1 : 0, g_lock_hud ? 1 : 0,
                 pct, g_elem_min_h);
    RECT work{};
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0);
    spdlog::info("display: DPI {}; screen {}x{}, work area {}x{}", g_dpi_result, GetSystemMetrics(SM_CXSCREEN),
                 GetSystemMetrics(SM_CYSCREEN), work.right - work.left, work.bottom - work.top);
}

class DisplayModule : public dh2::Module {
public:
    const char* Name() const override { return "display"; }
    int Priority() const override { return 40; }

    void OnInit() override {
        choose_resolution();
    }

    void OnGameDllLoaded(HMODULE game) override {
        if (!g_enabled || !game) return;
        static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
        auto checked = [&](uintptr_t rva, const char* what) -> void* {
            void* fn = reinterpret_cast<uint8_t*>(game) + rva;
            uint8_t got[5] = {};
            if (!dh2::core::safe_copy(fn, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
                spdlog::error("display: {} at GameDll7+{:#x} has unexpected bytes; not hooked", what, rva);
                return nullptr;
            }
            return fn;
        };
        if (void* fn = checked(0xc6be0, "compass dial"))
            g_rot_sprite = dh2::core::install_hook("HUD rotated sprite (compass dial, GameDll 0xc6be0)", fn,
                                                   reinterpret_cast<void*>(&hk_RotatedSprite));
        if (g_lock_hud) {
            if (void* fn = checked(0xdb810, "cHUDZoom::Draw"))
                g_zoom_draw = dh2::core::install_hook("cHUDZoom::Draw (scope, GameDll 0xdb810)", fn,
                                                      reinterpret_cast<void*>(&hk_ZoomDraw));
        }
    }

    void OnEngineReady(HMODULE engine) override {
        if (!g_enabled || !engine) return;
        g_engine_render = dh2::core::find_export_as<void**>(engine, "?render@@3PAVcRender@@A");
        g_create_render = dh2::core::install_hook(
            "cRenderInterface::CreateRender",
            dh2::core::find_export(engine, "?CreateRender@cRenderInterface@@QAEHPAUHWND__@@@Z"),
            reinterpret_cast<void*>(&hk_CreateRender));
        g_frustum = dh2::core::install_hook(
            "cRenderCamera::UpdatePerspectiveProjectionFrustum",
            dh2::core::find_export(engine, "?UpdatePerspectiveProjectionFrustum@cRenderCamera@@IAEXXZ"),
            reinterpret_cast<void*>(&hk_Frustum));
        g_rsprite = dh2::core::install_hook(
            "cRender::DrawSprite",
            dh2::core::find_export(engine, "?DrawSprite@cRender@@QAEXABU?$tRect@M@@0HI@Z"),
            reinterpret_cast<void*>(&hk_RenderSprite));
        g_rbar = dh2::core::install_hook("cRender::DrawBar",
                                        dh2::core::find_export(engine, "?DrawBar@cRender@@QAEXAAU?$tRect@M@@I@Z"),
                                        reinterpret_cast<void*>(&hk_RenderBar));
        g_rtri = dh2::core::install_hook(
            "cRender::DrawTriangle2D",
            dh2::core::find_export(engine, "?DrawTriangle2D@cRender@@QAEXABUtVect2@@00000HI@Z"),
            reinterpret_cast<void*>(&hk_RenderTri));
        g_rtri3 = dh2::core::install_hook(
            "cRender::DrawTriangle2D(3)",
            dh2::core::find_export(engine, "?DrawTriangle2D@cRender@@QAEXABUtVect2@@00I@Z"),
            reinterpret_cast<void*>(&hk_RenderTri3));
        g_rline = dh2::core::install_hook(
            "cRender::DrawLine2D",
            dh2::core::find_export(engine, "?DrawLine2D@cRender@@QAEXABUtVect2@@0I@Z"),
            reinterpret_cast<void*>(&hk_RenderLine));
        g_rline2 = dh2::core::install_hook(
            "cRender::DrawLine2D(2)",
            dh2::core::find_export(engine, "?DrawLine2D@cRender@@QAEXABUtVect2@@0II@Z"),
            reinterpret_cast<void*>(&hk_RenderLine2));
        g_print = dh2::core::install_hook("cFont::Print",
                                         dh2::core::find_export(engine, "?Print@cFont@@QAEXMMPAD@Z"),
                                         reinterpret_cast<void*>(&hk_FontPrint));
        if (void* printf_fn = dh2::core::find_export(engine, "?Printf@cFont@@QAAXMMPADZZ")) {
            auto mid = safetyhook::MidHook::create(printf_fn, &on_font_printf);
            if (mid) {
                new safetyhook::MidHook(std::move(*mid));
                spdlog::info("hook cFont::Printf: installed");
            } else {
                spdlog::error("hook cFont::Printf: create failed");
            }
        }
        g_common = dh2::core::wait_for_module("CommonDll7.dll", 60000, 50);
        if (!g_common) {
            spdlog::error("display: CommonDll7.dll did not load; HUD lock not installed");
            return;
        }
        g_gui = dh2::core::install_hook("GuiSetDisplay2D",
                                       dh2::core::find_export(g_common, "?GuiSetDisplay2D@@YAXXZ"),
                                       reinterpret_cast<void*>(&hk_GuiSetDisplay2D));
        g_mouse = dh2::core::install_hook("cGuiEvent::GetMouseEvent",
                                         dh2::core::find_export(g_common, "?GetMouseEvent@cGuiEvent@@SAXAAH0000@Z"),
                                         reinterpret_cast<void*>(&hk_GetMouseEvent));
        g_cursor = dh2::core::install_hook(
            "cGuiManager::DrawMouseCursor",
            dh2::core::find_export(g_common, "?DrawMouseCursor@cGuiManager@@AAEXXZ"),
            reinterpret_cast<void*>(&hk_DrawMouseCursor));
        g_sprite = dh2::core::install_hook("GuiDrawSprite",
                                          dh2::core::find_export(g_common, "?GuiDrawSprite@@YAXABU?$tRect@M@@0HI@Z"),
                                          reinterpret_cast<void*>(&hk_GuiDrawSprite));
        g_sprite_uv = dh2::core::install_hook(
            "GuiDrawSprite(uv)",
            dh2::core::find_export(g_common, "?GuiDrawSprite@@YAXABU?$tRect@M@@0HABU?$tRect@H@@@Z"),
            reinterpret_cast<void*>(&hk_GuiDrawSpriteUv));
        g_sprite_angle = dh2::core::install_hook(
            "GuiDrawSpriteAngle",
            dh2::core::find_export(g_common, "?GuiDrawSpriteAngle@@YAXABU?$tRect@M@@0HIM@Z"),
            reinterpret_cast<void*>(&hk_GuiDrawSpriteAngle));
        g_text = dh2::core::install_hook("GuiDrawText",
                                        dh2::core::find_export(g_common, "?GuiDrawText@@YAXPBDABU?$tRect@M@@@Z"),
                                        reinterpret_cast<void*>(&hk_GuiDrawText));
    }
};

DH2_MODULE(DisplayModule)

}  // namespace

namespace dh2::display {

float EffectiveHorizontalFov(float stored_degrees) { return expand_fov(stored_degrees, g_aspect); }

void MakeDpiAware() {
    const std::string ini = (dh2::core::self_dir() / "dh2coop.ini").string();
    if (GetPrivateProfileIntA("Display", "DpiAware", 1, ini.c_str()) == 0) {
        g_dpi_result = "off ([Display] DpiAware=0)";
        return;
    }
    // System aware, not per-monitor: the game does not handle WM_DPICHANGED.
    HMODULE user32 = GetModuleHandleA("user32.dll");
    using SetCtxFn = BOOL(WINAPI*)(HANDLE);
    using SetAwareFn = BOOL(WINAPI*)();
    auto set_ctx = user32 ? reinterpret_cast<SetCtxFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")) : nullptr;
    auto set_aware = user32 ? reinterpret_cast<SetAwareFn>(GetProcAddress(user32, "SetProcessDPIAware")) : nullptr;
    if (set_ctx && set_ctx(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-2)))) {  // DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
        g_dpi_result = "system aware (SetProcessDpiAwarenessContext)";
    } else if (set_aware && set_aware()) {
        g_dpi_result = "system aware (SetProcessDPIAware)";
    } else {
        g_dpi_result = "not changed (already set, or the call failed)";
    }
}

}  // namespace dh2::display
