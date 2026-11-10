# AlyssaPan status (generated)

Generated 2026-11-09 19:11 at commit `6895fbc` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **26**; stubs: **184**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 0 | 2 |
| `src/ga/gaDma.c` | 0 | 3 |
| `src/ga/gaInit3dlib.c` | 0 | 3 |
| `src/ga/gaInitDrawObject.c` | 0 | 13 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 0 | 1 |
| `src/ga/gaVifInit.c` | 0 | 3 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/game/ArtoolDebugOpen.c` | 0 | 1 |
| `src/game/Character_Init.c` | 0 | 4 |
| `src/game/Clipping_Portal.c` | 0 | 22 |
| `src/game/LinkMatrixTree.c` | 0 | 2 |
| `src/game/QuatSlerp.c` | 0 | 27 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 0 | 13 |
| `src/game/ahalloc.c` | 0 | 2 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 6 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 2 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/fileio.c` | 0 | 7 |
| `src/game/gaInitMeter.c` | 0 | 3 |
| `src/game/main.c` | 23 | 3 |
| `src/game/main2.c` | 0 | 4 |
| `src/game/movie.c` | 0 | 1 |
| `src/game/sound.c` | 0 | 4 |
| `src/game/ssGrpPortalCheck.c` | 0 | 8 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 3 |
| `src/game/ssSetTexUVforCubeMap.c` | 0 | 26 |
| `src/game/tiAvoidObject.c` | 0 | 3 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 1 |
| `src/game/wideXcmd_DebugPrint.c` | 0 | 12 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 65, partial 1, verify 1
- next: **game/main.c** (pinned in the queue)
- pinned queue: game/main.c, alyssapan/core/window.cpp, sdk/libmc.cpp
- partial: `game/main.c` 23/29 -- Smoke run: build green, layoutcheck OK; screenshot black, main2 is still a stub so no bank loads and no draw pass runs. callcheck flags DrawObjectRefraction lik
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c

Recent sessions:

- 2026-11-09 19:11 `game/main.c` partial: main.c slice 4: DrawObjectRefraction (the refraction pass over type-1 banks 6..85 and refracting characters) and main_rtn_DOF (main_rtn for one depth-of-field s
- 2026-11-09 18:58 `game/main.c` partial: main.c slice 3: main_rtn (the bank list draw: sort, fog, 3dlib set-up, per-part texture / draw-ctrl / light / clip) and main_rtn_Particle (particle rings); DRAW
- 2026-11-09 18:28 `game/main.c` partial: main.c slice 2: GlobalDrawObject, scene_draw_init, system_end, get_draw_obj; BANK_INFO.flags (+0x28, BANK_FLAG_DRAW_FIRST), ALL_TABLE (layout host), osTotalObjT
- 2026-11-09 18:17 `ga/gaInitInterrupt.c` verify: the ga VSync: gaInitInterrupt, gaVsyncCallBack, gaWaitVSync (3 of 3)
- 2026-11-09 18:10 `game/main.c` partial: main.c slice 1: main, FirstInitialization, game_init, system_init, ControllerMain, tiItemInit, scene_end and the small draw passes (15 functions); its callees d

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/QuatSlerp.c` | todo | 0/27/177 | 50.3 | 119 | 13 |
| `game/Clipping_Portal.c` | todo | 0/22/73 | 44.8 | 101 | 12 |
| `ga/gaInitDrawObject.c` | todo | 0/13/24 | 5.5 | 98 | 11 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/12/109 | 40.6 | 80 | 12 |
| `game/ssGrpPortalCheck.c` | todo | 0/8/59 | 26.4 | 78 | 7 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/26/96 | 35.3 | 55 | 5 |
| `game/ahRelocateSunPs2ObjectArry.c` | todo | 0/13/93 | 76.0 | 50 | 7 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/6/30 | 23.6 | 33 | 4 |
| `game/fileio.c` | todo | 0/7/40 | 23.3 | 27 | 3 |
| `game/Character_Init.c` | todo | 0/4/49 | 30.6 | 21 | 3 |
| `game/main2.c` | todo | 0/4/18 | 6.2 | 20 | 3 |
| `game/sound.c` | todo | 0/4/69 | 15.5 | 19 | 3 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9470` | ssTrigSetVariable | 16 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 9 | sym | stub | game/ssGrpPortalCheck.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `018a3000` | gaBGT_End | 8 | sym | stub | ga/gaInitDrawObject.c |
| `018a2f30` | gaBGT_Start | 8 | sym | stub | ga/gaInitDrawObject.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `018d9100` | ssTrigCheckEvent | 8 | sym | stub | game/ssGrpPortalCheck.c |
| `01858c90` | XcmdScreenFilter | 8 | sym | stub | game/QuatSlerp.c |
| `01886690` | asChangeViewMatrix_forTheBank | 7 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018a22c8` | gaSetupFogCtrlData | 6 | sym | stub | ga/gaInitDrawObject.c |
| `01837570` | main_rtn_Particle2 | 6 | sym | stub | game/main.c |
| `018d9560` | ssTrigRemoveEvent | 6 | sym | stub | game/ssGrpPortalCheck.c |
| `0183e200` | SunAllocInit | 6 | sym | stub | game/ahalloc.c |
| `0185e360` | asCheckBankID | 5 | sym | stub | game/QuatSlerp.c |
| `018a22b0` | gaBGT_ExecDrawCtrlData_VIF1 | 5 | sym | stub | ga/gaInitDrawObject.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018362b0` | AllDrawObject | 3 | sym | stub | game/main.c |
| `0185e380` | asCheckBankType | 3 | sym | stub | game/QuatSlerp.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 3 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018c23b0` | objSortWithBlurIdx | 3 | sym | stub | game/fileio.c |
| `018c2880` | objSort | 3 | sym | stub | game/fileio.c |
| `018a7db8` | gaSet3dlibStruct | 3 | sym | stub | ga/gaInit3dlib.c |
| `01854560` | tnInit3dlibWork | 3 | sym | stub | game/QuatSlerp.c |
| `018a2358` | gaBGT_ExecFogCtrlData_VIF1 | 3 | sym | stub | ga/gaInitDrawObject.c |
| `018a3078` | gaObjectClippingCheck | 3 | sym | stub | ga/gaObjectClippingCheck.c |
| `018590b0` | sunBGT_LoadGSTexClutData_VIF1 | 3 | sym | stub | game/QuatSlerp.c |
| `01859520` | sunBGT_ExecTBufferData_VIF1 | 3 | sym | stub | game/QuatSlerp.c |
| `018a3150` | gaBGT_DrawObject | 3 | sym | stub | ga/gaBGT_DrawObject.c |
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

