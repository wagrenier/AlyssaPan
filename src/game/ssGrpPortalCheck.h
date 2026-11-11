// game/ssGrpPortalCheck.h -- CT3 game/ssGrpPortalCheck.c: the trigger system (event flags, variables,
// detectors, the trigger script) and the room portals a character steps through.
// Slice 1 (2026-11-10): the event / variable API, the script byte readers, the enable checks, the
// detector shapes, ProcessPortal and ssScnPortalCheck. The rest are prototypes from the call sites.
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
    short roomId;                       /* 0x08: entering copies it to CHARACTER.h100 */
    u_char pad0a[2];
    short part;                         /* 0x0c: its model part in the bank's parts header */
    short bank;                         /* 0x0e: the room's bank (the character's lightBank after entry) */
    u_int normal;                       /* 0x10: float[4], the plane's normal in the part's space (EE) */
    u_int corners;                      /* 0x14: float[4][4], the quad's corners in the part's space (EE);
                                           corner 0 is also the plane's point */
    u_long roomHash;                    /* 0x18: the room it leads into (CHARACTER.roomHash); 0 = no portal */
} TRIG_DETECTOR;
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

#define PORTAL_CHARA_COUNT 6            /* ssScnShiftCharList / ssGrpShiftCharList: the characters checked */
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
void Trigger_deinit(void);                                                               // CT3 0x018d4be0
void Trigger_init(void);                                                                 // CT3 0x018d4c80
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
