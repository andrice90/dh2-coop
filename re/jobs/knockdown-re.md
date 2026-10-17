# Job: knockdown-re — how an animal knocks the hunter down, and what a co-op "knock over" would call (Ghidra slot W; no game launches)

## Goal (stretch feature)
In the game some animals knock the player over: the camera drops to the ground as in death, then the player stands back up. The user wants co-op players to trigger that on each other by shooting or meleeing another player. Find the engine path so the lead can design it. This job is research only.

## Questions to answer
1. **The knockdown entry point on the player.** Which function(s) put the local hunter (`cScr_ModHunter`, GameDll7 vtable `+0x106ff0`; `cObjHumanEx`/`cObjCharacterEx` in CommonDll7) into the knocked-down state when an animal attack lands, and which state/clip names it uses (body states, camera mode, get-up). Look for: hunter body-state names in `SCRIPTS/MODELS/CHARACTERS/` and the hunter scripts (`HUMAN.SCR`, the teen/hunter body and legs scripts), natives such as `SetTimeBeforeGetUp` (GameDll `0x16bf0` per `re/notes/story-inventory.md` section 2), animal attack scripts and brains (`cAIHuntBrain_Animal`, attack tasks in CommonDll7), `OnHurt` paths (`cObjScriptedEntity::OnHurt` CommonDll `0x3e290`), strings like `knock`, `KnockDown`, `getup`, `GetUp`, `stand`, `fall`, `push`, `Tackle`, `Pounce`, `Charge`, `Hit`. Give RVAs, calling convention, arguments, and what each does to: hunter state, camera, input lock, health.
2. **Can we call it.** The smallest call that knocks the *local* hunter down and lets the engine stand them back up on its own, **without damage** (or with damage we can set to 0) and without ending the level. If it needs an attacker entity (for direction), say what it reads from it (position only? class?). If it is purely script-driven (a script state switch), say which native or `SetBodyState`/`HandleKey` call reproduces it from C++.
3. **Death vs knockdown.** How the engine tells them apart (health, flags such as hunter `+0x6CC` and `+0xC1` bit 2 that the F9 revive clears — see `PollClientRespawn` in `mod/src/coop/coop.cpp`). The knockdown must not set those, or must clear them itself.
4. **Player melee.** What the hunter's melee move is (key, action, body/weapon state names, e.g. a rifle-butt or knife swing), how it finds what it hit (trace, sphere, closest animal), and how our code can detect "the local player just did a melee swing" (a weapon state name like the `Fire`/`Reload` names `player_tracker.cpp` already reads via `cObjItemEx::GetCurrentState`, or a hunter state).
5. **Puppet clip.** Which clips in the puppet's pack (`objects/npc/mountainguide/mountainguide.skin,game.ap`; list with `py re\scripts\ap_list.py`) best show "knocked down, then get up" on another player's screen. `re/notes/anim-names.md` already lists `fall`, `liedead`, `deadfrontup`, `deadbackup`, `syncguidegetup*`. Say the order to play them in and which body-state names in `GUIDELEGS.SCR` drive them.

## Tools
- Ghidra: `.\re\scripts\decomp.ps1 -Slot W -Binary <dll> -Out re\notes\knock-<n>.c -Names ...`. Read the other `re\scripts\*.java` helpers (XrefStrings, CallersDecomp, DecompileAt, DataRefs) and `re\scripts\ghrun.ps1` before writing anything new. One Ghidra process at a time on slot W.
- PDB symbols: `tools-src\symbolize.ps1` (read its header).
- Script sources: `re/extracted/SCRIPTS/`.

## Deliverable
`re/notes/knockdown.md` only (raw dumps go to `re\notes\knock-*.c` / `*.txt`, gitignored). Sections: one per question above, then `## Proposed design inputs` (what the lead would call on the victim's machine, what the attacker's machine must detect, what the puppet plays), then `## Proposed MEMORY facts` with evidence. Mark anything not shown by code or data UNVERIFIED. Keep a `## Progress` line at the top. Do not quote game scripts beyond identifiers and single short lines.

## Rules
No game launches, no `mod/src` changes, no commits.
