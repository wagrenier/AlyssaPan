# Open items -- priorities, decisions, cross-TU questions, done log

The user's priorities first ("Next up"), then decisions with their date and who made them, then open
questions. Resolve items here when they are settled (move them to "Done" with the commit).

## Next up (in order)

0. **Bootstrap (bootstrap.md), state on 2026-11-09:**
   - ~~1. The game~~ done: Clock Tower 3, USA, `SLUS_206.33`; CodeWarrior C with a full symtab
     (architecture.md "Binary and toolchain").
   - ~~2. Ghidra project~~ done by the user: `SLUS_206.33` in `CLaude Decomp` beside FS2 (ghidra.md).
   - ~~3. Tools~~ carried over from YamiPan, renamed (`alyssapan.py`, tag `CT3`, work files `*_ct3.json`);
     FS2's harnesses (`*cmp.py`), script / pack readers, `rtti.py`, `iop.py`, `rxcmp.py` left behind.
     **`refresh.py` runs green on CT3 (2026-11-09):** 2,628 functions, 13,643 calls, 520 tail jumps; 1,814
     game functions (1,807 named), 814 library; `alyssapan_functions.txt` and `status.md` generated.
     The string gap (0 string references) is fixed (2026-11-09): `elf32.py` swapped in layout.json's
     regions only for an *unnamed* image section, and CT3's is named `main`; now any image without a
     standard `.`-named section gets them. `features.py`: 1,200 string refs, 1,285 float consts, 13,583
     calls, 172 tail jumps (the old counts read data as code past 0x018fe500).
   - ~~4. Names~~ done (2026-11-09, scheduled run, `tools/reverse/symtab.py`): `data/tags.json` and
     Ghidra's function tags carry `game-symbol` on all 2,621 symtab-named functions (2,613 FUNC addresses +
     8 asm entry points named by NOTYPE labels); no FUNC symbol lacks a Ghidra function; all 2,657 data /
     code-label symbols (996 `@<n>`, 374 `name$<n>`, 1,287 named) are labels in Ghidra (the loader had
     applied 2,650; 7 added); `tiXcmd___________Free0` (0x018e20b0) restored from Ghidra's demangling.
     Left for the TU sessions that reach them: the 7 Ghidra functions with no symbol (architecture.md
     "Naming sources"), named `game-inferred` when read.
   - ~~5. The TU map~~ done (2026-11-09, scheduled run, `tools/reverse/tumap.py`): 66 objects (51 `game/`,
     15 `ga/`), 64 written to `data/tus.json` beside `game/main.c` (architecture.md "The TU map" has the
     method and evidence). Doubts for the TU sessions, not blockers: (a) 1,298 of 1,814 functions reference
     no local, so the larger objects (`QuatSlerp.c` 177 functions, `wideXcmd_DebugPrint.c` 109,
     `tiXcmd_TransItem.c` 93, `ssSetTexUVforCubeMap.c` 96, `ahRelocateSunPs2ObjectArry.c` 93) are probably
     several objects each: the session that claims one pins its real extent with `tus.py define` and
     defines the rest; (b) the keys are names taken from the functions, except the five source files: a
     session may rename its TU's file when it learns the real name; (c) main's locals fall in two chunks
     (60, 61): main.c stays one TU (0x01834440..0x0183aed8) until its session says otherwise.
     **Bootstrap done**: the next run takes the queue (`game/main.c` first).
   - ~~6. Platform layer~~ YamiPan's Pan layer carried over (cpp-port.md); the siblings' shims still to
     come are item 2 below.
   - ~~7. First build~~ green (2026-11-09): `build.py --run 15` opens the window on D3D12, logs
     `[stub] main (CT3 0x01839d10) reached`, writes shots (black: nothing draws yet). `io::Disc` mounts
     only the root's 2 files (the path table is root-only: item 3). Then the queue.
