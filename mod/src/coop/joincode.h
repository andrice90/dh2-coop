#pragma once
// Join code: the one string a host gives a friend. "DH2-" + base64url (no padding) of the ASCII text
//   v1|<tailnet ipv4>|<port>|<tailscale auth key or empty>|<host player name>
// The friend's installer (tools-src/kit/install.ps1 has a PowerShell twin of Encode / Decode) uses the key to join
// the host's tailnet; the lobby's JOIN field takes the address and port out of it. Pure functions, no engine or
// Windows calls, so mod/net-test checks them. The auth key is a secret: never log a whole code or key, use
// Describe() / MaskKey() / MaskedCode().
#include <cstdint>
#include <string>
#include <string_view>

namespace dh2::coop::joincode {

constexpr size_t kMaxCodeChars = 300;  // the whole text, "DH2-" included
constexpr size_t kMaxNameChars = 32;

struct Fields {
    std::string ip;     // dotted IPv4 in 100.64.0.0/10 (a Tailscale address)
    uint16_t port = 0;  // 1..65535
    std::string key;    // "" or tskey-auth-... ("" = the friend is already on the host's tailnet)
    std::string name;   // host player name, <= 32 printable ASCII characters, no '|'
};

// Dotted quad without leading zeros, inside 100.64.0.0/10 (100.64.0.0 - 100.127.255.255).
bool IsTailnetIp(std::string_view ip);
// ^tskey-auth-[A-Za-z0-9-]+$
bool IsAuthKey(std::string_view key);
// Printable ASCII (0x20-0x7E) only, '|' and other characters become '_', trimmed, at most kMaxNameChars.
std::string CleanName(std::string_view name);

// "" and *err set when ip / port / key is invalid or the code would be longer than kMaxCodeChars. The name is
// cleaned (CleanName) instead of rejected.
std::string Encode(const Fields& fields, std::string* err = nullptr);
// Whitespace anywhere in `text` is ignored (chat clients wrap long lines). False and *err set on anything that is
// not a valid v1 code; `out` is left unchanged then.
bool Decode(std::string_view text, Fields& out, std::string* err = nullptr);
// True when the text, after trimming, starts with "DH2-" (any case): it is meant to be a code, valid or not.
bool LooksLikeCode(std::string_view text);

// `text` with every tskey-... and DH2-... run masked (MaskKey / "DH2-…(hidden)"): for logging command lines.
std::string RedactSecrets(std::string_view text);

// For logs and screens: never the whole key, never the whole code.
std::string MaskKey(std::string_view key);                  // "tskey-auth-kTE…" (first 14 chars)
std::string Describe(const Fields& fields);                 // "100.x.y.z:27420, host 'Name', key tskey-auth-kTE… (N chars)"
std::string MaskedCode(const Fields& fields);               // "DH2-…(key hidden)" or "DH2-…(no key)"

// Exposed for the tests.
std::string Base64UrlEncode(std::string_view bytes);
bool Base64UrlDecode(std::string_view text, std::string& out);

}  // namespace dh2::coop::joincode
