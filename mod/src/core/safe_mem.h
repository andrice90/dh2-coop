#pragma once
// Fault-tolerant memory helpers (SEH). Safe to call on arbitrary game pointers.
#include <cstddef>
#include <string>

namespace dh2::core {

// Copies n bytes; returns false (and leaves dst undefined) if src is unreadable.
bool safe_copy(const void* src, void* dst, size_t n);

template <class T>
bool safe_read(const void* src, T& out) {
    return safe_copy(src, &out, sizeof(T));
}

// Reads a NUL-terminated string (max_len chars). Returns "<null>" / "<unreadable>" on failure.
std::string safe_cstr(const char* p, size_t max_len = 260);

// "aa bb cc ..." of up to n bytes, "<unreadable>" on fault.
std::string hex_dump(const void* p, size_t n);

}  // namespace dh2::core
