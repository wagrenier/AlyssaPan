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
| 0x0189f718..0x018a88e0 | the `ga*` library (its last object `ga/gaInitMeter.c`, GCC, 0x018a8810..0x018a88e0): `gaPad*`, `gaDma*`, `gaVif*`, `gaInit3dlib`, `gaBGT_DrawObject*`, `gaDrawObject_*`, `gaSinCos` / matrices, `gaSearchNearLight4`, `gaDump*`, `gaVsyncCallBack` (asm-shaped objects without `gcc2_compiled.` are hand-written or VU-heavy) |
| 0x018a88e0..0x018d2a68 | game code (`ssCalcInversMatrix2`, `ss*` scene code ..; `gaInitMeter` at 0x018a8810 is the ga library's, 2026-11-09) |
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
- **A bank's set-up, scene_init (slice 6, 2026-11-09):** `FSMltLoadProc` calls `scene_init(bank)` once a
  bank's file is in: it copies the All_table record's +0x14 / +0x2c / +0xbc into `BaseobjCount` /
  `pModelPartsHeader` / `pBehaviorHeader` (sbss 0x01940750 / 0x01940380 / 0x01940208), clears the bank's
  `fogctrl`, takes +0xb8 into `FogCtrlAddr` for bank types 0 and 1 (type 1 also sets the frame clear's
  RGBAQ from the fog colour), sets the 3dlib (far 50000), inits and links the matrix tree, restarts the
  behaviour animations when the parts header's first byte is 1, then the subsystems' inits, and builds
  each draw list entry's (+0x1c, +0x18, +0x60) texture buffer (`sunSetupTBufferData(entry +0x14, *tex)`)
  and draw control (`gaSetupDrawCtrlData(entry +0x18, *entry +0x4)`: a 0x78-byte settings record).
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
- **The scene draw, AllDrawObject (slice 5, 2026-11-09):** `AllDrawObject(mode)` (0 normal, 2 / 3 the DOF
  slices; 3 skips the lights) draws, with `lastTex` restarted where noted: the opaque list (+0x18) of each
  loaded type-1 bank 6..85 under `SceneLightSet`; `asDrawShadowVolumeLoop` (before the characters, or
  after the mirrors when trigger event 0x3b is set); the characters of `CharacterSortBuff` (sbss
  0x01940170, 35 shorts, -1 ends) whose `translucent` byte (+0x52c) is 0 and whose light bank (+0xa0) is
  loaded: `CharLightSet`, their bank's +0x18 then +0x1c lists as a character pass; then back to front,
  each such character whose bank lacks flag 0x10 and whose light bank's All_table record has a mirror
  (`asEnumMirror(tbl)`): `asMirrorPlaneCalc(vPVbk, vPPbk copies)`, `asMakeMirrorMatrixByPlane`,
  `asMirrorCulling(0)`, `asBuildMirrorMatrixTree(tbl +0x2c)`,
  `asGAobjectControlPolygonDisplayRotateFlagReverse(tbl)`, the same two lists, `asMirrorCulling(1)`;
  undone after (flag reverse + `asBuildMirrorMatrixTreeBack`). Then the type-1 banks' +0x1c lists
  (flag-2 banks last), their particles (`main_rtn_Particle` + `_Particle2` mode 0), those of characters
  with flags 0xc0000000 set, and last the translucent characters: bit 0 = `tiChara_Translucence_Start`,
  the lists, `_End(alpha +0x530, ga_draw_fbuf_address)`; bit 1 = `_Start2`, `main_rtn_GHOST` (main_rtn
  with one fixed draw control `gaSetupDrawCtrlData(1,1,1,1,0,0,0)`, always the current scene's character
  fog, always sorted, no camera update; `_GHOST_DOF` likewise from `main_rtn_DOF`), `_End2(fbuf, chara)`.
- **HQ DOF (slice 5):** `ssHQDOF_DrawAllObject` draws the normal frame, then (unless `EffectBuffer_Use`)
  8 more: the eye moved by `HQDOFCameraOffset` / 10 (sdata 30 -> 3.0) along the camera's right / up
  vectors (`asViewMatrix2CamRUF(matrix_global_view, pos, right, up, forward)`): +R, +U, -R, -U, then the
  half diagonals; `asEyeForcusUP2CamMat(screen_0[8], matrix_global_view, eye, focusPos$905, up)`;
  `asBuildDOFMatrixTree(pModelPartsHeader[b])` for every loaded type-1 bank and refracting character;
  drawn into FBP 0xa8000 after the frame clear, blended back by `ssBGT_HQDOF_WriteBackFB((HQDOFOpacity
  << 7) / 100)` (30 % -> 38). focusPos$905 then takes `Cam[asCheckBankID(6)]` +0xf0 (`Cam`, bss
  0x01ae7c40, 7 x 0x1c0).
- **main.c's globals** are taken to be the .sbss run 0x01942ab0..0x01942b48 (`pad_key_button` ..
  `gArgc`): contiguous, all used by main.c's functions, `gArgv` / `gArgc` surely its own. A guess from
  placement; the locals (symtab 895..911) are `a$1034`, `skip_wait_temp$61`, `focusPos$905` and the
  literals ("JUNC", "SUND", "GLBL", "GAME.BIN" = @2700, the DrawMode jump table @1669).

## The ga library's DMA, ga/gaDma.c (2026-11-09)

- A GCC object 0x018a1188..0x018a1738 (16 functions, bss 0x01c8d708..0x01c8d730: `ga_dma_env`, then
  `dmaVif0`, `dmaVif1`, `dmafromSPR`, `dmatoSPR`, `dmaGIF` from sceDmaGetChan 0, 1, 8, 9, 2). The next
  object is hand asm (`gaDmaSync_VIF1.c` in the TU map): its first word, the label `_DmaVIF1_Sync_error`
  (0x018a1738), is `j gaDMAStopMessage_VIF1`, shared by its VIF1 waits that give up after 0x4000000 polls.
- Signatures (the decompiler drops forwarded arguments): `gaDmaSendN_toSPR(spr, addr, qwc, rele)`,
  `gaDmaSendN_fromSPR(addr, spr, qwc, rele)`, `gaDma*I_*SPR(spr|addr, addr|spr, tqwc, sqwc, qwc)` (qwc in
  t0, the 5th argument register), `gaDmaOpen_MFIFO_VIF1(ring, size)`. `rele` << 1 lands on D_CTRL.RELE.
  SADR is written by hand (`chan + 0x80`) as `spr - 0x70000000 | 0x80000000`; VIF0 / GIF sends turn a
  scratchpad address (>= 0x70000000) into that form, other addresses pass unchanged.
- MFIFO: Open suspends the DMAC (D_ENABLEW 0x10000), sets `ga_dma_env` rbadr / rbmsk ((size-1) & ~0xf) and
  **mfd = 1** (sceDmaPutEnv's input; not D_CTRL's MFD encoding, see open-items), clears RCYC and sets
  RELE in D_CTRL, points fromSPR's MADR at the ring and starts VIF1 in chain mode from it (TADR = ring,
  CHCR DIR | MOD chain | STR). Close clears STR on both and mfd.
- **Host:** the scratchpad is `g_eeScratchpad` (sdk/eekernel.h, `SCRATCHPAD(offset)`); libdma's toSPR /
  fromSPR normal and interleave transfers copy for real with SADR a host pointer; VIF / GIF sends and
  chain (`sceDmaSend`) transfers do nothing. D_CTRL / D_ENABLEW are host variables (sdk/eeregs.h).
  main.c's `SPR_3DLIB_WORK` is `SCRATCHPAD(0x400)` since this TU (ga3dlibwork's per-frame copy).

## The ga library's hand-written kicks and the BGT send, ga/gaDmaSync_VIF1.c (2026-11-10)

- Asm object 0x018a1738..0x018a1948 (5 functions + the stray `_DmaVIF1_Sync_error` jump). The syncs
  (`gaDmaSync_VIF1`, `gaDmaSync_GIF`) poll CHCR.STR (GIF: then GIF_STAT & 0x1f000c00, FQC + APATH) with one
  count of 0x4000000 polls and on timeout tail-jump to `gaDMAStopMessage_VIF1 / _GIF` (ga/wait.c) with
  **a0 = the caller's ra** (`move a0,ra`). `gaDmaSendN_VIF1(addr, qwc)`: MADR, QWC, CHCR 0x101;
  `gaDmaSendS_VIF1(tag)`: TADR, QWC 0, CHCR 0x105; both turn a scratchpad address into DMA form (+0x10000000).
- **`gaBGT_DmaSendN_fromSPR_VIF1(addr, qwc)`**: a packet below 0x70000000 is first copied (lq/sq) into the
  scratchpad packet area at GA_SPR_WORK.bgtPacketEE (+0xf0), which then **flips between 0x70000a00 and
  0x70001800** (double buffer); then wait fromSPR idle, SADR = packet + 0x10000000, QWC, wait until the
  ring is empty (D8_MADR == D1_TADR) or `(TADR - MADR) & D_RBSR` > qwc * 16, then CHCR |= STR.
- Host: the channel blocks are `sceDmaGetChan(n)`; kicks write the ROM's fields with STR already clear
  (the transfer is over); the copy and the flip are kept. sdk/eeregs.h gained D_RBSR and GIF_STAT (idle);
  sdk/scetypes.h `EE_RETURN_ADDRESS()` (MSVC has no `__builtin_return_address`).

## The ga library's VIF set-up, ga/gaVifInit.c (2026-11-09)

- A GCC object 0x018a1948..0x018a1a48 (6 functions) with its .data 0x0192e640..0x0192e790: 18 global
  `vifcode_*` qwords named by their content (`vifcode_mscal_0002_flush_mscnt_flushe` = MSCAL 2, FLUSH,
  MSCNT, FLUSHE), the renderer's canned VU kicks. MSCAL entries used: 0, 2, 4, 6, 8, 0xa.
- `gaVifInit` sends `vifcode_init_VIF0` (1 qword: STCYCL 1/1, ITOP 0, STMOD 0, STMASK -- its mask word is
  not in the qword) and `vifcode_init_VIF1` (4 qwords: STCYCL, OFFSET, BASE, ITOP, STMOD 0, MSKPATH3 imm 1,
  STMASK 0, STROW 1.0f x4). `gaVIFSync_VIF0/1` return STAT & 0x0f000007 / 0x1f00000f (FQC, VGW, VEW, VPS);
  `gaVUSync_VU0` polls VIF0_STAT bit 3. Callers: main.c (init, FLUSH then the VIF1 poll), SystemReReset;
  gaVUSync_VU0, gaVifcodeFlushe_VIF1, gaVIFSync_VIF0 have no ROM caller.
- **Host:** VIF0_STAT / VIF1_STAT are host variables that stay 0 (sdk/eeregs.h): the polls return at once.
  The packets go through gaDmaSendN_VIF1 (ga/gaDmaSync_VIF1.c, stubbed until its TU) and are dropped.

## The ga library's draw set-up, ga/gaInitDrawObject.c (2026-11-09, slice 1)

- A GCC object 0x018a1a48..0x018a3078, 24 functions: the GS register packets, the scratchpad work area
  the VU1 renderer reads, and the BGT ("background transfer") ring's start / sync / end.
- **Every ga packet has one shape:** +0x00 DMAtag CNT (qwc = qwords after the tag), +0x08 a kind mark
  `0xeeee000N` in the tag's upper half (0 gsinit and the mark packet, 1 dbuffer, 3 drawctrl, 5 fogctrl),
  +0x10 VIFcodes {NOP, NOP, NOP, DIRECT qwc-1}, +0x20 a GIFtag (PACKED A+D, EOP, NLOOP = pairs), +0x30 the
  A+D pairs. gsinit: COLCLAMP 1, PRMODECONT 0, SCANMSK 0, PABE 0. dbuffer: FRAME_1, ZBUF_1, SCISSOR_1,
  XYOFFSET_1 (written 0x8000 - offset), DTHE; its send rewrites FBP / ZBP from `ga_draw_fbuf_address` /
  `ga_zbuf_address` >> 11. drawctrl: PRMODE, TEST_1, ALPHA_1, FBA_1, TEX1_1, TEXA from 28 words + TEXA
  (`GA_DRAWCTRL_ARGS`, fields named). fogctrl: FOGCOL, plus the three fog floats at +0x40 (not sent).
- `gaSetupDBufferData` takes 14 values (FirstInitialization: a 512-wide PSMCT32 frame at 0, Z at 0,
  scissor 0..511 x 0..447, offset 0); `gaBGT_Start(ring, size)` forwards both to gaDmaOpen_MFIFO_VIF1.
- **The scratchpad layout** (`GA_SPR_WORK`, ga/gaInitDrawObject.h): 0x000.. the renderer's constants
  (view scale, Z scale / offset from the 3dlib work's [10] / [11], 255.0, +-0.5), two triangle GIFtags
  (0x90, 0xa0), +0x14 `ga_div_redraw_gspacket_work` (0x01957e40, 0xb0000 bytes) uncached, +0xec the 3dlib
  work's address, +0xf0 the BGT packet area (0x70000a00), 0x110..0x190 eight VIFcode qwords (MSCAL 6 / 8 /
  0xa, FLUSH / MSCNT / FLUSHE runs); 0x400 the 3dlib work copy (main), 0x800 the mark / sync packet, 0xa00
  the BGT packets. gaInitDrawObject also sends the VU1 microprogram (`vu1_object_draw_program`, .vutext
  0x018ff430, 0x335 qwords).
- `gaBGT_Status` is busy while VIF1_STAT & 0x01f0300f or fromSPR's MADR != VIF1's TADR; `gaBGT_Sync`
  sends an empty REF tag and waits for it.
- **Host:** the packets are built as the ROM builds them and go to gaBGT_DmaSendN_fromSPR_VIF1 (stub until
  ga/gaDmaSync_VIF1.c); the scratchpad keeps its EE layout and its address words hold EE addresses (u_int);
  the microprogram is a zeroed stand-in of its size. MADR / TADR / VIF1_STAT are host variables that read
  idle.
