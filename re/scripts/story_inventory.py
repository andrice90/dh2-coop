# Inventory story-mode scripts and script natives for re/notes/story-inventory.md
# Read-only on game data. Writes the deliverable markdown.
import os
import re
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(r"D:\scary hunt\dh2coop")
SCRIPTS = ROOT / "re" / "extracted" / "SCRIPTS"
OUT = ROOT / "re" / "notes" / "story-inventory.md"

STORY = (
    ["MAP1"]
    + [f"MAP2{s}" for s in "ABC"]
    + [f"MAP3{s}" for s in "ABC"]
    + ["MAP4A", "MAP4B"]
    + [f"MAP{n}" for n in range(5, 11)]
    + [f"MAP11{s}" for s in "ABC"]
    + [f"MAP12{s}" for s in "ABC"]
)
BONUS = ["MAPARENA", "MAPISLAND", "MAPTSAVO", "MAPMENU"]
HUNT = ["HUNTMAP1", "HUNTMAP2", "HUNTMAP3", "HUNTMAP4"]
MAP_ORDER = STORY + BONUS + HUNT

KEYWORDS = {"if", "while", "for", "switch", "return", "else", "sizeof", "catch", "do"}
RET_TYPES = {"void", "int", "float", "string", "bool", "double", "byte", "dword", "word"}

QTE_RE = re.compile(
    r"qte|struggle|melee|button|fight|closecombat|knife|mash|quicktime", re.I
)
TRANS_RE = re.compile(
    r"LoadMap|NextMap|ChangeLevel|ChangeMap|Restart|Checkpoint|SaveGame|LoadGame|"
    r"Respawn|OnDeath|PlayerDeath|KillPlayer|OpenMenu|LoadLevel|EndGame|GameOver|"
    r"spmap|SetMap|MissionComplete|GoToMenu|ReturnToMenu",
    re.I,
)
EXPORT_RE = re.compile(
    r"Checkpoint|Save|Load|Restart|Respawn|Death|Die|NextMap|Level", re.I
)
MAPTOKEN_RE = re.compile(
    r'"(MAP\d+[A-C]?|HUNTMAP\d+|MAPMENU|MAPARENA|MAPISLAND|MAPTSAVO)"', re.I
)


def rel(p: Path) -> str:
    return p.relative_to(ROOT).as_posix()


def build_index():
    idx = {}
    for dirpath, _, files in os.walk(SCRIPTS):
        for f in files:
            p = Path(dirpath) / f
            key = p.relative_to(SCRIPTS).as_posix().lower()
            idx[key] = p
    return idx


def resolve_spj_line(line, idx):
    s = line.strip()
    if not s or s.startswith("//") or s.startswith("#"):
        return None
    s = s.replace("\\", "/")
    if s.lower().startswith("scripts/"):
        s = s[8:]
    return idx.get(s.lower())


def read_text(p: Path) -> str:
    return p.read_text(encoding="latin1", errors="replace")


def lines_of(p: Path):
    return read_text(p).splitlines()


def mask_line(line):
    """Return (code_with_strings_blanked, hunter_string_count, raw_without_comments)."""
    out = []
    hunters = 0
    i = 0
    n = len(line)
    while i < n:
        if line[i] == '"':
            j = i + 1
            while j < n:
                if line[j] == "\\":
                    j += 2
                    continue
                if line[j] == '"':
                    break
                j += 1
            lit = line[i + 1 : j]
            if lit == "hunter":
                hunters += 1
            end = j + 1 if j < n else j
            out.append(" " * (end - i))
            i = end
            continue
        if line[i] == "/" and i + 1 < n and line[i + 1] == "/":
            break
        out.append(line[i])
        i += 1
    return "".join(out), hunters


def classify(name: str) -> str:
    n = name
    if n in {"Pause", "Wait", "WaitForTask"} or n.startswith("Wait"):
        return "timing/latent"
    if n in {"StartTask", "StopTask", "StopAllTask", "SetTaskParam", "ForceBrainActivate", "MoveToPos"} or re.search(
        r"Brain|Animal|AnimPak", n
    ):
        return "AI tasks and animals"
    if re.search(r"Task|Mission|Objective", n) and n not in {"SetTaskParam"}:
        return "mission/task"
    if re.search(r"Hud|Hint|PlayMessage|OpenMenu|MenuPage", n, re.I):
        return "HUD/hint/text"
    if re.search(r"CutScene|Camera|Movie|Cinematic|ExecuteScript", n, re.I):
        return "cutscene/camera/movie"
    if re.search(r"Sound|Music", n, re.I):
        return "sound/music"
    if re.search(r"Trigger|ActivateBy|ActivateAfter|ActivateHUD|DeactivateHUD", n, re.I) or n in {
        "Activate",
        "Deactivate",
        "ActivateTrigger",
        "DeactivateTrigger",
    }:
        return "triggers/activation"
    if re.search(r"NPC|Companion|Guide|Follow|Talk|SetCanBeAttacked", n, re.I):
        return "NPC/companion"
    if re.search(r"Spawn|Destroy|MakeVisible|Show|Hide|AddAnimal|AddEvent|Precache|AddAI", n, re.I):
        return "spawn/destroy/show/hide"
    if re.search(
        r"PFlags|Health|Weapon|Ammo|Item|Inventory|Freeze|Teleport|Hurt|Damage|Crouch|Equip|LockInput|Knife",
        n,
        re.I,
    ):
        return "player control"
    return "other"


def player_kind(line: str, call: str) -> str:
    c = call or ""
    if "GetDist" in c or "GetDist" in line:
        return "distance check"
    if "ActivateBy" in line and "hunter" in line:
        return "trigger tests player"
    if "FindEntScrObject" in line and "hunter" in line:
        return "bind player entity"
    if re.search(r"SetPFlags|ClearPFlags", c) or re.search(r"SetPFlags|ClearPFlags", line):
        return "player flags"
    if re.search(r"CutScene|BeginCamera|EndCamera", c) or re.search(
        r"ActivateCutSceneMode|DeactivateCutSceneMode|BeginCamera|EndCamera", line
    ):
        return "camera/cutscene"
    if re.search(r"MoveToPos|Teleport|SetPos", c):
        return "teleport/move"
    if re.search(r"LockInput|BrainFreeze|Freeze", c):
        return "freeze"
    if re.search(r"Health|Hurt|Kill|Damage", c):
        return "damage/health"
    if re.search(r"Ammo|Weapon|Item|Equip|Inventory|Fire|Reload", c):
        return "inventory/weapon"
    if "StartTask" in c or "StartTask" in line:
        return "task target"
    if "SetTaskParam" in line:
        return "task target"
    if "SetObjective" in line:
        return "task target"
    if re.search(r"\bplayer\b", line, re.I) and "hunter" not in line:
        return "player method"
    return "hunter string"


def split_commas(s):
    parts = []
    buf = []
    depth = 0
    for ch in s:
        if ch == "(":
            depth += 1
            buf.append(ch)
        elif ch == ")":
            if depth == 0:
                break
            depth -= 1
            buf.append(ch)
        elif ch == "," and depth == 0:
            parts.append("".join(buf).strip())
            buf = []
        else:
            buf.append(ch)
    if buf:
        parts.append("".join(buf).strip())
    return parts


