# Job: P3 — animation-name inventory for puppet bodies (mechanical; no game launch, no Ghidra)

Puppets (remote players) use an NPC body, `objects/npc/mountainguide/mountainguide.skin,game.ap`. They play animations through `cObjScriptedEntity::SetAnim(name)` (`mod/src/coop/puppets.cpp`, `pick_anim`): currently `"idle"`, `"walk/forward"` and `"run/forward"`. `"idle"` is a guess. Script files show names like `walk/forward`, `run/forward`, `walktoidle`, `idletorun`, `shotgun1/run`, `handgun3/walkforward`, ... (`re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR`).

## Tasks
1. Reverse the `.AP` animation-pack format enough to list the animation names it contains. The magic is `FBAF`. Inputs:
   - `re/extracted/OBJECTS/NPC/MOUNTAINGUIDE/GAME.AP`
   - `re/extracted/OBJECTS/NPC/OLDHUGH/GAME.AP`
   - `re/extracted/OBJECTS/CHARACTERS/TEENM/BODY/GAME.AP`

   Animation names are probably plain ASCII strings with `/` separators; start by dumping strings. Write `re/scripts/ap_list.py` (`py re\scripts\ap_list.py <file.AP>` prints one name per line), and confirm it against names the scripts use.
2. Produce `re/notes/anim-names.md`:
   - the full name list for MOUNTAINGUIDE `GAME.AP`
   - the best names for **idle** (standing), **walk forward**, **walk backward**, **strafe left/right**, **run forward**, **crouch** and **death**, plus weapon-holding variants (rifle/shotgun/handgun) if present
   - a recommended mapping for `pick_anim` (speed thresholds 0.6 m/s and 5.5 m/s; the hunter walks at about 5 m/s)
   - which other NPC packs share the same names, so any NPC body can be swapped in via `[Puppets] Model=`
3. Do **not** edit `mod/`. The lead applies the mapping.

## Deliverables
`re/scripts/ap_list.py`, `re/notes/anim-names.md` (with `## Proposed MEMORY facts`).
