# Engine net layer (Track A)

Evidence is from the existing `re/notes/net-*.c` dumps, plus a static pass over `game\Bin\Main7.exe`, `game\Bin\EngineDll7.dll`, `game\Bin\CommonDll7.dll` and `game\Data\GameDll7.dll` (pefile). RVAs are image-base relative. Engine preferred base is `0x10000000`; it relocates at runtime.

## Summary/Verdict

**Route E.** Do not revive native netcode as the co-op path.

Single player does not run a local server plus a local client on `cLocalDriver`. The thing the log calls a server is `cSpServer` inside `Main7.exe`: one in-process object that loads the map, spawns the hunter, and copies bytes between its own `cByteStream`s. `cNetServer::Create` (EngineDll7 RVA `0xb20d0`), `cNetClient::Init` / `TryConnect`, and `cObjManager::SetDriverType` have no code callers in EngineDll7 and are not imported by Main7, CommonDll7, or GameDll7. `cObjManager::m_nDriverType` (RVA `0x5826e0`) is a BSS zero. That matches the retail probe: `driverType=0` for the whole session and the RxTx / Dispatch counters stayed 0, because those functions were never entered.

`cRdpDriver` is a real UDP reliable/unreliable transport (handshake, seq, acks, server-info query). Nothing in this build calls it. The bytes it would carry are an opaque custom segment, not entity snapshots. `cObjEntity::ClientSpawn` (`0xabc90`) and `cObjEntity::ClientCommand` (`0xb9480`) return immediately. GameDll's only server-message handler, `MOD_SvParseMessage` (`0x3140`), implements message `0x87` and treats `0xff` as a no-op.

A hybrid that keeps `cRdpDriver` as the socket layer and writes new replication on top is possible and worse than ENet: the driver has no `WSAStartup` import, no NAT helper, and no game vocabulary. Copy the *shape* of `cSpServer` (one host ticks the world, a client is a view) in the new layer. Do not try to make the old streams hit the wire.

## Architecture

Two stacks that never meet.

**In-process game server (`cSpServer`, Main7).** `cGameManagerSp::cGameManagerSp` (`0x16220`) allocates `0x1fd8` bytes and calls `cSpServer::cSpServer` (`0x1b390`). The object extends `cObjManager`. Constructor state:

- `+0x28` server state = 0 (`Server_NotStarted`). Read by `FUN_0041e600` (`0x1e600`). Written by `FUN_0041e5d0` (`0x1e5d0`), which also copies the old value to `+0x2c`.
- `+0x1fd4` client-info state = 4 (not in the 0..3 table, so the name lookup returns `UnknownSvClientState`).
- Five `cByteStream`s, each `0x2000`/`0x2000`, at `+0x1e6c`, `+0x1eac`, `+0x1eec`, `+0x1f54`, `+0x1f94`.
- Cvar pointers: `sv_port` `+0x1e40`, `sv_fps` `+0x1e44`, `sv_multiplayer` `+0x1e48`, `sv_password` `+0x1e4c`, `cfMaxClients` `+0x1e58`. `cfNumClients` is bound to `+0x1d18`. `cfLobby` is bound to global `DAT_0043d6b4` (RVA `0x3d6b4`).

Server-state table at RVA `0x3bae0`, read by `FUN_0041e630` (`0x1e630`), four records, default name `UnknownServerState`:

| id | name |
|---|---|
| 0 | `Server_NotStarted` |
| 1 | `Server_Listening` |
| 2 | `Server_GameStarting` |
| 3 | `Server_GameStarted` |

Client-info table at RVA `0x3bb04`, same record shape, lookup cmp site RVA `0x1e697`:

| id | name |
|---|---|
| 0 | `ClientInfo_Joining` |
| 1 | `ClientInfo_Authenticated` |
| 2 | `ClientInfo_GameStarting` |
| 3 | `ClientInfo_GameStarted` |
| other | `UnknownSvClientState` |

Startup path for a map (`spmap` / `menumap` / `-spmap` / `-map`):

