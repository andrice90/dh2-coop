#include "net/upnp.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <objbase.h>
#include <oleauto.h>
#include <upnp.h>
#include <wrl/client.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace net::upnp {

using Clock = std::chrono::steady_clock;

// ---- pure helpers -------------------------------------------------------------------------------------------

bool ParseIpv4(const std::string& in, uint32_t& out) {
    size_t b = 0, e = in.size();
    while (b < e && (in[b] == ' ' || in[b] == '\t' || in[b] == '\r' || in[b] == '\n')) ++b;
    while (e > b && (in[e - 1] == ' ' || in[e - 1] == '\t' || in[e - 1] == '\r' || in[e - 1] == '\n')) --e;
    const std::string s = in.substr(b, e - b);
    uint32_t v = 0;
    int parts = 0;
    size_t i = 0;
    while (i <= s.size()) {
        if (parts == 4) return false;
        size_t j = i;
        uint32_t n = 0;
        while (j < s.size() && s[j] >= '0' && s[j] <= '9') {
            n = n * 10 + static_cast<uint32_t>(s[j] - '0');
            if (n > 255) return false;
            ++j;
        }
        if (j == i || j - i > 3) return false;
        v = (v << 8) | n;
        ++parts;
        if (j == s.size()) break;
        if (s[j] != '.') return false;
        i = j + 1;
        if (i == s.size()) return false;
    }
    if (parts != 4) return false;
    out = v;
    return true;
}

std::string FormatIpv4(uint32_t v) {
    char buf[20];
    std::snprintf(buf, sizeof buf, "%u.%u.%u.%u", (v >> 24) & 255, (v >> 16) & 255, (v >> 8) & 255, v & 255);
    return buf;
}

AddrKind Classify(const std::string& ipv4) {
    uint32_t v = 0;
    if (!ParseIpv4(ipv4, v)) return AddrKind::None;
    const uint32_t a = v >> 24, b = (v >> 16) & 255;
    if (a == 0 || a == 127 || a >= 224) return AddrKind::None;         // this-network, loopback, multicast, reserved
    if (a == 169 && b == 254) return AddrKind::None;                    // link-local
    if (a == 10) return AddrKind::Private;
    if (a == 172 && b >= 16 && b <= 31) return AddrKind::Private;
    if (a == 192 && b == 168) return AddrKind::Private;
    if (a == 100 && b >= 64 && b <= 127) return AddrKind::Cgnat;
    return AddrKind::Public;
}

std::string MaskIp(const std::string& ipv4) {
    uint32_t v = 0;
    if (!ParseIpv4(ipv4, v)) return "x.x.x.x";
    return std::to_string(v >> 24) + ".x.x." + std::to_string(v & 255);
}

int CodeFromHresult(int32_t hr) {
    const uint32_t u = static_cast<uint32_t>(hr);
    constexpr uint32_t kBase = 0x80040300u;  // UPNP_E_ACTION_SPECIFIC_BASE: codes 600..899
    if (u >= kBase && u <= kBase + 299u) return 600 + static_cast<int>(u - kBase);
    switch (u) {
        case 0x80040207u: return 401;  // UPNP_E_INVALID_ACTION
        case 0x80040208u: return 402;  // UPNP_E_INVALID_ARGUMENTS
        case 0x80040210u: return 501;  // UPNP_E_ACTION_REQUEST_FAILED
        default: return 0;
    }
}

Fault FaultOf(int code) {
    if (code == 0) return Fault::None;
    if (code < 0) return Fault::Transport;
    switch (code) {
        case 713:
        case 714:
        case 730: return Fault::NoSuchEntry;
        case 718:
        case 729: return Fault::Conflict;
        case 725: return Fault::PermanentOnly;
        case 724: return Fault::SamePortRequired;
        case 606: return Fault::NotAuthorized;
        case 728: return Fault::NoPortMaps;
        case 715:
        case 716:
        case 726:
        case 727:
        case 732: return Fault::WildcardOnly;
        default: return Fault::Other;
    }
}

uint32_t NormalizeLease(int raw) {
    if (raw < 0) return 3600;
    if (raw == 0) return 0;
    return static_cast<uint32_t>(std::clamp(raw, 120, 86400));
}

uint32_t RenewDelaySec(uint32_t leaseSec) {
    if (leaseSec == 0) return 600;
    return std::max<uint32_t>(30, leaseSec / 2);
}

uint16_t CandidatePort(uint16_t base, int attempt) {
    if (base == 0 || attempt < 0) return 0;
    const uint32_t p = static_cast<uint32_t>(base) + static_cast<uint32_t>(attempt);
    return p > 65535u ? 0 : static_cast<uint16_t>(p);
}

std::string FailureMessage(Fault fault, AddrKind kind, uint16_t port, int code) {
    if (kind == AddrKind::Cgnat) return "Your internet provider shares one address (CGNAT): join over Tailscale";
    if (kind == AddrKind::Private) return "Another router is in front (double NAT): join over Tailscale";
    if (fault == Fault::None && code == 0) return "The router has no internet address";
    switch (fault) {
        case Fault::Transport:
            return code == kErrNoRouter ? "Router has UPnP turned off" : "The router did not answer (UPnP)";
        case Fault::Conflict:
        case Fault::SamePortRequired:
            return "Port " + std::to_string(port) + " is forwarded to another device";
        case Fault::NotAuthorized: return "The router refuses port forwards from this PC (UPnP restricted)";
        case Fault::NoPortMaps: return "The router has no room for another port forward";
        default: return "The router refused the port forward (UPnP error " + std::to_string(code) + ")";
    }
}

std::string FormatState(const StateRecord& rec) {
    return "udp " + std::to_string(rec.externalPort) + " " + rec.client + "\n";
}

bool ParseState(const std::string& text, StateRecord& out) {
    std::istringstream in(text);
    std::string proto, port, client, extra;
    if (!(in >> proto >> port >> client)) return false;
    if (in >> extra) return false;
    if (proto != "udp") return false;
    if (port.empty() || port.size() > 5) return false;
    for (char c : port)
        if (c < '0' || c > '9') return false;
    const long p = std::strtol(port.c_str(), nullptr, 10);
    uint32_t ip = 0;
    if (p <= 0 || p > 65535 || !ParseIpv4(client, ip)) return false;
    out.externalPort = static_cast<uint16_t>(p);
    out.client = FormatIpv4(ip);
    return true;
}

// ---- router logic (Igd is an interface) ---------------------------------------------------------------------

namespace {

bool Cancelled(const Plan& plan) { return plan.cancelled && plan.cancelled(); }

std::string DoneMessage(const Outcome& o, const Plan& plan) {
    if (o.kind != AddrKind::Public) return FailureMessage(Fault::None, o.kind, o.externalPort, 0);
    std::string m = "Internet: open on port " + std::to_string(o.externalPort);
    const uint16_t first = plan.startPort ? plan.startPort : plan.internalPort;
    if (o.externalPort != first) m += " (" + std::to_string(first) + " was taken)";
    return m;
}

}  // namespace

