// game/LinkMatrixTree.c -- Clock Tower 3: a bank's matrix tree set-up (InitMatrixTree, LinkMatrixTree) and
// the game clock (ahTimeInit, ahTimeMaintenance). CodeWarrior C, symtab local chunk 70 (2026-11-10).

#include "game/LinkMatrixTree.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "game/unassigned_data.h"
#include "ga/gaInitInterrupt.h"

#include <stdio.h>
#include <string.h>

// ---- the game clock ----
//
// Ticks run at 5 per VSync (300 a second). The scene clock (gNext*, gTotalTicks) stops while the game is
// paused; the global clock (gGlobal*) always runs. gNextTicksInterval is this frame's step.

int gGlobalNextTicksInterval;                   /* sbss 0x0193cb08 */
int gGlobalNextTicks;                           /* sbss 0x0193cb0c */
int gGlobalTotalTicks;                          /* sbss 0x0193cb10 */
int gNextTicksInterval;                         /* sbss 0x0193cb18 */
int gNextTicks;                                 /* sbss 0x0193cb1c */
int gTotalTicks;                                /* sbss 0x0193cb20 */
static int Globallast_frame;                    /* sbss 0x0193cb04: ga_main_timer at the last call */
static int last_frame;                          /* sbss 0x0193cb14 */

// CT3 0x018517d0: once a frame (main): advance the clocks by the VSyncs since the last call; paused != 0
// holds the scene clock (it only moves its reference frame).
void ahTimeMaintenance(int paused)
{
    int now;

    if (paused) {
        last_frame = ga_main_timer;
    } else {
        now = ga_main_timer;
        gNextTicksInterval = (now - last_frame) * TICKS_PER_VSYNC;
        last_frame = now;
        gNextTicks += gNextTicksInterval;
        gTotalTicks += gNextTicksInterval;
    }
    now = ga_main_timer;
    gGlobalNextTicksInterval = (now - Globallast_frame) * TICKS_PER_VSYNC;
    Globallast_frame = now;
    gGlobalNextTicks += gGlobalNextTicksInterval;
    gGlobalTotalTicks += gGlobalNextTicksInterval;
}

// CT3 0x01851880: main, at boot. The two clocks are not set alike: the ROM sets gGlobalNextTicks (not
// its interval) to 5 and takes 5 off gGlobalNextTicksInterval -- kept (the next ahTimeMaintenance
// overwrites both intervals).
void ahTimeInit(void)
{
    gNextTicksInterval = TICKS_PER_VSYNC;
    gGlobalNextTicks = TICKS_PER_VSYNC;
    gTotalTicks = -TICKS_PER_VSYNC;
    gGlobalTotalTicks = -TICKS_PER_VSYNC;
    gNextTicks = 0;
    last_frame = ga_main_timer;
    gGlobalNextTicksInterval -= TICKS_PER_VSYNC;
    Globallast_frame = ga_main_timer;
}

// ---- the matrix tree ----
//
// A parts header holds its records from +0x40, 0x380 bytes each (MODEL_PARTS is the same record seen
// from the header: its offsets are these + 0x40), ended by a negative depth. The record offsets below are
// the ones BuildMatrixTree names (game/ahRelocateSunPs2ObjectArry.c); the *_INIT ones are the file's
// values InitMatrixTree copies into the working fields.

#define PARTS_RECORDS 0x40              /* the first record's offset in the header */
#define PARTS_REC_SIZE 0x380
#define REC_DEPTH 0x0                   /* short: depth in the tree, < 0 ends the records */
#define REC_KIND 0x2                    /* char: PARTS_KIND_* */
#define REC_OFFSET_INIT 0x40            /* float[4][4]: the file's REC_OFFSET; rows 0..2's w hold
                                           REC_PREV_SCALE_X/Y/Z, so they copy as 0 */