1. `cSpServer::StartGame` (`0x1ed80`). If state is 0, `StartListen` (`0x1ba40`). If state is 2 or 3, `StopServer` (`0x1b060`) first.
2. `StartListen` zeroes `+0x1d18` (`cfNumClients`) and calls virtual `+0x0c` on the `sv_multiplayer` cvar with the literal `"0"` (string RVA `0x2a8e8`). Then `FUN_0041e5d0(this, 1)` so the state is `Server_Listening`.
3. On a successful BSP load with the second argument 0, `StartGame` sets state 3 (`Server_GameStarted`) and calls `ClJoin` (`0x1e410`).
4. `ClJoin`, when `+0x1fd4 == 4`, builds a zeroed local client record, calls virtual `+0x78`, then `Listen` (`0x1bb90`) and, because state is 3, `ClientJoinGame(0)` (`0x1e0c0`).
5. `ClientJoinGame` sets `+0x1fd4 = 3` (`ClientInfo_GameStarted`) and `+0x1ba8 = 3`. With `cfLobby == 0` it looks for a saved record whose name is the literal `127.0.0.1:8888` (`sprintf` of `"127.0.0.1"` and `0x22b8`, stride `0x268`, name at record `+0x164`). No match: `SpawnClient` (`0x1deb0`) or `SpawnObserver` (`0x1dc30`). Those call GameDll through the export thunks (`MOD_SvCreatePlayerEntity` `0x29b0`, `MOD_SvSpawnPlayerEntity` `0x2b30`).
6. The log line is `cDebugSystem::SendToLog(0x21, "loading time : %dms (map : %dms, client: %dms)\n")` at the end of `StartGame`. That is the `[SERVER] loading time ...` line in `debug.log`. The `client:` number is time spent in `ClJoin`, not a socket client.

`Listen` only runs the byte-stream parse when state is 1 or 3. `Update` (`0x1bcd0`) only ticks the world when state is 3. `CheckAndParseClientMessage` (`0x1b200`) reads a byte from the server stream at `+0x1e6c` and passes it to virtual `+0x88`, then reads a byte from the client stream at `+0x1f54` and passes it to `ClParseMessage` (`0x1c480`). `EndMessage` (`0x1ad70`) copies bits from one of those streams into the other. All of that stays inside the one object.

Virtual `+0x88` is the thunk at `0x1b650`, which calls `[0x43d0c4]`. `cGameApp::InitGameDll` (`0x19e10`) copies the GameDll export block onto `DAT_0043d060`. Index 25 of that block is `MOD_SvParseMessage`.

**Transport (`cNetServer` / `cNetClient` / driver).** Exported, never imported. `cNetServer` extends `cObjManager`. Constructor `0xb3420`: client list at `+0x28`, count `+0x2c = 0`, driver `+0x3c = 0`, port `+0x34 = -1`, max `+0x38 = 0`, next client id `+0x40 = 1`. `cNetClient` constructor `0xad500`: driver pointer `+0x24`, connected channel `+0x28`, name at `+0x2c` set to `"Not available"`, connected flag `+0xb4`, `cNetGameState` embedded at `+0xb8`. The constructor always builds the RDP object (`malloc(0x134)`, `FUN_103336e0`). `Init` immediately calls `ResetLanDrv`, which replaces it according to `m_nDriverType`.

`Create` ignores its first argument. It stores arg2 at `+0x34` (port) and arg3 at `+0x38` (max clients), builds the driver, then calls driver virtual `+0x20` with `(&"abc", 4)`. That virtual is `FUN_1032a6e0` (`0x32a6e0`): it stores a server-info blob `{0x0b, u16 length, bytes}`.

## Driver selection

`m_nDriverType` is read in exactly two places, both in EngineDll7:

- `cNetServer::Create` (`0xb20d0`)
- `cNetClient::ResetLanDrv` (`0xad800`), which `Init` (`0xad9b0`) always calls

The only write is `cObjManager::SetDriverType` (`0xaad50`). Its only references are the export directory. Main7, CommonDll7 and GameDll7 do not import `SetDriverType`, `GetDriverType`, `cNetServer::*` or `cNetClient::*`. No string `SetDriverType` exists outside the Engine export name.

| `m_nDriverType` | object | size | constructor | vtable |
|---|---|---|---|---|
| 0 | `cLocalDriver` | `0xb4` | `FUN_1032d300` (`0x32d300`) | `0x43e1e0` |
| nonzero | `cRdpDriver` | `0x134` | `FUN_103336e0` (`0x3336e0`) | `0x43e844` |

Both constructors are passed `0x2000000`. The local constructor ignores it. The RDP constructor stores it at `+0x124`. What the constant means is `UNVERIFIED`.

`cLocalDriver` is in-process only. `Connect` (`0x32d570`) and `UpdateConnectStatus` (`0x32d7a0`) rendezvous through two globals, `DAT_10698ab4` (RVA `0x698ab4`) and `DAT_10698ab8` (RVA `0x698ab8`). One side sets a flag; the other side, seeing the flag, allocates a `0x40`-byte channel (`FUN_1032cbe0`) and returns it. Two processes do not share those globals. Loopback cannot join a second game instance.

