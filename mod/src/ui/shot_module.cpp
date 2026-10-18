// Test screenshots from inside the game: console `coop_shot <name>` saves the next frame's back buffer (overlay
// included) as Bin\shots\<name>.bmp. A screen capture of the window shows whatever other window is in front of it;
// with several test sessions on one desktop that was often another game.
#include "core/log.h"
#include "core/module.h"
#include "engine/console_cmds.h"
#include "engine/d3d8_hooks.h"

#include <d3d8.h>

#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {

std::string g_pending;  // name of the shot to take on the next Present, "" = none
bool g_registered = false;

void __cdecl cmd_shot(const char* arg) {
    std::string name;
    for (const char* p = arg; p && *p && name.size() < 48; ++p) {
        const char c = *p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
            name.push_back(c);
        }
    }
    if (name.empty()) name = "shot";
    g_pending = name;
}

bool write_bmp(const std::filesystem::path& path, const uint8_t* bits, int pitch, int w, int h) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) return false;
    const uint32_t row = static_cast<uint32_t>(w) * 4;
    const uint32_t data = row * static_cast<uint32_t>(h);
    uint8_t hdr[54] = {'B', 'M'};
    auto put32 = [&](int at, uint32_t v) {
        for (int i = 0; i < 4; ++i) hdr[at + i] = static_cast<uint8_t>(v >> (8 * i));
    };
    put32(2, 54 + data);
    put32(10, 54);
    put32(14, 40);
    put32(18, static_cast<uint32_t>(w));
    put32(22, static_cast<uint32_t>(h));
    hdr[26] = 1;
    hdr[28] = 32;
    put32(34, data);
    fwrite(hdr, 1, sizeof hdr, f);
    std::vector<uint8_t> line(row);
    for (int y = h - 1; y >= 0; --y) {  // BMP rows go bottom-up
        const uint8_t* src = bits + static_cast<size_t>(y) * static_cast<size_t>(pitch);
        for (uint32_t i = 0; i < row; i += 4) {
            line[i] = src[i];
            line[i + 1] = src[i + 1];
            line[i + 2] = src[i + 2];
            line[i + 3] = 0xff;
        }
        fwrite(line.data(), 1, row, f);
    }
    fclose(f);
    return true;
}

void take(IDirect3DDevice8* dev, const std::string& name) {
    IDirect3DSurface8* back = nullptr;
    IDirect3DSurface8* copy = nullptr;
    D3DSURFACE_DESC desc{};
    HRESULT hr = dev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back);
    if (SUCCEEDED(hr)) hr = back->GetDesc(&desc);
    // The SDK subset in third_party/d3d8 names only A8R8G8B8 (21); X8R8G8B8 is 22.
    const bool fmt_ok = static_cast<int>(desc.Format) == 22 || desc.Format == D3DFMT_A8R8G8B8;
    if (SUCCEEDED(hr) && fmt_ok) hr = dev->CreateImageSurface(desc.Width, desc.Height, desc.Format, &copy);
    if (SUCCEEDED(hr) && fmt_ok) hr = dev->CopyRects(back, nullptr, 0, copy, nullptr);
    bool saved = false;
    std::filesystem::path out;
    if (SUCCEEDED(hr) && fmt_ok) {
        D3DLOCKED_RECT lr{};
        hr = copy->LockRect(&lr, nullptr, 0x10);  // D3DLOCK_READONLY
        if (SUCCEEDED(hr)) {
            std::error_code ec;
            const std::filesystem::path dir = dh2::core::self_dir() / "shots";
            std::filesystem::create_directories(dir, ec);
            out = dir / (name + ".bmp");
            saved = write_bmp(out, static_cast<const uint8_t*>(lr.pBits), lr.Pitch, static_cast<int>(desc.Width),
                              static_cast<int>(desc.Height));
            copy->UnlockRect();
        }
    }
    if (copy) copy->Release();
    if (back) back->Release();
    spdlog::info("shot: '{}' {}x{} fmt={} hr={:#010x} saved={} {}", name, desc.Width, desc.Height,
                 static_cast<int>(desc.Format), static_cast<uint32_t>(hr), saved, out.string());
}

void on_present(void* device, const RECT*, const RECT*, HWND, const void*) {
    if (g_pending.empty() || !device) return;
    const std::string name = g_pending;
    g_pending.clear();
    take(static_cast<IDirect3DDevice8*>(device), name);
}

class ShotModule : public dh2::Module {
public:
    const char* Name() const override { return "shot"; }
    // After the overlay (200): its Present callback draws first, so the shot shows it.
    int Priority() const override { return 210; }

    void OnInit() override { dh2::d3d8::AddPresentCallback(&on_present); }

    void OnFrame(float) override {
        if (g_registered || !dh2::console_cmds::Ready()) return;
        g_registered = dh2::console_cmds::AddCommandStr("coop_shot", &cmd_shot);
    }
};

DH2_MODULE(ShotModule)

}  // namespace
