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
   `GlobalDrawObject`, `scene_draw_init`, `system_end`, `get_draw_obj`; slice 3 (2026-11-09): `main_rtn`, `main_rtn_Particle`; slice 4 (2026-11-09):
   `DrawObjectRefraction`, `main_rtn_DOF`; slice 5 (2026-11-09): `AllDrawObject`,
   `ssHQDOF_DrawAllObject`, `main_rtn_GHOST`, `main_rtn_GHOST_DOF`; slice 6 (2026-11-09): `scene_init`
   (ALL_TABLE +0x14 / +0xb8 / +0xbc, `DRAW_OBJ_ENTRY.drawCtrlArgs`, `GA_DRAWCTRL_ARGS` and `TEX_INFO`
   passed by value, hazards.md); slice 7 (2026-11-09): `main_rtn_Particle2` -- **main.c's code is complete (verify: the shot stays black until main2 loads a bank)**. Open from slice 7: (t) `PARTICLE2_ENTRY` / `PARTICLE2` (ct3_types.h) are named from this one use (ring of 0x80-byte records at +0x5c, count +0x46, head +0x48; record +0x20 offset qword whose w scales the part's w around 128, +0x68 the draw object); the particle TU confirms them. Open from
   slice 6: ~~(q) `gaSetupDBufferData` takes 14 argument slots~~ fixed 2026-11-09 (gaInitDrawObject.c
   slice 1); (r) `BANK_TYPE_0` (type 0 takes the fog record) and
   `PARTS_HEADER_ANIMATED` (the parts header's first byte == 1) are named from one use; type 1's fog
   colour clears the frame, so `BANK_TYPE_SHADOW` is likely the stage bank (with (j)); (s)
   `PARTICLE_ENTRY.drawCtrlId` (+4) is probably a `GA_DRAWCTRL_ARGS *` like the draw list's (not
   changed: no use seen). Open from slice 5: (n) the mirror arguments
   (`asEnumMirror(ALL_TABLE *)`, `asMirrorPlaneCalc(planeVec, planePoint)`, tbl +0x2c `matrixTree`) and
   `tiChara_Translucence_End(float alpha, int fbuf)` / `_End2(int fbuf, int chara)` come from the call
   sites: confirm them in their TUs; (o) `BANK_FLAG_NO_MIRROR` (0x10) and `EV_SHADOW_AFTER_MIRROR`
   (0x3b) are named from that one use; (p) HQ DOF's eye w for the diagonal samples follows the
   decompiler's operand order (up's w), unverified -- only xyz should matter. Open from slice 4: (j)
   `BANK_TYPE_SHADOW` (type 1) is the type of every bank AllDrawObject, ssHQDOF_DrawAllObject and
   DrawObjectRefraction draw among banks 6..85: it is the scene-object bank type, not a shadow kind --
   rename it when the bank loader TU names the type; (k) `CHARACTER_FLAG_REFRACTION` (0x80000000) is named from that one
   use; (l) `MODEL_PARTS.kind` (+0x42) == 9 skips a part in the DOF slices (`PARTS_KIND_NO_DOF`), other
   values unread; (m) `DRAW_OBJ_ENTRY.dofMode` (+0x10) is ssDOF_ClipObjectFromZRange's mode (< 2 tests
   the object's view Z against DOF_NearClip / DOF_FarClip; >= 2 draws only when the near clip is -10000). Open from slice 2: (f) `ALL_TABLE` (All_table_TBL's records, ct3_types.h) is
   `layout: host` with pointer fields at EE 0x10..0x7c; the file TU decides whether the bank's relocated
   words become host pointers or stay `u_int` EE addresses; (g) settled by slice 3: `main_rtn(bank, mode, chara, flags, charaIndex, lastTex)` -- chara != 0 is the
   character pass (scene fog +0x28, Character[].lightBank's light), flags' low 15 bits the pass (0 and 4
   sort), 0x8000 no clip check; (h) **decided 2026-11-09 (Claude's call, slice 7; the user may overrule)**: bank data keeps its EE layout and its relocated pointer words stay `u_int` EE addresses, resolved by `BankEePtr(bank, ee)` (ct3_types.h: `BankImage[]` / `BankImageEE[]`, PORT globals the bank loader in fileio.c's TU must fill; RosePan's `SprMan::eePtr` is the precedent). `DRAW_OBJ_POS` (+0x38) is the first word read this way. The reasoning: it writes a particle's position
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
   **Queue refilled 2026-11-09 (scheduled run, the queue was empty)** in the order the smoke run reaches
   the stubs: ~~ga/gaDma.c~~ (verify), ~~ga/gaVifInit.c~~ (verify; its calls made ga/gaDmaSync_VIF1.c's
   gaDmaSendN_VIF1 / gaDmaSync_VIF1 stubs: that hand-asm object is a small TU worth queueing next),
   ~~ga/gaInitDrawObject.c~~ (verify 2026-11-09: all 24 functions; the texture three have no ROM caller),
   ~~ga/gaInitMeter.c~~ (verify 2026-11-09; renamed from game/: a GCC object, the ga library's last; T1 mode writes now reach the host timer),
   ~~game/ahalloc.c~~ (verify 2026-11-09: all 7 functions; SunAllocInit runs at boot, the rest waits for
   a caller; its AlignedNum call made ~~game/AlignedNum.c~~ (verify 2026-11-09: AlignedNum, WaitLoop,
   IopLoadModule; IopLoadModules in ArtoolDebugOpen.c is its one caller, still a stub), hazards.md;
   open: (u) the heap's +0x14..0x34 is unread here -- whoever reads it names it; (v) the host block header
   is 0x20 bytes, so EE-sized areas hold slightly less: watch for _SunAlloc errors once loading runs),
   game/sound.c (partial 2026-11-09, slice 1: 38 of 69 functions -- boot / end, SE and SQ entry points,
   the stages' SQ rules, movie / demo mute, angle helpers; Sound_system_init now asks SdReqLoadSE for
   C_0000, T_0000, E_HAMMER, Z_REI; open: (w) the Sd driver TU (game/SdMasterVolumeSetSE.c, 0x018cbf90..)
   is the next sound blocker: every Sd* is a stub, so nothing sounds; its prototypes in
   SdMasterVolumeSetSE.h come from sound.c's call sites; (x) gFileSQ's words +4 / +8 / +0xc are
   SdPlaySQ arguments named sdArg2 / sdArg5 / sdArg6 until that TU reads them; (y) TRIG_VAR_STAGE (0x1d),
   TRIG_EVENT_SE_MUTE (0xb7), the stage-1 SQ events 0x3b1 / 0x3ba / 0x3bd / 0x3bf and STAGE_BGM_* are
   named from sound.c's use only; (z) gBfoScene / gNowScene / gStereo / gMasterVol* / gPlaySESp1 sit in
   lib/unassigned_data.cpp until their owner, with the ROM's initial values), slice 2 (2026-11-09: 68 of 69 -- SSProc /
   SSBGMProc / Branch1..5BGMProc, the sound objects and trigger actions, positional SEs, volume save and
   restore, SSPlayADS, SSPlaySESp1, SceneSELoad / BranchSELoad; left: SSLoadSEfromLoadPoint, which reads
   fileio.c's load-point record +0xd4 (a pointer to up to 5 rows of 0xc bytes, name pointer first): write
   it with fileio.c's TU. Open: (aa) `SD_SE.callback` (+0x50) is an EE code word in a 0x54-byte record,
   but SdSetParamSE / SdReqPlaySESp1 now receive host function pointers (SetPositionalParam_*): the Sd
   driver TU decides the host form (a table index, or a host-layout record); (ab) the trigger events and
   variables SSBGMProc / BranchNBGMProc use (0x9a boss battle, 0x49 Alyssa falls, 0x51, 0x96, 0xc9 / 0xd4
   chase, 0x53 / 0x54 T_BREAK, 0x26 piano volume, 0x58 quiet, the stage rooms' events) are named from
   sound.c's use only; (ac) `ahGetHash` / `asFindModelPartsID` now take a 64-bit hash (u_long): the ROM
   compares 0x3161c34c2 and 0x74dda77d0cbcf0c2, so main.c's Omni lookups pass 64-bit hashes too -- confirm
   when QuatSlerp.c's hash functions are written; (ad) `ALL_TABLE.sound` (EE 0x54) is a host pointer like
   its neighbours, its `objs` an EE word read with BankEePtr (decision h); (ae) `ADS_FILE.loop` is named
   from the table's pattern only), slice 3 (2026-11-09: SSLoadSEfromLoadPoint -- **sound.c's code is
   complete, verify**: the "+0xd4 record" was the bank image + 0x80 (the ALL_TABLE, FSMltLoadProc sets
   All_table_TBL[bank] to it) + 0x54 (`ALL_TABLE.sound`), whose word 0 is now `SOUND_OBJ_TABLE.loadSE`,
   the EE address of up to 5 `SE_LOAD_POINT` rows. Open: (af) the ROM passes the bank's image (BankInfo
   +0x1c, still `pad1c` in BANK_INFO) with no bank index; the host finds the bank by matching
   `BankImage[]` (PORT `SoundBankOfImage`) -- fileio.c's TU must fill BankImage before it calls this, or
   may change the call to pass the bank),
   game/Character_Init.c (partial 2026-11-09, slice 1: Character_All_Init, IOPInit, McInit, mc_alloc / mc_free /
   mc_save_data_init, mc_check, mc_check_always, MemorycardAutoLoadInsertedCheck3, mc_make_checksum,
   mc_load_new; left: MemorycardUpdate (0x17e8, reached every frame), mc_check2/3, mc_save_system_init, the save / load / format
   paths, SceneLightSet / CharLightSet. Open: (ag) CHARACTER's new fields (ct3_types.h: slot004 / slot018 /
   slot022, w090, w098, w0fc, h100, b102, b2f1, b2f2, w300, b534, h536) are named by offset from the reset
   only; hp / hpMax (+0x524 / +0x528, both Char_Init_MAX_HP) may be the other way round; (ah) CHARACTER.junc
   (+0x570) is a host pointer to CHARA_JUNC whose buffer sits at EE +0xc: whoever allocates it confirms
   the record; (ai) IopLoadModules is declared taking argv[0] (IOPInit passes it; the ROM body reads no
   argument -- SetDeviceName may): ArtoolDebugOpen.c's TU confirms; FSInit is empty (4 bytes);
   (aj) mc_alloc answers 1 on success where the ROM answers the block address; SdAlloc / SdFree are still
   stubs, so the card buffer never exists yet; (ak) CHARACTER still lives in ct3_types.h, not this TU's
   header: move it when the rest of its users are written); (al) mc_load_new bumps mc_save_normal_num,
   not its own step (disassembly 0x018b8d10 / 0x018b8df0): kept as the ROM's slip -- mc_save_normal's
   session checks whether it matters), slice 2 (2026-11-10: MemorycardUpdate -- its four flows split into
   static mc_update_* helpers, callcheck comment -- plus mc_save_init, mc_formatcheck, mc_formatcheck2,
   mc_check4, MemorycardAutoLoadFormatCheck; left: system_data_load_set, mc_load_set, mc_save_new,
   mc_save_normal, mc_free_area, mc_Getinfo_count, mc_check_always2, mc_check2/3, mc_save_system_init,
   data_save_set / ContinueBackupSet / ContinueBackupGet (data_save_set takes 0x01a31450, an unnamed bss
   address: name it there), mc_char_set, the lights. Open: (am) the mcErr_ID / mcAccess_ID names
   (Character_Init.h MC_ERR_*, MC_ACCESS_*) are read from MemorycardUpdate's flows only: numbers set on
   unlike paths stay MC_ERR_MSG_n until the menu TU that prints them names them; (an) SSStopSE /
   SSPlaySE(MC_SE_ACCESS) calls whose a2 the ROM leaves unset pass 1 (every traceable site holds 1);
   (ao) TRIG_EVENT_LOAD_SLOT2 (0x0f), TRIG_EVENT_MC_LOADED (0x99), TRIG_EVENT_SAVE_ALT (0xa9) and
   MC_DATA_USED (save image +0xc710) are named from this use only; (ap) tiFade_Start(float) is declared
   from the call (10.0f in f12); FadeEndTicks sits in lib/unassigned_data.cpp), slice 3 (2026-11-10:
   the save path -- mc_save_new, mc_save_normal, mc_save_set, McRegisterEvent, ContinueBackupSet
   (0x01a31450 is the symtab's continue_backup) -- system_data_load_set, mc_check2, mc_check_always2,
   mc_Getinfo_count, MemorycardAutoLoadInsertedCheck2; MC_HEADER (the 0x80-byte system data) in
   Character_Init.h; left: SceneLightSet, CharLightSet, data_save_set, mc_load_set, mc_char_set,
   mc_free_area, mc_check3, mc_save_system_init, ContinueBackupGet and the rest of the 19). Open: (aq)
   mc_save_set ORs AA_UidCount << 8 into header word 1 (the readers mask 0xfffffff0, so their compares
   stay consistent) while main bumps AA_UidCount by AA_UID_STEP 0x10: kept, the uid's units are
   unclear until the menus run; (ar) system_data_load_set(1) keeps only OMAKE_2 before OR-ing OMAKE_1 back, dropping
   OMAKE_BOOTED: the ROM's, kept; (as) KEY_MAP[19..20] = KEY_MAP[54..55] after a load (disassembly
   0x018b8778): the key-config TU names the pairs; (at) TRIG_EVENT_OMAKE_1/_2, TRIG_EVENT_MONAURAL,
   TRIG_VAR_*_VOLUME are named in Character_Init.h, main.c's system_init still uses the raw numbers;
   (au) mc_save_new writes the icons again even when sceMcChdir finds the directory (step 2, not 10) and
   frees the icon buffer before a close that may be refused: both the ROM's, kept; FlGetLen(dir, name,
   mode) is declared from the call (mode 1)); slice 4 (2026-11-10): SceneLightSet, CharLightSet (SCENE_LIGHT / LIGHT_SET / GA_LIGHT_WORK in
   ct3_types.h, MODEL_PARTS.lightColor / lightPos), mc_load_set, mc_char_set (a range table),
   mc_CheckMcWorkAllocc, mc_Getinfo_mc_type; data_load_set stubbed (next: it and data_save_set together,
   the save image's layout), left also mc_free_area, mc_check3, mc_save_system_init, ContinueBackupGet.
   Open: (av) MemorycardUpdate calls tiFade_Start(10.0f) with a0 (the fade type) unset: the port passes
   0, the fade TU says which types exist; (aw) PARTS_FLAG_FOG (bit 0) also switches a light part off in
   SceneLightSet / CharLightSet: probably a general hidden / off bit, rename when the parts loader is
   read; (ax) mc_load_set unregisters characters whose flags have 0xc0000000 (CHARACTER_FLAG_REFRACTION |
   _SHADOW, both named from one use): likely one in-use meaning; TrgCharacterDraw(mode, chara) and
   TrgSceneDraw(a0, a1, a2, name, bank) are declared from these calls (all leading arguments 0);
   (ay) MODEL_PARTS.attr is the EE address of the part's GA_LIGHT_WORK (BankEePtr), the word main_rtn
   copies to DRAW_OBJ_ATTR (+0x3c): gaSetLight's TU confirms the light block's other fields; ALL_TABLE.light
   is now SCENE_LIGHT * (main.c reads charaScale); slice 5 (2026-11-10): data_save_set, data_load_set
   (MC_SAVE_DATA, the save image; architecture.md "The save image"), mc_free_area, mc_check3,
   mc_save_system_init, MemorycardAutoLoad (its own states and messages, MC_AUTO_*) with Init /
   InsertedCheck / FreeCheck, ContinueBackupGet, the four mc_Getinfo_* readers, Set_AA_New_McAlloc_Mode
   -- **Character_Init.c's code is complete, verify** (nothing on screen reaches the card yet). Open:
   (az) the four MemorycardAutoLoad*Check functions now return int (the ROM's v0, hazards.md); (ba) the
   save image's 0xd590..0xdef0 and 0xe5cc..0x105d4 are not written by data_save_set: ssAI_MemSaveData
   (tiAvoidObject.c) and other savers fill them -- the TU that writes them names the ranges;
   (bb) data_save_set does not bound its record count by 35 (the ROM's); (bc) MC_BANK_* (0/1 scene,
   2 shape, 3 behaviour), TRIG_EVENT_CLEARED (0x0b), TRIG_EVENT_MC_LOADING (0x0c), TRIG_VAR_ROUND (0x59),
   TRIG_EVENT_GLBL_LOADED / _UNLOADED (0x23 / 0x25), TRIG_EVENT_TITLE_CANCEL (0x10), the events a load
   clears (9, 7, 8, 0x19, 10, 0x97, 0x9e, 0x15, 0x1a), MemorycardAutoLoad's answers 2 / 3, CAMERA.cut
   (+0x178), MODEL_PARTS.drawOnPos (+0x2f0) and PANIC_INFO's fields are named from this use only;
   (bd) CHARACTER.slot004[10] holds the character's behaviour banks (data_load_set compares the bank with
   each): rename it when the character TU reads it. Next: game/ArtoolDebugOpen.c, game/main2.c,
   game/fileio.c.
   **game/ArtoolDebugOpen.c (verify, 2026-11-10): all 15 functions** -- the file layer (architecture.md
   "The file layer"): IopLoadModules (now void: (ai) settled, the ROM reads no argument), FlInit (the
   pack's directory; a smoke run prints no FlInit error, so the directory loads and sceCdSearchFile finds
   the pack), the request queue and FlProc, FlLoad, FlGetLen, FlFindPacked. Host: io::Disc lays out the
   subfolders' files and answers sceCdSearchFile by name (item 3), sceCdBreak, SyncDCache. Open:
   (be) no smoke run reads a file yet (main2 / fileio.c are stubs): the first FlLoad / FlProc read is the
   real test; (bf) loose files (not in the pack) need IOPGetFileSize (game/IOPGetFileSize.c, stubbed:
   IOPComSync answers 0, so a loose file's size stays 0 and FlProc reads nothing): write that TU before
   any loose file is opened -- FSYS_IOP's answer is [0] size, [1] first sector (FlProc's use);
   (bg) CpGetEEIOPStatus's stub answers a zeroed record (the ROM: &gEEStat); EE_IOP_STAT's +0x50 is named
   iopBusy from FlProc's one use; (bh) gADSOpenSync and gSCD sit in lib/unassigned_data.cpp; gSCD is
   sceSifClientData[3] (120 bytes on the EE: 3 x 0x28), FlProc waits on [1]; (bi) FL_STATE_CANCEL (0x15)
   has no writer in this TU: whoever sets it names it; (bj) gValIOPGetFileLen (0x0193e350) is placed in
   this TU by address order, before IOPGetFileSize.c's static sFileNameToIOP: move it if that TU owns it.
   **game/main2.c (verify, 2026-11-10): all 18 functions** -- main2, InitLoadSceneChar (INFO / E_GL /
   0MH0 into global banks 5 / 3 / 1, then ATLD at power-on or TITL into bank 0), the scene / global /
   shape / behaviour triggers, CharacterAllFree, the self-discard queue (SELF_DISCARD_LOAD, main2.h).
   A smoke run now leaves main2 and runs the frame loop (no crash, ~26,000 frames in 15 s: nothing paces
   it yet); the shot stays black because FSMltLoad is a stub, so no bank ever loads. **game/fileio.c is
   the next blocker** (FSMltLoad / _End / _Status / _EndWaitFile / FSMltSetDrawBank, BankImage[] /
   BankImageEE[] for decision h). Open: (bk) PORT BankReady (main2.c) skips TrgGLBLSceneDraw /
   TrgSceneDraw's set-up while All_table_TBL[bank] or pModelPartsHeader[bank] is null -- drop it once the
   loader fills them (the ROM's FSMltLoadEndWaitFile returns only when the bank is in); (bl) CHARACTER's
   fields renamed from this TU: behaviorBank[10] (+4, was slot004: (bd) settled), shapeName (+0x18),
   behaviorName[10][10] (+0x22), brain (+0xfc, was w0fc), legParts[8] (+0x54c, the eight leg parts'
   EE addresses + 0x40 via the new BankEeAddr; their readers resolve them with BankEePtr(chr->bank, ..));
   MODEL_PARTS.hash (+0x48, u_long) and .matrix (+0x1c0) named; (am) CHARACTER.brain is an EE word:
   TrgCharacterShapeFree reads its +0x24e id through it, which is only safe while nothing sets it (the AI
   TU, tiAvoidObject.c / ssAI_*, decides the host form); (bm) ROM slips kept: CharacterAllFree passes a
   behaviour slot index where TrgCharacterBehaviorFree takes a bank; TrgCharacterBehaviorSet writes slot
   10 when all ten are taken; TrgSceneLoad / TrgSceneDraw answer the caller's s0 for a global bank (the
   port answers the handle / bank); (bn) EV_TITLE_23 / _25 (0x23 / 0x25: Character_Init.h calls them
   TRIG_EVENT_GLBL_LOADED / _UNLOADED -- one name should win), EV_SELF_DISCARD_LOADING (0x4a), EV_NO_E00
   (0xb6), FS_LOAD_* (FSMltLoad's kind 0..3), the enemy scene banks (E_02 0x27, E_03 / E_04 0x1e, E_05
   0x22, E_00 0x2e) and GLBL_BANK_* are named from this TU's use only; (bo) Character_Init(chara, a1, a2)
   (0x018b3a60) is now declared and stubbed in Character_Init.c: TrgCharacterShapeSet passes its own
   fifth then fourth argument; (bp) FogClearColor (main.c) is now shared through main.h.
   **game/fileio.c (partial, 2026-11-10, slice 1: 32 of 40 functions)** -- the bank queue (FQue[20]:
   FSMltLoad, FSMltInsertQuePriority, the cancels, IsQueWaiting / FsIsQueExisting, FSMltLoadStatus /
   _End / _EndWaitFile / _EndWaitAll), the bank table (FSMltInit / FSInit / FS_work_init, the finds,
   FSMltSetDrawBank, SceneEndBank), the whole-file queue (sPreLoad[6]: FSLoad, FSLoadEnd, FSLoadEndWait,
   FSLoadCancelQue, FSLoadProc), FileSoundServer(_DontMltLoad), FSFileType, fs_error, DebugBankDisp
   (architecture.md "The bank loader"). **Slice 2 (2026-11-10): FSMltLoadProc, Extract, ReserveBank
   written; the first global bank loads (TrgGLBLSceneLoad) and the run now crashes in scene_init
   (main.c, reading (*tbl)->baseobjCount) -- THE BLOCKER, item 1 (bv).** Left in fileio.c:
   SceneEndBankMovie, the sorts (CharacterSort, objSort*, SortAlphaObjects) and Round2Change*.
   (bv) ~~ahRelocateFileObject~~ written 2026-11-10 (game/ahRelocateSunPs2ObjectArry.c slice 1, with
   ahRelocateBehavior and ahRelocateSunPs2ObjectArry; architecture.md "The file relocator"): the global
   bank now relocates without a fault, and scene_init faults one step later (main.c, reading
   (*tbl)->partsHeader as a host pointer: it gets the counts at ALL_TABLE +0x38 / +0x3c, 8 and 0xc --
   real data, so the relocation is plausible). **THE BLOCKER is now the EE-layout conversion below**: one
   session, as a host task on game/main.c (queued at the top): ALL_TABLE's fields become u_int (the
   relocator proves every word 0x10..0xe0 but +0x14 / +0x38 / +0x3c is a pointer), every reader goes
   through BankEePtr (main.c ~45 sites, Character_Init.c 5, sound.c 3, main2.c 2, fileio.c 1); ALLobject[]
   and LinkMatrixTree / asBuildMirrorMatrixTree* take u_int lists; DRAW_OBJ_ENTRY (the sun object record,
   +0 draw object, +4, +8, +0x14, +0x18 pointers), TEX_INFO, PARTICLE_ENTRY and PARTICLE2_ENTRY go to EE
   layout with u_int pointer words (the relocator's offsets are their pointer fields). Was: **Next: ahRelocateFileObject (game/ahRelocateSunPs2ObjectArry.c, CT3 0x0183e710, a stub) and
   ALL_TABLE in EE layout ((f) settled by (h)'s rule, Claude's call 2026-11-10, the user may overrule):
   the bank image keeps the EE layout, so All_table_TBL[bank] = image + 0x80 must be read as u_int words
   through BankEePtr; ALL_TABLE's pointer fields become u_int and every reader (main.c, Character_Init.c,
   sound.c) goes through BankEePtr; the lists (objOpaque / objTrans / objAfter / particles*) are arrays of
   EE words, and DRAW_OBJ_ENTRY / TEX_INFO ((i), layout host) must follow. ahRelocateFileObject(image,
   persona) was declared host-side as (u_char *image, int persona): it relocates to the EE address
   BankImageEE[] of the bank holding the image (fileio.c fills it before the call; a global bank's EE
   address is GLOBAL_BANK_AREA_EE + its offset in gGlobalBankArea).** Queued at the top (tus.py queue).
   **Done 2026-11-10 (host task game/ct3_types.h, scheduled run): the EE-layout conversion.** ALL_TABLE,
   DRAW_OBJ_ENTRY, TEX_INFO, PARTICLE_ENTRY, PARTICLE2_ENTRY and PARTICLE2 are layout ee (ct3_types.h,
   static_asserts); every pointer word is a u_int read through the new BANK_PTR(type, bank, word)
   (BankEePtr with a cast); ALLobject[] is u_int * (a host pointer to a list of EE words, as are
   osTotalObjTable's sorted lists); ctrlNow / timNow in the main_rtn family compare EE words;
   LinkMatrixTree and ssPartMotionBlurInsertObjToDrawTable (stubs) take u_int lists -- their TUs need
   the bank to resolve the entries (add it, or resolve through the list's owner). A smoke run now runs
   scene_init to the end (all three draw lists walked, sunSetupTBufferData / AmbientAlphaInit reached)
   and the frame loop's updates; **THE BLOCKER is now main.c:2106**, the actress light: ActressMP is
   still NULL (main2's TrgCharacterShapeSet sets it only once Alyssa's shape bank has an ALL_TABLE, and
   that bank never loads in the smoke run): find why the shape bank does not load (FSMltLoad of the
   character shape, fileio.c) rather than guard the read -- the ROM reads ActressMP unguarded. Open:
   (bx) ALL_TABLE's end is unknown (the relocator reads to 0xe0 at least); +0x38 / +0x3c are named
   count38 / count3c (the +0x30 / +0x34 lists' lengths) until their reader names them; (by)
   PARTICLE2 +0x6c (obj6c) is relocated and unread; (bz) TEX_INFO.tim is resolved with the entry's own
   bank: a texture shared from another bank would need that bank -- none seen yet.
   **game/ahRelocateSunPs2ObjectArry.c slice 2 (partial, 2026-11-10):** the animation pass
   (asUpDateSceneAnimation, asUpDateGAobjectsAnimation, asCheckAnimationExecutable_Scene,
   asCheckNowMoving, asForceOperateAnimation_Scene / _Character), the shadow init (asShadowDrawing_Init /
   _Disable / _DetachLightPosition, Shadow_OnceInit, asGetShadowMatrixGlobalView), asInitMirrorMatrix,
   asGetBeziePosition, ExecuteAnimationHistory_init (architecture.md "Animation and shadows"). A smoke
   run reaches the new stubs asMakeStraw / ExecuteAnimation / asSeekBarAnimation / asSearchObject, then
   still crashes at main.c:2106 (ActressMP, unchanged). Next in this TU: BuildMatrixTree (0xa38, VU0)
   with BMT_01 / BMT_20, asMakeStraw, ExecuteAnimation (0x16bc), Execute_ExecuteAnimationHistory. Open:
   (ca) asMakeStraw and ExecuteAnimation take the bank as a PORT first argument (their words are the
   bank's EE words); the history records (lpEAS, 0x10 bytes: anim, tbl, a sign pair for AbsSignI32, a
   word stored to partsHeader +0x28) must carry the bank too -- whoever writes the history adder
   decides; (cb) BANK_INFO +0x30..0x70 (shadowVec, shadowLength, shadowColor, shadow64 / 68, shadowOrder,
   shadowDraw, shadow58 / 6d / 70) are named from the init and _CheckTransMode only; (cc)
   matrix_global_view_for_shadow is a symtab local, assumed this TU's (made static here); SkinWork and
   the EAS / shadow globals are defined here or in lib/unassigned_data.cpp with their EE addresses;
   (cd) ALL_TABLE +0x50 named `straw` from asMakeStraw's call only.
   **Slice 3 (partial, 2026-11-10):** asSeekBarInitializeForX / FreeForX / Animation, asSeatArrange_Init,
   BMT_01, ExecuteAnimationHistory_Free; asSceneAnimationStart (14 args, from the disassembly),
   asSeatArrange_ObjectMatchGlid / _FlameInit stubbed. **Next in this TU: BuildMatrixTree (0xa38) and
   BMT_20 (0x848)** -- read BuildMatrixTree from mipsdis.py, not the decompiler: it drops arguments
   (a `gaMulMatrix(dst, parent)` with two) and the record +0x8c parent matrix pointer is an EE word
   (BANK_PTR) or a host pointer InitMatrixTree / LinkMatrixTree set: decide with those (LinkMatrixTree.c);
   then asMakeStraw, ExecuteAnimation, Execute_ExecuteAnimationHistory. Open: (ce) **decided (Claude's
   call)**: ALL_TABLE +0xe8 holds a _SunAlloc'd SEEKBAR_X in the ROM; a host pointer does not fit the EE
   word, so the host keeps it in `SeekBarX[bank]` (PORT) and the word stays 0 -- a reader of +0xe8 in
   another TU must use SeekBarX; SEEKBAR / SEEKBAR_X are host layout (host-only memory); (cf) who fills
   SEEKBAR +0x20..+0x30 (hash, planePart, chara, anims) is not read yet: init zeroes them, so the bars
   stay idle until that TU; (cg) **MODEL_PARTS (ct3_types.h) is indexed from the parts header, but the
   records start at header + 0x40** (asFindModelParts answers header + 0x40 + i * 0x380; BMT_01 walks
   from +0x40 to a negative index): every MODEL_PARTS offset is the record's + 0x40, so fields at record
   0x340.. (BMT_01's view matrix) do not fit it -- BMT_01 uses record offsets (REC_*, PARTS_REC); main.c's
   `ActressMP + 0x1c0` is a record offset (= MODEL_PARTS 0x200), not MODEL_PARTS.matrix. A later session
   could re-base MODEL_PARTS on the record (a 0x40 header struct + records) across its users; (ch)
   asSceneAnimationStart's arguments 7..14 are unnamed (the seek bar passes the mode as one of 7 / 8 / 9
   set, and 0, 1, 0, 1, 0 after); (ci) the smoke run is not deterministic: of two 15 s runs one crashed
   at main.c's ActressMP read (frame ~1560) and one logged the same crash but ran on (the crash handler
   or the frame thread), so the ActressMP blocker stands either way.
   **Slice 4 (partial, 2026-11-10):** BuildMatrixTree (from the disassembly), asBehaviorStopAnimation,
   asSceneAutoStartAnimation, asStartAnimationFromTop, asShadowDrawing_Fin, asDrawShadowVolumeLoop,
   asGetForceMatrixOperationFlag, asMirrorPlaneCalc (vPVbk / vPPbk defined here),
   asGAobjectControlPolygonDisplayRotateFlagReverse; absi written early in QuatSlerp.c (BuildMatrixTree
   indexes by it); BMT_20, the shadow volume functions, asMakeWatchMeMatrixToPosition and
   OmniRangeChange stubbed. ~~**game/LinkMatrixTree.c is queued first**~~ written 2026-11-10 (verify: all 4 functions -- InitMatrixTree, LinkMatrixTree, ahTimeInit, ahTimeMaintenance; the parent words are MATUNIT_EE or BankEeAddr as (cj) decided, PartsParentMatrix's temporary 0 line removed; the tick globals moved out of unassigned data into LinkMatrixTree.h. Open: (cn) LinkMatrixTree finds its bank by matching pModelPartsHeader[] (PORT PartsHeaderBank): every caller passes pModelPartsHeader[bank]; (co) record +0x100 / +0x140 are the matrices a part's ga draw object reads through its words +0x58 / +0x5c (DRAW_OBJ_MATRIX / _MATRIX2), named from this one use -- the ga draw TU names them; a static part (kind 1) draws through its parent's; (cp) PARTS_KIND 3 keeps REC_SCALE (InitMatrixTree), kinds 0xd, 0x11, 0x15, 0x16 never hold their view matrices (REC_VIEW_HOLD 0) -- unnamed; record +0x2d0 / +0xd0 unread; (cq) ahTimeInit sets gGlobalNextTicks (not its interval) to 5 and takes 5 off gGlobalNextTicksInterval: kept as the ROM has it): a part's record
   +0x8c parent word is 0 in the file and LinkMatrixTree fills it (matUnit for a root, else the parent
   record's +0x180 / +0x200 matrix, same bank); until it is written PartsParentMatrix reads 0 as matUnit
   (a marked temporary PORT line: remove it with LinkMatrixTree). Next in this TU: BMT_20 (0x848),
   asMakeStraw, ExecuteAnimation, Execute_ExecuteAnimationHistory. Open: (cj) **decided (Claude's
   call)**: the parent word stays an EE address (bank data, decision h) -- LinkMatrixTree must store
   MATUNIT_EE (0x01939e40) for matUnit and BankEeAddr(bank, matrix) for a part; (ck) BEHAVIOR /
   BEHAVIOR_TRACK (ahRelocateSunPs2ObjectArry.h) are named from four small functions; asBehaviorStop /
   asSceneAutoStartAnimation find the behaviour's bank by the nearest BankImage[] below it (PORT
   BankOfHostPtr, also used for the polygon flag lists) -- a bank image size table would make it exact;
   (cl) BANK_INFO +0x58 is the bank's _SunAlloc'd shadow block: the host keeps it in ShadowBlock[bank]
   (PORT, like SeekBarX), so the allocator (asShadowPolygonPositions or asDrawShadowVolumeInit) must use
   it; (cm) the record fields BuildMatrixTree names (REC_DEPTH, REC_LPOS, REC_LINK_*, REC_WATCH_*,
   REC_PARENT, REC_LOCAL, REC_OFFSET, REC_BASE, REC_LIGHT, REC_BLEND, REC_PREV_SCALE_X, PARTS_KIND_OMNI /
   DIRECTIONAL / WATCH / NO_POS / LINK, WATCH_*) come from this one function; REC_FLAG_HOLD is the same
   bit as PARTS_FLAG_FOG -- one of the two names is wrong; asShadowDrawing_CheckTransMode's modes
   (SHADOW_TRANS_*) are named from asDrawShadowVolumeLoop's calls only.
   **Slice 5 (partial, 2026-11-10):** asCopyDoubleBuffer / asCopyDoubleBufferL2V (BANK_INFO.drawOnPrev,
   CHARACTER.flagsPrev, gNoDraw), BMT_20, Execute_ExecuteAnimationHistory (EAS_RECORD, FrameCounter;
   AbsSignI32 written early in QuatSlerp.c), asSearchObject (SEARCH_OBJ, ALL_TABLE.search),
   asSeatArrange_FlameInit / _ObjectMatchGlid, asCheckAnimationExecutable_Character,
   asShadowDrawing_Enable / _SetTransOrderBW (architecture.md "Double buffer, seats, searching parts").
   ahCos, ahAcos, asMakeRotMatrixByQuartanion (QuatSlerp.c) and gaInnerProductXYZ (gaInit3dlib.c) are
   new stubs. **Next in this TU: asMakeStraw (0xcdc, VU0 + quaternions: read it from the disassembly,
   a slice of its own), then ExecuteAnimation (0x16bc)** -- both reached every frame. Open: (cr)
   **decided (Claude's call)**: EAS_RECORD is host layout {anim, tbl, sign, step, bank} -- the ROM's are
   0x10 bytes; the history adder (not found yet: whoever _SunAllocs lpEAS) must write these and set
   iEASidx; (cs) the parts header +0x20 / +0x24 / +0x28 (PARTS_HDR_ANIM_*), ALL_TABLE.ctrl's first list
   (OBJ_CTRL_ATTACH_*), SEARCH_OBJ's fields, the seat grid's bits and SEAT_* are named from one use each;
   (ct) asSearchObject's "nothing seen" flag is set once per call and never cleared (the ROM's), so one
   blind part makes every later part of the bank w 0 -- kept; (cu) asCopyDoubleBuffer / asSearchObject
   / asSeatArrange_* read All_table_TBL and the eye parts unchecked in the ROM: the host skips a NULL
   (PORT lines); (cv) CHARACTER_FLAG_SHADOW is used as "in use" by asSearchObject and the double buffer,
   with (ax): rename it when the character TU reads the bit.
   Slice 6 (2026-11-10, asMakeStraw, SetExecuteAnimationHistory, asGetBehaviorPlayableTicks): (cw)
   asMakeStraw reads parts A / B at record +0x1c0 (REC_MATRIX_PREV, BuildMatrixTree's kept world
   matrix) and +0x20 (REC_LPOS) from the disassembly -- if straws lag a frame on screen, that is the
   ROM's; (cx) STRAW / STRAW_TABLE / STRAW_SKIN and the ga object's +0x48 / +0x60 / +0x64 (GAOBJ_*,
   normals after a 0x10 header) are named from this one use, and the ga object and its arrays are
   read as bank data (BANK_PTR, decision h): if gaInitDrawObject builds them on the host instead, the
   skin list needs the host form; (cy) QuatSlerp(q, q0, q1, ts) takes ts = {1 - t, t},
   asDelSqueezeInformation(out, a, b) answers the twist, ConvertMatrix2Quat / ConvertQuat2Matrix /
   ssCalcInversMatrix2 take (out, in): confirm in QuatSlerp.c / Convert.c / ssCalcInversMatrix2.c (new
   stub TUs); (cz) SetExecuteAnimationHistory is the history adder (cr): callers EA_100 and
   ExecuteAnimation pass the bank in front (PORT); BEHAVIOR_TRACK +0x10 is now `playable`.
   Open from slice 7 (2026-11-10): (da) asBuildDOFMatrixTree reads a matrix at record +0x1c0
   (REC_DOF_MATRIX) and a scale diagonal at record +0x4c / +0x5c / +0x6c (REC_DOF_SCALE_*), named from
   that one use; kind 3 (PARTS_KIND_3) is skipped there, meaning unread; (db) ALL_TABLE +0xcc is `mirror`
   (MIRROR_LIST, MIRROR 0x50 bytes: part index + three plane points at +0x10); mrr_pActvMirrorMO / _MP
   are host pointers defined in this TU (asMirrorCulling and the mirror tree read them); (dc) EA_001's
   track kinds 0x0b / 0x0c / 0x1f / 0x20 (TRACK_KIND_*), TRACK_KEY_RAW, the part hash and
   BEHAVIOR_GROUP +0 `part` are named from EA_001; InterpolateAnimationKey (0x0184dbc0, this TU, now a
   stub) is assumed to take (track, dir) and answer 0 when the track ran out; asZipTrans2Trans_XYZ1 /
   asZipQuat2Quat (QuatSlerp.c) take (out, in[, shift]).
   Open from slice 8 (2026-11-10, ExecuteAnimation's dispatch, EA_100): (dd) InterpolateAnimationKey
   answers 1 when the track's time reached its end (its decompilation: clamped there in flags & 3
   modes 0 / 1), and EA_100 reads it so (1: stop or restart the track); EA_001 sets its done byte on
   0 -- check EA_001 against its disassembly (the slot bytes may mean "still running"); (de) the part's
   pose slots PARTS_ANIM_ROT / _POS / _BASE_ROT / _BASE_POS / _VECTOR (0x280..0x300), PARTS_ANIM_DIRTY
   (byte 0x43 bit 0), TRACK_KIND_VECTOR 0x0d and ANIM_CTRL_HOLD / _ONCE (header mode byte 1 / 2) are
   named from EA_100 alone; the header words PARTS_HDR_ANIM_ABS / _SIGN / _STEP are ExecuteAnimation's
   ticks left / direction / step length (AnimWeights); (df) ExecuteAnimation's general body (any flag
   mix but one-hot, 0x0184a688..) is an ALYSSAPAN_STUB inside the function: write it when
   asCharacterAnimationExecute / asResetCharacterPosition (the callers that can mix flags) are written;
   its shape kinds ('*', '3', '>', '=', 'H', '4', '"', '!', ')') read ALL_TABLE +0x18 / +0x30 / +0x40 /
   +0x44 lists, as EA_010 will; (dg) asZipVECTOR2VECTOR_XYZ1 and asInterpolateVector (QuatSlerp.c)
   written early from their disassembly; QuatSlerp / ConvertMatrix2Quat / ConvertQuat2Matrix /
   asZipQuat2Quat / asZipTrans2Trans_XYZ1 / gaMulMatrix are still stubs, so EA_100's poses are zeros
   until those TUs land.
   Open from slice 9 (2026-11-10, InterpolateAnimationKey / IAK_0, the play range, behaviour lookup,
   shadow setters, asCopyDoubleBufferL2W): (dh) (dd)'s reading confirmed for both functions (1 = the
   time reached `playable`: forwards at or past it, backwards at or below it); EA_001's done byte on 0
   is still unchecked; (di) BEHAVIOR_TRACK +0x14 is `rangeStart` (asChangePlayRange; a looping track goes
   on from it) and `playable` is the range end; flags & 3 = TRACK_PLAY_ONCE / _ONCE_REVERSE / _LOOP /
   _LOOP_REVERSE, bit 0 also the direction; TRACK_SLOW_STEP (value0e == anim_play_slow snaps the time
   to 500); the kinds '"' '!' ')' '*' '3' '4' '=' '>' 'H' and ANIM_SUBKEY (0x18-byte sub-records of the
   vertex kinds, +8 an EE data word, +0xc a count, +0x14 'H''s short) are named from this one function;
   (dj) written early in QuatSlerp.c from the disassembly: asMemCopy128c / 16c, asQuat2ZipQuat,
   asZipQuat2Quat, asQuatSlerpZipVECTOR, asTrans2ZipTrans_XYZ, asZipTrans2Trans_XYZ1,
   asInterpolateZipTrans_XYZ, asZipVECTOR2VECTOR_XYZ0, asVECTOR2ZipVECTOR_XYZ, asInterpolateZipVECTOR_XYZ,
   asZipUV2UV, asInterpolateNumber (the old stubs of asZipQuat2Quat / asZipTrans2Trans_XYZ1 carried the
   wrong addresses 0x0185c6d0 / 0x0185cdb0: fixed). Still stubs there: asInterpolateZipUV,
   asInterpolateZipFloat0_7_9X / 1_1_14X, asContInterpolateZipNormal_XYZ / ZipTrans_XYZ / Vector (the
   vertex kinds); QuatSlerp itself (Convert.c) is still a stub, so rotation keys blend to zeros;
   (dk) asShadowDrawing_SetShadowAngleLockParameter takes its floats in f12 / f13 and the bank in a0:
   the C order (bank, a, b) is a guess for ssGrpPortalCheck.c's call site; BankInfo +0x64 / +0x68 are
   its two floats, +0x6d the angle lock, +0x6c (shadowOrder) is what SetShadowValue sets;
   (dl) asFindBehavior / _ByCharacter answer host BEHAVIOR pointers (the list words resolved by
   BankOfHostPtr's bank); BEHAVIOR +0 is its 64-bit `hash`.
   Open from slice 10 (2026-11-10, the shadow pass order, asSceneAnimationStart,
   asUpdateAnimation_Matrix, the play rate): (dm) CHARACTER +0x350 is the EE word of the character's
   motion table (0x30-byte records: hash, +8 the motion's hash), written by asInitPlayerBehaviorControler
   (wideXcmd_DebugPrint.c, a stub): asSceneAnimationStart's SCENE_ANIM_CHARACTER + toEnd path reads it
   through CharaMotionTable(), which answers NULL (an empty table, index 0) until that TU gives the word
   a host form; (dn) asShadowDrawing_CheckTransMode's mode-0 sort reuses one register as the character
   loop's index and the count of shadowOrder changes (the disassembly, 0x01847918..0x01847a1c): a change
   skips the next character and iShadowTransModeFin always ends 1; kept as the ROM does it -- a smoke
   run with several shadowed characters of mixed order is where it shows; (do) the two hashes of
   CHARA_AXIS_HASH_ROW1 / _ROW2 (asUpdateAnimation_Matrix, CHARACTER.refractAxis) have no names yet;
   (dp) BEHAVIOR_TRACK +0xe is `playRate` (percent; was value0e): asSceneAnimationStart sets speed * 100,
   or 0 with toEnd. Left in this TU: EA_010, the general body of ExecuteAnimation, asMirrorCulling,
   asShadowPolygonPositions and asStockShadowVolume (one piece: ga draw objects with the
   ShadowVolume1_* VIF data, through the host draw module), asDrawShadowVolume / _Init, asShadowEnd,
   asShadowStart and the rest of the brief's list.
   Open from slice 11 (2026-11-10, the mirror trees, asMirrorCulling, the shadow volume pass): (dq)
   CAMERA +0x40 is named `view2` from one use (asBuildMirrorMatrixTree multiplies Cam[6]'s in where
   the DOF tree has matrix_global_view): the camera TU names it; (dr) MIRROR holds four corners at
   +0x10 (not three and a pad): asMirrorCulling projects all four; (ds) gaMulMatrix must allow
   out == b (asBuildMirrorMatrixTree: m2 = view2 * m2; the VU code works in registers) -- the
   gaInit3dlib.c TU must keep that; (dt) asShadowStart passes sunSetupTBufferData a TEX_INFO whose tim
   is the ELF data address 0x01937c80 (tex_attr_A$2750, 512 x 448 log2 9 x 9, the shadow buffer read
   as a texture), not a bank address: the QuatSlerp.c TU that writes sunSetupTBufferData resolves it
   (bytes in this slice's comment); (du) ALL_TABLE +0x6c is `shadow` (the relocator's 7-word table with
   8 key tables; word 0 the volume's ga draw objects) and BANK_INFO +0x50 `shadowFrame` (the
   FrameCounter of the stock), both named from asDrawShadowVolume: asStockShadowVolume confirms them;
   (dv) the volume object's kind byte (+1): 1 / 5 subtract (SDW_drawctrl_S), 2 / 6 add
   (SDW_drawctrl_A), 6 ends the list; what tells 5 from 1 and 6 from 2 is asStockShadowVolume's;
   (dw) asDrawShadowVolumeInit's four stores into SDW_drawctrl_A +0x10..+0x1c (ShadowVolume1/2 tex_st
   and vertex colour addresses) are overwritten by gaSetupDrawCtrlData's packet head at once: dropped,
   unless the ga TU finds the head keeps them. Left in this TU: EA_010, the general body of
   ExecuteAnimation, asShadowPolygonPositions + asStockShadowVolume, asShadowEnd, and the brief's rest.
   Slice 12 (2026-11-10): (dx) TRACK_FLAG_STEP_FROM_RUNS (bit 0) is what asCheckAnimationDirection_X
   answers: probably "plays in reverse" (step starts at the end); rename when IAK's reverse path
   confirms it. (dy) asSceneAnimationStop_S stops a character's motion slots only under
   CHARACTER_FLAG_REFRACTION: the bit likely means more than refraction (an "active / drawn"
   character); CHARACTER +0x398 is short[6] motion slots (CHARA_MOTION_SLOTS). (dz) A first read of
   asStockShadowVolume for (dv): mode 2 flushes the pending odd plane (_iPcnt_3177: kind 5, gMP) and
   odd quad (_iMcnt_3178: kind 6, gMM) as one-face volumes (ShadowVolume1_*); mode -1 stocks 4 corners
   into gMM and emits a two-face volume (ShadowVolume2_*, kind 2) on every second call. Both write a
   gaCopyObject2 of the template at 0x01937ba0 and the face normal (cross of two edges, rsqrt).
   (bw) Extract is untested against a known image: the first scene_init that runs on relocated data is
   its test (a wrong decode shows as garbage tables); a harness decoding one CT3X file from CT3PACK.DAT
   beside the ROM's (eerun.py) would prove it sooner.
   Open: (bq) BANK_INFO renamed from the loader's use: +0 `state` (was `count`; BANK_STATE_*, a group's
   later banks hold their place 2..n), +0x20 `drawOn` (was `loaded`: FSMltSetDrawBank's mode), +0x1c
   `image` (EE address), +0x24 `handle`; every user was updated. A bank's type is the FS_LOAD_* kind it
   was loaded as (FSFileType: 0 GLOBAL, 1 SCENE, 2 / 3 PERSONA), so BANK_TYPE_SHADOW is the scene bank,
   BANK_TYPE_2 the shape bank and BANK_TYPE_NO_ROOM_SOUND the behaviour bank: rename them to
   BANK_TYPE_SCENE / _SHAPE / _BEHAVIOR in one pass ((j) settled); (br) FSMltInit gives banks 6..85 EE
   slots 0x00410000 + (i - 6) * 0x40000 -- the first twelve overlap the GLBL heap (EE 0x00400000, 3 MB):
   FSMltLoadProc / ReserveBank say how the ROM keeps them apart; the host gives the slots their own 20 MB
   (gBankSlotArea) and fills BankImage / BankImageEE for them; global banks get theirs when
   FSMltLoadProc allocates them (BankImage[i] from _SunAlloc, which SceneEndBank now frees); (bs)
   settled 2026-11-10: FQUE +0x00 is `dir` (FSMltLoadProc fills it with FSFileType, opens dir + name); (bt) BANK_FLAG_SCENE_INIT (flags bit 0: SceneEndBank
   calls scene_end) -- settled: FSMltLoadProc sets it on the group right after scene_init; (bu) FSMltLoadStatus's
   answers: main2.c reads "non-zero" as "still loading", but FS_STATUS_LOADED (1) is non-zero too, so
   main2's waits also run for loaded files (harmless: FSMltLoadEndWaitFile returns at once).
2. **The siblings' shims (the user's rule of 2026-11-09: "reuse the shims of the other pan projects like
   the windows handling and all the rest")**, as host tasks, in cpp-port.md "To carry over" order, each a
   class: ~~(a) gamecontrollerdb + icon + cursor auto-hide + pause / mute in the background + DPI scale~~
   (done 2026-11-09; pause in the background is written but no run has exercised it: smoke windows sit
   in the background, so turning it on there would freeze the run); (b) exclusive fullscreen, window position and presets; (c) the HDR swapchain,
   MSAA, anisotropy, GPU driver choice; (d) ~~the real memory card~~ (done 2026-11-09: `io::MemoryCards` behind
   sdk/libmc.cpp; a scratch test wrote, listed and read a save through sceMc* calls; no smoke run reaches
   McInit yet, so the game's own mc_* code has not exercised it), reverb and volume
   groups (YamiPan's SPU2 is next to them), the audio dump; (e) `libsdr` (CT3 calls `sceSdRemote`), newlib
   `rand`; (f) .iso reading, save import / export; (g) RmlUi menus and translations; (h) CI and AlyssaPan's
   own `extern/`. A shim CT3's boot needs earlier (libsdr, the card) moves ahead of TUs when a run hits it.
3. ~~**io::Disc's path table is root-only**~~ (2026-11-10, game/ArtoolDebugOpen.c): CT3 reads sectors
   through libcdvd and finds the pack with sceCdSearchFile, so the subfolders' files now get sectors and
   `Disc::find` answers by name; the ISO records still list only the root (no CT3 code reads them).

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

- (2026-11-09, ga/gaDma.c) gaDmaOpen_MFIFO_VIF1 writes `ga_dma_env.mfd = 1` and D_CTRL with RELE set,
  RCYC cleared and MFD untouched; D_CTRL's MFD for VIF1 is 2. So either the SDK's sceDmaPutEnv translates
  mfd (1 -> VIF1), or the ga library's D_CTRL bitfield struct differs from the EE manual's. Harmless on the
  host (no MFIFO); read sceDmaPutEnv (0x01811b78) when the BGT ring buffer (gaBGT_*) TU needs it.
- (2026-11-09, ga/gaDma.c) The host's `sceDmaSend` (chain) does nothing on any channel, toSPR included.
  If a TU chains packets into the scratchpad (gaDmaSendS_toSPR has no caller yet), libdma must walk tags.

- (2026-11-09, ga/gaInitDrawObject.c) gaSetupDrawCtrlData adds its 27th word at bit 15 of TEX1_1, where
  the GS has nothing (L is bits 19..20): kept as the ROM does it (`GA_DRAWCTRL_ARGS.tex1Bit15`). Does the
  ga library mean L there (a bug in the game, harmless while the word is 0), or another register?
- (2026-11-09, ga/gaInitDrawObject.c, Claude's call) The scratchpad keeps its EE layout and its address
  words hold EE addresses (u_int): +0x14 `ga_div_redraw_gspacket_work | 0x20000000`, +0xec the 3dlib work
  (0x70000400), +0xf0 the BGT packet area (0x70000a00). A host reader maps a scratchpad address with
  SCRATCHPAD(ee - SCRATCHPAD_EE_BASE); `ga_div_redraw_gspacket_work` (bss 0x01957e40, 0xb0000 bytes) has no
  host definition yet: its reader's TU adds it and the mapping (with the BankEePtr precedent, (h)).
- (2026-11-09, ga/gaInitDrawObject.c) GA_SPR_WORK's floats from the 3dlib work are named by index
  (`work21`, `work23x16`...): name them when gaSet3dlibStruct's TU names ga3dlibwork's fields.
- (2026-11-09, ga/gaInitDrawObject.c slice 2) gaSetupTBufferData / gaBGT_LoadGSTexClutData_VIF1 have no
  ROM caller: `GA_TEX_DESC` / `GA_TBUFFER_DATA` are named from their loads alone, address words kept as EE
  u_int. QuatSlerp.c's `sunSetupTBufferData` (TEX_INFO by value) is the live copy: compare it when that TU
  is written, and decide there how the host turns these uploads into GPU textures (decoded at the source).

## Done

- 2026-11-09: AlyssaPan created: the repo, YamiPan's Pan layer (C++, HEAD 17e0108 plus 93c73f0's SPU2
  voices and sound card) trimmed to the generic, SDK, CMake, tools, the skill and its references (this
  file, architecture.md, cpp-port.md, bootstrap.md, reconstruction.md, ghidra.md, hazards.md with the
  siblings' digest). Build green, first smoke run and first `refresh.py` done. Nothing committed: the
  user decides.