### What does not select the UDP driver

**`sv_multiplayer`.** Registered in `cSpServer::RegisterConsole` (`0x21750`) as an int cvar, default `"0"`, range `"0"`..`"1"`, pointer stored at `cSpServer+0x1e48`. The only later use of that pointer is `StartListen`, which passes `"0"` to virtual `+0x0c`. There is no branch on its value. The method name of `+0x0c` is `UNVERIFIED`; the argument is the literal `"0"`.

**`-join <addr>`.** `FUN_00401dd0` (`0x1dd0`) and `cGameApp::FUN_0040f310` (`0xf310`).

- A second process hits `CreateMutexA(..., "GI UNIQUE MUTEX")`. If `GetLastError()==0xb7` (`ERROR_ALREADY_EXISTS`) and the command line has neither `-dedicated` nor `-join`, startup jumps to the failure path. So `-join` (or `-dedicated`) is a real, reachable way to start a second process.
- `FUN_0040f310` then does `cConsole::Exec("cl_join %s")` with the next switch argument. The only `cl_join` string in any of the four binaries is that format at Main7 RVA `0x29cb0`. `cSpServer::ClRegisterConsole` (`0x1d560`) registers `cl_disconnect`, `cl_playerName` and `cl_password`. It does not register `cl_join`. The Exec has no handler.

**`-dedicated`.** Three effects, none of them a socket:

- `FUN_00401dd0` sets `DAT_0043d2bc = 1` (RVA `0x3d2bc`). That also bypasses the mutex, as above.
- `InitConsole` (`0x329ec0`) treats `-dedicated` like `-console` and opens `cConsoleWnd`.
- Back in `FUN_00401dd0`, if the flag is set: `cConsole::RealExec("r_api 0; snd_usesound 0")`. `FUN_0040f310` sets `m_bAllowBackground = 1`. `FUN_00407230` skips the sound manager.

It does not call `SetDriverType`, `Create`, or `StartListen` by itself. A `-dedicated` process still reaches `FUN_0040f310`, which still runs `InitGameDll` and the menu init. Map load still happens only if something execs `spmap` / `menumap` / `-spmap` / `-map`.

**`sv_port`.** Default `sprint("%d", 0x6ab6)` = **27318**, clamp strings `"1000"`..`"32768"`. `cSpServer::GetListenPort` (`0x21610`) reads it (or returns `0x6ab6` if the slot is missing or its type byte at `+0x14` is not 1). Callers of `0x21610`: none. `Create` never sees this cvar.

**Menu.** No menu path in the decompiled app or `cSpServer` calls `SetDriverType` or `Create`. `cfLobby` only changes the `ClientJoinGame` save-record branch.

## cRdpDriver protocol

Class is unnamed in the export table. Identity is the vtable at `0x43e844` and the catch strings `cRdpDriver::*` at `0x43e638`–`0x43e934`.

Socket, in `Listen` (`0x333a40`), `StartConnect` (`0x332ad0`) and `Connect` (`0x333d20`):

```
socket(2, 2, 0x11)   // AF_INET, SOCK_DGRAM, IPPROTO_UDP
```

`Listen` then `setsockopt(SOL_SOCKET=0xffff, SO_REUSEADDR=4)` and `setsockopt(SOL_SOCKET, SO_BROADCAST=0x20)`, and `bind`s `sockaddr_in` with `htons(port)`. Failure prints `Listen: bind on port %d failed\n`. There is no compiled-in default port. The port is the second argument of `Create`, later passed by `CheckNewClients` into driver virtual `+0x08` (`Listen`).

`WSAStartup` / `WSACleanup` are not imported and the strings are absent. A first `socket()` call from this DLL returns `WSANOTINITIALISED` unless some other module already started Winsock. None of the four binaries imports `WSAStartup`.

### Packet header

`cRdpDriver::ParseSegment` (`0x334440`) switches on byte 0. Lengths below are what the function returns (bytes consumed).

