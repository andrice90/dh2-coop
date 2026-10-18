# Ground rules for Claude worker agents (2026-10-17)

You are one of several Claude agents working in parallel on the DH2 co-op mod (`D:\scary hunt\dh2coop`, branch `main`). The lead (Opus, main thread) reviews and merges your work. Read these before anything else, then your brief.

## Never
- Never write to the original install `D:\scary hunt\Activision Value\Magic Wand\DNGH2` (reading its logs is OK).
- Never commit game binaries, assets, extracted data (`re/extracted/`), decompiled code, or long verbatim script quotes. Notes may name functions, offsets, and short identifiers.
- No DRM / disc-check work. No Cheat Engine. No slow motion. Neutral branding.
- Never close, kill, or send input to a game process that you did not start from **your own** checkout's `game*` folders. Other agents and the user run games at the same time. Match processes by full path (`<your root>\game\Bin\Main7.exe`, `...\game2\Bin\Main7.exe`), never by name alone. Four old helpers (`event-test`, `mapflow-test`, `move-test`, `focus-test`) match `*\game\Bin` and would grab another checkout's game; do not use them in parallel, or fix them to use `$root` first.
- Do not run `git` commands that write in the main checkout (`D:\scary hunt\dh2coop`) unless your brief says so. Research agents write their note files there and leave committing to the lead.
- Do not edit `STATUS.md`, `MEMORY.md`, or `docs/mod-learnings.md`; put proposed facts in a "Proposed MEMORY facts" section of your note. The lead merges them.

## Running the game
- Use **your** checkout's `run-coop-local.ps1` with **your** `-Port` (given in the brief). It refuses to start if a game from your checkout is already running and ignores other checkouts.
- Build with `.\mod\build.ps1` in your checkout (it only refuses when your own `game\Bin\Main7.exe` runs).
- Drive games through the command file `<tree>\Bin\dh2coop.cmd` (one console line per write; wait ~1.2 s between writes). Do **not** send keyboard or mouse input: games ignore input unless their window is in front (`[Coop] InputOnlyWhenFocused=1`), and many windows are open. If a test truly needs input, add a `coop_*` console command in your worktree instead, or report the need to the lead.
- Screenshots: `tools-src\capture-window.ps1 -ProcessId <pid> -Out <path> -NoFocus -PrintWindow` (without `-PrintWindow` it copies the screen, and other agents' windows at the same spot end up in the shot). Save under `re\notes\coop-local\` (gitignored) with a prefix naming your job.
- Logs: `<tree>\Bin\dh2coop.log` (`.1/.2` older runs) and `<tree>\Bin\debug.log` (`debug.1/.2.log`). A new launch rotates them: copy what you need first. Read logs with shared access (the game keeps them open).
- When a run ends, close only your own games: `CloseMainWindow()`, wait 5 s, then `Stop-Process -Force` for survivors. Do not pipe a long-running script into `Select-Object -First`: that stops the script before its cleanup.
- Keep at most one co-op session (2–4 windows) of yours running at a time.

## Reverse engineering
- Ghidra headless: `.\re\scripts\ghrun.ps1 -Slot <your slot>` / `.\re\scripts\decomp.ps1` with **only the slot letter in your brief** (projects lock per slot). The lead uses slot L.
- Base addresses: offsets are module-relative (`GameDll7.dll+0x...`, `EngineDll7`, `CommonDll7`, `Main7.exe`). Verify bytes before writing a hook site.
- Extracted data and scripts: `re\extracted\` (SCRIPTS, OBJECTS, LEVELS, ...). Prior notes: `re\notes\*.md`. Start with `re\notes\coop-checklist.md` (what MAP1/MAP2 taught us), `re\notes\story-objectives.md`, `MEMORY.md`, `docs\mod-learnings.md`, `STATUS.md`.

## Code and commits (worktree agents)
- Work only in your worktree on your branch. Match the surrounding code style and comment density. Keep new behavior behind an ini key when it is risky; default it to what you verified.
- Commit when a step works, author `andri <andrice90@gmail.com>`:
  `git -c user.name=andri -c user.email=andrice90@gmail.com commit -q -F <msgfile> -- <paths>`
  Write the message file without a BOM (`[IO.File]::WriteAllText`). End every message with the line
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`
- Do not merge into `main`; the lead cherry-picks.

## Deliverable
- A note `re\notes\<your job>.md` (in your worktree for code jobs; in the main checkout for research jobs) with: what you did, evidence (log lines, addresses, screenshot paths), verdicts marked VERIFIED / UNVERIFIED, open questions, and "Proposed MEMORY facts".
- Your final message to the lead: a short summary, the commits (hash + subject), what is verified, what is not, and anything that needs the lead's or the user's decision.
