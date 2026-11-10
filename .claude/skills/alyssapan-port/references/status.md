# AlyssaPan status (generated)

Generated 2026-11-10 00:59 at commit `bd6bd3e` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **202**; stubs: **247**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 0 | 2 |
| `src/ga/gaDma.c` | 16 | 0 |
| `src/ga/gaDmaSync_VIF1.c` | 0 | 3 |
| `src/ga/gaInit3dlib.c` | 0 | 5 |
| `src/ga/gaInitDrawObject.c` | 24 | 0 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaInitMeter.c` | 6 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 0 | 1 |
| `src/ga/gaPadInit.c` | 0 | 1 |
| `src/ga/gaSearchNearLight4.c` | 0 | 1 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 0 | 5 |
| `src/game/Character_Init.c` | 34 | 5 |
| `src/game/Clipping_Portal.c` | 0 | 32 |
| `src/game/LinkMatrixTree.c` | 0 | 4 |
| `src/game/QuatSlerp.c` | 0 | 33 |
| `src/game/SdMasterVolumeSetSE.c` | 0 | 32 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 0 | 28 |
| `src/game/ahalloc.c` | 7 | 0 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 7 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 2 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/fileio.c` | 0 | 10 |
| `src/game/hhXcmd_DressUp.c` | 0 | 1 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 0 | 7 |
| `src/game/movie.c` | 0 | 2 |
| `src/game/sound.c` | 73 | 0 |
| `src/game/ssGrpPortalCheck.c` | 0 | 11 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 3 |
| `src/game/ssSetTexUVforCubeMap.c` | 0 | 30 |
| `src/game/tiAvoidObject.c` | 0 | 3 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 1 |
| `src/game/wideXcmd_DebugPrint.c` | 0 | 13 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 55, partial 1, verify 9, done 2
- next: **game/Character_Init.c** (pinned in the queue)
- pinned queue: game/Character_Init.c, game/ArtoolDebugOpen.c, game/main2.c, game/fileio.c
- partial: `game/Character_Init.c` 34/49 -- Left: data_load_set and data_save_set together next (they share the save image layout: ItemList, ItemData, KeyItemList, LetterList/Data, ArrowList/Data, GlobalE
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `game/AlignedNum.c` 4/4 -- WaitLoop is split in Ghidra: WaitLoop 0x0189f650 plus WaitLoop_body 0x0189f680 (game-inferred), one C function. The j at 0x0189f718 belongs to the next object (
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo

Recent sessions:

- 2026-11-10 00:59 `game/Character_Init.c` partial: slice 4: SceneLightSet, CharLightSet (the bank light tables SCENE_LIGHT / LIGHT_SET / GA_LIGHT_WORK, part light fields), mc_load_set, mc_char_set, mc_CheckMcWor
- 2026-11-10 00:36 `game/Character_Init.c` partial: slice 3: the save path (mc_save_new with its icons and icon.sys, mc_save_normal, mc_save_set, McRegisterEvent, ContinueBackupSet), system_data_load_set with MC_
- 2026-11-10 00:15 `game/Character_Init.c` partial: slice 2: MemorycardUpdate (the four card flows: save menu, load menu, auto save, auto load), mc_save_init, mc_formatcheck, mc_formatcheck2, mc_check4, Memorycar
- 2026-11-09 23:56 `game/Character_Init.c` partial: slice 1: Character_All_Init, IOPInit, McInit, the memory card work buffer (mc_alloc, mc_free, mc_save_data_init), card checks (mc_check, mc_check_always, Memory
- 2026-11-09 23:29 `game/sound.c` verify: slice 3: SSLoadSEfromLoadPoint - the bank's SE load list (SOUND_OBJ_TABLE.loadSE, SE_LOAD_POINT rows); sound.c's 69 functions are written
- 2026-11-09 23:18 `game/sound.c` partial: slice 2: 30 functions - SSProc, SSBGMProc and Branch1..5BGMProc (scene codes, chase BGM, T_BREAK), sound objects and trigger actions, positional SEs, volume sav
- 2026-11-09 22:53 `game/sound.c` partial: slice 1: 38 of 69 functions - boot and end, SE and SQ entry points, the stage SQ rules (Branch1..5 SSPlaySQ/SSStopSQ), movie and demo mute, angle helpers; gFile
- 2026-11-09 22:29 `game/AlignedNum.c` verify: AlignedNum, WaitLoop and IopLoadModule reconstructed (3 functions, the whole object)

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/ssGrpPortalCheck.c` | todo | 0/11/59 | 26.4 | 168 | 7 |
| `game/QuatSlerp.c` | todo | 0/33/177 | 50.3 | 162 | 14 |
| `game/SdMasterVolumeSetSE.c` | todo | 0/32/61 | 22.9 | 121 | 7 |
| `game/Clipping_Portal.c` | todo | 0/32/73 | 44.8 | 116 | 12 |
| `game/ahRelocateSunPs2ObjectArry.c` | todo | 0/28/93 | 76.0 | 89 | 10 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/13/109 | 40.6 | 82 | 12 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/30/96 | 35.3 | 65 | 5 |
| `game/fileio.c` | todo | 0/10/40 | 23.3 | 45 | 4 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/7/30 | 23.6 | 39 | 4 |
| `ga/gaDmaSync_VIF1.c` | todo | 0/3/5 | 0.5 | 35 | 3 |
| `ga/gaInit3dlib.c` | todo | 0/5/25 | 3.6 | 24 | 2 |
| `game/main2.c` | todo | 0/7/18 | 6.2 | 23 | 3 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9100` | ssTrigCheckEvent | 54 | sym | stub | game/ssGrpPortalCheck.c |
| `018d1600` | SdSetEffect | 27 | sym | stub | game/SdMasterVolumeSetSE.c |
| `018d9470` | ssTrigSetVariable | 25 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 19 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9430` | ssTrigGetVariable | 17 | sym | stub | game/ssGrpPortalCheck.c |
| `01886690` | asChangeViewMatrix_forTheBank | 12 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018a17f8` | gaBGT_DmaSendN_fromSPR_VIF1 | 12 | sym | stub | ga/gaDmaSync_VIF1.c |
| `018d9560` | ssTrigRemoveEvent | 12 | sym | stub | game/ssGrpPortalCheck.c |
| `0185e360` | asCheckBankID | 11 | sym | stub | game/QuatSlerp.c |
| `018cd190` | SdSetParamSE | 9 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `01858c90` | XcmdScreenFilter | 8 | sym | stub | game/QuatSlerp.c |
| `018a7db8` | gaSet3dlibStruct | 7 | sym | stub | ga/gaInit3dlib.c |
| `018ce0d0` | SdVolumeSQ | 7 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0185e380` | asCheckBankType | 6 | sym | stub | game/QuatSlerp.c |
| `01854560` | tnInit3dlibWork | 6 | sym | stub | game/QuatSlerp.c |
| `018a3078` | gaObjectClippingCheck | 6 | sym | stub | ga/gaObjectClippingCheck.c |
| `018590b0` | sunBGT_LoadGSTexClutData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `01859520` | sunBGT_ExecTBufferData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `018a3150` | gaBGT_DrawObject | 6 | sym | stub | ga/gaBGT_DrawObject.c |
| `018cc0f0` | SdStopSE | 6 | sym | stub | game/SdMasterVolumeSetSE.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018c23b0` | objSortWithBlurIdx | 5 | sym | stub | game/fileio.c |
| `018c2880` | objSort | 5 | sym | stub | game/fileio.c |
| `01859530` | sunSetupTBufferData | 5 | sym | stub | game/QuatSlerp.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01889cd0` | Clipping_Portal | 4 | sym | stub | game/Clipping_Portal.c |
| `01858f20` | OneMaxObjectDrawZOnly | 4 | sym | stub | game/QuatSlerp.c |
| `018a3d20` | gaBGT_DrawObject_NoClippingCheck2 | 4 | sym | stub | ga/gaBGT_DrawObject.c |
| `018a1778` | gaDmaSendN_VIF1 | 4 | sym | stub | ga/gaDmaSync_VIF1.c |
| `018a1740` | gaDmaSync_VIF1 | 4 | sym | stub | ga/gaDmaSync_VIF1.c |
| `018c7ec0` | fs_error | 4 | sym | stub | game/fileio.c |
| `0185e150` | asFindModelPartsID | 4 | sym | stub | game/QuatSlerp.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018ccde0` | SdFindPlaySE | 3 | sym | stub | game/SdMasterVolumeSetSE.c |
| `0188df70` | tiDrawLensFlares | 3 | sym | stub | game/Clipping_Portal.c |
| `01853620` | PanicFilter | 3 | sym | stub | game/QuatSlerp.c |
| `018fb940` | asMonsterDistanceStack_inPanic | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `0188cdc0` | tiVibrationOff | 3 | sym | stub | game/Clipping_Portal.c |

