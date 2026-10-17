# Job: snake-re — what it takes to mirror snakes (cobras) like the other animals (Ghidra slot W; no game launches)

## Goal (user decision 2026-10-17)
Snakes must be shared: one snake for everyone, killed by anyone, counting for everyone (MAP7 task 6 is "kill 1 cobra"; MAP3B has 3 cobras, MAP3C 4; `re/notes/story-objectives.md`). Today every process has its own snake: the animal mirror (`mod/src/coop/animals.cpp`) only takes entities whose vtable is `cScrAnimal` (GameDll7 `+0xff470`; `player_tracker.cpp` builds `Animals()` from that), and snakes are `cObjSnake` / `cObjCobra` (script `AddAnimal(ANIMAL_COBRA, ...)`, task `Task_CobraAttack` with target `"hunter"`).

## Questions
1. Snake classes: C++ class names, vtable RVAs (GameDll7 / CommonDll7), base classes. Are they `cObjCharacterEx` / `cObjAnimal` descendants (do the mirror's calls apply: `FreezeBrain` on the brain at entity `+0x200`, health at `+0x19C`, body-state names, `PresentDeath` vt `+0x154`, `cObjAnimal::ClaimCallback`)? List what differs.
2. Placement: BSP entities (same entity id on every process, like the other animals) or spawned at runtime by a script (`AddAnimal`): if spawned, in what order and from which trigger, and would ids still match on every process?
3. Brain and attack: brain class, how `Task_CobraAttack` resolves `"hunter"` (`cAIHuntTask_AttackBase::SetParam` `0xbc3d0`? attack list membership `DAT_101aea7c`?), how the bite damages the hunter (vt `+0x14C` hurt?), and whether a bite is a sync attack (`PlaySA`).
4. Death and kill count: how a snake dies (bullet `OnHurt` path, health ≤ 0), what counts it for `SetTaskAnimalToShoot(ANIMAL_COBRA, ...)`, and whether the existing kill/death commit (`kAnimalDied`) and client `AnimalHit` would cover it once snakes are in `Animals()`.
5. Body-state names the snake uses (from its body script), so a client mirror can apply the host's state.

## Deliverable
`re/notes/snake.md` (raw dumps `re\notes\snake-*.c|txt`). One section per question, `## Proposed change` (the exact vtable list / class test for the tracker, and anything the mirror must do differently for snakes), `## Proposed MEMORY facts` with evidence. Mark guesses UNVERIFIED. Keep a `## Progress` line at the top. No game-script quotes beyond identifiers.

## Rules
No game launches, no `mod/src` changes, no commits. Ghidra only with `-Slot W`.
