# Job: npc-follow-re — where a guide/companion NPC keeps "the player", so the host can point it at the nearest player (Ghidra slot W; no game launches)

## Goal
In co-op the host runs every NPC brain. Story scripts make NPCs follow or guide `"hunter"`, which on the host is only the host's hunter. MAP3A Abby guides (`SetAIType(AITYPE_GUIDE|AITYPE_COVERSTAY, ...)`), Reggie on MAP9/MAP10 switches between guide and companion, and MAP8/MAP9/MAP10/MAP12A companions follow `"hunter"` (`re/notes/story-objectives.md` sections 2 and 4). The lead wants the host to make those NPCs treat the **nearest player** as the player: either a remote player's puppet entity (an `obj_humanEx` on the host, position = that player) or the host hunter.

## Questions
1. **`SetAIType`.** The native that scripts call (`cScrNPC`/`cScrCharacterEx` method; find it in the native tables listed in `re/notes/story-inventory.md` section 2). What it stores for the second argument (`"hunter"`, `"a"`, empty): an entity pointer, an entity id, a name string resolved later, or a script object index. Offsets on the brain (`cScrBrain_NPC` / C++ brain class) or on the character.
2. **Follow / guide tasks.** `AITaskNPC_Follow` and the guide behaviour (`AITYPE_GUIDE`: lead, wait until the player is near, `SetFollowDistance`, `SetHelloDistance`). Where each reads "the player" every frame: the stored target from question 1, a global "local player" getter (e.g. a `cObjHunter`/`IsPlayer` lookup, `GetPlayer`, `FindEntScrObject("hunter")`), or the `cAIHuntAnimalInterface` attack list (`re/notes/lead-combat.md`). Give RVAs, offsets, and the call that resolves it.
3. **Retarget hook point.** The smallest place where the host could substitute another entity:
   - (a) overwrite the stored target field each frame;
   - (b) hook a resolver function and return the nearest player;
   - (c) hook the distance check.
   For each option, the risks: what else reads that field or calls that resolver (animals? the HUD? mission scripts?), and whether a puppet (`cObjHumanEx`, no brain, no `cScr_ModHunter` vtable, optional physics body) would satisfy the type checks on that path (`IsEntityKindOf`, `IsPlayer` bit 3 of `+0x3B4`, vtable compares).
4. **Script-side "hunter" lookups that decide progress.** `GetDistBetween2Ais(name, name)` (GameDll `0x9200`, used on MAP1, MAP2C, MAP4B, MAP12A) and any `FindEntScrObject("hunter")` the NPC natives use internally. Confirm the native's resolution path (name → entity), and whether one hook there covers every script name lookup of `"hunter"`.
5. **Animals' "hunter" task targets.** `SetTaskParam("target", "hunter")` on animal tasks (MAP5 elephants, cobras, chase tasks): where the task resolves that name, and whether the brain re-targets to `GetClosestAttackTarget` afterwards anyway (`lead-combat.md` P5d says "verify").

## Tools
- Ghidra: `.\re\scripts\decomp.ps1 -Slot W ...`, `.\re\scripts\ghrun.ps1 -Slot W ...` with the helpers in `re\scripts\` (XrefStrings, CallersDecomp, DecompileAt, DataRefs, DumpPtrs, FuncBefore — `FuncBefore.java` finds the parent of a `Catch@` funclet, which is how profiler label strings like `cScrX::Method` lead to the method). One Ghidra process at a time on slot W.
- Scripts: `re/extracted/SCRIPTS/` (NPC scripts under `MODELS/NPC/`, `GUIDE.SCR`, the mission trigger files).
- Existing notes: `re/notes/knockdown.md`, `re/notes/lead-combat.md`, `re/notes/story-objectives.md`, `re/notes/lead-player-tracker.md`.

## Deliverable
`re/notes/npc-follow.md` only (raw dumps `re\notes\npcf-*.c|txt`, gitignored). One section per question, then `## Proposed design` (which hook, what the host writes or returns, how to pick the nearest player, what must stay the host hunter), then `## Proposed MEMORY facts` with evidence. Mark anything not shown by code or data UNVERIFIED. Keep a `## Progress` line at the top and fill sections as you go. Do not quote game scripts beyond identifiers and single short lines.

## Rules
No game launches, no `mod/src` changes, no commits.
