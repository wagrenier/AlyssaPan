// game/ssSetTexUVforCubeMap.h -- CT3 game/ssSetTexUVforCubeMap.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "ga/gaInitDrawObject.h"
#include "game/ct3_types.h"

// ---- part motion blur (2026-11-12) -------------------------------------------------------------------------
// One blurred part's record (PartBlurMaxObj[bank][i], 0-ended; 0xe4 bytes on the EE, ssPartMotionBlurStart
// allocates it from gHeapJunc). Each of its count PART_BLUR_OBJs (objs, 0x140 apart) keeps a ring of
// ringMax + 1 copies of one draw list entry; head..tail are the copies in use. layout: host -- the
// pointer words move on a 64-bit host; the EE offsets are in the comments.
#define PART_BLUR_MAX_OBJ_MAX 16        /* PartBlurMaxObj's slots per bank (0x40 bytes of EE words) */
#define PART_BLUR_MODE_TRAIL 0          /* mode: each frame the newest copy is pushed and the ring fades */
#define PART_BLUR_MODE_ONE_FRAME 1      /* every copy interpolated between last frame's and this frame's
                                           matrix (tiOneFrameMotionBlurUpdate) */
#define PART_BLUR_FADE 128.0f           /* the alpha the oldest copy has lost (ssPartMotionBlurUpdateAlpha) */
typedef struct PART_BLUR_MAX_OBJ {
    GA_DRAWCTRL_ARGS args;              /* 0x00: the entries' draw control before the blur (AllEnd restores it) */
    u_char pad78[8];
    float prevMatrix[4][4];             /* 0x80: the part's matrix last frame (ONE_FRAME) */
    float color[4];                     /* 0xc0: RGBA every copy draws with (the copies' DRAW_OBJ_POS) */
    float alphaPercent;                 /* 0xd0: ONE_FRAME: the copies' alpha, percent of the part's */
    PART_BLUR_OBJ *objs;                /* 0xd4: count blur objects */
    u_int ambPos;                       /* 0xd8: the entries' DRAW_OBJ_POS before the blur (an EE address of
                                           ALL_TABLE.partsPos: the part's RGBA) */
    u_char mode;                        /* 0xdc: PART_BLUR_MODE_* */
    u_char pad_dd;
    u_short parts;                      /* 0xde: the MODEL_PARTS record (ONE_FRAME reads its matrix) */
    u_char ringMax;                     /* 0xe0: the ring holds ringMax + 1 copies */
    u_char head;                        /* 0xe1: the oldest copy */
    u_char tail;                        /* 0xe2: where the next copy goes */
    u_char count;                       /* 0xe3: objs' length */
    const float *ambColor;              /* PORT: ambPos as a host pointer (BankEePtr; the record has no bank) */
    int bank;                           /* PORT: the bank whose parts it blurs (the copies' EE words: BankEeAddr) */
} PART_BLUR_MAX_OBJ;

// One copy in a PART_BLUR_OBJ's ring (PART_BLUR_COPY_SIZE bytes, EE layout: main_rtn draws it as a draw
// list entry). Every word in it is an EE address (BankEeAddr of the blurred part's bank, hazards.md).
#define BLUR_COPY_ENTRY 0x00            /* DRAW_OBJ_ENTRY: the blurred entry's, obj / drawCtrlArgs / drawCtrl
                                           redirected to the copy's object and the PART_BLUR_OBJ's draw control */
#define BLUR_COPY_OBJ 0x20              /* the entry's ga draw object's first BLUR_COPY_OBJ_SIZE bytes */
#define BLUR_COPY_OBJ_SIZE 0x80
#define BLUR_COPY_MAT58 0xc0            /* float[4][4]: the matrix the object's +0x58 word points at */
#define BLUR_COPY_MAT5C 0x100           /* float[4][4]: the matrix the object's +0x5c word points at */
// The EE head of a PART_BLUR_OBJ (ct3_types.h has no ga types): the entry's draw control settings with
// alpha blending and the alpha test on, and the GS data built from them, which every copy draws with.
#define PART_BLUR_OBJ_ARGS 0x00         /* GA_DRAWCTRL_ARGS */
#define PART_BLUR_OBJ_DRAWCTRL 0x80     /* GA_DRAWCTRL_DATA */
static_assert(offsetof(PART_BLUR_MAX_OBJ, prevMatrix) == 0x80, "PART_BLUR_MAX_OBJ.prevMatrix");
static_assert(offsetof(PART_BLUR_MAX_OBJ, color) == 0xc0, "PART_BLUR_MAX_OBJ.color");
static_assert(offsetof(PART_BLUR_MAX_OBJ, alphaPercent) == 0xd0, "PART_BLUR_MAX_OBJ.alphaPercent");

