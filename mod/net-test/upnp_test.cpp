// Tests for net/upnp (router logic against a fake router, the worker thread's lifecycle, the wrong-password guard)
// and an optional check against the real router.
//   dh2upnp_test.exe                 offline tests; exit code 0 = all passed
//   dh2upnp_test.exe --live [port]   real router (default port 27700): find it, map, verify, remove, verify gone.
//                                    Adds one mapping with description "DH2 coop" for a few seconds and removes it.
//   dh2upnp_test.exe --query [port]  real router, read-only: is that UDP port mapped (exit 10) or not (exit 0)?
// Every socket stays on 127.0.0.1, so a new build path never raises the Windows Firewall prompt.

#include "net/ban_guard.h"
#include "net/session.h"
#include "net/upnp.h"

#include <winsock2.h>
#include <windows.h>
#include <objbase.h>
#include <ws2tcpip.h>

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace net::upnp;
using Clock = std::chrono::steady_clock;

struct Tally {
    int pass = 0;
    int fail = 0;
    void Check(bool ok, const std::string& what) {
        if (ok) {
            ++pass;
            return;
        }
        ++fail;
        std::fprintf(stderr, "FAIL %s\n", what.c_str());
    }
};

long long Ms(Clock::time_point since) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - since).count();
}

bool WaitFor(int timeoutMs, const std::function<bool()>& pred) {
    const auto t0 = Clock::now();
    while (Ms(t0) < timeoutMs) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return pred();
}

// ---- a router in memory --------------------------------------------------------------------------------------

struct FakeIgd : Igd {
    std::mutex mu;
    std::string wan = "97.12.34.166";
    int wanErr = 0;
    std::map<uint16_t, Entry> table;
    bool permanentOnly = false;       // AddPortMapping with a lease answers 725
    std::map<uint16_t, int> addErrFor;  // forced AddPortMapping error per external port
    int addErrAll = 0;
    int deleteErr = 0;
    int deleteDelayMs = 0;
    int getErr = 0;                   // forced GetSpecificPortMappingEntry error (not 714)
    std::vector<std::string> calls;

    int ExternalIp(std::string& ip) override {
        std::lock_guard lk(mu);
        calls.push_back("ip");
        if (wanErr) return wanErr;
        ip = wan;
        return 0;
    }
    int GetEntry(uint16_t ext, Entry& out) override {
        std::lock_guard lk(mu);
        calls.push_back("get " + std::to_string(ext));
        if (getErr) return getErr;
        const auto it = table.find(ext);
        if (it == table.end()) return 714;
        out = it->second;
        return 0;
    }
    int AddEntry(uint16_t ext, uint16_t internal, const std::string& client, const std::string& desc,
                 uint32_t lease) override {
        std::lock_guard lk(mu);
        calls.push_back("add " + std::to_string(ext) + " lease " + std::to_string(lease));
        if (addErrAll) return addErrAll;
        if (const auto f = addErrFor.find(ext); f != addErrFor.end()) return f->second;
        if (permanentOnly && lease != 0) return 725;
        const auto it = table.find(ext);
        if (it != table.end() && it->second.client != client) return 718;
        table[ext] = Entry{internal, client, true, desc, lease};
        return 0;
    }
    int DeleteEntry(uint16_t ext) override {
        int delay = 0;
        {
            std::lock_guard lk(mu);
            calls.push_back("del " + std::to_string(ext));
            delay = deleteDelayMs;
        }
        if (delay) std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        std::lock_guard lk(mu);
        if (deleteErr) return deleteErr;
        return table.erase(ext) ? 0 : 714;
    }
    int EnumEntry(int index, uint16_t& ext, Entry& out) override {
        std::lock_guard lk(mu);
        if (index < 0 || static_cast<size_t>(index) >= table.size()) return 713;
        auto it = table.begin();
        std::advance(it, index);
        ext = it->first;
        out = it->second;
        return 0;
    }
    int Count(const std::string& prefix) {
        std::lock_guard lk(mu);
        int n = 0;
        for (const auto& c : calls)
            if (c.rfind(prefix, 0) == 0) ++n;
        return n;
    }
    size_t Size() {
        std::lock_guard lk(mu);
        return table.size();
    }
    bool Has(uint16_t ext) {
        std::lock_guard lk(mu);
        return table.count(ext) != 0;
    }
};

// What the worker holds: forwards to the shared fake so the test can inspect it.
struct Proxy : Igd {
    explicit Proxy(std::shared_ptr<FakeIgd> f) : keep_(f), f_(*f) {}
    int ExternalIp(std::string& ip) override { return f_.ExternalIp(ip); }
    int GetEntry(uint16_t e, Entry& o) override { return f_.GetEntry(e, o); }
    int AddEntry(uint16_t e, uint16_t i, const std::string& c, const std::string& d, uint32_t l) override {
        return f_.AddEntry(e, i, c, d, l);
    }
    int DeleteEntry(uint16_t e) override { return f_.DeleteEntry(e); }
    int EnumEntry(int i, uint16_t& e, Entry& o) override { return f_.EnumEntry(i, e, o); }
    std::shared_ptr<FakeIgd> keep_;
    FakeIgd& f_;
};

const char* const kMe = "192.168.77.5";

Plan MakePlan(uint16_t port = 27420) {
    Plan p;
    p.internalPort = port;
    p.client = kMe;
    p.leaseSec = 3600;
    return p;
}

Entry Other(uint16_t internalPort = 27420, const char* client = "192.168.77.9", const char* desc = "torrent") {
    return Entry{internalPort, client, true, desc, 0};
}

// ---- pure helpers ----------------------------------------------------------------------------------------------

