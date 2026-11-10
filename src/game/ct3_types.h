// game/ct3_types.h -- CT3's shared game structs whose owning TU is not written yet (game/main.c first,
// 2026-11-09). Only the fields read so far are named; each struct moves to its owner's header when that TU
// is written (Character_All_Init's game/Character_Init.c, fileio.c's bank table, ...).
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// One character slot of Character[35] (0x01a4bfc0, 0x580 bytes each: the symtab's 0xc080 / 35).
typedef struct CHARACTER {
    s_char id;                          /* 0x000: -1 = slot free */
    u_char pad001;
    short bank;                         /* 0x002: its bank (BankInfo index), < 0 = none */
    short slot004[10];                  /* 0x004: Character_All_Init: -1 */
    u_char slot018[10];                 /* 0x018: Character_All_Init: 0 */
    u_char slot022[10][10];             /* 0x022: Character_All_Init: 0 */
    u_char pad086[2];
    u_int flags;                        /* 0x088: CHARACTER_FLAG_* */
    u_char pad08c[4];
    u_long w090;                        /* 0x090: Character_All_Init: 0 (a doubleword store) */
    u_long w098;                        /* 0x098: Character_All_Init: 0 (a doubleword store) */
    short lightBank;                    /* 0x0a0: the bank whose light main_rtn applies, -1 = none */
    u_char pad0a2[0xe];
    qword posBefore;                   /* 0x0b0: last frame's position (main copies pos here first) */
    qword pos;                          /* 0x0c0 */
    u_char pad0d0[0x18];
    int rotBefore;                      /* 0x0e8: last frame's value of 0x0ec */
    int rot;                            /* 0x0ec */
    u_char pad0f0[0xc];
    int w0fc;                           /* 0x0fc: Character_All_Init: 0 */
    short h100;                         /* 0x100: Character_All_Init: -1 */
    s_char b102;                        /* 0x102: Character_All_Init: -1 */
    u_char pad103[0x2f1 - 0x103];
    u_char b2f1;                        /* 0x2f1: Character_All_Init: 1 */
    u_char b2f2;                        /* 0x2f2: Character_All_Init: 0 */
    u_char pad2f3[0xd];
    int w300[4];                        /* 0x300: Character_All_Init: 0 */
    u_char behavior[0x524 - 0x310];     /* 0x310: asInitPlayerBehaviorControler's (0x310..0x488 at least) */
    int hp;                             /* 0x524: Character_All_Init: Char_Init_MAX_HP[i] */
    int hpMax;                          /* 0x528: Character_All_Init: Char_Init_MAX_HP[i] (which of the
                                           two is the maximum is a guess from the table's name) */
    u_char translucent;                 /* 0x52c: CHARA_TRANS_*; non-zero: AllDrawObject's last pass draws it */
    u_char pad52d[3];
    float translucentAlpha;             /* 0x530: tiChara_Translucence_End's first argument */
    u_char b534;                        /* 0x534: Character_All_Init: 0 */
    u_char pad535;
    short h536[6];                      /* 0x536: Character_All_Init: 0 */
    u_char pad542[0x570 - 0x542];
    struct CHARA_JUNC *junc;            /* 0x570: a block of gHeapJunc, freed with its buffer by
                                           Character_All_Init. layout: host (an EE word there) */
    u_char pad578[0x580 - 0x570 - sizeof(void *)];
} CHARACTER;
static_assert(sizeof(CHARACTER) == 0x580, "CHARACTER");
static_assert(offsetof(CHARACTER, bank) == 0x002, "CHARACTER.bank");
static_assert(offsetof(CHARACTER, flags) == 0x088, "CHARACTER.flags");
static_assert(offsetof(CHARACTER, lightBank) == 0x0a0, "CHARACTER.lightBank");
static_assert(offsetof(CHARACTER, posBefore) == 0x0b0, "CHARACTER.posBefore");
static_assert(offsetof(CHARACTER, pos) == 0x0c0, "CHARACTER.pos");
static_assert(offsetof(CHARACTER, rotBefore) == 0x0e8, "CHARACTER.rotBefore");
static_assert(offsetof(CHARACTER, translucent) == 0x52c, "CHARACTER.translucent");
static_assert(offsetof(CHARACTER, translucentAlpha) == 0x530, "CHARACTER.translucentAlpha");
static_assert(offsetof(CHARACTER, slot022) == 0x022, "CHARACTER.slot022");
static_assert(offsetof(CHARACTER, w090) == 0x090, "CHARACTER.w090");
static_assert(offsetof(CHARACTER, w0fc) == 0x0fc, "CHARACTER.w0fc");
static_assert(offsetof(CHARACTER, b2f1) == 0x2f1, "CHARACTER.b2f1");
static_assert(offsetof(CHARACTER, w300) == 0x300, "CHARACTER.w300");
static_assert(offsetof(CHARACTER, hp) == 0x524, "CHARACTER.hp");
static_assert(offsetof(CHARACTER, h536) == 0x536, "CHARACTER.h536");
static_assert(offsetof(CHARACTER, junc) == 0x570, "CHARACTER.junc");

