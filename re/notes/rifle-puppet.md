# Rifle puppet — bolt-action model, muzzle flash, hand alignment

## Progress
Done. Model map, clip choice, and solo MAP3B shots are below. Open: `rifle1.model` has no clip `SetAnim` can apply, so that mesh stays in bind pose.

## Problem
A two-PC test (2026-10-17) saw the bolt-action pickup on other puppets: the gun was not sitting in the hands, and a muzzle flash stayed frozen on. The host log named the local weapon `objects/items/game/weapons/rifle21st/rifle2.skin`. `weapon_kind()` mapped every model containing `rifle` to `kWeaponRifle` (3), and `weapon_model()` attached `objects/items/game/weapons/rifle1/rifle1.skin`. `choose_clips()` then asked that mesh for `idlealertneutral` / `fire` (the shotgun/handgun NPC names).

## Weapon folders
`re/extracted/OBJECTS/ITEMS/GAME/WEAPONS` (the `*1ST` directories are first-person):

| Folder | Role |
|---|---|
| `HANDGUN3` | Third-person pistol. Bones include `bolt`, `flame`. Clips include `fire`, `reload`, `idlealertneutral`, `walkforwardalert`, `run`. |
| `SHOTGUN1` | Third-person shotgun. Bones include `bolt`. No `flame` bone. Same NPC clip set as the handgun, plus `reload0`/`reload1`/`reload2`. |
| `RIFLE1` | Only third-person rifle. Bones: `bolt`, `lev`, `flame`, `shell`, `hammer`. Clips: `give3030`, `idle3030`, `oldh_give3030`, `oldh_idle3030`, `stand`. No `fire`, no `idlealertneutral`. |
| `RIFLE21ST` | First-person bolt-action (`rifle2`). Bones: `bolt`, `flame`, `bolts`, `magazine`, `shell`. Clips: `idle`, `fire`, `reload`, `run`, `walk`, `cs_idle`, `stand`, `hurt`, `defend0`–`2`. No third-person folder. |
| `HANDGUN51ST` | First-person `handgun5`. Bones include `flame`. Clips: `idle`, `fire`, `reload`, `walk`, `run`. No third-person folder. |
| `RIFLE1ST`, `RIFLE31ST`, `RIFLE41ST`, `RIFLE51ST`, `DBRIFLE1ST`, `HANDGUN21ST`, `SHOTGUN1ST`, `KNIFE1ST`, `MACHETA1ST`, `BOW1ST`, `STRIKER1ST`, `TRANQGUN1ST`, `TRANQHANDGUN1ST`, `BLOWDARTS1ST`, `MBRANCH1ST` | First-person only. |
| `MBRANCH`, `BLOWDARTS`, `RADIO` | Third-person props. `MBRANCH` clips are `cs_pumafight`, `stand` only. Not a player long-gun. |

There is no third-person skin whose name matches `rifle2` or `handgun5`.

## NPC scripts (hand and clips)
`SCRIPTS/MODELS/NPC` has no weapon scripts. The weapon machines are `SCRIPTS/MODELS/ITEMS/WEAPONS/{RIFLE/RIFLENPC.SCR,SGUN/SGUNNPC.SCR,HGUN/HGUNNPC.SCR}`.

Default hand (`m_bSwitchedGunHand == FALSE`):

| Script | Attach + first LinkPart | Second LinkPart |
|---|---|---|
| `RIFLENPC.SCR`, `SGUNNPC.SCR` | `BipLHand` | `BipLFoot` |
| `HGUNNPC.SCR` | `BipRHand` | `BipLFoot` |

`cScrEntity::LinkPart` (CommonDll `0x3f590`, `re/notes/lead-attach.c` in the main tree) syncs **weapon model part 0** to the named owner bone. A second call replaces the first. `BipLFoot` is not a second grip. This change keeps the single hand link the shotgun puppet already uses. No attach offset appears in those scripts.

`SetAnim` names in the NPC machines (`idlealertneutral`, `fire`, `reload` / `reload1`, `walkforwardalert`, `run`, …) are the short names stored on `handgun3.model` and `shotgun1.model`. `RIFLENPC.SCR` uses the same names, but `rifle1.model` does not contain them. NPC body packs `OBJECTS/NPC/*/RIFLE1.AP` are `give3030` / `oldh_idle3030` only (`re/notes/anim-names.md`). The puppet body is the mountain guide, which has `shotgun1.ap` / `handgun3.ap`, so the **arms** stay on those prefixes. The **gun mesh** must be a name that exists on that skin.

The player machine `RIFLE.SCR` (`cBodyDbrifle` and the other `cBodyWeapon` rifles) attaches `BipRHand` and plays `idle` / `fire` / `reload` — the first-person clip names.

## Muzzle flash
`flame` is the muzzle-flash bone (present on `rifle1.model`, `rifle2.model`, `handgun3.model`, `handgun5.model`; absent on third-person `shotgun1.model`). Bind pose leaves it visible. Asking `rifle1` for `idlealertneutral` never leaves that pose, which is the frozen flash on a puppet that was given the bolt-action.

Runtime (MAP3B, `debug.log` / `dh2coop.log`):

