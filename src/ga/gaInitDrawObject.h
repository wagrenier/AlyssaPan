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

typedef struct GA_DRAWCOMMAND_DATA {
    u_char head[0x38];
    u_long rgbaq;                       /* 0x38: the frame clear's RGBAQ (Q 1.0, A 0x80, RGB = fog colour) */
    u_char tail[0x10];
} GA_DRAWCOMMAND_DATA;
static_assert(sizeof(GA_DRAWCOMMAND_DATA) == 0x50, "GA_DRAWCOMMAND_DATA");
static_assert(offsetof(GA_DRAWCOMMAND_DATA, rgbaq) == 0x38, "GA_DRAWCOMMAND_DATA.rgbaq");

void gaInitDrawObject(void);                                                             // CT3 0x018a1a48
void gaInit3dlibWork(void *work);                                                        // CT3 0x018a1c40
void gaSetupGSInitData(GA_GSINIT_DATA *gsinit);                                          // CT3 0x018a1d58
void gaBGT_ExecGSInitData_VIF1(GA_GSINIT_DATA *gsinit);                                  // CT3 0x018a1de0
void gaSetupDBufferData(GA_DBUFFER_DATA *dbuf, int a1, int a2, int a3, int a4, int a5, int a6, int a7); // CT3 0x018a1df8
void gaBGT_ExecDBufferData_VIF1(GA_DBUFFER_DATA *dbuf);                                  // CT3 0x018a1f60
void gaSetupDrawCtrlData(GA_DRAWCTRL_DATA *ctrl, int a1, int a2, int a3, int a4, int a5, int a6, int a7); // CT3 0x018a1fd8
void gaBGT_ExecDrawCtrlData_VIF1(GA_DRAWCTRL_DATA *ctrl);                                // CT3 0x018a22b0
void gaBGT_ExecDrawCommandData_VIF1(GA_DRAWCOMMAND_DATA *com);                           // CT3 0x018a2e70
void gaBGT_Start(void *ringbuffer, int size);                                            // CT3 0x018a2f30
void gaBGT_End(void);                                                                    // CT3 0x018a3000