// The gHeapJunc block at CHARACTER.junc: only its buffer pointer (EE +0xc) is read so far (Character_All_Init
// frees it, then the block). layout: host -- the EE's 0xc bytes before it are kept, the pointer is a host one.
typedef struct CHARA_JUNC {
    u_char pad00[0xc];
    void *buf;                          /* EE 0x0c: a gHeapJunc block */
} CHARA_JUNC;

// CHARACTER.translucent (AllDrawObject): any bit keeps the character out of the opaque and mirror passes.
#define CHARA_TRANS_TRANSLUCENT 0x1     /* drawn last between tiChara_Translucence_Start / _End */
#define CHARA_TRANS_GHOST 0x2           /* (without 0x1) drawn last by main_rtn_GHOST, _Start2 / _End2 */

#define CHARACTER_MAX 35                /* 0x23: Character[], AI_flag[], DirectLightCharaCounter[] */
#define CHARACTER_FLAG_REFRACTION 0x80000000u /* DrawObjectRefraction: with _SHADOW, the character's bank
                                             is drawn in the refraction pass (name from that use) */
#define CHARACTER_FLAG_SHADOW 0x40000000u /* main: the character's bank casts a shadow polygon */

// One camera of Cam[7] (0x01ae7c40, 0x1c0 bytes each; indexed by asCheckBankID: scene banks 0..5 and
// the shared one). Only what main.c reads.
typedef struct CAMERA {
    u_char pad000[0xf0];
    float focus[4];                     /* 0x0f0: the point looked at (ssHQDOF_DrawAllObject's focusPos) */
    u_char pad100[0xc0];
} CAMERA;
static_assert(sizeof(CAMERA) == 0x1c0, "CAMERA");
static_assert(offsetof(CAMERA, focus) == 0x0f0, "CAMERA.focus");

#define CAMERA_MAX 7                    /* Cam[]: 3136 bytes in the symtab */

// One loaded bank of BankInfo[86] (0x01aea360, 0x90 bytes each). A bank heading a group of banks has
// count > 0: the loops over BankInfo skip the rest of the group.
typedef struct BANK_INFO {
    int count;                          /* 0x00: > 0: banks in this group (this one included) */
    int groupCount;                     /* 0x04: added to the index (minus one) to skip the group */
    char name[16];                      /* 0x08: the bank's file ("GAME.BIN", "MENU.BIN", ...) */
    int type;                           /* 0x18: BANK_TYPE_* */
    u_char pad1c[4];
    int loaded;                         /* 0x20: non-zero while the bank is in memory */
    u_char pad24[4];
    u_int flags;                        /* 0x28: BANK_FLAG_* */
    u_char pad2c[0x64];
} BANK_INFO;
static_assert(sizeof(BANK_INFO) == 0x90, "BANK_INFO");
static_assert(offsetof(BANK_INFO, name) == 0x08, "BANK_INFO.name");
static_assert(offsetof(BANK_INFO, type) == 0x18, "BANK_INFO.type");
static_assert(offsetof(BANK_INFO, loaded) == 0x20, "BANK_INFO.loaded");
static_assert(offsetof(BANK_INFO, flags) == 0x28, "BANK_INFO.flags");

#define BANK_MAX 86                     /* 0x56: BankInfo[], the per-scene tables (FogCtrlAddr, ...) */
#define BANK_TYPE_SHADOW 1              /* main: banks of this type cast shadow polygons; scene_init:
                                           their fog colour clears the frame (a stage bank?) */
