// game/Clipping_Portal.h -- CT3 game/Clipping_Portal.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// ---- wind (tiWindInit / tiWindUpdate / tiWindEnd, 2026-11-11) ----
// A bank's wind records (ALL_TABLE.winds, a 0-ended list of EE words). layout: ee -- bank data; matrix
// is a relocated EE address (BankEePtr of the record's bank). Each frame tiWindUpdate carries the three
// local points into the frame of the part's matrix200; the readers of the world points (the particle
// and pendulum code, later slices) name them.
typedef struct WIND {
    int bank;                           /* 0x00: the bank that registered it (tiWindInit); tiWindEnd drops it */
    int part;                           /* 0x04: the model part whose matrix200 carries it */
    float radius;                       /* 0x08: the wind capsule's radius (tiCheckWindForce) */
    u_int matrix;                       /* 0x0c: EE address of that part's matrix200 (part 0's when the
                                           part's kind is PARTS_KIND_WIND_ON_ROOT) */
    float local10[4];                   /* 0x10: a point in the part's frame -> world10 */
    float world10[4];                   /* 0x20 */
    float local30[4];                   /* 0x30 -> world30 */
    float local40[4];                   /* 0x40 -> world40 */
    float world30[4];                   /* 0x50 */
    float world40[4];                   /* 0x60 */
} WIND;
static_assert(offsetof(WIND, matrix) == 0x0c, "WIND.matrix");
static_assert(offsetof(WIND, world10) == 0x20, "WIND.world10");
static_assert(offsetof(WIND, world40) == 0x60, "WIND.world40");
#define WIND_MAX 10                     /* WindList's slots */
#define PARTS_KIND_WIND_ON_ROOT 1       /* MODEL_PARTS.kind: the wind takes part 0's frame (= PARTS_KIND_ITEM_LOCKED;
                                           named from this use) */
extern WIND *WindList[WIND_MAX];                /* sbss 0x0193d290 */
int tiCheckWindForce(float *p, WIND *wind);                                             // CT3 0x0188c8b0
// ssCalcParticleOrbit's wind motion (orbit kind 4) for one record of the first particle system: head is
// the system's header, whose first quad is the acceleration; obj the record's draw object (its +0x10
// gets the position), rec the 0xc0-byte record (+0x40 position, +0x80 velocity, +0xac last time); now
// the time in 1/300 s ticks. The record layout belongs to ssCalcParticleOrbit's TU (PTCL_* in the .c).
void tiParticleSetPosition_WIND(const float *head, u_char *obj, u_char *rec, int now);   // CT3 0x0188c040
// The angle of v in one plane, in degrees (-180..180): axis 0 = (x, y), 1 = (y, z), 2 = (x, z) -- the
// first is the "x" of atan2. Other axes answer 0.
float tiGetDegFromVec(float *v, char axis);                                              // CT3 0x01894a90

// ---- light belts (tiLightBelt2Init, 2026-11-11; tiLightBelt2Update, 2026-11-11 slice 6) ----
// A bank's light belts (ALL_TABLE.lightBelts, a 0-ended list of EE words). layout: ee -- bank data;
// points is a relocated EE address. A belt either sits on one part (link 0: every point at the part's
// position) or spans two parts: halfA / halfB are half the span, each in its own part's frame (w 0).
typedef struct LIGHT_BELT2 {
    int partA;                          /* 0x00: the model part it starts at */
    u_short rows;                       /* 0x04: rows of verts[] (3: edge, centre, edge; tiLightBelt2Update) */
    u_short count;                      /* 0x06: points[]'s length = vertices per row */
    u_int verts;                        /* 0x08: EE address of u_int[rows * count]: each a LIGHT_BELT2_VERT's
                                           EE address, row-major (row 1 the centre line) */
    int link;                           /* 0x0c: 0 = on partA alone; else spans partA .. partB */
    u_int points;                       /* 0x10: EE address of float[count][4] */
    int partB;                          /* 0x14: the part it ends at (link != 0) */
    u_int pad18[2];
    float halfA[4];                     /* 0x20: (B.pos - A.pos) / 2 in A's frame (inverse of A's matrix), w 0 */
    float halfB[4];                     /* 0x30: (A.pos - B.pos) / 2 in B's frame, w 0 */
} LIGHT_BELT2;
static_assert(offsetof(LIGHT_BELT2, points) == 0x10, "LIGHT_BELT2.points");
static_assert(offsetof(LIGHT_BELT2, halfA) == 0x20, "LIGHT_BELT2.halfA");
static_assert(sizeof(LIGHT_BELT2) == 0x40, "LIGHT_BELT2");
static_assert(offsetof(LIGHT_BELT2, verts) == 0x08, "LIGHT_BELT2.verts");

