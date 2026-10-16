# Animation names in NPC `.AP` packs

## Progress

Done. Format parsed, `re/scripts/ap_list.py` checked against the guide scripts, locomotion and weapon names recorded, NPC `GAME.AP` name lists compared. No game launch.

## Format

Little-endian. Checked with `ap_list.py` on the three packs in the job, plus the other NPC `GAME.AP` files, the mountainguide weapon packs, and `OBJECTS\ANIMALS\ASIATICBEAR\MAP6.AP`.

```
0x00  char[4]  "FBAF"
0x04  uint16   version 0x0411
0x06  uint32   per-file id (algorithm not identified)
0x0A  uint8    banner length (0x24)
0x0B  char[]   "Make Model (build 402 - Sep 30 2005)"
      uint8    0x00
      uint16   tag 0xEFF1 (bytes F1 EF)     geometry chunk
      uint32   size A    field offset + size = geometry end
      uint32   size B    field offset + size = the same end
      ...      geometry
      uint8[4] bridge (bytes differ per file; not interpreted)
      uint16   tag 0xEFE8 (bytes E8 EF)     animation chunk
      uint32   size       field offset + size = file size - 10
      uint8    repeat count
      uint16   clip count
      then clip records
```

Every file examined ends with the six bytes `E0 EF 00 00 00 00`.

A clip record is a length-prefixed class name `ModelAnimBone` (length byte `0x0D`) followed by a length-prefixed ASCII clip name. Two bytes in front of the class name are `FF FF` on most records. In `MOUNTAINGUIDE\GAME.AP` three names use another value on every copy: `find` is `0x012D`, `idlenormalneutral` and `syncguidegetup_foot` are `0x013A`. What those values mean is **UNVERIFIED**.

The header repeat count is how many times the whole name list is stored. The copies are the same names in the same order; the key payloads differ in size. `MOUNTAINGUIDE\GAME.AP` is 4 × 332 records (1328 `ModelAnimBone` hits). `TEENM\BODY\GAME.AP` is 2 × 44. `POACHER\GAME.AP` is 4 × 1 (`blink` only). `ASIATICBEAR\MAP6.AP` is 3 × 1. What the repeats are (LOD, submesh, or something else) is **UNVERIFIED**. Skeleton bone names such as `BipHead` live in the `.SKIN` (`FBSF`), not in this name field.

`MOUNTAINGUIDE\GAME.AP` anchors: file size 941837, anim tag `E8 EF` at offset 439710, repeat 4, clip count `0x014C` = 332. `OLDHUGH\GAME.AP` is 942125 bytes with the same 332 names. `TEENM\BODY\GAME.AP` is 133222 bytes, anim tag at offset 53410, repeat 2, clip count 44.

`ap_list.py` prints the first copy, one name per line. It rejects a file whose later copies disagree with the first.

## MOUNTAINGUIDE GAME.AP names

332 clips. First name is `defaultpose` (calling that a bind pose is inferred from the name; no script calls `SetAnim("defaultpose")`).

The block `GMAF*`, `GMAP*`, `GMDB*`, `GMHA*` is 268 of the 332 names (`GMAF` 81, `GMAP` 87, `GMDB` 6, `GMHA` 94). `GUIDE.SCR` lines 881–900 pass the unsuffixed ids (`"GMAF01"` … `"GMAF24"`) to `SetSoundShader`, not to `SetAnim`. Whether the `GMAF01a/b/c` clips are the matching performance is **UNVERIFIED**.

