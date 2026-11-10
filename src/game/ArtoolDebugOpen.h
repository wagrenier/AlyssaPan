// game/ArtoolDebugOpen.h -- CT3 game/ArtoolDebugOpen.c (CT3 0x0189e180..0x0189f460): the file layer. The
// IOP boot (IopLoadModules), the pack directory of DATA\CT3PACK.DAT (FlInit), and the read queue: up to
// 20 requests that FlProc, once a frame, turns into whole-sector sceCdRead calls. A file in the pack is
// found by its name in gFlPackInfo; any other is sized by the IOP's file server (IOPGetFileSize) and read
// by its sectors on the disc. Reconstructed 2026-11-10.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// ---- limits -------------------------------------------------------------------------------------------
#define FL_REQ_MAX        20     // gReqRd's rows (the searches stop at 0x14)
#define FL_PACK_MAX       1100   // gFlPackInfo's rows (35200 bytes; FlInit's TOO SMALL check, 0x8980)
#define FL_SECTOR_SIZE    0x800
#define FL_DIV_READ_WAIT  4      // frames FlProc waits between two pieces of one read (states 0xe / 0xf)

// FL_REQ_RD.flags
#define FL_REQ_OPENING    0x1    // set by FlReqOpen; cleared once the size is known
#define FL_REQ_READING    0x2    // set by FlReqRead; cleared when the last sector has landed
#define FL_REQ_BUSY       (FL_REQ_OPENING | FL_REQ_READING)

// FL_REQ_RD.state: FlProc's steps. 0 = free (a request goes to 1 when opened).
enum FL_STATE {
    FL_STATE_FREE        = 0,
    FL_STATE_OPEN        = 1,    // size known (the pack) -> IDLE; else ask the IOP
    FL_STATE_SIZE_ASK    = 2,    // IOPGetFileSize when the channel is free
    FL_STATE_SIZE_WAIT   = 3,    // the IOP's answer: size and the file's first sector
    FL_STATE_IDLE        = 4,    // 4..8: nothing asked: count up to ROTATE; a read asked: PLAN or START
    FL_STATE_IDLE_LAST   = 8,
    FL_STATE_ROTATE      = 9,    // let the next request have a turn
    FL_STATE_PLAN        = 10,   // a loose file: its sectors from the IOP's base sector
    FL_STATE_START       = 11,   // the read's cursor from its plan
    FL_STATE_READ        = 12,   // sceCdRead when the drive and the IOP are free
    FL_STATE_READ_WAIT   = 13,   // the drive done: advance the cursor
    FL_STATE_PAUSE       = 14,   // between two pieces: FL_DIV_READ_WAIT frames
    FL_STATE_PAUSE_WAIT  = 15,
    FL_STATE_READ_DONE   = 0x14, // the last piece: the file position moves on, the request idles
    FL_STATE_CANCEL      = 0x15, // (no writer in the ROM) drop both flags and idle
};

// gReqRd's rows: one open file. layout: host -- the buffers are host pointers (EE +0x54 / +0x60 were
// EE addresses); only this TU reads the record.
typedef struct FL_REQ_RD {
    u_int flags;            // +0x00 FL_REQ_*
    int handle;             // +0x04 0 = free; gHandleCounter's value at FlReqOpen
    char name[0x38];        // +0x08 a loose file's disc path ("\DIR\NAME;1"); "" for a packed file
    int packIdx;            // +0x40 gFlPackInfo's row, -1 for a loose file
    int pos;                // +0x44 the next sector to read, relative to the file
    short state;            // +0x48 FL_STATE_*
    short pad4a;
    int pad4c;
    int size;               // +0x50 bytes; -1 until known
    u_char *buff;           // +0x54 FlReqRead's destination
    int startSect;          // +0x58 the read's first sector (absolute once planned); -1 = from pos
    int numSect;            // +0x5c sectors to read; -1 = to the end of the file
    u_char *curBuf;         // +0x60 the cursor: where the next piece lands
    int curSect;            // +0x64 its sector
    int remainSect;         // +0x68 sectors still to read
    int baseSect;           // +0x6c the file's first sector on the disc
} FL_REQ_RD;

// gFlPackInfo's rows: CT3PACK.DAT's directory, read from the pack's sectors 1.. (layout: ee).
typedef struct FL_PACK_INFO {
    char name[0x10];        // +0x00 upper case ("EVENT.INF", "GAME_END.DAT"; 12 characters at most)
    int partSize;           // +0x10 a two-part file's first part: a packed NAME.SDT is the SDT (this many
                            //       bytes) then its .HD, likewise .SQ (SdLoadSEProc); 0 otherwise (876 of
                            //       980 rows, CT3PACK.DAT read 2026-11-10)
    int size;               // +0x14 bytes
    int sect;               // +0x18 the first sector: relative to the pack in the file, absolute after FlInit
    int numSect;            // +0x1c sectors
} FL_PACK_INFO;
static_assert(sizeof(FL_PACK_INFO) == 0x20, "FL_PACK_INFO is 0x20 bytes");
static_assert(offsetof(FL_PACK_INFO, partSize) == 0x10, "FL_PACK_INFO.partSize");

// CT3PACK.DAT's sector 0, read into gBuf1Sect.
#define FL_PACK_HDR_COUNT  0  // word 0: the directory's rows (980)
#define FL_PACK_HDR_SECTS  2  // word 2: the header's sectors, sector 0 included (17)

// ---- globals ------------------------------------------------------------------------------------------
extern int gHandleCounter;                       /* sbss 0x0193e310 */
extern char gDeviceName[0x20];                   /* sbss 0x0193e320: "cdrom0:\" (SetDeviceName) */
extern int gNumFlPackFiles;                      /* sbss 0x0193e340 */
extern int gValIOPGetFileLen[4];                 /* sbss 0x0193e350: IOPGetFileSize's answer: size, first sector */
extern FL_REQ_RD gReqRd[FL_REQ_MAX];             /* bss  0x0194e3e0 */
extern FL_PACK_INFO gFlPackInfo[FL_PACK_MAX];    /* bss  0x0194ecc0 */
extern u_int gBuf1Sect[FL_SECTOR_SIZE / 4];      /* bss  0x01957640: one sector (the pack's header) */

// ---- functions ----------------------------------------------------------------------------------------
void ArtoolDebugOpen(void);                                                   // CT3 0x0189e180
void FlProc(void);                                                            // CT3 0x0189e190
int FlReqRead(int handle, void *buff, int startSect, int numSect);            // CT3 0x0189e780
void FlReqClose(int handle);                                                  // CT3 0x0189e970
int FlReqOpen(const char *dir, const char *name);                             // CT3 0x0189ea40
int GetNowReadIdx(void);                                                      // CT3 0x0189ec50
int FlSetProcessIndex(void);                                                  // CT3 0x0189ecb0
int FlGetProcessIndex(void);                                                  // CT3 0x0189ed10
int FlLoad(const char *dir, const char *name, void *buff);                    // CT3 0x0189ed80
int FlIsExecute(int handle);                                                  // CT3 0x0189ee70
int FlGetLen(const char *dir, const char *name, int mode);                    // CT3 0x0189ef10
int FlFindPacked(const char *name);                                           // CT3 0x0189f0d0
void FlInit(void);                                                            // CT3 0x0189f160
char *SetDeviceName(void);                                                    // CT3 0x0189f2d0
void IopLoadModules(void);                                                    // CT3 0x0189f2f0
