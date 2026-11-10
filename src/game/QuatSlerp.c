// game/QuatSlerp.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/QuatSlerp.h"
#include "ga/gaInit3dlib.h"
#include "game/unassigned_data.h"
#include "ga/gaSinCos.h"
#include "game/ssGrpPortalCheck.h"
#include "game/main.h"
#include "sdk/eekernel.h"
#include "ga/gaInitDrawObject.h"
#include "game/main2.h"
#include "game/fileio.h"
#include "game/ahalloc.h"
#include "game/Convert.h"
#include "game/sound.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

// CT3 0x019395b0 (RoomNameTbl, 95 names) and 0x01939730 (RoomIdTbl, 95 trigger events + -1): row i
// is a room (its files' first four characters) and the event RoomInFlagOn registers in it.
static const char *const RoomNameTbl[ROOM_TBL_COUNT] = {
    "0A01", "0A02", "0A03", "0A06", "0A07", "0A10", "0A11", "0A16", "0A17", "0A19",
    "0A23", "0A27", "0A30", "0B01", "0B03", "0B06", "0B07", "0B19", "1A02", "1A04",
    "1A10", "1A11", "1B01", "1B03", "1B04", "1B05", "1B07", "1B08", "1B09", "1B11",
    "1B12", "1C01", "1C02", "1C03", "1C04", "1C05", "1C06", "2A01", "2A02", "2B01",
    "2B02", "2B03", "2B04", "2B05", "2B06", "2B07", "2B08", "2D01", "2D02", "2D03",
    "3A01", "3A02", "3C01", "3C02", "3C03", "3C05", "3C06", "3C07", "3D01", "3D02",
    "3D03", "3D05", "3E01", "3F01", "4A01", "4A02", "4A03", "4A04", "4A05", "4A06",
    "4A07", "4A08", "4A09", "4A10", "4A11", "4A12", "4B01", "4B02", "4B03", "4B04",
    "4B05", "4B06", "4B07", "4B08", "4B09", "4B10", "4C01", "4N01", "4N02", "4N03",
    "5A01", "5A01", "5A02", "5A03", "5A04",
};
static const int RoomIdTbl[ROOM_TBL_COUNT + 1] = {
    1846, 1847, 1848, 1851, 1852, 1855, 1856, 1861, 1862, 1864, 1867, 1871,
    1922, 1872, 1874, 1875, 1876, 1879, 1801, 1802, 1803, 1804, 1806, 1808,
    1809, 1810, 1812, 1813, 1814, 1816, 1817, 1818, 1819, 1820, 1821, 1822,
    1823, 1833, 1834, 1835, 1836, 1837, 1838, 1839, 1840, 1841, 1842, 1843,
    1844, 1845, 1915, 1916, 1910, 1917, 1911, 1881, 1882, 1907, 1883, 1884,
    1908, 1885, 1909, 1886, 1887, 1888, 1919, 1889, 1893, 1894, 1890, 1918,
    1891, 1892, 1895, 1896, 1902, 1904, 1906, 1921, 1920, 1897, 1898, 1899,
    1914, 1905, 1912, 1900, 1901, 1903, 1913, 1913, 1925, 1923, 1924, -1,
};


