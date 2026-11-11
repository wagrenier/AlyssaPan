// game/ssGrpPortalCheck.h -- CT3 game/ssGrpPortalCheck.c: the trigger system (event flags, variables,
// detectors, the trigger script) and the room portals a character steps through.
// Slice 1 (2026-11-10): the event / variable API, the script byte readers, the enable checks, the
// detector shapes, ProcessPortal and ssScnPortalCheck. Slice 2: Trigger_init / _deinit, TriggerGlobalInit,
// the trigger work records and the delay-enable ring. The rest are prototypes from the call sites.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

// ---- event flags and variables ------------------------------------------------------------------------
// Two sets of each: the global set (GlobalEventFlag / GlobalVariable, saved with the game) and one temp
// set per bank (TempEventFlag[bank] / TempVariable[bank], this TU's statics, allocated by Trigger_init from
// the bank's trigger data). Every accessor takes (id, global, bank): global != 0 picks the global set and
// ignores bank (callers pass 0 or -1 there).
#define TRIG_SET_TEMP 0
#define TRIG_SET_GLOBAL 1
#define TRIG_VAR_ROOM_ENTRY 0x20        /* ssScnPortalCheck: copied into CHARACTER.b102 on entering a room */

// ---- trigger detectors ---------------------------------------------------------------------------------
// One detector of a bank (ALL_TABLE.detectors: a NULL-terminated list of EE words, relocated bank data;
// layout: ee). A detector with a room hash is a portal: a character inside its planes enters that room.
// Only the fields this TU's slice 1 reads; Trigger_main / CheckPlaneEntry name the rest.
typedef struct TRIG_DETECTOR {
    u_int matrix;                       /* 0x00: EE address of its part's world matrix (MODEL_PARTS.matrix);
                                           InitTriggerDetectorShape sets it */
    u_char pad04[4];
    union {
        short roomId;                   /* 0x08: a portal: entering copies it to CHARACTER.h100 */
        int numPlanes;                  /* 0x08: a plane volume (CheckInsidePlanes): its planes' count */
    };
    short part;                         /* 0x0c: its model part in the bank's parts header */
    short bank;                         /* 0x0e: the room's bank (the character's lightBank after entry) */
    u_int normal;                       /* 0x10: a portal: float[4], the plane's normal in the part's space;
                                           a plane volume: TRIG_PLANE[numPlanes] (EE) */
    u_int corners;                      /* 0x14: a portal: float[4][4], the quad's corners in the part's
                                           space, corner 0 also the plane's point; a plane volume: the
                                           planes' points, float[4] each (EE) */
    u_long roomHash;                    /* 0x18: the room it leads into (CHARACTER.roomHash); 0 = no portal */
    float centre[4];                    /* 0x20: a box (CheckAxisAlignedBounds): its centre, part space */
    float halfSize[4];                  /* 0x30: its half extents along the world axes (xyz) */
} TRIG_DETECTOR;
static_assert(offsetof(TRIG_DETECTOR, centre) == 0x20, "TRIG_DETECTOR.centre");
static_assert(offsetof(TRIG_DETECTOR, halfSize) == 0x30, "TRIG_DETECTOR.halfSize");
static_assert(offsetof(TRIG_DETECTOR, part) == 0x0c, "TRIG_DETECTOR.part");
static_assert(offsetof(TRIG_DETECTOR, corners) == 0x14, "TRIG_DETECTOR.corners");
static_assert(offsetof(TRIG_DETECTOR, roomHash) == 0x18, "TRIG_DETECTOR.roomHash");

