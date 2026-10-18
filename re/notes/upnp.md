# UPnP port forward and direct join (2026-10-18)

Built by the upnp agent (Sonnet, stopped by the user before its note), finished and integrated by the lead.

## What it does
- While this PC hosts (`NetHostPort()` non-zero), `coop/upnp_module.cpp` asks `net::upnp` to forward the bound UDP port.
- `net/upnp.cpp` worker thread (COM MTA): find the router with `UPnP.UPnPDeviceFinder` (`FindByType` IGD:1/IGD:2, about 9 s on this
  PC), take its description URL (`IUPnPDeviceDocumentAccess`), pick WANIPConnection:2/1 or WANPPPConnection:1, then raw SOAP:
  `GetExternalIPAddress` (classify public / private = double NAT / CGNAT), `GetSpecificPortMappingEntry` (port taken by another
  device: try +1..+9; ours: reuse), `AddPortMapping` (description "DH2 coop", lease `[Coop] UPnPLeaseSec` 3600, 725 -> lease 0),
  renew at half the lease, `DeletePortMapping` when hosting stops, on ExitProcess (hook, 1.5 s cap) and for a leftover recorded in
  `dh2coop-upnp.state` at the next launch. The router found last time is reused only while it is still the default gateway.
- Lobby (`overlay_module.cpp` `draw_host_addresses`): Internet (public IP:port + join code), Tailnet, LAN rows with Copy.
- Join code v2 (`coop/joincode.*`): `DH2-` + base64url(`v2|ip|port|password|name`).
- `net/ban_guard.h`: 5 BadPassword from one address in 10 min -> that address is dropped at connect for 10 min.

## eero quirks (this PC's router)
- UPnP faults come back as HTTP 200 with a SOAP fault: Windows' `IUPnPService::InvokeAction` turns them into 0x80040216, so the
  mod does its own SOAP.
- HTTP/1.0 replies without Content-Length; `GetGenericPortMappingEntry` closes without a reply.
- Leases are ignored: a 300 s mapping was still listed after it ran out (17:10 -> still there 17:16). A crash's forward stays until
  the next launch (leftover record) or `uninstall.ps1`.
- `HNetCfg.NATUPnP.StaticPortMappingCollection` is NULL even with UPnP on.

## Evidence (VERIFIED on this PC, 2026-10-18)
- 17:39 two windows, port 27425, `UPnPOnLoopback=1`: "AddPortMapping UDP ext 27425 -> 192.168.5.200:27425 lease 3600 -> ok",
  "forwarding ... verified in 9277 ms"; host lobby screenshot `re/notes/coop-local/lead-lobby-host.png`; client joined by typed
  `127.0.0.1:27425`; `coop_joinfield file` parsed the host's code.
- 17:40 console `exit` on the host: "ExitProcess(0) called from MSVCR70.dll+0x442a: removing the port forward",
  "DeletePortMapping UDP ext 27425 -> ok (304 ms)"; read-only router query afterwards: 714 (gone).
- net-test 3688/0 (join code v2), dh2upnp_test 293/0, installer-test all pass.

## UNVERIFIED
- A friend joining from another network through the forward (needs a second connection, e.g. a phone hotspot).
- Window X / menu Quit (same ExitProcess path as `exit`, not run).
- A crash leftover removed at the next launch (unit-tested against the fake router only).
- Routers that need the same external port (724), refuse leases (725), double NAT / CGNAT messages (unit-tested only).

## Not done on purpose
- No hidden per-session password: the user wants friends to be able to type the public IP:port. `[Coop] Password` is the opt-in.
