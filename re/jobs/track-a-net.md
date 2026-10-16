# Job: Track A — engine net layer verdict (Ghidra slot A)

A previous agent was cut off before writing its summary. Its raw work is in `re\notes\net-*.c` and `re\notes\net-*.txt` (decomps of cNetServer/cNetClient/cNetGameState, driver functions, Main7 app and SpServer code, object manager, GameDll7). Read those first, then fill gaps with `re\scripts\decomp.ps1 -Slot A` / `probe.ps1 -Slot A` / `ghrun.ps1 -Slot A`.

Known facts and leads:
- EngineDll7 exports cNetServer (Create(int,int,int) at RVA 0xb20d0, CheckNewClients, DisconnectClient, DispatchMessages, AreRemotesIgnored, EnableRemotePing), cNetClient (TryConnect, ConnectionSucceeded), cNetGameState, cNetClientInfo. Create branches on the static `cObjManager::m_nDriverType` (0 vs non-zero) and allocates 0xb4 or 0x134 bytes.
- Debug strings name two drivers: `cLocalDriver::*` (loopback) and `cRdpDriver::*` (UDP: Listen, StartConnect, Connect, RequestServerList, GetServerInfo, ParseSegment, PacketDispatcher, RxTx, ReceivePacketFrom). String addresses around 0x1043e218–0x1043e934 in EngineDll7 .rdata.
- Main7.exe strings: sv_multiplayer, sv_port, sv_password, sv_fps, sv_changeMap, sv_restart, sv_stop, cl_join %s, cl_password, cl_playerName, cl_disconnect, cfLobby. Switches: -join (Main7), -dedicated (EngineDll7), -spmap (GameDll7).
- Last notes from the previous agent: `GetListenPort` (the only reader of sv_port) has zero callers. Main7 `FUN_00401dd0` references both `-join` and `-dedicated`.
- Runtime probe (retail run, launched with `+mod data -developper -cheats`): `driverType=0` the whole session; cNetServer RxTx/Dispatch counters stayed 0. The game checks switches `-exp`, `-spmap`, `-batchmode`, `-developper`, `-cheats`, `-map`, `-join`, `-testmodels`, `-debug`. GameDll7 runs `cConsole::Exec('menumap menumap')` at startup. Single-player in-map behaviour was not probed.

Answer, with evidence (function names, RVAs, decomp snippets):
1. Does single player run a local server plus a local client through cLocalDriver? Trace the full path: what calls cNetServer::Create (args) and how the client connects.
2. What sets m_nDriverType to the UDP driver: sv_multiplayer, -join, -dedicated, a menu? Trace what `-join <addr>` and `cl_join` do end to end. Is any path still reachable in retail, or is it stubbed out?
3. cRdpDriver: socket setup, default port, packet header, reliability and acks, sequence numbers, connect handshake, server list and info queries.
4. Replication model: which messages carry entity state server to client and input or commands client to server. Find the message type enum or dispatch table, and which entity methods serialize (Write/Read/Pack/Delta/NetSeg/ClientSpawn).
5. Multi-client: client arrays and limits, cNetClientInfo layout, what happens on a second connection, and what AreRemotesIgnored guards.
6. Verdict: Route N (revive native netcode), Route E (custom ENet layer) or hybrid. List concrete patches and hooks for Route N, ranked by risk. Then write a step-by-step runtime experiment plan to prove two local game instances can connect (exact command lines, console commands, cvars and hooks to log).

Deliverable: `re\notes\net-layer.md`, with sections Summary/Verdict, Architecture, Driver selection, cRdpDriver protocol, Replication, Multi-client, Patches needed, Runtime experiment plan, Open questions, and Proposed MEMORY facts.
