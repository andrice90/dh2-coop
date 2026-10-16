"""Build re/notes/class-catalog.md from the export and class lists.

Inputs (all under re/exports/):
  <dll>.classes.txt        "name count" per line. Names prefixed with '7' are
                           vtable twins (??_7X@@6B@) and '0' prefixed are ctor
                           artifacts; both fold into the plain class name.
  <dll>.demangled.txt      "mangled<TAB>demangled" (from demangle_exports.py).

Subsystem grouping is name-based only. Nothing here is verified against code.
Run after demangle_exports.py:  py re\\scripts\\build_class_catalog.py
"""

import collections
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXPORTS = ROOT / "re" / "exports"
NOTES = ROOT / "re" / "notes"
OUT = NOTES / "class-catalog.md"

DLLS = ["EngineDll7.dll", "CommonDll7.dll", "GameDll7.dll"]
SHORT = {"EngineDll7.dll": "Engine", "CommonDll7.dll": "Common", "GameDll7.dll": "Game"}

SUBSYSTEMS = [
    "Networking",
    "Entities/World",
    "AI",
    "Animation",
    "Rendering/Shaders",
    "Camera",
    "Physics/Collision",
    "Sound",
    "Input",
    "Files/Archives",
    "Lua/Scripting",
    "GUI/HUD",
    "Game rules/Weapons",
    "Math/Containers",
    "Other",
]