// A group portal of a bank (ALL_TABLE.groupPortals: a NULL-terminated list of EE words; layout: ee). Its
// quad is in world space; a character crossing it takes one of two room ids by the side it came from.
typedef struct GRP_PORTAL {
    u_char pad00[0x40];
    float corners[4][4];                /* 0x40: the quad's corners (world) */
    u_char pad80[0x10];
    float normal[4];                    /* 0x90: the plane's normal (world) */
    float dist;                         /* 0xa0: the plane's distance: normal . p for a point on it */
    short part;                         /* 0xa4: its model part (PARTS_FLAG_HIDDEN switches it off) */
    short roomIdFront;                  /* 0xa6: CHARACTER.h100 when crossed from the front, -1 = keep */
    short roomIdBack;                   /* 0xa8: CHARACTER.h100 when crossed from the back, -1 = keep */
} GRP_PORTAL;
static_assert(offsetof(GRP_PORTAL, normal) == 0x90, "GRP_PORTAL.normal");
static_assert(offsetof(GRP_PORTAL, roomIdBack) == 0xa8, "GRP_PORTAL.roomIdBack");
#define ROOM_ID_KEEP -1

// ---- the trigger script (Trigger_init) ----------------------------------------------------------------
// ALL_TABLE.trigger points at three EE words (layout: ee): the bank's "other" block (a header, then the
// script Trigger_main runs), its detector block and its actor block. The blocks are byte streams read
// with ssTrigRead*Byte (little-endian, unaligned).
typedef struct TRIG_DATA {
    u_int other;                        /* 0x00: OthData: the header, then the script */
    u_int detectors;                    /* 0x04: DetData: four 4-byte offsets per detector record */
    u_int actors;                       /* 0x08: ActData: two 4-byte offsets per actor */
} TRIG_DATA;
static_assert(sizeof(TRIG_DATA) == 0x0c, "TRIG_DATA");

// One plane of a plane volume (TRIG_DETECTOR.normal as a list; layout: ee, 0x20 bytes): the volume's
// inside is behind every plane. Each plane owns numPoints entries of the points list, the first on it.
typedef struct TRIG_PLANE {
    float normal[4];                    /* 0x00: part space; the matrix moves it with its w */
    int numPoints;                      /* 0x10: how far the points list advances past this plane */
    u_char pad14[0xc];
} TRIG_PLANE;
static_assert(sizeof(TRIG_PLANE) == 0x20, "TRIG_PLANE");

// The other block's header, in read order: a word Trigger_init skips, then six u_shorts: detectors,
// actors, temp event flags, temp variables, delay-enable slots, and the root detectors' count; the root
// detectors' ids (u_short each) follow. Trigger_main walks the roots, a true detector its children.

// TrigWork[bank]: the header's counts. layout: host (EE 12 bytes: +4 is a pointer).
typedef struct TRIG_WORK {
    u_short numDetectors;               /* EE 0x00: DetWork[bank]'s records */
    u_short numActors;                  /* EE 0x02: ActWork[bank]'s records */
    const u_char *roots;                /* EE 0x04: the other block past its header: the root detector ids */
    short numRoots;                     /* EE 0x08: the header's last short */
} TRIG_WORK;