- **Textures (slice 2, 2026-11-09; no caller in the ROM -- the game uses its sun* copies in
  QuatSlerp.c, which likely share this shape):** `gaSetupTBufferData(tbuf, tex, tbp, cbp, clut, wms, wmt,
  minu, maxu, minv, maxv, tfx)` builds a 0x130 `GA_TBUFFER_DATA`: kind 6, TEX0_1 (TCC 1, CLD from tex
  +0x3c, CBP = cbp + clut x the CLUT block: 0x100 words for PSMT8, 0x40 otherwise), CLAMP_1, MIPTBP1_1 /
  MIPTBP2_1 (levels packed after level 0, each >= 0x100 words; widths >= 128 for PSMT8 / PSMT4, >= 64
  else; PSMCT16 is sized at 4 bpp, a ROM slip) and TEXFLUSH; then, unsent, the level and CLUT addresses
  `gaBGT_LoadGSTexClutData_VIF1` uploads. The source (`GA_TEX_DESC`, 0x60): width, height, TW, TH, PSM,
  last mip level (-1 none), 7 level data addresses (+0x1c), CLUT count (+0x38), CLD (+0x3c), 8 CLUT
  addresses (+0x40). An upload is a 9-qword scratchpad packet at 0x70000800: CNT 7 (mark `0xdddd000N`: 0
  / 1 CLUT env / REF, 2 / 3 level env / REF, 0xf TEXFLUSH), `gaBGT_SetLoadGSEnv`'s 7 qwords (BITBLTBUF,
  TRXPOS, TRXREG, TRXDIR 0, IMAGE tag; returns the image's qwords by PSM), then a REF tag to the data.

## The 3D library, ga/gaInit3dlib.c (2026-11-10)

- A GCC object 0x018a7700..0x018a85c8, 25 symtab functions of VU0 macro code (read from mipsdis.py; the
  decompiler cannot follow lqc2 / vmula chains), plus two blocks with no symbol, no Ghidra function and
  no caller at 0x018a77a8 / 0x018a7808 (normalize xyz; the second answers the length): dead code.
- **Matrix convention**: rows, row vectors on the left (v * M = x * M[0] + y * M[1] + z * M[2] + w *
  M[3]). `gaMulMatrix(dst, a, b)` is **dst = b * a** (each row of b through a); gaRotMatrixX/Y/Z(dst,
  angle, src) = src * R(angle); gaRotMatrixXYZ(dst, angles, src) = src * SetRotMatrixXYZ(angles).
  SetRotMatrixXYZ builds Z(angles[2]) * Y(angles[1]) * X(angles[0]) by right-multiplication; the ROM's
  SetRotMatrixZYX starts from **gaSetRotMatrixX(angles[2])**, then Y(angles[1]), then Z(angles[0]).
- `gaInversMatrix` is the rigid inverse (3x3 transposed by MMI PEXTLW / PEXTUW / PCPYLD / PCPYUD, rows'
  w 0; row 3 = -(t * R^T) with t's w kept). `gaContMulMatrix(recs, n)` runs the VU0 micro routines 0x000 /
  0x010 (`vudis.py 18fe500 f3 0 10`: the same product as gaMulMatrix) over 0xc-byte {dst, a, b} records,
  pipelined so record i + 1 is loaded before record i is stored. `gaInit3dlib` DMAs that microprogram
  (0x018fe500, 0xf3 qwords) to VIF0; the host sends a zeroed stand-in and does the products natively.
- **The 3dlib work** (`GA_3DLIB_WORK`, gaInit3dlib.h; ga3dlibwork, main's per-frame copy to 0x70000400):
  gaSet3dlibStruct(clipZ, work, drawbuffer, fogctrl, screen) writes 0x74 bytes: screen W / H and scales,
  zMax by ZBUF.PSM (Z32 4294967040, Z24 16777215, else 65535), the projection distance, the fog floats,
  near / far, the four side planes (secant sqrt(t^2 + d^2) / d at +0x30, tangent +-t / d at +0x40, order
  left, right, top, bottom with t = cx / sx, (w - cx) / sx, (h - cy) / sy, cy / sy), Z and fog scales,
  the subpixel scales (+-16) and the GS offsets ((ofs + 2048 + centre) * 16). screen_0's twelve floats
  are named GA_SCREEN_* from this use.
- **gaSetLight(lightWork, view)**: GA_LIGHT_WORK +0x10 `dirView` = the four directional lights (dirPos)
  under the view's rotation (row 3's xyz cleared), stored as columns, rows x, y, z, -w; +0x50 `omniView`
  = the four point lights' xyz under the whole view, columns, row 3 = their own w. The point-light
  products reuse one qword whose w carries over (0 while the view's w column is 0, 0, 0, 1).
- **gaSetZRollMatrix(roll)**: a 2x2 rotation by roll scaled by the work's scaleX / scaleY (read through
  GA_SPR_WORK.work3dlibEE) into the scratchpad at 0x70000300 ([0] c * sx, [1] s * sy, [4] -s * sx, [5] c *
  sy), for the VU1 renderer. gaCopyObject (0x018a8140) is a no-op in the ROM (it stores into its own stack
  copy); gaCopyObject2 is the copy the game uses.

## The ga helpers, ga/gaSinCos.c (2026-11-10)

- A hand-written asm object 0x018a6b70..0x018a7538 (13 symtab functions), its .data the global
  `ga_sincos_table` (0x0190e640, 0x20000 bytes): 0x4000 {sin, cos} float pairs, entry i = angle i * 4 in
  the ga unit (0x10000 = a full turn). The ROM's floats are not bit-equal to a host sinf (about half
  differ in the last bit; cos at the quarter turn is 1.79e-9), so the port embeds them
  (`src/ga/ga_sincos_table.inc`, generated from the ELF like ah_acos_table.inc).
- **The index**: `((angle + 2) & 0xfffc) >> 2` -- the nearest entry, any int wrapped by its low 16 bits.
  gaSinCos(s, c, angle). gaSetRotMatrixX/Y/Z(m, angle) write the identity, then c / s / -s (the sine's
  sign bit flipped) in the axis's 2x2 (X: [1][1] c, [1][2] s, [2][1] -s, [2][2] c; Y: [0][0] c, [0][2] s,
  [2][0] -s; Z: [0][0] c, [0][1] s, [1][0] -s); an angle that rounds to entry 0 branches to the plain
  identity (no -0.0).
- gaInt / gaFloat are VFTOI0 / VITOF0; gaMemCopy32 / 64 / 128(dst, src, count) are memmove in words /
  dwords / qwords (backwards when dst > src); gaMemFill8(dst, byte, count) is memset.
- **gaProjectScreen(out, m, in, count)** runs the VU0 micro routines 0x020 / 0x028 / 0x038 (`vudis.py
  18fe500 f3 20 28 38`), two points a call, pipelined: p = in * m (w as 1), Q = SPR+0x8c / p.z, out =
  (SPR+0x70 / 16 - 2048 + SPR+0x60 / 16 * p * Q for x, y; SPR+0x78 / 16 + SPR+0x68 * Q; 1) -- GA_SPR_WORK's
  work21 / work22 / work23x16 / work26 / work27 and work6b, i.e. the GS 12.4 coordinates back in pixels
  around the screen centre. count is rounded up to even.
- **gaSetPointLight(lw, m, pos, color, index[4])**: GA_LIGHT_WORK.omniView / omniColor for the lights
  index[] names until NEAR_LIGHT_NONE; omniView's unused columns keep m's rows (rows 0..2) and 1.0 (row
  3); with no light only row 3 = (1, 1, 1, 1) is stored. gaContApplyMatrix(out, m, in, n): out = in * m,
  all four components. gaProjectScreen, gaSetPointLight, gaContApplyMatrix, gaInt, gaFloat,
  gaMemCopy64 and gaMemFill8 have no ROM caller.

## SunAlloc, game/ahalloc.c (2026-11-09)

