# AlyssaPan status (generated)

Generated 2026-11-13 03:41 at commit `d74eea0` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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
| `01850000` | 200/200 (100%) | game/BRANCH_CLEAR.c, game/tiAvoidObject.c, game/ahRelocateSunPs2ObjectArry.c |
| `01860000` | 113/113 (100%) | game/tiAvoidObject.c, game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/asUseArrow.c |
| `01870000` | 82/82 (100%) | game/tiBrain_ChgTrace_BOSS_BALOWS.c, game/tiBrain_ChgTrace_Ghost.c, game/ssBrain_ChgHideAtk1.c |
| `01880000` | 103/103 (100%) | game/Clipping_Portal.c, game/asEyeForcusUP2CamMat.c, game/ssBrain_ChgHideAtk1.c |

## Reconstruction (src/)

- reconstructed CT3 functions: **1851**; stubs: **14**

| file | reconstructed | stubs |
|---|---|---|
| `src/ga/gaBGT_DrawObject.c` | 9 | 0 |
| `src/ga/gaDma.c` | 16 | 0 |
| `src/ga/gaDmaSync_VIF1.c` | 6 | 0 |
| `src/ga/gaDrawObject.c` | 13 | 0 |
| `src/ga/gaDrawObject_Redraw_Polygon.c` | 12 | 0 |
| `src/ga/gaInit3dlib.c` | 25 | 0 |
| `src/ga/gaInitDrawObject.c` | 24 | 0 |
| `src/ga/gaInitInterrupt.c` | 3 | 0 |
| `src/ga/gaInitMeter.c` | 6 | 0 |
| `src/ga/gaObjectClippingCheck.c` | 1 | 0 |
| `src/ga/gaSearchNearLight4.c` | 1 | 0 |
| `src/ga/gaSinCos.c` | 14 | 0 |
| `src/ga/gaVifInit.c` | 6 | 0 |
| `src/ga/pad_button_read.c` | 4 | 0 |
| `src/ga/wait.c` | 4 | 0 |
| `src/game/AA_GetDataWord.c` | 16 | 0 |
| `src/game/AA_SoundMonoSoart.c` | 20 | 0 |
| `src/game/AA_cpuCycles.c` | 4 | 0 |
| `src/game/AA_mpegTS.c` | 31 | 0 |
| `src/game/AA_scTag2.c` | 7 | 0 |
| `src/game/AA_viBuf.c` | 13 | 0 |
| `src/game/AA_videoCallback.c` | 11 | 0 |
| `src/game/AlignedNum.c` | 4 | 0 |
| `src/game/ArtoolDebugOpen.c` | 15 | 0 |
| `src/game/BRANCH_CLEAR.c` | 184 | 0 |
| `src/game/Character_Init.c` | 49 | 0 |
| `src/game/Clipping_Portal.c` | 73 | 0 |
| `src/game/Convert.c` | 3 | 0 |
| `src/game/CpDivideEECom.c` | 18 | 0 |
| `src/game/IOPGetFileSize.c` | 4 | 0 |
| `src/game/LinkMatrixTree.c` | 4 | 0 |
| `src/game/PicPaz.c` | 9 | 0 |
| `src/game/QuatSlerp.c` | 5 | 0 |
| `src/game/RotateCamera.c` | 2 | 0 |
| `src/game/SdMasterVolumeSetSE.c` | 61 | 0 |
| `src/game/ahRelocateSunPs2ObjectArry.c` | 93 | 0 |
| `src/game/ahalloc.c` | 7 | 0 |
| `src/game/asEyeForcusUP2CamMat.c` | 30 | 0 |
| `src/game/asMothOperate.c` | 1 | 0 |
| `src/game/asMothPause.c` | 1 | 0 |
| `src/game/asMothRouteSearch.c` | 3 | 0 |
| `src/game/asUseArrow.c` | 11 | 0 |
| `src/game/asXcmd.c` | 21 | 0 |
| `src/game/copy2area.c` | 2 | 0 |
| `src/game/fileio.c` | 40 | 0 |
| `src/game/hhXcmd_BGM_SE_VOLUME.c` | 1 | 0 |
| `src/game/hhXcmd_DressUp.c` | 26 | 0 |
| `src/game/main.c` | 29 | 0 |
| `src/game/main2.c` | 18 | 0 |
| `src/game/movie.c` | 20 | 0 |
| `src/game/sound.c` | 73 | 0 |
| `src/game/ssBrain_ChgHideAtk1.c` | 34 | 0 |
| `src/game/ssCalcInversMatrix2.c` | 5 | 0 |
| `src/game/ssGrpPortalCheck.c` | 74 | 0 |
| `src/game/ssHitCheckAttackToScn.c` | 37 | 0 |
| `src/game/ssSetTexUVforCubeMap.c` | 96 | 0 |
| `src/game/tiAvoidObject.c` | 95 | 0 |
| `src/game/tiBrain_ChgTrace_BOSS_BALOWS.c` | 56 | 12 |
| `src/game/tiBrain_ChgTrace_Ghost.c` | 31 | 0 |
| `src/game/tiXcmd_TransItem.c` | 93 | 0 |
| `src/game/tkXDTrg_GetSceneHash.c` | 50 | 0 |
| `src/game/tnXcmd.c` | 8 | 0 |
| `src/game/wideXcmd_DebugPrint.c` | 109 | 0 |
| `src/game/wideXcmd_TriggerInit.c` | 51 | 0 |
| `src/game/xcmd_tn.c` | 59 | 0 |

