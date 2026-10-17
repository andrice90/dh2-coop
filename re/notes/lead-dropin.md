# Drop-in / drop-out and cutscene skip (lead, 2026-10-17)

## Problem (before this change)
- On every `PlayerJoined` the host sent its whole journal to **every** client (`events::Replay`). With 3+ players the clients already in the game got it again:
  - `kMapStart` made them `spmap` the map they were on, which is the same-map reload that crashes at the next teardown.
  - `kPlaySA` replayed cutscene meshes.
  - `kMissionEnd` would show the results again.
- Live commits went to every client, including one still loading the map. That client logged `no such trigger here` and lost the event (STATUS next step 8).
- A late joiner replayed every old trigger fire. Each fire with a cutscene put its camera on a rail, and later fires were deferred behind it, so the joiner watched every past cutscene in a row.
- The journal held 64 commits. MAP2A passes that before the bridge.

## Design (events.cpp, story.cpp, coop.cpp)
1. **Client asks.** Once the client's map is loaded and its hunter exists, it sends `kSyncRequest` (9), with its map name and map serial. It re-asks every 10 s until answered, and again after every map load.
2. **Host answers that client only** (`Session::SendGameEventTo`):
   - Always its `kMapStart` first.
   - If the client is on another map, nothing else. The client follows with `spmap` and asks again when that map is up.
   - Same map: every journal commit in order, then `kSyncDone` (10) with the count. Each replayed `kTriggerFire` carries its age in seconds in `f0`.
3. **Live commits go to synced clients only.** `g_synced` is the set of clients that got `kSyncDone` for the host's current map. It is cleared when the host loads a map. `kMapStart` still goes to every client.
4. **`kMapStart` on the client** is keyed by the host's map serial:
   - A repeat of the serial already seen is ignored. That repeat is the start of each sync reply.
   - A first contact on the same map does not reload.
   - A new serial for the current map is a host replay and reloads (still crash-prone, as before).
   - A second announcement of a map the client is already loading (same serial, within 30 s) does not start a second `spmap`.
5. **Catch-up on the client** (`events::CatchingUp()`, from the request until the replay settles):
   - Replayed fires older than 3 s watch their trigger for a cutscene to skip. They are watched before the cutscene deferral, so a map-start cutscene that played before the join is skipped too.
   - Fires that are still fresh play their cutscene normally.
   - When the replay is applied, the camera is back on the hunter and no fires are deferred (at least 1 s after `kSyncDone`, at most 15 s), the client is moved 1.5 m beside the host if it is more than 20 m away. It is checked once more 4 s later, because a skipped cutscene can still place the hunter.
6. **Leave.** `PlayerLeft` removes the puppet and the player's `g_synced` entry. A client that rejoins asks again and gets the full replay.
7. Journal cap 1024.

## Cutscene skip (GameDll7 v1.0, lead decompile `re/notes/lead-skip.c`, `lead-cut*.c`)
- `0x6b350` `__fastcall(cObjTriggerEvent*)`, the trigger-level skip. `0x6b600` is its input callback (`(ent, pressed)`; action `skip_cutscene`).
  - If `+0x1f0 == 0`: sets it to 1, hides the skip HUD element, then calls `0x6a940` with the trigger's script object (`cObjScriptedEntity::GetScrObject`, CommonDll export).
- `0x6a940` `__fastcall(cScrTriggerEvent*)` = `cScrTriggerEvent::SkipCutScene` (profiler label string at `0x10b6e8`).
  - When `+0x30 != -1`, it runs `cClassManager::GetObjectA(g_pClassManager, +0x30)`, then record `+8`, then `0x8ed10`.
  - With a global flag set (`DAT_101657f0`), it also clears player `+0x6c8` and calls `0x465e0`.
- `0x8ed10` `__fastcall(cScrCutScene*)` = `cScrCutScene::Skip`:
  - sets `m_bSkip` (`+0x4c`; export table `0x8ee40` names it);
  - calls vt `+0x60`, then `StopSoundEffect` (`0x8e7d0`);
  - sets the script state to `"Skip"`.
  - The script's `Skip` state then runs `EndCamera`, `StopAllTask` on its actors, and `DeactivateCutSceneMode`. The `Activate` state's `End` returns early on `m_bSkip`.
- `cScrCutScene` is `0x54` bytes, vtable `0x110998` (constructor `0x8e260`, create `0x8ea70`). We skip only when the executed object's vtable is that one: a trigger can also execute non-cutscene scripts.
- `kCutsceneSkip` (11): the host hooks `0x6b350`. A skip that flips `+0x1f0` from 0 is committed, and clients watch that trigger and skip its cutscene when it is running. A client's own skip stays local.

## Tests
- 4-player and drop-in launches: `run-coop-local.ps1 -Players N`, `-LateJoin`, `-AddClient` (worker job `coop4-launcher`).
- Log lines:
  - host: `events: sync player N on 'MAP': replayed a of b commits`;
  - client: `events: synced with host`, `story: will skip the cutscene of trigger`, `story: skipped cutscene of`, `events: catch-up finished after … ms; host … m away, moving to the host`, `coop: teleport to host`.

## Open
- The `+0x1f0` reset rule is unknown: whether `ActivateHUD` clears it so a trigger can be skipped twice.
- Same-map host replay still reloads clients and still crashes at the next teardown.