#define BANK_TYPE_0 0                   /* scene_init: takes the table's fog record, like type 1 */
#define BANK_SCENE_MAX 6                /* asCheckBankID: banks 0..5 have their own view matrix, the
                                           rest share bank 6's; GlobalDrawObject draws banks 0..5 */
#define BANK_FLAG_DRAW_FIRST 0x2        /* scene_draw_init: ARRW.BIN, "E_*" and "M<digit>*" banks;
                                           GlobalDrawObject draws them before the other banks
                                           (AllDrawObject's "trans" pass draws them after the rest) */
#define BANK_FLAG_NO_MIRROR 0x10        /* AllDrawObject: a character of this bank is not mirrored
                                           (name from that use) */

// A loaded scene's object tables (All_table_TBL[86], a record of the scene's bank): each list is a
// NULL-terminated array of draw objects. Only the fields main.c reads; fileio.c owns the layout.
// layout: host -- pointers (EE offsets in the comments); the file TU decides whether the bank's
// relocated words become host pointers or stay u_int EE addresses (open-items.md item 1).
typedef struct ALL_TABLE {
    u_char pad00[0x10];
    float (*partsPos)[4];               /* EE 0x10: per model part; main_rtn draws a part only while
                                           its w > PARTS_POS_HIDDEN_W; main_rtn_Particle2 starts from it */
    int baseobjCount;                   /* EE 0x14: scene_init -> BaseobjCount[bank] */
    void **objOpaque;                   /* EE 0x18: GlobalDrawObject's first main_rtn list */
    void **objTrans;                    /* EE 0x1c: main_rtn mode 1; scene_draw_init sorts it */
    float *light;                       /* EE 0x28: [2] (+8) scales a character's lit parts (main_rtn) */
    u_char *partsHeader;                /* EE 0x2c: the MODEL_PARTS header (scene_init -> pModelPartsHeader;
                                           InitMatrixTree, asBuildMirrorMatrixTree / _Back) */
    struct SOUND_OBJ_TABLE *sound;      /* EE 0x54: the scene's sound objects (sound.c's SSFindSoundObj) */
    void **particles;                   /* EE 0x58: main_rtn_Particle's list */
    void **objAfter;                    /* EE 0x60: after the particles; non-empty -> ExportDrawToTex */
    float *partsLight;                  /* EE 0x70: per model part: TEX_KIND_LIGHT parts' DRAW_OBJ_LIGHT */
    void **particles2;                  /* EE 0x78: main_rtn_Particle2 mode 0 */
    void **particles2After;             /* EE 0x7c: main_rtn_Particle2 mode 1; non-empty -> ExportDrawToTex */
    u_char *fog;                        /* EE 0xb8: FOG_INFO (scene_init -> FogCtrlAddr, bank types 0 / 1) */
    void *behavior;                     /* EE 0xbc: scene_init -> pBehaviorHeader[bank] */
} ALL_TABLE;
static_assert(offsetof(ALL_TABLE, partsPos) == 0x10, "ALL_TABLE.partsPos");

// A bank's sound objects (ALL_TABLE.sound): the sources the trigger actions and room-in start play.
// layout: ee -- bank data; loadSE and objs are relocated words (BankEePtr, open-items.md item 1 h).
typedef struct SOUND_OBJ_TABLE {
    u_int loadSE;                       /* 0x00: EE address of SE_LOAD_POINT[SE_LOAD_POINT_MAX], the SE
                                           banks SSLoadSEfromLoadPoint loads with the bank */
    u_int objs;                         /* 0x04: EE address of SOUND_OBJ[], ended by id 0 */
} SOUND_OBJ_TABLE;
// One row of a bank's SE load list (SOUND_OBJ_TABLE.loadSE). layout: ee.
typedef struct SE_LOAD_POINT {          /* 0x0c bytes */
    u_int name;                         /* 0x00: EE address of the SE bank's file name; 0 ends the list */
    u_char pad04[8];                    /* 0x04: not read by sound.c */
} SE_LOAD_POINT;
static_assert(sizeof(SE_LOAD_POINT) == 0xc, "SE_LOAD_POINT");
#define SE_LOAD_POINT_MAX 5             /* SSLoadSEfromLoadPoint's row bound */
#define BANK_ALL_TABLE_OFFSET 0x80      /* FSMltLoadProc: All_table_TBL[bank] = bank image + 0x80 */
static_assert(sizeof(SOUND_OBJ_TABLE) == 0x8, "SOUND_OBJ_TABLE");
static_assert(offsetof(SOUND_OBJ_TABLE, objs) == 0x4, "SOUND_OBJ_TABLE.objs");
typedef struct SOUND_OBJ {              /* 0x28 bytes */
    int id;                             /* 0x00: the trigger tables' sound object id; 0 ends the list */
    int seNo;                           /* 0x04: the C_0000 SE it plays */
    int autoStart;                      /* 0x08: != 0: plays when the room is entered */
    u_char pad0c[0x18];
    int positional;                     /* 0x24: != 0: SetPositionalParam_Alys pans it every frame */
} SOUND_OBJ;
static_assert(sizeof(SOUND_OBJ) == 0x28, "SOUND_OBJ");
static_assert(offsetof(SOUND_OBJ, positional) == 0x24, "SOUND_OBJ.positional");
#define SOUND_OBJ_MAX 1000              /* SSAutoStartSoundRoomIn's scan bound */

#define PARTS_POS_HIDDEN_W -255.0f      /* ALL_TABLE.partsPos[i][3] at or below: the part is not drawn */

// One model part of a bank's parts header (pModelPartsHeader[bank], 0x380 bytes each). main_rtn reads
// two fields; the rest belongs to the TU that loads the header.
typedef struct MODEL_PARTS {
    u_char pad000[0x42];
    u_char kind;                        /* 0x42: PARTS_KIND_NO_DOF: main_rtn_DOF skips the part */
    u_char pad043;
    int attr;                           /* 0x44: copied into the part's draw object (+0x3c) every draw */
    u_char pad048[8];
    u_int flags;                        /* 0x50: PARTS_FLAG_* */
    u_char pad054[0xdc];
    float pos[4];                       /* 0x130: the part's position (SSGetPositional: a sound source) */
    u_char pad140[0x240];
} MODEL_PARTS;
static_assert(sizeof(MODEL_PARTS) == 0x380, "MODEL_PARTS");
static_assert(offsetof(MODEL_PARTS, kind) == 0x42, "MODEL_PARTS.kind");
static_assert(offsetof(MODEL_PARTS, attr) == 0x44, "MODEL_PARTS.attr");
static_assert(offsetof(MODEL_PARTS, flags) == 0x50, "MODEL_PARTS.flags");
static_assert(offsetof(MODEL_PARTS, pos) == 0x130, "MODEL_PARTS.pos");

#define PARTS_FLAG_FOG 0x1              /* a fog volume: never drawn; the bank's fog part sets FOG_mode */
#define PARTS_KIND_NO_DOF 9             /* MODEL_PARTS.kind: not drawn in the depth-of-field slices */
#define PARTS_FLAG_ZONLY_FIRST 0x8      /* main_rtn: OneMaxObjectDrawZOnly runs before the part */

// A bank's fog record (FogCtrlAddr[bank]): the fog part, the clear colour, and gaSetupFogCtrlData's
// arguments for the bank's own objects (+0x10) and for characters lit in it (+0x28).
typedef struct FOG_INFO {
    int parts;                          /* 0x00: the fog part's index (MODEL_PARTS) */
    int clearColor[3];                  /* 0x04: R, G, B in the low bytes (main.c's FogClearColor) */
    float fog[3];                       /* 0x10: gaSetupFogCtrlData's floats */
    int fogArg[3];                      /* 0x1c: its ints */
    float charaFog[3];                  /* 0x28: the same for main_rtn's character pass */
    int charaFogArg[3];                 /* 0x34 */
} FOG_INFO;
static_assert(sizeof(FOG_INFO) == 0x40, "FOG_INFO");
static_assert(offsetof(FOG_INFO, charaFog) == 0x28, "FOG_INFO.charaFog");

// A texture of a draw list entry (DRAW_OBJ_ENTRY.tex). layout: host (tim is a pointer; EE offsets in
// the comments). tim's first two words are the texture's width and height.
typedef struct TEX_INFO {
    u_int *tim;                         /* EE 0x00: the texture; NULL = none */
    int tbuf[6];                        /* EE 0x04: sunSetupTBufferData's arguments */
    int pad1c[3];
    int kind;                           /* EE 0x28: TEX_KIND_* */
} TEX_INFO;

#define TEX_KIND_LIGHT 3                /* the part's DRAW_OBJ_LIGHT comes from ALL_TABLE.partsLight */
#define TEX_SMALL_MAX 32                /* both sides <= 32: the BGT ring restarts before the upload */

// One entry of a draw list (ALLobject[bank][i], NULL-terminated; objSort's tables). layout: host.
typedef struct DRAW_OBJ_ENTRY {
    u_char *obj;                        /* EE 0x00: the ga draw object (DRAW_OBJ_* offsets below) */
    struct GA_DRAWCTRL_ARGS *drawCtrlArgs; /* EE 0x04: the entry's draw control settings (scene_init sets
                                           drawCtrl from them); non-NULL and new: switch to drawCtrl */
    TEX_INFO *tex;                      /* EE 0x08 */
    int parts;                          /* EE 0x0c: MODEL_PARTS index */
    int dofMode;                        /* EE 0x10: ssDOF_ClipObjectFromZRange's mode (< 2: test the
                                           object's depth against the slice) */
    void *tbuffer;                      /* EE 0x14: the texture's upload packet */
    void *drawCtrl;                     /* EE 0x18: GA_DRAWCTRL_DATA */
} DRAW_OBJ_ENTRY;

// The ga draw object's bytes main_rtn touches (gaBGT_DrawObject's TU owns the layout).
#define DRAW_OBJ_FLAGS 0x00             /* u_char: DRAW_OBJ_FLAG_* */
#define DRAW_OBJ_CLIP 0x02              /* u_char: gaObjectClippingCheck's answer */
#define DRAW_OBJ_LIGHT 0x30             /* float */
#define DRAW_OBJ_POS 0x38               /* u_int: EE address of the object's position qword (BankEePtr) */
#define DRAW_OBJ_ATTR 0x3c              /* int: MODEL_PARTS.attr */
#define DRAW_OBJ_FLAG_CLIP 0x20         /* always drawn through the clipping path */

// A particle system's ring state (PARTICLE_ENTRY.state; the particle TU owns it). main_rtn_Particle
// draws the live slots head .. tail, wrapping at ringSize.
typedef struct PARTICLE_STATE {
    u_char pad00[0x46];
    u_short ringSize;                   /* 0x46 */
    u_short head;                       /* 0x48: the oldest live slot */
    u_short tail;                       /* 0x4a: one past the newest; head == tail: none live */
} PARTICLE_STATE;
static_assert(offsetof(PARTICLE_STATE, ringSize) == 0x46, "PARTICLE_STATE.ringSize");
static_assert(offsetof(PARTICLE_STATE, tail) == 0x4a, "PARTICLE_STATE.tail");

// One entry of a bank's particle list (ALL_TABLE.particles, NULL-terminated). layout: host.
typedef struct PARTICLE_ENTRY {
    int pad00;
    int drawCtrlId;                     /* EE 0x04: non-zero: the draw control to switch to */
    TEX_INFO *tex;                      /* EE 0x08 */
    PARTICLE_STATE *state;              /* EE 0x0c */
    u_char pad10[0x10];
    void *tbuffer;                      /* EE 0x20: the texture's upload packet */
    void *drawCtrl;                     /* EE 0x24: GA_DRAWCTRL_DATA */
} PARTICLE_ENTRY;

// One entry of a bank's second particle list (ALL_TABLE.particles2 / particles2After, NULL-terminated;
// main_rtn_Particle2). A ring of PARTICLE2 records, each drawn as one ga draw object placed at the
// entry's model part (ALL_TABLE.partsPos[parts]) plus the record's offset. layout: host (EE offsets in
// the comments).
typedef struct PARTICLE2 {
    u_char pad00[0x20];
    float offset[4];                    /* EE 0x20: added to the part's position; w scales its w
                                           around PARTS_POS_W_CENTER */
    u_char pad30[0x38];
    u_char *obj;                        /* EE 0x68: the ga draw object (DRAW_OBJ_* offsets) */
    u_char pad70[0x10];
} PARTICLE2;
static_assert(sizeof(PARTICLE2) == 0x80, "PARTICLE2");
static_assert(offsetof(PARTICLE2, offset) == 0x20, "PARTICLE2.offset");
static_assert(offsetof(PARTICLE2, obj) == 0x68, "PARTICLE2.obj");

typedef struct PARTICLE2_ENTRY {
    u_char pad00[0x40];
    u_short parts;                      /* EE 0x40: index into ALL_TABLE.partsPos */
    u_short pad42;
    u_short ringSize;                   /* EE 0x44 */
    u_short count;                      /* EE 0x46: live records; 0 = nothing to draw */
    u_short head;                       /* EE 0x48: the oldest live record */
    u_char pad4a[6];
    int drawCtrlId;                     /* EE 0x50: non-zero: the draw control to switch to */
    TEX_INFO *tex;                      /* EE 0x54: uploaded only for particles2 (not ...After) */
    int pad58;
    PARTICLE2 *ring;                    /* EE 0x5c */
    u_char pad60[0x14];
    void *tbuffer;                      /* EE 0x74: the texture's upload packet */
    void *drawCtrl;                     /* EE 0x78: GA_DRAWCTRL_DATA */
} PARTICLE2_ENTRY;
static_assert(offsetof(PARTICLE2_ENTRY, parts) == 0x40, "PARTICLE2_ENTRY.parts");
static_assert(offsetof(PARTICLE2_ENTRY, drawCtrlId) == 0x50, "PARTICLE2_ENTRY.drawCtrlId");

#define PARTS_POS_W_CENTER 128.0f       /* main_rtn_Particle2: w' = (w + 128) * scale - 128 */

// PORT (2026-11-09, open-items.md 1 h): a bank's relocated data (its ga draw objects first) keeps the
// EE layout, so a pointer word in it stays a u_int EE address -- a host pointer would overlap the next
// field (DRAW_OBJ_POS's word is followed by DRAW_OBJ_ATTR). The bank loader (fileio.c's TU) records each
// bank's host image and the EE address it was relocated to; BankEePtr turns such a word into a host
// pointer (RosePan's SprMan::eePtr is the precedent). Not ROM globals.
extern u_char *BankImage[BANK_MAX];             /* PORT: the bank's data on the host */
extern u_int BankImageEE[BANK_MAX];             /* PORT: the EE address its words were relocated to */
static inline void *BankEePtr(int bank, u_int ee)
{
    return ee ? BankImage[bank] + (int)(ee - BankImageEE[bank]) : NULL;
}

#define PARTICLE_OBJ_SIZE 0x80          /* ParticleObject[bank][i]: one ga draw object per ring slot */

#define OS_OBJ_TABLE_BYTES 2000         /* osTotalObjTable: one scene's sort table, 3 x 86 of them */

// A screen filter's fade (DemoFilterChangeWork, FilterChangeWork: 0x28 bytes). from[0] / to[0] are the
// alpha main sets to 0xff or 0, the next three the colour; timer counts down to 0.
typedef struct FILTER_CHANGE_WORK {
    short active;                       /* 0x00 */
    short mode;                         /* 0x02 */
    int timer;                          /* 0x04 */
    int from[4];                        /* 0x08 */
    int to[4];                          /* 0x18 */
} FILTER_CHANGE_WORK;
static_assert(sizeof(FILTER_CHANGE_WORK) == 0x28, "FILTER_CHANGE_WORK");
static_assert(offsetof(FILTER_CHANGE_WORK, timer) == 0x04, "FILTER_CHANGE_WORK.timer");
static_assert(offsetof(FILTER_CHANGE_WORK, to) == 0x18, "FILTER_CHANGE_WORK.to");

// One fog animation slot of FogAnimeBuff[86] (0x48 bytes): system_init clears its head.
typedef struct FOG_ANIME {
    short a;                            /* 0x00 */
    short b;                            /* 0x02 */
    u_char c;                           /* 0x04 */
    u_char d;                           /* 0x05 */
    u_char rest[0x42];
} FOG_ANIME;
static_assert(sizeof(FOG_ANIME) == 0x48, "FOG_ANIME");