// One vertex of a belt's ribbon (tiLightBelt2Update, 2026-11-11). layout: ee -- bank data. The model has the
// same position in several vertex slots: tiLightBelt2Update writes slots[0] and copies it to the rest.
typedef struct LIGHT_BELT2_VERT {
    u_int slots;                        /* 0x00: EE address of u_int[n]: each the EE address of a float[4] */
    u_int pad04;
    int n;                              /* 0x08: slots[]'s length */
    float width2;                       /* 0x0c: the half width toward row 2, on a centre-row vertex */
    float width0;                       /* 0x10: the half width toward row 0, on a centre-row vertex */
} LIGHT_BELT2_VERT;
static_assert(offsetof(LIGHT_BELT2_VERT, width0) == 0x10, "LIGHT_BELT2_VERT.width0");
#define LIGHT_BELT2_ROW_CENTRE 1        /* verts[]'s row of the belt's points (view space) */
#define LIGHT_BELT2_ROW_EDGE0 0         /* the centre row pushed a quarter turn one way on screen (width0) */
#define LIGHT_BELT2_ROW_EDGE2 2         /* ... and a quarter turn the other way (width2) */
#define LIGHT_BELT2_TURN0 0x3ffc        /* gaSetRotMatrixZ angle (0x10000 a full turn): row 0's side */
#define LIGHT_BELT2_TURN2 0xbff4        /* ... row 2's side */
#define PARTS_FLAG43_BELT (PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS)   /* a two-part belt sets both (3) on its parts */

// ---- pendulums (huriko; tiHurikoUpdate, 2026-11-11 slice 3) ----
// A bank's pendulums (ALL_TABLE.huriko, a 0-ended list of EE words). layout: ee -- bank data; the swing
// parameters named from tiCalc_HurikoOPEN / _CLOSED (2026-11-11 slice 15).
typedef struct HURIKO {
    short trigPart;                     /* 0x00: ssTrigCheckEnable's part: the pendulum swings while enabled;
                                           tiHurikoInit reads the word as its model part (the high half 0),
                                           tiHurikoReset_On compares the word with its id */
    u_char pad02[2];
    u_short chains;                     /* 0x04: chains of joints (tiHurikoInit) */
    u_short joints;                     /* 0x06: joints per chain, the root first */
    float drag;                         /* 0x08: / mass: the step's motion along the joint's normal is damped */
    float mass;                         /* 0x0c: the step change (this step less the last) is taken back / mass */
    u_int jointTable;                   /* 0x10: EE word -> u_int[chains * joints]: HURIKO_JOINT EE words,
                                           chain-major */
    u_int hits;                         /* 0x14: EE word -> a 0-ended list of HURIKO_HIT EE words */
    u_int flags;                        /* 0x18: HURIKO_FLAG_* */
    float gravity;                      /* 0x1c: times dt^2 / length: the pull back to the rest pose */
    float stiffness;                    /* 0x20: HURIKO_FLAG_SPRINGS: the springs to the four neighbours */
    float damping;                      /* 0x24: the velocity is scaled by it each step */
    float maxAccel;                     /* 0x28: > 0: a step's acceleration is clamped to it */
} HURIKO;
static_assert(offsetof(HURIKO, drag) == 0x08, "HURIKO.drag");
static_assert(offsetof(HURIKO, hits) == 0x14, "HURIKO.hits");
static_assert(offsetof(HURIKO, gravity) == 0x1c, "HURIKO.gravity");
static_assert(offsetof(HURIKO, maxAccel) == 0x28, "HURIKO.maxAccel");
static_assert(offsetof(HURIKO, flags) == 0x18, "HURIKO.flags");