// DetWork[bank][i]: one detector's state. layout: host (EE 0x1c bytes: +0x14 is a pointer). The *Pos
// fields are offsets into DetData[bank]: the act mode byte (the shape follows it: kind, character,
// count, then the shapes' ids), the condition count and the actor count (the actors' u_short ids follow).
typedef struct TRIG_DET_WORK {
    u_char state;                       /* EE 0x00: DET_STATE_*; 0 at init */
    u_char numConds;                    /* EE 0x01 */
    u_char numActors;                   /* EE 0x02 */
    u_char actMode;                     /* EE 0x03: DET_ACT_MODE_* */
    int cursor;                         /* EE 0x04: the read position the readers advance */
    u_int actModePos;                   /* EE 0x08 */
    u_int condsPos;                     /* EE 0x0c */
    u_int actorsPos;                    /* EE 0x10 */
    const u_char *children;             /* EE 0x14: DetData + the record's fourth offset + 2: child ids */
    short numChildren;                  /* EE 0x18: the list's leading short: run while this one is true */
} TRIG_DET_WORK;
#define DET_STATE_FALSE 0
#define DET_STATE_TRUE 1
#define DET_ACT_MODE_EDGE 0             /* (any value but 1) actors run when the state changes */
#define DET_ACT_MODE_LEVEL 1            /* actors run every frame: activate while true, deactivate while false */
// ssTrigCheckDetector's shape kinds (the byte after the act mode); each is followed by the character
// it tests and a count of shapes (u_short part ids; u_long room hashes for DET_SHAPE_ROOM).
#define DET_SHAPE_BOX 0                 /* CheckAxisAlignedBounds */
#define DET_SHAPE_PLANE_ENTRY 1         /* CheckPlaneEntry: crossed a quad from the front */
#define DET_SHAPE_INSIDE_PLANES 2       /* CheckInsidePlanes */
#define DET_SHAPE_ROOM 3                /* CheckRoomEntry */
#define DET_SHAPE_CONTACT 4             /* CheckContactSurface */
#define DET_SHAPE_ALWAYS 5              /* always true */
// ssTrigProceedDetector's answer, and the actor run mode (also TRIG_ACT_WORK.state's bit).
#define TRIG_RUN_NONE 0
#define TRIG_RUN_ACTIVATE 1
#define TRIG_RUN_DEACTIVATE 2

// One command list of an actor (activate or deactivate; ssTrigExecuteActCmd runs it). layout: ee.
typedef struct TRIG_ACT_CMDS {
    int cursor;                         /* 0x00: the next command's offset into ActData (pos + 1 at a start) */
    u_int pos;                          /* 0x04: offset into ActData[bank]: the commands' count byte */
    int wait;                           /* 0x08: -1 at a start (the running command's own state) */
    u_char num;                         /* 0x0c: the commands' count */
    u_char step;                        /* 0x0d: 0 at a start */
    u_char pad0e[2];
} TRIG_ACT_CMDS;
static_assert(sizeof(TRIG_ACT_CMDS) == 0x10, "TRIG_ACT_CMDS");

// ActWork[bank][i]: one actor's state and its activate / deactivate command lists. layout: ee.
typedef struct TRIG_ACT_WORK {
    u_int state;                        /* 0x00: TRIG_RUN_* bits: the lists running; 0 at init */
    short det[2];                       /* 0x04: the detector that started each list (-1 = a script call,
                                           ssTrigExecuteActor) */
    TRIG_ACT_CMDS cmds[2];              /* 0x08: activate, 0x18: deactivate (ACT_CMDS_*) */
} TRIG_ACT_WORK;
#define ACT_CMDS_ACTIVATE 0
#define ACT_CMDS_DEACTIVATE 1
static_assert(offsetof(TRIG_ACT_WORK, cmds[0].pos) == 0x0c, "TRIG_ACT_WORK activate pos");
static_assert(offsetof(TRIG_ACT_WORK, cmds[0].num) == 0x14, "TRIG_ACT_WORK activate num");
static_assert(offsetof(TRIG_ACT_WORK, cmds[1].pos) == 0x1c, "TRIG_ACT_WORK deactivate pos");
static_assert(offsetof(TRIG_ACT_WORK, cmds[1].num) == 0x24, "TRIG_ACT_WORK deactivate num");
static_assert(sizeof(TRIG_ACT_WORK) == 0x28, "TRIG_ACT_WORK");

// curActInfo (lbss 0x01943d60, 8 bytes): the actor ssTrigProceedActor is running, for the commands that
// ask (ssTrigGetCurActInfo).
typedef struct TRIG_CUR_ACT {
    int mode;                           /* 0x00: TRIG_RUN_* */
    u_short actor;                      /* 0x04 */
    short bank;                         /* 0x06 */
} TRIG_CUR_ACT;
static_assert(sizeof(TRIG_CUR_ACT) == 8, "TRIG_CUR_ACT");

