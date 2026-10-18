#pragma once
// Join code: the one string a host gives a friend for a join over the internet (the host's UPnP port forward).
// "DH2-" + base64url (no padding) of the ASCII text
//   v2|<host ipv4>|<port>|<password or empty>|<host player name>
// The lobby's JOIN field takes a code, or a plain IP / IP:port (LAN, tailnet). The password is [Coop] Password when
// the host set one ("" = none). Pure functions, no engine or Windows calls, so mod/net-test checks them. A code with
// a password is a secret: never log a whole code, use Describe() / MaskedCode(). (v1 held a Tailscale key; dropped.)
#include <cstdint>
#include <string>
#include <string_view>

namespace dh2::coop::joincode {

constexpr size_t kMaxCodeChars = 300;  // the whole text, "DH2-" included
constexpr size_t kMaxNameChars = 32;
constexpr size_t kMaxPassChars = 63;   // [Coop] Password is read into 64 bytes

struct Fields {
    std::string ip;     // dotted IPv4 a friend can be sent to (IsUsableIp)
    uint16_t port = 0;  // 1..65535
    std::string pass;   // "" or printable ASCII without '|', <= kMaxPassChars
    std::string name;   // host player name, <= 32 printable ASCII characters, no '|'
};

// Dotted quad without leading zeros.
bool ParseIpv4(std::string_view ip, unsigned (&octets)[4]);
// Inside 100.64.0.0/10 (100.64.0.0 - 100.127.255.255): a Tailscale address.
bool IsTailnetIp(std::string_view ip);
// A unicast address another PC can send to: not 0.x, 127.x, 169.254.x, multicast / reserved (224+).
bool IsUsableIp(std::string_view ip);
// Printable ASCII (0x20-0x7E) only, '|' and other characters become '_', trimmed, at most kMaxNameChars.
std::string CleanName(std::string_view name);

// "" and *err set when ip / port / pass is invalid or the code would be longer than kMaxCodeChars. The name is
// cleaned (CleanName) instead of rejected.
std::string Encode(const Fields& fields, std::string* err = nullptr);
// Whitespace anywhere in `text` is ignored (chat clients wrap long lines). False and *err set on anything that is
// not a valid v2 code; `out` is left unchanged then.
bool Decode(std::string_view text, Fields& out, std::string* err = nullptr);
// True when the text, after trimming, starts with "DH2-" (any case): it is meant to be a code, valid or not.
bool LooksLikeCode(std::string_view text);

// `text` with every DH2-... run masked ("DH2-…(hidden)"): for logging command lines.
std::string RedactSecrets(std::string_view text);

// For logs and screens: never the password.
std::string Describe(const Fields& fields);    // "203.x.x.7:27420, host 'Name', password set" / "no password"
                                               // (a public address masked; LAN / tailnet addresses whole)
std::string MaskedCode(const Fields& fields);  // "DH2-…(hidden)"

// Exposed for the tests.
std::string Base64UrlEncode(std::string_view bytes);
bool Base64UrlDecode(std::string_view text, std::string& out);

}  // namespace dh2::coop::joincode
