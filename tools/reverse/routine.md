---
name: alyssapan-next-tu
description: Advance the Clock Tower 3 port in AlyssaPan by one step (a bootstrap step or one TU), commit and push.
---

AlyssaPan scheduled run: advance the Clock Tower 3 (PS2, USA, SLUS_206.33) -> PC port by exactly ONE step -- the next unfinished bootstrap step while there is one, one translation unit (TU) or host task after that -- commit, push, report.

Repository: C:\Users\willi\Documents\git\AlyssaPan, on the work branch ct3-port. Work only inside it; ../YamiPan, ../RosePan, ../ReiPan, ../MioPan, ../MikuPan and ../GhidraMCP are read-only references (YamiPan is the Pan layer's source and the closest precedent). Nobody is watching this run: never wait for answers or ask questions -- decide, write doubts into .claude/skills/alyssapan-port/references/open-items.md, and carry on.

Python: run every Python command with the launcher `py` (`py preflight.py`, `py tus.py next`), also where the skill writes `python <tool>.py`: on this machine `python` can be the Microsoft Store stub ("Python was not found").

Time (the user's rule): this run has 60 minutes from the run lock's `started`. Check the clock at each step; at 45 minutes start nothing new and finish what is coherent (status partial or verify, notes for the next run); at 60 minutes finish, push and release the lock whatever is left. No game run lasts more than 10 minutes: `build.py --run N` and `tus.py check --run N` carry their own limit; anything else started in the background gets a limit before it starts.

1. Preflight. From C:\Users\willi\Documents\git\AlyssaPan\tools\reverse run `py preflight.py`. It checks the user's stop switch (work/STOP), a running autopilot, the run lock, a clean tree, the work branch and Ghidra serving SLUS_206.33, then takes the run lock (work/run.lock).
   - Exit 3: the previous run is still going. Reply "previous run still going" and stop.
   - Any other non-zero exit: stop, change nothing, and reply with its line. A dirty tree or a stale lock usually means a run crashed: say so, so the user can look.

2. Advance one step. Invoke the Skill tool with skill "alyssapan-port" and args "next" (if that skill is not listed, read C:\Users\willi\Documents\git\AlyssaPan\.claude\skills\alyssapan-port\SKILL.md and follow it), then:
   - While open-items.md "Next up" item 0 (the bootstrap) has a step that is not struck through (~~...~~), the skill's "Bootstrap" applies: do the first unfinished one (references/bootstrap.md says what CT3 changes; item 0 says what is left), one step per run, and when it is too big for a run, a coherent part of it. Steps 4 and 5 are tools: write them under tools/reverse/ (in the tools README), run them, and apply what they find (data/tags.json, Ghidra through the tools, `tus.py define` for every object of the TU map). Record facts in architecture.md and tick or update item 0 in open-items.md. Commit with git yourself: look at `git status`, `git add -A`, `git commit` with a message naming the bootstrap step and ending with the Co-Authored-By line from your session instructions. The build must stay green (`py build.py`).
   - Once the bootstrap is through: the skill's "One TU per session" protocol end to end: `py tus.py next --claim`, brief, boundaries, names, header, code, `py tus.py check <TU> --run 15`, record knowledge in the skill references, and `py tus.py finish <TU> --status done|verify|partial|blocked --summary ... --notes ... --coauthor "<the Co-Authored-By line from your session instructions>"`. `tus.py next` honours the pinned queue in tools/reverse/data/tus.json (game/main.c first, then the queued host tasks: the siblings' shims), then partial TUs, then the frontier. A host task skips boundaries, names, header and code: it is host work as its task text says, under cpp-port.md's rules. Work on that one TU or task only; do not start a second one. When main's TU is written, delete the stub gameMain in src/lib/unassigned_stubs.cpp and build game/main.c with main=gameMain (CMakeLists.txt says how).
   - An empty queue means re-assess, not "follow the frontier": refill it from open-items.md "Next up" in the order a smoke run reaches the stubs (`py tus.py queue <key>`), say so in the report, and take the first.

3. Look at the screen once the run draws -- the visual test decides, not the logs. Smoke runs save screenshots to tools/reverse/work/shots/ (shot_<frame>.png every 60 frames, last.png); open them with the Read tool. Reference pictures come from the game's own assets (decoded at their source into work/ref/). A TU on the drawing path is done only when the screenshot shows what the reference shows, at 4:3 and at a wide aspect (any aspect ratio is the port's first feature); a wrong picture, or code still to write, means --status partial with the difference in the notes; complete code nothing on screen can show yet is --status verify.

4. Ghidra (GhidraMCP 7.0.0, one process serving CT3 beside FS2): pass program="SLUS_206.33" on every raw MCP call, strict_mode="off" on renames. The tools save their own writes; after a raw MCP write, call save_program for SLUS_206.33. The symtab's names are the game's own: never rename them.

5. Push. After the commit, run `git -c credential.interactive=never push origin HEAD` (pushing each run's commit to origin/ct3-port is approved: open-items.md "Decisions"). Never force-push, rewrite history, or delete branches or tags. If the push fails, say so in the report and leave the commit local.

6. Release the lock: `py preflight.py --release` from tools/reverse -- always, also when the step failed or was abandoned.

7. Report in 3-5 lines: the step or TU, its status, the commit hash and whether it was pushed, what the screenshot shows compared with the reference (once there is one), and the next blocker.

Rules: never start other sessions, agents or scheduled tasks; do not edit tools/reverse/autopilot.py, tools/reverse/preflight.py or this routine (tools/reverse/routine.md); never switch branches; never touch ../YamiPan or the other sibling checkouts; leave the tree clean (look at `git status` before committing for files that are not yours). Do not put backticks in --notes or --summary (the shell runs them). Bash heredocs with apostrophes or backslashes break in this harness: write scripts and JSON with the Write tool. If the step cannot be finished, keep what is coherent and builds, revert what does not, and leave notes the next run can act on.
