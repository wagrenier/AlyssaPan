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
    float world[4][4];                  /* 0x040: local * the part's matrix180 (gaMulMatrix): the polygon's corners
                                           (4; 3 with HIT_DATA_FLAG_TRIANGLE), as CheckLOS walks them */
    float localN[4];                    /* 0x080 */
    float worldN[4];                    /* 0x090: the plane's normal (CheckLOS: a segment crosses from its front) */
    float vel[4];                       /* 0x0a0: kind 2: how far the corners' sum moved this frame, / 4 (/ 3) */
    float localV[6][4];                 /* 0x0b0: the shape's vectors (3 used: 0, 3, 4; all 6 with HIT_DATA_FLAG_TRIANGLE) */
    float localHalf[3];                 /* 0x110: copied to half (2 floats; 3 with _TRIANGLE) */
    u_char pad11c[4];
    float worldV[6][4];                 /* 0x120: localV in the world */
    float half[3];                      /* 0x180: the half sizes the box tests read (slice 4, 2026-11-11):
                                           a quad's along worldV[3] / [4] about its centre worldV[0]
                                           (ssHitCheckOBB_Plane, from the call); a triangle's sides' half
                                           lengths, side i centred on worldV[i] along the unit worldV[3 + i]
                                           (ssHitCheckOBB_TriangleSides) */
    u_char pad18c[4];
    float planeD;                       /* 0x190: worldN . world[0] (xyz): the plane's distance along its normal */
    u_int flags;                        /* 0x194: HIT_DATA_FLAG_* (LOS_HIT_FLAGS) */
    short part;                         /* 0x198: its part (parts header index; LOS_HIT_ID) */
    s_char kind;                        /* 0x19a: SS_L2W_*: which pass moves it */
    u_char pad19b;
} HIT_DATA;
static_assert(offsetof(HIT_DATA, worldV) == 0x120, "HIT_DATA.worldV");
static_assert(offsetof(HIT_DATA, planeD) == 0x190, "HIT_DATA.planeD");
static_assert(offsetof(HIT_DATA, kind) == 0x19a, "HIT_DATA.kind");
#define HIT_DATA_FLAG_TRIANGLE 0x40000000u /* three corners (and six vectors); clear: a quad (CheckLOS, 2026-11-11) */
#define HIT_DATA_FLAG_NO_LOS 0x10000000u /* set: lines of sight pass through (CheckLOS / CheckLOS_Multi skip it) */
// A scene bank's ALL_TABLE.hits seen as CheckLOS reads it (slice 2, 2026-11-11): the shapes sorted into an
// 8 x 8 grid of cells over XZ, two index lists per cell. layout: ee -- bank data; the same record as
// HIT_TABLE (wideXcmd_DebugPrint.h), whose +0x08..0x2c are a character bank's (FS_LOAD_SHAPE) instead.
#define HIT_GRID_N 8                    /* cells per side; a cell is x + z * HIT_GRID_N */
typedef struct HIT_SCENE_GRID {
    u_int hitData;                      /* 0x000: HIT_TABLE.hitData: u_int[], EE words of HIT_DATA */
    u_int checkList;                    /* 0x004: u_int[checkCount]: EE words of the HIT_DATA a character's box
                                           is tested against (ssAddHitDataToCheckList, slice 5, 2026-11-11) */
    int checkCount;                     /* 0x008: ssHitCheckAndProcess clears it for each character */
    u_char pad0c[4];
    u_int cellA[HIT_GRID_N * HIT_GRID_N]; /* 0x010: per cell, u_short[cellACount]: indices into hitData --
                                           the still shapes, sorted by the grid writer (the bank file) */
    u_int cellB[HIT_GRID_N * HIT_GRID_N]; /* 0x110: the moving shapes (kind SS_L2W_FRAME), re-sorted each
                                           frame by ssDivideHitDataToSpaceCell (slice 5) */
    u_char cellACount[HIT_GRID_N * HIT_GRID_N]; /* 0x210 */
    u_char cellBCount[HIT_GRID_N * HIT_GRID_N]; /* 0x250: ssCollisionCheck clears it once per bank and frame */
    float originX;                      /* 0x290: the grid's corner, relative to part 0's position */
    float originZ;                      /* 0x294 */
    float cellW;                        /* 0x298: a cell's size along x */
    float cellD;                        /* 0x29c: along z */
} HIT_SCENE_GRID;
static_assert(offsetof(HIT_SCENE_GRID, cellACount) == 0x210, "HIT_SCENE_GRID.cellACount");
static_assert(offsetof(HIT_SCENE_GRID, cellD) == 0x29c, "HIT_SCENE_GRID.cellD");
static_assert(offsetof(HIT_SCENE_GRID, checkCount) == 0x008, "HIT_SCENE_GRID.checkCount");
// More HIT_DATA flags (slice 5, 2026-11-11, from ssAddHitDataToCheckList; named from that one reader).
#define HIT_DATA_FLAG_NO_COLLIDE 0x00800000u   /* set: never in a character's check list */
#define HIT_DATA_FLAG_CHECKING 0x02000000u     /* ssHitCheckAndProcess sets it while it slides along the
                                                  shape, so its recursion leaves the shape out */