// DlyEnaWork[bank]: a ring of delayed part switches (ssDERingFront / Rear, numMaxDECount[bank] slots).
// ssTrigExecDelayEnable applies an entry once its wait has run out, then pops the front. layout: ee.
typedef struct TRIG_DELAY_ENABLE {
    short part;                         /* 0x00: the bank's model part */
    u_char op;                          /* 0x02: DE_OP_* */
    u_char wait;                        /* 0x03: frames left; push sets DE_WAIT_PENDING */
} TRIG_DELAY_ENABLE;
static_assert(sizeof(TRIG_DELAY_ENABLE) == 4, "TRIG_DELAY_ENABLE");
#define DE_OP_HIDE 0                    /* set PARTS_FLAG_HIDDEN */
#define DE_OP_SHOW 1                    /* clear it */
#define DE_OP_TOGGLE 2                  /* flip it */
#define DE_OP_CANCELLED 3               /* ssTrigDE_DisableReq: popped without effect */
#define DE_WAIT_PENDING 1               /* ssTrigDE_ChangeReq walks the entries that still have it */
#define DE_SLOTS_PER_COUNT 2            /* Trigger_init: numMaxDECount = 2 x the header's count */

extern TRIG_WORK TrigWork[BANK_MAX];                    /* bss 0x01ad7810 */
extern TRIG_DET_WORK *DetWork[BANK_MAX];                /* bss 0x01940db0 */
extern TRIG_ACT_WORK *ActWork[BANK_MAX];                /* bss 0x01940f10 */

TRIG_DELAY_ENABLE *ssTrigDEPop_Front(TRIG_DELAY_ENABLE *ring, int bank);                 // CT3 0x018d3f30
int ssTrigDEPush_Back(TRIG_DELAY_ENABLE *ring, int bank, const TRIG_DELAY_ENABLE *req);  // CT3 0x018d3f90
int ssTrigDE_ChangeReq(TRIG_DELAY_ENABLE *ring, int bank, short part, u_char op);        // CT3 0x018d3e00
void ssTrigDE_DisableReq(TRIG_DELAY_ENABLE *ring, int bank, short part);                 // CT3 0x018d3ea0
void ssTrigExecDelayEnable(int index, int bank);                                         // CT3 0x018d4010
const u_char *ssTrigGetOtherData(int bank);                                              // CT3 0x018d9650
const u_char *ssTrigGetActData(int bank);                                                // CT3 0x018d9670
const u_char *ssTrigGetDetData(int bank);                                                // CT3 0x018d9690
u_char ssTrigReadNumDeactivateCmd(const TRIG_ACT_WORK *act, int bank);                   // CT3 0x018d96b0
u_char ssTrigReadNumActivateCmd(const TRIG_ACT_WORK *act, int bank);                     // CT3 0x018d96f0
u_char ssTrigReadActMode(const TRIG_DET_WORK *det, int bank);                            // CT3 0x018d9750
u_char ssTrigReadNumActors(const TRIG_DET_WORK *det, int bank);                          // CT3 0x018d9790
u_char ssTrigReadNumConds(const TRIG_DET_WORK *det, int bank);                           // CT3 0x018d97d0

#define PORTAL_CHARA_COUNT 6           /* ssScnShiftCharList / ssGrpShiftCharList: the characters checked */
#define PORTAL_AMBIENT_FADE 0x40        /* ssScnPortalCheck: tnChangeCharAmbientColorSet's third short */
#define PORTAL_DIRECT_LIGHT_FRAMES 20   /* DirectLightCharaCounter after a room change */
#define PORTAL_INSIDE_ANGLE 65014.0f    /* CheckPlaneEntry: the corner angles (ahAcos units, 65536 = a turn)
                                           summed around the crossing point: at least this = inside the quad */

extern short ssScnShiftCharList[PORTAL_CHARA_COUNT];   /* sdata 0x01939d68: 0, 2, 3, 4, 5, 6 */
extern short ssGrpShiftCharList[PORTAL_CHARA_COUNT];   /* sdata 0x01939d78: 0, 2, 3, 4, 5, 6 */

