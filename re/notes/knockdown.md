# Knockdown — animal hit that drops the hunter, and a co-op "knock over"

**## Progress:** done. Questions 1–5, design inputs, and proposed facts are filled. No game launch. Runtime look of the camera is UNVERIFIED.

Research only. RVAs are module-relative, v1.0 build 434. Raw dumps: `re/notes/knock-1.c` through `knock-10.c`, `knock-xref-common.txt`, `knock-xref-game.txt` (gitignored).

## 1. The knockdown entry point on the player

The drop-and-get-up is a **sync attack** (`PlaySA`), not the death path and not `SetTimeBeforeGetUp`.

An animal attack task decides a clip, then calls **both** the animal and the hunter:

`cAIHuntTask_AttackBase::TryPlaySA` — CommonDll `FUN_100cadf0` RVA `0xcadf0`, `__fastcall(task)`. Evidence: catch-block string `cAIHuntTask_AttackBase::TryPlaySA`. Cats go through `FUN_100cba60` RVA `0xcba60`, which calls the same function.

What it requires before it will start:

- Brain target id at `brain+0x254` (`SetTarget`). Cleared when the target is null or `entity+0xC1` bit 2 is set.
- Neither character has `+0x368 == 1` (already inside an SA).
- Hunter virtual `+0x218` returns non-zero (`LAB_100485e0`, see below).
- Animal virtual `+0x208(kind, hunter)` returns a clip name. `kind` is the string `attack`, or `truck` when the hunter is in a vehicle. That virtual is `cObjCharacterEx::ChooseSA` (CommonDll `0x27ce0`): it walks `AddSA` entries and keeps a name only when `IsSAOk` (`0x1a650`) finds the kind as a substring and `RelativePosRotOkForSA` accepts distance and facing. The body of `RelativePosRotOkForSA` was not decompiled.
- The task's own `saname` list contains that exact clip (`pl_*_syncattack*`, registered from the animal script with `SetTaskParam("saname", ...)`).
- A placement test `FUN_100c3170` returns non-zero. Body not decompiled. It fills the rotation of the sync matrix.

Then it writes the **target's** position (`+0xE8`) into the matrix translation (`+0x30`) and calls virtual `+0x210(clip, matrix)` on the hunter and on the animal. It stores the hunter at `task+0x98` and sets `task+0x18 = 0x2000`.

**Hunter entry.** `cScr_ModHunter` vtable GameDll `+0x106ff0` slot `+0x210` is `cObjHunter::PlaySA`, `FUN_10050e20` RVA `0x50e20`:

```
void __thiscall PlaySA(void* hunter, char* clip, m4x4* basis);
```

It does, in order:

1. Virtual `+0x220`, which is a jmp to imported `cObjHumanEx::ResetTorso`. Not `BeginCamera`.
2. Looks up an object from model part 0 and sets that object's `+0x14c = 1`. OnEndSA restores the previous dword from `hunter+0x500`. What object this is (camera mode vs. something else) is **UNVERIFIED**.
3. `cObjCharacterEx::PlaySA(char*, m4x4&)` CommonDll `0x27f10`. That sets `character+0x368 = 1`, freezes the phys object (saves flags, writes `0xfffffff8` / `7`), calls `CharacterFreeze` and `CharacterBrainFreeze`, sets every model part's body state to `CutScene`, and calls virtual `+0x174(clip)`.
4. `FUN_100480f0(hunter, "PlaySA")` — sets **every** part whose machine matches the body replicator to state `PlaySA`.

`HUMANLEGS.SCR` state `PlaySA` has an empty `Begin`. While `TestBodyCmd(BODYCMD_PLAYER_PLAYSA)` is set it stays there. When the command clears it plays `synchuntergetup` (`SetAnim("synchuntergetup")`) and then `Idle` (or `TurnCoDrive` in a vehicle). That clip is in `OBJECTS\CHARACTERS\TEENM\BODY\GAME.AP`. The fall clips are not in `GAME.AP`. They live in the map pak loaded with the teen body, names `pl_<species>_syncattack1` plus `pl_back_*`, `pl_left_*`, `pl_right_*` on the animals that have them (byte search of `TEENM\BODY\*.AP`). Examples: `pl_l_syncattack1` on MAP3B / HUNTMAP1, `pl_gw_syncattack1` on MAP2A, `pl_p_syncattack1` on HUNTMAP4. A map that does not load that pak does not have the clip.