Outcome EstablishMapping(Igd& igd, const Plan& plan) {
    Outcome o;
    const uint16_t start = plan.startPort ? plan.startPort : plan.internalPort;
    if (Cancelled(plan)) {
        o.cancelled = true;
        return o;
    }
    int e = igd.ExternalIp(o.publicIp);
    if (e != 0) {
        o.fault = FaultOf(e);
        o.code = e;
        o.message = FailureMessage(o.fault, AddrKind::None, start, e);
        return o;
    }
    o.kind = Classify(o.publicIp);
    if (o.kind == AddrKind::None || o.kind == AddrKind::Cgnat) {
        o.message = FailureMessage(Fault::None, o.kind, start, 0);
        return o;
    }

    for (int i = 0; i < kPortAttempts; ++i) {
        const uint16_t port = CandidatePort(start, i);
        if (port == 0) break;
        if (Cancelled(plan)) {
            o.cancelled = true;
            return o;
        }
        ++o.portsTried;
        Entry cur;
        bool ours = false;
        if (igd.GetEntry(port, cur) == 0) {
            ours = cur.client == plan.client &&
                   (cur.description == plan.description || cur.internalPort == plan.internalPort);
            if (!ours) {  // another device (or program) has this external port
                o.fault = Fault::Conflict;
                o.code = 718;
                continue;
            }
        }
        // No entry, or a router that answers a missing entry oddly: AddPortMapping gives the real verdict.
        if (plan.beforeAdd) plan.beforeAdd(port);
        uint32_t lease = plan.leaseSec;
        int a = igd.AddEntry(port, plan.internalPort, plan.client, plan.description, lease);
        if (a != 0 && FaultOf(a) == Fault::PermanentOnly && lease != 0) {
            lease = 0;
            o.permanentFallback = true;
            a = igd.AddEntry(port, plan.internalPort, plan.client, plan.description, 0);
        }
        if (a == 0) {
            o.ok = true;
            o.externalPort = port;
            o.leaseSec = lease;
            o.reusedStale = ours;
            o.fault = Fault::None;
            o.code = 0;
            Entry chk;
            o.verified = igd.GetEntry(port, chk) == 0 && chk.client == plan.client &&
                         chk.internalPort == plan.internalPort;
            o.message = DoneMessage(o, plan);
            return o;
        }
        o.fault = FaultOf(a);
        o.code = a;
        if (o.fault != Fault::Conflict) break;  // only a taken port is worth another try
    }
    o.message = FailureMessage(o.fault == Fault::None ? Fault::Conflict : o.fault, AddrKind::Public, start, o.code);
    return o;
}

int RenewMapping(Igd& igd, const Plan& plan, const Outcome& out) {
    int a = igd.AddEntry(out.externalPort, plan.internalPort, plan.client, plan.description, out.leaseSec);
    if (a != 0 && FaultOf(a) == Fault::PermanentOnly && out.leaseSec != 0)
        a = igd.AddEntry(out.externalPort, plan.internalPort, plan.client, plan.description, 0);
    return a;
}

Removal RemoveMapping(Igd& igd, uint16_t externalPort, const std::string& client, const std::string& description,
                      bool requireDescription, int* code) {
    if (code) *code = 0;
    Entry cur;
    const int g = igd.GetEntry(externalPort, cur);
    if (g == 0) {
        if (cur.client != client) return Removal::NotOurs;
        if (requireDescription && cur.description != description) return Removal::NotOurs;
    } else if (FaultOf(g) == Fault::NoSuchEntry) {
        return Removal::AlreadyGone;
    }
    // A failed query is not a reason to leave the port open: try the delete anyway.
    const int d = igd.DeleteEntry(externalPort);
    if (d == 0) return Removal::Removed;
    if (FaultOf(d) == Fault::NoSuchEntry) return Removal::AlreadyGone;
    if (code) *code = d;
    return Removal::Failed;
}

Removal RemoveLeftover(Igd& igd, const StateRecord& rec, const std::string& description) {
    return RemoveMapping(igd, rec.externalPort, rec.client, description, true, nullptr);
}

int RemoveAllOurs(Igd& igd, const std::string& client, const std::string& description, uint16_t keepExternalPort) {
    std::vector<uint16_t> mine;
    for (int i = 0; i < 128; ++i) {
        uint16_t ext = 0;
        Entry e;
        if (igd.EnumEntry(i, ext, e) != 0) break;
        if (e.client == client && e.description == description && ext != keepExternalPort) mine.push_back(ext);
    }
    int removed = 0;
    for (uint16_t ext : mine) {
        if (igd.DeleteEntry(ext) == 0) ++removed;
    }
    return removed;
}

// ---- SOAP over plain HTTP -----------------------------------------------------------------------------------

// The router is controlled with hand-written SOAP over an ordinary outbound TCP connection (no listening socket).
// Windows' own UPnP control layer (IUPnPService::InvokeAction) cannot do it: it treats an HTTP 200 reply that holds
// a SOAP fault as a broken response (UPNP_E_ERROR_PROCESSING_RESPONSE), and eero routers send every UPnP error that
// way, so it could not tell "no such entry" (714) or "leases refused" (725) from a dead router. eero's replies are
// HTTP/1.0 without Content-Length (the body ends when the connection closes), and its GetGenericPortMappingEntry
// closes the connection without any reply. Windows COM is used for the discovery only (see OpenRouter).