| byte | name (from behavior) | layout |
|---|---|---|
| `0x01` | connect request | 9 bytes sent by `Connect`: `01 00 00 00 00` then the u32 at driver `+0xd0`. Constructor sets that field (`param[0x34]`) to `0x200`, so a fresh driver sends `01 00 00 00 00 00 02 00 00`. |
| `0x02` | connect answer | 10 bytes. `[0]=0x02`, `[1]` bit 0 = accepted (0 = full), `[2..3]=1`, `[4..5]=0`, `[6..9]=` driver `+0xd0`. Built in the `0x01` case. |
| `0x03` | handshake finish / reliable ack | Client sends 2 bytes: `03`, then answer byte `[2]`. If the peer is in state 4 and byte `[1]` matches the peer seq at `+0x10`, state becomes 5 (connected) and seq increments. Also walks the reliable queue and drops the matching entry. |
| `0x04` | reliable payload | `[1..4]` seq u32, `[5..6]` length u16, `[7..]` payload. Consumed = length + 7. Copied and queued (`FUN_10331b30`). |
| `0x05` | drop peer | 1 byte. Sets the matching connection's state dword `+0xb0` to 6. |
| `0x06` | ping | 5 bytes consumed. Replies 2 bytes: `07`, echo of `[1]`, via `sendto`. |
| `0x07` | pong | 2 bytes. Refreshes the peer tick. |
| `0x08` | ack window | 9 bytes. Forwards two u32s at `[1]` and `[5]` to `FUN_10330fd0`. |
| `0x09` | unreliable payload | Same header as `0x04`. Accepted only when `[1..4]` is strictly greater than the last seq at peer `+0xc0` (`puVar5[0x30]`). Older seqs increment a drop counter at peer `+0xe4` and are discarded. |
| `0x0a` | server-info query | 1 byte. If driver `+0x0c` holds an info blob (the `0x0b` buffer from `Create`) and the sender is not this host, `sendto` echoes that blob. |
| `0x0b` | server-info response | `[1..2]` length u16, `[3..]` payload. Resolves the sender to a name and calls driver virtual `+0x28`. |

`AreRemotesIgnored` is checked at the top of `ParseSegment`: if driver `+4` is nonzero and `FUN_10331fd0` (`0x331fd0`) returns false, the datagram is dropped. `FUN_10331fd0` returns true only for `127.0.0.1` or an address returned by `gethostbyname(gethostname())`.

### Reliability

`cRdpDriver::PacketDispatcher` (`0x333170`) walks the connection list. For each peer not in state 1, 6 or 7 it pulls the reliable queue (virtual `+0x0c` on the queue object) and resends through `FUN_10332310` when the ack has not arrived and `GetTickCount() - lastSend > timeout` at peer `+0xb4`. A successful send refreshes the tick and bumps counters at driver `+0xf4` / `+0xf8`. State 7 older than 15000 ms is forced to state 1 (dead). States 1 and 6 signal two events at peer `+0x124` and `+0x128` and unlink the peer.

Peer state dword `+0xb0`, written by `ParseSegment` and the dispatcher:

| value | when |
|---|---|
| 4 | `0x01` accepted, object allocated (`FUN_10331450`, `0x15c` bytes) |
| 5 | `0x03` completed the handshake, or the first `0x04` arrived while still in 4 |
| 6 | `0x05` received |
| 7 | dispatcher is holding a peer it is about to drop; 15 s later it becomes 1 |
| 1 | dead. Early-out in `ParseSegment` unless the opcode is `0x0a` or `0x0b` |

`Connect` retries the `0x01` datagram up to 10 times, waiting 500 ms (`FUN_103320c0(500)`) and reading with `ReceivePacketFrom` (`0x332160`). It accepts the answer only when byte 0 is `0x02` and byte 1 has bit 0 set. Bit 0 clear (server full) closes the socket and returns 0. On success it sends `0x03` and stores the channel with state 5.

`cNetClient::Init(ignored, host, port)` calls driver `+0x18` = `StartConnect` (`0x332ad0`): create the UDP socket, `inet_addr` / `gethostbyname`, store the sockaddr. It does not send. `TryConnect` calls driver `+0x1c` = `UpdateConnectStatus` (`0x334170`), which is what reaches `Connect`. `ConnectionSucceeded` (`0xadd60`) is true once `+0x28` (the channel) is non-NULL, else it calls virtual `+0x44` (the slot `TryConnect` occupies on `cNetClient`).

`cNetGameState` framing sits inside the `0x04` / `0x09` payload, not in front of it. `SendCustomSegment` (`0xb0920`):

- `param_3 == 2` (reliable): asks the channel virtual `+4` for a buffer, then writes `04`, u16 length, payload. Payload capped at `0x8000`.
- `param_3 == 1` (unreliable): writes a 7-byte prefix whose first byte is `0x09`, then `04`, u16 length, payload, and sends through channel virtual `+0x10`.

`ParseNetSegment` (`0xb0530`) walks a buffer and accepts only chunks whose first byte is `0x04`. `ParseCustomSegment` (`0xb02c0`) reads the u16 length and queues the payload. It does not look at entity ids. `ReadCustomMessage` (`0xb0190`) dequeues that payload. The engine net layer is a byte pipe.

### Server list