**While it plays.** `ProcessPlaySA` CommonDll `FUN_100c3c40` RVA `0xc3c40` (virtual slot `+0xA4` on the attack-task vtables around `0x134748`). Each frame, if the current anim time crosses a script `sadmgtime`, it calls task virtual `+0xEC` (`FUN_100c2ee0` RVA `0xc2ee0`). That builds a direction from the animal position to the target position and calls the target's virtual `+0x14C` (hunter hurt) with **type 1**, the damage float, the target position, and that direction. Passing a 0 damage still enters hurt; it does not skip the call. The script `sadmgval` on the samples read (wild boar) is not 0.

**Get-up.** When the anim status passes the end threshold, the task calls virtual `+0xC0`, `FUN_100c3960` RVA `0xc3960`. That calls virtual `+0x214` on the partner and on the animal. Hunter slot `+0x214` is `cObjHunter::OnEndSA`, `FUN_10053cd0` RVA `0x53cd0`, `__fastcall(hunter)`:

- Clears `+0x368`.
- `CharacterFreeze` / `CharacterBrainFreeze`, virtual `+0x1F0(0)`, restores the phys flags saved at `+0x384/+0x388/+0x38C` and the dword at the camera-side `+0x14C`.
- Repositions from the root bone (`FUN_10050f80`) and calls virtual `+0x248` (`FUN_10048630`), which rebinds part 1's anim pak to the partner. It does not set a body state.

Clearing `+0x368` is what lets the legs leave `PlaySA`. Hunter virtual `+0x218` (`LAB_100485e0`) returns 0 when there is no object at `+0x648`, or that object's `+0x1F0` is 3 or 4, or `hunter+0x4D8` has bit `0x800` or `0x1000`. If `+0x368 != 0` it does not take the "finished" path. The task treats a 0 from `+0x218` as "abort", and then also calls `+0xC0` and sets the **animal** parts to `SAHurt`.

`cObjCharacterEx::OnEndSA` (CommonDll `0x28360`) is the base. It clears `+0x368`, unfreezes, and sets every part to `Idle`. The hunter override does not call that base; the legs script is what plays `synchuntergetup` instead of jumping straight to `Idle`.

**Not this feature:**

- `SetTimeBeforeGetUp` (GameDll `0x16bf0`, `__thiscall`, one stack float, `ret 4`) stores that float at `scriptEntity+0x28` then `entity+0x6FC`. On the hunter, `+0x6FC` is the stamina-blend timer in `FUN_1004fa90` RVA `0x4fa90`. No knockdown branch there.
- `HUMANLEGS` states `Falling` / `HardFall` / `Rising` are the jump arc (`jump/2`, `jump/3`) gated by `BODYCMD_PLAYER_HARDFALL` / `BODYCMD_PLAYER_ONGROUND`.
- `CameraShake` is a one-shot `shake` clip back to `Idle`. Hunter hurt uses it for a non-SA hit (below).
- Guide states `StayDown` / `Getup` are the puppet, section 5. They are not on the hunter.

Health, the dead bit, and `+0x6CC` are not written by `PlaySA` or `OnEndSA`.

## 2. Can we call it

Yes, on the **victim's** machine, without an animal object.

Smallest pair, both `__thiscall` on the local `cScr_ModHunter`:

| Call | Slot | RVA | Args |
|---|---|---|---|
| `PlaySA` | `+0x210` | GameDll `0x50e20` | `char* clip`, `m4x4& basis` |
| `OnEndSA` | `+0x214` | GameDll `0x53cd0` | none |