# Explicit assignments for names the prefix rules would get wrong.
OVERRIDE = {
    # Networking
    "cPingManager": "Networking",
    "cLanDataChannel": "Networking",
    # Entities / world
    "cBaseClass": "Entities/World",
    "cClassManager": "Entities/World",
    "cLevelMap": "Entities/World",
    "cBspTree": "Entities/World",
    "cBspModel": "Entities/World",
    "cObjEntity": "Entities/World",
    "cObjManager": "Entities/World",
    "tBspBush": "Entities/World",
    "tLeafPvs": "Entities/World",
    "tVXBsp": "Entities/World",
    "tVXPrelitBsp": "Entities/World",
    "cVehicleDef": "Entities/World",
    "cWheelDef": "Entities/World",
    "cVehicleFX": "Entities/World",
    "cVehWheelTrail": "Entities/World",
    "cObjHintNode": "Entities/World",
    "cObjScriptedBspModel": "Entities/World",
    # AI
    "cObjHuntAreaAIEntity": "AI",
    "cAnimalDef": "Game rules/Weapons",
    "cPathGrid": "AI",
    "cAreaPathFinder": "AI",
    "cPathHandle": "AI",
    "cPathQueue": "AI",
    "cPPathQueue": "AI",
    "cPCloseList": "AI",
    "cCloseList": "AI",
    "cSegmentList": "AI",
    "cPathFinderInterface": "AI",
    "cPathSolInterface": "AI",
    # Animation
    "cBoneSound": "Sound",
    "cBoneSoundStep": "Sound",
    "cBoneLightVisual": "Rendering/Shaders",
    "cBoneFx": "Animation",
    "cScriptedBodyStateMachine": "Animation",
    "cPartHumanLegs": "Animation",
    "cPartHumanTorso": "Animation",
    "cJoint": "Physics/Collision",
    # Rendering
    "cModelReplicator": "Rendering/Shaders",
    "cModelColl": "Physics/Collision",
    "cModelFileLoad": "Files/Archives",
    "cModelFileLoadConvert": "Files/Archives",
    "cModelFileLoadPak": "Files/Archives",
    "cModelFileSave": "Files/Archives",
    "cModelFileSaveStd": "Files/Archives",
    "cModelPartLink": "Rendering/Shaders",
    "cRenderCamera": "Camera",
    "cLightAppearance": "Rendering/Shaders",
    "cPvsCache": "Rendering/Shaders",
    "cSky": "Rendering/Shaders",
    "cTerrain": "Rendering/Shaders",
    "cTerrainGrass": "Rendering/Shaders",
    "cBspRain": "Rendering/Shaders",
    "cBspModelDecals": "Rendering/Shaders",
    "cDecalManager": "Rendering/Shaders",
    # Camera
    "cCamera": "Camera",
    "cDebugCamera": "Camera",
    # Physics
    "PHYSICS": "Physics/Collision",
    "cPhysVehicle": "Physics/Collision",
    "cPhysWhinch": "Physics/Collision",
    "cCMParticle": "Physics/Collision",
    "cCMEllipsys": "Physics/Collision",
    "tPhysObjData": "Physics/Collision",
    "cDynaSkeleton": "Physics/Collision",
    "cDynaSkeletonRigid": "Physics/Collision",
    "cDynaSkeletonComplex": "Physics/Collision",
    "cDynaControl": "Physics/Collision",
    "cDynaControlLink": "Physics/Collision",
    "cDynaControlRigid": "Physics/Collision",
    "cDynaControlRigid2": "Physics/Collision",
    # Files
    "cDataItemDefPool": "Files/Archives",
    "cSaveBuffer": "Files/Archives",
    "tMD5_CTX": "Files/Archives",
    "tTargaHeader": "Files/Archives",
    "cMultiFile": "Files/Archives",
    "cAsyncReader": "Files/Archives",
    "cExportInf": "Lua/Scripting",
    "cExportKey": "Lua/Scripting",
    # Lua / scripting
    "cScrContainer": "Lua/Scripting",
    "cScrScaner": "Lua/Scripting",
    "cScrMemScaner": "Lua/Scripting",
    "cVMachine": "Lua/Scripting",
    "FcVMachine": "Lua/Scripting",
    "cObjScrFx": "Rendering/Shaders",
    "cObjScrPureFX": "Rendering/Shaders",
    "cScrAIHunt": "Lua/Scripting",
    # GUI / HUD
    "cFont": "GUI/HUD",
    "cGuiSaveMan": "GUI/HUD",
    "cConsoleWnd": "GUI/HUD",
    "cConsoleView": "GUI/HUD",
    "cConsole": "Other",
    "cConsoleFunc": "Other",
    "cConsoleSlot": "Other",
    "cConsoleVar": "Other",
    # Game rules
    "cProfile": "Game rules/Weapons",
    "cHunterProfile": "Game rules/Weapons",
    "cObjItem": "Game rules/Weapons",
    "cObjItemEx": "Game rules/Weapons",
    "cScrItem": "Game rules/Weapons",
    "cScrItemEx": "Game rules/Weapons",
    "cItemDef": "Game rules/Weapons",
    "cWeaponDef": "Game rules/Weapons",
    "cScopeDef": "Game rules/Weapons",
    "cReserveDef": "Game rules/Weapons",
    # Math / containers
    "cKeyArrayEx": "Math/Containers",
    "cStrHash": "Math/Containers",
    "FcStrHash": "Math/Containers",
    "cBufList": "Math/Containers",
    "FcBufList": "Math/Containers",
    "tKeyItemList": "Math/Containers",
    "cGenericMatrix": "Math/Containers",
    "cSquareMatrix": "Math/Containers",
    "_tPlane": "Math/Containers",
    # Other
    "cClock": "Other",
    "tTimeMap": "Other",
    "cCmdLine": "Other",
    "cApplication": "Other",
    "cClass": "Other",
    "cDebugSystem": "Other",
    "cBackTraceException": "Other",
    "FcBackTraceException": "Other",
    "cStructuredException": "Other",
    "cPT": "Other",
    "cKeyProperty": "Other",
    "cKeyChoiceProperty": "Other",
    "cKeyColorProperty": "Other",
    "cKeyDirectionProperty": "Other",
    "cKeyFileNameProperty": "Other",
    "cKeyFlagsProperty": "Other",
    "cKeyFloatProperty": "Other",
    "cKeyIntProperty": "Other",
    "cKeyStringProperty": "Other",
    "cKeyVectorProperty": "Other",
    "cKeyArrayEx_": "Other",
    "FcRenderPipelineInterface": "Rendering/Shaders",
    "Striper": "Rendering/Shaders",
    "cScrHuman": "Lua/Scripting",
}

