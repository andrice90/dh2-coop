# Job: npc-health-re — where a story NPC's health lives (the "keep him alive" bar) and what damages it on a client (Ghidra slot V; no game launches)

## Problem (user, 2026-10-17)
MAP2A Uncle Hugh (`hugs2a`, script `cScrHugs2a`, brain `cScrBrain_NPC`, GameDll class `cObjNPC`) has a health bar and an objective to keep him alive (`AddAIToBeProtected`). In co-op his health was different on host and client, and it dropped on both, at different rates.
Co-op model: the host runs every NPC; clients mirror the NPC (`mod/src/coop/animals.cpp`, `kNpc` mirrors: position, yaw, body-state name, and `+0x19C` health copied from the host every frame, brain frozen with `cObjCharacterEx::CharacterBrainFreeze`). Host log evidence: the host's `+0x19C` for Hugh read 50 before each of many network "hits" (the mod applied `FuncInflictDamage`, 50 → 0) and was back at 50 a frame later: something rewrites `+0x19C` from another field.

## Questions
1. **The real health.** Which field holds an NPC's health (cObjNPC / cObjCharacter / its profile-like object), how it relates to `+0x19C`, and which function copies one into the other each frame. RVAs, offsets, class.
2. **The bar.** Which value the protect-objective health bar on the HUD shows (find the HUD code for protected AIs: `AddAIToBeProtected` native, mission `AIToBeProtected` list, the HUD element that draws it). RVA of the read.
3. **What damages it.** Every path that lowers it during MAP2A: animal attacks (`cObjScriptedEntity::OnHurt` CommonDll `0x3e290`, `cObjCharacter::OnHurt` `0xb0f0`, the attack task's timed damage `FUN_100c2ee0`), script natives (`FuncInflictDamage`, `SetHealth`-style, bleeding/regeneration), and anything that runs on a client whose NPC and animals are frozen mirrors (client scripts still run). Read `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/` and the MAP2A trigger script for health-related calls.
4. **What makes him "dead"** for the mission (the fail path) and whether it reads the same field.

## Deliverable
`re/notes/npc-health.md` (raw dumps `re\notes\npch-*.c|txt`). One section per question, then `## Proposed fix` (on the host: what to read for the snapshot's health; on a client: what to write every frame so the bar and the death check match the host, and which local damage path to block), `## Proposed MEMORY facts` with evidence. Mark guesses UNVERIFIED. Keep a `## Progress` line at the top. Do not quote game scripts beyond identifiers and single short lines.

## Rules
No game launches (another worker owns them), no `mod/src` changes, no commits. Ghidra only with `-Slot V` (`re\scripts\decomp.ps1`, `ghrun.ps1`, helpers in `re\scripts\` including `FuncBefore.java`, `DataRefs.java`, `CallersDecomp.java`, `XrefStrings.java`).