`PlaySA` does not take an attacker. The animal task's matrix has the **victim** position at `+0x30`. Rotation comes from the placement test; a mod-built rotation is **UNVERIFIED** against the clip. The name overload `cObjCharacterEx::PlaySA(char*, char*)` (CommonDll `0x1a7a0`) looks up the partner with virtual `+0xF0(name, "obj_character_ex")` and then calls the matrix overload on both, using the partner position. The hunter override is the matrix one. Calling the name overload on a hunter hits the override only if that slot was not replaced; on `cScr_ModHunter` slot `+0x210` is the matrix function, so the name form is the base class.

`PlaySA` alone does not stand the hunter back up. Nothing in GameDll calls `OnEndSA` except the vtable. The attack task is what calls it when the clip finishes. A co-op knock has no attack task on the victim, so the victim machine calls `OnEndSA` when the clip's anim status finishes (or after a fixed time). After that, `HUMANLEGS` `PlaySA` plays `synchuntergetup` and goes to `Idle`, **if** the legs are in `PlaySA` and `BODYCMD_PLAYER_PLAYSA` follows `+0x368`. That command link is **UNVERIFIED** (the flag and the command move together in this path; the bit assignment was not read).

Damage is not inside `PlaySA`. Skip the task's `+0xEC` call and health stays where it is. Do not also call hunter `+0x14C`.

The clip string has to be one the loaded teen body pak actually contains. `synchuntergetup` is the get-up, not the fall. Using it as the `PlaySA` clip is **UNVERIFIED** and would play the get-up twice if the legs script still appends it.

`SetBodyState("PlaySA")` / `HandleKey` is not a substitute. The freeze, `+0x368`, and the part-0 clip come from `PlaySA`.

## 3. Death vs knockdown

Hunter hurt is vtable `+0x14C`, `FUN_100530c0` RVA `0x530c0`:

```
void __thiscall Hurt(void* hunter, cObjEntity* attacker, float dmg, int type, tVect3* pos, float* dir);
```

- Returns immediately if `hunter+0x6CC != 0`.
- Subtracts `dmg` from profile attribute 9 (`+0x1F8`), unless `m_bGodMode`.
- If health is then `<= 0` and `hunter+0x6CC` is still 0, calls `FUN_10050350` RVA `0x50350` (`cObjHunter::OnDie`).

`OnDie`:

- Sets `+0xC1 |= 4`.
- `FUN_100480f0(hunter, "Death")` — every body part to state `Death`. `HUMANLEGS` `Death` plays `death` and does not leave.
- Sets `+0x6CC = 1`. That is the flag `PollClientRespawn` clears. While it is non-zero, `Hurt` returns immediately.
- Removes the hunter from the animal attack-target list and raises the mission-failed message. That is the level end.

`PlaySA` / `OnEndSA` do not set `+0x6CC`, do not set `+0xC1` bit 2, and do not write attribute 9.

A non-sync animal hit still goes through `Hurt` with type 1. For type 1, if not already in an SA (`+0x368 == 0`) and `+0x4D8` bit 2 is clear, it calls `FUN_10048260(hunter, "CameraShake")` (legs only) and applies an impulse with virtual `+0x90`. That shake returns to `Idle` by itself. It does not play `synchuntergetup` and it does set health down by `dmg`. If `dmg` reaches 0 health, it is `OnDie`, not a knockdown.

`cObjScriptedEntity::OnHurt` (CommonDll `0x3e290`) writes `entity+0x19C` and, at 0 health with `+0xC1` bit 2 clear, calls virtual `+0x154`. The hunter does not use that health field. Hunter health is profile attribute 9. `cObjCharacter::OnHurt` (CommonDll `0xb0f0`) forwards to the scripted hurt unless god mode, then notifies the brain at `+0x210`. The playable hunter's hurt override is the GameDll function above.

## 4. Player melee

The melee swing is the weapon body state `SecFire` in `WEAPON.SCR`.

- Command `com_secfire` (storage `DAT_10189e40`, `re/notes/console.md`). Registered default is `-1` (unbound). The live key is that binding; this job did not find a shipped default key.
- `Idle` / move states call `CheckSecFire()`. A rising `BODYCMD_WEAPON_SECFIRE` does `setstate(SecFire)`.
- `SecFire` `Begin` plays clip `kick`, sets `PLAYER_SHOOTING`, and leaves aim. `End` clears `PLAYER_SHOOTING`.
- When `GetAnimStatus()` passes `m_pWeapon.m_fAnimPercentF2` it calls `m_pWeapon.SecFire()` once.