```
defaultpose
bumpride
cover
coveridle
uncover
hit
scare
hurtdown
hurtsit
hurtrise
GMAF01a
GMAF01b
GMAF01c
GMAF02a
GMAF02b
GMAF02c
GMAF03a
GMAF03b
GMAF03c
GMAF04a
GMAF04b
GMAF04c
GMAF05a
GMAF05b
GMAF05c
GMAF06a
GMAF06b
GMAF06c
GMAF07a
GMAF07b
GMAF07c
GMAF08a
GMAF08b
GMAF08c
GMAF09a
GMAF09b
GMAF09c
GMAF10a
GMAF10b
GMAF10c
GMAF11a
GMAF11b
GMAF11c
GMAF12a
GMAF12b
GMAF12c
GMAF12d
GMAF13a
GMAF13b
GMAF13c
GMAF14a
GMAF14b
GMAF14c
GMAF15a
GMAF15b
GMAF15c
GMAF16a
GMAF16b
GMAF16c
GMAF17a
GMAF17b
GMAF17c
GMAF18a
GMAF18b
GMAF18c
GMAF19a
GMAF19b
GMAF19c
GMAF20a
GMAF20b
GMAF20c
GMAF21a
GMAF21b
GMAF22a
GMAF22b
GMAF23a
GMAF23b
GMAF23c
GMAF24a
GMAF24b
GMAF24c
GMAF25a
GMAF25b
GMAF25c
GMAF25d
GMAF26a
GMAF26b
GMAF26c
GMAF27a
GMAF27b
GMAF27c
GMAP01a
GMAP01b
GMAP01c
GMAP01d
GMAP02a
GMAP02b
GMAP02c
GMAP03a
GMAP03b
GMAP03c
GMAP04a
GMAP04b
GMAP04c
GMAP05a
GMAP05b
GMAP05c
GMAP05d
GMAP06a
GMAP06b
GMAP06c
GMAP07a
GMAP07b
GMAP09a
GMAP09b
GMAP09c
GMAP09d
GMAP10a
GMAP10b
GMAP10c
GMAP11a
GMAP11b
GMAP11c
GMAP12a
GMAP12b
GMAP12c
GMAP13a
GMAP13b
GMAP13c
GMAP13d
GMAP14a
GMAP14b
GMAP14c
GMAP14d
GMAP15a
GMAP15b
GMAP15c
GMAP16a
GMAP16b
GMAP16c
GMAP17a
GMAP17b
GMAP17c
GMAP18a
GMAP18b
GMAP18c
GMAP18d
GMAP19a
GMAP19b
GMAP19c
GMAP19d
GMAP20a
GMAP20b
GMAP20c
GMAP21a
GMAP21b
GMAP21c
GMAP21d
GMAP21e
GMAP21f
GMAP22a
GMAP22b
GMAP22c
GMAP22d
GMAP22e
GMAP22f
GMAP23a
GMAP23b
GMAP23c
GMAP23d
GMAP23e
GMAP23f
GMAP24a
GMAP24b
GMAP24c
GMAP25a
GMAP25b
GMAP25c
GMDB01a
GMDB01b
GMDB01c
GMDB02a
GMDB02b
GMDB02c
GMHA01a
GMHA01b
GMHA01c
GMHA02a
GMHA02b
GMHA02c
GMHA02d
GMHA03a
GMHA03b
GMHA03c
GMHA03d
GMHA03f
GMHA04a
GMHA04b
GMHA04c
GMHA05a
GMHA05b
GMHA05c
GMHA06a
GMHA06b
GMHA06c
GMHA07a
GMHA07b
GMHA07c
GMHA08a
GMHA08b
GMHA08c
GMHA09a
GMHA09b
GMHA09c
GMHA10a
GMHA10b
GMHA10c
GMHA10d
GMHA10e
GMHA10f
GMHA11a
GMHA11b
GMHA11c
GMHA12a
GMHA12b
GMHA12c
GMHA13a
GMHA13b
GMHA13c
GMHA14a
GMHA14b
GMHA14c
GMHA15a
GMHA15b
GMHA15c
GMHA16a
GMHA16b
GMHA16c
GMHA17a
GMHA17b
GMHA17c
GMHA18a
GMHA18b
GMHA18c
GMHA19a
GMHA19b
GMHA19c
GMHA19d
GMHA19e
GMHA19f
GMHA20a
GMHA20b
GMHA20c
GMHA21a
GMHA21b
GMHA21c
GMHA21d
GMHA21e
GMHA21f
GMHA22a
GMHA22b
GMHA22c
GMHA23a
GMHA23b
GMHA23c
GMHA23d
GMHA23e
GMHA24a
GMHA24b
GMHA25a
GMHA25b
GMHA25c
GMHA26a
GMHA26b
GMHA26c
GMHA27a
GMHA27b
GMHA27c
w_call1
w_call2
w_call3
w_ouch1
w_ouch2
w_ouch3
w_scrm1
w_scrm2
w_scrm3
blink
walk/forward
walk/backward
walk/forwardlimp
walk/left
walk/right
run/forwardlimp
run/forward
walktoidle
runtoidle
idletowalk
idletorun
conversation
radiotalk
pickup
point
call
hurt
followme
followmeleft
followmeright
find
give
liedeadidle
liedead
deadfrontup
deadbackup
idle
idlenormalneutral
idlenormalgestures
turn180
turn45left
turn45right
turn90left
turn90right
turn135left
turn135right
fall
bats_defend
disarmmine
crouchidlecar
syncguidegetup
syncguidegetup_back
synctrap
syncguidegetup_foot
```