void TestHelpers(Tally& t) {
    uint32_t v = 0;
    t.Check(ParseIpv4("97.12.34.166", v) && v == 0x610c22a6u, "parse 97.12.34.166");
    t.Check(ParseIpv4("  10.0.0.1\r\n", v), "parse trims");
    for (const char* bad : {"", "1.2.3", "1.2.3.4.5", "256.1.1.1", "1..2.3", "a.b.c.d", "1.2.3.", "1.2.3.4x", "1234.1.1.1"})
        t.Check(!ParseIpv4(bad, v), std::string("reject '") + bad + "'");
    t.Check(FormatIpv4(0x610c22a6u) == "97.12.34.166", "format");

    struct Row {
        const char* ip;
        AddrKind kind;
    } rows[] = {{"97.12.34.166", AddrKind::Public},   {"8.8.8.8", AddrKind::Public},
                {"172.15.0.1", AddrKind::Public},     {"172.32.0.1", AddrKind::Public},
                {"100.63.255.255", AddrKind::Public}, {"100.128.0.1", AddrKind::Public},
                {"10.1.2.3", AddrKind::Private},      {"172.16.0.1", AddrKind::Private},
                {"172.31.255.255", AddrKind::Private}, {"192.168.4.1", AddrKind::Private},
                {"100.64.0.1", AddrKind::Cgnat},      {"100.127.255.254", AddrKind::Cgnat},
                {"0.0.0.0", AddrKind::None},          {"127.0.0.1", AddrKind::None},
                {"169.254.1.1", AddrKind::None},      {"224.0.0.1", AddrKind::None},
                {"255.255.255.255", AddrKind::None},  {"", AddrKind::None},
                {"not an ip", AddrKind::None}};
    for (const Row& r : rows) t.Check(Classify(r.ip) == r.kind, std::string("classify '") + r.ip + "'");

    t.Check(MaskIp("97.12.34.166") == "97.x.x.166", "mask");
    t.Check(MaskIp("junk") == "x.x.x.x", "mask junk");
    t.Check(MaskIp("97.12.34.166").find("12") == std::string::npos, "mask hides the middle octets");

    // 600..899 -> 0x80040300 + (code - 600); 401 / 402 / 501 have their own HRESULTs.
    t.Check(CodeFromHresult(static_cast<int32_t>(0x80040372u)) == 714, "hresult 714");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x8004037Du)) == 725, "hresult 725");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x80040300u)) == 600, "hresult 600");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x8004042Bu)) == 899, "hresult 899");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x8004042Cu)) == 0, "hresult past 899");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x80040207u)) == 401, "hresult 401");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x80040208u)) == 402, "hresult 402");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x80040210u)) == 501, "hresult 501");
    t.Check(CodeFromHresult(static_cast<int32_t>(0x80040211u)) == 0, "hresult transport error is not a fault");
    t.Check(CodeFromHresult(0) == 0 && CodeFromHresult(static_cast<int32_t>(0x80004005u)) == 0, "hresult unrelated");

    t.Check(FaultOf(0) == Fault::None, "fault none");
    t.Check(FaultOf(714) == Fault::NoSuchEntry && FaultOf(713) == Fault::NoSuchEntry, "fault 713 714");
    t.Check(FaultOf(718) == Fault::Conflict && FaultOf(729) == Fault::Conflict, "fault conflict");
    t.Check(FaultOf(725) == Fault::PermanentOnly, "fault 725");
    t.Check(FaultOf(724) == Fault::SamePortRequired, "fault 724");
    t.Check(FaultOf(606) == Fault::NotAuthorized, "fault 606");
    t.Check(FaultOf(728) == Fault::NoPortMaps, "fault 728");
    t.Check(FaultOf(727) == Fault::WildcardOnly && FaultOf(726) == Fault::WildcardOnly, "fault wildcard");
    t.Check(FaultOf(kErrTransport) == Fault::Transport && FaultOf(kErrNoRouter) == Fault::Transport, "fault transport");
    t.Check(FaultOf(501) == Fault::Other && FaultOf(402) == Fault::Other, "fault other");

    t.Check(NormalizeLease(3600) == 3600, "lease default");
    t.Check(NormalizeLease(0) == 0, "lease 0 stays permanent");
    t.Check(NormalizeLease(5) == 120 && NormalizeLease(119) == 120, "lease floor");
    t.Check(NormalizeLease(1000000) == 86400, "lease ceiling");
    t.Check(NormalizeLease(-1) == 3600, "lease negative -> default");
    t.Check(RenewDelaySec(3600) == 1800, "renew at half");
    t.Check(RenewDelaySec(120) == 60, "renew 120");
    t.Check(RenewDelaySec(40) == 30, "renew floor 30");
    t.Check(RenewDelaySec(0) == 600, "permanent re-check every 10 minutes");

    t.Check(CandidatePort(27420, 0) == 27420 && CandidatePort(27420, 9) == 27429, "candidate ports");
    t.Check(CandidatePort(65530, 5) == 65535 && CandidatePort(65530, 6) == 0, "candidate overflow");
    t.Check(CandidatePort(0, 0) == 0, "candidate zero base");

    t.Check(FailureMessage(Fault::Transport, AddrKind::None, 27420, kErrNoRouter) == "Router has UPnP turned off",
            "message no router");
    t.Check(FailureMessage(Fault::None, AddrKind::Cgnat, 27420, 0).find("CGNAT") != std::string::npos, "message cgnat");
    t.Check(FailureMessage(Fault::None, AddrKind::Private, 27420, 0).find("double NAT") != std::string::npos,
            "message double nat");
    t.Check(FailureMessage(Fault::Conflict, AddrKind::Public, 27420, 718) == "Port 27420 is forwarded to another device",
            "message conflict");
    t.Check(FailureMessage(Fault::Other, AddrKind::Public, 27420, 501).find("501") != std::string::npos,
            "message other has the code");

    StateRecord rec;
    t.Check(ParseState(FormatState(StateRecord{27700, "192.168.5.200"}), rec) && rec.externalPort == 27700 &&
                rec.client == "192.168.5.200",
            "state round trip");
    for (const char* bad : {"", "udp", "tcp 27700 192.168.5.200", "udp 0 192.168.5.200", "udp 70000 192.168.5.200",
                            "udp 27700 not-an-ip", "udp 27700 192.168.5.200 extra", "udp 27x 192.168.5.200"})
        t.Check(!ParseState(bad, rec), std::string("state rejects '") + bad + "'");
}

// ---- EstablishMapping and friends against the fake -----------------------------------------------------------

void TestEstablish(Tally& t) {
    {  // free port, public address
        FakeIgd f;
        std::vector<uint16_t> asked;
        Plan p = MakePlan();
        p.beforeAdd = [&](uint16_t e) { asked.push_back(e); };
        const Outcome o = EstablishMapping(f, p);
        t.Check(o.ok && o.reachable() && o.externalPort == 27420 && o.leaseSec == 3600 && o.verified, "free: mapped");
        t.Check(o.message == "Internet: open on port 27420", "free: message");
        t.Check(f.table.count(27420) && f.table[27420].client == kMe && f.table[27420].description == "DH2 coop" &&
                    f.table[27420].internalPort == 27420 && f.table[27420].leaseSec == 3600,
                "free: entry content");
        t.Check(asked.size() == 1 && asked[0] == 27420, "free: beforeAdd saw the port");
        t.Check(!o.reusedStale && !o.permanentFallback && o.portsTried == 1, "free: flags");
    }
    {  // taken by another device: +1
        FakeIgd f;
        f.table[27420] = Other();
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.externalPort == 27421, "taken: moves to +1");
        t.Check(o.message == "Internet: open on port 27421 (27420 was taken)", "taken: message");
        t.Check(f.table[27420].client == "192.168.77.9", "taken: the other device's mapping is untouched");
    }
    {  // several taken
        FakeIgd f;
        for (uint16_t p = 27420; p < 27424; ++p) f.table[p] = Other(p);
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.externalPort == 27424 && o.portsTried == 5, "taken x4: 27424");
    }
    {  // our own stale mapping (same client + description): reused
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "DH2 coop", 100};
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.externalPort == 27420 && o.reusedStale, "stale: reused");
        t.Check(f.table[27420].leaseSec == 3600, "stale: lease refreshed");
    }
    {  // same client, other description AND other internal port: someone else's program on this PC
        FakeIgd f;
        f.table[27420] = Entry{9999, kMe, true, "uTorrent", 0};
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.externalPort == 27421 && f.table[27420].description == "uTorrent", "other app on this PC: left alone");
    }
    {  // same client, other description but the same internal port: a manual forward to this very port
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "manual", 0};
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.externalPort == 27420 && o.reusedStale, "same internal port counts as ours");
    }
    {  // all ten taken
        FakeIgd f;
        for (uint16_t p = 27420; p < 27430; ++p) f.table[p] = Other(p);
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(!o.ok && o.fault == Fault::Conflict && o.portsTried == 10, "all taken: fails");
        t.Check(o.message == "Port 27420 is forwarded to another device", "all taken: message");
        t.Check(f.Count("add") == 0, "all taken: never tried to add");
    }
    {  // router refuses leases
        FakeIgd f;
        f.permanentOnly = true;
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.permanentFallback && o.leaseSec == 0 && f.table[27420].leaseSec == 0, "725: lease 0");
        t.Check(f.Count("add 27420 lease 3600") == 1 && f.Count("add 27420 lease 0") == 1, "725: tried the lease first");
    }
    {  // permanent requested stays permanent without a fallback flag
        FakeIgd f;
        Plan p = MakePlan();
        p.leaseSec = 0;
        const Outcome o = EstablishMapping(f, p);
        t.Check(o.ok && !o.permanentFallback && o.leaseSec == 0, "lease 0 requested");
    }
    {  // double NAT: maps, but not reachable
        FakeIgd f;
        f.wan = "192.168.1.2";
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.kind == AddrKind::Private && !o.reachable(), "double nat: mapped, not reachable");
        t.Check(o.message.find("double NAT") != std::string::npos, "double nat: message");
    }
    {  // CGNAT: nothing is mapped
        FakeIgd f;
        f.wan = "100.80.1.2";
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(!o.ok && o.kind == AddrKind::Cgnat && f.Count("add") == 0 && f.Count("get") == 0, "cgnat: no mapping");
        t.Check(o.message.find("CGNAT") != std::string::npos, "cgnat: message");
    }
    {  // router not connected: 0.0.0.0
        FakeIgd f;
        f.wan = "0.0.0.0";
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(!o.ok && o.kind == AddrKind::None && o.message == "The router has no internet address", "wan 0.0.0.0");
    }
    {  // no router
        FakeIgd f;
        f.wanErr = kErrNoRouter;
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(!o.ok && o.message == "Router has UPnP turned off", "no router");
    }
    {  // a race: the port was free at the probe, taken at the add (718): next port
        FakeIgd f;
        f.addErrFor[27420] = 718;
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.externalPort == 27421, "718 at add: next port");
    }
    {  // not authorized: stop at once
        FakeIgd f;
        f.addErrAll = 606;
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(!o.ok && o.fault == Fault::NotAuthorized && f.Count("add") == 1, "606: stops");
        t.Check(o.message.find("refuses") != std::string::npos, "606: message");
    }
    {  // a router that answers a missing entry with a strange error: the add decides
        FakeIgd f;
        f.getErr = 501;
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(o.ok && o.externalPort == 27420, "odd probe error: add anyway");
    }
    {  // an unknown error: stops, keeps the code
        FakeIgd f;
        f.addErrAll = 501;
        const Outcome o = EstablishMapping(f, MakePlan());
        t.Check(!o.ok && o.code == 501 && o.message.find("501") != std::string::npos, "501: message");
    }
    {  // port near the top
        FakeIgd f;
        const Outcome o = EstablishMapping(f, MakePlan(65535));
        t.Check(o.ok && o.externalPort == 65535, "port 65535");
        FakeIgd g;
        g.table[65535] = Other(65535);
        const Outcome o2 = EstablishMapping(g, MakePlan(65535));
        t.Check(!o2.ok && o2.fault == Fault::Conflict && o2.portsTried == 1, "port 65535 taken: no wraparound");
    }
    {  // cancelled before and during
        FakeIgd f;
        Plan p = MakePlan();
        p.cancelled = [] { return true; };
        const Outcome o = EstablishMapping(f, p);
        t.Check(o.cancelled && !o.ok && f.calls.empty(), "cancelled at once: no router call");
        FakeIgd g;
        int polls = 0;
        Plan q = MakePlan();
        q.cancelled = [&] { return ++polls > 1; };  // allows the WAN query, cancels before the port probe
        const Outcome o2 = EstablishMapping(g, q);
        t.Check(o2.cancelled && !o2.ok && g.Count("add") == 0 && g.Size() == 0, "cancelled mid-way: nothing added");
    }
}

