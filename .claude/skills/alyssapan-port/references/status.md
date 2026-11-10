# AlyssaPan status (generated)

Generated 2026-11-10 05:34 at commit `01398ea` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 0; tagged game-symbol in Ghidra: 2621
- library matches planned: 0; tagged lib-match / -low in Ghidra: 0 / 0
- classes (named vtables): 0

## Naming (game code; library ranges excluded)

- game functions: **1814**, named **1808** (100%); library functions: 814
- by provenance: game-symbol 2621, lib-match 0, lib-match-low 0, lib-context 0, game-inferred 1
- context-name ledger: 0 entries; overrides of wrong library matches: 0
- placed in a source file: 1814/1814 in 65 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `01850000` | 197/200 (98%) | game/QuatSlerp.c, game/tiAvoidObject.c, game/ahRelocateSunPs2ObjectArry.c |
| `018d0000` | 180/182 (99%) | game/ssGrpPortalCheck.c, game/wideXcmd_TriggerInit.c, game/hhXcmd_DressUp.c |
| `018b0000` | 146/147 (99%) | game/Character_Init.c, game/movie.c, game/AA_SoundMonoSoart.c |
| `01830000` | 39/39 (100%) | game/main.c, game/ahalloc.c, game/ahRelocateSunPs2ObjectArry.c |
| `01840000` | 84/84 (100%) | game/ahRelocateSunPs2ObjectArry.c |
| `01860000` | 113/113 (100%) | game/tiAvoidObject.c, game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/asUseArrow.c |
| `01870000` | 82/82 (100%) | game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/tiBrain_ChgTrace_Ghost.c, game/ssBrain_ChgHideAtk1.c |
| `01880000` | 103/103 (100%) | game/Clipping_Portal.c, game/asEyeForcusUP2CamMat.c, game/ssBrain_ChgHideAtk1.c |

## Reconstruction (src/)