#define HIT_DATA_FLAG_NOT_PLAYER 0x00080000u   /* the player's character passes through it */
#define HIT_DATA_FLAG_PLAYER_ONLY 0x00040000u  /* only the player's character collides with it */
// ---- a character's box and the scene grid (slice 5, 2026-11-11) ----
#define CHARA_OBB(c) ((SS_OBB *)(c)->obb)       /* CHARACTER.obb seen as its box */
// The character's box (CHARACTER.obb) around its collision points (hits: its shape bank's HIT_TABLE,
// colWorld), in the frame turned by *yaw (and *pitch) that the move ssHitBPos -> ssHitNPos faces; grow
// stretches it along that move by the move's length. *yaw / *pitch == -1 ask for them to be measured
// (and are written back); with no move the yaw is -rot and the pitch 0. The old box goes to obbPrev.
void ssMakeCharOBB(struct CHARACTER *chara, struct HIT_TABLE *hits, int grow, int *yaw, int *pitch); // CT3 0x018acdc0
// The grid cells (CHARACTER.cells) the box's XZ bounds cover; parts: the scene bank's parts (part 0
// places the grid). Appends to cellCount (the caller clears it).
void ssGetCharSpaceCell(struct CHARACTER *chara, struct MODEL_PARTS *parts, HIT_SCENE_GRID *grid); // CT3 0x018abf00
// The scene's moving shapes (kind SS_L2W_FRAME, part not NO_HIT) into the cellB lists of the cells their
// corners' XZ bounds cover.
void ssDivideHitDataToSpaceCell(struct MODEL_PARTS *parts, HIT_SCENE_GRID *grid);        // CT3 0x018abc90
// The shapes of the character's cells (cellA; also cellB when withMoving) into grid->checkList, once each,
// leaving out NO_COLLIDE / CHECKING shapes, those of NO_HIT parts and the player / non-player ones.
void ssAddHitDataToCheckList(struct CHARACTER *chara, struct MODEL_PARTS *parts, HIT_SCENE_GRID *grid,
                             int withMoving);                                            // CT3 0x018ab9c0