RULES = [
    (r"^cNet", "Networking"),
    (r"^cPhys|^cDynaControl|^cDynaSkeleton|^cCollManager|^cCollModel|^cCollisionBaseClass|^cCM|^PHYSICS$|^tPhys", "Physics/Collision"),
    (r"^cAI|^cPath|^cHunt|^cScrAIHunt$", "AI"),
    (r"^cAnim|^cBone|^cBody|^cModelAnim|^cModelSkin|^cLimbSolver|^cJoint$", "Animation"),
    (r"^cSound|^cBoneSound", "Sound"),
    (r"^cInput", "Input"),
    (r"^cGui", "GUI/HUD"),
    (r"^cMOD_", "Game rules/Weapons"),
    (r"^cCamera|^cDebugCamera|^cRenderCamera", "Camera"),
    (r"^cFX|^cCFX|^cHFX|^cObjFx|^cObjPureFX|^cObjCachedFX|^cObjScrFx|^cObjScrPureFX", "Rendering/Shaders"),
    (r"^cRender|^cShader|^cTexture|^cMesh|^cMaterial|^cLight|^cBaseLight|^cBaseStaticLight|"
     r"^cDirectionalLight|^cDynamicLight|^cPointRadialLight|^cPointSpotLight|^cSky$|^cTerrain|"
     r"^cPipelineInterface|^cRenderPipeline|"
     r"^cBlendTerrain|^cColormapTerrain|^cRSD|^cModel$|^cModelDef|^cModelManager|^cModelMesh|"
     r"^cModelPart$|^cModelStatic|^cModelLOD|^cModelMakeup|^cRenderBuffer|^FcRender", "Rendering/Shaders"),
    (r"^cFile|^cDiskFile|^cCdFile|^cZipFile|^cArcFile|^cMemFile|^cMemBuffer|^cByteStream|^cDataStream|"
     r"^cStream$|^cFileManager|^cResource|^cDataItem|^FcFileManager|^FcByteStream|"
     r"^cMultiFile", "Files/Archives"),
    (r"^cObj", "Entities/World"),
    (r"^cScr|^cVMachine", "Lua/Scripting"),
    (r"^cVehicle|^cWheel|^cVehWheel", "Entities/World"),
    (r"^t(Quat|Vect2|Vect3|AABB|Plane)$|^cRLE|^cGenericMatrix|^cSquareMatrix|^cKey", "Math/Containers"),
]

METHOD_TIERS = [
    (5, r"^(Spawn|ClientSpawn|Update|Tick|Create|Damage|Hurt|OnHit|OnHurt|Kill|Die|Fire|Load|Save|Send|"
        r"Receive|Serializ|Dispatch|Connect|Disconnect|Target|Impulse)"),
    (4, r"(Pos|Position|Transform|Time|Pause|Health|Dead|Death|Player|Client|Server|Ping|Attach|Activate|"
        r"Process|Parse|Hit|Remote|Shot|Damage)"),
    (3, r"^(Init|Destroy|Open|Close|Draw|Move|Trigger|Read|Write|Play|Run|Execute|Compile|Precache)"),
    (2, r"^(Get|Set|Is|Has|Add|Remove|Clear|Reset)"),
]