namespace {

std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

std::string Lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

uint32_t ToUint(const std::string& s) { return static_cast<uint32_t>(std::strtoul(s.c_str(), nullptr, 10)); }

bool ToBool(const std::string& s) {
    const std::string l = Lower(Trim(s));
    return !(l.empty() || l == "0" || l == "false" || l == "no");
}

// Chunked transfer coding. True when the final (zero) chunk has arrived.
bool DecodeChunked(const std::string& raw, std::string& out) {
    out.clear();
    size_t pos = 0;
    for (;;) {
        const size_t eol = raw.find("\r\n", pos);
        if (eol == std::string::npos) return false;
        const unsigned long n = std::strtoul(raw.c_str() + pos, nullptr, 16);  // chunk extensions are ignored
        pos = eol + 2;
        if (n == 0) return true;
        if (raw.size() < pos + n + 2) return false;
        out.append(raw, pos, n);
        pos += n + 2;
    }
}

void EnsureWinsock() {
    static std::once_flag once;
    std::call_once(once, [] {
        WSADATA d;
        WSAStartup(MAKEWORD(2, 2), &d);  // counted: ENet's own start / stop does not disturb it
    });
}

struct ParsedHttp {
    bool haveHead = false;
    int status = 0;
    std::string body;
    bool complete = false;  // the body is known to be whole (Content-Length reached, last chunk seen)
};

ParsedHttp ParseHttp(const std::string& raw) {
    ParsedHttp p;
    if (raw.compare(0, 5, "HTTP/") != 0) return p;
    const size_t eol = raw.find("\r\n");
    const size_t sp = raw.find(' ');
    if (sp != std::string::npos && sp < (eol == std::string::npos ? raw.size() : eol))
        p.status = std::atoi(raw.c_str() + sp + 1);
    const size_t end = raw.find("\r\n\r\n");
    if (end == std::string::npos) return p;
    p.haveHead = true;
    const std::string head = Lower(raw.substr(0, end));
    const std::string rest = raw.substr(end + 4);
    long contentLength = -1;
    if (const size_t cl = head.find("\r\ncontent-length:"); cl != std::string::npos)
        contentLength = std::atol(head.c_str() + cl + 17);
    if (head.find("\r\ntransfer-encoding:") != std::string::npos && head.find("chunked") != std::string::npos) {
        p.complete = DecodeChunked(rest, p.body);
    } else if (contentLength >= 0) {
        p.body = rest.substr(0, std::min<size_t>(rest.size(), static_cast<size_t>(contentLength)));
        p.complete = rest.size() >= static_cast<size_t>(contentLength);
    } else {
        p.body = rest;  // ends when the connection closes
    }
    return p;
}

}  // namespace

std::string XmlEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string XmlUnescape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '&') {
            struct E {
                const char* name;
                char ch;
            };
            static const E kEntities[] = {{"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}};
            bool matched = false;
            for (const E& e : kEntities) {
                const size_t n = std::strlen(e.name);
                if (s.compare(i, n, e.name) == 0) {
                    out += e.ch;
                    i += n - 1;
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
        }
        out += s[i];
    }
    return out;
}

bool XmlValue(const std::string& xml, const std::string& tag, std::string& out) {
    const std::string open = "<" + tag;
    size_t pos = 0;
    while ((pos = xml.find(open, pos)) != std::string::npos) {
        const size_t after = pos + open.size();
        if (after >= xml.size()) return false;
        const char c = xml[after];
        if (c == '>' || c == ' ' || c == '/' || c == '\t' || c == '\r' || c == '\n') {
            const size_t gt = xml.find('>', after);
            if (gt == std::string::npos) return false;
            if (xml[gt - 1] == '/') {  // <tag/>
                out.clear();
                return true;
            }
            const size_t close = xml.find("</" + tag + ">", gt + 1);
            if (close == std::string::npos) return false;
            out = XmlUnescape(Trim(xml.substr(gt + 1, close - gt - 1)));
            return true;
        }
        pos = after;  // another tag that starts with the same letters
    }
    return false;
}

int SoapFaultCode(const std::string& body) {
    std::string v;
    if (XmlValue(body, "errorCode", v)) {
        const int code = std::atoi(v.c_str());
        return code > 0 ? code : 501;
    }
    return body.find("Fault>") != std::string::npos ? 501 : 0;
}

std::string BuildSoap(const std::string& serviceType, const std::string& action, const std::string& argsXml) {
    return "<?xml version=\"1.0\"?>\r\n<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
           "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:" +
           action + " xmlns:u=\"" + serviceType + "\">" + argsXml + "</u:" + action + "></s:Body></s:Envelope>";
}

bool ParseUrl(const std::string& text, Url& out) {
    const std::string t = Trim(text);
    if (Lower(t.substr(0, 7)) != "http://") return false;
    size_t i = 7;
    size_t hostEnd = i;
    while (hostEnd < t.size() && t[hostEnd] != ':' && t[hostEnd] != '/' && t[hostEnd] != '?') ++hostEnd;
    if (hostEnd == i) return false;
    Url u;
    u.host = t.substr(i, hostEnd - i);
    i = hostEnd;
    if (i < t.size() && t[i] == ':') {
        size_t j = i + 1;
        long port = 0;
        while (j < t.size() && t[j] >= '0' && t[j] <= '9') {
            port = port * 10 + (t[j] - '0');
            if (port > 65535) return false;
            ++j;
        }
        if (j == i + 1 || port == 0) return false;
        u.port = static_cast<uint16_t>(port);
        i = j;
    }
    u.path = i < t.size() ? t.substr(i) : "/";
    if (u.path[0] != '/') u.path = "/" + u.path;
    out = u;
    return true;
}

std::string ResolveUrl(const std::string& base, const std::string& ref) {
    const std::string r = Trim(ref);
    if (r.empty()) return {};
    if (Lower(r.substr(0, 7)) == "http://") return r;
    Url b;
    if (!ParseUrl(base, b)) return {};
    const std::string origin = "http://" + b.host + (b.port != 80 ? ":" + std::to_string(b.port) : std::string());
    if (r[0] == '/') return origin + r;
    const size_t slash = b.path.rfind('/');
    return origin + b.path.substr(0, slash + 1) + r;
}

int ServiceScore(const std::string& type) {
    if (type == "urn:schemas-upnp-org:service:WANIPConnection:2") return 3;
    if (type == "urn:schemas-upnp-org:service:WANIPConnection:1") return 2;
    if (type == "urn:schemas-upnp-org:service:WANPPPConnection:1") return 1;
    return 0;
}

std::vector<ServiceDesc> ParseDescription(const std::string& xml, const std::string& documentUrl) {
    std::vector<ServiceDesc> out;
    std::string base;
    if (!XmlValue(xml, "URLBase", base) || Lower(Trim(base).substr(0, 7)) != "http://") base = documentUrl;
    size_t pos = 0;
    while ((pos = xml.find("<service>", pos)) != std::string::npos) {
        const size_t end = xml.find("</service>", pos);
        if (end == std::string::npos) break;
        const std::string block = xml.substr(pos, end - pos);
        std::string type, ctrl;
        if (XmlValue(block, "serviceType", type) && XmlValue(block, "controlURL", ctrl)) {
            const std::string abs = ResolveUrl(base, ctrl);
            if (!abs.empty()) out.push_back(ServiceDesc{type, abs});
        }
        pos = end;
    }
    return out;
}

HttpReply HttpExchange(const Url& url, const std::string& method, const std::string& headers, const std::string& body,
                       uint32_t timeoutMs) {
    HttpReply reply;
    EnsureWinsock();
    const auto deadline = Clock::now() + std::chrono::milliseconds(timeoutMs);
    const auto remaining = [&]() -> int {
        const auto r = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count();
        return r > 0 ? static_cast<int>(r) : 0;
    };

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo* res = nullptr;
    if (getaddrinfo(url.host.c_str(), std::to_string(url.port).c_str(), &hints, &res) != 0 || !res) return reply;
    SOCKET s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s == INVALID_SOCKET) {
        freeaddrinfo(res);
        return reply;
    }
    struct Closer {
        SOCKET s;
        ~Closer() { closesocket(s); }
    } closer{s};

    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    const int rc = connect(s, res->ai_addr, static_cast<int>(res->ai_addrlen));
    freeaddrinfo(res);
    if (rc != 0) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) return reply;
        fd_set wr, ex;
        FD_ZERO(&wr);
        FD_ZERO(&ex);
        FD_SET(s, &wr);
        FD_SET(s, &ex);
        const int rem = remaining();
        timeval tv{rem / 1000, (rem % 1000) * 1000};
        if (select(0, nullptr, &wr, &ex, &tv) <= 0 || !FD_ISSET(s, &wr)) return reply;
        int err = 0, len = sizeof err;
        getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
        if (err != 0) return reply;
    }
    nonBlocking = 0;
    ioctlsocket(s, FIONBIO, &nonBlocking);

    std::string req = method + " " + url.path + " HTTP/1.1\r\nHost: " + url.host + ":" + std::to_string(url.port) + "\r\n";
    if (!body.empty()) req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    req += headers + "Connection: close\r\n\r\n" + body;
    for (size_t sent = 0; sent < req.size();) {
        const DWORD to = static_cast<DWORD>(std::max(remaining(), 1));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&to), sizeof to);
        const int n = send(s, req.data() + sent, static_cast<int>(req.size() - sent), 0);
        if (n <= 0) return reply;
        sent += static_cast<size_t>(n);
    }

    std::string raw;
    for (;;) {
        const int rem = remaining();
        if (rem == 0) break;
        const DWORD to = static_cast<DWORD>(std::min(rem, 1000));
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&to), sizeof to);
        char buf[4096];
        const int n = recv(s, buf, sizeof buf, 0);
        if (n > 0) {
            raw.append(buf, static_cast<size_t>(n));
            if (ParseHttp(raw).complete) break;
            continue;
        }
        if (n == 0) break;  // closed: the body is whatever came
        if (WSAGetLastError() == WSAETIMEDOUT) continue;
        break;
    }
    const ParsedHttp p = ParseHttp(raw);
    reply.ok = p.status > 0;
    reply.status = p.status;
    reply.body = p.body;
    return reply;
}