void TestRemoval(Tally& t) {
    {
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "DH2 coop", 0};
        int code = 0;
        t.Check(RemoveMapping(f, 27420, kMe, "DH2 coop", false, &code) == Removal::Removed && !f.Has(27420), "remove");
        t.Check(RemoveMapping(f, 27420, kMe, "DH2 coop", false, &code) == Removal::AlreadyGone, "remove again: already gone");
    }
    {
        FakeIgd f;
        f.table[27420] = Other();
        t.Check(RemoveMapping(f, 27420, kMe, "DH2 coop", false, nullptr) == Removal::NotOurs && f.Has(27420),
                "another device's mapping is not removed");
    }
    {  // description check is optional for our own session, required for a leftover
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "renamed by router", 0};
        t.Check(RemoveMapping(f, 27420, kMe, "DH2 coop", true, nullptr) == Removal::NotOurs && f.Has(27420),
                "strict: other description left alone");
        t.Check(RemoveMapping(f, 27420, kMe, "DH2 coop", false, nullptr) == Removal::Removed, "loose: removed");
    }
    {
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "DH2 coop", 0};
        f.deleteErr = 501;
        int code = 0;
        t.Check(RemoveMapping(f, 27420, kMe, "DH2 coop", false, &code) == Removal::Failed && code == 501 && f.Has(27420),
                "delete error reported");
    }
    {  // a failing probe does not stop the delete
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "DH2 coop", 0};
        f.getErr = 501;
        t.Check(RemoveMapping(f, 27420, kMe, "DH2 coop", false, nullptr) == Removal::Removed, "probe error: delete anyway");
    }
    {
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "DH2 coop", 0};
        t.Check(RemoveLeftover(f, StateRecord{27420, kMe}, "DH2 coop") == Removal::Removed, "leftover removed");
        f.table[27421] = Entry{27421, kMe, true, "other tool", 0};
        t.Check(RemoveLeftover(f, StateRecord{27421, kMe}, "DH2 coop") == Removal::NotOurs && f.Has(27421),
                "leftover with another description stays");
        f.table[27422] = Entry{27422, "192.168.77.9", true, "DH2 coop", 0};  // a friend's PC on the same router
        t.Check(RemoveLeftover(f, StateRecord{27422, kMe}, "DH2 coop") == Removal::NotOurs && f.Has(27422),
                "leftover that now points at another PC stays");
    }
    {
        FakeIgd f;
        f.table[27420] = Entry{27420, kMe, true, "DH2 coop", 0};
        f.table[27421] = Entry{27421, kMe, true, "DH2 coop", 0};
        f.table[27422] = Entry{27422, "192.168.77.9", true, "DH2 coop", 0};  // someone else's PC
        f.table[27423] = Entry{27423, kMe, true, "other", 0};
        t.Check(RemoveAllOurs(f, kMe, "DH2 coop", 27421) == 1, "clean: one removed");
        t.Check(!f.Has(27420) && f.Has(27421) && f.Has(27422) && f.Has(27423), "clean: only ours and not the live one");
    }
    {  // renew
        FakeIgd f;
        const Plan p = MakePlan();
        const Outcome o = EstablishMapping(f, p);
        f.calls.clear();
        t.Check(RenewMapping(f, p, o) == 0 && f.Count("add 27420 lease 3600") == 1, "renew re-adds with the lease");
        f.table.clear();
        t.Check(RenewMapping(f, p, o) == 0 && f.Has(27420), "renew restores a mapping the router lost");
        f.addErrAll = 718;
        t.Check(RenewMapping(f, p, o) == 718, "renew reports a conflict");
    }
}

// ---- the worker thread against the fake ---------------------------------------------------------------------

std::string TempFile(const char* name) {
    char dir[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, dir);
    return std::string(dir) + name;
}

std::string Slurp(const std::string& path) {
    std::string s;
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
        char buf[256];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
        std::fclose(f);
    }
    return s;
}

void SetupService(const std::shared_ptr<FakeIgd>& fp, const std::string& stateFile, uint32_t renewMs = 0) {
    ResetForTest();
    Config cfg;
    cfg.enabled = true;
    cfg.leaseSec = 3600;
    cfg.stateFile = stateFile;
    cfg.bindAddress = kMe;  // no dependence on this PC's adapters
    cfg.renewMsForTest = renewMs;
    Configure(cfg);
    SetOpenerForTest([fp](RouterInfo& info, std::string&) -> std::unique_ptr<Igd> {
        info.name = "fake";
        info.serviceType = "urn:schemas-upnp-org:service:WANIPConnection:1";
        return std::make_unique<Proxy>(fp);
    });
}