// CT3 0x0185df90
// Written early (ahRelocateSunPs2ObjectArry.c slice 13): VU0 RSQRT of x2 + y2 + z2, then 1 / Q.
// (The ROM also stores the normalised vector to a stack slot nobody reads.)
float asVectorLength3D(const float *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

// ---- GS register words (written early, ahRelocateSunPs2ObjectArry.c slice 14: asShadowEnd) ----

// CT3 0x0185c450
u_long asDR_RGBAQ(u_char r, u_char g, u_char b, u_char a, float q)
{
    u_int qbits;
    memcpy(&qbits, &q, sizeof(qbits));
    return (u_long)r | (u_long)g << 8 | (u_long)b << 16 | (u_long)a << 24 | (u_long)qbits << 32;
}

// CT3 0x0185c3a0
u_int asDR_UV(u_short u, u_short v)
{
    return (u_int)u | (u_int)v << 16;
}

// CT3 0x0185c410: pixels to the GS's 12.4 window coordinates (offset 0x800).
u_long asDR_XYZ2(short x, short y, u_short z)
{
    return (u_long)(u_short)((x + 0x800) * 0x10) | (u_long)(u_short)((y + 0x800) * 0x10) << 16 | (u_long)z << 32;
}

// CT3 0x0185c3d0: as asDR_XYZ2 (the register it goes to differs, not the word).
u_long asDR_XYZ3(short x, short y, u_short z)
{
    return (u_long)(u_short)((x + 0x800) * 0x10) | (u_long)(u_short)((y + 0x800) * 0x10) << 16 | (u_long)z << 32;
}

// ---- the Cont* writers EA_010 calls (written early, ahRelocateSunPs2ObjectArry.c slice 14) ----
// Each walks n elements (the ROM's do / while on a non-zero count): float vectors 0x10 apart, packed
// ones 3 shorts apart.

// CT3 0x0185cf30: as asZipTrans2Trans_XYZ1, w = 0 (a direction).
void asZipTrans2Trans_XYZ0(float *out, const short *in, u_int shift)
{
    float scale = 1.0f / (float)(1 << shift);
    int i;
    for (i = 0; i < 3; i++)
        out[i] = (float)in[i] * scale;
    out[3] = 0.0f;
}

// CT3 0x0185d2a0
void asContInterpolateVector(float *out, const float *a, const float *b, const float *w, int n)
{
    for (; n != 0; n--, out += 4, a += 4, b += 4)
        asInterpolateVector(out, a, b, w);
}

// CT3 0x0185d1f0: a vertex whose w is skipW keeps its value.
void asContInterpolateVector_nw(float skipW, float *out, const float *a, const float *b, const float *w,
                                int n)
{
    for (; n != 0; n--, out += 4, a += 4, b += 4)
        if (a[3] != skipW)
            asInterpolateVector(out, a, b, w);
}

// CT3 0x0185c480: the qwords whose w is not skipW.
void asMemCopy128c_nw(float skipW, void *dst, const void *src, int n)
{
    float *d = (float *)dst;
    const float *s = (const float *)src;
    for (; n != 0; n--, d += 4, s += 4)
        if (s[3] != skipW)
            memmove(d, s, 16);
}

// CT3 0x0185c9e0: packed normals (shift 14), w 0.
void asContZipNormal2Normal_XYZ0(float *out, const short *in, int n)
{
    for (; n != 0; n--, out += 4, in += 3)
        asZipTrans2Trans_XYZ0(out, in, ZIP_NORMAL_SHIFT);
}

// CT3 0x0185cd30
void asContZipTrans2Trans_XYZ1(float *out, const short *in, int n, u_int shift)
{
    for (; n != 0; n--, out += 4, in += 3)
        asZipTrans2Trans_XYZ1(out, in, shift);
}

// CT3 0x0185cc90: a vertex whose x short is `skip` keeps its value.
void asContZipTrans2Trans_XYZ1_nw(float *out, const short *in, int n, u_int shift, short skip)
{
    for (; n != 0; n--, out += 4, in += 3)
        if (in[0] != skip)
            asZipTrans2Trans_XYZ1(out, in, shift);
}

// CT3 0x0185cbe0
void asContInterpolateGA_VECTORZipTrans_XYZ1(float *out, const float *a, const short *b, const float *w,
                                             int n, u_int shift)
{
    alignas(16) float v[4];
    for (; n != 0; n--, out += 4, a += 4, b += 3) {
        asZipTrans2Trans_XYZ1(v, b, shift);
        asInterpolateVector(out, a, v, w);
    }
}

// CT3 0x0185cb20
void asContInterpolateGA_VECTORZipTrans_XYZ1_nw(float *out, const float *a, const short *b, const float *w,
                                                int n, u_int shift, short skip)
{
    alignas(16) float v[4];
    for (; n != 0; n--, out += 4, a += 4, b += 3) {
        if (b[0] != skip) {
            asZipTrans2Trans_XYZ1(v, b, shift);
            asInterpolateVector(out, a, v, w);
        }
    }
}

// CT3 0x0185c920: the blend renormalised (VU0 RSQRT: xyz * 1 / |xyz|; w as blended).
void asContInterpolateGA_VECTORZipNormal_XYZ0(float *out, const float *a, const short *b, const float *w,
                                              int n)
{
    alignas(16) float v[4];
    for (; n != 0; n--, out += 4, a += 4, b += 3) {
        float len;
        asZipTrans2Trans_XYZ0(v, b, ZIP_NORMAL_SHIFT);
        asInterpolateVector(out, a, v, w);
        len = 1.0f / sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
        out[0] *= len;
        out[1] *= len;
        out[2] *= len;
    }
}

// CT3 0x0185c5d0: an unsigned 7.9 fixed value (u_short / 512) into x; the other lanes untouched.
void asZipFloat0_7_9X2Float(float *out, const u_short *in)
{
    out[0] = (float)*in / 512.0f;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01856c30
void AmbientAlphaUpdate(void)
{
    ALYSSAPAN_STUB("AmbientAlphaUpdate", 0x01856c30);
}

// CT3 0x01942970 (ClatteringData): the controller rattles (struggles the player shakes free of).
CLATTERING ClatteringData[CLATTERING_MAX];

// CT3 0x01852f10: drop a rattle; 1 for a slot out of range or already free, else 0.
int ControllerClatteringFree(int slot)
{
    if (slot < 0 || slot >= CLATTERING_MAX)
        return 1;
    if (ClatteringData[slot].state < 0)
        return 1;
    ClatteringData[slot].state = CLATTERING_FREE;
    return 0;
}

// CT3 0x01852f60: the rattle's state; a finished one (WON / LOST) is freed as it is read.
// (No range check: the ROM's.)
int ControllerClatteringGet(int slot)
{
    int state = ClatteringData[slot].state;
    if (state > 0)
        ClatteringData[slot].state = CLATTERING_FREE;
    return state;
}

// CT3 0x01852f90: per frame unless paused (PAUSE_RUNNING still runs): a button press on controller
// 1, or any of its stick bytes moving CLATTERING_STICK_SWING from the last counted position, counts
// one; decayFrames idle frames take one back. Won at goal, lost when timeLeft runs out.
void ControllerClatteringUpdate(void)
{
    int moved = 0;
    int i;

    if (pause != 0 && (pause & PAUSE_RUNNING) == 0)
        return;
    for (i = 0; i < CLATTERING_MAX; i++) {
        CLATTERING *c = &ClatteringData[i];
        if (c->state != CLATTERING_RUNNING)
            continue;
        // the ROM compares stick[2], [3], [0], [1] with the slot's [0..3] (it stores them so too)
        if (abs(pad[0].stick[2] - c->stick[0]) >= CLATTERING_STICK_SWING
            || abs(pad[0].stick[3] - c->stick[1]) >= CLATTERING_STICK_SWING
            || abs(pad[0].stick[0] - c->stick[2]) >= CLATTERING_STICK_SWING
            || abs(pad[0].stick[1] - c->stick[3]) >= CLATTERING_STICK_SWING) {
            // (moved stays set for the later slots: the ROM's)
            moved = 1;
            c->stick[0] = pad[0].stick[2];
            c->stick[1] = pad[0].stick[3];
            c->stick[2] = pad[0].stick[0];
            c->stick[3] = pad[0].stick[1];
        }
        if (pad_trg_button[0] != 0 || moved) {
            c->count++;
            c->idle = 0;
        } else if (++c->idle >= c->decayFrames) {
            c->idle = 0;
            if (--c->count < 0)
                c->count = 0;
        }
        if (--c->timeLeft == 0)
            c->state = CLATTERING_LOST;
        else if (c->count >= c->goal)
            c->state = CLATTERING_WON;
    }
}

// CT3 0x01853150: open a rattle in the first free slot and answer the slot.
int ControllerClatteringSet(const CLATTERING_SET *set)
{
    CLATTERING *c;
    int i;

    for (i = 0; i < CLATTERING_MAX; i++)
        if (ClatteringData[i].state < 0)
            break;
    // PORT: with no free slot the ROM fills slot 10, past the array (over pad_key_button); the host
    // answers CLATTERING_MAX without writing (ControllerClatteringFree refuses it).
    if (i == CLATTERING_MAX)
        return i;
    c = &ClatteringData[i];
    c->state = CLATTERING_RUNNING;
    c->count = 0;
    c->idle = 0;
    c->stick[0] = c->stick[1] = c->stick[2] = c->stick[3] = 0;
    c->w08 = set->w08 < 0 ? CLATTERING_DEF_W08 : set->w08;
    c->decayFrames = set->decayFrames < 0 ? CLATTERING_DEF_DECAY : set->decayFrames;
    c->goal = set->goal < 0 ? CLATTERING_DEF_GOAL : set->goal;
    c->timeLeft = set->time < 0 ? CLATTERING_DEF_TIME : set->time;
    return i;
}

// CT3 0x01853220: every slot free (the ROM unrolls the first eight).
void ControllerClatteringInit(void)
{
    int i;
    for (i = 0; i < CLATTERING_MAX; i++)
        ClatteringData[i].state = CLATTERING_FREE;
}

// CT3 0x01852010: bit `event` of SkipEventFlag (the demos the player has seen and may skip).
int DemoSkipCheckEvent(int event)
{
    return (u_char)SkipEventFlag[event >> 3] >> (event & 7) & 1;
}

// CT3 0x01940e80 (Panicdef) and 0x01940e44 (AdvantageousTimeTime): .sbss beside PanicInfoData.
PANIC_DEF Panicdef;
int AdvantageousTimeTime;

// stub of CT3 0x01856660
void FogAnimeUpdate(void)
{
    ALYSSAPAN_STUB("FogAnimeUpdate", 0x01856660);
}

// stub of CT3 0x01854d60
void MonsterAIUpdate(int mode)
{
    ALYSSAPAN_STUB("MonsterAIUpdate", 0x01854d60);
}

// stub of CT3 0x01853620
void PanicFilter(void)
{
    ALYSSAPAN_STUB("PanicFilter", 0x01853620);
}

// CT3 0x01854410: the player's panic state back to calm, and the panic tuning.
void PanicModeInit(void)
{
    int i;

    TransparentBodyTime = PANIC_TRANSPARENT_TIME;
    AdvantageousTimeTime = PANIC_ADVANTAGEOUS_TIME;
    PanicInfoData.mode = 0;
    PanicInfoData.b01 = 0xff;
    PanicInfoData.h02 = 0;
    for (i = 0; i < 8; i++)
        PanicInfoData.w04[i] = 0;
    PanicInfoData.w24 = 0;
    PanicInfoData.f28 = 10000.0f;
    PanicInfoData.w2c = -1;
    Panicdef.f00 = 3.5f;
    Panicdef.f04 = 8.0f;
    Panicdef.f08 = 5000.0f;
    Panicdef.f0c = 2.0f;
    Panicdef.w10 = 30;
    Panicdef.w14 = 4;
    Panicdef.w18 = 8;
    ssTrigSetVariable(TRIG_VAR_PANIC, 0, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_3, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_26, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_48, 1, 0);
}

// stub of CT3 0x018537b0
void PanicUpdate(void)
{
    ALYSSAPAN_STUB("PanicUpdate", 0x018537b0);
}

// stub of CT3 0x018550b0
void PlayerControlUpdate(int mode)
{
    ALYSSAPAN_STUB("PlayerControlUpdate", 0x018550b0);
}

// stub of CT3 0x0185aa50
void VistaVisionMain(void)
{
    ALYSSAPAN_STUB("VistaVisionMain", 0x0185aa50);
}

// The scene bank (a loaded BANK_TYPE_SHADOW group head) whose room is roomHash, or -1. drawn: also
// require DRAW_BANK_ON (WarpObjectUpdate's test; WarpObject's has none). The ROM inlines it in both.
static int WarpFindRoomBank(u_long roomHash, int drawn)
{
    int i;
    for (i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].state != BANK_STATE_LOADED)
            continue;
        if (BankInfo[i].type == BANK_TYPE_SHADOW && (!drawn || BankInfo[i].drawOn == DRAW_BANK_ON)
            && roomHash == GetRoomName_file(BankInfo[i].name))
            return i;
        i += BankInfo[i].groupCount - 1;
    }
    return -1;
}