namespace {

std::string Arg(const char* name, const std::string& value) {
    return std::string("<") + name + ">" + XmlEscape(value) + "</" + name + ">";
}
std::string Arg(const char* name, uint32_t value) { return Arg(name, std::to_string(value)); }

class SoapIgd final : public Igd {
public:
    SoapIgd(Url control, std::string serviceType, uint32_t timeoutMs)
        : url_(std::move(control)), type_(std::move(serviceType)), timeoutMs_(timeoutMs) {}

    int ExternalIp(std::string& ip) override {
        std::string body;
        int e = Call("GetExternalIPAddress", "", body);
        if (e == 0 && !XmlValue(body, "NewExternalIPAddress", ip)) e = kErrTransport;
        Note("GetExternalIPAddress", e == 0 ? MaskIp(ip) : std::string(), e, e == 0);
        return e;
    }

    int GetEntry(uint16_t ext, Entry& out) override {
        std::string body;
        int e = Call("GetSpecificPortMappingEntry", Arg("NewRemoteHost", "") + Arg("NewExternalPort", ext) + Arg("NewProtocol", "UDP"),
                     body);
        if (e == 0) {
            std::string port, enabled, lease;
            if (!XmlValue(body, "NewInternalPort", port) || !XmlValue(body, "NewInternalClient", out.client)) {
                e = kErrTransport;
            } else {
                out.internalPort = static_cast<uint16_t>(ToUint(port));
                out.enabled = !XmlValue(body, "NewEnabled", enabled) || ToBool(enabled);
                out.description.clear();
                XmlValue(body, "NewPortMappingDescription", out.description);
                out.leaseSec = XmlValue(body, "NewLeaseDuration", lease) ? ToUint(lease) : 0;
            }
        }
        Note("GetSpecificPortMappingEntry", "UDP ext " + std::to_string(ext), e, e == 0 || FaultOf(e) == Fault::NoSuchEntry);
        return e;
    }

    int AddEntry(uint16_t ext, uint16_t internal, const std::string& client, const std::string& description,
                 uint32_t lease) override {
        std::string body;
        const int e = Call("AddPortMapping",
                           Arg("NewRemoteHost", "") + Arg("NewExternalPort", ext) + Arg("NewProtocol", "UDP") +
                               Arg("NewInternalPort", internal) + Arg("NewInternalClient", client) +
                               Arg("NewEnabled", 1u) + Arg("NewPortMappingDescription", description) +
                               Arg("NewLeaseDuration", lease),
                           body);
        char what[96];
        std::snprintf(what, sizeof what, "UDP ext %u -> %s:%u lease %u", ext, client.c_str(), internal, lease);
        Note("AddPortMapping", what, e, true);
        return e;
    }

    int DeleteEntry(uint16_t ext) override {
        std::string body;
        const int e = Call("DeletePortMapping",
                           Arg("NewRemoteHost", "") + Arg("NewExternalPort", ext) + Arg("NewProtocol", "UDP"), body);
        Note("DeletePortMapping", "UDP ext " + std::to_string(ext), e, true);
        return e;
    }

    int EnumEntry(int index, uint16_t& ext, Entry& out) override {
        std::string body;
        const int e = Call("GetGenericPortMappingEntry", Arg("NewPortMappingIndex", static_cast<uint32_t>(index)), body);
        if (e != 0) return e;
        std::string port, proto, internal, enabled, lease;
        if (!XmlValue(body, "NewExternalPort", port) || !XmlValue(body, "NewInternalClient", out.client)) return kErrTransport;
        ext = static_cast<uint16_t>(ToUint(port));
        XmlValue(body, "NewProtocol", proto);
        XmlValue(body, "NewInternalPort", internal);
        out.internalPort = static_cast<uint16_t>(ToUint(internal));
        out.enabled = !XmlValue(body, "NewEnabled", enabled) || ToBool(enabled);
        out.description.clear();
        XmlValue(body, "NewPortMappingDescription", out.description);
        if (Lower(Trim(proto)) != "udp") out.description = "(not udp)";  // never ours
        out.leaseSec = XmlValue(body, "NewLeaseDuration", lease) ? ToUint(lease) : 0;
        return 0;
    }

private:
    int Call(const char* action, const std::string& args, std::string& bodyOut) {
        const std::string headers = "Content-Type: text/xml; charset=\"utf-8\"\r\nSOAPAction: \"" + type_ + "#" + action + "\"\r\n";
        const auto t0 = Clock::now();
        HttpReply r = HttpExchange(url_, "POST", headers, BuildSoap(type_, action, args), timeoutMs_);
        lastMs_ = static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count());
        if (!r.ok) return kErrTransport;  // no answer, or the connection closed without one
        bodyOut = std::move(r.body);
        if (const int fault = SoapFaultCode(bodyOut)) return fault;
        if (r.status == 500) return 501;
        if (r.status < 200 || r.status >= 300) return kErrTransport;
        return 0;
    }