void TestService(Tally& t) {
    const std::string stateFile = TempFile("dh2upnp-test.state");
    std::remove(stateFile.c_str());
    {  // host start -> open, renew, hosting stops -> removed
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        SetupService(fp, stateFile, 40);
        t.Check(GetStatus().state == State::Off, "service: starts off");
        Want(27700);
        t.Check(GetStatus().state == State::Working, "service: Working right after Want");
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "service: reaches Open");
        const Status s = GetStatus();
        t.Check(s.externalPort == 27700 && s.internalPort == 27700 && s.publicIp == "97.12.34.166" && s.mapped &&
                    s.kind == AddrKind::Public && s.message == "Internet: open on port 27700",
                "service: status content");
        t.Check(f.Has(27700), "service: the router has the mapping");
        t.Check(Slurp(stateFile) == FormatState(StateRecord{27700, kMe}), "service: leftover record written");
        t.Check(WaitFor(2000, [&] { return f.Count("add 27700") >= 3; }), "service: renews while hosting");
        Want(27700);  // same port again: nothing happens
        Want(0);
        t.Check(WaitFor(3000, [&] { return !f.Has(27700); }), "service: hosting stops -> mapping removed");
        t.Check(WaitFor(1000, [] { return GetStatus().state == State::Off; }), "service: back to Off");
        t.Check(Slurp(stateFile).empty(), "service: leftover record removed");
        const int adds = f.Count("add");
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        t.Check(f.Count("add") == adds, "service: no renewals after stopping");
    }
    {  // a new port while hosting moves the mapping
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "port change: first Open");
        Want(27701);
        t.Check(WaitFor(3000, [&] { return f.Has(27701) && !f.Has(27700); }), "port change: moved");
        Want(0);
        t.Check(WaitFor(3000, [&] { return f.Size() == 0; }), "port change: removed at the end");
    }
    {  // coop_upnp off / on
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "off/on: Open");
        SetUserOff(true);
        t.Check(WaitFor(3000, [&] { return f.Size() == 0; }), "off: mapping removed");
        t.Check(WaitFor(1000, [] { return GetStatus().state == State::Off; }) &&
                    GetStatus().message == "Internet: off (coop_upnp off)",
                "off: status says why");
        SetUserOff(false);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }) && f.Has(27700), "on: mapped again");
        Want(0);
        t.Check(WaitFor(3000, [&] { return f.Size() == 0; }), "off/on: removed at the end");
    }
    {  // a leftover record: the earlier run's mapping is removed first, a stranger's mapping stays
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        f.table[27700] = Entry{27700, kMe, true, "DH2 coop", 3600};
        f.table[27701] = Entry{27701, "192.168.77.9", true, "DH2 coop", 3600};
        {
            FILE* sf = std::fopen(stateFile.c_str(), "wb");
            std::fputs(FormatState(StateRecord{27700, kMe}).c_str(), sf);
            std::fclose(sf);
        }
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "leftover: Open");
        std::vector<std::string> calls;
        {
            std::lock_guard lk(f.mu);
            calls = f.calls;
        }
        size_t firstDel = calls.size(), firstAdd = calls.size();
        for (size_t i = 0; i < calls.size(); ++i) {
            if (calls[i] == "del 27700" && firstDel == calls.size()) firstDel = i;
            if (calls[i].rfind("add 27700", 0) == 0 && firstAdd == calls.size()) firstAdd = i;
        }
        t.Check(firstDel < firstAdd, "leftover: deleted before the new mapping is added");
        t.Check(f.Has(27701), "leftover: another PC's mapping untouched");
        Want(0);
        t.Check(WaitFor(3000, [&] { return !f.Has(27700); }), "leftover: removed at the end");
    }
    {  // double NAT through the service
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        f.wan = "10.0.0.2";
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Failed; }), "double nat: Failed");
        const Status s = GetStatus();
        t.Check(s.mapped && s.kind == AddrKind::Private && s.message.find("double NAT") != std::string::npos,
                "double nat: mapped but not open");
        Want(0);
        t.Check(WaitFor(3000, [&] { return f.Size() == 0; }), "double nat: removed at the end");
    }
    {  // CGNAT through the service: nothing added
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        f.wan = "100.70.0.2";
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Failed; }), "cgnat: Failed");
        t.Check(GetStatus().kind == AddrKind::Cgnat && !GetStatus().mapped && f.Count("add") == 0, "cgnat: nothing mapped");
        Want(0);
    }
    {  // no router, then the router appears on the retry
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        SetupService(fp, stateFile);
        std::atomic<bool> routerUp{false};
        SetOpenerForTest([fp, &routerUp](RouterInfo& info, std::string& why) -> std::unique_ptr<Igd> {
            if (!routerUp) {
                why = "no router answered";
                return nullptr;
            }
            info.name = "fake";
            return std::make_unique<Proxy>(fp);
        });
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Failed; }) &&
                    GetStatus().message == "Router has UPnP turned off",
                "no router: Failed with the plain message");
        routerUp = true;
        Refresh();
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "no router: Refresh finds it");
        Want(0);
        t.Check(WaitFor(3000, [&] { return f.Size() == 0; }), "no router: removed at the end");
        Shutdown(3000);  // the opener above refers to routerUp, a local of this block
    }
    {  // disabled in the ini
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        SetupService(fp, stateFile);
        Config cfg;
        cfg.enabled = false;
        cfg.bindAddress = kMe;
        Configure(cfg);
        Want(27700);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        t.Check(GetStatus().state == State::Off && f.calls.empty() && GetStatus().message == "Internet: off ([Coop] UPnP=0)",
                "disabled: never touches the router");
        Want(0);
    }
    {  // clean
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        f.table[27500] = Entry{27500, kMe, true, "DH2 coop", 0};
        f.table[27501] = Entry{27501, "192.168.77.9", true, "DH2 coop", 0};
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "clean: Open");
        CleanStale();
        t.Check(WaitFor(3000, [&] { return !f.Has(27500); }), "clean: stale one removed");
        t.Check(f.Has(27700) && f.Has(27501), "clean: live mapping and the other PC's stay");
        Want(0);
        t.Check(WaitFor(3000, [&] { return !f.Has(27700); }), "clean: live one removed at the end");
    }
    {  // exit is fast
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        f.deleteDelayMs = 100;
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "exit: Open");
        const auto t0 = Clock::now();
        Shutdown(1500);
        const long long ms = Ms(t0);
        t.Check(!f.Has(27700), "exit: mapping gone");
        t.Check(ms >= 90 && ms < 600, "exit: took " + std::to_string(ms) + " ms");
        Want(27702);  // after Shutdown nothing starts again
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        t.Check(!f.Has(27702), "exit: no mapping after shutdown");
        Shutdown(1500);  // twice is fine
    }
    {  // exit is bounded when the router is slow
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        SetupService(fp, stateFile);
        Want(27700);
        t.Check(WaitFor(3000, [] { return GetStatus().state == State::Open; }), "slow exit: Open");
        {
            std::lock_guard lk(f.mu);
            f.deleteDelayMs = 4000;
        }
        const auto t0 = Clock::now();
        Shutdown(1500);
        const long long ms = Ms(t0);
        t.Check(ms >= 1400 && ms < 2500, "slow exit: bounded at " + std::to_string(ms) + " ms");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    {  // exit while still looking for the router: nothing is added afterwards
        auto fp = std::make_shared<FakeIgd>();
        FakeIgd& f = *fp;
        SetupService(fp, stateFile);
        std::atomic<bool> inOpen{false};
        SetOpenerForTest([fp, &inOpen](RouterInfo& info, std::string&) -> std::unique_ptr<Igd> {
            inOpen = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            info.name = "fake";
            return std::make_unique<Proxy>(fp);
        });
        Want(27700);
        t.Check(WaitFor(2000, [&] { return inOpen.load(); }), "exit during search: searching");
        const auto t0 = Clock::now();
        Shutdown(1500);
        t.Check(Ms(t0) < 1000, "exit during search: returns when the search ends");
        t.Check(f.Size() == 0 && f.Count("add") == 0, "exit during search: nothing was added");
    }
    ResetForTest();
    std::remove(stateFile.c_str());
}

// ---- the SOAP client against a fake router on 127.0.0.1 ------------------------------------------------------

