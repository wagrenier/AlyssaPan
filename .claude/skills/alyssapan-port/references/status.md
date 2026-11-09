# AlyssaPan status (generated)

Generated 2026-11-09 18:29 at commit `0fce112` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 0; tagged game-symbol in Ghidra: 2621
- library matches planned: 0; tagged lib-match / -low in Ghidra: 0 / 0
- classes (named vtables): 0

## Naming (game code; library ranges excluded)

- game functions: **1814**, named **1807** (100%); library functions: 814
- by provenance: game-symbol 2621, lib-match 0, lib-match-low 0, lib-context 0, game-inferred 0
- context-name ledger: 0 entries; overrides of wrong library matches: 0
- placed in a source file: 1814/1814 in 65 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `01850000` | 197/200 (98%) | game/QuatSlerp.c, game/tiAvoidObject.c, game/ahRelocateSunPs2ObjectArry.c |
| `018d0000` | 180/182 (99%) | game/ssGrpPortalCheck.c, game/wideXcmd_TriggerInit.c, game/hhXcmd_DressUp.c |
| `018b0000` | 146/147 (99%) | game/Character_Init.c, game/movie.c, game/AA_SoundMonoSoart.c |
| `01890000` | 147/148 (99%) | game/ssSetTexUVforCubeMap.c, game/Clipping_Portal.c, game/ArtoolDebugOpen.c |
| `01830000` | 39/39 (100%) | game/main.c, game/ahalloc.c, game/ahRelocateSunPs2ObjectArry.c |
| `01840000` | 84/84 (100%) | game/ahRelocateSunPs2ObjectArry.c |
| `01860000` | 113/113 (100%) | game/tiAvoidObject.c, game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/asUseArrow.c |
| `01870000` | 82/82 (100%) | game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/tiBrain_ChgTrace_Ghost.c, game/ssBrain_ChgHideAtk1.c |

## Reconstruction (src/)

- reconstructed CT3 functions: **22**; stubs: **171**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaDma.c` | 0 | 3 |
| `src/ga/gaInit3dlib.c` | 0 | 2 |
| `src/ga/gaInitDrawObject.c` | 0 | 11 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaVifInit.c` | 0 | 3 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/game/ArtoolDebugOpen.c` | 0 | 1 |
| `src/game/Character_Init.c` | 0 | 4 |
| `src/game/Clipping_Portal.c` | 0 | 21 |
| `src/game/LinkMatrixTree.c` | 0 | 2 |
| `src/game/QuatSlerp.c` | 0 | 20 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 0 | 13 |
| `src/game/ahalloc.c` | 0 | 2 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 6 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 2 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/fileio.c` | 0 | 7 |
| `src/game/gaInitMeter.c` | 0 | 3 |
| `src/game/main.c` | 19 | 6 |
| `src/game/main2.c` | 0 | 4 |
| `src/game/movie.c` | 0 | 1 |
| `src/game/sound.c` | 0 | 4 |
| `src/game/ssGrpPortalCheck.c` | 0 | 8 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 3 |
| `src/game/ssSetTexUVforCubeMap.c` | 0 | 24 |
| `src/game/tiAvoidObject.c` | 0 | 3 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 1 |
| `src/game/wideXcmd_DebugPrint.c` | 0 | 12 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 65, partial 1, verify 1
- next: **game/main.c** (pinned in the queue)
- pinned queue: game/main.c, alyssapan/core/window.cpp, sdk/libmc.cpp
- partial: `game/main.c` 19/29 -- Smoke run: main loops, GlobalDrawObject runs every frame but draws nothing (no bank loaded: main2 is a stub); screenshot black, no reference yet. callcheck flag
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c

Recent sessions:

