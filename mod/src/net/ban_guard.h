#pragma once

// Per-address wrong-password counter for the host. With the UPnP port forward the game port is reachable from the
// internet, so a stranger can guess passwords. After kMaxFailures wrong passwords from one address within kWindowMs
// that address is refused for kBanMs. Header-only and clock-free (the caller passes a millisecond clock) so the
// net-test can drive it with a fake time.

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <unordered_map>

namespace net {

class BanGuard {
public:
    static constexpr int kMaxFailures = 5;
    static constexpr uint64_t kWindowMs = 10ull * 60 * 1000;
    static constexpr uint64_t kBanMs = 10ull * 60 * 1000;
    static constexpr size_t kMaxTracked = 256;  // bounded: an address scan must not grow this forever

    // One wrong password from `ip` (any stable per-address key, ENet's host field). True exactly once per ban: the
    // call that crosses the limit, so the caller logs it once.
    bool NoteFailure(uint32_t ip, uint64_t nowMs) {
        Prune(nowMs);
        Entry& e = entries_[ip];
        if (e.bannedUntil > nowMs) return false;  // already banned
        if (e.failures == 0 || nowMs - e.first > kWindowMs) {
            e.failures = 0;
            e.first = nowMs;
        }
        if (++e.failures < kMaxFailures) return false;
        e.bannedUntil = nowMs + kBanMs;
        e.failures = 0;
        return true;
    }

    bool IsBanned(uint32_t ip, uint64_t nowMs) const {
        if (entries_.empty()) return false;
        const auto it = entries_.find(ip);
        return it != entries_.end() && it->second.bannedUntil > nowMs;
    }

    // A correct password clears the address's count (a friend who mistyped twice is not on the way to a ban).
    void NoteSuccess(uint32_t ip) {
        const auto it = entries_.find(ip);
        if (it != entries_.end() && it->second.bannedUntil == 0) entries_.erase(it);
    }

    size_t Tracked() const { return entries_.size(); }

private:
    struct Entry {
        int failures = 0;
        uint64_t first = 0;
        uint64_t bannedUntil = 0;
    };

    void Prune(uint64_t nowMs) {
        if (entries_.size() < kMaxTracked) return;
        for (auto it = entries_.begin(); it != entries_.end();) {
            const Entry& e = it->second;
            const bool banExpired = e.bannedUntil != 0 && e.bannedUntil <= nowMs;
            const bool windowExpired = e.bannedUntil == 0 && nowMs - e.first > kWindowMs;
            it = (banExpired || windowExpired) ? entries_.erase(it) : std::next(it);
        }
        // Still full of live entries (a scan from many addresses): start over rather than refuse to count.
        if (entries_.size() >= kMaxTracked) entries_.clear();
    }

    std::unordered_map<uint32_t, Entry> entries_;
};

}  // namespace net