static void WarpReservationClear(WARP_CMD *cmd)
{
    cmd->state = -1;
    cmd->mode = 0;
    cmd->chara = 0;
    cmd->bank = 0;
    cmd->part = 0;
    cmd->bank2 = 0;
    cmd->partHash = 0;
    cmd->roomHash = 0;
}

// CT3 0x0185bd50: each parked warp whose room's bank is now loaded and drawn is queued (by hash,
// through ExecuteWarpObject2) and its slot freed.
void WarpObjectUpdate(void)
{
    // callcheck: GetRoomName_file (in WarpFindRoomBank: the ROM inlines the bank search)
    int i, bank;

    for (i = 0; i < WARP_RESERVE_MAX; i++) {
        WARP_CMD *cmd = &ReservationWarpCommand[i];
        if (cmd->state < 0)
            continue;
        bank = WarpFindRoomBank(cmd->roomHash, 1);
        if (bank < 0)
            continue;
        cmd->bank2 = bank;
        cmd->part = -1;
        cmd->roomHash = cmd->partHash;
        asWarpObjectBody(WARP_QUEUE, cmd);
        WarpReservationClear(cmd);
    }
}

// CT3 0x019423a0 (ReservationWarpCommand), 0x0193cb30 / 34 / 38 (lpWC, iWCnum, iWCcnt: the queue).
WARP_CMD ReservationWarpCommand[WARP_RESERVE_MAX];
static WARP_CMD *lpWC;
static int iWCnum;
static int iWCcnt;

// CT3 0x0185bf80: no parked warps, an empty queue (the ROM unrolls the clear by eight; it drops the
// queue's block without freeing it).
void WarpObjectUpdateInit(void)
{
    int i;
    for (i = 0; i < WARP_RESERVE_MAX; i++)
        WarpReservationClear(&ReservationWarpCommand[i]);
    lpWC = NULL;
    iWCnum = 0;
    iWCcnt = 0;
}

// CT3 0x0185bec0: park a warp in the first free slot (none free: dropped).
void WarpObjectUpdateSet(const WARP_CMD *cmd)
{
    int i;
    for (i = 0; i < WARP_RESERVE_MAX; i++) {
        WARP_CMD *r = &ReservationWarpCommand[i];
        if (r->state < 0) {
            r->state = 1;
            r->mode = cmd->mode;
            r->chara = cmd->chara;
            r->bank = cmd->bank;
            r->roomHash = cmd->roomHash;
            r->partHash = cmd->partHash;
            r->part = -1;
            r->bank2 = 0;
            return;
        }
    }
}

// CT3 0x0185bb20: warp chara to the part partHash of room's scene bank. An empty room name queues
// the warp with partHash's low word as the part index and bank as given. A room not loaded parks it
// (and, for the player, the warps of characters 11 / 12 whose CHARACTER_FLAG_REFRACTION bit is set);
// a loaded one finds the part's index in the bank and queues it.
void WarpObject(const char *room, int mode, int chara, u_long partHash, int bank)
{
    // callcheck: GetRoomName_file (in WarpFindRoomBank: the ROM inlines the bank search)
    WARP_CMD cmd;
    const u_char *hdr;
    int i;

    memset(&cmd, 0, sizeof(cmd));
    cmd.mode = (short)mode;
    cmd.chara = (short)chara;
    cmd.bank = (short)bank;
    if (room[0] == 0) {
        cmd.bank2 = -1;
        cmd.part = (int)partHash;
        cmd.roomHash = 0;
        asWarpObjectBody(WARP_QUEUE, &cmd);
        return;
    }
    cmd.roomHash = ahGetHash(room);
    cmd.bank2 = WarpFindRoomBank(cmd.roomHash, 0);
    if (cmd.bank2 < 0) {
        cmd.part = -1;
        cmd.bank2 = -1;
        cmd.partHash = partHash;
        WarpObjectUpdateSet(&cmd);
        if (cmd.chara == 0) {
            if (Character[WARP_FOLLOWER_1].flags & CHARACTER_FLAG_REFRACTION)
                WarpObject(room, mode, WARP_FOLLOWER_1, partHash, bank);
            if (Character[WARP_FOLLOWER_2].flags & CHARACTER_FLAG_REFRACTION)
                WarpObject(room, mode, WARP_FOLLOWER_2, partHash, bank);
        }
        return;
    }
    cmd.roomHash = 0;
    cmd.bank = (short)cmd.bank2;
    cmd.part = -1;
    hdr = (const u_char *)pModelPartsHeader[cmd.bank2];
    for (i = 0; (s_char)hdr[PARTS_HDR_RECORDS + i * PARTS_REC_SIZE + 2] != PARTS_KIND_END; i++) {
        u_long h;
        memcpy(&h, hdr + PARTS_HDR_RECORDS + i * PARTS_REC_SIZE + PARTS_REC_HASH, sizeof(h));
        if (h == partHash) {
            cmd.part = i;
            break;
        }
    }
    asWarpObjectBody(WARP_QUEUE, &cmd);
}

// stub of CT3 0x0185b4c0
void ExecuteWarpObject(int mode, int chara, int part, int bank)
{
    ALYSSAPAN_STUB("ExecuteWarpObject", 0x0185b4c0);
}

// stub of CT3 0x0185ae90
void ExecuteWarpObject2(int mode, int bank, int chara, int bank2)
{
    ALYSSAPAN_STUB("ExecuteWarpObject2", 0x0185ae90);
}

// stub of CT3 0x01858c90
void XcmdScreenFilter(void)
{
    ALYSSAPAN_STUB("XcmdScreenFilter", 0x01858c90);
}

// stub of CT3 0x01858bc0
void XcmdScreenFilterSkip(void)
{
    ALYSSAPAN_STUB("XcmdScreenFilterSkip", 0x01858bc0);
}

