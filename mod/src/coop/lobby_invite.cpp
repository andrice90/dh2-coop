// Join code for the lobby (HOST: copy the code for the UPnP port forward; JOIN: paste a code or type IP[:port]) and
// the console twins. See coop.h "Join code" and coop/joincode.h. A code can hold the [Coop] Password, so logs only
// get Describe() / masked forms.
#include "coop/coop.h"

#include <windows.h>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <spdlog/spdlog.h>

#include "coop/clipboard.h"
#include "coop/joincode.h"
#include "core/log.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"

namespace dh2::coop {
namespace {

std::string g_pending_paste;
bool g_has_pending_paste = false;

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    size_t lead = 0;
    while (lead < s.size() && std::isspace(static_cast<unsigned char>(s[lead]))) ++lead;
    return s.substr(lead);
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

// coop_joincode show     logs what the code holds (masked)
// coop_joincode file     also writes the whole code to <log dir>\dh2coop-joincode.txt (tests only)
// (The engine drops a console command that is called without its argument, so there is no bare form.)
void __cdecl cmd_joincode(const char* arg) {
    const std::string a = trim(core::safe_cstr(arg, 32));
    if (a != "show" && a != "file") {
        spdlog::warn("coop_joincode: usage coop_joincode show | file  (file writes the whole code for tests)");
        return;
    }
    const JoinCodeResult r = MakeJoinCode(LoadLobbyPrefs().name);
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

// coop_joinfield <text>: the same as typing / pasting <text> into the lobby's JOIN field. "file" pastes the code that
// `coop_joincode file` wrote.
void __cdecl cmd_joinfield(const char* arg) {
    std::string text = trim(core::safe_cstr(arg, 700));
    if (text.empty() || text == "<null>") {
        spdlog::warn("coop_joinfield: usage coop_joinfield <join code | ip[:port] | file>");
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
    spdlog::info("coop_joinfield: {} chars -> {}", text.size(),
                 in.kind == JoinInput::Kind::Code      ? "code: " + in.message
                 : in.kind == JoinInput::Kind::Bad     ? "bad: " + in.message
                 : in.kind == JoinInput::Kind::Address ? "address " + in.host + ":" + std::to_string(in.port)
                                                       : std::string("empty"));
    SetPendingJoinPaste(text);
}

}  // namespace

JoinCodeResult MakeJoinCode(const std::string& playerName) {
    JoinCodeResult r;
    const LobbyView v = Lobby();
    if (v.state != LobbyState::Hosting) {
        r.message = "Host first: the join code appears once the router has opened the port.";
        return r;
    }
    if (!v.internetOpen || v.internetIp.empty()) {
        r.message = v.internetMessage.empty() ? "The router has not opened the port (UPnP)." : v.internetMessage;
        return r;
    }
    joincode::Fields f;
    f.ip = v.internetIp;
    f.port = v.internetPort;
    f.pass = HostPassword();
    f.name = playerName.empty() ? std::string("Hunter") : playerName;
    std::string err;
    r.code = joincode::Encode(f, &err);
    if (r.code.empty()) {
        r.message = err;
    } else {
        r.masked = joincode::MaskedCode(f);
        r.message = "Join code copied. Friends paste it into JOIN.";
    }
    return r;
}

bool CopyJoinCode(const std::string& playerName, std::string& message) {
    JoinCodeResult r = MakeJoinCode(playerName);
    if (r.code.empty()) {
        message = r.message;
        return false;
    }
    joincode::Fields f;
    joincode::Decode(r.code, f);
    const bool ok = SetClipboardUnicode(r.code, ProcessMainWindow());
    spdlog::info("coop: join code {} ({}, {} chars)", ok ? "copied to the clipboard" : "NOT copied (clipboard busy)",
                 joincode::Describe(f), r.code.size());
    message = ok ? r.message : "Could not open the clipboard. Try again.";
    return ok;
}

JoinInput ParseJoinInput(const std::string& text, uint16_t defaultPort) {
    JoinInput in;
    const std::string t = trim(text);
    if (t.empty()) return in;
    if (joincode::LooksLikeCode(t)) {
        joincode::Fields f;
        std::string err;
        if (!joincode::Decode(t, f, &err)) {
            in.kind = JoinInput::Kind::Bad;
            in.message = err;
            return in;
        }
        in.kind = JoinInput::Kind::Code;
        in.host = f.ip;
        in.port = f.port;
        in.pass = f.pass;
        in.hostName = f.name;
        in.message = "Join code" + (f.name.empty() ? std::string() : " from " + f.name) + ": " + f.ip + " port " +
                     std::to_string(f.port) + ". Press JOIN.";
        return in;
    }
    // host, host:port (one colon; IPv6 is not supported by the game's network layer anyway)
    std::string host = t;
    uint16_t port = defaultPort ? defaultPort : 27420;
    const size_t colon = t.find(':');
    if (colon != std::string::npos) {
        if (t.find(':', colon + 1) != std::string::npos) {
            in.kind = JoinInput::Kind::Bad;
            in.message = "Type the host's address as IP or IP:port (for example 192.168.1.20:27420).";
            return in;
        }
        host = t.substr(0, colon);
        const std::string ps = t.substr(colon + 1);
        char* end = nullptr;
        const long p = std::strtol(ps.c_str(), &end, 10);
        if (ps.empty() || !end || *end != '\0' || p < 1 || p > 65535) {
            in.kind = JoinInput::Kind::Bad;
            in.message = "The port after ':' must be 1 to 65535.";
            return in;
        }
        port = static_cast<uint16_t>(p);
    }
    if (host.empty() || host.find(' ') != std::string::npos) {
        in.kind = JoinInput::Kind::Bad;
        in.message = "Type the host's address as IP or IP:port (for example 192.168.1.20:27420).";
        return in;
    }
    in.kind = JoinInput::Kind::Address;
    in.host = host;
    in.port = port;
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
    console_cmds::AddCommandStr("coop_joincode", &cmd_joincode);
    console_cmds::AddCommandStr("coop_joinfield", &cmd_joinfield);
}

}  // namespace dh2::coop
