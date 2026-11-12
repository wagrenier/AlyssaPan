// game/CpDivideEECom.c -- CT3 0x018d1cb0..0x018d2a68: the EE half of the link to Capcom's sound driver
// (SNDDRV.IRX). Reconstructed 2026-11-12 from the symtab's 17 functions (local chunk 122).
//
// Two RPC clients call the driver: gEE_CliendData (SND_RPC_ID, CpCall_IOPSndDriver: every command, its
// send size picked by the command) and gEE_CliendData_2 (SND_RPC_ID_2, CpCall_IOPSndDriver_2: the two
// IOP -> SPU2 transfers, posted without waiting; an end callback clears their gCpEE_IOPCallBack bit and
// CpCheckEE_IOPComEnd polls it). The driver calls back through the EE's own server (EE_COM_RPC_ID, run by
// the receive thread): _CpDivideEECom writes the status words the game polls (gEEStat) or DMAs EE memory
// to the IOP. CpMakeSnddrvInitStr turns gArg_CpDriverInit into the module's argument strings.
//
// PORT: SNDDRV.IRX is not reconstructed, so no server answers SND_RPC_ID / _2 (sdk/sif.cpp answers zero
// replies at once and runs the end callback) and nothing calls EE_COM_RPC_ID; gAdrIOPStat stays 0 and the
// status DMA to it is skipped by sceSifSetDma. Nothing sounds until the driver runs (open-items.md).
// The ROM reads gEEStat and gCpEE_IOPCallBack through their uncached alias (0x2xxxxxxx); the host has no
// cache. The receive thread's sceSifRpcLoop returns at once on the host (sdk/sifcmd.h).

#include "game/CpDivideEECom.h"

#include "sdk/eekernel.h"
#include "sdk/sifcmd.h"
#include "sdk/sifman.h"
#include "sdk/sifrpc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ---- data ---------------------------------------------------------------------------------------------
SND_JSL_COM gTransJSLCOM;                         /* bss 0x01943c40 */
SND_TRANS_ADS gTransADS;                          /* bss 0x01943b40 */
SND_DRV_INIT gArg_CpDriverInit;                   /* bss 0x01943d40 (IopLoadCapcomSound fills it) */
u_char gTransJSL[0xb0];                           /* bss 0x01943c80 */
u_char gTransJSLCOM2[0x600];                      /* bss 0x01c8bd80 */
u_char gTransSER[0x10];                           /* bss 0x01943c00 */
EE_IOP_STAT gEEStat;                              /* bss 0x01943a40 */
u_int gCpEE_IOPCallBack[4];                       /* bss 0x01943a00 */
u_int gAdrIOPStat;                                /* bss 0x01943a10 */
static sceSifDmaData gTransDmaDataEEToIOP;        /* bss 0x01943b00 (global in the symtab; only used here) */
int gTransDmaDataEEToIOPCueNo;                    /* bss 0x01943ac0 */
int gEEComRecieveTHSemaID;                        /* bss 0x01943978 */
static SemaParam gEEComRecieveTHSemaParam;        /* bss 0x01943980 (global in the symtab; only used here) */
int gEEComRecieveTHID;                            /* bss 0x01943998 */
static ThreadParam gEEComRecieveTHParam;          /* bss 0x019439a0 (global in the symtab; only used here) */
static u_char gEEComRecieveTH_Stack[0x1000];      /* bss 0x01c8ad80 (global in the symtab; only used here) */

static u_short snddrvComMode;                     /* LOCAL 0x0194391c: gArg_CpDriverInit.h1a; 1 = the driver
                                                     passes EE pointers, no DMA (EE_COM_SIF_DMA) */
static sceSifEndFunc mySifCBFunc;                 /* LOCAL mySifCBFunc$163 0x01943918 */
static sceSifClientData gEE_CliendData_2;         /* LOCAL 0x01943920 */
static sceSifClientData gEE_CliendData;           /* LOCAL 0x01943950 */
static u_char gEE_RetBuf_2[0x1800];               /* LOCAL 0x01c86580: CpCall_IOPSndDriver_2's reply */
static u_char gEE_RetBuf[0x1800];                 /* LOCAL 0x01c87d80: CpCall_IOPSndDriver's reply */
static u_char gEE_RpcArg[0x1800];                 /* LOCAL 0x01c89580: the EE server's receive buffer */

