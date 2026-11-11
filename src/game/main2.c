// game/main2.c -- Clock Tower 3: the title's set-up (main2, InitLoadSceneChar) and the bank triggers the
// event scripts and the save loader call -- scenes and global banks ("%s.BIN"), a character's shape
// ("%s.BIN") and behaviours ("%s.ANM") loaded, drawn and freed by bank -- and the self-discard queue that
// swaps a scene bank while the game runs. Reconstructed 2026-11-10 from CT3 0x018b20f0..0x018b3a54.

#include "game/main2.h"

#include "common/alyssapan_stub.h"
#include "game/Character_Init.h"
#include "game/Clipping_Portal.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ahalloc.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/ssGrpPortalCheck.h"
#include "game/tiAvoidObject.h"
#include "game/tkXDTrg_GetSceneHash.h"
#include "game/unassigned_data.h"
#include "ga/gaInitDrawObject.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// The global banks InitLoadSceneChar loads (bank index, file).
#define GLBL_BANK_TITLE 0               /* "ATLD" at power-on, "TITL" after */
#define GLBL_BANK_0MH0 1
#define GLBL_BANK_E_GL 3
#define GLBL_BANK_INFO 5

// The enemies whose shape brings a scene bank of its own (TrgCharacterShapeLoad / _Set / _Free).
#define CHARA_E02 2                     /* "E_02" in bank 0x27 */
#define CHARA_E03 3                     /* "E_03" in bank 0x1e; also tiPipeJointInit */
#define CHARA_E04 4                     /* "E_04" in bank 0x1e */
#define CHARA_E05 5                     /* "E_05" in bank 0x22 (5 and 6) */
#define CHARA_E05B 6
#define E00_BANK 0x2e                   /* "E_00": drawn with Alyssa (TrgCharacterDraw) */

#define CHARA_LEG_PART_OFFSET 0x40      /* CHARACTER.legParts: part address + 0x40 */
enum CHARA_LEG {                        /* CHARACTER.legParts[], in the ROM's order */
    CHARA_LEG_R_THIGH,
    CHARA_LEG_R_CALF,
    CHARA_LEG_R_FOOT,
    CHARA_LEG_IK_R,
    CHARA_LEG_L_THIGH,
    CHARA_LEG_L_CALF,
    CHARA_LEG_L_FOOT,
    CHARA_LEG_IK_L,
};
static const char *const sLegPartNames[8] = {   /* 0x01934fd0.. */
    "Bip01 R Thigh", "Bip01 R Calf", "Bip01 R Foot", "ik_R",
    "Bip01 L Thigh", "Bip01 L Calf", "Bip01 L Foot", "ik_L",
};

#define BRAIN_ID 0x24e                  /* the AI brain's id (a short), ssAI_BrainEnd's second argument */
#define CHARA_FLAGS_DRAWN (CHARACTER_FLAG_REFRACTION | CHARACTER_FLAG_SHADOW)

// PORT: a bank the loader has not filled (fileio.c's FSMltLoad is still a stub, so none is) has no
// tables: the draw set-up after the load is skipped instead of reading null tables, which the ROM never
// meets because its FSMltLoadEndWaitFile only returns once the bank is in.
static int BankReady(int bank)
{
    return bank >= 0 && bank < BANK_MAX && All_table_TBL[bank] != NULL && pModelPartsHeader[bank] != NULL;
}

SELF_DISCARD_LOAD SelfDiscardSceneLoadFlag[SELF_DISCARD_MAX];  /* sbss 0x01942580 */
static int Alyssa_Draw_Off_Check;       /* data 0x0193ef70 (a local symbol): TrgSceneDraw took Alyssa's
                                           shadow flag off with her light bank, and gives it back */