// One joint of a pendulum chain (tiHurikoInit, 2026-11-11 slice 4). layout: ee -- bank data.
typedef struct HURIKO_JOINT {
    u_int posRef;                       /* 0x00: EE word -> u_int[copies]: EE words of the joint's vertex
                                           position, [0] the master (part-local; world while swinging) */
    u_int normalRef;                    /* 0x04: EE word -> u_int[copies]: EE words of its vertex normal
                                           (tiGetNewNormalData writes [0], copies it to the rest) */
    int copies;                         /* 0x08: the vertex's copies (posRef / normalRef entries) */
    float length;                       /* 0x0c: the rest offset from the joint above, straight down */
    float downLen;                      /* 0x10: the spring to the joint below: its rest length */
    float leftLen;                      /* 0x14: ... to the previous chain's */
    float rightLen;                     /* 0x18: ... to the next chain's */
    int hit;                            /* 0x1c: the last step pushed it out of a hit (velocity cleared) */
    u_int up;                           /* 0x20: EE address of the joint above's position (k >= 1) */
    u_int down;                         /* 0x24: ... of the joint below's (not set on the last joint) */
    u_int left;                         /* 0x28: ... of the same joint in the previous chain (cyclic) */
    u_int right;                        /* 0x2c: ... in the next chain (cyclic) */
    float restDir[4];                   /* 0x30: part-local: world pos - (up's world pos + (0,-1,0) * length),
                                           w 0 (k >= 1) */
    float vel[4];                       /* 0x40: the swing's velocity (world); tiHurikoReset and a hit set
                                           (0, 0, 0, 1) (VU0 vf0) */
    float step[4];                      /* 0x50: the last step's motion (world position - pos); tiHurikoReset
                                           sets (0, 0, 0, 1) */
    float pos[4];                       /* 0x60: k >= 1: the world position's xyz; the root: its local
                                           position (all four words) */
} HURIKO_JOINT;
static_assert(offsetof(HURIKO_JOINT, copies) == 0x08, "HURIKO_JOINT.copies");
static_assert(offsetof(HURIKO_JOINT, length) == 0x0c, "HURIKO_JOINT.length");
static_assert(offsetof(HURIKO_JOINT, downLen) == 0x10, "HURIKO_JOINT.downLen");
static_assert(offsetof(HURIKO_JOINT, hit) == 0x1c, "HURIKO_JOINT.hit");
static_assert(offsetof(HURIKO_JOINT, vel) == 0x40, "HURIKO_JOINT.vel");
static_assert(offsetof(HURIKO_JOINT, step) == 0x50, "HURIKO_JOINT.step");
static_assert(offsetof(HURIKO_JOINT, up) == 0x20, "HURIKO_JOINT.up");
static_assert(offsetof(HURIKO_JOINT, restDir) == 0x30, "HURIKO_JOINT.restDir");
static_assert(offsetof(HURIKO_JOINT, pos) == 0x60, "HURIKO_JOINT.pos");
#define HURIKO_REST_DOWN_Y -1.0f        /* @338 (0x019389f0) = (0, -1, 0, 0): a joint hangs straight down */

