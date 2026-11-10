// ga/gaInitDrawObject.h -- CT3 ga/gaInitDrawObject.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// The ga library's draw packets (sizes from the symtab: gsinit 0x70, drawbuffer_* 0x80,
// drawctrl_* 0x90, drawcom_* 0x50). Their insides are read with gaInitDrawObject.c's TU; main.c only
// writes drawcom's clear colour (RGBAQ at +0x38).
typedef struct GA_GSINIT_DATA {
    u_char data[0x70];
} GA_GSINIT_DATA;
static_assert(sizeof(GA_GSINIT_DATA) == 0x70, "GA_GSINIT_DATA");

typedef struct GA_DBUFFER_DATA {
    u_char data[0x80];
} GA_DBUFFER_DATA;
static_assert(sizeof(GA_DBUFFER_DATA) == 0x80, "GA_DBUFFER_DATA");

typedef struct GA_DRAWCTRL_DATA {
    u_char data[0x90];
} GA_DRAWCTRL_DATA;
static_assert(sizeof(GA_DRAWCTRL_DATA) == 0x90, "GA_DRAWCTRL_DATA");

// gaSetupDrawCtrlData's settings, passed by value (scene_init copies 0x78 bytes: 28 words in a1..t3 and
// on the stack, then one doubleword at +0x70). A draw list entry points at its own (DRAW_OBJ_ENTRY).
// The fields are read with gaInitDrawObject.c's TU.
typedef struct GA_DRAWCTRL_ARGS {
    u_int word[28];                     /* 0x00 */
    u_long reg70;                       /* 0x70: a 64-bit value (a GS register?) */
} GA_DRAWCTRL_ARGS;
static_assert(sizeof(GA_DRAWCTRL_ARGS) == 0x78, "GA_DRAWCTRL_ARGS");
static_assert(offsetof(GA_DRAWCTRL_ARGS, reg70) == 0x70, "GA_DRAWCTRL_ARGS.reg70");

typedef struct GA_DRAWCOMMAND_DATA {
    u_char head[0x38];
    u_long rgbaq;                       /* 0x38: the frame clear's RGBAQ (Q 1.0, A 0x80, RGB = fog colour) */
    u_char tail[0x10];
} GA_DRAWCOMMAND_DATA;
static_assert(sizeof(GA_DRAWCOMMAND_DATA) == 0x50, "GA_DRAWCOMMAND_DATA");
static_assert(offsetof(GA_DRAWCOMMAND_DATA, rgbaq) == 0x38, "GA_DRAWCOMMAND_DATA.rgbaq");

// One bank's fog packet (fogctrl[86], 0x50 bytes each; the symtab's 6880 / 86).
typedef struct GA_FOGCTRL_DATA {
    u_char data[0x50];
} GA_FOGCTRL_DATA;
static_assert(sizeof(GA_FOGCTRL_DATA) == 0x50, "GA_FOGCTRL_DATA");

void gaInitDrawObject(void);                                                             // CT3 0x018a1a48
void gaInit3dlibWork(void *work);                                                        // CT3 0x018a1c40
void gaSetupGSInitData(GA_GSINIT_DATA *gsinit);                                          // CT3 0x018a1d58
void gaBGT_ExecGSInitData_VIF1(GA_GSINIT_DATA *gsinit);                                  // CT3 0x018a1de0
void gaSetupDBufferData(GA_DBUFFER_DATA *dbuf, int a1, int a2, int a3, int a4, int a5, int a6, int a7); // CT3 0x018a1df8
void gaBGT_ExecDBufferData_VIF1(GA_DBUFFER_DATA *dbuf);                                  // CT3 0x018a1f60
void gaSetupDrawCtrlData(GA_DRAWCTRL_DATA *ctrl, GA_DRAWCTRL_ARGS args);                 // CT3 0x018a1fd8 (args by value)
void gaBGT_ExecDrawCtrlData_VIF1(GA_DRAWCTRL_DATA *ctrl);                                // CT3 0x018a22b0
void gaSetupFogCtrlData(GA_FOGCTRL_DATA *fog, float f0, float f1, float f2, int i0, int i1, int i2); // CT3 0x018a22c8
void gaBGT_ExecFogCtrlData_VIF1(GA_FOGCTRL_DATA *fog);                                  // CT3 0x018a2358
void gaBGT_ExecDrawCommandData_VIF1(GA_DRAWCOMMAND_DATA *com);                           // CT3 0x018a2e70
void gaBGT_Start(void *ringbuffer, int size);                                            // CT3 0x018a2f30
void gaBGT_End(void);                                                                    // CT3 0x018a3000
