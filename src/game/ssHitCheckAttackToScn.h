// game/ssHitCheckAttackToScn.h -- CT3 game/ssHitCheckAttackToScn.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
struct HIT_TABLE;
struct MODEL_PARTS;

void ssProcessHit(void);                                                                 // CT3 0x018b1e60
void ssHitGlobalInit(void);                                                              // CT3 0x018b20e0
void ssHitInit(int scene);                                                               // CT3 0x018b2070

// ---- the frame's collision pass (slice 1, 2026-11-11) ----
extern u_char gSceneL2WStatus[86];              /* sbss 0x0193eef0: per bank, its hit data is in the
                                                   world this frame (ssCollisionCheck sets it once) */
// One character ssProcessHit collides this frame. layout: host (the ROM's: 8 bytes on the stack, the
// Character[] address at 0, the bank short at 4; ssCollisionCheck walks them in 8-byte steps).
typedef struct SS_HIT_CHARA {
    struct CHARACTER *chara;
    short bank;                         /* its shape bank (CHARACTER.bank) */
} SS_HIT_CHARA;
#define SS_HIT_CHARA_MAX 35             /* CHARACTER_MAX: the ROM's 0x118-byte stack list */
// One queued attack (ssHitAtList.list, 14 bytes; layout: ee, a static of this TU).
typedef struct SS_HIT_ATTACK {
    short power;                        /* 0x0: the motion record's short at +0x2c / the arrow's stage */
    short chara;                        /* 0x2: the attacker (Character[] index, or the arrow) */
    short atk;                          /* 0x4: its attack id (a character's shape attack, the arrow's bank) */
    short target;                       /* 0x6: a scene attack's target, -1 = none (then not checked) */
    short motion;                       /* 0x8 */
    short bank;                         /* 0xa: the bank it strikes */
    u_char kind;                        /* 0xc: SS_HIT_ATK_* */
    u_char b0d;                         /* 0xd: ssHitAddAttackRequest's sixth argument */
} SS_HIT_ATTACK;
static_assert(sizeof(SS_HIT_ATTACK) == 0xe, "SS_HIT_ATTACK");
#define SS_HIT_ATK_CHARACTER 0          /* ssHitCheckCharacterAttack */
#define SS_HIT_ATK_SCENE 1              /* (any non-0) ssHitCheckSceneAttack, when target != -1 */
#define SS_HIT_ATTACK_MAX 255           /* the count is a byte: 0xff refuses more */
void ssHitClearAttackRequest(void);                                                      // CT3 0x018aa8d0
void ssHitCheckAttackList(void);                                                         // CT3 0x018aa830
void ssHitCheckCharacterAttack(SS_HIT_ATTACK *a);                                        // CT3 0x018aa570
void ssHitCheckSceneAttack(SS_HIT_ATTACK *a);                                            // CT3 0x018aa370
void ssCollisionCheck(struct CHARACTER *chara, int bank, SS_HIT_CHARA *list, int count); // CT3 0x018b1a40
// One hit shape of a bank (HIT_TABLE.hitData / dmgData; slice 1, 2026-11-11, from ssTrans*HitDataL2W).
// layout: ee -- bank data. Local values are the shape in its part's space; ssTrans*HitDataL2W writes the
// world ones. Its +0x194 / +0x198 are LOS_HIT_FLAGS / LOS_HIT_ID (CheckLOS's hit record).
typedef struct HIT_DATA {
    float local[4][4];                  /* 0x000: the shape's frame in its part */
    float world[4][4];                  /* 0x040: local * the part's matrix180 (gaMulMatrix); row 0 is the plane normal */
    float localPt[4];                   /* 0x080 */
    float worldPt[4];                   /* 0x090: a point of the plane */
    float vel[4];                       /* 0x0a0: kind 2: how far the rows' sum moved this frame, / 4 (/ 3 with _SIX) */
    float localV[6][4];                 /* 0x0b0: the shape's vectors (3 used: 0, 3, 4; all 6 with HIT_DATA_FLAG_SIX) */
    u_int w110[3];                      /* 0x110: copied to w180 (2 words; 3 with _SIX) */
    u_char pad11c[4];
    float worldV[6][4];                 /* 0x120: localV in the world */
    u_int w180[3];                      /* 0x180 */
    u_char pad18c[4];
    float planeD;                       /* 0x190: worldPt . world[0] (xyz) */
    u_int flags;                        /* 0x194: HIT_DATA_FLAG_* (LOS_HIT_FLAGS) */
    short part;                         /* 0x198: its part (parts header index; LOS_HIT_ID) */
    s_char kind;                        /* 0x19a: SS_L2W_*: which pass moves it */
    u_char pad19b;
} HIT_DATA;
static_assert(offsetof(HIT_DATA, worldV) == 0x120, "HIT_DATA.worldV");
static_assert(offsetof(HIT_DATA, planeD) == 0x190, "HIT_DATA.planeD");
static_assert(offsetof(HIT_DATA, kind) == 0x19a, "HIT_DATA.kind");
#define HIT_DATA_FLAG_SIX 0x40000000u   /* six vectors and three rows (the flagged shape; its meaning unread) */
// parts: pModelPartsHeader[bank] + PARTS_HDR_RECORDS; hits: the bank's ALL_TABLE.hits.
void ssTransHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits, int mode);     // CT3 0x018ac920
void ssTransScnDmgHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits, int mode); // CT3 0x018ac0d0
#define SS_L2W_INIT 3                   /* ssHitInit's mode: the shapes of kind 3, once */
#define SS_L2W_FRAME 2                  /* ssCollisionCheck's, once per bank and frame: kind 2 (moving), with vel */
void ssTransColHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits);            // CT3 0x018ac570
void ssMoveAllHitObjCoordinate(struct HIT_TABLE *hits, const float *d);                  // CT3 0x018ad760
void ssSplitVectorHandV(float *h, float *v, const float *src);                           // CT3 0x018b1820