`RequestServerList` is `FUN_10332d80` (`0x332d80`), driver virtual `+0x2c`. Failure strings: `RequestServerInfo: Unable to create socket`, `failed to create search socket (err=%d)`, `Failed to bind to %d`. `GetServerInfo` is `FUN_10332760` (`0x332760`), virtual `+0x24`. The query datagram is opcode `0x0a`; the answer is the `0x0b` blob. `Create` publishes `"abc"` (4 bytes) as that blob. Main7 exports `EngClSearchServers`, `EngClSearchServer`, `EngClGetFoundServersCount`, `EngClGetFoundServerInfo` as thunks in the engine-export table filled by `EngineExport` (`0x73c0`). Those thunks forward into whatever `cGameManagerSp` points at. They do not call `cRdpDriver` themselves, and nothing in the traced startup calls them.

`cLocalDriver` has the same vtable slots (`Connect` `0x32d570`, `ParseSegment` `0x32def0`, `RxTx` `0x32e450`, `RequestServerList` `0x32dae0`) and no `socket` calls. `ParseSegment` there moves bytes between the two global channels.

## Replication

There is no entity replication on this wire.

- No export on `cObjEntity` is named Write, Read, Pack, Delta, or NetSeg. `ClientSpawn` and `ClientCommand` are empty. `ClearState` (`0xb7510`) zeroes a `tEntityStateDescriptor` of `0x2c` dwords plus a few sentinel fields (`entity id -1`, model `-1`, byte `0xff`). Nothing in `cNetGameState`, `cNetServer`, or `cRdpDriver` reads or writes that struct.
- `cObjManager::GetReplicatorByClassId` (`0xbd320`) indexes `m_pReplicators`. `cEntityReplicator` is the class factory (`AddReplicator`), the same mechanism as `cModelReplicator`. It is not a delta compressor.
- Server to client in the transport: whatever the caller passed to `cNetServer::SendMessageToClient` (`0xb2560`) or `cNetGameState::SendCustomSegment`. Retail has no such caller. The only framing is `0x04` / `0x09` outside and `0x04 | u16 | bytes` inside.
- Client to server: `cNetClient::SendCustomSegment` (`0xae530`) into the same pipe. `ReceiveClientMessages` (`0xb2ab0`) and `ReceiveServerMessages` (`0xae0e0`) both end in `ParseNetSegment`, which only queues.
- Game-level messages that do exist are in-process. `MOD_SvParseMessage` (`GameDll7` `0x3140`):
  - `0x87`: `ReadInt8` default `8`, then a string, then if a global entity pointer is set, call virtual `+100` with `0x80` and virtual `+0x7c` with the string, float `0x40200000` (2.5f) and that byte. Returns 1.
  - `0xff`: return 1.
  - `0x80` and every other id: return 0. `cSpServer::ParseMessage` (`0x1aed0`) then rewinds the stream (virtual `+0x1c`).
- `MOD_SvClientCommand` (`0x1c90`) calls virtual `+0xb8` on an entity pointer. That is a direct call, not a datagram. `ClSendCommandsToServer` (`0x1d400`) samples a 12-dword `tClientCommand`-sized buffer through function pointer `DAT_0043d0dc` and pushes it with `FUN_0041c090`. Same process.

So the "replication model" in retail is: the server and the client are the same `cSpServer`, and the player entity is spawned locally. There is nothing to delta.

## Multi-client

**Transport.** Clients are an intrusive linked list at `cNetServer+0x28`, not a fixed array. `cNetClientInfo` is allocated at `0x40` bytes in `CheckNewClients` (`0xb3660`):

| offset | contents |
|---|---|
| `+0x00` | vtable |
| `+0x04` | embedded `cNetGameState` (constructor writes through `+0x30`, so the state object ends at `+0x34`; `0x04+0x34=0x38`) |
| `+0x38` | channel pointer (the object `Listen` returned) |
| `+0x3c` | client id |

`CheckNewClients` accepts a new client when `+0x2c < +0x38` (count < max). The only write of `+0x2c` found is the constructor storing 0, and `GetClientCount` (`0xb17a0`) returns it. The compare therefore stays true for any max above 0 unless some other store was missed (`UNVERIFIED` beyond the decompiled methods). The driver has its own cap: `ParseSegment` case `0x01` accepts only when driver `+0xdc` < driver `+0xcc`. `Listen` stores `Create`'s max argument at `+0xcc`. On accept it allocates a `0x15c`-byte peer, copies 4 bytes from the request at `+1` into peer `+0x14` and 4 bytes from `+5` into peer `+0x13c`, sets state 4, and replies `0x02` with the accept bit set. When the cap is hit it replies `0x02` with the accept bit clear and does not allocate. Ids come from `cNetServer+0x40`, which starts at 1 and increments per accepted client. `SendMessageToClient(..., clientId)` with id `-1` walks the whole list.

