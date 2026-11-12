# Open items -- priorities, decisions, cross-TU questions, done log

The user's priorities first ("Next up"), then decisions with their date and who made them, then open
questions. Resolve items here when they are settled (move them to "Done" with the commit).

## Next up (in order)

0. **Bootstrap (bootstrap.md), state on 2026-11-09:**
   - ~~1. The game~~ done: Clock Tower 3, USA, `SLUS_206.33`; CodeWarrior C with a full symtab
     (architecture.md "Binary and toolchain").
   - ~~2. Ghidra project~~ done by the user: `SLUS_206.33` in `CLaude Decomp` beside FS2 (ghidra.md).
   - ~~3. Tools~~ carried over from YamiPan, renamed (`alyssapan.py`, tag `CT3`, work files `*_ct3.json`);
     FS2's harnesses (`*cmp.py`), script / pack readers, `rtti.py`, `iop.py`, `rxcmp.py` left behind.
     **`refresh.py` runs green on CT3 (2026-11-09):** 2,628 functions, 13,643 calls, 520 tail jumps; 1,814
     game functions (1,807 named), 814 library; `alyssapan_functions.txt` and `status.md` generated.
     The string gap (0 string references) is fixed (2026-11-09): `elf32.py` swapped in layout.json's
     regions only for an *unnamed* image section, and CT3's is named `main`; now any image without a
     standard `.`-named section gets them. `features.py`: 1,200 string refs, 1,285 float consts, 13,583
     calls, 172 tail jumps (the old counts read data as code past 0x018fe500).
   - ~~4. Names~~ done (2026-11-09, scheduled run, `tools/reverse/symtab.py`): `data/tags.json` and
     Ghidra's function tags carry `game-symbol` on all 2,621 symtab-named functions (2,613 FUNC addresses +
     8 asm entry points named by NOTYPE labels); no FUNC symbol lacks a Ghidra function; all 2,657 data /
     code-label symbols (996 `@<n>`, 374 `name$<n>`, 1,287 named) are labels in Ghidra (the loader had
     applied 2,650; 7 added); `tiXcmd___________Free0` (0x018e20b0) restored from Ghidra's demangling.
     Left for the TU sessions that reach them: the 7 Ghidra functions with no symbol (architecture.md
     "Naming sources"), named `game-inferred` when read.
   - ~~5. The TU map~~ done (2026-11-09, scheduled run, `tools/reverse/tumap.py`): 66 objects (51 `game/`,
     15 `ga/`), 64 written to `data/tus.json` beside `game/main.c` (architecture.md "The TU map" has the
     method and evidence). Doubts for the TU sessions, not blockers: (a) 1,298 of 1,814 functions reference
     no local, so the larger objects (`QuatSlerp.c` 177 functions, `wideXcmd_DebugPrint.c` 109,
     `tiXcmd_TransItem.c` 93, `ssSetTexUVforCubeMap.c` 96, `ahRelocateSunPs2ObjectArry.c` 93) are probably
     several objects each: the session that claims one pins its real extent with `tus.py define` and
     defines the rest; (b) the keys are names taken from the functions, except the five source files: a
     session may rename its TU's file when it learns the real name; (c) main's locals fall in two chunks
     (60, 61): main.c stays one TU (0x01834440..0x0183aed8) until its session says otherwise.
     **Bootstrap done**: the next run takes the queue (`game/main.c` first).
   - ~~6. Platform layer~~ YamiPan's Pan layer carried over (cpp-port.md); the siblings' shims still to
     come are item 2 below.
   - ~~7. First build~~ green (2026-11-09): `build.py --run 15` opens the window on D3D12, logs
     `[stub] main (CT3 0x01839d10) reached`, writes shots (black: nothing draws yet). `io::Disc` mounts
     only the root's 2 files (the path table is root-only: item 3). Then the queue.

