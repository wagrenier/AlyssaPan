# AlyssaPan status (generated)

Generated 2026-11-10 19:50 at commit `9785850` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **813**; stubs: **177**

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
| `src/ga/gaPadInit.c` | 1 | 0 |
| `src/ga/gaSearchNearLight4.c` | 0 | 1 |
| `src/ga/gaSinCos.c` | 14 | 0 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 0 | 1 |
| `src/ga/wait.c` | 0 | 2 |
| `src/game/AA_GetDataWord.c` | 0 | 8 |
| `src/game/AA_SoundMonoSoart.c` | 0 | 8 |
| `src/game/AA_cpuCycles.c` | 0 | 1 |
| `src/game/AA_mpegTS.c` | 0 | 12 |
| `src/game/AA_videoCallback.c` | 0 | 11 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 15 | 0 |
| `src/game/Character_Init.c` | 48 | 1 |
| `src/game/Clipping_Portal.c` | 0 | 34 |
| `src/game/Convert.c` | 0 | 2 |
| `src/game/CpDivideEECom.c` | 0 | 10 |
| `src/game/IOPGetFileSize.c` | 4 | 0 |
| `src/game/LinkMatrixTree.c` | 4 | 0 |
| `src/game/QuatSlerp.c` | 189 | 0 |
| `src/game/SdMasterVolumeSetSE.c` | 61 | 0 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 93 | 0 |
| `src/game/ahalloc.c` | 7 | 0 |
| `src/game/asEyeForcusUP2CamMat.c` | 0 | 9 |
| `src/game/asMothOperate.c` | 0 | 1 |
| `src/game/asMothPause.c` | 0 | 1 |
| `src/game/asMothRouteSearch.c` | 0 | 2 |
| `src/game/asUseArrow.c` | 0 | 6 |
| `src/game/asXcmd.c` | 0 | 1 |
| `src/game/copy2area.c` | 0 | 1 |
| `src/game/fileio.c` | 40 | 0 |
| `src/game/hhXcmd_DressUp.c` | 0 | 1 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 18 | 0 |
| `src/game/movie.c` | 20 | 0 |
| `src/game/sound.c` | 73 | 0 |
| `src/game/ssCalcInversMatrix2.c` | 0 | 1 |
| `src/game/ssGrpPortalCheck.c` | 0 | 11 |
| `src/game/ssHitCheckAttackToScn.c` | 0 | 5 |
| `src/game/ssSetTexUVforCubeMap.c` | 0 | 31 |
| `src/game/tiAvoidObject.c` | 0 | 7 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 3 |
| `src/game/wideXcmd_DebugPrint.c` | 107 | 1 |
| `src/game/wideXcmd_TriggerInit.c` | 0 | 1 |
| `src/lib/unassigned_stubs.cpp` | 0 | 1 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 41, partial 1, verify 23, done 3
- next: **game/wideXcmd_DebugPrint.c** (pinned in the queue)
- pinned queue: game/wideXcmd_DebugPrint.c, game/tkXDTrg_GetSceneHash.c, game/asUseArrow.c, game/asMothRouteSearch.c, game/asXcmd.c, game/asEyeForcusUP2CamMat.c, game/ssHitCheckAttackToScn.c, game/ssSetTexUVforCubeMap.c, game/ssGrpPortalCheck.c
- partial: `game/wideXcmd_DebugPrint.c` 107/109 -- Next slice: asCharacterMortionFunctionOperation (0x1578, VU0 and the per-character trigger chains: split it if needed) -- the last function of the TU. New stubs
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
- verify: `game/fileio.c` 40/40 -- All 40 functions written. Round2ChangeChar keeps the ROM order of its 150 Round2ChangeSectInfo pairs as three tables; round values named from the file names onl
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/main2.c` 18/18 -- Smoke run leaves main2 and runs the frame loop with no crash; the shot stays black because FSMltLoad (fileio.c) is a stub so no bank loads. PORT BankReady skips
- verify: `game/movie.c` 20/21 -- Playback kept in the ROM shape: host EE kernel runs the decoder thread and vblank handler, sdk/libmpeg decodes, the picture goes through AA_GetDataWord.c displa
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo

Recent sessions:

- 2026-11-10 19:50 `game/wideXcmd_DebugPrint.c` partial: slice 9: the ground key-table entries -- standing, walking, running and the run stop (iRunCount), the move end, crouching and crawling (KEY_MAP_HIDE is the crou
- 2026-11-10 19:35 `game/wideXcmd_DebugPrint.c` partial: slice 8: the cat walk (fall, hidden feet part, death drop), the lever-mash locks (asPIA1_KEY_TBL_LOCK), panic run / stand (confirms the panic event answers), hi
- 2026-11-10 19:18 `game/wideXcmd_DebugPrint.c` partial: slice 7: the pad move (asCharacterMoveByPad: tank and camera-relative steering, the turn-around flag PIA_KEY.h0c; asCharacterMoveByPadUD), the panic events (asP
- 2026-11-10 18:56 `game/wideXcmd_DebugPrint.c` partial: slice 6: the player key dispatch (asPlayerInputAnalyze with its typed tables asPIA0 / asPIA1 / asPIAK, every key-table entry now (CHARACTER *, PIA_KEY *)), 19 s
- 2026-11-10 18:34 `game/wideXcmd_DebugPrint.c` partial: slice 5: asCheckCharacterHited (the boss battle invulnerability vsBossMuteki / AfterPanicStartMuteki, the stage bosses scripted strikes, then Alyssa's hit table
- 2026-11-10 18:13 `game/wideXcmd_DebugPrint.c` partial: slice 4: asCharacterAnimationExecute (the per-frame motion play, what follows a motion's end, root motion), asChangeAlyssaMortionByDamaging, asPIA1_KEY_TBL_HAIH
- 2026-11-10 17:56 `game/wideXcmd_DebugPrint.c` partial: slice 3: asRequestExecute (the motion queue consumer, body and blend slots), asResetCharacterPosition, asCharacterForceMoveOperate, asCatWalkEventOperation (4 f
- 2026-11-10 17:38 `game/wideXcmd_DebugPrint.c` partial: slice 2: positioning (asCharacterPositioning, as1CharacterPositioning, OnlyDeltaPos, the pause review, asResetCharacterRotation), the panic / barrier / lock sta

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/ssGrpPortalCheck.c` | todo | 0/11/59 | 26.4 | 480 | 10 |
| `game/Clipping_Portal.c` | todo | 0/34/73 | 44.8 | 160 | 18 |
| `game/ssSetTexUVforCubeMap.c` | todo | 0/31/96 | 35.3 | 86 | 9 |
| `game/CpDivideEECom.c` | todo | 0/10/18 | 3.3 | 80 | 10 |
| `game/asEyeForcusUP2CamMat.c` | todo | 0/9/30 | 23.6 | 57 | 6 |
| `game/tiAvoidObject.c` | todo | 0/7/82 | 30.5 | 27 | 4 |
| `game/Convert.c` | todo | 0/2/2 | 0.8 | 23 | 0 |
| `game/ssHitCheckAttackToScn.c` | todo | 0/5/37 | 32.0 | 22 | 3 |
| `ga/gaBGT_DrawObject.c` | todo | 0/2/9 | 7.6 | 18 | 1 |
| `game/AA_mpegTS.c` | todo | 0/12/31 | 2.6 | 17 | 0 |
| `game/AA_videoCallback.c` | todo | 0/11/11 | 1.7 | 17 | 1 |
| `game/asUseArrow.c` | todo | 0/6/11 | 6.4 | 16 | 1 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `018d9100` | ssTrigCheckEvent | 171 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9560` | ssTrigRemoveEvent | 110 | sym | stub | game/ssGrpPortalCheck.c |
| `018d95e0` | ssTrigRegisterEvent | 62 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9470` | ssTrigSetVariable | 43 | sym | stub | game/ssGrpPortalCheck.c |
| `018d9430` | ssTrigGetVariable | 38 | sym | stub | game/ssGrpPortalCheck.c |
| `01851b10` | ConvertQuat2Matrix | 17 | sym | stub | game/Convert.c |
| `018d1e70` | CpGetEEIOPStatus | 14 | sym | stub | game/CpDivideEECom.c |
| `01886690` | asChangeViewMatrix_forTheBank | 12 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `018a3150` | gaBGT_DrawObject | 8 | sym | stub | ga/gaBGT_DrawObject.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `018a3078` | gaObjectClippingCheck | 7 | sym | stub | ga/gaObjectClippingCheck.c |
| `0188d3c0` | tiFade_Start | 6 | sym | stub | game/Clipping_Portal.c |
| `018518d0` | ConvertMatrix2Quat | 6 | sym | stub | game/Convert.c |
| `0188cdc0` | tiVibrationOff | 6 | sym | stub | game/Clipping_Portal.c |
| `018d1fa0` | CpSifDmaTransEEToIOP | 6 | sym | stub | game/CpDivideEECom.c |
| `0188ce10` | tiVibrationOn | 5 | sym | stub | game/Clipping_Portal.c |
| `01866c10` | asShootArrowSetting | 5 | sym | stub | game/asUseArrow.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018a3d20` | gaBGT_DrawObject_NoClippingCheck2 | 5 | sym | stub | ga/gaBGT_DrawObject.c |
| `018bd830` | AA_clearGsMem | 4 | sym | stub | game/AA_GetDataWord.c |
| `01883ca0` | asMothPause | 4 | sym | stub | game/asMothPause.c |
| `018a7538` | gaSearchNearLight4 | 4 | sym | stub | ga/gaSearchNearLight4.c |
| `01896a90` | ssPartMotionBlurInsertObjToDrawTable | 4 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `01889cd0` | Clipping_Portal | 4 | sym | stub | game/Clipping_Portal.c |
| `018a0b40` | gaPadRead | 4 | sym | stub | ga/pad_button_read.c |
| `018db340` | asXcmd_AddForLockToTargChara | 3 | sym | stub | game/asXcmd.c |
| `018870c0` | camera_init0 | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `01886dc0` | CamChange_S | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `018881a0` | camera_main | 3 | sym | stub | game/asEyeForcusUP2CamMat.c |
| `0188df70` | tiDrawLensFlares | 3 | sym | stub | game/Clipping_Portal.c |
| `0188cd70` | tiVibrationTimer | 3 | sym | stub | game/Clipping_Portal.c |
| `018a8690` | gaDMAStopMessage_VIF1 | 3 | sym | stub | ga/wait.c |
| `018c1ce0` | AA_videoDecAbort | 3 | sym | stub | game/AA_mpegTS.c |
| `018666c0` | asInitArrowInfo | 2 | sym | stub | game/asUseArrow.c |
| `01882c10` | asInitMothInfo | 2 | sym | stub | game/asMothRouteSearch.c |
| `0189dec0` | ssBGT_ZBufferClear | 2 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018eda60` | SceneDoorFlagClear | 2 | sym | stub | game/tkXDTrg_GetSceneHash.c |
| `018aa950` | CheckLOS_Multi | 2 | sym | stub | game/ssHitCheckAttackToScn.c |
| `018ab180` | CheckLOS | 2 | sym | stub | game/ssHitCheckAttackToScn.c |

