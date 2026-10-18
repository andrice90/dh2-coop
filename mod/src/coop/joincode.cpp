#include "coop/joincode.h"

#include <cctype>
#include <vector>

namespace dh2::coop::joincode {
namespace {

constexpr char kPrefix[] = "DH2-";
constexpr size_t kPrefixLen = 4;
constexpr char kEllipsis[] = "\xE2\x80\xA6";  // U+2026 in UTF-8
constexpr char kKeyPrefix[] = "tskey-auth-";
constexpr size_t kKeyPrefixLen = 11;
constexpr size_t kMaskedKeyChars = 14;  // "tskey-auth-" + 3 more

void fail(std::string* err, const char* text) {
    if (err) *err = text;
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Whole string is 1..maxDigits digits, no leading zero (except "0" itself). Value in *out.
bool parse_uint(std::string_view s, size_t maxDigits, unsigned& out) {
    if (s.empty() || s.size() > maxDigits) return false;
    if (s.size() > 1 && s[0] == '0') return false;
    unsigned v = 0;
    for (char c : s) {
        if (!is_digit(c)) return false;
        v = v * 10 + static_cast<unsigned>(c - '0');
    }
    out = v;
    return true;
}

bool parse_port(std::string_view s, uint16_t& out) {
    unsigned v = 0;
    if (!parse_uint(s, 5, v) || v < 1 || v > 65535) return false;
    out = static_cast<uint16_t>(v);
    return true;
}

bool printable_ascii(char c) { return c >= 0x20 && c <= 0x7E; }

int b64_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

bool iequals_prefix(std::string_view s, const char* prefix, size_t n) {
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (std::toupper(static_cast<unsigned char>(s[i])) != std::toupper(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

std::string strip_space(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) out.push_back(c);
    }
    return out;
}

std::vector<std::string_view> split_bar(std::string_view s) {
    std::vector<std::string_view> parts;
    size_t from = 0;
    for (;;) {
        const size_t bar = s.find('|', from);
        if (bar == std::string_view::npos) {
            parts.push_back(s.substr(from));
            return parts;
        }
        parts.push_back(s.substr(from, bar - from));
        from = bar + 1;
    }
}

}  // namespace

bool IsTailnetIp(std::string_view ip) {
    unsigned oct[4] = {};
    size_t from = 0;
    for (int i = 0; i < 4; ++i) {
        const size_t dot = ip.find('.', from);
        const bool last = i == 3;
        if ((dot == std::string_view::npos) != last) return false;
        const std::string_view part = ip.substr(from, last ? std::string_view::npos : dot - from);
        if (!parse_uint(part, 3, oct[i]) || oct[i] > 255) return false;
        from = dot + 1;
    }
    return oct[0] == 100 && oct[1] >= 64 && oct[1] <= 127;
}

bool IsAuthKey(std::string_view key) {
    if (key.size() <= kKeyPrefixLen || key.substr(0, kKeyPrefixLen) != kKeyPrefix) return false;
    for (char c : key.substr(kKeyPrefixLen)) {
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || is_digit(c) || c == '-';
        if (!ok) return false;
    }
    return true;
}

std::string CleanName(std::string_view name) {
    std::string out;
    for (char c : name) {
        if (out.size() >= kMaxNameChars) break;
        out.push_back(printable_ascii(c) && c != '|' ? c : '_');
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    size_t lead = 0;
    while (lead < out.size() && out[lead] == ' ') ++lead;
    return out.substr(lead);
}

std::string Base64UrlEncode(std::string_view bytes) {
    static const char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 3 <= bytes.size(); i += 3) {
        const unsigned v = (static_cast<unsigned char>(bytes[i]) << 16) | (static_cast<unsigned char>(bytes[i + 1]) << 8) |
                           static_cast<unsigned char>(bytes[i + 2]);
        out.push_back(kAlphabet[(v >> 18) & 63]);
        out.push_back(kAlphabet[(v >> 12) & 63]);
        out.push_back(kAlphabet[(v >> 6) & 63]);
        out.push_back(kAlphabet[v & 63]);
    }
    const size_t rest = bytes.size() - i;
    if (rest == 1) {
        const unsigned v = static_cast<unsigned char>(bytes[i]) << 16;
        out.push_back(kAlphabet[(v >> 18) & 63]);
        out.push_back(kAlphabet[(v >> 12) & 63]);
    } else if (rest == 2) {
        const unsigned v = (static_cast<unsigned char>(bytes[i]) << 16) | (static_cast<unsigned char>(bytes[i + 1]) << 8);
        out.push_back(kAlphabet[(v >> 18) & 63]);
        out.push_back(kAlphabet[(v >> 12) & 63]);
        out.push_back(kAlphabet[(v >> 6) & 63]);
    }
    return out;
}

bool Base64UrlDecode(std::string_view text, std::string& out) {
    if (text.size() % 4 == 1) return false;
    std::string bytes;
    bytes.reserve(text.size() / 4 * 3 + 2);
    unsigned acc = 0;
    int bits = 0;
    for (char c : text) {
        const int v = b64_value(c);
        if (v < 0) return false;
        acc = (acc << 6) | static_cast<unsigned>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            bytes.push_back(static_cast<char>((acc >> bits) & 0xFF));
        }
    }
    // The unused low bits of the last character must be zero (the one spelling an encoder produces).
    if (bits > 0 && (acc & ((1u << bits) - 1)) != 0) return false;
    out = std::move(bytes);
    return true;
}

std::string Encode(const Fields& f, std::string* err) {
    if (!IsTailnetIp(f.ip)) {
        fail(err, "The address is not a Tailscale address (100.64.0.0 to 100.127.255.255).");
        return {};
    }
    if (f.port < 1) {
        fail(err, "The port must be 1 to 65535.");
        return {};
    }
    if (!f.key.empty() && !IsAuthKey(f.key)) {
        fail(err, "The friend key must look like tskey-auth-...");
        return {};
    }
    std::string payload = "v1|" + f.ip + "|" + std::to_string(f.port) + "|" + f.key + "|" + CleanName(f.name);
    std::string code = std::string(kPrefix) + Base64UrlEncode(payload);
    if (code.size() > kMaxCodeChars) {
        fail(err, "The join code would be too long.");
        return {};
    }
    return code;
}

bool LooksLikeCode(std::string_view text) {
    size_t lead = 0;
    while (lead < text.size() && std::isspace(static_cast<unsigned char>(text[lead]))) ++lead;
    return iequals_prefix(text.substr(lead), kPrefix, kPrefixLen);
}

bool Decode(std::string_view text, Fields& out, std::string* err) {
    const std::string code = strip_space(text);
    if (code.size() > kMaxCodeChars) {
        fail(err, "That join code is too long.");
        return false;
    }
    if (!iequals_prefix(code, kPrefix, kPrefixLen)) {
        fail(err, "A join code starts with DH2-");
        return false;
    }
    std::string payload;
    if (!Base64UrlDecode(std::string_view(code).substr(kPrefixLen), payload)) {
        fail(err, "That join code is damaged (copy it again, all of it).");
        return false;
    }
    for (char c : payload) {
        if (!printable_ascii(c)) {
            fail(err, "That join code is damaged (copy it again, all of it).");
            return false;
        }
    }
    const std::vector<std::string_view> parts = split_bar(payload);
    if (parts.size() != 5) {
        fail(err, "That join code is damaged (copy it again, all of it).");
        return false;
    }
    if (parts[0] != "v1") {
        fail(err, "That join code is from a newer kit. Ask the host for the new kit.");
        return false;
    }
    Fields f;
    if (!IsTailnetIp(parts[1])) {
        fail(err, "That join code holds an address that is not a Tailscale address.");
        return false;
    }
    f.ip = std::string(parts[1]);
    if (!parse_port(parts[2], f.port)) {
        fail(err, "That join code holds an invalid port.");
        return false;
    }
    if (!parts[3].empty() && !IsAuthKey(parts[3])) {
        fail(err, "That join code holds an invalid friend key.");
        return false;
    }
    f.key = std::string(parts[3]);
    if (parts[4].size() > kMaxNameChars) {
        fail(err, "That join code holds a host name that is too long.");
        return false;
    }
    f.name = std::string(parts[4]);
    out = std::move(f);
    return true;
}

std::string MaskKey(std::string_view key) {
    if (key.empty()) return "(none)";
    return std::string(key.substr(0, kMaskedKeyChars)) + kEllipsis;
}

std::string Describe(const Fields& f) {
    std::string s = f.ip + ":" + std::to_string(f.port) + ", host '" + f.name + "', ";
    if (f.key.empty())
        s += "no key";
    else
        s += "key " + MaskKey(f.key) + " (" + std::to_string(f.key.size()) + " chars)";
    return s;
}

std::string MaskedCode(const Fields& f) {
    return std::string(kPrefix) + kEllipsis + (f.key.empty() ? "(no key)" : "(key hidden)");
}

}  // namespace dh2::coop::joincode
