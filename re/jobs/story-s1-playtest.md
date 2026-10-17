# Job: story-s1-playtest — MAP3B co-op play-through record + smoke test of the next story maps (YOU OWN GAME LAUNCHES; no Ghidra)

Read `docs/handoff-cursor-story.md` first: it holds the rules, tools and the task list. This file is the spec for tasks 1–2 there.

## Task 1: MAP3B play-through with the user
1. `.\run-coop-local.ps1 -Map MAP3B`. Tell the user the windows are ready: left = host, right = client.
2. Ask the user to play MAP3B to the end (or until something breaks) in the host window, and to move the client player around too. Ask them to report what looks different between the windows.
3. While they play, do not build or close windows. After they finish, quote these lines into your note before any relaunch:
   - every line matching `story:|events:|mission|exception|Access violation` from both `Bin\dh2coop.log`;
   - the crash text from both `Bin\debug.log`, if any.
4. Write `tools-src\story-digest.ps1`:
   - Parameters: `-HostLog`, `-ClientLog`; defaults are the two `Bin\dh2coop.log` files.
   - Output: one time-sorted table `time | role | kind | detail` built from those lines. `kind` is `arm`, `fire`, `request`, `apply`, `mission`, `task`, `exception`.
   - Test it on the play-through logs and paste its output into the note.
5. Note `re/notes/story-s1-map3b.md`:
   - What happened, in the user's words.
   - The digest table.
   - Each divergence: what, when, host vs client, decisive log lines, and a hypothesis marked UNVERIFIED.
   - Screenshots under `re/notes/coop-local/` (gitignored).

## Task 2: smoke test of MAP12B, MAP12C, MAP4A, MAP11B
For each map:
1. `.\run-coop-local.ps1 -Map <MAP>`, wait 30 s.
2. Capture both windows with `-NoFocus`.
3. Read both logs.
4. Close the windows (you launched them).

Add one table row per map:
- `coop: welcomed` on the client?
- `animals: client received N`?
- The trigger events armed at map start (`story: [solo] trigger event activate ... name=`) on host and client: are they the same?
- Any `exception`?
- Crash dialog text, if any.
- Screenshot file names.

## Rules
- Edit only: `tools-src\story-digest.ps1`, `re/notes/story-s1-map3b.md`, `STATUS.md` (your worker log section and Next steps).
- Commit after each task (author andri, trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`).
- No source changes to `mod/src` in this job. Engine/sync problems go into STATUS "Next steps" for the lead.
