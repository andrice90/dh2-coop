#pragma once

// UPnP port forward for the host: while a lobby is hosted, the home router (UPnP IGD) is asked to forward the game's
// UDP port to this PC, so friends can join over the internet by public address. The forward exists only while
// hosting: it is removed when hosting stops and when the game exits, and it carries a lease so a crash cannot leave
// it open for long. See re/notes/upnp.md.
//
// The router is found with the Windows UPnP COM API (IUPnPDeviceFinder): the SSDP Discovery service does the multicast
// search, so this process opens no listening socket and Windows Firewall shows no prompt. The router is then
// controlled with SOAP over ordinary outbound HTTP (also nothing to prompt about). All router work runs on one worker
// thread; the game thread only calls the small functions at the bottom and reads a status snapshot.
//
// The first half of this header is the router-independent logic (Igd is an interface), so the net-test drives it
// against a fake router, and the SOAP client against a fake HTTP server on 127.0.0.1. No Windows headers here.

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace net::upnp {

// ---- pure helpers -------------------------------------------------------------------------------------------

enum class AddrKind {
    None,     // not an address (0.0.0.0, loopback, link-local, multicast, unparsable)
    Public,   // routable: friends can reach it
    Private,  // 10/8, 172.16/12, 192.168/16: another router sits in front (double NAT)
    Cgnat,    // 100.64.0.0/10: the provider shares one address between customers
};

bool ParseIpv4(const std::string& text, uint32_t& hostOrder);
std::string FormatIpv4(uint32_t hostOrder);
AddrKind Classify(const std::string& ipv4);
// "97.x.x.166": the only form of a public address that may reach a log file.
std::string MaskIp(const std::string& ipv4);

// Router errors. 0 = success, positive = the UPnP fault code (401, 402, 501, 600..899), negative = no answer from
// the router (transport / COM failure).
constexpr int kErrTransport = -1;
constexpr int kErrNoRouter = -2;

// The UPnP fault code behind an IUPnPService::InvokeAction failure HRESULT: 0x80040300 + (code - 600) for 600..899,
// 0x80040207/8/10 for 401/402/501. 0 = not a fault (a transport problem).
int CodeFromHresult(int32_t hr);

enum class Fault {
    None,
    NoSuchEntry,       // 713, 714: no mapping there
    Conflict,          // 718: the external port is mapped to another device
    PermanentOnly,     // 725: the router refuses leases
    SamePortRequired,  // 724: external port must equal the internal one
    NotAuthorized,     // 606, 729: the router blocks UPnP changes
    NoPortMaps,        // 728: table full
    WildcardOnly,      // 726, 727, 730
    Transport,         // no answer (kErrTransport / kErrNoRouter)
    Other,
};
Fault FaultOf(int code);

// [Coop] UPnPLeaseSec: 0 stays 0 (permanent), anything else is clamped to 120..86400.
uint32_t NormalizeLease(int raw);
// Seconds until the next renewal: half the lease (at least 30). A permanent mapping is re-checked every 10 minutes.
uint32_t RenewDelaySec(uint32_t leaseSec);
// External port to try on the n-th attempt: base, base+1, ... 0 when it would pass 65535.
uint16_t CandidatePort(uint16_t base, int attempt);
constexpr int kPortAttempts = 10;

// What a failed or non-public outcome says in the lobby (one line, plain words).
std::string FailureMessage(Fault fault, AddrKind kind, uint16_t port, int code);

// The leftover record: written before a mapping is added, deleted after it is removed. If it is still there at
// the next launch, the previous run ended without removing its mapping (crash, kill): that exact mapping goes first.
struct StateRecord {
    uint16_t externalPort = 0;
    std::string client;  // this PC's LAN address the mapping pointed at
};
std::string FormatState(const StateRecord& rec);
bool ParseState(const std::string& text, StateRecord& out);

// ---- the router, as an interface ---------------------------------------------------------------------------

struct Entry {
    uint16_t internalPort = 0;
    std::string client;
    bool enabled = true;
    std::string description;
    uint32_t leaseSec = 0;
};

class Igd {
public:
    virtual ~Igd() = default;
    // Each returns 0 or an error as described at kErrTransport.
    virtual int ExternalIp(std::string& ip) = 0;
    virtual int GetEntry(uint16_t externalPort, Entry& out) = 0;  // UDP, any remote host
    virtual int AddEntry(uint16_t externalPort, uint16_t internalPort, const std::string& client,
                         const std::string& description, uint32_t leaseSec) = 0;
    virtual int DeleteEntry(uint16_t externalPort) = 0;
    // The index-th entry of the router's table (GetGenericPortMappingEntry); 713 / 714 = past the end.
    virtual int EnumEntry(int index, uint16_t& externalPort, Entry& out) = 0;
};

struct Plan {
    uint16_t internalPort = 0;  // the bound game port
    uint16_t startPort = 0;     // first external port to try; 0 = internalPort
    std::string client;         // this PC's LAN IPv4
    std::string description = "DH2 coop";
    uint32_t leaseSec = 3600;
    std::function<bool()> cancelled;  // polled between router calls; empty = never
    std::function<void(uint16_t)> beforeAdd;  // called with the external port just before AddPortMapping
};

struct Outcome {
    bool ok = false;         // a mapping exists now
    bool cancelled = false;  // stopped before it was created
    std::string publicIp;    // the router's WAN address (full: only the status API may show it)
    AddrKind kind = AddrKind::None;
    uint16_t externalPort = 0;
    uint32_t leaseSec = 0;             // the lease the router accepted (0 = permanent)
    bool permanentFallback = false;    // the router refused leases (725): mapped with lease 0
    bool reusedStale = false;          // the external port already pointed at this PC (an earlier run's)
    bool verified = false;             // GetSpecificPortMappingEntry shows the mapping we asked for
    int portsTried = 0;
    Fault fault = Fault::None;
    int code = 0;
    std::string message;               // lobby line
    bool reachable() const { return ok && kind == AddrKind::Public; }
};

// WAN address, classification, external port search (a port mapped to another device moves on to +1..+9; one that
// points at this PC with our description or internal port is reused), AddPortMapping with the lease, 725 retry with
// lease 0. CGNAT and a missing WAN address stop before any mapping is made. A private WAN address (double NAT)
// still maps, so a forward on the outer router completes the path, but is not reachable().
Outcome EstablishMapping(Igd& igd, const Plan& plan);
// Re-adds the same mapping (refreshes the lease, restores it after a router restart). 0 or an error.
int RenewMapping(Igd& igd, const Plan& plan, const Outcome& out);

enum class Removal { Removed, AlreadyGone, NotOurs, Failed };
// Deletes the mapping unless the router shows it now belongs to something else.
Removal RemoveMapping(Igd& igd, uint16_t externalPort, const std::string& client, const std::string& description,
                      bool requireDescription, int* code = nullptr);
// Removes the previous run's mapping named by `rec`. Returns what happened to it.
Removal RemoveLeftover(Igd& igd, const StateRecord& rec, const std::string& description);
// Deletes every mapping with our description that points at `client` (table walk, at most 128 entries), except
// `keepExternalPort`. Returns how many were deleted.
int RemoveAllOurs(Igd& igd, const std::string& client, const std::string& description, uint16_t keepExternalPort);

// ---- SOAP over plain HTTP (the control side; see upnp.cpp for why not Windows' own control layer) --------------

struct Url {
    std::string host;
    uint16_t port = 80;
    std::string path = "/";
};
bool ParseUrl(const std::string& text, Url& out);  // http://host[:port][/path] only
// An absolute http URL for `ref` (absolute, root-relative or relative) against `base`; "" when it cannot be done.
std::string ResolveUrl(const std::string& base, const std::string& ref);

struct ServiceDesc {
    std::string type;        // urn:schemas-upnp-org:service:WANIPConnection:1
    std::string controlUrl;  // absolute
};
// The services in a device description (URLBase honoured), nested devices included.
std::vector<ServiceDesc> ParseDescription(const std::string& xml, const std::string& documentUrl);
// 3 WANIPConnection:2, 2 WANIPConnection:1, 1 WANPPPConnection:1, 0 anything else.
int ServiceScore(const std::string& type);

std::string XmlEscape(const std::string& s);
std::string XmlUnescape(const std::string& s);
// Text of the first <tag>...</tag> (or "" for <tag/>), entities decoded. False when there is none.
bool XmlValue(const std::string& xml, const std::string& tag, std::string& out);
// The UPnP errorCode of a SOAP fault (also when the HTTP status is 200); 501 for a fault without one; 0 = not a fault.
int SoapFaultCode(const std::string& body);
std::string BuildSoap(const std::string& serviceType, const std::string& action, const std::string& argsXml);

struct HttpReply {
    bool ok = false;  // a status line came back
    int status = 0;
    std::string body;
};
// One request on a fresh outbound TCP connection, answered within timeoutMs in total. Reads Content-Length, chunked
// and read-until-close bodies. Not for the game thread.
HttpReply HttpExchange(const Url& url, const std::string& method, const std::string& headers, const std::string& body,
                       uint32_t timeoutMs);
// An Igd that talks SOAP to `control` (tests point it at a fake server on 127.0.0.1).
std::unique_ptr<Igd> MakeSoapIgd(const Url& control, const std::string& serviceType, uint32_t timeoutMs = 4000);

// ---- service (worker thread) --------------------------------------------------------------------------------

enum class State { Off, Working, Open, Failed };

struct Status {
    State state = State::Off;
    uint16_t externalPort = 0;  // the port friends use (0 until mapped)
    uint16_t internalPort = 0;  // the game port being forwarded
    std::string publicIp;       // full address, for the lobby / join code only (never logged)
    AddrKind kind = AddrKind::None;
    bool mapped = false;        // a mapping exists on the router (also true for double NAT, where state is Failed)
    uint32_t leaseSec = 0;
    std::string message = "Internet: off";  // "Internet: open on port 27420"
};

struct Config {
    bool enabled = true;                 // [Coop] UPnP
    uint32_t leaseSec = 3600;            // [Coop] UPnPLeaseSec (NormalizeLease already applied)
    std::string description = "DH2 coop";
    std::string stateFile;               // "" = no leftover record
    std::string bindAddress;             // a non-loopback [Coop] BindAddress: used as the mapping's target
    uint32_t renewMsForTest = 0;         // tests: renew this often instead of at half the lease
};

void Configure(const Config& cfg);
// Hosting started on this UDP port, or stopped (0). Repeating the same port does nothing. Never blocks.
void Want(uint16_t internalPort);
// coop_upnp off / on: off removes the mapping now and keeps it off until on.
void SetUserOff(bool off);
// A line for the lobby when no mapping is wanted (disabled, loopback test copy). Ignored while a mapping exists.
void NoteOff(const std::string& message);
// Try again now (after a failure).
void Refresh();
// coop_upnp clean: remove every "DH2 coop" mapping that points at this PC except the live one.
void CleanStale();
Status GetStatus();
// Process exit or session end: remove the mapping and stop the worker, waiting at most waitMs. Safe to call twice.
void Shutdown(uint32_t waitMs);

// Test and tools: discovers the router on the calling thread (which must have COM initialised) and returns an Igd
// for it. `why` says what went wrong when it returns null; `info` names the router. Not used by the game path
// directly (the worker calls it too).
struct RouterInfo {
    std::string name;
    std::string serviceType;
    std::string gatewayHost;  // the router's LAN address (the host of its device description URL)
    std::string controlUrl;   // where its WAN connection service takes SOAP (lets a later search be skipped)
    int devicesFound = 0;
};
std::unique_ptr<Igd> OpenRouter(RouterInfo& info, std::string& why);
// This PC's LAN IPv4 on the interface that reaches `gatewayIp` ("" = the default route's interface).
std::string LocalAddressToward(const std::string& gatewayIp);

// Tests only: the worker finds its router through `opener` instead of OpenRouter, and ResetForTest returns the
// service to its start state (needs the worker finished or never started).
using Opener = std::function<std::unique_ptr<Igd>(RouterInfo&, std::string&)>;
void SetOpenerForTest(Opener opener);
void ResetForTest();

}  // namespace net::upnp