#define REC_PREV_SCALE_X 0x4c
#define REC_PREV_SCALE_Y 0x5c
#define REC_PREV_SCALE_Z 0x6c
#define REC_BLEND 0x7c                  /* float: InitMatrixTree sets 1 (world = local * REC_OFFSET) */
#define REC_BASE_INIT 0x80              /* float[4][4]: the file's REC_BASE; row 0's w is REC_PARENT */
#define REC_PARENT 0x8c                 /* u_int: EE address of the parent's matrix (LinkMatrixTree) */
#define REC_SCALE_INIT 0xc0             /* float[4]: the file's REC_SCALE (not for PARTS_KIND_KEEP_SCALE) */
#define REC_2D0_INIT 0xd0               /* float[4]: the file's REC_2D0 */
#define REC_GA_MATRIX 0x100             /* float[4][4]: the part's draw object's first matrix (DRAW_OBJ_MATRIX) */
#define REC_GA_MATRIX2 0x140            /* float[4][4]: its second (DRAW_OBJ_MATRIX2) */
#define REC_MATRIX 0x180                /* float[4][4]: the world matrix (MODEL_PARTS.matrix) */
#define REC_MATRIX_PREV 0x1c0           /* float[4][4]: InitMatrixTree sets the unit matrix */
#define REC_LOCAL 0x200                 /* float[4][4]: parent * REC_BASE */
#define REC_OFFSET 0x240
#define REC_BASE 0x280
#define REC_SCALE 0x2c0
#define REC_2D0 0x2d0                   /* float[4]: unread so far */
#define REC_VIEW_HOLD 0x34c             /* float: != 0 keeps the view matrices; LinkMatrixTree sets 1, then
                                           0 for a drawn part and for the kinds below */

// The ga draw object's words LinkMatrixTree fills (EE addresses into the parts header).
#define DRAW_OBJ_MATRIX 0x58            /* u_int: the part's REC_GA_MATRIX */
#define DRAW_OBJ_MATRIX2 0x5c           /* u_int: the part's REC_GA_MATRIX2 */

#define PARTS_KIND_STATIC 1             /* hangs from the nearest non-static ancestor's REC_MATRIX and
                                           draws through its parent's draw matrices */
#define PARTS_KIND_KEEP_SCALE 3         /* InitMatrixTree leaves REC_SCALE as it is */
#define PARTS_KIND_OMNI 4
#define PARTS_KIND_DIRECTIONAL 5
#define PARTS_KIND_0D 0xd               /* unnamed kinds that, like lights and look-at parts, never hold */
#define PARTS_KIND_11 0x11
#define PARTS_KIND_WATCH 0x14
#define PARTS_KIND_15 0x15
#define PARTS_KIND_16 0x16

#define PARTS_DEPTH_MAX 32              /* LinkMatrixTree's parent stacks (0x80 bytes on the ROM's stack) */
#define DRAW_LISTS 3                    /* LinkMatrixTree's object lists, each ended by 0 */

#define REC(header, i) ((header) + PARTS_RECORDS + (i) * PARTS_REC_SIZE)
#define RW(rec, off) (*(u_int *)((rec) + (off)))
#define RF(rec, off) (*(float *)((rec) + (off)))

// PORT: the bank whose parts header this is (the ROM's words are addresses and need none). Every
// caller passes pModelPartsHeader[bank]; -1 when no bank has it.
static int PartsHeaderBank(const u_char *header)
{
    int bank;

    for (bank = 0; bank < BANK_MAX; bank++) {
        if (pModelPartsHeader[bank] == header) {
            return bank;
        }
    }
    return -1;
}

