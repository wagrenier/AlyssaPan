# AlyssaPan status (generated)

Generated 2026-11-09 22:22 at commit `8d88e6a` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **91**; stubs: **200**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 0 | 2 |
| `src/ga/gaDma.c` | 16 | 0 |
| `src/ga/gaDmaSync_VIF1.c` | 0 | 3 |
| `src/ga/gaInit3dlib.c` | 0 | 3 |
| `src/ga/gaInitDrawObject.c` | 24 | 0 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaInitMeter.c` | 6 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 0 | 1 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/game/AlignedNum.c` | 0 | 1 |
| `src/game/ArtoolDebugOpen.c` | 0 | 1 |
| `src/game/Character_Init.c` | 0 | 5 |
| `src/game/Clipping_Portal.c` | 0 | 31 |
| `src/game/LinkMatrixTree.c` | 0 | 4 |
| `src/game/QuatSlerp.c` | 0 | 30 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 0 | 28 |
| `src/game/ahalloc.c` | 7 | 0 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 7 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 2 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/fileio.c` | 0 | 8 |
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

- 65 TUs: todo 58, verify 7, done 2
- next: **game/AlignedNum.c** (pinned in the queue)
- pinned queue: game/AlignedNum.c, game/sound.c, game/Character_Init.c, game/ArtoolDebugOpen.c, game/main2.c, game/fileio.c
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra

Recent sessions:

- 2026-11-09 22:22 `game/ahalloc.c` verify: SunAlloc block heaps: all 7 functions (init, alloc with alignment pads and splits, free with neighbour merges, border check, debug map)
- 2026-11-09 22:10 `ga/gaInitMeter.c` verify: ga/gaInitMeter.c: all 6 functions (gaInitMeter, gaStartMeter, gaEndCpuMeter, gaEndGSMeter, gaGetCpuTimer, gaGetEEcoreTimer) and its 3 data words; TU renamed fro
- 2026-11-09 21:53 `ga/gaInitDrawObject.c` verify: slice 2: the texture three -- gaSetupTBufferData (TEX0, CLAMP, MIPTBP1/2, TEXFLUSH and the upload record, GA_TEX_DESC / GA_TBUFFER_DATA), gaBGT_SetLoadGSEnv (BI
- 2026-11-09 21:42 `ga/gaInitDrawObject.c` partial: slice 1: 21 of 24 functions -- the ga packet builders (gsinit, dbuffer with its 14 values, drawctrl with GA_DRAWCTRL_ARGS fields named, fogctrl), the scratchpad
- 2026-11-09 21:11 `ga/gaVifInit.c` verify: gaVifInit, gaVUSync_VU0, gaVifcodeFlush_VIF1, gaVifcodeFlushe_VIF1, gaVIFSync_VIF0, gaVIFSync_VIF1 and the 18 vifcode qwords of .data 0x0192e640..0x0192e790 rec
- 2026-11-09 20:50 `ga/gaDma.c` verify: ga DMA wrappers: all 16 functions; host scratchpad buffer, libdma SPR transfers, D_CTRL and D_ENABLEW registers
- 2026-11-09 20:31 `sdk/libmc.cpp` done: the real memory card: io::MemoryCards (RosePan libmc as a class, folders memcard/mc<port>-<slot>/, the async sceMcSync handshake kept) behind every sceMc entry 
- 2026-11-09 20:20 `alyssapan/core/window.cpp` done: siblings shims (a): pad database, window and exe icon, mute and pause in the background, overlay DPI scale, cursor auto-hide

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/QuatSlerp.c` | todo | 0/30/177 | 50.3 | 151 | 14 |
| `game/Clipping_Portal.c` | todo | 0/31/73 | 44.8 | 113 | 12 |
| `game/ssGrpPortalCheck.c` | todo | 0/10/59 | 26.4 | 82 | 7 |
| `game/ahRelocateSunPs2ObjectArry.c` | todo | 0/28/93 | 76.0 | 82 | 9 |
| `game/wideXcmd_DebugPrint.c` | todo | 0/12/109 | 40.6 | 80 | 12 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/30/96 | 35.3 | 64 | 5 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/7/30 | 23.6 | 39 | 4 |
| `ga/gaDmaSync_VIF1.c` | todo | 0/3/5 | 0.5 | 35 | 3 |
| `game/fileio.c` | todo | 0/8/40 | 23.3 | 34 | 3 |
| `game/Character_Init.c` | todo | 0/5/49 | 30.6 | 25 | 3 |
| `game/main2.c` | todo | 0/5/18 | 6.2 | 21 | 3 |
| `game/sound.c` | todo | 0/4/69 | 15.5 | 19 | 3 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9470` | ssTrigSetVariable | 16 | sym | stub | game/ssGrpPortalCheck.c |
| `01886690` | asChangeViewMatrix_forTheBank | 12 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018a17f8` | gaBGT_DmaSendN_fromSPR_VIF1 | 12 | sym | stub | ga/gaDmaSync_VIF1.c |
| `0185e360` | asCheckBankID | 11 | sym | stub | game/QuatSlerp.c |
| `018d9100` | ssTrigCheckEvent | 10 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 9 | sym | stub | game/ssGrpPortalCheck.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `01858c90` | XcmdScreenFilter | 8 | sym | stub | game/QuatSlerp.c |
| `018a7db8` | gaSet3dlibStruct | 7 | sym | stub | ga/gaInit3dlib.c |
| `0185e380` | asCheckBankType | 6 | sym | stub | game/QuatSlerp.c |
| `01854560` | tnInit3dlibWork | 6 | sym | stub | game/QuatSlerp.c |
| `018a3078` | gaObjectClippingCheck | 6 | sym | stub | ga/gaObjectClippingCheck.c |
| `018590b0` | sunBGT_LoadGSTexClutData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `01859520` | sunBGT_ExecTBufferData_VIF1 | 6 | sym | stub | game/QuatSlerp.c |
| `018a3150` | gaBGT_DrawObject | 6 | sym | stub | ga/gaBGT_DrawObject.c |
| `018d9560` | ssTrigRemoveEvent | 6 | sym | stub | game/ssGrpPortalCheck.c |
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
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018b4480` | SceneLightSet | 3 | sym | stub | game/Character_Init.c |
| `0188df70` | tiDrawLensFlares | 3 | sym | stub | game/Clipping_Portal.c |
| `01853620` | PanicFilter | 3 | sym | stub | game/QuatSlerp.c |
| `018fb940` | asMonsterDistanceStack_inPanic | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `0188cdc0` | tiVibrationOff | 3 | sym | stub | game/Clipping_Portal.c |
| `018b49f0` | CharLightSet | 3 | sym | stub | game/Character_Init.c |
| `018f4fc0` | asCharacterOperate | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018c7ec0` | fs_error | 3 | sym | stub | game/fileio.c |
| `0185e580` | ahGetHash | 3 | sym | stub | game/QuatSlerp.c |
| `0185e150` | asFindModelPartsID | 3 | sym | stub | game/QuatSlerp.c |
| `018fe370` | asSetCharacterKeyEnable | 3 | sym | stub | game/wideXcmd_DebugPrint.c |
| `018b3480` | SelfDiscardSceneLoadInit | 2 | sym | stub | game/main2.c |