Native `cMOD_ScrWeapon::SecFire` is GameDll `FUN_100365b0` RVA `0x365b0`. It calls the weapon **entity** virtual `+0x1CC` with no arguments. Knife `cObjKnife::SecFire` is `FUN_100323a0` RVA `0x323a0`; unless a field is already 2 it calls `FUN_100317e0(weapon, 2)`.

`FUN_100317e0` RVA `0x317e0` is the hit:

- Parent must be `IsEntityKindOf(..., "obj_hunter")`. Otherwise it returns.
- Origin from hunter virtual `+0xB4`, direction from virtual `+0x27C`, plus a muzzle offset from the weapon def (`+0x90/+0x94` when the argument is 2).
- `cBspTree::RayHit(bsp, origin, dir, 1.5, hit, 0x1e3736)`. Length is the constant `1.5`. Not a sphere and not "closest animal".
- On a hit it builds a small hit object and hands it to a manager with the weapon-def float at `+0x60` as the scale (kick damage lives on `cMOD_WeaponDef::GetKickDamage`, export RVA `0x5510`; that this float is that getter is **UNVERIFIED**).

Detect the swing the same way `player_tracker.cpp` detects `Fire` / `Reload`: `cObjItemEx::GetCurrentState` (CommonDll `0x32c00`) equals `SecFire`. A rising edge is the start of the kick. The ray is later in the clip, at `m_fAnimPercentF2`, so the edge is early. Whether a co-op puppet is in that BSP ray is **UNVERIFIED** (puppets are not a phys body). Detect the swing on the attacker and send the knock; do not rely on the ray touching the puppet.

## 5. Puppet clip

`py re\scripts\ap_list.py` on `OBJECTS\NPC\MOUNTAINGUIDE\GAME.AP` (332 names) includes `fall`, `liedead`, `liedeadidle`, `deadfrontup`, `deadbackup`, `syncguidegetup`, `syncguidegetup_back`, `syncguidegetup_foot`. `synchuntergetup` is not in this pack.

`GUIDELEGS.SCR` (same states in `SGUNNPC.SCR` / `HGUNNPC.SCR` / `RIFLENPC.SCR` with the same short clip names):

| Order | Body state | Clip | Then |
|---|---|---|---|
| Front, down | `StayDown` | `deadfrontup` | Holds. `Update` waits for the anim and does not change state. |
| Front, up | `Getup` | `syncguidegetup` | `Idle` |
| Back, down | `StayDown1` | `deadbackup` | Holds, same as `StayDown`. |
| Back, up | `Getup1` | `syncguidegetup_back` | `Idle` |
| Foot only | `Getup2` | `syncguidegetup_foot` | `Idle`. No matching down state. |

Play `StayDown` then `Getup`, or `StayDown1` then `Getup1`. The down state will not advance by itself.

`Liedead` plays `fall` and then `FinalLiedead` (`liedead`) and sets `PLAYER_NPC_LIEDEAD`. That is the death collapse, not the get-up. `liedeadidle` is in the pak and unused by these states.

Whether `deadfrontup` is a fall into the pose or a static lie is **UNVERIFIED** (one clip, then the last frame holds). Armed parts: the NPC weapon scripts call the short name; the pak also has the prefixed form (`shotgun1/…`). Which string the puppet must pass was already left open in `re/notes/anim-names.md`.

## Proposed design inputs

On the **victim's** machine, when a knock message arrives:

1. If `+0x6CC != 0` or `+0xC1` bit 2 is set, this is a death, not a knock. Do not call `PlaySA`.
2. Call hunter `PlaySA(clip, basis)` (`+0x210`). `basis+0x30` = the victim position. `clip` = a `pl_*_syncattack*` name that this map's teen body pak contains. No attacker pointer. Do not call `+0x14C`.
3. When that clip finishes, call `OnEndSA` (`+0x214`). That clears `+0x368` and restores phys. The legs script then plays `synchuntergetup` and returns to `Idle`.
4. Do not write `+0x6CC` or `+0xC1`. Do not set health to 0.

