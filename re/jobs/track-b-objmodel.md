# Job: Track B — object model (Ghidra slot B)

A previous agent was cut off before writing its summary. Its raw work: `re\notes\objmodel-decomp-*.c` (engine entities, common entities, common AI, game player, game animal, game exports, Main7). Read them with targeted searches (they are large: up to 680 KB), then fill gaps with `re\scripts\decomp.ps1 -Slot B`, `probe.ps1 -Slot B` or `ghrun.ps1 -Slot B`. The demangled export lists are in `re\exports\*.demangled.txt`, and the class catalog is in `re\notes\class-catalog.md`.

Goals (give evidence for each):
1. Entity class hierarchy: vtables (`??_7Class@@6B@` exports), constructor chains and class sizes (malloc size before each ctor). Cover cObjEntity, cObjBaseEntity, cObjScriptedEntity, cScrEntity, cObjHuntAreaAIEntity, the player/hunter class, and the animal classes.
2. Field layouts as C++ struct drafts with offsets: transform (position, orientation), velocity, health/energy, brain pointer, model/anim pointer, entity id, flags, team, and the owner/target pointer.
3. Managers and access paths from a hook: cObjManager, cObjWorld, entity list iteration, get local player, and get camera. Include exported statics and singletons.
4. A function table with RVAs and calling conventions for:
   - spawn/create by class or template name: cObjEntity::Spawn, ClientSpawn
   - destroy
   - damage: cScrEntity::Hurt(int,int,int)
   - death: cAIHuntBrain_Animal::Die(int)
   - weapon fire and hit test/raycast
   - per-entity AI tick: cAIHuntBrain_Animal, cAIHuntBrainEx Update
   - animal target selection: how the animal picks the player (cAIHuntSenseEx, cAISense_See, cAIHuntAnimalInterface)
   - per-entity Update(float)

   **The key question:** can one animal's AI tick be skipped without breaking its animation or physics?
5. A puppet strategy: the best way to spawn a second human model driven by our code, using ClientSpawn, scripted entities, or the NPC companion classes (cAIBrain_Human).
6. Time scale: cAIHuntAnimalInterface::SetTimeMultiplier and the slow-motion link. Slow-mo will be disabled in co-op.

Deliverables: `re\notes\object-model.md`, with sections Hierarchy, Struct drafts, Managers, Function table, Hook candidates ranked for co-op (puppets, AI suppression, damage routing, target selection), Open questions, and Proposed MEMORY facts. Also `mod\src\game\types_draft.h`: C++20 structs with padding to the known offsets. Add comments marking each field VERIFIED or UNVERIFIED with evidence, and `static_assert(offsetof(...))` only for verified fields. It must compile standalone in 32-bit MSVC, so include only `<cstdint>` and `<cstddef>`.