def norm_code(tok):
    tok = tok.strip()
    tok = re.sub(r"\(.*?\)", "", tok).strip()
    if re.fullmatch(r"-?0x[0-9a-fA-F]+", tok) or re.fullmatch(r"-?\d+", tok):
        if tok.lower().startswith("0x"):
            return hex(int(tok, 16))
        return tok
    return tok


def rva_from_label(label):
    val = int(label, 16)
    base = 0x10000000
    if val >= base:
        return val - base
    return None


def parse_registrar_dump(text, binary):
    chunks = re.split(r"\n(?=// ===== )", text)
    rows = []
    funcs = []
    for ch in chunks:
        m = re.search(r"// ===== \S+\s+(\S+)\s+RVA (0x[0-9a-fA-F]+)", ch)
        if not m:
            continue
        func = m.group(1)
        reg_rva = int(m.group(2), 16)
        sig_class = func.split("::")[0] if "::" in func else ""
        parent = ""
        pm = re.search(r"\bs_((?:c|C)[A-Za-z0-9_]+)_", ch)
        if pm:
            parent = pm.group(1)
        # mangled comment
        mm = re.search(r"\?ScrExport@(\w+)@@", ch)
        if mm and not sig_class:
            sig_class = mm.group(1)
        funcs.append((func, reg_rva, sig_class, parent, binary))
        # walk ExportFnc( ... ); then the next s_Export*Function
        for mcall in re.finditer(r"ExportFnc\s*\(", ch):
            k = mcall.end() - 1
            depth = 0
            p = k
            while p < len(ch):
                if ch[p] == "(":
                    depth += 1
                elif ch[p] == ")":
                    depth -= 1
                    if depth == 0:
                        p += 1
                        break
                p += 1
            inside = ch[k + 1 : p - 1]
            parts = split_commas(inside)
            name = parts[0].strip().strip('"') if parts else ""
            codes = [norm_code(x) for x in parts[1:]]
            rest = ch[p : p + 500]
            em = re.search(
                r"s_Export(?:Static)?Function\s*\((.*?)\)\s*;",
                rest,
                re.S,
            )
            impl = ""
            impl_rva = None
            kind = ""
            if em:
                blob = em.group(0)
                if "s_ExportStaticFunction" in blob:
                    kind = "static"
                else:
                    kind = "method"
                lm = re.search(r"(?:FUN_|LAB_)([0-9A-Fa-f]+)", blob)
                if lm:
                    impl = lm.group(0)
                    impl_rva = rva_from_label(lm.group(1))
                else:
                    am = re.search(r",\s*([A-Za-z_][A-Za-z0-9_]*)\s*,", blob)
                    if am:
                        impl = am.group(1)
            rows.append(
                {
                    "binary": binary,
                    "name": name,
                    "func": func,
                    "reg_rva": reg_rva,
                    "sig_class": sig_class,
                    "parent": parent,
                    "codes": codes,
                    "impl": impl,
                    "kind": kind,
                    "impl_rva": impl_rva,
                }
            )
    return rows, funcs


def class_body(text, class_name):
    m = re.search(r"\bclass\s+" + re.escape(class_name) + r"\b[^{]*\{", text)
    if not m:
        return ""
    i = m.end() - 1
    depth = 0
    for p in range(i, len(text)):
        if text[p] == "{":
            depth += 1
        elif text[p] == "}":
            depth -= 1
            if depth == 0:
                return text[i : p + 1]
    return text[i:]


def instances_in(body):
    counts = Counter()
    # only non-comment lines
    for line in body.splitlines():
        code, _ = mask_line(line)
        m = re.search(r"\b(c[A-Za-z0-9_]+)\s+[A-Za-z_][A-Za-z0-9_]*\s*<\s*instance\s*>", code)
        if m:
            counts[m.group(1)] += 1
    return counts


def defined_names(all_files):
    names = set()
    def_re = re.compile(
        r"\b(?:" + "|".join(RET_TYPES) + r"|c[A-Za-z0-9_]+)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\("
    )
    for p in all_files:
        for line in lines_of(p):
            code, _ = mask_line(line)
            for m in def_re.finditer(code):
                names.add(m.group(1))
    return names