// A pendulum's hit (ALL_TABLE.hurikoHits, a 0-ended list of EE words): two part-local points carried
// into the part's frame each frame (tiHurikoHitUpdate). layout: ee -- bank data.
typedef struct HURIKO_HIT {
    int part;                           /* 0x00: its model part (PARTS_KIND_WIND_ON_ROOT: part 0's) */
    short pairInA;                      /* 0x04: non-zero: both points at a (a[0], a[1]); 0: a and b */
    u_char pad06[2];
    u_int partFlags;                    /* 0x08: EE address of the part's MODEL_PARTS.flags (tiHurikoInit);
                                           PARTS_FLAG_HIDDEN: not carried */
    u_int a;                            /* 0x0c: EE word -> the first point (two with pairInA) */
    u_int b;                            /* 0x10: EE word -> the second point */
    u_int matrix;                       /* 0x14: EE address of the part's MODEL_PARTS.matrix200 (tiHurikoInit) */
    u_char pad18[8];
    float world0[4];                    /* 0x20 */
    float world1[4];                    /* 0x30 */
} HURIKO_HIT;
static_assert(offsetof(HURIKO_HIT, matrix) == 0x14, "HURIKO_HIT.matrix");
static_assert(offsetof(HURIKO_HIT, world0) == 0x20, "HURIKO_HIT.world0");
static_assert(sizeof(HURIKO_HIT) == 0x40, "HURIKO_HIT");
#define HURIKO_HIT_A_RADIUS 0x20        /* the a record's float: tiCheckHit_Capsule's radius */
// p against the hit: a plane (world0 on it, world1 its normal) or a capsule world0..world1; p pushed out
// when inside. PORT: the capsule takes the bank (its radius is behind the a word).
int tiCheckHit_Face(float *p, HURIKO_HIT *hit);                                          // CT3 0x01890f30
int tiCheckHit_Capsule(float *p, HURIKO_HIT *hit, int bank);                             // CT3 0x01890fe0
#define HURIKO_FLAG_CLOSED 0x1          /* tiCalc_HurikoCLOSED swings it (the last chain links to the first),
                                           else tiCalc_HurikoOPEN */
#define HURIKO_FLAG_SPRINGS 0x2         /* springs to the joints above, below and beside (stiffness) */
#define HURIKO_FLAG_DISABLED 0x4        /* its trigger was off (set each frame it is off) */
#define HURIKO_FLAG_RESET 0x8           /* re-enabled: tiHurikoReset on the next enabled frame */
#define HURIKO_FLAG_SEAMS 0x10          /* the midpoints between neighbouring chains are hit-tested too */
#define HURIKO_FLAG_WIND 0x80000000u    /* the live winds blow on it */
#define HURIKO_HITS_MAX 50              /* hFace / hCapsule: 200 bytes each (symtab) */

// ---- lens flares (tiLensFlaresInit, 2026-11-11 slice 7) ----
// A bank's lens flares (ALL_TABLE.lensFlares, a 0-ended list of EE words). layout: ee -- bank data; every
// pointer is a relocated EE word (BankEePtr). tiLensFlaresInit points each flare at its part and fills
// each element's sprite packet; tiDrawLensFlares (slice 12) spreads the elements along the line from the
// flare's screen point to the screen centre.
typedef struct LENS_FLARE_ELEM {
    u_int tex;                          /* 0x00: TEX_INFO, 0 = none; u0 = maxu, u1 = u0 + minu, v0 = maxv,
                                           v1 = v0 + minv (the sprite's texel rectangle) */
    u_int tbuffer;                      /* 0x04: the texture's upload packet */
    u_int size;                         /* 0x08: the sprite's size (tiDrawLensFlares halves it) */
    float factor;                       /* 0x0c: where on the line to the centre it sits */
    u_int packet;                       /* 0x10: LENS_FLARE_PACKET */
} LENS_FLARE_ELEM;
static_assert(sizeof(LENS_FLARE_ELEM) == 0x14, "LENS_FLARE_ELEM");

typedef struct LENS_FLARE {
    int parts;                          /* 0x00: the model part (tiDrawLensFlares reads its low u_short as the
                                           trigger part) */
    u_int partsPos;                     /* 0x04: <- EE address of ALL_TABLE.partsPos[parts] (w + 128: alpha) */
    u_int partPos;                      /* 0x08: <- EE address of that part's MODEL_PARTS.pos */
    int count;                          /* 0x0c: elements */
    u_int elems;                        /* 0x10: LENS_FLARE_ELEM[count] */
} LENS_FLARE;
static_assert(offsetof(LENS_FLARE, elems) == 0x10, "LENS_FLARE.elems");