- reconstructed CT3 functions: **341**; stubs: **241**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 0 | 2 |
| `src/ga/gaDma.c` | 16 | 0 |
| `src/ga/gaDmaSync_VIF1.c` | 0 | 3 |
| `src/ga/gaInit3dlib.c` | 0 | 6 |
| `src/ga/gaInitDrawObject.c` | 24 | 0 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaInitMeter.c` | 6 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 0 | 1 |
| `src/ga/gaPadInit.c` | 0 | 1 |
| `src/ga/gaSearchNearLight4.c` | 0 | 1 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 15 | 0 |
| `src/game/Character_Init.c` | 48 | 1 |
| `src/game/Clipping_Portal.c` | 0 | 33 |
| `src/game/Convert.c` | 0 | 2 |
| `src/game/CpDivideEECom.c` | 0 | 1 |
| `src/game/IOPGetFileSize.c` | 0 | 3 |
| `src/game/LinkMatrixTree.c` | 4 | 0 |
| `src/game/QuatSlerp.c` | 2 | 49 |
| `src/game/SdMasterVolumeSetSE.c` | 0 | 34 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 53 | 14 |
| `src/game/ahalloc.c` | 7 | 0 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 9 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 2 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/fileio.c` | 33 | 4 |
| `src/game/hhXcmd_DressUp.c` | 0 | 1 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 18 | 0 |
| `src/game/movie.c` | 0 | 2 |
| `src/game/sound.c` | 73 | 0 |
| `src/game/ssCalcInversMatrix2.c` | 0 | 1 |
| `src/game/ssGrpPortalCheck.c` | 0 | 11 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 3 |
| `src/game/ssSetTexUVforCubeMap.c` | 0 | 30 |
| `src/game/tiAvoidObject.c` | 0 | 6 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 3 |
| `src/game/wideXcmd_DebugPrint.c` | 0 | 13 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 49, partial 3, verify 13, done 3
- next: **game/ahRelocateSunPs2ObjectArry.c** (pinned in the queue)
- pinned queue: game/ahRelocateSunPs2ObjectArry.c, game/fileio.c
- partial: `game/ahRelocateSunPs2ObjectArry.c` 53/93 -- Next in this TU: ExecuteAnimation (0x16bc, reached every frame, a slice of its own), then EA_100 and InterpolateAnimationKey / IAK_0 (now stubbed). Shot still b
- partial: `game/fileio.c` 33/40 -- Written: FSMltLoadProc (steps FSMLT_*), ReserveBank, Extract; FQUE +0 is dir. The first global bank now loads and the run crashes in scene_init (main.c) reading
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `game/AlignedNum.c` 4/4 -- WaitLoop is split in Ghidra: WaitLoop 0x0189f650 plus WaitLoop_body 0x0189f680 (game-inferred), one C function. The j at 0x0189f718 belongs to the next object (
- verify: `game/ArtoolDebugOpen.c` 15/15 -- Smoke run: FlInit prints no error, so the pack directory loads and sceCdSearchFile finds the pack; no run reads a file yet (main2 and fileio.c are stubs), the s
- verify: `game/Character_Init.c` 48/49 -- All 49 functions written; nothing on screen reaches the card yet (main2 is a stub), shot black as before. The save image layout is in architecture.md (The save 
- verify: `game/LinkMatrixTree.c` 4/4 -- Code complete, nothing on screen shows it yet: the smoke run still crashes at main.c 2106 (ActressMP NULL, the known blocker) and the shot stays black. LinkMatr
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/main2.c` 18/18 -- Smoke run leaves main2 and runs the frame loop with no crash; the shot stays black because FSMltLoad (fileio.c) is a stub so no bank loads. PORT BankReady skips
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo

Recent sessions:

- 2026-11-10 05:34 `game/ahRelocateSunPs2ObjectArry.c` partial: slice 7: asStartAnimation, asDecInterpolateTicks, asGetBehaviorPlayingTicks, asBehaviorActEndAnimation, asForceOperateMirroring_Character, asBuildDOFMatrixTree 
- 2026-11-10 05:16 `game/ahRelocateSunPs2ObjectArry.c` partial: slice 6: asMakeStraw from the disassembly (the straw joints: slerped and Bezier-blended frames cached per weight in SkinWork, the plain and squeeze skin loops w
- 2026-11-10 05:03 `game/ahRelocateSunPs2ObjectArry.c` partial: slice 5: the double buffer (asCopyDoubleBuffer, asCopyDoubleBufferL2V, gNoDraw), BMT_20, Execute_ExecuteAnimationHistory (EAS_RECORD host layout, FrameCounter, 
- 2026-11-10 04:33 `game/LinkMatrixTree.c` verify: all 4 functions: InitMatrixTree, LinkMatrixTree (parent words MATUNIT_EE or BankEeAddr, decision h), ahTimeInit, ahTimeMaintenance; tick globals moved here; Par
- 2026-11-10 04:24 `game/ahRelocateSunPs2ObjectArry.c` partial: slice 4: BuildMatrixTree from the disassembly (look-at, link, light parts; parent words stay EE addresses), behaviour start / stop, asShadowDrawing_Fin, asDrawS
- 2026-11-10 03:56 `game/ahRelocateSunPs2ObjectArry.c` partial: slice 3: seek bars (asSeekBarInitializeForX, asSeekBarFreeForX, asSeekBarAnimation), asSeatArrange_Init, BMT_01, ExecuteAnimationHistory_Free; ALL_TABLE +0xc0 /
- 2026-11-10 03:34 `game/ahRelocateSunPs2ObjectArry.c` partial: slice 2: the animation pass (asUpDateSceneAnimation, asUpDateGAobjectsAnimation, asCheckAnimationExecutable_Scene, asCheckNowMoving, asForceOperateAnimation_Sce
- 2026-11-10 03:12 `game/ct3_types.h` done: ALL_TABLE, DRAW_OBJ_ENTRY, TEX_INFO, PARTICLE_ENTRY, PARTICLE2_ENTRY in EE layout; pointer words u_int read through BANK_PTR in main.c, Character_Init.c, sound.

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/QuatSlerp.c` | partial | 2/49/177 | 50.3 | 242 | 17 |
| `game/ssGrpPortalCheck.c` | todo | 0/11/59 | 26.4 | 225 | 10 |
| `game/SdMasterVolumeSetSE.c` | todo | 0/34/61 | 22.9 | 134 | 9 |
| `game/Clipping_Portal.c` | todo | 0/33/73 | 44.8 | 130 | 14 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/30/96 | 35.3 | 79 | 8 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/13/109 | 40.6 | 76 | 11 |
| `ga/gaInit3dlib.c` | todo | 0/6/25 | 3.6 | 55 | 4 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/9/30 | 23.6 | 52 | 5 |
| `ga/gaDmaSync_VIF1.c` | todo | 0/3/5 | 0.5 | 35 | 3 |
| `game/ahRelocateSunPs2ObjectArry.c` | partial | 53/14/93 | 76.0 | 30 | 1 |
| `game/tiAvoidObject.c` | todo | 0/6/82 | 30.5 | 26 | 4 |
| `game/fileio.c` | partial | 33/4/40 | 23.3 | 23 | 2 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9100` | ssTrigCheckEvent | 58 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9470` | ssTrigSetVariable | 33 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 30 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9560` | ssTrigRemoveEvent | 30 | sym | stub | game/ssGrpPortalCheck.c |
| `018d1600` | SdSetEffect | 27 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018a7900` | gaMulMatrix | 20 | sym | stub | ga/gaInit3dlib.c |
| `018d9430` | ssTrigGetVariable | 18 | sym | stub | game/ssGrpPortalCheck.c |
| `0185e360` | asCheckBankID | 18 | sym | stub | game/QuatSlerp.c |
| `0185e150` | asFindModelPartsID | 14 | sym | stub | game/QuatSlerp.c |
| `018a17f8` | gaBGT_DmaSendN_fromSPR_VIF1 | 12 | sym | stub | ga/gaDmaSync_VIF1.c |
| `01886690` | asChangeViewMatrix_forTheBank | 12 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `0185e580` | ahGetHash | 12 | sym | stub | game/QuatSlerp.c |
| `018cd190` | SdSetParamSE | 9 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0185e100` | asFindModelParts | 9 | sym | stub | game/QuatSlerp.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `01858c90` | XcmdScreenFilter | 8 | sym | stub | game/QuatSlerp.c |
| `018ce0d0` | SdVolumeSQ | 7 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0185e380` | asCheckBankType | 7 | sym | stub | game/QuatSlerp.c |
| `018a7db8` | gaSet3dlibStruct | 7 | sym | stub | ga/gaInit3dlib.c |
| `01854560` | tnInit3dlibWork | 6 | sym | stub | game/QuatSlerp.c |
| `018590b0` | sunBGT_LoadGSTexClutData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `01859520` | sunBGT_ExecTBufferData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `018a3078` | gaObjectClippingCheck | 6 | sym | stub | ga/gaObjectClippingCheck.c |
| `018a3150` | gaBGT_DrawObject | 6 | sym | stub | ga/gaBGT_DrawObject.c |
| `018cc0f0` | SdStopSE | 6 | sym | stub | game/SdMasterVolumeSetSE.c |
| `01859530` | sunSetupTBufferData | 5 | sym | stub | game/QuatSlerp.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018c23b0` | objSortWithBlurIdx | 5 | sym | stub | game/fileio.c |
| `018c2880` | objSort | 5 | sym | stub | game/fileio.c |
| `0188d3c0` | tiFade_Start | 4 | sym | stub | game/Clipping_Portal.c |
| `01854d60` | MonsterAIUpdate | 4 | sym | stub | game/QuatSlerp.c |
| `018a1778` | gaDmaSendN_VIF1 | 4 | sym | stub | ga/gaDmaSync_VIF1.c |
| `018a1740` | gaDmaSync_VIF1 | 4 | sym | stub | ga/gaDmaSync_VIF1.c |
| `018a7538` | gaSearchNearLight4 | 4 | sym | stub | ga/gaSearchNearLight4.c |
| `018a7780` | gaInnerProductXYZ | 4 | sym | stub | ga/gaInit3dlib.c |
| `01847660` | asShadowDrawing_CheckTransMode | 4 | sym | stub | game/ahRelocateSunPs2ObjectArry.c |
| `0184dbc0` | InterpolateAnimationKey | 4 | sym | stub | game/ahRelocateSunPs2ObjectArry.c |
| `01851b10` | ConvertQuat2Matrix | 4 | sym | stub | game/Convert.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |

