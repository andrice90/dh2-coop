# Job: animal-park-re — why a story animal vanishes (parked at -10000) when it chases a client far from the host (Ghidra slot X; no game launches)

## Problem
Two-PC test and local repro (`re/notes/session-repro.md` task 1, MAP2A): the mountain lion (entity 157) switched to the client (the host retargets animals to the nearest player, `mod/src/coop/combat.cpp` `RetargetToNearestPlayer`), stalked it, reached `IdleAttack` near the client, and then on the host its position became `(-10000,-10000,-10000)` (the mod calls that "parked"; `animals.cpp` treats `y < -1000` as parked) while its state name stayed (`LookL45` / `KeepAnim`). The host hunter was ~100-120 m away at that moment. In the user's session the client crashed right after (its mirror was mid-attack).

## Questions
1. What moves an animal to `(-10000,-10000,-10000)`: function(s), RVA, class (CommonDll7 `cAIHuntAnimalInterface`, `cAIHuntBrain_Animal`, an animal manager, a script native such as `Hide`/`Park`/`Despawn`/`SetActive`). Search for the constant (`-10000.0f` = `0xC61C4000`) in CommonDll7 and GameDll7, and xrefs.
2. The condition: distance to the player (which player getter: `FUN_10006ae0`/`FUN_10006c30` in GameDll, `GetPlayer`, the attack-list entry 0, `"hunter"` by name), lost target, flee point reached (`SetMissionOnFleePointReachedAction`), LOD/activity radius, or the animal leaving the player's view. Thresholds (metres) and where they come from.
3. What un-parks an animal (comes back near the player? script `MoveToPos`?).
4. The smallest change that makes the condition count every player (host hunter and the puppets of remote players, entity `+0xE8` positions; the mod has `puppets::NearestLivingPuppet(pos, best_sq)`), in the style of `mod/src/coop/npc_follow.cpp` (a mid hook that replaces one player lookup result) or a hook on the distance check. What else uses the same player lookup there.

## Deliverable
`re/notes/animal-park.md` (raw dumps `re\notes\park-*.c|txt`). One section per question, `## Proposed change` (exact hook site, bytes to verify, what to substitute), `## Proposed MEMORY facts` with evidence. Mark guesses UNVERIFIED. Keep a `## Progress` line at the top.

## Rules
No game launches, no `mod/src` changes, no commits. Ghidra only with `-Slot X` (`re\scripts\decomp.ps1`, `ghrun.ps1`; helpers in `re\scripts\`, including `FuncBefore.java`, `DataRefs.java`, `CallersDecomp.java`, `XrefStrings.java`).