// An element's sprite packet (0x60 bytes of u_long words, EE data): a DMA CNT tag of 5 qwords (its VIF
// codes 0), a qword of VIF NOP x3 / DIRECT 4, a GIF tag (one REGLIST loop of six registers), then PRIM,
// RGBAQ (tiDrawLensFlares), UV, XYZF2, UV, XYZF2. tiLensFlaresInit writes all but RGBAQ and the XYZF2s.
#define LENS_FLARE_PKT_DMATAG 0         /* word index; word 1 is the tag's VIF codes */
#define LENS_FLARE_PKT_VIFCODES 2       /* words 2..3: VIF NOP x3, VIF DIRECT 4 */
#define LENS_FLARE_PKT_GIFTAG 4
#define LENS_FLARE_PKT_REGS 5
#define LENS_FLARE_PKT_PRIM 6
#define LENS_FLARE_PKT_RGBAQ 7
#define LENS_FLARE_PKT_UV0 8
#define LENS_FLARE_PKT_XYZF0 9
#define LENS_FLARE_PKT_UV1 10
#define LENS_FLARE_PKT_XYZF1 11
#define LENS_FLARE_PKT_WORDS 12
#define LENS_FLARE_DMATAG 0x10000005ull         /* DMA CNT, 5 qwords */
#define LENS_FLARE_VIF_DIRECT4 0x50000004ull    /* VIF DIRECT, 4 qwords (word 3's upper half) */
#define LENS_FLARE_GIFTAG 0x6400000000008001ull /* NLOOP 1, EOP, REGLIST, NREG 6 */
#define LENS_FLARE_GIFREGS 0x434310ull          /* PRIM, RGBAQ, UV, XYZF2, UV, XYZF2 */
#define LENS_FLARE_PRIM_SPRITE 6ull
#define LENS_FLARE_UV_HALF_U 8          /* added to u * 16: half a texel */
#define LENS_FLARE_UV_HALF_V 9          /* added to v * 16 (the ROM's own 9, not 8) */

// ---- particles (tiParticleUpdate, 2026-11-11 slice 3) ----
// PORT: the bank is added to the helpers that read a PARTICLE2_ENTRY's ring (an EE word in the bank).
struct PARTICLE2_ENTRY;
struct CAMERA;
void tiCalc_LifeSpan(struct PARTICLE2_ENTRY *ent, int bank);                            // CT3 0x0188f9a0
// ROM (ticks, entry, enabled); PORT: bank added.
void tiGenerateParticle(int ticks, struct PARTICLE2_ENTRY *ent, int enabled, int bank);  // CT3 0x0188fba0
struct PARTICLE2_EMITTER;
// dst <- a unit matrix moved to one of the emitter's vertices (taken at random, each once until all are
// used), in the frame of the emitter's part. ROM (dst, emitter); PORT: bank added (its words are EE).
void tiGetmatL2WFromMeshEmitter(float (*dst)[4], struct PARTICLE2_EMITTER *em, int bank); // CT3 0x0188fa40
#define PARTICLE_EMIT_DIV 5.0f          /* records = emitRate * (ticks + idleTicks) / 5: emitRate per 1/60 s */
#define PARTICLE_LIFE_TICKS 300.0f      /* lifeTenths * 0.1 * 300: the life in 1/300 s ticks */
#define PARTICLE_JITTER_PERCENT 100.0f  /* dirJitter / scaleJitter are percent */
// after: 0 for ALL_TABLE.particles2, 1 for particles2After; cam: Cam[0] for the scene banks, else Cam[6].
void tiCalc_Orbit(int ticks, struct PARTICLE2_ENTRY *ent, int after, struct CAMERA *cam, int bank); // CT3 0x0188f2b0

