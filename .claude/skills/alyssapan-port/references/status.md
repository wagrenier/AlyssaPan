# AlyssaPan status (generated)

Generated 2026-11-09 20:20 at commit `3f2e3ba` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **32**; stubs: **219**

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
| `src/game/Character_Init.c` | 0 | 5 |
| `src/game/Clipping_Portal.c` | 0 | 31 |
| `src/game/LinkMatrixTree.c` | 0 | 4 |
| `src/game/QuatSlerp.c` | 0 | 30 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 0 | 28 |
| `src/game/ahalloc.c` | 0 | 2 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 7 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 2 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/fileio.c` | 0 | 7 |
| `src/game/gaInitMeter.c` | 0 | 3 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 0 | 5 |
| `src/game/movie.c` | 0 | 1 |
| `src/game/sound.c` | 0 | 4 |
| `src/game/ssGrpPortalCheck.c` | 0 | 10 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 3 |
| `src/game/ssSetTexUVforCubeMap.c` | 0 | 30 |
| `src/game/tiAvoidObject.c` | 0 | 3 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 1 |
| `src/game/wideXcmd_DebugPrint.c` | 0 | 12 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 64, verify 2, done 1
- next: **sdk/libmc.cpp** (pinned in the queue)
- pinned queue: sdk/libmc.cpp
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra

Recent sessions:

- 2026-11-09 20:20 `alyssapan/core/window.cpp` done: siblings shims (a): pad database, window and exe icon, mute and pause in the background, overlay DPI scale, cursor auto-hide
- 2026-11-09 19:50 `game/main.c` verify: main.c slice 7: main_rtn_Particle2 (the second particle list: ring records placed at their model part position plus an offset, w scaled around 128); PARTICLE2_E
- 2026-11-09 19:39 `game/main.c` partial: main.c slice 6: scene_init (a bank's set-up after load: BaseobjCount / pModelPartsHeader / pBehaviorHeader / FogCtrlAddr from ALL_TABLE +0x14 / +0x2c / +0xbc / 
- 2026-11-09 19:28 `game/main.c` partial: main.c slice 5: AllDrawObject (the scene draw: banks, sorted characters, mirrors, translucent banks, particles, translucent and ghost characters), ssHQDOF_DrawA
- 2026-11-09 19:11 `game/main.c` partial: main.c slice 4: DrawObjectRefraction (the refraction pass over type-1 banks 6..85 and refracting characters) and main_rtn_DOF (main_rtn for one depth-of-field s
- 2026-11-09 18:58 `game/main.c` partial: main.c slice 3: main_rtn (the bank list draw: sort, fog, 3dlib set-up, per-part texture / draw-ctrl / light / clip) and main_rtn_Particle (particle rings); DRAW
- 2026-11-09 18:28 `game/main.c` partial: main.c slice 2: GlobalDrawObject, scene_draw_init, system_end, get_draw_obj; BANK_INFO.flags (+0x28, BANK_FLAG_DRAW_FIRST), ALL_TABLE (layout host), osTotalObjT
- 2026-11-09 18:17 `ga/gaInitInterrupt.c` verify: the ga VSync: gaInitInterrupt, gaVsyncCallBack, gaWaitVSync (3 of 3)

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/QuatSlerp.c` | todo | 0/30/177 | 50.3 | 151 | 14 |
| `ga/gaInitDrawObject.c` | todo | 0/13/24 | 5.5 | 126 | 11 |
| `game/Clipping_Portal.c` | todo | 0/31/73 | 44.8 | 113 | 12 |
| `game/ahRelocateSunPs2ObjectArry.c` | todo | 0/28/93 | 76.0 | 82 | 9 |
| `game/ssGrpPortalCheck.c` | todo | 0/10/59 | 26.4 | 82 | 7 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/12/109 | 40.6 | 80 | 12 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/30/96 | 35.3 | 64 | 5 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/7/30 | 23.6 | 39 | 4 |
| `game/fileio.c` | todo | 0/7/40 | 23.3 | 31 | 3 |
| `game/Character_Init.c` | todo | 0/5/49 | 30.6 | 25 | 3 |
| `game/main2.c` | todo | 0/5/18 | 6.2 | 21 | 3 |
| `game/sound.c` | todo | 0/4/69 | 15.5 | 19 | 3 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9470` | ssTrigSetVariable | 16 | sym | stub | game/ssGrpPortalCheck.c |
| `018a3000` | gaBGT_End | 14 | sym | stub | ga/gaInitDrawObject.c |
| `018a2f30` | gaBGT_Start | 14 | sym | stub | ga/gaInitDrawObject.c |
| `01886690` | asChangeViewMatrix_forTheBank | 12 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `0185e360` | asCheckBankID | 11 | sym | stub | game/QuatSlerp.c |
| `018a22c8` | gaSetupFogCtrlData | 11 | sym | stub | ga/gaInitDrawObject.c |
| `018d9100` | ssTrigCheckEvent | 10 | sym | stub | game/ssGrpPortalCheck.c |
| `018a22b0` | gaBGT_ExecDrawCtrlData_VIF1 | 9 | sym | stub | ga/gaInitDrawObject.c |
| `018d95e0` | ssTrigRegisterEvent | 9 | sym | stub | game/ssGrpPortalCheck.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `01858c90` | XcmdScreenFilter | 8 | sym | stub | game/QuatSlerp.c |
| `018a7db8` | gaSet3dlibStruct | 7 | sym | stub | ga/gaInit3dlib.c |
| `0185e380` | asCheckBankType | 6 | sym | stub | game/QuatSlerp.c |
| `01854560` | tnInit3dlibWork | 6 | sym | stub | game/QuatSlerp.c |
| `018a2358` | gaBGT_ExecFogCtrlData_VIF1 | 6 | sym | stub | ga/gaInitDrawObject.c |
| `018a3078` | gaObjectClippingCheck | 6 | sym | stub | ga/gaObjectClippingCheck.c |
| `018590b0` | sunBGT_LoadGSTexClutData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `01859520` | sunBGT_ExecTBufferData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `018a3150` | gaBGT_DrawObject | 6 | sym | stub | ga/gaBGT_DrawObject.c |
| `018d9560` | ssTrigRemoveEvent | 6 | sym | stub | game/ssGrpPortalCheck.c |
| `0183e200` | SunAllocInit | 6 | sym | stub | game/ahalloc.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018c23b0` | objSortWithBlurIdx | 5 | sym | stub | game/fileio.c |
| `018c2880` | objSort | 5 | sym | stub | game/fileio.c |
| `01859530` | sunSetupTBufferData | 5 | sym | stub | game/QuatSlerp.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01889cd0` | Clipping_Portal | 4 | sym | stub | game/Clipping_Portal.c |
| `01858f20` | OneMaxObjectDrawZOnly | 4 | sym | stub | game/QuatSlerp.c |
| `018a3d20` | gaBGT_DrawObject_NoClippingCheck2 | 4 | sym | stub | ga/gaBGT_DrawObject.c |
| `018a1fd8` | gaSetupDrawCtrlData | 4 | sym | stub | ga/gaInitDrawObject.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `0188df70` | tiDrawLensFlares | 3 | sym | stub | game/Clipping_Portal.c |
| `01853620` | PanicFilter | 3 | sym | stub | game/QuatSlerp.c |
| `018b4480` | SceneLightSet | 3 | sym | stub | game/Character_Init.c |
| `018a1f60` | gaBGT_ExecDBufferData_VIF1 | 3 | sym | stub | ga/gaInitDrawObject.c |
| `018a2e70` | gaBGT_ExecDrawCommandData_VIF1 | 3 | sym | stub | ga/gaInitDrawObject.c |
| `0188cdc0` | tiVibrationOff | 3 | sym | stub | game/Clipping_Portal.c |
| `018f4fc0` | asCharacterOperate | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018fb940` | asMonsterDistanceStack_inPanic | 3 | sym | stub | game/wideXcmd_DebugPrint.c |