// CT3 0x0185b980: WARP_QUEUE appends cmd (the block grows by WARP_QUEUE_GROW in gHeapJunc);
// WARP_EXECUTE runs every queued warp in order and frees the queue. Other modes do nothing.
void asWarpObjectBody(int mode, const WARP_CMD *cmd)
{
    int i;

    if ((char)mode == WARP_EXECUTE) {
        for (i = 0; i < iWCcnt; i++) {
            const WARP_CMD *w = &lpWC[i];
            if (w->roomHash == 0)
                ExecuteWarpObject(w->mode, w->chara, w->part, w->bank);
            else
                ExecuteWarpObject2(w->mode, w->bank, w->chara, w->bank2);
        }
        if (lpWC != NULL)
            _SunFree(&gHeapJunc, (void **)&lpWC);
        lpWC = NULL;
        iWCnum = 0;
        iWCcnt = 0;
    } else if ((char)mode == WARP_QUEUE) {
        if (iWCcnt >= iWCnum) {
            WARP_CMD *grown;
            iWCnum += WARP_QUEUE_GROW;
            grown = (WARP_CMD *)_SunAlloc(&gHeapJunc, iWCnum * (int)sizeof(WARP_CMD), 0x10);
            if (lpWC == NULL) {
                lpWC = grown;
            } else {
                for (i = 0; i < iWCcnt; i++)
                    grown[i] = lpWC[i];
                _SunFree(&gHeapJunc, (void **)&lpWC);
                lpWC = grown;
            }
        }
        lpWC[iWCcnt++] = *cmd;
    }
}

// stub of CT3 0x0185c100
void asWarpSceneObject(void *table, int maxId, float *pos)
{
    ALYSSAPAN_STUB("asWarpSceneObject", 0x0185c100);
}

// stub of CT3 0x01854d30
void ssMenuUpdate(int mode)
{
    ALYSSAPAN_STUB("ssMenuUpdate", 0x01854d30);
}

// ---- part lookup and name hashes (slice 1, 2026-11-10) ----

// CT3 0x0185e580
// Each character's low 7 bits, rotated left by 7 * i (mod 64), XORed in: the ROM builds the rotation
// from 32-bit halves in four shift ranges (< 25, < 32, < 58, and the wrap), which is a 64-bit rotate.
// A name of up to 9 characters never overlaps itself, so asConvertHash2String reads it back
// (PARTS_HASH_BIP01 is "Bip01", PARTS_HASH_BASE_POINT "BasePoint").
u_long ahGetHash(const char *name)
{
    u_long hash = 0;
    u_int shift = 0;

    if (name == NULL)
        return 0;
    for (; *name != '\0'; name++) {
        u_long c = (u_long)(*name & HASH_CHAR_MASK);
        hash ^= shift == 0 ? c : (c << shift | c >> (64 - shift));
        shift = (shift + HASH_CHAR_BITS) & 0x3f;
    }
    return hash;
}

// CT3 0x0193ce58 (szHash$257): 9 characters and the terminator.
static char szHash[10];

// CT3 0x0185e420
// The inverse for short names: nine 7-bit characters from the low end, terminated. (The ROM unrolls
// the first eight and writes each byte twice, zero first.)
char *asConvertHash2String(u_long hash)
{
    int i;
    for (i = 0; i < 9; i++) {
        szHash[i] = (char)(hash & HASH_CHAR_MASK);
        hash >>= HASH_CHAR_BITS;
    }
    szHash[9] = '\0';
    return szHash;
}

// CT3 0x0185e360: the global banks (0..5) keep their number; every scene / character bank answers 6.
int asCheckBankID(int bank)
{
    if (bank > GLOBAL_BANK_LAST)
        bank = GLOBAL_BANK_LAST + 1;
    return bank;
}

// CT3 0x0185e380: 1 for a scene / character bank (6..), 0 for a global one.
int asCheckBankType(int bank)
{
    return bank > GLOBAL_BANK_LAST;
}

// CT3 0x0185e150
// The index of the part whose hash matches, -1 when none (or no header). Four names have a cache word
// in the parts header (+0x10..+0x1c): 0 means not looked up yet, -1 not in this model, > 0 the index.
// A miss stores -1 and a hit its index in the cache of the name looked up (an index 0 is stored as 0,
// so part 0 is searched again each time: the ROM's).
int asFindModelPartsID(u_char *header, u_long hash)
{
    int cache = -1;             /* which cache word this name has; -1 none */
    u_int i, count;

    if (header == NULL)
        return -1;
    if (hash == PARTS_HASH_BIP01)
        cache = 0;
    else if (hash == PARTS_HASH_BASE_POINT)
        cache = 1;
    else if (hash == PARTS_HASH_CACHE2)
        cache = 2;
    else if (hash == PARTS_HASH_CACHE3)
        cache = 3;
    if (cache >= 0) {
        int known = *(int *)(header + PARTS_HDR_CACHE + cache * 4);
        if (known == -1)
            return -1;
        if (known > 0)
            return known;
    }

    count = *(u_short *)(header + PARTS_HDR_COUNT);
    for (i = 0; i < count; i++) {
        u_long partHash;
        memcpy(&partHash, header + PARTS_HDR_RECORDS + i * PARTS_REC_SIZE + PARTS_REC_HASH, sizeof(partHash));
        if (partHash == hash)
            break;
    }
    if (cache >= 0)
        *(int *)(header + PARTS_HDR_CACHE + cache * 4) = i < count ? (int)i : -1;
    return i < count ? (int)i : -1;
}

// CT3 0x0185deb0: answers its argument (a character's struct is its own handle).
void *asFindCharacterStruct(void *chara)
{
    return chara;
}

// stub of CT3 0x01858f20
int OneMaxObjectDrawZOnly(int bank, int index)
{
    ALYSSAPAN_STUB("OneMaxObjectDrawZOnly", 0x01858f20);
    return {};
}

// ---- the view matrix read back as a camera (slice 1, 2026-11-10, from the disassembly) ----

// CT3 0x0185a670: a focal length in the PS2's projection units (screen_0[8], 512) to a Leica's mm.
float asFocalLength_PS22LEICA(float f)
{
    return f * FOCAL_LEICA_MM / FOCAL_PS2_UNITS;
}

// CT3 0x0185a6b0: the inverse.
float asFocalLength_LEICA2PS2(float f)
{
    return f * FOCAL_PS2_UNITS / FOCAL_LEICA_MM;
}

// CT3 0x0185a8a0
// The view matrix's rotation (its translation row replaced by vf0, (0, 0, 0, 1)) inverted in place:
// its rows are the camera's right, up and front (each copied when asked for). pos = -(row 3).xyz with
// row 3's own w, multiplied through the inverse (all four lanes).
void asViewMatrix2PosRUF(float *view, float *pos, float *right, float *up, float *front)
{
    float inv[4][4];
    float v[4];
    int i;

    memcpy(inv, view, sizeof(inv));
    inv[3][0] = inv[3][1] = inv[3][2] = 0.0f;
    inv[3][3] = 1.0f;
    gaInversMatrix(inv, inv);
    if (right != NULL)
        memcpy(right, inv[0], sizeof(inv[0]));
    if (up != NULL)
        memcpy(up, inv[1], sizeof(inv[1]));
    if (front != NULL)
        memcpy(front, inv[2], sizeof(inv[2]));
    if (pos != NULL) {
        v[0] = -view[12];
        v[1] = -view[13];
        v[2] = -view[14];
        v[3] = view[15];
        for (i = 0; i < 4; i++)
            pos[i] = inv[0][i] * v[0] + inv[1][i] * v[1] + inv[2][i] * v[2] + inv[3][i] * v[3];
    }
}