// ---- the records' motions (tiCalc_Orbit, 2026-11-11 slice 8) ----
// PARTICLE2_ENTRY.orbit: which motion tiCalc_Orbit runs on every live record.
enum {
    ORBIT_TRANS = 0,                    /* tiCalc_Trans: velocity and acceleration */
    ORBIT_TO_GOAL = 1,                  /* tiCalc_TransToGoal: accelerate toward part2's position */
    ORBIT_TO_GOAL_STOP = 2,             /* tiCalc_TransToGoal_Stop: the same, until within the stop distance */
    ORBIT_LEAF = 3,                     /* tiCalc_Leaf */
    ORBIT_TRANS_WIND = 4,               /* tiCalc_Trans_WIND */
    ORBIT_LEAF_WIND = 5,                /* tiCalc_Leaf_WIND */
    ORBIT_TWIST = 6                     /* tiCalc_Twist */
};
#define ORBIT_TICKS_PER_SEC 300.0f      /* the motions' dt = ticks / 300 */
#define ORBIT_ALPHA_SCALE 100.0f        /* the alpha curve's percent -> PARTICLE2.color[3] */
// The goal part's MODEL_PARTS.flags bit that turns the goal motions into tiCalc_Goal_Disable (coasting).
// It is PARTS_FLAG_FOG's bit (main_rtn: never drawn): a hidden part, probably (open-items 7 kb).
#define ORBIT_GOAL_PART_OFF 0x1

// The motions take (ticks, record); PORT: the bank is added last (the record's mats is a bank EE word).
// Each adds ticks to the record's age and moves PARTICLE2_MATS.local's row 3.
struct PARTICLE2;
void tiCalc_Trans(int ticks, struct PARTICLE2 *rec, int bank);                           // CT3 0x0188f1c0
void tiCalc_Goal_Disable(int ticks, struct PARTICLE2 *rec, int bank);                    // CT3 0x0188eeb0
// goal: the goal part's MODEL_PARTS.pos (host pointer; the ROM passes the entry's partPos word).
void tiCalc_TransToGoal(int ticks, struct PARTICLE2 *rec, float *goal, int bank);        // CT3 0x0188f090
void tiCalc_TransToGoal_Stop(int ticks, struct PARTICLE2 *rec, float *goal, int bank);   // CT3 0x0188ef30
void tiCalc_Leaf(int ticks, struct PARTICLE2 *rec, int arg, int bank);                   // CT3 0x0188ea90
void tiCalc_Trans_WIND(int ticks, struct PARTICLE2 *rec, int bank);                      // CT3 0x0188c6e0
void tiCalc_Leaf_WIND(int ticks, struct PARTICLE2 *rec, int arg, int bank);              // CT3 0x0188c210
void tiCalc_Twist(int ticks, struct PARTICLE2 *rec, struct PARTICLE2_ENTRY *ent, int bank); // CT3 0x0188e8c0
// view: the camera's view matrix. Rebuilds PARTICLE2_MATS.localView and rotView from local.
void tiCalc_matL2V(struct PARTICLE2 *rec, float (*view)[4], int bank);                   // CT3 0x0188e7c0
// eye: the camera position (asViewMatrix2PosRUF). Hands the eye in the record's frame to
// SetSTValueForRefraction.
void tiCalc_Refraction(struct PARTICLE2 *rec, float *eye, int bank);                     // CT3 0x0188e740
// obj: the record's ga draw object (host); eye: the eye in the record's frame; localView: its
// PARTICLE2_MATS.localView; index: PARTICLE2.refrIndex. Argument order from the disassembly (a0..a2, f12).
void SetSTValueForRefraction(u_char *obj, float *eye, float (*localView)[4], float index); // CT3 0x01893f90
void tiHurikoReset(int bank, HURIKO *h);                                                 // CT3 0x01890c00
void tiCalc_HurikoOPEN(HURIKO *h, int bank, int ticks);                                  // CT3 0x01892310
void tiCalc_HurikoCLOSED(HURIKO *h, int bank, int ticks);                                // CT3 0x018915a0
void tiGetNewNormalData(HURIKO *h);                                                      // CT3 0x018912b0
void tiGetCharaDirection(float *dir, const struct CHARACTER *chr);                           // CT3 0x018948e0
// dst.xyz = src.xyz / |src.xyz|; w scaled the same (VU0: the whole vector times rsqrt(x2+y2+z2)).
void gaNormalizeVectorXYZ(float *dst, const float *src);                                 // CT3 0x01891570
float gaNormalizeVector3XYZ(float *dst, const float *src);                              // CT3 0x018922c0

