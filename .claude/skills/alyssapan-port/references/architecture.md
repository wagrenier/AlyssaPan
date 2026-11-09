# Clock Tower 3: what the game is made of

Facts with their address and source; guesses go to open-items.md. Replace what a fact supersedes.

## Binary and toolchain (bootstrap 1, 2026-11-09)

- **Clock Tower 3, USA, `SLUS_206.33`** (ELF dated 2003-01-24, `SYSTEM.CNF`: `BOOT2 = cdrom0:\SLUS_206.33;1`,
  `VER = 1.00`, `VMODE = NTSC`). The disc's files are in `C:\Users\willi\Documents\Decomp\Clocl Tower 3`
  (sic): `SLUS_206.33`, `SYSTEM.CNF`, `DATA\CT3PACK.DAT` (one archive), `DATA\MOVIE1`..`MOVIE3`, `IOP\`
  (`IOPRP255.IMG`, `SNDDRV.IRX`, `MODHSYN.IRX`, `MODMIDI.IRX`, `MODMSIN.IRX`, `SDRDRV.IRX`, `LIBSD.IRX`,
  `FSYS_IOP.IRX`, `PADMAN.IRX`, `SIO2MAN.IRX`, `MCMAN.IRX`, `MCSERV.IRX`).
- **Toolchain:** `.comment` = "MW MIPS C Compiler (2.4.1.01)" / "PlayStation2": the game is **C compiled by
  Metrowerks CodeWarrior** (the same family as FS2), its objects carrying MW's local symbols `@<n>`
  (anonymous literals) and `<name>$<n>` (function statics). The SDK, newlib and libgcc are prebuilt GCC 2.9x
  objects (`gcc2_compiled.`, `__gnu_compiled_c` markers). No C++ in the game.
- **Symbols:** a full `.symtab` (6,179 entries, 0x18230 bytes at file 0x142220, strings at 0x12bd40):
  2,433 global and 178 local functions, 552 global and 1,456 local objects, 295 section symbols, no file
  symbols. Ghidra's ELF loader applied them: 2,628 functions, 2,621 named (2026-11-09). These names are
  the game's own, tag `game-symbol`, never renamed. Source-file strings in the image: `ahalloc.c`,
  `movie.c`, `fileio.c`, `sound.c`, `xcmd_tn.c` (all game files), plus the SDK's.
- **Layout:** one loaded section `main` at **0x01810000** (file 0x80, 0x12bc80 bytes), entry 0x01810008,
  `heap` at 0x01c8d780 (size 0); a `.relmain` relocation section (0x44148 bytes) is kept, unusual for a
  retail ELF and unused by the port (the port never loads the ELF). Linked high in the 32 MB: what lies
  below 0x01810000 at run time (a loader, buffers) is open (open-items.md).
- **SDK versions:** libpad and libmc check their IOP modules' versions (strings 0x01931bf0, 0x01932010);
  `IOPRP255.IMG` is the IOP replacement image (SDK 2.5.x era). libmatch.py's references (FF3's SDK 3.0, FF2's
  2.8) are newer: with the symtab, matching is a cross-check, not a naming source.
- **Same engine as a sibling?** None known: Sunsoft-era code (`SunAlloc`, `SunFree` in `ahalloc.c`), no
  shared names with FF2, FF3, RoR or FS2 found yet.

## Address map (2026-11-09, from the symtab)

`tools/reverse/data/layout.json` has the regions and library ranges.

| range | what |
|---|---|
| 0x01810000..0x01834440 | crt0, SDK (libgraph, libdma, libvifpk, libipu / MPEG, libkernl, libcdvd, libmc, libpad), newlib libc / libm |
| 0x01834440..0x0183aed8 | the game's main object: `main_rtn_GHOST_DOF`, `main_rtn_GHOST`, `tiItemInit`, `FirstInitialization`, `game_init`, `ControllerMain`, the `ss*DrawAllObject` effects, `AllDrawObject`, `main_rtn*`, `scene_*`, `system_init` / `system_end`, **`main` 0x01839d10** (0x11c8 bytes) |
| 0x0183aed8..0x0183d9e0 | libgcc: long long and soft double (`__muldi3` .. `dptoul`) |
| 0x0183d9e0..0x0189f718 | game code (734 functions in one unmarked run; `ahalloc.c`'s `SunAlloc` first) .. `IopLoadModule` 0x0189f6a0 |
| 0x0189f718..0x018a8810 | the `ga*` library: `gaPad*`, `gaDma*`, `gaVif*`, `gaInit3dlib`, `gaBGT_DrawObject*`, `gaDrawObject_*`, `gaSinCos` / matrices, `gaSearchNearLight4`, `gaDump*`, `gaVsyncCallBack` (asm-shaped objects without `gcc2_compiled.` are hand-written or VU-heavy) |
| 0x018a8810..0x018d2a68 | game code (425 functions: `gaInitMeter`, `ss*` scene code ..) |
| 0x018d2a68..0x018d2ed4 | sdremote client: `sceSdRemoteInit`, `sceSdTransToIOP`, `sceSdCallBack`, `sceSdRemote` |
| 0x018d2ee0..0x018fe4bc | game code (`ssGrpPortalCheck` .. the `as*` character behaviour: `asCheckCharacterHited`, `asInitPlayerBehaviorControler`, `ChrMoveControllerMode_init` last) |
| 0x018fe500..0x01906a00 | `.vutext`: VU microcode, with `.DVP.overlay` sections in the symtab (dvp-as) and `.vubss` at 0x01c8d780 |
| 0x01906a00..0x01930f80 | `.data` |
| 0x01930f80..0x01944b00 | `.rodata` (the string table starts at 0x01930f80) |
| 0x01944b00..0x01c8d780 | `.bss` / `.sbss` |

**TU boundaries.** The symtab has no file symbols, and the game's objects have no section symbols (only
the GCC-built library objects do: 183 `.text` sections). But ELF local symbols are emitted per input
object, in link order: after the library groups (symtab index ~1,003 on), runs of MW locals (`@542`..,
`iCngFlag$2863`, `ShadowVolume1_vertex_index_data` ..) belong to one object each, and a game function that
references a run's statics is in that object. That, the five source strings and the name prefixes are
the TU map's evidence.

**The TU map (bootstrap 5, 2026-11-09, `tumap.py`; `work/tumap_ct3.json`).** What the symtab showed:
- **MW writes an object's locals in hash order** (`@542, @502, @443, @442, @500 ..` in ahalloc.c's run), so a
  run is not sorted, but **no two objects' locals interleave**, in any subsection.
- **The coarse regions mix MW's subsections.** Sorted by address, the locals' symtab index restarts at each
  subsection start: `.data` 0x01906a00.., `.rodata` 0x01931300 (the libraries' from 0x01930f80)..0x01937610,
  `.sdata` ..0x0193bc80, `.sbss` ..0x01944b40, `.bss` .. (gp 0x0193f5f0 sits in `.sbss`).
- A cut between two locals is possible only where every subsection's addresses are sorted across it: 141
  possible cuts, 142 chunks. Real object bounds are a subset (main's own locals fall in two chunks, 60 and
  61, symtab 895..911, just before libgcc's): a chunk is an object or part of one.
- Each game function keyed by the median chunk of what it references (510 of 1,814 functions reference a
  local; 23 dropped as outliers off the longest non-decreasing run); runs of one key are objects; the `ga`
  range's `.text` section symbols (0x0189f718, 0x018a1188 .. 0x018a86a8, 0x018a8810) start its asm
  objects, and its MW objects (`pad_button_read` 0x018a0880, `gaPadInit`, `gaInitInterrupt`) sit between.
- Result: **66 objects** (51 `game/`, 15 `ga/`), 64 written to `data/tus.json` (main's two pieces stay
  `game/main.c`). The source strings land where expected: `ahalloc.c` 0x0183d9e0, `movie.c` 0x018bda90,
  `fileio.c` 0x018c2170, `sound.c` 0x018c7fb0, `xcmd_tn.c` 0x018ee5f0. 1,298 functions reference no
  local: where they sit between two objects, the cut follows their names; **large objects (QuatSlerp.c 177
  functions, wideXcmd_DebugPrint.c 109, tiXcmd_TransItem.c 93) are likely several objects without statics**:
  a TU session pins its real extent (`tus.py define`) and names the rest. Keys other than the five source
  files are names from the functions (common prefix, else the first), not the game's.

## Naming sources (bootstrap 4, 2026-11-09, `symtab.py`)

- **The symtab, all of it applied.** Entries by bind / type: GLOBAL FUNC 2,433, LOCAL FUNC 178, GLOBAL
  OBJECT 552, LOCAL OBJECT 1,456, GLOBAL NOTYPE 56, LOCAL NOTYPE 1,206, LOCAL SECTION 295, and two FUNC
  entries with bind 13 (STB_LOPROC: `gaNormalizeVectorXYZ` 0x01891570, `gaNormalizeVector3XYZ`
  0x018922c0, in the game-code run, not the `ga` library's range; why their bind is odd is unread). 2,613 distinct FUNC addresses, every one a Ghidra function.
- **Asm objects name their entry points with NOTYPE labels**, not FUNC symbols: `ENTRYPOINT` / `_start`
  (Ghidra's `entry`, 0x01810008), `_exit` 0x018101c8, `gaDrawObject_Redraw_Polygon` 0x0189f720,
  `gaDrawObject1_Redraw_Polygon` 0x0189f84c, `gaDrawObject_Redraw_Polygon_DivSub` 0x0189f994,
  `gaDrawObject_Redraw_Polygon_Output` 0x018a00f8, `gaDrawObject1_Redraw_Polygon_Output` 0x018a026c,
  `trans_for_gs` 0x018a03e0; also the `Localmacro_*` branch labels inside the `ga` DMA code. Tagged
  `game-symbol` with the rest (2,621 in all).
- **Seven Ghidra functions have no symbol at all**: 0x0185c26c, 0x0185c4b0, 0x0185c4e8 (game code),
  0x0189f680 (just before `IopLoadModule` 0x0189f6a0), 0x018bf05c, 0x018d2838 (before the sdremote
  client), 0x018dab3c. Probably code reached only through a branch or a table, or Ghidra splitting a body;
  each gets a `game-inferred` name, or is merged into its owner, when its TU is read.
- **Data labels**: 2,657 symbols in the image (996 MW literals `@<n>`, 374 statics `<name>$<n>`, 1,287
  named), all labels in Ghidra. Ghidra lists `name$<n>` statics under the name with the `$` kept, which is
  how a TU's statics are found.

## Modules (name prefixes, 2026-11-09; to be refined as TUs are read)

- `ga*`: the game's own 3D / hardware library (pad, DMA, VIF, draw objects with clipping, lights, math,
  meters, vsync). `gaBGT_DrawObject_SetupDma_light0..2`: up to three lights, VU1 lighting.
- `ss*`: scene and screen effects (`ssVaselineDrawAllObject`, `ssWave`, `ssHazy`, `ssUltima`, `ssBlur`,
  `ssDOF`, `ssHQDOF`, portals `ssGrpPortalCheck` / `ssScnPortalCheck`, `ssCalc*`).
- `as*`: character behaviour and motion (`asRequestExecute`, `asCheckCharacterHited`, the motion requests).
- `Sun*`: the allocator (`ahalloc.c`: `SunAlloc`, `SunFree`, `SunAllocUsedBorderCheck`).
- `tiXcmd*` / `hhXcmd*` (and the `xcmd_tn.c` string): script commands, prefixes perhaps their authors'.
- `main_rtn*`, `scene_*`, `system_*`: the frame loop and mode switches in main's object.
- `movie.c`, `sound.c`, `fileio.c`: their own files (strings at 0x01935480, 0x019365f0, 0x01936098).

## main's object, game/main.c (2026-11-09, first slice)

- **The boot** (main 0x01839d10): GS / DMA reset, `gaDmaInit`, `gaVifInit`, the GS init packet (`gsinit`),
  the BGT ring buffer (`bgt_ringbuffer` = 0x01ad8000, the first 1 KB boundary inside `bgt_ringbuffer_work`
  0x01ad7c20, 0x8000 bytes used), frame buffers at 0 / 0x38000, Z at 0x70000, `gaInitInterrupt`,
  `FirstInitialization` (the three SunAlloc heaps, the frame-clear packets, `SkipMain = 4`), two cleared
  frames, then the display: **512 x 448 interlaced frame mode** (DISPLAY 0x001bf9ff0203227c: DX 636 DY 50
  MAGH 4, DW 2559 DH 447; DISPFB FBW 8), `IOPInit(argv)`, `ArtoolDebugOpen`, `game_init`.
- **Then forever:** `system_init`, `main2` (title and scene set-up), the frame loop until `fReset` or
  `load_continue`, and the teardown (`SceneEndBank(0, 86)`, `system_end`, the heaps rebuilt).
- **The heaps:** `gHeapJunc` "JUNC" over `gJuncAllocArea` (0x96000), `gHeapSound` "SUND" over
  `gSoundAllocArea` (0xe1000), `gHeapGlobalScene` "GLBL" over `gGlobalBankArea` = **EE 0x00400000, 3 MB of
  free RAM below the ELF** (the host gives it a buffer: lib/unassigned_data.cpp).
- **The frame:** pad (`gaPadRead(pad, mode)`, `pad` = two 0x24-byte PAD_DATA; `pad_*_button[0..1]` are the
  frame's copies), `ControllerMain`, the BGT packets, `ga3dlibwork` DMA'd to the scratchpad (0x70000400,
  0x40 qw), animation and shadows (banks of type 1 and characters with flag 0x40000000), triggers, the
  time stop (`time_stop_fream` / `time_stop_flag` bits 1/2/4/8), the demo skip (`DEMO_SKIP_FLAG` 0..5,
  10..13, START = `KEY_MAP[11]`, fades through `DemoFilterChangeWork`), characters (pos copied to
  posBefore at +0xb0), effects, the camera, the actress light (her model's matrix at `ActressMP`+0x1c0
  times the vector at +0x20, VU0 in the ROM, passed to `asWarpSceneObject`), **the draw pass by
  `DrawMode`** (0 normal, 1 DOF, 2 reflect map -- empty in retail, 3 blur, 4 ultima, 5 hazy, 6 wave,
  7 vaseline, 8 HQ DOF, 9 global objects only; frame clear colour = the scene's fog colour), fades,
  VSync **by busy-waiting on `ga_main_timer`** (the VSync callback `gaInitInterrupt` installs), movies,
  time. `SkipMain` / `DemoSkip` fast-forward: no pad, no drawing, ticks advanced by 5 / 10.
- **The ga library's VSync (`ga/gaInitInterrupt.c`, 2026-11-09)**, a GCC object (`gcc2_compiled.` in
  its local run; its `.data` is `wait_vsync_state` alone, 0x0192e790 = 0): `gaVsyncCallBack` (static)
  counts `ga_main_timer` every VSync and `ga_down_timer` for VSyncs no wait was pending on; a pending
  `gaWaitVSync` is released and T1 zeroed (the CPU meter's origin). `gaWaitVSync()` takes no argument
  (main passes nothing it reads), spins until the next VSync, sets DISPFB's FBP to
  `ga_draw_fbuf_address >> 11` (word address -> 2048-word page), swaps `ga_draw_fbuf_address` /
  `ga_disp_fbuf_address` and calls `sceGsPutDispEnv(&ga_disp)`; it answers the CSR FIELD bit read on entry.
  `ga_main_timer` / `ga_down_timer` (bss 0x01c8d730 / 0x01c8d734) are commons defined here.
- **Banks and the global pass (slice 2, 2026-11-09):** `asCheckBankID(b)` = min(b, 6) and
  `asCheckBankType(b)` = b >= 6: banks 0..5 have their own view matrix, the rest share bank 6's.
  `GlobalDrawObject` (every DrawMode's last pass) draws the loaded banks 0..5 -- those with
  `BANK_INFO.flags` (+0x28) bit 2 first, then the rest -- each: view matrix, `FlushCache`, Z clear,
  `SceneLightSet`, then `ALLobject[b]` set to the All_table record's lists +0x18 / +0x1c / +0x60 for
  `main_rtn` and the particle passes (+0x58, +0x78 / +0x7c); the first non-empty +0x60 / +0x7c list calls
  `ExportDrawToTex` once. `scene_draw_init` sets bit 2 for type 0/1 banks named `ARRW.BIN` or `E_*`, and
  type 0 banks `M<digit>*`; finds the parts `ACTRESS_OMNI_1` / `_2` / `_BASE` (`ahGetHash` +
  `asFindModelPartsID`) into `Omni0_num` / `Omni1_num` / `ACTRESS_LIGHT_BASE_MAX_ID`; sorts the +0x1c list
  into `osTotalObjTable` (bss 0x01a59870, 516000 bytes = [3][86][2000]; `objSort` gets [1][scene]).
- **The list draw, main_rtn (slice 3, 2026-11-09):** `main_rtn(bank, mode, chara, flags, charaIndex,
  lastTex)`: banks >= 6 switch the view matrix, `asViewMatrix2PosRUF(matrix_global_view, CameraPos, 0,
  0, 0)`; an empty list (`get_draw_obj` == 1) returns at once. Mode 1 lists are sorted into
  `osTotalObjTable[mode][bank]` for passes (flags & 0x7fff) 0 and 4, blurred parts inserted
  (`ssPartMotionBlurInsertObjToDrawTable`). Fog: `gaSetupFogCtrlData(&fogctrl[bank], 3 floats, 3 ints)`
  ($f12..$f14 + a1..a3) from the bank's FogCtrlAddr record +0x10, or the current scene's +0x28 for a
  character pass; then `gaSet3dlibStruct(50000.0f, ga3dlibwork, &drawbuffer_0, &fogctrl[bank],
  screen_0)` (screen_0, sdata 0x01939d10: 0, 0, 512, 448, 256, 224, 0.832, 0.94, 512, 5, 40000, 2^24-1),
  `Clipping_Portal` when `Clipping_System` == 1, `tnInit3dlibWork(ga3dlibwork, SPR 0x70000400)`. The fog
  record's part (+0x00) with parts flag bit 0 sets `FOG_mode`. Per entry (0x1c bytes: obj, draw-ctrl id,
  tex, parts, -, tbuffer, drawctrl): parts flag bit 0 (fog volume) or `partsPos[parts].w` <= -255 skip;
  bit 3 runs `OneMaxObjectDrawZOnly` once per part; clip unless flags 0x8000; texture upload
  (`sunSetupTBufferData` + `sunBGT_LoadGSTexClutData_VIF1`) and draw-ctrl switch only on change; obj+0x3c
  = parts +0x44; texture kind 3 sets obj+0x30 from All_table +0x70[parts] (character pass: times the
  light bank's All_table +0x28 [2], eased over `DirectLightCharaCounter` frames); a new texture restarts
  the BGT ring (`gaBGT_End` / `gaBGT_Start(bgt_ringbuffer, 0x8000)`) when bank 0 is `MENU.BIN` (bank <
  6) or the texture is <= 32 x 32. `main_rtn_Particle`: the same fog set-up, then each particle system's
  ring slots head .. tail (u16 at state +0x48 / +0x4a, wrap at +0x46), objects `ParticleObject[bank][i]`
  + slot * 0x80. `main_rtn_Particle2` reads a qword pointer at the draw object's +0x38 (open-items (h)).
  screen_0 holds a fixed 512 x 448 viewport: the 3D TU must widen it from DisplayGeometry (any aspect).
- **main.c's globals** are taken to be the .sbss run 0x01942ab0..0x01942b48 (`pad_key_button` ..
  `gArgc`): contiguous, all used by main.c's functions, `gArgv` / `gArgc` surely its own. A guess from
  placement; the locals (symtab 895..911) are `a$1034`, `skip_wait_temp$61`, `focusPos$905` and the
  literals ("JUNC", "SUND", "GLBL", "GAME.BIN" = @2700, the DrawMode jump table @1669).

## Data (2026-11-09)

- `DATA\CT3PACK.DAT` (the ELF names `DATA\CT3PACK.DAT;1` and `\DATA\CT3PACK.DAT;1` at 0x01934b90,
  0x01934c10): one archive; its format is not read yet. Whether its pictures are TIM2 is open.
- `DATA\MOVIE1..3`: movies (`movie.c`; "pts buffer overflow" at 0x01935520 says MPEG timestamps: PSS).
- `game_end.dat` (0x01935308): a file the game opens by name.

## The IOP side (2026-11-09)

The EE loads `cdrom0:\IOP\IOPRP255.IMG;1` (an IOP reboot), then `SIO2MAN`, `PADMAN`, `MCMAN`, `MCSERV`,
`LIBSD`, `SDRDRV`, `MODHSYN`, `MODMIDI`, `MODMSIN`, `FSYS_IOP` (strings 0x01934ca0..0x01934de0) and,
from `sound.c`, `SNDDRV.IRX` (0x01936960). `MODHSYN` / `MODMIDI` / `MODMSIN` are SCE's hardware-synth MIDI
stack; `SDRDRV` serves the EE's sdremote client; `SNDDRV.IRX` (151 KB) is presumably the game's own
driver and `FSYS_IOP.IRX` its file server. None is read yet. No sibling met modhsyn (MioPan, ReiPan,
RosePan, YamiPan all ran a game's own driver over libsd).

## PC platform layer (bootstrap 6, 2026-11-09)

YamiPan's Pan layer, carried over and renamed (cpp-port.md has the map): window, render thread, 2D
canvas, TIM2 texture store, present / HDR grade, screenshots, overlay and panels, input, config, logger,
crash report, EE kernel and interrupts (one EE thread at a time by priority), IOP kernel, SIF RPC
servers, the SPU2 model and voices, the audio device, libmpeg on FFmpeg. Left in YamiPan (FS2's own): the
rena model renderer, smooth motion, the frame passes, the teleport panel. The build's only game code is
the stub `gameMain` (`main`, CT3 0x01839d10) in `src/lib/unassigned_stubs.cpp`.