// ---- reflection maps (2026-11-12) --------------------------------------------------------------------------
// ALL_TABLE.reflects: a 0-ended list of EE addresses of REFLECT_ENTRY, one per reflective part. Each frame
// ReflectMapUpdate rewrites the texture coordinates of its meshes' packets from the eye: a plane map (one
// texture, the reflected vector's x / y) or a cube map (six faces in one texture, laid out as a cross of
// 0.25 x 0.2 cells). Every word below is an EE address in the bank (BankEePtr).
#define REFLECT_FLAG_PLANE 0x1          /* REFLECT_ENTRY.flags: a plane map (else a cube map) */
#define REFLECT_FLAG_ALWAYS 0x2         /* updated even when the part's pos is off screen */
typedef struct REFLECT_ENTRY {
    u_short part;                       /* 0x00: the MODEL_PARTS record (ssTrigCheckEnable's part too) */
    u_char count;                       /* 0x02: maps' length */
    u_char flags;                       /* 0x03: REFLECT_FLAG_* */
    u_int maps;                         /* 0x04: REFLECT_MAP[count] */
} REFLECT_ENTRY;
static_assert(sizeof(REFLECT_ENTRY) == 0x08, "REFLECT_ENTRY");

// One mesh of the entry and the rectangle of the texture its coordinates map into.
typedef struct REFLECT_MAP {
    u_int obj;                          /* 0x00: the ga draw object (REFLECT_OBJ_*, DRAW_OBJ_PACKET) */
    float u0, v0;                       /* 0x04: the rectangle's corner in the texture */
    float w, h;                         /* 0x0c: its size */
    u_int tris;                         /* 0x14: cube maps: REFLECT_TRI[triCount] */
    u_short triCount;                   /* 0x18 */
    u_char pad1a[2];
} REFLECT_MAP;
static_assert(sizeof(REFLECT_MAP) == 0x1c, "REFLECT_MAP");

// A cube-mapped triangle: its face is chosen once from the reflection at its centre.
typedef struct REFLECT_TRI {
    float center[4];                    /* 0x00 */
    float normal[4];                    /* 0x10 */
    u_short vert[3];                    /* 0x20: indices into the object's vertices */
    u_char pad26[0xa];
} REFLECT_TRI;
static_assert(sizeof(REFLECT_TRI) == 0x30, "REFLECT_TRI");

// The draw object words the reflection reads (DRAW_OBJ_PACKET holds the ST pairs: 8 bytes per vertex after
// the packet's first qword; the normals have a qword before them too).
#define REFLECT_OBJ_VERTS 0x48          /* int: the plane map's vertex count */
#define REFLECT_OBJ_POS 0x60            /* u_int: float[verts][4], the vertices in the part's frame */
#define REFLECT_OBJ_NORMAL 0x64         /* u_int: a qword, then float[verts][4] */
#define REFLECT_PACKET_ST 0x10          /* the ST pairs' offset in the packet (and the normals' in theirs) */

// The cube's faces (ssGetReflectFace: the reflected vector's longest axis and its sign).
#define CUBE_FACE_NEG_Z 0
#define CUBE_FACE_NEG_X 1
#define CUBE_FACE_POS_Y 2
#define CUBE_FACE_POS_X 3
#define CUBE_FACE_NEG_Y 4
#define CUBE_FACE_POS_Z 5
#define CUBE_FACE_MAX 6

#define PARTS_KIND_REFLECT_ROOT 1       /* MODEL_PARTS.kind: ReflectMapUpdate maps the part in part 0's frame */