// stub of CT3 0x01859520
void sunBGT_ExecTBufferData_VIF1(void *tbuffer)
{
    ALYSSAPAN_STUB("sunBGT_ExecTBufferData_VIF1", 0x01859520);
}

// stub of CT3 0x018590b0
void sunBGT_LoadGSTexClutData_VIF1(void *tbuffer)
{
    ALYSSAPAN_STUB("sunBGT_LoadGSTexClutData_VIF1", 0x018590b0);
}

// stub of CT3 0x01859530
void sunSetupTBufferData(void *tbuffer, TEX_INFO tex)
{
    ALYSSAPAN_STUB("sunSetupTBufferData", 0x01859530);
}

// CT3 0x01854560: gaInit3dlibWork with the copy built in: the 3dlib work (32 floats, ga3dlibwork) is
// copied to dst (the scratchpad's 0x400 in every caller), then the renderer's constants are derived
// from the copy, store for store as gaInit3dlibWork does (the ROM keeps dst's address at +0xec; the
// host keeps its EE address, as gaInit3dlibWork does).
void tnInit3dlibWork(void *work, void *dst)
{
    GA_SPR_WORK *spr = (GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET);
    float *w = (float *)dst;
    int i;

    spr->work3dlibEE = SCRATCHPAD_EE_BASE + (u_int)((u_char *)dst - g_eeScratchpad);
    for (i = 0; i < 32; i++)
        w[i] = ((const float *)work)[i];
    spr->viewScaleX = w[2] / w[0];
    spr->viewScaleY = w[3] / w[1];
    spr->zScale = 1.0f / (w[11] - w[10]);
    spr->work6 = w[6];
    spr->zOffset = ((w[11] + w[10]) * -0.5f) / (w[11] - w[10]);
    spr->work21 = w[21];
    spr->work22 = w[22];
    spr->work23x16 = w[23] * 16.0f;
    spr->zero06c = 0;
    spr->work26 = w[26];
    spr->work27 = w[27];
    spr->work6b = w[6];
    spr->work9 = w[9];
    spr->work25 = w[25];
    spr->colourMax = 255.0f;
    spr->work24 = w[24];
    // (the ROM ends with SYNC)
}

// CT3 0x0185a6f0
// As asViewMatrix2PosRUF, but pos is the camera's eye: the view origin moved back along front by
// three focal lengths in mm (asFocalLength_PS22LEICA(screen_0[8]) * -3); w stays the origin's.
void asViewMatrix2CamRUF(float (*view)[4], float *pos, float *right, float *up, float *forward)
{
    float origin[4], r[4], u[4], f[4];
    float back;

    asViewMatrix2PosRUF(&view[0][0], origin, r, u, f);
    if (right != NULL)
        memcpy(right, r, sizeof(r));
    if (up != NULL)
        memcpy(up, u, sizeof(u));
    if (forward != NULL)
        memcpy(forward, f, sizeof(f));
    if (pos != NULL) {
        back = asFocalLength_PS22LEICA(screen_0[SCREEN_PROJ_DIST]) * -CAMERA_EYE_FOCALS;
        pos[0] = origin[0] + f[0] * back;
        pos[1] = origin[1] + f[1] * back;
        pos[2] = origin[2] + f[2] * back;
        pos[3] = origin[3];
    }
}

// CT3 0x01a4b900 (scene_alpha_anim): one ambient-alpha animation per bank.
SCENE_ALPHA_ANIM scene_alpha_anim[BANK_MAX];

// CT3 0x01856e70
// Each part's alpha (MODEL_PARTS.alpha) starts as its ALL_TABLE.partsPos w, up to the record whose
// kind byte is -1; the bank's alpha animation is cleared.
void AmbientAlphaInit(int scene)
{
    u_char *header = pModelPartsHeader[scene];
    float (*partsPos)[4] = BANK_PTR(float (*)[4], scene, All_table_TBL[scene]->partsPos);
    MODEL_PARTS *part;
    int i;

    for (i = 0, part = (MODEL_PARTS *)header; (s_char)part->kind != PARTS_KIND_END; i++, part = (MODEL_PARTS *)((u_char *)part + PARTS_REC_SIZE))
        part->alpha = partsPos[i][3];
    memset(&scene_alpha_anim[scene], 0, sizeof(scene_alpha_anim[scene]));
}

// stub of CT3 0x01856070
void TexFileSetInit(int scene)
{
    ALYSSAPAN_STUB("TexFileSetInit", 0x01856070);
}

// stub of CT3 0x018532a0
void EventSoundEnd(void)
{
    ALYSSAPAN_STUB("EventSoundEnd", 0x018532a0);
}

// CT3 0x0185a650: the eye of matrix_global_view.
void CameraMatrix2CameraPos(float *pos)
{
    asViewMatrix2CamRUF(matrix_global_view, pos, NULL, NULL, NULL);
}

// CT3 0x0185a5e0: the point CAMERA_TARGET_DIST (300) ahead of matrix_global_view's eye; w the eye's.
void CameraMatrix2CameraTrg3m(float *trg)
{
    float pos[4], front[4];

    asViewMatrix2CamRUF(matrix_global_view, pos, NULL, NULL, front);
    trg[0] = pos[0] + front[0] * CAMERA_TARGET_DIST;
    trg[1] = pos[1] + front[1] * CAMERA_TARGET_DIST;
    trg[2] = pos[2] + front[2] * CAMERA_TARGET_DIST;
    trg[3] = pos[3];
}

// ---- banks and characters (slice 1, 2026-11-10) ----

// CT3 0x0185a400
// The first character whose shape bank, or one of whose ten behaviour banks, is `bank`; -1 none.
int BankID_TO_CharID(short bank)
{
    int i, j;
    for (i = 0; i < CHARACTER_MAX; i++) {
        if (Character[i].bank == bank)
            return i;
        for (j = 0; j < 10; j++)
            if (Character[i].behaviorBank[j] == bank)
                return i;
    }
    return -1;
}

// CT3 0x0185a480
// The bank (0..85) holding the character's room: a bank in use whose file name's first four
// characters hash to the character's roomHash (0 never matches); -1 none.
int CharID_TO_BankID(int chara)
{
    u_long room;
    int i;
    for (i = 0; i < BANK_MAX; i++) {
        room = Character[chara].roomHash;
        if (room == GetRoomName_file(BankInfo[i].name) && room != 0 && BankInfo[i].state != 0)
            return i;
    }
    return -1;
}