**~~TOP PRIORITY (the user, 2026-11-11): split `game/QuatSlerp.c`~~ done 2026-11-11 (scheduled run):
two objects, not the ten the names suggested.** The symtab proves that 0x01851f00..0x0185e9d0 (173
functions, BRANCH_CLEAR .. rnd) is **one CodeWarrior object**: its 75 locals (symtab 1088..1162) form one
hash-ordered run that no cut can separate, tying DemoSkipGetEvent to rnd, and its `.rodata` strings and
`.sdata` statics run monotonically through it (architecture.md "The TU map", "The split of QuatSlerp.c").
It is now **`game/BRANCH_CLEAR.c`** (`git mv` of the old file, code unchanged, state partial, the old
sessions and notes carried over); **`game/QuatSlerp.c`** keeps QuatSlerp, ahCos, ahSin, ahAcos
(0x01851bf0..0x01851f00, verify). BRANCH_CLEAR.h includes QuatSlerp.h; every include of the old header now
names BRANCH_CLEAR.h. The smoke shot is the same clock-tower scene as the run before. **Claude's call, the
user may overrule:** the candidate runs listed here before (rooms, rattles, panic, filters, warps, LZSS ..)
are sections of the one file, so they stay together; splitting them would invent object boundaries the
symtab contradicts. Doubts: (a) the head (no locals) may be Convert.c's tail rather than its own object --
no symbol decides; (b) the key `BRANCH_CLEAR.c` follows tumap's rule (first function); the real file name
is unknown (no source string). The method is in reconstruction.md "CT3" for the other large objects
(wideXcmd_DebugPrint.c, tiXcmd_TransItem.c, ssSetTexUVforCubeMap.c, ahRelocateSunPs2ObjectArry.c):
`seams.py` on each will say whether they too are single objects.
1. **`main`'s TU first** -- **partial (2026-11-09):** slice 1 written (main, the boot and init functions,
   ControllerMain, tiItemInit, scene_end, the small draw passes; 15 functions); slice 2 (2026-11-09):
   `GlobalDrawObject`, `scene_draw_init`, `system_end`, `get_draw_obj`; slice 3 (2026-11-09): `main_rtn`, `main_rtn_Particle`; slice 4 (2026-11-09):
   `DrawObjectRefraction`, `main_rtn_DOF`; slice 5 (2026-11-09): `AllDrawObject`,
   `ssHQDOF_DrawAllObject`, `main_rtn_GHOST`, `main_rtn_GHOST_DOF`; slice 6 (2026-11-09): `scene_init`
   (ALL_TABLE +0x14 / +0xb8 / +0xbc, `DRAW_OBJ_ENTRY.drawCtrlArgs`, `GA_DRAWCTRL_ARGS` and `TEX_INFO`
   passed by value, hazards.md); slice 7 (2026-11-09): `main_rtn_Particle2` -- **main.c's code is complete (verify: the shot stays black until main2 loads a bank)**. Open from slice 7: (t) `PARTICLE2_ENTRY` / `PARTICLE2` (ct3_types.h) are named from this one use (ring of 0x80-byte records at +0x5c, count +0x46, head +0x48; record +0x20 offset qword whose w scales the part's w around 128, +0x68 the draw object); the particle TU confirms them. Open from
   slice 6: ~~(q) `gaSetupDBufferData` takes 14 argument slots~~ fixed 2026-11-09 (gaInitDrawObject.c
   slice 1); (r) `BANK_TYPE_0` (type 0 takes the fog record) and
   `PARTS_HEADER_ANIMATED` (the parts header's first byte == 1) are named from one use; type 1's fog
   colour clears the frame, so `BANK_TYPE_SHADOW` is likely the stage bank (with (j)); (s)
   `PARTICLE_ENTRY.drawCtrlId` (+4) is probably a `GA_DRAWCTRL_ARGS *` like the draw list's (not
   changed: no use seen). Open from slice 5: (n) the mirror arguments
   (`asEnumMirror(ALL_TABLE *)`, `asMirrorPlaneCalc(planeVec, planePoint)`, tbl +0x2c `matrixTree`) and
   `tiChara_Translucence_End(float alpha, int fbuf)` / `_End2(int fbuf, int chara)` come from the call
   sites: confirm them in their TUs; (o) `BANK_FLAG_NO_MIRROR` (0x10) and `EV_SHADOW_AFTER_MIRROR`
   (0x3b) are named from that one use; (p) HQ DOF's eye w for the diagonal samples follows the
   decompiler's operand order (up's w), unverified -- only xyz should matter. Open from slice 4: (j)
   `BANK_TYPE_SHADOW` (type 1) is the type of every bank AllDrawObject, ssHQDOF_DrawAllObject and
   DrawObjectRefraction draw among banks 6..85: it is the scene-object bank type, not a shadow kind --
   rename it when the bank loader TU names the type; (k) `CHARACTER_FLAG_REFRACTION` (0x80000000) is named from that one
   use; (l) `MODEL_PARTS.kind` (+0x42) == 9 skips a part in the DOF slices (`PARTS_KIND_NO_DOF`), other
   values unread; (m) `DRAW_OBJ_ENTRY.dofMode` (+0x10) is ssDOF_ClipObjectFromZRange's mode (< 2 tests
   the object's view Z against DOF_NearClip / DOF_FarClip; >= 2 draws only when the near clip is -10000). Open from slice 2: (f) `ALL_TABLE` (All_table_TBL's records, ct3_types.h) is
   `layout: host` with pointer fields at EE 0x10..0x7c; the file TU decides whether the bank's relocated
   words become host pointers or stay `u_int` EE addresses; (g) settled by slice 3: `main_rtn(bank, mode, chara, flags, charaIndex, lastTex)` -- chara != 0 is the
   character pass (scene fog +0x28, Character[].lightBank's light), flags' low 15 bits the pass (0 and 4
   sort), 0x8000 no clip check; (h) **decided 2026-11-09 (Claude's call, slice 7; the user may overrule)**: bank data keeps its EE layout and its relocated pointer words stay `u_int` EE addresses, resolved by `BankEePtr(bank, ee)` (ct3_types.h: `BankImage[]` / `BankImageEE[]`, PORT globals the bank loader in fileio.c's TU must fill; RosePan's `SprMan::eePtr` is the precedent). `DRAW_OBJ_POS` (+0x38) is the first word read this way. The reasoning: it writes a particle's position
   through a pointer at the ga draw object's +0x38 (`*(qword **)(obj + 0x38)`), and the same object keeps
   an int at +0x3c (`MODEL_PARTS.attr`): a host 8-byte pointer there overlaps it, so the draw object is
   bank data that must keep EE layout with `u_int` EE addresses, and the port needs an EE-address ->
   host-pointer helper for relocated bank data (the file TU's decision, with (f)); (i) the draw list and
   particle records (`DRAW_OBJ_ENTRY`, `TEX_INFO`, `PARTICLE_ENTRY`, ct3_types.h) are `layout: host`
   for the same reason: their owner (objSort / the bank loader) decides; osTotalObjTable keeps the EE
   2000 bytes per bank, so on the host it holds 250 pointers, not 500. Open from wideXcmd_DebugPrint.c slice 1 (2026-11-10): (u) `CHARACTER_FLAG_REFRACTION` (0x80000000) is also
   asCharacterOperate's gate (set: the whole operation; clear: only asRequestExecute) -- probably an "active /
   operated" flag rather than refraction: rename when the flag's writer is read; `CHARACTER_FLAG_PLAYER`
   (0x20000000) is named from the same function; (v) `CHARA_TARGET` (w300[0]) is an EE word for an object with
   flags at +0x194 and a vector at +0x90: `CharaTarget` answers none until the TU that writes it decides the host
   form; (w) asCharacterOperate clears the first slot's `keyTicks` (chr, not ch) when the keys are off: kept as the
   ROM does, harmless since only Alyssa (slot 0) counts; (x) the TU is probably two objects (wideXcmd_* then the
   as* player code from 0x018f3fa0): not split yet, no local-symbol evidence read. Open from
   wideXcmd_DebugPrint.c slice 2 (2026-11-10): (aa) **asInitPlayerBehaviorControler now sets
   CHARA_MOTION_TABLE** (the bank's ALL_TABLE +0xb8 word: `fog` for scene banks, the motion table for
   character banks -- the field wants a union or a second name when the bank loader TU reads it);
   wideXcmd_DebugPrint.c reads it through `MotionRecord` (BankEePtr of the character's bank). The
   `CharaMotionTable` helpers in ahRelocateSunPs2ObjectArry.c and QuatSlerp.c still answer none: their
   TUs can switch to the same BankEePtr read; (ab) motion record fields named from one use each
   (MOTION_REC_NEXT / _NEXT_MODE +0x10 / +0x14, _KIND +0x1c with stair kinds 1 and 3, _FACING +0x1d with
   backward kinds 0x0f..0x12, _RUN +0x1f); (ac) CHARA_TARGET2 (w300[1]) joins CHARA_TARGET: both EE words,
   CharaTarget / CharaTarget2 answer none, so asCheckAlyssaOnStairway answers 0 on the host; (ad)
   asMonsterDistanceStack_inPanic's growth copies the old entries only (the ROM copies the new count, 16
   entries past the old block: a read past it, no effect on the kept data); (ae) asResetCharacterRotation
   walks part records (PARTS_REC, as asResetCharacterPosition passes header + 0x40): type short at +0
   (MODEL_PARTS +0x40, 1 = turned), PARTS_FLAG43_WARPED also marks a placement change (as1CharacterPositioning,
   asCharacterPositioningOnlyDeltaPos): it may be a generic dirty bit (PARTS_ANIM_DIRTY is the same bit);
   ~~(af) the behaviour API's host form~~ (2026-11-10, slice 3): asFindBehaviorByCharacter answers a host
   BEHAVIOR*; the (bank, anim) calls take it as (BankOfHostPtr(b), BankEeAddr(bank, b)) -- BankOfHostPtr
   is now exported from ahRelocateSunPs2ObjectArry.c (PORT helper); (ag) the slot arrays are named from
   asRequestExecute (wideXcmd_DebugPrint.h CHARA_SLOT_*: slot 0 the body, slot 5 the blend slot; fade /
   fade length / prev / playable / time) -- CHARA_BHV_W35C..W470 stay as aliases until the readers
   (asCharacterAnimationExecute, asMotionAdderByNowMotion) confirm them; (ah) asCharacterForceMoveOperate
   needs CharaTarget (CHARA_TARGET's EE object, its +0x198 part index): it returns early on the host
   until the target's writer decides the host form (with ac); asRequestExecute's slot-0 start clears
   keyTicks (+0x520) and Alyssa's slot-4 starts need hp (+0x524) != 0: both read as the struct names
   them, which fits hp being the current value (161's doubt). Open from wideXcmd_DebugPrint.c slice 4
   (2026-11-10): (ai) asCharacterAnimationExecute confirms (ag) for fade (the parts header's
   PARTS_HDR_ANIM_ABS * _SIGN round trip), fade length (PARTS_HDR_ANIM_STEP while it plays) and time
   (playing ticks / 100); new from it: CHARA_SLOT_ENDED (0x3a4), CHARA_ROOT_PREV (0x310), CHARA_BASE_ROT_PREV
   (0x330), CHARA_ROOT_MOVE (0x358, 1 at init) * MOTION_REC_ROOT_MOVE (record +0x1a) = root motion on,
   CHARA_SLOT_CHANGE_AT_KEY (change bit 2: atKey); the ROM reads slot 0's record even when slot 0 is -1
   (the record before the table): the port reads none (root motion off then); (aj) asChangeAlyssaMortionByDamaging's
   motions (0xda / 0xdb the damage pair, 0xef -> 0x65 and 0x68 -> 0x6e fixed follow-ups) and DAMAGE_JITTER
   (flags bit 0) are named from that one use; its turn lives in a static helper (TurnToDamage), so callcheck
   lists asRadianByVectors / gaSetRotMatrixY as missing from it: explained, not a gap; (ak) MOTION_HAIHAI
   (0x30) and its steps 0x31 / 0x32, PIA_KEY_H0A (key +0x0a) named from asPIA1_KEY_TBL_HAIHAI; it requests
   for character 0 as the ROM does. Open from wideXcmd_DebugPrint.c slice 5 (2026-11-10,
   asCheckCharacterHited): (al) ALL_TABLE +0x20 is now `hits` (HIT_TABLE: count +0x24, recs +0x28) and
   HIT_REC (12 bytes: attacker EE word, attack, who, isObject, bank) is named from this reader only: the
   collision TU that fills it (ssProcessHit's, probably) confirms it; (am2) **the attacker word's bank is a
   guess**: HitAttackerKind resolves rec->attacker in Character[who].bank (its byte +0x1a2 is
   HIT_KIND_PANIC 3 / HIT_KIND_NORMAL 4) -- if the writer stores an address in another bank, change the
   helper; (an2) the attacker characters 2..6 / 32 are named HIT_CHARA_* after the SE bank each plays
   (E_HAMMER, none, E_ONO, E_SMA, E_SWO, none); the object banks by their name word (E_93, E_14, E_95,
   SCSR, E_97); CHARA_HIT_LANDED (+0x51c) and CHARA_BOSS_STRIKE (+0x4e0) from this use; the triggers
   (0x95 MUTEKI, 0x46 / 0x50 BOSS_STRIKE / STRUCK, 0x52 BOSS_ACTION, 0x6f / 0x88 / 0xb0 NO_HIT) too;
   (ao2) the ROM's dead test kept as a comment: the SWO / SMA flinch tests attack == 0x2b / 0x2c inside
   the attack == 0x20 / 0x1e case, so that flinch never sounds; vsBossMuteki / AfterPanicStartMuteki live
   in unassigned_data until their owner (the boss / panic TU) is written. Open from wideXcmd_DebugPrint.c slice 6
   (2026-11-10, the key dispatch): (ap) **every key-table entry now takes (CHARACTER *, PIA_KEY *)** --
   asPIA0_* answer int (PIA_NONE / PIA_MOVE / PIA_HOLD), asPIA1_* and asPIAK_* are void -- so asPIA0 /
   asPIA1 / asPIAK are typed host tables (the ROM's sdata 0x0193baf0 / 0x0193bb60 / 0x0193bbd0); PIA_KEY
   (cnt, trg, h08, h0a, h0c) is the stack record, h0a / h0c named from their uses only; (aq) the record's
   +0x1d (MOTION_REC_FACING) indexes asPIAK (19 kinds: 3 / 6 / 9 / 12 no pad move, 15..18 the backward
   MoveBLR): it is the pad-control kind more than a facing -- rename with the motion table's reader;
   (ar) the target copies at 0x490 / 0x4b0 (vec, flags, part) overlap refractAxis (0x4b0) and CHARA_BOSS_STRIKE
   (0x4e0, the pos copy): CHARACTER's 0x490..0x4f0 wants a union once the readers are known; (as) pBossInfo's
   pointer words (+0x100 chain anchors, +0x110 chain flags) have no host form: BossInfoPtr / ChainAnchor answer
   none, so asChainLengthCheck sees no chain (-1) and PULLED never aims -- the boss TU decides (with h);
   (at) motion names from one use each: MOTION_PULLED 0x41, MOTION_CHAINED 3, MOTION_PULLED_FREE 8,
   MOTION_WATER 0x51, MOTION_DOWN 0xd9, the hold motions 0x0c / 0xc6 / 0xc7, SITNOUP's 0xb5 / 0xc1; trigger
   variables 7 (WATER_READY 6), 0x34, 0x3e and event 0x91 (TRIG_EVENT_NO_PAD_MOVE) too; (au) the smoke run
   wrote an empty last.png once (shot_27180.png was fine): if it recurs, look at the shot writer's last frame.
   Open from wideXcmd_DebugPrint.c slice 7 (2026-11-10, the pad move, panic events, face motions): (av) the
   pad move reads pad[0]'s bytes +0x1e / +0x1f (PAD_DATA.stick[2..3]) as the left stick's x / y and KEY_MAP[34]
   as the stick dead zone: gaPadRead's TU confirms which stick; asCharacterMoveByPad's length test reads
   an uninitialised z (sp+0x168) in the ROM: the port uses 0; (aw) CAMERA.viewPrev (+0x80) and
   CAMERA.bank (+0x168) are named from this one use (the camera TU confirms them); controller_type 0 / 1 / 2
   named CONTROLLER_CAMERA / _TANK / _CAMERA_HOLD from how the move reads them; PIA_KEY.h0c = the stick
   points behind her (> 150 degrees; > 90 with event 0x6f): probably the turn-around motion's cue, its
   reader (asPIA1 STAND / WALK / RUN) confirms; (ax) asPanicEventOperation's modes (PANIC_EV_*) and answers
   (scream 1, tumble 2, freeze 3) are named from the function alone -- the callers (asPIA1 PRUN / PSTAND,
   asCharacterMortionFunctionOperation) confirm them; group 0x0e = running (MOTION_GROUP_RUN) likewise;
   event 0x8b TRIG_EVENT_NO_PANIC_EVENT; (ay) asMotionAdderByNowMotion's second pAM list compares slot 3's
   *time* (0x464) where the first compares slot 2's motion: a ROM slip, kept; AM / pAM / AME / pAME are
   host tables generated from the ELF (EE records, host pointers); MOTION_TABLE_H12 is pAM's arg (name
   still open); the blink pairs (ALYS 0x42 / 0x43, CALH 0x103 / 0x104, CALJ 3 / 4) and character ids
   0x1c / 0x0d named from this use. Open from slice 8 (2026-11-10): (az) the PRUN / PSTAND callers
   confirm (ax): PANIC_EV_TAKE_RUN maps freeze / tumble / scream to 0xb1 (= CHARA_MOTION_KAKOKYU, the
   breathing) / 0xa7 / 0xaf, PANIC_EV_TAKE_STOP never answers a tumble there; (ba) TRIG_VAR_WATER_STATE (7)
   is the held item's state, not only the water's: 2 = the bow ready (ITEM_STATE_BOW, asYumi_Run_Check), 6
   = the water: rename it when the item TU reads it; (bb) asPIA1_KEY_TBL_PSTAND tests event 0x68, already
   named EV_FORCE_MATRIX_OPERATION from the shadow code: one of the two names is wrong; (bc) asPIA1_KEY_TBL_LOCK
   requests the motion's MOTION_REC_NEXT at once for any motion it has no lever mash for (the ROM's
   default 1, kept); (bd) KEY_MAP actions 12..15 / 17 / 18 / 21 / 25 (GETUP, ARROW2, ITEM, RUN, HIDE),
   TRIG_VAR_BOW_53, TRIG_EVENT_HIDE_HELD (0x87) / _HIDE_66 / _LOCK_9A and the cat walk / hiding / stair /
   bow motion indices are named from these uses only; (be) pModelPartsHeader[bank] is indexed as
   MODEL_PARTS[] from +0 by the cat walk (part i's flags at i * 0x380 + 0x50), which disagrees with
   QuatSlerp.h's "records from +0x40" comment: the loader TU settles it; (bf) callcheck flags the BEHIND /
   SITBEHIND entries for asRadianByO / asISubRotate16: the helpers AimBehind / BehindTurnMotion make
   those calls. Open from slice 9 (2026-11-10): (bg) TRIG_EVENT_LOCK_9A swaps in a whole second set of
   ground motions (0x53 / 0x54 / 0x55 / 0x59 / 0x5a / 0x5b / 0xc3): "hurt" is a guess, the event's setter
   (a trigger script) names it; KEY_MAP_HIDE is really the crouch button (SIT / STAND / MOVEEND) and
   KEY_MAP 24 (KEY_MAP_NO_CROUCH), TRIG_EVENT_CRAWL_88, MOTION_STAND_1 and the walk / run / crawl motion
   indices are named from these uses only; (bh) asShootArrowSetting's first argument is a u_char kind
   and asPIA0_KEY_TBL_YUMI passes ArrowList's low byte (lbu): ArrowList may be a u_char array, not
   int[50] -- asUseArrow.c settles it; (bi) callcheck flags MOVEEND / RUN / WALK / STAND for
   asYumi_Run_Check / asWater_Run_Check: the helper ItemKeyTaken makes those calls. Open from slice 10
   (2026-11-11, asCharacterMortionFunctionOperation: **the TU's code is complete, verify**): (bj) the
   motion record's +0x20 / +0x24 / +0x28 / +0x2c are named from this one reader as an attack / no-hit
   window (MOTION_REC_HIT_*), KIND2 4 / 10 / 0x0c..0x11, GROUP 4 / 5 / 0x0f..0x1b and FACING's
   CHARACTER_FLAG_FACING_HELD (0x08000000) likewise; KIND2 10 is the attack kind (it was named
   MOTION_REC_KIND2_RESET from asRequestExecute: one value, two names); (bk) asAlyssaLockedByAnything's
   high byte 0x100 / 0x200 are read as "held by Character 2 / 32" (their catch motions 0x23 / 0xe5 and
   the part PART_HASH_CATCH she is pinned to): confirm in the catcher TUs; ~~(bl) the ssHitAddAttackRequest
   (8 arguments: a2 0, a4 -1, a5 1 at the one site) and ssHit_GetMaxIDFromAtkID prototypes come from the
   call site~~ confirmed 2026-11-11 (ssHitCheckAttackToScn.c slice 1; the request is void); (bm) MothInfo (bss 0x0193ced0, 40 bytes) is declared as u_char[0x28] in unassigned_data
   until asMothOperate.c types it; (bn) PORT: a motion index with no table record (MotionRecord NULL) is
   read as no motion (the ROM would read garbage). callcheck's asFindModelParts / asRadianByVectors are
   in the helpers PartOfBank / CatchPin. **Next:**
   ~~`ga/gaInitInterrupt.c`~~ (verify, 2026-11-09: main's frame loops now), then in the order a smoke
   run reaches the stubs: `main2` (game/main2.c: title and scene set-up), `IOPInit` / `Character_All_Init`
   (Character_Init.c), `FS_work_init` / `FileSoundServer` (fileio.c), `SunAllocInit` (ahalloc.c), the ga
   library (`gaDmaInit`, `gaVifInit`, `gaSetupGSInitData`, the BGT packets, `gaPadRead`), `movie.c`.
   Open from slice 1, for the TUs that own them: (a) the 106 globals main.c's slice uses live in
   `lib/unassigned_data.cpp` (declared in `game/unassigned_data.h`, structs in `game/ct3_types.h`): move
   each to its owner when written; (b) `BANK_INFO.loaded` (+0x20) is an int here, maybe a pointer;
   (c) the trigger events' names in main.c (`EV_HOLD_*`, 0xca / 0xcc) and `controller_mode` 100 are read
   from main's use only; (d) the prototypes in the new TU headers come from call sites: confirm each when
   its function is written; (e) main.c's .sbss run is a placement guess (architecture.md).
   **Queue refilled 2026-11-09 (scheduled run, the queue was empty)** in the order the smoke run reaches
   the stubs: ~~ga/gaDma.c~~ (verify), ~~ga/gaVifInit.c~~ (verify; its calls made ga/gaDmaSync_VIF1.c's
   gaDmaSendN_VIF1 / gaDmaSync_VIF1 stubs: that hand-asm object is a small TU worth queueing next),
   ~~ga/gaInitDrawObject.c~~ (verify 2026-11-09: all 24 functions; the texture three have no ROM caller),
   ~~ga/gaInitMeter.c~~ (verify 2026-11-09; renamed from game/: a GCC object, the ga library's last; T1 mode writes now reach the host timer),
   ~~game/ahalloc.c~~ (verify 2026-11-09: all 7 functions; SunAllocInit runs at boot, the rest waits for
   a caller; its AlignedNum call made ~~game/AlignedNum.c~~ (verify 2026-11-09: AlignedNum, WaitLoop,
   IopLoadModule; IopLoadModules in ArtoolDebugOpen.c is its one caller, still a stub), hazards.md;
   open: (u) the heap's +0x14..0x34 is unread here -- whoever reads it names it; (v) the host block header
   is 0x20 bytes, so EE-sized areas hold slightly less: watch for _SunAlloc errors once loading runs),
   game/sound.c (partial 2026-11-09, slice 1: 38 of 69 functions -- boot / end, SE and SQ entry points,
   the stages' SQ rules, movie / demo mute, angle helpers; Sound_system_init now asks SdReqLoadSE for
   C_0000, T_0000, E_HAMMER, Z_REI; open: (w) the Sd driver TU (game/SdMasterVolumeSetSE.c, 0x018cbf90..)
   is the next sound blocker (slice 1 written 2026-11-10: the FSS command ring and thread, volumes, reverb, SE stop / lookup, load-end wait, SdAlloc / SdFree, SdRnd; left: SdProc and its five procs, SE play / load / supply, SdSetParamSE, SQ and the stream's play; the IOP side, CpCall_IOPSndDriver, is a stub, and FSS_Thread_Init runs only once IopLoadCapcomSound is written; slice 3 written 2026-11-10: SdReqLoadSE / FindSEFile x2 / SdReqLoadSECancel / SdPurgeSE and the SE play side -- SdReqPlaySE, SdSetParamSE, NextPlaySE, SdPlaySE, SdSupplyCHSE, SdPlaySEUpdate, SdPlaySEProc, SdPlaySESpProc; the SE loads now start at boot and step to the CpSifDmaTransEEToIOP / CpCall_IOPSndDriver_2 stubs. Left: SdLoadSQProc, SdReqPlaySESp1, the SE stops (SdStopSEAll, SdStopSESDT, Sp1), SQ play / stop / volume, the ADS stream, SdSoundEnable / Disable, SdChangeStereoMono, IopLoadCapcomSound. Slice 4 written 2026-11-10: the SE stops (SdStopSEAll, SdStopSESDT, the Sp1 three), SdReqPlaySESp1, IsPlaySEReady, the load waits, SdPlaySQ / SdStopSQ / SdStopSQAll / SdStopSQAllNow / SdVolumeSQ, SdPlayStream, SdSoundEnable / Disable, SdChangeStereoMono (it takes the stereo flag: sound.c now passes gStereo). Left: SdLoadSQProc, SdPlayADSProc, IopLoadCapcomSound. Slice 5 written 2026-11-10: SdPlayADSProc (SD_ADS_PHASE), IopLoadCapcomSound (FSS_Thread now runs from boot; CpDivideEECom.c's CpCall_IOPSndDriver stub answers a zero reply word, not NULL, because FSS_Thread reads it), SdSetParamSESDT, SdFindLoadSEInfo (SD_FIND_LOAD_SE_*). Slice 6 written 2026-11-10: SdLoadSQProc (SD_SQ_LD_STEP, SD_SQ_REGISTER, SND_CMD_SQ_*) -- **the TU's code is complete (verify)**: nothing plays until the IOP side answers; CpDivideEECom.c (the IOP side's EE half: 5 new stubs) is the next sound blocker. Open from slice 6: (av) SND_CMD_SQ_0C / 0D / 17 / 18 / START are named by the step that sends them, the IOP side unread; the SQ reuses SND_CMD_SE_REGISTER / SE_PURGE with the player (gSQi, always 0) in place of the slot; (aw) the ROM's slips kept: step 6 prints SdLoadSEProc's DMA message (@2330), steps 3 / 6 / 9 keep fd after FlReqClose, SD_SQ_LD_STOP (0x1b) sends the stop twice, 0x16..0x18 are listed but never set. Settled by slice 6: (ap) SD_SQ_LD +0x70 / +0x74 effectType / effectDepth (SdSetEffect core 0 after effectDelay +0xa4 frames), +0x78 spuAddr, +0x7c / +0x80 / +0x84 iopSq / iopHd / iopBd (the old iopBd at +0x80 was the .HD), +0x88..+0x90 sizes, +0x98 fd, +0xa8 stopWait; gFileSQ's sdArg2 / 5 / 6 are spuAddr / effectType / effectDepth (x settled too); (aq) the swap: once row 0 has ended (0x1d), row 1 is copied into it and cleared (0x1e). Open from slice 5: (at) SND_TRANS_ADS.path is 0x40 bytes, not 0x60 (SdPlayADSProc writes +0x48..+0x80); its other words and gArg_CpDriverInit's (SND_DRV_INIT) are named by value only -- the IOP side names them; (au) SND_CMD_ADS_OPEN / ADS_START / INIT_08 / INIT_07 / INIT_25 and EE_IOP_ADS_OPENED (sndFlags bit 1) are named from the EE side only; SND_CMD_ADS_ENABLE (0x2a0000) is also IopLoadCapcomSound's stream set-up, so it may be an init rather than an enable. Open from slice 4: (ap) SD_SQ_LD +0x70 / +0x74 / +0x78 / +0xa4 are named arg5 / arg6 / arg2 / arg4 after SdPlaySQ's arguments and +0x7c / +0x80 iopSq / iopBd after SdStopSQAllNow's frees: SdLoadSQProc names them; (aq) SdPlaySQ over a playing row 0 writes row 1 and flags row 0 to stop: the swap is SdLoadSQProc's, unread; (ar) EE_IOP_STAT +0x40 bit 0 (EE_IOP_SND_READY) and the new SND_CMD_* words (STEREO, STOP_SQ_NOW, ADS_STOP, SOUND_ON / OFF, ADS_ENABLE / DISABLE) are named from the EE side only; (as) SdStopSESp1 sets no return value in the ROM (SSStopSESp1 passes v0 on): the port answers 0. Open: (am) SD_SE.unk30 (the play word's third byte, 999 by default) and reqArg2 (>= 1000 resident, 0x3e0 shares one channel) are named from the EE side only; (an) SdPurgeSE sends gSELd[gLdi].reg, not its own row's, kept as the ROM's slip; (ao) PLAY_SE_SP1 +0x20 was named state: it is posVol (MidiControl_DemoStart's 2 = two tenths), renamed in sound.c), so nothing sounds; its prototypes in
   SdMasterVolumeSetSE.h come from sound.c's call sites; (x) gFileSQ's words +4 / +8 / +0xc are
   SdPlaySQ arguments named sdArg2 / sdArg5 / sdArg6 until that TU reads them; (y) TRIG_VAR_STAGE (0x1d),
   TRIG_EVENT_SE_MUTE (0xb7), the stage-1 SQ events 0x3b1 / 0x3ba / 0x3bd / 0x3bf and STAGE_BGM_* are
   named from sound.c's use only; (z) gBfoScene / gNowScene / gStereo / gMasterVol* / gPlaySESp1 sit in
   lib/unassigned_data.cpp until their owner, with the ROM's initial values), slice 2 (2026-11-09: 68 of 69 -- SSProc /
   SSBGMProc / Branch1..5BGMProc, the sound objects and trigger actions, positional SEs, volume save and
   restore, SSPlayADS, SSPlaySESp1, SceneSELoad / BranchSELoad; left: SSLoadSEfromLoadPoint, which reads
   fileio.c's load-point record +0xd4 (a pointer to up to 5 rows of 0xc bytes, name pointer first): write
   it with fileio.c's TU. Open: ~~(aa)~~ settled 2026-11-10 (Sd driver slice 3, Claude's call): SD_SE and PLAY_SE_SP1 are `layout: host` with an `SD_SE_UPDATE` host function pointer, called with the SE's handle. Was: `SD_SE.callback` (+0x50) is an EE code word in a 0x54-byte record,
   but SdSetParamSE / SdReqPlaySESp1 now receive host function pointers (SetPositionalParam_*): the Sd
   driver TU decides the host form (a table index, or a host-layout record); (ab) the trigger events and
   variables SSBGMProc / BranchNBGMProc use (0x9a boss battle, 0x49 Alyssa falls, 0x51, 0x96, 0xc9 / 0xd4
   chase, 0x53 / 0x54 T_BREAK, 0x26 piano volume, 0x58 quiet, the stage rooms' events) are named from
   sound.c's use only; (ac) `ahGetHash` / `asFindModelPartsID` now take a 64-bit hash (u_long): the ROM
   compares 0x3161c34c2 and 0x74dda77d0cbcf0c2, so main.c's Omni lookups pass 64-bit hashes too -- confirm
   when QuatSlerp.c's hash functions are written; (ad) `ALL_TABLE.sound` (EE 0x54) is a host pointer like
   its neighbours, its `objs` an EE word read with BankEePtr (decision h); (ae) `ADS_FILE.loop` is named
   from the table's pattern only), slice 3 (2026-11-09: SSLoadSEfromLoadPoint -- **sound.c's code is
   complete, verify**: the "+0xd4 record" was the bank image + 0x80 (the ALL_TABLE, FSMltLoadProc sets
   All_table_TBL[bank] to it) + 0x54 (`ALL_TABLE.sound`), whose word 0 is now `SOUND_OBJ_TABLE.loadSE`,
   the EE address of up to 5 `SE_LOAD_POINT` rows. Open: (af) the ROM passes the bank's image (BankInfo
   +0x1c, still `pad1c` in BANK_INFO) with no bank index; the host finds the bank by matching
   `BankImage[]` (PORT `SoundBankOfImage`) -- fileio.c's TU must fill BankImage before it calls this, or
   may change the call to pass the bank),
   game/Character_Init.c (partial 2026-11-09, slice 1: Character_All_Init, IOPInit, McInit, mc_alloc / mc_free /
   mc_save_data_init, mc_check, mc_check_always, MemorycardAutoLoadInsertedCheck3, mc_make_checksum,
   mc_load_new; left: MemorycardUpdate (0x17e8, reached every frame), mc_check2/3, mc_save_system_init, the save / load / format
   paths, SceneLightSet / CharLightSet. Open: (ag) CHARACTER's new fields (ct3_types.h: slot004 / slot018 /
   slot022, w090, w098, w0fc, h100, b102, b2f1, b2f2, w300, b534, h536) are named by offset from the reset
   only; hp / hpMax (+0x524 / +0x528, both Char_Init_MAX_HP) may be the other way round; (ah) CHARACTER.junc
   (+0x570) is a host pointer to CHARA_JUNC whose buffer sits at EE +0xc: whoever allocates it confirms
   the record; (ai) IopLoadModules is declared taking argv[0] (IOPInit passes it; the ROM body reads no
   argument -- SetDeviceName may): ArtoolDebugOpen.c's TU confirms; FSInit is empty (4 bytes);
   (aj) mc_alloc answers 1 on success where the ROM answers the block address; SdAlloc / SdFree are still
   stubs, so the card buffer never exists yet; (ak) CHARACTER still lives in ct3_types.h, not this TU's
   header: move it when the rest of its users are written); (al) mc_load_new bumps mc_save_normal_num,
   not its own step (disassembly 0x018b8d10 / 0x018b8df0): kept as the ROM's slip -- mc_save_normal's
   session checks whether it matters), slice 2 (2026-11-10: MemorycardUpdate -- its four flows split into
   static mc_update_* helpers, callcheck comment -- plus mc_save_init, mc_formatcheck, mc_formatcheck2,
   mc_check4, MemorycardAutoLoadFormatCheck; left: system_data_load_set, mc_load_set, mc_save_new,
   mc_save_normal, mc_free_area, mc_Getinfo_count, mc_check_always2, mc_check2/3, mc_save_system_init,
   data_save_set / ContinueBackupSet / ContinueBackupGet (data_save_set takes 0x01a31450, an unnamed bss
   address: name it there), mc_char_set, the lights. Open: (am) the mcErr_ID / mcAccess_ID names
   (Character_Init.h MC_ERR_*, MC_ACCESS_*) are read from MemorycardUpdate's flows only: numbers set on
   unlike paths stay MC_ERR_MSG_n until the menu TU that prints them names them; (an) SSStopSE /
   SSPlaySE(MC_SE_ACCESS) calls whose a2 the ROM leaves unset pass 1 (every traceable site holds 1);
   (ao) TRIG_EVENT_LOAD_SLOT2 (0x0f), TRIG_EVENT_MC_LOADED (0x99), TRIG_EVENT_SAVE_ALT (0xa9) and
   MC_DATA_USED (save image +0xc710) are named from this use only; (ap) tiFade_Start(float) is declared
   from the call (10.0f in f12); FadeEndTicks sits in lib/unassigned_data.cpp), slice 3 (2026-11-10:
   the save path -- mc_save_new, mc_save_normal, mc_save_set, McRegisterEvent, ContinueBackupSet
   (0x01a31450 is the symtab's continue_backup) -- system_data_load_set, mc_check2, mc_check_always2,
   mc_Getinfo_count, MemorycardAutoLoadInsertedCheck2; MC_HEADER (the 0x80-byte system data) in
   Character_Init.h; left: SceneLightSet, CharLightSet, data_save_set, mc_load_set, mc_char_set,
   mc_free_area, mc_check3, mc_save_system_init, ContinueBackupGet and the rest of the 19). Open: (aq)
   mc_save_set ORs AA_UidCount << 8 into header word 1 (the readers mask 0xfffffff0, so their compares
   stay consistent) while main bumps AA_UidCount by AA_UID_STEP 0x10: kept, the uid's units are
   unclear until the menus run; (ar) system_data_load_set(1) keeps only OMAKE_2 before OR-ing OMAKE_1 back, dropping
   OMAKE_BOOTED: the ROM's, kept; (as) KEY_MAP[19..20] = KEY_MAP[54..55] after a load (disassembly
   0x018b8778): the key-config TU names the pairs; (at) TRIG_EVENT_OMAKE_1/_2, TRIG_EVENT_MONAURAL,
   TRIG_VAR_*_VOLUME are named in Character_Init.h, main.c's system_init still uses the raw numbers;
   (au) mc_save_new writes the icons again even when sceMcChdir finds the directory (step 2, not 10) and
   frees the icon buffer before a close that may be refused: both the ROM's, kept; FlGetLen(dir, name,
   mode) is declared from the call (mode 1)); slice 4 (2026-11-10): SceneLightSet, CharLightSet (SCENE_LIGHT / LIGHT_SET / GA_LIGHT_WORK in
   ct3_types.h, MODEL_PARTS.lightColor / lightPos), mc_load_set, mc_char_set (a range table),
   mc_CheckMcWorkAllocc, mc_Getinfo_mc_type; data_load_set stubbed (next: it and data_save_set together,
   the save image's layout), left also mc_free_area, mc_check3, mc_save_system_init, ContinueBackupGet.
   Open: (av) MemorycardUpdate calls tiFade_Start(10.0f) with a0 (the fade type) unset: the port passes
   0, the fade TU says which types exist; (aw) PARTS_FLAG_FOG (bit 0) also switches a light part off in
   SceneLightSet / CharLightSet: probably a general hidden / off bit, rename when the parts loader is
   read; (ax) mc_load_set unregisters characters whose flags have 0xc0000000 (CHARACTER_FLAG_REFRACTION |
   _SHADOW, both named from one use): likely one in-use meaning; TrgCharacterDraw(mode, chara) and
   TrgSceneDraw(a0, a1, a2, name, bank) are declared from these calls (all leading arguments 0);
   (ay) MODEL_PARTS.attr is the EE address of the part's GA_LIGHT_WORK (BankEePtr), the word main_rtn
   copies to DRAW_OBJ_ATTR (+0x3c): gaSetLight's TU confirms the light block's other fields; ALL_TABLE.light
   is now SCENE_LIGHT * (main.c reads charaScale); slice 5 (2026-11-10): data_save_set, data_load_set
   (MC_SAVE_DATA, the save image; architecture.md "The save image"), mc_free_area, mc_check3,
   mc_save_system_init, MemorycardAutoLoad (its own states and messages, MC_AUTO_*) with Init /
   InsertedCheck / FreeCheck, ContinueBackupGet, the four mc_Getinfo_* readers, Set_AA_New_McAlloc_Mode
   -- **Character_Init.c's code is complete, verify** (nothing on screen reaches the card yet). Open:
   (az) the four MemorycardAutoLoad*Check functions now return int (the ROM's v0, hazards.md); (ba) the
   save image's 0xd590..0xdef0 and 0xe5cc..0x105d4 are not written by data_save_set: ssAI_MemSaveData
   (tiAvoidObject.c) and other savers fill them -- the TU that writes them names the ranges;
   (bb) data_save_set does not bound its record count by 35 (the ROM's); (bc) MC_BANK_* (0/1 scene,
   2 shape, 3 behaviour), TRIG_EVENT_CLEARED (0x0b), TRIG_EVENT_MC_LOADING (0x0c), TRIG_VAR_ROUND (0x59),
   TRIG_EVENT_GLBL_LOADED / _UNLOADED (0x23 / 0x25), TRIG_EVENT_TITLE_CANCEL (0x10), the events a load
   clears (9, 7, 8, 0x19, 10, 0x97, 0x9e, 0x15, 0x1a), MemorycardAutoLoad's answers 2 / 3, CAMERA.cut
   (+0x178), MODEL_PARTS.drawOnPos (+0x2f0) and PANIC_INFO's fields are named from this use only;
   (bd) CHARACTER.slot004[10] holds the character's behaviour banks (data_load_set compares the bank with
   each): rename it when the character TU reads it. Next: game/ArtoolDebugOpen.c, game/main2.c,
   game/fileio.c.
   **game/ArtoolDebugOpen.c (verify, 2026-11-10): all 15 functions** -- the file layer (architecture.md
   "The file layer"): IopLoadModules (now void: (ai) settled, the ROM reads no argument), FlInit (the
   pack's directory; a smoke run prints no FlInit error, so the directory loads and sceCdSearchFile finds
   the pack), the request queue and FlProc, FlLoad, FlGetLen, FlFindPacked. Host: io::Disc lays out the
   subfolders' files and answers sceCdSearchFile by name (item 3), sceCdBreak, SyncDCache. Open:
   (be) no smoke run reads a file yet (main2 / fileio.c are stubs): the first FlLoad / FlProc read is the
   real test; (bf) loose files (not in the pack) need IOPGetFileSize -- **game/IOPGetFileSize.c written
   (verify, 2026-11-10: all 4 functions; gSCD moved there from lib/unassigned_data.cpp)**, but no host
   server answers FSYS_IOP's 'SUNA' (0x53554e41, fno the same number, send the 0x40-byte name, answer 0x10
   bytes: [0] size, [1] first sector), so sdk/sif.cpp answers zeros and a loose file's size stays 0: a host
   task (an FSYS_IOP server on io::Disc, registered like YamiPan's rxsnddrv) before any loose file is
   opened; 'SUN@' (IOPComTest) and 'SUNB' (IOPSetSDT) are bound but no ROM code calls them;
   (bg) CpGetEEIOPStatus's stub answers a zeroed record (the ROM: &gEEStat); EE_IOP_STAT's +0x50 is named
   iopBusy from FlProc's one use; (bh) gADSOpenSync and gSCD sit in lib/unassigned_data.cpp; gSCD is
   sceSifClientData[3] (120 bytes on the EE: 3 x 0x28), FlProc waits on [1]; (bi) FL_STATE_CANCEL (0x15)
   has no writer in this TU: whoever sets it names it; (bj) gValIOPGetFileLen (0x0193e350) is placed in
   this TU by address order, before IOPGetFileSize.c's static sFileNameToIOP; the symtab lists it as a global outside that object's local run, so it stays in ArtoolDebugOpen.c (2026-11-10).
   **game/main2.c (verify, 2026-11-10): all 18 functions** -- main2, InitLoadSceneChar (INFO / E_GL /
   0MH0 into global banks 5 / 3 / 1, then ATLD at power-on or TITL into bank 0), the scene / global /
   shape / behaviour triggers, CharacterAllFree, the self-discard queue (SELF_DISCARD_LOAD, main2.h).
   A smoke run now leaves main2 and runs the frame loop (no crash, ~26,000 frames in 15 s: nothing paces
   it yet); the shot stays black because FSMltLoad is a stub, so no bank ever loads. **game/fileio.c is
   the next blocker** (FSMltLoad / _End / _Status / _EndWaitFile / FSMltSetDrawBank, BankImage[] /
   BankImageEE[] for decision h). Open: (bk) PORT BankReady (main2.c) skips TrgGLBLSceneDraw /
   TrgSceneDraw's set-up while All_table_TBL[bank] or pModelPartsHeader[bank] is null -- drop it once the
   loader fills them (the ROM's FSMltLoadEndWaitFile returns only when the bank is in); (bl) CHARACTER's
   fields renamed from this TU: behaviorBank[10] (+4, was slot004: (bd) settled), shapeName (+0x18),
   behaviorName[10][10] (+0x22), brain (+0xfc, was w0fc), legParts[8] (+0x54c, the eight leg parts'
   EE addresses + 0x40 via the new BankEeAddr; their readers resolve them with BankEePtr(chr->bank, ..));
   MODEL_PARTS.hash (+0x48, u_long) and .matrix (+0x1c0) named; (am) CHARACTER.brain is an EE word:
   TrgCharacterShapeFree reads its +0x24e id through it, which is only safe while nothing sets it (the AI
   TU, tiAvoidObject.c / ssAI_*, decides the host form); (bm) ROM slips kept: CharacterAllFree passes a
   behaviour slot index where TrgCharacterBehaviorFree takes a bank; TrgCharacterBehaviorSet writes slot
   10 when all ten are taken; TrgSceneLoad / TrgSceneDraw answer the caller's s0 for a global bank (the
   port answers the handle / bank); (bn) EV_TITLE_23 / _25 (0x23 / 0x25: Character_Init.h calls them
   TRIG_EVENT_GLBL_LOADED / _UNLOADED -- one name should win), EV_SELF_DISCARD_LOADING (0x4a), EV_NO_E00
   (0xb6), FS_LOAD_* (FSMltLoad's kind 0..3), the enemy scene banks (E_02 0x27, E_03 / E_04 0x1e, E_05
   0x22, E_00 0x2e) and GLBL_BANK_* are named from this TU's use only; (bo) Character_Init(chara, a1, a2)
   (0x018b3a60) is now declared and stubbed in Character_Init.c: TrgCharacterShapeSet passes its own
   fifth then fourth argument; (bp) FogClearColor (main.c) is now shared through main.h.
   **game/fileio.c (partial, 2026-11-10, slice 1: 32 of 40 functions)** -- the bank queue (FQue[20]:
   FSMltLoad, FSMltInsertQuePriority, the cancels, IsQueWaiting / FsIsQueExisting, FSMltLoadStatus /
   _End / _EndWaitFile / _EndWaitAll), the bank table (FSMltInit / FSInit / FS_work_init, the finds,
   FSMltSetDrawBank, SceneEndBank), the whole-file queue (sPreLoad[6]: FSLoad, FSLoadEnd, FSLoadEndWait,
   FSLoadCancelQue, FSLoadProc), FileSoundServer(_DontMltLoad), FSFileType, fs_error, DebugBankDisp
   (architecture.md "The bank loader"). **Slice 2 (2026-11-10): FSMltLoadProc, Extract, ReserveBank
   written; the first global bank loads (TrgGLBLSceneLoad) and the run now crashes in scene_init
   (main.c, reading (*tbl)->baseobjCount) -- THE BLOCKER, item 1 (bv).** Left in fileio.c:
   ~~SceneEndBankMovie, the sorts, Round2ChangeSectInfo~~ (slice 3, 2026-11-10); Round2ChangeChar.
   (bv) ~~ahRelocateFileObject~~ written 2026-11-10 (game/ahRelocateSunPs2ObjectArry.c slice 1, with
   ahRelocateBehavior and ahRelocateSunPs2ObjectArry; architecture.md "The file relocator"): the global
   bank now relocates without a fault, and scene_init faults one step later (main.c, reading
   (*tbl)->partsHeader as a host pointer: it gets the counts at ALL_TABLE +0x38 / +0x3c, 8 and 0xc --
   real data, so the relocation is plausible). **THE BLOCKER is now the EE-layout conversion below**: one
   session, as a host task on game/main.c (queued at the top): ALL_TABLE's fields become u_int (the
   relocator proves every word 0x10..0xe0 but +0x14 / +0x38 / +0x3c is a pointer), every reader goes
   through BankEePtr (main.c ~45 sites, Character_Init.c 5, sound.c 3, main2.c 2, fileio.c 1); ALLobject[]
   and LinkMatrixTree / asBuildMirrorMatrixTree* take u_int lists; DRAW_OBJ_ENTRY (the sun object record,
   +0 draw object, +4, +8, +0x14, +0x18 pointers), TEX_INFO, PARTICLE_ENTRY and PARTICLE2_ENTRY go to EE
   layout with u_int pointer words (the relocator's offsets are their pointer fields). Was: **Next: ahRelocateFileObject (game/ahRelocateSunPs2ObjectArry.c, CT3 0x0183e710, a stub) and
   ALL_TABLE in EE layout ((f) settled by (h)'s rule, Claude's call 2026-11-10, the user may overrule):
   the bank image keeps the EE layout, so All_table_TBL[bank] = image + 0x80 must be read as u_int words
   through BankEePtr; ALL_TABLE's pointer fields become u_int and every reader (main.c, Character_Init.c,
   sound.c) goes through BankEePtr; the lists (objOpaque / objTrans / objAfter / particles*) are arrays of
   EE words, and DRAW_OBJ_ENTRY / TEX_INFO ((i), layout host) must follow. ahRelocateFileObject(image,
   persona) was declared host-side as (u_char *image, int persona): it relocates to the EE address
   BankImageEE[] of the bank holding the image (fileio.c fills it before the call; a global bank's EE
   address is GLOBAL_BANK_AREA_EE + its offset in gGlobalBankArea).** Queued at the top (tus.py queue).
   **Done 2026-11-10 (host task game/ct3_types.h, scheduled run): the EE-layout conversion.** ALL_TABLE,
   DRAW_OBJ_ENTRY, TEX_INFO, PARTICLE_ENTRY, PARTICLE2_ENTRY and PARTICLE2 are layout ee (ct3_types.h,
   static_asserts); every pointer word is a u_int read through the new BANK_PTR(type, bank, word)
   (BankEePtr with a cast); ALLobject[] is u_int * (a host pointer to a list of EE words, as are
   osTotalObjTable's sorted lists); ctrlNow / timNow in the main_rtn family compare EE words;
   LinkMatrixTree and ssPartMotionBlurInsertObjToDrawTable (stubs) take u_int lists -- their TUs need
   the bank to resolve the entries (add it, or resolve through the list's owner). A smoke run now runs
   scene_init to the end (all three draw lists walked, sunSetupTBufferData / AmbientAlphaInit reached)
   and the frame loop's updates; **THE BLOCKER is now main.c:2106**, the actress light: ActressMP is
   still NULL (main2's TrgCharacterShapeSet sets it only once Alyssa's shape bank has an ALL_TABLE, and
   that bank never loads in the smoke run): find why the shape bank does not load (FSMltLoad of the
   character shape, fileio.c) rather than guard the read -- the ROM reads ActressMP unguarded. Open:
   (bx) ALL_TABLE's end is unknown (the relocator reads to 0xe0 at least); +0x38 / +0x3c are named
   count38 / count3c (the +0x30 / +0x34 lists' lengths) until their reader names them; (by)
   PARTICLE2 +0x6c (obj6c) is relocated and unread; (bz) TEX_INFO.tim is resolved with the entry's own
   bank: a texture shared from another bank would need that bank -- none seen yet.
   **game/ahRelocateSunPs2ObjectArry.c slice 2 (partial, 2026-11-10):** the animation pass
   (asUpDateSceneAnimation, asUpDateGAobjectsAnimation, asCheckAnimationExecutable_Scene,
   asCheckNowMoving, asForceOperateAnimation_Scene / _Character), the shadow init (asShadowDrawing_Init /
   _Disable / _DetachLightPosition, Shadow_OnceInit, asGetShadowMatrixGlobalView), asInitMirrorMatrix,
   asGetBeziePosition, ExecuteAnimationHistory_init (architecture.md "Animation and shadows"). A smoke
   run reaches the new stubs asMakeStraw / ExecuteAnimation / asSeekBarAnimation / asSearchObject, then
   still crashes at main.c:2106 (ActressMP, unchanged). Next in this TU: BuildMatrixTree (0xa38, VU0)
   with BMT_01 / BMT_20, asMakeStraw, ExecuteAnimation (0x16bc), Execute_ExecuteAnimationHistory. Open:
   (ca) asMakeStraw and ExecuteAnimation take the bank as a PORT first argument (their words are the
   bank's EE words); the history records (lpEAS, 0x10 bytes: anim, tbl, a sign pair for AbsSignI32, a
   word stored to partsHeader +0x28) must carry the bank too -- whoever writes the history adder
   decides; (cb) BANK_INFO +0x30..0x70 (shadowVec, shadowLength, shadowColor, shadow64 / 68, shadowOrder,
   shadowDraw, shadow58 / 6d / 70) are named from the init and _CheckTransMode only; (cc)
   matrix_global_view_for_shadow is a symtab local, assumed this TU's (made static here); SkinWork and
   the EAS / shadow globals are defined here or in lib/unassigned_data.cpp with their EE addresses;
   (cd) ALL_TABLE +0x50 named `straw` from asMakeStraw's call only.
   **Slice 3 (partial, 2026-11-10):** asSeekBarInitializeForX / FreeForX / Animation, asSeatArrange_Init,
   BMT_01, ExecuteAnimationHistory_Free; asSceneAnimationStart (14 args, from the disassembly),
   asSeatArrange_ObjectMatchGlid / _FlameInit stubbed. **Next in this TU: BuildMatrixTree (0xa38) and
   BMT_20 (0x848)** -- read BuildMatrixTree from mipsdis.py, not the decompiler: it drops arguments
   (a `gaMulMatrix(dst, parent)` with two) and the record +0x8c parent matrix pointer is an EE word
   (BANK_PTR) or a host pointer InitMatrixTree / LinkMatrixTree set: decide with those (LinkMatrixTree.c);
   then asMakeStraw, ExecuteAnimation, Execute_ExecuteAnimationHistory. Open: (ce) **decided (Claude's
   call)**: ALL_TABLE +0xe8 holds a _SunAlloc'd SEEKBAR_X in the ROM; a host pointer does not fit the EE
   word, so the host keeps it in `SeekBarX[bank]` (PORT) and the word stays 0 -- a reader of +0xe8 in
   another TU must use SeekBarX; SEEKBAR / SEEKBAR_X are host layout (host-only memory); (cf) who fills
   SEEKBAR +0x20..+0x30 (hash, planePart, chara, anims) is not read yet: init zeroes them, so the bars
   stay idle until that TU; (cg) **MODEL_PARTS (ct3_types.h) is indexed from the parts header, but the
   records start at header + 0x40** (asFindModelParts answers header + 0x40 + i * 0x380; BMT_01 walks
   from +0x40 to a negative index): every MODEL_PARTS offset is the record's + 0x40, so fields at record
   0x340.. (BMT_01's view matrix) do not fit it -- BMT_01 uses record offsets (REC_*, PARTS_REC); main.c's
   `ActressMP + 0x1c0` is a record offset (= MODEL_PARTS 0x200), not MODEL_PARTS.matrix. A later session
   could re-base MODEL_PARTS on the record (a 0x40 header struct + records) across its users; (ch)
   asSceneAnimationStart's arguments 7..14 are unnamed (the seek bar passes the mode as one of 7 / 8 / 9
   set, and 0, 1, 0, 1, 0 after); (ci) the smoke run is not deterministic: of two 15 s runs one crashed
   at main.c's ActressMP read (frame ~1560) and one logged the same crash but ran on (the crash handler
   or the frame thread), so the ActressMP blocker stands either way.
   **Slice 4 (partial, 2026-11-10):** BuildMatrixTree (from the disassembly), asBehaviorStopAnimation,
   asSceneAutoStartAnimation, asStartAnimationFromTop, asShadowDrawing_Fin, asDrawShadowVolumeLoop,
   asGetForceMatrixOperationFlag, asMirrorPlaneCalc (vPVbk / vPPbk defined here),
   asGAobjectControlPolygonDisplayRotateFlagReverse; absi written early in QuatSlerp.c (BuildMatrixTree
   indexes by it); BMT_20, the shadow volume functions, asMakeWatchMeMatrixToPosition and
   OmniRangeChange stubbed. ~~**game/LinkMatrixTree.c is queued first**~~ written 2026-11-10 (verify: all 4 functions -- InitMatrixTree, LinkMatrixTree, ahTimeInit, ahTimeMaintenance; the parent words are MATUNIT_EE or BankEeAddr as (cj) decided, PartsParentMatrix's temporary 0 line removed; the tick globals moved out of unassigned data into LinkMatrixTree.h. Open: (cn) LinkMatrixTree finds its bank by matching pModelPartsHeader[] (PORT PartsHeaderBank): every caller passes pModelPartsHeader[bank]; (co) record +0x100 / +0x140 are the matrices a part's ga draw object reads through its words +0x58 / +0x5c (DRAW_OBJ_MATRIX / _MATRIX2), named from this one use -- the ga draw TU names them; a static part (kind 1) draws through its parent's; (cp) PARTS_KIND 3 keeps REC_SCALE (InitMatrixTree), kinds 0xd, 0x11, 0x15, 0x16 never hold their view matrices (REC_VIEW_HOLD 0) -- unnamed; record +0x2d0 / +0xd0 unread; (cq) ahTimeInit sets gGlobalNextTicks (not its interval) to 5 and takes 5 off gGlobalNextTicksInterval: kept as the ROM has it): a part's record
   +0x8c parent word is 0 in the file and LinkMatrixTree fills it (matUnit for a root, else the parent
   record's +0x180 / +0x200 matrix, same bank); until it is written PartsParentMatrix reads 0 as matUnit
   (a marked temporary PORT line: remove it with LinkMatrixTree). Next in this TU: BMT_20 (0x848),
   asMakeStraw, ExecuteAnimation, Execute_ExecuteAnimationHistory. Open: (cj) **decided (Claude's
   call)**: the parent word stays an EE address (bank data, decision h) -- LinkMatrixTree must store
   MATUNIT_EE (0x01939e40) for matUnit and BankEeAddr(bank, matrix) for a part; (ck) BEHAVIOR /
   BEHAVIOR_TRACK (ahRelocateSunPs2ObjectArry.h) are named from four small functions; asBehaviorStop /
   asSceneAutoStartAnimation find the behaviour's bank by the nearest BankImage[] below it (PORT
   BankOfHostPtr, also used for the polygon flag lists) -- a bank image size table would make it exact;
   (cl) BANK_INFO +0x58 is the bank's _SunAlloc'd shadow block: the host keeps it in ShadowBlock[bank]
   (PORT, like SeekBarX), so the allocator (asShadowPolygonPositions or asDrawShadowVolumeInit) must use
   it; (cm) the record fields BuildMatrixTree names (REC_DEPTH, REC_LPOS, REC_LINK_*, REC_WATCH_*,
   REC_PARENT, REC_LOCAL, REC_OFFSET, REC_BASE, REC_LIGHT, REC_BLEND, REC_PREV_SCALE_X, PARTS_KIND_OMNI /
   DIRECTIONAL / WATCH / NO_POS / LINK, WATCH_*) come from this one function; REC_FLAG_HOLD is the same
   bit as PARTS_FLAG_FOG -- one of the two names is wrong; asShadowDrawing_CheckTransMode's modes
   (SHADOW_TRANS_*) are named from asDrawShadowVolumeLoop's calls only.
   **Slice 5 (partial, 2026-11-10):** asCopyDoubleBuffer / asCopyDoubleBufferL2V (BANK_INFO.drawOnPrev,
   CHARACTER.flagsPrev, gNoDraw), BMT_20, Execute_ExecuteAnimationHistory (EAS_RECORD, FrameCounter;
   AbsSignI32 written early in QuatSlerp.c), asSearchObject (SEARCH_OBJ, ALL_TABLE.search),
   asSeatArrange_FlameInit / _ObjectMatchGlid, asCheckAnimationExecutable_Character,
   asShadowDrawing_Enable / _SetTransOrderBW (architecture.md "Double buffer, seats, searching parts").
   ahCos, ahAcos, asMakeRotMatrixByQuartanion (QuatSlerp.c) and gaInnerProductXYZ (gaInit3dlib.c) are
   new stubs. **Next in this TU: asMakeStraw (0xcdc, VU0 + quaternions: read it from the disassembly,
   a slice of its own), then ExecuteAnimation (0x16bc)** -- both reached every frame. Open: (cr)
   **decided (Claude's call)**: EAS_RECORD is host layout {anim, tbl, sign, step, bank} -- the ROM's are
   0x10 bytes; the history adder (not found yet: whoever _SunAllocs lpEAS) must write these and set
   iEASidx; (cs) the parts header +0x20 / +0x24 / +0x28 (PARTS_HDR_ANIM_*), ALL_TABLE.ctrl's first list
   (OBJ_CTRL_ATTACH_*), SEARCH_OBJ's fields, the seat grid's bits and SEAT_* are named from one use each;
   (ct) asSearchObject's "nothing seen" flag is set once per call and never cleared (the ROM's), so one
   blind part makes every later part of the bank w 0 -- kept; (cu) asCopyDoubleBuffer / asSearchObject
   / asSeatArrange_* read All_table_TBL and the eye parts unchecked in the ROM: the host skips a NULL
   (PORT lines); (cv) CHARACTER_FLAG_SHADOW is used as "in use" by asSearchObject and the double buffer,
   with (ax): rename it when the character TU reads the bit.
   Slice 6 (2026-11-10, asMakeStraw, SetExecuteAnimationHistory, asGetBehaviorPlayableTicks): (cw)
   asMakeStraw reads parts A / B at record +0x1c0 (REC_MATRIX_PREV, BuildMatrixTree's kept world
   matrix) and +0x20 (REC_LPOS) from the disassembly -- if straws lag a frame on screen, that is the
   ROM's; (cx) STRAW / STRAW_TABLE / STRAW_SKIN and the ga object's +0x48 / +0x60 / +0x64 (GAOBJ_*,
   normals after a 0x10 header) are named from this one use, and the ga object and its arrays are
   read as bank data (BANK_PTR, decision h): if gaInitDrawObject builds them on the host instead, the
   skin list needs the host form; (cy) QuatSlerp(q, q0, q1, ts) takes ts = {1 - t, t},
   asDelSqueezeInformation(out, a, b) answers the twist, ConvertMatrix2Quat / ConvertQuat2Matrix /
   ssCalcInversMatrix2 take (out, in): confirm in QuatSlerp.c / Convert.c / ssCalcInversMatrix2.c (new
   stub TUs); (cz) SetExecuteAnimationHistory is the history adder (cr): callers EA_100 and
   ExecuteAnimation pass the bank in front (PORT); BEHAVIOR_TRACK +0x10 is now `playable`.
   Open from slice 7 (2026-11-10): (da) asBuildDOFMatrixTree reads a matrix at record +0x1c0
   (REC_DOF_MATRIX) and a scale diagonal at record +0x4c / +0x5c / +0x6c (REC_DOF_SCALE_*), named from
   that one use; kind 3 (PARTS_KIND_3) is skipped there, meaning unread; (db) ALL_TABLE +0xcc is `mirror`
   (MIRROR_LIST, MIRROR 0x50 bytes: part index + three plane points at +0x10); mrr_pActvMirrorMO / _MP
   are host pointers defined in this TU (asMirrorCulling and the mirror tree read them); (dc) EA_001's
   track kinds 0x0b / 0x0c / 0x1f / 0x20 (TRACK_KIND_*), TRACK_KEY_RAW, the part hash and
   BEHAVIOR_GROUP +0 `part` are named from EA_001; InterpolateAnimationKey (0x0184dbc0, this TU, now a
   stub) is assumed to take (track, dir) and answer 0 when the track ran out; asZipTrans2Trans_XYZ1 /
   asZipQuat2Quat (QuatSlerp.c) take (out, in[, shift]).
   Open from slice 8 (2026-11-10, ExecuteAnimation's dispatch, EA_100): (dd) InterpolateAnimationKey
   answers 1 when the track's time reached its end (its decompilation: clamped there in flags & 3
   modes 0 / 1), and EA_100 reads it so (1: stop or restart the track); EA_001 sets its done byte on
   0 -- check EA_001 against its disassembly (the slot bytes may mean "still running"); (de) the part's
   pose slots PARTS_ANIM_ROT / _POS / _BASE_ROT / _BASE_POS / _VECTOR (0x280..0x300), PARTS_ANIM_DIRTY
   (byte 0x43 bit 0), TRACK_KIND_VECTOR 0x0d and ANIM_CTRL_HOLD / _ONCE (header mode byte 1 / 2) are
   named from EA_100 alone; the header words PARTS_HDR_ANIM_ABS / _SIGN / _STEP are ExecuteAnimation's
   ticks left / direction / step length (AnimWeights); (df) ExecuteAnimation's general body (any flag
   mix but one-hot, 0x0184a688..) is an ALYSSAPAN_STUB inside the function: write it when
   asCharacterAnimationExecute / asResetCharacterPosition (the callers that can mix flags) are written;
   its shape kinds ('*', '3', '>', '=', 'H', '4', '"', '!', ')') read ALL_TABLE +0x18 / +0x30 / +0x40 /
   +0x44 lists, as EA_010 will; (dg) asZipVECTOR2VECTOR_XYZ1 and asInterpolateVector (QuatSlerp.c)
   written early from their disassembly; QuatSlerp / ConvertMatrix2Quat / ConvertQuat2Matrix /
   asZipQuat2Quat / asZipTrans2Trans_XYZ1 / gaMulMatrix are still stubs, so EA_100's poses are zeros
   until those TUs land.
   Open from slice 9 (2026-11-10, InterpolateAnimationKey / IAK_0, the play range, behaviour lookup,
   shadow setters, asCopyDoubleBufferL2W): (dh) (dd)'s reading confirmed for both functions (1 = the
   time reached `playable`: forwards at or past it, backwards at or below it); EA_001's done byte on 0
   is still unchecked; (di) BEHAVIOR_TRACK +0x14 is `rangeStart` (asChangePlayRange; a looping track goes
   on from it) and `playable` is the range end; flags & 3 = TRACK_PLAY_ONCE / _ONCE_REVERSE / _LOOP /
   _LOOP_REVERSE, bit 0 also the direction; TRACK_SLOW_STEP (value0e == anim_play_slow snaps the time
   to 500); the kinds '"' '!' ')' '*' '3' '4' '=' '>' 'H' and ANIM_SUBKEY (0x18-byte sub-records of the
   vertex kinds, +8 an EE data word, +0xc a count, +0x14 'H''s short) are named from this one function;
   (dj) written early in QuatSlerp.c from the disassembly: asMemCopy128c / 16c, asQuat2ZipQuat,
   asZipQuat2Quat, asQuatSlerpZipVECTOR, asTrans2ZipTrans_XYZ, asZipTrans2Trans_XYZ1,
   asInterpolateZipTrans_XYZ, asZipVECTOR2VECTOR_XYZ0, asVECTOR2ZipVECTOR_XYZ, asInterpolateZipVECTOR_XYZ,
   asZipUV2UV, asInterpolateNumber (the old stubs of asZipQuat2Quat / asZipTrans2Trans_XYZ1 carried the
   wrong addresses 0x0185c6d0 / 0x0185cdb0: fixed). Still stubs there: asInterpolateZipUV,
   asInterpolateZipFloat0_7_9X / 1_1_14X, asContInterpolateZipNormal_XYZ / ZipTrans_XYZ / Vector (the
   vertex kinds); QuatSlerp itself (Convert.c) is still a stub, so rotation keys blend to zeros;
   (dk) asShadowDrawing_SetShadowAngleLockParameter takes its floats in f12 / f13 and the bank in a0:
   the C order (bank, a, b) is a guess for ssGrpPortalCheck.c's call site; BankInfo +0x64 / +0x68 are
   its two floats, +0x6d the angle lock, +0x6c (shadowOrder) is what SetShadowValue sets;
   (dl) asFindBehavior / _ByCharacter answer host BEHAVIOR pointers (the list words resolved by
   BankOfHostPtr's bank); BEHAVIOR +0 is its 64-bit `hash`.
   Open from slice 10 (2026-11-10, the shadow pass order, asSceneAnimationStart,
   asUpdateAnimation_Matrix, the play rate): (dm) CHARACTER +0x350 is the EE word of the character's
   motion table (0x30-byte records: hash, +8 the motion's hash), written by asInitPlayerBehaviorControler
   (wideXcmd_DebugPrint.c, a stub): asSceneAnimationStart's SCENE_ANIM_CHARACTER + toEnd path reads it
   through CharaMotionTable(), which answers NULL (an empty table, index 0) until that TU gives the word
   a host form; (dn) asShadowDrawing_CheckTransMode's mode-0 sort reuses one register as the character
   loop's index and the count of shadowOrder changes (the disassembly, 0x01847918..0x01847a1c): a change
   skips the next character and iShadowTransModeFin always ends 1; kept as the ROM does it -- a smoke
   run with several shadowed characters of mixed order is where it shows; (do) the two hashes of
   CHARA_AXIS_HASH_ROW1 / _ROW2 (asUpdateAnimation_Matrix, CHARACTER.refractAxis) have no names yet;
   (dp) BEHAVIOR_TRACK +0xe is `playRate` (percent; was value0e): asSceneAnimationStart sets speed * 100,
   or 0 with toEnd. Left in this TU: EA_010, the general body of ExecuteAnimation, asMirrorCulling,
   asShadowPolygonPositions and asStockShadowVolume (one piece: ga draw objects with the
   ShadowVolume1_* VIF data, through the host draw module), asDrawShadowVolume / _Init, asShadowEnd,
   asShadowStart and the rest of the brief's list.
   Open from slice 11 (2026-11-10, the mirror trees, asMirrorCulling, the shadow volume pass): (dq)
   CAMERA +0x40 is named `view2` from one use (asBuildMirrorMatrixTree multiplies Cam[6]'s in where
   the DOF tree has matrix_global_view): the camera TU names it; (dr) MIRROR holds four corners at
   +0x10 (not three and a pad): asMirrorCulling projects all four; (ds) gaMulMatrix must allow
   out == b (asBuildMirrorMatrixTree: m2 = view2 * m2; the VU code works in registers) -- the
   gaInit3dlib.c TU must keep that; (dt) asShadowStart passes sunSetupTBufferData a TEX_INFO whose tim
   is the ELF data address 0x01937c80 (tex_attr_A$2750, 512 x 448 log2 9 x 9, the shadow buffer read
   as a texture), not a bank address: the QuatSlerp.c TU that writes sunSetupTBufferData resolves it
   (bytes in this slice's comment); (du) ALL_TABLE +0x6c is `shadow` (the relocator's 7-word table with
   8 key tables; word 0 the volume's ga draw objects) and BANK_INFO +0x50 `shadowFrame` (the
   FrameCounter of the stock), both named from asDrawShadowVolume: asStockShadowVolume confirms them;
   (dv) the volume object's kind byte (+1): 1 / 5 subtract (SDW_drawctrl_S), 2 / 6 add
   (SDW_drawctrl_A), 6 ends the list; what tells 5 from 1 and 6 from 2 is asStockShadowVolume's;
   (dw) asDrawShadowVolumeInit's four stores into SDW_drawctrl_A +0x10..+0x1c (ShadowVolume1/2 tex_st
   and vertex colour addresses) are overwritten by gaSetupDrawCtrlData's packet head at once: dropped,
   unless the ga TU finds the head keeps them. Left in this TU: EA_010, the general body of
   ExecuteAnimation, asShadowPolygonPositions + asStockShadowVolume, asShadowEnd, and the brief's rest.
   Slice 12 (2026-11-10): (dx) TRACK_FLAG_STEP_FROM_RUNS (bit 0) is what asCheckAnimationDirection_X
   answers: probably "plays in reverse" (step starts at the end); rename when IAK's reverse path
   confirms it. (dy) asSceneAnimationStop_S stops a character's motion slots only under
   CHARACTER_FLAG_REFRACTION: the bit likely means more than refraction (an "active / drawn"
   character); CHARACTER +0x398 is short[6] motion slots (CHARA_MOTION_SLOTS). (dz) A first read of
   asStockShadowVolume for (dv): mode 2 flushes the pending odd plane (_iPcnt_3177: kind 5, gMP) and
   odd quad (_iMcnt_3178: kind 6, gMM) as one-face volumes (ShadowVolume1_*); mode -1 stocks 4 corners
   into gMM and emits a two-face volume (ShadowVolume2_*, kind 2) on every second call. Both write a
   gaCopyObject2 of the template at 0x01937ba0 and the face normal (cross of two edges, rsqrt).
   Slice 13 (2026-11-10): (ea) the shadow volume's stock is written (asStockShadowVolume, settles dv /
   dz: plus quads kinds 1 / 5, minus quads 2 / 6; the flush's lone minus quad is the list's last, no end
   mark). Its objects keep the ROM's EE addresses of ShadowVolume1/2 index, ST and colour data and the
   template's light-work words (0x01937820 / 0x01937830) and 0x01941840: the gaBGT_DrawObject TU must
   resolve them (as tex_attr_A in asShadowStart). (eb) The caster positions' w is the ROM's, unclamped:
   segment rings carry w = 1 + radius (the vmulx.xyzw / vadd.xyzw fields), polygon vertices w0 = M * v's
   w; whether the VU1 volume microcode reads w decides if it matters. (ec) SP_ANCHOR_HASH
   0x74dda77d0cbcf0c2 (a character bank's shadow anchor part) has no name yet: hash the shape files' part
   names (ahGetHash) to find it. (ed) asViewMatrix2PosRUF is called with one buffer for all four outputs
   (the camera position): whichever it writes last wins, so its TU decides what asShadowPolygonPositions
   tests against. (ee) asCheckAnimationState_S indexes a 5-byte stack array by the track's playing byte
   unchecked (the host bounds it); CHARACTER +0x3b0 is a 42-entry motion request queue with modes at
   +0x404 (CHARA_MOTION_QUEUE*): the motion TU names them. gaCopyObject2, gaInnerProductXYZ and
   asVectorLength3D were written early in their TUs.
   Slice 14 (2026-11-10): asCheckAnimationFlamePassing_S, asShadowEnd and EA_010 written; left in the TU:
   ExecuteAnimation's general body (a marked stub, no caller reaches it yet). (ef) asCheckAnimationFlamePassing_S's
   scene branch tests a backward track's wrap with its first condition again (the decompiler's reading,
   kept: such a track passes only the frame it came from); PASS_DIR_* (1 forward, 2 backward, 3 both) and
   PASS_FRAME_ANY (-10) are named from this one use; the argument order of its caller
   ssTrigCheckConditions (unwritten) is the ROM's slots. (eg) asShadowEnd: asShadowDrawing_CheckTransMode(1)
   < 2 lays the shadow buffer over the screen as 8 x 7 sprites, >= 2 as four Gouraud fans about the
   projected BANK_INFO.shadowAnchor (renamed from pad40); BANK_INFO.shadowOrder is also the overlay's
   strength in percent (negative: lighten with W, positive: darken with B) -- rename it when its other
   users are next touched; the fans clamp x to X + W / 0.832 (615) while their corners stop at 512, as the
   ROM. The overlay needs sunSetupTBufferData (QuatSlerp.c, a stub) to bind the shadow buffer as a texture.
   (eh) EA_010 names ALL_TABLE +0x30 copyWords, +0x40 objGroups ((first, count) pairs), +0x44
   objGroupIndex (short indices into objOpaque), ANIM_SUBKEY +0x10 obj, the ga object's +0x78 UV offset
   and the colour data's alpha bytes (+0x10 + 4 i + 3) from this one use. TRACK_KIND_COPY stores a
   copyWords word (an EE value, unresolved) through the sun object record's word 2: the gaBGT_DrawObject
   TU decides what that word is. EA_010 keeps the ROM's one stack vector across tracks (`held`, zeroed
   where the ROM's lanes are stale). The QuatSlerp.c Cont* writers, asZipTrans2Trans_XYZ0, the asDR_*
   GS words and asZipFloat0_7_9X2Float were written early.
   Slice 15 (2026-11-10): ExecuteAnimation's general body written (any mix of play / replay / matOnly
   other than one alone): **the TU's code is complete (verify)**; EA_100 and EA_010 now share its per-track
   helpers (PoseTrack, MatKey, ReplayTrack). (ei) The general body shifts every track's time1c *before*
   InterpolateAnimationKey, EA_100 *after* (both as their ROM bodies): a pass test
   (asCheckAnimationFlamePassing_S) after a mixed call sees the old time where EA_100's sees the new; kept.
   (ej) The ROM's one stack vector is shared by the pose kinds' temporaries and the replay kinds (the
   general body's FLOAT0_7_9 blend reads lanes y..w a pose track wrote there): the port passes one
   scratch vector through both, zeroed at entry where the ROM's is stale. The two written callers pass
   one flag each; asCharacterAnimationExecute and asResetCharacterPosition (unwritten, other TUs) are
   the ones that may mix them: when they run, the shot decides.
   (bw) Extract is untested against a known image: the first scene_init that runs on relocated data is
   its test (a wrong decode shows as garbage tables); a harness decoding one CT3X file from CT3PACK.DAT
   beside the ROM's (eerun.py) would prove it sooner.
   **game/fileio.c slice 3 (partial, 2026-11-10):** the sorts (CharacterSort, objSort,
   objSortWithBlurIdx, SortAlphaObjects: CAM2OBJ rows in pCam2objSort, a merge sort far to near that
   is not stable -- on equal distances the second half's row goes first), SceneEndBankMovie (parks
   banks at BANK_STATE_MOVIE_PARKED + i instead of clearing them), Round2ChangeSectInfo. Left:
   Round2ChangeChar (0x878). The smoke run passes CharacterSort and still crashes at main.c:2106
   (ActressMP NULL: asFindModelParts, QuatSlerp.c, is a stub that answers NULL; so is the new
   asVectorDistance stub, which answers 0, so every sort keeps its input order). Open: (ek)
   CharacterSort counts a character only with both CHARACTER_FLAG_REFRACTION and _SHADOW set: the two
   bits are more likely 'exists' / 'drawn' than refraction / shadow -- rename them when their setter is
   read; (el) PART_BLUR_OBJ (+0x110 listIndex, +0x114 sortedIndex), ALLpartBlurObj[2][86][48],
   NumPartBlurObj[2][86] and aftSortALLpartBlurObj (PORT: 49 slots, the ROM writes [count]) sit in
   lib/unassigned_data.cpp until the ss part-blur TU (ssSetTexUVforCubeMap.c) owns them; (em)
   PARTS_FLAG_SORT_CENTER (0x4), DRAW_OBJ_CENTER (+0x10) and the record's +0x20 sort point
   (REC_SORT_POINT, MODEL_PARTS +0x60) are named from the sorts only; CharacterSort and main.c's
   actress light both read record +0x1c0 (REC_MATRIX_PREV) with the +0x20 point; (en) CharacterSort's
   rows and CharacterSortBuff got one PORT row each (the ROM writes row / slot 35 when all 35 count).
   **Slice 4 (2026-11-10, verify): Round2ChangeChar -- fileio.c's code is complete.** The ROM's 150
   unrolled Round2ChangeSectInfo calls became three tables (Alyssa's 19 files, four costumes' 19, 25
   shared pairs) in ROM order. Open: (eo) the argument's meaning is read from the file names only:
   0 ROUND2_NEW_GAME (FlInit, stop), 1 ROUND2_CLEARED (FlInit, then the shared swaps), 2..5
   ROUND2_COSTUME_1..4 (CAL1..CAL4, no FlInit first: a costume picked after another keeps the earlier
   swaps of files the new one does not cover -- none, as all four tables cover the same 19 names), any
   other value only the shared swaps; hhXcmd_DressUp's TU says what its value is.
   Open: (bq) BANK_INFO renamed from the loader's use: +0 `state` (was `count`; BANK_STATE_*, a group's
   later banks hold their place 2..n), +0x20 `drawOn` (was `loaded`: FSMltSetDrawBank's mode), +0x1c
   `image` (EE address), +0x24 `handle`; every user was updated. A bank's type is the FS_LOAD_* kind it
   was loaded as (FSFileType: 0 GLOBAL, 1 SCENE, 2 / 3 PERSONA), so BANK_TYPE_SHADOW is the scene bank,
   BANK_TYPE_2 the shape bank and BANK_TYPE_NO_ROOM_SOUND the behaviour bank: rename them to
   BANK_TYPE_SCENE / _SHAPE / _BEHAVIOR in one pass ((j) settled); (br) FSMltInit gives banks 6..85 EE
   slots 0x00410000 + (i - 6) * 0x40000 -- the first twelve overlap the GLBL heap (EE 0x00400000, 3 MB):
   FSMltLoadProc / ReserveBank say how the ROM keeps them apart; the host gives the slots their own 20 MB
   (gBankSlotArea) and fills BankImage / BankImageEE for them; global banks get theirs when
   FSMltLoadProc allocates them (BankImage[i] from _SunAlloc, which SceneEndBank now frees); (bs)
   settled 2026-11-10: FQUE +0x00 is `dir` (FSMltLoadProc fills it with FSFileType, opens dir + name); (bt) BANK_FLAG_SCENE_INIT (flags bit 0: SceneEndBank
   calls scene_end) -- settled: FSMltLoadProc sets it on the group right after scene_init; (bu) FSMltLoadStatus's
   answers: main2.c reads "non-zero" as "still loading", but FS_STATUS_LOADED (1) is non-zero too, so
   main2's waits also run for loaded files (harmless: FSMltLoadEndWaitFile returns at once).
   **game/QuatSlerp.c slice 1 (partial, 2026-11-10; the queue was empty, refilled with it: asFindModelParts
   was the ActressMP blocker):** the part lookup (asFindModelParts / _ID with the parts header's four
   cache words), ahGetHash (a 64-bit XOR of 7-bit characters rotated 7 apart) and its inverse
   asConvertHash2String, asCheckBankID / _Type, asFindCharacterStruct, asVectorDistance (squared),
   ahAcos (the ROM's ah_acos_table carried as src/game/ah_acos_table.inc: a host acos differs in 866 of
   4098 entries), ahCos / ahSin, the camera read-back (asViewMatrix2PosRUF / CamRUF,
   CameraMatrix2CameraPos / Trg3m, asFocalLength_*), the 1.1.14 / 0.7.9 / UV packed blends,
   BankID_TO_CharID / CharID_TO_BankID / GetRoomName_file, RoomFlagClear / BRANCH_CLEAR / RoomInFlagOn
   (RoomNameTbl / RoomIdTbl from the ELF), OmniRangeChange, AmbientAlphaInit, ControllerClatteringInit.
   **The ActressMP crash (main.c:2106) is gone**: a 15 s smoke run ends without a fault (27,000+ frames);
   the shot stays black (the ga draw calls are stubs). Open: (ep) **(ec) settled**: the hashes read back
   -- 0x3161c34c2 is "Bip01" (sound.h's PARTS_HASH_SOUND_ORIGIN), 0x74dda77d0cbcf0c2 "BasePoint"
   (EA_001_PART_HASH / SP_ANCHOR_HASH); the other two cached names are longer than 9 characters (ends
   "...sa_ta", "...sa_se"): hash the shape files' part names to find them; any other u_long hash
   constant can be read back the same way (py: 7-bit groups from the low end); (eq) asUV2ZipUV_R stores the two
   floats themselves (8 bytes), so asInterpolateZipUV's output is a float pair where its inputs are
   packed shorts -- EA_010's out + 8 slot must be 8 bytes of floats (check when the vertex kinds draw);
   (er) CHARACTER +0x90 renamed roomHash (CharID_TO_BankID, RoomInFlagOn read Character[0]'s); MODEL_PARTS
   +0x120 is `alpha`; SCENE_ALPHA_ANIM (5 words) and CLATTERING (+0x10 id) are named from their inits
   only; REC_OMNI_* (record +0x30 / +0x34 / +0x38 / +0x2ec) from OmniRangeChange; (es) gaSinCos(s, c,
   angle) is a new stub in ga/gaSinCos.c (ahSin / ahCos read the first / second answer): every ah angle
   helper answers 0 until it is written; gaInversMatrix is still a stub, so the camera read-back answers
   zeros. Left in this TU (frontier): GetAreaNumber (next reached), sunSetupTBufferData /
   sunBGT_*_VIF1, PanicModeInit, AmbientAlphaUpdate, the warp objects, asMakeRotMatrixByQuartanion,
   QuatSlerp, ControllerClatteringUpdate and the brief's other 100.
   **game/QuatSlerp.c slice 2 (partial, 2026-11-10):** GetAreaNumber (RoomNameID tables from the ELF),
   the controller rattles (ControllerClatteringFree / Get / Update / Set, CLATTERING named), DemoSkipCheckEvent,
   PanicModeInit (PANIC_DEF Panicdef, PANIC_INFO +0x28 now float f28), QuatSlerp,
   asMakeRotMatrixByQuartanion, the warp family (WarpObject, WarpObjectUpdate / _Set / _Init,
   asWarpObjectBody; WARP_CMD), tnInit3dlibWork. A 15 s smoke run: no fault, 27,000 frames, shot black.
   Open: (et) the rattle's stick bytes are PAD_DATA +0x1c..0x1f read as [2], [3], [0], [1]: which stick is
   which is gaPadRead's TU (pad_button_read.c); CLATTERING +0x08 (default 0x70) is set but never read
   here; ControllerClatteringSet with no free slot writes slot 10 past the array in the ROM (over
   pad_key_button): the host answers 10 without writing; (eu) TRIG_EVENT_AREA_PASS2 / _PASS3 (0x5db /
   0x5da), AREA_STAGE3_HOME 52, AREA_STAGE4_0A16 47, TRIG_VAR_PANIC (0x1c) and TRIG_EVENT_PANIC_3 / _26
   / _48 are named from this use only; PANIC_DEF's fields by offset until PanicUpdate is read;
   (ev) WarpObject's followers (Character[11] / [12] with flags bit 31) suggest CHARACTER_FLAG_REFRACTION
   (0x80000000) is a wider "accompanies the player" bit: rename when the character TU reads it;
   WARP_CMD.bank2 / mode and the ExecuteWarpObject / 2 argument order come from asWarpObjectBody's calls
   (stubs: confirm when written); WarpObject's prototype (room, mode, chara, partHash, bank) is from its
   body -- ssTrigExecuteActCmd / tkXDTrg_ActorProc_AlsOpn call it. Left (frontier): ExecuteWarpObject /
   2, PanicFilter, XcmdScreenFilter / Skip, DemoScreenFilter (the screen filters), sunSetupTBufferData,
   sunBGT_LoadGSTexClutData_VIF1 / _ExecTBufferData_VIF1 / _SetLoadGSEnv (the texture upload: the next
   blocker for the picture with the ga draw stubs), and the brief's other 92.
   **game/QuatSlerp.c slice 3 (partial, 2026-11-10):** the texture buffers (sunSetupTBufferData,
   sunBGT_SetLoadGSEnv, sunBGT_LoadGSTexClutData_VIF1, sunBGT_ExecTBufferData_VIF1; SUN_TEX_ATTR 0x34 and
   SUN_TBUFFER_DATA 0xd8 in QuatSlerp.h, TEX_INFO's words named: tbp, cbp, clut, CLAMP's six, tfx), the
   panic sounds (EventSoundUpdate, PanicSound, LavenderSound, InvisibleSound), PanicCheck, ssMenuUpdate,
   PlayerControlUpdate. PANIC_INFO +0x24 is the float gauge, +0x28 gaugeMax; Panicdef +0x08 beatFrom.
   15 s smoke run: no fault, 27,360 frames, shot still black. Open: (ew) **the texture still never
   reaches the GPU**: the packets are built byte for byte and the host DMAC drops them; the picture needs
   a host texture module that decodes a SUN_TEX_ATTR (its mipData / clutData EE words, psm) at the
   source and binds it where TEX0.TBP points -- the user's no-GS-emulator rule; whoever writes the ga
   draw path (gaBGT_DrawObject) should key textures by the SUN_TBUFFER_DATA's tex0 word; (ex) the host
   resolves TEX_INFO.tim / clutData with a bank search (SunTexEePtr) plus one hard-wired game-data record
   (tex_attr_A$2750, the shadow buffer read as a texture, 0x01937c80): a new game-data texture record
   from another caller answers NULL and leaves the packet's body unset -- add it there; (ey)
   TEX_TFX_HIGHLIGHT2 replaced TEX_KIND_LIGHT (the same word is TEX0.TFX): main_rtn's light rule keys on
   TFX 3 -- check against a bank whose highlight parts are known; (ez) the panic events and SEs
   (TRIG_EVENT_LAVENDER 0x1a, _INVISIBLE 0x97, _PLAYER_STOP 0x90, PanicCheck's ON / OFF events,
   TRIG_VAR_PANIC_OFF 0x0b, SE 0x23 / 0x24 / 0x43 / 0x45 in slot 4), PANIC_MODE_FULL (mode bit 0) and
   tiVibrationOn's argument names are from this use only; act_small / act_large (0x0193f840 / 44) sit in
   lib/unassigned_data.cpp (Clipping_Portal.c slice 1 left them there: shared with QuatSlerp.c); the
   heartbeat's second SdSetParamSE argument (SD_SE_PARAM_30, level / 2) is unk30 until the Sd TU names it.
   Left (frontier): PanicUpdate, PanicFilter, XcmdScreenFilter / Skip, DemoScreenFilter, MonsterAIUpdate,
   FogAnimeUpdate, ExecuteWarpObject / 2, ssGetBankID, ssStrCmp4Byte and the brief's other ~70.
   **game/QuatSlerp.c slice 4 (partial, 2026-11-10):** the screen filters (PanicFilter, DemoScreenFilter,
   XcmdScreenFilter / Skip: SCREEN_FILTER_OBJ, the 0xa0-byte TRIFAN packets, and their draw controls from
   the disassembly), EventSoundEnd, ssGetBankID / ssStrCmp4Byte, the slide decoder (tnSlideThawing,
   decode, tn_getc, tn_putc: Okumura LZSS), AmbientAlphaUpdate. FILTER_CHANGE_WORK's +0x08 is the
   **target** and +0x18 the **current** colour (renamed from from / to; main.c follows), A R G B; FILTER_ON
   / FILTER_OFF_AT_END / FILTER_MODE_ADD / _BLEND named. PANIC_INFO +0x2c is filterAlpha, Panicdef +0x10 /
   +0x14 / +0x18 filterMax / filterUp / filterDown. SCENE_ALPHA_ANIM is {on, steps, float target[3]}
   (no vitof on the target words: floats). 15 s smoke run: no fault, shot black; AmbientAlphaUpdate now
   reaches LosHitFadeUpdate / DistanceAttenuationUpdate / AngleAttenuationUpdate (stubs). Open: (fa) the
   filter packets carry the GS's fixed 512x448 fan; a host draw module that takes them must cover the
   DisplayGeometry frame (any aspect), not the packet's XYZ; (fb) PanicFilter's skip events 0x26 / 0 are
   TRIG_EVENT_PANIC_26 / event 0 from this use only; (fc) the filters' mode 1 draw is the ROM's TEST_1
   ATST 5 / AREF 0x7f with ATE 0 (no alpha test) -- kept. Next in this TU: AngleAttenuationUpdate (+
   AngleAttenuationTarget, DistanceAttenuationUpdate, LosHitFadeUpdate), PanicUpdate, MonsterAIUpdate,
   FogAnimeUpdate, TexFileSetInit, ExecuteWarpObject / 2.
   **game/QuatSlerp.c slice 5 (partial, 2026-11-10):** the part fades AmbientAlphaUpdate runs every frame:
   DistanceAttenuationUpdate / _Target / _AlphaSet (ALL_TABLE +0x80, DIST_ATTEN 0x28), AngleAttenuation*
   (+0x84, ANGLE_ATTEN 0x24), LosHitFadeUpdate / _Target (+0x74, LOS_FADE 0x28; CheckLOS / CheckLOS_Multi
   declared in ssHitCheckAttackToScn.h with LOS_SEG / LOS_MULTI from the call sites, now stubs there);
   DemoSkipRegisterEvent, asVectorLength. MODEL_PARTS +0x124 / +0x128 / +0x12c are losAlpha / distAlpha /
   angleAlpha; PARTS_FLAG_LOS_FADING 0x10 / _LOS_CLEAR 0x20. The eye / subject lists share ATTEN_EYE_* /
   ATTEN_SUBJ_* bits and ATTEN_CHARA (0x10: chara, hash). 15 s smoke run: no fault, shot black. Open:
   (fd) ATTEN_CHARA_LIVE (Character flags 0x80000000 and 0x40000000 both) is the fades' test of a
   character: 0x80000000 is CHARACTER_FLAG_REFRACTION from main.c's one use -- this second use says it is
   wider (a character in the scene?): rename it when Character_Init.c's TU reads the bit; (fe) Cam[6] +0xc0
   (CAMERA_EYE_POS) is the distance and LOS fades' camera position (the angle fade overwrites it with
   asViewMatrix2PosRUF's): name the CAMERA field with the camera TU; (ff) the ROM's eye-character branch
   with no list reads a part hash from low EE memory (hazards.md): the port skips those characters --
   check whether any bank data has flag 2 with list 0; (fg) LOS_MULTI.hits are host pointers to records
   whose short at 0x198 is an id: ssHitCheckAttackToScn.c's TU names the record; ~~(fh) asJointParts_byMatrix's
   fourth argument~~ settled 2026-11-10 (slice 6): (mat, pos, bank, hash); main2.c passes
   DrawOnPosSetHashTemp (the ROM's a3).
   **Slice 6 (2026-11-10):** PanicUpdate, MonsterAIUpdate, GAME_OVER_END, PlayerControlUpdate2,
   asJointParts, asWarpSceneObject, asGetCrossPointInPlaneWithLine, asRadianByO,
   asContInterpolateZipNormal_XYZ / _Trans_XYZ; PANIC_INFO named (recoveryItem, recoveryTime, rise[8]),
   Panicdef's calmFall / fullFall, PanicRecoveryTbl and HidePointUpValue (ROM data), ssAtkCharaList2
   (unassigned_data). 15 s smoke run: no fault, shot black. Open: (fi) PanicUpdate registers 0xca
   (TRIG_EVENT_MC_RELOADED, Character_Init.h's name from mc_load_set) when a full panic starts unless 0xcc
   is set: the name is probably narrower than the event -- rename it when ssGrpPortalCheck.c's TU names
   events; (fj) PANIC_RISE_* (threat 0..2, const 4..5, attack 6..7; slot 3 unread) are named from
   PanicUpdate alone: the TUs that write PanicInfoData.rise confirm them; likewise TRIG_EVENT_PANIC_* and
   TRIG_VAR_HIDE_POINT / _PANIC_SUSPEND / _PANIC_VIBRATE; (fk) asWarpSceneObject's VU0 MULA/MADD
   broadcasts are read as x, y, z, w (the decompiler hides the field) and its byte 0x43 bit
   (PARTS_FLAG43_WARPED) has no reader yet; PORT: it finds the bank whose ALL_TABLE it was given to
   resolve partsHeader (BANK_PTR); (fl) FUN_0185c26c (lower-cases a string in place, no callers) is left
   unnamed: it sits right after ssGetBankID, probably its inlined tail or a static. Slice 7 (2026-11-10):
   FogAnimeUpdate, VistaVisionMain, OneMaxObjectDrawZOnly, asJointParts_byMatrix,
   asMakeWatchMeMatrixToPosition, asDelSqueezeInformation, ExecuteWarpObject / 2 (now 5 arguments),
   DemoSkipGetEvent (SkipFlagTbl from the ROM), CheckSameRoomAsMonster, PanicDownSet; FOG_ANIME,
   CHAR_AMB_CHANGE, MODEL_PARTS.warpH54 named, CHARACTER.w098 is roomHashPrev. Open: (fm) the
   letterbox bars (VistaVisionMain) are full-width overlays: when the host draw module exists they must
   span the window at any aspect, top and bottom, not a 4:3 column; (fn) WATCH_AXIS_* and
   VISTA_* / TRIG_EVENT_VISTA_* are named from these functions alone; FogAnimeUpdate's gate reuses
   TRIG_VAR_PANIC_OFF (0xb), whose name is narrower than its use (it also stops the fog easing); (fo)
   BANK_TYPE_SHADOW (1) is also the room type the warp's ambient step tests: rename when fileio.c's TU
   names the types. Slice 8 (2026-11-10): PanicSet, PanicStopMovie (PanicOffMovieFile from the ROM),
   Kekkai_Check, ssPointCheckWithinScreen, PauseUpdate, UpperCase, GetBankID, PlayMovie,
   PlayMovieReturnLoad, SystemReReset, asGetObjectPositionAtWorld, asGetAlyssaPosition,
   asGetAnalogStickInformation, asSetObjectTrans, asGetObjectTransInitialValue (MODEL_PARTS.transInit
   at 0xf0 named). Open: (fp) ssPointCheckWithinScreen tests against screen_0's 512 x 448 PS2 frame:
   at a wide aspect it calls points beside the 4:3 column off screen; decide with the camera / renderer
   TU whether it reads DisplayGeometry (it gates game logic, so changing it changes behaviour); (fq)
   KekkaiOn (sbss 0x0193f9a4, no symbol) and ITEM_KEKKAI (2) / TRIG_EVENT_KEKKAI_USED (0x98) are named
   from Kekkai_Check alone; (fr) asGetObjectTransInitialValue answers asVectorLength of what it copied
   (the ROM tail-calls it): RotateCamera.c's callers say whether they read it; (fs) PARTS_FLAG43_TRANS
   (0x2) is named from asSetObjectTrans alone; (ft) AA_StartMovie (movie.c) is now a stub PlayMovie
   calls. Next in this TU: TexFileSetInit (reached: the scene's texture-file sets, FSLoad
   of DATA/IMG/ pages into double / ring buffers, then patching TEX_INFO words -- decide the host
   texture path first: decode at the source, no GS), tnTrigTexFileSetNext with TexFileSetNext,
   tnTrigTexFileSetPage with TexFileSetPage, GetAreaFloor, then the rest of the 24 left.
   Slice 9 (2026-11-10): VistaVisionStart / End (now * 2 + VISTA_SHOW_NOW / VISTA_HIDE_NOW: now = 1
   slides), PlayerDamage, asObjectRotate (MODEL_PARTS.placeRot at 0x2c0 named: rows 0..2 of the part
   placement matrix whose row 3 is drawOnPos), asPosRUF2ViewMatrix / asCamRUF2ViewMatrix,
   asRadianByVectors / asRadianByPositions_Rorate, asISubRotate16, asVectorLength1D / 2D,
   asVectorDistance2, LowerCase, absf, tnRand, rnd / rndf; the three FUN_ of the slice were Ghidra loop
   splits (LowerCase_body, asMemCopy128c_nw_body, asMemCopy128c_body, game-inferred). Open: (fu)
   placeRot and PARTS_FLAG43_TRANS together (asObjectRotate sets flags43 = 3, an assignment, not an OR):
   whoever reads flags43 names what 3 means; (fv) asFindMotionID (0x0185e060) is left: it reads a pointer
   at CHARACTER +0x350 (inside behavior[]) to 0x30-byte motion records keyed by a u_long hash, ended
   by hash 0 -- the behaviour-controller TU decides whether that word is a host pointer or an EE word;
   (fw) asObjectRotate with an axis other than 0..2 copies stack garbage in the ROM: the port uses the
   unit matrix (PORT note). Left in this TU: TexFileSetInit (reached), TexFileSetNext / Page and their
   tnTrig wrappers, GetAreaFloor, asFindMotionID, and what else status.md lists.
   Slice 10 (2026-11-10): the paged textures -- TexFileSetInit (reached from scene_init),
   TexFileSetNext, TexFileSetPage, tnTrigTexFileSetNext / Page. ALL_TABLE +0x8c is `texFileSet`, a
   TEX_FILE_SET_TABLE {sets, files, buffers} (QuatSlerp.h; the relocator's 0x8c block confirms every
   word): sets of 0x14 (a -1-ended part list, slide flag, page, file list, buffer set), file lists of
   {count, page names}, buffer sets of {count, TEX_PAGE_SLOT[0x18]: handle, mipData[4], clut}. Pages load
   from DATA/IMG/ into buffers inside the bank (BANK_PTR) and the slot's EE words are copied into the
   part's SUN_TEX_ATTR (mipData, first CLUT word), so the existing texture path sees the page. page_set
   (sbss 0x0193cb4c, static) tells whether the double buffer preloaded the next or the previous page.
   15 s smoke run: no fault, shot black (no host draw module). Open: (fx) the ROM keeps the ring's
   bound p < pages - 1 (the last page is never preloaded) and TexFileSetNext with no matching set goes
   on with the last set: both kept; (fy) tnTrigTexFileSetPage's argument roles (part, literal, a2, page
   & 0xffff as a trigger variable, scene) are read from its body: ssGrpPortalCheck.c's action table
   confirms them; (fz) the slot's mipData[0] is the FSLoad buffer, so a page file is raw level-0 pixels
   with its other levels and CLUT at fixed bank addresses: check against a DATA/IMG file once one is
   decoded at its source. Left in this TU: GetAreaFloor, asFindMotionID (fv).
   **game/QuatSlerp.c slice 11 (verify, 2026-11-10): the TU's code is complete (177 of 177).**
   GetAreaFloor (CHARACTER *; bank +2 >= 0; roomHash vs roomHashPrev; the room literals @3345..@3458 at
   0x01933758.. read from the ELF: 0A10 0A01 0A06 0B01 0B06 1C01 1C02 1C06 1C04 1B11 1B07 1B08 1B12 2B01
   2B04 4A04..4A12) sets TRIG_EVENT_AREA_PASS2 / PASS3 on a step between paired rooms, as GetAreaNumber
   reads them; asFindMotionID walks the motion table. 15 s smoke run: no fault, shot black. Open: (ga)
   asFindMotionID reads CHARACTER +CHARA_MOTION_TABLE through a static MotionTableOf that answers NULL
   (index -1), the same gap as ahRelocateSunPs2ObjectArry.c's CharaMotionTable (fv): the TU that writes
   asInitPlayerBehaviorControler gives the word its host form and both helpers become one; (gb)
   GetAreaFloor's 1C02 branch has no else (from a room other than 1C01 / 1C04 the flags stay), read from
   the decompilation and the callee order, not stepped; the PASS2 / PASS3 names stay as GetAreaNumber's.
   **Queue refilled 2026-11-10 (scheduled run, the queue was empty)** in the order the smoke run reaches
   the stubs: ga/gaInit3dlib.c (gaSet3dlibStruct, gaMulMatrix, gaInit3dlib, gaSetZRollMatrix,
   gaInversMatrix, gaSetLight all reached at boot or every frame), then ga/gaSinCos.c.
   **ga/gaInit3dlib.c (verify, 2026-11-10): all 25 functions** from the disassembly (architecture.md "The
   3D library"; hazards.md: gaMulMatrix is dst = b * a). A 15 s smoke run: no fault, shot black (no host
   draw module); gaSetZRollMatrix now reaches **gaSinCos (ga/gaSinCos.c, a stub: the next TU, queued)** --
   with gaSetRotMatrixX/Y/Z, gaSetPointLight, gaContApplyMatrix it is the rest of the ga math. Open:
   (gc) GA_3DLIB_WORK's and screen_0's names (GA_SCREEN_*: offsets, centre, scale, near / far, [11]) are
   read from gaSet3dlibStruct's arithmetic only; the side-plane order (left, right, top, bottom) follows
   the signs -- the clipper (Clipping_Portal.c / the VU1 program) confirms; (gd) GA_LIGHT_WORK +0x00..0x0f
   is still unread; (ge) gaContMulMatrix's records are host layout (three pointers; 0xc bytes in the ROM)
   because it has no ROM caller; gaCopyObject is kept as the ROM's no-op; the two unnamed blocks at
   0x018a77a8 / 0x018a7808 are dead and not written (named nowhere: no symbol, no Ghidra function);
   (gf) gaSetZRollMatrix returns early (PORT) when GA_SPR_WORK.work3dlibEE is not a scratchpad address --
   the ROM would read EE memory there; boot sets it to 0x70000400 before any frame.
   **ga/gaSinCos.c (verify, 2026-11-10): all 13 functions** from the disassembly and VU0 micro 0x020 /
   0x028 / 0x038 (architecture.md "The ga helpers"); ga_sincos_table embedded from the ELF. A 15 s smoke
   run: no fault, ~26,700 frames, shot black (no host draw module). Open: (gg) gaProjectScreen's per-point
   formula assumes the micro's DIV result is the one its MADDQ uses 7 instructions later (the intended
   pipeline); a count of 0 never ends in the ROM, the host returns; (gh) gaSetPointLight's omniView
   columns past the last light keep the view matrix's entries, as the ROM's registers leave them --
   harmless only if the VU1 light code reads k columns; (gi) the memmove calls ahRelocateSunPs2ObjectArry.c
   writes in place of gaMemCopy32 / 128 (callcheck comments) could now call them: left as they are.
   **Queue empty after this TU**: the next run refills it from the smoke run's first reached stubs.
   **game/movie.c (partial, 2026-11-10, slice 1): 16 of 20 functions** -- the front end (AA_OpenMovie,
   AA_PlayMovieReady, AA_MovieProc, AA_StartMovie, AA_MovieIOPInit), the bank choice
   (AA_AutoBank_Seelect, AA_MovieWorkCheck / Clean, AA_DeleteSceneBank), the memory map (AA_MemoryMapSet /
   Free), the small ones. The object is SCE's ezmpegstr main.c: Claude's call (2026-11-10) is that it owns
   the sample's globals (voBuf, AA_audioDec, AA_videoDec, AA_infile, AA_voBufTag / Data) and all of sbss
   0x0193f0d4..0x0193f704 (movie.c's symtab locals run through it; the globals before AA_FileHandole stay
   with AA_GetDataWord.c). **Next in this TU (slice 2): AA_movie, initAll, readMpeg, termAll** -- the
   decode loop: decide first how the host plays it (sdk/libmpeg + the movie seam, RosePan's moviestream;
   threads and the IPU DMA handler have no host meaning), then write them. Open: (gj) the caption file is
   read to EE 0x200000 with no size limit; the host reads it to AA_MojiImage (1 MB, PORT) --
   AA_GetDataWord.c's readers of AA_MojiData must use AA_MojiImage; (gk) AA_MovieWorkClean adds each
   dropped group's size - 1 to the ROW index (ROM bug, kept): a pre-chosen layout whose banks hold groups
   skips rows and leaves their *work unset; (gl) AudioDec / VideoDec / StrFile / VoBuf are opaque blocks of
   the symtab's sizes in their owners' new headers (AA_SoundMonoSoart.h, AA_mpegTS.h, AA_videoCallback.h):
   the owners name the fields; (gm) TRIG_EVENT_OMAKE_2 (0xc5) registered means AA_OpenMovie reads no
   captions (the name is Character_Init's); (gn) sceGsSetDefDBuff is a stub in lib/unassigned_stubs.cpp
   (the movie's GS reset: no host meaning until slice 2 picks the draw path); (go) callcheck lists FlProc /
   FileSoundServer / SunAllocUsedBorderCheck as missing: the port factors the ROM's inlined frame wait and
   heap-bank count into AA_WaitFrame / AA_GlobalHeapBanks.
   **game/movie.c slice 2 (2026-11-10): AA_movie, initAll, readMpeg, termAll -- all 20 functions written
   (verify: no smoke run plays a movie).** Claude's call (2026-11-10): the playback keeps the ROM's shape
   (the host EE kernel runs the decoder thread and the vblank handler; sdk/libmpeg is the decoder; the
   picture reaches the screen through AA_GetDataWord.c's display code, whose TU picks the host draw).
   The neighbours' prototypes are now declared from movie.c's calls (AA_videoCallback.h, AA_mpegTS.h,
   AA_SoundMonoSoart.h, AA_GetDataWord.h, new AA_cpuCycles.h and copy2area.h) and stubbed. Open: (gp) a
   run that reaches AA_movie hangs on those stubs (hazards.md): the movie TUs come before any smoke run
   that plays a movie; (gq) VideoDec.hid_endimage / hid_vblank, StrFile.size / openError and ReadBuf.size
   are named from movie.c's use, after ezmpegstr: their owners confirm; (gr) the host's AddDmacHandler
   registers nothing, so AA_handler_endimage never runs: AA_GetDataWord.c decides what replaces the GIF
   transfer end; (gs) initAll tells CreateThread 0x20000 bytes of stack where AA_MemoryMapSet allocated
   0x10800 (ROM quirk, harmless on the host, whose threads have their own stacks); (gt) KEY_MAP[20]
   (KEY_MAP_MOVIE_STOP) and tiFade_Start types 4 / 5 after the fade to black / white are named from this
   use; (gu) AA_isCountVblank (sbss 0x0193f0bc) is defined in lib/unassigned_data.cpp until
   AA_GetDataWord.c takes it; (gv) the smoke run left work/shots/last.png empty (0 bytes) while the
   numbered shots are fine: worth a look in build.py's shot writer.
2. **The siblings' shims (the user's rule of 2026-11-09: "reuse the shims of the other pan projects like
   the windows handling and all the rest")**, as host tasks, in cpp-port.md "To carry over" order, each a
   class: ~~(a) gamecontrollerdb + icon + cursor auto-hide + pause / mute in the background + DPI scale~~
   (done 2026-11-09; pause in the background is written but no run has exercised it: smoke windows sit
   in the background, so turning it on there would freeze the run); (b) exclusive fullscreen, window position and presets; (c) the HDR swapchain,
   MSAA, anisotropy, GPU driver choice; (d) ~~the real memory card~~ (done 2026-11-09: `io::MemoryCards` behind
   sdk/libmc.cpp; a scratch test wrote, listed and read a save through sceMc* calls; no smoke run reaches
   McInit yet, so the game's own mc_* code has not exercised it), reverb and volume
   groups (YamiPan's SPU2 is next to them), the audio dump; (e) `libsdr` (CT3 calls `sceSdRemote`), newlib
   `rand`; (f) .iso reading, save import / export; (g) RmlUi menus and translations; (h) CI and AlyssaPan's
   own `extern/`. A shim CT3's boot needs earlier (libsdr, the card) moves ahead of TUs when a run hits it.
3. ~~**io::Disc's path table is root-only**~~ (2026-11-10, game/ArtoolDebugOpen.c): CT3 reads sectors
   through libcdvd and finds the pack with sceCdSearchFile, so the subfolders' files now get sectors and
   `Disc::find` answers by name; the ISO records still list only the root (no CT3 code reads them).
4. **ga/gaBGT_DrawObject.c -- partial (2026-11-10, slice 1):** the no-clip path is written
   (NoClippingCheck2, nc2 light0 / light1) and hands each object's blocks to `AlyssaPan_DrawGaObject`;
   ATLD's quads arrive (architecture.md "The ga draw object's send"). **Nothing draws yet**: the host
   module (`rendering::GaMeshRecorder`) only records. Left, in order: (a) **the transform**: read the VU1
   programs at MSCAL 0x1a / 0x22 (+ 2 x (flags & 3)) with `vudis.py` -- how the header (the scratchpad's
   0x60..0x90 3dlib constants, the object's matrix at +0x58 and position at +0x38) maps a vertex to the
   screen, what flags & 3 selects, and how light0 / light1 make the colour; (b) **a GPU pass** that draws the
   recorded meshes with that transform, widened (not stretched) at wide aspects from DisplayGeometry, the
   texture from the TEX_INFO / tbuffer upload sunBGT_ExecTBufferData_VIF1 sends (decoded at its source,
   ct3pack.py's decoder is the reference), and GS reg values +0x70 / +0x78 (probably TEX0 and a blend /
   clamp register: confirm by reading ATLD's); (c) the billboard path (flags & 0x20, 0x018a3984, shared with
   gaBGT_DrawObject_NoClippingCheck), gaBGT_DrawObject (the clipped path) and the non-nc2 set-ups; (d) the
   camera (game/asEyeForcusUP2CamMat.c, next in the queue) places the quads. Doubts: the index record's
   fourth halfword (repeats the first in ATLD) and the ST / colour blocks' single VIF head qword are read
   from ATLD's data only.
   **Slice 2 (2026-11-10):** (a) done for light0 (architecture.md "The VU1 object program"): the
   transform, projection, colour, ST offset, fog and the variant's cull run on the host
   (`GaMeshRecorder::project` / `cull`), and seams.cpp's DrawGaFrame draws each frame's triangles through
   UiCanvas, squeezed at wide aspects. **Still black**: ATLD's object matrix (obj +0x58) is all zeros in the
   smoke run (asChangeViewMatrix_forTheBank 0x01886690 and the camera are stubs: game/asEyeForcusUP2CamMat.c,
   next, fills it). Left: (b') the texture (TEX_INFO / the tbuffer upload, decoded at its source), a depth
   test and fog in the draw (UiCanvas has neither: a ga pass of its own may be cleaner), the frame end (CT3
   never kicks UiCanvas; the ga frame closes at the first object after a vblank, PORT); UiCanvas's window
   origin is FS2's (1792, 512 wide) while CT3's window is 2048..2560 x 2048..2496 (centre 2304, 2272): the
   ui_canvas.cpp check must set CT3's; (e) light1's lighting (0x296 / 0x40b; light1 meshes are grey);
   (c) as before. Doubt: GA_OBJ_REG70 / REG78 are floats (Z offset + fog base, ST offset), so their names
   in gaBGT_DrawObject.h and the AlyssaPanGaObject fields are kept for now but mislead.
   **Slice 3 (2026-11-10):** (c) done for the game code -- gaBGT_DrawObject (the clipped path),
   gaBGT_DrawObject_NoClippingCheck (and NoClippingCheck2's billboard jump into it), SetupDma_light0 / 1 / 2
   (architecture.md "The ga draw object's clipped path"): **every function of the TU is written** (nc2_light2
   read, dead). Their callees are new stub TUs: ga/gaDrawObject.c (11: the local view on VU0, the index
   passes, the reference tables, SetBig / SetNear) and ga/gaDrawObject_Redraw_Polygon.c (1); no smoke run
   reaches them yet (gaObjectClippingCheck is a stub). Left for the host module, as before: (b') the
   texture, depth and fog, the frame end, CT3's window origin in UiCanvas; (e) light1's lighting; new:
   (f) billboards are not handed to the host (their local view, gaDrawObject_VertexLocalView*_Billboard, is
   not read: the host's full-matrix transform would turn them); (g) light2 (VU1 MSCAL 0xa) is lit as light1;
   (h) the shadow volume's objects (ahRelocateSunPs2ObjectArry.c slice 13) carry EE words outside any bank
   (the template's light work 0x01937820 / 0x01937830, 0x01941840): ObjEePtr resolves through the object's
   bank, so those objects need another resolver when they reach a draw call. Doubts: the light0 set-up's
   index-qword stores (+0x10 CNT, +0x14 MSCNT) and lit's (+0x10 MSCNT) are written as the ROM writes them,
   unexplained until gaDrawObject_RemakeIndex* is read; GA_OBJ_FLAG2_NEAR (byte +2 & 0x04) is named from
   this one use.
   **Slice 4 (2026-11-10): game code complete, status verify.** nc2_SetupDma_light2 written for the record:
   its code is nc2_light1's instruction for instruction (only branch targets move; the MSCAL comes from the
   flags), so it delegates; its two trailing words (nop, j gaDMAStopMessage_VIF1 at 0x018a4fd4) follow its
   jr ra and are unreachable. The TU's ROM code is all written; the shot stays black until the camera
   (game/asEyeForcusUP2CamMat.c, next) fills ATLD's object matrix. Claude's call: the TU closes as verify,
   not partial -- what is left is host draw-module work, not this TU's code, and it stays here as its list:
   (b') texture, depth, fog, frame end, CT3's window origin in UiCanvas; (e) light1's lighting; (f)
   billboards; (g) light2's VU1 program; (h) the shadow-volume resolver. The camera session (or a host task
   queued after it) takes them when ATLD's quads land on screen; the TU's picture check is then due.
   **Correction (2026-11-10, the user: "nothing renders 2D"):** right -- no 2D draw reached the screen at
   all, whatever the game sent: Renderer::uploadVertices cycled the vertex buffer on upload, and the scene
   pass then drew nothing (a probe quad through UiCanvas stayed invisible on D3D12 and Vulkan; with the
   upload not cycled it shows). Fixed in renderer.cpp (hazards.md). Still in the way of ATLD, besides the
   camera: UiCanvas maps window y with FS2's origin (1792) and a 512-high frame, so CT3's window (2048..2496,
   centre 2272) lands half below the frame -- the ui_canvas.cpp host task must set CT3's origin.
5. **game/asEyeForcusUP2CamMat.c (the camera) -- verify (2026-11-10, slice 4; slice 1 below):** 15 functions written
   (architecture.md "The camera"): asEyeForcusUP2CamMat, asCameraPositionTranserUP, the cut chain
   (CamChange_S, CamChangeSub, asGetDestinationTRG, asGetDstinationFromTargetList,
   asCopyCharacterParameterTRG / _EYE), the geometry helpers (asCheckInnerPositions,
   asCheckThePositionInRectAngle, asGetLineRadTargetPos, asGetPlaneRadTargetPos), the bank view switch
   (asChangeViewMatrix_forTheBank), CamGetNowDOFLevel, CamChangeFOV_S, camera_init / _init0 / _destroy.
   The smoke run now gets through CamChangeSub to **asGetDestinationEYE** (0x01884f50, 0xdc8 bytes, the
   slice's last function: next) -- then **camera_view** (0x018875d0, reached each frame, the one that fills
   the view: the shot stays black until it runs) and camera_main. Doubts: (a) CAMERA's eye / focus / up
   *From / *To names come from CamChangeSub's copies and asGetDestination*'s arguments; focal[3] is the one
   asChangeViewMatrix_forTheBank reads, [0..2] live / from / to -- camera_view confirms; b17a, b17c, b17f,
   b181, w164 unnamed (init / cut values only); (b) **CAMERA.bank is an int** (CamChange_S writes and
   compares 4 bytes; was a short from asCharacterMoveByPad's stock); (c) ALL_TABLE.ctrl is one table for
   the cameras (+0 cuts, +4 targets, +0xc cut count) and the controlled parts (+8 list, +0xe count): name
   it when its loader is read; (d) the camera globals (AlissaEyeHeight, CameraAngle, ZelCam, CamStock,
   lpCopyMP, ...) are defined in this TU with the symtab's addresses: no other TU had them -- move them if
   a symtab object run puts them elsewhere; Cam[] stays in lib/unassigned_data.cpp; (e) asGetPlaneRadTargetPos
   reads quad[-1] in the ROM when three or four edges face away (@1029's -1 indices): the port returns 1
   with w 0 there (not reachable with a convex quad and its own normal); (f) camera_init0 indexes Cam[] by
   the bank number as the ROM does: a guard stops past Cam[6] (no caller passes more).
   **Slice 2 (2026-11-10, scheduled run): camera_view, camera_view0** and their helpers (CamChange,
   CamChange_Hash_S_Hash, asFindCamera, asCopyCameraMatrix, asCameraPositionTranser, asCamModeChange):
   the smoke run's first object now gets a real view (eye (10, 10, 10) at the origin), the shot is still
   black (the object lands at window (2804, 1884)): the eye stays camera_init0's until **asGetDestinationEYE**
   (0x01884f50, 0x1740 bytes -- the size above was wrong -- seven eye kinds, VU-heavy: next, alone) places
   it; then camera_main (0x018881a0, 0x1aac). Answered: (a) focal[0..3] are live / from / to / drawing,
   b17a / b17c / b17f / b181 / w164 are eyeMode / trgMode / focalMoving / moveSpeed / moveTime, changeArg
   is moveTicks (renamed). Doubts: (g) asFindCamera reads the part hash at ALL_TABLE.partsHeader + i *
   0x380 + 0x48, i.e. MODEL_PARTS[i].hash with pModelPartsHeader as the records' start (as main.c), while
   PARTS_REC (ahRelocateSunPs2ObjectArry.h) adds a 0x40 header: one of the two is off by 0x40 -- check with
   a real bank; (h) CameraQuake guards a 0 period (the EE's divu answer is undefined there); (i) the smoke
   run now passes about 1,800 frames a second (458 shots in 15 s, was 7): nothing waits on a present while
   nothing draws -- watch it once the picture shows; (j) the follow cameras (iCameraMode 0 / 1 / 2) add
   the turned translation to the turn's row 3 on xyz only, and asCameraPositionTranser's move writes w
   too (from.w + to.w * step): both as the ROM.
   **Slice 3 (2026-11-10, scheduled run): asGetDestinationEYE** (0xdc8 bytes after all), CamSetNowDOFLevel,
   CamChangeZELCAM_S (architecture.md "The camera"). The smoke run reaches it for the global banks 0..5
   only (kind 0, cut 0, eye part 1, no path): the shot stays black; the first object (light0, identity
   matrix) still lands at window (2304, 2272) with z inf -- that looks like the object's own w, not the
   camera: look at it from the draw side. Left in the TU: **camera_main** (0x018881a0, 0x1aac bytes,
   "Camera ID %d": the pad-driven camera; next, alone). Doubts: (k) the eye kind names (PART, RAIL,
   STAY, PLANE, RING, SEGMENT) and answer names are from this one reader; CAM_PART_TYPE_AXES (part +2 ==
   3) too; (l) ZelCam is a float[6] with int words at [4] / [5] (CamChangeZELCAM_S stores ints): a
   struct would be truer once camera_main's use is read; (m) the ROM reads Character[part].rot for a bank
   part (PART kind, type not 3) and the bank's part record at the character's slot (RING, a character's
   eye part): kept, guarded (host: index past CHARACTER_MAX reads rot 0); (n) the ROM reads the path record
   for every kind, also for 0xff (past the list): the host reads it only for the kinds that use it; a
   character eye part not found (lpCopyMP NULL) reads a zero record on the host (address 0 in the ROM);
   (o) OBJ_CTRL.kind is a point count for a camera path: the seek bars' CTRL_KIND_SEEKBAR (2) may be a
   count too.
   **Slice 4 (2026-11-10, scheduled run): camera_main -- the TU's 30 functions are written (verify).**
   The smoke run calls it with CAMERA_MODE_SCENE every frame (ControllerMain, data_load_set); the debug
   cameras (0 / 1 Alyssa flies, 2 the ND camera with ND_MOVE_FLAT / _VERTICAL / _DRIVE) and the
   controller-100 cut step (L1 / L2, "Camera ID %d") are not reached; the shot stays black (the draw-side
   z inf of slice 3). Doubts: (p) PAD_DATA.press (0x0c..0x17) is named in the DualShock 2's pressure order
   from the pairing here (each byte read under its SCE_PAD* bit): gaPadRead's TU confirms it, and the host
   pad must fill it (else every pressure step reads 0 -> the table's first entry blended to 0); (q) the
   stick rest value is 120 with the dead zone 0x53..0xad (the ROM's, not 0x80), kept; (r) bVerticalMove
   (sbss 0x0194180c, far from this TU's run) has no reader seen: lib/unassigned_data.cpp; GlobalCameraMode
   (0x0193cf08) sits inside this TU's sbss run and could move here; (s) MOTION_DEBUG_CAMERA (0x8a) and the
   motion 0 request on a mode change are named from this one use; (t) ND_MOVE_DRIVE steps NDiChar on
   pad_key_button (repeat), wraps at 34, and the mode-2 entry copies Character[0].rot into NDAlyssaRot[1]
   only while NDAlyssaPos.w is 0 (the first time).

6. **game/ssGrpPortalCheck.c (triggers and portals) -- partial (2026-11-10, slice 1; slice 4 2026-11-11):** written: the
   event / variable API (ssTrigCheckEvent, _RegisterEvent, _RemoveEvent, _GetVariable, _SetVariable), the
   byte readers (ssTrigRead1/2/4/8Byte), ssTrigCheckEnable, ssTrigCheckInputEnable, CheckRoomEntry,
   InitTriggerDetectorShape and the portals (ProcessPortal, ssScnPortalCheck, CheckPlaneEntry,
   ssGrpPortalCheck); 15 s smoke run: no fault, shot black (as before). **Slice 2 (2026-11-11):**
   Trigger_init / Trigger_deinit (both take the bank: scene_init / scene_end pass it in a0, main.c fixed),
   TriggerGlobalInit, ssTrigGetOther/Det/ActData, the five ssTrigRead* count readers, the delay-enable
   ring (ssTrigDEPop_Front, _PushBack, _ChangeReq, _DisableReq, ssTrigExecDelayEnable); ALL_TABLE +0x4c
   is now `trigger` (TRIG_DATA: other / detectors / actors). Open from slice 2: (c) TRIG_WORK.header12 and
   TRIG_DET_WORK.kind / body are named from Trigger_init only -- Trigger_main / ssTrigCheckDetector name
   them; (d) TRIG_WORK and TRIG_DET_WORK are layout host (they hold host pointers into the bank image);
   (e) Trigger_init returns early for a bank whose ALL_TABLE.trigger is 0 (PORT guard; the ROM reads EE 0).
   Still to write: Trigger_main and the detector / condition / actor functions. Old plan: Trigger_init (fills
   OthData / DetData / ActData, TrigWork 0xc-byte records at 0x01ad7810, DetWork 0x1c-byte and ActWork
   0x28-byte _SunAlloc blocks, TempEventFlag / TempVariable), then Trigger_main (three passes over
   banks 0..5 then 6..85: ssTrigProcDetRecur, ssTrigProceedActor, ssTrigExecDelayEnable), Trigger_deinit,
   TriggerGlobalInit and the detector / condition / actor functions. Open: (a) the temp sets are NULL
   until Trigger_init is written: the five accessors answer 0 / do nothing then (PORT guards, remove
   them when Trigger_init fills every bank it can be asked about, or keep them if a bank without
   trigger data can be asked); (b) ProcessPortal gates characters on flag 0x80000000, which main's passes
   named CHARACTER_FLAG_REFRACTION from one use: the portal pass makes it look like a "live / placed"
   bit -- rename when the character TU sets it; (c) CHARACTER.lightBank (+0xa0) is the room's bank a
   character stands in (the portal sets it), not only its light: rename with (b); (d) TRIG_DETECTOR /
   GRP_PORTAL field names come from the portal readers only (+0x08 / +0xa6 / +0xa8 room ids copied to
   CHARACTER.h100: h100 is the character's room id); (e) TRIG_VAR_ROOM_ENTRY (0x20) is named from
   ssScnPortalCheck's one read (into CHARACTER.b102); (f) the ALL_TABLE.detectors and .groupPortals
   lists are guarded for a 0 word (PORT) -- the ROM reads EE address 0.
   **Slice 3 (2026-11-11):** Trigger_main, ssTrigProcDetRecur, ssTrigProceedDetector,
   ssTrigCheckDetector, CheckAxisAlignedBounds, CheckInsidePlanes, CheckContactSurface, the actor runners
   (ssTrigRunActors, _RunActor, _ProceedActor, _CancelActor, _ExecuteActor), the waits
   (ssTrigWaitByAnimFrame / _ForGlobal), ssTrigCheckRandom, ssTrigCheckSceneDraw, ssTrigGetCurActInfo.
   Slice 2's (c) settled: header12 is numRoots (script -> roots), DET kind / body are numChildren /
   children. Open from slice 3: (g) **ssTrigCheckConditions (0x018d7b70) and ssTrigExecuteActCmd
   (0x018d5450) are stubs** (conditions answer 0, so no detector turns true; the command stub answers
   "finished") -- the next slices, then ssTrigCheckInput, ssTrigCheckVariable, ssTrigLookAtCheck,
   ssTrigAttackHitCheck, Trigger_reset; (h) CheckContactSurface's targets have no host form
   (ContactTargetPart answers -1, with 1 (v) / (ac)); (i) TRIG_ACT_CMDS.wait / .step and TRIG_DETECTOR's
   box / plane-volume fields are named from their starts and the shape readers only: ssTrigExecuteActCmd
   confirms them; (j) the shape kind > 5 answers 0 (the ROM's switch has no default).
   **Slice 4 (2026-11-11):** (g) done but for the command runner: ssTrigCheckConditions (the condition
   format, AND groups closed by TRIG_COND_OR, list kinds with an any bit, is in ssGrpPortalCheck.h above
   TRIG_COND_EVENT), ssTrigCheckInput, ssTrigCheckVariable, ssTrigLookAtCheck, ssTrigAttackHitCheck,
   ssTrigSetEnable, Trigger_reset. **Only ssTrigExecuteActCmd (0x018d5450, 0x249c bytes, ~70 command
   kinds by u16 id from 0x6a8 down, 78 callees) is left**: the smoke run now reaches its stub (detectors
   turn true with real bank data). Read it in two or three slices: the dispatcher and its skip sizes first
   (cVar31 = skipping, like the conditions' groups), then the commands by callee TU. Open from slice 4:
   (k) ssTrigAttackHitCheck's hit lists (ALL_TABLE.hits by BANK_INFO.type: a scene bank's two lists at
   +0x2b0 / +0x2b8, a shape bank's at +0x1c and HIT_TABLE's +0x24; HitListsScene / HitListsShape in the .c)
   are named from this reader only: the collision TU (ssHitCheckAttackToScn.c) confirms (slice 1,
   2026-11-11: ssProcessHit clears exactly those four words each frame -- HIT_TABLE.objHitCount / w2b8,
   w1c / count; the writers, ssHitCheckAndProcess and the attack checks, still to read); (l) the records'
   attacker objects are EE words of an unknown bank: HitObjectId resolves them by the bank whose EE image
   starts nearest below (BankOfHostPtr's rule, EE side; PORT); ~~(m) ssRadianByO is declared double (the EE
   returns a software double in v0) in ssHitCheckAttackToScn.h, stubbed: ssTrigLookAtCheck answers by
   the stub's 0 until that TU writes it~~ written 2026-11-11 (ssHitCheckAttackToScn.c slice 1); (n) TRIG_INPUT_KEY / _TRG / _CNT are named from the pad words
   (pad_key_button / _trg_ / _cnt_), not from their meaning; (o) ssTrigLookAtCheck's angle is the whole
   view cone in degrees (half = angle * 0x10000 / 720), read from the arithmetic only.
   **Slice 5 (2026-11-11): ssTrigExecuteActCmd, ssXcmd_functions and suXcmd_AnmDirection written: the TU's
   code is complete.** The command ids and operand sizes are TRIG_CMD_* in ssGrpPortalCheck.h; the smoke
   run parses the menu banks' lists cleanly (consecutive offsets match every size). The 256-slot x-command
   table is filled from the ELF (0x0193a500); 232 of its functions had no declaration and are declared in
   their TUs' headers with the SS_XCMD shape and stubbed (new files tiXcmd_TransItem.c, tnXcmd.c,
   PicPaz.c, RotateCamera.c, hhXcmd_BGM_SE_VOLUME.c). **The picture went black (it showed the grey ATLD
   banner before):** bisected with probes: (p) bank 0's list (after tnXcmd_AutoLoad, slot 255) hides parts
   2 and 3 (the banner) and shows 6 and 7 (5 delayed) -- the ROM's own switch, so parts 6 / 7 are the next
   thing to draw: find why the host draws nothing for them (their draw path or ATLD); (q) bank 5's
   TRIG_CMD_CAM_CHANGE (scene -1: CamChange_S on bank 5 itself, the first scene-bank cut) also blanks the
   view on its own: the cut's camera on the host (asEyeForcusUP2CamMat.c / the animated eye, cf. item 7)
   is the suspect; (r) bank 5 then waits in wideXcmd_PlayAnimeToFream2 (slot 106, a stub answering 0).
   Open: (s) the five-arg ssTrigWaveStart / Vaseline / MotionBlur / Particle / HQDOF / Hazy prototypes
   (ssSetTexUVforCubeMap.h) follow the call sites (Wave takes 12, the last four on the stack; Vaseline 9);
   (t) asSceneAnimationStart's stack arguments for commands 501 / 502 were read from the disassembly
   (501: started = byte 4, toEnd 0; 502: started 0, toEnd = byte 4; then byte 5, target, chara) -- 502's
   byte order is assumed like 501's; (u) the host's SSActPlaySound / SSActVolume / SSActLoadBankSE /
   CamChangeFOV_S take fewer arguments than the ROM passes: the extra operands are read and dropped;
   (v) ssXcmd_LockToTargChara (short bank), asXcmd_AddForLockToTargChara and wideXcmd_DebugPrint (void)
   sit in the table through a cast until their TUs give them the SS_XCMD shape.
   **Picture session (2026-11-11, scheduled run): verify.** The black screen was not (p)'s part switch: every
   ga object's +0x38 word (the EE address of its part's partsPos, the ambient the light0 program adds) was 0
   because `ssSetAmbColorToAllObj` (0x01895f00, game/ssSetTexUVforCubeMap.c) was a stub; it is written now
   (the one function of that TU, Claude's call: it is this TU's picture fix). The shot at frame 120 shows
   ATLD's "Checking... Do not remove memory card..." text as work/ref/ATLD/trans_04 has it, centred, at
   4:3 (683 x 512) and wide (1280 x 540: same shape, centred, not stretched); from frame 180 the ROM's own
   switch hides parts 2 / 3 (the text) and shows 5 / 6 / 7. Left, for other TUs: (w) parts 6 / 7 (the
   icon sheet's arrow marks, window ~2410..2476 x 2384, colour 90 90 90 78) still sample an empty cell
   (ST offset obj +0x78 = 0.5, 0): the UV track (EA_010, written) must move it -- bank 0's animation does
   not reach that key, probably because the scripts wait in stubs (wideXcmd_PlayAnimeToFream2, (r)); ~~(x)
   the shot reads "slot 2" where the decoded reference reads "slot 1"~~ settled 2026-11-11 (ga_atld.cpp):
   the text is a paged texture of 10 slide pages, page 1 is the check of slot 2 (architecture.md "ATLD's
   messages are a paged texture"); (q) and (r) stand (camera cut, PlayAnimeToFream2).

7. **(2026-11-11, the user: "2D is still not showing up") The first picture.** Fixed: the black screen was
   Execute_ExecuteAnimationHistory writing an EE address into every camera cut's focal length (hazards.md
   "Two part indexings"). Now ATLD's quads land on screen (2 of 2 triangles kept, 15 s smoke run) as grey
   untextured shapes right of centre; the reference (work/ref/ATLD/trans_04: "Checking... Do not remove
   memory card...", trans_00 / _02: the arrow marks) is white text with a black outline, centred. Left,
   in the ga host draw module (ga/gaBGT_DrawObject.c's open list (b'), alyssapan/rendering/ga_mesh.cpp and
   seams.cpp DrawGaFrame): (a) **the texture** (TEX_INFO / the object's GS registers +0x70 / +0x78 to a
   TextureStore texture decoded from the bank's TIM2), (b) the colour: light0's RGBA comes out 0 0 0 127
   (vertex colour + the object's +0x38 qword) and the shot shows grey -- check what DrawGaFrame does with
   it, (c) the window origin (UiCanvas's FS2 origin 1792 vs CT3's 2048, open-items 4 Correction), (d)
   depth / fog / light1. **Queued first** (the user's ask): the host task
   `alyssapan/rendering/ga_atld.cpp` -- ATLD at 4:3 and wide as the reference shows it.
   **Host task ga_atld.cpp, partial (2026-11-11, scheduled run):** (a) done -- sunBGT_ExecTBufferData_VIF1
   hands the texture buffer's TEX0 / CLAMP, level 0 and the CLUT TEX0.CBP picks to `AlyssaPan_DrawGaTexture`
   (alyssapan_drawga.h); `TextureStore::imageOfGsTexture` decodes PSMT4 / T8 (CSM1) / CT32 / CT24 / CT16
   at their source; each recorded mesh keeps the texture bound when it came and DrawGaFrame draws it
   textured, blended. (c) done -- DrawGaFrame maps CT3's window (512 x 448 centred on 2304, 2272) onto the
   canvas, centred, 448 lines to 512. **Picture:** black, as before, for a game reason: (b) the vertex
   colour is colour + vf20, and vf20 is the part's ALL_TABLE.partsPos qword -- the ambient colour
   AmbientAlphaUpdate moves toward `scene_alpha_anim[bank]`, which only the event command
   `tnXcmd_Scene_Ambient_Change` (0x018f1940, game/xcmd_tn.c, not written) sets. With the ambient forced
   to 0x80 (a probe, stripped) the shot shows ATLD's text exactly as work/ref/ATLD/trans_04 has it,
   centred, at 4:3 (683 x 512) and wide (1280 x 540: same shape, centred). Queued first: game/xcmd_tn.c
   (Claude's call, Decisions). Left here: depth and fog (UiCanvas has neither), HIGHLIGHT / HIGHLIGHT2 drawn
   as MODULATE, CT3's ALPHA_1 / TEX1_1 for ga draws not read (alpha blend and bilinear assumed); the icons'
   ST offset (obj +0x78 = 0.5, 0) points their quads at an empty cell of the 128 x 32 sheet (u 64..80) --
   likely a blink / animation cell the game moves (find the writer); a second bank's objects (a light1 grey
   band and a gold ornament, window 2002..2230 x 2042..2145) draw top left under ATLD's camera: check
   whether the ROM hides them (partsPos w, bank drawOn) or they belong to the next screen.
   **game/xcmd_tn.c slice 1, partial (2026-11-11, scheduled run):** tnXcmd_Scene_Ambient_Change and 21
   small commands written (architecture.md "The x commands"). The smoke run cannot reach them: their one
   caller is `ssTrigExecuteActCmd` (command 0x514 through `ssXcmd_functions`, 0x0193a500), still a stub
   in game/ssGrpPortalCheck.c, so the shot is unchanged (black ATLD; the grey band and gold ornament top
   left). **Queued next: game/ssGrpPortalCheck.c's ssTrigExecuteActCmd** with the 256-word table (Claude's
   call, Decisions); the table needs every slot's function declared (stubs for the ones not written).
   Open: (u) callcheck reports ALL-CAPS calls (BRANCH_CLEAR, GAME_OVER_END) as missing though they are
   there -- its call pattern seems to skip upper-case names as macros; (v) PARTS_FLAG_FOG (bit 0) is
   the "not drawn" bit the room-in icon commands clear / set: rename it (PARTS_FLAG_HIDDEN?) when the
   fog part's use is read again; (w) the strncpy in wideXcmd_GlobalFree / tnXcmd_Menu_MapOff (the ROM:
   strcpy) bounds the copy of a 16-byte name: callcheck lists it.
   **game/xcmd_tn.c slice 2, partial (2026-11-11 09:22, scheduled run):** 15 commands written
   (wideXcmd_PlayAnimeToFream2, the clips, SetPanic / EndPanic, Continue, TimeStop, ShapeFree /
   BehaviorFree / BankFree, StayRoom, ReadChengeMsg, Menu_MapLoad, Char / SceneFogAnimation). **Picture:**
   frame 120 shows ATLD's memory-card text ("Checking... Do not remove memory card...") at 4:3, centred,
   as the reference; from frame 180 on the screen is black (so are the 09:05 and 09:21 runs' last.png:
   the progress pictures since the dispatcher landed take the last frame). **Next blocker: the script
   reaches the stub `tiXcmd_XFade_Break` (0x018e4390, game/tiXcmd_TransItem.c), which answers 0 =
   XCMD_WAIT, so the script waits on it every frame** -- queue it (or the x-fade commands with it) before
   the rest of xcmd_tn.c. Open from slice 2: (x) tnXcmd_BankFree's behaviour case passes a register the
   ROM never set as the character (s1): the port asks BankID_TO_CharID for the bank's owner -- check
   BankID_TO_CharID answers behaviour banks; (y) RoomFileName_STR_Tbl (0x019394d0) and load_id
   (0x0194493c) are defined in xcmd_tn.c (one reader each); the data's owning TU (beside QuatSlerp.c's
   RoomNameID_STR_Tbl) may claim the table; (z) XCMD_FRAME_TIME (10) is the ROM's frame * 10 against
   asGetSceneAnimationCurrent's time / 100: the units are taken as given, not checked against a run.
   Left: SetRoodersPts, SetAllControlDisableCamera, ScreenAdjust, the omni commands, Filter_Change,
   TimeSet, SetMovieWorkBuff, 1P01_PazzleAnim, and 10 beyond the slice.
   **game/xcmd_tn.c slice 3, partial (2026-11-11 12:55, scheduled run):** 11 commands written
   (SetRoodersPts, SetAllControlDisableCamera, ScreenAdjust, All / CharOmniEffect, OmniRange,
   Filter_Change, SetMovieWorkBuff, 1P01_PazzleInit, tnCurrentSceneSet, TexFileSetNext); MODEL_PARTS
   +0x70 / +0x74 / +0x78 named omniChanging / omniSteps / omniRange (OmniRangeChange's record is the part
   + 0x40). Picture unchanged (frame 120 the memory-card text, then flat grey: no new command is reached).
   Open from slice 3: (aa) tnXcmd_OmniRange with three arguments reads argv[4] / argv[5] in the ROM
   (stale dispatcher slots): the host skips the falloff -- look at a script that sends 3 arguments;
   (ab) ScreenAdjust writes ga_disp.display DX / DY only: the host's picture placement ignores them (the
   renderer owns geometry), so the options screen's adjust does nothing visible -- decide whether it
   should nudge the host viewport; (ac) TRIG_VAR_PUZZLE_ROW / _COL (0x12 / 0x21), ITEM_ROW_HELD rows 1..3
   and ArrowData[1] / [2] as the score's items are named from one use; (ad) SetMovieWorkBuff drops the
   ROM's sprintf of the clash message (WideErrorMsgDebug is empty in retail), as BankFree does.
   Also this run: game/tiXcmd_TransItem.c taken off the pinned queue (its last handoff: only the eight
   boss stubs left, waiting for pBossInfo's host form); it stays partial and comes back after the queue.
   Left in xcmd_tn.c: Key_Lock (0x16f0 bytes), TimeSet, 1P01_PazzleAnim, VariableCal, PlayAnimeToFream,
   PlayAnimeRange, Character_Ambient_Change, VistaVision, TexFileSetPage.
   **game/xcmd_tn.c slice 4, partial (2026-11-11 13:10, scheduled run):** 7 commands written (TimeSet,
   1P01_PazzleAnim, PlayAnimeToFream, PlayAnimeRange, VariableCal, Character_Ambient_Change,
   TexFileSetPage); statics TimeSet_Backup (0x0193b90c) and pazzle1P01_Main (0x01944938) defined. Picture
   unchanged (frame 120 the memory-card text, then flat grey: none of them is reached). Open from slice 4:
   (ae) ~~(ac)'s puzzle names~~ settled: 1P01 is a clock, ROW the hour (0..11), COL the minute (0..59),
   24 track frames an hour (XCMD_PUZZLE_*); (af) asSceneAnimationStart's 12th argument (named
   stepFromRuns) is "backwards" at every x-command call (PlayAnimeToFream2's to < now, PazzleAnim's turn
   back, PlayAnimeToFream's argv[6], PlayAnimeRange's argv[8]): rename it when its TU reads it; the 10th
   (started) / 11th (toEnd) names stand unconfirmed (PlayAnimeRange passes argv[5] as the 10th,
   PlayAnimeToFream argv[3] as the 11th); (ag) CHAR_AMB_SET (xcmd_tn.c, layout EE) is
   tnChangeCharAmbientColorSet's request: mode 1 (six arguments) / 3 (seven, + flag byte +0x18) named from
   this one use; the portal and warp callers pass mode 0 with only 4 shorts -- the callee's TU
   (ssSetTexUVforCubeMap.c) confirms the fields and may move the struct to its header; (ah) VariableCal
   with a zero divisor (/ or %) and the anim commands with an unknown source or selection leave the
   variable / animation alone on the host (the ROM uses stale registers). callcheck gaps of this slice are
   the static helpers (XcmdSetDigit, XcmdPuzzleTurn, XcmdAnimSelect, XcmdValue). Left in xcmd_tn.c:
   Key_Lock (0x16f0 bytes, its own session) and VistaVision (0x3c0).
   **game/xcmd_tn.c slice 5, verify (2026-11-11 13:25, scheduled run): the TU's code is complete.**
   tnXcmd_VistaVision (the letterbox command: argv[0] "ON" / "on" brings the bars in, else out; argv[1]
   1 slides; a fourth argument makes ON skippable only when it is "ON" / "on"; statics
   VistaVisionEndCheck / WaitTime / TimeOutError (= 3600) defined, steps VV_CMD_*) and tnXcmd_Key_Lock
   (a door part's KEY_PART_LOCKED bit, MODEL_PARTS.flags bit 0, cleared by per-stage rule tables keyed on
   TRIG_VAR_STAGE, GetAreaNumber and argv[1]'s key id: the ROM's 0x16f0-byte compare tree as data).
   Picture unchanged (frame 120 the memory-card text, then flat grey; neither command is reached). Open
   from slice 5: (ai) trigger events 0x17 / 0x18 / 0xb2 are named from VistaVision only
   (TRIG_EVENT_VISTA_17 removed when the bars are gone, _OFF_18 removed when they are asked in, _AT_ONCE
   registered for argv[1] 0); VistaVision also holds TRIG_EVENT_PANIC_HOLD (0) while the bars run, so
   PANIC_HOLD may be a wider "cut-scene" hold; (aj) Key_Lock's ~40 story events stay numbers (one use
   each); the rule tables were written from the decompiler's tree: the ROM's two fall-throughs (stage 3
   area 0x15 into 0x14, stage 4/5 areas 0x2b / 0x2c into 0x29 / 0x2d) only reach rules for other keys, so
   first-match tables are equivalent -- a harness (the *cmp.py pattern) could prove it; (ak) bit 0 of
   MODEL_PARTS.flags has three names by part kind (PARTS_FLAG_FOG, PARTS_FLAG_HIDDEN, KEY_PART_LOCKED):
   whichever TU loads the parts header may unify them.
   **Host task ga_atld.cpp, done (2026-11-11 13:45, scheduled run): ATLD draws as the game picks it.**
   With the x commands reached, frame 120 shows the memory-card text textured, centred, the same 451 px
   wide at 4:3 (683 x 512) and wide (1214 x 512 in a 1280 x 540 window), 22 px left of centre in both
   (the text's place inside its 512-texel page). It reads "slot 2" because the text is a **paged
   texture**: 10 slide pages packed in ATLD, each with its own CLUT, and the script shows page 1 (the
   check of slot 2) -- `ct3pack.py tex` now thaws every page to `work/ref/ATLD/page_0_NN.png`, and the
   shot matches page_0_01 (architecture.md). Brightness: texels x 90/128 (the bank's ambient 90), the
   brightest 151 -- the game's own value. Not shown by any ATLD picture, left for the screen that needs
   them: depth and fog in DrawGaFrame, HIGHLIGHT / HIGHLIGHT2 (drawn as MODULATE), CT3's ALPHA_1 / TEX1_1
   (alpha blend and bilinear assumed); the icons' empty sheet cell stays with (w) above (the UV track).
   **Next blocker (the picture after ATLD):** frame 180 black, from 240 on the whole frame is flat 128
   grey. The run reaches the stub `tiFade_Start` (0x0188d3c0) right after ATLD's last draw, then the
   scene teardown (ssParticleDeInit, ssPartMotionBlurAllEnd, tiWindEnd) and `hhXcmd_LoadSE`: the grey is
   the next scene's first screen or a fade nothing drives -- tiFade_Start's TU (and the fade draw) first.
   **Queued first: game/Clipping_Portal.c** (tiFade_Start's object; Claude's call, this run).
   **Update 2026-11-11 (Clipping_Portal.c slice 1, scheduled run):** tiFade_Start and tiFade_Draw are
   written. The fade that runs after ATLD is type 4 (FADE_TYPE_OUT2_BLACK, mc_load_set's) over 300 ticks:
   it reaches f 1 (ALPHA_1 FIX 128, full black) within ~60 frames and stays (OUT2 kinds never end
   themselves). The ROM draws it as a raw GIF A+D SPRITE (init_black / init_white), which the host drops
   (no GS emulation), so the shot is still flat grey where the ROM is black. **Queued first (Claude's call):
   the host task alyssapan/rendering/ga_fill.cpp**: a native full-screen fill in alyssapan_drawga.h that
   tiFade_Draw calls instead of the packet, drawn over the whole window at any aspect; and find where the
   flat 128 grey comes from (no draw seen makes it). Open from slice 1: (ha) WIND (ALL_TABLE +0x98) and
   LIGHT_BELT2 (+0xa0) are named from their init / update only: their readers (particles, pendulums,
   tiLightBelt2Update) name the points; MODEL_PARTS +0x200 (matrix200) is a second matrix whose writer is
   unread; (hb) PARTS_KIND_WIND_ON_ROOT (1) = PARTS_KIND_ITEM_LOCKED: one of the two names is wrong;
   (hc) tiVibrationTimer counts and calls tiVibrationOff every frame past the limit, timed or not (the
   ROM's; harmless: Off only stops motors); (hd) tiWindInit returns on a 0 list word (PORT guard: the ROM
   reads EE address 0); (he) tiVibrationOn's prototype is now (port, small, large, time): the callers'
   argument names (PanicSound's) stand.
   **Host task ga_fill.cpp, done (2026-11-11, scheduled run): the fade draws.** tiFade_Draw calls
   AlyssaPan_DrawGaFill (alyssapan_drawga.h) instead of sending init_black / init_white; the fill keeps draw
   order with the ga objects and covers the whole window. Smoke run: frame 120 ATLD's text as before, frames
   180..420 black edge to edge at 4:3 (683 x 512) and wide (1214 x 512 in 1280 x 540) where they were flat
   128 grey. Open from it: (ia) the grey was no draw: it showed while no frame was kicked (nothing after
   ATLD's last object closed a frame); a probe run with the fill left out is black too. Why a long gap
   without kicks presents grey (the renderer says it shows the last frame again; the present and screenshot
   passes load DONT_CARE: present_pass.cpp, screenshotter.cpp) is not read -- worth a look when a screen
   stalls; (ib) the fade's draw control (PRMODE / TEST_1 / ALPHA_1 packet) is still built and sent as the ROM
   does and dropped by the host; only its FIX reaches the fill. **Next blocker:** after the fade the run
   tears the scene down (ssParticleDeInit, ssPartMotionBlurAllEnd stubs) and stops at the stub hhXcmd_LoadSE
   with nothing more drawn: the next screen's load (the queue's game/Clipping_Portal.c rest, then the SE / load
   x commands) comes next.
   **Update 2026-11-11 (Clipping_Portal.c slice 2, scheduled run):** Clipping_Portal (the portal's side
   planes), tiCharaFootAdjust and tiCalc_FootPos (two-bone leg IK onto the floor) are written; the smoke run
   is unchanged (frame 120 ATLD's text, then black): none of them runs in it (Clipping_System 0, no
   character yet). Open from slice 2: (ja) LOS_HIT (ssHitCheckAttackToScn.h): CheckLOS's caller buffer runs
   on past LOS_SEG -- hit point at +0x20, hit record at +0x30 (flags +0x194, LOS_HIT_FLAG_FLOOR 0x100000)
   -- read from tiCharaFootAdjust's stack use only; CheckLOS's TU confirms and fills them (until then the
   stub leaves no record and no foot bends); (jb) CheckLOS's bank for the feet is CHARACTER.lightBank
   (+0xa0): the scene bank the character stands in -- the field may want a better name; (jc)
   CHARACTER.footAdjust (+0x56c, bits CHARA_FOOT_ADJUST_R / _L) has no writer read yet; (jd) the record's
   +0x20 (MODEL_PARTS +0x60) is named LEG_REC_PIVOT from this use only (the point the joint turns about,
   in its own frame); (je) the IK's turn axes are cross products of unit vectors, not normalised, passed
   to asMakeRotMatrixByQuartanion as the ROM does; whether it normalises is that TU's to say; (jf)
   Clipping_Portal's left and bottom secants use the centre alone (CX / sx, CY / sy) while the right and
   top use the span to the edge: the ROM's, kept. Slice left: tiPipeJointUpdate with tiPipeJointInit (its
   globals pPipe_Joint / _Base / _Target, length1 / 2, matParent, prevMatrix, Pipe_Vel: do them together),
   tiDrawShadingOff with CaptureDBuffer and tiVRAMViewer (a frame-buffer capture: needs a host call),
   ExportDrawToTex, tiDrawLensFlares, tiLightBelt2Update.
   **Update 2026-11-11 (Clipping_Portal.c slice 3, scheduled run):** tiParticleUpdate, tiCalc_LifeSpan,
   tiHurikoUpdate, gaNormalizeVectorXYZ, tiSet_CharaTranslucenceOFF written; tiGenerateParticle,
   tiCalc_Orbit, tiHurikoReset, tiCalc_HurikoOPEN / _CLOSED, tiGetNewNormalData stubbed (their prototypes
   from these call sites). Smoke run unchanged (frame 120 ATLD's memory-card text, then black). Open from
   slice 3: (jg) PORT: tiCalc_LifeSpan, tiGenerateParticle and tiCalc_Orbit take the bank as an extra
   argument (the entry's ring is an EE word in it); (jh) PARTICLE2.life (+0x50) / .age (+0x54) and
   HURIKO (trigPart +0, flags +0x18: CLOSED 1, DISABLED 4, RESET 8) are named from these readers only;
   tiHurikoReset_On's "bit 2" is probably DISABLED (4), which leads to a RESET next frame -- confirm in its
   slice; (ji) BANK_TYPE_BEHAVIOR (3 = FS_LOAD_BEHAVIOR) is skipped by both updates (the particle one only
   past the scene banks); neither honours BANK_FLAG_ANIM_FORCE_RUN while paused (the ROM tests only
   pause 0 or PAUSE_RUNNING), kept; (jj) tiCalc_LifeSpan walks every live record and drops any expired
   one by moving head past it, not only the leading ones: the ROM's, kept; (jk) ALL_TABLE +0x68 is now
   huriko. Slice left as before (pipe joint, shading-off capture, ExportDrawToTex, lens flares,
   tiLightBelt2Update) plus the six new stubs and tiParticleInit / tiHurikoInit.
   Slice 4 (2026-11-11): tiHurikoInit, tiHurikoHitUpdate, tiHurikoReset_On, tiSet_CharaTranslucenceON.
   Open from it: (jl) HURIKO's first word is read three ways (ssTrigCheckEnable's short, tiHurikoInit's
   part index as a word, tiHurikoReset_On's id as a word): kept as trigPart + HurikoWord0, rename when
   tiHurikoReset / the trig code says which it is; (jm) HURIKO_JOINT (posRef double indirection, length,
   up / down / left / right position EE words, restDir, pos) and HURIKO_HIT (ALL_TABLE +0x90 hurikoHits)
   are named from tiHurikoInit / tiHurikoHitUpdate only: tiCalc_HurikoOPEN / _CLOSED and the hit readers
   (ssProcessHit?) confirm them; the roots keep their local position at +0x60 while the other joints get
   world xyz there, the ROM's, kept; (jn) HurikoPart reuses PARTS_KIND_WIND_ON_ROOT (kind 1 = part 0's
   frame) -- the name wants to become generic (PARTS_KIND_ON_ROOT) when its writer is read;
   (jo) tiHurikoHitUpdate skips forced-stop banks among the scene banks too, unlike tiHurikoUpdate: the
   ROM's, kept; (jp) tiSet_CharaTranslucenceON stores the state (1 or 2), OFF clears bits: the ROM's.
   Slice 5 (2026-11-11): tiPipeJointInit, tiPipeJointUpdate, gaNormalizeVector3XYZ, tiXFadeOut_Start;
   XfadeEndTicks (0x01942950) declared. Open from it: (jq) the pipe is Character[3]'s (CHARA_E03) tube,
   parts s_tyubu01 (base) / n_29 (elbow joint) / s_tyubu02 (target): two-bone IK (35 / 55) plus a damped
   spring (Pipe_Vel, gravity 0.5) toward the target's record +0x2f0 (REC_PIPE_AIM, MODEL_PARTS +0x330,
   named from this one use -- its writer names it); the gate is Character[3].flags' two top bits
   (REFRACTION and SHADOW, see (u)); the ROM normalises base -> joint and never reads it (dropped,
   commented); nothing on screen yet: chara 3 never loads in the smoke run; (jr) **tiFreeBossData left a
   stub on purpose**: it _SunFree's gHeapJunc blocks held at pBossInfo +0xe8 / +0xec / +0xfc..+0x110 /
   +0x13c / +0x140 (and +0x140's +0x24 / +0x28), 4 bytes apart, gated by +0x90 / +0x91 (chain counts) and
   +0xc4, then pBossInfo itself and trigger variables 0x53, 0x54, 0x52, 0x56, 5 (-1); host pointers do not
   fit those slots, so it waits for the boss TU that allocates pBossInfo to decide its layout (as
   BossInfoPtr in wideXcmd_DebugPrint.c); (js) **the lens flares need a host draw call first**:
   tiLensFlaresInit walks ALL_TABLE +0x88 (EE words -> flare records: +0 u_short trigger part (whole word
   = parts index at init), +4 <- EE of partsPos[i] (its w + 128 is the sprite alpha), +8 <- EE of the
   part's MODEL_PARTS.pos, +0xc element count, +0x10 EE elements of 0x14: +0 tex EE (10 words: 0..6 go
   to sunSetupTBufferData as the decompiler shows them -- check its 8 slots against TEX_INFO by value --,
   6 is also the u size, 7 u0, 8 the v size, 9 v0), +4 tbuffer
   EE, +8 u_int size (half = size / 2), +0xc float distance factor, +0x10 EE of a 0x60-byte DMA/GIF
   packet: one REGLIST SPRITE PRIM 6 / RGBAQ (+0x38) / UV (+0x40) / XYZF2 (+0x48) / UV (+0x50) / XYZF2
   (+0x58), UV in 1/16 texels + 8 / 9) and sets the draw control 0x0193d6c0 (gaSetupDrawCtrlData 1,1,0,1,0,1,0);
   tiDrawLensFlares projects each enabled flare through matrix_global_view, keeps it inside 512 x 448,
   and spreads the elements along the line to the screen centre (256, 224) by len * factor. On the host
   that is a textured screen sprite at the GS window's coordinates: write a host task for
   AlyssaPan_DrawGaSprite (anchored through DisplayGeometry, not 512 x 448), then the pair.
   Slice 6 (2026-11-11): tiLightBelt2Update (the light-belt ribbons) and tiXFadeOut_Start2. Open from it:
   (jt) MODEL_PARTS +0x330 (pos330, = (jq)'s REC_PIPE_AIM) and +0x340 (matrix340) are named by offset:
   the belt reads +0x330 as the part's live position beside matrix200 (+0x200) and inverts +0x340 to put
   view-space vertices back into the part's draw space, so +0x340 looks like the part's draw (local ->
   view) matrix -- the matrix-tree TU (LinkMatrixTree / BuildMatrixTree) names both; (ju) a two-part belt
   sets flags43 = 3 (PARTS_FLAG43_BELT = WARPED | TRANS) on both parts and drawOnPos w = 1 on A: the
   flags' reader is still unread; (jv) LIGHT_BELT2_VERT (+0 slots EE, +8 n, +0xc / +0x10 the widths) and
   rows (+4, expected 3) are from this one use; the bank loop skips BANK_FLAG_ANIM_FORCE_STOP (0x4)
   banks, so that flag may mean more than the animation; (jw) init_black_drawctrl (sbss 0x0193d380) is
   set up for XFade_Type 2..4 (XFADE_TYPE_FROM_BLACK) but nothing staged here reads it: the cross-fade's
   draw (tiXcmd_TransItem.c or main.c's XFade pass) is its reader -- give the kinds 2 and 4 names there.
   Slice 7 (2026-11-11): tiParticleInit and tiLensFlaresInit (scene_init's set-up, data only);
   PARTICLE2_ENTRY +0x30..+0x3c / +0x42 / +0x58 / +0x7c, PARTICLE2_EMITTER, PARTICLE2.pos, LENS_FLARE /
   LENS_FLARE_ELEM and the sprite packet's words, ALL_TABLE.lensFlares (+0x88), drawctrl_flare (bss
   0x0193d6c0, a symtab global) with FLARE_DRAWCTRL_ARGS from the disassembly. Open from it: (jx) **(js)
   corrected**: MODEL_PARTS offsets count from the parts header (part i = header + i * 0x380, the record
   proper at +0x40), so the flare's +8 and the particle's +0x7c are MODEL_PARTS.pos (+0x130) and +0x58 /
   the emitter's +4 are matrix200 (+0x200); (jy) the flare element's texel rectangle reads TEX_INFO as
   u0 = maxu, u1 = maxu + minu, v0 = maxv, v1 = maxv + minv (REGION_REPEAT's fields reused as a sub-rect,
   named from this use), UV + 8 / + 9 (the ROM's 9 for v, kept); (jz) a particle ring record's draw
   object gets DRAW_OBJ_POS = the record's pos (+0x40) and +0x58 / +0x5c = obj6c + 0x40 / + 0x80
   (DRAW_OBJ_W58 / W5C, unread: the particle draw names them); PARTICLE2_ENTRY.w38 (-1 at init) waits for
   tiGenerateParticle; (ka) the translucence pair (tiChara_Translucence_Start / End / Start2 / End2,
   ~0x400 bytes) needs a host frame-buffer capture first: Start copies the draw buffer to 0xa8000 through
   tiVRAMViewer (a 512 x 448 sprite from VRAM), End draws it back over the character at alpha * 128 / 100,
   End2 wobbles six sprite corners with ahCos -- a host task (AlyssaPan capture to a texture, drawn
   through DisplayGeometry) before them. Nothing on screen: the smoke run never reaches a scene's
   draw (frame 120 memory-card text, then the flat grey of (ia)).
   Slice 8 (2026-11-11): tiCalc_Orbit and its motions (tiCalc_Trans, _Goal_Disable, _TransToGoal,
   _TransToGoal_Stop, _matL2V, _Refraction), from the disassembly; PARTICLE2 laid out (vel, accel, color,
   scale, refrIndex, five curve caches, mats -> PARTICLE2_MATS local / localView / rotView) and
   PARTICLE2_ENTRY's orbit kind, leafArg and five curve words. Open from it: (kb) **PARTICLE2.offset was
   renamed color**: tiCalc_Orbit writes the VRGB curve's signed bytes to +0x20..0x28 and alpha / 100 to
   +0x2c, and main_rtn_Particle2 adds them to ALL_TABLE.partsPos and writes the sum through DRAW_OBJ_POS
   into PARTICLE2.pos -- so partsPos, DRAW_OBJ_POS and PARTICLE2.pos are probably a part's colour (rgb,
   a around 128), not a position: rename them when the ga draw reads the word (gaBGT_DrawObject);
   ORBIT_GOAL_PART_OFF is PARTS_FLAG_FOG's bit (both read as "the part is hidden" so far); (kc) the
   motions integrate as the ROM does (step v dt + a dt^2, then v += a 2dt): kept, not a bug to fix; (kd)
   the curve readers (ssSetTexUVforCubeMap.c: ssParticleCashClear, tiParticleGetSizeValue, ..._Refractive_
   IndexValue, ssParticleGetVRGBValue / AlphaValue / UVValue) are declared with host curve pointers and
   short caches from the call sites -- their TU confirms; DRAW_OBJ_UV_U / _V (+0x78 / +0x7c) are their
   UV outputs, type unread. Nothing on screen yet (no particle bank loads in the smoke run).
   Slice left: tiDrawShadingOff with CaptureDBuffer and tiVRAMViewer, ExportDrawToTex, tiDrawLensFlares
   (after the host sprite), tiBumpSTValueUpdate / tiRefractionMapUpdate / SetSTValueForRefraction (the
   refraction trio), the translucence four (ka), tiCalc_Leaf / _Leaf_WIND / _Trans_WIND / _Twist (the
   remaining motions), tiGenerateParticle with tiGetmatL2WFromMeshEmitter (the emitter record:
   +4 matrix, +8 count, +0xc used, +0x10 used bytes, +0x14 vertices; host form still to decide), plus the
   six slice-3 helper stubs.
   Slice 9 (2026-11-11): tiHurikoReset, tiGetNewNormalData, tiGetCharaDirection, tiFreeBossData. (ke)
   tiHurikoReset works in MODEL_PARTS.matrix200 (+0x200) while tiHurikoInit took the rest pose in
   .matrix (+0x1c0): both as the ROM reads them -- the swing TU (tiCalc_HurikoOPEN / _CLOSED) says
   which frame the joints live in; (kf) HURIKO_JOINT.swing40 / swing50 are only reset to (0, 0, 0, 1)
   so far: the swing names them; (kg) tiGetNewNormalData's last chain looks at chain nc - 2 - closed
   and (closed) chain 0, as the ROM indexes -- kept; it takes the bank by BankOfHostPtr (the ROM's words
   are absolute); (kh) tiFreeBossData frees only pBossInfo itself: its pointer words (0xe8..0x110,
   0x13c, 0x140 and the sub-block's 0x24 / 0x28) have no host form until the boss TU that allocates
   them decides (BossInfoFree is a no-op, as wideXcmd_DebugPrint.c's BossInfoPtr); the five trigger
   variables (0x53, 0x54, 0x52, 0x56, 5) it resets to -1 are unnamed. Open from slice 10 (2026-11-11):
   (ki) 0x53 / 0x54 are now TRIG_VAR_CHAIN_B / _A (tiLightBelt2UpdateFor*_Battle: 2 drops the set's
   events, 3 registers the holding chains'); the belts' source / end words (0xf4 / 0xf8, 0x104 / 0x108)
   go through BossInfoWord, which answers none like BossInfoPtr: only the events run on the host until
   the boss TU gives pBossInfo's words a host form (with as, kh); (kj) PARTICLE2_ENTRY's chara (+1),
   dirJitter (+2), scaleJitter (+3), dir (+0x10), accel (+0x20), idleTicks (+0x38, was w38), tail
   (+0x4a), refrIndex (+0x4c), PARTICLE2.startPos (+0x70) and the emitter's count / used / usedFlags /
   verts are named from tiGenerateParticle / tiGetmatL2WFromMeshEmitter alone; the 300 in the life
   (lifeTenths * 0.1 * 300) reads as 1/300 s ticks, the 5 in the emit count as emitRate per 1/60 s
   (tiParticleInit's 60): both unconfirmed; DRAW_OBJ_W78 (two words cleared per new record) unread.
   Open from slice 11 (2026-11-11, the GS frame-buffer copies): (kk) **the host cannot draw a frame-buffer
   copy yet**: CaptureDBuffer, ExportDrawToTex and tiVRAMViewer build and send their packets as the ROM
   does and name the copy to the host (alyssapan_drawga.h AlyssaPan_DrawGaCapture / AlyssaPan_DrawGaCopy,
   added this slice); GaMeshRecorder records them in draw order (Mesh::Copy) and DrawGaFrame skips them.
   The renderer needs a render-target copy (the canvas so far, scaled into a picture kept per GS address)
   and a sprite that samples it; until then the shade-off draws nothing and the objects after
   ExportDrawToTex draw untextured (sunBGT_ExecTBufferData_VIF1 finds no pixels for the bound copy, so it
   unbinds) -- a host task, rendering/ga_copy.cpp, before the first scene that uses refraction or the
   shade-off; at wide aspects the copy must cover the widened frame, not 512 x 448 (the user's rule);
   (kl) the smoke run reaches ExportDrawToTex (log: first frame-buffer capture, 0xa8000, 512 x 512) but not
   tiDrawShadingOff (fShadeOff is the event command's); (km) tiDrawShadingOff's turned copies pass x + 512
   / y + 448 as the size, so each spans x .. 2x + 511 (kept, ROM quirk); (kn) SUN_TEX_ATTR +0x18 is now
   `bytes` (w * h * bytes per pixel, tiVRAMViewer's), still unread by sunSetupTBufferData;
   sunSetupTBufferDataFrom (QuatSlerp.c, PORT) takes a host record for the game's own records (data and
   stack) that SunTexEePtr cannot resolve.
   Open from slice 12 (2026-11-11: tiDrawLensFlares, tiBumpSTValueUpdate, tiRefractionMapUpdate,
   SetSTValueForRefraction, tiCheckHit_Face / _Capsule, tiCheckWindForce, tiCalc_Leaf_WIND): (ko) **the
   lens flares send GIF SPRITE packets the host does not draw** (gaBGT_ExecDrawCommandData_VIF1 drops
   them): a textured-sprite seam in alyssapan_drawga.h (the packet's RGBAQ, UVs and XYZF2 corners with the
   bound texture, 512 x 448 window anchored from DisplayGeometry at wide aspects) is host work before the
   first scene with a flare; (kp) TEX_INFO +0x18 / +0x20 (named minu / minv) read as sizes in
   tiLensFlaresInit (u1 = u0 + minu) and as divisors (1 / (minu + 1)) in tiBumpSTValueUpdate: they are
   probably width - 1 / height - 1 (or widths), with +0x1c / +0x24 the offsets: rename when the texture
   loader names them; (kq) ct3_types.h DRAW_OBJ_PACKET_QWC (+0x48) / DRAW_OBJ_PACKET (+0x68) disagree with
   alyssapan_drawga.h (vertexCount / the ST block); Clipping_Portal.c uses its own REFR_OBJ_* names
   (drawga's reading): reconcile in gaBGT_DrawObject.c's TU; (kr) tiBumpSTValueUpdate and
   tiRefractionMapUpdate skip a scene bank's group with groupCount - 1 unconditionally in the ROM (0
   would loop forever): the host guards it like the other loops; (ks) HURIKO_HIT_A_RADIUS (the a record's
   +0x20 float) and WIND.radius (+0x08) are named from tiCheckHit_Capsule / tiCheckWindForce alone;
   tiCheckHit_Capsule takes the bank on the host. **Next in this TU: tiCalc_HurikoCLOSED** (0xd20, a
   spring chain: reads HURIKO words 2, 3, 6..10 (+0x08 .. +0x28, unnamed), gGlobalNextTicksInterval / 5,
   the hits list (HURIKO +0x14) split into hFace / hCapsule by pairInA, flag 0x80000000 = winds on,
   flag 2 = a length spring, joints' +0x30 / +0x50 / +0x60 qwords), then tiCalc_HurikoOPEN, tiCalc_Leaf,
   tiCalc_Twist (slice 13 did the rest).
   Open from slice 13 (2026-11-11: tiChara_Translucence_Start / _End / _Start2 / _End2, tiXFadeOut_Draw,
   tiBGT_MoveImage, tiDR_XYZF2, tiCalc_Trans_WIND): (kt) **the host has no draw-target switch**: the ROM
   points the draw buffer at the effect buffer while translucent characters / ghosts draw, then lays it
   back over the frame. The port captures the frame before them (AlyssaPan_DrawGaCapture), lets them draw
   opaque and lays the capture back at 0x80 - FIX: exact for _End ((E - F) * a + F), **approximate for
   _End2** (one copy at the three shifted copies' combined alpha 1 - (1 - a)^3; the ghost's edge shimmer
   -- copies shifted by edgeLevel and stretched by the amplitudes * |cos(phase)| -- is lost, and the ghost
   layer's own texel alpha is taken as 0x80). An offscreen-layer seam (draw into a target, composite with
   per-sprite offsets) in alyssapan_drawga.h would make both exact: host work before the first scene with
   a ghost; (ku) CHARACTER.h536[0..3] named from _End2 (CHARA_EDGE_SPEED_X / _Y, _PHASE_X / _Y; [4] / [5]
   the amplitudes); (kv) tiBGT_MoveImage tells the host a capture only when its source is the draw buffer
   (tiXFadeOut_Draw's only use so far); other local copies have no host picture; (kw) init_black under
   init_black_drawctrl (ABE 0) is AlyssaPan_DrawGaFill at FIX 0x80; XFade kind 2 / 4 have no names yet
   (XFADE_TYPE_FROM_BLACK); (kx) the statics of this slice are named by address (drawctrl_0193d600,
   drawctrl_0193d540, tex_0193d440): the symtab's $n names were not looked up.
   Open from slice 14 (2026-11-11: tiCalc_Leaf, tiCalc_Twist, tiGetTransRotMatrix, tiGetDegFromVec,
   tiParticleSetPosition_WIND; the leaf's face and step/turn shared with _Leaf_WIND as static helpers, so
   callcheck reports their calls as missing from the two callers -- explained): (ky) **tiCalc_Twist reads
   +0x80 / +0x82 of its entry**, past PARTICLE2_ENTRY's 0x80 bytes (tiCalc_Orbit hands the entry itself,
   s6 = a1): ct3_types.h's PARTICLE2_TWIST (angle, radius) says so; the bank loader / particle TU should
   confirm such entries really are longer; (kz) tiParticleSetPosition_WIND's record (0xc0 bytes: +0x40
   position, +0x80 velocity, +0xac last time) and draw object (+0x10 position) are PTCL_* offsets in
   Clipping_Portal.c and host pointers in the prototype: ssCalcParticleOrbit's TU (the first particle
   system) owns that layout and decides; its first argument is the system header, whose first quad is the
   acceleration; (la) vecOne (sdata 0x01939e80, (1, 1, 1, 1)) is now defined in unassigned_data.cpp.
   ~~Left: tiCalc_HurikoCLOSED (0xd20), tiCalc_HurikoOPEN (0xbc4)~~ written 2026-11-11 (slice 15): **the
   TU's code is complete (verify: no smoke run reaches a pendulum yet)**. Open from slice 15: (lb) the two
   ROM functions are one static solver (HurikoSwing, closed flag), so callcheck lists their calls under
   the helper -- explained; (lc) HURIKO drag / mass / gravity / stiffness / damping / maxAccel (+0x08 /
   +0x0c / +0x1c..+0x28), hits (+0x14), HURIKO_FLAG_SPRINGS (2) / _SEAMS (0x10) / _WIND (0x80000000) and
   HURIKO_JOINT downLen / leftLen / rightLen / hit / vel / step are named from this reader only, by what
   the arithmetic does: the bank data's real values (a skirt or curtain in a stage bank) should confirm
   them; (ld) dt is always gGlobalNextTicksInterval / 5 (the ticks argument, gNextTicksInterval for
   character banks, scales only the wind), as the ROM does -- a paused character's cloth still swings by
   the global clock; (le) hFace / hCapsule (50 slots each) are bounded on the host (PORT), the ROM is not;
   (lf) the max-accel clamp's length is taken from xyz (the vaddbc pair's lanes read as the usual sum).

8. **(2026-11-11, the user: "queue the title blockers") The way to the title screen.** A probe run
   (16:25, reverted: every stubbed x command answering XCMD_DONE, AA_movie returning at once) showed the
   path after ATLD: the script's **hhXcmd_LoadSE** (a stub answers XCMD_WAIT, so the script waits on it
   for good), then two movies, **m_capcom.mov** and **m_title.mov** (data/movie2; the decode loop hangs on
   the stub decoder thread AA_videoDecMain), then **hhXcmd_PlayADS** (the title music stream); after that
   the run stopped presenting frames near frame 1860 and **TITL.BIN** (bank 0, the title: the clock logo,
   "CLOCK TOWER 3", New Game / Load Game / Options / Cinema Theater, cloud textures, the content warning;
   `ct3pack.py tex TITL.BIN` -> work/ref/TITL/) never loaded. Queued in this order: game/hhXcmd_DressUp.c
   (LoadSE and PlayADS), the host task alyssapan/core/movie_skip.cpp (the smoke runs' cinematic skip), the
   host task alyssapan/core/title_trace.cpp (why frames stop after PlayADS), ga/pad_button_read.c (gaPadRead:
   the title menu needs the pad). TITL draws through the same ga path as ATLD; the real movie decoder
   (AA_mpegTS.c, AA_videoCallback.c, AA_GetDataWord.c) comes after the title.
   **game/hhXcmd_DressUp.c (verify, 2026-11-11): all 22 functions** -- LoadSE / PurgeSE / PlaySE /
   StopSE, the three ...ChangeSE, EffectSE, the ADS three, the SQ (MIDI) three, the Sp1 pair, DressUp,
   FootSEConversion. A smoke run now passes LoadSE and stops in m_capcom.mov's decode loop (log: Open Mpeg
   File "m_capcom.mov"; shots end at frame 420, flat grey): **the next blocker is the movie skip**
   (alyssapan/core/movie_skip.cpp, queued next); PlayADS runs once the movies are skipped. Open: (ka2)
   hhXcmd_DressUp passes Round2ChangeChar the script variable argv[0]'s value, so (eo)'s ROUND2_* is
   what the scripts store there -- the script that sets it confirms the costume numbers; (kb2) the floor
   word: FootSEConversion reads the material byte +0x19b of the object CHARACTER.w300[0] names (so w300[0],
   CHARA_TARGET, is the floor / ground object, not a lock-on target: rename it with its writer); its host
   FootGround answers none (with (v)), so no footstep sounds yet; materials 0..11 (two left / right SEs
   each; stalkers on 9..11 use Alyssa's C_0000 bank); (kc2) the stalkers' footstep banks give their
   Character[] slots: E_HA 2, E_RY 3, E_ON 4, E_SM 5, E_SW 6, E_BA 0x20 (one pair, SE 8 / 9, on every
   material) -- they agree with HIT_CHARA_* (wideXcmd_DebugPrint.h: 2 E_HAMMER, 4 E_ONO, 5 E_SMA, 6 E_SWO);
   (kd2) named from this use only: FOOT_SE_NO 0x1482a, event 0xd2 (Alyssa stepped), SE_PARAM_8000 (sent with
   SD_SE_PARAM_NAME for reqArg2 >= 1000, SdSetParamSE reads no argument for it), SD_SE_PARAM_30 = the pitch
   (PitchChangeSE), the 0x96 hold that spares banks 0..5 in PlaySE but not in PlaySESp1; (ke2) PORT:
   StopADS / VolumeADS pass 0 for argv[0] (SSStopADS / SSVolumeADS drop a0) and VolumeADS drops argv[2]
   (SdVolumeStream takes one argument); StopMIDI copies at most 15 bytes of gSQLd[0].name (the ROM's
   strcpy into 16).
   **alyssapan/core/movie_skip.cpp (verify, 2026-11-11):** [game] skip_intro (ALYSSAPAN_SKIP_INTRO; off for
   players, build.py --run sets it) makes AA_movie end each movie at once through the PORT branch
   AA_movieSkipped (movie.c): nothing decoded, TRIG_EVENT_MOVIE_SKIPPED registered where readMpeg's pad
   check would allow a skip, the GS cleared to the fade colour, tiFade handed on, memory map freed, banks
   reloaded. **Correction to the DressUp note above:** the run did not hang in the decode loop; the script
   waited on the stub **aaXcmd_MovieFedeOutCancel** (game/PicPaz.c, CT3 0x018da0a0: sets AA_MvInWhite /
   AA_MvOutWhite / AA_MvFedeOut2Md / AA_MvFedeOutCut from its arguments, answers XCMD_DONE), so the movie
   never started. A reverted probe (that stub answering XCMD_DONE) showed both m_capcom.mov and m_title.mov
   skipped (Movie Play Start / skipped / End for each) and frames running on past 900 (the old run stopped
   at 420), still flat grey: TITL does not draw yet. game/PicPaz.c is queued first (Claude, scheduled run
   16:42): its slice should start with aaXcmd_MovieFedeOutCancel. Open: (kf2) the skip leaves out the
   ROM's 60-frame fade after a skip (no picture to fade); (kg2) whether hhXcmd_PlayADS runs after the
   movies is not yet seen in the log (it prints nothing): title_trace.cpp's session checks it.
   **game/PicPaz.c (verify, 2026-11-11):** all 9 functions (the aa x commands: movie fades, the card file
   list / cursor / details, the work buffer, SceneEndBank by name; aa_search, aa_to_big). The smoke run
   now passes aaXcmd_MovieFedeOutCancel: m_capcom.mov and m_title.mov start and are skipped; the shot stays
   flat grey (TITL draws nothing yet): title_trace.cpp is next. Open: (kh2) the fade words map onto movie.h
   as "WHITE" = AA_MV_WHITE, "BLACK" = AA_MV_FADE (fade in from black), "-" = AA_MV_BLACK (none): AA_MV_BLACK
   reads oddly for "no fade-in" -- rename the AA_MV_* set when movie.c's fade code is re-read; (ki2) the
   card commands' variable / event numbers come from the script: nothing exercised them yet (the card
   screen is past the title); (kj2) the TU now ends at 0x018dab60 (aa_to_big's split loop test
   FUN_018dab3c), game/asXcmd.c starts at asXcmd_PanicSet 0x018dab60.
   **alyssapan/core/title_trace.cpp (done, 2026-11-11 17:25, no code):** with PicPaz.c in, the frames
   **no longer stop**: a 90 s run presents 2,820 frames and a 240 s run 7,200+ (the 16:25 probe's stop near
   frame 1860 came from its own every-stub-answers-done hack, not from a game loop). After m_capcom.mov and
   m_title.mov the ATLD script goes into the **attract cycle**: demo movies from data/movie3 one after
   another (d02, d04, d03, d01, d01, d04 in the 240 s run, a random pick each time), the shot flat grey
   throughout, and InitLoadSceneChar never runs again, so **TITL.BIN never loads**. main2.c loads TITL
   only on a scene restart with PowerOn set (ATLD at power-on, TITL after), so the ATLD scene is the
   "press start" screen: the script waits for a button between demos, and **gaPadRead is still a stub**
   (ga/pad_button_read.c, CT3 0x018a0b40; main.c ControllerMain and movie.c's skip both call it), so
   nothing ever presses START. **The blocker is ga/pad_button_read.c**, already next in the queue; its
   check run should pass a scripted START (build.py --pad) after m_title.mov (about frame 900) and look
   for TITL. Open: (kk2) hhXcmd_PlayADS (the title music) still prints nothing, so whether it runs
   during the attract cycle is unseen (kg2 stays open); (kl2) the script's start test (which event or pad
   word it reads) was not read: pad_button_read.c's session sees it when START does or does not end the
   scene.
   **The title draws (2026-11-11 18:55, the user's live session).** The grey was the title itself drawn
   untextured: two of ATLD's texture packets were empty because SunTexEePtr resolved their records (EE
   0x412b00.., past 0x410000) into free slot bank 6 -- the GLBL heap (EE 0x400000, 3 MB) and the scene
   slots (0x410000 on) share EE addresses in the ROM, and a free slot always has its host image. The three
   EE resolvers (SunTexEePtr, BankOfEeAddr, ssGrpPortalCheck.c HitObjectId) now skip BANK_STATE_FREE banks.
   A 40 s run shows the clock logo, CLOCK TOWER 3, NEW / LOAD / OPTIONS with the cursor and the copyright
   lines, at 4:3 and 16:9 (the 3 and the menu come late in the intro animation: let runs go 40 s or more).
   Also: ssXcmd_SetAllControlDisable (game/wideXcmd_TriggerInit.c) was the stub the script waited on after
   START; with it, START on NEW reaches tnXcmd_LoadWait E_00 (Alyssa's bank, the first scene). Open: ~~(ko2)
   at wide aspects the backdrop panels end short of the window edges with visible seams~~ fixed
   2026-11-11 (alyssapan/rendering/title_wide.cpp, host task): the ga frame is scissored to the 4:3 box
   (Decisions), and the lighter band was CLAMP_1 REGION_REPEAT drawn as clamp (hazards.md); 4:3, 16:9
   and 21:9 shots show the same title, pillarboxed, with the clouds and the tower behind the logo; (kp2) ExportDrawToTex binds the screen copy,
   which has no host picture, so the objects after it draw untextured until rendering/ga_copy.cpp (kk);
   (kq2) the next title-path blocker is the E_00 load after NEW.
   **For 3D after it** (not queued yet): a ga pass of its own with depth and fog, widened from
   DisplayGeometry (UiCanvas has neither and squeezes), gaObjectClippingCheck and ga/gaDrawObject.c (the
   clipped path most scene objects take), light1 / light2 lighting and billboards (item 4 (e) (f) (g)),
   then characters (skinned parts) once a room loads.

## Decisions

- 2026-11-11 (Claude, scheduled run 19:15, for the user to overrule): **the ga 2D frame stays 4:3.** The
  title's backdrops are quads the size of the GS's 512 x 448 frame (some run a few texels past it, others
  -460..970 for the rotating sky), the ROM leaving the edges to SCISSOR_1. At a wide aspect the canvas
  draws that frame squeezed about the centre and now scissors it to the 4:3 box (seams.cpp gaScissor):
  black pillars, no seams, nothing stretched. Taken from the user's YamiPan decision (textured backdrops
  stay 4:3, YamiPan commit ba4ceff). Extending the panels to the window edges would stretch their
  pictures or show the larger quads over nothing. The 3D pass, widened from DisplayGeometry (item 8 "For
  3D after it"), does not take this scissor: only the ga meshes UiCanvas draws do.

- 2026-11-11 (Claude, scheduled run 16:42, for the user to overrule): **game/PicPaz.c queued first.** Its
  stub aaXcmd_MovieFedeOutCancel is the real wait before the title movies (item 8); the movie skip
  (skip_intro, reused from the config rather than a new key) is in place behind it.

- 2026-11-11 (the user): **the title blockers go first.** Queue: game/hhXcmd_DressUp.c, host tasks
  alyssapan/core/movie_skip.cpp and alyssapan/core/title_trace.cpp, ga/pad_button_read.c, then the rest
  (Clipping_Portal.c's remaining slice after them). Item 8 has the path and the evidence.

- 2026-11-11 (Claude, scheduled run 09:22, for the user to overrule): **game/tiXcmd_TransItem.c queued
  ahead of the rest of xcmd_tn.c.** The smoke run's script waits on the stub tiXcmd_XFade_Break every
  frame after the memory-card text; its TU (the slice with the x-fade commands) comes first.

- 2026-11-11 (Claude, scheduled run, for the user to overrule): **game/ssGrpPortalCheck.c (the
  dispatcher ssTrigExecuteActCmd) queued ahead of the rest of xcmd_tn.c.** The ambient command is
  written but nothing calls it until the dispatcher runs command 0x514; more x commands would not change
  the picture either.

- 2026-11-11 (Claude, scheduled run, for the user to overrule): **game/xcmd_tn.c queued ahead of
  ga_atld.cpp's rest.** The host draw of ATLD is right (a forced ambient shows the reference), and what
  keeps the shot black is the ambient colour the event command tnXcmd_Scene_Ambient_Change sets; no host
  work can change the picture before it runs.

- 2026-11-10 (the user, after the ui_canvas.cpp run): **the draw path and the camera go ahead of the
  triggers.** Queue now: `ga/gaBGT_DrawObject.c`, `game/asEyeForcusUP2CamMat.c`, then
  `alyssapan/rendering/ui_canvas.cpp` again (the ATLD picture check: work/ref/ATLD/ at 4:3 and wide),
  then `game/ssGrpPortalCheck.c` and the rest as before. Reason: ATLD, the first screen, is world-space
  quads drawn by gaBGT_DrawObject_NoClippingCheck2 under the scene camera; no trigger takes part.

- 2026-11-10 (the user): **the queue works toward the first picture** before more player logic. Pinned
  in this order: the host task `alyssapan/iop/fsys_server.cpp` (FSYS_IOP on io::Disc: `sdk/sif.cpp`
  answers zeros, so no bank loads), the host task `alyssapan/rendering/ui_canvas.cpp` (the first 2D draw
  through UiCanvas, at 4:3 and wide), then `game/ssGrpPortalCheck.c` (triggers), `game/asEyeForcusUP2CamMat.c`
  (camera), `ga/gaBGT_DrawObject.c` (the 3D draw path, with CT3's own host draw module). `wideXcmd_DebugPrint.c`
  (Alyssa's controls) waits behind them: none of it can change the screenshot until these exist.

- 2026-11-10 (Claude, scheduled run, SdMasterVolumeSetSE.c slice 2, for the user to overrule):
  **`SD_SE_LD` is `layout: host`** (its +0xa0 buffer is SdAlloc's host pointer); the IOP-bound records
  inside it (`spuCom` +0xc0, `SD_SE_REGISTER` +0x100) keep EE layout, and IOP addresses stay `u_int`
  (iop::SystemMemory places them below 4 GB). `SD_SQ_LD` stays `layout: ee` until SdLoadSQProc's session
  meets the same buffer word and decides likewise. sceSifAllocIopHeap / sceSifFreeIopHeap and
  sdremote's sceSdTransToIOP are now host code in sdk/sif.cpp (they were declared, never defined).

- 2026-11-10 (Claude, scheduled run, for the user to overrule): **the empty queue was refilled in the
  order a smoke run reaches stubs** (the run log's first stub per TU): ga/gaDmaSync_VIF1.c (taken),
  game/IOPGetFileSize.c, game/SdMasterVolumeSetSE.c, ga/gaPadInit.c, game/movie.c,
  game/wideXcmd_DebugPrint.c, game/tkXDTrg_GetSceneHash.c, game/asUseArrow.c, game/asMothRouteSearch.c,
  game/asXcmd.c, game/asEyeForcusUP2CamMat.c, game/ssHitCheckAttackToScn.c, game/ssSetTexUVforCubeMap.c,
  game/ssGrpPortalCheck.c. The shims of item 2 stay unqueued: no run reaches them yet.
- 2026-11-09 (the user): **AlyssaPan ports Clock Tower 3** (the copy with debug symbols, `SLUS_206.33`,
  already loaded in Ghidra). It **keeps YamiPan's priorities and design guidelines**, **reuses the shims of
  the other Pan projects ("the windows handling and all the rest")** and **builds on YamiPan's C++
  adaptation** of the Pan layer.
- 2026-11-09 (the user): **scheduled runs**: the desktop app's task `alyssapan-next-tu` (cron
  `*/20 * * * *`, prompt = `tools/reverse/routine.md`) advances the port by one step per run -- the next
  bootstrap step while item 0 above has one, then one TU or host task -- gated by `preflight.py`. Pushing
  each run's commit to origin/ct3-port (`github.com/wagrenier/AlyssaPan`) follows YamiPan's approval of
  2026-10-31; never force. The queue after the bootstrap: `game/main.c` (pinned 0x01834440..0x0183aed8,
  29 functions), then the host tasks `alyssapan/core/window.cpp` (shims a) and `sdk/libmc.cpp` (the card).
- 2026-11-09 (Claude's call at bootstrap, for the user to overrule):
  - The repo is `C:\Users\willi\Documents\git\AlyssaPan`, a new git repository on `main` with nothing
    committed yet; the work branch for the TU loop is to be **`ct3-port`** (the tools refuse `main`).
  - The annotation tag is **`CT3`** (`// CT3 0x...`); provenance tag `game-symbol` for every symtab name.
  - The Pan layer came from YamiPan's HEAD (17e0108, then 93c73f0's audio), not from its working tree
    (uncommitted edits there stayed out). FS2's own host modules stayed in YamiPan (cpp-port.md "Left in
    YamiPan"); the renderer was trimmed to the 2D path; TextureStore decodes a TIM2 file directly (FS2's
    `Tim2Texture` gone); the movie seam takes the picture count; sdk/sif.cpp starts IOP modules from a
    table (empty) instead of booting RXSNDDRV; FS2's `mcerror.cpp` dropped.
  - Game directories: `src/game/` for the game, `src/ga/` for the `ga*` library, `src/iop/` for IOP code;
    file names from the game's strings where they exist (`movie.c`), built as C++ like every sibling.
  - The game data stays in the user's folder `C:\Users\willi\Documents\Decomp\Clocl Tower 3`; nothing from
    it is committed. A data folder is one with `DATA\CT3PACK.DAT` and `IOP\`.
  - CT3's Ghidra program lives in YamiPan's project (`CLaude Decomp`), as the user set it up: every call
    passes `program="SLUS_206.33"`.
  - No autopilot.py (a terminal loop): the scheduled task is the automation (see the decision above).

The user's standing rules, carried from the earlier ports (SKILL.md "The user's rules" says how they
apply here):

- 2026-11-07 (the user, YamiPan): **time limits**: a session has 60 minutes; no game run lasts more than 10
  minutes, and every background command has a limit before it starts.
- 2026-11-04 (the user, YamiPan): **Japanese stays Japanese**: Shift-JIS text from the ROM goes into the
  source as the Japanese itself (reconstruction.md "Japanese text"). CT3 (USA) is English, but its data may
  still hold Shift-JIS.
- 2026-11-03 (the user, YamiPan): **the Pan layer is modern OO C++; the game layer stays authentic**
  (cpp-port.md).
- 2026-11-02 (the user, YamiPan): (1) **no GS emulator, not even for textures**; decode assets at their
  source; (2) **name the patterns**: repeated values and flag bits become enums / `#define`s / `constexpr`
  as the code is read; (3) **any aspect ratio is the port's #1 feature**; (4) reuse the siblings'
  abstractions instead of reimplementing the basics.
- 2026-11-02 (the user, YamiPan): **source code first**: name what a TU needs, reconstruct, do not seed
  names for their own sake (CT3's symtab makes this easy).
- 2026-10-31 (the user, YamiPan): pushing each finished commit to the work branch's origin is approved;
  never force. (AlyssaPan has no remote yet.)
- 2026-10-30 (the user, RosePan): **skip cinematics in smoke runs** (`skip_intro` in the ini; smoke runs
  force it off so pad timing holds; the smoke pad holds START through films).
- 2026-10-28 (the user, RosePan): the port displays correctly at any aspect ratio, from one
  renderer-owned source; never a fixed 4:3 / 640x448 in host draw code.
- 2026-10-22 (the user, RosePan): drawing never goes through an emulated GS; the game's draw decisions
  are reconstructed and the packet-writing step calls a native host draw module per subsystem.
- 2026-10-15 (the user, ReiPan): methods, not free functions (C++ games). CT3 is C: free functions, as the
  symtab names them.
- ReiPan / RosePan (the user): **the picture is the test**; a drawing TU is `done` only when the shot
  matches the game's own assets; complete code nothing can show yet is `verify`.
- ReiPan / RosePan (the user): automated porting runs at effort xhigh or above; sessions never start
  other sessions, agents or scheduled tasks, and never edit `autopilot.py` or the routine.
- ReiPan (the user): an empty queue means re-assess, not "follow the frontier".
- MioPan (the user): debug probes are stripped before committing; frame smoothing interpolates, never
  extrapolates, and adds no input lag.

## Open questions

- (2026-11-11, game/asXcmd.c) all 20 functions written: **verify** (script commands; no smoke run
  reaches a scene script yet; main.c's AttackTargetStack clears run). To confirm: (a) AttackTargetStack
  (0x0193fa30, now defined in asXcmd.c) is pairs {bank, target}; its reader is not written, and
  "target" as a Character[] slot is a guess from the name LockToTargChara; only the bank half is ever
  cleared (kept as the ROM). (b) asXcmd_SetMoth names MOTH by use without renaming: w20 = the route's
  point count, mem = the route (short[w20], gHeapJunc), w22 = the route step, w02 2 = following a route,
  w14..w18 = its three numbers; rename them in asMothOperate's TU. (c) PanicSet's source 8
  ("PanicValuePriority") is the ROM's rise[8], i.e. PANIC_INFO.gauge: PanicSet (QuatSlerp.c) now adds
  to gauge for PANIC_RISE_GAUGE instead of indexing past the array. (d) CHARA_BHV_W518 is the motion
  ALYSSA_ComeHere plays (CHARA_COME_HERE_MOTION, -1 none); CHARA_AIM_SPEED / _BANK / _PART are also the
  auto-aim command's. (e) asXcmd_AttachObject writes a short record over MODEL_PARTS 0x70..0x75 (the
  omniChanging / omniSteps words): PARTS_KIND_ATTACHED 0x1e parts; the reader (the attached part's
  update) is not written. (f) CAM_QUAKE.h08 is the hold (duration = fadeIn + hold + fadeOut). (g)
  PlayMotionByNowMotion sees no motion table until CHARA_MOTION_TABLE has a host form (as
  ahRelocateSunPs2ObjectArry.c); SeekBarSetting returns when the host SeekBarX[bank] is NULL; SetHitPoint
  skips a division by 0 (the EE does not trap).
- (2026-11-11, game/asMothRouteSearch.c) the whole TU written (asInitMothInfo, asDestroyMothInfo,
  asMothRouteSearch): **verify** (the route search runs only from asMothOperate, still a stub). MothInfo
  moved out of unassigned_data into MOTH_INFO (asMothRouteSearch.h; wideXcmd_DebugPrint.c reads
  MothInfo.bank). **To confirm in asMothOperate / asXcmd_I_have_x_Mothes:** (a) MOTH (0x50 on the EE) is
  layout: host with a void pointer at EE 0x1c; most fields stay wNN; state 7, routeSearch (+0x26), timer
  (+0x28), target (+0x40) and bank (+3, the collision bank, 0 when armed) are named from one use each;
  (b) MothInfo.stage is 1 when the armed bank is MT1A.BIN, and endTicks[stage] is stamped once (it starts
  at -1 from asInitMothInfo, so the ROM test for 0 only fires after something clears it: who?);
  (c) the route search tests each ring first end twice (it builds the next candidate before stepping):
  kept as the ROM; (d) TRIG_EVENT_MOTHS_GONE 0x47 / _ARMED 0x8c are named from these two uses.
- (2026-11-11, game/asUseArrow.c slice 2) asArrowOperate written: **the file is complete (verify: no
  arrow on screen until a stage with the bow runs)**. The states were renamed by what this function does
  with them: 4 ARROW_STATE_CHARGE (MOTION_BOW_DRAW raises a kind 0 arrow's level past 0x95 / 299 ticks),
  5 ARROW_STATE_FLYING (released on MOTION_BOW_SHOOT or _HOLD), 7 ARROW_STATE_RETURN (hidden, then READY);
  ARROW_HIT_STUCK / _END replace ARROW_HIT_KIND0 / 1 (END also when the arrow passes its mark). **To
  confirm:** (a) HIT_TABLE +0x2a0 / +0x2a4 (attackObjs, attackObjCount: EE words of objects whose +0xda id
  is an arrow's hitId) are named from this reader; (b) the ssHitAddAttackRequest arguments here are
  (object index, the arrow bank, 1, attack 2..6 / 0x20 by TRIG_VAR_STAGE, the target character, 0 or 1,
  0, -1 or 0x12): they do not fit the prototype's names (chara, atk, a2, power, a4, a5, motion, bank) from
  the one earlier call site -- the collision TU names them; (c) the target is the first character 1..34
  with CHARACTER_FLAG_SHADOW set and CHARACTER_FLAG_NO_HIT clear; (d) trigger variables 3 / 0x33 / 0x35
  (TRIG_VAR_ARROW_LEVEL / _STATES / _LEVELS, a nibble an arrow) and event 0x65 (TRIG_EVENT_ARROW_LANDED)
  named from these writes; (e) **PORT:** pSrcMPstock / pDstMPstock are EE words with no bank: ArrowStockPos
  takes the loaded bank image with the highest BankImageEE at or below the word -- a BankOfEe helper in
  ct3_types.h would serve every such reader; (f) shootArg1 / shootArg2 are the speed and the power (scaled
  at the release); the power has no reader here.

- (2026-11-11, game/asUseArrow.c slice 1) 10 of 11 functions written; **asArrowOperate (0x1230 bytes,
  CT3 0x01866d80) is the next slice** (stage: work/stage/tu_game_asUseArrow.c). ARROW (0x50) and
  ARROW_INFO (host layout) are in asUseArrow.h; HIT_TABLE gained objHitCount / objHits (+0x2b0 / +0x2b4)
  and HIT_OBJ_REC (0x20) in wideXcmd_DebugPrint.h, named from these readers. **To confirm:** (a) the
  HIT_OBJ_REC.object word and its +0xda short are read as EE words of ArrowInfo.bank (the collision TU
  says whose bank); (b) ARROW_STATE_5 and ARROW_HIT_KIND0/1 are named from asArrowOperate's writes only;
  (c) the two bow part hashes (ARROW_PART_HASH_NOCK / _AIM) are named by use: dir = aim - nock;
  (d) asGetTheArrowRad sets kind 3 (BOW_ARROW_NONE's value) on a steep arrow: ARROW_KIND_STEEP is a
  guess at why; (e) asAttackedArrowInformation / asArrowInformation return ARROW * (the ROM an EE
  address; their callers tiBrain_Damage_BOSS1, ssXcmd_AXE_CheckHitAndCompDir are not written yet).

- (2026-11-11, game/tkXDTrg_GetSceneHash.c slice 4) tkXDTrg_ActorProc_Ini (the old form, 15+ arguments)
  read: the file's 50 functions are all written (verify: no door on screen yet). It finds the door's parts
  by name, "<prefix>_Dor<room>_<door>[_<suffix>]" with room = argv[3] (the other room, also otherBank's
  bank) and door = argv[0]'s 2 digits; argv[2] / argv[3] go to camRoom[0..4] / [5..9]. **Answered by the
  names:** +0x4c (h4c) is the TDS_Dor part and +0x4e (trigUse) TDS_Dor.._Cam; +0x48 (h48) is SE_Dor.._n,
  +0x4a / +0x46 SE_Dor.._o / .._c (so the "sound objects" are model parts: a sound's position); a kind's
  +2 / +4 are SE_Dor.._Ham_n / _Ham_o (Vit_ for kind index 2): +4 is the kind's open sound part, not
  unknown. Rename h48 / h4c / kind.h4 when their readers are read. **PORT:** Ini_Error is called without
  its (id, bank, door, name) arguments (the host's prints nothing).

- (2026-11-11, game/tkXDTrg_GetSceneHash.c slice 3) The dispatcher, the Ini_New / IniMon parsers, AlsOpn /
  AlsClo, MonDraw, CharacterInit and the x command read. **Answered:** slots 0 / 2 are Alyssa's open / close
  (DTRG_SLOT_ALS_*); +0x40 is the warp part; DTrgPraMon and DTrgPraAls are written by door 99's trigger
  (tkXDTrg_CharacterInit); flag 0x2 comes from a 'u' at the code's 2nd character, 0x8 from a non-'W' 7th
  character. **Still guessed:** +0x48 / +0x4c (written by Ini_New, no reader seen), the kind entries' +4 / +6
  (IniMon writes them), events 0x32 (DTRG_EVENT_NO_OPEN) and 0xd2 (DTRG_EVENT_DOOR_SOUND), Character flag
  0x10000000 (CHARACTER_FLAG_DOOR), AlsOpn's step 5 that skips the wait. **PORT choices:**
  tkXDTrg_ActorGetSceneHead returns its head (the ROM is void, callers read v0); with no head free before
  the clean-up the host takes one the clean-up freed (the ROM writes DTrgHed[-1]); Ini_New with no
  detector on the portal part leaves otherBank (the ROM reads NULL). **Left (slice 4):**
  tkXDTrg_ActorProc_Ini (0x018ebf90, stubbed: answers 1 and resets the record), which builds the part names
  ("TDS", "_Cam", "CP_Dor", "Ugd", "Ham_n", "Vit_n"...) from the scene's name.

- (2026-11-11, game/tkXDTrg_GetSceneHash.c slice 2) Door actor procs read. **Answered:** events 600 / 700
  are OPENED / MOVING (architecture.md "Door actor records and procs"); CloseCancel's fields are slots 3..5.
  **Guessed, to confirm with tkXcmd_DoorTrigger / tkXDTrg_ActorProc_Ini:** flag 0x2 (DTRG_FLAG_SIDE: picks
  the kind's other speed and, at Axe / Ham doors, the weapon sound -- the side the stalker breaks through?),
  flag 0x8 (DTRG_FLAG_SE_ALT), kind 0, slots 0 and 2, record +0x40 / +0x48 / +0x4c and the key entries'
  +4 / +6, who writes DTrgPraMon. camRoom is declared 10 bytes (0x2e..0x38): in a camera record the name
  may overlap otherBank / part; only its first 4 chars are read. tkXDTrg_MonDraw is stubbed (next slice).

- (2026-11-11, game/tkXDTrg_GetSceneHash.c slice 1) **To confirm when the door actors are read:** the
  meaning of door events 600.. / 700.. (DOOR_FLAG_600 / _700, cleared with OPEN by tkXcmd_DoorFlagClear);
  DTrgHed +0 / +7 / +8 and the DTrgPraDor fields CloseCancel clears (named by offset); SCN_ENTRY +6 / +7
  (0x28 / 0x03) and +0xc..0xf (branch 3), Lnk / DorExt / DorPol (copied raw). WaitUntil answering 1 (done)
  when its comparison FAILS is the ROM's (disassembly-checked): if the scripts read as "wait while", fine;
  if a smoke run hangs on one, look here first. PORT: tkBri_DoorDraw's neighbour searches are bounded to
  its 8 slots and its name buffers widened to 16 (the ROM's strcat into a 4-byte local overruns it).
  tkBri_DoorDraw reads Character[0].roomHash (the ROM's literal slot 0, not Player_BankID).

- (2026-11-11, game/tiXcmd_TransItem.c slice 7) **Named from one use, to confirm where they are read:**
  CHARA_JUNC +0x00 `poleCount` and +0x0c `poles` (was `buf`: u_int[poleCount] EE addresses of the
  ghost poles' part positions, tiXcmd_SetGhostPole; the ghost AI that reads them is unread);
  DRAW_OBJ_PACKET_QWC (+0x48, the ga draw object's packet is this + 1 qwords) and DRAW_OBJ_PACKET (+0x68,
  its relocated packet word), both from tiXcmd_CopySunPs2Object only. CopySunPs2Object copies 0x100 bytes
  of the texture upload packet (TI_TBUFFER_COPY_SIZE) where SUN_TBUFFER_DATA is 0xd8: kept as the ROM
  (bank packets are probably 0x100-spaced; unverified). PORT deviations: CopySunPs2Object returns when a
  part has no draw-list record (the ROM reads through NULL). SetGhostPole leaks a block already there, as
  the ROM. callcheck lists atoff / ssTrigGetVariable as missing from wideXcmd_GetRate: they live in its
  static helper TiRateOperand -- explained, not a gap.
- (2026-11-11, game/tiXcmd_TransItem.c slice 6) **tiXcmd_CheckMCFile's play-time minutes look like a ROM
  bug, kept:** the ROM computes minutes = (time - hours) / 1800, taking off the hour *count* rather than
  hours * 108000 frames, and never wraps them at 60 (seconds do wrap), so a save past one hour shows
  minute digits from the total minutes' last two digits. The port keeps the ROM's arithmetic (disassembly
  0x018e7354..0x018e7418 confirms the subtraction). Whether to fix it is the user's call. Also kept from
  the ROM: VibrationON_OFF reads its variable without the 0xffff mask the other commands apply;
  VibrationStart's event / variable motors (argc 5 / 6) are passed unmasked. The vibration commands'
  effect waits on tiVibrationOn / tiVibrationOff (Clipping_Portal.c stubs). GetCharaData's two keys are
  "HP" (0x01936df8) and "HP_MAX" (0x01936e00).

- (2026-11-11, game/tiXcmd_TransItem.c slice 5) **tiXcmd_GetItem's two out-of-range writes, guarded on
  the host:** arrow id 3 (ARROW_UNLISTED: counted, never listed) increments ArrowData[3], but the symtab
  gives ArrowData 12 bytes (3 ints, sbss 0x0193f858): the ROM writes the unnamed word at 0x0193f864. The
  port counts only ids 0..2. Either an arrow kind 3 exists whose count lives in that word (then ArrowData
  wants 4 ints and the save image's arrowData with it) or no script picks up arrow 3: a script reader
  answers it. Item id 0 would write ItemList[-1] (KeyItemList[23] in the ROM): not written. Also named
  from this one use: TRIG_VAR_GOT_ITEM_KIND (global 0x23), ITEM_ROW_EVENT (+0x14), LETTER_DATA_EVENT (+8),
  ITEM_FIXED_SLOTS (items 1..4 own a slot).
- (2026-11-11, game/tiXcmd_TransItem.c slice 4) **KeyItemData in the save image?** KeyItemData (data
  0x0192f170, 100 x 0x18 = 0x960, right after ItemData) is now defined in src/lib/unassigned_data.cpp from
  the ELF. Character_Init.h's save struct has `aiData[0x960]` at 0xd590, right after itemData and "not
  written here": the same size as KeyItemData, so it is probably KeyItemData that data_save_set /
  data_load_set skip or copy elsewhere. Read the save path's disassembly before the save TU is done.
  Item-row fields keep their offsets (ITEM_FIELD_04..10) until a reader of the rows names them; table 2
  (between key items and arrows) does nothing in Set/GetItemData. XCMD_SCOPE_LITERAL (2) and COMPARE_*
  were named from CompareVariable alone.

- (2026-11-11, game/tiXcmd_TransItem.c slice 3) **Named from one use:** KEY_FRAME_* (tiXcmd_KeyNumToFrame's
  button picture frames, by the libpad bit each matches; anything else answers 0, circle's frame);
  KEY_MAP_CONFIG_FIRST..LAST (16..34: SetKeyToKeyMap's swap loop, which includes 34 = KEY_MAP_STICK_DEAD as
  the ROM does) and KEY_MAP_LINKED_FIRST (19 / 20, skipped: the pair system_data_load_set copies, (as)).
  GetAlyssaLife / SetStatusBar read Character[0] (Alyssa), CheckLife reads Character[Player_BankID]: kept.
  GetItemListLastIndex has two identical loops over ItemList for argv[2] 1 and 0 (disassembly
  0x018e5978 / 0x018e59d0): one of them probably meant KeyItemList; the port keeps the ROM's behaviour.

- (2026-11-11, game/tiXcmd_TransItem.c slice 2) **Named from one use, to confirm where they are read:**
  CHARACTER +0x534 `edgeLevel` and h536[4] / [5] `CHARA_EDGE_PARAM_A / _B` (edge shading: only the x
  commands write them; the ghost pass in Clipping_Portal.c should read them); tiSet_CharaTranslucenceON's
  flag 2 taken as CHARA_TRANS_GHOST; `TRIG_EVENT_FADED_OUT` 0x4a (the OUT fades register it, the INs
  remove it); FADE_TYPE_* 0..7 from tiXcmd_FadeStart's strings (FADE_TYPE_MC_LOAD 4 = OUT2 black). The
  pad words are libpad's swapped layout (sdk/libpad.h: 0x40 cross, 0x10 triangle, 0x80 square, d-pad in
  the high byte), as asEyeForcusUP2CamMat.c already reads them. tiXcmd_CharaEdgeShadeAnim's fourth
  group's "-" waits while below its target (the others: above): kept as the ROM has it.
- (2026-11-11, slice 2's smoke run) **The last shot turned flat grey** (frames 60..360 black): past
  tiXcmd_FadeStart the script reaches tiFade_Start (stub), and the log's first drawn object is a
  4-vertex quad, rgba 90/90/90, texture 512x256 PSM 0x14. Which screen that is (and its reference) is the
  next drawing-path TU's question.

- (2026-11-11, game/tiXcmd_TransItem.c slice 1) **The boss commands that store pointers in pBossInfo wait
  for the boss TU's host form** (with (as) above): GetBloodAttackObject (+0x130 / +0x134: a part's EE
  address), SetWarp (+0x13c: a _SunAlloc'd list of part addresses), Init / End / SPAttack_SCISSORS_MAN
  (+0x140: a 0x70-byte record with two part lists at +0x24 / +0x28), BOSS_SetChain*, Balows2th (+0x10c /
  +0x110 lists). They stay stubs; SetPointForGhostArea already writes an EE word (BankEeAddr) into the
  junc record (CHARA_JUNC.ghostPoint) and CheckRange into pSrcMPstock / pDstMPstock, as the bank-data rule
  (1 h) has it. Decide with the boss TU whether pBossInfo stays EE layout (then these are BankEeAddr words
  too, plus the bank) or becomes a host struct.
  Slice 9 (2026-11-11): Balows2th is written without its pointer writes (PORT in its comment): the
  chain sets' holding arrays at +0x10c / +0x110 and Character[32].brain's bytes (+0, +1, +0x181; the AI
  TU's host form, (1 am)) are skipped. Fill them in when either host form lands.
  Slice 10 (2026-11-11): UseItem written -- **the TU's code is complete except the eight boss stubs above**,
  so it stays partial until the boss TU decides pBossInfo's form; the next run should take the next queued
  TU, not this one. Open: (a) ITEM_USE_* (argv[4]: 0 slot from a variable, 1 drop, 2 find by id) and
  ITEM_KIND_* (row byte +0: 0 / 1 give-take, 2 / 3 signal, 4 heal, 5 calm, 6 invisible, 0xff count only)
  are named from this one function; TRIG_EVENT_CALM_ALT_READY / _USED (0x44b / 0x44a) and
  TRIG_EVENT_NO_CALM (0x1a) too; (b) the row's +0x14 int (ITEM_ROW_EVENT, "registered on pick-up") is
  removed by a give-take use, so it is the "holding it" event, and +4 is both the use event and the
  heal / calm amount; (c) the HEAL kind confirms CHARACTER.hp (+0x524) is the current value and hpMax
  (+0x528) its top (the 161 / (ag) doubt settled); (d) PORT: the ROM writes one word past the list when
  ITEM_USE_FIND does not find the item, and trusts script slots / rows; the host refuses those.

- (2026-11-10, host task `alyssapan/rendering/ui_canvas.cpp`, **blocked**) **The first screen has no 2D
  packet step to route to UiCanvas.** It is ATLD, the memory-card check ("Checking... Do not remove memory
  card..."; reference `ct3pack.py tex ATLD.BIN` -> `work/ref/ATLD/`), and its five objTrans entries are
  textured quads in world space (z 0, x -845..-525) that main_rtn sends to
  gaBGT_DrawObject_NoClippingCheck2 under the scene camera (architecture.md "The boot screen and the ga
  draw object's data"). The first picture therefore needs **ga/gaBGT_DrawObject.c** (a native host draw
  module for ga draw objects: positions, ST, colours from the object's unpack blocks, the light pass,
  the texture from TEX_INFO) and **the camera** (camera_init0 / camera_view / camera_main in
  game/asEyeForcusUP2CamMat.c, stubs), not the triggers: none of ATLD's draw goes through Trigger_main.
  **Reordered (the user, 2026-11-10, "Decisions"): ga/gaBGT_DrawObject.c and game/asEyeForcusUP2CamMat.c
  now come before game/ssGrpPortalCheck.c.**
  UiCanvas stays the module for the true 2D draws (tiFade_Draw, the ssBGT_* screen sprites) when their
  TUs come. Also seen: the 15 s smoke run now makes ~420 frames (60 a second; earlier runs made ~26,000),
  and the ActressMP read in main.c no longer logs a crash in this run.

- (2026-11-10, host task `alyssapan/iop/fsys_server.cpp`) **The queue's premise "no bank loads" was
  stale**: banks never go through the file server (every `*.BIN` is in CT3PACK.DAT, found by FlFindPacked
  and read with sceCdRead), and a probe showed INFO, E_GL, 0MH0 and ATLD loaded and set up at boot, with
  TrgGLBLSceneDraw past BankReady (camera_init0 reached) -- before this task too. The server is written
  anyway (the movies are loose files) and answers sizes and sectors from io::Disc. The black screenshot
  is the draw path's, not the loader's: the next pinned task (`alyssapan/rendering/ui_canvas.cpp`) is the
  real way to the first picture. Two small doubts: (a) FSYS_IOP's answer for a name not on the disc is
  unread (the host answers -1 / -1; FlProc stores both as the file's size and base sector) -- read
  `IOP\FSYS_IOP.IRX` if a missing loose file ever matters; (b) main2.c's comment above BankReady still
  says FSMltLoad is a stub: the guard stays useful, the comment wants refreshing when main2.c is next
  touched.

- (2026-11-09) What lies below 0x01810000 at run time? The ELF is linked high in the 32 MB and keeps
  `.relmain` relocations; the port does not load the ELF, so this only matters if code addresses memory
  under its own image (a buffer, a second program). Look in `main` / `system_init`.
- (2026-11-09) Is `SNDDRV.IRX` the game's own driver (to reconstruct under `src/iop/`, as RXSNDDRV was) or a
  middleware build? And how much of the sound goes through SCE's `MODHSYN` / `MODMIDI` (hardware-synth
  MIDI, which no sibling met) against `sdremote`? `sound.c`'s TU answers it.
- (2026-11-09) `CT3PACK.DAT`'s format, and whether CT3's pictures are TIM2 (TextureStore decodes TIM2 only).
- (2026-11-09) Is the "one EE thread at a time, by priority" model right for CT3? (YamiPan's for FS2.)

- (2026-11-09, ga/gaDma.c) gaDmaOpen_MFIFO_VIF1 writes `ga_dma_env.mfd = 1` and D_CTRL with RELE set,
  RCYC cleared and MFD untouched; D_CTRL's MFD for VIF1 is 2. So either the SDK's sceDmaPutEnv translates
  mfd (1 -> VIF1), or the ga library's D_CTRL bitfield struct differs from the EE manual's. Harmless on the
  host (no MFIFO); read sceDmaPutEnv (0x01811b78) when the BGT ring buffer (gaBGT_*) TU needs it.
- (2026-11-09, ga/gaDma.c) The host's `sceDmaSend` (chain) does nothing on any channel, toSPR included.
  If a TU chains packets into the scratchpad (gaDmaSendS_toSPR has no caller yet), libdma must walk tags.

- (2026-11-09, ga/gaInitDrawObject.c) gaSetupDrawCtrlData adds its 27th word at bit 15 of TEX1_1, where
  the GS has nothing (L is bits 19..20): kept as the ROM does it (`GA_DRAWCTRL_ARGS.tex1Bit15`). Does the
  ga library mean L there (a bug in the game, harmless while the word is 0), or another register?
- (2026-11-09, ga/gaInitDrawObject.c, Claude's call) The scratchpad keeps its EE layout and its address
  words hold EE addresses (u_int): +0x14 `ga_div_redraw_gspacket_work | 0x20000000`, +0xec the 3dlib work
  (0x70000400), +0xf0 the BGT packet area (0x70000a00). A host reader maps a scratchpad address with
  SCRATCHPAD(ee - SCRATCHPAD_EE_BASE); `ga_div_redraw_gspacket_work` (bss 0x01957e40, 0xb0000 bytes) has no
  host definition yet: its reader's TU adds it and the mapping (with the BankEePtr precedent, (h)).
- (2026-11-09, ga/gaInitDrawObject.c) GA_SPR_WORK's floats from the 3dlib work are named by index
  (`work21`, `work23x16`...): name them when gaSet3dlibStruct's TU names ga3dlibwork's fields.
- (2026-11-09, ga/gaInitDrawObject.c slice 2) gaSetupTBufferData / gaBGT_LoadGSTexClutData_VIF1 have no
  ROM caller: `GA_TEX_DESC` / `GA_TBUFFER_DATA` are named from their loads alone, address words kept as EE
  u_int. QuatSlerp.c's `sunSetupTBufferData` (TEX_INFO by value) is the live copy: compare it when that TU
  is written, and decide there how the host turns these uploads into GPU textures (decoded at the source).
- (2026-11-10, ga/gaPadInit.c) The TU map's cut between pad_button_read.c and gaPadInit.c looks wrong:
  gaPadInit's only statics, `pad_dma_buf0` (0x0193e580) / `pad_dma_buf1` (0x0193e480), sit in the same
  unbroken symtab local run as pad_button_read's statics (indices 1607..1618: id, rdata$185, term_id,
  pad_button_read, exid, shock_data, rterm_id, the buffers, act_align, phase, act_direct), with no section
  symbol until the GCC gaDma object's .text at 0x018a1188. So one CodeWarrior object most likely spans
  0x018a0880..0x018a1188. The ga/pad_button_read.c session should take gaPadInit and its buffers into its
  file (define 018a0880-018a1188) and retire ga/gaPadInit.c; left apart for now (one TU per run).
  **Done 2026-11-11**: ga/pad_button_read.c defines 018a0880-018a1188 with gaPadInit as its tail;
  ga/gaPadInit.c and its TU entry are gone.
- (2026-11-11, ga/pad_button_read.c) gaPadRead now reads (verify). A 40 s run with a scripted START at
  pad reads 500 / 700 / 900 / 1100 (build.py --pad) logged no TITL load, but the shot turned from flat
  grey (the attract cycle, every earlier run) to black by frame 1200: something answered the pad. The
  next run on the title path should look at shots 900..1200 and the run log for which scene / BIN loads
  after START (kl2: the script's start test is still unread).
- (2026-11-11, sdk/libpad.cpp) Vibration mapping doubt: CT3 aligns actuator 0 (small, on / off) to byte
  0 and actuator 1 (large, level) to byte 1 (act_align {0, 1, 0xff...}), but the shim's
  scePadSetActDirect calls rumble(data[1] != 0, data[0]): the large motor's level becomes on / off and
  the small motor's 1 a near-zero strength. Fix in the shim (read the align from scePadSetActAlign) when
  vibration is tested; tiVibrationOn is the caller.

## Done

- 2026-11-09: AlyssaPan created: the repo, YamiPan's Pan layer (C++, HEAD 17e0108 plus 93c73f0's SPU2
  voices and sound card) trimmed to the generic, SDK, CMake, tools, the skill and its references (this
  file, architecture.md, cpp-port.md, bootstrap.md, reconstruction.md, ghidra.md, hazards.md with the
  siblings' digest). Build green, first smoke run and first `refresh.py` done. Nothing committed: the
  user decides.