`DisconnectClient` (`0xb2fb0`) and the dead-channel path in `ReceiveClientMessages` call virtual `+0x44` (`ClientDisconnected`, `0xb26d0`), which only resolves the name and port. The body does not free a game entity. The list node is then destroyed.

**`AreRemotesIgnored`.** `IgnoreRemotes` (`0xb1910`) writes its argument to `driver+4` and returns 1, or returns 0 if no driver. `AreRemotesIgnored` (`0xb1960`) reads that dword, or 0 if no driver. The base driver constructor (`FUN_1032b0c0`, `0x32b0c0`) sets `+4` to 0, so remotes are allowed until someone calls `IgnoreRemotes(1)`. No such caller exists. When the flag is nonzero, `ParseSegment` keeps only datagrams from `127.0.0.1` or this machine's own hostname address. It is a local-IP filter, not "ignore the listen socket".

**Game object.** Separate from the transport list. `cfMaxClients` default `"1"`, range `"1"`..`"32"`, stored at `+0x1e58`, never passed to `Create`. `cfNumClients` is `+0x1d18`, zeroed by `StartListen`, decremented (not below 0) by `ClDisconnect` (`0x1d2d0`). The `0x268`-stride array at `+0x1e38` (count `+0x1e34`) is the save/load client record searched by the string `127.0.0.1:8888`. `SpawnClient` always takes `param_1 == 0` in the traced path and writes one view entity at `+0x1c74`. A second remote player has no slot in this code.

## Patches needed

Route N, ranked from least risky to most. None of these make animals, weapons, or a second hunter replicate. They only reach a point where two processes can exchange a buffer.

1. **Log only.** Hook `SetDriverType` (`0xaad50`), `Create` (`0xb20d0`), `cNetClient::Init` (`0xad9b0`), `TryConnect` (`0xadc00`), `CheckNewClients` (`0xb3660`), `DispatchMessages` (`0xb2260`), and `WS2_32!sendto` / `recvfrom`. Print args and `WSAGetLastError`. Changes nothing. Confirms the retail path still never enters them.
2. **`WSAStartup(MAKEWORD(2,2), &data)` once, from the ASI, before any driver call.** The DLL cannot do this itself. Low risk. Without it step 4 fails inside `socket()`.
3. **`SetDriverType(1)` in both processes before `Create` / `Init`.** One global. Low risk until something calls `ResetLanDrv`; then the process gets `cRdpDriver` instead of `cLocalDriver`. Retail never calls either, so the observable change is zero until step 4.
4. **Host pump.** `new cNetServer` via export `??0cNetServer@@QAE@XZ` (`0xb3420`), then `Create(0, 27318, 4)`. Each frame, on the thread that already pumps the game: `RxTx`, `CheckNewClients`, `ReceiveClientMessages`, `DispatchMessages`. Medium risk: uses the engine allocator and a UDP port, still does not touch `cSpServer` or entities. `Create`'s first argument is ignored. Do not read `sv_port`; nothing else does.
5. **Client pump.** `new cNetClient` (`0xad500`), `Init(0, "127.0.0.1", 27318)`, then `TryConnect` until `ConnectionSucceeded`. Each frame `RxTx`, `ReceiveServerMessages`, and one `SendCustomSegment` of a fixed 8-byte marker. Medium risk, same scope as step 4. `Init`'s first argument is ignored.
6. **Leave `IgnoreRemotes` alone.** Default 0 is what a second PC needs. Calling `IgnoreRemotes(1)` would make the host drop every address that is not local. That includes a second machine, and it can also drop a second process on the same machine if the stack reports a non-loopback source address.
7. **Do not hook `sv_multiplayer`, `-join`, or `-dedicated` as the enable.** `StartListen` feeds `"0"` back into the cvar on every map load. `-join` execs a missing command. `-dedicated` turns rendering and sound off. Using `-dedicated` only so process B survives the mutex is fine; using it as the host is not, because `r_api 0` leaves no world to look at.
8. **High, and still not co-op.** Forwarding `cSpServer`'s `cByteStream`s into `SendCustomSegment`, or teaching `MOD_SvParseMessage` new ids. The streams are not snapshots, `ClientSpawn` is empty, and the client record stride `0x268` is a save blob keyed by `127.0.0.1:8888`. This is a new protocol written into old functions. That is Route E with extra coupling. Stop here.

