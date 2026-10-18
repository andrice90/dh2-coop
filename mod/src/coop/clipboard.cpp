#include "coop/clipboard.h"

#include <windows.h>

#include <vector>

namespace dh2::coop {
namespace {

bool open_clipboard(HWND owner) {
    for (int i = 0; i < 20; ++i) {
        if (OpenClipboard(owner)) return true;
        Sleep(10);
    }
    return false;
}

struct FindWindowCtx {
    DWORD pid = 0;
    HWND found = nullptr;
};

BOOL CALLBACK find_window(HWND hwnd, LPARAM lparam) {
    auto* ctx = reinterpret_cast<FindWindowCtx*>(lparam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == ctx->pid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
        ctx->found = hwnd;
        return FALSE;
    }
    return TRUE;
}

}  // namespace

void* ProcessMainWindow() {
    FindWindowCtx ctx;
    ctx.pid = GetCurrentProcessId();
    EnumWindows(&find_window, reinterpret_cast<LPARAM>(&ctx));
    return ctx.found;
}

bool SetClipboardUnicode(const std::string& utf8, void* owner) {
    // LF -> CRLF, then UTF-8 -> UTF-16.
    std::string text;
    text.reserve(utf8.size() + 16);
    for (size_t i = 0; i < utf8.size(); ++i) {
        if (utf8[i] == '\n' && (i == 0 || utf8[i - 1] != '\r')) text.push_back('\r');
        text.push_back(utf8[i]);
    }
    const int n = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (n <= 0) return false;
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(n) * sizeof(wchar_t));
    if (!mem) return false;
    auto* dst = static_cast<wchar_t*>(GlobalLock(mem));
    if (!dst) {
        GlobalFree(mem);
        return false;
    }
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, dst, n);
    GlobalUnlock(mem);

    if (!open_clipboard(static_cast<HWND>(owner))) {
        GlobalFree(mem);
        return false;
    }
    EmptyClipboard();
    const bool ok = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
    if (!ok) GlobalFree(mem);  // the clipboard owns the memory after a successful SetClipboardData
    CloseClipboard();
    return ok;
}

bool GetClipboardUnicode(std::string& utf8, void* owner) {
    if (!open_clipboard(static_cast<HWND>(owner))) return false;
    bool ok = false;
    if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) {
        if (const auto* w = static_cast<const wchar_t*>(GlobalLock(h))) {
            const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
            if (n > 0) {
                std::vector<char> buf(static_cast<size_t>(n));
                WideCharToMultiByte(CP_UTF8, 0, w, -1, buf.data(), n, nullptr, nullptr);
                utf8.assign(buf.data());
                ok = true;
            }
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return ok;
}

}  // namespace dh2::coop