On the **attacker's** machine:

- Melee: rising `GetCurrentState() == "SecFire"` on the active weapon. The engine ray is 1.5 long from the hunter and will not reliably see a puppet. Pick the victim in co-op code (remote player in range, in front).
- A shot can use the same victim message. Nothing in the animal SA path is required on the attacker.

On the **other** machines, the puppet is not a hunter and has no `PlaySA` get-up. Drive the guide states: `StayDown` then `Getup` (front), or `StayDown1` then `Getup1` (back). Do not use `Liedead`.

## Proposed MEMORY facts

- `cScr_ModHunter` vtable GameDll `+0x106ff0` slot `+0x210` is `FUN_10050e20` (`cObjHunter::PlaySA(char*, m4x4&)`). Slot `+0x214` is `FUN_10053cd0` (`OnEndSA`). Slot `+0x14C` is `FUN_100530c0` (hurt: profile attribute 9; at health `<= 0` calls `FUN_10050350`). Slot `+0x220` jumps to `cObjHumanEx::ResetTorso`. Evidence: `re/notes/knock-4.c` vtable dump and `knock-3.c`.
- `FUN_10050350` sets `hunter+0xC1 |= 4`, body state `Death` on every part, and `hunter+0x6CC = 1`, and it is the mission-failed path. `FUN_100530c0` returns immediately when `+0x6CC != 0`. `PlaySA` / `OnEndSA` do not write those fields. Evidence: `knock-3.c`.
- `cObjCharacterEx::PlaySA(char*, m4x4&)` CommonDll `0x27f10` sets `+0x368 = 1`, freezes phys and brain, sets parts to `CutScene`, then virtual `+0x174(clip)`. `OnEndSA` `0x28360` clears `+0x368` and sets parts to `Idle`. Evidence: `knock-1.c`. String at CommonDll RVA `0x122f44` is `Idle`.
- Animal sync start is CommonDll `FUN_100cadf0` (`TryPlaySA`). It calls virtual `+0x210` on the target and the animal with the victim position in the matrix translation. End is task virtual `+0xC0` = `FUN_100c3960`, which calls both characters' virtual `+0x214`. Timed damage is task virtual `+0xEC` = `FUN_100c2ee0`, which calls the target `+0x14C` with type 1. Evidence: `knock-2.c`, `knock-7.c`.
- `SetTimeBeforeGetUp` GameDll `0x16bf0` is `mov [entity+0x6FC], float; ret 4` (`knock-6.c` bytes). Hunter `FUN_1004fa90` uses `+0x6FC` as a stamina blend, not a knockdown.
- Weapon secondary fire: state name `SecFire`, native `FUN_100365b0`, hit ray `FUN_100317e0` (`cBspTree::RayHit` length `1.5`, flags `0x1e3736`, parent must be `obj_hunter`). Evidence: `knock-3.c`, `knock-10.c`, `WEAPON.SCR` state `SecFire`.
- Mountainguide `GAME.AP` contains `deadfrontup`, `deadbackup`, `syncguidegetup`, `syncguidegetup_back`, `syncguidegetup_foot`, `fall`, `liedead`. `GUIDELEGS.SCR` states `StayDown`, `StayDown1`, `Getup`, `Getup1`, `Getup2` play the first five. `StayDown` / `StayDown1` do not leave the state.

## Open questions

- No launch, so it is **UNVERIFIED** that `PlaySA` of a `pl_*` clip is the camera-on-the-ground the user sees, and that `OnEndSA` plus the legs script is enough to stand back up with no attack task running.
- `BODYCMD_PLAYER_PLAYSA` was not tied to `+0x368` by a direct read.
- The object whose `+0x14C` `PlaySA` sets to 1 was not named.
- `RelativePosRotOkForSA` and `FUN_100c3170` (distance, angle, matrix rotation) were not decompiled.
- Retail default key for `com_secfire` was not in a config in this tree (registered default `-1`).
- Whether `deadfrontup` / `deadbackup` read as a fall or as a held lie is **UNVERIFIED**.
