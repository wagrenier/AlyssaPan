# AlyssaPan status (generated)

Generated 2026-11-11 21:26 at commit `d36241c` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 0; tagged game-symbol in Ghidra: 2621
- library matches planned: 0; tagged lib-match / -low in Ghidra: 0 / 0
- classes (named vtables): 0

## Naming (game code; library ranges excluded)

- game functions: **1814**, named **1811** (100%); library functions: 814
- by provenance: game-symbol 2621, lib-match 0, lib-match-low 0, lib-context 0, game-inferred 4
- context-name ledger: 3 entries; overrides of wrong library matches: 0
- placed in a source file: 1814/1814 in 64 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `018d0000` | 180/182 (99%) | game/ssGrpPortalCheck.c, game/wideXcmd_TriggerInit.c, game/hhXcmd_DressUp.c |
| `018b0000` | 146/147 (99%) | game/Character_Init.c, game/movie.c, game/AA_SoundMonoSoart.c |
| `01830000` | 39/39 (100%) | game/main.c, game/ahalloc.c, game/ahRelocateSunPs2ObjectArry.c |
| `01840000` | 84/84 (100%) | game/ahRelocateSunPs2ObjectArry.c |
| `01850000` | 200/200 (100%) | game/QuatSlerp.c, game/tiAvoidObject.c, game/ahRelocateSunPs2ObjectArry.c |
| `01860000` | 113/113 (100%) | game/tiAvoidObject.c, game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/asUseArrow.c |
| `01870000` | 82/82 (100%) | game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/tiBrain_ChgTrace_Ghost.c, game/ssBrain_ChgHideAtk1.c |
| `01880000` | 103/103 (100%) | game/Clipping_Portal.c, game/asEyeForcusUP2CamMat.c, game/ssBrain_ChgHideAtk1.c |

## Reconstruction (src/)

