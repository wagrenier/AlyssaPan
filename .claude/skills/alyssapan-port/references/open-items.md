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
   is the next sound blocker (slice 1 written 2026-11-10: the FSS command ring and thread, volumes, reverb, SE stop / lookup, load-end wait, SdAlloc / SdFree, SdRnd; left: SdProc and its five procs, SE play / load / supply, SdSetParamSE, SQ and the stream's play; the IOP side, CpCall_IOPSndDriver, is a stub, and FSS_Thread_Init runs only once IopLoadCapcomSound is written; slice 3 written 2026-11-10: SdReqLoadSE / FindSEFile x2 / SdReqLoadSECancel / SdPurgeSE and the SE play side -- SdReqPlaySE, SdSetParamSE, NextPlaySE, SdPlaySE, SdSupplyCHSE, SdPlaySEUpdate, SdPlaySEProc, SdPlaySESpProc; the SE loads now start at boot and step to the CpSifDmaTransEEToIOP / CpCall_IOPSndDriver_2 stubs. Left: SdLoadSQProc, SdReqPlaySESp1, the SE stops (SdStopSEAll, SdStopSESDT, Sp1), SQ play / stop / volume, the ADS stream, SdSoundEnable / Disable, SdChangeStereoMono, IopLoadCapcomSound. Slice 4 written 2026-11-10: the SE stops (SdStopSEAll, SdStopSESDT, the Sp1 three), SdReqPlaySESp1, IsPlaySEReady, the load waits, SdPlaySQ / SdStopSQ / SdStopSQAll / SdStopSQAllNow / SdVolumeSQ, SdPlayStream, SdSoundEnable / Disable, SdChangeStereoMono (it takes the stereo flag: sound.c now passes gStereo). Left: SdLoadSQProc, SdPlayADSProc, IopLoadCapcomSound. Slice 5 written 2026-11-10: SdPlayADSProc (SD_ADS_PHASE), IopLoadCapcomSound (FSS_Thread now runs from boot; CpDivideEECom.c's CpCall_IOPSndDriver stub answers a zero reply word, not NULL, because FSS_Thread reads it), SdSetParamSESDT, SdFindLoadSEInfo (SD_FIND_LOAD_SE_*). Slice 6 written 2026-11-10: SdLoadSQProc (SD_SQ_LD_STEP, SD_SQ_REGISTER, SND_CMD_SQ_*) -- **the TU's code is complete (verify)**: nothing plays until the IOP side answers; CpDivideEECom.c (the IOP side's EE half: 5 new stubs) is the next sound blocker. Open from slice 6: (av) SND_CMD_SQ_0C / 0D / 17 / 18 / START are named by the step that sends them, the IOP side unread; the SQ reuses SND_CMD_SE_REGISTER / SE_PURGE with the player (gSQi, always 0) in place of the slot; (aw) the ROM's slips kept: step 6 prints SdLoadSEProc's DMA message (@2330), steps 3 / 6 / 9 keep fd after FlReqClose, SD_SQ_LD_STOP (0x1b) sends the stop twice, 0x16..0x18 are listed but never set. Settled by slice 6: (ap) SD_SQ_LD +0x70 / +0x74 effectType / effectDepth (SdSetEffect core 0 after effectDelay +0xa4 frames), +0x78 spuAddr, +0x7c / +0x80 / +0x84 iopSq / iopHd / iopBd (the old iopBd at +0x80 was the .HD), +0x88..+0x90 sizes, +0x98 fd, +0xa8 stopWait; gFileSQ's sdArg2 / 5 / 6 are spuAddr / effectType / effectDepth (x settled too); (aq) the swap: once row 0 has ended (0x1d), row 1 is copied into it and cleared (0x1e). Open from slice 5: (at) SND_TRANS_ADS.path is 0x40 bytes, not 0x60 (SdPlayADSProc writes +0x48..+0x80); its other words and gArg_CpDriverInit's (SND_DRV_INIT) are named by value only -- the IOP side names them; (au) SND_CMD_ADS_OPEN / ADS_START / INIT_08 / INIT_07 / INIT_25 and EE_IOP_ADS_OPENED (sndFlags bit 1) are named from the EE side only; SND_CMD_ADS_ENABLE (0x2a0000) is also IopLoadCapcomSound's stream set-up, so it may be an init rather than an enable. Open from slice 4: (ap) SD_SQ_LD +0x70 / +0x74 / +0x78 / +0xa4 are named arg5 / arg6 / arg2 / arg4 after SdPlaySQ's arguments and +0x7c / +0x80 iopSq / iopBd after SdStopSQAllNow's frees: SdLoadSQProc names them; (aq) SdPlaySQ over a playing row 0 writes row 1 and flags row 0 to stop: the swap is SdLoadSQProc's, unread; (ar) EE_IOP_STAT +0x40 bit 0 (EE_IOP_SND_READY) and the new SND_CMD_* words (STEREO, STOP_SQ_NOW, ADS_STOP, SOUND_ON / OFF, ADS_ENABLE / DISABLE) are named from the EE side only; (as) SdStopSESp1 sets no return value in the ROM (SSStopSESp1 passes v0 on): the port answers 0. Open: (am) SD_SE.unk30 (the play word's third byte, 999 by default) and reqArg2 (>= 1000 resident, 0x3e0 shares one channel) are named from the EE side only; (an) SdPurgeSE sends gSELd[gLdi].reg, not its own row's, kept as the ROM's slip; (ao) PLAY_SE_SP1 +0x20 was named state: it is posVol (MidiControl_DemoStart's 2 = two tenths), renamed in sound.c), so nothing sounds; its prototypes in
   SdMasterVolumeSetSE.h come from sound.c's call sites; (x) gFileSQ's words +4 / +8 / +0xc are
   SdPlaySQ arguments named sdArg2 / sdArg5 / sdArg6 until that TU reads them; (y) TRIG_VAR_STAGE (0x1d),
   TRIG_EVENT_SE_MUTE (0xb7), the stage-1 SQ events 0x3b1 / 0x3ba / 0x3bd / 0x3bf and STAGE_BGM_* are
   named from sound.c's use only; (z) gBfoScene / gNowScene / gStereo / gMasterVol* / gPlaySESp1 sit in
   lib/unassigned_data.cpp until their owner, with the ROM's initial values), slice 2 (2026-11-09: 68 of 69 -- SSProc /
   SSBGMProc / Branch1..5BGMProc, the sound objects and trigger actions, positional SEs, volume save and
   restore, SSPlayADS, SSPlaySESp1, SceneSELoad / BranchSELoad; left: SSLoadSEfromLoadPoint, which reads
   fileio.c's load-point record +0xd4 (a pointer to up to 5 rows of 0xc bytes, name pointer first): write
   it with fileio.c's TU. Open: ~~(aa)~~ settled 2026-11-10 (Sd driver slice 3, Claude's call): SD_SE and PLAY_SE_SP1 are `layout: host` with an `SD_SE_UPDATE` host function pointer, called with the SE's handle. Was: `SD_SE.callback` (+0x50) is an EE code word in a 0x54-byte record,
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
   real test; (bf) loose files (not in the pack) need IOPGetFileSize -- **game/IOPGetFileSize.c written
   (verify, 2026-11-10: all 4 functions; gSCD moved there from lib/unassigned_data.cpp)**, but no host
   server answers FSYS_IOP's 'SUNA' (0x53554e41, fno the same number, send the 0x40-byte name, answer 0x10
   bytes: [0] size, [1] first sector), so sdk/sif.cpp answers zeros and a loose file's size stays 0: a host
   task (an FSYS_IOP server on io::Disc, registered like YamiPan's rxsnddrv) before any loose file is
   opened; 'SUN@' (IOPComTest) and 'SUNB' (IOPSetSDT) are bound but no ROM code calls them;
   (bg) CpGetEEIOPStatus's stub answers a zeroed record (the ROM: &gEEStat); EE_IOP_STAT's +0x50 is named
   iopBusy from FlProc's one use; (bh) gADSOpenSync and gSCD sit in lib/unassigned_data.cpp; gSCD is
   sceSifClientData[3] (120 bytes on the EE: 3 x 0x28), FlProc waits on [1]; (bi) FL_STATE_CANCEL (0x15)
   has no writer in this TU: whoever sets it names it; (bj) gValIOPGetFileLen (0x0193e350) is placed in
   this TU by address order, before IOPGetFileSize.c's static sFileNameToIOP; the symtab lists it as a global outside that object's local run, so it stays in ArtoolDebugOpen.c (2026-11-10).
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
   ~~SceneEndBankMovie, the sorts, Round2ChangeSectInfo~~ (slice 3, 2026-11-10); Round2ChangeChar.
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
   Slice 13 (2026-11-10): (ea) the shadow volume's stock is written (asStockShadowVolume, settles dv /
   dz: plus quads kinds 1 / 5, minus quads 2 / 6; the flush's lone minus quad is the list's last, no end
   mark). Its objects keep the ROM's EE addresses of ShadowVolume1/2 index, ST and colour data and the
   template's light-work words (0x01937820 / 0x01937830) and 0x01941840: the gaBGT_DrawObject TU must
   resolve them (as tex_attr_A in asShadowStart). (eb) The caster positions' w is the ROM's, unclamped:
   segment rings carry w = 1 + radius (the vmulx.xyzw / vadd.xyzw fields), polygon vertices w0 = M * v's
   w; whether the VU1 volume microcode reads w decides if it matters. (ec) SP_ANCHOR_HASH
   0x74dda77d0cbcf0c2 (a character bank's shadow anchor part) has no name yet: hash the shape files' part
   names (ahGetHash) to find it. (ed) asViewMatrix2PosRUF is called with one buffer for all four outputs
   (the camera position): whichever it writes last wins, so its TU decides what asShadowPolygonPositions
   tests against. (ee) asCheckAnimationState_S indexes a 5-byte stack array by the track's playing byte
   unchecked (the host bounds it); CHARACTER +0x3b0 is a 42-entry motion request queue with modes at
   +0x404 (CHARA_MOTION_QUEUE*): the motion TU names them. gaCopyObject2, gaInnerProductXYZ and
   asVectorLength3D were written early in their TUs.
   Slice 14 (2026-11-10): asCheckAnimationFlamePassing_S, asShadowEnd and EA_010 written; left in the TU:
   ExecuteAnimation's general body (a marked stub, no caller reaches it yet). (ef) asCheckAnimationFlamePassing_S's
   scene branch tests a backward track's wrap with its first condition again (the decompiler's reading,
   kept: such a track passes only the frame it came from); PASS_DIR_* (1 forward, 2 backward, 3 both) and
   PASS_FRAME_ANY (-10) are named from this one use; the argument order of its caller
   ssTrigCheckConditions (unwritten) is the ROM's slots. (eg) asShadowEnd: asShadowDrawing_CheckTransMode(1)
   < 2 lays the shadow buffer over the screen as 8 x 7 sprites, >= 2 as four Gouraud fans about the
   projected BANK_INFO.shadowAnchor (renamed from pad40); BANK_INFO.shadowOrder is also the overlay's
   strength in percent (negative: lighten with W, positive: darken with B) -- rename it when its other
   users are next touched; the fans clamp x to X + W / 0.832 (615) while their corners stop at 512, as the
   ROM. The overlay needs sunSetupTBufferData (QuatSlerp.c, a stub) to bind the shadow buffer as a texture.
   (eh) EA_010 names ALL_TABLE +0x30 copyWords, +0x40 objGroups ((first, count) pairs), +0x44
   objGroupIndex (short indices into objOpaque), ANIM_SUBKEY +0x10 obj, the ga object's +0x78 UV offset
   and the colour data's alpha bytes (+0x10 + 4 i + 3) from this one use. TRACK_KIND_COPY stores a
   copyWords word (an EE value, unresolved) through the sun object record's word 2: the gaBGT_DrawObject
   TU decides what that word is. EA_010 keeps the ROM's one stack vector across tracks (`held`, zeroed
   where the ROM's lanes are stale). The QuatSlerp.c Cont* writers, asZipTrans2Trans_XYZ0, the asDR_*
   GS words and asZipFloat0_7_9X2Float were written early.
   Slice 15 (2026-11-10): ExecuteAnimation's general body written (any mix of play / replay / matOnly
   other than one alone): **the TU's code is complete (verify)**; EA_100 and EA_010 now share its per-track
   helpers (PoseTrack, MatKey, ReplayTrack). (ei) The general body shifts every track's time1c *before*
   InterpolateAnimationKey, EA_100 *after* (both as their ROM bodies): a pass test
   (asCheckAnimationFlamePassing_S) after a mixed call sees the old time where EA_100's sees the new; kept.
   (ej) The ROM's one stack vector is shared by the pose kinds' temporaries and the replay kinds (the
   general body's FLOAT0_7_9 blend reads lanes y..w a pose track wrote there): the port passes one
   scratch vector through both, zeroed at entry where the ROM's is stale. The two written callers pass
   one flag each; asCharacterAnimationExecute and asResetCharacterPosition (unwritten, other TUs) are
   the ones that may mix them: when they run, the shot decides.
   (bw) Extract is untested against a known image: the first scene_init that runs on relocated data is
   its test (a wrong decode shows as garbage tables); a harness decoding one CT3X file from CT3PACK.DAT
   beside the ROM's (eerun.py) would prove it sooner.
   **game/fileio.c slice 3 (partial, 2026-11-10):** the sorts (CharacterSort, objSort,
   objSortWithBlurIdx, SortAlphaObjects: CAM2OBJ rows in pCam2objSort, a merge sort far to near that
   is not stable -- on equal distances the second half's row goes first), SceneEndBankMovie (parks
   banks at BANK_STATE_MOVIE_PARKED + i instead of clearing them), Round2ChangeSectInfo. Left:
   Round2ChangeChar (0x878). The smoke run passes CharacterSort and still crashes at main.c:2106
   (ActressMP NULL: asFindModelParts, QuatSlerp.c, is a stub that answers NULL; so is the new
   asVectorDistance stub, which answers 0, so every sort keeps its input order). Open: (ek)
   CharacterSort counts a character only with both CHARACTER_FLAG_REFRACTION and _SHADOW set: the two
   bits are more likely 'exists' / 'drawn' than refraction / shadow -- rename them when their setter is
   read; (el) PART_BLUR_OBJ (+0x110 listIndex, +0x114 sortedIndex), ALLpartBlurObj[2][86][48],
   NumPartBlurObj[2][86] and aftSortALLpartBlurObj (PORT: 49 slots, the ROM writes [count]) sit in
   lib/unassigned_data.cpp until the ss part-blur TU (ssSetTexUVforCubeMap.c) owns them; (em)
   PARTS_FLAG_SORT_CENTER (0x4), DRAW_OBJ_CENTER (+0x10) and the record's +0x20 sort point
   (REC_SORT_POINT, MODEL_PARTS +0x60) are named from the sorts only; CharacterSort and main.c's
   actress light both read record +0x1c0 (REC_MATRIX_PREV) with the +0x20 point; (en) CharacterSort's
   rows and CharacterSortBuff got one PORT row each (the ROM writes row / slot 35 when all 35 count).
   **Slice 4 (2026-11-10, verify): Round2ChangeChar -- fileio.c's code is complete.** The ROM's 150
   unrolled Round2ChangeSectInfo calls became three tables (Alyssa's 19 files, four costumes' 19, 25
   shared pairs) in ROM order. Open: (eo) the argument's meaning is read from the file names only:
   0 ROUND2_NEW_GAME (FlInit, stop), 1 ROUND2_CLEARED (FlInit, then the shared swaps), 2..5
   ROUND2_COSTUME_1..4 (CAL1..CAL4, no FlInit first: a costume picked after another keeps the earlier
   swaps of files the new one does not cover -- none, as all four tables cover the same 19 names), any
   other value only the shared swaps; hhXcmd_DressUp's TU says what its value is.
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
   **game/QuatSlerp.c slice 1 (partial, 2026-11-10; the queue was empty, refilled with it: asFindModelParts
   was the ActressMP blocker):** the part lookup (asFindModelParts / _ID with the parts header's four
   cache words), ahGetHash (a 64-bit XOR of 7-bit characters rotated 7 apart) and its inverse
   asConvertHash2String, asCheckBankID / _Type, asFindCharacterStruct, asVectorDistance (squared),
   ahAcos (the ROM's ah_acos_table carried as src/game/ah_acos_table.inc: a host acos differs in 866 of
   4098 entries), ahCos / ahSin, the camera read-back (asViewMatrix2PosRUF / CamRUF,
   CameraMatrix2CameraPos / Trg3m, asFocalLength_*), the 1.1.14 / 0.7.9 / UV packed blends,
   BankID_TO_CharID / CharID_TO_BankID / GetRoomName_file, RoomFlagClear / BRANCH_CLEAR / RoomInFlagOn
   (RoomNameTbl / RoomIdTbl from the ELF), OmniRangeChange, AmbientAlphaInit, ControllerClatteringInit.
   **The ActressMP crash (main.c:2106) is gone**: a 15 s smoke run ends without a fault (27,000+ frames);
   the shot stays black (the ga draw calls are stubs). Open: (ep) **(ec) settled**: the hashes read back
   -- 0x3161c34c2 is "Bip01" (sound.h's PARTS_HASH_SOUND_ORIGIN), 0x74dda77d0cbcf0c2 "BasePoint"
   (EA_001_PART_HASH / SP_ANCHOR_HASH); the other two cached names are longer than 9 characters (ends
   "...sa_ta", "...sa_se"): hash the shape files' part names to find them; any other u_long hash
   constant can be read back the same way (py: 7-bit groups from the low end); (eq) asUV2ZipUV_R stores the two
   floats themselves (8 bytes), so asInterpolateZipUV's output is a float pair where its inputs are
   packed shorts -- EA_010's out + 8 slot must be 8 bytes of floats (check when the vertex kinds draw);
   (er) CHARACTER +0x90 renamed roomHash (CharID_TO_BankID, RoomInFlagOn read Character[0]'s); MODEL_PARTS
   +0x120 is `alpha`; SCENE_ALPHA_ANIM (5 words) and CLATTERING (+0x10 id) are named from their inits
   only; REC_OMNI_* (record +0x30 / +0x34 / +0x38 / +0x2ec) from OmniRangeChange; (es) gaSinCos(s, c,
   angle) is a new stub in ga/gaSinCos.c (ahSin / ahCos read the first / second answer): every ah angle
   helper answers 0 until it is written; gaInversMatrix is still a stub, so the camera read-back answers
   zeros. Left in this TU (frontier): GetAreaNumber (next reached), sunSetupTBufferData /
   sunBGT_*_VIF1, PanicModeInit, AmbientAlphaUpdate, the warp objects, asMakeRotMatrixByQuartanion,
   QuatSlerp, ControllerClatteringUpdate and the brief's other 100.
   **game/QuatSlerp.c slice 2 (partial, 2026-11-10):** GetAreaNumber (RoomNameID tables from the ELF),
   the controller rattles (ControllerClatteringFree / Get / Update / Set, CLATTERING named), DemoSkipCheckEvent,
   PanicModeInit (PANIC_DEF Panicdef, PANIC_INFO +0x28 now float f28), QuatSlerp,
   asMakeRotMatrixByQuartanion, the warp family (WarpObject, WarpObjectUpdate / _Set / _Init,
   asWarpObjectBody; WARP_CMD), tnInit3dlibWork. A 15 s smoke run: no fault, 27,000 frames, shot black.
   Open: (et) the rattle's stick bytes are PAD_DATA +0x1c..0x1f read as [2], [3], [0], [1]: which stick is
   which is gaPadRead's TU (pad_button_read.c); CLATTERING +0x08 (default 0x70) is set but never read
   here; ControllerClatteringSet with no free slot writes slot 10 past the array in the ROM (over
   pad_key_button): the host answers 10 without writing; (eu) TRIG_EVENT_AREA_PASS2 / _PASS3 (0x5db /
   0x5da), AREA_STAGE3_HOME 52, AREA_STAGE4_0A16 47, TRIG_VAR_PANIC (0x1c) and TRIG_EVENT_PANIC_3 / _26
   / _48 are named from this use only; PANIC_DEF's fields by offset until PanicUpdate is read;
   (ev) WarpObject's followers (Character[11] / [12] with flags bit 31) suggest CHARACTER_FLAG_REFRACTION
   (0x80000000) is a wider "accompanies the player" bit: rename when the character TU reads it;
   WARP_CMD.bank2 / mode and the ExecuteWarpObject / 2 argument order come from asWarpObjectBody's calls
   (stubs: confirm when written); WarpObject's prototype (room, mode, chara, partHash, bank) is from its
   body -- ssTrigExecuteActCmd / tkXDTrg_ActorProc_AlsOpn call it. Left (frontier): ExecuteWarpObject /
   2, PanicFilter, XcmdScreenFilter / Skip, DemoScreenFilter (the screen filters), sunSetupTBufferData,
   sunBGT_LoadGSTexClutData_VIF1 / _ExecTBufferData_VIF1 / _SetLoadGSEnv (the texture upload: the next
   blocker for the picture with the ga draw stubs), and the brief's other 92.
   **game/QuatSlerp.c slice 3 (partial, 2026-11-10):** the texture buffers (sunSetupTBufferData,
   sunBGT_SetLoadGSEnv, sunBGT_LoadGSTexClutData_VIF1, sunBGT_ExecTBufferData_VIF1; SUN_TEX_ATTR 0x34 and
   SUN_TBUFFER_DATA 0xd8 in QuatSlerp.h, TEX_INFO's words named: tbp, cbp, clut, CLAMP's six, tfx), the
   panic sounds (EventSoundUpdate, PanicSound, LavenderSound, InvisibleSound), PanicCheck, ssMenuUpdate,
   PlayerControlUpdate. PANIC_INFO +0x24 is the float gauge, +0x28 gaugeMax; Panicdef +0x08 beatFrom.
   15 s smoke run: no fault, 27,360 frames, shot still black. Open: (ew) **the texture still never
   reaches the GPU**: the packets are built byte for byte and the host DMAC drops them; the picture needs
   a host texture module that decodes a SUN_TEX_ATTR (its mipData / clutData EE words, psm) at the
   source and binds it where TEX0.TBP points -- the user's no-GS-emulator rule; whoever writes the ga
   draw path (gaBGT_DrawObject) should key textures by the SUN_TBUFFER_DATA's tex0 word; (ex) the host
   resolves TEX_INFO.tim / clutData with a bank search (SunTexEePtr) plus one hard-wired game-data record
   (tex_attr_A$2750, the shadow buffer read as a texture, 0x01937c80): a new game-data texture record
   from another caller answers NULL and leaves the packet's body unset -- add it there; (ey)
   TEX_TFX_HIGHLIGHT2 replaced TEX_KIND_LIGHT (the same word is TEX0.TFX): main_rtn's light rule keys on
   TFX 3 -- check against a bank whose highlight parts are known; (ez) the panic events and SEs
   (TRIG_EVENT_LAVENDER 0x1a, _INVISIBLE 0x97, _PLAYER_STOP 0x90, PanicCheck's ON / OFF events,
   TRIG_VAR_PANIC_OFF 0x0b, SE 0x23 / 0x24 / 0x43 / 0x45 in slot 4), PANIC_MODE_FULL (mode bit 0) and
   tiVibrationOn's argument names are from this use only; act_small / act_large (0x0193f840 / 44) sit in
   lib/unassigned_data.cpp until the vibration TU (Clipping_Portal.c's tiVibration*) owns them; the
   heartbeat's second SdSetParamSE argument (SD_SE_PARAM_30, level / 2) is unk30 until the Sd TU names it.
   Left (frontier): PanicUpdate, PanicFilter, XcmdScreenFilter / Skip, DemoScreenFilter, MonsterAIUpdate,
   FogAnimeUpdate, ExecuteWarpObject / 2, ssGetBankID, ssStrCmp4Byte and the brief's other ~70.
   **game/QuatSlerp.c slice 4 (partial, 2026-11-10):** the screen filters (PanicFilter, DemoScreenFilter,
   XcmdScreenFilter / Skip: SCREEN_FILTER_OBJ, the 0xa0-byte TRIFAN packets, and their draw controls from
   the disassembly), EventSoundEnd, ssGetBankID / ssStrCmp4Byte, the slide decoder (tnSlideThawing,
   decode, tn_getc, tn_putc: Okumura LZSS), AmbientAlphaUpdate. FILTER_CHANGE_WORK's +0x08 is the
   **target** and +0x18 the **current** colour (renamed from from / to; main.c follows), A R G B; FILTER_ON
   / FILTER_OFF_AT_END / FILTER_MODE_ADD / _BLEND named. PANIC_INFO +0x2c is filterAlpha, Panicdef +0x10 /
   +0x14 / +0x18 filterMax / filterUp / filterDown. SCENE_ALPHA_ANIM is {on, steps, float target[3]}
   (no vitof on the target words: floats). 15 s smoke run: no fault, shot black; AmbientAlphaUpdate now
   reaches LosHitFadeUpdate / DistanceAttenuationUpdate / AngleAttenuationUpdate (stubs). Open: (fa) the
   filter packets carry the GS's fixed 512x448 fan; a host draw module that takes them must cover the
   DisplayGeometry frame (any aspect), not the packet's XYZ; (fb) PanicFilter's skip events 0x26 / 0 are
   TRIG_EVENT_PANIC_26 / event 0 from this use only; (fc) the filters' mode 1 draw is the ROM's TEST_1
   ATST 5 / AREF 0x7f with ATE 0 (no alpha test) -- kept. Next in this TU: AngleAttenuationUpdate (+
   AngleAttenuationTarget, DistanceAttenuationUpdate, LosHitFadeUpdate), PanicUpdate, MonsterAIUpdate,
   FogAnimeUpdate, TexFileSetInit, ExecuteWarpObject / 2.
   **game/QuatSlerp.c slice 5 (partial, 2026-11-10):** the part fades AmbientAlphaUpdate runs every frame:
   DistanceAttenuationUpdate / _Target / _AlphaSet (ALL_TABLE +0x80, DIST_ATTEN 0x28), AngleAttenuation*
   (+0x84, ANGLE_ATTEN 0x24), LosHitFadeUpdate / _Target (+0x74, LOS_FADE 0x28; CheckLOS / CheckLOS_Multi
   declared in ssHitCheckAttackToScn.h with LOS_SEG / LOS_MULTI from the call sites, now stubs there);
   DemoSkipRegisterEvent, asVectorLength. MODEL_PARTS +0x124 / +0x128 / +0x12c are losAlpha / distAlpha /
   angleAlpha; PARTS_FLAG_LOS_FADING 0x10 / _LOS_CLEAR 0x20. The eye / subject lists share ATTEN_EYE_* /
   ATTEN_SUBJ_* bits and ATTEN_CHARA (0x10: chara, hash). 15 s smoke run: no fault, shot black. Open:
   (fd) ATTEN_CHARA_LIVE (Character flags 0x80000000 and 0x40000000 both) is the fades' test of a
   character: 0x80000000 is CHARACTER_FLAG_REFRACTION from main.c's one use -- this second use says it is
   wider (a character in the scene?): rename it when Character_Init.c's TU reads the bit; (fe) Cam[6] +0xc0
   (CAMERA_EYE_POS) is the distance and LOS fades' camera position (the angle fade overwrites it with
   asViewMatrix2PosRUF's): name the CAMERA field with the camera TU; (ff) the ROM's eye-character branch
   with no list reads a part hash from low EE memory (hazards.md): the port skips those characters --
   check whether any bank data has flag 2 with list 0; (fg) LOS_MULTI.hits are host pointers to records
   whose short at 0x198 is an id: ssHitCheckAttackToScn.c's TU names the record; ~~(fh) asJointParts_byMatrix's
   fourth argument~~ settled 2026-11-10 (slice 6): (mat, pos, bank, hash); main2.c passes
   DrawOnPosSetHashTemp (the ROM's a3).
   **Slice 6 (2026-11-10):** PanicUpdate, MonsterAIUpdate, GAME_OVER_END, PlayerControlUpdate2,
   asJointParts, asWarpSceneObject, asGetCrossPointInPlaneWithLine, asRadianByO,
   asContInterpolateZipNormal_XYZ / _Trans_XYZ; PANIC_INFO named (recoveryItem, recoveryTime, rise[8]),
   Panicdef's calmFall / fullFall, PanicRecoveryTbl and HidePointUpValue (ROM data), ssAtkCharaList2
   (unassigned_data). 15 s smoke run: no fault, shot black. Open: (fi) PanicUpdate registers 0xca
   (TRIG_EVENT_MC_RELOADED, Character_Init.h's name from mc_load_set) when a full panic starts unless 0xcc
   is set: the name is probably narrower than the event -- rename it when ssGrpPortalCheck.c's TU names
   events; (fj) PANIC_RISE_* (threat 0..2, const 4..5, attack 6..7; slot 3 unread) are named from
   PanicUpdate alone: the TUs that write PanicInfoData.rise confirm them; likewise TRIG_EVENT_PANIC_* and
   TRIG_VAR_HIDE_POINT / _PANIC_SUSPEND / _PANIC_VIBRATE; (fk) asWarpSceneObject's VU0 MULA/MADD
   broadcasts are read as x, y, z, w (the decompiler hides the field) and its byte 0x43 bit
   (PARTS_FLAG43_WARPED) has no reader yet; PORT: it finds the bank whose ALL_TABLE it was given to
   resolve partsHeader (BANK_PTR); (fl) FUN_0185c26c (lower-cases a string in place, no callers) is left
   unnamed: it sits right after ssGetBankID, probably its inlined tail or a static. Slice 7 (2026-11-10):
   FogAnimeUpdate, VistaVisionMain, OneMaxObjectDrawZOnly, asJointParts_byMatrix,
   asMakeWatchMeMatrixToPosition, asDelSqueezeInformation, ExecuteWarpObject / 2 (now 5 arguments),
   DemoSkipGetEvent (SkipFlagTbl from the ROM), CheckSameRoomAsMonster, PanicDownSet; FOG_ANIME,
   CHAR_AMB_CHANGE, MODEL_PARTS.warpH54 named, CHARACTER.w098 is roomHashPrev. Open: (fm) the
   letterbox bars (VistaVisionMain) are full-width overlays: when the host draw module exists they must
   span the window at any aspect, top and bottom, not a 4:3 column; (fn) WATCH_AXIS_* and
   VISTA_* / TRIG_EVENT_VISTA_* are named from these functions alone; FogAnimeUpdate's gate reuses
   TRIG_VAR_PANIC_OFF (0xb), whose name is narrower than its use (it also stops the fog easing); (fo)
   BANK_TYPE_SHADOW (1) is also the room type the warp's ambient step tests: rename when fileio.c's TU
   names the types. Slice 8 (2026-11-10): PanicSet, PanicStopMovie (PanicOffMovieFile from the ROM),
   Kekkai_Check, ssPointCheckWithinScreen, PauseUpdate, UpperCase, GetBankID, PlayMovie,
   PlayMovieReturnLoad, SystemReReset, asGetObjectPositionAtWorld, asGetAlyssaPosition,
   asGetAnalogStickInformation, asSetObjectTrans, asGetObjectTransInitialValue (MODEL_PARTS.transInit
   at 0xf0 named). Open: (fp) ssPointCheckWithinScreen tests against screen_0's 512 x 448 PS2 frame:
   at a wide aspect it calls points beside the 4:3 column off screen; decide with the camera / renderer
   TU whether it reads DisplayGeometry (it gates game logic, so changing it changes behaviour); (fq)
   KekkaiOn (sbss 0x0193f9a4, no symbol) and ITEM_KEKKAI (2) / TRIG_EVENT_KEKKAI_USED (0x98) are named
   from Kekkai_Check alone; (fr) asGetObjectTransInitialValue answers asVectorLength of what it copied
   (the ROM tail-calls it): RotateCamera.c's callers say whether they read it; (fs) PARTS_FLAG43_TRANS
   (0x2) is named from asSetObjectTrans alone; (ft) AA_StartMovie (movie.c) is now a stub PlayMovie
   calls. Next in this TU: TexFileSetInit (reached: the scene's texture-file sets, FSLoad
   of DATA/IMG/ pages into double / ring buffers, then patching TEX_INFO words -- decide the host
   texture path first: decode at the source, no GS), tnTrigTexFileSetNext with TexFileSetNext,
   tnTrigTexFileSetPage with TexFileSetPage, GetAreaFloor, then the rest of the 24 left.
   Slice 9 (2026-11-10): VistaVisionStart / End (now * 2 + VISTA_SHOW_NOW / VISTA_HIDE_NOW: now = 1
   slides), PlayerDamage, asObjectRotate (MODEL_PARTS.placeRot at 0x2c0 named: rows 0..2 of the part
   placement matrix whose row 3 is drawOnPos), asPosRUF2ViewMatrix / asCamRUF2ViewMatrix,
   asRadianByVectors / asRadianByPositions_Rorate, asISubRotate16, asVectorLength1D / 2D,
   asVectorDistance2, LowerCase, absf, tnRand, rnd / rndf; the three FUN_ of the slice were Ghidra loop
   splits (LowerCase_body, asMemCopy128c_nw_body, asMemCopy128c_body, game-inferred). Open: (fu)
   placeRot and PARTS_FLAG43_TRANS together (asObjectRotate sets flags43 = 3, an assignment, not an OR):
   whoever reads flags43 names what 3 means; (fv) asFindMotionID (0x0185e060) is left: it reads a pointer
   at CHARACTER +0x350 (inside behavior[]) to 0x30-byte motion records keyed by a u_long hash, ended
   by hash 0 -- the behaviour-controller TU decides whether that word is a host pointer or an EE word;
   (fw) asObjectRotate with an axis other than 0..2 copies stack garbage in the ROM: the port uses the
   unit matrix (PORT note). Left in this TU: TexFileSetInit (reached), TexFileSetNext / Page and their
   tnTrig wrappers, GetAreaFloor, asFindMotionID, and what else status.md lists.
   Slice 10 (2026-11-10): the paged textures -- TexFileSetInit (reached from scene_init),
   TexFileSetNext, TexFileSetPage, tnTrigTexFileSetNext / Page. ALL_TABLE +0x8c is `texFileSet`, a
   TEX_FILE_SET_TABLE {sets, files, buffers} (QuatSlerp.h; the relocator's 0x8c block confirms every
   word): sets of 0x14 (a -1-ended part list, slide flag, page, file list, buffer set), file lists of
   {count, page names}, buffer sets of {count, TEX_PAGE_SLOT[0x18]: handle, mipData[4], clut}. Pages load
   from DATA/IMG/ into buffers inside the bank (BANK_PTR) and the slot's EE words are copied into the
   part's SUN_TEX_ATTR (mipData, first CLUT word), so the existing texture path sees the page. page_set
   (sbss 0x0193cb4c, static) tells whether the double buffer preloaded the next or the previous page.
   15 s smoke run: no fault, shot black (no host draw module). Open: (fx) the ROM keeps the ring's
   bound p < pages - 1 (the last page is never preloaded) and TexFileSetNext with no matching set goes
   on with the last set: both kept; (fy) tnTrigTexFileSetPage's argument roles (part, literal, a2, page
   & 0xffff as a trigger variable, scene) are read from its body: ssGrpPortalCheck.c's action table
   confirms them; (fz) the slot's mipData[0] is the FSLoad buffer, so a page file is raw level-0 pixels
   with its other levels and CLUT at fixed bank addresses: check against a DATA/IMG file once one is
   decoded at its source. Left in this TU: GetAreaFloor, asFindMotionID (fv).
   **game/QuatSlerp.c slice 11 (verify, 2026-11-10): the TU's code is complete (177 of 177).**
   GetAreaFloor (CHARACTER *; bank +2 >= 0; roomHash vs roomHashPrev; the room literals @3345..@3458 at
   0x01933758.. read from the ELF: 0A10 0A01 0A06 0B01 0B06 1C01 1C02 1C06 1C04 1B11 1B07 1B08 1B12 2B01
   2B04 4A04..4A12) sets TRIG_EVENT_AREA_PASS2 / PASS3 on a step between paired rooms, as GetAreaNumber
   reads them; asFindMotionID walks the motion table. 15 s smoke run: no fault, shot black. Open: (ga)
   asFindMotionID reads CHARACTER +CHARA_MOTION_TABLE through a static MotionTableOf that answers NULL
   (index -1), the same gap as ahRelocateSunPs2ObjectArry.c's CharaMotionTable (fv): the TU that writes
   asInitPlayerBehaviorControler gives the word its host form and both helpers become one; (gb)
   GetAreaFloor's 1C02 branch has no else (from a room other than 1C01 / 1C04 the flags stay), read from
   the decompilation and the callee order, not stepped; the PASS2 / PASS3 names stay as GetAreaNumber's.
   **Queue refilled 2026-11-10 (scheduled run, the queue was empty)** in the order the smoke run reaches
   the stubs: ga/gaInit3dlib.c (gaSet3dlibStruct, gaMulMatrix, gaInit3dlib, gaSetZRollMatrix,
   gaInversMatrix, gaSetLight all reached at boot or every frame), then ga/gaSinCos.c.
   **ga/gaInit3dlib.c (verify, 2026-11-10): all 25 functions** from the disassembly (architecture.md "The
   3D library"; hazards.md: gaMulMatrix is dst = b * a). A 15 s smoke run: no fault, shot black (no host
   draw module); gaSetZRollMatrix now reaches **gaSinCos (ga/gaSinCos.c, a stub: the next TU, queued)** --
   with gaSetRotMatrixX/Y/Z, gaSetPointLight, gaContApplyMatrix it is the rest of the ga math. Open:
   (gc) GA_3DLIB_WORK's and screen_0's names (GA_SCREEN_*: offsets, centre, scale, near / far, [11]) are
   read from gaSet3dlibStruct's arithmetic only; the side-plane order (left, right, top, bottom) follows
   the signs -- the clipper (Clipping_Portal.c / the VU1 program) confirms; (gd) GA_LIGHT_WORK +0x00..0x0f
   is still unread; (ge) gaContMulMatrix's records are host layout (three pointers; 0xc bytes in the ROM)
   because it has no ROM caller; gaCopyObject is kept as the ROM's no-op; the two unnamed blocks at
   0x018a77a8 / 0x018a7808 are dead and not written (named nowhere: no symbol, no Ghidra function);
   (gf) gaSetZRollMatrix returns early (PORT) when GA_SPR_WORK.work3dlibEE is not a scratchpad address --
   the ROM would read EE memory there; boot sets it to 0x70000400 before any frame.
   **ga/gaSinCos.c (verify, 2026-11-10): all 13 functions** from the disassembly and VU0 micro 0x020 /
   0x028 / 0x038 (architecture.md "The ga helpers"); ga_sincos_table embedded from the ELF. A 15 s smoke
   run: no fault, ~26,700 frames, shot black (no host draw module). Open: (gg) gaProjectScreen's per-point
   formula assumes the micro's DIV result is the one its MADDQ uses 7 instructions later (the intended
   pipeline); a count of 0 never ends in the ROM, the host returns; (gh) gaSetPointLight's omniView
   columns past the last light keep the view matrix's entries, as the ROM's registers leave them --
   harmless only if the VU1 light code reads k columns; (gi) the memmove calls ahRelocateSunPs2ObjectArry.c
   writes in place of gaMemCopy32 / 128 (callcheck comments) could now call them: left as they are.
   **Queue empty after this TU**: the next run refills it from the smoke run's first reached stubs.
   **game/movie.c (partial, 2026-11-10, slice 1): 16 of 20 functions** -- the front end (AA_OpenMovie,
   AA_PlayMovieReady, AA_MovieProc, AA_StartMovie, AA_MovieIOPInit), the bank choice
   (AA_AutoBank_Seelect, AA_MovieWorkCheck / Clean, AA_DeleteSceneBank), the memory map (AA_MemoryMapSet /
   Free), the small ones. The object is SCE's ezmpegstr main.c: Claude's call (2026-11-10) is that it owns
   the sample's globals (voBuf, AA_audioDec, AA_videoDec, AA_infile, AA_voBufTag / Data) and all of sbss
   0x0193f0d4..0x0193f704 (movie.c's symtab locals run through it; the globals before AA_FileHandole stay
   with AA_GetDataWord.c). **Next in this TU (slice 2): AA_movie, initAll, readMpeg, termAll** -- the
   decode loop: decide first how the host plays it (sdk/libmpeg + the movie seam, RosePan's moviestream;
   threads and the IPU DMA handler have no host meaning), then write them. Open: (gj) the caption file is
   read to EE 0x200000 with no size limit; the host reads it to AA_MojiImage (1 MB, PORT) --
   AA_GetDataWord.c's readers of AA_MojiData must use AA_MojiImage; (gk) AA_MovieWorkClean adds each
   dropped group's size - 1 to the ROW index (ROM bug, kept): a pre-chosen layout whose banks hold groups
   skips rows and leaves their *work unset; (gl) AudioDec / VideoDec / StrFile / VoBuf are opaque blocks of
   the symtab's sizes in their owners' new headers (AA_SoundMonoSoart.h, AA_mpegTS.h, AA_videoCallback.h):
   the owners name the fields; (gm) TRIG_EVENT_OMAKE_2 (0xc5) registered means AA_OpenMovie reads no
   captions (the name is Character_Init's); (gn) sceGsSetDefDBuff is a stub in lib/unassigned_stubs.cpp
   (the movie's GS reset: no host meaning until slice 2 picks the draw path); (go) callcheck lists FlProc /
   FileSoundServer / SunAllocUsedBorderCheck as missing: the port factors the ROM's inlined frame wait and
   heap-bank count into AA_WaitFrame / AA_GlobalHeapBanks.
   **game/movie.c slice 2 (2026-11-10): AA_movie, initAll, readMpeg, termAll -- all 20 functions written
   (verify: no smoke run plays a movie).** Claude's call (2026-11-10): the playback keeps the ROM's shape
   (the host EE kernel runs the decoder thread and the vblank handler; sdk/libmpeg is the decoder; the
   picture reaches the screen through AA_GetDataWord.c's display code, whose TU picks the host draw).
   The neighbours' prototypes are now declared from movie.c's calls (AA_videoCallback.h, AA_mpegTS.h,
   AA_SoundMonoSoart.h, AA_GetDataWord.h, new AA_cpuCycles.h and copy2area.h) and stubbed. Open: (gp) a
   run that reaches AA_movie hangs on those stubs (hazards.md): the movie TUs come before any smoke run
   that plays a movie; (gq) VideoDec.hid_endimage / hid_vblank, StrFile.size / openError and ReadBuf.size
   are named from movie.c's use, after ezmpegstr: their owners confirm; (gr) the host's AddDmacHandler
   registers nothing, so AA_handler_endimage never runs: AA_GetDataWord.c decides what replaces the GIF
   transfer end; (gs) initAll tells CreateThread 0x20000 bytes of stack where AA_MemoryMapSet allocated
   0x10800 (ROM quirk, harmless on the host, whose threads have their own stacks); (gt) KEY_MAP[20]
   (KEY_MAP_MOVIE_STOP) and tiFade_Start types 4 / 5 after the fade to black / white are named from this
   use; (gu) AA_isCountVblank (sbss 0x0193f0bc) is defined in lib/unassigned_data.cpp until
   AA_GetDataWord.c takes it; (gv) the smoke run left work/shots/last.png empty (0 bytes) while the
   numbered shots are fine: worth a look in build.py's shot writer.
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

- 2026-11-10 (Claude, scheduled run, SdMasterVolumeSetSE.c slice 2, for the user to overrule):
  **`SD_SE_LD` is `layout: host`** (its +0xa0 buffer is SdAlloc's host pointer); the IOP-bound records
  inside it (`spuCom` +0xc0, `SD_SE_REGISTER` +0x100) keep EE layout, and IOP addresses stay `u_int`
  (iop::SystemMemory places them below 4 GB). `SD_SQ_LD` stays `layout: ee` until SdLoadSQProc's session
  meets the same buffer word and decides likewise. sceSifAllocIopHeap / sceSifFreeIopHeap and
  sdremote's sceSdTransToIOP are now host code in sdk/sif.cpp (they were declared, never defined).

- 2026-11-10 (Claude, scheduled run, for the user to overrule): **the empty queue was refilled in the
  order a smoke run reaches stubs** (the run log's first stub per TU): ga/gaDmaSync_VIF1.c (taken),
  game/IOPGetFileSize.c, game/SdMasterVolumeSetSE.c, ga/gaPadInit.c, game/movie.c,
  game/wideXcmd_DebugPrint.c, game/tkXDTrg_GetSceneHash.c, game/asUseArrow.c, game/asMothRouteSearch.c,
  game/asXcmd.c, game/asEyeForcusUP2CamMat.c, game/ssHitCheckAttackToScn.c, game/ssSetTexUVforCubeMap.c,
  game/ssGrpPortalCheck.c. The shims of item 2 stay unqueued: no run reaches them yet.
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
- (2026-11-10, ga/gaPadInit.c) The TU map's cut between pad_button_read.c and gaPadInit.c looks wrong:
  gaPadInit's only statics, `pad_dma_buf0` (0x0193e580) / `pad_dma_buf1` (0x0193e480), sit in the same
  unbroken symtab local run as pad_button_read's statics (indices 1607..1618: id, rdata$185, term_id,
  pad_button_read, exid, shock_data, rterm_id, the buffers, act_align, phase, act_direct), with no section
  symbol until the GCC gaDma object's .text at 0x018a1188. So one CodeWarrior object most likely spans
  0x018a0880..0x018a1188. The ga/pad_button_read.c session should take gaPadInit and its buffers into its
  file (define 018a0880-018a1188) and retire ga/gaPadInit.c; left apart for now (one TU per run).

## Done

- 2026-11-09: AlyssaPan created: the repo, YamiPan's Pan layer (C++, HEAD 17e0108 plus 93c73f0's SPU2
  voices and sound card) trimmed to the generic, SDK, CMake, tools, the skill and its references (this
  file, architecture.md, cpp-port.md, bootstrap.md, reconstruction.md, ghidra.md, hazards.md with the
  siblings' digest). Build green, first smoke run and first `refresh.py` done. Nothing committed: the
  user decides.