static const char s_fmtHex[] = "0x%X";            /* @567 0x01936978 */
static const char s_fmtDec[] = "%d";              /* @568 0x01936980 */

// The ticks CpCall_IOPSndDriver(_2) and CpEEInit_RpcClient spin between polls.
#define CP_POLL_WAIT 10000

// The reply size of most commands (one word).
#define CP_REPLY_WORD 4

// ---- the EE's server ----------------------------------------------------------------------------------

// CT3 0x018d1cb0: the driver's command `command` with its record `data`; answers the reply (retVal, or for
// EE_COM_SIF_DMA in comm mode 1 the record's +8 pointer itself).
static void *_CpDivideEECom(u_int command, void *data, int size)
{
    static int retVal;                            /* LOCAL retVal$255 0x019438a4 */
    const int *arg = (const int *)data;
    void *ret = NULL;

    (void)size;
    command &= 0xffff0000u;
    if (command == EE_COM_SET_IOP_BUSY) {
        CpGetEEIOPStatus()->iopBusy = arg[2];
        retVal = 0;
        ret = &retVal;
    } else if (command == EE_COM_SET_SND_FLAGS) {
        CpGetEEIOPStatus()->sndFlags = (u_int)arg[2];
        retVal = 0;
        ret = &retVal;
    } else if (command == EE_COM_SET_ADS_ACTIVE) {
        CpGetEEIOPStatus()->adsActive = arg[2];
        retVal = 0;
        ret = &retVal;
    } else if (command == EE_COM_SET_STAT_30) {
        CpGetEEIOPStatus()->unk30 = arg[2];
        retVal = 0;
        ret = &retVal;
    } else if (command == EE_COM_SIF_DMA) {
        // PORT: the record's words are 32-bit EE pointers / IOP addresses, widened through uintptr_t.
        if (snddrvComMode == 1) {
            ret = (void *)(uintptr_t)(u_int)arg[2];
        } else {
            retVal = CpSifDmaTransEEToIOP((const void *)(uintptr_t)(u_int)arg[2], (u_int)arg[3], (u_int)arg[4],
                                          arg[5], arg[6]);
            ret = &retVal;
        }
    }
    return ret;
}

// CT3 0x018d1db0: the receive thread: serve EE_COM_RPC_ID into gEE_RpcArg. (The ROM's entry takes no
// argument; the host's ThreadParam entry type has one.)
static void _CpEEComRecieveThread(void *)
{
    static sceSifServeData EESd;                  /* LOCAL EESd$251 0x019438b0 */
    static sceSifQueueData EEQd;                  /* LOCAL EEQd$250 0x01943900 */

    sceSifSetRpcQueue(&EEQd, gEEComRecieveTHID);
    sceSifRegisterRpc(&EESd, EE_COM_RPC_ID, _CpDivideEECom, gEE_RpcArg, NULL, NULL, &EEQd);
    sceSifRpcLoop(&EEQd);
}

// ---- end callbacks (interrupt context in the ROM: each ends with sync; ei) -----------------------------

// CT3 0x018d1e10
static void _CpCB_Null(void *para)
{
    (void)para;
}

// CT3 0x018d1e20: SND_CMD_TRANS_SPU2 has ended.
static void _CpCB_CmdCpSendDataIOPToSPU2(void *para)
{
    *(u_int *)para &= ~CP_CMD_CB_SPU2;
}

// CT3 0x018d1e40: SND_CMD_TRANS_WAVE has ended.
static void _CpCB_CmdCpSendWaveToSPU2(void *para)
{
    *(u_int *)para &= ~CP_CMD_CB_WAVE;
}

// ---- status ---------------------------------------------------------------------------------------------

// CT3 0x018d1e60
u_int *CpGetCPCMDCALLBACK(void)
{
    return gCpEE_IOPCallBack;
}

// CT3 0x018d1e70
EE_IOP_STAT *CpGetEEIOPStatus(void)
{
    return &gEEStat;
}