KEY_SHORTLIST = [
    # Networking (engine internal server/client)
    ("Networking", "cNetServer::Create(int,int,int)", "open the internal server"),
    ("Networking", "cNetServer::DispatchMessages(void)", "server per-frame message pump"),
    ("Networking", "cNetServer::ReceiveClientMessages(void)", "server receive path"),
    ("Networking", "cNetServer::SendMessageToClient(void *,int,int,int)", "server -> client send"),
    ("Networking", "cNetServer::DisconnectClient(int)", "drop a client by id"),
    ("Networking", "cNetClient::Init(int,char const *,int)", "client init, takes a char const * argument"),
    ("Networking", "cNetClient::TryConnect(void)", "client connect attempt"),
    ("Networking", "cNetClient::ReceiveServerMessages(int)", "client receive path"),
    ("Networking", "cNetGameState::ParseNetSegment(class cObjManager *,unsigned char *,int)", "state segment decode into objects"),
    ("Networking", "cNetGameState::SendUnreliableMessages(class cLanDataChannel *)", "unreliable send over LAN channel"),
    # Entities / spawn / transform
    ("Entities", "cObjEntity::Spawn(void)", "base entity spawn"),
    ("Entities", "cObjEntity::ClientSpawn(void)", "client-side spawn hook"),
    ("Entities", "cObjEntity::ClientCommand(struct tClientCommand &)", "per-entity client command"),
    ("Entities", "cObjEntity::Update(float)", "per-entity tick (dt)"),
    ("Entities", "cObjCharacterEx::Spawn(void)", "character spawn"),
    ("Entities", "cClassManager::UpdateAll(float)", "global entity update loop"),
    ("Entities", "cClassManager::UpdateOne(int,float)", "single-entity update"),
    # Damage / death
    ("Damage", "cScrEntity::Hurt(int,int,int)", "script damage entry"),
    ("Damage", "cObjScriptedEntity::ScrHurt(class cObjScriptedEntity *,float,int)", "scripted damage with source"),
    ("Damage", "cObjCharacter::OnHit(class cObjEntity *,struct tVect3 const &,struct tVect3 const &)", "hit callback"),
    ("Damage", "cObjCharacter::OnHurt(class cObjScriptedEntity *,float,int,struct tVect3 const &,struct tVect3 const &)", "hurt callback"),
    ("Damage", "cAIHuntAnimalInterface::FuncInflictDamage(void *,float)", "animal damage function"),
    ("Damage", "cAIHuntBrain_Animal::OnHurt(class cObjEntity *)", "animal hurt reaction"),
    ("Damage", "cAIHuntBrain_Animal::Die(int)", "animal death"),
    # AI target / time
    ("AI target", "cAIHuntBrain_Animal::TargetReached(struct tVect3 &,unsigned char)", "animal target reached"),
    ("AI target", "cAIHuntBrain_Animal::UpdateSenses(float)", "animal sensing tick"),
    ("Time", "cAIHuntAnimalInterface::SetTimeMultiplier(float)", "animal time multiplier"),
    ("Time", "cBaseClass::Pause(float)", "pause with time arg"),
    # Map load / save
    ("Map/save", "cLevelMap::Load(char *,unsigned char)", "map load"),
    ("Map/save", "cLevelMap::Save(char *)", "map save"),
    ("Map/save", "cObjWorld::Load(class cByteStream &)", "world state load"),
    ("Map/save", "cObjWorld::Save(class cByteStream &)", "world state save"),
    ("Map/save", "cObjEntity::Save(class cByteStream &)", "entity state save"),
    ("Map/save", "cGuiSaveMan::SaveGame(char const *)", "save slot write"),
    ("Map/save", "cGuiSaveMan::LoadGame(char const *)", "save slot read"),
    ("Map/save", "cProfile::Save(unsigned char *)", "player profile save"),
    ("Map/save", "cHunterProfile::UpdateSaveSlot(void)", "hunter profile save slot"),
    # Player
    ("Player", "cObjHumanEx::ClientCommand(struct tClientCommand &)", "player client command"),
    ("Player", "cObjCharacterEx::UpdateAttr(int,float,int)", "player attribute update"),
]


def load_classes(dll):
    """Return {base_name: {"count": int, "vtable": bool, "zero_twin": bool}}."""
    raw = {}
    for line in (EXPORTS / f"{dll}.classes.txt").read_text(encoding="ascii").splitlines():
        parts = line.split()
        if len(parts) == 2:
            raw[parts[0]] = int(parts[1])
    bases = {}
    for name, count in raw.items():
        if name[0] in "07" and name[1:] in raw:
            continue  # twin of a plain name, handled below
        bases[name] = {"count": count, "vtable": False, "zero_twin": False}
    for name, count in raw.items():
        if name[0] in "07" and name[1:] in raw:
            base = name[1:]
            if name[0] == "7":
                bases[base]["vtable"] = True
            else:
                bases[base]["zero_twin"] = True
    return bases, len(raw)


def load_demangled(dll):
    rows = []
    for line in (EXPORTS / f"{dll}.demangled.txt").read_text(encoding="ascii").splitlines():
        mangled, _, demangled = line.partition("\t")
        rows.append((mangled, demangled))
    return rows


SCOPE_RX = re.compile(r"^\?([^?@$][^@]*)@([^@?$][^@]*)@@")