- `handgun3` and `shotgun1`: `idlealertneutral` and `fire` return `parts=1`. The shotgun has no `flame` bone. The handgun returns to `idlealertneutral` when the 600 ms fire window ends.
- `rifle2.model` `idle` returns `parts=1`. `fire`, and `rifle1.model` `idle3030` / `stand`, return `parts=-1` and log `Could not forward animation <name> from <gun> to Torso model <that skin>`. Those names are not safe to play. The bolt-action puppet stays on `idle`, so the forward line does not repeat and the flash does not stay up. A flash only during the shot is **UNVERIFIED**: the `fire` clip is what would show it, and that call throws.
- Spawn of `handgun3`, `shotgun1`, `rifle1`, and `rifle2` each log once `Could not create scripted body state machine with class cBody…`. The process stays up. Same class of line the shotgun puppet already logged. `handgun5` did not log it.

## Kind map
| `weaponId` | Model substring | Skin on the puppet | Hand | Arm pack | Gun clips |
|---|---|---|---|---|---|
| 1 `kWeaponHandgun` | `handgun` / `pistol` (not `handgun5`) | `handgun3/handgun3.skin` | `BipRHand` | `handgun3/` | NPC short names |
| 2 `kWeaponShotgun` | `shotgun` / `12ga` | `shotgun1/shotgun1.skin` | `BipLHand` | `shotgun1/` | NPC short names (`reload1`) |
| 3 `kWeaponRifle` | `rifle` (not `rifle2`) | `rifle1/rifle1.skin` | `BipLHand` | `shotgun1/` | none (every clip forwards) |
| 4 `kWeaponOther` | anything else | none | | | |
| 5 `kWeaponRifle2` | `rifle2` (covers `rifle21st`) | `rifle21st/rifle2.skin` | `BipLHand` | `shotgun1/` | `idle` only |
| 6 `kWeaponHandgun5` | `handgun5` | `handgun51st/handgun5.skin` | `BipRHand` | `handgun3/` | `idle` only |

`rifle3` / `rifle4` / `rifle5` / `dbrifle` still fall through to kind 3 (the only third-person rifle). Melee (`knife`, `macheta`, `mbranch`) stays kind 4: no third-person player melee with a fire clip.

## Test
`[Puppets] SpikeWeapon` (default 0). With `Spike=1` and `SpikeSway=0`, the solo puppet stands 3 m in front, holds that kind, and the shot counter bumps every 3 s. `FireMs` in the ini is now the fire-window length (it was unused; the code used 400). Solo `Main7.exe +mod data -nointro -developper -cheats -spmap MAP3B` from this worktree. Two other `Main7` processes were already running from `dh2coop\game` and `game2`; they were left alone.

Shots in `re/notes/coop-local/` (gitignored):

| Kind | Idle | During the shot | ~1 s after | Log |
|---|---|---|---|---|
| 1 handgun3 | `rifle-puppet-k1-idle.png` | `rifle-puppet-k1-fire.png` | `rifle-puppet-k1-after.png` | `idlealertneutral` parts=1, `fire` parts=1, back to idle |
| 2 shotgun1 | `rifle-puppet-k2-idle.png` | `rifle-puppet-k2-fire.png` | `rifle-puppet-k2-after.png` | same, `fire` parts=1 |
| 3 rifle1 | `rifle-puppet-k3-idle.png` | `rifle-puppet-k3-fire.png` | `rifle-puppet-k3-after.png` | no gun `SetAnim` in the final build. Earlier `idle3030` and `stand` both `parts=-1` |
| 5 rifle2 | `rifle-puppet-k5-idle.png` | `rifle-puppet-k5-fire.png` | `rifle-puppet-k5-after.png` | `idle` parts=1. No `Could not forward` once `fire` is not requested. Arms still play `shotgun1/fire` |
| 6 handgun5 | `rifle-puppet-k6-idle.png` | `rifle-puppet-k6-fire.png` | `rifle-puppet-k6-after.png` | final build: `idle` parts=1, debug.log had no `Could not` line |

No fatal engine dialog. Processes exited only when this job closed its own pid.

## Proposed MEMORY facts
- Third-person weapon skins under `OBJECTS/ITEMS/GAME/WEAPONS` are only `handgun3`, `shotgun1`, `rifle1`, plus `mbranch`, `blowdarts`, `radio`. `rifle2` and `handgun5` exist only as `RIFLE21ST` and `HANDGUN51ST`. Evidence: directory listing of that folder.
- `rifle1.model` clip names are `give3030`, `idle3030`, `oldh_give3030`, `oldh_idle3030`, `stand`. It has a `flame` bone and no `fire` / `idlealertneutral`. Evidence: length-prefixed strings in `RIFLE1/RIFLE1.MODEL`.
- `rifle2.model` (first-person) clip names include `idle`, `fire`, `reload`, `walk`, `run`, and it has a `flame` bone. Evidence: length-prefixed strings in `RIFLE21ST/RIFLE2.MODEL`.
- Third-person `shotgun1.model` has no `flame` bone. The first-person shotgun does. Evidence: same string parse.
- `cScrEntity::LinkPart` syncs the weapon's model part 0 onto the named bone; the NPC scripts' second call (`BipLFoot`) replaces that link rather than adding a second hand. Evidence: decompile at CommonDll `0x3f590` (`re/notes/lead-attach.c` in the main checkout).
- `cModelPart::SetAnim("fire")` on `rifle21st/rifle2.skin` and `handgun51st/handgun5.skin` returns through an SEH exception (`parts=-1`) and `debug.log` records `Could not forward animation fire from rifle2 to Torso model objects/items/game/weapons/rifle21st/rifle2.skin` (same shape for `handgun5`, and for `idle3030` / `stand` on `rifle1.skin`). `SetAnim("idle")` on rifle2 and handgun5 returns `parts=1`. Evidence: `game/Bin/dh2coop.log` and `debug.log` from the MAP3B spike runs 2026-10-17 15:47–15:53.