## Locomotion picks

Source of the state names: `SCRIPTS\MODELS\NPC\GUIDE\GUIDELEGS.SCR`. Presence in the pack: `ap_list.py` on `OBJECTS\NPC\MOUNTAINGUIDE\GAME.AP`.

| Wanted | Clip in `GAME.AP` | Why |
|---|---|---|
| Standing idle | `idlenormalneutral` | `SetEntity` line 14 and state `Idle` line 42. `idle` is also a real clip in the pack, and no `SetAnim("idle")` exists in `GUIDELEGS.SCR` or `GUIDE.SCR`. What `idle` looks like is **UNVERIFIED**. |
| Walk forward | `walk/forward` | state `WalkForward` line 90 |
| Walk backward | `walk/backward` | state `WalkBackward` line 122. State `RunBackward` line 237 uses this same clip. There is no `run/backward`. |
| Strafe left | `walk/left` | state `WalkLeft` line 150. State `RunLeft` line 265 uses this same clip. There is no `run/left`. |
| Strafe right | `walk/right` | state `WalkRight` line 178. State `RunRight` line 293 uses this same clip. There is no `run/right`. |
| Run forward | `run/forward` | state `RunForward` line 207 |
| Crouch | none | The only `crouch*` clip is `crouchidlecar`, played by state `TurnDrive` line 1246 (vehicle). `UpdateCrouchState` line 17 is empty. |
| Death, collapse | `fall` | state `Liedead` line 914, then it switches to `FinalLiedead` |
| Death, held pose | `liedead` | state `FinalLiedead` line 941. `liedeadidle` is also in the pack; that state does not call it. |
| Knockdown, front | `deadfrontup` | state `StayDown` line 858 |
| Knockdown, back | `deadbackup` | state `StayDown1` line 886 |

`GUIDELEGS.SCR` has 42 distinct `SetAnim` strings. 39 are in this pack. Missing from every `OBJECTS\**\*.AP`: `turn180left`, `turn180right` (script lines 606 and 579). The pack does contain `turn180`. Missing from this pack but present in `OBJECTS\NPC\HAMISI\GAME.AP` only: `mineblast` (script line 396).

## pick_anim mapping

`pick_anim` today only has a speed (`puppets.cpp`: under 0.6 m/s idle, under 5.5 m/s walk, else run). The hunter walks at about 5 units/s (`MEMORY.md`), which is under 5.5, so a normal walk stays on the walk clip. Run plays only above 5.5.

Use these directory strings. They match `GUIDELEGS.SCR` and `GAME.AP`:

| Speed | Clip |
|---|---|
| `< 0.6` | `idlenormalneutral` |
| `< 5.5` | `walk/forward` |
| `>= 5.5` | `run/forward` |