// CT3 0x019392f0 (RoomNameID_STR_Tbl) and 0x01939110 (RoomNameID_ID_Tbl, + -1): a room's area number.
static const char *const RoomNameID_STR_Tbl[AREA_TBL_COUNT] = {
    "0A01", "0A02", "0A03", "0A19", "0A01", "0A10", "0A11", "0A06", "0A07", "0A27",
    "1A02", "1A10", "1A11", "1C01", "1C02", "1C03", "1C01", "1C02", "1C04", "1C05",
    "1C06", "1A04", "1B01", "1B03", "1B04", "1B08", "1B11", "1B05", "1B07", "1B04",
    "1B08", "1B09", "1B11", "1B12", "1B08", "2A01", "2B01", "2B06", "2B02", "2B03",
    "2B04", "2B01", "2A02", "2B08", "2B07", "2B05", "2D01", "2D02", "2D03", "0B01",
    "0B19", "0B03", "0A16", "0B01", "0A10", "0A11", "0A07", "0A17", "0B06", "0B07",
    "0A23", "3A01", "3A02", "3C06", "3C05", "3C01", "3C02", "3C03", "3C07", "0A11",
    "0A10", "0A01", "0A01", "0A02", "3D03", "3D01", "3D02", "3E01", "3F01", "3D05",
    "4A08", "4A03", "4A02", "4A01", "4A04", "4A10", "4A09", "4A07", "4A04", "4A06",
    "4A12", "4A11", "4A05", "4A06", "4C01", "5A01", "4B05", "4B04", "4B06", "4B07",
    "4B08", "4N01", "4N02", "4B09", "4N03", "4B01", "4B10", "4B02", "4B03", "0A16",
    "0A30", "5A01", "5A03", "5A04", "5A02", "0A11", "0A10", "0A01",
};
static const int RoomNameID_ID_Tbl[AREA_TBL_COUNT + 1] = {
     1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  3,  4,  4,  5,  5,  5,  6,  6,  6,  6,
     6,  7,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9, 10, 10, 11, 12, 12, 12, 12,
    13, 13, 14, 14, 14, 14, 15, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18,
    18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 26, 26, 27, 27, 28, 29, 29, 30, 31, 31,
    32, 33, 34, 34, 35, 36, 36, 36, 36, 37, 38, 38, 38, 38, 39, 40, 41, 42, 43, 43,
    43, 43, 43, 44, 44, 45, 46, 46, 47, 47, 47, 48, 49, 50, 51, 52, 52, 52, -1,
};

// CT3 0x01852240: the area (1..52) the character's room belongs to, -1 when it is in no listed room
// (or in no bank: lightBank -1). Stages 3 and 4 override a few rooms of the home.
int GetAreaNumber(int chara)
{
    char room[16];
    int pass;
    int i;

    if (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0) == 3 && Character[chara].lightBank != -1) {
        strcpy(room, asConvertHash2String(Character[chara].roomHash));
        if (memcmp(room, "0A11", ROOM_NAME_LEN) == 0 || memcmp(room, "0A10", ROOM_NAME_LEN) == 0
            || memcmp(room, "0A01", ROOM_NAME_LEN) == 0)
            return AREA_STAGE3_HOME;
    }
    if (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0) == 4 && Character[chara].lightBank != -1) {
        strcpy(room, asConvertHash2String(Character[chara].roomHash));
        if (memcmp(room, "0A16", ROOM_NAME_LEN) == 0)
            return AREA_STAGE4_0A16;
    }
    pass = 0;
    if (Character[chara].lightBank != -1) {
        strcpy(room, asConvertHash2String(Character[chara].roomHash));
        for (i = 0; RoomNameID_ID_Tbl[i] != -1; i++) {
            if (memcmp(room, RoomNameID_STR_Tbl[i], ROOM_NAME_LEN) != 0)
                continue;
            // the ROM's test order: PASS2 skips a row unless one was skipped already; PASS3 then
            // skips one more unless two were
            if (ssTrigCheckEvent(TRIG_EVENT_AREA_PASS2, 1, 0) != 0 && pass != 1) {
                pass++;
                continue;
            }
            if (ssTrigCheckEvent(TRIG_EVENT_AREA_PASS3, 1, 0) != 0 && pass != 2) {
                pass++;
                continue;
            }
            return RoomNameID_ID_Tbl[i];
        }
    }
    return -1;
}

// CT3 0x0185c2e0: ahGetHash of the name's first four characters (a room's files share them).
u_long GetRoomName_file(const char *name)
{
    char room[ROOM_NAME_LEN + 1];
    memcpy(room, name, ROOM_NAME_LEN);
    room[ROOM_NAME_LEN] = '\0';
    return ahGetHash(room);
}

// CT3 0x0185e100: the part's record (header + 0x40 + i * 0x380), NULL when the model has none.
u_char *asFindModelParts(u_char *header, u_long hash)
{
    int id = asFindModelPartsID(header, hash);
    if (id < 0)
        return NULL;
    return header + PARTS_HDR_RECORDS + id * PARTS_REC_SIZE;
}

// stub of CT3 0x01854b20
int asJointParts(int srcBank, int bank, u_long hash)
{
    ALYSSAPAN_STUB("asJointParts", 0x01854b20);
    return {};
}

// stub of CT3 0x01854750
int asJointParts_byMatrix(float (*mat)[4], int *pos, int bank)
{
    ALYSSAPAN_STUB("asJointParts_byMatrix", 0x01854750);
    return {};
}

// stub of CT3 0x01859f00
void asGetCrossPointInPlaneWithLine(float out[4], float a[4], float b[4], float dir[4], float plane[4])
{
    ALYSSAPAN_STUB("asGetCrossPointInPlaneWithLine", 0x01859f00);
}

// CT3 0x018520b0
// A point light's range change in progress (REC_OMNI_CHANGING): the inverse range (REC_OMNI_INV_RANGE)
// moves 1 / steps of the way to 1 / REC_OMNI_RANGE each call; the last step lands on it and ends the
// change.
void OmniRangeChange(u_char *rec)
{
    int *changing = (int *)(rec + REC_OMNI_CHANGING);
    int *steps = (int *)(rec + REC_OMNI_STEPS);
    float range = *(float *)(rec + REC_OMNI_RANGE);
    float *inv = (float *)(rec + REC_OMNI_INV_RANGE);

    if (*changing == 0)
        return;
    if (*steps == 0) {
        *changing = 0;
        *inv = 1.0f / range;
    } else {
        *inv = *inv + (1.0f / range - *inv) / (float)*steps;
        (*steps)--;
    }
}

// ---- the rooms' trigger events (slice 1, 2026-11-10) ----

// CT3 0x01852130: every room's event removed.
void RoomFlagClear(void)
{
    int i;
    for (i = 0; RoomIdTbl[i] != -1; i++)
        ssTrigRemoveEvent(RoomIdTbl[i], 1, 0);
}

// CT3 0x01851f00: a jump to RoomFlagClear.
void BRANCH_CLEAR(void)
{
    RoomFlagClear();
}

// CT3 0x01852180: the event of the room the player character (Character[0]) is in, registered: its
// roomHash read back as a name, matched on four characters.
void RoomInFlagOn(void)
{
    char room[16];
    int i;

    strcpy(room, asConvertHash2String(Character[0].roomHash));
    for (i = 0; RoomIdTbl[i] != -1; i++) {
        if (memcmp(room, RoomNameTbl[i], ROOM_NAME_LEN) == 0) {
            ssTrigRegisterEvent(RoomIdTbl[i], 1, 0);
            return;
        }
    }
}

// CT3 0x0185e040
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 4): BuildMatrixTree indexes by it.
int absi(int v)
{
    return v < 0 ? -v : v;
}

// CT3 0x0185e390
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 5): Execute_ExecuteAnimationHistory calls it.
void AbsSignI32(int v, int *abs, int *sign)
{
    if (v < 0) {
        *abs = -v;
        *sign = -1;
    } else if (v > 0) {
        *abs = v;
        *sign = 1;
    } else {
        *abs = 0;
        *sign = 0;
    }
}