// A point is inside a shape's polygon when the angles it sees the corners' edges under (ahAcos units, a
// full turn 0x10000) add up to at least this: a full turn less ~0.8%.
#define LOS_INSIDE_ANGLE 65014.0f
// parts: pModelPartsHeader[bank] + PARTS_HDR_RECORDS; hits: the bank's ALL_TABLE.hits.
void ssTransHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits, int mode);     // CT3 0x018ac920
void ssTransScnDmgHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits, int mode); // CT3 0x018ac0d0
#define SS_L2W_INIT 3                   /* ssHitInit's mode: the shapes of kind 3, once */
#define SS_L2W_FRAME 2                  /* ssCollisionCheck's, once per bank and frame: kind 2 (moving), with vel */
void ssTransColHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits);            // CT3 0x018ac570
void ssMoveAllHitObjCoordinate(struct HIT_TABLE *hits, const float *d);                  // CT3 0x018ad760
void ssSplitVectorHandV(float *h, float *v, const float *src);                           // CT3 0x018b1820
// An attack object (HIT_TABLE.objs of a character bank, or a scene attack; slice 2, 2026-11-11, from
// ssTrans*AtkHitDataL2W). layout: ee -- bank data; only +0xd0.. read so far. HIT_OBJ_ID (+0xda) is its part.
typedef struct HIT_OBJ {
    u_char pad00[0xd0];
    u_int local;                        /* 0xd0: EE word of the bank: float[rows][4], the shape in its part */
    u_int world;                        /* 0xd4: EE word: float[rows][4], the same in the world */
    short rows;                         /* 0xd8: qwords in local / world (moved 4 at a time) */
    short part;                         /* 0xda: HIT_OBJ_ID: its part (parts header index) */
    s_char atk;                         /* 0xdc: HIT_OBJ_ATK: the attack id it serves */
} HIT_OBJ;
static_assert(offsetof(HIT_OBJ, part) == 0xda, "HIT_OBJ.part");
static_assert(offsetof(HIT_OBJ, atk) == 0xdc, "HIT_OBJ.atk");
// A character's damage shape (a character bank's HIT_TABLE.hitData[0..dmgObjCount); slice 2, from
// ssTransDmgHitDataL2W): three local matrices moved by the part's matrix180. layout: ee -- bank data.
typedef struct HIT_DMG {
    float local[3][4][4];               /* 0x000 */
    float w0c0[4];                      /* 0x0c0: copied to w190 each time the shape moves */
    float world[3][4][4];               /* 0x0d0 */
    float w190[4];                      /* 0x190 */
    short part;                         /* 0x1a0: its part (parts header index) */
} HIT_DMG;
static_assert(offsetof(HIT_DMG, world) == 0xd0, "HIT_DMG.world");
static_assert(offsetof(HIT_DMG, part) == 0x1a0, "HIT_DMG.part");
// The box ssHitCheckOBB_OBB tests (slice 3, 2026-11-11, from its reads): an attack object (HIT_OBJ +0,
// ssHitMakeAttackOBB fills it), a damage shape's world matrices (HIT_DMG +0xd0: centre and axes are
// world[2], the half sizes w190) or a body box (HIT_TABLE.bodyObb). layout: ee -- bank data.
typedef struct SS_OBB {
    float corner[8][4];                 /* 0x00: the box's eight corners (ssHitCheckOneObject walks them as
                                           a path 0-1-..-7: the first edge that crosses a shape's plane) */
    float center[4];                    /* 0x80 */
    float axis[3][4];                   /* 0x90: unit axes */
    float half[3];                      /* 0xc0: half sizes along them */
} SS_OBB;
static_assert(offsetof(SS_OBB, half) == 0xc0, "SS_OBB.half");
#define HIT_DMG_OBB(d) ((const SS_OBB *)(d)->world) /* a damage shape seen as its box */
#define HIT_OBJ_OBB(o) ((const SS_OBB *)(o))        /* an attack object seen as its box */
int ssHitCheckOBB_OBB(const SS_OBB *a, const SS_OBB *b);                                 // CT3 0x018adf30
// The part positions ssHitMakeAttackOBB and ssHitCheckAttackToScn read (the attack part's transInit /
// drawOnPos, copied by ssHitCheck*Attack before the box is made).
extern float ssHitNPos[4];                      /* bss 0x0193ef50 */
extern float ssHitBPos[4];                      /* bss 0x0193ef60 */
// != 0 when the box obj (its corners and axes, SS_OBB) touches the shape h: its corners' path must cross
// h's plane, then the box must meet the quad (ssHitCheckOBB_Plane) or a triangle's sides
// (ssHitCheckOBB_TriangleSides), or the crossing edge pierce the triangle. On a hit with v != NULL, out =
// v less its part along h's normal (the motion slid along the surface; out w = the normal's w). self == h
// (the shape the caller stands on) widens h by SS_HIT_SELF_GROW for the test. ssHitCheckAttackToScn
// passes NULLs; ssHitCheckAllObject / ssHitReCheckAllObject the rest (slice 4, 2026-11-11).
#define SS_HIT_SELF_GROW 5.0f
int ssHitCheckOneObject(HIT_OBJ *obj, HIT_DATA *h, const float *v, float *out, HIT_DATA *self); // CT3 0x018b0350
// != 0 when the box meets one of the triangle h's three sides (h->worldV, h->half). PORT: the ROM passes
// &h->worldV and reads the half lengths 0x60 past it; here the shape itself.
int ssHitCheckOBB_TriangleSides(const SS_OBB *box, const HIT_DATA *h);                   // CT3 0x018ad7a0
// != 0 when the box meets the quad h (centre worldV[0], axes worldV[3] / [4], half sizes h->half; the
// same PORT as TriangleSides).
int ssHitCheckOBB_Plane(const SS_OBB *box, const HIT_DATA *h);                           // CT3 0x018af580
// != 0 when the scene attack / character attack `obj` strikes a damage shape of the scene bank `scn`
// (whose part is not PARTS_FLAG10_NO_HIT and that faces the attack, or moves); the first one goes to
// ssHit_informationScn with the point where the attack crosses its plane.
int ssHitCheckAttackToScn(HIT_OBJ *obj, struct HIT_TABLE *scn, struct HIT_TABLE *atkHits, SS_HIT_ATTACK *a); // CT3 0x018a9fe0
// != 0 when obj strikes the target character (a->target) of the damage table `dmg`: its body box, then
// each damage shape; every one struck goes to ssHit_information.
int ssHitCheckAttack(HIT_OBJ *obj, struct HIT_TABLE *dmg, SS_HIT_ATTACK *a, struct HIT_TABLE *atkHits,
                     struct HIT_TABLE *scnHits);                                         // CT3 0x018aa220