// CT3 0x018514d0: point every record's REC_PARENT at its parent's matrix, and every draw object of the
// bank's three lists at its part's draw matrices. A non-static part's parent is its parent's REC_LOCAL
// (matUnit for a root); a static part's is the REC_MATRIX of the nearest non-static part above it. A
// static part draws through its parent's matrices.
void LinkMatrixTree(u_int *objList, u_char *partsHeader)
{
    u_int local[PARTS_DEPTH_MAX];       /* local[d + 1]: EE address of the depth-d part's REC_LOCAL */
    u_int world[PARTS_DEPTH_MAX + 16];  /* world[d + 1]: its REC_MATRIX (static parts leave it) */
    int bank = PartsHeaderBank(partsHeader);
    u_char *rec;
    int list;
    short depth;

    if (bank < 0) {
        // PORT: no bank owns the header, so its words cannot become EE addresses. Should not happen.
        fprintf(stderr, "[LinkMatrixTree] parts header %p belongs to no bank\n", (void *)partsHeader);
        return;
    }
    local[0] = MATUNIT_EE;
    world[0] = MATUNIT_EE;
    for (rec = REC(partsHeader, 0); (depth = *(short *)(rec + REC_DEPTH)) >= 0; rec += PARTS_REC_SIZE) {
        RF(rec, REC_VIEW_HOLD) = 1.0f;
        local[depth + 1] = BankEeAddr(bank, rec + REC_LOCAL);
        if ((signed char)rec[REC_KIND] == PARTS_KIND_STATIC) {
            RW(rec, REC_PARENT) = world[depth];
        } else {
            world[depth + 1] = BankEeAddr(bank, rec + REC_MATRIX);
            RW(rec, REC_PARENT) = local[depth];
        }
    }

    for (list = 0; list < DRAW_LISTS; list++) {
        for (; *objList != 0; objList++) {
            DRAW_OBJ_ENTRY *entry = BANK_PTR(DRAW_OBJ_ENTRY *, bank, *objList);
            u_char *obj = BANK_PTR(u_char *, bank, entry->obj);
            int part = entry->parts;
            if ((signed char)REC(partsHeader, part)[REC_KIND] == PARTS_KIND_STATIC) {
                short parentDepth = *(short *)(REC(partsHeader, part) + REC_DEPTH) - 1;
                while (*(short *)(REC(partsHeader, part) + REC_DEPTH) != parentDepth) {
                    part--;
                }
            }
            rec = REC(partsHeader, part);
            *(u_int *)(obj + DRAW_OBJ_MATRIX) = BankEeAddr(bank, rec + REC_GA_MATRIX);
            *(u_int *)(obj + DRAW_OBJ_MATRIX2) = BankEeAddr(bank, rec + REC_GA_MATRIX2);
            RF(rec, REC_VIEW_HOLD) = 0.0f;
        }
        objList++;                      /* past the list's 0 */
    }

    for (rec = REC(partsHeader, 0); *(short *)(rec + REC_DEPTH) >= 0; rec += PARTS_REC_SIZE) {
        switch ((signed char)rec[REC_KIND]) {
        case PARTS_KIND_16:
        case PARTS_KIND_15:
        case PARTS_KIND_WATCH:
        case PARTS_KIND_11:
        case PARTS_KIND_0D:
        case PARTS_KIND_DIRECTIONAL:
        case PARTS_KIND_OMNI:
            RF(rec, REC_VIEW_HOLD) = 0.0f;
            break;
        }
    }
}

// CT3 0x018516a0: reset every record's working matrices from the file's values (REC_OFFSET, REC_BASE,
// REC_SCALE, REC_2D0; REC_MATRIX_PREV the unit matrix, REC_BLEND 1), then asCopyDoubleBufferL2V(.., 1).
// The w words REC_PREV_SCALE_* and REC_PARENT live inside the copied matrices: they copy as 0 and keep
// their own values.
void InitMatrixTree(u_char *partsHeader)
{
    static const float unit[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    u_char *rec;

    for (rec = REC(partsHeader, 0); *(short *)(rec + REC_DEPTH) >= 0; rec += PARTS_REC_SIZE) {
        u_int scaleX = RW(rec, REC_PREV_SCALE_X);
        u_int scaleY = RW(rec, REC_PREV_SCALE_Y);
        u_int scaleZ = RW(rec, REC_PREV_SCALE_Z);
        u_int parent = RW(rec, REC_PARENT);

        RW(rec, REC_PREV_SCALE_X) = 0;
        RW(rec, REC_PREV_SCALE_Y) = 0;
        RW(rec, REC_PREV_SCALE_Z) = 0;
        RW(rec, REC_PARENT) = 0;
        memcpy(rec + REC_BASE, rec + REC_BASE_INIT, 0x40);
        memcpy(rec + REC_OFFSET, rec + REC_OFFSET_INIT, 0x40);
        if ((signed char)rec[REC_KIND] != PARTS_KIND_KEEP_SCALE) {
            memcpy(rec + REC_SCALE, rec + REC_SCALE_INIT, 0x10);
        }
        memcpy(rec + REC_2D0, rec + REC_2D0_INIT, 0x10);
        RW(rec, REC_PREV_SCALE_X) = scaleX;
        RW(rec, REC_PREV_SCALE_Y) = scaleY;
        RW(rec, REC_PREV_SCALE_Z) = scaleZ;
        RF(rec, REC_BLEND) = 1.0f;
        RW(rec, REC_PARENT) = parent;
        memcpy(rec + REC_MATRIX_PREV, unit, sizeof(unit));
    }
    asCopyDoubleBufferL2V(partsHeader, 1);
}
