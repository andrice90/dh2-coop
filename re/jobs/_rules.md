# Job rules (apply to every job)

You are a worker agent on a co-op mod for the 2005 PC game Cabela's Dangerous Hunts 2. The user owns the retail disc. Working directory is the project root `D:\scary hunt\dh2coop` (Windows; PowerShell and Git Bash available; Python is `py`, with pefile installed).

1. Read `CLAUDE.md`, `MEMORY.md` and `PLAN.md` first.
2. Write only the files your job lists as deliverables, plus new helper scripts under `re\scripts\`. Never edit `MEMORY.md`, `PLAN.md`, `CLAUDE.md` or another job's deliverables. Put facts you have verified in a section of your deliverable named `## Proposed MEMORY facts`, each with evidence (export name, RVA, string address, log line, or file path). The lead reviews them and merges them.
3. Never write to `D:\scary hunt\Activision Value\` (original install). Runtime tests use `game\` only, and only if your job says you own game launches.
4. No DRM, disc-check, or copy-protection work of any kind.
5. Do not `git commit`, push, or change git config.
6. Ghidra: use `.\re\scripts\decomp.ps1 -Slot <your slot> ...` only with the slot letter your job assigns. Never run two Ghidra processes on the same slot at once. Extra Ghidra Java scripts already exist in `re\scripts\` (XrefStrings, CallersDecomp, DecompileAt, DataRefs, DumpPtrs, Probe); read them before writing new ones. `re\scripts\ghrun.ps1` may be a generic runner; check its header.
7. Separate verified from inferred. Mark anything not directly shown by code, data or logs as `UNVERIFIED`.
8. Raw decompiler dumps go in `re\notes\` as `*.c` or `*.txt` (gitignored). Summaries go in the `.md` deliverables.
9. When done, print a final summary of at most 250 words: what you found, the deliverables written, and open questions.