// CT3 0x0185d0d0
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 8): EA_100 calls it. sq zero, then
// each short into the high half of its float, 0x3f80 into w's.
void asZipVECTOR2VECTOR_XYZ1(float *out, const short *in)
{
    int i;
    for (i = 0; i < 3; i++) {
        u_int bits = (u_int)(u_short)in[i] << 16;
        memcpy(&out[i], &bits, sizeof(bits));
    }
    out[3] = 1.0f;
}

// ---- written early: the packed keys InterpolateAnimationKey reads (ahRelocateSunPs2ObjectArry.c
// slice 9, 2026-11-10; from the disassembly) ----

// vftoi0: truncate toward zero, saturating at the int range (the VU's clamp).
static int VuFtoi0(float f)
{
    if (f >= 2147483647.0f)
        return 0x7fffffff;
    if (f <= -2147483648.0f)
        return (int)0x80000000;
    return (int)f;
}

// CT3 0x0185c4d0
// n qwords (lq / sq), counted down from n.
void asMemCopy128c(void *dst, const void *src, u_int n)
{
    memmove(dst, src, (size_t)n * 16);
}

// CT3 0x0185c500
// n halfwords (lhu / sh).
void asMemCopy16c(void *dst, const void *src, u_int n)
{
    memmove(dst, src, (size_t)n * 2);
}

// CT3 0x0185c770
// q * ZIP_QUAT_SCALE (vmulx), vftoi0, each word's low half.
void asQuat2ZipQuat(short *out, const float *q)
{
    int i;
    for (i = 0; i < 4; i++)
        out[i] = (short)VuFtoi0(q[i] * ZIP_QUAT_SCALE);
}

// CT3 0x0185c700
// The packed quaternions unpacked, slerped by w ({1 - t, t}) and packed into out.
void asQuatSlerpZipVECTOR(short *out, const short *a, const short *b, float *w)
{
    alignas(16) float q[4], qa[4], qb[4];
    asZipQuat2Quat(qa, a);
    asZipQuat2Quat(qb, b);
    QuatSlerp(q, qa, qb, w);
    asQuat2ZipQuat(out, q);
}

// CT3 0x0185ce40
// v * (1 << shift) (vmulx), vftoi0, the low halves of xyz.
void asTrans2ZipTrans_XYZ(short *out, const float *v, u_int shift)
{
    float scale = (float)(1 << shift);
    int i;
    for (i = 0; i < 3; i++)
        out[i] = (short)VuFtoi0(v[i] * scale);
}

// CT3 0x0185cdb0
// The packed positions unpacked (XYZ1), blended by w (asInterpolateVector) and packed into out.
void asInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, u_int shift)
{
    alignas(16) float v[4], va[4], vb[4];
    asZipTrans2Trans_XYZ1(va, a, shift);
    asZipTrans2Trans_XYZ1(vb, b, shift);
    asInterpolateVector(v, va, vb, w);
    asTrans2ZipTrans_XYZ(out, v, shift);
}

// CT3 0x0185d100
// asZipVECTOR2VECTOR_XYZ1 with w 0 (sq zero, nothing stored over it).
void asZipVECTOR2VECTOR_XYZ0(float *out, const short *in)
{
    int i;
    for (i = 0; i < 3; i++) {
        u_int bits = (u_int)(u_short)in[i] << 16;
        memcpy(&out[i], &bits, sizeof(bits));
    }
    out[3] = 0.0f;
}

// CT3 0x0185d190
// The high halves of xyz (bfloat16).
void asVECTOR2ZipVECTOR_XYZ(short *out, const float *v)
{
    int i;
    for (i = 0; i < 3; i++) {
        u_int bits;
        memcpy(&bits, &v[i], sizeof(bits));
        out[i] = (short)(bits >> 16);
    }
}

// CT3 0x0185d120
// The packed vectors unpacked (XYZ0), blended by w and packed into out.
void asInterpolateZipVECTOR_XYZ(short *out, const short *a, const short *b, const float *w)
{
    alignas(16) float v[4], va[4], vb[4];
    asZipVECTOR2VECTOR_XYZ0(va, a);
    asZipVECTOR2VECTOR_XYZ0(vb, b);
    asInterpolateVector(v, va, vb, w);
    asVECTOR2ZipVECTOR_XYZ(out, v);
}

// CT3 0x0185d060
// u, v: the first two shorts / ZIP_UV_SCALE (div.s); the next two shorts as they are.
void asZipUV2UV(float *out, const short *in)
{
    out[0] = (float)in[0] / ZIP_UV_SCALE;
    out[1] = (float)in[1] / ZIP_UV_SCALE;
    out[2] = (float)in[2];
    out[3] = (float)in[3];
}

// CT3 0x0185d1b0
// a w[0] + b w[1]; a when a == b (c.eq.s).
float asInterpolateNumber(float a, float b, const float *w)
{
    if (a == b)
        return a;
    return a * w[0] + b * w[1];
}

// CT3 0x0185d330
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 8): EA_100 calls it. vsub into vf0 for
// the MAC zero flags: all four set leaves a; else vmulax / vmaddy.
void asInterpolateVector(float *out, const float *a, const float *b, const float *w)
{
    int i;
    float r[4];
    if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]) {
        memmove(out, a, sizeof(r));
        return;
    }
    for (i = 0; i < 4; i++)
        r[i] = a[i] * w[0] + b[i] * w[1];
    memcpy(out, r, sizeof(r));
}

// stub of CT3 0x0185d4d0
void asMakeWatchMeMatrixToPosition(float (*out)[4], float (*m)[4], float *target, int axis)
{
    ALYSSAPAN_STUB("asMakeWatchMeMatrixToPosition", 0x0185d4d0);
}

// ---- angles (slice 1, 2026-11-10; 0x10000 = a full turn) ----

// CT3 0x0190a600 (ah_acos_table): acos(i / 4096) for i = 0..4097, the ROM's floats.
static const u_int ah_acos_table_bits[AH_ACOS_TABLE_SIZE] = {
#include "game/ah_acos_table.inc"
};

static float ahAcosTable(int i)
{
    float f;
    memcpy(&f, &ah_acos_table_bits[i], sizeof(f));
    return f;
}

// CT3 0x01851dc0
// A negative cosine answers 0x8000 - acos(|c|). 0 is 0x4000, |c| >= 1 is 0 (or 0x8000); from
// AH_ACOS_NEAR_ONE the table is too coarse and acosf is called (on the signed value, masked to 16
// bits -- the ROM returns that directly, without the 0x8000 - step); below it the table is
// interpolated linearly at c * 4096.
int ahAcos(float c)
{
    int sign = 1, base = 0, angle, i;
    float x;

    if (c < 0.0f) {
        sign = -1;
        base = ANGLE_HALF_TURN;
        c = -c;
    }
    if (c == 0.0f)
        return ANGLE_QUARTER_TURN;
    if (c >= 1.0f) {
        angle = 0;
    } else if (c >= AH_ACOS_NEAR_ONE) {
        return (int)(ANGLE_FULL_TURN_F * acosf(c * (float)sign) / AH_TWO_PI) & 0xffff;
    } else {
        x = c * AH_ACOS_STEPS;
        i = (int)x;
        angle = (int)(ahAcosTable(i) + (x - (float)i) * (ahAcosTable(i + 1) - ahAcosTable(i)));
    }
    return base + sign * angle;
}

// CT3 0x01851d60: gaSinCos's second answer.
float ahCos(int angle)
{
    float s, c = 0.0f;
    gaSinCos(&s, &c, angle);
    return c;
}