// CT3 0x018d1e80: for SND_CMD_TRANS_SPU2 / SND_CMD_TRANS_WAVE, 0 once ended (`poll` 0 waits for it, else
// 1 while running); -1 for any other command.
int CpCheckEE_IOPComEnd(u_int command, int poll)
{
    int ret = -1;

    if (command == SND_CMD_TRANS_SPU2) {
        if (poll == 0) {
            do {
                ret = 0;
            } while ((*(volatile u_int *)CpGetCPCMDCALLBACK() & CP_CMD_CB_SPU2) != 0);
        } else if ((*CpGetCPCMDCALLBACK() & CP_CMD_CB_SPU2) == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else if (command == SND_CMD_TRANS_WAVE) {
        if (poll == 0) {
            do {
                ret = 0;
            } while ((*(volatile u_int *)CpGetCPCMDCALLBACK() & CP_CMD_CB_WAVE) != 0);
        } else {
            ret = (*CpGetCPCMDCALLBACK() & CP_CMD_CB_WAVE) != 0;
        }
    }
    return ret;
}

// ---- transfers and calls --------------------------------------------------------------------------------

// CT3 0x018d1fa0: SIF DMA `size` bytes from `src` to the IOP address `iopAddr`, retried until queued, then
// waited for. `intr` 1 uses the interrupt-handler calls, 0 the thread ones.
int CpSifDmaTransEEToIOP(const void *src, u_int iopAddr, u_int size, int a3, int intr)
{
    int ret = 0;

    if (size > CP_SIF_DMA_MAX)
        return CP_SIF_DMA_TOO_BIG;

    if (intr == 1) {
        for (;;) {
            gTransDmaDataEEToIOP.data = (void *)src;
            gTransDmaDataEEToIOP.addr = (void *)(uintptr_t)iopAddr;
            gTransDmaDataEEToIOP.size = (int)size;
            gTransDmaDataEEToIOP.mode = a3;
            iSyncDCache((void *)src, (u_char *)src + size);
            gTransDmaDataEEToIOPCueNo = (int)isceSifSetDma(&gTransDmaDataEEToIOP, 1);
            if (gTransDmaDataEEToIOPCueNo != 0)
                break;
            gTransDmaDataEEToIOPCueNo = 0;
        }
    } else {
        if (intr != 0)
            return CP_SIF_DMA_BAD_MODE;
        for (;;) {
            gTransDmaDataEEToIOP.data = (void *)src;
            gTransDmaDataEEToIOP.addr = (void *)(uintptr_t)iopAddr;
            gTransDmaDataEEToIOP.size = (int)size;
            gTransDmaDataEEToIOP.mode = a3;
            SyncDCache((void *)src, (u_char *)src + size);
            gTransDmaDataEEToIOPCueNo = (int)sceSifSetDma(&gTransDmaDataEEToIOP, 1);
            if (gTransDmaDataEEToIOPCueNo != 0)
                break;
            gTransDmaDataEEToIOPCueNo = 0;
        }
    }

    if (gTransDmaDataEEToIOPCueNo == 0) {
        ret = CP_SIF_DMA_REFUSED;
    } else {
        int stat;
        do {
            if (intr == 1) {
                stat = isceSifDmaStat((u_int)gTransDmaDataEEToIOPCueNo);
            } else {
                if (intr != 0)
                    return CP_SIF_DMA_BAD_MODE_WAIT;
                stat = sceSifDmaStat((u_int)gTransDmaDataEEToIOPCueNo);
            }
        } while (stat >= 0);
    }
    return ret;
}

// CT3 0x018d2120: post SND_CMD_TRANS_SPU2 / SND_CMD_TRANS_WAVE (a 0x20-byte record) on the second channel,
// setting its gCpEE_IOPCallBack bit for the end callback to clear; other commands are dropped.
int *CpCall_IOPSndDriver_2(u_int command, void *data)
{
    while (sceSifCheckStatRpc(&gEE_CliendData_2.rpcd) == 1)
        CpEEWait(CP_POLL_WAIT);

    u_int kind = command & 0xffff0000u;
    if (kind == SND_CMD_TRANS_SPU2 || kind == SND_CMD_TRANS_WAVE) {
        if (kind == SND_CMD_TRANS_SPU2) {
            *CpGetCPCMDCALLBACK() |= CP_CMD_CB_SPU2;
            mySifCBFunc = (sceSifEndFunc)_CpCB_CmdCpSendDataIOPToSPU2;
        } else if (kind == SND_CMD_TRANS_WAVE) {
            *CpGetCPCMDCALLBACK() |= CP_CMD_CB_WAVE;
            mySifCBFunc = (sceSifEndFunc)_CpCB_CmdCpSendWaveToSPU2;
        }
        while (sceSifCallRpc(&gEE_CliendData_2, command, SIF_RPC_M_NOWAIT, data, sizeof(SND_JSL_COM),
                             gEE_RetBuf_2, CP_REPLY_WORD, mySifCBFunc, CpGetCPCMDCALLBACK()) != 0) {
        }
    }
    return (int *)gEE_RetBuf_2;
}

// One call on the first channel, retried until the SIF takes it (each of CpCall_IOPSndDriver's branches).
#define CP_CALL(mode, ssize, rsize, func)                                                                   \
    while (sceSifCallRpc(&gEE_CliendData, command, (mode), data, (ssize), gEE_RetBuf, (rsize), (func),       \
                         NULL) != 0) {                                                                      \
    }

// CT3 0x018d2250: send `command` with its record `data` to the driver and wait for the reply; the send and
// reply sizes are the command's. Answers the reply buffer.
// callcheck: sceSifCallRpc (inside CP_CALL)
int *CpCall_IOPSndDriver(u_int command, void *data)
{
    while (sceSifCheckStatRpc(&gEE_CliendData.rpcd) == 1)
        CpEEWait(CP_POLL_WAIT);

    u_int kind = command & 0xffff0000u;
    if (kind == 0x270000u) {
        // The low byte counts 0x20-byte records.
        CP_CALL(0, (int)((command & 0xff) << 5), CP_REPLY_WORD, NULL)
    } else if (kind == SND_CMD_PLAY_SE) {
        CP_CALL(0, sizeof(SND_JSL_COM), CP_REPLY_WORD, (sceSifEndFunc)_CpCB_Null)
    } else if (kind == SND_CMD_ADS_DISABLE || kind == 0x300000u || kind == 0x2e0000u || kind == SND_CMD_ADS_STOP
               || kind == SND_CMD_ADS_START || kind == 0) {
        CP_CALL(0, 0, CP_REPLY_WORD, NULL)
    } else if (kind == SND_CMD_ADS_OPEN) {
        CP_CALL(SIF_RPC_M_NOWAIT, sizeof(SND_TRANS_ADS), CP_REPLY_WORD, (sceSifEndFunc)_CpCB_Null)
    } else if (kind == SND_CMD_ADS_VOLUME || kind == SND_CMD_ADS_ENABLE) {
        CP_CALL(0, sizeof(SND_TRANS_ADS), CP_REPLY_WORD, NULL)
    } else if (kind == SND_CMD_SE_REGISTER) {
        CP_CALL(0, sizeof(gTransJSL), CP_REPLY_WORD, NULL)
    } else if (kind == SND_CMD_SOUND_ON || kind == SND_CMD_SOUND_OFF || kind == 0x130000u
               || kind == SND_CMD_SE_KEY_ON_CH || kind == 0x2f0000u || kind == SND_CMD_SE_PURGE
               || kind == 0x90000u) {
        CP_CALL(0, 0, CP_REPLY_WORD, NULL)
    } else if (kind == 0x140000u) {
        CP_CALL(0, 0, 0x1c, NULL)
    } else if (kind == 0x50000u) {
        CP_CALL(0, 0, 0x98, NULL)
    } else {
        CP_CALL(0, sizeof(SND_JSL_COM), CP_REPLY_WORD, NULL)
    }
    return (int *)gEE_RetBuf;
}

#undef CP_CALL

// ---- set-up -------------------------------------------------------------------------------------------

// CT3 0x018d2620: start the thread that serves the driver's calls to the EE.
void CpCreateEEComRecieveThread(int priority, int stackSize)
{
    gEEComRecieveTHSemaParam.initCount = 1;
    gEEComRecieveTHSemaParam.maxCount = 0;
    gEEComRecieveTHSemaID = CreateSema(&gEEComRecieveTHSemaParam);

    gEEComRecieveTHParam.entry = _CpEEComRecieveThread;
    gEEComRecieveTHParam.stack = gEEComRecieveTH_Stack;
    gEEComRecieveTHParam.option = 0;
    gEEComRecieveTHParam.gpReg = NULL;            // PORT: the ROM passes &_gp
    gEEComRecieveTHParam.stackSize = stackSize;
    gEEComRecieveTHParam.initPriority = priority;
    gEEComRecieveTHID = CreateThread(&gEEComRecieveTHParam);
    if (gEEComRecieveTHID >= 0)
        StartThread(gEEComRecieveTHID, NULL);
}

// CT3 0x018d26b0: ask the driver for its status block's IOP address and DMA gEEStat there.
// PORT: the ROM keeps gEEStat's 32-bit uncached alias in gTransJSLCOM.arg[0]; the host DMAs from the
// pointer it got, and the record carries its low 32 bits only.
void CpExchangeTransBuffer(void)
{
    EE_IOP_STAT *stat = &gEEStat;
    gTransJSLCOM.arg[0] = (int)(uintptr_t)stat;
    gAdrIOPStat = (u_int)*CpCall_IOPSndDriver(SND_CMD_GET_STAT_ADDR, &gTransJSLCOM);
    CpSifDmaTransEEToIOP(stat, gAdrIOPStat, sizeof(EE_IOP_STAT), 0, 0);
}

// CT3 0x018d2700: bind both driver channels, retrying until the IOP has registered each.
void CpEEInit_RpcClient(void)
{
    do {
        sceSifBindRpc(&gEE_CliendData, SND_RPC_ID, 0);
        CpEEWait(CP_POLL_WAIT);
    } while (gEE_CliendData.serve == NULL);
    do {
        sceSifBindRpc(&gEE_CliendData_2, SND_RPC_ID_2, 0);
        CpEEWait(CP_POLL_WAIT);
    } while (gEE_CliendData_2.serve == NULL);
}

// CT3 0x018d2770: clear every command record and the status.
void CpInit_Gloval_EE(void)
{
    memset(gTransJSL, 0, sizeof(gTransJSL));
    memset(&gTransJSLCOM, 0, sizeof(gTransJSLCOM));
    memset(gTransJSLCOM2, 0, sizeof(gTransJSLCOM2));
    memset(gTransSER, 0, sizeof(gTransSER));
    memset(&gTransADS, 0, sizeof(gTransADS));
    memset(&gTransDmaDataEEToIOP, 0, sizeof(gTransDmaDataEEToIOP));
    gTransDmaDataEEToIOPCueNo = 0;
    memset(CpGetEEIOPStatus(), 0, sizeof(EE_IOP_STAT));
    gAdrIOPStat = 0;
    memset(CpGetCPCMDCALLBACK(), 0, sizeof(gCpEE_IOPCallBack));
}

// CT3 0x018d2820: count down to below zero; the ROM pads the loop with nops. Its loop head is
// CT3 0x018d2838 (FUN_018d2838, split off by Ghidra at the branch target: hazards.md, left alone).
void CpEEWait(int count)
{
    volatile int n = count;
    while (n-- > 0) {
    }
}

// CT3 0x018d2850: the driver's ten start arguments as consecutive strings in `buf` (three hex words, then
// decimals); records the comm mode (h1a). Answers the bytes written as the ROM's low byte, 0 past 0xfa.
int CpMakeSnddrvInitStr(char *buf, const SND_DRV_INIT *init)
{
    char s0[16], s1[16], s2[16], s3[16], s4[16], s5[16], s6[16], s7[16], s8[16], s9[16];

    sprintf(s0, s_fmtHex, init->w00);
    sprintf(s1, s_fmtHex, init->w04);
    sprintf(s2, s_fmtHex, init->w08);
    sprintf(s3, s_fmtDec, init->w0c);
    sprintf(s4, s_fmtDec, (u_int)init->h10);
    sprintf(s5, s_fmtDec, (u_int)init->h12);
    sprintf(s6, s_fmtDec, (u_int)init->h14);
    sprintf(s7, s_fmtDec, (u_int)init->h16);
    sprintf(s8, s_fmtDec, (u_int)init->h18);
    sprintf(s9, s_fmtDec, (u_int)init->h1a);
    snddrvComMode = init->h1a;

    char *p = strcpy(buf, s0);
    p = strchr(p, 0);
    p = strcpy(p + 1, s1);
    p = strchr(p, 0);
    p = strcpy(p + 1, s2);
    p = strchr(p, 0);
    p = strcpy(p + 1, s3);
    p = strchr(p, 0);
    p = strcpy(p + 1, s4);
    p = strchr(p, 0);
    p = strcpy(p + 1, s5);
    p = strchr(p, 0);
    p = strcpy(p + 1, s6);
    p = strchr(p, 0);
    p = strcpy(p + 1, s7);
    p = strchr(p, 0);
    p = strcpy(p + 1, s8);
    p = strchr(p, 0);
    p = strcpy(p + 1, s9);
    p = strchr(p, 0);

    u_char len = (u_char)(p + 1 - buf);
    if (len > 0xfa)
        len = 0;
    return (signed char)len;
}
