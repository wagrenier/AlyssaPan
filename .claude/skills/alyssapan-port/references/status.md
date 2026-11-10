# AlyssaPan status (generated)

Generated 2026-11-10 14:36 at commit `b24ca8e` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **649**; stubs: **169**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 0 | 2 |
| `src/ga/gaDma.c` | 16 | 0 |
| `src/ga/gaDmaSync_VIF1.c` | 6 | 0 |
| `src/ga/gaInit3dlib.c` | 25 | 0 |
| `src/ga/gaInitDrawObject.c` | 24 | 0 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaInitMeter.c` | 6 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 0 | 1 |
| `src/ga/gaPadInit.c` | 0 | 1 |
| `src/ga/gaSearchNearLight4.c` | 0 | 1 |
| `src/ga/gaSinCos.c` | 14 | 0 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/ga/wait.c` | 0 | 2 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 15 | 0 |
| `src/game/Character_Init.c` | 48 | 1 |
| `src/game/Clipping_Portal.c` | 0 | 34 |
| `src/game/Convert.c` | 0 | 2 |
| `src/game/CpDivideEECom.c` | 0 | 5 |
| `src/game/IOPGetFileSize.c` | 4 | 0 |
| `src/game/LinkMatrixTree.c` | 4 | 0 |
| `src/game/QuatSlerp.c` | 189 | 0 |
| `src/game/SdMasterVolumeSetSE.c` | 25 | 26 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 93 | 0 |
| `src/game/ahalloc.c` | 7 | 0 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 9 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 2 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/fileio.c` | 40 | 0 |
| `src/game/hhXcmd_DressUp.c` | 0 | 1 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 18 | 0 |
| `src/game/movie.c` | 0 | 3 |
| `src/game/sound.c` | 73 | 0 |
| `src/game/ssCalcInversMatrix2.c` | 0 | 1 |
| `src/game/ssGrpPortalCheck.c` | 0 | 11 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 5 |
| `src/game/ssSetTexUVforCubeMap.c` | 0 | 31 |
| `src/game/tiAvoidObject.c` | 0 | 6 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 3 |
| `src/game/wideXcmd_DebugPrint.c` | 0 | 17 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 44, partial 1, verify 20, done 3
- next: **game/SdMasterVolumeSetSE.c** (pinned in the queue)
- pinned queue: game/SdMasterVolumeSetSE.c, ga/gaPadInit.c, game/movie.c, game/wideXcmd_DebugPrint.c, game/tkXDTrg_GetSceneHash.c, game/asUseArrow.c, game/asMothRouteSearch.c, game/asXcmd.c, game/asEyeForcusUP2CamMat.c, game/ssHitCheckAttackToScn.c, game/ssSetTexUVforCubeMap.c, game/ssGrpPortalCheck.c
- partial: `game/SdMasterVolumeSetSE.c` 25/61 -- Left: SdLoadSQProc (0x018ce6c0, 0x138c bytes, the SQ twin of SdLoadSEProc: reuse its step pattern and decide SD_SQ_LD layout host for its buffer word), SdPlaySE
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaDmaSync_VIF1.c` 5/5 -- Host: kicks leave CHCR.STR clear (the transfer is over), so the syncs return at once; the BGT send keeps the copy and the flip, the packet goes nowhere (draw mo
- verify: `ga/gaInit3dlib.c` 25/25 -- 15 s smoke run: no fault, shot black (no host draw module). gaMulMatrix is dst = b * a (hazards.md). gaSetZRollMatrix now reaches gaSinCos: ga/gaSinCos.c is que
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaSinCos.c` 13/13 -- 15 s smoke run: no fault, about 26700 frames, shot black (no host draw module yet). ga_sincos_table.inc generated from the ELF (the ROM floats differ from sinf)
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `game/AlignedNum.c` 4/4 -- WaitLoop is split in Ghidra: WaitLoop 0x0189f650 plus WaitLoop_body 0x0189f680 (game-inferred), one C function. The j at 0x0189f718 belongs to the next object (
- verify: `game/ArtoolDebugOpen.c` 15/15 -- Smoke run: FlInit prints no error, so the pack directory loads and sceCdSearchFile finds the pack; no run reads a file yet (main2 and fileio.c are stubs), the s
- verify: `game/Character_Init.c` 48/49 -- All 49 functions written; nothing on screen reaches the card yet (main2 is a stub), shot black as before. The save image layout is in architecture.md (The save 
- verify: `game/IOPGetFileSize.c` 4/4 -- All 4 functions written. No host server answers FSYS_IOP SUNA yet (sif.cpp answers zeros), so a loose file's size reads 0: a host FSYS_IOP server on io::Disc is
- verify: `game/LinkMatrixTree.c` 4/4 -- Code complete, nothing on screen shows it yet: the smoke run still crashes at main.c 2106 (ActressMP NULL, the known blocker) and the shot stays black. LinkMatr
- verify: `game/QuatSlerp.c` 177/177 -- 15 s smoke run: no fault, shot black (no host draw module yet). callcheck: GetAreaFloor's memcmp and ssTrigRemoveEvent come through ROOM_IS and AreaFloorClear (
- verify: `game/ahRelocateSunPs2ObjectArry.c` 93/93 -- All 93 functions written; nothing reaches the general body yet (the two written callers pass one flag each; asCharacterAnimationExecute and asResetCharacterPosi
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/fileio.c` 40/40 -- All 40 functions written. Round2ChangeChar keeps the ROM order of its 150 Round2ChangeSectInfo pairs as three tables; round values named from the file names onl
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/main2.c` 18/18 -- Smoke run leaves main2 and runs the frame loop with no crash; the shot stays black because FSMltLoad (fileio.c) is a stub so no bank loads. PORT BankReady skips
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo

Recent sessions:

- 2026-11-10 14:36 `game/SdMasterVolumeSetSE.c` partial: slice 2: SdProc, the SE bank loader (SdLoadSEProc and its steps, NextLoadSE), SdVolumeSetSQ, SdMasterVolumeSetSQ; FL_PACK_INFO.partSize; host IOP heap and sceSd
- 2026-11-10 14:15 `game/SdMasterVolumeSetSE.c` partial: slice 1: the FSS command ring and thread (FIFO, FSS_Thread, _Init, _Sync), SE master and stream volumes, reverb, SE stop and lookup, load-end wait, SdAlloc, SdF
- 2026-11-10 13:49 `game/IOPGetFileSize.c` verify: IOPGetFileSize, IOPBindRPC, bind_rpc, IOPComSync: the EE side of FSYS_IOP's file server; gSCD moved here
- 2026-11-10 13:34 `ga/gaDmaSync_VIF1.c` verify: all 5 functions from the disassembly: the VIF1 / GIF syncs with their timeout reports, the VIF1 normal and chain kicks, and the BGT ring send (memory packets co
- 2026-11-10 13:14 `ga/gaSinCos.c` verify: all 13 functions from the disassembly: gaSinCos and the X/Y/Z rotation matrices over the embedded ga_sincos_table, gaInt/gaFloat, the word/dword/qword copies an
- 2026-11-10 12:58 `ga/gaInit3dlib.c` verify: all 25 functions from the disassembly: VU0 vector and matrix helpers, gaContMulMatrix (VU0 micro routines 0 and 0x10), gaSet3dlibStruct (GA_3DLIB_WORK), gaSetLi
- 2026-11-10 12:30 `game/QuatSlerp.c` verify: slice 11: GetAreaFloor (the floor flags a step between paired rooms sets, room literals from the ELF) and asFindMotionID; the TU's code is complete, 177 of 177
- 2026-11-10 12:14 `game/QuatSlerp.c` partial: slice 10: the paged textures (TexFileSetInit, TexFileSetNext, TexFileSetPage, tnTrigTexFileSetNext and Page; ALL_TABLE.texFileSet and the TEX_FILE_SET tables na

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/ssGrpPortalCheck.c` | todo | 0/11/59 | 26.4 | 376 | 10 |
| `game/Clipping_Portal.c` | todo | 0/34/73 | 44.8 | 153 | 18 |
| `game/SdMasterVolumeSetSE.c` | partial | 25/26/61 | 22.9 | 92 | 7 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/17/109 | 40.6 | 91 | 12 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/31/96 | 35.3 | 86 | 9 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/9/30 | 23.6 | 57 | 6 |
| `game/tiAvoidObject.c` | todo | 0/6/82 | 30.5 | 26 | 4 |
| `game/Convert.c` | todo | 0/2/2 | 0.8 | 23 | 0 |
| `game/ssHitCheckAttackToScn.c` | todo | 0/5/37 | 32.0 | 22 | 3 |
| `ga/gaBGT_DrawObject.c` | todo | 0/2/9 | 7.6 | 18 | 1 |
| `game/tkXDTrg_GetSceneHash.c` | todo | 0/3/50 | 21.0 | 14 | 2 |
| `game/CpDivideEECom.c` | todo | 0/5/18 | 3.3 | 13 | 1 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9560` | ssTrigRemoveEvent | 104 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9100` | ssTrigCheckEvent | 98 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 56 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9470` | ssTrigSetVariable | 36 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9430` | ssTrigGetVariable | 26 | sym | stub | game/ssGrpPortalCheck.c |
| `01851b10` | ConvertQuat2Matrix | 17 | sym | stub | game/Convert.c |
| `01886690` | asChangeViewMatrix_forTheBank | 12 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018cd190` | SdSetParamSE | 10 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `018a3150` | gaBGT_DrawObject | 8 | sym | stub | ga/gaBGT_DrawObject.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `018cd3c0` | SdReqPlaySE | 8 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018ce0d0` | SdVolumeSQ | 7 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018a3078` | gaObjectClippingCheck | 7 | sym | stub | ga/gaObjectClippingCheck.c |
| `018518d0` | ConvertMatrix2Quat | 6 | sym | stub | game/Convert.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018a3d20` | gaBGT_DrawObject_NoClippingCheck2 | 5 | sym | stub | ga/gaBGT_DrawObject.c |
| `0188d3c0` | tiFade_Start | 4 | sym | stub | game/Clipping_Portal.c |
| `01883ca0` | asMothPause | 4 | sym | stub | game/asMothPause.c |
| `018a7538` | gaSearchNearLight4 | 4 | sym | stub | ga/gaSearchNearLight4.c |
| `018f4fc0` | asCharacterOperate | 4 | sym | stub | game/wideXcmd_DebugPrint.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01889cd0` | Clipping_Portal | 4 | sym | stub | game/Clipping_Portal.c |
| `0188cdc0` | tiVibrationOff | 4 | sym | stub | game/Clipping_Portal.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018cc1b0` | SdStopSEAllSp1 | 3 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018870c0` | camera_init0 | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `01886dc0` | CamChange_S | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018fe370` | asSetCharacterKeyEnable | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018881a0` | camera_main | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018f6f80` | asCharacterPositioning | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018cc040` | SdStopSESDT | 3 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0188df70` | tiDrawLensFlares | 3 | sym | stub | game/Clipping_Portal.c |
| `018cbfe0` | SdStopSEAll | 3 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0188cd70` | tiVibrationTimer | 3 | sym | stub | game/Clipping_Portal.c |
| `018fb940` | asMonsterDistanceStack_inPanic | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018d1fa0` | CpSifDmaTransEEToIOP | 3 | sym | stub | game/CpDivideEECom.c |
| `018a8690` | gaDMAStopMessage_VIF1 | 3 | sym | stub | ga/wait.c |
| `018666c0` | asInitArrowInfo | 2 | sym | stub | game/asUseArrow.c |
| `01882c10` | asInitMothInfo | 2 | sym | stub | game/asMothRouteSearch.c |