Replace the current `"idle"`. That string is a different clip in the same pack, not the standing state.

If a later change has velocity relative to the puppet facing, the NPC pack still has only one clip per side. Backward, left, and right stay on the walk clips even when the NPC script is in the run states (lines 237, 265, 293). There is no crouch clip to map.

For a death pose, play `fall` and then `liedead`. One shot with no follow-up: `liedead`.

Weapon clips are not in `GAME.AP`. The puppet model string is `objects/npc/mountainguide/mountainguide.skin,game.ap`, so a `SetAnim` of a `shotgun1/…` name has no pack to resolve against unless that pak is loaded too. Whether adding `,shotgun1.ap` to the model string is enough is **UNVERIFIED** (the hunter model string is `skin,game.ap,HUNTMAP4.ap` in `MEMORY.md`; that shape was not re-tested here).

## Weapon-holding variants

NPC weapon body scripts call the short name (`SGUNNPC.SCR` line 153 `SetAnim("walkforward")`, line 219 `SetAnim("run")`; `HGUNNPC.SCR` is the same pattern; `RIFLENPC.SCR` line 153 `walkforward`, line 219 `run`). The pak stores the prefixed directory name. `GUIDE.SCR` lines 16–39 register footsteps with the prefixed form (`shotgun1/run`, `shotgun1/walkforward`, `handgun3/walkforward`, …). Where the short script name gains the `shotgun1/` or `handgun3/` prefix is **UNVERIFIED** (it is not in the `SetAnim` call). The puppet passes a raw C string, and the unarmed leg script's raw strings already match `GAME.AP`, so a weapon clip has to be the prefixed directory name.

`OBJECTS\NPC\MOUNTAINGUIDE\SHOTGUN1.AP` (59 clips) and `HANDGUN3.AP` (55 clips). The same name lists are on the 17 NPCs in the shared group below. There is no non-alert `shotgun1/walkbackward`, `shotgun1/walkleft`, `shotgun1/walkright`, or `shotgun1/runbackward`.

| Wanted | Shotgun directory name | Handgun directory name |
|---|---|---|
| Standing | `shotgun1/idlenormalneutral` | `handgun3/idlenormalneutral` |
| Walk forward | `shotgun1/walkforward` | `handgun3/walkforward` |
| Walk back | `shotgun1/walkbackwardalert` | `handgun3/walkbackwardalert` |
| Strafe left | `shotgun1/walkstrafeleftalert` | `handgun3/walkstrafeleftalert` |
| Strafe right | `shotgun1/walkstraferightalert` | `handgun3/walkstraferightalert` |
| Run forward | `shotgun1/run` | `handgun3/run` |
| Crouch | none | none |
| Collapse / held death | `shotgun1/fall` then `shotgun1/liedead` | `handgun3/fall` then `handgun3/liedead` |

Rifle locomotion is not in the NPC packs. `MOUNTAINGUIDE\RIFLE1.AP` is three clips: `oldh_give3030`, `give3030`, `oldh_idle3030`. A byte search of `OBJECTS\**\*.AP` finds `rifle1/walkforward` nowhere. The only `rifle1/walk`, `rifle1/run`, and `rifle1/idle` clips are in `OBJECTS\CHARACTERS\TEENM\BODY\RIFLE1.AP` (15 clips: `rifle1/changein`, `changeout`, `fire`, `reload0`, `reload1`, `reload2`, `idle`, `run`, `walk`, `cs_idle`, `defend0`, `defend1`, `defend2`, `hurt`, `kick`). Those short names match `SCRIPTS\MODELS\ITEMS\WEAPONS\WEAPON.SCR` (the player weapon machine: `SetAnim("walk")`, `SetAnim("run")`, `SetAnim("idle")`), not the NPC rifle machine. `FIANCE\RIFLE1.AP` contains only `rifle1/give3030`.

