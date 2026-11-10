// ga/gaDmaSync_VIF1.h -- CT3 ga/gaDmaSync_VIF1.c (0x018a1738..0x018a1948): the ga library's hand-written
// VIF1 / GIF DMA kicks and syncs, and the BGT ring's send (a packet through the scratchpad into the MFIFO
// that feeds VIF1). Reconstructed 2026-11-10 from the disassembly (mipsdis.py): an asm object (a .text
// section symbol, the error labels, no gcc2_compiled.).
#pragma once

#include "sdk/scetypes.h"

// The syncs give up after this many polls and report their caller (ga/wait.c).
#define GA_DMA_SYNC_TIMEOUT 0x4000000

// The BGT packet area's second half in the scratchpad: GA_SPR_WORK.bgtPacketEE flips between
// GA_SPR_BGT_PACKET (0xa00, gaInitDrawObject.h) and this one after each packet copied in from memory.
#define GA_SPR_BGT_PACKET_B 0x1800

void gaDmaSync_VIF1(void);                             // CT3 0x018a1740
void gaDmaSendN_VIF1(void *addr, int qwc);             // CT3 0x018a1778
void gaDmaSendS_VIF1(void *tag);                       // CT3 0x018a17b8 (no caller in the ROM)
// The BGT ring's send: a packet in memory is first copied into the scratchpad packet area (which then
// flips halves); a scratchpad packet goes as it is.
void gaBGT_DmaSendN_fromSPR_VIF1(void *addr, int qwc); // CT3 0x018a17f8
void gaDmaSync_GIF(void);                              // CT3 0x018a18e8 (no caller in the ROM)