def extract_methods(rows):
    """Return {class: {method: overload_count}} for function exports only."""
    out = collections.defaultdict(lambda: collections.Counter())
    for mangled, demangled in rows:
        if "(" not in demangled:
            continue  # data member or vftable
        m = SCOPE_RX.match(mangled)
        if not m:
            continue
        method, scope = m.group(1), m.group(2)
        if method.startswith("_") or method.startswith("?"):
            continue
        out[scope][method] += 1
    return out


def method_score(name):
    if re.match(r"^Createc[A-Z]", name):
        return 0  # per-class factory (CreatecObjX), not informative
    for score, rx in METHOD_TIERS:
        if re.search(rx, name):
            return score
    return 1


def notable_methods(methods, cls_name, limit=6):
    names = [n for n in methods if n != cls_name]
    names.sort(key=lambda n: (-method_score(n), n))
    return names[:limit]


def classify(name):
    if name in OVERRIDE:
        return OVERRIDE[name]
    for rx, sub in RULES:
        if re.search(rx, name):
            return sub
    return "Other"


def main():
    per_dll = {}
    all_rows = {}
    for dll in DLLS:
        bases, raw_lines = load_classes(dll)
        rows = load_demangled(dll)
        per_dll[dll] = (bases, raw_lines)
        all_rows[dll] = rows

    # Referenced type names that have no export list of their own.
    class_names = set()
    for dll in DLLS:
        class_names.update(per_dll[dll][0].keys())
    referenced = collections.Counter()
    ref_rx = re.compile(r"\b(?:class|struct) ([ct][A-Z]\w*)\b|\b([ct][A-Z]\w*)::")
    for dll in DLLS:
        for _, demangled in all_rows[dll]:
            for g1, g2 in ref_rx.findall(demangled):
                name = g1 or g2
                if name not in class_names:
                    referenced[name] += 1

    # Entries: (dll, base, count, vtable, zero_twin, subsystem, methods)
    entries = []
    methods_by_dll = {dll: extract_methods(all_rows[dll]) for dll in DLLS}
    for dll in DLLS:
        bases, _ = per_dll[dll]
        for base, info in bases.items():
            methods = methods_by_dll[dll].get(base, {})
            entries.append({
                "dll": dll,
                "name": base,
                "count": info["count"],
                "vtable": info["vtable"],
                "zero_twin": info["zero_twin"],
                "sub": classify(base),
                "methods": methods,
            })

    entries.sort(key=lambda e: (e["sub"], DLLS.index(e["dll"]), e["name"]))

    # Shortlist: resolve demangled keys to mangled names.
    resolved = []
    missing = []
    for area, key, why in KEY_SHORTLIST:
        hits = []
        for dll in DLLS:
            for mangled, demangled in all_rows[dll]:
                if key in demangled:
                    hits.append((dll, mangled, demangled))
        if not hits:
            missing.append(key)
            continue
        dll, mangled, demangled = hits[0]
        resolved.append((area, key, SHORT[dll], mangled, why, len(hits)))

    counts = {dll: per_dll[dll][1] for dll in DLLS}
    total_base = sum(len(per_dll[dll][0]) for dll in DLLS)
    total_exports = sum(len(all_rows[dll]) for dll in DLLS)

    lines = []
    w = lines.append
    w("# Class catalog (Track F)")
    w("")
    w("Generated by `re/scripts/build_class_catalog.py` from `re/exports/*.classes.txt` and `re/exports/*.demangled.txt`. "
      "Subsystem grouping and method ranking are name-based only; nothing here is verified against code. "
      "Export counts are the plain-name counts from `*.classes.txt`.")
    w("")
    w("## Co-op relevant shortlist")
    w("")
    w("Mangled names are copied from the export lists. Use them for hooking by export name.")
    w("")
    w("| # | Area | Class::Method (demangled) | DLL | Mangled export | Why |")
    w("|---|---|---|---|---|---|")
    for i, (area, key, dll_short, mangled, why, n) in enumerate(resolved, 1):
        dup = f" (+{n-1} overloads)" if n > 1 else ""
        w(f"| {i} | {area} | `{key}`{dup} | {dll_short} | `{mangled}` | {why} |")
    if missing:
        w("")
        w("Not resolved (check the demangled key): " + ", ".join(f"`{m}`" for m in missing))
    w("")

    w("## Counting notes")
    w("")
    w(f"- Distinct classes: **{total_base}** ("
      f"Engine {len(per_dll['EngineDll7.dll'][0])}, Common {len(per_dll['CommonDll7.dll'][0])}, "
      f"Game {len(per_dll['GameDll7.dll'][0])}). "
      f"`*.classes.txt` has {counts['EngineDll7.dll']} / {counts['CommonDll7.dll']} / {counts['GameDll7.dll']} lines, "
      "because each class also appears as a `7`-prefixed vtable line (`??_7X@@6B@`) and some as a `0`-prefixed ctor line.")
    w(f"- Demangled exports: {total_exports} (Engine 8192, Common 3174, Game 133). "
      "EngineDll7.dll.exports.txt has 8587 lines, but 395 are trailing blanks. MEMORY.md's \"8587\" is a line count, not an export count.")
    w("- Method names come from function exports only (`(...)` in the demangled text). Constructors, destructors, operators, "
      "vftables and data members are excluded. Overloads are merged by name.")
    w("- \"Notable\" methods are picked by name keywords: Spawn/Update/Create/Damage/Hurt/Kill/Fire/Load/Save/Send/Receive first, "
      "then position/time/health/player/client/server words, then Get/Set/Is/Add.")
    w("- `Replicator` appears in many per-class methods (`GetReplicator`, `CreateInstance`, `WriteType`). Name alone does not show "
      "that it is network replication, so these classes are not filed under Networking.")
    w("")
    w("## Subsystem index")
    w("")
    for sub in SUBSYSTEMS:
        members = [e for e in entries if e["sub"] == sub]
        w(f"- {sub}: {len(members)} classes")
    w("")

    for sub in SUBSYSTEMS:
        members = [e for e in entries if e["sub"] == sub]
        w(f"## {sub}")
        w("")
        if sub == "Other":
            w("Includes console/cvar classes (`cConsole*`), command line, clock/time map, exceptions, property classes (`cKey*Property`), "
              "debug system and class-registry helpers.")
            w("")
        if not members:
            w("_No classes._")
            w("")
            continue
        w("| Class | DLL | Exports | Vtable | Notable methods |")
        w("|---|---|---:|:-:|---|")
        for e in members:
            notable = ", ".join(notable_methods(e["methods"], e["name"]))
            vt = "yes" if e["vtable"] else ""
            w(f"| `{e['name']}` | {SHORT[e['dll']]} | {e['count']} | {vt} | {notable} |")
        w("")

    w("## Referenced types without their own export list")
    w("")
    w("These names appear in demangled signatures or scopes but have no entry in `*.classes.txt`. "
      "Likely types used only as parameters or return values.")
    w("")
    if referenced:
        w(", ".join(f"`{n}` ({c})" for n, c in referenced.most_common(60)))
    else:
        w("_None._")
    w("")

    # Class-like names that appear only in the string dumps (no export entry).
    string_names = collections.Counter()
    sdir = NOTES / "strings"
    if sdir.is_dir():
        for sfile in sorted(sdir.glob("*.txt")):
            for line in sfile.read_text(encoding="ascii", errors="ignore").splitlines():
                for name in re.findall(r"\b(c[A-Z]\w*)::", line):
                    string_names[name] += 1
                for name in re.findall(r"\\(MOD_\w+)\.cpp", line):
                    string_names[name] += 1
    string_only = [(n, c) for n, c in string_names.most_common() if n not in class_names]
    w("## Names seen only in strings (no export entry)")
    w("")
    w("Found as `Class::` text in `re/notes/strings/*.txt` or as a `MOD_*.cpp` source path, with no entry in `*.classes.txt`. "
      "Presence in the binary is confirmed by the string, but the class has no export to hook.")
    w("")
    w(", ".join(f"`{n}` ({c})" for n, c in string_only) if string_only else "_None._")
    w("")

    OUT.write_text("\n".join(lines), encoding="utf-8", newline="\n")

    counts_by_sub = collections.Counter(e["sub"] for e in entries)
    print(f"wrote {OUT}")
    print(f"classes={len(entries)} shortlist={len(resolved)} missing={missing}")
    for sub in SUBSYSTEMS:
        print(f"  {sub}: {counts_by_sub.get(sub, 0)}")


if __name__ == "__main__":
    sys.exit(main())