// CT3 0x01851d90: gaSinCos's first answer.
float ahSin(int angle)
{
    float s = 0.0f, c;
    gaSinCos(&s, &c, angle);
    return s;
}

// CT3 0x0185ddf0: the rotation of angle (0x10000 a turn) about axis, through the unit quaternion.
void asMakeRotMatrixByQuartanion(float (*m)[4], float *axis, int angle)
{
    float q[4];
    float s, c;
    float inv;

    // VU0: xyz normalised by RSQRT (w keeps axis[3] until the cosine replaces it)
    inv = 1.0f / sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    q[0] = axis[0] * inv;
    q[1] = axis[1] * inv;
    q[2] = axis[2] * inv;
    gaSinCos(&s, &c, (int)((u_int)angle >> 1));
    q[0] *= s;
    q[1] *= s;
    q[2] *= s;
    q[3] = c;
    ConvertQuat2Matrix(m, q);
}

// VU0's 4-lane dot as the ROM sums it (ACC: w + z + y + x).
static float QuatDot(const float *a, const float *b)
{
    return a[3] * b[3] + a[2] * b[2] + a[1] * b[1] + a[0] * b[0];
}

// CT3 0x01851bf0: spherical blend of two unit quaternions, ts = {weight of q0, weight of q1} (1 - t,
// t). q0 is negated while the dot is negative (the short way round); nearly equal quaternions
// (dot >= 0.9999998) answer q1. ts[3] is overwritten with the angle as a float (the ROM's scratch).
void QuatSlerp(float *q, float *q0, float *q1, float *ts)
{
    float a[4];
    float dot, inv, s0, s1, len;
    int angle, i;

    for (i = 0; i < 4; i++)
        a[i] = q0[i];
    for (;;) {
        dot = QuatDot(a, q1);
        if (!(dot < 0.0f))
            break;
        for (i = 0; i < 4; i++)
            a[i] *= -1.0f;
    }
    if (!(dot < 0.9999998f)) {
        for (i = 0; i < 4; i++)
            q[i] = q1[i];
        return;
    }
    angle = ahAcos(dot);
    inv = 1.0f / ahSin(angle);
    ts[3] = (float)angle;
    // vmulw.xyz + vftoi0: each weight times the angle, truncated
    s0 = ahSin((int)(ts[0] * ts[3])) * inv;
    s1 = ahSin((int)(ts[1] * ts[3])) * inv;
    for (i = 0; i < 4; i++)
        q[i] = a[i] * s0 + q1[i] * s1;
    // VU0 RSQRT of the w + z + y + x sum of squares, all four lanes
    len = 1.0f / sqrtf(QuatDot(q, q));
    for (i = 0; i < 4; i++)
        q[i] *= len;
}

// stub of CT3 0x0185d380
int asDelSqueezeInformation(float (*out)[4], float (*a)[4], float (*b)[4])
{
    ALYSSAPAN_STUB("asDelSqueezeInformation", 0x0185d380);
    return {};
}

// CT3 0x0185c7d0
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 9; the stub carried 0x0185c6d0). The
// four shorts as floats, then vmulx by 1 / ZIP_QUAT_SCALE (0x38800000).
void asZipQuat2Quat(float *out, const short *in)
{
    int i;
    for (i = 0; i < 4; i++)
        out[i] = (float)in[i] * (1.0f / ZIP_QUAT_SCALE);
}

// CT3 0x0185ceb0
// Written early (slice 9; the stub carried 0x0185cdb0): xyz = the shorts * 1 / (1 << shift) (div.s,
// then vmulx), w = 1.
void asZipTrans2Trans_XYZ1(float *out, const short *in, u_int shift)
{
    float scale = 1.0f / (float)(1 << shift);
    int i;
    for (i = 0; i < 3; i++)
        out[i] = (float)in[i] * scale;
    out[3] = 1.0f;
}


// stub of CT3 0x0185c840
void asContInterpolateZipNormal_XYZ(short *out, const short *a, const short *b, const float *w, int n)
{
    ALYSSAPAN_STUB("asContInterpolateZipNormal_XYZ", 0x0185c840);
}

// stub of CT3 0x0185ca50
void asContInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, int n,
                                   u_int shift)
{
    ALYSSAPAN_STUB("asContInterpolateZipTrans_XYZ", 0x0185ca50);
}

// CT3 0x0185c5a0: a float to the 7.9 form (mul.s by 512, cvt.w.s truncates, the low halfword kept).
void asFloat2ZipFloat0_7_9X(short *out, const float *in)
{
    *out = (short)(int)(512.0f * *in);
}

// CT3 0x0185c530: both unpacked (unsigned), blended by asInterpolateNumber, packed again.
void asInterpolateZipFloat0_7_9X(short *out, const short *a, const short *b, const float *w)
{
    float fa, fb, f;
    asZipFloat0_7_9X2Float(&fa, (const u_short *)a);
    asZipFloat0_7_9X2Float(&fb, (const u_short *)b);
    f = asInterpolateNumber(fa, fb, w);
    asFloat2ZipFloat0_7_9X(out, &f);
}

// CT3 0x0185c6d0: a 1.1.14 fixed-point short to a float (div.s by 16384).
void asZipFloat1_1_14X2Float(float *out, const short *in)
{
    *out = (float)*in / ZIP_FLOAT_1_1_14_SCALE;
}

// CT3 0x0185c6a0: and back (mul.s, cvt.w.s truncates).
void asFloat2ZipFloat1_1_14X(short *out, const float *in)
{
    *out = (short)(int)(ZIP_FLOAT_1_1_14_SCALE * *in);
}

// CT3 0x0185c630: both unpacked, blended by asInterpolateNumber, packed again.
void asInterpolateZipFloat1_1_14X(short *out, const short *a, const short *b, const float *w)
{
    float fa, fb, f;
    asZipFloat1_1_14X2Float(&fa, a);
    asZipFloat1_1_14X2Float(&fb, b);
    f = asInterpolateNumber(fa, fb, w);
    asFloat2ZipFloat1_1_14X(out, &f);
}

// CT3 0x0185d020
// Not a packer: the first two floats (u, v) are copied into out's 8 bytes as they are (the ROM moves
// them as four halfwords), so what asInterpolateZipUV writes is a float pair.
void asUV2ZipUV_R(short *out, const float *uv)
{
    memcpy(out, uv, 2 * sizeof(float));
}

// CT3 0x0185cfb0: both packed UVs unpacked (asZipUV2UV), blended (asInterpolateVector), and the float
// u, v stored into out (asUV2ZipUV_R).
void asInterpolateZipUV(short *out, const short *a, const short *b, const float *w)
{
    float ua[4], ub[4], u[4];
    asZipUV2UV(ua, a);
    asZipUV2UV(ub, b);
    asInterpolateVector(u, ua, ub, w);
    asUV2ZipUV_R(out, u);
}

// stub of CT3 0x0185dc30
float asRadianByO(float *v, int axis)
{
    ALYSSAPAN_STUB("asRadianByO", 0x0185dc30);
    return {};
}


// CT3 0x0185ded0
// The *squared* distance: vsub.xyz, then gaInnerProductXYZ of the difference with itself (no root).
float asVectorDistance(const float *a, const float *b)
{
    float d[4];
    d[0] = a[0] - b[0];
    d[1] = a[1] - b[1];
    d[2] = a[2] - b[2];
    d[3] = a[3];
    return gaInnerProductXYZ(d, d);
}