void ssSetTexUVforCubeMap(float u, float v, float u0, float v0, float w, float h, float *st, int face); // CT3 0x018951d0
void ssCalcCubeMapUVfromFace(const float *r, float *u, float *v, int face);           // CT3 0x018952f0
int ssGetReflectFace(const float *r);                                                    // CT3 0x018954a0
int ssGetReflectVector(float *out, float *normal, float *incident);                      // CT3 0x018955c0
void ssCalcReflectPlaneMap(int bank, const REFLECT_ENTRY *ent, const float *eye);        // CT3 0x01895680
void ssCalcReflectCubeMap(int bank, const REFLECT_ENTRY *ent, const float *eye, float (*m)[4]); // CT3 0x01895820

extern PART_BLUR_MAX_OBJ *PartBlurMaxObj[BANK_MAX][PART_BLUR_MAX_OBJ_MAX]; /* bss 0x01a42280 */
extern u_char **ParticleData[BANK_MAX];         /* sbss 0x01940a10: per particle list entry, its records */
// ---- the 2D particles (the wave and vaseline passes' screen diamonds, 2026-11-12) ----------------------
// A ring of screen-space diamonds rising through a rectangle of the 512 x 448 screen: each record is a
// copy of the GS packet SS_draw_diamond (a triangle fan of 6 vertices about the centre, each RGBAQ, UV,
// XYZ2 in PACKED mode) the game rewrites in place, then its own parameters. The UVs are the vertices'
// screen positions shifted by a random (du, dv): the pass draws the frame onto itself through them, so
// each diamond refracts what lies behind it (the heat-wave look). PORT: the packet stays in the record
// as the ROM writes it; the host draw pass (ssBGT_WaveDrawDiamond's session) reads its vertices.
#define SS_DIAMOND_VERTS 6              /* SS_draw_diamond: centre, bottom, left, top, right, bottom */
#define SS_DIAMOND_PACKET_QW 21         /* DMA tag + VIF DIRECT, GIF tag, 6 x 3 registers */
#define SS_2DP_XYZ_OFFSET 0x800         /* XYZ2: (screen + 2048) * 16, the GS's offset */
#define SS_2DP_SUBPIXEL 16.0f           /* UV and XYZ2 carry 4 fraction bits */
#define SS_2DP_V_FLOOR 16.0f            /* a V under one texel is clamped (the diamond leaves the top) */
#define SS_2DP_V_CENTRE_TOP 0x20        /* ...to these: centre and sides 2.0, bottom 3.0, top 1.0 */
#define SS_2DP_V_BOTTOM_TOP 0x30
#define SS_2DP_V_TOP_TOP 0x10
#define SS_2DP_SPEED_MIN 10             /* the randomised speed's floor (tenths of a pixel per frame) */
#define SS_2DP_SPEED_MAX 0xff
#define SS_2DP_SIZE_MIN 10              /* the randomised size's range (pixels) */
#define SS_2DP_SIZE_MAX 100
#define SS_2DP_SWAY_RAND 200            /* du, dv = sway * (tnRand(200) - 100) / 1000 */
#define SS_2DP_SWAY_SCALE 0.001f
#define SS_2DP_NO_OBJECT 0xff           /* ssTrigWaveStart's part: the rectangle comes from the arguments */
#define SS_2DP_SCREEN_W 512             /* the ROM's screen the diamonds live in (ssGetScreenRange, vaseline) */
#define SS_2DP_SCREEN_H 448
#define SS_2DP_UV_EDGE 16.0f            /* ss2DParticleSetDrawData2: a UV within a texel of an edge is pushed */
#define SS_2DP_U_MAX 8176.0f            /* in to 1, 2, 3 texels from it; U up to texel 511, */
#define SS_2DP_V_MAX 7152.0f            /* V up to texel 447 */
#define SS_WAVE_OBJ_MATRIX 0x140       /* ssTrigWaveStart: the object's matrix is parts header + part * 0x380
                                           + 0x140 (MODEL_PARTS +0x100 by PARTS_REC; open-items.md) */

typedef struct SS_DIAMOND_VERTEX {      /* one vertex of the packet, 3 PACKED qwords */
    u_int rgbaq[4];                     /* 0x00: RGBAQ: 0x80 grey; alpha 0x40 at the centre, 0 outside */
    u_int u, v;                         /* 0x10: UV: the texel (screen position + du / dv) * 16 */
    u_int uv2, uv3;
    u_int x, y;                         /* 0x20: XYZ2: (screen position + 2048) * 16 */
    u_int z, xyz3;
} SS_DIAMOND_VERTEX;
static_assert(sizeof(SS_DIAMOND_VERTEX) == 0x30, "SS_DIAMOND_VERTEX");

// A record of ss2DpData (0x160 bytes, layout: ee -- the ROM's offsets, all 32-bit words and smaller).
typedef struct SS_2DP_REC {
    u_int head[12];                     /* 0x000: DMA tag, VIF DIRECT 19, GIF tag (NLOOP 6, EOP, PRIM fan) */
    SS_DIAMOND_VERTEX vtx[SS_DIAMOND_VERTS]; /* 0x030 */
    float du;                           /* 0x150: the U shift (sway) */
    float dv;                           /* 0x154: the V shift */
    short x;                            /* 0x158: the centre on screen */
    short y;                            /* 0x15a: from the rectangle's bottom, rising speed / 10 a frame */
    u_short age;                        /* 0x15c: frames lived; past ss2DpInfo.life it is popped */
    u_char speed;                       /* 0x15e: tenths of a pixel per frame */
    u_char size;                        /* 0x15f: the diamond's half-diagonal in pixels */
} SS_2DP_REC;
static_assert(sizeof(SS_2DP_REC) == 0x160, "SS_2DP_REC");
static_assert(offsetof(SS_2DP_REC, du) == 0x150, "SS_2DP_REC.du");
static_assert(offsetof(SS_2DP_REC, age) == 0x15c, "SS_2DP_REC.age");

// A wave object of a bank (ALL_TABLE.waves[part], layout: ee): a mesh whose screen extent bounds the
// diamonds. Pointer words are EE addresses in the bank (BankEePtr).
typedef struct SS_WAVE_OBJ {
    u_int pad00;
    u_int matrix;                       /* 0x04: float[4][4]; ssTrigWaveStart points it at the part's */
    u_int verts;                        /* 0x08: float[vertCount][4] */
    u_short part;                       /* 0x0c: the part whose matrix it takes */
    u_short vertCount;                  /* 0x0e */
} SS_WAVE_OBJ;
static_assert(sizeof(SS_WAVE_OBJ) == 0x10, "SS_WAVE_OBJ");

// ss2DpInfo (data 0x01939c90, 36 bytes on the EE; layout: host -- obj is a host pointer, bank is PORT).
typedef struct SS_2DP_INFO {
    short x, y, w, h;                   /* EE 0x00: the rectangle (ssGetScreenRange writes it) */
    SS_WAVE_OBJ *obj;                   /* EE 0x08: the wave object, NULL = a fixed rectangle */
    float sizeScale;                    /* EE 0x0c: size * the object's depth at the start: size = this / depth */
    int frame;                          /* EE 0x10: frames since the start (the emission clock) */
    u_short max;                        /* EE 0x14: the ring's records */
    u_short head;                       /* EE 0x16: the oldest */
    u_short tail;                       /* EE 0x18: the next free */
    u_short life;                       /* EE 0x1a: frames a diamond lives: h * 10 / speed */
    u_char speed;                       /* EE 0x1c */
    u_char speedRand;                   /* EE 0x1d: +-percent */
    u_char size;                        /* EE 0x1e */
    u_char sizeRand;                    /* EE 0x1f: +-percent */
    u_char sway;                        /* EE 0x20: the UV shift's range, thousandths of a pixel per step */
    int bank;                           /* PORT: the bank obj's EE words resolve in */
} SS_2DP_INFO;

// A rectangle of the 512 x 448 screen as ssVaselineGetScreenPos takes it (8 bytes in one 64-bit register,
// read as unsigned halves) and returns its point in (x, y); w and h of the result are 0 (layout: ee).
typedef struct SS_SCREEN_RECT {
    u_short x, y, w, h;
} SS_SCREEN_RECT;
static_assert(sizeof(SS_SCREEN_RECT) == 8, "SS_SCREEN_RECT");

extern SS_2DP_INFO ss2DpInfo;                   /* data 0x01939c90 */
extern SS_2DP_REC *ss2DpData;                   /* 0x0193f778: the 2D particles' records (ssTrigVaselineStart, _WaveStart) */
extern u_int SS_draw_diamond[SS_DIAMOND_PACKET_QW * 4]; /* data 0x01938de0: the record's packet template */

// ---- the screen passes' sprite (SS_draw_sprite, 2026-11-12) ----
// One GS sprite in REGLIST mode (PRIM, RGBAQ, UV, XYZ2, UV, XYZ2) the BGT screen passes (hazy, ultima, blur,
// DOF, HQDOF write-back) fill through ssSetSpriteCol / _UV / _Pos before they send it. layout: ee; the
// 64-bit registers are kept as (lo, hi) word pairs. PORT: the host draw passes read it as the ROM's GS
// would (the passes' sessions).
#define SS_SPRITE_UV_CENTRE 0xf         /* ssSetSpriteUV: texel * 16 + 15 */
#define SS_SPRITE_X_BIAS 0.45           /* ssSetSpritePos: (x + 0.45) * 16, (y + 0.4) * 16 (doubles) */
#define SS_SPRITE_Y_BIAS 0.4
#define SS_SPRITE_XY_OFFSET 0x8000      /* + 2048 * 16, the GS offset */
#define SS_SPRITE_Q_ONE 0x3f800000u     /* RGBAQ's Q: 1.0f */
#define SS_SPRITE_Z_MASK 0xffffff
typedef struct SS_SPRITE_PACKET {
    u_int head[12];                     /* 0x00: DMA tag (5 qwords), VIF DIRECT 4, GIF tag (NLOOP 1, EOP, REGLIST 6) */
    u_int prim[2];                      /* 0x30: PRIM 6 (sprite) */
    u_int rgbaq[2];                     /* 0x38 */
    u_int uv0[2];                       /* 0x40 */
    u_int xyz0[2];                      /* 0x48 */
    u_int uv1[2];                       /* 0x50 */
    u_int xyz1[2];                      /* 0x58 */
} SS_SPRITE_PACKET;
static_assert(sizeof(SS_SPRITE_PACKET) == 0x60, "SS_SPRITE_PACKET");
extern SS_SPRITE_PACKET SS_draw_sprite;         /* data 0x01938d80 */
void ssSetSpriteCol(int r, int g, int b, int a);                                          // CT3 0x0189dfc0
void ssSetSpriteUV(int u0, int v0, int u1, int v1);                                       // CT3 0x0189df70
void ssSetSpritePos(float x0, float y0, float x1, float y1, float z);                     // CT3 0x0189e010

// The rectangle (x, y, w, h) the object's vertices cover on the 512 x 448 screen, into info; returns the
// depth (view z) of its first vertex.
float ssGetScreenRange(SS_WAVE_OBJ *obj, SS_2DP_INFO *info);                              // CT3 0x0189cff0
void ss2DParticleSetDrawData(int i);                                                      // CT3 0x0189c150
void ssCalc2DParticleOrbit(void);                                                         // CT3 0x0189c4b0
SS_2DP_REC *ss2DParticlePop_Front(SS_2DP_REC *data);                                      // CT3 0x0189c6b0
int ss2DParticleLifeCheck(u_short i);                                                     // CT3 0x0189c700
void ssCalc2DParticleLifeSpan(void);                                                      // CT3 0x0189c7a0
int ss2DParticlePush_Back(SS_2DP_REC *data);                                              // CT3 0x0189c7f0
void ssGenerate2DParticle(void);                                                          // CT3 0x0189cc10

int ssGetNumBlurDrawObj(const PART_BLUR_MAX_OBJ *max);                                   // CT3 0x018968d0
void ssPartMotionBlurUpdateAlpha(PART_BLUR_MAX_OBJ *max);                                // CT3 0x018965e0
void ssPartMotionBlurPush_Back(PART_BLUR_MAX_OBJ *max);                                  // CT3 0x01896720
void tiOneFrameMotionBlurUpdate(int bank, PART_BLUR_MAX_OBJ *max);                       // CT3 0x01897430
// r, g, b, a: the new ambient of every part of the bank; -1 keeps that channel.
void ssSetAmbientColor(float r, float g, float b, float a, int bank);                    // CT3 0x01896530
int ssGetNumParticleEmit(int bank);                                                      // CT3 0x0189a8d0