## Runtime experiment plan

Goal of the experiment: two local processes complete the `cRdpDriver` handshake and each delivers one custom segment. Not: two hunters in one map. This job did not launch the game.

Use two sandbox copies so `debug.log`, `Cfg\console.cfg` and `data.arc` are not shared. Whether a single install file-locks the archive is `UNVERIFIED`. Disc image mounted, working directory `Bin` of that copy, ASI loader already in place (`dinput8.dll`).

**Process A, host window**

```
Bin\Main7.exe +mod data -developper -cheats
```

**Process B, second instance**

```
Bin\Main7.exe +mod data -developper -cheats -join 127.0.0.1
```

`-join` is only there for the mutex. Expect a console complaint for the unknown command `cl_join 127.0.0.1`, or a silent no-op. Expect no UDP traffic. That negative result is the control.

**ASI, both processes, after `InitEngineDll` has returned** (EngineDll7 is loaded and relocated; resolve exports by name, never by the preferred base):

1. `WSAStartup(0x0202, &wsa)`.
2. `cObjManager::SetDriverType(1)`.
3. Log thread installs detours on `sendto`, `recvfrom`, `cNetServer::Create`, `cNetClient::Init`, `cNetClient::TryConnect`, `cNetServer::CheckNewClients`, `cRdpDriver::ParseSegment` (`0x334440`). Each log line: tick, direction, opcode byte, length, `ntohs` of the sockaddr port, `inet_ntoa` of the address.
4. Host only (decide host by the absence of `-join`): construct `cNetServer`, `Create(0, 27318, 4)`.
5. Client only: construct `cNetClient`, `Init(0, "127.0.0.1", 27318)`.
6. Once per frame, from the existing frame hook (do not spawn a thread; `PacketDispatcher` takes peer critical sections and the game is not built for a second pump):
   - Host: `RxTx`, `CheckNewClients`, `ReceiveClientMessages`, `DispatchMessages`. On a nonnegative return from `CheckNewClients`, log the id and `cNetClientInfo::GetRemoteIp` / `GetRemotePort`.
   - Client: `TryConnect`. When `ConnectionSucceeded` becomes nonzero, send `SendCustomSegment` of the 8 bytes `44 48 32 43 4f 4f 50 01` with the reliable flag (`param_3 = 2`), once.
7. Host, inside the `ReceiveClientMessages` hook or by calling `ReadCustomMessage` on that client's `cNetGameState`: log the payload. Pass condition is the 8 bytes above, from `127.0.0.1`, after a `0x01` / `0x02` / `0x03` exchange in the `sendto` log.
8. Quit both. Do not load a hunt map for this test. `StartListen` does not open the socket, and a map load only adds noise.

Cvars and switches that are **not** part of the pass condition: `sv_multiplayer`, `sv_port`, `sv_password`, `cfMaxClients`, `cfLobby`, `cl_password`, `cl_playerName`, `-dedicated`, `-spmap`. Setting `sv_multiplayer 1` in `console.cfg` is overwritten in spirit by `StartListen` pushing `"0"`, and it is never branched on anyway.

Fail signatures worth recording:

- `socket` returns `-1` and `WSAGetLastError` is `10093`: step 1 did not run.
- `Listen: bind on port 27318 failed`: something else owns the port, or a previous host did not close it.
- Answer `0x02` with byte 1 equal to 0: driver cap (`+0xcc`) was 0. `Create`'s third argument did not stick.
- Handshake completes but the 8-byte payload never surfaces: `DispatchMessages` / `ReceiveClientMessages` not called on the same objects that connected, or `ParseNetSegment` rejected a chunk that was not `0x04`.

## Open questions

- The `cConsoleVar` virtual at `+0x0c` is called with `"0"` from `StartListen`. The slot's real name (set vs. notify) was not decompiled.
- `cNetServer+0x2c` is only stored as 0 in the decompiled methods, yet `CheckNewClients` compares it to the max. Either the cap is accidentally always open, or a list helper increments it and Ghidra folded that store. The driver cap at `+0xcc` is the one `ParseSegment` actually tests. Where `+0xdc` is incremented (list helper `FUN_10331100` vs. an explicit add) was not pinned to one instruction.
- `0x2000000` stored at `cRdpDriver+0x124`, and the `0x200` cookie at `+0xd0`, have no named constant.
- Message `0x87` in `MOD_SvParseMessage` reads a byte and a string and calls two entity virtuals. What the player does with that (a shot, a bark, a camera) belongs to the object-model track.
- A second process against the same `data.arc` was not launched. The mutex behavior is from the code; the file-sharing behavior is not.
- `tEntityStateDescriptor` may still be filled by entity `Think` / save code. That is outside this track. It is not read by the net classes.