// ---- vibration ----
// Ticks run 5 a frame (LinkMatrixTree.c): tiVibrationOn's time is in frames.
#define VIB_TICKS_PER_FRAME 5
#define VIB_LARGE_SCALE 0.75f           /* tiVibrationOn: the large motor adds level * 0.75, capped at 255 */
#define VIB_LARGE_MAX 0xff
extern int VibTimeBaseTicks;                    /* sbss 0x0193f770: the vibration stops once VibTimeTicks passes it */

// ---- fade ----
#define FADE_TICKS_PER_SECOND 30.0f     /* tiFade_Start: FadeEndTicks = seconds * 30 */

void tiLightBelt2UpdateForALYSSA_Battle(void);                                           // CT3 0x01889f70
void tiLightBelt2UpdateForBOSS_Battle(void);                                             // CT3 0x0188a1f0
void tiLightBelt2Update(void);                                                           // CT3 0x0188a610
void tiCharaFootAdjust(void);                                                            // CT3 0x0188b790
void tiPipeJointUpdate(void);                                                            // CT3 0x0188bb60
void tiWindEnd(int scene);                                                               // CT3 0x0188cb10
void tiWindUpdate(void);                                                                 // CT3 0x0188cbe0
void tiVibrationTimer(void);                                                             // CT3 0x0188cd70
void tiVibrationOff(int port);                                                           // CT3 0x0188cdc0
// small: the small motor on (1) or off; large: the large motor's level (1 counts as 2; adds 0.75 of it to
// act_large); time: frames until tiVibrationTimer stops it, 0 = until tiVibrationOff.
void tiVibrationOn(int port, u_char small, int large, int time);                          // CT3 0x0188ce10
// base ** n (the name says factorial; the code is a power). tiDrawShadingOff's.
int tiFactorial(int base, u_int n);                                                       // CT3 0x01894870
void tiDrawShadingOff(void);                                                             // CT3 0x0188ced0

// ---- GS frame-buffer copies (slice 11, 2026-11-11) ----
#define EFFECT_FBUF_ADDRESS 0xa8000u    /* GS word address of the effect buffer (CaptureDBuffer's picture) */
#define GS_FRAME_WIDTH 512              /* the frame buffer's pixels: the GS window 0..511 x 0..447 */
#define GS_FRAME_HEIGHT 448
#define GS_FRAME_LOG2 9                 /* TEX0 TW / TH of the whole frame read as a texture */
#define VRAM_ALPHA_OPAQUE 0x80          /* tiVRAMViewer's alphaFix: the copy alone */
// The frame drawn so far, scaled into width x height pixels at EFFECT_FBUF_ADDRESS.
void CaptureDBuffer(u_int width, u_int height);                                          // CT3 0x018945f0
// A GS region (fbp; texW x texH pixels, TEX0 tw / th / psm, bytesPerPixel) drawn as a sprite over the
// window rectangle x, y, w x h, blended (Cs - Cd) * alphaFix + Cd. The ROM's f12..f15, then a0..t3.
void tiVRAMViewer(float x, float y, float w, float h, u_int fbp, int texW, int texH, u_int tw, u_int th,
                  u_int psm, int bytesPerPixel, int alphaFix);                           // CT3 0x01894ed0