## Translation units (one session each: `tus.py`)

- 65 TUs: partial 1, verify 66, done 14, blocked 1
- next: **game/tiBrain_ChgTrace_BOSS_BALOWS.c** (pinned in the queue)
- pinned queue: game/tiBrain_ChgTrace_BOSS_BALOWS.c, alyssapan/rendering/ui_canvas.cpp
- partial: `game/tiBrain_ChgTrace_BOSS_BALOWS.c` 54/69 -- Left: the nine other tiSetMode setters (stubs from stubgen), Init_BOSS1, Init_BOSS_SCISSORS_W, Damage_BOSS1. Smoke run unchanged (grey at the end); no run reach
- verify: `alyssapan/core/movie_skip.cpp` 0/0 -- AlyssaPan_SkipMovies seam (src/alyssapan/core/movie_skip.h/.cpp) reads the config's skip_intro (ALYSSAPAN_SKIP_INTRO; off for players); AA_movie takes the PORT 
- verify: `ga/gaBGT_DrawObject.c` 9/9 -- Game code complete. Host draw-module work left (open-items item 4 slice 4): texture, depth, fog, frame end, CT3 window origin in UiCanvas, light1 lighting, bill
- verify: `ga/gaDma.c` 16/16 -- Complete code, nothing on screen shows it yet (shot black, as before). The queue was empty: refilled in smoke-run stub order (gaVifInit, gaInitDrawObject, gaIni
- verify: `ga/gaDmaSync_VIF1.c` 5/5 -- Host: kicks leave CHCR.STR clear (the transfer is over), so the syncs return at once; the BGT send keeps the copy and the flip, the packet goes nowhere (draw mo
- verify: `ga/gaDrawObject.c` 11/11 -- All 11 functions written. Not reached by the smoke run (gaObjectClippingCheck stub; title scene draws through nc2): title scene unchanged at 4:3. Open: the bill
- verify: `ga/gaDrawObject_Redraw_Polygon.c` 6/6 -- Probe on the NEW path: 2 divided triangles -> 7 emitted, all inside the 512x448 screen (clipping right). The packet is built ROM-exact but not sent (kv: the hos
- verify: `ga/gaInit3dlib.c` 25/25 -- 15 s smoke run: no fault, shot black (no host draw module). gaMulMatrix is dst = b * a (hazards.md). gaSetZRollMatrix now reaches gaSinCos: ga/gaSinCos.c is que
- verify: `ga/gaInitDrawObject.c` 24/24 -- No ROM caller for the texture three (the game uses its sun* copies in QuatSlerp.c): complete, nothing on screen can show them. Shot black as before: packets go 
- verify: `ga/gaInitInterrupt.c` 3/3 -- GCC object; wait_vsync_state (data 0x0192e790), ga_main_timer, ga_down_timer defined here. gaWaitVSync takes no argument and returns the CSR FIELD bit; main.c c
- verify: `ga/gaInitMeter.c` 6/6 -- Complete code, nothing on screen shows it (shot black as before; the meter stubs no longer appear in the run log). Host: T1/T0 mode writes now reach the host ti
- verify: `ga/gaObjectClippingCheck.c` 1/1 -- Title scene unchanged at 4:3 and 16:9. Doubts in open-items: the 4:3 side planes must widen with the 3D pass; MAC flag timing; BankOfHostPtr for objects outside
- verify: `ga/gaSearchNearLight4.c` 1/1 -- w is the inverse range; a light with ftoi0(w*w) low 16 bits nonzero (any w >= 1, so w = 1.0) is never chosen, as in the ROM. PORT: lightCount 0 answers none (RO
- verify: `ga/gaSinCos.c` 13/13 -- 15 s smoke run: no fault, about 26700 frames, shot black (no host draw module yet). ga_sincos_table.inc generated from the ELF (the ROM floats differ from sinf)
- verify: `ga/gaVifInit.c` 6/6 -- GCC object. VIF0/VIF1 STAT are host variables that read idle (sdk/eeregs.h), so the polls return at once and main frame loop passes the FLUSH wait. gaDmaSendN_V
- verify: `ga/pad_button_read.c` 4/4 -- Complete; callcheck clean. 15 s smoke: flat grey as before (attract cycle). 40 s run with scripted START at reads 500..1100: no TITL in the log, but the shot is
- verify: `ga/wait.c` 4/20 -- Complete code, nothing on screen shows it: the release build compiled out every body but wait, so a DMA timeout says nothing in the ROM either. The dumps have n
- verify: `game/AA_GetDataWord.c` 15/15 -- GIF chains are host AA_IMAGE_TAG records drawn at the vblank send (DrawUiMovie picture, captions as a 4-bit TIM2 with the CLUT dt, fade as FrameScale plus addit
- verify: `game/AA_SoundMonoSoart.c` 20/20 -- verify: smoke runs skip movies, so no audio path runs; the 30 s run still reaches the title screen. callcheck lists fptodp / dpmul / __fixdfdi missing: the soft
- verify: `game/AA_cpuCycles.c` 4/4 -- Complete code, nothing on screen shows it: smoke runs skip the movies (skip_intro), so the clock never runs; the shot is the same as the runs before (memory car
- verify: `game/AA_mpegTS.c` 31/31 -- verify: no smoke run plays a movie (shot unchanged: the clock-tower scene). VideoDec / ViBuf / VoBuf are layout host; movie bank EE addresses resolved by AA_Mov
- verify: `game/AA_scTag2.c` 7/7 -- Smoke runs skip the movies, so nothing on screen shows this code; the shot is the same clock-tower scene. The ring can now retire blocks (open-items ht, hu done
- verify: `game/AA_viBuf.c` 13/13 -- ezmpegstr vibuf.c, all 13 written. Host: no IPU; ViBufHostIpu moves D4 MADR to what MpegStream's decoder has parsed (AlyssaPanMpegVideoPending), DMA addresses i
- verify: `game/AA_videoCallback.c` 11/11 -- verify: smoke runs skip the movies, so no shot shows the stream; the 25 s run still reaches the title scene. AA_videoDecPutTs now answers int (the ROM tests it)
- verify: `game/AlignedNum.c` 4/4 -- WaitLoop is split in Ghidra: WaitLoop 0x0189f650 plus WaitLoop_body 0x0189f680 (game-inferred), one C function. The j at 0x0189f718 belongs to the next object (
- verify: `game/ArtoolDebugOpen.c` 15/15 -- Smoke run: FlInit prints no error, so the pack directory loads and sceCdSearchFile finds the pack; no run reads a file yet (main2 and fileio.c are stubs), the s
- verify: `game/BRANCH_CLEAR.c` 173/173 -- MotionTableOf now resolves CHARACTER +0x350 via BankEePtr of the bank, as MotionRecord in wideXcmd_DebugPrint.c does (open-items ga done). 15 s smoke run: no fa
- verify: `game/Character_Init.c` 49/49 -- All 49 functions written; the title smoke run does not reach Character_Init (the card check screen draws), junc_heap's NEW path to room 0A02 does: look there fo
- verify: `game/Clipping_Portal.c` 73/73 -- Smoke run unchanged (content warning screen at 4:3); no pendulum is on its path, so verify. callcheck lists the two entry points calls under the static HurikoSw
- verify: `game/Convert.c` 2/2 -- No smoke run reaches either yet (the shot is the same clock-tower scene); the animation TUs (ExecuteAnimation, EA_100, asMakeStraw) will exercise them. Order of
- verify: `game/CpDivideEECom.c` 18/18 -- Complete code nothing on screen shows: SNDDRV.IRX is not reconstructed, so sif.cpp answers zero replies and gAdrIOPStat stays 0 (the status DMA is skipped). The
- verify: `game/IOPGetFileSize.c` 4/4 -- All 4 functions written. No host server answers FSYS_IOP SUNA yet (sif.cpp answers zeros), so a loose file's size reads 0: a host FSYS_IOP server on io::Disc is
- verify: `game/LinkMatrixTree.c` 4/4 -- Code complete, nothing on screen shows it yet: the smoke run still crashes at main.c 2106 (ActressMP NULL, the known blocker) and the shot stays black. LinkMatr
- verify: `game/PicPaz.c` 9/10 -- The smoke run passes aaXcmd_MovieFedeOutCancel: both title movies start and are skipped; the shot stays flat grey since TITL draws nothing yet (title_trace.cpp 
- verify: `game/QuatSlerp.c` 4/4 -- Evidence (architecture.md The TU map, The split of QuatSlerp.c; tools seams.py, databands.py): the range's 75 locals are symtab 1088..1162, one hash-ordered run
- verify: `game/RotateCamera.c` 2/2 -- Smoke run unchanged (content warning at frame 11040); no run reaches these commands. ROM quirk kept: the leave action is checked with ssTrigCheckInput mode 1, w
- verify: `game/SdMasterVolumeSetSE.c` 61/61 -- All 61 functions written. callcheck MISSING on SdLoadSQProc is its step helpers (SdLoadSQOpen/Read/Send/Command), as for SdLoadSEProc. ROM slips kept: step 6 pr
- verify: `game/ahRelocateSunPs2ObjectArry.c` 93/93 -- All 93 functions written; nothing reaches the general body yet (the two written callers pass one flag each; asCharacterAnimationExecute and asResetCharacterPosi
- verify: `game/ahalloc.c` 7/7 -- Host block header is 0x20 bytes (EE 0x10). SunAllocInit runs at boot; nothing calls _SunAlloc yet. AlignedNum is a stub answering 0, so game/AlignedNum.c is que
- verify: `game/asEyeForcusUP2CamMat.c` 30/30 -- The smoke run calls camera_main with CAMERA_MODE_SCENE each frame; the debug modes and the cut step are not reached. Shot still black: the first object lands at
- verify: `game/asMothOperate.c` 1/1 -- No moth is armed on the NEW path, so nothing past the early return ran (verify). Queue refilled in smoke-run order: asMothOperate, gaSearchNearLight4, ssCalcInv
- verify: `game/asMothPause.c` 1/1 -- Complete; callcheck clean. No moths are armed at the title so nothing can show it: verify. Queue refilled from a probe run pressing START: asMothPause was the o
- verify: `game/asMothRouteSearch.c` 3/3 -- The route search runs only from asMothOperate (a stub); MOTH is layout host with most fields still wNN; open-items has the doubts (stage and endTicks, the twice
- verify: `game/asUseArrow.c` 11/11 -- No arrow on screen until a stage with the bow runs. States renamed by use (4 CHARGE, 5 FLYING, 7 RETURN; ARROW_HIT_STUCK and _END). HIT_TABLE +0x2a0/+0x2a4 atta
- verify: `game/asXcmd.c` 20/20 -- Smoke run unchanged (the content-warning screen; no scene script runs yet). PanicSet source 8 now adds to PANIC_INFO.gauge (the ROM indexes rise[8]); AttackTarg
- verify: `game/copy2area.c` 2/2 -- Movies are skipped in smoke runs, so the PCM path is not reached; the title scene renders unchanged. Verify when a movie plays with sound: the 4-byte PCM packet
- verify: `game/fileio.c` 40/40 -- All 40 functions written. Round2ChangeChar keeps the ROM order of its 150 Round2ChangeSectInfo pairs as three tables; round values named from the file names onl
- verify: `game/hhXcmd_BGM_SE_VOLUME.c` 1/1 -- Not reached by the 15 s smoke run (memory card check, then the content warning near frame 11880, as before); no run opens the options. Defaults named SOUND_BGM_
- verify: `game/hhXcmd_DressUp.c` 22/22 -- A smoke run now passes hhXcmd_LoadSE and stops in the m_capcom.mov decode loop (shots end at frame 420, flat grey): the movie skip host task is the next blocker
- verify: `game/main.c` 29/29 -- Smoke run: build green, layoutcheck OK, callcheck: main_rtn_Particle2 matches the ROM (extra BankEePtr only); AllDrawObject / GlobalDrawObject / DrawObjectRefra
- verify: `game/main2.c` 18/18 -- Smoke run leaves main2 and runs the frame loop with no crash; the shot stays black because FSMltLoad (fileio.c) is a stub so no bank loads. PORT BankReady skips
- verify: `game/movie.c` 20/21 -- Playback kept in the ROM shape: host EE kernel runs the decoder thread and vblank handler, sdk/libmpeg decodes, the picture goes through AA_GetDataWord.c displa
- verify: `game/sound.c` 69/69 -- All 69 written. SSLoadSEfromLoadPoint takes the bank image (BankInfo +0x1c); All_table_TBL[bank] is image + 0x80, so the old +0xd4 record is ALL_TABLE.sound, wo
- verify: `game/ssBrain_ChgHideAtk1.c` 34/34 -- Approach1 is the Trace1 walk toward the player room with a play rate by distance (800); Search1 adds the look-around at a search pole and the hidden wait BRAIN_
- verify: `game/ssCalcInversMatrix2.c` 5/5 -- No smoke run reaches the attack collision yet (the title scene draws as before). HIT_DATA gains material (+0x19b) and hp (+0x19e), HIT_OBJ damage (+0xde), SS_OB
- verify: `game/ssGrpPortalCheck.c` 59/59 -- Code complete. The black screen was the ga objects' +0x38 word (EE address of the part's partsPos, the ambient light0 adds) left 0 by the ssSetAmbColorToAllObj 
- verify: `game/ssHitCheckAttackToScn.c` 37/37 -- All 37 functions written; no smoke run reaches ssCollisionCheck yet (no character before E_00), the title shot is unchanged. CHARACTER.w300[0..3] are EE words o
- verify: `game/ssSetTexUVforCubeMap.c` 96/96 -- Slice 8, the last: every draw buffer and draw control argument read from the disassembly; statics drawctrl_ssSprite2, passTicks 274, dy 243 as the symtab has th
- verify: `game/tiAvoidObject.c` 82/82 -- All 82 functions written; the AI does not run at the title, so verify. Open: open-items.md tiAvoidObject.c slice 6 (x) pole flags 0x04 0x08 0x80 and brain words
- verify: `game/tiBrain_ChgTrace_Ghost.c` 31/31 -- Code complete, 0 stubs. Not reached by the smoke run (boot screens and content warning as before): verify once a stage with a ghost runs. Doubts gh11..gh14 in o
- verify: `game/tiXcmd_TransItem.c` 93/93 -- All 93 functions written. The attack record is a host struct SPATTACK_SCISSORS (tiXcmd_TransItem.h); tiFreeBossData frees its lists first as the ROM. Phase bits
- verify: `game/tkXDTrg_GetSceneHash.c` 50/50 -- Callcheck MISSING asFindModelPartsID in Ini is the DoorFindPart helper. The part names settle h48 h4c and the kind entries' +4 as SE / TDS parts (open-items.md 
- verify: `game/tnXcmd.c` 8/8 -- Queue refill: only the stale blocked ui_canvas.cpp was queued and nothing on the frontier, so the smallest script-reached todo TU was taken. No smoke run reache
- verify: `game/wideXcmd_DebugPrint.c` 109/109 -- All 109 functions written. Nothing on screen reaches it yet (the shot is the warning screen): verify in gameplay. New stubs ssHitAddAttackRequest, ssHit_GetMaxI
- verify: `game/wideXcmd_TriggerInit.c` 46/46 -- TU code complete, verify: no smoke run reaches the test x command. open-items 1 am decided (Claude): CHARACTER.brain is a handle (slot+1) into CharaBrainHost vi
- verify: `game/xcmd_tn.c` 57/57 -- All 57 functions written. VistaVision: ON or on brings the bars in, argv[1] 1 slides, a fourth argument makes ON skippable; statics VistaVisionEndCheck, VistaVi
- verify: `sdk/libgraph.cpp` 0/0 -- Movies are skipped in smoke runs, so nothing shows it: verify when a movie plays (movie.c reset reads only disp DISPLAY). Probe findings in open-items.md item 8
- blocked: `alyssapan/rendering/ui_canvas.cpp` 0/0 -- Blocked on ga/gaBGT_DrawObject.c (host draw module for ga draw objects) and the camera (asEyeForcusUP2CamMat.c camera_init0 / camera_view stubs); triggers are n

Recent sessions:

- 2026-11-13 03:41 `game/tiBrain_ChgTrace_BOSS_BALOWS.c` partial: slice 5: Attack_BOSS_VITRIOL, Attack_BOSS_HAMM (grab, rattle, bite, strikes), Approach_BOSS1 (submode end tests, the setter and picker dispatch, shout, side-ste
- 2026-11-13 03:19 `game/tiBrain_ChgTrace_BOSS_BALOWS.c` partial: slice 4: SCISSORS_W attack (strike, vanish, warp, reappear, special), AXE trace and attack, VITRIOL trace (roar and sprays), tiDefenseMode_AXE, the six tiSelSPM
- 2026-11-13 02:55 `game/tiBrain_ChgTrace_BOSS_BALOWS.c` partial: slice 3: BALOWS and SCISSORS_M attack bodies (the grab and its rattle, the blood throw, the swing, charge, stamp, down hit; the SCISSORS_M slash combos, thrust 
- 2026-11-13 02:35 `game/tiBrain_ChgTrace_BOSS_BALOWS.c` partial: slice 2: the BALOWS and SCISSORS_M special traces, tiCalcHPRate / ChargeTime / PlayerHitPoint / BalowsAttackRate, the two defense modes, tiSelSPMotion_BALOWS / 
- 2026-11-13 02:16 `game/tiBrain_ChgTrace_BOSS_BALOWS.c` partial: slice 1: the 18 boss mode changes (Chg Trace / Attack / Approach for BALOWS, SCISSORS_M / W, AXE, VITRIOL, HAMM; ChgReturn_BOSS1), Trace_BOSS_HAMM, Return_BOSS1
- 2026-11-13 01:50 `game/hhXcmd_BGM_SE_VOLUME.c` verify: hhXcmd_BGM_SE_VOLUME written: the option menu script's sound command (SET copies trigger event 0xa8 and variables 0x4e/0x4f to the sound buffers, volumes cut to
- 2026-11-13 01:30 `game/RotateCamera.c` verify: RotateCamera and LockerCamera, the two x commands that turn a bank's parts (bearing to Alyssa; the stick look-around with zoom)
- 2026-11-13 01:12 `game/tnXcmd.c` verify: all 8 tn x commands: the lever struggle, the player and monster control locks, PlayerDamage, Probability

## Frontier (called by reconstructed code, not written yet)

- empty: nothing reconstructed calls an unwritten function yet. Start from `main` (crt0 `_start` calls it) or the queue in `references/open-items.md`.