## Proposed MEMORY facts

- `cNetServer::Create` (export `?Create@cNetServer@@QAEHHHH@Z`, RVA `0xb20d0`) ignores its first argument, stores arg2 as the listen port at `+0x34` and arg3 as the max client count at `+0x38`, and allocates `cLocalDriver` (`0xb4`, `FUN_1032d300`) when `cObjManager::m_nDriverType==0` or `cRdpDriver` (`0x134`, `FUN_103336e0`) otherwise. Evidence: decomp in `re/notes/net-decomp.c`.
- `cObjManager::m_nDriverType` (RVA `0x5826e0`) is written only by `SetDriverType` (`0xaad50`) and read only by `Create` and `cNetClient::ResetLanDrv` (`0xad800`). `SetDriverType` has no code caller and is not imported by Main7, CommonDll7, or GameDll7. The BSS value is 0. Evidence: xref dump `re/notes/net-probe1.c`; import scan of the four retail binaries.
- Single-player "server" is `cSpServer` in Main7 (`ctor` `0x1b390`, `StartGame` `0x1ed80`, `StartListen` `0x1ba40`, `ClJoin` `0x1e410`, `ClientJoinGame` `0x1e0c0`). `StartGame` logs `loading time : %dms (map : %dms, client: %dms)\n` via `SendToLog` channel `0x21`. It never calls `cNetServer`. Evidence: `re/notes/net-decomp-main7-spserver.c`; Main7 imports.
- Server state at `cSpServer+0x28`: 0 `Server_NotStarted`, 1 `Server_Listening`, 2 `Server_GameStarting`, 3 `Server_GameStarted`. Table RVA `0x3bae0`, lookup `FUN_0041e630` (`0x1e630`). Client-info state at `+0x1fd4`: 0 `ClientInfo_Joining`, 1 `ClientInfo_Authenticated`, 2 `ClientInfo_GameStarting`, 3 `ClientInfo_GameStarted`, anything else `UnknownSvClientState`. Table RVA `0x3bb04`, lookup cmp at `0x1e697`. Constructor writes 0 and 4 respectively; `ClientJoinGame` writes 3.
- `StartListen` calls virtual `+0x0c` on the `sv_multiplayer` cvar with the string `"0"` (RVA `0x2a8e8`) and zeroes `+0x1d18`. `sv_multiplayer` is not branched on. Evidence: bytes at `0x1ba81` `8b 8e 48 1e 00 00` and the following `push 0x42a8e8; call [eax+0xc]`.
- `GetListenPort` (`0x21610`) is the only reader of `sv_port`. Default and fallback are `0x6ab6` (27318). Callers: none. `ClientJoinGame` formats `127.0.0.1:8888` (`0x22b8`) as a save-record key, not as a bind.
- `-join` is handled in `FUN_0040f310` (`0xf310`) by `cConsole::Exec` of `cl_join <arg>`. The string `cl_join` occurs once in Main7 (RVA `0x29cb0`) and nowhere else. No console registration exists. `-dedicated` sets `DAT_0043d2bc`, opens the console window (`InitConsole` `0x329ec0`), execs `r_api 0; snd_usesound 0`, and is one of the two switches that let a second process pass `CreateMutexA(..., "GI UNIQUE MUTEX")` in `FUN_00401dd0` (`0x1dd0`).
- `cRdpDriver::Listen` (`0x333a40`) opens `socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)` and `bind`s the port it is given. `ParseSegment` (`0x334440`) opcodes `0x01` connect, `0x02` answer, `0x03` ack, `0x04` reliable, `0x05` drop, `0x06`/`0x07` ping/pong, `0x08` ack window, `0x09` unreliable, `0x0a`/`0x0b` server info. EngineDll7 does not import `WSAStartup`.
- `AreRemotesIgnored` (`0xb1960`) reads driver `+4`. Nonzero makes `ParseSegment` drop datagrams that are not from `127.0.0.1` or the local hostname (`FUN_10331fd0`, `0x331fd0`). Default is 0. No caller of `IgnoreRemotes`.
- `cObjEntity::ClientSpawn` (`0xabc90`) and `ClientCommand` (`0xb9480`) return without writing memory. `MOD_SvParseMessage` (GameDll7 `0x3140`, export block index 25, called from Main7 thunk `0x1b650`) handles `0x87` and `0xff` only.
- `cLocalDriver` rendezvous is the process globals at Engine RVA `0x698ab4` and `0x698ab8`. It cannot connect two processes.