void ReflectMapUpdate(void);                                                             // CT3 0x01895c10
// set: {chara, bank, 0, 0} (ExecuteWarpObject's call: the character takes the room bank's ambient).
void tnChangeCharAmbientColorSet(short *set);                                            // CT3 0x018962a0
void ssChangeCharAmbientColor(void);                                                     // CT3 0x01895fb0
void ssPartMotionBlurUpdate(void);                                                       // CT3 0x01896d50
void ssPartMotionBlurAllEnd(int scene);                                                  // CT3 0x01896f00
void ssPartMotionBlurGlobalInit(void);                                                   // CT3 0x01897e30
void ssParticleDeInit(int scene);                                                        // CT3 0x0189a920
void ssParticle_main(void);                                                              // CT3 0x0189ae30
void ssBGT_VaselineDrawDiamond(void);                                                    // CT3 0x0189b7e0
void ssBGT_VaselineSetDrawEnvPostRend(void);                                             // CT3 0x0189b840
void ssBGT_WaveDrawDiamond(void);                                                        // CT3 0x0189bff0
void ssBGT_WaveSetDrawEnvPostRend(void);                                                 // CT3 0x0189c070
void ss2DParticle_main(void);                                                            // CT3 0x0189cd60
void ssBGT_HazyDrawSprite(int level, int transTime);                                     // CT3 0x0189d330
void ssBGT_HazySetDrawEnvPostRend(void);                                                 // CT3 0x0189d560
void ssBGT_UltimaDrawSprite(int *frame);                                                 // CT3 0x0189d640
void ssBGT_UltimaSetDrawEnvPostRend(void);                                               // CT3 0x0189d820
void ssTrigScreenBlurEnd(void);                                                          // CT3 0x0189d900
void ssBGT_BlurDrawSprite(int level);                                                    // CT3 0x0189d940
void ssBGT_BlurSetDrawEnvPostRend(void);                                                 // CT3 0x0189d9b0
void ssBGT_DOFDrawSprite(int pass, int level);                                           // CT3 0x0189db60
void ssBGT_DOFSetDrawEnvPostRend(int pass, int level);                                   // CT3 0x0189dc40
void ssBGT_DOFSetDrawEnvPreRend(int pass, int level);                                    // CT3 0x0189dd70
// 1: the object is drawn in this depth-of-field slice. PORT: bank resolves the object's matrix word (+0x58).
int ssDOF_ClipObjectFromZRange(u_char *obj, u_int mode, int bank);                       // CT3 0x0189da90
int ssGetNumBlurDrawObjForScene(int scene, int mode);                                    // CT3 0x01896920
void ssBGT_ZBufferClear(void);                                                           // CT3 0x0189dec0
void ssPartMotionBlurInsertObjToDrawTable(int bank, void *table, u_int *list, int count, int mode); // CT3 0x01896a90
void ssBGT_HQDOF_WriteBackFB(int alpha);                                                 // CT3 0x0189b390
void ssBGT_HQDOF_SetDrawEnvPreRend(void);                                                // CT3 0x0189b510
void ssSetAmbColorToAllObj(int scene);                                                   // CT3 0x01895f00
void ssParticleInit(int scene);                                                          // CT3 0x0189a9d0

// The screen effects the trigger commands start and end (ssTrigExecuteActCmd, 2026-11-11): the arguments in
// the order the command reads them; the ROM call sites give the counts (Wave: 12, the last four on the stack).
void ssTrigParticleStart(int particle, int bank, int prerollTenths);                     // CT3 0x018988b0
void ssTrigParticleStop(int particle, int bank);                                         // CT3 0x018988a0
void ssTrigParticleAllOffImmediately(int particle, int bank);                            // CT3 0x01898710
// ringMax: the ring keeps ringMax + 1 copies; mode PART_BLUR_MODE_*; alphaPercent: ONE_FRAME's copies' alpha.
void ssPartMotionBlurStart(int part, int ringMax, int bank, int mode, int alphaPercent); // CT3 0x018976d0
void ssPartMotionBlurEnd(int part, int bank);                                            // CT3 0x01897140
void ssTrigScreenBlurStart(int level);                                                   // CT3 0x0189d920
void ssTrigHazyScreenStart(int level, int time);                                         // CT3 0x0189d310
void ssTrigHazyScreenEnd(void);                                                          // CT3 0x0189d2f0
// x, y, w, h: the rectangle when part is SS_2DP_NO_OBJECT, else the bounds of ALL_TABLE.waves[part]; size,
// speed (tenths of a pixel a frame) and their +-percent; sway: the UV shift; count: the ring's records.
void ssTrigWaveStart(int x, int y, int w, int h, int size, int sizeRand, int speed, int speedRand,
                     int sway, int count, int bank, int part);                           // CT3 0x0189ce60
void ssTrigWaveEnd(void);                                                                // CT3 0x0189ce30
// x, y, w, h: the rectangle; size and its +-percent; sway: the UV shift; count: the diamonds; inside: 1
// scatters them inside the rectangle, 0 over the rest of the screen.
void ssTrigVaselineStart(int x, int y, int w, int h, int size, int sizeRand, int sway, int count,
                         int inside);                                                    // CT3 0x0189bd40
SS_SCREEN_RECT ssVaselineGetScreenPos(SS_SCREEN_RECT range, int inside);                 // CT3 0x0189b640
void ss2DParticleSetDrawData2(SS_DIAMOND_VERTEX *vtx, int x, int y, int size, float du, float dv); // CT3 0x0189b950
void ssTrigVaselineEnd(void);                                                            // CT3 0x0189b920
void ssTrigHQDOFStart(int cameraOffset, int opacity);                                    // CT3 0x0189b5a0
void ssTrigHQDOFEnd(void);                                                               // CT3 0x0189b580

// The particle curve readers (from tiCalc_Orbit, 2026-11-11; argument order from the disassembly). age: the
// record's PARTICLE2.age; curve: the entry's curve (host pointer: the ROM passes the bank's EE word);
// cache: the record's cache word for that curve. ssParticleCashClear runs before each pass over a ring.
void ssParticleCashClear(void);                                                          // CT3 0x01899070
void tiParticleGetSizeValue(int age, float *scale, void *curve, short *cache);           // CT3 0x01898a60
void tiParticleGetRefractive_IndexValue(int age, float *index, void *curve, short *cache); // CT3 0x01898980
// r, g, b: signed bytes (the ROM reads them back with lb).
void ssParticleGetVRGBValue(int age, s_char *r, s_char *g, s_char *b, void *curve, short *cache); // CT3 0x01898bc0
// alpha: percent (lh; tiCalc_Orbit divides by 100).
void ssParticleGetAlphaValue(int age, short *alpha, void *curve, short *cache);          // CT3 0x01898e60
// u, v: floats, the draw object's +0x78 / +0x7c (DRAW_OBJ_UV_U / _V).
void ssParticleGetUVValue(int age, void *u, void *v, void *curve, short *cache);         // CT3 0x01898d20
// w, h: the 2D size curve's answer (ssParticleSetSize scales the record's base size by it).
void ssParticleGetSizeValue(int age, float *w, float *h, void *curve, short *cache);     // CT3 0x01898f30

// The first particle kind's ring records (PARTICLE_ENTRY.data -> ParticleData[bank][i]): 0xc0 bytes per
// slot beside the slot's draw object (PARTICLE_OBJ_SIZE). Only the words read so far are named.
#define PARTICLE_REC_SIZE 0xc0
#define PARTICLE_REC_BORN 0xa8          /* int: the tick the slot was emitted (ages are now - born) */
#define PARTICLE_REC_SIZE_CACHE 0xb8    /* short: the curve readers' cache words (ssParticleSet*) */
#define PARTICLE_REC_ALPHA_CACHE 0xba
#define PARTICLE_REC_UV_CACHE 0xbc
#define PARTICLE_REC_VRGB_CACHE 0xbe

u_char *ssParticlePop_Front(u_char *objs, PARTICLE_STATE *st);                           // CT3 0x01897fd0
int ssParticleLifeCheck(const PARTICLE_STATE *st, u_short slot, const u_char *data, int now); // CT3 0x0189a510
// PORT: bank resolves the entry's EE state word.
void ssCalcParticleLifeSpan(const PARTICLE_ENTRY *ent, u_char *objs, const u_char *data, int now,
                            int bank);                                                   // CT3 0x0189a5b0