    void Note(const char* action, const std::string& what, int code, bool expected) {
        if (code == 0) spdlog::info("upnp: {} {} -> ok ({} ms)", action, what, lastMs_);
        else if (expected) spdlog::info("upnp: {} {} -> UPnP error {} ({} ms)", action, what, code, lastMs_);
        else spdlog::warn("upnp: {} {} -> failed ({}) ({} ms)", action, what, code, lastMs_);
    }

    Url url_;
    std::string type_;
    uint32_t timeoutMs_;
    long lastMs_ = 0;
};

}  // namespace

std::unique_ptr<Igd> MakeSoapIgd(const Url& control, const std::string& serviceType, uint32_t timeoutMs) {
    return std::make_unique<SoapIgd>(control, serviceType, timeoutMs);
}

// ---- discovery (Windows UPnP COM) ---------------------------------------------------------------------------

namespace {

using Microsoft::WRL::ComPtr;

std::string Narrow(const wchar_t* w, size_t len) {
    if (!w || len == 0) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n > 0 ? n : 0), '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w, static_cast<int>(len), s.data(), n, nullptr, nullptr);
    return s;
}

std::string TakeBstr(BSTR b) {
    std::string s = b ? Narrow(b, SysStringLen(b)) : std::string();
    SysFreeString(b);
    return s;
}

std::string Hex(HRESULT hr) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%08x", static_cast<unsigned>(hr));
    return buf;
}

// Calls fn(element) for every item of a UPnP collection (IUPnPDevices) through its IEnumVARIANT.
template <class Elem, class Coll, class Fn>
void ForEach(Coll* coll, Fn fn) {
    if (!coll) return;
    ComPtr<IUnknown> unk;
    if (FAILED(coll->get__NewEnum(&unk)) || !unk) return;
    ComPtr<IEnumVARIANT> en;
    if (FAILED(unk.As(&en))) return;
    for (;;) {
        VARIANT v;
        VariantInit(&v);
        ULONG got = 0;
        if (en->Next(1, &v, &got) != S_OK || got != 1) break;
        ComPtr<Elem> item;
        if ((v.vt == VT_UNKNOWN || v.vt == VT_DISPATCH) && v.punkVal)
            v.punkVal->QueryInterface(__uuidof(Elem), reinterpret_cast<void**>(item.GetAddressOf()));
        VariantClear(&v);
        if (item) fn(item.Get());
    }
}

}  // namespace

// The SSDP Discovery service does the multicast search for us (IUPnPDeviceFinder talks to it), so this process opens
// no listening socket and Windows Firewall has nothing to ask about. COM only finds the router and tells us the URL of
// its device description; everything after that is the SOAP client above.
std::unique_ptr<Igd> OpenRouter(RouterInfo& info, std::string& why) {
    info = RouterInfo{};
    struct Doc {
        std::string url;
        std::string name;
    };
    std::vector<Doc> docs;
    HRESULT lastFail = S_OK;
    {
        ComPtr<IUPnPDeviceFinder> finder;
        const HRESULT hr = CoCreateInstance(__uuidof(UPnPDeviceFinder), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&finder));
        if (FAILED(hr) || !finder) {
            why = "UPnP device finder unavailable (" + Hex(hr) + ")";
            return nullptr;
        }
        static const wchar_t* const kTypes[] = {L"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
                                                L"urn:schemas-upnp-org:device:InternetGatewayDevice:2"};
        for (const wchar_t* type : kTypes) {
            BSTR bt = SysAllocString(type);
            ComPtr<IUPnPDevices> devices;
            const HRESULT fr = finder->FindByType(bt, 0, &devices);
            SysFreeString(bt);
            if (FAILED(fr)) {
                lastFail = fr;
                continue;
            }
            ForEach<IUPnPDevice>(devices.Get(), [&](IUPnPDevice* d) {
                ++info.devicesFound;
                BSTR n = nullptr;
                std::string name;
                if (SUCCEEDED(d->get_FriendlyName(&n))) name = TakeBstr(n);
                ComPtr<IUPnPDeviceDocumentAccess> access;
                if (FAILED(d->QueryInterface(__uuidof(IUPnPDeviceDocumentAccess), reinterpret_cast<void**>(access.GetAddressOf()))) ||
                    !access)
                    return;
                BSTR u = nullptr;
                if (FAILED(access->GetDocumentURL(&u))) return;
                const std::string url = TakeBstr(u);
                for (const Doc& have : docs)
                    if (have.url == url) return;
                docs.push_back(Doc{url, name});
            });
            if (!docs.empty()) break;
        }
    }
    if (docs.empty()) {
        why = FAILED(lastFail) ? "UPnP search failed (" + Hex(lastFail) + ")"
                               : (info.devicesFound ? "the router gave no device description" : "no router answered");
        return nullptr;
    }
    why = "the router has no usable WAN connection service";
    for (const Doc& doc : docs) {
        Url docUrl;
        if (!ParseUrl(doc.url, docUrl)) continue;
        const HttpReply r = HttpExchange(docUrl, "GET", "", "", 4000);
        if (!r.ok || r.status != 200) {
            why = "the router's device description could not be read";
            continue;
        }
        std::vector<ServiceDesc> services = ParseDescription(r.body, doc.url);
        std::stable_sort(services.begin(), services.end(), [](const ServiceDesc& a, const ServiceDesc& b) {
            return ServiceScore(a.type) > ServiceScore(b.type);
        });
        for (const ServiceDesc& s : services) {
            if (ServiceScore(s.type) == 0) continue;
            Url control;
            if (!ParseUrl(s.controlUrl, control)) continue;
            auto igd = MakeSoapIgd(control, s.type);
            std::string ip;
            if (igd->ExternalIp(ip) != 0 || ip.empty()) {  // not the connected one, try the next
                why = "the router did not answer GetExternalIPAddress";
                continue;
            }
            info.name = doc.name;
            info.serviceType = s.type;
            info.gatewayHost = docUrl.host;
            info.controlUrl = s.controlUrl;
            return igd;
        }
    }
    return nullptr;
}

// The next hop of the default route ("" = none): a routing-table lookup only, nothing is sent.
static std::string DefaultGatewayIpv4() {
    MIB_IPFORWARDROW row{};
    if (GetBestRoute(_byteswap_ulong(0x01010101u), 0, &row) != NO_ERROR || row.dwForwardNextHop == 0) return {};
    return FormatIpv4(_byteswap_ulong(row.dwForwardNextHop));
}