// ---- line-of-sight tests (from LosHitFadeUpdateTarget's calls, QuatSlerp.c slice 5, 2026-11-10) ----
// CheckLOS(bank, seg): != 0 when the segment from -> to hits the bank's collision.
typedef struct LOS_SEG {
    float from[4];                      /* 0x00: w 1 */
    float to[4];                        /* 0x10: w 1 */
} LOS_SEG;
static_assert(sizeof(LOS_SEG) == 0x20, "LOS_SEG");
#define LOS_MULTI_MAX 255
// CheckLOS_Multi(bank, q): != 0 on a hit; fills hits[0..count) with the hit records, whose short at
// LOS_HIT_ID the caller compares with its own id list. layout: host (the ROM's: 0x420 bytes on the
// stack, from at 0, to at 0x10, hits at 0x20, count at 0x41c; the hits are pointers).
typedef struct LOS_MULTI {
    float from[4];
    float to[4];
    void *hits[LOS_MULTI_MAX];
    u_int count;
} LOS_MULTI;
#define LOS_HIT_ID 0x198                /* short: the hit record's id (the faded part's hitIds list) */
int CheckLOS_Multi(int bank, LOS_MULTI *q);                                              // CT3 0x018aa950
int CheckLOS(int bank, LOS_SEG *seg);                                                    // CT3 0x018ab180
// ---- attack requests (from asCharacterMortionFunctionOperation's calls, 2026-11-11) ----
// chara strikes with its shape's attack `atk` (power: the motion record's short at +0x2c; motion: its slot
// 0 motion) against `bank` (its lightBank, or the moth's bank); a2 0, a4 -1, a5 1 at the one call site.
void ssHitAddAttackRequest(int chara, int atk, int kind, int power, int target, int b0d, int motion, int bank); // CT3 0x018aa8e0
// The part index (of bank's parts header) of attack `atk`.
int ssHit_GetMaxIDFromAtkID(int bank, int atk);                                          // CT3 0x018ac6a0
// What CheckLOS leaves after the segment on a hit, as tiCharaFootAdjust reads it (2026-11-11): the
// caller's buffer runs on past LOS_SEG -- +0x20 the hit point, +0x30 the hit record (an EE word there).
// layout: host (the record is a host pointer here); CheckLOS's TU confirms both and fills them.
typedef struct LOS_HIT {
    LOS_SEG seg;                        /* 0x00 */
    float point[4];                     /* 0x20: where the segment meets the surface */
    void *record;                       /* 0x30: the hit record (LOS_HIT_ID +0x198, LOS_HIT_FLAGS +0x194) */
} LOS_HIT;
#define LOS_HIT_FLAGS 0x194             /* u_int in the hit record */
#define LOS_HIT_FLAG_FLOOR 0x100000u    /* a surface feet stand on (tiCharaFootAdjust's only test) */
// The direction of v in the plane that drops `axis` (1: the XZ plane, from z and x), radians in [0, 2pi);
// a software double on the EE (from ssTrigLookAtCheck's call, 2026-11-11).
double ssRadianByO(const float *v, int axis);                                            // CT3 0x018ad300