1. **`main`'s TU first** -- **partial (2026-11-09):** slice 1 written (main, the boot and init functions,
   ControllerMain, tiItemInit, scene_end, the small draw passes; 15 functions); slice 2 (2026-11-09):
   `GlobalDrawObject`, `scene_draw_init`, `system_end`, `get_draw_obj`; slice 3 (2026-11-09): `main_rtn`, `main_rtn_Particle`. Left in main.c:
   `AllDrawObject`, `DrawObjectRefraction`, `ssHQDOF_DrawAllObject`, `main_rtn_Particle2`,
   `main_rtn_DOF` / `_GHOST` / `_GHOST_DOF`, `scene_init` (stubs in main.c). Open from slice 2: (f) `ALL_TABLE` (All_table_TBL's records, ct3_types.h) is
   `layout: host` with pointer fields at EE 0x10..0x7c; the file TU decides whether the bank's relocated
   words become host pointers or stay `u_int` EE addresses; (g) settled by slice 3: `main_rtn(bank, mode, chara, flags, charaIndex, lastTex)` -- chara != 0 is the
   character pass (scene fog +0x28, Character[].lightBank's light), flags' low 15 bits the pass (0 and 4
   sort), 0x8000 no clip check; (h) **blocks `main_rtn_Particle2`**: it writes a particle's position
   through a pointer at the ga draw object's +0x38 (`*(qword **)(obj + 0x38)`), and the same object keeps
   an int at +0x3c (`MODEL_PARTS.attr`): a host 8-byte pointer there overlaps it, so the draw object is
   bank data that must keep EE layout with `u_int` EE addresses, and the port needs an EE-address ->
   host-pointer helper for relocated bank data (the file TU's decision, with (f)); (i) the draw list and
   particle records (`DRAW_OBJ_ENTRY`, `TEX_INFO`, `PARTICLE_ENTRY`, ct3_types.h) are `layout: host`
   for the same reason: their owner (objSort / the bank loader) decides; osTotalObjTable keeps the EE
   2000 bytes per bank, so on the host it holds 250 pointers, not 500. **Next:**
   ~~`ga/gaInitInterrupt.c`~~ (verify, 2026-11-09: main's frame loops now), then in the order a smoke
   run reaches the stubs: `main2` (game/main2.c: title and scene set-up), `IOPInit` / `Character_All_Init`
   (Character_Init.c), `FS_work_init` / `FileSoundServer` (fileio.c), `SunAllocInit` (ahalloc.c), the ga
   library (`gaDmaInit`, `gaVifInit`, `gaSetupGSInitData`, the BGT packets, `gaPadRead`), `movie.c`.
   Open from slice 1, for the TUs that own them: (a) the 106 globals main.c's slice uses live in
   `lib/unassigned_data.cpp` (declared in `game/unassigned_data.h`, structs in `game/ct3_types.h`): move
   each to its owner when written; (b) `BANK_INFO.loaded` (+0x20) is an int here, maybe a pointer;
   (c) the trigger events' names in main.c (`EV_HOLD_*`, 0xca / 0xcc) and `controller_mode` 100 are read
   from main's use only; (d) the prototypes in the new TU headers come from call sites: confirm each when
   its function is written; (e) main.c's .sbss run is a placement guess (architecture.md).
2. **The siblings' shims (the user's rule of 2026-11-09: "reuse the shims of the other pan projects like
   the windows handling and all the rest")**, as host tasks, in cpp-port.md "To carry over" order, each a
   class: (a) gamecontrollerdb + icon + cursor auto-hide + pause / mute in the background + DPI scale
   (small, one session); (b) exclusive fullscreen, window position and presets; (c) the HDR swapchain,
   MSAA, anisotropy, GPU driver choice; (d) the real memory card (RosePan's libmc), reverb and volume
   groups (YamiPan's SPU2 is next to them), the audio dump; (e) `libsdr` (CT3 calls `sceSdRemote`), newlib
   `rand`; (f) .iso reading, save import / export; (g) RmlUi menus and translations; (h) CI and AlyssaPan's
   own `extern/`. A shim CT3's boot needs earlier (libsdr, the card) moves ahead of TUs when a run hits it.
3. **io::Disc's path table is root-only** (FS2 needed no more): CT3 opens `\DATA\CT3PACK.DAT;1` and
   `\IOP\*.IRX`. Add directories when the file TU shows how CT3 reads (sectors through libcdvd, or names
   through `FSYS_IOP`).

## Decisions

- 2026-11-09 (the user): **AlyssaPan ports Clock Tower 3** (the copy with debug symbols, `SLUS_206.33`,
  already loaded in Ghidra). It **keeps YamiPan's priorities and design guidelines**, **reuses the shims of
  the other Pan projects ("the windows handling and all the rest")** and **builds on YamiPan's C++
  adaptation** of the Pan layer.
- 2026-11-09 (the user): **scheduled runs**: the desktop app's task `alyssapan-next-tu` (cron
  `*/20 * * * *`, prompt = `tools/reverse/routine.md`) advances the port by one step per run -- the next
  bootstrap step while item 0 above has one, then one TU or host task -- gated by `preflight.py`. Pushing
  each run's commit to origin/ct3-port (`github.com/wagrenier/AlyssaPan`) follows YamiPan's approval of
  2026-10-31; never force. The queue after the bootstrap: `game/main.c` (pinned 0x01834440..0x0183aed8,
  29 functions), then the host tasks `alyssapan/core/window.cpp` (shims a) and `sdk/libmc.cpp` (the card).
- 2026-11-09 (Claude's call at bootstrap, for the user to overrule):
  - The repo is `C:\Users\willi\Documents\git\AlyssaPan`, a new git repository on `main` with nothing
    committed yet; the work branch for the TU loop is to be **`ct3-port`** (the tools refuse `main`).
  - The annotation tag is **`CT3`** (`// CT3 0x...`); provenance tag `game-symbol` for every symtab name.
  - The Pan layer came from YamiPan's HEAD (17e0108, then 93c73f0's audio), not from its working tree
    (uncommitted edits there stayed out). FS2's own host modules stayed in YamiPan (cpp-port.md "Left in
    YamiPan"); the renderer was trimmed to the 2D path; TextureStore decodes a TIM2 file directly (FS2's
    `Tim2Texture` gone); the movie seam takes the picture count; sdk/sif.cpp starts IOP modules from a
    table (empty) instead of booting RXSNDDRV; FS2's `mcerror.cpp` dropped.
  - Game directories: `src/game/` for the game, `src/ga/` for the `ga*` library, `src/iop/` for IOP code;
    file names from the game's strings where they exist (`movie.c`), built as C++ like every sibling.
  - The game data stays in the user's folder `C:\Users\willi\Documents\Decomp\Clocl Tower 3`; nothing from
    it is committed. A data folder is one with `DATA\CT3PACK.DAT` and `IOP\`.
  - CT3's Ghidra program lives in YamiPan's project (`CLaude Decomp`), as the user set it up: every call
    passes `program="SLUS_206.33"`.
  - No autopilot.py (a terminal loop): the scheduled task is the automation (see the decision above).

The user's standing rules, carried from the earlier ports (SKILL.md "The user's rules" says how they
apply here):

- 2026-11-07 (the user, YamiPan): **time limits**: a session has 60 minutes; no game run lasts more than 10
  minutes, and every background command has a limit before it starts.
- 2026-11-04 (the user, YamiPan): **Japanese stays Japanese**: Shift-JIS text from the ROM goes into the
  source as the Japanese itself (reconstruction.md "Japanese text"). CT3 (USA) is English, but its data may
  still hold Shift-JIS.
- 2026-11-03 (the user, YamiPan): **the Pan layer is modern OO C++; the game layer stays authentic**
  (cpp-port.md).
- 2026-11-02 (the user, YamiPan): (1) **no GS emulator, not even for textures**; decode assets at their
  source; (2) **name the patterns**: repeated values and flag bits become enums / `#define`s / `constexpr`
  as the code is read; (3) **any aspect ratio is the port's #1 feature**; (4) reuse the siblings'
  abstractions instead of reimplementing the basics.
- 2026-11-02 (the user, YamiPan): **source code first**: name what a TU needs, reconstruct, do not seed
  names for their own sake (CT3's symtab makes this easy).
- 2026-10-31 (the user, YamiPan): pushing each finished commit to the work branch's origin is approved;
  never force. (AlyssaPan has no remote yet.)
- 2026-10-30 (the user, RosePan): **skip cinematics in smoke runs** (`skip_intro` in the ini; smoke runs
  force it off so pad timing holds; the smoke pad holds START through films).
- 2026-10-28 (the user, RosePan): the port displays correctly at any aspect ratio, from one
  renderer-owned source; never a fixed 4:3 / 640x448 in host draw code.
- 2026-10-22 (the user, RosePan): drawing never goes through an emulated GS; the game's draw decisions
  are reconstructed and the packet-writing step calls a native host draw module per subsystem.
- 2026-10-15 (the user, ReiPan): methods, not free functions (C++ games). CT3 is C: free functions, as the
  symtab names them.
- ReiPan / RosePan (the user): **the picture is the test**; a drawing TU is `done` only when the shot
  matches the game's own assets; complete code nothing can show yet is `verify`.
- ReiPan / RosePan (the user): automated porting runs at effort xhigh or above; sessions never start
  other sessions, agents or scheduled tasks, and never edit `autopilot.py` or the routine.
- ReiPan (the user): an empty queue means re-assess, not "follow the frontier".
- MioPan (the user): debug probes are stripped before committing; frame smoothing interpolates, never
  extrapolates, and adds no input lag.

## Open questions

- (2026-11-09) What lies below 0x01810000 at run time? The ELF is linked high in the 32 MB and keeps
  `.relmain` relocations; the port does not load the ELF, so this only matters if code addresses memory
  under its own image (a buffer, a second program). Look in `main` / `system_init`.
- (2026-11-09) Is `SNDDRV.IRX` the game's own driver (to reconstruct under `src/iop/`, as RXSNDDRV was) or a
  middleware build? And how much of the sound goes through SCE's `MODHSYN` / `MODMIDI` (hardware-synth
  MIDI, which no sibling met) against `sdremote`? `sound.c`'s TU answers it.
- (2026-11-09) `CT3PACK.DAT`'s format, and whether CT3's pictures are TIM2 (TextureStore decodes TIM2 only).
- (2026-11-09) Is the "one EE thread at a time, by priority" model right for CT3? (YamiPan's for FS2.)

## Done

- 2026-11-09: AlyssaPan created: the repo, YamiPan's Pan layer (C++, HEAD 17e0108 plus 93c73f0's SPU2
  voices and sound card) trimmed to the generic, SDK, CMake, tools, the skill and its references (this
  file, architecture.md, cpp-port.md, bootstrap.md, reconstruction.md, ghidra.md, hazards.md with the
  siblings' digest). Build green, first smoke run and first `refresh.py` done. Nothing committed: the
  user decides.