const char* const kSvc = "urn:schemas-upnp-org:service:WANIPConnection:1";

// The device description of the eero this was developed on (names and ids removed).
const char* const kEeroDescription =
    "<?xml version=\"1.0\"?>\n<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n   <specVersion><major>1</major><minor>0</minor></specVersion>\n"
    "   <URLBase>http://192.168.4.1:1900/</URLBase>\n   <device>\n      <deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>\n"
    "      <serviceList>\n         <service>\n            <serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>\n"
    "            <controlURL>/l3f</controlURL>\n            <eventSubURL>/l3f/events</eventSubURL>\n            <SCPDURL>/l3f.xml</SCPDURL>\n         </service>\n      </serviceList>\n"
    "      <deviceList>\n         <device>\n            <deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>\n            <serviceList>\n"
    "               <service>\n                  <serviceType>urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1</serviceType>\n                  <controlURL>/ifc</controlURL>\n               </service>\n            </serviceList>\n"
    "            <deviceList>\n               <device>\n                  <deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>\n                  <serviceList>\n"
    "                     <service>\n                        <serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>\n                        <serviceId>urn:upnp-org:serviceId:WANIPConnection</serviceId>\n"
    "                        <controlURL>/ipc</controlURL>\n                        <eventSubURL>/ipc/events</eventSubURL>\n                        <SCPDURL>/ipc.xml</SCPDURL>\n                     </service>\n"
    "                  </serviceList>\n               </device>\n            </deviceList>\n         </device>\n      </deviceList>\n      <presentationURL>https://eero.com</presentationURL>\n   </device>\n</root>\n";

struct Quirks {
    bool faultAs200 = true;       // eero: a UPnP error comes as HTTP 200 with a SOAP fault inside
    bool noContentLength = true;  // eero: HTTP/1.0, the body ends when the connection closes
    bool genericEmpty = true;     // eero: GetGenericPortMappingEntry closes the connection without a reply
    bool chunked = false;
    bool hang = false;            // accepts, reads the request, never answers
};

class FakeServer {
public:
    FakeServer(std::shared_ptr<FakeIgd> igd, Quirks q) : igd_(std::move(igd)), q_(q) {
        WSADATA d;
        WSAStartup(MAKEWORD(2, 2), &d);
        listen_ = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a = {};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // loopback only: no firewall prompt
        a.sin_port = 0;
        bind(listen_, reinterpret_cast<sockaddr*>(&a), sizeof a);
        listen(listen_, 8);
        int len = sizeof a;
        getsockname(listen_, reinterpret_cast<sockaddr*>(&a), &len);
        port_ = ntohs(a.sin_port);
        th_ = std::thread([this] { Loop(); });
    }
    ~FakeServer() {
        stop_ = true;
        if (th_.joinable()) th_.join();
        closesocket(listen_);
    }
    uint16_t Port() const { return port_; }
    Url Control() const { return Url{"127.0.0.1", port_, "/ipc"}; }
    std::string DocumentUrl() const { return "http://127.0.0.1:" + std::to_string(port_) + "/igd.xml"; }
    std::vector<std::string> Requests() {
        std::lock_guard lk(mu_);
        return requests_;
    }

private:
    void Loop() {
        while (!stop_) {
            fd_set rd;
            FD_ZERO(&rd);
            FD_SET(listen_, &rd);
            timeval tv{0, 100 * 1000};
            if (select(0, &rd, nullptr, nullptr, &tv) <= 0) continue;
            SOCKET c = accept(listen_, nullptr, nullptr);
            if (c == INVALID_SOCKET) continue;
            Handle(c);
            closesocket(c);
        }
    }

    static std::string Header(const std::string& head, const char* name) {
        std::string lower = head;
        for (char& ch : lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        const std::string key = std::string("\r\n") + name + ":";
        const size_t p = lower.find(key);
        if (p == std::string::npos) return {};
        const size_t e = head.find("\r\n", p + 2);
        std::string v = head.substr(p + key.size(), e - p - key.size());
        while (!v.empty() && v.front() == ' ') v.erase(v.begin());
        return v;
    }

    void Send(SOCKET c, int status, const std::string& body) {
        std::string head;
        if (q_.chunked) {
            head = "HTTP/1.1 " + std::to_string(status) + " OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n";
            const size_t half = body.size() / 2;
            char n1[16], n2[16];
            std::snprintf(n1, sizeof n1, "%zx", half);
            std::snprintf(n2, sizeof n2, "%zx", body.size() - half);
            head += std::string(n1) + "\r\n" + body.substr(0, half) + "\r\n" + n2 + "\r\n" + body.substr(half) + "\r\n0\r\n\r\n";
            send(c, head.data(), static_cast<int>(head.size()), 0);
            return;
        }
        if (q_.noContentLength) head = "HTTP/1.0 " + std::to_string(status) + " OK\r\nServer: fake\r\nCONTENT-TYPE: text/xml\r\nEXT: \r\n\r\n";
        else head = "HTTP/1.1 " + std::to_string(status) + " OK\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n";
        head += body;
        send(c, head.data(), static_cast<int>(head.size()), 0);
    }

    void Fault(SOCKET c, int code) {
        const std::string body =
            "<?xml version=\"1.0\"?>\r\n<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><s:Fault>"
            "<faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail><UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
            "<errorCode>" + std::to_string(code) + "</errorCode><errorDescription>x</errorDescription></UPnPError></detail></s:Fault></s:Body></s:Envelope>";
        Send(c, q_.faultAs200 ? 200 : 500, body);
    }

    void Ok(SOCKET c, const std::string& action, const std::string& outs) {
        Send(c, 200,
             "<?xml version=\"1.0\"?>\r\n<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:" + action +
                 "Response xmlns:u=\"" + kSvc + "\">" + outs + "</u:" + action + "Response></s:Body></s:Envelope>");
    }

    void Handle(SOCKET c) {
        std::string raw;
        DWORD to = 1000;
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&to), sizeof to);
        size_t headEnd = std::string::npos;
        long cl = 0;
        for (int i = 0; i < 200; ++i) {
            char buf[2048];
            const int n = recv(c, buf, sizeof buf, 0);
            if (n <= 0) break;
            raw.append(buf, static_cast<size_t>(n));
            if (headEnd == std::string::npos) {
                headEnd = raw.find("\r\n\r\n");
                if (headEnd != std::string::npos) {
                    const std::string v = Header(raw.substr(0, headEnd + 2), "content-length");
                    cl = v.empty() ? 0 : std::atol(v.c_str());
                }
            }
            if (headEnd != std::string::npos && raw.size() >= headEnd + 4 + static_cast<size_t>(cl)) break;
        }
        if (headEnd == std::string::npos) return;
        const std::string head = raw.substr(0, headEnd + 2);
        const std::string body = raw.substr(headEnd + 4);
        if (raw.compare(0, 4, "GET ") == 0) {
            {
                std::lock_guard lk(mu_);
                requests_.push_back("GET");
            }
            std::string xml = kEeroDescription;
            const std::string url = "http://192.168.4.1:1900/";
            xml.replace(xml.find(url), url.size(), "http://127.0.0.1:" + std::to_string(port_) + "/");
            Send(c, 200, xml);
            return;
        }
        std::string action = Header(head, "soapaction");
        const size_t hash = action.find('#');
        if (hash != std::string::npos) action = action.substr(hash + 1);
        while (!action.empty() && (action.back() == '"' || action.back() == ' ')) action.pop_back();
        {
            std::lock_guard lk(mu_);
            requests_.push_back(action);
        }
        if (q_.hang) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            return;
        }
        std::string v;
        const auto num = [&](const char* tag) -> uint32_t {
            return XmlValue(body, tag, v) ? static_cast<uint32_t>(std::strtoul(v.c_str(), nullptr, 10)) : 0u;
        };
        const auto str = [&](const char* tag) -> std::string { return XmlValue(body, tag, v) ? v : std::string(); };
        FakeIgd& f = *igd_;
        if (action == "GetExternalIPAddress") {
            std::string ip;
            const int e = f.ExternalIp(ip);
            return e ? Fault(c, e) : Ok(c, action, "<NewExternalIPAddress>" + ip + "</NewExternalIPAddress>");
        }
        if (action == "GetSpecificPortMappingEntry") {
            Entry en;
            const int e = f.GetEntry(static_cast<uint16_t>(num("NewExternalPort")), en);
            if (e) return Fault(c, e);
            return Ok(c, action,
                      "<NewInternalPort>" + std::to_string(en.internalPort) + "</NewInternalPort><NewInternalClient>" + en.client +
                          "</NewInternalClient><NewEnabled>" + (en.enabled ? "1" : "0") + "</NewEnabled><NewPortMappingDescription>" +
                          XmlEscape(en.description) + "</NewPortMappingDescription><NewLeaseDuration>" + std::to_string(en.leaseSec) +
                          "</NewLeaseDuration>");
        }
        if (action == "AddPortMapping") {
            const int e = f.AddEntry(static_cast<uint16_t>(num("NewExternalPort")), static_cast<uint16_t>(num("NewInternalPort")),
                                     str("NewInternalClient"), str("NewPortMappingDescription"), num("NewLeaseDuration"));
            return e ? Fault(c, e) : Ok(c, action, "");
        }
        if (action == "DeletePortMapping") {
            const int e = f.DeleteEntry(static_cast<uint16_t>(num("NewExternalPort")));
            return e ? Fault(c, e) : Ok(c, action, "");
        }
        if (action == "GetGenericPortMappingEntry") {
            if (q_.genericEmpty) return;  // closes without a word
            uint16_t ext = 0;
            Entry en;
            const int e = f.EnumEntry(static_cast<int>(num("NewPortMappingIndex")), ext, en);
            if (e) return Fault(c, e);
            return Ok(c, action,
                      "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + std::to_string(ext) +
                          "</NewExternalPort><NewProtocol>UDP</NewProtocol><NewInternalPort>" + std::to_string(en.internalPort) +
                          "</NewInternalPort><NewInternalClient>" + en.client + "</NewInternalClient><NewEnabled>1</NewEnabled><NewPortMappingDescription>" +
                          XmlEscape(en.description) + "</NewPortMappingDescription><NewLeaseDuration>" + std::to_string(en.leaseSec) +
                          "</NewLeaseDuration>");
        }
        Fault(c, 401);
    }

    std::shared_ptr<FakeIgd> igd_;
    Quirks q_;
    SOCKET listen_ = INVALID_SOCKET;
    uint16_t port_ = 0;
    std::thread th_;
    std::atomic<bool> stop_{false};
    std::mutex mu_;
    std::vector<std::string> requests_;
};