std::string LocalAddressToward(const std::string& gatewayIp) {
    uint32_t gw = 0;
    const bool haveGw = ParseIpv4(gatewayIp, gw);
    const uint32_t destHost = haveGw ? gw : 0x01010101u;  // a routing-table lookup only: nothing is sent
    DWORD ifIndex = 0;
    if (GetBestInterface(_byteswap_ulong(destHost), &ifIndex) != NO_ERROR) ifIndex = 0;

    ULONG size = 16 * 1024;
    std::vector<unsigned char> buf;
    ULONG r = ERROR_BUFFER_OVERFLOW;
    for (int tries = 0; tries < 3 && r == ERROR_BUFFER_OVERFLOW; ++tries) {
        buf.resize(size);
        r = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                 nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()), &size);
    }
    if (r != NO_ERROR) return {};
    std::string fallback;
    for (auto* a = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()); a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (ifIndex && a->IfIndex != ifIndex) continue;
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            const auto* sin = reinterpret_cast<const sockaddr_in*>(u->Address.lpSockaddr);
            if (!sin || sin->sin_family != AF_INET) continue;
            const uint32_t ip = _byteswap_ulong(sin->sin_addr.S_un.S_addr);
            if (ip == 0 || (ip >> 24) == 127 || (ip >> 16) == 0xa9fe) continue;
            if (haveGw && u->OnLinkPrefixLength > 0 && u->OnLinkPrefixLength <= 32) {
                const uint32_t mask = u->OnLinkPrefixLength == 32 ? 0xffffffffu : ~(0xffffffffu >> u->OnLinkPrefixLength);
                if ((ip & mask) == (gw & mask)) return FormatIpv4(ip);
            }
            if (fallback.empty()) fallback = FormatIpv4(ip);
        }
    }
    return fallback;
}

// ---- service: one worker thread -----------------------------------------------------------------------------