- 2026-11-09 18:28 `game/main.c` partial: main.c slice 2: GlobalDrawObject, scene_draw_init, system_end, get_draw_obj; BANK_INFO.flags (+0x28, BANK_FLAG_DRAW_FIRST), ALL_TABLE (layout host), osTotalObjT
- 2026-11-09 18:17 `ga/gaInitInterrupt.c` verify: the ga VSync: gaInitInterrupt, gaVsyncCallBack, gaWaitVSync (3 of 3)
- 2026-11-09 18:10 `game/main.c` partial: main.c slice 1: main, FirstInitialization, game_init, system_init, ControllerMain, tiItemInit, scene_end and the small draw passes (15 functions); its callees d

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/QuatSlerp.c` | todo | 0/20/177 | 50.3 | 99 | 13 |
| `game/Clipping_Portal.c` | todo | 0/21/73 | 44.8 | 95 | 12 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/12/109 | 40.6 | 80 | 12 |
| `ga/gaInitDrawObject.c` | todo | 0/11/24 | 5.5 | 78 | 11 |
| `game/ssGrpPortalCheck.c` | todo | 0/8/59 | 26.4 | 78 | 7 |
| `game/ahRelocateSunPs2ObjectArry.c` | todo | 0/13/93 | 76.0 | 50 | 7 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/24/96 | 35.3 | 50 | 5 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/6/30 | 23.6 | 30 | 4 |
| `game/main.c` | partial | 19/6/29 | 26.5 | 28 | 2 |
| `game/fileio.c` | todo | 0/7/40 | 23.3 | 23 | 3 |
| `game/Character_Init.c` | todo | 0/4/49 | 30.6 | 21 | 3 |
| `game/main2.c` | todo | 0/4/18 | 6.2 | 20 | 3 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9470` | ssTrigSetVariable | 16 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 9 | sym | stub | game/ssGrpPortalCheck.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `018d9100` | ssTrigCheckEvent | 8 | sym | stub | game/ssGrpPortalCheck.c |
| `01858c90` | XcmdScreenFilter | 8 | sym | stub | game/QuatSlerp.c |
| `01838600` | main_rtn | 6 | sym | stub | game/main.c |
| `018d9560` | ssTrigRemoveEvent | 6 | sym | stub | game/ssGrpPortalCheck.c |
| `0183e200` | SunAllocInit | 6 | sym | stub | game/ahalloc.c |
| `01886690` | asChangeViewMatrix_forTheBank | 4 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `01837570` | main_rtn_Particle2 | 4 | sym | stub | game/main.c |
| `01893cf0` | ExportDrawToTex | 4 | sym | stub | game/Clipping_Portal.c |
| `018a2f30` | gaBGT_Start | 4 | sym | stub | ga/gaInitDrawObject.c |
| `018a3000` | gaBGT_End | 4 | sym | stub | ga/gaInitDrawObject.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018362b0` | AllDrawObject | 3 | sym | stub | game/main.c |
| `018a1f60` | gaBGT_ExecDBufferData_VIF1 | 3 | sym | stub | ga/gaInitDrawObject.c |
| `0188cdc0` | tiVibrationOff | 3 | sym | stub | game/Clipping_Portal.c |
| `018f4fc0` | asCharacterOperate | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018fb940` | asMonsterDistanceStack_inPanic | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `0183d9e0` | CheckAllocNum0 | 3 | sym | stub | game/ahalloc.c |
| `0185e580` | ahGetHash | 3 | sym | stub | game/QuatSlerp.c |
| `0185e150` | asFindModelPartsID | 3 | sym | stub | game/QuatSlerp.c |
| `018fe370` | asSetCharacterKeyEnable | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018b3480` | SelfDiscardSceneLoadInit | 2 | sym | stub | game/main2.c |
| `0185bf80` | WarpObjectUpdateInit | 2 | sym | stub | game/QuatSlerp.c |
| `018c7e00` | FS_work_init | 2 | sym | stub | game/fileio.c |
| `018b4070` | Character_All_Init | 2 | sym | stub | game/Character_Init.c |
| `018666c0` | asInitArrowInfo | 2 | sym | stub | game/asUseArrow.c |
| `01882c10` | asInitMothInfo | 2 | sym | stub | game/asMothRouteSearch.c |
| `01836f30` | DrawObjectRefraction | 2 | sym | stub | game/main.c |
| `0188df70` | tiDrawLensFlares | 2 | sym | stub | game/Clipping_Portal.c |
| `01853620` | PanicFilter | 2 | sym | stub | game/QuatSlerp.c |
| `0185e360` | asCheckBankID | 2 | sym | stub | game/QuatSlerp.c |
| `0189dec0` | ssBGT_ZBufferClear | 2 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018b4480` | SceneLightSet | 2 | sym | stub | game/Character_Init.c |
| `018379d0` | main_rtn_Particle | 2 | sym | stub | game/main.c |
| `018a22b0` | gaBGT_ExecDrawCtrlData_VIF1 | 2 | sym | stub | ga/gaInitDrawObject.c |
| `018a2e70` | gaBGT_ExecDrawCommandData_VIF1 | 2 | sym | stub | ga/gaInitDrawObject.c |
| `018a1a48` | gaInitDrawObject | 2 | sym | stub | ga/gaInitDrawObject.c |
| `018a0b40` | gaPadRead | 2 | sym | stub | ga/pad_button_read.c |