- The game's heaps: `SUN_HEAP` (EE 0x34 bytes; +0 name, +4 area size, +8 first block, +0xc blocks in use,
  +0x10 the search hint; +0x14..0x34 unread by ahalloc.c) over a fixed area. main's `system_init` makes
  three: gHeapJunc (0x96000 at 0x01bce3c0), gHeapSound (0xe1000 at 0x01aed3c0), gHeapGlobalScene (3 MB at
  EE 0x00400000). Callers use `_SunAlloc(heap, size, align)` / `_SunFree(heap, &ptr)` (34 and ~20 callers:
  file loading, characters, particles, the memory card's saves, sound).
- Blocks (EE header 0x10: tag = the size asked, 0 = free; whole size; prev; next) tile the area in address
  order. `_SunAlloc` raises align to 4, needs AlignedNum(size, align) + header, first fit from the hint
  (prev starts NULL even when the hint is not the first block). `SetToEmptyBlock` aligns by moving the
  block up -- the pad joins the block below, or (no block below) the head becomes a free 0x14-byte pad
  block -- and splits off any rest of 0x14 bytes or more. `_SunFree` merges with free neighbours by a
  4-bit case (prev / prev used / next / next used); freeing the area's first block under a used one leaves
  the hint above it (the ROM's), so that block is found again only after a free lowers the hint.
- `SunAllocUsedBorderCheck` returns where the used part ends (the last block's user bytes if free, else the
  area's end): movie.c and PicPaz.c use it to borrow the heap's tail. `CheckAllocNum0` is empty.
- Host: `SUN_BLOCK` is 0x20 bytes (host pointers; still a multiple of 16, so sizes the game aligns keep
  user data 16-aligned as on the EE); `SUN_BLOCK_MIN` = header + 4.

## Data (2026-11-09)

- `DATA\CT3PACK.DAT` (the ELF names `DATA\CT3PACK.DAT;1` and `\DATA\CT3PACK.DAT;1` at 0x01934b90,
  0x01934c10): one archive, 431,290,368 bytes (210,591 sectors). **Its directory (2026-11-10, FlInit):**
  sector 0 is a header of words, [0] the file count (980), [1] the header's bytes (34816), [2] its
  sectors including sector 0 (17); sectors 1..16 hold 0x20-byte rows (`FL_PACK_INFO`: name[0x14] upper
  case, size in bytes, first sector relative to the pack, sector count). The first rows are EVENT.INF,
  VARIABLE.INF, ALYSSAS.ICO / ALYSSAD.ICO (the card icons), GAME_END.DAT, 0MH0.BIN; the last ARTG_P25.IMG.
  The files' own formats are not read yet. Whether its pictures are TIM2 is open.
- `DATA\MOVIE1..3`: movies (`movie.c`; "pts buffer overflow" at 0x01935520 says MPEG timestamps: PSS).
  The player is SCE's ezmpegstr sample, AA_-prefixed (movie.c slice 2, 2026-11-10): initAll creates the
  decoder thread (`AA_videoDecMain`, priority 3; readMpeg's `AA_switchThread` rotates that queue), hooks
  the vblank (`AA_vblankHandler`) and the GIF transfer end (`AA_handler_endimage`), and registers
  `AA_videoCallback` (M2V) and `AA_pcmCallback` (PCM) on the read ring; readMpeg reads 64 KB a turn,
  demuxes with `sceMpegDemuxPssRing`, lets the pad skip after 16 decoded frames (KEY_MAP[20] or [11] with
  AA_MovieStopOkFlag; [11] alone with DEMO_SKIP_SET, or, with event 0x62 registered, while DEMO_SKIP_FLAG
  is 0) and starts the audio ~41 ms (0xb9533b cycles) after the first picture. VideoDec starts with the
  sceMpeg (+8 frameCount) and keeps the two handler ids at +0xd0 / +0xd4; StrFile +4 is the size, +0x34
  CT3's give-up flag (AA_movie then resets the system: "File Open Error!!"). The host keeps this shape:
  the EE kernel runs the thread and the vblank handler, sdk/libmpeg decodes with FFmpeg.
- `game_end.dat` (0x01935308): a file the game opens by name.

## The boot screen and the ga draw object's data (2026-11-10, host task `alyssapan/rendering/ui_canvas.cpp`)

- **CT3 has no TIM2.** A bank's textures are `SUN_TEX_ATTR` records (QuatSlerp.h) reached from each draw
  list entry (`DRAW_OBJ_ENTRY.tex` -> `TEX_INFO.tim`), raw GS pixels with 32-bit CLUTs (8-bit CLUTs in
  CSM1 order); `tools/reverse/ct3pack.py tex <BANK>.BIN` decodes them to `work/ref/<bank>/`.
- **The first screen is ATLD** (the global title bank at power-on, main2.c): the memory-card check, "Checking...
  Do not remove memory card (8MB) (for PlayStation(R)2) in MEMORY CARD slot 1 or analog controller
  (DUALSHOCK(R)2)." -- a 512x256 PSMT4 texture (white text, dark outline), plus a 128x32 PSMT8 sheet of
  two icons (an arrow and a triangle) drawn with CLUTs 0 and 2. ATLD's ALL_TABLE has only an objTrans
  list (5 entries; objOpaque / objAfter empty), so main_rtn mode 1 draws it, every entry through
  **gaBGT_DrawObject_NoClippingCheck2** -- the ga mesh path, not a sprite path: **CT3 has no 2D packet step
  before the triggers' screens** (the text is a textured quad in world space under the scene camera).
- **The ga draw object** (read from ATLD's data, the drawing TU confirms): +0x00 flags byte (0x20 the
  clipped / billboard path, & 0x1c the light kind: 0 light0, 8 / 0x10 light1), +0x10 a centre (xyzw),
  +0x20 a bounding radius (twice), +0x44 the primitive count (2 for a quad: a strip), +0x48 the vertex
  count (4), +0x50 an index record ({0x00100000, 0x20, 0x00200030, 0x00300010} for the quad), then **four
  VIF unpack blocks the renderer sends as they are**: +0x60 positions (V4-32 floats; the text quad: x
  -845..-525, y -80..80, z 0, w 1), +0x68 STMASK 0x10101010 + UNPACK V2-32 at VU 0x80 (ST, 0.001..0.999), +0x6c
  UNPACK V4-8 at VU 0x100 (colours: (0, 0, 0, 0x7f) for the text, 0x4e alpha for the icons -- the light
  pass makes the final colour), +0x64 UNPACK V4-32 at VU 0x80.. normals (0, 0, -1). The icons sit at x
  -558..-524, y -73..-53. The view comes from camera_init0 / camera_view (stubs): the quads' place on
  screen is the camera's.

## The ga objects' textures and colour (2026-11-11, host task `alyssapan/rendering/ga_atld.cpp`)

- **The texture:** main_rtn's passes set a part's texture up once per new `TEX_INFO.tim`
  (sunSetupTBufferData + sunBGT_LoadGSTexClutData_VIF1) and send its 8-qword TEX0_1 / CLAMP_1 packet
  (sunBGT_ExecTBufferData_VIF1) whenever the part's TEX_INFO changes; every ga object after it draws with
  it. Host: that send hands TEX0, CLAMP, SUN_TEX_ATTR level 0 and the CLUT to `AlyssaPan_DrawGaTexture`;
  the CLUT is `clutData[(TEX0.CBP x 64 - tbuf.cbp) / clutWords]`. Pixels are linear in PSM order (an image
  transfer, no swizzle); CT32 CLUT entries, CSM1 for 8-bit. ATLD's text: 512 x 256 PSMT4, TW 9 TH 8, TFX
  MODULATE, TCC 1, CLAMP 0xff001fff; its icons: 128 x 32 PSMT8, TW 7 TH 5, CLUT 0 / 2.
- **The colour:** the light0 program's vf20 (the qword at obj +0x38's EE address) is the part's
  `ALL_TABLE.partsPos[parts]`: **an ambient RGB offset plus the part's alpha in w**, not a position.
  AmbientAlphaUpdate moves xyz toward `scene_alpha_anim[bank].target` over its steps and copies the part's
  alpha into w; `tnXcmd_Scene_Ambient_Change(steps, r, g, b)` (0x018f1940, an event command) sets the
  target. ATLD's vertex colours are (0, 0, 0, 0x7f) and its partsPos starts at 0: MODULATE draws the text
  black until the event sets the ambient (white text needs 0x80).
- **The x commands (2026-11-11, xcmd_tn.c slice 1):** the trigger script's command 0x514 in
  `ssTrigExecuteActCmd` (0x018d5450) reads a 2-byte length, an index byte and an argc byte, then argc
  NUL-terminated strings from ActData, and calls `ssXcmd_functions[index](argc, argv, bank)` (data
  0x0193a500, 256 code words; `lui 0x194 / addiu -0x5b00`, call at 0x018d6d2c). A zero low byte in v0 means
  "again next frame": the cursor steps back over the command (length + 6). Slot 138 (0x0193a728) is
  tnXcmd_Scene_Ambient_Change, 139 tnXcmd_Char_DirectLight_Change. The port's slot type is `SS_XCMD`
  (game/xcmd_tn.h): every command takes (int argc, char **argv, int bank) and answers int.
- **The window:** the VU1 projection centres on (2304, 2272) in a 512 x 448 frame; ATLD's text quad
  lands at 2086..2521 x 2149..2395 (centred), its icons at 2410..2522 x 2353..2384.

## The ga draw object's send, ga/gaBGT_DrawObject.c (2026-11-10, slice 1)

- An asm object (0x018a3148..0x018a4fd8): every function keeps its state in the scratchpad through
  `t9 = 0x70000000` (GA_SPR_WORK: +0xe0 prim count, +0xe4 vertex count, +0xf0 the packet area, +0x104 the
  packet's end, +0x220 the attr argument, +0x224 ra, +0x240.. s0-s7 saved). Read it with `mipsdis.py`, not
  the decompiler.
- **gaBGT_DrawObject_NoClippingCheck2**: flags & 0x20 -> the billboard path shared with
  gaBGT_DrawObject_NoClippingCheck (0x018a3984, not written yet); else flags & 0x1c == 0 -> nc2 light0,
  anything else -> nc2 light1. **nc2 light2 (0x018a4c78) is dead**: its call at 0x018a3f44 is never
  branched to. After the set-up the packet goes to the BGT ring through D8 (SPR -> memory, waiting for room
  behind VIF1's TADR) and the packet area flips between 0xa00 and 0x1800.
- **The nc2 set-ups' chain**: CNT 15 (UNPACK V4-32 14 qwords to VU1 0x1a0: the scratchpad's 0x60..0x90
  with +0x78 = obj +0x70, +0x80 0, +0x84 vertices, +0x88 prims, +0x90 the GIFtag's NLOOP = prims | EOP;
  obj +0x38's position qword; light1: obj +0x5c's 3 qwords; obj +0x58's 4 qwords; the scratchpad's 0xb0 and
  0xc0 = obj +0x78), then REF tags: light0 colours (+0x6c, (n+3)/4 + 1 qwords: a VIF head qword, then RGBA
  bytes), positions (+0x60, n qwords, the tag's own UNPACK V4-32 to VU 0), MSCAL 0x1a + 2 x (flags & 3),
  ST (+0x68, (n+1)/2 + 1: a head qword, then s t floats); light1 positions, MSCAL 0x22 + 2 x (flags & 3),
  normals (+0x64, n + 1), a scratchpad REF (0x170), colours, the light work (+0x3c, CNT 17 qwords copied
  in), a scratchpad REF (0x130), ST; both: the index record (+0x50: UNPACK V4-16 to 0x17f, (prims + 1) & ~1
  elements), an optional DIRECT upload (attr: halfword qwc, data at +0x10; main_rtn passes 0) and the
  scratchpad REF at 0x120 (FLUSH MSCNT FLUSHE). light1 also stores obj +0x30 at the scratchpad's 0xcc.
- **The index record**: four halfwords per primitive, vertex offsets in qwords (x 0x10); ATLD's quad is
  `0 1 2 0 / 3 2 1 3` -- the fourth repeats the first: read as a triangle each (the VU1 program confirms).
- Host: the set-ups call `AlyssaPan_DrawGaObject` (rendering/alyssapan_drawga.h) with the blocks in place;
  `rendering::GaMeshRecorder` copies a frame's meshes (positions, ST, RGBA, triangles, matrix). Smoke run:
  ATLD's first object arrives as light0, flags 1, 4 vertices, 2 triangles, v0 (-845 80 0), st 1/2048.

## The VU1 object program (vu1_object_draw_program, .vutext 0x018ff430; gaBGT_DrawObject.c slice 2, 2026-11-10)

Read with `vudis.py 18ff430 335 --all` (mind hazards.md's vudis misprints). Micro addresses:
- **Entries** 0x1a / 0x1c / 0x1e / 0x20 (light0, + 2 x variant) all branch to **0x14f** with vi15 = the
  kick routine (variant 0 -> 0x1fe, 1 -> 0x22c, 2 / 3 -> 0x1d9); 0x22..0x28 (light1) to **0x17f** with the
  same vi15 set. Entry 0 (0x2a: vf31, vf29 from VU 0 / 1) is never sent (no reference to
  `vifcode_mscal_0000`): **vf31 stays 0**.
- **The header** at VU 0x1a0 (the set-up's CNT 15): vf16 = scratchpad 0x60 (projection scale x, y, z x 16),
  vf17 = 0x70 (screen centre x, y; obj +0x70's two floats: Z offset, fog base), vf18 = 0x80 (0, vertices,
  prims, w = work[6] the projection distance), vf19 = the GIFtag, vf20 = obj +0x38's qword, then light1's
  light matrix (5..7), the object matrix vf24..vf27 (8..11), vf29 = 0xb0 (work9 fog bias, work25 fog
  scale, 255, work24 fog distance bias), vf30 = 0xc0 (obj +0x78's two floats: the ST offset; w obj +0x30).
  **obj +0x70 / +0x78 are floats, not GS register values.**
- **0x14f (light0)** loads the header and calls **0x04d**: v' = vf24 x + vf25 y + vf26 z + vf27 into the
  vertex's slot (VU 0..), the colour bytes (VU 256..) to floats; [E]. The chain's MSCNT then runs **0x0f5**,
  the projection (wrap-pipelined, two vertices a pass): Q = vf18.w / v'.z; XYZ = vf17.xyz + vf31 + vf16 *
  (v'.x, v'.y, 1) * Q; F = (vf17.w + vf29.x) * 16 + vf29.y / (vf29.w + |v'|^2); RGBA = clamp(colour + vf20,
  0, vf29.z); STQ = (st + vf30.xy) * Q; ftoi0 into VU 0 / 128 / 256 (XYZF2, ST, RGBAQ), then jr vi15.
  ATLD: the screen centre is **(2304, 2272)** window units (2048 + 256, 2048 + 224: a 512 x 448 window).
- **The kick routines** walk the index record at 0x17f (x / y / z halfwords = the three vertices' qword
  offsets x 16; the fourth is unused) and copy each vertex's ST / RGBAQ / XYZF2 after the GIFtag at 0x1bf,
  then XGKICK. Variants 0 and 1 cull: the screen edges' outer product (OPMULA / OPMSUB), z's sign flag (MAC
  0x20) sets the third XYZF2's ADC bit -- variant 0 drops z < 0, variant 1 drops z >= 0; 0x1d9 keeps all.
- 0x0af is the same projection over an index list (the clipped path's); 0x084 is light1's transform (no
  colour conversion); light1's lighting is at 0x296 / 0x40b (via 0x192 / 0x196), not read.
- Host: `rendering::GaMeshRecorder::project` / `cull` do this per object; seams.cpp's DrawGaFrame hands a
  frame's triangles to UiCanvas (squeezed at wide aspects = the projection widened). Smoke run 2026-11-10:
  ATLD's object matrix (obj +0x58) is all zeros (asChangeViewMatrix_forTheBank and the camera are stubs),
  so v'.z = 0 and nothing is drawn.

## The ga draw object's clipped path, ga/gaBGT_DrawObject.c (2026-11-10, slice 3)

- **gaBGT_DrawObject** (0x018a3150, the clipped path) points GA_SPR_WORK 0x20..0x2c at the division
  buffers (ga_div_redraw_index_work 0x01a2be40, ga_div_redraw_vertex_work 0x01a2c640, the cursors
  ga_bigdiv_index_work 0x0193e6c0 / ga_neardiv_index_work 0x0193e8c0), then per light kind (flags & 0x1c:
  0 / 8 / 0x10 = light0 / 1 / 2, anything else a `break`): vertex block at packet + 0xd0 (light0) or + 0xc0
  (lit) -> 0xf4; gaDrawObject_VertexLocalViewWithClippingFlagSet (or _Billboard when flags & 0x20), which
  leaves the vertex block's end in 0xf8; index block at 0xf8 + 0x10 (light0) / + 0x170 (lit) -> 0xfc;
  RemakeIndexWithClippingCheck (or _WithNear when the object's byte +2 & 0x04); if primitives remain
  (0xe0): MakeVertexReferenceTable, the set-up, the D8 copy, the packet flip. The end: SetNear if the near
  cursor moved (near pass only), SetBig if the big cursor moved, then gaDrawObject_Redraw_Polygon always.
  ROM slip: near pass + light2 + not a billboard jumps into light1's chain (0x018a385c `j 0x018a3754`).
- **gaBGT_DrawObject_NoClippingCheck** (0x018a3958) is the same chain with VertexLocalView(_Billboard),
  RemakeIndex, MakeVertexReferenceTableNoClip and no end; light2 goes to light1's chain (its own at
  0x018a3c08 is dead). NoClippingCheck2's billboard objects jump into its body at 0x018a3984.
- **The set-ups** (SetupDma_light0 / 1 / 2; light1 and light2 differ only in the MSCAL REF: scratchpad
  0x110 MSCAL 6 vs 0x180 MSCAL 0xa): 0x80 = vertexCount - 0xe4, 0x84 = vertexCount, 0x88 = 0xe0 (the
  remade prim count); the header is CNT 11 + UNPACK V4-32 10 qwords to VU1 0x1a0 (the 3dlib 0x60..0x90, the
  position, light matrix for lit, scratchpad 0xb0 / 0xc0; no object matrix: the vertices arrive in the local
  view). light0: colours REF at packet + 0xc0, MSCAL 2 in the vertex block's VIF qword, ST REF at 0xf8,
  index qword +0x10 = CNT / MSCNT. Lit: at 0xf8 REF normals (n + 1), REF MSCAL, REF colours, CNT 17 + the
  light work copied in, REF FLUSHE MSCNT FLUSHE, REF ST; index qword +0x10 = MSCNT. The tail at 0x100 (left
  by RemakeIndex): with attr, CNT 1 + FLUSH, REF the upload (halfword qwc at attr, data at attr + 0x10,
  SPR bit if in the scratchpad), then REF FLUSH MSCNT FLUSHE; 0x104 = the end.
- Host: the set-ups build that packet in the scratchpad and hand the object, whole, to the draw module
  (the GPU clips; the EE's big / near division is the GS's workaround); billboards are not handed over yet
  (their local view is not read), light2 is lit as light1. The 15 s smoke run of 2026-11-10 reaches neither
  gaBGT_DrawObject nor the gaDrawObject stubs (gaObjectClippingCheck is a stub; ATLD draws through nc2).

## The file layer, game/ArtoolDebugOpen.c (2026-11-10)

`IopLoadModules` reboots the IOP with `cdrom0:\IOP\IOPRP255.IMG;1` and loads the ten modules from
`gDeviceName` ("cdrom0:\") + `\IOP\*.IRX;1`, then `IOPBindRPC` and `IopLoadCapcomSound(gDeviceName)`.
`FlInit` reads the pack's directory through `sceOpen` / `sceRead` (the IOP's cdrom0: device), then finds
the pack's own first sector with `sceCdSearchFile` and adds it to every row. Reading is a queue of 20
requests (`gReqRd`, `FL_REQ_RD`, 0x70 bytes on the EE): `FlReqOpen(dir, name)` looks the bare name up in
the pack first (`FlFindPacked`, upper case), else keeps "\" + DIR + NAME + ";1" for the IOP's file server
(`IOPGetFileSize` answers the size and the file's first sector). `FlReqRead(handle, buff, startSect,
numSect)` (-1 / -1: from the position to the end), `FlReqClose`, `FlIsExecute` (opening or reading).
`FlProc`, called once a frame by the loaders, steps the request `IR` points at (FL_STATE_*): it reads all
remaining sectors with one `sceCdRead` (into the uncached alias of the buffer), holds back while
`gADSOpenSync` (an ADS stream opening) or `CpGetEEIOPStatus()->+0x50` (the IOP using the drive) is set,
and rotates to the next request after five idle steps. `FlLoad` is the blocking form (FlProc + a vsync
per step). `FlGetLen(dir, name, mode)`: mode != 0 tries the pack by name, else opens the file and seeks
to its end. The host serves the folder as the disc (`io::Disc`): files in DATA\ and IOP\ have sectors
and `sceCdSearchFile` finds them by name; `sceCdRead` finishes inside the call.

## Banks and triggers, game/main2.c (2026-11-10)

Banks 0..5 are global (`GLBL_BANK_MAX`): `TrgScene*` hands them to `TrgGLBL*`. A new game loads INFO
(bank 5), E_GL (3), 0MH0 (1), then the title into bank 0: **ATLD** at the first power-on, **TITL** after
(`PowerOn`); every file is `<4 chars>.BIN` (scenes, shapes) or `.ANM` (behaviours) through
`FSMltLoad(file, kind, bank, arg)`, kind 0 global / 1 scene / 2 shape / 3 behaviour. Drawing a bank
waits for it (`FSMltLoadEndWaitFile`, which answers the bank), switches it on (`FSMltSetDrawBank`),
places it (part 0's `drawOnPos`: origin, a part of another bank, or a joint portal part, kind 0x1b,
matched by the part's hash) and runs `scene_draw_init`; global banks also get `camera_init0` and cut 0.
Enemies 2..6 bring a scene bank of their own (E_02 -> 0x27, E_03 / E_04 -> 0x1e, E_05 -> 0x22) and Alyssa
brings E_00 (0x2e). The self-discard queue (5 x 0xc0 at 0x01942580) swaps a scene bank while the game runs:
queued, the old bank undrawn and ended, the new one loaded, then drawn at the saved place with
`SkipMain = 4`.

## The bank loader, game/fileio.c (2026-11-10, slice 1)

- **BankInfo[86]** (0x01aea360, 0x90 each): +0 state (0 free, 1 loaded = a group's head, -1 loading, a
  group's later banks their place 2..n), +4 groupCount, +8 name, +0x18 type = the FS_LOAD_* kind,
  +0x1c the bank's EE memory, +0x20 drawOn, +0x24 the FSMltLoad handle, +0x28 flags (bit 0: scene set
  up, SceneEndBank calls scene_end). Banks 0..5 are global: their memory comes from the GLBL heap and
  goes back to it in SceneEndBank; banks 6..85 are fixed 256 KB slots from EE 0x00410000 (FSMltInit),
  a file larger than a slot taking a group. Files live in DATA/GLOBAL/ (kind 0), DATA/SCENE/ (1) and
  DATA/PERSONA/ (2 shapes, 3 behaviours) -- FSFileType.
- **FQue[20]** (0x01c833c0, 0x40 each: +0x10 name, +0x30 kind, +0x34 bank, +0x38 count, +0x3c handle):
  row 0 is the file in flight. FSMltLoad drops queued requests whose banks overlap the new one, then
  appends it; a global request goes after the other global ones (FSMltInsertQuePriority). Handles come
  from gHandleCounter, the file layer's counter (ArtoolDebugOpen.c).
- **Waiting** (FSMltLoadEndWaitFile, FSLoadEndWait) runs FileSoundServer and sceGsSyncV in a loop and
  puts ga_main_timer back afterwards: a load does not count as game time.
- **sPreLoad[6]** (0x01942b80): FSLoad's whole-file queue, independent of the banks: FSLoadProc reads
  all but the last sector straight into the caller's buffer and the last one into BufLastSector
  (0x01c643c0), copying only the file's bytes.
- **FileSoundServer**, once a frame: FlProc, FSMltLoadProc, FSLoadProc, SdProc, SSProc.
- **FSMltLoadProc** (slice 2, 2026-11-10; steps FSMLT_* in fileio.h, the ROM's numbers 0..12, 0x14): a
  request already in its bank (loaded or loading, same name and bank) only takes the new handle. Else
  SceneEndBank + ReserveBank (the banks become state -(place), groupCount = count - place + 1), the path
  FQUE.dir (FSFileType) + name, open (retried each frame), sector 0 into Buf1stSector (0x01c82bc0). A
  request without bank / count takes the file's (plain +0x1e / +0x20, packed +0x7f0 / +0x7f2). A global
  bank's image comes from the GLBL heap (AlignedNum(size, 0x800), align 0x40). Plain files are read
  straight into the image; **packed files** (+4 = 'CT3X' 0x43543358; pCT3H 0x01939e90 = &Buf1stSector;
  +8 unpacked size, +0xc escape byte, +0x18 246 words for codes 10..255, +0x3f0 256 words for code 9)
  are read 60 sectors at a time into ExtBuf (0x01c64bc0, right before Buf1stSector) and unpacked by
  **Extract**: escape 0 = the escape byte, 1 n b = n bytes, 2 n s = n shorts, 3 n w = n words, 4 d =
  previous word + (signed) d, 9 b = byteWords[b], n >= 10 = words[n - 10]; 5..8 are errors; a code cut
  by a read's end waits in ExtBufTmp (12 bytes). The unpacked image gets bank / count at +0x1e / +0x20.
  Done: an optional byte sum (+0x24 / +0x28 for "BHVR" files from +0x40; +0x44 / +0x48 for others from
  +0x80; marker 0x77777777) -- a mismatch reads the file again; then ahRelocateFileObject(image, shape or
  behaviour), states 1..n, and for any kind but behaviour: All_table_TBL[bank] = image + 0x80,
  scene_init, flags |= 1, (scene) sFirstCheck = 0, BuildMatrixTree(bank, 1, 1), (global / scene)
  SSLoadSEfromLoadPoint(image). A read error ("LOAD RETRY %s") frees the banks and starts over.

## The file relocator, game/ahRelocateSunPs2ObjectArry.c (2026-11-10, slice 1)

`ahRelocateFileObject` (0x0183e710, 0x3b80 bytes), `ahRelocateBehavior` (0x0183e440) and
`ahRelocateSunPs2ObjectArry` (0x0183e280) relocate a loaded CT3X file in place. Header: +4 the kind
(`BHVR` 0x52564842, `SHPE` 0x45504853, `ROOM` 0x4d4f4f52; anything else is left alone), +8 the EE base the
file's pointers are linked for; the relocator adds (load address - base) to every non-zero pointer word,
walking through the pointers it has just fixed, then writes the load address to +8 (a second call is a
no-op). BHVR: a 0-ended list of behaviours from +0x40. SHPE / ROOM: the ALL_TABLE at +0x80; every
ALL_TABLE word 0x10..0xe0 it touches is a pointer except the counts +0x14 (baseobjCount, the count of
the +0x48 word list), +0x38 / +0x3c (the counts of the +0x30 / +0x34 lists) -- so the whole table is EE
words. Slots (ALL_TABLE offsets): 0x10 partsPos, 0x18 / 0x1c objOpaque / objTrans (0-ended lists of
sun object records: +0 the ga draw object, whose +0x50, +0x60..+0x6c are pointers; +4, +8, +0x14, +0x18),
0x20 a record with a persona form (0x2c0 bytes for scenes, short form with byte counts +0x18 / +0x19 for
shape / behaviour files), 0x24, 0x28 light, 0x2c partsHeader (short(+2) MODEL_PARTS of 0x380, +0x44 attr),
0x30 / 0x34 / 0x40 / 0x44 / 0x48, 0x4c, 0x50, 0x54 sound, 0x58 particles (PARTICLE_ENTRY: +0 draw object,
+0xc a record whose +0x2c points to a shared block relocated once, flagged by its +4 = 1), 0x5c, 0x60
objAfter, 0x64, 0x68 and 0xa0 (grids of u_short(+4) x u_short(+6) cells), 0x6c (seven words and eight
{count, list} key tables of 0x10 records), 0x70 partsLight, 0x74 / 0x80 / 0x84 (-1-ended records of 0x28 /
0x28 / 0x24), 0x78 / 0x7c particles2 / particles2After (records with a ring of u_short(+0x44) 0x80-byte
records at +0x5c, each with a draw object at +0x68; +0x30 a shared block flagged at its +0xc; only the
After list's records relocate +0x64), 0x88, 0x8c, 0x90, 0x94, 0x98; scenes only: 0xb8 fog, 0xbc behavior
(ahRelocateBehavior), 0xc0, 0xc4, 0xcc, 0xd4, 0xd8, 0xdc, 0xe0, and +0xe8 cleared; shape / behaviour files
relocate 0xb8 and a different 0xc0 record. A behaviour: short(+8) groups of 8 bytes from +0x10, each
u_short(+2) tracks of 0x20 bytes at its +4; a track's +9 byte is its kind ('H', '>', '=') and its keys
(0x18 bytes: +4 length, +8 pointer) are walked by the lengths. PORT: the host relocates to
BankImageEE[bank] (the bank found by its BankImage[]), so the words stay EE addresses (open-items.md 1 h);
`RelocPtr` maps them back into the image during the walk.

## Animation and shadows, game/ahRelocateSunPs2ObjectArry.c (2026-11-10, slice 2)

The same object holds the animation pass. Each frame main calls asUpDateGAobjectsAnimation (first
Execute_ExecuteAnimationHistory, then a 0x4000-byte SkinWork block from gHeapJunc, asMakeStraw(tbl +0x50
`straw`, tbl +0x2c partsHeader) for every animating bank and for every character with both flags
0x80000000 and 0x40000000, then the block freed) and asUpDateSceneAnimation (asSeekBarAnimation,
ExecuteAnimation(tbl +0xbc behavior, tbl, 0, 1, 0, 0, 0), BuildMatrixTree(bank, 2, 0), asSearchObject).
A bank animates when it is loaded, drawn (drawOn 1) or BANK_FLAG_DRAW_FIRST, of type 0 or 1, and
asCheckAnimationExecutable_Scene says so: type 0 always; otherwise while the world moves (asCheckNowMoving:
pause 0 or PAUSE_RUNNING, no pause_reservation) unless BANK_FLAG_ANIM_FORCE_STOP (4), and while paused
only with BANK_FLAG_ANIM_FORCE_RUN (8). asForceOperateAnimation_Scene(bank, mode) sets them: mode 1 stop
(+ 0x20 STOPPED), 2 run, 0 release; _Character passes the character's bank.

ExecuteAnimation(behavior, tbl, mat, play, replay, matOnly, atKey) (slice 8) dispatches on its three
byte flags: play alone -> EA_100 (the scene pass above: records the call for the history, then every
running track's key at this frame's ticks into its group's part), replay alone -> EA_010 (the history
replay), matOnly alone -> EA_001; any other mix runs a general body that is the three in one. A
behaviour is groups (part index, tracks) of 0x20-byte tracks; a track's keys are `runs` records of
(length & 0x1ffffff) bytes after record 0, which InterpolateAnimationKey overwrites with the key at the
track's time (first word of each record: its frame; time counts frames * 100). The parts header's
words +0x20 / +0x24 / +0x28 are ticks left / direction / step length: while ticks are left a key is
blended into the part's pose (weights (ticks - this frame's) / step and its complement), else set.
Pose tracks land in the part at 0x280 (local rotation rows), 0x2b0 (local position), 0x2c0 / 0x2f0
(base rotation / position = drawOnPos) and 0x300 (kind 0x0d's vector), each setting bit 0 of byte 0x43.
BankInfo[] +0x30..0x70 is the bank's shadow (asShadowDrawing_Init): +0x30 a vector (0,0,0,1) whose w
asShadowDrawing_Disable clears, +0x54 the attached light position, +0x5c 500, +0x60 RGBA, +0x64 200, +0x68
100, +0x6c a signed order byte (asShadowDrawing_CheckTransMode sorts > 0 / <= 0 apart), +0x6e "draw this
shadow". CAMERA +0 is the view matrix: asGetShadowMatrixGlobalView copies Cam[6]'s into the local
matrix_global_view_for_shadow. asInitMirrorMatrix sets mrr_ReverseMatrix to identity (vf0 rotated by
vmr32). asGetBeziePosition is a cubic Bezier with weights t^3, 3t^2 s, 3ts^2, s^3.

Seek bars and seats (slice 3, 2026-11-10). ALL_TABLE +0xc0 is a table of controlled parts (+8 a list of
8-byte {ends, kind, part}, +0xe its count); scene_init's asSeekBarInitializeForX copies the kind-2 ones
into 0x40-byte SEEKBARs (_SunAlloc'd, kept at ALL_TABLE +0xe8). Each animated frame asSeekBarAnimation
transforms a bar's two ends by its part's world matrix, crosses that line with a plane (a part's
position: the bank's own part, or a character's part found by hash while the character is active),
takes t = the crossing's distance along the bar (first non-zero axis), clamped to [0, 1], and starts
each of the bar's 12-byte animations at frame (int)(B t + A (1 - t)) via asSceneAnimationStart. Seats:
ALL_TABLE +0xdc {count, rows}, rows of 0x10 bytes (+4 cells, +0xe their count), cells of 0x10 bytes
whose bytes 2..3 (home) are copied to 4..5 and 6..7 at init. Model part records start at parts header
+ 0x40, 0x380 each, ended by a negative index; BMT_01 (BuildMatrixTree(bank, 0, 1)) copies each
non-kind-1 record's position (+0xf0) to +0x2f0 and, unless record +0x34c is set, builds record +0x340 =
view * matrix (+0x180) and +0x300 = that * diag(record +0x2c0 xyz).

## Double buffer, seats, searching parts, game/ahRelocateSunPs2ObjectArry.c (2026-11-10, slice 5)

- **The double buffer** (`asCopyDoubleBuffer`, main after the draw): each part record's view matrices
  (+0x300 draw, +0x340 view; 0x80 bytes) are copied to the matrices its ga draw object reads (+0x100,
  +0x140), skipping static parts (kind 1) and parts whose copied view hold (+0x14c, row 0 w of +0x140)
  is set. A scene bank whose `drawOn` has just become 1 (BANK_INFO +0x80 `drawOnPrev`), or a character
  that has just taken CHARACTER_FLAG_SHADOW (CHARACTER +0x8c `flagsPrev`), gets mode 1 instead: both ga
  matrices zeroed but +0x130 = `gNoDraw$117` (1e8, 1e8, 1e8, 0) -- last frame's matrices never draw.
- **BMT_20** is BuildMatrixTree(bank, 2, 0) unrolled: no view pass except under a link part; the one
  difference from BuildMatrixTree's own loop: a watch part turns only when its target's w != 0.
- **Seats** (ALL_TABLE +0xdc): a row's +0 grid of bytes (+0xa width, +0xc size in 16-byte units, +8
  the row's part); a cell's +0 part, +4/+5 grid place, +6/+7 second place, +0xc == 1 uses both.
  `asSeatArrange_FlameInit` keeps grid bits 0..1 and marks the 4 x 4 squares under each cell with bit 2
  (row 0's grid and width for every row); `asSeatArrange_ObjectMatchGlid` puts a cell's part at the
  row part's position + x axis * (x + 1.5) * 25 + z axis * (y + 1.5) * 25 (REC_BASE row 3).
- **Searching parts** (ALL_TABLE +0xd8, `SEARCH_OBJ` 0x50 bytes): a part (+0xc) of an owner character
  (+0xe) follows the nearest target (this bank's parts, other characters' parts by hash) seen from the
  owner's eye part (hash +0x10) inside a cone about eye -> rest part (hash +0x18; +0x22 half angle) and
  a range (+0x20); else it goes back to the rest part. Straight at +0x28 per 5 ticks, or (+0x24 != 0)
  turning about the eye by at most +0x26 a frame. The part's w is 1 while it sees its goal.
- **The animation history** (`lpEAS`, `iEASidx`): Execute_ExecuteAnimationHistory plays each record's
  animation with the parts header +0x20 / +0x24 (|sign|, sign) and +0x28 (step) set around it, and for an
  all-ones animation copies each attached part's parent word into ALL_TABLE.ctrl's first list (+0, count
  +0xc, 0x20-byte entries: +8 parent, +0xe part). `FrameCounter` (0x0194494c) counts its calls.

## The shadow pass order and scene animation starts, game/ahRelocateSunPs2ObjectArry.c (2026-11-10, slice 10)

- **The shadow pass order** (asShadowDrawing_CheckTransMode, statics `BID$2565` 0x0193c610 u_short[256],
  `BidxB$2566` 0x0193c604, `BidxW$2567` 0x0193c600): mode 0 lists the drawn scene banks (BANK_TYPE_SHADOW,
  shadowDraw on) and then the banks of the shadowed, opaque characters; shadowOrder > 0 fills BID from
  the front, <= 0 from the back, 0xff marks empty slots. Mode 2 hands out the next bank (front first,
  then the back from the end; iShadowTransOrderBW reverses that), -1 when both are spent. Modes 0 / 1
  answer iShadowTransModeFin: 2 with asShadowDrawing_SetTransMode(1), else 1 (open item dn).
  asShadowDrawing_SetShadowLevel sets the shadow's alpha (BankInfo +0x63).
- **asSceneAnimationStart(speed, hash, tbl, from, to, partId, pose, other, byPart, started, toEnd,
  stepFromRuns, target, chara)**: target 1 starts the scene behaviour's tracks of the part's groups whose
  kind the three switches pick (pose: 0x0b / 0x0c / 0x0d / 0x1f / 0x20 / '>' / '='; other: '*' 'H' '"'
  '!' ')'; byPart: '4' / '3' whose first key names the part at +0x1a / +0x1c, and part 0's groups): range,
  flag bits 0 / 1, playRate = speed * 100 (0 with toEnd), pass check, start; answers 0 when one started.
  Target 2 sends the character a motion request by hash, or with toEnd by its motion-table index.
- **asUpdateAnimation_Matrix** (main, after the animations): BuildMatrixTree(bank, 0, 1) for each drawn
  (or BANK_FLAG_DRAW_FIRST) scene bank of type 0 / 1 and each refracting shadowed character's bank, the
  character's refractAxis (+0x4b0) from row 1 or row 2 of one of two parts' world matrices, then
  asCopyDoubleBufferL2W for the drawn banks of type 0 / 1 / 2, and asResetCharacterRotation(0, 0, 0).

## The shadow volume's build, game/ahRelocateSunPs2ObjectArry.c (2026-11-10, slice 13)

- **The shadow table** (ALL_TABLE.shadow, 7 words then 8 (count, caster list) pairs): 0 the volume's ga
  objects, 1 their positions (0x80 an object), 2 their normals (a 16-byte UNPACK V4-32 header, then 8
  qwords: 0x90 an object), 3 near vertices, 4 far vertices, 5 light rays, 6 edge planes; the pair the
  shadow level (BankInfo +0x63, shadowColor[3], 0..7) picks lists SHADOW_CASTERs (0x10: part, polygon
  count, polygons, vertex count, hide part, vertices). BankInfo +0x40 is the shadow's anchor.
- **asShadowPolygonPositions(bank)** (main): shadowFrame = FrameCounter, shadowDraw |= 1,
  iPleaseShadowDraw; the light = the attached position's qword (shadowLightPos), for a shape bank
  (FS_LOAD_SHAPE) with the angle lock (+0x6d) a point shadow68 from (anchor + shadow64 up) toward the
  light, level with it. Casters last to first, skipped while the hide part's REC_FLAGS bit 0 is set,
  placed by REC_MATRIX_PREV * diag(REC_SCALE). Polygons: vertices pushed shadowLength away from the
  light; each face whose normal (v0 - v1) x (v0 - v2) points along the light's ray gives its edges' quads. A segment (no
  polygons: two qwords whose w is radius * 10 | points << 16) gives a ring: two half rings about its
  ends turned about the light ray, or end-on one full ring. A quad (far a, near a, near b, far b) is a
  plus face when the camera (asViewMatrix2PosRUF of matrix_global_view_for_shadow) and the caster's
  centre lie on the same side of the plane through the light and the edge, else a minus face.
- **asStockShadowVolume(bank, quad, mode)**: 0 begins at the table's buffers, 1 / -1 take a plus / minus
  quad, two of a sign making a ShadowVolume2 object (4 vertices, 8 indices; kinds 1 / 2), 2 flushes a
  lone plus quad as kind 5 and a lone minus quad as the list's last, kind 6 (ShadowVolume1: 2 / 4).
  Each object: gaCopyObject2 of the template (sdata 0x01937ba0), its buffers, the face normal per vertex.
  Plus faces draw subtracting, minus faces adding (asDrawShadowVolume).
- **asCheckAnimationState_S** answers what asSceneAnimationStop_S would stop: 1 playing, 2 stop requested,
  0 none (a character: its motion slots, then its 42-entry request queue at +0x3b0 / modes +0x404);
  **asGetSceneAnimationCurrent** a track's time in frames * 2, + 1 while it plays.

## The matrix tree set-up and the game clock, game/LinkMatrixTree.c (2026-11-10)

- **InitMatrixTree(header)** (scene_init, data_load_set, asJointParts_byMatrix): for each record
  (header + 0x40 + i * 0x380, ended by a negative depth) copy the file's values into the working fields:
  +0x80 -> REC_BASE (+0x280), +0x40 -> REC_OFFSET (+0x240), +0xc0 -> REC_SCALE (+0x2c0, not kind 3),
  +0xd0 -> +0x2d0; the w words +0x4c / +0x5c / +0x6c (REC_PREV_SCALE_*) and +0x8c (REC_PARENT) sit
  inside those matrices and copy as 0; REC_BLEND (+0x7c) = 1, REC_MATRIX_PREV (+0x1c0) = unit; then
  asCopyDoubleBufferL2V(header, 1).
- **LinkMatrixTree(lists, header)** right after: two depth stacks (0x80 bytes each on the ROM's stack)
  give every record its REC_PARENT -- the parent's REC_LOCAL (+0x200; matUnit for a root), or for a static
  part (kind 1) the REC_MATRIX (+0x180) of the nearest non-static ancestor; REC_VIEW_HOLD (+0x34c) = 1.
  Then the three draw lists (ALL_TABLE.objOpaque on, 0-ended): each entry's ga draw object gets +0x58 /
  +0x5c = its part's record +0x100 / +0x140 (a static part: its parent's), and that part's
  REC_VIEW_HOLD = 0; lights (4, 5), look-at parts (0x14) and kinds 0xd, 0x11, 0x15, 0x16 get 0 too.
  On the host the words are EE addresses (MATUNIT_EE or BankEeAddr; decision h).
- **The clock**: 5 ticks per VSync of ga_main_timer. ahTimeMaintenance(paused) once a frame: the scene
  clock (gNextTicksInterval / gNextTicks / gTotalTicks) only when not paused, the global clock
  (gGlobal*) always; statics last_frame (0x0193cb14) and Globallast_frame (0x0193cb04).

## Part lookup, name hashes, the camera read-back, game/QuatSlerp.c (2026-11-10, slice 1)

- **Names are hashed, not stored** (ahGetHash, 0x0185e580): hash ^= rotl64(c & 0x7f, 7 i mod 64) over
  the characters. A name of up to 9 characters never overlaps itself, so asConvertHash2String
  (0x0185e420) reads it back: 0x3161c34c2 = "Bip01", 0x74dda77d0cbcf0c2 = "BasePoint". A room is the
  hash of its files' first four characters (GetRoomName_file); CHARACTER.roomHash holds one, and
  RoomNameTbl / RoomIdTbl (0x019395b0 / 0x01939730, 95 rooms, "0A01".."5A04") give each room a trigger
  event (1801..1925) that RoomInFlagOn registers and RoomFlagClear removes.
- **The parts header** (pModelPartsHeader[bank]): +2 u_short part count, +0x10..+0x1c four int cache
  words for "Bip01", "BasePoint" and two longer names (0 = not looked up, -1 = absent, > 0 = index),
  records of 0x380 bytes from +0x40 with the u_long hash at record +8. asFindModelParts answers the
  record (header + 0x40 + i * 0x380).
- **Banks 0..5 are global**: asCheckBankID folds 6.. to 6, asCheckBankType answers bank > 5.
- **The camera from a view matrix** (asViewMatrix2PosRUF): the rotation's inverse rows are right / up
  / front, the position is -translation through it; asViewMatrix2CamRUF moves the eye back by
  3 * asFocalLength_PS22LEICA(screen_0[8]) (512 * 4.3267 / 680.33 = 3.26 per focal, so about 9.8 units);
  CameraMatrix2CameraTrg3m is the point 300 ahead (the name says 3 m, so the world unit is likely the cm).
- **Angles** are 0x10000 per turn; ahAcos interpolates the ROM's 4098-entry ah_acos_table below 0.9988
  and calls acosf above (that branch returns acosf's own 16 bits, also for a negative cosine).

## The camera, game/asEyeForcusUP2CamMat.c (2026-11-10, slice 1)

- **Seven cameras** (Cam[7], 0x1c0 bytes): one per global bank 0..5, Cam[6] shared by every scene bank
  (asCheckBankID). Each holds the view (+0), view2 (+0x40: the drawing view --
  asChangeViewMatrix_forTheBank copies it into matrix_global_view and turns focal[3], LEICA mm, into
  screen_0[8]), viewPrev (+0x80: view2 before the last cut, unless iCamChangeFlag), then eye / focus / up
  each live / from / to (+0xc0..+0x14c) and four focal lengths (+0x150, 2.88 at init).
- **A cut** (CamChange_S(bank, cut, mode, arg)): -> 1 for no bank, 5 when the camera already shows that
  cut of that bank. The first scene-bank cut after camera_init0 skips that test once and holds main
  for 2 more frames (SkipMain += 2). CamChangeSub then needs the bank loaded and its control table
  (ALL_TABLE +0xc0: +0 the cut records, +4 the target records, +0xc the cut count) to have the cut;
  it copies live -> from, keeps the table's three EE words in the camera (+0x16c..+0x174), takes the
  cut's DOF level (+0x1a: 4 = none; any other turns DrawMode to DOF for a scene bank), its eye kind
  (+0x18) and target mode (+0x17), and has asGetDestinationEYE / _TRG fill the "to" set; a scene bank's
  cut registers trigger event 0xb3.
- **Cut records** are 0x20 bytes (+0 part hash, +0x10 character slot, +0x12 "on a character", +0x13..15
  three target indices, +0x17 target mode, +0x18 eye kind, +0x1a DOF level); **target records** 0x10
  (+0 part hash, +8 part index in the camera's bank, +0xa character slot, +0xc "on a character"). A
  character's part is looked up again into lpCopyMP[slot] (asFindModelParts) when its hash changed;
  a part's world position is its record +0xf0. Target mode 1 looks at one target, 3 takes x / y / z
  from three, 0 / 2 / 4 leave the focus.
- **The view builder** (asEyeForcusUP2CamMat): front = |focus - eye|, right = |up x front|, up' =
  |front x right|, then asCamRUF2ViewMatrix with screen_0[8] swapped for the call; 0 when eye == focus.
- camera_init: AlissaEyeHeight 135, CameraBehind 250, FOV_deg 40 (screen_0[8] = asFocalLength_LEICA2PS2(40)),
  every camera reset (identity view, DOF level 4, cut / bank -1).
- **The frame's camera** (camera_view, slice 2): CameraError sets matUnit as the view and stops. Else
  asCopyCameraMatrix keeps each view as view2 and focal[0] as focal[3] (**the drawing view lags the
  built one by a frame**), a stocked cut is retried (CamChange_Hash_S_Hash stocks a cut by eye-part hash
  in CamStock while its bank is not loaded and drawn), and every camera whose bank is loaded runs
  camera_view0; a scene bank whose animation runs also gets the look-around stick (CameGri_Mode 0: pad
  stick bytes 2 / 3, 1: bytes 0 / 1; dead zone the s_char at KEY_MAP + 0x44; deflection eased by
  (1 - cos) / 4 into CameGri_dvsX / _dvsZ, turned by CameGri_DU about up and CameGri_DR about right) and
  the three CamQuake shakes (0x14 bytes: amp, period, fade in / out, duration, time, phase; along the
  view's right / up / forward); a stopped one runs camera_view0 once when its b182 asks. Then Cam[6] is
  made the drawing view, and iCameraMode 0 / 1 builds a follow view behind Character[0] (CameraAngle
  ints: [0] pitch, [1] yaw; AlissaEyeHeight up, CameraBehind back), 2 the same from the ND* set, 4 gives
  a scene bank without a cut cut 0 (then the ND view when iNeoCameraLock).
- **camera_view0** (a camera's frame): screen_0[8] from focal[0]; moveTime += the frame's ticks; the eye
  / target kinds are the cut's while eyeMode is 0xff (the move), else eyeMode / trgMode;
  asGetDestinationEYE / _TRG give eye / focus / up / focal; during the move they blend from the *From
  set by w = moveTime / moveTicks (changeMode 2: (1 - cos) / 2), and at its end asCamModeChange makes the
  cut's kinds eyeMode / trgMode. Kinds 0, 1, 3..6 set the *To set; kind 2 keeps the eye's distance from
  the focus raised by ZelCam[3] within ZelCam[1]..[0]. The live set moves toward *To by
  asCameraPositionTranser at the cut's moveSpeed (+0x1b; 0 jumps, the step grows with the remaining
  distance up to 360 and with 20 / speed), then asEyeForcusUP2CamMat builds the view.
- **Where a cut puts the eye** (asGetDestinationEYE, slice 3): the focus comes from the cut's target list
  first (every kind), the up is the eye part's z row (part record +0x1a0) normalized when the part's type
  byte (+2) is 3, else (0, 1, 0, 0), the focal length is the cut's float +8. The eye part is the bank's
  record (+0xe) or the character's lpCopyMP (+0x10, +0x12 = 1). A part record's world matrix is at +0x180
  (rows x, y, z, pos). The cut's **path** is the OBJ_CTRL record at cut +0x19 of the camera's ctrlList
  (0xff = none): its ends word points at float[4] points in its part's space, its kind short is the point
  count (< 0: closed). Kinds: 0 the eye part's position (no target: looks 300 along its y row, or along
  Character[part].rot's z row); 1 RAIL: per path segment asGetLineRadTargetPos at the cut's angle (+0xc),
  the answer nearest the focus; 2 ORBIT: ZelCam[1] from the focus raised by ZelCam[3], toward the live eye
  or (ZelCam[5] == 1) at ZelCam[4]'s angle around the target part's flattened z row; 3 STAY: the live eye
  (answer 1, then _TRG runs); 4 PLANE: a closed quad path (count -4), the eye on it along the part's axis
  (y row for type 3, else z) * -300 and the focus that far in front; 5 RING: point 0's x = radius, y =
  tolerance, the eye at the eye part's height, horizontally toward it; 6 SEGMENT: the point of the first
  segment at the cut's +0xc distance from the focus (the root nearer the far end; none: the line's
  closest point pushed out). Answers 0 placed, 2 no target (mode ONE / AXES needed; ORBIT: ONE), 3 / 4 /
  5 / 6 the path does not fit, -1 an unknown kind. CamChangeZELCAM_S (a trigger command) sets ZelCam
  ([4] / [5] are int words), CamSetNowDOFLevel Cam[6]'s DOF level (4 ends the DOF pass).
- **camera_main(pad, mode)** (slice 4) sets GlobalCameraMode and iCameraMode. Modes 0 / 1 (debug):
  Alyssa flies -- the d-pad and L1 / L2 give a view-space step through 5-entry pressure tables
  (foword / side 4..12, length 2..6; a pressure byte's bits 6..7 pick the pair, bits 0..5 blend in 64ths),
  the right stick adds to it, the left stick and the face buttons (turn_angle_table 16..256) turn
  CameraAngle, R1 / R2 scale CameraBehind and AlissaEyeHeight by 0.95 / 1.125; the step turns by the yaw
  (mode 1: and the pitch) and moves Character[0].pos, whose rot takes the yaw + 0x8000. Mode 2: the ND
  camera (NDAlyssaPos starts at Alyssa; R1 cycles flat / vertical / drive; drive moves Character[NDiChar]
  along its own turn, R2 steps NDiChar, triangle / cross raise NDCameraEyePoint). Mode 3 becomes 4 with
  controller_num 4 and iSm 0. Mode 4, the game's: a mode change asks motion 0; in controller mode 100 L1 /
  L2 step iSm through the scene bank's cuts (CamChange {300, scene, iSm - 1, 1}, "Camera ID %d" into
  szCamModeAdd). Every time step is gGlobalNextTicksInterval / 5; stick bytes 0x53..0xad read as 120.

## Areas, rattles, warps, the slerp, game/QuatSlerp.c (2026-11-10, slice 2)

- **Areas** (GetAreaNumber): a room's area number 1..52 (the save file's area, the map / scene name
  menus) from RoomNameID_STR_Tbl / RoomNameID_ID_Tbl (0x019392f0 / 0x01939110, 118 rows). Rooms listed
  two or three times (0A01, 0A10, 0A11, 1C01, 1B08, ...) answer their later rows while the trigger
  events 0x5db / 0x5da are set (later story passes); stage 3 makes 0A11 / 0A10 / 0A01 area 52, stage 4
  makes 0A16 area 47. A character with lightBank -1 is in no area (-1).
- **Controller rattles** (ClatteringData, 10 x 0x20): the struggle the ghost / boss attacks, the moths
  and the rapid lever start. Each frame a button press on controller 1, or a 0x60 swing of any analog
  byte (PAD_DATA +0x1c..0x1f) from the last counted position, adds one; decayFrames idle frames take one
  back; count >= goal wins (state 1), timeLeft reaching 0 loses (state 2). Defaults 150 frames, decay 10,
  goal 50. ControllerClatteringGet answers the state and frees a finished slot.
- **Warps** (WarpObject, the trigger scripts' warp command): a character goes to a part (marker) of a
  room's scene bank. A loaded room queues the command (asWarpObjectBody(0), a block in gHeapJunc that
  grows by 16); main runs the queue with asWarpObjectBody(1) at a safe point of the frame. A room not
  loaded parks the command in ReservationWarpCommand (10 slots); WarpObjectUpdate queues it once the
  room's bank is loaded and drawn. The player's parked warps take Character[11] / [12] along when their
  flags have bit 31 (CHARACTER_FLAG_REFRACTION: so that bit is more than the refraction pass -- maybe
  "follows the player"). The execution (ExecuteWarpObject / 2) is not written yet.
- **QuatSlerp(q, q0, q1, ts)**: ts = {1 - t, t}; q0 negated while the dot is negative; dot >=
  0.9999998 answers q1; ts[3] is overwritten with the angle (0x10000 a turn) as a float; the weights are
  ahSin(ts[i] * angle) / ahSin(angle) and the result is renormalised. asMakeRotMatrixByQuartanion
  builds the unit quaternion of an axis and an angle (half angle through gaSinCos) and converts it.
- **PanicModeInit** resets PanicInfoData (b01 0xff, +0x28 10000.0, +0x2c -1), TransparentBodyTime 360,
  AdvantageousTimeTime 90, and Panicdef (3.5, 8.0, 5000.0, 2.0, 30, 4, 8), trigger variable 0x1c = 0,
  and removes events 3, 0x26, 0x48.
- **tnInit3dlibWork(src, dst)** is gaInit3dlibWork with the 32-float copy to the scratchpad built in.

## Letterbox, fog easing, warps, joints, game/QuatSlerp.c (2026-11-10, slice 7)

- **VistaVision** is the letterbox: two black SPRITE packets in sdata (VistaVisionPacketUp / Down,
  0x50 bytes each, 512 wide in GS coordinates 0x8000..0xa000), each VistaVisionHeightSize *
  Counter / 100 lines tall at alpha Counter * 1.28. The script sets VistaVisionNumber to a request
  (1 at once, 2 gone at once, 3 slide in, 4 slide out, 8 percent a frame); 100 = shown. Events 0x15
  (on), 0x16 (keep on), 0xab (fully in), 0xac (fully out). The bars are drawn through their own draw
  control into the BGT ring after the frame: a host draw module must anchor them to the window's top
  and bottom at any aspect (they are full-width bars, not 4:3 content).
- **FogAnimeBuff[86]** (0x48 bytes per bank) eases a bank's FOG_INFO to targets: flags bits 1/2/4
  fog[0..2], 8 fogArg[3], 0x10 the clear colour (also drawcom_frameclear's RGBAQ); a second set
  (charaFlags / charaTime) for the character fog at +0x28. Each value moves 1 / time of the way a frame
  and snaps when time reaches 0. Skipped: paused (unless PAUSE_RUNNING), trigger variable 0xb >= 0,
  undrawn banks and type-3 banks.
- **The warps**: ExecuteWarpObject (by part index) and ExecuteWarpObject2 (by part hash in bank2, the
  5th argument in t0 = WARP_CMD +0x18). Mode 0 copies a part's three local matrices (0x240..0x300)
  onto another part; otherwise the character takes the marker part's position, its +0x54 short
  (CHARACTER +0x100), the Y turn from the bank's part 0 row 2 to the marker's, and joins the room:
  roomHash = ahGetHash of the bank file's first four characters (the old one kept at +0x98), lightBank
  (+0xa0) = the bank, a room bank (type 1) ends the character's CharAmbChangeFlag entry and sets the
  room's ambient (tnChangeCharAmbientColorSet {chara, bank, 0, 0}); the player's room becomes
  idCurrentScene (RoomInFlagOn). ExecuteWarpObject skips the room step when the bank is already the
  character's (CharID_TO_BankID).
- **asJointParts_byMatrix** joins a bank to a portal frame: two quaternion turns that align the joint
  part's rows 0 and 1 with the frame's become part 0's base rotation (+0x2c0 rows, PARTS_ANIM_BASE_ROT)
  and drawOnPos = pos - joint.pos turned. Returns 2 when the bank has no such part.
- **OneMaxObjectDrawZOnly** draws all list entries of one part with drawctrl_zbuf (alpha test NEVER,
  AFAIL ZB_ONLY, ZTST GREATER): a depth pre-pass, no colour.
- SkipFlagTbl (sdata 0x01938200, 137 ints, -1 ended) lists the skippable demos' trigger events in
  SkipEventFlag's bit order. Character slots 1..6 are the stalkers (CheckSameRoomAsMonster).

## Triggers and portals, game/ssGrpPortalCheck.c (2026-11-10, slice 1)

Event flags and variables come in two sets: the global set (`GlobalEventFlag`, 0x01941590, bits;
`GlobalVariable`, 0x01941190, ints; both saved) and a temp set per bank (`TempEventFlag[86]` 0x01944240,
`TempVariable[86]` 0x019440e0, LOCAL statics, `_SunAlloc` blocks made by `Trigger_init`, the flags
`(count >> 3) + 1` bytes). Every accessor is `(id, global, bank)`: global != 0 ignores bank (callers pass
-1). The set / get of variables masks the id to 16 bits only on the set. The trigger data is read with
little-endian byte readers (`ssTrigRead1/2/4/8Byte(&pos, data)`).

A bank's ALL_TABLE +0x24 lists its trigger detectors (TRIG_DETECTOR, bank data): +0 the EE address of
its part's world matrix (InitTriggerDetectorShape), +0xc the part, +0xe a bank, +0x10 / +0x14 EE pointers
to a local plane normal and four local quad corners, +0x18 a room hash (non-zero: a portal). +0xe0 lists
group portals (GRP_PORTAL) already in world space: corners at +0x40, normal +0x90, distance +0xa0, part
+0xa4, room ids +0xa6 (crossed from the front) / +0xa8 (from the back). `ProcessPortal` runs every frame
for characters 0, 2..6 (`ssScnShiftCharList` / `ssGrpShiftCharList`) that have flag 0x80000000 and stand
in a drawn bank (BankInfo.drawOn). A crossing is: last frame's position (CHARACTER +0xb0) on the front
of the plane (>= 0), this frame's (+0xc0) behind (< 0) -- group portals take either direction -- and the
crossing point inside the quad: the four corner directions' neighbouring angles (ahAcos units, 65536 a
turn) sum to at least 65014. Entering a portal moves the character into the room: roomHashPrev /
roomHash, +0xa0 to the detector's bank, +0x100 the room id, the ambient fade (tnChangeCharAmbientColorSet,
0x40), 20 frames of direct light, b102 = global variable 0x20; for Alyssa (id 0) also idCurrentScene,
SSAutoStartSoundRoomIn, RoomInFlagOn and GetAreaFloor. A part with flags bit 0 (PARTS_FLAG_HIDDEN)
switches its detector off (ssTrigCheckEnable answers the same bit).

**The trigger run (slice 3, 2026-11-11).** `Trigger_main` (once a frame from main) makes three passes
over banks 0..5 -- detectors (`ssTrigProcDetRecur` on the bank's root list), running actor lists
(`ssTrigProceedActor`), delayed part switches (`ssTrigExecDelayEnable`, front to rear) -- then, unless
`pause` is set without PAUSE_RUNNING or `pause_reservation` is set, the same three over banks 6..85. A
bank runs when drawn (BankInfo.drawOn; a BANK_FLAG_DRAW_FIRST bank among 6..85 and the delay passes:
when loaded, state > 0), not a behaviour bank (type FS_LOAD_BEHAVIOR) and without BANK_FLAG_ANIM_STOPPED;
a bank that runs skips the rest of its group. The "other" block's header ends with the root detectors'
count, their u_short ids follow. A detector record (DetData) is four offsets: act mode byte (then the
shape: kind, character, count, then u_short part ids -- u_long room hashes for kind 3), condition count
byte (conditions follow), actor count byte (u_short actor ids follow), and a child list (count short, ids).
Shape kinds: 0 a box (TRIG_DETECTOR +0x20 centre through the part's matrix, +0x30 half extents on the
world axes), 1 a quad crossed from the front (CheckPlaneEntry), 2 a plane volume (+0x08 plane count, +0x10
TRIG_PLANE[] (normal, +0x10 points per plane), +0x14 points: inside = behind every plane), 3 the
character's room hash, 4 the character in the bank's light touching the part through a target (w300[0] /
[1], target +0x198), 5 always. `ssTrigProceedDetector`: act mode 1 (level) activates every frame the shape
and conditions hold and deactivates every frame they do not; any other mode (edge) only on a change. A
true detector then runs its children. An actor (ActWork, 0x28 bytes) has state bits 1 / 2 (its activate /
deactivate list running), the starting detector per list (+4 / +6; -1 from a script's ssTrigExecuteActor)
and two 0x10-byte command lists at +8 / +0x18 (cursor, count offset, wait -1, count, step 0): a start
rewinds the cursor to offset + 1; `ssTrigExecuteActCmd` (0x018d5450, 0x249c bytes) steps a list and answers
non-0 when it has finished. curActInfo (0x01943d60: mode, actor, bank) names the running actor. First smoke
run with it: real bank data reaches ssTrigCheckConditions and ssTrigExecuteActCmd (both stubs).

The command lists (slice 5, 2026-11-11): each command is a u16 id and fixed operands (TRIG_CMD_* in
ssGrpPortalCheck.h has every id and size; ids 1..6 control and warps, 100s sound, 200 events, 300s
camera, 500s animation and texture pages, 700s waits, 800s scene loads, 900 variables, 1000..1003
if / endif / else / return, 1100s tests, 1200s actors, 1300 the x commands, 1400s screen effects, 1500s
shadows, 1600s movies, 1700s characters; 0xfffe ends a list). Names inside a list are inline 5-byte
strings (4 letters and a NUL). Blocks nest by TRIG_ACT_CMDS.step; a false test sets the skip depth and
skipped commands step over their operands. A command that answers 0 (a wait, an x command, PauseUpdate,
a running actor) steps the cursor back over itself and ends the frame's run. TRIG_CMD_XCMD (1300) is
a strings' length u16, the table index, argc and argc C strings: ssXcmd_functions[index](argc, argv,
bank) (0x0193a500, 256 distinct functions: wideXcmd_*, ssXcmd_*, tiXcmd_*, tkXcmd_*, tnXcmd_*, asXcmd_*,
hhXcmd_*, aaXcmd_*, suXcmd_AnmDirection, RotateCamera / LockerCamera, PicPaz).

## The IOP side (2026-11-09)

The EE loads `cdrom0:\IOP\IOPRP255.IMG;1` (an IOP reboot), then `SIO2MAN`, `PADMAN`, `MCMAN`, `MCSERV`,
`LIBSD`, `SDRDRV`, `MODHSYN`, `MODMIDI`, `MODMSIN`, `FSYS_IOP` (strings 0x01934ca0..0x01934de0) and,
from `sound.c`, `SNDDRV.IRX` (0x01936960). `MODHSYN` / `MODMIDI` / `MODMSIN` are SCE's hardware-synth MIDI
stack; `SDRDRV` serves the EE's sdremote client; `SNDDRV.IRX` (151 KB) is presumably the game's own
driver and `FSYS_IOP.IRX` its file server. None is read yet. No sibling met modhsyn (MioPan, ReiPan,
RosePan, YamiPan all ran a game's own driver over libsd).

**The file server, FSYS_IOP.IRX (2026-11-10, host task `alyssapan/iop/fsys_server.cpp`).** The EE binds
three services ('SUN@' / 'SUNA' / 'SUNB', game/IOPGetFileSize.c) and calls only 'SUNA': FlProc sends a
0x40-byte name (`\DIR\NAME;1`, upper case, built by FlReqOpen) and reads back {size, first sector} (0x10
bytes), then reads the sectors itself with sceCdRead. **It serves loose files only** -- a name found in
CT3PACK.DAT's directory (FlFindPacked) never reaches the IOP, and every bank (`*.BIN`), `.INF`, `.ICO`,
`.SDT` / `.BD` is in the pack (980 rows); the loose files are the movies (`DATA\MOVIE1..3\*.MOV`) and the
boot files. The host server (`iop::FsysServer`) answers from `io::Disc::find`, registered when
sceSifLoadModule loads `FSYS_IOP.IRX` (sdk/sif.cpp `kIopModules`); probed 2026-11-10:
`\DATA\MOVIE2\M_CAPCOM.MOV;1` -> 6,569,988 bytes (the file's size) at sector 1,635,587. A name not on the
disc answers -1 / -1: the module's own answer is unread (open-items.md). **The banks load without it**:
INFO, E_GL, 0MH0 and ATLD pass every FSMltLoadProc phase to the set-up step at boot (probed 2026-11-10),
and TrgGLBLSceneDraw gets past BankReady (camera_init0 / CamChange_S are reached).

**The memory card (2026-11-09, sdk/libmc.cpp's host task).** CT3 links libmc's EE side at
0x01826d38..0x01828530. Its `sceMcInit` binds `MCSERV` and refuses an IOP side older than mcserv 0x20a /
mcman 0x20e (returns -120 / -121, strings 0x01932010 / 0x01932038): the host answers 0. The game calls
eleven entry points (feat_ct3.json's call graph): `sceMcInit` (McInit), `sceMcGetInfo` (mc_check,
mc_check2..4, mc_formatcheck, mc_formatcheck2, mc_free_area), `sceMcOpen` / `sceMcClose` (mc_load_new,
mc_save_new, mc_save_normal), `sceMcRead` / `sceMcSeek` (mc_load_new), `sceMcWrite` / `sceMcMkdir` /
`sceMcChdir` (mc_save_new / mc_save_normal), `sceMcFormat` (MemorycardUpdate), `sceMcSync` (all of them).
No `sceMcGetDir` or `sceMcDelete`: a save is found by opening it. `mc_check` polls `sceMcGetInfo` until
four syncs in a row agree on the type and answers 0 only for type 2 (PS2 card); `mc_save_new`'s step 0x15
builds icon.sys in place (`sceMcIconSys`, 0x3c4 bytes, OffsLF at +6, TransRate 0x60) and writes it whole.

## PC platform layer (bootstrap 6, 2026-11-09)

YamiPan's Pan layer, carried over and renamed (cpp-port.md has the map): window, render thread, 2D
canvas, TIM2 texture store, present / HDR grade, screenshots, overlay and panels, input, config, logger,
crash report, EE kernel and interrupts (one EE thread at a time by priority), IOP kernel, SIF RPC
servers, the SPU2 model and voices, the audio device, libmpeg on FFmpeg. Left in YamiPan (FS2's own): the
rena model renderer, smooth motion, the frame passes, the teleport panel. The build's only game code is
the stub `gameMain` (`main`, CT3 0x01839d10) in `src/lib/unassigned_stubs.cpp`.

## Lights (game/Character_Init.c, 2026-11-10)

Light sources are model parts: MODEL_PARTS.lightPos (+0x320, w a falloff; 1.0 for the bank's
Omni0_num / Omni1_num parts) and lightColor (+0x310); bit 0 of the part's flags switches one off. Each
bank's ALL_TABLE.light (EE +0x28) is a SCENE_LIGHT: [part, set] pairs ended by -2, LIGHT_SET records
(0x14: four directional parts, four point parts, -2 = the next nearest of an optional list), the
character scale main_rtn reads (+8), a character's default directional parts (+0xc) and the point lights
characters choose from (+0x10). A lit part's MODEL_PARTS.attr is the EE address of its GA_LIGHT_WORK
(dirColor +0x90, omniColor +0xd0, dirPos +0x110, omniPos +0x150), which gaSetLight loads under
matrix_global_view (scenes) or mrr_ReverseGlobalView = matrix_global_view x mrr_ReverseMatrix
(characters). gaSearchNearLight4(nearest, pos, n, lights, count) answers four light indices per
position (0xff = none). Overrides: AllOmniEffect[4] and CharOmniEffect[chara][4] ([bank, part] pairs,
cleared when the bank is gone); CharDirectLightSet[chara] forces directional parts, eased over its fade
counter (or DirectLightCharaCounter[chara]).

## The save image (game/Character_Init.c, 2026-11-10, slice 5)

A save image is MC_DATA_SIZE (0x10610) bytes in EE layout (MC_SAVE_DATA, Character_Init.h), the same in
the card's work buffer (mc_databuff[8]) and in continue_backup (the continue after a death).
data_save_set writes it and data_load_set reads it back:

- 0x0000: 35 bank records of 0x30 bytes (position, bank, character, name, type, loaded), ended by bank -1:
  BankInfo[2] first (if loaded), then banks 6..85, one per group (BankInfo.groupCount skips the rest).
  A load ends and reloads the global banks (INFO 5, E_GL 3, 0MH0 1, GAME 0), then the current scene's
  record, then the others by type (0/1 scene, drawn at the saved part-0 position through
  DrawOnPosSetVectorTemp; 2 character shape; 3 character behaviour).
- 0x0690: Character[35], 0x580 bytes each, through mc_char_set's field list.
- 0xc710: save_count, progress_time, the area (GetAreaNumber(0): mc_Getinfo_msg_num), the next /
  current / camera scenes, the camera's cut (CAMERA +0x178).
- 0xc72c: GlobalEventFlag (0x101 bytes, event e = bit e & 7 of byte e >> 3; McRegisterEvent writes it, and
  mc_Getinfo_ClearFlag reads event 0x0b, the game-cleared event, which also picks the round on a load);
  0xc830: GlobalVariable[256] (mc_Getinfo_BranchNo reads variable 0x1d, the stage).
- 0xcc30: ItemData; 0xd590: 0x960 bytes not written here (the AI's ssAI_MemSaveData, probably);
  0xdef0: LetterData; 0xe3a0: ItemList, KeyItemList, LetterList, ArrowData, ArrowList (to 0xe5cc).
- 0x105d4: PanicInfoData (0x30 bytes). 0xe5cc..0x105d4 is not written by these two functions.

The auto-load at boot (MemorycardAutoLoad, xcmd_tn.c) has its own states (mcMain_ID 0..200) and messages
(MC_AUTO_ERR_*). It checks slot 1, then slot 2, and takes the first save whose header is the game's.

## Sound (game/sound.c, 2026-11-09)

- The game's sound layer (`SS*`, `Sound_*`, `Branch*`) sits on the Sd driver's EE side
  (game/SdMasterVolumeSetSE.c, `Sd*`, from 0x018cbf90), which talks to the IOP driver through
  `CpCall_IOPSndDriver_FIFO` under the semaphore `fss_sid`.
- SE banks: `gFileSE` (0x01939ea0, 6 rows + end) picks a bank by its file name's first two letters --
  C_ (slot 4, SPU2 0x0e8000), E_ (6, 0x182630), Z_ (7, 0x1b96f0), R_ and B_ (5, 0x1562b0), T_ (8, 0x1d40c0);
  files are `DATA\SOUND\<name>`. Boot loads C_0000, T_0000, E_HAMMER and Z_REI, waiting for each.
- SQ music: `gFileSQ` (0x01939f00, 13 rows + end) by full name under `DATA\SOUND\SQ\`: PIANO, TUISEKI(2),
  CHITEI, FUAN, HAKABA, HINMIN, KOJO, KYOHU, OMOIDE, TSUNORU, MISSHORT, MISLONG.
- Stages (trigger variable 0x1d) own their BGMs: stage 2 HINMIN / KYOHU, 3 CHITEI / HAKABA, 4 TSUNORU /
  KOJO / FUAN / OMOIDE, 5 OMOIDE. `gPlayingStageBGM` records the one playing (2..9) so SSSoundEnable can
  restart it after a movie; a chase BGM (`gPlayingChaseBGM`) blocks them.
- The option menu's 0..20 volume buffers become the driver's 0..255 in SSSetSoundMode.

- (slice 2) **Scene codes are room names.** `gNowScene` / `gBfoScene` hold the current bank's file name's
  first four letters as a little-endian word ("5A02" = 0x32304135; boot sets "Nxxx" / "Bxxx"). SSBGMProc
  reads it each frame from `BankInfo[idCurrentScene].name` when that bank is a loaded single bank past
  bank 5; a change runs the stage's BranchNBGMProc once with sceneChange 1 (start / stop BGM, reverb via
  `SdSetEffect(1, 3, depth)` or dry `SdSetEffect(1, 0, 0)`), otherwise it runs per frame with 0.
- (slice 2) **The chase BGM** (TUISEKI on stages 1-2, TUISEKI2 on 3-4) plays while trigger event 0xc9 or
  0xd4 holds and stops 15 frames after (`gChaseStopDelay`), the stage BGM coming back; trigger variable
  0x4d == 1 keeps it off. Variable 0x58 (SSProc) turns music down (1: to 0x28, 2: silent) and SEs off.
  The boss battle (event 0x9a) runs a T_BREAK.ADS countdown (0x294 / 900 frames) into 22.ADS (23.ADS on
  stage 5). Stage enemy SE banks (BranchSELoad): 1 E_HAMMER, 2 E_RYUSAN, 3 E_ONO, 4 E_SWO, 5 E_BAL.
- (slice 2) **The Sd driver's records** sound.c reads (SdMasterVolumeSetSE.h, EE layout): `gSdSE`
  (0x01c848c0, 34 x 0x54: one playing SE, its fields set through `SdSetParamSE(handle, mask, ...)`, whose
  mask bits each take the next argument -- 0x80000000 vol +0x34, 0x40000000 pan +0x38, 0x20000000 +0x30,
  0x10000000 posVol +0x3c, 0x100000 posMode +0x1c, 0x80000 objId +0x20, 0x40000 bank +0x24, 0x20000 name
  +0x40, 0x10000 callback +0x50), `gSELd` (0x01c85400, 10 x 0x1c0 bank loads: name +0x3c, slot +0x78),
  `gSQLd` (0x01943340, 2 x 0x1c0 SQ players: name +0x38, start volume +0x9c, flags +0xa0, target volume
  +0xac), `gPlaySESp1` (0x019436d0, 5 x 0x34 special SEs), `gPlayADS` (0x019437f0, the stream's name first).
- (slice 2) **Positional SEs.** A bank's sound objects (`ALL_TABLE.sound`, EE 0x54 -> `SOUND_OBJ_TABLE`,
  objs at +4: 0x28-byte `SOUND_OBJ` {id, seNo, autoStart, ..., positional +0x24}) start on room entry; a
  positional one gets `SetPositionalParam_Alys` as its per-frame callback: the source is model part objId
  of the bank (`MODEL_PARTS.pos`, +0x130), heard from the player's `posBefore` (or the camera), volume
  0x7f minus 3 per 55 units, pan 0..0x7f from the angle to the camera's view.

## The sound driver's EE side, game/SdMasterVolumeSetSE.c (2026-11-10, slice 1)

- **One command path.** Every Sd* that talks to the IOP takes `fss_sid` (sbss 0x01943020), fills a
  command record, queues (command word, record, reply pointer) on `RingBuff` (bss 0x01943030, 64 x 12
  bytes, write index `wp` 0x01942ee8, read index `rp` 0x01942eec) through `CpCall_IOPSndDriver_FIFO`
  (0x018d1c60, wakes `fss_tid`), and releases the semaphore. `FSS_Thread` (0x018d1bd0, priority 2, a 0x1000
  stack `fss_stack$75` at 0x01c838c0) sleeps until `rp != wp`, then sends the oldest entry through
  `CpCall_IOPSndDriver` (game/CpDivideEECom.c, 0x018d2250) and copies the reply word if asked.
  `FSS_Thread_Init` (0x018d1b40) raises its caller to priority 3 and creates both; `FSS_Thread_Sync`
  waits for an idle semaphore and an empty ring. `IopLoadCapcomSound` is the init's likely caller (unread).
- **Command words** (CpDivideEECom.h `SND_CMD_*`): 0x150000 reverb (`effectJSLCOM$121` +8 core, +0xc type,
  +0x10 / +0x14 depth word, +0x18 type changed), 0x240000 SE master volume (`gTransJSLCOM` +8), 0x290000 stop
  an SE's channels (`seStopJSLCOM$1185` +0xc channel bits, +0x10 0x80000000 or 0x4000000d), 0x310000 the
  ADS volume (`gTransADS` +0x68, v | v << 16, v = vol * gMasterVolADS / 255). The JSL records are 8 words.
- **Reverb** (`SdSetEffect`): per core, depth clamped to 0x7f, stored as depth << 7 | 0x7f in
  `gSdEffectDepth[2]` beside `gSdEffectType[2]`; resent only on a change; core 1 maps types 4..8 to 3.
- `SD_SE` +0x14 is the voice channel bits; `SD_SE_LD` +0x00 is the load's handle (not a used flag) and
  +0x94 is non-zero while loading; `gPlayADS` is `SD_PLAY_ADS` (+0x38 volume, +0x44 SdStopStream's argument);
  `EE_IOP_STAT` +0x20 non-zero while a stream plays. `SdAlloc` hands out gHeapSound's one block
  (`gUseSoundAlloc`); `SdRnd` is an LCG (x * 0x7d2b89dd + 0xbc614e, seed 0x12345678).

### Slice 2 (2026-11-10): SdProc and the SE bank loader

- `SdProc` (once a frame from FileSoundServer and every wait loop) steps five static procs: SE loads, SQ
  loads, SE play, special-SE play, the ADS stream.
- **An SE bank is three files**, `NAME.SDT` (16-byte rows; the loader writes the slot `jls` into byte 1
  and clears bit 7 of byte 4), `NAME.HD` and `NAME.BD`. `SdLoadSEProc` (0x018cfe40) walks one `gSELd` row
  (`gLdi`) through `SD_SE_LD_STEP` (SdMasterVolumeSetSE.h): open, size, `SdAlloc` the whole file
  ("SD1" / "SD2" / "SD3", 0x800-aligned), read, `sceSifAllocIopHeap` + `CpSifDmaTransEEToIOP` for the SDT
  and HD. The **.BD** goes in 0x32000-byte pieces: `sceSdTransToIOP` (sdremote: a plain SIF DMA) to the
  staging block `gSEHeapBD`, then command 0x120000 on the driver's second channel
  (`CpCall_IOPSndDriver_2`, record `SD_SE_LD.spuCom`: staging block, SPU2 address `spuAddr + done`, size
  rounded to 0x40) moves it to SPU2 RAM; `gExecDriver_2` spaces two transfers 60 frames apart;
  `CpCheckEE_IOPComEnd(0x120000, 1)` polls the end. Last, command `0xa0000 | jls` hands the driver the
  register record (`SD_SE_REGISTER` at +0x100: "cdrom0:\" + dir + name, upper case with '\', IOP HD,
  staging block, SPU2 address, IOP SDT, BD size). `NextLoadSE` picks the oldest waiting request newer
  than the current one.
- **A packed .SDT carries its .HD** (`FL_PACK_INFO` +0x10 `partSize`, read from CT3PACK.DAT: 104 of 980
  rows, e.g. `C_0000.SDT` 1280 of 7392 bytes; `.SQ` rows too): one IOP block for both, `iopHd = iopSdt +
  partSize`, and the HD steps are skipped (step 3 jumps to 7).
- `SdReqLoadSE` (unread body, dec seen) fills `dir` +0x04 (upper case), `name` +0x3c, `spuAddr` +0x74, `jls`
  +0x78, `stopPlaying` +0xa4 and starts at step 1; `stopPlaying == 1` stops the slot's SEs whose
  +0x0c word is below 1000, and clears `gPlaySESp1` rows of that slot (+0 is the slot, +8 below 1000).
- SQ volumes: 0x1b0000 the master (`gTransJSLCOM`, unk00 0, arg[0]), 0x1c0000 one player
  (`gSQLd[i]` +0xc0 record: unk00 = i, arg[0] 0xff, arg[1] volume).
- **An SQ (slice 6, 2026-11-10) is three files too**, `NAME.SQ`, `NAME.HD`, `NAME.BD`; `SdLoadSQProc`
  (0x018ce6c0) walks `gSQLd[gSQi]` (gSQi is never written: row 0) through `SD_SQ_LD_STEP` with the SE
  loader's steps (SdAlloc "SD4" / "SD5" / "SD6" into the one global `sqAllocPtr`; a packed .SQ carries its
  .HD; the .BD in 0x3a000-byte pieces through `gSQHeapBD`, SPU2 address `gFileSQ.spuAddr` = 0x20000), then
  `0xa0000 | player` with `SD_SQ_REGISTER` (+0x100: path, IOP HD, staging BD, SPU2 address, IOP SQ, BD
  size, the player at +0xa8 / +0xaa), then one command a frame: 0xc0000, 0xd0000, 0x170000, 0x180000
  (arg[0] 0), the SQ master volume, after `effectDelay` frames `SdSetEffect(0, effectType, effectDepth)`
  (gFileSQ: type 4, or 5 for PIANO / MISSHORT / MISLONG), 0x190000 (arg[0] 1, the start), the player's
  volume. PLAYING (0x19) waits for SdVolumeSQ (0x1a: to `vol` at once, or one unit a frame with FADE) or
  the stop (0x1c: 0x1a0000 arg[1] 0xb, two frames, free the IOP blocks, `0xb0000 | player` with the
  register record, clear it), then **row 1 is copied into row 0** and cleared: the queued SQ starts.


### Slice 3 (2026-11-10): SE requests and play

- `SdReqLoadSE(dir, name, jls, spuAddr, stopPlaying)` answers the handle of a row already holding the
  bank (by upper-case name), else cancels the row on the same SPU2 address or slot (`SdReqLoadSECancel`:
  close, free, `SdPurgeSE` when it had loaded) and fills a free row at step 1; ten rows, then
  "### ERR: SdLoadSE, load max ###". `SdPurgeSE` sends 0xb0000 | jls.
- **An SE's life** (`gSdSE`, 34 rows, `SD_SE.state`): `SdReqPlaySE(jls, seNo, reqArg2)` takes a free row
  (handle from `gSdSEHandleCounter`; vol / pan / +0x30 = 999); with none, every channel stops. Each frame
  `SdPlaySEProc` asks the driver (0x60000, no record) for the sounding channels into `gSdSEKeyOnCH`: keyed
  -> playing once its channel sounds, playing -> free once it stops, the row's callback(handle) every frame
  meanwhile. Then **one** key-on a frame: `NextPlaySE` picks the oldest request after `gSEi`; else one
  playing SE with VOLUME / PAN / +0x30 / CALLBACK bits (0xe0010000) gets an update (`sIdxUpdate$366`).
- **Channels** (`SdSupplyCHSE`): 22 SPU2 voices as bits; a ring (`sBitRingCH$971`, from 1) skips those
  sounding and the last given (`sBitLastSupplyCH$972`); reqArg2 0x3e0 takes over the channel of another
  0x3e0 SE instead (stopping it, stop word 1).
- **The play command** 0x260000 goes in the bank row's `SD_SE_LD.spuCom`: unk00 the slot, arg[0]
  seNo | 0xa1000000 (key on) or | 0x21000000 (update), | 0x100000 volume, | 0x400000 +0x30's byte,
  | 0x10000 pan; arg[1] channel << 24 | +0x30 << 16 | (volume ? volume - 0x7f : 0x80) << 8 | pan. The volume
  is scaled by `posVol` in tenths (0 full, 1 silent); with no parameter bits, a posVol alone sends
  0x7f scaled. paramFlags 0x8000 makes a request wait until the slot holds the bank named in `SD_SE.name`.
- **Special SEs** (`gPlaySESp1`, `SdPlaySESpProc`): every `interval` ga_main_timer ticks a row starts SE
  seNoBase + SdRnd() % range with its positional parameters and update (unless `stopped`).
## The shadow overlay, frame-pass checks and the replay pass, game/ahRelocateSunPs2ObjectArry.c (2026-11-10, slice 14)

- **asShadowEnd** ends a bank's shadow pass. The shadow buffer (0xc0000, cleared to 0x80) holds the
  volume count; two full-screen sprites fold it (`gaDC_EBclear_D$2764` adds 0x60 with FIX 0x80, then
  `gaDC_EBclear_F$2769` subtracts 0xe0), drawing returns to the frame (`asSD_BackBufferAdd`,
  `drawbuffer_0`), and the buffer read as a texture is laid over it in shadowColor RGB * |shadowOrder| /
  100 (alpha 0x80): negative shadowOrder lightens (`EBclearW`, Cd + Cs * FIX 0xff), positive darkens
  (`EBclearB`, Cd - Cs * FIX). Split pass: `asFS$2787`, 56 sprites of 64 x 64. Plain pass: four
  TRIFANs (Gouraud) from BANK_INFO +0x40 (the shadow anchor, asShadowPolygonPositions writes it)
  projected by matrix_global_view and screen_0 (CX + 0.832 * d * x / z, CY - 0.94 * d * y / z), coloured
  at the centre and black at the screen corners; a side whose anchor left the screen is not drawn. The
  fans' packets live in the bank's _SunAlloc'd shadow block (BankInfo +0x58, 0x280 bytes; +0x70 = 1).
  screen_0 = {0, 0, 512, 448, 256, 224, 0.832, 0.94, 512, 5, 40000, 16777215}.
- **asCheckAnimationFlamePassing_S** is the triggers' "animation passed frame N" test: BEHAVIOR_TRACK
  +0x1c (time1c) holds last frame's frame (high half) and this frame's (low half); forward tracks pass
  (last, now], backward [now, last), each wrapping past the end; a character's motion asks the
  behaviour its motion table record names (record +8: the behaviour's hash), group 0's first track.
- **EA_010** (ExecuteAnimation's replay) writes the key each running or ending track stands on into its
  target without moving the time: the part's +0x300 (FLOAT0_7_9) / +0x310 (VECTOR_XYZ) / +0x5a (NUMBER) /
  +0x58 (FLOAT1_1_14), a group of ga objects' UV offset (+0x78, groups via ALL_TABLE +0x40 / +0x44 into
  objOpaque), the vertex positions (+0x60) / normals (+0x64, data at +0x10) / alphas (+0x6c) of the
  objects the key's sub-records name (ANIM_SUBKEY +0x10), and for COPY tracks a word of ALL_TABLE +0x30
  stored through each grouped sun object record's word 2. The packed keys' `_nw` writers skip a vertex
  whose first short is -0x8000 (or whose w is 0): BEHAVIOR_TRACK.length bit 0x2000000.
