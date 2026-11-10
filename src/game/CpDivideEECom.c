// game/CpDivideEECom.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/CpDivideEECom.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018d1e70
// The ROM answers &gEEStat (bss 0x01943a40); until this TU is written the stub answers a zeroed record of
// its own, so FlProc's read of +0x50 (the IOP not busy) holds (game/ArtoolDebugOpen.c, 2026-11-10).
EE_IOP_STAT * CpGetEEIOPStatus(void)
{
    static EE_IOP_STAT s_idle;
    ALYSSAPAN_STUB("CpGetEEIOPStatus", 0x018d1e70);
    return &s_idle;
}

// stub of CT3 0x018d2250
// The ROM answers its RPC reply buffer; FSS_Thread reads the word whenever the queued command asked for a
// reply (SdPlaySEProc's SND_CMD_SE_KEY_ON_CH), so the stub answers a zero word of its own (no channel
// sounds) instead of NULL -- FSS_Thread runs since IopLoadCapcomSound was written (2026-11-10).
int * CpCall_IOPSndDriver(u_int command, void *data)
{
    static int s_reply;
    ALYSSAPAN_STUB("CpCall_IOPSndDriver", 0x018d2250);
    s_reply = 0;
    return &s_reply;
}

// stub of CT3 0x018d2120
int * CpCall_IOPSndDriver_2(u_int command, void *data)
{
    ALYSSAPAN_STUB("CpCall_IOPSndDriver_2", 0x018d2120);
    return {};
}

// stub of CT3 0x018d1e80
int CpCheckEE_IOPComEnd(u_int command, int poll)
{
    ALYSSAPAN_STUB("CpCheckEE_IOPComEnd", 0x018d1e80);
    return {};
}

// stub of CT3 0x018d1fa0
int CpSifDmaTransEEToIOP(const void *src, u_int iopAddr, u_int size, int a3, int intr)
{
    ALYSSAPAN_STUB("CpSifDmaTransEEToIOP", 0x018d1fa0);
    return {};
}

// stub of CT3 0x018d2620
void CpCreateEEComRecieveThread(int priority, int stackSize)
{
    ALYSSAPAN_STUB("CpCreateEEComRecieveThread", 0x018d2620);
}

// stub of CT3 0x018d2700
void CpEEInit_RpcClient(void)
{
    ALYSSAPAN_STUB("CpEEInit_RpcClient", 0x018d2700);
}

// stub of CT3 0x018d26b0
void CpExchangeTransBuffer(void)
{
    ALYSSAPAN_STUB("CpExchangeTransBuffer", 0x018d26b0);
}

// stub of CT3 0x018d2770
void CpInit_Gloval_EE(void)
{
    ALYSSAPAN_STUB("CpInit_Gloval_EE", 0x018d2770);
}

// stub of CT3 0x018d2850
int CpMakeSnddrvInitStr(char *buf, const SND_DRV_INIT *init)
{
    ALYSSAPAN_STUB("CpMakeSnddrvInitStr", 0x018d2850);
    return {};
}