- reconstructed CT3 functions: **1235**; stubs: **233**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 9 | 0 |
| `src/ga/gaDma.c` | 16 | 0 |
| `src/ga/gaDmaSync_VIF1.c` | 6 | 0 |
| `src/ga/gaDrawObject.c` | 0 | 11 |
| `src/ga/gaDrawObject_Redraw_Polygon.c` | 0 | 1 |
| `src/ga/gaInit3dlib.c` | 25 | 0 |
| `src/ga/gaInitDrawObject.c` | 24 | 0 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaInitMeter.c` | 6 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 0 | 1 |
| `src/ga/gaSearchNearLight4.c` | 0 | 1 |
| `src/ga/gaSinCos.c` | 14 | 0 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 4 | 0 |
| `src/ga/wait.c` | 0 | 2 |
| `src/game/AA_GetDataWord.c` | 0 | 8 |
| `src/game/AA_SoundMonoSoart.c` | 0 | 8 |
| `src/game/AA_cpuCycles.c` | 0 | 1 |
| `src/game/AA_mpegTS.c` | 0 | 12 |
| `src/game/AA_videoCallback.c` | 0 | 11 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 15 | 0 |
| `src/game/Character_Init.c` | 48 | 1 |
| `src/game/Clipping_Portal.c` | 73 | 0 |
| `src/game/Convert.c` | 0 | 2 |
| `src/game/CpDivideEECom.c` | 0 | 10 |
| `src/game/IOPGetFileSize.c` | 4 | 0 |
| `src/game/LinkMatrixTree.c` | 4 | 0 |
| `src/game/PicPaz.c` | 9 | 0 |
| `src/game/QuatSlerp.c` | 189 | 0 |
| `src/game/RotateCamera.c` | 0 | 2 |
| `src/game/SdMasterVolumeSetSE.c` | 61 | 0 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 93 | 0 |
| `src/game/ahalloc.c` | 7 | 0 |
| `src/game/asEyeForcusUP2CamMat.c` | 30 | 0 |
| `src/game/asMothOperate.c` | 0 | 1 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 7 |
| `src/game/asXcmd.c` | 0 | 20 |
| `src/game/copy2area.c` | 0 | 1 |
| `src/game/fileio.c` | 40 | 0 |
| `src/game/hhXcmd_BGM_SE_VOLUME.c` | 0 | 1 |
| `src/game/hhXcmd_DressUp.c` | 26 | 0 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 18 | 0 |
| `src/game/movie.c` | 20 | 0 |
| `src/game/sound.c` | 73 | 0 |
| `src/game/ssCalcInversMatrix2.c` | 0 | 1 |
| `src/game/ssGrpPortalCheck.c` | 74 | 0 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 8 |
| `src/game/ssSetTexUVforCubeMap.c` | 1 | 50 |
| `src/game/tiAvoidObject.c` | 0 | 8 |
| `src/game/tiXcmd_TransItem.c` | 85 | 8 |
| `src/game/tkXDTrg_GetSceneHash.c` | 50 | 0 |
| `src/game/tnXcmd.c` | 0 | 8 |
| `src/game/wideXcmd_DebugPrint.c` | 109 | 0 |
| `src/game/wideXcmd_TriggerInit.c` | 1 | 43 |
| `src/game/xcmd_tn.c` | 59 | 0 |
| `src/lib/unassigned_stubs.cpp` | 0 | 1 |

## Translation units (one session each: `tus.py`)

- 64 TUs: todo 29, partial 3, verify 33, done 8, blocked 1
- next: **game/asUseArrow.c** (pinned in the queue)
- pinned queue: alyssapan/rendering/ui_canvas.cpp, game/asUseArrow.c, game/asMothRouteSearch.c, game/asXcmd.c, game/ssHitCheckAttackToScn.c, game/ssSetTexUVforCubeMap.c
- partial: `game/tiXcmd_TransItem.c` 85/93 -- Slice 10. Written: tiXcmd_UseItem (ITEM_USE_* modes from argv[4], ITEM_KIND_* from the row byte +0: give-take, signal, heal Character[0].hp up to hpMax, calm vi
- partial: `game/wideXcmd_TriggerInit.c` 1/46 -- Title draws at 4:3 and 16:9 after 40 s (logo, 3, menu, copyright). Fix in SunTexEePtr, BankOfEeAddr, HitObjectId: free slot banks shared EE addresses with the G
- verify: `alyssapan/core/movie_skip.cpp` 0/0 -- AlyssaPan_SkipMovies seam (src/alyssapan/core/movie_skip.h/.cpp) reads the config's skip_intro (ALYSSAPAN_SKIP_INTRO; off for players); AA_movie takes the PORT 
- verify: `ga/gaBGT_DrawObject.c` 9/9 -- Game code complete. Host draw-module work left (open-items item 4 slice 4): texture, depth, fog, frame end, CT3 window origin in UiCanvas, light1 lighting, bill
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaDmaSync_VIF1.c` 5/5 -- Host: kicks leave CHCR.STR clear (the transfer is over), so the syncs return at once; the BGT send keeps the copy and the flip, the packet goes nowhere (draw mo
- verify: `ga/gaInit3dlib.c` 25/25 -- 15 s smoke run: no fault, shot black (no host draw module). gaMulMatrix is dst = b * a (hazards.md). gaSetZRollMatrix now reaches gaSinCos: ga/gaSinCos.c is que
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaSinCos.c` 13/13 -- 15 s smoke run: no fault, about 26700 frames, shot black (no host draw module yet). ga_sincos_table.inc generated from the ELF (the ROM floats differ from sinf)
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `ga/pad_button_read.c` 4/4 -- Complete; callcheck clean. 15 s smoke: flat grey as before (attract cycle). 40 s run with scripted START at reads 500..1100: no TITL in the log, but the shot is
- verify: `game/AlignedNum.c` 4/4 -- WaitLoop is split in Ghidra: WaitLoop 0x0189f650 plus WaitLoop_body 0x0189f680 (game-inferred), one C function. The j at 0x0189f718 belongs to the next object (
- verify: `game/ArtoolDebugOpen.c` 15/15 -- Smoke run: FlInit prints no error, so the pack directory loads and sceCdSearchFile finds the pack; no run reads a file yet (main2 and fileio.c are stubs), the s
- verify: `game/Character_Init.c` 48/49 -- All 49 functions written; nothing on screen reaches the card yet (main2 is a stub), shot black as before. The save image layout is in architecture.md (The save 
- verify: `game/Clipping_Portal.c` 73/73 -- Smoke run unchanged (content warning screen at 4:3); no pendulum is on its path, so verify. callcheck lists the two entry points calls under the static HurikoSw
- verify: `game/IOPGetFileSize.c` 4/4 -- All 4 functions written. No host server answers FSYS_IOP SUNA yet (sif.cpp answers zeros), so a loose file's size reads 0: a host FSYS_IOP server on io::Disc is
- verify: `game/LinkMatrixTree.c` 4/4 -- Code complete, nothing on screen shows it yet: the smoke run still crashes at main.c 2106 (ActressMP NULL, the known blocker) and the shot stays black. LinkMatr
- verify: `game/PicPaz.c` 9/10 -- The smoke run passes aaXcmd_MovieFedeOutCancel: both title movies start and are skipped; the shot stays flat grey since TITL draws nothing yet (title_trace.cpp 
- verify: `game/QuatSlerp.c` 177/177 -- 15 s smoke run: no fault, shot black (no host draw module yet). callcheck: GetAreaFloor's memcmp and ssTrigRemoveEvent come through ROOM_IS and AreaFloorClear (
- verify: `game/SdMasterVolumeSetSE.c` 61/61 -- All 61 functions written. callcheck MISSING on SdLoadSQProc is its step helpers (SdLoadSQOpen/Read/Send/Command), as for SdLoadSEProc. ROM slips kept: step 6 pr
- verify: `game/ahRelocateSunPs2ObjectArry.c` 93/93 -- All 93 functions written; nothing reaches the general body yet (the two written callers pass one flag each; asCharacterAnimationExecute and asResetCharacterPosi
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/asEyeForcusUP2CamMat.c` 30/30 -- The smoke run calls camera_main with CAMERA_MODE_SCENE each frame; the debug modes and the cut step are not reached. Shot still black: the first object lands at
- verify: `game/fileio.c` 40/40 -- All 40 functions written. Round2ChangeChar keeps the ROM order of its 150 Round2ChangeSectInfo pairs as three tables; round values named from the file names onl
- verify: `game/hhXcmd_DressUp.c` 22/22 -- A smoke run now passes hhXcmd_LoadSE and stops in the m_capcom.mov decode loop (shots end at frame 420, flat grey): the movie skip host task is the next blocker
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/main2.c` 18/18 -- Smoke run leaves main2 and runs the frame loop with no crash; the shot stays black because FSMltLoad (fileio.c) is a stub so no bank loads. PORT BankReady skips
- verify: `game/movie.c` 20/21 -- Playback kept in the ROM shape: host EE kernel runs the decoder thread and vblank handler, sdk/libmpeg decodes, the picture goes through AA_GetDataWord.c displa
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo
- verify: `game/ssGrpPortalCheck.c` 59/59 -- Code complete. The black screen was the ga objects' +0x38 word (EE address of the part's partsPos, the ambient light0 adds) left 0 by the ssSetAmbColorToAllObj 
- verify: `game/tkXDTrg_GetSceneHash.c` 50/50 -- Callcheck MISSING asFindModelPartsID in Ini is the DoorFindPart helper. The part names settle h48 h4c and the kind entries' +4 as SE / TDS parts (open-items.md 
- verify: `game/wideXcmd_DebugPrint.c` 109/109 -- All 109 functions written. Nothing on screen reaches it yet (the shot is the warning screen): verify in gameplay. New stubs ssHitAddAttackRequest, ssHit_GetMaxI
- verify: `game/xcmd_tn.c` 57/57 -- All 57 functions written. VistaVision: ON or on brings the bars in, argv[1] 1 slides, a fourth argument makes ON skippable; statics VistaVisionEndCheck, VistaVi
- blocked: `alyssapan/rendering/ui_canvas.cpp` 0/0 -- Blocked on ga/gaBGT_DrawObject.c (host draw module for ga draw objects) and the camera (asEyeForcusUP2CamMat.c camera_init0 / camera_view stubs); triggers are n

Recent sessions:

- 2026-11-11 21:26 `game/tkXDTrg_GetSceneHash.c` verify: slice 4: tkXDTrg_ActorProc_Ini, the old-form door record that finds its camera cut and parts by name (Cam_Dor, TDS_Dor, CP_Dor, HT_Dor, SE_Dor plus the room and
- 2026-11-11 21:20 `game/tkXDTrg_GetSceneHash.c` partial: slice 3: the DoorTrigger x command and its actor dispatcher (code parser, scene heads, door records), the Ini_New and IniMon parsers, Alyssa's door open and clo
- 2026-11-11 21:08 `game/tkXDTrg_GetSceneHash.c` partial: slice 2: the door actor procs (AnimToFrameDoor, AnimRangeDoor, ChangeCamera, CmnClo, the ScW / ScM / Axe / Vit / Ham Opn, OpnOut and Clo, Sht, HitPtl, AlsCam, I
- 2026-11-11 20:56 `game/tkXDTrg_GetSceneHash.c` partial: slice 1: the branch scene tables (ScnTbl and the Br tables), scene ids, door flags, the door-trigger state init, CloseCancel, the door bridge draw (tkBri_DoorDr
- 2026-11-11 20:33 `game/wideXcmd_DebugPrint.c` verify: slice 10: asCharacterMortionFunctionOperation (attack and no-hit windows, per-character run events, Alyssa's stairs and panic triggers, turn speed, lever mash e
- 2026-11-11 20:11 `game/Clipping_Portal.c` verify: slice 15: tiCalc_HurikoOPEN and tiCalc_HurikoCLOSED, one pendulum solver (pull to rest pose, normal drag, springs, accel clamp, damping, length constraint, caps
- 2026-11-11 19:54 `game/Clipping_Portal.c` partial: slice 14: tiCalc_Leaf, tiCalc_Twist (turn about the velocity around startPos less radius along unit (vel+1) x vel), tiGetTransRotMatrix, tiGetDegFromVec, tiPart
- 2026-11-11 19:45 `game/Clipping_Portal.c` partial: slice 13: the translucent-character and ghost passes (tiChara_Translucence_Start / End / Start2 / End2, host composite from a capture at the inverse FIX), tiXFa

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/ssSetTexUVforCubeMap.c` | partial | 1/50/96 | 35.3 | 117 | 10 |
| `game/CpDivideEECom.c` | todo | 0/10/18 | 3.3 | 80 | 10 |
| `ga/gaDrawObject.c` | todo | 0/11/11 | 6.9 | 39 | 0 |
| `game/ssHitCheckAttackToScn.c` | todo | 0/8/37 | 32.0 | 28 | 3 |
| `game/tiAvoidObject.c` | todo | 0/8/82 | 30.5 | 28 | 4 |
| `game/Convert.c` | todo | 0/2/2 | 0.8 | 23 | 0 |
| `game/AA_GetDataWord.c` | todo | 0/8/15 | 4.9 | 21 | 2 |
| `game/AA_SoundMonoSoart.c` | todo | 0/8/20 | 4.0 | 18 | 2 |
| `game/AA_mpegTS.c` | todo | 0/12/31 | 2.6 | 17 | 0 |
| `game/asUseArrow.c` | todo | 0/7/11 | 6.4 | 17 | 1 |
| `game/AA_videoCallback.c` | todo | 0/11/11 | 1.7 | 17 | 1 |
| `ga/gaObjectClippingCheck.c` | todo | 0/1/1 | 0.2 | 12 | 1 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `01851b10` | ConvertQuat2Matrix | 17 | sym | stub | game/Convert.c |
| `018d1e70` | CpGetEEIOPStatus | 14 | sym | stub | game/CpDivideEECom.c |
| `01883ca0` | asMothPause | 10 | sym | stub | game/asMothPause.c |
| `018a3078` | gaObjectClippingCheck | 7 | sym | stub | ga/gaObjectClippingCheck.c |
| `018518d0` | ConvertMatrix2Quat | 6 | sym | stub | game/Convert.c |
| `018d1fa0` | CpSifDmaTransEEToIOP | 6 | sym | stub | game/CpDivideEECom.c |
| `018a4fe0` | gaDrawObject_VertexLocalViewWithClippingFlagSet | 6 | sym | stub | ga/gaDrawObject.c |
| `018a51e8` | gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard | 6 | sym | stub | ga/gaDrawObject.c |
| `018a5fe0` | gaDrawObject_MakeVertexReferenceTable | 6 | sym | stub | ga/gaDrawObject.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01866c10` | asShootArrowSetting | 5 | sym | stub | game/asUseArrow.c |
| `01899070` | ssParticleCashClear | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018ab180` | CheckLOS | 4 | sym | stub | game/ssHitCheckAttackToScn.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018a7538` | gaSearchNearLight4 | 4 | sym | stub | ga/gaSearchNearLight4.c |
| `018bd830` | AA_clearGsMem | 4 | sym | stub | game/AA_GetDataWord.c |
| `018a8690` | gaDMAStopMessage_VIF1 | 4 | sym | stub | ga/wait.c |
| `018962a0` | tnChangeCharAmbientColorSet | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018c1ce0` | AA_videoDecAbort | 3 | sym | stub | game/AA_mpegTS.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018a53c8` | gaDrawObject_RemakeIndexWithClippingCheck | 3 | sym | stub | ga/gaDrawObject.c |
| `018a5730` | gaDrawObject_RemakeIndexWithClippingCheck_WithNear | 3 | sym | stub | ga/gaDrawObject.c |
| `018a5100` | gaDrawObject_VertexLocalView | 3 | sym | stub | ga/gaDrawObject.c |
| `018a52f0` | gaDrawObject_VertexLocalView_Billboard | 3 | sym | stub | ga/gaDrawObject.c |
| `018a5b88` | gaDrawObject_RemakeIndex | 3 | sym | stub | ga/gaDrawObject.c |
| `018a6038` | gaDrawObject_MakeVertexReferenceTableNoClip | 3 | sym | stub | ga/gaDrawObject.c |
| `018aa950` | CheckLOS_Multi | 2 | sym | stub | game/ssHitCheckAttackToScn.c |
| `0189da90` | ssDOF_ClipObjectFromZRange | 2 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018c1d80` | AA_videoDecSetStream | 2 | sym | stub | game/AA_mpegTS.c |
| `018bff20` | AA_strFileOpen | 2 | sym | stub | game/AA_videoCallback.c |
| `018d2120` | CpCall_IOPSndDriver_2 | 2 | sym | stub | game/CpDivideEECom.c |
| `018d1e80` | CpCheckEE_IOPComEnd | 2 | sym | stub | game/CpDivideEECom.c |
| `018c0310` | AA_cpuCycles | 2 | sym | stub | game/AA_cpuCycles.c |
| `018c1cd0` | AA_videoDecGetState | 2 | sym | stub | game/AA_mpegTS.c |
| `018c1ba0` | AA_videoDecFlush | 2 | sym | stub | game/AA_mpegTS.c |
| `018a6080` | gaDrawObject_SetBig | 2 | sym | stub | ga/gaDrawObject.c |
| `0189f720` | gaDrawObject_Redraw_Polygon | 2 | sym | stub | ga/gaDrawObject_Redraw_Polygon.c |
| `0189dec0` | ssBGT_ZBufferClear | 2 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `0189d900` | ssTrigScreenBlurEnd | 2 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018aa8e0` | ssHitAddAttackRequest | 2 | sym | stub | game/ssHitCheckAttackToScn.c |

