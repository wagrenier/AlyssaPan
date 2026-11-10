# AlyssaPan status (generated)

Generated 2026-11-10 12:58 at commit `fd66f69` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **600**; stubs: **181**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 0 | 2 |
| `src/ga/gaDma.c` | 16 | 0 |
| `src/ga/gaDmaSync_VIF1.c` | 0 | 3 |
| `src/ga/gaInit3dlib.c` | 25 | 0 |
| `src/ga/gaInitDrawObject.c` | 24 | 0 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaInitMeter.c` | 6 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 0 | 1 |
| `src/ga/gaPadInit.c` | 0 | 1 |
| `src/ga/gaSearchNearLight4.c` | 0 | 1 |
| `src/ga/gaSinCos.c` | 0 | 4 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 15 | 0 |
| `src/game/Character_Init.c` | 48 | 1 |
| `src/game/Clipping_Portal.c` | 0 | 34 |
| `src/game/Convert.c` | 0 | 2 |
| `src/game/CpDivideEECom.c` | 0 | 1 |
| `src/game/IOPGetFileSize.c` | 0 | 3 |
| `src/game/LinkMatrixTree.c` | 4 | 0 |
| `src/game/QuatSlerp.c` | 189 | 0 |
| `src/game/SdMasterVolumeSetSE.c` | 0 | 34 |
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

- 65 TUs: todo 48, verify 17, done 3
- next: **ga/gaSinCos.c** (pinned in the queue)
- pinned queue: ga/gaSinCos.c
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaInit3dlib.c` 25/25 -- 15 s smoke run: no fault, shot black (no host draw module). gaMulMatrix is dst = b * a (hazards.md). gaSetZRollMatrix now reaches gaSinCos: ga/gaSinCos.c is que
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `game/AlignedNum.c` 4/4 -- WaitLoop is split in Ghidra: WaitLoop 0x0189f650 plus WaitLoop_body 0x0189f680 (game-inferred), one C function. The j at 0x0189f718 belongs to the next object (
- verify: `game/ArtoolDebugOpen.c` 15/15 -- Smoke run: FlInit prints no error, so the pack directory loads and sceCdSearchFile finds the pack; no run reads a file yet (main2 and fileio.c are stubs), the s
- verify: `game/Character_Init.c` 48/49 -- All 49 functions written; nothing on screen reaches the card yet (main2 is a stub), shot black as before. The save image layout is in architecture.md (The save 
- verify: `game/LinkMatrixTree.c` 4/4 -- Code complete, nothing on screen shows it yet: the smoke run still crashes at main.c 2106 (ActressMP NULL, the known blocker) and the shot stays black. LinkMatr
- verify: `game/QuatSlerp.c` 177/177 -- 15 s smoke run: no fault, shot black (no host draw module yet). callcheck: GetAreaFloor's memcmp and ssTrigRemoveEvent come through ROOM_IS and AreaFloorClear (
- verify: `game/ahRelocateSunPs2ObjectArry.c` 93/93 -- All 93 functions written; nothing reaches the general body yet (the two written callers pass one flag each; asCharacterAnimationExecute and asResetCharacterPosi
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/fileio.c` 40/40 -- All 40 functions written. Round2ChangeChar keeps the ROM order of its 150 Round2ChangeSectInfo pairs as three tables; round values named from the file names onl
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/main2.c` 18/18 -- Smoke run leaves main2 and runs the frame loop with no crash; the shot stays black because FSMltLoad (fileio.c) is a stub so no bank loads. PORT BankReady skips
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo

Recent sessions:

- 2026-11-10 12:58 `ga/gaInit3dlib.c` verify: all 25 functions from the disassembly: VU0 vector and matrix helpers, gaContMulMatrix (VU0 micro routines 0 and 0x10), gaSet3dlibStruct (GA_3DLIB_WORK), gaSetLi
- 2026-11-10 12:30 `game/QuatSlerp.c` verify: slice 11: GetAreaFloor (the floor flags a step between paired rooms sets, room literals from the ELF) and asFindMotionID; the TU's code is complete, 177 of 177
- 2026-11-10 12:14 `game/QuatSlerp.c` partial: slice 10: the paged textures (TexFileSetInit, TexFileSetNext, TexFileSetPage, tnTrigTexFileSetNext and Page; ALL_TABLE.texFileSet and the TEX_FILE_SET tables na
- 2026-11-10 11:54 `game/QuatSlerp.c` partial: slice 9: the letterbox requests (VistaVisionStart, VistaVisionEnd), PlayerDamage, asObjectRotate (MODEL_PARTS.placeRot named), asPosRUF2ViewMatrix and asCamRUF2
- 2026-11-10 11:34 `game/QuatSlerp.c` partial: slice 8: the panic setters (PanicSet, PanicStopMovie with PanicOffMovieFile from the ROM), Kekkai_Check, ssPointCheckWithinScreen, PauseUpdate, UpperCase, GetBa
- 2026-11-10 11:25 `game/QuatSlerp.c` partial: slice 7: the letterbox (VistaVisionMain with its two ROM sprite packets and draw control), the fog easing (FogAnimeUpdate, FOG_ANIME named), the depth pre-pass 
- 2026-11-10 10:57 `game/QuatSlerp.c` partial: slice 6: PanicUpdate (the panic gauge: rises, calming items, full panic, PANIC_INFO and Panicdef named, PanicRecoveryTbl and HidePointUpValue from the ROM), Mon
- 2026-11-10 10:39 `game/QuatSlerp.c` partial: slice 5: the part fades AmbientAlphaUpdate runs (DistanceAttenuationUpdate, Target, AlphaSet; AngleAttenuationUpdate, Target, AlphaSet; LosHitFadeUpdate and Tar

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/ssGrpPortalCheck.c` | todo | 0/11/59 | 26.4 | 376 | 10 |
| `game/Clipping_Portal.c` | todo | 0/34/73 | 44.8 | 153 | 18 |
| `game/SdMasterVolumeSetSE.c` | todo | 0/34/61 | 22.9 | 151 | 9 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/17/109 | 40.6 | 91 | 12 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/31/96 | 35.3 | 86 | 9 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/9/30 | 23.6 | 57 | 6 |
| `ga/gaDmaSync_VIF1.c` | todo | 0/3/5 | 0.5 | 39 | 3 |
| `ga/gaSinCos.c` | todo | 0/4/13 | 2.4 | 36 | 1 |
| `game/tiAvoidObject.c` | todo | 0/6/82 | 30.5 | 26 | 4 |
| `game/Convert.c` | todo | 0/2/2 | 0.8 | 23 | 0 |
| `game/ssHitCheckAttackToScn.c` | todo | 0/5/37 | 32.0 | 22 | 3 |
| `ga/gaBGT_DrawObject.c` | todo | 0/2/9 | 7.6 | 18 | 1 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9560` | ssTrigRemoveEvent | 104 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9100` | ssTrigCheckEvent | 98 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 56 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9470` | ssTrigSetVariable | 36 | sym | stub | game/ssGrpPortalCheck.c |
| `018d1600` | SdSetEffect | 27 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018d9430` | ssTrigGetVariable | 26 | sym | stub | game/ssGrpPortalCheck.c |
| `01851b10` | ConvertQuat2Matrix | 17 | sym | stub | game/Convert.c |
| `018a17f8` | gaBGT_DmaSendN_fromSPR_VIF1 | 16 | sym | stub | ga/gaDmaSync_VIF1.c |
| `01886690` | asChangeViewMatrix_forTheBank | 12 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018cc0f0` | SdStopSE | 11 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018cd190` | SdSetParamSE | 10 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `018a6c10` | gaSetRotMatrixY | 8 | sym | stub | ga/gaSinCos.c |
| `018a6ba0` | gaSetRotMatrixX | 8 | sym | stub | ga/gaSinCos.c |
| `018a3150` | gaBGT_DrawObject | 8 | sym | stub | ga/gaBGT_DrawObject.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `018cd3c0` | SdReqPlaySE | 8 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018ce0d0` | SdVolumeSQ | 7 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018a3078` | gaObjectClippingCheck | 7 | sym | stub | ga/gaObjectClippingCheck.c |
| `018518d0` | ConvertMatrix2Quat | 6 | sym | stub | game/Convert.c |
| `018a6c70` | gaSetRotMatrixZ | 6 | sym | stub | ga/gaSinCos.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018a3d20` | gaBGT_DrawObject_NoClippingCheck2 | 5 | sym | stub | ga/gaBGT_DrawObject.c |
| `0188d3c0` | tiFade_Start | 4 | sym | stub | game/Clipping_Portal.c |
| `01883ca0` | asMothPause | 4 | sym | stub | game/asMothPause.c |
| `018a1778` | gaDmaSendN_VIF1 | 4 | sym | stub | ga/gaDmaSync_VIF1.c |
| `018a1740` | gaDmaSync_VIF1 | 4 | sym | stub | ga/gaDmaSync_VIF1.c |
| `018a7538` | gaSearchNearLight4 | 4 | sym | stub | ga/gaSearchNearLight4.c |
| `018f4fc0` | asCharacterOperate | 4 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018a6d00` | gaMemCopy32 | 4 | sym | missing | ga/gaSinCos.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01889cd0` | Clipping_Portal | 4 | sym | stub | game/Clipping_Portal.c |
| `0188cdc0` | tiVibrationOff | 4 | sym | stub | game/Clipping_Portal.c |
| `018a6b70` | gaSinCos | 4 | sym | stub | ga/gaSinCos.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018cc1b0` | SdStopSEAllSp1 | 3 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018cddd0` | SdStopStream | 3 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018870c0` | camera_init0 | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `01886dc0` | CamChange_S | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018fe370` | asSetCharacterKeyEnable | 3 | sym | stub | game/wideXcmd_DebugPrint.c |