// time: seconds (f12; x30 frames), type: Fade_Type (a0, a byte). MemorycardUpdate leaves a0 unset (open-items.md).
#define FADE_TYPE_MC_LOAD 4             /* mc_load_set (name from that use; = FADE_TYPE_OUT2_BLACK) */
// tiXcmd_FadeStart's kinds (2026-11-11): direction by twos, white the odd one.
#define FADE_TYPE_IN_BLACK 0
#define FADE_TYPE_OUT_BLACK 1
#define FADE_TYPE_IN_WHITE 2
#define FADE_TYPE_OUT_WHITE 3
#define FADE_TYPE_OUT2_BLACK 4
#define FADE_TYPE_OUT2_WHITE 5
#define FADE_TYPE_IN2_BLACK 6
#define FADE_TYPE_IN2_WHITE 7
void tiFade_Start(float time, int type);                                                 // CT3 0x0188d3c0
void tiFade_Draw(void);                                                                // CT3 0x0188d160
void tiXFadeOut_Draw(void);                                                              // CT3 0x0188d3f0
void tiDrawLensFlares(void);                                                             // CT3 0x0188df70
void tiParticleUpdate(void);                                                             // CT3 0x01890070
void tiHurikoUpdate(void);                                                               // CT3 0x01892ee0
void tiHurikoHitUpdate(void);                                                            // CT3 0x018931f0
void tiBumpSTValueUpdate(void);                                                          // CT3 0x01893850
void tiRefractionMapUpdate(void);                                                        // CT3 0x01894230
void tiFreeBossData(void);                                                               // CT3 0x01889df0
void tiWindEnd_All(void);                                                                // CT3 0x0188cb60
void ExportDrawToTex(void);                                                              // CT3 0x01893cf0
// work: the 3dlib work (GA_3DLIB_WORK, ga3dlibwork); screen: screen_0's twelve floats (GA_SCREEN_*).
void Clipping_Portal(void *work, float *screen);                                        // CT3 0x01889cd0
// thigh..ik: CHARACTER.legParts' records (host pointers); target: the floor point; footY: out.
// PORT: the bank is added in front. Answers 1 when the leg was bent.
int tiCalc_FootPos(int bank, u_char *thigh, u_char *calf, u_char *foot, u_char *ik, float *target,
                   float *footY);                                                       // CT3 0x0188b240
void tiChara_Translucence_End2(int fbuf, int chara);                                     // CT3 0x0188d720
void tiChara_Translucence_Start2(void);                                                  // CT3 0x0188dbd0
void tiChara_Translucence_End(float alpha, int fbuf);                                    // CT3 0x0188dca0
void tiChara_Translucence_Start(void);                                                   // CT3 0x0188dd40
// A window position (v: x, y pixels; z, f) as a GIF XYZF2 word.
void tiDR_XYZF2(u_long *dst, const float *v);                                            // CT3 0x01894950
// A GS local-to-local copy (BITBLTBUF / TRXPOS / TRXREG / TRXDIR 2): rrw x rrh pixels from sbp (sx, sy)
// to dbp (dx, dy). The ROM's a0..t3, then dy, rrw, rrh on the stack.
void tiBGT_MoveImage(u_int sbp, u_int sbw, u_int dbp, u_int dbw, u_int psm, u_int sx, u_int sy, u_int dx,
                     u_int dy, u_int rrw, u_int rrh);                                    // CT3 0x01894cf0
void tiLightBelt2Init(int scene);                                                        // CT3 0x0188b050
void tiWindInit(int scene);                                                              // CT3 0x0188ccb0
void tiLensFlaresInit(int scene);                                                        // CT3 0x0188e4a0
void tiParticleInit(int scene);                                                          // CT3 0x01890350
void tiHurikoInit(int scene);                                                            // CT3 0x018933f0
void tiPipeJointInit(int bank);                                                          // CT3 0x0188bf30 (main2.c: chara 3's shape)

// From ssTrigExecuteActCmd's commands (2026-11-11): the arguments the command passes; the last is 1 there.
void tiSet_CharaTranslucenceON(int chara, int level, int flag);                          // CT3 0x0188ded0
void tiSet_CharaTranslucenceOFF(int chara, int flag);                                    // CT3 0x0188ddc0
void tiXFadeOut_Start(float time);                                                       // CT3 0x0188d6e0
// From tiXcmd_XFadeOut2Start (2026-11-11): time in seconds, kind a byte.
void tiXFadeOut_Start2(float time, u_char kind);                                         // CT3 0x0188d5e0

// From tiXcmd_TransItem.c (2026-11-11). dst = src turned about the point pos (pos's translation out,
// src, then back: two gaMulMatrix with matUnit + -pos / + pos rows).
void tiGetTransRotMatrix(float (*dst)[4], float *pos, float (*src)[4]);                  // CT3 0x018949c0
// Sets bit 2 of the pendulums (huriko) of bank `bank` whose id is `id` (0: all); byChara: `bank` is a
// Character[] index and its bank is taken.
void tiHurikoReset_On(int bank, int id, int byChara);                                    // CT3 0x01890ea0