`BLOWDARTS.AP` is `cs01`, `cs05`. `MAP1.AP` is `bb_syncattack1`, `ml_syncattack1`. Map paks are not a second copy of the locomotion set.

## Teen body `GAME.AP`

`OBJECTS\CHARACTERS\TEENM\BODY\GAME.AP` is the first-person arms pack. The skin string in `TEENMM.SKIN` points at `teenmm1st.skin` and the mesh name `fpshands1`. It is not a puppet body. 44 names, repeat count 2:

`float`, `camera_shake`, `camera_blur`, `camera_idle`, `bumpride`, `scare`, `walk/forward`, `walk/backward`, `walk/left`, `walk/right`, `dodgeleft`, `dodgeright`, `walk/forward_limp`, `walk/backward_limp`, `walk/left_limp`, `walk/right_limp`, `crouchwalk/forward`, `crouchwalk/backward`, `crouchwalk/left`, `crouchwalk/right`, `crouchrun/forward`, `crouchrun/backward`, `crouchrun/left`, `crouchrun/right`, `crouchidle`, `TRUCK/crouchidlecar`, `standcrouch`, `run/forward`, `run/backward`, `run/left`, `run/right`, `run/forward_limp`, `run/backward_limp`, `run/left_limp`, `run/right_limp`, `sprint`, `trap/trap_close`, `fallback`, `cs_idle`, `heli_idle`, `death`, `shake`, `shake2`, `synchuntergetup`.

This is the pack that actually has crouch locomotion and per-direction run clips. Those names are not in the NPC `GAME.AP`.

## Shared packs

`[Puppets] Model=` loads `skin,game.ap` for one NPC. The speed mapping above is safe on any `GAME.AP` that contains `idlenormalneutral`, `walk/forward`, and `run/forward`.

Same 332 names, same order, as mountainguide (files are not byte-identical; `ABBY\GAME.AP` is 901154 bytes, mountainguide is 941837, oldhugh is 942125):

`ABBY`, `ABO1`, `ABO2`, `ABO3`, `AUSRANGER1`, `AUSRANGER2`, `BENEDECK`, `MOUNTAINGUIDE`, `OLDHUGH`, `REGINALD`, `REGINALD_W`, `SIBERIAN1`, `SIBERIAN2`, `SURVIVOR`, `TRIBALLEADER`, `WIRAKE`, `WIRAKE_W`.

Their `SHOTGUN1.AP` (59 names) and `HANDGUN3.AP` (55 names) match mountainguide too.

Also safe for the three speed clips, different full list:

| NPC | `GAME.AP` clips | Gap |
|---|---|---|
| `HAMISI` | 67 | Has the directional walk clips and `liedead`. Extra: `hurt_tree`, `mineblast`, `oldhughbear`, `tiedtotree`. Missing the 268 `GM*` clips and `syncguidegetup_foot`. Shotgun pak is the shared 59 minus `shotgun1/syncguidegetup_foot`. |
| `AUSRANGER3` | 25 | Has `idlenormalneutral`, `walk/forward`, `run/forward`, `liedead`. No `walk/backward`, `walk/left`, or `walk/right`. |

Not safe for `idlenormalneutral` (these have `idle`, `walk/forward`, `run/forward` instead, except where noted):

| NPC | Clips | What is there |
|---|---|---|
| `INDIANGIRL` | 16 | `idle`, `walk/forward`, `run/forward`, turns, `crying`, `jump`, `tiedtotree`. No back, strafe, or `liedead`. |
| `NEPHEW` | 15 | Same shape as indiangirl without `tiedtotree`. `TEENGIRL` is this same 15-name list. |
| `FIANCE` | 13 | `defaultpose`, cover, hit, scare, blink, four sync-attack clips. No locomotion. |
| `POACHER` | 1 | `blink` only. `POACHER\SHOTGUN1.AP` is the shared shotgun list plus `cs03`. `MAP5_POACHER.AP` is `cs03`. |