// v bounced off the plane of normal n by k: out = unit(v - k (n.v) n), answers the length before the
// unit; 0 (out untouched) when k (n.v) > 0 (from its one caller, ssXcmd_AXE_CheckHitAndCompDir).
float ssGetReflectVector3(float k, float *out, const float *n, const float *v);          // CT3 0x018b0860
void ssTransScnAtkHitDataL2W(struct MODEL_PARTS *parts, HIT_OBJ *obj);                   // CT3 0x018ac600
HIT_OBJ *ssTransAtkHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits, int atk); // CT3 0x018ac730
void ssTransDmgHitDataL2W(struct MODEL_PARTS *parts, struct HIT_TABLE *hits);            // CT3 0x018ac840

// ---- line-of-sight tests (from LosHitFadeUpdateTarget's calls, BRANCH_CLEAR.c slice 5, 2026-11-10) ----
// CheckLOS(bank, hit): != 0 when the segment from -> to crosses a polygon of the scene bank's collision
// from its front (slice 2, 2026-11-11); the nearest crossing's point and record are written past the
// segment (LOS_HIT), and nothing else on a miss.
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
// ---- attack requests (from asCharacterMortionFunctionOperation's calls, 2026-11-11) ----
// chara strikes with its shape's attack `atk` (power: the motion record's short at +0x2c; motion: its slot
// 0 motion) against `bank` (its lightBank, or the moth's bank); a2 0, a4 -1, a5 1 at the one call site.
void ssHitAddAttackRequest(int chara, int atk, int kind, int power, int target, int b0d, int motion, int bank); // CT3 0x018aa8e0
// The part index (of bank's parts header) of attack `atk`.
int ssHit_GetMaxIDFromAtkID(int bank, int atk);                                          // CT3 0x018ac6a0
// What CheckLOS leaves after the segment on a hit (confirmed by CheckLOS, 2026-11-11): +0x20 the nearest
// hit point, +0x30 its HIT_DATA (an EE word on the EE). layout: host (the record is a host pointer here).
typedef struct LOS_HIT {
    LOS_SEG seg;                        /* 0x00 */
    float point[4];                     /* 0x20: where the segment meets the surface */
    void *record;                       /* 0x30: the hit record (LOS_HIT_ID +0x198, LOS_HIT_FLAGS +0x194) */
} LOS_HIT;
#define LOS_HIT_FLAGS 0x194             /* u_int in the hit record */
#define LOS_HIT_FLAG_FLOOR 0x100000u    /* a surface feet stand on (tiCharaFootAdjust's only test) */
int CheckLOS(int bank, LOS_HIT *hit);                                                    // CT3 0x018ab180
// The direction of v in the plane that drops `axis` (1: the XZ plane, from z and x), radians in [0, 2pi);
// a software double on the EE (from ssTrigLookAtCheck's call, 2026-11-11).
double ssRadianByO(const float *v, int axis);                                            // CT3 0x018ad300