CALL_RE = re.compile(r"(?<![\w.])([A-Za-z_][A-Za-z0-9_]*)\s*\(")
METH_RE = re.compile(r"\.([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def calls_in_line(code):
    found = []
    for m in CALL_RE.finditer(code):
        name = m.group(1)
        if name not in KEYWORDS and name != "new":
            found.append(name)
    for m in METH_RE.finditer(code):
        found.append(m.group(1))
    return found


def snippet(line, limit=180):
    s = " ".join(line.strip().split())
    if len(s) > limit:
        return s[: limit - 3] + "..."
    return s


def esc(s):
    return s.replace("|", "\\|")


def main():
    idx = build_index()
    all_script_files = [p for p in idx.values() if p.suffix.lower() in {".scr", ".spl", ".lua", ".spj"}]
    defs = defined_names([p for p in all_script_files if p.suffix.lower() != ".spj"])

    map_files = {}
    map_meta = {}
    missing = []
    for mname in MAP_ORDER:
        spj = idx.get(f"levels/{mname.lower()}/main.spj")
        files = []
        raw_lines = []
        if spj is None:
            missing.append(f"LEVELS/{mname}/MAIN.SPJ")
        else:
            raw_lines = lines_of(spj)
            for ln in raw_lines:
                hit = resolve_spj_line(ln, idx)
                if ln.strip() and not ln.strip().startswith("//") and hit is None and "/" in ln:
                    missing.append(f"{mname}: {ln.strip()}")
                if hit is not None:
                    files.append(hit)
        # unique preserve order
        seen = set()
        uniq = []
        for f in files:
            if f not in seen:
                seen.add(f)
                uniq.append(f)
        map_files[mname] = uniq
        missions, triggers, cuts, models, worlds, other = [], [], [], [], [], []
        for f in uniq:
            rels = f.relative_to(SCRIPTS).as_posix().lower()
            base = f.name
            if "/missions/" in "/" + rels or rels.startswith("missions/"):
                missions.append(base)
            elif rels.startswith("triggers/") or "/triggers/" in rels:
                triggers.append(base)
            elif "cutscene/" in rels:
                cuts.append(base)
            elif rels.startswith("models/") or "/models/" in rels:
                models.append(base)
            elif f.name.lower() == "world.scr" and f.parent.name.lower() == mname.lower():
                worlds.append(f)
            else:
                other.append(base)
        world = worlds[0] if worlds else idx.get(f"levels/{mname.lower()}/world.scr")
        inst = Counter()
        gclass = ""
        wlines = t_mission = t_trigger = 0
        if world and world.exists():
            wtext = read_text(world)
            wlines = len(wtext.splitlines())
            gm = re.search(r"\bnew\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*globals\s*\)", wtext)
            if gm:
                gclass = gm.group(1)
                inst = instances_in(class_body(wtext, gclass))
            else:
                inst = instances_in(wtext)
                classes = re.findall(r"\bclass\s+([A-Za-z_][A-Za-z0-9_]*)", wtext)
                gclass = "no globals constructor; instances are on " + ", ".join(classes)
        mission_lines = {}
        for base in missions:
            p = next((f for f in uniq if f.name == base and "mission" in f.as_posix().lower()), None)
            if p:
                mission_lines[base] = len(lines_of(p))
                t_mission += mission_lines[base]
        trigger_lines = {}
        for base in triggers:
            p = next((f for f in uniq if f.name == base and "trigger" in f.as_posix().lower()), None)
            if p:
                trigger_lines[base] = len(lines_of(p))
                t_trigger += trigger_lines[base]
        map_meta[mname] = {
            "missions": missions,
            "triggers": triggers,
            "cuts": cuts,
            "models": models,
            "other": other,
            "world": world,
            "wlines": wlines,
            "mission_lines": mission_lines,
            "trigger_lines": trigger_lines,
            "inst": inst,
            "gclass": gclass,
            "nfiles": len(uniq),
        }

    # usage
    # name -> map -> count, and sites
    use_map = defaultdict(Counter)  # name -> map -> count
    first = {}  # name -> (map, path, line, text)
    file_maps = defaultdict(set)
    for mname, files in map_files.items():
        for f in files:
            file_maps[f].add(mname)
    story_bonus = set(STORY + BONUS)
    catalog_names = set()

    def consider_file(mname, f):
        try:
            lns = lines_of(f)
        except OSError:
            return
        for i, line in enumerate(lns, 1):
            code, _ = mask_line(line)
            if not code.strip():
                continue
            for name in calls_in_line(code):
                if name in defs:
                    continue
                use_map[name][mname] += 1
                if name not in first:
                    first[name] = (mname, f, i, snippet(line))

    for mname in MAP_ORDER:
        for f in map_files[mname]:
            consider_file(mname, f)

    # natives
    gtext = (ROOT / "re" / "notes" / "story-natives-gamedll.c").read_text(encoding="latin1", errors="replace")
    ctext = (ROOT / "re" / "notes" / "story-natives-commondll.c").read_text(encoding="latin1", errors="replace")
    ltext = (ROOT / "re" / "notes" / "lead-natives.c").read_text(encoding="latin1", errors="replace")
    grows, gfuncs = parse_registrar_dump(gtext, "GameDll7")
    crows, cfuncs = parse_registrar_dump(ctext, "CommonDll7")
    lrows, _lfuncs = parse_registrar_dump(ltext, "lead-natives.c")
    all_rows = grows + crows
    for r in all_rows + lrows:
        catalog_names.add(r["name"])

    # player refs: unique files in story+bonus closures
    player_rows = []
    seen_pl = set()
    sb_files = []
    for mname in STORY + BONUS:
        for f in map_files[mname]:
            if f not in seen_pl:
                seen_pl.add(f)
                sb_files.append(f)
    kind_counts = Counter()
    for f in sb_files:
        lns = lines_of(f)
        is_trigger = "trigger" in f.as_posix().lower()
        for i, line in enumerate(lns, 1):
            code, hunters = mask_line(line)
            if hunters == 0 and not re.search(r"(?<![\w])player\s*\.", code) and "::globals.player" not in code.lower() and "globals.player" not in code.lower():
                # also player. after blanking? player. remains in code
                if not re.search(r"(?<![\w.])player\s*\.", code):
                    continue
            if not code.strip() and hunters == 0:
                continue
            # recompute hunters only in code (comments already stripped by mask)
            show = hunters > 0 or re.search(r"(?<![\w.])player\s*\.", code) or "globals.player" in code.lower()
            if not show:
                continue
            # call names on the line
            names = calls_in_line(code)
            call = ", ".join(names) if names else ("hunter" if hunters else "player")
            kind = player_kind(line, " ".join(names))
            if is_trigger and kind == "hunter string":
                kind = "trigger tests player"
            extra = ""
            if "GetDist" in " ".join(names):
                for j in range(i, min(i + 4, len(lns))):
                    if j == i:
                        continue
                    nxt = lns[j - 1].strip()
                    if nxt.startswith("if") or nxt.startswith("if("):
                        extra = " next if: " + snippet(nxt, 120)
                        break
            maps = sorted(file_maps.get(f, []), key=lambda x: MAP_ORDER.index(x) if x in MAP_ORDER else 99)
            map_s = ",".join(maps)
            if len(maps) > 6:
                map_s = f"shared by {len(maps)} maps ({maps[0]}..{maps[-1]})"
            player_rows.append((rel(f), i, snippet(line), call, kind + extra, map_s))
            kind_counts[kind] += 1

    # cutscenes
    cut_files = []
    for dirpath, _, files in os.walk(SCRIPTS / "CUTSCENE"):
        for fn in files:
            if fn.lower().endswith(".scr"):
                cut_files.append(Path(dirpath) / fn)
    cut_files.sort()
    cut_call_counts = Counter()
    cut_player = Counter()
    cut_player_ex = {}
    skip_files = []
    exec_sites = []
    for f in cut_files:
        text = read_text(f)
        if re.search(r"\bstate\s+Skip\b", text):
            skip_files.append(f)
        for i, line in enumerate(text.splitlines(), 1):
            code, _ = mask_line(line)
            for name in calls_in_line(code):
                cut_call_counts[name] += 1
            if re.search(r"player\s*\.", code) or "globals.player" in code.lower():
                for name in re.findall(r"player\s*\.\s*([A-Za-z_][A-Za-z0-9_]*)", code):
                    cut_player[name] += 1
                    cut_player_ex.setdefault(name, (f, i, snippet(line)))
    # who calls ExecuteScript cutscene
    for mname in STORY + BONUS:
        for f in map_files[mname]:
            if "cutscene" in f.as_posix().lower():
                continue
            for i, line in enumerate(lines_of(f), 1):
                if re.search(r"ExecuteScript\s*\(", line) and not line.strip().startswith("//"):
                    code, _ = mask_line(line)
                    if "ExecuteScript" in code:
                        exec_sites.append((mname, f, i, snippet(line)))

    # QTE across all scripts
    qte_hits = []
    for p in all_script_files:
        if p.suffix.lower() == ".spj":
            continue
        for i, line in enumerate(lines_of(p), 1):
            mm = QTE_RE.search(line)
            if not mm:
                continue
            commented = line.strip().startswith("//")
            code, _ = mask_line(line)
            in_string = (not commented) and (QTE_RE.search(code) is None)
            where = "comment" if commented else ("string" if in_string else "code")
            qte_hits.append((rel(p), i, where, mm.group(0), snippet(line, 160)))

    # transitions
    trans_hits = []
    seen_t = set()
    for mname in STORY + BONUS + HUNT:
        for f in map_files[mname]:
            if f in seen_t:
                continue
            seen_t.add(f)
            for i, line in enumerate(lines_of(f), 1):
                code, _ = mask_line(line)
                if TRANS_RE.search(code) or MAPTOKEN_RE.search(code):
                    trans_hits.append((rel(f), i, snippet(line, 160)))

    # exports
    export_hits = []
    for exp in (ROOT / "re" / "exports").glob("*.demangled.txt"):
        for line in lines_of(exp):
            # demangled is after tab
            if "\t" in line:
                name = line.split("\t", 1)[1]
            else:
                name = line
            if EXPORT_RE.search(name):
                export_hits.append((exp.name, name.strip()))

    # mission blocks
    mission_info = {}
    for mname in STORY + BONUS:
        meta = map_meta[mname]
        blocks = []
        end_lines = []
        maptokens = []
        for f in map_files[mname]:
            if "mission" in f.as_posix().lower() and f.parent.name.lower() == "missions":
                lns = lines_of(f)
                # CreateTask blocks
                buf = []
                inb = False
                for i, line in enumerate(lns, 1):
                    code, _ = mask_line(line)
                    if "CreateTask" in code:
                        inb = True
                        buf = [(i, snippet(line, 140))]
                        continue
                    if inb:
                        if code.strip():
                            buf.append((i, snippet(line, 140)))
                        if "EndCreateTask" in code:
                            blocks.append(buf)
                            inb = False
                    if re.search(r"SetMission\s*\(", code):
                        blocks.append([(i, snippet(line, 140))])  # will filter later
                # cleaner: separate setmission
            for i, line in enumerate(lines_of(f), 1):
                code, _ = mask_line(line)
                if re.search(
                    r"CompleteTask|UncompleteTask|UpdateMissionStatus|GlobalsCheckMissionStatus|ActivateTriggerOnEnd|ActivateTriggerOnFail|SetCompleteCondition|OnFail|FailMission|MissionFail",
                    code,
                ):
                    end_lines.append((rel(f), i, snippet(line, 160)))
                mm = MAPTOKEN_RE.search(code)
                if mm and f.parent.name.lower() == mname.lower():
                    maptokens.append((rel(f), i, mm.group(1), snippet(line, 120)))
        mission_info[mname] = {"blocks": blocks, "ends": end_lines, "tokens": maptokens}

    # The CreateTask parser above also appended SetMission into blocks mixed.
    # Re-parse missions cleanly.
    mission_info = {}
    for mname in STORY + BONUS:
        tasks = []
        setmission = []
        ends = []
        tokens = []
        seen_end = set()
        for f in map_files[mname]:
            lns = lines_of(f)
            rels = f.relative_to(SCRIPTS).as_posix().lower()
            if rels.startswith("missions/"):
                inb = False
                buf = []
                for i, line in enumerate(lns, 1):
                    code, _ = mask_line(line)
                    if re.search(r"SetMission\s*\(", code):
                        setmission.append((rel(f), i, snippet(line)))
                    if re.search(r"\.CreateTask\s*\(", code):
                        inb = True
                        buf = [(i, snippet(line, 150))]
                        continue
                    if inb:
                        if code.strip() and not code.strip().startswith("//"):
                            buf.append((i, snippet(line, 150)))
                        if "EndCreateTask" in code:
                            tasks.append(buf)
                            inb = False
                            buf = []
            for i, line in enumerate(lns, 1):
                code, _ = mask_line(line)
                if not code.strip():
                    continue
                if re.search(
                    r"CompleteTask|UncompleteTask|UpdateMissionStatus|GlobalsCheckMissionStatus|ActivateTriggerOnEnd|ActivateTriggerOnFail|SetCompleteCondition",
                    code,
                ):
                    key = (str(f), i)
                    if key not in seen_end:
                        seen_end.add(key)
                        ends.append((rel(f), i, snippet(line, 170)))
                if f.parent.name.lower() == mname.lower() or "cutscene" in rels:
                    mm = MAPTOKEN_RE.search(line if False else code)
                    if mm:
                        tokens.append((rel(f), i, mm.group(1)))
        mission_info[mname] = {"tasks": tasks, "setmission": setmission, "ends": ends, "tokens": tokens}

    # NPCs
    npc_classes = re.compile(r"cScrNPC|cScrCustomCharacter|cScrHuman|Guide", re.I)
    npc_info = {}
    for mname in STORY + BONUS:
        meta = map_meta[mname]
        npcs = []
        for cls, n in meta["inst"].items():
            if npc_classes.search(cls):
                npcs.append((cls, n))
        # instance names from world
        names = []
        world = meta["world"]
        if world and world.exists():
            body = ""
            wtext = read_text(world)
            if meta["gclass"] and not meta["gclass"].startswith("UNVERIFIED"):
                body = class_body(wtext, meta["gclass"])
            else:
                body = wtext
            for line in body.splitlines():
                code, _ = mask_line(line)
                m = re.search(
                    r"\b(cScrNPC|cScrCustomCharacterEx|cScrCustomCharacter|cScrHuman\w*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*<\s*instance",
                    code,
                )
                if m:
                    names.append((m.group(1), m.group(2)))
        quotes = []
        name_set = {n for _, n in names}
        if name_set:
            seenq = set()
            for f in map_files[mname]:
                if f in seenq:
                    continue
                seenq.add(f)
                for i, line in enumerate(lines_of(f), 1):
                    code, _ = mask_line(line)
                    if not any(re.search(r"\b" + re.escape(n) + r"\b", code) for n in name_set):
                        continue
                    if re.search(r"StartTask|SetTaskParam|PlayMessage|PlaySound|Follow|MoveToPos|LookTo", code):
                        quotes.append((rel(f), i, snippet(line, 160)))
        npc_info[mname] = {"npcs": npcs, "names": names, "quotes": quotes}

    # ---- write markdown ----
    L = []
    w = L.append
    w("# Story-mode script inventory")
    w("")
    w("## Progress")
    w("Done: maps, native catalog (GameDll + CommonDll ExportFnc dumps), usage, player refs, cutscenes/QTE, mission flow, transitions, NPCs, summary. Game was not launched.")
    w("")
    w("Scope: story maps " + ", ".join(STORY) + ". Bonus: " + ", ".join(BONUS) + ". Hunt maps appear only in native-usage counts (and in the transition search). Script paths are under `re/extracted/SCRIPTS`. Line numbers are 1-based in the file on disk. A call is counted only when it sits outside a `//` comment and outside a string. An identifier is omitted from section 3 when a script file defines a function of that name.")
    w("")
    w("## 1. Maps")
    w("")
    w("Globals class is the class constructed with `new <Class>(globals)` in the map `WORLD.SCR`. `<instance>` counts are members of that class only.")
    w("")
    w("| Map | Mission script (lines) | Trigger script (lines) | Cutscene scripts | Model scripts | Globals class | `<instance>` counts | WORLD.SCR lines |")
    w("|---|---|---|---:|---:|---|---|---:|")
    for mname in STORY + BONUS:
        meta = map_meta[mname]
        mis = ", ".join(f"{b} ({meta['mission_lines'].get(b, '?')})" for b in meta["missions"]) or "—"
        trig = ", ".join(f"{b} ({meta['trigger_lines'].get(b, '?')})" for b in meta["triggers"]) or "—"
        inst = ", ".join(f"{c}×{n}" for c, n in sorted(meta["inst"].items())) or "—"
        w(f"| {mname} | {esc(mis)} | {esc(trig)} | {len(meta['cuts'])} | {len(meta['models'])} | {esc(meta['gclass'] or '—')} | {esc(inst)} | {meta['wlines']} |")
    w("")
    w("Model and cutscene file names, and other MAIN.SPJ entries that are not mission/trigger/model/cutscene/this map's WORLD.SCR:")
    w("")
    for mname in STORY + BONUS:
        meta = map_meta[mname]
        w(f"- **{mname}** cutscenes: {', '.join(meta['cuts']) or '—'}")
        w(f"  - models ({len(meta['models'])}): {', '.join(meta['models']) or '—'}")
        if meta["other"]:
            w(f"  - other: {', '.join(meta['other'])}")
    if missing:
        w("")
        w("MAIN.SPJ lines that did not resolve to a file:")
        for item in missing:
            w(f"- `{item}`")
    w("")

    # section 2
    w("## 2. Native catalog")
    w("")
    w("### 2a. Global natives (`re/notes/lead-natives.c`, `FUN_1000ba30`, RVA `0xba30`)")
    w("")
    w("Each row is one `cExportKey::ExportFnc` in that function. Codes are the numeric arguments after the name, as written (second slot is `0` or `1`, then return type, `-1`, argument types, `0xffffffff` terminator). Implementation RVA = decompiler address − `0x10000000`.")
    w("")
    w("| Name | Impl RVA | Impl symbol | Codes |")
    w("|---|---|---|---|")
    globals_rows = [r for r in lrows if r["func"] == "FUN_1000ba30"] or lrows
    dump_globals = [r for r in grows if r["func"] == "FUN_1000ba30"]
    for r in globals_rows:
        impl_r = f"`0x{r['impl_rva']:x}`" if r["impl_rva"] is not None else "—"
        w(f"| `{r['name']}` | {impl_r} | `{r['impl'] or '—'}` | `{', '.join(r['codes'])}` |")
    w("")
    same = [r["name"] for r in globals_rows] == [r["name"] for r in dump_globals]
    w(f"Count: {len(globals_rows)} `ExportFnc` entries in `re/notes/lead-natives.c` `FUN_1000ba30`. The same function in `re/notes/story-natives-gamedll.c` has {len(dump_globals)} entries; name lists match: {same}.")
    w("")
    w("### 2b. Class-method dumps")
    w("")
    w("Commands run one after another on slot W (no game launch):")
    w("")
    w(r"`.\re\scripts\ghrun.ps1 -Slot W -Binary GameDll7.dll -Script CallersDecomp.java -ScriptArgs 're\notes\story-natives-gamedll.c','ExportFnc'`")
    w("")
    w(f"Result: {len(gfuncs)} callers, written to `re/notes/story-natives-gamedll.c`.")
    w("")
    w(r"`.\re\scripts\ghrun.ps1 -Slot W -Binary CommonDll7.dll -Script CallersDecomp.java -ScriptArgs 're\notes\story-natives-commondll.c','ExportFnc'`")
    w("")
    w(f"Result: {len(cfuncs)} callers, written to `re/notes/story-natives-commondll.c`.")
    w("")
    w("Registrar list (signature class when Ghidra named the function; otherwise the function is `FUN_`/`LAB_`). `parent string` is the `s_<Name>_` symbol copied into the export block. On CommonDll, named `ScrExport` functions store the **parent** class string, not the signature class (example: `cScrHuman::ScrExport` copies `cScrCharacter`, `AllocMem(0xe)` = 14 bytes = `cScrCharacter` plus NUL). For unnamed GameDll registrars the owning class is the parent string only when that is all the dump shows; treat a subclass relationship as UNVERIFIED.")
    w("")
    w("| Binary | Registrar | RVA | Signature class | Parent string in export block | ExportFnc count | static/method |")
    w("|---|---|---|---|---|---:|---|")
    for binary, funcs, rows in (("GameDll7", gfuncs, grows), ("CommonDll7", cfuncs, crows)):
        for func, reg_rva, sig, parent, _b in funcs:
            sub = [r for r in rows if r["func"] == func]
            kinds = ",".join(sorted({r["kind"] or "?" for r in sub}))
            w(
                f"| {binary} | `{func}` | `0x{reg_rva:x}` | `{sig or '—'}` | `{parent or '—'}` | {len(sub)} | {kinds} |"
            )
    w("")
    w("### 2c. Method and extra-registrar natives")
    w("")
    w("`FUN_1000ba30` rows are in 2a and are not repeated here. `kind` is `static` when the next call is `s_ExportStaticFunction`, `method` when it is `s_ExportFunction`.")
    w("")
    w("| Name | Binary | Registrar RVA | Signature class | Parent string | Kind | Impl RVA | Impl symbol | Codes |")
    w("|---|---|---|---|---|---|---|---|---|")
    for r in all_rows:
        if r["func"] == "FUN_1000ba30":
            continue
        impl_r = f"`0x{r['impl_rva']:x}`" if r["impl_rva"] is not None else "—"
        w(
            f"| `{r['name']}` | {r['binary']} | `0x{r['reg_rva']:x}` | `{r['sig_class'] or '—'}` | `{r['parent'] or '—'}` | {r['kind'] or '—'} | {impl_r} | `{r['impl'] or '—'}` | `{', '.join(r['codes'])}` |"
        )
    w("")
    w(f"Rows in 2c: {sum(1 for r in all_rows if r['func'] != 'FUN_1000ba30')}. Distinct ExportFnc names in both dumps: {len(catalog_names)}.")
    w("")

    # section 3
    w("## 3. Native usage in story scripts")
    w("")
    w("Counts are call sites in each map's `MAIN.SPJ` closure (a shared script is counted on every map that lists it). The numeric column is that sum, not a unique file:line count. `in catalog` is yes when section 2 has that name. Category is assigned from the identifier, in this order: timing (`Pause`/`Wait`/`WaitForTask`/prefix `Wait`); AI (`StartTask`, `StopTask`, `StopAllTask`, `SetTaskParam`, `ForceBrainActivate`, `MoveToPos`, or name contains `Brain`/`Animal`/`AnimPak`); mission (`Task`/`Mission`/`Objective`); HUD (`Hud`/`Hint`/`PlayMessage`/`OpenMenu`/`MenuPage`); cutscene (`CutScene`/`Camera`/`Movie`/`Cinematic`/`ExecuteScript`); sound (`Sound`/`Music`); triggers (`Trigger`, `Activate`, `Deactivate`, `ActivateBy`, `ActivateAfter`, HUD activate); NPC (`NPC`/`Companion`/`Guide`/`Follow`/`Talk`/`SetCanBeAttacked`); spawn (`Spawn`/`Destroy`/`MakeVisible`/`Show`/`Hide`/`AddAnimal`/`AddEvent`/`Precache`/`AddAI`); player control (`PFlags`/`Health`/`Weapon`/`Ammo`/`Item`/`Inventory`/`Freeze`/`Teleport`/`Hurt`/`Damage`/`Crouch`/`Equip`/`LockInput`/`Knife`); else other.")
    w("")
    # group
    grouped = defaultdict(list)
    for name in use_map:
        grouped[classify(name)].append(name)
    cat_order = [
        "mission/task",
        "HUD/hint/text",
        "cutscene/camera/movie",
        "player control",
        "AI tasks and animals",
        "NPC/companion",
        "spawn/destroy/show/hide",
        "triggers/activation",
        "sound/music",
        "timing/latent",
        "other",
    ]
    for cat in cat_order:
        names = sorted(grouped.get(cat, []), key=lambda n: (-sum(use_map[n].values()), n))
        w(f"### {cat} ({len(names)})")
        w("")
        if not names:
            w("None.")
            w("")
            continue
        w("| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |")
        w("|---|---|---:|---|---|")
        for name in names:
            per = use_map[name]
            # unique sites approximated by max? No: shared files counted per map.
            # Recompute sites as sum over files once: we didn't store per file.
            # Store sites as sum of counts on maps, but note shared inflation in column sum.
            # We need unique sites. Reconstruct: not stored.
            # I'll show sum and the per-map list. Add a note that sum multiplies shared files.
            total_sum = sum(per.values())
            parts = [f"{m}:{per[m]}" for m in MAP_ORDER if per[m]]
            per_s = " ".join(parts)
            fm = first.get(name)
            ex = "—"
            if fm:
                ex = f"`{rel(fm[1])}:{fm[2]}` `{esc(fm[3])}`"
            incat = "yes" if name in catalog_names else "no"
            w(f"| `{name}` | {incat} | {total_sum} | {per_s} | {ex} |")
        w("")
    w("A line in a shared script (for example `MAPWORLDBASE.SCR`) is counted once per map that includes it, so the sum is larger than the number of source lines.")
    w("")
    overlap = sorted(n for n in use_map if n in defs)
    w(f"Identifiers that are both script-defined and called are excluded above. Script-defined names collected: {len(defs)}. None of the excluded names are listed here by construction.")
    w("")
    also_native = sorted(n for n in defs if n in catalog_names)
    w(f"Script-defined names that are also ExportFnc names (calls excluded from the table because the scripts define them): {', '.join('`'+n+'`' for n in also_native) or '—'}.")
    w("")

    # section 4
    w("## 4. Player references")
    w("")
    w("Every non-comment line in a story or bonus `MAIN.SPJ` closure that contains the string `\"hunter\"`, a `player.` / `::globals.player` call. Hunt-map scripts are not in this table (`re/notes/lead-mission.md` already covers their `\"hunter\"` uses). Kind is taken from the call on the same line. For `GetDist*`, the following `if` is quoted when it is within the next three lines.")
    w("")
    w("| Kind | Count |")
    w("|---|---:|")
    for k, n in kind_counts.most_common():
        w(f"| {k} | {n} |")
    w(f"| **total lines** | {len(player_rows)} |")
    w("")
    w("| File:line | Call | What it decides | Maps that include this file |")
    w("|---|---|---|---|")
    for path, i, text, call, kind, map_s in player_rows:
        w(f"| `{path}:{i}` | `{esc(call)}` | {esc(kind)} — `{esc(text)}` | {esc(map_s)} |")
    w("")

    # section 5
    w("## 5. Cutscenes and QTE/melee")
    w("")
    w(f"Cutscene `.scr` files on disk under `re/extracted/SCRIPTS/CUTSCENE`: {len(cut_files)}. Files whose text contains `state Skip`: {len(skip_files)}.")
    w("")
    w("Calls inside those cutscene files (every identifier call, including ones scripts also define), top of the list by count, plus every player-method call:")
    w("")
    w("| Call in CUTSCENE\\* | Count |")
    w("|---|---:|")
    for name, n in cut_call_counts.most_common(40):
        w(f"| `{name}` | {n} |")
    w("")
    w("Player methods called as `player.<method>` inside cutscene files:")
    w("")
    w("| Method | Count | Example |")
    w("|---|---:|---|")
    for name, n in cut_player.most_common():
        ex = cut_player_ex.get(name)
        exs = f"`{rel(ex[0])}:{ex[1]}` `{esc(ex[2])}`" if ex else "—"
        w(f"| `{name}` | {n} | {exs} |")
    w("")
    w("How a cutscene is entered from outside `CUTSCENE` (calls to `ExecuteScript` in story/bonus closures):")
    w("")
    if not exec_sites:
        w("No `ExecuteScript` call in story/bonus files outside `CUTSCENE`.")
    else:
        w("| Map | File:line | Call |")
        w("|---|---|---|")
        # unique by file line
        seen_e = set()
        for mname, f, i, text in exec_sites:
            key = (str(f), i)
            if key in seen_e:
                continue
            seen_e.add(key)
            w(f"| {mname} | `{rel(f)}:{i}` | `{esc(text)}` |")
    w("")
    w("Start/end pattern counted across cutscene files (a file counts if the call text appears outside comments):")
    w("")
    markers = [
        "ExecuteScript",
        "BeginCamera",
        "EndCamera",
        "ActivateCutSceneMode",
        "DeactivateCutSceneMode",
        "PLAYER_INCUTSCENE",
        "SetPFlags",
        "ClearPFlags",
        "ActivateHUD",
        "DeactivateHUD",
        "m_bSkip",
    ]
    w("| Marker | Cutscene files containing it |")
    w("|---|---:|")
    for mk in markers:
        c = 0
        for f in cut_files:
            hit = False
            for line in lines_of(f):
                code, _ = mask_line(line)
                if mk in code:
                    hit = True
                    break
            if hit:
                c += 1
        w(f"| `{mk}` | {c} |")
    w("")
    w("Quoted start (`re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR`): `player.SetPFlags(PLAYER_INCUTSCENE)`, `player.ActivateCutSceneMode()`, `c1cam1.BeginCamera()`, `trigger.ActivateHUD()` in `state Activate` / `Begin`. Quoted end in `End`: `trigger.DeactivateHUD()`, `player.ClearPFlags(PLAYER_INCUTSCENE)`, `player.DeactivateCutSceneMode()`, and `c1cam1.EndCamera()` in `Update` before `setstate(Inactive)`. The `Skip` state calls `EndCamera` and `StopAllTask`, then the same flag clear in `End`.")
    w("")
    w("### QTE / struggle / melee / button / fight / knife")
    w("")
    w("Case-insensitive search of every `.scr`/`.spl`/`.lua` under `re/extracted/SCRIPTS` for `QTE`, `Struggle`, `Melee`, `Button`, `Fight`, `CloseCombat`, `Knife`, `Mash`, `QuickTime`. `where` is `comment`, `string` (the match sits inside quotes), or `code`.")
    w("")
    w(f"Hits: {len(qte_hits)}. In code: {sum(1 for h in qte_hits if h[2]=='code')}. In strings: {sum(1 for h in qte_hits if h[2]=='string')}. In comments: {sum(1 for h in qte_hits if h[2]=='comment')}.")
    w("")
    if not qte_hits:
        w("No matches in the extracted scripts.")
    else:
        w("| File:line | Where | Match | Line |")
        w("|---|---|---|---|")
        for path, i, where, match, text in qte_hits:
            w(f"| `{path}:{i}` | {where} | `{match}` | `{esc(text)}` |")
    w("")
    knife_natives = [r for r in all_rows if re.search(r"knife|melee|qte|struggle|fight|button", r["name"], re.I)]
    w("ExportFnc names matching the same words (native exists even if scripts do not call it):")
    w("")
    if not knife_natives:
        w("None.")
    else:
        for r in knife_natives:
            impl_r = f"0x{r['impl_rva']:x}" if r["impl_rva"] is not None else "—"
            w(f"- `{r['name']}` registrar `{r['func']}` `0x{r['reg_rva']:x}` impl `{impl_r}` parent `{r['parent']}` sig `{r['sig_class'] or '—'}`")
    w("")

    # section 6
    w("## 6. Mission/task flow per map")
    w("")
    w("Tasks are the `CreateTask` … `EndCreateTask` blocks in `MISSIONS\\*.SCR` (calls on `m_pMission` kept; blank and comment lines dropped). Completion and failure calls are every non-comment `CompleteTask`, `UncompleteTask`, `UpdateMissionStatus`, `GlobalsCheckMissionStatus`, `ActivateTriggerOnEnd`, `ActivateTriggerOnFail`, `SetCompleteCondition` in that map's `MAIN.SPJ` closure.")
    w("")
    for mname in STORY + BONUS:
        info = mission_info[mname]
        w(f"### {mname}")
        w("")
        if info["setmission"]:
            for path, i, text in info["setmission"]:
                w(f"- SetMission `{path}:{i}` `{esc(text)}`")
        else:
            w("- No `SetMission` call in the mission script.")
        w(f"- Task blocks: {len(info['tasks'])}")
        for bi, block in enumerate(info["tasks"], 1):
            body = " | ".join(f"{ln}:{esc(tx)}" for ln, tx in block)
            src = block[0][0]
            w(f"  - task {bi} (starts line {src}): {body}")
        if info["tokens"]:
            tok = ", ".join(f"`{p}:{i}` `{t}`" for p, i, t in info["tokens"])
            w(f"- Quoted map-name strings in this map's own WORLD/CUTSCENE files: {tok}")
        w("- Complete / fail / status calls:")
        if not info["ends"]:
            w("  - none in this map's script closure")
        else:
            for path, i, text in info["ends"]:
                w(f"  - `{path}:{i}` `{esc(text)}`")
        w("")

    # section 7
    w("## 7. Map transitions, checkpoints, saves, death")
    w("")
    w("Searched every file in story, bonus, and hunt `MAIN.SPJ` closures (each file once). A call that loads another map, restarts the mission, saves, loads a checkpoint, or handles player death is quoted below. Keyword collisions are listed separately so they are not read as those calls.")
    w("")

    def trans_bucket(text):
        if re.search(r"CanBeRespawned|StampedeRespawn", text):
            return "animal CanBeRespawned / StampedeRespawn (not a player respawn)"
        if re.search(r"OpenMenuPage\s*\(", text):
            return "OpenMenuPage (page index)"
        if re.search(r"\bOpenMenu\s*\(", text):
            return "OpenMenu("
        if re.search(r"\bRestartMission\s*\(", text):
            return "RestartMission("
        if re.search(r"\bRestart\s*\(", text):
            return "Restart("
        if re.search(r"SaveGame\s*\(|LoadGame\s*\(|Checkpoint", text, re.I):
            return "save/load/checkpoint"
        if re.search(r"\bSetMap(LensFlare)?\s*\(", text):
            return "SetMap lens-flare rect (not a level load)"
        if re.search(r"LoadMap|NextMap|ChangeLevel|ChangeMap|LoadLevel|spmap|GoToMenu|ReturnToMenu|MissionComplete|EndGame|GameOver", text, re.I):
            return "map-load or end name"
        if re.search(r"OnDeath|PlayerDeath|KillPlayer", text, re.I):
            return "player death name"
        if MAPTOKEN_RE.search(text):
            return "quoted map token"
        return "other keyword"

    buckets = defaultdict(list)
    for path, i, text in trans_hits:
        buckets[trans_bucket(text)].append((path, i, text))
    wanted = [
        "Restart(",
        "RestartMission(",
        "save/load/checkpoint",
        "map-load or end name",
        "player death name",
        "OpenMenu(",
        "quoted map token",
    ]
    for key in wanted:
        rows = buckets.get(key, [])
        w(f"### {key}")
        w("")
        if not rows:
            w("No matches in the script closures.")
            w("")
            continue
        w("| File:line | Line |")
        w("|---|---|")
        for path, i, text in rows:
            w(f"| `{path}:{i}` | `{esc(text)}` |")
        w("")
    w("### Player damage lines")
    w("")
    dmg = [row for row in player_rows if row[4].startswith("damage/health")]
    if not dmg:
        w("No story/bonus player-reference line classified as damage/health.")
    else:
        w("| File:line | Line |")
        w("|---|---|")
        for path, i, text, call, kind, map_s in dmg:
            w(f"| `{path}:{i}` | `{esc(text)}` |")
    w("")
    w("`IsLegHurt` and `GetHealth` are queries. `Hurt(` on those lines is the damage call.")
    w("")
    w("### Keyword collisions (not map load, save, checkpoint, or player respawn)")
    w("")
    for key in (
        "SetMap lens-flare rect (not a level load)",
        "animal CanBeRespawned / StampedeRespawn (not a player respawn)",
        "OpenMenuPage (page index)",
        "other keyword",
    ):
        rows = buckets.get(key, [])
        w(f"- `{key}`: {len(rows)} lines.")
        for path, i, text in rows[:8]:
            w(f"  - `{path}:{i}` `{esc(text)}`")
        if len(rows) > 8:
            w(f"  - … {len(rows) - 8} more")
    w("")
    w("There is no script call in these closures whose text is `LoadMap`, `NextMap`, `Checkpoint`, `SaveGame`, or `LoadGame`. `Restart` / `RestartMission` / `OpenMenu` / `SaveGame` exist as ExportFnc names (section 2c, GameDll registrar `0xc2d40` and CommonDll `cGuiSaveMan::ScrExport` `0x102270`). Whether a script calls `Restart` is the table above, not an inference from the export.")
    w("")
    w("Candidate C++ exports from `re/exports/*.demangled.txt` whose demangled name matches `Checkpoint`, `Save`, `Load`, `Restart`, `Respawn`, `Death`, `Die`, `NextMap`, or `Level`. These are names only (UNVERIFIED: not tied to a script call).")
    w("")
    # filter noise: require the match to be a camel-case token boundary-ish by listing unique names
    # Group and drop very generic Load/Get if huge — still list all unique function names but skip compiler noise
    shown = []
    for src, name in export_hits:
        # keep if any strong token OR (Level/Save/Load/Die/Death/Restart) appears as part of an identifier
        if re.search(r"Checkpoint|Respawn|NextMap|SaveGame|LoadGame|Restart|PlayerDeath|OnDeath", name, re.I) or re.search(
            r"(Save|Load|Death|Die|Level)", name
        ):
            shown.append((src, name))
    w(f"Matching demangled lines: {len(shown)}.")
    w("")
    # Prefer the stronger subset in full, and a capped 'Level/Load/Save/Die' list if enormous
    strong = [x for x in shown if re.search(r"Checkpoint|Respawn|NextMap|SaveGame|LoadGame|Restart|Death|Die", x[1], re.I)]
    w(f"Stronger subset (`Checkpoint`/`Respawn`/`NextMap`/`SaveGame`/`LoadGame`/`Restart`/`Death`/`Die`): {len(strong)}.")
    w("")
    for src, name in strong:
        w(f"- `{src}`: `{name}`")
    weak = [x for x in shown if x not in strong]
    w("")
    w(f"Remaining matches (name contains `Save`, `Load`, or `Level` but not the stronger tokens): {len(weak)}. First 80:")
    w("")
    for src, name in weak[:80]:
        w(f"- `{src}`: `{name}`")
    if len(weak) > 80:
        w(f"- … {len(weak) - 80} more in `re/exports/*.demangled.txt`")
    w("")

    # section 8
    w("## 8. Companions / NPCs")
    w("")
    w("Character `<instance>` members of the globals class (`cScrNPC`, `cScrCustomCharacter`, `cScrCustomCharacterEx`, `cScrHuman*`). Quotes are non-comment lines in the map closure that name that instance and call `StartTask`, `SetTaskParam`, `PlayMessage`, `PlaySound`, `Follow`, `MoveToPos`, or `LookTo`. `cScrHunter player` is the player bind, not an NPC.")
    w("")
    for mname in STORY + BONUS:
        info = npc_info[mname]
        if not info["names"] and not info["npcs"]:
            w(f"- **{mname}**: no NPC/custom-character instances in globals.")
            continue
        bits = ", ".join(f"`{c} {n}`" for c, n in info["names"]) or ", ".join(f"{c}×{n}" for c, n in info["npcs"])
        w(f"- **{mname}**: {bits}. Behavior lines: {len(info['quotes'])}.")
        for path, i, text in info["quotes"][:25]:
            w(f"  - `{path}:{i}` `{esc(text)}`")
        if len(info["quotes"]) > 25:
            w(f"  - … {len(info['quotes']) - 25} more lines")
    w("")

    w("## 9. Summary for the lead")
    w("")
    local_player = Counter()
    for path, i, text, call, kind, map_s in player_rows:
        if map_s.startswith("shared"):
            continue
        for m in map_s.split(","):
            if m in STORY or m in BONUS:
                local_player[m] += 1
    cut_n = {m: len(map_meta[m]["cuts"]) for m in STORY + BONUS}
    npc_n = {m: len(npc_info[m]["names"]) for m in STORY + BONUS}
    ranked = sorted(STORY, key=lambda m: (cut_n[m], npc_n[m], local_player[m], len(mission_info[m]["tasks"])))
    def brief(m):
        return f"{m} (cutscenes {cut_n[m]}, npc instances {npc_n[m]}, map-local player lines {local_player[m]}, tasks {len(mission_info[m]['tasks'])})"
    qte_code = [h for h in qte_hits if h[2] == "code"]
    w(f"- Player-reference lines in story+bonus closures: {len(player_rows)}. Kinds: " + ", ".join(f"{k} {n}" for k, n in kind_counts.most_common()) + ".")
    w(f"- Cutscene scripts named by story+bonus MAIN.SPJ: {sum(cut_n.values())} on {sum(1 for m in STORY+BONUS if cut_n[m])} maps. Cutscene files on disk: {len(cut_files)}. Files containing `state Skip`: {len(skip_files)}.")
    w(f"- QTE/Struggle/Melee/Button/CloseCombat/Mash/QuickTime: no code hits. Code hits for the search words: {len(qte_code)} (knife distance, knife equip, `Pumafight` anim state, knife class names). String/comment hits are dialogue or comments (section 5).")
    w(f"- Maps with a globals NPC or custom-character instance: {sum(1 for m in STORY+BONUS if npc_n[m])} of {len(STORY)+len(BONUS)}.")
    w("- No `LoadMap` / `NextMap` / `Checkpoint` / `SaveGame` / `LoadGame` call in the script closures (section 7).")
    w("- Fewest cutscenes, then fewest NPC instances, then fewest map-local player lines: " + "; ".join(brief(m) for m in ranked[:5]) + ".")
    w("- Other end of that order: " + "; ".join(brief(m) for m in ranked[-5:]) + ".")
    w("- Cutscenes start with `ExecuteScript(\"cScrCutScene…\")` from trigger scripts. Inside the cutscene script the player calls are `SetPFlags(PLAYER_INCUTSCENE)`, `ActivateCutSceneMode`, `ClearPFlags`, `DeactivateCutSceneMode`, plus camera `BeginCamera` / `EndCamera` (section 5).")
    w("- Task definitions are `m_pMission.CreateTask` blocks in `MISSIONS\\*.SCR`. Scripts advance them with `UpdateMissionStatus` and `ActivateTriggerOnEnd` / `ActivateTriggerOnFail` (section 6). `CompleteTask` and `GlobalsCheckMissionStatus` counts are in section 3.")
    w("")

    w("## Proposed MEMORY facts")
    w("")
    w("- GameDll7 `FUN_1000ba30` (RVA `0xba30`) is the global script-native registrar (`cExportKey::ExportFnc` + `cBaseClass::s_ExportStaticFunction`). Evidence: `re/notes/lead-natives.c` and the same function in `re/notes/story-natives-gamedll.c`. Already recorded in MEMORY.md; repeated here only as the index for section 2a.")
    w("- CommonDll7 class method natives are registered by `ScrExport` functions that call `cExportKey::ExportFnc` then `cBaseClass::s_ExportFunction`. Evidence: `re/notes/story-natives-commondll.c` headers, including `cScrCharacter::ScrExport` RVA `0x10700` (`StartTask`, `SetTaskParam`, `ForceBrainActivate`, `LockInput`, `BrainFreeze`) and `cScrHuman::ScrExport` RVA `0xeae0` (`SetHumanSize`, `IsCrouched`, `SetCrouched`).")
    w("- Those CommonDll `ScrExport` functions copy the parent class name into the export block, not the leaf class: `cScrHuman::ScrExport` copies `s_cScrCharacter` with `AllocMem(0xe)` (14 bytes). Evidence: `re/notes/story-natives-commondll.c` lines 24–29.")
    w("- GameDll7 has further `ExportFnc` registrars besides `0xba30` (25 callers in the slot W dump, listed in section 2b). Unnamed `FUN_` registrars store a parent-class string the same way; the leaf class is not named in that decompilation.")
    w("- Story map globals bind the player with `setidx(player, FindEntScrObject(\"hunter\"))` on a `cScrHunter player <instance>` member. Evidence: `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR` lines 59 and 106–108 (`InitPlayer`). Other maps use the same call where section 4 lists `FindEntScrObject`.")
    w("")

    OUT.write_text("\n".join(L) + "\n", encoding="utf-8")
    print(f"wrote {OUT} bytes={OUT.stat().st_size}")
    print(f"maps {len(STORY)+len(BONUS)} missing {len(missing)} natives {len(all_rows)} usage names {len(use_map)} player lines {len(player_rows)} qte {len(qte_hits)} trans {len(trans_hits)} exec {len(exec_sites)}")
    print("missing sample", missing[:30])
    print("kind", kind_counts)
    print("gfuncs", len(gfuncs), "cfuncs", len(cfuncs), "global rows", len(globals_rows))


if __name__ == "__main__":
    main()