// The scene bank an enemy's shape needs (name, bank, TrgSceneLoad's third argument); 0 = none.
static int EnemySceneOf(int chara, const char **name, int *loadArg)
{
    if (chara == CHARA_E05B || chara == CHARA_E05) {
        *name = "E_05", *loadArg = 2;
        return 0x22;
    }
    if (chara == CHARA_E04) {
        *name = "E_04", *loadArg = 1;
        return 0x1e;
    }
    if (chara == CHARA_E03) {
        *name = "E_03", *loadArg = 1;
        return 0x1e;
    }
    if (chara == CHARA_E02) {
        *name = "E_02", *loadArg = 1;
        return 0x27;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// After a game is started or continued: a new game loads the title banks and keeps a continue image;
// a continue restores it.
// CT3 0x018b20f0

void main2(void)
{
    CharacterDisable = 0;
    if (load_continue == 0) {
        InitLoadSceneChar();
        ContinueBackupSet();
        SkipMain = 2;
    } else {
        ContinueBackupGet();
        load_continue = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The global banks: INFO, E_GL, 0MH0, then the title (ATLD at power-on, TITL after); the frame clears to
// the title's fog colour.
// CT3 0x018b2170

void InitLoadSceneChar(void)
{
    idNextScene = 0;
    idCurrentScene = 1;
    ssTrigRegisterEvent(EV_TITLE_23, 1, 0);
    ssTrigRegisterEvent(EV_TITLE_25, 1, 0);
    TrgGLBLSceneLoad((char *)"INFO", GLBL_BANK_INFO);
    TrgGLBLSceneDraw(1, (char *)"INFO", GLBL_BANK_INFO);
    TrgGLBLSceneLoad((char *)"E_GL", GLBL_BANK_E_GL);
    TrgGLBLSceneDraw(1, (char *)"E_GL", GLBL_BANK_E_GL);
    TrgGLBLSceneLoad((char *)"0MH0", GLBL_BANK_0MH0);
    TrgGLBLSceneDraw(1, (char *)"0MH0", GLBL_BANK_0MH0);
    if (load_continue == 0) {
        if (PowerOn == 0) {
            TrgGLBLSceneLoad((char *)"ATLD", GLBL_BANK_TITLE);
            TrgGLBLSceneDraw(1, (char *)"ATLD", GLBL_BANK_TITLE);
            PowerOn = 1;
            idCurrentScene = 0;
        } else {
            TrgGLBLSceneLoad((char *)"TITL", GLBL_BANK_TITLE);
            TrgGLBLSceneDraw(1, (char *)"TITL", GLBL_BANK_TITLE);
            idCurrentScene = 0;
        }
    } else {
        ContinueBackupGet();
        load_continue = 0;
    }
    // Host: All_table_TBL[0] is null until the bank loader fills it (fileio.c's TU).
    FogCtrlAddr[0] = All_table_TBL[0] != NULL ? BANK_PTR(u_char *, 0, All_table_TBL[0]->fog) : NULL;
    drawcom_frameclear.rgbaq = FogClearColor(FogCtrlAddr[0]);
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the ROM (4 bytes).
// CT3 0x018b22f0

void SwapDrawingDoor(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// A character's draw flags on or off; Alyssa brings E_00 with her unless EV_NO_E00 is set.
// CT3 0x018b2300

void TrgCharacterDraw(int on, int chara)
{
    if (on == 0) {
        Character[chara].flags &= ~CHARA_FLAGS_DRAWN;
        if (chara == CHARA_ALYSSA && ssTrigCheckEvent(EV_NO_E00, 1, 0) == 0)
            TrgSceneDraw(0, 0, SCENE_PLACE_ORIGIN, (char *)"E_00", E00_BANK);
    } else {
        Character[chara].flags |= CHARA_FLAGS_DRAWN;
        if (chara == CHARA_ALYSSA && ssTrigCheckEvent(EV_NO_E00, 1, 0) == 0)
            TrgSceneDraw(1, 0, SCENE_PLACE_ORIGIN, (char *)"E_00", E00_BANK);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Every character's behaviours, shape and junction block freed.
// CT3 0x018b23f0

void CharacterAllFree(void)
{
    int i, j;

    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *chr = &Character[i];

        for (j = 0; j < CHARA_BEHAVIOR_MAX; j++) {
            // The ROM passes the slot index where TrgCharacterBehaviorFree looks for a bank (kept).
            if (chr->behaviorBank[j] > 0)
                TrgCharacterBehaviorFree(i, j);
        }
        if (chr->bank > 0)
            TrgCharacterShapeFree(i);
        if (chr->junc != NULL) {
            _SunFree(&gHeapJunc, (void **)&chr->junc->poles);
            _SunFree(&gHeapJunc, (void **)&chr->junc);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// A loaded behaviour ("<name>.ANM") taken into the character's first slot with that bank, or its first
// free one, and drawn.
// CT3 0x018b24c0

void TrgCharacterBehaviorSet(int chara, char *name, int bank)
{
    CHARACTER *chr = &Character[chara];
    char file[16];
    int i;

    sprintf(file, "%s.ANM", name);
    if (FSMltLoadStatus(bank, file) != 0)
        bank = FSMltLoadEndWaitFile(bank, file);
    for (i = 0; i < CHARA_BEHAVIOR_MAX; i++) {
        if ((short)bank == chr->behaviorBank[i])
            break;
    }
    if (i == CHARA_BEHAVIOR_MAX) {
        for (i = 0; i < CHARA_BEHAVIOR_MAX; i++) {
            if (chr->behaviorBank[i] == -1)
                break;
        }
    }
    // The ROM writes slot 10 (past the array, into shapeName) when all ten are taken: kept.
    strcpy(chr->behaviorName[i], file);
    FSMltSetDrawBank(bank, chr->behaviorName[i], DRAW_BANK_ON);
    chr->behaviorBank[i] = (short)bank;
}

// ──────────────────────────────────────────────────────────────────────
// The behaviour in `bank` waited for, undrawn and its slot emptied.
// CT3 0x018b2610

void TrgCharacterBehaviorFree(int chara, int bank)
{
    CHARACTER *chr = &Character[chara];
    int i;

    for (i = 0; i < CHARA_BEHAVIOR_MAX; i++) {
        if ((short)bank == chr->behaviorBank[i])
            break;
    }
    if (i != CHARA_BEHAVIOR_MAX) {
        short *slot = &chr->behaviorBank[i];
        char *file = chr->behaviorName[i];

        if (FSMltLoadStatus(*slot, file) != 0)
            FSMltLoadEndWaitFile(*slot, file);
        FSMltSetDrawBank(*slot, file, DRAW_BANK_OFF);
        *slot = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A loaded shape ("<name>.BIN") becomes the character's: drawn, the character reset, the enemy's own
// scene bank drawn, Alyssa's head (ActressMP) and the legs' parts found, the omni effects cleared.
// CT3 0x018b26f0

void TrgCharacterShapeSet(int chara, char *name, int bank, int initA, int initB)
{
    CHARACTER *chr = &Character[chara];
    char file[16];
    const char *scene;
    int loadArg, sceneBank, i;
    u_char *header;

    sprintf(file, "%s.BIN", name);
    if (FSMltLoadStatus(bank, file) != 0)
        bank = FSMltLoadEndWaitFile(bank, file);
    strcpy(chr->shapeName, file);
    FSMltSetDrawBank(bank, chr->shapeName, DRAW_BANK_ON);
    chr->bank = (short)bank;
    Character_Init(chara, initB, initA);

    sceneBank = EnemySceneOf(chara, &scene, &loadArg);
    if (sceneBank != 0) {
        char sceneName[5];

        strcpy(sceneName, scene);
        TrgSceneDraw(1, 0, SCENE_PLACE_ORIGIN, sceneName, sceneBank);
    }

    if (chara == CHARA_ALYSSA && All_table_TBL[bank] != NULL)   // host: null until the loader fills it
        ActressMP = asFindModelParts(BANK_PTR(u_char *, bank, All_table_TBL[bank]->partsHeader), ahGetHash("Bip01 Head"));

    header = pModelPartsHeader[bank];
    for (i = 0; i < 8; i++) {
        int id = asFindModelPartsID(header, ahGetHash(sLegPartNames[i]));
        u_char *part = header + CHARA_LEG_PART_OFFSET + id * (int)sizeof(MODEL_PARTS);

        chr->legParts[i] = header != NULL ? BankEeAddr(bank, part) : 0;
    }

    if (chara == CHARA_E03)
        tiPipeJointInit(bank);
    for (i = 0; i < 8; i++)
        CharOmniEffect[chara][i] = 0xffff;
    if (TrgCharStopFlag & TRG_CHAR_STOP_CHAR)
        MonsterAIUpdate(1);
}

// ──────────────────────────────────────────────────────────────────────
// The character's shape undrawn and freed (its brain ended, its enemy scene bank undrawn, its animation
// stopped).
// CT3 0x018b2b00

void TrgCharacterShapeFree(int chara)
{
    CHARACTER *chr = &Character[chara];
    const char *scene;
    int loadArg, sceneBank;

    if (chr->bank < 0)
        return;
    if ((chr->flags & CHARACTER_FLAG_REFRACTION) || (chr->flags & CHARACTER_FLAG_SHADOW))
        TrgCharacterDraw(0, chara);
    if (FSMltLoadStatus(chr->bank, chr->shapeName) != 0)
        FSMltLoadEndWaitFile(chr->bank, chr->shapeName);
    if (chr->brain != 0) {
        // PORT: brain is an EE word nothing on the host sets yet (open-items.md 1 am).
        ssAI_BrainEnd(chr, *(short *)((u_char *)(uintptr_t)chr->brain + BRAIN_ID));
    }
    FSMltSetDrawBank(chr->bank, chr->shapeName, DRAW_BANK_OFF);

    sceneBank = EnemySceneOf(chara, &scene, &loadArg);
    if (sceneBank != 0) {
        char sceneName[5];

        strcpy(sceneName, scene);
        // The ROM polls the scene's bank with the bare name (no ".BIN"), then undraws it.
        if (FSMltLoadStatus(sceneBank, sceneName) != 0)
            FSMltLoadEndWaitFile(sceneBank, sceneName);
        TrgSceneDraw(0, 0, SCENE_PLACE_ORIGIN, sceneName, sceneBank);
    }
    asForceOperateAnimation_Character(chara, 0);
    chr->bank = -1;
}

// ──────────────────────────────────────────────────────────────────────
// CT3 0x018b2d00

void TrgCharacterBehaviorLoad(int chara, char *name, int bank, int arg)
{
    char file[16];

    (void)chara;
    sprintf(file, "%s.ANM", name);
    FSMltLoad(file, FS_LOAD_BEHAVIOR, bank, arg);
}

// ──────────────────────────────────────────────────────────────────────
// The shape's load started, with the enemy's own scene bank.
// CT3 0x018b2d60

void TrgCharacterShapeLoad(int chara, char *name, int bank, int arg)
{
    char file[16];
    const char *scene;
    int loadArg, sceneBank;

    sprintf(file, "%s.BIN", name);
    FSMltLoad(file, FS_LOAD_SHAPE, bank, arg);
    sceneBank = EnemySceneOf(chara, &scene, &loadArg);
    if (sceneBank != 0)
        TrgSceneLoad((char *)scene, sceneBank, loadArg);
}

// ──────────────────────────────────────────────────────────────────────
// One frame of each queued self-discard load (SELF_DISCARD_STATE).
// CT3 0x018b2e60

void SelfDiscardSceneLoadUpdate(void)
{
    int i;

    for (i = 0; i < SELF_DISCARD_MAX; i++) {
        SELF_DISCARD_LOAD *rec = &SelfDiscardSceneLoadFlag[i];
        char cur[5];

        if (rec->state < 0)
            continue;
        if (rec->state == SELF_DISCARD_DONE) {
            if (rec->draw) {
                if (rec->loadBank < GLBL_BANK_MAX) {
                    TrgGLBLSceneDraw(1, rec->name, rec->loadBank);
                } else {
                    memcpy(DrawOnPosSetVectorTemp, rec->pos, 3 * sizeof(float));
                    memcpy(DrawOnPosSetMatrixTemp, rec->mat, sizeof(DrawOnPosSetMatrixTemp));
                    DrawOnPosSetHashTemp = rec->hash;
                    TrgSceneDraw(1, 0, SCENE_PLACE_TEMP, rec->name, BANK_NONE);
                    SkipMain = 4;
                    ssTrigRemoveEvent(EV_SELF_DISCARD_LOADING, 1, 0);
                }
                WarpObjectUpdate();
            }
            rec->state = SELF_DISCARD_FREE;
            strcpy(rec->name, "");
            rec->loadBank = 0;
            rec->loadArg = 0;
            rec->draw = 0;
            rec->bank = 0;
            rec->handle = 0;
            rec->pos[0] = rec->pos[1] = rec->pos[2] = 0.0f;
            ssTrigRemoveEvent(EV_SELF_DISCARD_LOADING, 1, 0);
        } else if (rec->state == SELF_DISCARD_LOADING) {
            if (FSMltLoadEnd(rec->handle) >= 0)
                rec->state = SELF_DISCARD_DONE;
        } else if (rec->state == SELF_DISCARD_START) {
            if (rec->loadBank < GLBL_BANK_MAX) {
                memcpy(cur, BankInfo[0].name, 4);
                cur[4] = 0;
                TrgGLBLSceneDraw(0, cur, 0);
                rec->handle = TrgGLBLSceneLoad(rec->name, rec->loadBank);
            } else {
                BANK_INFO *old = &BankInfo[rec->bank];

                memcpy(cur, old->name, 4);
                cur[4] = 0;
                if (old->state < 0) {
                    printf("*************************************\n");
                    printf("Warning!! SelfDiscard : BANK(%d)\n", rec->bank);
                    printf("             Use Flag : %d\n", old->state);
                    printf("           Draw Scene : %s\n", cur);
                    printf("        Loading Scene : %s\n", old->name);
                    printf("*************************************\n");
                }
                TrgSceneDraw(0, 0, 0, cur, rec->bank);
                SceneEndBank(rec->bank, old->groupCount);
                SceneDoorFlagClear(GetRoomName_file(cur));
                camera_destroy(rec->bank, 1);
                rec->handle = TrgSceneLoad(rec->name, rec->loadBank, rec->loadArg);
            }
            rec->state = SELF_DISCARD_LOADING;
        } else if (rec->state == SELF_DISCARD_QUEUED) {
            rec->state = SELF_DISCARD_START;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Queues a scene swap in the first free slot: `bank` is dropped and `name` loaded into loadBank, placed
// at the origin (part -1), at part `part` of `bank` by its matrix and hash (byPos 0), or at that part's
// position offset by the bank's (byPos != 0).
// CT3 0x018b31b0

void SelfDiscardSceneLoad(char *name, int loadBank, int loadArg, int draw, int byPos, int part, int bank)
{
    SELF_DISCARD_LOAD *rec;
    MODEL_PARTS *parts;
    int i;

    for (i = 0;; i++) {
        if (i >= SELF_DISCARD_MAX)
            return;
        if (SelfDiscardSceneLoadFlag[i].state < 0)
            break;
    }
    ssTrigRegisterEvent(EV_SELF_DISCARD_LOADING, 1, 0);
    rec = &SelfDiscardSceneLoadFlag[i];
    rec->state = SELF_DISCARD_QUEUED;
    strcpy(rec->name, name);
    rec->loadBank = loadBank;
    rec->loadArg = loadArg;
    rec->draw = (u_char)draw;
    rec->bank = (short)(bank & 0xff);
    if (part == -1) {
        rec->pos[0] = rec->pos[1] = rec->pos[2] = 0.0f;
        memcpy(rec->mat, matUnit, sizeof(rec->mat));
        rec->hash = 0;
        return;
    }
    parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    if (byPos == 0) {
        memcpy(rec->mat, parts[part].matrix, sizeof(rec->mat));
        memcpy(rec->pos, parts[part].pos, sizeof(rec->pos));
        rec->hash = parts[part].hash;
        return;
    }
    rec->pos[0] = parts[part].drawOnPos[0] + parts[0].drawOnPos[0];
    rec->pos[1] = parts[part].drawOnPos[1] + parts[0].drawOnPos[1];
    rec->pos[2] = parts[part].drawOnPos[2] + parts[0].drawOnPos[2];
    memcpy(rec->mat, matUnit, sizeof(rec->mat));
    rec->hash = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Every slot free, at the origin.
// CT3 0x018b3480

void SelfDiscardSceneLoadInit(void)
{
    int i;

    for (i = 0; i < SELF_DISCARD_MAX; i++) {
        SELF_DISCARD_LOAD *rec = &SelfDiscardSceneLoadFlag[i];

        rec->state = SELF_DISCARD_FREE;
        strcpy(rec->name, "");
        rec->loadBank = 0;
        rec->loadArg = 0;
        rec->draw = 0;
        rec->bank = 0;
        rec->handle = 0;
        rec->pos[0] = rec->pos[1] = rec->pos[2] = 0.0f;
        memcpy(rec->mat, matUnit, sizeof(rec->mat));
        rec->hash = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A scene bank drawn or undrawn (global banks go to TrgGLBLSceneDraw). Drawn, it is placed (part: a part
// of srcBank, or SCENE_PLACE_*) and set up; undrawn, characters lit by it lose their shadow flag, which
// Alyssa gets back when her bank is drawn again. Answers the bank.
// CT3 0x018b3590

int TrgSceneDraw(int on, int srcBank, int part, char *name, int bank)
{
    char file[16];
    int i;

    if (bank < GLBL_BANK_MAX && bank != BANK_NONE) {
        TrgGLBLSceneDraw(on, name, bank);
        return bank;    // the ROM answers the caller's s0 here; no caller reads it
    }
    sprintf(file, "%s.BIN", name);
    if (on == 0) {
        FSMltSetDrawBank(bank, file, DRAW_BANK_OFF);
        if (Character[CHARA_ALYSSA].lightBank == bank && (Character[CHARA_ALYSSA].flags & CHARACTER_FLAG_SHADOW)) {
            Alyssa_Draw_Off_Check = 1;
            Character[CHARA_ALYSSA].flags &= ~CHARACTER_FLAG_SHADOW;
        }
        for (i = 1; i < CHARACTER_MAX; i++) {
            if (Character[i].lightBank == bank && (Character[i].flags & CHARACTER_FLAG_SHADOW))
                Character[i].flags &= ~CHARACTER_FLAG_SHADOW;
        }
        return bank;
    }

    bank = FSMltLoadEndWaitFile(bank, file);
    FSMltSetDrawBank(bank, file, DRAW_BANK_ON);
    if (!BankReady(bank))
        return bank;
    {
        MODEL_PARTS *dst = (MODEL_PARTS *)pModelPartsHeader[bank];

        if (part == SCENE_PLACE_TEMP) {
            if (DrawOnPosSetHashTemp == 0) {
                memcpy(dst->drawOnPos, DrawOnPosSetVectorTemp, 3 * sizeof(float));
            } else {
                int r = asJointParts_byMatrix(DrawOnPosSetMatrixTemp, (float *)&DrawOnPosSetVectorTemp, bank,
                                              DrawOnPosSetHashTemp);
                if (r == JOINT_NOT_FOUND_2 || r == JOINT_NOT_FOUND_1)
                    printf("Not Find JointPortal Object\n");
            }
        } else if (part == SCENE_PLACE_ORIGIN) {
            dst->drawOnPos[0] = dst->drawOnPos[1] = dst->drawOnPos[2] = 0.0f;
        } else {
            MODEL_PARTS *src = (MODEL_PARTS *)pModelPartsHeader[srcBank];

            if (src[part].kind == PARTS_KIND_JOINT_PORTAL) {
                int r = asJointParts(srcBank, bank, src[part].hash);
                if (r == JOINT_NOT_FOUND_2 || r == JOINT_NOT_FOUND_1)
                    printf("Not Find JointPortal Object\n");
            } else {
                dst->drawOnPos[0] = src[part].drawOnPos[0] + src[0].drawOnPos[0];
                dst->drawOnPos[1] = src[part].drawOnPos[1] + src[0].drawOnPos[1];
                dst->drawOnPos[2] = src[part].drawOnPos[2] + src[0].drawOnPos[2];
            }
        }
    }
    scene_draw_init(bank);
    if (Character[CHARA_ALYSSA].lightBank == bank && Alyssa_Draw_Off_Check != 0) {
        Alyssa_Draw_Off_Check = 0;
        Character[CHARA_ALYSSA].flags |= CHARACTER_FLAG_SHADOW;
    }
    return bank;
}

// ──────────────────────────────────────────────────────────────────────
// A scene bank's load started (global banks go to TrgGLBLSceneLoad). Answers FSMltLoad's handle.
// CT3 0x018b38b0

int TrgSceneLoad(char *name, int bank, int arg)
{
    char file[16];

    if (bank < GLBL_BANK_MAX && bank != BANK_NONE)
        return TrgGLBLSceneLoad(name, bank);    // the ROM answers the caller's s0 (garbage) here
    sprintf(file, "%s.BIN", name);
    return FSMltLoad(file, FS_LOAD_SCENE, bank, arg);
}

// ──────────────────────────────────────────────────────────────────────
// A global bank's load started. The ROM's v0 is FSMltLoad's: SelfDiscardSceneLoadUpdate keeps it.
// CT3 0x018b3940

int TrgGLBLSceneLoad(char *name, int bank)
{
    char file[16];

    sprintf(file, "%s.BIN", name);
    return FSMltLoad(file, FS_LOAD_GLBL, bank, 1);
}

// ──────────────────────────────────────────────────────────────────────
// A global bank drawn (waited for, at the origin, set up, its camera at cut 0) or undrawn.
// CT3 0x018b3990

void TrgGLBLSceneDraw(int on, char *name, int bank)
{
    char file[16];

    sprintf(file, "%s.BIN", name);
    if (on == 0) {
        FSMltSetDrawBank(bank, file, DRAW_BANK_OFF);
        return;
    }
    FSMltLoadEndWaitFile(bank, file);
    FSMltSetDrawBank(bank, file, DRAW_BANK_ON);
    if (!BankReady(bank))
        return;
    {
        MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
        parts->drawOnPos[0] = parts->drawOnPos[1] = parts->drawOnPos[2] = 0.0f;
    }
    scene_draw_init(bank);
    camera_init0(bank, 0);
    CamChange_S(bank, 0, 0, 0);
}
