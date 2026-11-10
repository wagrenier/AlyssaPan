// sdk/eeregs.h -- the EE registers FS2 touches. On the console these are memory-mapped; here they are
// host variables, and the timer counts objects that move by themselves (ee::Kernel's clock), so that
// `REG_RCNT0_COUNT = 0` and `x = REG_RCNT0_COUNT` read as the ROM wrote them.
#pragma once

#include "scetypes.h"

class EeTimerCount
{
public:
    explicit constexpr EeTimerCount(int timer) : m_timer(timer) {}
    operator u_int() const;
    u_int operator=(u_int value);

private:
    int m_timer;
};

// Tn_MODE: a write sets the timer's clock source (CLKS, bits 0..1) and the rest as written; the host
// timer keeps counting from where it was, at the new rate.
class EeTimerMode
{
public:
    explicit constexpr EeTimerMode(int timer) : m_timer(timer) {}
    operator u_int() const;
    u_int operator=(u_int value);

private:
    int m_timer;
};

inline EeTimerCount g_regRcnt0Count{0};
inline EeTimerCount g_regRcnt1Count{1};
inline EeTimerMode g_regRcnt0Mode{0};
inline EeTimerMode g_regRcnt1Mode{1};
inline volatile u_int g_regDStat;
inline volatile u_int g_regIpuToChcr;
inline volatile u_int g_regIpuToMadr;
inline volatile u_int g_regIpuToQwc;
inline volatile u_int g_regDCtrl;    // D_CTRL 0x1000e000: kept as written, nothing reads its bits on the host
inline volatile u_int g_regDEnableW; // D_ENABLEW 0x1000f590: the DMAC never runs on its own here
inline volatile u_int g_regVif0Stat; // VIF0_STAT 0x10003800: no VIF on the host, so it always reads idle (0)
inline volatile u_int g_regVif1Stat; // VIF1_STAT 0x10003c00: likewise
inline volatile u_int g_regVif1Mark; // VIF1_MARK 0x10003c30: kept as written
inline volatile u_int g_regSprFromMadr; // D8_MADR 0x1000d010 } equal on the host: the BGT ring reads empty
inline volatile u_int g_regVif1Tadr;    // D1_TADR 0x10009030 }
inline volatile u_int g_regDRbsr;       // D_RBSR 0x1000e040: the MFIFO ring's mask, kept as written
inline volatile u_int g_regGifStat;     // GIF_STAT 0x10003020: no GIF on the host, so it always reads idle (0)
inline volatile u_long g_regGsPmode;
inline volatile u_long g_regGsSmode2;
inline volatile u_long g_regGsDispfb1;
inline volatile u_long g_regGsDisplay1;
inline volatile u_long g_regGsDispfb2;
inline volatile u_long g_regGsDisplay2;
inline volatile u_long g_regGsBgcolor;
inline volatile u_long g_regGsCsr;

#define REG_RCNT0_COUNT g_regRcnt0Count
#define REG_RCNT1_COUNT g_regRcnt1Count
#define REG_RCNT0_MODE g_regRcnt0Mode
#define REG_RCNT1_MODE g_regRcnt1Mode
#define REG_DMAC_4_IPU_TO_CHCR g_regIpuToChcr
#define REG_DMAC_4_IPU_TO_MADR g_regIpuToMadr
#define REG_DMAC_4_IPU_TO_QWC g_regIpuToQwc
#define REG_DMAC_CTRL g_regDCtrl
#define REG_DMAC_ENABLEW g_regDEnableW
#define REG_VIF0_STAT g_regVif0Stat
#define REG_VIF1_STAT g_regVif1Stat
#define REG_VIF1_MARK g_regVif1Mark
#define REG_DMAC_8_SPR_FROM_MADR g_regSprFromMadr
#define REG_DMAC_1_VIF1_TADR g_regVif1Tadr
#define REG_DMAC_RBSR g_regDRbsr
#define REG_GIF_STAT g_regGifStat
#define REG_GS_PMODE g_regGsPmode
#define REG_GS_SMODE2 g_regGsSmode2
#define REG_GS_DISPFB1 g_regGsDispfb1
#define REG_GS_DISPLAY1 g_regGsDisplay1
#define REG_GS_DISPFB2 g_regGsDispfb2
#define REG_GS_DISPLAY2 g_regGsDisplay2
#define REG_GS_BGCOLOR g_regGsBgcolor
#define REG_GS_CSR g_regGsCsr
#define D_STAT (&g_regDStat)