void TestSoapPure(Tally& t) {
    Url u;
    t.Check(ParseUrl("http://192.168.4.1:1900/igd.xml", u) && u.host == "192.168.4.1" && u.port == 1900 && u.path == "/igd.xml", "url full");
    t.Check(ParseUrl("http://router/", u) && u.host == "router" && u.port == 80 && u.path == "/", "url default port");
    t.Check(ParseUrl("http://10.0.0.1", u) && u.path == "/" && u.port == 80, "url no path");
    t.Check(ParseUrl("HTTP://10.0.0.1:5000?x=1", u) && u.path == "/?x=1" && u.port == 5000, "url query");
    for (const char* bad : {"", "https://10.0.0.1/", "ftp://x/", "http://", "http://:80/", "http://h:0/", "http://h:99999/", "10.0.0.1"})
        t.Check(!ParseUrl(bad, u), std::string("url rejects '") + bad + "'");

    t.Check(ResolveUrl("http://192.168.4.1:1900/", "/ipc") == "http://192.168.4.1:1900/ipc", "resolve root-relative");
    t.Check(ResolveUrl("http://192.168.4.1:1900/dir/igd.xml", "ctl") == "http://192.168.4.1:1900/dir/ctl", "resolve relative");
    t.Check(ResolveUrl("http://h/", "http://other:81/x") == "http://other:81/x", "resolve absolute");
    t.Check(ResolveUrl("http://h:80/a/b.xml", "/c") == "http://h/c", "resolve drops port 80");
    t.Check(ResolveUrl("junk", "/c").empty() && ResolveUrl("http://h/", "").empty(), "resolve failures");

    const auto svcs = ParseDescription(kEeroDescription, "http://192.168.4.1:1900/igd.xml");
    t.Check(svcs.size() == 3, "description: 3 services, nested devices included");
    bool found = false;
    for (const auto& s : svcs)
        if (s.type == kSvc) found = s.controlUrl == "http://192.168.4.1:1900/ipc";
    t.Check(found, "description: WANIPConnection control URL from URLBase");
    t.Check(ServiceScore(kSvc) == 2 && ServiceScore("urn:schemas-upnp-org:service:WANIPConnection:2") == 3 &&
                ServiceScore("urn:schemas-upnp-org:service:WANPPPConnection:1") == 1 &&
                ServiceScore("urn:schemas-upnp-org:service:Layer3Forwarding:1") == 0,
            "service scores");
    const auto noBase = ParseDescription(
        "<root><device><serviceList><service><serviceType>T</serviceType><controlURL>/c</controlURL></service></serviceList></device></root>",
        "http://10.1.1.1:49152/d.xml");
    t.Check(noBase.size() == 1 && noBase[0].controlUrl == "http://10.1.1.1:49152/c", "description: no URLBase -> document URL");
    t.Check(ParseDescription("not xml at all", "http://h/").empty(), "description: garbage");

    std::string v;
    t.Check(XmlValue("<a><NewX>12</NewX></a>", "NewX", v) && v == "12", "xml value");
    t.Check(XmlValue("<NewXY>1</NewXY><NewX>2</NewX>", "NewX", v) && v == "2", "xml value skips a longer tag");
    t.Check(XmlValue("<NewX/>", "NewX", v) && v.empty(), "xml empty element");
    t.Check(XmlValue("<NewX a=\"1\"> x &amp; y </NewX>", "NewX", v) && v == "x & y", "xml attributes, trim, entity");
    t.Check(!XmlValue("<A>1</A>", "B", v) && !XmlValue("<B>1", "B", v), "xml missing / unterminated");
    t.Check(XmlEscape("a&b<c>\"d") == "a&amp;b&lt;c&gt;&quot;d" && XmlUnescape(XmlEscape("a&b<c>\"d'")) == "a&b<c>\"d'",
            "xml escape round trip");

    t.Check(SoapFaultCode("<UPnPError><errorCode>714</errorCode></UPnPError>") == 714, "fault code");
    t.Check(SoapFaultCode("<s:Fault><faultcode>x</faultcode></s:Fault>") == 501, "fault without a code");
    t.Check(SoapFaultCode("<u:AddPortMappingResponse xmlns:u=\"x\"></u:AddPortMappingResponse>") == 0, "no fault");
    const std::string soap = BuildSoap(kSvc, "AddPortMapping", "<NewX>1</NewX>");
    t.Check(soap.find("<u:AddPortMapping xmlns:u=\"" + std::string(kSvc) + "\"><NewX>1</NewX></u:AddPortMapping>") != std::string::npos,
            "soap envelope");
}

