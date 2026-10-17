# Job: props-research — shootable world props (bottles, cans, stumps) in co-op (research only; Ghidra slot W; no game launches)

## Problem (two-PC user test 2026-10-17)
"Shootable environment items don't sync: MAP1 glass bottles to shoot, only the host can shoot them ... bottles and the stumps." MAP1 task 3 is "shoot 4 cans and have ammo" (`TARGET_CAN`, `NEED_AMMO_FOR_COMPLETE`; `re/notes/story-objectives.md`). In co-op the host runs the mission; a client's shot at a bottle only happens on the client's copy.

## Questions
1. What these props are: script classes and C++ classes (`cScrTrap`? a `TARGET_CAN` task object? `cObjCustomCharacterEx`?), where they are placed (`re/extracted/SCRIPTS/LEVELS/MAP1/`, `TRIGGERS/TRIGGERMISSION01.SCR`, `MISSIONS/MISSION01.SCR`), and the same for "stumps" (search the scripts for stump/log/target props on the story maps).
2. How a hit on one is detected and counted: which C++ entry (`cObjScriptedEntity::OnHurt` at CommonDll `0x3e290`, a class override, a trigger-hit `cObjTriggerHit::OnHurt`, or a raycast in the weapon code), what the script callback does (break animation/`SetAnim`, `Hide`, task progress `UpdateMissionStatus`/`CompleteTask`), and whether the entity ids match across instances (BSP-placed: the mod's `EntityById` works for them).
3. What a client's hit does today under the co-op model (`re/notes/lead-story-coop.md`, `mod/src/coop/story.cpp`, `events.h`): breaks locally only? counts locally? swallowed?
4. **Proposed design** for: a client's hit on a prop goes to the host as a request (like `AnimalHit` / `kTriggerFire`), the host applies the real hit (the script counts it), and the break is replayed on every client (prop anim `kPropAnim` already exists for script `SetAnim` on props — check whether it already covers the break, and whether the host's own breaks reach clients today). Name the function the host would call, its RVA/convention and arguments, and what must not be replayed twice.

## Deliverable
`re/notes/props-research.md` only (raw dumps `re\notes\props-*.c|txt`). One section per question, `## Proposed design`, `## Proposed MEMORY facts` with evidence. Mark guesses UNVERIFIED. Keep a `## Progress` line at the top. Do not quote game scripts beyond identifiers and single short lines.

## Rules
No game launches, no `mod/src` changes, no commits. Ghidra only with `-Slot W` (`re\scripts\decomp.ps1`, `ghrun.ps1`; helpers in `re\scripts\` including `FuncBefore.java`).
