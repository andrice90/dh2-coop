// Join code and friend key for the lobby (HOST: save the key, copy the code; JOIN: paste the code) and their console
// twins. See coop.h "Join code and friend key" and coop/joincode.h. The friend key is a secret: it only ever lives in
// memory briefly and in dh2coop-lobby.ini as a DPAPI blob; logs get "friend key saved (N chars)" and masked forms.
#include "coop/coop.h"

#include <windows.h>
#include <shellapi.h>
#include <wincrypt.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/clipboard.h"
#include "coop/joincode.h"
#include "core/log.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"

namespace dh2::coop {
namespace {

constexpr char kUrlAcl[] = "https://login.tailscale.com/admin/acls/file";
constexpr char kUrlKeys[] = "https://login.tailscale.com/admin/settings/keys";

int g_key_chars = -1;  // cache: -1 = not read yet
std::string g_pending_paste;
bool g_has_pending_paste = false;

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    size_t lead = 0;
    while (lead < s.size() && std::isspace(static_cast<unsigned char>(s[lead]))) ++lead;
    return s.substr(lead);
}

void wipe(std::string& s) {
    if (!s.empty()) SecureZeroMemory(s.data(), s.size());
    s.clear();
}

// ---- DPAPI (CryptProtectData, current user, no UI) as base64 text for the ini ----

bool protect_to_base64(const std::string& plain, std::string& b64) {
    DATA_BLOB in{};
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()));
    in.cbData = static_cast<DWORD>(plain.size());
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"dh2coop friend key", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
        return false;
    DWORD n = 0;
    bool ok = CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &n) && n > 0;
    if (ok) {
        std::string text(n, '\0');
        ok = CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, text.data(), &n);
        if (ok) {
            text.resize(n);
            b64 = std::move(text);
        }
    }
    LocalFree(out.pbData);
    return ok;
}

bool unprotect_from_base64(const std::string& b64, std::string& plain) {
    DWORD n = 0;
    if (!CryptStringToBinaryA(b64.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &n, nullptr, nullptr) || n == 0) return false;
    std::vector<BYTE> blob(n);
    if (!CryptStringToBinaryA(b64.c_str(), 0, CRYPT_STRING_BASE64, blob.data(), &n, nullptr, nullptr)) return false;
    DATA_BLOB in{};
    in.pbData = blob.data();
    in.cbData = n;
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) return false;
    plain.assign(reinterpret_cast<const char*>(out.pbData), out.cbData);
    SecureZeroMemory(out.pbData, out.cbData);
    LocalFree(out.pbData);
    return true;
}

bool read_friend_key(std::string& key) {
    char buf[2048] = {};
    GetPrivateProfileStringA("Lobby", "FriendKey", "", buf, sizeof buf, LobbyIniPath().c_str());
    if (!buf[0]) return false;
    std::string plain;
    if (!unprotect_from_base64(buf, plain) || !joincode::IsAuthKey(plain)) {
        wipe(plain);
        return false;
    }
    key = std::move(plain);
    return true;
}

uint16_t code_port() {
    const LobbyView v = Lobby();
    if (v.state == LobbyState::Hosting && v.port) return v.port;
    return LoadLobbyPrefs().host_port;
}

bool write_text_file(const char* name, const std::string& text, std::string& where) {
    const std::filesystem::path path = core::log_dir() / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << text;
    where = path.string();
    return static_cast<bool>(out);
}

// ---- console twins ----

void __cdecl cmd_friendkey(const char* arg) {
    std::string key = trim(core::safe_cstr(arg, 256));
    if (key.empty() || key == "<null>" || key == "<unreadable>") {
        spdlog::warn("coop_friendkey: usage coop_friendkey <tskey-auth-...> | clear  (now: {})",
                     FriendKeyChars() > 0 ? "a key is saved" : "no key saved");
        return;
    }
    if (key == "clear") {
        ClearFriendKey();
        return;
    }
    std::string msg;
    SaveFriendKey(key, msg);
    wipe(key);
    spdlog::info("coop_friendkey: {}", msg);
}

// coop_joincode show     logs what the code holds (masked)
// coop_joincode file     also writes the whole code to <log dir>\dh2coop-joincode.txt (tests only)
// (The engine drops a console command that is called without its argument, so there is no bare form.)
void __cdecl cmd_joincode(const char* arg) {
    const std::string a = trim(core::safe_cstr(arg, 32));
    if (a != "show" && a != "file") {
        spdlog::warn("coop_joincode: usage coop_joincode show | file  (file writes the whole code for tests)");
        return;
    }
    const std::string name = LoadLobbyPrefs().name;
    const JoinCodeResult r = MakeJoinCode(code_port(), name);
    if (r.code.empty()) {
        spdlog::warn("coop_joincode: no code: {}", r.message);
        return;
    }
    joincode::Fields f;
    joincode::Decode(r.code, f);
    spdlog::info("coop_joincode: {} = {}, {} chars", r.masked, joincode::Describe(f), r.code.size());
    if (a == "file") {
        std::string where;
        if (write_text_file("dh2coop-joincode.txt", r.code, where))
            spdlog::info("coop_joincode: whole code written to {} (tests only)", where);
        else
            spdlog::warn("coop_joincode: could not write dh2coop-joincode.txt");
    }
}