void TestSoapServer(Tally& t, const char* label, Quirks q) {
    const std::string tag = std::string("soap[") + label + "] ";
    auto fp = std::make_shared<FakeIgd>();
    FakeIgd& f = *fp;
    FakeServer srv(fp, q);
    auto igd = MakeSoapIgd(srv.Control(), kSvc, 3000);

    std::string ip;
    t.Check(igd->ExternalIp(ip) == 0 && ip == "97.12.34.166", tag + "external address");
    Entry e;
    t.Check(igd->GetEntry(27700, e) == 714, tag + "unmapped port -> 714");
    t.Check(igd->AddEntry(27700, 27700, kMe, "DH2 coop & <test>", 3600) == 0, tag + "add");
    t.Check(f.table.count(27700) && f.table[27700].description == "DH2 coop & <test>" && f.table[27700].leaseSec == 3600 &&
                f.table[27700].client == kMe && f.table[27700].internalPort == 27700,
            tag + "the router received every argument");
    t.Check(igd->GetEntry(27700, e) == 0 && e.client == kMe && e.internalPort == 27700 && e.description == "DH2 coop & <test>" &&
                e.leaseSec == 3600 && e.enabled,
            tag + "get returns the entry");
    t.Check(igd->AddEntry(27700, 27700, "192.168.77.9", "x", 60) == 718, tag + "another client -> 718");
    f.permanentOnly = true;
    t.Check(igd->AddEntry(27701, 27701, kMe, "x", 600) == 725, tag + "lease refused -> 725");
    t.Check(igd->AddEntry(27701, 27701, kMe, "x", 0) == 0, tag + "permanent accepted");
    f.permanentOnly = false;
    uint16_t ext = 0;
    const int en = igd->EnumEntry(0, ext, e);
    if (q.genericEmpty) t.Check(en == kErrTransport, tag + "empty reply to the table walk -> transport error");
    else t.Check(en == 0 && ext == 27700, tag + "table walk");
    t.Check(igd->DeleteEntry(27700) == 0 && !f.Has(27700), tag + "delete");
    t.Check(igd->DeleteEntry(27700) == 714, tag + "delete again -> 714");
    t.Check(RemoveMapping(*igd, 27701, kMe, "x", true, nullptr) == Removal::Removed, tag + "RemoveMapping through SOAP");

    Plan p = MakePlan(27420);
    f.table[27420] = Other();
    const Outcome o = EstablishMapping(*igd, p);
    t.Check(o.ok && o.externalPort == 27421 && o.verified && o.reachable(), tag + "EstablishMapping end to end (taken port, verified)");
    t.Check(RemoveMapping(*igd, 27421, kMe, "DH2 coop", false, nullptr) == Removal::Removed, tag + "and removal");
    const auto reqs = srv.Requests();
    t.Check(!reqs.empty() && reqs[0] == "GetExternalIPAddress", tag + "SOAPAction header carried the action");
}

void TestSoapEdges(Tally& t) {
    {  // connection refused: fast
        auto fp = std::make_shared<FakeIgd>();
        uint16_t dead = 0;
        {
            FakeServer srv(fp, Quirks{});
            dead = srv.Port();
        }
        auto igd = MakeSoapIgd(Url{"127.0.0.1", dead, "/ipc"}, kSvc, 3000);
        std::string ip;
        const auto t0 = Clock::now();
        t.Check(igd->ExternalIp(ip) == kErrTransport, "refused: transport error");
        t.Check(Ms(t0) < 3500, "refused: Windows retries a refused loopback connect for ~2 s (" + std::to_string(Ms(t0)) + " ms)");
    }
    {  // a router that never answers: bounded by the timeout
        auto fp = std::make_shared<FakeIgd>();
        Quirks q;
        q.hang = true;
        FakeServer srv(fp, q);
        auto igd = MakeSoapIgd(srv.Control(), kSvc, 600);
        std::string ip;
        const auto t0 = Clock::now();
        t.Check(igd->ExternalIp(ip) == kErrTransport, "hang: transport error");
        const long long ms = Ms(t0);
        t.Check(ms >= 500 && ms < 1500, "hang: bounded by the timeout (" + std::to_string(ms) + " ms)");
    }
    {  // device description fetched over the same client, then control URL
        auto fp = std::make_shared<FakeIgd>();
        FakeServer srv(fp, Quirks{});
        Url doc;
        t.Check(ParseUrl(srv.DocumentUrl(), doc), "description url parses");
        const HttpReply r = HttpExchange(doc, "GET", "", "", 3000);
        t.Check(r.ok && r.status == 200, "description GET");
        const auto svcs = ParseDescription(r.body, srv.DocumentUrl());
        Url control;
        bool ok = false;
        for (const auto& s : svcs)
            if (s.type == kSvc && ParseUrl(s.controlUrl, control)) ok = control.port == srv.Port() && control.path == "/ipc";
        t.Check(ok, "description -> control URL on the same server");
        std::string ip;
        t.Check(MakeSoapIgd(control, kSvc)->ExternalIp(ip) == 0, "control URL from the description works");
    }
}

void TestSoap(Tally& t) {
    TestSoapPure(t);
    Quirks eero;  // the defaults are eero's
    TestSoapServer(t, "eero", eero);
    Quirks standard;
    standard.faultAs200 = false;
    standard.noContentLength = false;
    standard.genericEmpty = false;
    TestSoapServer(t, "standard", standard);
    Quirks chunked = standard;
    chunked.chunked = true;
    TestSoapServer(t, "chunked", chunked);
    TestSoapEdges(t);
}

// ---- the wrong-password guard ---------------------------------------------------------------------------------

void TestBanGuard(Tally& t) {
    using net::BanGuard;
    constexpr uint64_t kMin = 60 * 1000;
    {
        BanGuard g;
        const uint32_t ip = 0x0100007f;
        uint64_t now = 1000;
        for (int i = 0; i < 4; ++i) t.Check(!g.NoteFailure(ip, now += 1000) && !g.IsBanned(ip, now), "ban: 4 failures are fine");
        t.Check(g.NoteFailure(ip, now += 1000), "ban: the 5th fails -> banned (reported once)");
        t.Check(g.IsBanned(ip, now), "ban: banned now");
        t.Check(!g.NoteFailure(ip, now + 1), "ban: further failures are not reported again");
        t.Check(g.IsBanned(ip, now + 9 * kMin), "ban: still banned after 9 minutes");
        t.Check(!g.IsBanned(ip, now + 10 * kMin + 1), "ban: lifted after 10 minutes");
        t.Check(!g.IsBanned(ip + 1, now), "ban: another address is not affected");
        // after the ban a fresh count starts
        uint64_t later = now + 11 * kMin;
        for (int i = 0; i < 4; ++i) t.Check(!g.NoteFailure(ip, later += 1000), "ban: fresh count after the ban");
        t.Check(!g.IsBanned(ip, later), "ban: not banned again by 4");
    }
    {  // failures older than the window do not add up
        BanGuard g;
        uint64_t now = 0;
        for (int i = 0; i < 4; ++i) g.NoteFailure(7, now += 1000);
        now += 11 * kMin;
        t.Check(!g.NoteFailure(7, now), "window: an old streak is forgotten");
        for (int i = 0; i < 3; ++i) t.Check(!g.NoteFailure(7, now += 1000), "window: the new streak counts from 1");
        t.Check(g.NoteFailure(7, now += 1000), "window: 5 within 10 minutes bans");
    }
    {  // a good login clears the count
        BanGuard g;
        uint64_t now = 0;
        for (int i = 0; i < 4; ++i) g.NoteFailure(9, now += 1000);
        g.NoteSuccess(9);
        for (int i = 0; i < 4; ++i) t.Check(!g.NoteFailure(9, now += 1000), "success: count restarted");
        t.Check(g.Tracked() == 1, "success: one entry");
    }
    {  // bounded memory
        BanGuard g;
        for (uint32_t i = 0; i < 5000; ++i) g.NoteFailure(i, 1000 + i);
        t.Check(g.Tracked() <= BanGuard::kMaxTracked, "bounded: " + std::to_string(g.Tracked()) + " tracked");
    }
}

// Pump a host and a client until pred or timeout. Events are drained into the vectors.
struct ClientRun {
    bool welcomed = false;
    bool rejectedBadPassword = false;
    bool disconnected = false;
};