// The per-slot curves (curve: the entry's curve; obj: the slot's draw object; rec: its record; now: the
// pass's tick). PORT: bank resolves the object's EE colour word.
void ssParticleSetVRGB(void *curve, u_char *obj, u_char *rec, int now, int bank);       // CT3 0x018990a0
void ssParticleSetUV(void *curve, u_char *obj, u_char *rec, int now);                   // CT3 0x01899150
void ssParticleSetAlpha(void *curve, u_char *obj, u_char *rec, const s_char *src, int now,
                        int bank);                                                      // CT3 0x018991a0
void ssParticleSetSize(void *curve, u_char *rec, int now);                              // CT3 0x018992c0

// The emitter of a mesh-shaped system (PARTICLE_STATE.emitter): words read so far.
#define PARTICLE_EMITTER_VERTS 0x08     /* u_int: EE address of the mesh's vertices */
#define PARTICLE_EMITTER_PART 0x0c      /* u_short: the part whose matrix200 ssParticleInit writes to word 0 */
#define PARTICLE_EMITTER_VERT_COUNT 0x0e /* u_short: the mesh's vertex count */
#define PARTICLE_EMIT_STEP 5.0f         /* ssGenerateParticle: emitRate is per 5 ticks (1/60 s) */
#define PARTICLE_PREROLL_STEP 5         /* ssParticle_main: the run-ahead's ticks per pass */

// One emission pass of a system (ssParticle_main, per entry and pass): enabled = the trigger part is on.
// PORT: bank resolves the entry's EE words (every ss particle function below takes it).
void ssGenerateParticle(const PARTICLE_ENTRY *ent, u_char *objs, u_char *data, int enabled, int now,
                        int bank);                                                       // CT3 0x0189a750
// A new slot at the ring's tail (0 when the ring is full).
int ssParticlePush_Back(u_char *objs, u_char *data, u_int obj, PARTICLE_STATE *st,
                        const PARTICLE_ENTRY *ent, int now, int bank);                   // CT3 0x018983a0
// A new slot at mesh vertex vert (the emitter's vertices through its matrix).
int ssMeshParticlePush_Back(u_char *objs, u_char *data, u_int obj, PARTICLE_STATE *st,
                            const PARTICLE_ENTRY *ent, u_int vert, int now, int bank);  // CT3 0x01898020
// The next mesh vertex to emit from (vert: the cursor) and its slot (0 when the ring is full).
int ssMeshParticleGetNextVertID2(u_int *vert, u_short count, u_int verts, u_char *objs, u_char *data,
                                 const PARTICLE_ENTRY *ent, PARTICLE_STATE *st, int now,
                                 int bank);                                              // CT3 0x0189a640
void ssCalcParticleOrbit(const PARTICLE_ENTRY *ent, u_char *objs, u_char *data, int now,
                         int bank);                                                      // CT3 0x018998e0
// The motions of one live slot (obj its draw object, rec its record): dt from the record's last move.
void ssParticleSetPosition(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now); // CT3 0x018997f0
void tiParticleSetPositionGoalDisable(u_char *obj, u_char *rec, int now);               // CT3 0x018994e0
void tiParticleSetPositionToGoal(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now,
                                 int bank);                                             // CT3 0x018996c0
void tiParticleSetPositionToGoal_Stop(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now,
                                      int bank);                                        // CT3 0x01899550
void tiParticleSetPosition_Twist(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now); // CT3 0x01899320

// The part kinds the trigger command's particle all-off tells apart (MODEL_PARTS.kind).
#define PARTS_KIND_PARTICLE 0x15        /* an ALL_TABLE.particles system's trigger part */
#define PARTS_KIND_PARTICLE2 0x18       /* an ALL_TABLE.particles2 / particles2After system's */

// The copies of a blur object as draw list words (newest first). PORT: bank gives the copies (gHeapJunc)
// a virtual EE address BANK_PTR maps back.
int ssAddBlurObjToDrawTableInOrder(u_int *out, const PART_BLUR_OBJ *obj, int bank);     // CT3 0x018969f0
#define PART_BLUR_COPY_SIZE 0x140       /* one copy in PART_BLUR_OBJ.copies */