void ssGrpPortalCheck(CHARACTER *chara);                                                 // CT3 0x018d2ee0
void ssScnPortalCheck(CHARACTER *chara);                                                 // CT3 0x018d3240
void ProcessPortal(void);                                                                // CT3 0x018d33d0
int CheckPlaneEntry(int bank, int chara, int part);                                      // CT3 0x018d3770
void InitTriggerDetectorShape(int scene);                                                // CT3 0x018d3d90
void Trigger_main(void);                                                                 // CT3 0x018d4120
void ssTrigProcDetRecur(int count, const u_char *ids, int bank);                         // CT3 0x018d4710
int ssTrigProceedDetector(short det, int bank);                                          // CT3 0x018d88a0
int ssTrigCheckDetector(short det, int bank);                                            // CT3 0x018d8580
int ssTrigCheckConditions(TRIG_DET_WORK *det, int bank);                                 // CT3 0x018d7b70
int CheckAxisAlignedBounds(int bank, int chara, short part);                             // CT3 0x018d3520
int CheckInsidePlanes(int bank, int chara, short part);                                  // CT3 0x018d3bb0
int CheckContactSurface(short bank, int chara, short part);                              // CT3 0x018d36f0
void ssTrigRunActors(short det, int mode, int bank);                                     // CT3 0x018d7a90
void ssTrigRunActor(short det, u_short actor, u_int mode, int bank);                     // CT3 0x018d79f0
void ssTrigProceedActor(u_short actor, int mode, int bank);                              // CT3 0x018d78f0
int ssTrigExecuteActCmd(TRIG_ACT_CMDS *cmds, int bank);                                  // CT3 0x018d5450
void ssTrigCancelActor(u_short actor, int mode, int bank);                               // CT3 0x018d9190
int ssTrigExecuteActor(int *state, u_short actor, u_int mode, int bank);                 // CT3 0x018d9220
int ssTrigCheckRandom(int percent);                                                      // CT3 0x018d8e50
int ssTrigCheckSceneDraw(int nameWord);                                                  // CT3 0x018d89e0
int ssTrigWaitByAnimFrameForGlobal(int *wait, int ticks);                                // CT3 0x018d94c0
int ssTrigWaitByAnimFrame(int *wait, int ticks);                                         // CT3 0x018d9510
void ssTrigGetCurActInfo(TRIG_CUR_ACT *out);                                             // CT3 0x018d9730
void Trigger_deinit(int bank);                                                               // CT3 0x018d4be0
void Trigger_init(int bank);                                                                 // CT3 0x018d4c80
void TriggerGlobalInit(void);                                                            // CT3 0x018d5230
int ssTrigCheckEnable(int part, int bank);                                               // CT3 0x018d8e80
int ssTrigCheckInputEnable(u_int mask);                                                  // CT3 0x018d89c0
int ssTrigCheckEvent(int event, int global, int bank);                                   // CT3 0x018d9100
int CheckRoomEntry(int chara, u_long roomHash);                                          // CT3 0x018d9160
int ssTrigGetVariable(int var, int global, int bank);                                    // CT3 0x018d9430
void ssTrigSetVariable(int var, int value, int global, int bank);                        // CT3 0x018d9470
void ssTrigRemoveEvent(int event, int global, int bank);                                 // CT3 0x018d9560
void ssTrigRegisterEvent(int event, int global, int bank);                               // CT3 0x018d95e0
u_char ssTrigRead1Byte(int *pos, const u_char *data);                                    // CT3 0x018d9810
u_short ssTrigRead2Byte(int *pos, const u_char *data);                                   // CT3 0x018d9830
u_int ssTrigRead4Byte(int *pos, const u_char *data);                                     // CT3 0x018d9870
u_long ssTrigRead8Byte(int *pos, const u_char *data);                                    // CT3 0x018d98e0
