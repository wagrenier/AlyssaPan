# AlyssaPan status (generated)

Generated 2026-11-11 14:19 at commit `555835b` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 0; tagged game-symbol in Ghidra: 2621
- library matches planned: 0; tagged lib-match / -low in Ghidra: 0 / 0
- classes (named vtables): 0

## Naming (game code; library ranges excluded)

- game functions: **1814**, named **1811** (100%); library functions: 814
- by provenance: game-symbol 2621, lib-match 0, lib-match-low 0, lib-context 0, game-inferred 4
- context-name ledger: 3 entries; overrides of wrong library matches: 0
- placed in a source file: 1814/1814 in 65 files

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

- reconstructed CT3 functions: **1082**; stubs: **294**

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
| `src/ga/gaPadInit.c` | 1 | 0 |
| `src/ga/gaSearchNearLight4.c` | 0 | 1 |
| `src/ga/gaSinCos.c` | 14 | 0 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 0 | 2 |
| `src/ga/wait.c` | 0 | 2 |
| `src/game/AA_GetDataWord.c` | 0 | 8 |
| `src/game/AA_SoundMonoSoart.c` | 0 | 8 |
| `src/game/AA_cpuCycles.c` | 0 | 1 |
| `src/game/AA_mpegTS.c` | 0 | 12 |
| `src/game/AA_videoCallback.c` | 0 | 11 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 15 | 0 |
| `src/game/Character_Init.c` | 48 | 1 |
| `src/game/Clipping_Portal.c` | 11 | 30 |
| `src/game/Convert.c` | 0 | 2 |
| `src/game/CpDivideEECom.c` | 0 | 10 |
| `src/game/IOPGetFileSize.c` | 4 | 0 |
| `src/game/LinkMatrixTree.c` | 4 | 0 |
| `src/game/PicPaz.c` | 0 | 7 |
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
| `src/game/hhXcmd_DressUp.c` | 0 | 21 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 18 | 0 |
| `src/game/movie.c` | 20 | 0 |
| `src/game/sound.c` | 73 | 0 |
| `src/game/ssCalcInversMatrix2.c` | 0 | 1 |
| `src/game/ssGrpPortalCheck.c` | 74 | 0 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 6 |
| `src/game/ssSetTexUVforCubeMap.c` | 1 | 44 |
| `src/game/tiAvoidObject.c` | 0 | 7 |
| `src/game/tiXcmd_TransItem.c` | 85 | 8 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 8 |
| `src/game/tnXcmd.c` | 0 | 8 |
| `src/game/wideXcmd_DebugPrint.c` | 107 | 1 |
| `src/game/wideXcmd_TriggerInit.c` | 0 | 44 |
| `src/game/xcmd_tn.c` | 59 | 0 |
| `src/lib/unassigned_stubs.cpp` | 0 | 1 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 35, partial 4, verify 27, done 5, blocked 1
- next: **alyssapan/rendering/ga_fill.cpp** (pinned in the queue)
- pinned queue: alyssapan/rendering/ga_fill.cpp, game/Clipping_Portal.c, alyssapan/rendering/ui_canvas.cpp, game/wideXcmd_DebugPrint.c, game/tkXDTrg_GetSceneHash.c, game/asUseArrow.c, game/asMothRouteSearch.c, game/asXcmd.c, game/ssHitCheckAttackToScn.c, game/ssSetTexUVforCubeMap.c
- partial: `game/Clipping_Portal.c` 11/73 -- Fade after ATLD is type 4 (OUT2_BLACK) reaching f 1; tiFade_Draw sends the ROM's GIF sprite packet, which the host drops, so the shot stays flat grey: host task
- partial: `game/tiXcmd_TransItem.c` 85/93 -- Slice 10. Written: tiXcmd_UseItem (ITEM_USE_* modes from argv[4], ITEM_KIND_* from the row byte +0: give-take, signal, heal Character[0].hp up to hpMax, calm vi
- partial: `game/wideXcmd_DebugPrint.c` 107/109 -- Next slice: asCharacterMortionFunctionOperation (0x1578, VU0 and the per-character trigger chains: split it if needed) -- the last function of the TU. New stubs
- verify: `ga/gaBGT_DrawObject.c` 9/9 -- Game code complete. Host draw-module work left (open-items item 4 slice 4): texture, depth, fog, frame end, CT3 window origin in UiCanvas, light1 lighting, bill
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaDmaSync_VIF1.c` 5/5 -- Host: kicks leave CHCR.STR clear (the transfer is over), so the syncs return at once; the BGT send keeps the copy and the flip, the packet goes nowhere (draw mo
- verify: `ga/gaInit3dlib.c` 25/25 -- 15 s smoke run: no fault, shot black (no host draw module). gaMulMatrix is dst = b * a (hazards.md). gaSetZRollMatrix now reaches gaSinCos: ga/gaSinCos.c is que
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaPadInit.c` 1/1 -- Complete; nothing on screen can show pad start-up (shot black as before). The symtab puts pad_dma_buf0/1 in pad_button_read.c's local run (indices 1607..1618): 
- verify: `ga/gaSinCos.c` 13/13 -- 15 s smoke run: no fault, about 26700 frames, shot black (no host draw module yet). ga_sincos_table.inc generated from the ELF (the ROM floats differ from sinf)
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `game/AlignedNum.c` 4/4 -- WaitLoop is split in Ghidra: WaitLoop 0x0189f650 plus WaitLoop_body 0x0189f680 (game-inferred), one C function. The j at 0x0189f718 belongs to the next object (
- verify: `game/ArtoolDebugOpen.c` 15/15 -- Smoke run: FlInit prints no error, so the pack directory loads and sceCdSearchFile finds the pack; no run reads a file yet (main2 and fileio.c are stubs), the s
- verify: `game/Character_Init.c` 48/49 -- All 49 functions written; nothing on screen reaches the card yet (main2 is a stub), shot black as before. The save image layout is in architecture.md (The save 
- verify: `game/IOPGetFileSize.c` 4/4 -- All 4 functions written. No host server answers FSYS_IOP SUNA yet (sif.cpp answers zeros), so a loose file's size reads 0: a host FSYS_IOP server on io::Disc is
- verify: `game/LinkMatrixTree.c` 4/4 -- Code complete, nothing on screen shows it yet: the smoke run still crashes at main.c 2106 (ActressMP NULL, the known blocker) and the shot stays black. LinkMatr
- verify: `game/QuatSlerp.c` 177/177 -- 15 s smoke run: no fault, shot black (no host draw module yet). callcheck: GetAreaFloor's memcmp and ssTrigRemoveEvent come through ROOM_IS and AreaFloorClear (
- verify: `game/SdMasterVolumeSetSE.c` 61/61 -- All 61 functions written. callcheck MISSING on SdLoadSQProc is its step helpers (SdLoadSQOpen/Read/Send/Command), as for SdLoadSEProc. ROM slips kept: step 6 pr
- verify: `game/ahRelocateSunPs2ObjectArry.c` 93/93 -- All 93 functions written; nothing reaches the general body yet (the two written callers pass one flag each; asCharacterAnimationExecute and asResetCharacterPosi
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/asEyeForcusUP2CamMat.c` 30/30 -- The smoke run calls camera_main with CAMERA_MODE_SCENE each frame; the debug modes and the cut step are not reached. Shot still black: the first object lands at
- verify: `game/fileio.c` 40/40 -- All 40 functions written. Round2ChangeChar keeps the ROM order of its 150 Round2ChangeSectInfo pairs as three tables; round values named from the file names onl
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/main2.c` 18/18 -- Smoke run leaves main2 and runs the frame loop with no crash; the shot stays black because FSMltLoad (fileio.c) is a stub so no bank loads. PORT BankReady skips
- verify: `game/movie.c` 20/21 -- Playback kept in the ROM shape: host EE kernel runs the decoder thread and vblank handler, sdk/libmpeg decodes, the picture goes through AA_GetDataWord.c displa
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo
- verify: `game/ssGrpPortalCheck.c` 59/59 -- Code complete. The black screen was the ga objects' +0x38 word (EE address of the part's partsPos, the ambient light0 adds) left 0 by the ssSetAmbColorToAllObj 
- verify: `game/xcmd_tn.c` 57/57 -- All 57 functions written. VistaVision: ON or on brings the bars in, argv[1] 1 slides, a fourth argument makes ON skippable; statics VistaVisionEndCheck, VistaVi
- blocked: `alyssapan/rendering/ui_canvas.cpp` 0/0 -- Blocked on ga/gaBGT_DrawObject.c (host draw module for ga draw objects) and the camera (asEyeForcusUP2CamMat.c camera_init0 / camera_view stubs); triggers are n

Recent sessions:

- 2026-11-11 14:19 `game/Clipping_Portal.c` partial: slice 1: vibration (tiVibrationOn / Off / Timer), wind (tiWindInit / Update / End / End_All), tiLightBelt2Init, tiFade_Start, tiFade_Draw, tiFactorial; WIND, LI
- 2026-11-11 13:54 `alyssapan/rendering/ga_atld.cpp` done: ATLD draws as the game picks it: the memory-card text textured, centred, same size at 4:3 and 1280x540; the slot 2 wording is page 1 of a 10-page slide texture,
- 2026-11-11 13:35 `game/xcmd_tn.c` verify: slice 5: the letterbox command (tnXcmd_VistaVision, its three statics and steps) and the key locks (tnXcmd_Key_Lock as per-stage rule tables); the TU code is co
- 2026-11-11 13:11 `game/xcmd_tn.c` partial: slice 4: 7 x commands: the play-time clock digits, the clock puzzle turn, scene animation to a frame and over a range, variable arithmetic, a character ambient 
- 2026-11-11 12:55 `game/xcmd_tn.c` partial: slice 3: 11 x commands: the clear's points, control lock, screen adjust, omni lights and range, screen filter, movie buffers, puzzle start, current scene, textu
- 2026-11-11 12:30 `game/tiXcmd_TransItem.c` partial: the item use command (UseItem)
- 2026-11-11 12:08 `game/tiXcmd_TransItem.c` partial: reconstruct 2 functions: the script delay command and the boss second phase (Balows2th)
- 2026-11-11 11:47 `game/tiXcmd_TransItem.c` partial: reconstruct 6 functions: part scale and rotation, scene animation skip to a variable, scope letters, caption flag, character room change

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/Clipping_Portal.c` | partial | 11/30/73 | 44.8 | 124 | 14 |
| `game/ssSetTexUVforCubeMap.c` | partial | 1/44/96 | 35.3 | 107 | 10 |
| `game/CpDivideEECom.c` | todo | 0/10/18 | 3.3 | 80 | 10 |
| `ga/gaDrawObject.c` | todo | 0/11/11 | 6.9 | 39 | 0 |
| `game/tiAvoidObject.c` | todo | 0/7/82 | 30.5 | 27 | 4 |
| `game/ssHitCheckAttackToScn.c` | todo | 0/6/37 | 32.0 | 23 | 3 |
| `game/Convert.c` | todo | 0/2/2 | 0.8 | 23 | 0 |
| `game/AA_mpegTS.c` | todo | 0/12/31 | 2.6 | 17 | 0 |
| `game/AA_videoCallback.c` | todo | 0/11/11 | 1.7 | 17 | 1 |
| `game/asUseArrow.c` | todo | 0/7/11 | 6.4 | 17 | 1 |
| `ga/pad_button_read.c` | todo | 0/2/3 | 2.1 | 16 | 2 |
| `game/tkXDTrg_GetSceneHash.c` | todo | 0/8/50 | 21.0 | 15 | 2 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `01851b10` | ConvertQuat2Matrix | 17 | sym | stub | game/Convert.c |
| `018d1e70` | CpGetEEIOPStatus | 14 | sym | stub | game/CpDivideEECom.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `01883ca0` | asMothPause | 7 | sym | stub | game/asMothPause.c |
| `018a3078` | gaObjectClippingCheck | 7 | sym | stub | ga/gaObjectClippingCheck.c |
| `018a4fe0` | gaDrawObject_VertexLocalViewWithClippingFlagSet | 6 | sym | stub | ga/gaDrawObject.c |
| `018a51e8` | gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard | 6 | sym | stub | ga/gaDrawObject.c |
| `018a5fe0` | gaDrawObject_MakeVertexReferenceTable | 6 | sym | stub | ga/gaDrawObject.c |
| `018518d0` | ConvertMatrix2Quat | 6 | sym | stub | game/Convert.c |
| `018d1fa0` | CpSifDmaTransEEToIOP | 6 | sym | stub | game/CpDivideEECom.c |
| `01866c10` | asShootArrowSetting | 5 | sym | stub | game/asUseArrow.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018bd830` | AA_clearGsMem | 4 | sym | stub | game/AA_GetDataWord.c |
| `018a7538` | gaSearchNearLight4 | 4 | sym | stub | ga/gaSearchNearLight4.c |
| `018962a0` | tnChangeCharAmbientColorSet | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01889cd0` | Clipping_Portal | 4 | sym | stub | game/Clipping_Portal.c |
| `018a8690` | gaDMAStopMessage_VIF1 | 4 | sym | stub | ga/wait.c |
| `018a0b40` | gaPadRead | 4 | sym | stub | ga/pad_button_read.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018a53c8` | gaDrawObject_RemakeIndexWithClippingCheck | 3 | sym | stub | ga/gaDrawObject.c |
| `018a5730` | gaDrawObject_RemakeIndexWithClippingCheck_WithNear | 3 | sym | stub | ga/gaDrawObject.c |
| `018a5100` | gaDrawObject_VertexLocalView | 3 | sym | stub | ga/gaDrawObject.c |
| `018a52f0` | gaDrawObject_VertexLocalView_Billboard | 3 | sym | stub | ga/gaDrawObject.c |
| `018a5b88` | gaDrawObject_RemakeIndex | 3 | sym | stub | ga/gaDrawObject.c |
| `018a6038` | gaDrawObject_MakeVertexReferenceTableNoClip | 3 | sym | stub | ga/gaDrawObject.c |
| `0188df70` | tiDrawLensFlares | 3 | sym | stub | game/Clipping_Portal.c |
| `018c1ce0` | AA_videoDecAbort | 3 | sym | stub | game/AA_mpegTS.c |
| `018666c0` | asInitArrowInfo | 2 | sym | stub | game/asUseArrow.c |
| `01882c10` | asInitMothInfo | 2 | sym | stub | game/asMothRouteSearch.c |
| `018a6080` | gaDrawObject_SetBig | 2 | sym | stub | ga/gaDrawObject.c |
| `0189f720` | gaDrawObject_Redraw_Polygon | 2 | sym | stub | ga/gaDrawObject_Redraw_Polygon.c |
| `01890ea0` | tiHurikoReset_On | 2 | sym | stub | game/Clipping_Portal.c |
| `01889df0` | tiFreeBossData | 2 | sym | stub | game/Clipping_Portal.c |
| `0189dec0` | ssBGT_ZBufferClear | 2 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `0188ded0` | tiSet_CharaTranslucenceON | 2 | sym | stub | game/Clipping_Portal.c |
| `0188ddc0` | tiSet_CharaTranslucenceOFF | 2 | sym | stub | game/Clipping_Portal.c |
| `018eda60` | SceneDoorFlagClear | 2 | sym | stub | game/tkXDTrg_GetSceneHash.c |
| `018aa950` | CheckLOS_Multi | 2 | sym | stub | game/ssHitCheckAttackToScn.c |