ClientRun RunClient(net::Session& host, uint16_t port, uint32_t passHash, int timeoutMs) {
    net::Session client;
    net::Session::Identity id;
    id.name = "stranger";
    id.passHash = passHash;
    id.modBuild = 0xABCD;
    ClientRun r;
    if (!client.Join("127.0.0.1", port, id)) return r;
    const auto t0 = Clock::now();
    while (Ms(t0) < timeoutMs && !r.welcomed && !r.rejectedBadPassword && !r.disconnected) {
        host.Service();
        client.Service();
        host.DrainEvents();
        for (const net::Event& e : client.DrainEvents()) {
            if (e.type == net::EventType::Welcome) r.welcomed = true;
            if (e.type == net::EventType::Reject && e.reason == net::RejectReason::BadPassword) r.rejectedBadPassword = true;
            if (e.type == net::EventType::Disconnected || e.type == net::EventType::TimedOut) r.disconnected = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    client.Disconnect();
    for (int i = 0; i < 5; ++i) {
        host.Service();
        host.DrainEvents();
    }
    return r;
}

void TestSessionGuard(Tally& t) {
    net::Session::SetBindAddress("127.0.0.1");
    net::Session host;
    net::Session::Identity id;
    id.name = "host";
    id.modBuild = 0xABCD;
    id.passHash = 0x1234;
    constexpr uint16_t kPort = 27798;
    if (!host.StartHost(kPort, 3, id)) {
        t.Check(false, "session guard: host bind 127.0.0.1:27798");
        return;
    }
    for (int i = 1; i <= 5; ++i) {
        const ClientRun r = RunClient(host, kPort, 0x9999, 3000);
        t.Check(r.rejectedBadPassword && !r.welcomed, "session guard: wrong password #" + std::to_string(i) + " is rejected");
    }
    // The 5th wrong password banned 127.0.0.1: nothing gets in, not even the right password, and no Reject text.
    const ClientRun refused = RunClient(host, kPort, 0x1234, 2500);
    t.Check(!refused.welcomed && !refused.rejectedBadPassword, "session guard: the banned address is not welcomed");
    t.Check(refused.disconnected, "session guard: the banned address is dropped at once");
    t.Check(host.Peers().empty(), "session guard: no peer slot is held");
    host.Disconnect();
}

// ---- the real router ------------------------------------------------------------------------------------------

int Live(uint16_t port) {
    auto logger = spdlog::stdout_logger_mt("live");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] %v");
    int bad = 0;
    auto check = [&](bool ok, const char* what) {
        std::printf("%s: %s\n", ok ? "OK  " : "FAIL", what);
        if (!ok) ++bad;
    };

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    RouterInfo info;
    std::string why;
    auto igd = OpenRouter(info, why);
    if (!igd) {
        std::printf("no router: %s\n", why.c_str());
        return 2;
    }
    std::printf("router '%s' service %s gateway %s devices %d\n", info.name.c_str(), info.serviceType.c_str(),
                info.gatewayHost.c_str(), info.devicesFound);
    std::string ip;
    igd->ExternalIp(ip);
    const AddrKind kind = Classify(ip);
    std::printf("wan %s kind %s\n", MaskIp(ip).c_str(),
                kind == AddrKind::Public ? "public" : kind == AddrKind::Private ? "private" : kind == AddrKind::Cgnat ? "cgnat" : "none");
    const std::string me = LocalAddressToward(info.gatewayHost);
    std::printf("this PC toward the router: %s\n", me.c_str());
    check(!me.empty(), "found this PC's LAN address");

    Entry before;
    const int pre = igd->GetEntry(port, before);
    std::printf("before: GetSpecificPortMappingEntry(%u) -> %d\n", port, pre);
    if (pre == 0) {
        std::printf("port %u is already mapped (client %s, '%s'): not touching it\n", port, before.client.c_str(),
                    before.description.c_str());
        return 3;
    }
    check(FaultOf(pre) == Fault::NoSuchEntry, "an unmapped port answers 713 / 714");

    const std::string stateFile = TempFile("dh2upnp-live.state");
    std::remove(stateFile.c_str());
    ResetForTest();
    Config cfg;
    cfg.leaseSec = 120;  // short: a crash of this test cannot leave it open for long
    cfg.stateFile = stateFile;
    Configure(cfg);
    Want(port);
    const auto t0 = Clock::now();
    WaitFor(60000, [] { return GetStatus().state != State::Working; });
    const Status s = GetStatus();
    std::printf("status after %lld ms: state %d port %u->%u wan %s lease %u mapped %d | %s\n", Ms(t0),
                static_cast<int>(s.state), s.externalPort, s.internalPort, MaskIp(s.publicIp).c_str(), s.leaseSec,
                s.mapped, s.message.c_str());
    check(s.mapped, "the router accepted the mapping");

    if (s.mapped) {
        Entry e;
        const int g = igd->GetEntry(s.externalPort, e);
        std::printf("independent check: GetSpecificPortMappingEntry(%u) -> %d client %s internal %u '%s' lease %u\n",
                    s.externalPort, g, e.client.c_str(), e.internalPort, e.description.c_str(), e.leaseSec);
        check(g == 0 && e.client == me && e.internalPort == port && e.description == "DH2 coop",
              "the router lists the mapping for this PC");
        check(Slurp(stateFile) == FormatState(StateRecord{s.externalPort, me}), "leftover record written");
    }

    const auto t1 = Clock::now();
    Shutdown(1500);
    std::printf("Shutdown took %lld ms\n", Ms(t1));
    check(Ms(t1) < 1500, "removal finished inside the 1.5 s budget");
    Entry after;
    const int post = igd->GetEntry(s.mapped ? s.externalPort : port, after);
    std::printf("after: GetSpecificPortMappingEntry -> %d\n", post);
    check(FaultOf(post) == Fault::NoSuchEntry, "the mapping is gone from the router");
    check(Slurp(stateFile).empty(), "leftover record removed");
    if (post == 0) {  // never leave it behind
        igd->DeleteEntry(s.externalPort);
        std::printf("cleanup: deleted the leftover mapping by hand\n");
    }
    std::printf("%s\n", bad ? "LIVE TEST FAILED" : "LIVE TEST PASSED");
    return bad ? 1 : 0;
}

// Read-only: is this external port mapped on the router, and to whom? Exit code 0 = not mapped, 10 = mapped.
int Query(uint16_t port) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    RouterInfo info;
    std::string why;
    auto igd = OpenRouter(info, why);
    if (!igd) {
        std::printf("no router: %s\n", why.c_str());
        return 2;
    }
    Entry e;
    const int r = igd->GetEntry(port, e);
    if (r == 0) {
        std::printf("QUERY UDP %u: MAPPED to %s:%u '%s' lease %u\n", port, e.client.c_str(), e.internalPort,
                    e.description.c_str(), e.leaseSec);
        return 10;
    }
    std::printf("QUERY UDP %u: %s (code %d)\n", port, FaultOf(r) == Fault::NoSuchEntry ? "not mapped" : "unknown", r);
    return FaultOf(r) == Fault::NoSuchEntry ? 0 : 3;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--query") {
        spdlog::set_level(spdlog::level::off);
        return Query(static_cast<uint16_t>(argc >= 3 ? std::atoi(argv[2]) : 27700));
    }
    if (argc >= 2 && std::string(argv[1]) == "--live") {
        const int port = argc >= 3 ? std::atoi(argv[2]) : 27700;
        return Live(static_cast<uint16_t>(port));
    }
    spdlog::set_level(spdlog::level::warn);
    spdlog::set_pattern("[%l] %v");
    Tally t;
    TestHelpers(t);
    TestEstablish(t);
    TestRemoval(t);
    TestSoap(t);
    TestBanGuard(t);
    TestService(t);
    TestSessionGuard(t);
    std::printf("dh2upnp_test: %d checks passed, %d failed\n", t.pass, t.fail);
    return t.fail ? 1 : 0;
}
