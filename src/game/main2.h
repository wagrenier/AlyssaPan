// game/main2.h -- CT3 game/main2.c: the title's set-up and the bank triggers -- scenes ("%s.BIN"),
// character shapes ("%s.BIN") and behaviours ("%s.ANM") loaded, drawn and freed by bank, and the
// self-discard queue that swaps a scene bank in the background (written 2026-11-10).
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// ---- banks and names ----

#define GLBL_BANK_MAX 6                 /* banks 0..5 are the global ones: TrgScene* hands them to TrgGLBL* */
#define BANK_NONE (-1)                  /* TrgScene*'s bank -1: never a global bank */

// FSMltLoad's file kinds (its second argument; names from these callers).
#define FS_LOAD_GLBL 0                  /* TrgGLBLSceneLoad */
#define FS_LOAD_SCENE 1                 /* TrgSceneLoad */
#define FS_LOAD_SHAPE 2                 /* TrgCharacterShapeLoad */
#define FS_LOAD_BEHAVIOR 3              /* TrgCharacterBehaviorLoad */

// FSMltSetDrawBank's mode.
#define DRAW_BANK_OFF 0
#define DRAW_BANK_ON 1

// TrgSceneDraw's part argument: a part index of srcBank, or one of these.
#define SCENE_PLACE_ORIGIN (-1)         /* the bank's part 0 drawOnPos = 0 */
#define SCENE_PLACE_TEMP (-2)           /* at DrawOnPosSetVectorTemp, or by DrawOnPosSetMatrixTemp when
                                           DrawOnPosSetHashTemp != 0 (asJointParts_byMatrix) */
#define PARTS_KIND_JOINT_PORTAL 0x1b    /* MODEL_PARTS.kind: the new bank is jointed to it (asJointParts) */
#define JOINT_NOT_FOUND_1 1             /* asJointParts / _byMatrix: both print "Not Find JointPortal Object" */
#define JOINT_NOT_FOUND_2 2

#define CHARA_BEHAVIOR_MAX 10           /* CHARACTER.behaviorBank[] / behaviorName[] */
#define CHARA_ALYSSA 0                  /* Character[0]: the player (her head becomes ActressMP) */

// The trigger events this TU touches (ssGrpPortalCheck.c owns them; names from this use only).
#define EV_TITLE_23 0x23                /* InitLoadSceneChar registers it */
#define EV_TITLE_25 0x25                /* InitLoadSceneChar registers it */
#define EV_SELF_DISCARD_LOADING 0x4a    /* registered while a self-discard load runs */
#define EV_NO_E00 0xb6                  /* set: TrgCharacterDraw leaves E_00 alone */

// ---- the self-discard queue ----
// SelfDiscardSceneLoadFlag[5] (0x01942580, 0xc0 bytes each): a scene bank swapped in the background.
// SelfDiscardSceneLoad queues it, SelfDiscardSceneLoadUpdate steps it once a frame: wait a frame, drop the
// old bank and start the load, wait for it, then draw it (placed) and free the slot. layout: ee (no pointers).
typedef struct SELF_DISCARD_LOAD {
    s_char state;                       /* 0x00: SELF_DISCARD_* */
    u_char draw;                        /* 0x01: draw the bank once loaded */
    short bank;                         /* 0x02: the bank to drop (BankInfo index; the caller's & 0xff) */
    char name[8];                       /* 0x04: the scene to load (4 chars) */
    int loadBank;                       /* 0x0c: TrgSceneLoad's bank (< GLBL_BANK_MAX: a global bank) */
    int loadArg;                        /* 0x10: TrgSceneLoad's third argument */
    int handle;                         /* 0x14: FSMltLoad's answer, polled by FSMltLoadEnd */
    u_char pad18[8];
    float pos[4];                       /* 0x20: DrawOnPosSetVectorTemp for the draw */
    u_char pad30[0x10];
    float mat[4][4];                    /* 0x40: DrawOnPosSetMatrixTemp */
    u_long hash;                        /* 0x80: DrawOnPosSetHashTemp (0: place by pos) */
    u_char pad88[0x38];
} SELF_DISCARD_LOAD;
static_assert(sizeof(SELF_DISCARD_LOAD) == 0xc0, "SELF_DISCARD_LOAD");
static_assert(offsetof(SELF_DISCARD_LOAD, loadBank) == 0x0c, "SELF_DISCARD_LOAD.loadBank");
static_assert(offsetof(SELF_DISCARD_LOAD, handle) == 0x14, "SELF_DISCARD_LOAD.handle");
static_assert(offsetof(SELF_DISCARD_LOAD, pos) == 0x20, "SELF_DISCARD_LOAD.pos");
static_assert(offsetof(SELF_DISCARD_LOAD, mat) == 0x40, "SELF_DISCARD_LOAD.mat");
static_assert(offsetof(SELF_DISCARD_LOAD, hash) == 0x80, "SELF_DISCARD_LOAD.hash");

#define SELF_DISCARD_MAX 5
enum SELF_DISCARD_STATE {
    SELF_DISCARD_FREE = -1,
    SELF_DISCARD_QUEUED = 0,            /* waits one frame */
    SELF_DISCARD_START = 1,             /* drops the old bank, starts the load */
    SELF_DISCARD_LOADING = 2,           /* FSMltLoadEnd(handle) >= 0 -> done */
    SELF_DISCARD_DONE = 3,              /* draws it if asked, frees the slot */
};

extern SELF_DISCARD_LOAD SelfDiscardSceneLoadFlag[SELF_DISCARD_MAX];  /* sbss 0x01942580 */

// ---- functions ----

void main2(void);                                                                        // CT3 0x018b20f0
void InitLoadSceneChar(void);                                                            // CT3 0x018b2170
void SwapDrawingDoor(void);                                                              // CT3 0x018b22f0
void TrgCharacterDraw(int on, int chara);                                                // CT3 0x018b2300
void CharacterAllFree(void);                                                             // CT3 0x018b23f0
void TrgCharacterBehaviorSet(int chara, char *name, int bank);                           // CT3 0x018b24c0
void TrgCharacterBehaviorFree(int chara, int bank);                                      // CT3 0x018b2610
void TrgCharacterShapeSet(int chara, char *name, int bank, int initA, int initB);        // CT3 0x018b26f0
void TrgCharacterShapeFree(int chara);                                                   // CT3 0x018b2b00
void TrgCharacterBehaviorLoad(int chara, char *name, int bank, int arg);                 // CT3 0x018b2d00
void TrgCharacterShapeLoad(int chara, char *name, int bank, int arg);                    // CT3 0x018b2d60
void SelfDiscardSceneLoadUpdate(void);                                                   // CT3 0x018b2e60
void SelfDiscardSceneLoad(char *name, int loadBank, int loadArg, int draw, int byPos, int part,
                          int bank);                                                     // CT3 0x018b31b0
void SelfDiscardSceneLoadInit(void);                                                     // CT3 0x018b3480
int TrgSceneDraw(int on, int srcBank, int part, char *name, int bank);                   // CT3 0x018b3590
int TrgSceneLoad(char *name, int bank, int arg);                                         // CT3 0x018b38b0
int TrgGLBLSceneLoad(char *name, int bank);                                              // CT3 0x018b3940
void TrgGLBLSceneDraw(int on, char *name, int bank);                                     // CT3 0x018b3990