// coop_joinfield <text>: the same as pasting <text> into the lobby's JOIN address field. "file" pastes the code that
// `coop_joincode file` wrote (a console line holds only 127 characters, a code with a real key is longer).
void __cdecl cmd_joinfield(const char* arg) {
    std::string text = trim(core::safe_cstr(arg, 700));
    if (text.empty() || text == "<null>") {
        spdlog::warn("coop_joinfield: usage coop_joinfield <join code | ip | file>");
        return;
    }
    if (text == "file") {
        std::ifstream in(core::log_dir() / "dh2coop-joincode.txt", std::ios::binary);
        std::string code((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        text = trim(code);
        if (text.empty()) {
            spdlog::warn("coop_joinfield: no dh2coop-joincode.txt in the log folder");
            return;
        }
    }
    const JoinInput in = ParseJoinInput(text);
    spdlog::info("coop_joinfield: pasted {} chars -> {}", text.size(),
                 in.kind == JoinInput::Kind::Code      ? "code: " + in.message
                 : in.kind == JoinInput::Kind::BadCode ? "bad code: " + in.message
                                                       : "address");
    SetPendingJoinPaste(text);
}

// coop_tshelp file: writes the ACL snippet to <log dir>\dh2coop-acl.txt. The real button also opens the browser and
// copies to the clipboard; neither belongs in a test.
void __cdecl cmd_tshelp(const char* arg) {
    const std::string a = trim(core::safe_cstr(arg, 32));
    if (a != "file") {
        spdlog::warn("coop_tshelp: usage coop_tshelp file  (writes the ACL snippet; the lobby button opens the pages)");
        return;
    }
    std::string where;
    if (write_text_file("dh2coop-acl.txt", AclSnippet(Lobby().tailnetIp, code_port()), where))
        spdlog::info("coop_tshelp: ACL snippet written to {}", where);
    else
        spdlog::warn("coop_tshelp: could not write dh2coop-acl.txt");
}

}  // namespace

bool SaveFriendKey(const std::string& rawKey, std::string& message) {
    std::string key = trim(rawKey);
    if (!joincode::IsAuthKey(key)) {
        message = "That is not a Tailscale auth key. It starts with tskey-auth- (Tailscale admin, Settings, Keys).";
        wipe(key);
        return false;
    }
    std::string blob;
    if (!protect_to_base64(key, blob)) {
        message = "Windows could not protect the key for this account.";
        wipe(key);
        return false;
    }
    const size_t chars = key.size();
    wipe(key);
    if (!WritePrivateProfileStringA("Lobby", "FriendKey", blob.c_str(), LobbyIniPath().c_str())) {
        message = "Could not write dh2coop-lobby.ini.";
        return false;
    }
    g_key_chars = -1;
    // Read it back through DPAPI: a key that cannot be read again is worse than none.
    std::string check;
    const bool readable = read_friend_key(check);
    wipe(check);
    if (!readable) {
        message = "The key was saved but cannot be read back. Try again.";
        return false;
    }
    spdlog::info("coop: friend key saved ({} chars)", chars);
    message = "Friend key saved (" + std::to_string(chars) + " characters).";
    return true;
}

void ClearFriendKey() {
    WritePrivateProfileStringA("Lobby", "FriendKey", nullptr, LobbyIniPath().c_str());
    g_key_chars = 0;
    spdlog::info("coop: friend key removed");
}

int FriendKeyChars() {
    if (g_key_chars < 0) {
        std::string key;
        g_key_chars = read_friend_key(key) ? static_cast<int>(key.size()) : 0;
        wipe(key);
    }
    return g_key_chars;
}

JoinCodeResult MakeJoinCode(uint16_t port, const std::string& playerName) {
    JoinCodeResult r;
    const std::string ip = Lobby().tailnetIp;
    if (!joincode::IsTailnetIp(ip)) {
        r.message = "No tailnet address yet. Start Tailscale on this PC (it can take a few seconds to connect).";
        return r;
    }
    joincode::Fields f;
    f.ip = ip;
    f.port = port;
    f.name = playerName;
    r.hasKey = read_friend_key(f.key);
    std::string err;
    r.code = joincode::Encode(f, &err);
    if (r.code.empty()) {
        r.message = err;
    } else {
        r.masked = joincode::MaskedCode(f);
        r.message = r.hasKey ? "Join code copied. It carries your friend key: send it only to friends."
                             : "Join code copied, IP only: for friends already on your tailnet. Save a friend key to "
                               "let new friends join without an account.";
    }
    wipe(f.key);
    return r;
}

bool CopyJoinCode(uint16_t port, const std::string& playerName, std::string& message) {
    JoinCodeResult r = MakeJoinCode(port, playerName);
    if (r.code.empty()) {
        message = r.message;
        return false;
    }
    joincode::Fields f;
    joincode::Decode(r.code, f);
    const bool ok = SetClipboardUnicode(r.code, ProcessMainWindow());
    spdlog::info("coop: join code {} ({}, {} chars)", ok ? "copied to the clipboard" : "NOT copied (clipboard busy)",
                 joincode::Describe(f), r.code.size());
    wipe(r.code);
    message = ok ? r.message : "Could not open the clipboard. Try again.";
    return ok;
}

std::string AclSnippet(const std::string& hostTailnetIp, uint16_t port) {
    const std::string dst =
        (hostTailnetIp.empty() ? std::string("<HOST_TAILNET_IP>") : hostTailnetIp) + ":" + std::to_string(port ? port : 27420);
    return "{\n"
           "  \"tagOwners\": { \"tag:dh2friend\": [\"autogroup:admin\"] },\n"
           "  \"acls\": [\n"
           "    { \"action\": \"accept\", \"src\": [\"autogroup:member\"], \"dst\": [\"*:*\"] },\n"
           "    { \"action\": \"accept\", \"src\": [\"tag:dh2friend\"], \"dst\": [\"" +
           dst +
           "\"] }\n"
           "  ]\n"
           "}\n";
}

bool OpenTailscaleHelp(uint16_t port, std::string& message) {
    const std::string ip = Lobby().tailnetIp;
    const bool copied = SetClipboardUnicode(AclSnippet(ip, port), ProcessMainWindow());
    ShellExecuteA(nullptr, "open", kUrlAcl, nullptr, nullptr, SW_SHOWNORMAL);
    ShellExecuteA(nullptr, "open", kUrlKeys, nullptr, nullptr, SW_SHOWNORMAL);
    spdlog::info("coop: Tailscale setup help opened (ACL snippet {}, tailnet ip {})", copied ? "copied" : "NOT copied",
                 ip.empty() ? "none" : ip);
    message = std::string(copied ? "The ACL snippet is on your clipboard. " : "") +
              "In the Tailscale admin page that opened: paste it over the whole policy file and Save (this replaces the "
              "default allow-all policy: if you have your own rules, merge by hand). Then Settings > Keys > Generate auth "
              "key: Reusable on, Ephemeral on, Tags tag:dh2friend, expiry 90 days. Paste that key into Friend key here and "
              "press Save." +
              (ip.empty() ? " Tailscale is not connected on this PC yet, so the snippet still says <HOST_TAILNET_IP>."
                          : "");
    return true;
}

JoinInput ParseJoinInput(const std::string& text) {
    JoinInput in;
    const std::string t = trim(text);
    if (t.empty()) return in;
    if (!joincode::LooksLikeCode(t)) {
        in.kind = JoinInput::Kind::Address;
        in.host = t;
        return in;
    }
    joincode::Fields f;
    std::string err;
    if (!joincode::Decode(t, f, &err)) {
        in.kind = JoinInput::Kind::BadCode;
        in.message = err;
        return in;
    }
    in.kind = JoinInput::Kind::Code;
    in.host = f.ip;
    in.port = f.port;
    in.hostName = f.name;
    in.message = "Join code accepted" + (f.name.empty() ? std::string() : " (host " + f.name + ")") + ": " + f.ip + ", port " +
                 std::to_string(f.port) + ". Press JOIN.";
    wipe(f.key);  // the key is for the installer, not for the game
    return in;
}

void SetPendingJoinPaste(const std::string& text) {
    g_pending_paste = text;
    g_has_pending_paste = true;
}

bool TakePendingJoinPaste(std::string& text) {
    if (!g_has_pending_paste) return false;
    text = std::move(g_pending_paste);
    g_pending_paste.clear();
    g_has_pending_paste = false;
    return true;
}

void RegisterInviteCommands() {
    console_cmds::AddCommandStr("coop_friendkey", &cmd_friendkey);
    console_cmds::AddCommandStr("coop_joincode", &cmd_joincode);
    console_cmds::AddCommandStr("coop_joinfield", &cmd_joinfield);
    console_cmds::AddCommandStr("coop_tshelp", &cmd_tshelp);
}

}  // namespace dh2::coop