## Script cross-check

`py re\scripts\ap_list.py` on `MOUNTAINGUIDE\GAME.AP` prints every `GUIDELEGS.SCR` `SetAnim` argument except `mineblast`, `turn180left`, and `turn180right`.

`GUIDE.SCR` `AddStep` weapon names (`shotgun1/…`, `handgun3/…`) are absent from `GAME.AP` and present in `SHOTGUN1.AP` and `HANDGUN3.AP` respectively. `GUIDE.SCR` itself has no `SetAnim` calls.

## Proposed MEMORY facts

- `.AP` animation packs are `FBAF`, version uint16 `0x0411`, banner `Make Model (build 402 - Sep 30 2005)`. Clip names are the length-prefixed ASCII string on each `ModelAnimBone` record in the `E8 EF` chunk. The chunk's uint8 repeat count stores that name list more than once; the copies match. Evidence: `re/scripts/ap_list.py` on `OBJECTS\NPC\MOUNTAINGUIDE\GAME.AP` (offset 439710, repeat 4, 332 names), `OBJECTS\NPC\OLDHUGH\GAME.AP` (same 332 names, file size 942125), `OBJECTS\CHARACTERS\TEENM\BODY\GAME.AP` (offset 53410, repeat 2, 44 names), `OBJECTS\NPC\POACHER\GAME.AP` (repeat 4, one name `blink`).
- Guide standing idle is the clip `idlenormalneutral`. `walk/forward`, `walk/backward`, `walk/left`, `walk/right`, and `run/forward` are the locomotion clips. Run-back and run-strafe reuse the walk clips. There is no crouch locomotion clip in mountainguide `GAME.AP`; `crouchidlecar` is the drive pose. Death collapse is `fall`, held death is `liedead`. Evidence: `SCRIPTS\MODELS\NPC\GUIDE\GUIDELEGS.SCR` lines 14, 42, 90, 122, 150, 178, 207, 237, 265, 293, 858, 886, 914, 941, 1246; those names are in the `ap_list.py` output for `MOUNTAINGUIDE\GAME.AP`. `idle` is a separate clip in that output and is not a `SetAnim` argument in `GUIDELEGS.SCR` or `GUIDE.SCR`.
- These NPC `GAME.AP` files carry that same 332-name list (not the same bytes): `ABBY`, `ABO1`, `ABO2`, `ABO3`, `AUSRANGER1`, `AUSRANGER2`, `BENEDECK`, `MOUNTAINGUIDE`, `OLDHUGH`, `REGINALD`, `REGINALD_W`, `SIBERIAN1`, `SIBERIAN2`, `SURVIVOR`, `TRIBALLEADER`, `WIRAKE`, `WIRAKE_W`. Evidence: `ap_list.py` name tuples compare equal; sizes differ (`ABBY` 901154, `MOUNTAINGUIDE` 941837, `OLDHUGH` 942125). `HAMISI\GAME.AP` (67 names) and `AUSRANGER3\GAME.AP` (25 names) also contain `idlenormalneutral`, `walk/forward`, and `run/forward`. `POACHER\GAME.AP` contains only `blink`.
- Shotgun and handgun locomotion clips are prefixed directory names in `SHOTGUN1.AP` and `HANDGUN3.AP` (`shotgun1/idlenormalneutral`, `shotgun1/walkforward`, `shotgun1/run`, `handgun3/…`), not in `GAME.AP`. NPC `RIFLE1.AP` has no walk/run set. Evidence: `ap_list.py` on `OBJECTS\NPC\MOUNTAINGUIDE\SHOTGUN1.AP` (59 names), `HANDGUN3.AP` (55 names), `RIFLE1.AP` (`oldh_give3030`, `give3030`, `oldh_idle3030`); byte search of `OBJECTS\**\*.AP` finds `rifle1/walk` only in `OBJECTS\CHARACTERS\TEENM\BODY\RIFLE1.AP`.