namespace {

struct Shared {
    std::mutex mu;
    std::condition_variable cv;      // wakes the worker
    std::condition_variable doneCv;  // wakes Shutdown
    Config cfg;
    uint16_t wantPort = 0;           // 0 = not hosting
    bool userOff = false;
    bool exitRequested = false;
    bool cleanRequested = false;
    bool refreshRequested = false;
    bool started = false;
    bool done = false;
    Status status;
    std::string offMessage = "Internet: off";
    Opener opener;  // tests only
};

// Leaked on purpose: the worker may still run when static destructors do (process exit), and a destroyed mutex
// there is a crash.
Shared& G() {
    static Shared* g = new Shared;
    return *g;
}

void SetStatus(Shared& g, State state, const std::string& message) {
    std::lock_guard lk(g.mu);
    g.status.state = state;
    g.status.message = message;
}

std::string ReadFile(const std::string& path) {
    if (path.empty()) return {};
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

class Worker {
public:
    explicit Worker(bool comOk) : comOk_(comOk) {}

    void Run() {
        Shared& g = G();
        for (;;) {
            bool exit = false, clean = false, refresh = false;
            uint16_t want = 0;
            {
                std::unique_lock lk(g.mu);
                auto changed = [&] {
                    const uint16_t w = g.userOff ? 0 : g.wantPort;
                    return g.exitRequested || g.cleanRequested || g.refreshRequested || w != applied_;
                };
                if (!changed()) {
                    if (wake_ == Clock::time_point::max()) g.cv.wait(lk, changed);
                    else g.cv.wait_until(lk, wake_, changed);
                }
                exit = g.exitRequested;
                clean = g.cleanRequested;
                refresh = g.refreshRequested;
                want = g.userOff ? 0 : g.wantPort;
                g.cleanRequested = false;
                g.refreshRequested = false;
            }
            try {
                if (exit) {
                    Teardown("process exit");
                    return;
                }
                if (clean) DoClean();
                if (want != applied_ || (refresh && want != 0)) {
                    if (live_) Teardown(want == 0 ? "hosting stopped" : "port changed");
                    applied_ = want;
                    wake_ = Clock::time_point::max();
                    if (want != 0) Establish(want);
                    else GoOff();
                } else if (want != 0 && Clock::now() >= wake_) {
                    if (live_) Renew();
                    else Establish(want);
                }
            } catch (const std::exception& ex) {
                spdlog::error("upnp: worker error: {}", ex.what());
                live_.reset();
                SetStatus(G(), State::Failed, "UPnP hit an error");
                wake_ = Clock::now() + std::chrono::seconds(300);
            } catch (...) {
                spdlog::error("upnp: worker error (unknown)");
                live_.reset();
                SetStatus(G(), State::Failed, "UPnP hit an error");
                wake_ = Clock::now() + std::chrono::seconds(300);
            }
        }
    }

private:
    struct Live {
        std::unique_ptr<Igd> igd;
        Plan plan;
        Outcome out;
        RouterInfo info;
    };

    static Clock::duration RenewDelay(uint32_t leaseSec, uint32_t overrideMs) {
        if (overrideMs) return std::chrono::milliseconds(overrideMs);
        return std::chrono::seconds(RenewDelaySec(leaseSec));
    }

    static uint32_t RenewOverrideMs() {
        std::lock_guard lk(G().mu);
        return G().cfg.renewMsForTest;
    }

    // The router found last time answers again in a few milliseconds; the search (SSDP through Windows) takes ~9 s.
    // Only while it is still this PC's default gateway and its control URL points at that same address: after a
    // network change another device could sit at the old control address, so then search again.
    std::unique_ptr<Igd> Open(RouterInfo& info, std::string& why) {
        Opener opener;
        {
            std::lock_guard lk(G().mu);
            opener = G().opener;
        }
        if (opener) return opener(info, why);
        Url control;
        if (!cachedInfo_.controlUrl.empty() &&
            (!ParseUrl(cachedInfo_.controlUrl, control) || control.host != cachedInfo_.gatewayHost ||
             DefaultGatewayIpv4() != cachedInfo_.gatewayHost)) {
            spdlog::info("upnp: the default gateway changed since the last search; searching again");
            cachedInfo_ = RouterInfo{};
        }
        if (!cachedInfo_.controlUrl.empty()) {
            auto igd = MakeSoapIgd(control, cachedInfo_.serviceType, 2000);
            std::string ip;
            if (igd->ExternalIp(ip) == 0 && !ip.empty()) {
                info = cachedInfo_;
                spdlog::info("upnp: router '{}' again, no new search", info.name);
                return MakeSoapIgd(control, cachedInfo_.serviceType);
            }
            spdlog::info("upnp: the router found last time does not answer; searching again");
            cachedInfo_ = RouterInfo{};
        }
        auto igd = OpenRouter(info, why);
        if (igd) cachedInfo_ = info;
        return igd;
    }

    bool StillWanted(uint16_t port) const {
        Shared& g = G();
        std::lock_guard lk(g.mu);
        return !g.exitRequested && !g.userOff && g.wantPort == port;
    }

    void Fail(uint16_t port, const std::string& message, AddrKind kind = AddrKind::None, int retrySec = 300) {
        Shared& g = G();
        {
            std::lock_guard lk(g.mu);
            if (g.status.state != State::Failed || g.status.message != message)
                spdlog::info("upnp: port {} -> {}", port, message);
            g.status.state = State::Failed;
            g.status.message = message;
            g.status.mapped = false;
            g.status.externalPort = 0;
            g.status.publicIp.clear();
            g.status.kind = kind;
        }
        wake_ = Clock::now() + std::chrono::seconds(retrySec);
    }

    void GoOff() {
        Shared& g = G();
        std::lock_guard lk(g.mu);
        g.status = Status{};
        g.status.message = g.offMessage;
    }

    void WriteState(const StateRecord& rec) {
        std::string path;
        {
            std::lock_guard lk(G().mu);
            path = G().cfg.stateFile;
        }
        if (path.empty()) return;
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (out) out << FormatState(rec);
    }

    void EraseState() {
        std::string path;
        {
            std::lock_guard lk(G().mu);
            path = G().cfg.stateFile;
        }
        if (!path.empty()) std::remove(path.c_str());
    }

    void Establish(uint16_t port) {
        Shared& g = G();
        Config cfg;
        {
            std::lock_guard lk(g.mu);
            cfg = g.cfg;
            g.status = Status{};
            g.status.state = State::Working;
            g.status.internalPort = port;
            g.status.leaseSec = cfg.leaseSec;
            g.status.message = "Internet: asking the router to open port " + std::to_string(port) + "...";
        }
        if (!comOk_) return Fail(port, "UPnP is not available on this PC");
        const auto t0 = Clock::now();

        auto live = std::make_unique<Live>();
        std::string why;
        live->igd = Open(live->info, why);
        const auto ms = [&] { return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count(); };
        if (!live->igd) {
            spdlog::info("upnp: no router for UPnP in {} ms: {}", ms(), why);
            return Fail(port, why.rfind("UPnP device finder unavailable", 0) == 0 ? "UPnP is not available on this PC"
                                                                                     : "Router has UPnP turned off");
        }
        spdlog::info("upnp: router '{}' ({}), {} device(s) found, {} ms", live->info.name, live->info.serviceType,
                     live->info.devicesFound, ms());
        if (!StillWanted(port)) return;

        std::string client;
        {
            const uint32_t bound = [&] {
                uint32_t v = 0;
                return ParseIpv4(cfg.bindAddress, v) ? v : 0u;
            }();
            client = bound ? cfg.bindAddress : LocalAddressToward(live->info.gatewayHost);
        }
        if (client.empty()) return Fail(port, "Could not find this PC's network address");

        // The previous run ended without removing its mapping (crash, kill): that exact one goes first.
        if (const std::string text = ReadFile(cfg.stateFile); !text.empty()) {
            StateRecord rec;
            if (ParseState(text, rec)) {
                int code = 0;
                const Removal r = RemoveMapping(*live->igd, rec.externalPort, rec.client, cfg.description, true, &code);
                static const char* const kWhat[] = {"removed", "already gone", "belongs to something else now", "failed"};
                spdlog::info("upnp: leftover mapping from an earlier run (UDP {} -> {}): {}", rec.externalPort,
                             rec.client, kWhat[static_cast<int>(r)]);
                if (r != Removal::Failed) EraseState();
            } else {
                EraseState();
            }
        }

        live->plan.internalPort = port;
        live->plan.client = client;
        live->plan.description = cfg.description;
        live->plan.leaseSec = cfg.leaseSec;
        live->plan.cancelled = [this, port] { return !StillWanted(port); };
        live->plan.beforeAdd = [this, client](uint16_t ext) { WriteState(StateRecord{ext, client}); };
        live->out = EstablishMapping(*live->igd, live->plan);
        const Outcome& o = live->out;

        if (o.cancelled) {
            EraseState();
            return;
        }
        if (!o.ok) {
            EraseState();
            spdlog::info("upnp: no port forward ({} tries, wan {}, {}): {}", o.portsTried, MaskIp(o.publicIp),
                         o.kind == AddrKind::Public ? "public" : o.kind == AddrKind::Private ? "private"
                         : o.kind == AddrKind::Cgnat ? "cgnat" : "none",
                         o.message);
            // Keep what is known about the address: CGNAT / double NAT is useful to show.
            Fail(port, o.message, o.kind);
            return;
        }
        spdlog::info("upnp: forwarding UDP {} -> {}:{} (wan {}, lease {}{}{}{}) in {} ms", o.externalPort, client, port,
                     MaskIp(o.publicIp), o.leaseSec == 0 ? std::string("permanent") : std::to_string(o.leaseSec) + " s",
                     o.permanentFallback ? ", router refuses leases: a crash leaves it open until the next launch" : "",
                     o.reusedStale ? ", reused an earlier mapping" : "", o.verified ? ", verified" : ", NOT verified",
                     ms());
        if (o.kind == AddrKind::Private)
            spdlog::info("upnp: the router's address {} is private: another router is in front (double NAT)",
                         MaskIp(o.publicIp));
        {
            std::lock_guard lk(g.mu);
            g.status.state = o.reachable() ? State::Open : State::Failed;
            g.status.mapped = true;
            g.status.externalPort = o.externalPort;
            g.status.internalPort = port;
            g.status.publicIp = o.publicIp;
            g.status.kind = o.kind;
            g.status.leaseSec = o.leaseSec;
            g.status.message = o.message;
        }
        wake_ = Clock::now() + RenewDelay(o.leaseSec, cfg.renewMsForTest);
        live_ = std::move(live);
    }

    void Renew() {
        Live& l = *live_;
        int code = 0;
        std::string ip;
        if (l.igd->ExternalIp(ip) == 0 && ip != l.out.publicIp) {
            spdlog::info("upnp: the router's address changed ({} -> {})", MaskIp(l.out.publicIp), MaskIp(ip));
            l.out.publicIp = ip;
            l.out.kind = Classify(ip);
        }
        if (l.out.leaseSec == 0) {  // permanent: just make sure it is still there
            Entry e;
            code = l.igd->GetEntry(l.out.externalPort, e);
            if (code == 0 && e.client != l.plan.client) code = 718;
        } else {
            code = RenewMapping(*l.igd, l.plan, l.out);
        }
        if (code == 0) {
            spdlog::info("upnp: renewed UDP {} (lease {} s)", l.out.externalPort, l.out.leaseSec);
            {
                std::lock_guard lk(G().mu);
                G().status.publicIp = l.out.publicIp;
                G().status.kind = l.out.kind;
                G().status.state = l.out.reachable() ? State::Open : State::Failed;
                G().status.message = DoneMessage(l.out, l.plan);
            }
            wake_ = Clock::now() + RenewDelay(l.out.leaseSec, RenewOverrideMs());
            return;
        }
        spdlog::warn("upnp: renewing UDP {} failed ({}); searching for the router again", l.out.externalPort, code);
        const uint16_t port = applied_;
        live_.reset();  // the router may have restarted: a fresh search, no delete (it would not answer either)
        Establish(port);
    }

    void Teardown(const char* why) {
        if (!live_) return;
        const auto t0 = Clock::now();
        int code = 0;
        Removal r = Removal::Failed;
        try {
            r = RemoveMapping(*live_->igd, live_->out.externalPort, live_->plan.client, live_->plan.description, false,
                              &code);
        } catch (...) {
        }
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
        static const char* const kWhat[] = {"removed", "was already gone", "belongs to something else now (left alone)",
                                            "FAILED"};
        spdlog::info("upnp: UDP {} {} ({}) in {} ms{}", live_->out.externalPort, kWhat[static_cast<int>(r)], why, ms,
                     r == Removal::Failed ? "; it expires with its lease" : "");
        if (r != Removal::Failed) EraseState();
        live_.reset();
        GoOff();
    }

    void DoClean() {
        std::unique_ptr<Igd> own;
        Igd* igd = nullptr;
        std::string client;
        uint16_t keep = 0;
        std::string desc;
        {
            std::lock_guard lk(G().mu);
            desc = G().cfg.description;
        }
        if (live_) {
            igd = live_->igd.get();
            client = live_->plan.client;
            keep = live_->out.externalPort;
        } else if (comOk_) {
            RouterInfo info;
            std::string why;
            own = Open(info, why);
            if (!own) {
                spdlog::info("upnp: clean: no router ({})", why);
                return;
            }
            igd = own.get();
            client = LocalAddressToward(info.gatewayHost);
        }
        if (!igd || client.empty()) return;
        const int n = RemoveAllOurs(*igd, client, desc, keep);
        spdlog::info("upnp: clean: removed {} leftover '{}' mapping(s) pointing at {}", n, desc, client);
    }

    bool comOk_;
    RouterInfo cachedInfo_;
    std::unique_ptr<Live> live_;
    uint16_t applied_ = 0;
    Clock::time_point wake_ = Clock::time_point::max();
};

DWORD WINAPI WorkerThread(LPVOID) {
    const HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool ok = SUCCEEDED(co);
    if (!ok) spdlog::warn("upnp: CoInitializeEx failed ({})", Hex(co));
    try {
        Worker w(ok);
        w.Run();
    } catch (...) {
        spdlog::error("upnp: worker ended by an exception");
    }
    if (ok) CoUninitialize();
    Shared& g = G();
    {
        std::lock_guard lk(g.mu);
        g.done = true;
    }
    g.doneCv.notify_all();
    return 0;
}

// Caller holds g.mu.
void EnsureWorker(Shared& g) {
    if (g.started) return;
    HANDLE h = CreateThread(nullptr, 0, &WorkerThread, nullptr, 0, nullptr);
    if (!h) {
        g.status.state = State::Failed;
        g.status.message = "UPnP could not start";
        return;
    }
    CloseHandle(h);
    g.started = true;
}

void Wake(Shared& g) { g.cv.notify_all(); }

}  // namespace

void Configure(const Config& cfg) {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    g.cfg = cfg;
}

void Want(uint16_t internalPort) {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    if (g.exitRequested) return;
    if (g.wantPort == internalPort) return;
    g.wantPort = internalPort;
    if (internalPort == 0) {
        if (g.started) Wake(g);
        return;
    }
    if (!g.cfg.enabled) {
        g.status.message = g.offMessage = "Internet: off ([Coop] UPnP=0)";
        return;
    }
    if (g.userOff) return;
    g.status = Status{};
    g.status.state = State::Working;
    g.status.internalPort = internalPort;
    g.status.message = "Internet: asking the router to open port " + std::to_string(internalPort) + "...";
    EnsureWorker(g);
    Wake(g);
}

void SetUserOff(bool off) {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    if (g.userOff == off) return;
    g.userOff = off;
    g.offMessage = off ? "Internet: off (coop_upnp off)" : "Internet: off";
    if (off) {
        if (g.started) Wake(g);
        if (g.status.state == State::Off) g.status.message = g.offMessage;
        return;
    }
    if (g.wantPort != 0 && g.cfg.enabled) {
        g.status = Status{};
        g.status.state = State::Working;
        g.status.internalPort = g.wantPort;
        g.status.message = "Internet: asking the router to open port " + std::to_string(g.wantPort) + "...";
        EnsureWorker(g);
        Wake(g);
    }
}

void NoteOff(const std::string& message) {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    g.offMessage = message;
    if (g.status.state == State::Off && !g.status.mapped) g.status.message = message;
}

void Refresh() {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    if (!g.started || g.wantPort == 0) return;
    g.refreshRequested = true;
    Wake(g);
}

void CleanStale() {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    EnsureWorker(g);
    g.cleanRequested = true;
    Wake(g);
}

Status GetStatus() {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    return g.status;
}

void Shutdown(uint32_t waitMs) {
    Shared& g = G();
    std::unique_lock lk(g.mu);
    g.exitRequested = true;
    if (!g.started || g.done) return;
    const auto t0 = Clock::now();
    Wake(g);
    const bool finished = g.doneCv.wait_for(lk, std::chrono::milliseconds(waitMs), [&] { return g.done; });
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
    lk.unlock();
    if (finished) spdlog::info("upnp: shutdown done in {} ms", ms);
    else spdlog::warn("upnp: shutdown still busy after {} ms; the mapping expires with its lease", ms);
}

void SetOpenerForTest(Opener opener) {
    Shared& g = G();
    std::lock_guard lk(g.mu);
    g.opener = std::move(opener);
}

void ResetForTest() {
    Shared& g = G();
    std::unique_lock lk(g.mu);
    if (g.started && !g.done) {  // stop the old worker first (it removes whatever it still holds)
        g.exitRequested = true;
        Wake(g);
        g.doneCv.wait_for(lk, std::chrono::seconds(5), [&] { return g.done; });
    }
    g.cfg = Config{};
    g.wantPort = 0;
    g.userOff = false;
    g.exitRequested = false;
    g.cleanRequested = false;
    g.refreshRequested = false;
    g.started = false;
    g.done = false;
    g.status = Status{};
    g.offMessage = "Internet: off";
    g.opener = nullptr;
}

}  // namespace net::upnp
