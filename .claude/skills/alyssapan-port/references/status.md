# AlyssaPan status (generated)

Generated 2026-11-11 09:33 at commit `17c5442` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed CT3 functions: **966**; stubs: **402**

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
| `src/game/Clipping_Portal.c` | 0 | 37 |
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
| `src/game/asUseArrow.c` | 0 | 6 |
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
| `src/game/tiXcmd_TransItem.c` | 0 | 92 |
| `src/game/tkXDTrg_GetSceneHash.c` | 0 | 7 |
| `src/game/tnXcmd.c` | 0 | 8 |
| `src/game/wideXcmd_DebugPrint.c` | 107 | 1 |
| `src/game/wideXcmd_TriggerInit.c` | 0 | 44 |
| `src/game/xcmd_tn.c` | 39 | 20 |
| `src/lib/unassigned_stubs.cpp` | 0 | 1 |

## Translation units (one session each: `tus.py`)

- 65 TUs: todo 36, partial 4, verify 26, done 4, blocked 1
- next: **game/tiXcmd_TransItem.c** (pinned in the queue)
- pinned queue: game/tiXcmd_TransItem.c, game/xcmd_tn.c, alyssapan/rendering/ga_atld.cpp, alyssapan/rendering/ui_canvas.cpp, game/wideXcmd_DebugPrint.c, game/tkXDTrg_GetSceneHash.c, game/asUseArrow.c, game/asMothRouteSearch.c, game/asXcmd.c, game/ssHitCheckAttackToScn.c, game/ssSetTexUVforCubeMap.c
- partial: `alyssapan/rendering/ga_atld.cpp` 0/0 -- Shot still black for a game reason: the light0 colour is vertex colour plus the part's ALL_TABLE.partsPos (the ambient RGB), which only the event command tnXcmd
- partial: `game/wideXcmd_DebugPrint.c` 107/109 -- Next slice: asCharacterMortionFunctionOperation (0x1578, VU0 and the per-character trigger chains: split it if needed) -- the last function of the TU. New stubs
- partial: `game/xcmd_tn.c` 37/57 -- 15 commands written. Picture: frame 120 shows ATLD memory-card text at 4:3 as before; black from frame 180 because the script waits on the stub tiXcmd_XFade_Bre
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
- blocked: `alyssapan/rendering/ui_canvas.cpp` 0/0 -- Blocked on ga/gaBGT_DrawObject.c (host draw module for ga draw objects) and the camera (asEyeForcusUP2CamMat.c camera_init0 / camera_view stubs); triggers are n

Recent sessions:

- 2026-11-11 09:33 `game/xcmd_tn.c` partial: slice 2: wideXcmd_PlayAnimeToFream2 (the reached stub), clips, panic, continue, time stop, shape / behaviour / bank free, stay room, message bank, menu map, fog
- 2026-11-11 09:21 `game/ssGrpPortalCheck.c` verify: picture: ssSetAmbColorToAllObj (game/ssSetTexUVforCubeMap.c) fills every ga object's +0x38 ambient pointer; ATLD's Checking text shows at 4:3 and wide
- 2026-11-11 09:05 `game/ssGrpPortalCheck.c` partial: triggers slice 5: ssTrigExecuteActCmd (every command id and operand size, TRIG_CMD_*), the 256-slot ssXcmd_functions table from the ELF, suXcmd_AnmDirection; 23
- 2026-11-11 08:37 `game/ssGrpPortalCheck.c` partial: triggers slice 4: ssTrigCheckConditions and its checks (ssTrigCheckInput, ssTrigCheckVariable, ssTrigLookAtCheck, ssTrigAttackHitCheck), ssTrigSetEnable, Trigge
- 2026-11-11 08:08 `game/xcmd_tn.c` partial: slice 1: tnXcmd_Scene_Ambient_Change and 21 small x commands (waits, variables, room-in icons, bank and flag commands), BankUseCheck; SS_XCMD slot type
- 2026-11-11 07:55 `alyssapan/rendering/ga_atld.cpp` partial: ga textures: sunBGT_ExecTBufferData_VIF1 hands TEX0, CLAMP, level 0 and the CLUT to AlyssaPan_DrawGaTexture; TextureStore decodes CT3 GS textures at their sourc
- 2026-11-11 07:24 `game/ssGrpPortalCheck.c` partial: triggers slice 3: Trigger_main, the detector walk (ssTrigProcDetRecur, ssTrigProceedDetector, ssTrigCheckDetector and its box, plane-volume and contact shapes),
- 2026-11-11 07:05 `game/ssGrpPortalCheck.c` partial: triggers slice 2: Trigger_init, Trigger_deinit, TriggerGlobalInit, the trigger block getters and count readers, the delay-enable ring (ssTrigDEPop_Front, PushBa

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/Clipping_Portal.c` | todo | 0/37/73 | 44.8 | 163 | 18 |
| `game/ssSetTexUVforCubeMap.c` | partial | 1/44/96 | 35.3 | 96 | 8 |
| `game/CpDivideEECom.c` | todo | 0/10/18 | 3.3 | 80 | 10 |
| `ga/gaDrawObject.c` | todo | 0/11/11 | 6.9 | 39 | 0 |
| `game/tiAvoidObject.c` | todo | 0/7/82 | 30.5 | 27 | 4 |
| `game/ssHitCheckAttackToScn.c` | todo | 0/6/37 | 32.0 | 23 | 3 |
| `game/Convert.c` | todo | 0/2/2 | 0.8 | 23 | 0 |
| `game/AA_mpegTS.c` | todo | 0/12/31 | 2.6 | 17 | 0 |
| `game/AA_videoCallback.c` | todo | 0/11/11 | 1.7 | 17 | 1 |
| `game/asUseArrow.c` | todo | 0/6/11 | 6.4 | 16 | 1 |
| `game/tkXDTrg_GetSceneHash.c` | todo | 0/7/50 | 21.0 | 14 | 2 |
| `game/AA_SoundMonoSoart.c` | todo | 0/8/20 | 4.0 | 13 | 1 |

## Frontier (called by reconstructed code, not written yet)

| CT3 addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `01851b10` | ConvertQuat2Matrix | 17 | sym | stub | game/Convert.c |
| `018d1e70` | CpGetEEIOPStatus | 14 | sym | stub | game/CpDivideEECom.c |
| `0188ced0` | tiDrawShadingOff | 9 | sym | stub | game/Clipping_Portal.c |
| `01893cf0` | ExportDrawToTex | 8 | sym | stub | game/Clipping_Portal.c |
| `018a3078` | gaObjectClippingCheck | 7 | sym | stub | ga/gaObjectClippingCheck.c |
| `0188d3c0` | tiFade_Start | 6 | sym | stub | game/Clipping_Portal.c |
| `018a4fe0` | gaDrawObject_VertexLocalViewWithClippingFlagSet | 6 | sym | stub | ga/gaDrawObject.c |
| `018a51e8` | gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard | 6 | sym | stub | ga/gaDrawObject.c |
| `018a5fe0` | gaDrawObject_MakeVertexReferenceTable | 6 | sym | stub | ga/gaDrawObject.c |
| `018518d0` | ConvertMatrix2Quat | 6 | sym | stub | game/Convert.c |
| `0188cdc0` | tiVibrationOff | 6 | sym | stub | game/Clipping_Portal.c |
| `018d1fa0` | CpSifDmaTransEEToIOP | 6 | sym | stub | game/CpDivideEECom.c |
| `0188ce10` | tiVibrationOn | 5 | sym | stub | game/Clipping_Portal.c |
| `01883ca0` | asMothPause | 5 | sym | stub | game/asMothPause.c |
| `01866c10` | asShootArrowSetting | 5 | sym | stub | game/asUseArrow.c |
| `01896920` | ssGetNumBlurDrawObjForScene | 5 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018bd830` | AA_clearGsMem | 4 | sym | stub | game/AA_GetDataWord.c |
| `018a7538` | gaSearchNearLight4 | 4 | sym | stub | ga/gaSearchNearLight4.c |
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
| `018962a0` | tnChangeCharAmbientColorSet | 3 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `0188df70` | tiDrawLensFlares | 3 | sym | stub | game/Clipping_Portal.c |
| `0188cd70` | tiVibrationTimer | 3 | sym | stub | game/Clipping_Portal.c |
| `018c1ce0` | AA_videoDecAbort | 3 | sym | stub | game/AA_mpegTS.c |
| `018666c0` | asInitArrowInfo | 2 | sym | stub | game/asUseArrow.c |
| `01882c10` | asInitMothInfo | 2 | sym | stub | game/asMothRouteSearch.c |
| `018a6080` | gaDrawObject_SetBig | 2 | sym | stub | ga/gaDrawObject.c |
| `0189f720` | gaDrawObject_Redraw_Polygon | 2 | sym | stub | ga/gaDrawObject_Redraw_Polygon.c |
| `0189dec0` | ssBGT_ZBufferClear | 2 | sym | stub | game/ssSetTexUVforCubeMap.c |
| `018eda60` | SceneDoorFlagClear | 2 | sym | stub | game/tkXDTrg_GetSceneHash.c |
| `018aa950` | CheckLOS_Multi | 2 | sym | stub | game/ssHitCheckAttackToScn.c |

