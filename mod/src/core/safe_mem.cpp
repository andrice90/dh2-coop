#include "core/safe_mem.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace dh2::core {

// No C++ objects with destructors in this function: required for __try/__except.
bool safe_copy(const void* src, void* dst, size_t n) {
    if (!src) return false;
    __try {
        memcpy(dst, src, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

std::string safe_cstr(const char* p, size_t max_len) {
    if (!p) return "<null>";
    std::string s;
    s.reserve(64);
    for (size_t i = 0; i < max_len; ++i) {
        char c;
        if (!safe_copy(p + i, &c, 1)) return s.empty() ? "<unreadable>" : s + "<fault>";
        if (c == '\0') return s;
        s.push_back(c);
    }
    return s + "...";
}

std::string hex_dump(const void* p, size_t n) {
    if (n > 256) n = 256;
    unsigned char buf[256];
    if (!safe_copy(p, buf, n)) return "<unreadable>";
    std::string s;
    s.reserve(n * 3);
    char tmp[4];
    for (size_t i = 0; i < n; ++i) {
        std::snprintf(tmp, sizeof tmp, "%02x", buf[i]);
        if (i) s.push_back(' ');
        s += tmp;
    }
    return s;
}

}  // namespace dh2::core
