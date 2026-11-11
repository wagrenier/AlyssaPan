// ga/pad_button_read.c -- Clock Tower 3, the ga library's controller reader (CT3 0x018a0880..0x018a1188).
// Reconstructed 2026-11-11. The object's extent comes from the symtab: its unbroken local run (indices
// 1607..1618: id, rdata$185, term_id, pad_button_read, exid, shock_data, rterm_id, pad_dma_buf0/1,
// act_align, phase, act_direct) holds gaPadInit's two DMA buffers too, and closes at the next GCC
// object's .text (0x018a1188), so gaPadInit is this file's tail (ga/gaPadInit.c, retired this session).
//
// gaPadRead is libpad's sample detection walk, per port: find the controller id, lock a DualShock 2 in
// analog mode, align its actuators, enter pressure mode, then read every frame -- buttons, sticks
// (mirrored about 0x80), pressure bytes -- and derive trg (pressed this frame) and key (key repeat).
// Host: sdk/libpad.cpp answers the walk as a DualShock 2 on port 0, port 1 empty.

#include "ga/pad_button_read.h"

#include "game/main.h"
#include "sdk/libpad.h"

#define GA_PAD_SLOT 0          /* no multitap: slot 0 of each port */
#define GA_PAD_MODE_DEFAULT 0  /* scePadInit's mode: the only one libpad defines */

/* scePadInfoMode's controller ids */
#define PAD_ID_DIGITAL 4
#define PAD_ID_DUALSHOCK2 7

/* rdata[0]: the read's status (0 = a good report); rdata[1]: high nibble the terminal id, low nibble the
   report's length in halfwords. */
#define PAD_RDATA_OK 0
#define PAD_RDATA_BUTTONS 2   /* two bytes, big-endian, active low */
#define PAD_RDATA_STICK 4     /* four bytes: right X, right Y, left X, left Y */
#define PAD_RDATA_PRESS 8     /* twelve pressure bytes */
#define PAD_RDATA_SIZE 32
#define PAD_STICK_CENTRE 0x80

/* gaPadRead's mode GA_PAD_READ_GAME: the left stick past these presses the d-pad. */
#define PAD_STICK_DPAD_LO 0x20
#define PAD_STICK_DPAD_HI 0xe0

/* key repeat (PAD_DATA.repeat): the first repeat after 15 frames held, then every 3rd frame */
#define PAD_REPEAT_DELAY 15
#define PAD_REPEAT_RATE 0x12

/* phase[]: where the detection walk stands for each port (libpad's sample numbers them so) */
#define PAD_PHASE_DETECT 0          /* read the controller id */
#define PAD_PHASE_DIGITAL_EXID 40   /* a digital id: does the pad have an analog mode? */
#define PAD_PHASE_DIGITAL_LOCK 41   /* switch it to analog and lock the mode */
#define PAD_PHASE_DIGITAL_WAIT 42   /* wait for the switch, then detect again */
#define PAD_PHASE_ACT_ALIGN 70      /* a DualShock 2: align the actuators */
#define PAD_PHASE_ACT_WAIT 71       /* no pressure mode: wait for the align */
#define PAD_PHASE_PRESS_ENTER 76    /* enter pressure mode */
#define PAD_PHASE_PRESS_WAIT 77     /* wait for pressure mode */
#define PAD_PHASE_READY 99          /* read */

#define PAD_MAIN_MODE_ANALOG 1      /* scePadSetMainMode: offs 1 = analog */
#define PAD_MAIN_MODE_LOCK 3        /* lock: the analog button cannot switch it back */
#define PAD_ACT_ALL (-1)            /* scePadInfoAct: the actuator count */

/* act_align / act_direct: actuator 0 (the small motor, on / off) at byte 0, actuator 1 (the large motor,
   level 0..255) at byte 1, the rest unused (0xff). */
#define PAD_ACT_BYTES 6
#define PAD_ACT_NONE 0xff

/* shock_data: one vibration request for whichever pad gaPadRead drives next. */
#define SHOCK_ON 0      /* 1 while a request stands */
#define SHOCK_FRAMES 1  /* frames left (0xff on a request) */
#define SHOCK_SMALL 2   /* the small motor: 1 = on */
#define SHOCK_LARGE 3   /* the large motor's level; 0 stops the request after one frame */
#define SHOCK_SET 4     /* 1 while a request stands (never read here) */
#define SHOCK_BYTES 5
#define SHOCK_FRAMES_MAX 0xff

static int id[GA_PAD_PORTS];                  /* bss 0x0193e450: the controller id per port */
static int exid[GA_PAD_PORTS];                /* bss 0x0193e448: the extended id per port */
static int phase[GA_PAD_PORTS];               /* bss 0x0193e458: PAD_PHASE_* per port */
static int term_id[GA_PAD_PORTS];             /* data 0x019398b8: this frame's terminal id per port */
static int rterm_id[GA_PAD_PORTS];            /* data 0x019398c0: last frame's terminal id per port */
static u_char shock_data[SHOCK_BYTES];        /* bss 0x0193e440: SHOCK_* */
alignas(16) static u_char act_align[PAD_ACT_BYTES];  /* bss 0x0193e460 */
alignas(8) static u_char act_direct[PAD_ACT_BYTES];  /* bss 0x0193e468 */

// libpad DMAs each port's state into its buffer: scePadDmaBufferMax qwords, 64-byte aligned on the EE.
alignas(64) static u_long128 pad_dma_buf0[scePadDmaBufferMax]; /* bss 0x0193e580, port 0 */
alignas(64) static u_long128 pad_dma_buf1[scePadDmaBufferMax]; /* bss 0x0193e480, port 1 */

// CT3 0x018a0880: reads one port's report into pad (buttons, then the sticks of a DualShock 2, then the
// pressure bytes when pressure mode is on, else zeros); returns the terminal id (rdata[1]'s high nibble),
// 0 when libpad has no report or the report's status is bad (pad left as it was).
static int pad_button_read(int port, int slot, PAD_DATA *pad)
{
    static u_char rdata[PAD_RDATA_SIZE]; /* bss 0x0193e420 (rdata$185) */
    int i;

    if (scePadRead(port, slot, rdata) == 0)
        return 0;
    if (rdata[0] != PAD_RDATA_OK)
        return 0;

    pad->cnt = (u_short)~(rdata[PAD_RDATA_BUTTONS] << 8 | rdata[PAD_RDATA_BUTTONS + 1]);
    if (id[port] == PAD_ID_DUALSHOCK2) {
        /* mirrored about the centre: 0x00 -> 0xff, 0x7f / 0x80 -> 0x80, 0xff -> 0x01 */
        for (i = 0; i < PAD_STICK_COUNT; i++) {
            u_char v = rdata[PAD_RDATA_STICK + i];
            pad->stick[i] = v < PAD_STICK_CENTRE ? (u_char)(0xff - v) : (u_char)(0x100 - v);
        }
    }
    if (pad->press_mode == 0) {
        for (i = 0; i < PAD_PRESS_COUNT; i++)
            pad->press[i] = 0;
    } else {
        for (i = 0; i < PAD_PRESS_COUNT; i++)
            pad->press[i] = rdata[PAD_RDATA_PRESS + i];
    }
    return rdata[1] >> 4;
}

// CT3 0x018a0ac0: asks gaPadRead to run the motors (small == 1: the small one; large: the large one's
// level); both off clears the request. The request is one for every pad: only pad's flag says which reads
// it.
void gaPadSetVibration(PAD_DATA *pad, u_char small, u_char large)
{
    shock_data[SHOCK_SMALL] = small == 1;
    pad->vibration = 1;
    if (shock_data[SHOCK_SMALL] || large != 0) {
        shock_data[SHOCK_FRAMES] = SHOCK_FRAMES_MAX;
        shock_data[SHOCK_ON] = 1;
        shock_data[SHOCK_SET] = 1;
        shock_data[SHOCK_LARGE] = large;
    } else {
        shock_data[SHOCK_ON] = 0;
        shock_data[SHOCK_FRAMES] = 0;
        shock_data[SHOCK_SMALL] = 0;
        shock_data[SHOCK_LARGE] = 0;
        shock_data[SHOCK_SET] = 0;
    }
}

// CT3 0x018a0b40: one frame of both ports: the detection walk (one step per frame) until a port is
// PAD_PHASE_READY, then its report, trg (pressed this frame), key (key repeat) and vibration. Port 0
// unplugged sets CntDrawnOut (main clears the pad and forces the pause trigger). mode: GA_PAD_READ_*.
void gaPadRead(PAD_DATA *pad, int mode)
{
    int port;

    for (port = 0; port < GA_PAD_PORTS; port++) {
        PAD_DATA *p = &pad[port];
        int state;
        u_short last;

        p->stick[PAD_STICK_LX] = PAD_STICK_CENTRE;
        p->stick[PAD_STICK_LY] = PAD_STICK_CENTRE;
        p->stick[PAD_STICK_RX] = PAD_STICK_CENTRE;
        p->stick[PAD_STICK_RY] = PAD_STICK_CENTRE;

        state = scePadGetState(port, GA_PAD_SLOT);
        if (state == scePadStateDiscon)
            phase[port] = PAD_PHASE_DETECT;

        switch (phase[port]) {
        case PAD_PHASE_DETECT:
            p->press_mode = 0;
            if (port == GA_PAD_PORT_0)
                CntDrawnOut = state == scePadStateDiscon;
            if (state == scePadStateDiscon)
                break;
            id[port] = scePadInfoMode(port, GA_PAD_SLOT, InfoModeCurID, 0);
            if (id[port] == 0)
                break;
            exid[port] = scePadInfoMode(port, GA_PAD_SLOT, InfoModeCurExID, 0);
            if (exid[port] > 0)
                id[port] = exid[port];
            p->id = (u_char)id[port];
            if (id[port] == PAD_ID_DUALSHOCK2)
                phase[port] = PAD_PHASE_ACT_ALIGN;
            else if (id[port] == PAD_ID_DIGITAL)
                phase[port] = PAD_PHASE_DIGITAL_EXID;
            break;

        case PAD_PHASE_DIGITAL_EXID:
            if (scePadInfoMode(port, GA_PAD_SLOT, InfoModeCurExID, 0) == 0) {
                phase[port] = PAD_PHASE_READY;
                break;
            }
            phase[port]++;
            /* fall through */
        case PAD_PHASE_DIGITAL_LOCK:
            if (scePadSetMainMode(port, GA_PAD_SLOT, PAD_MAIN_MODE_ANALOG, PAD_MAIN_MODE_LOCK) == 1)
                phase[port]++;
            break;

        case PAD_PHASE_DIGITAL_WAIT:
            if (scePadGetState(port, GA_PAD_SLOT) != scePadStateExecCmd)
                phase[port] = PAD_PHASE_DETECT;
            break;

        case PAD_PHASE_ACT_ALIGN:
            if (scePadInfoAct(port, GA_PAD_SLOT, PAD_ACT_ALL, 0) == 0)
                phase[port] = PAD_PHASE_READY;
            act_align[0] = 0;
            act_align[1] = 1;
            act_align[2] = PAD_ACT_NONE;
            act_align[3] = PAD_ACT_NONE;
            act_align[4] = PAD_ACT_NONE;
            act_align[5] = PAD_ACT_NONE;
            if (scePadSetActAlign(port, GA_PAD_SLOT, act_align) != 0) {
                if (scePadInfoPressMode(port, GA_PAD_SLOT) == 1)
                    phase[port] = PAD_PHASE_PRESS_ENTER;
                else
                    phase[port]++;
            }
            break;

        case PAD_PHASE_ACT_WAIT:
            if (scePadGetState(port, GA_PAD_SLOT) != scePadStateExecCmd)
                phase[port] = PAD_PHASE_READY;
            break;

        case PAD_PHASE_PRESS_ENTER:
            if (scePadEnterPressMode(port, GA_PAD_SLOT) == 1)
                phase[port]++;
            break;

        case PAD_PHASE_PRESS_WAIT:
            if (scePadGetReqState(port, GA_PAD_SLOT) == scePadReqStateFailed)
                phase[port]--;
            if (scePadGetReqState(port, GA_PAD_SLOT) == scePadReqStateComplete) {
                p->press_mode = 1;
                phase[port] = PAD_PHASE_READY;
            }
            /* fall through: this frame is read too */
        default:
            last = p->cnt;
            if (state != scePadStateStable && state != scePadStateFindCTP1)
                break;
            term_id[port] = pad_button_read(port, GA_PAD_SLOT, p);
            if (term_id[port] == 0)
                break;
            if (rterm_id[port] != 0 && term_id[port] != rterm_id[port]) {
                /* another kind of controller in the port: detect it again */
                rterm_id[port] = 0;
                phase[port] = PAD_PHASE_DETECT;
            }
            if (id[port] == PAD_ID_DUALSHOCK2) {
                if (mode == GA_PAD_READ_GAME) {
                    if (p->stick[PAD_STICK_LY] > PAD_STICK_DPAD_HI)
                        p->cnt |= SCE_PADLup;
                    if (p->stick[PAD_STICK_LX] < PAD_STICK_DPAD_LO)
                        p->cnt |= SCE_PADLright;
                    if (p->stick[PAD_STICK_LY] < PAD_STICK_DPAD_LO)
                        p->cnt |= SCE_PADLdown;
                    if (p->stick[PAD_STICK_LX] > PAD_STICK_DPAD_HI)
                        p->cnt |= SCE_PADLleft;
                }
                if (p->vibration != 0) {
                    /* one frame of the request, or the motors off once its frames are spent */
                    if (shock_data[SHOCK_FRAMES] == 0) {
                        p->vibration = 0;
                        act_direct[0] = 0;
                        act_direct[1] = 0;
                        scePadSetActDirect(port, GA_PAD_SLOT, act_direct);
                    } else {
                        shock_data[SHOCK_FRAMES]--;
                        act_direct[0] = shock_data[SHOCK_SMALL];
                        act_direct[1] = shock_data[SHOCK_LARGE];
                        if (shock_data[SHOCK_LARGE] == 0)
                            shock_data[SHOCK_FRAMES] = 0;
                        scePadSetActDirect(port, GA_PAD_SLOT, act_direct);
                    }
                }
            }
            if (last == p->cnt) {
                p->key = 0;
                if (p->repeat < PAD_REPEAT_DELAY) {
                    p->key = 0;
                    p->repeat++;
                } else if (mode == GA_PAD_READ_GAME) {
                    if (p->repeat < PAD_REPEAT_RATE) {
                        p->key = 0;
                        p->repeat++;
                    } else {
                        p->key = p->cnt;
                        p->repeat = PAD_REPEAT_DELAY;
                    }
                } else {
                    p->key = p->cnt;
                }
            } else {
                p->repeat = 0;
                p->key = p->cnt;
            }
            p->trg = p->cnt ^ (p->old & p->cnt);
            p->old = p->cnt;
            rterm_id[port] = term_id[port];
            break;
        }
    }
}

// CT3 0x018a1130: starts libpad and opens both controller ports; 1 when both opened, 0 when either
// failed (port 1 is not tried once port 0 fails). The caller (IOPInit) ignores the result.
int gaPadInit(void)
{
    scePadInit(GA_PAD_MODE_DEFAULT);
    if (scePadPortOpen(GA_PAD_PORT_0, GA_PAD_SLOT, pad_dma_buf0) == 0)
        return 0;
    return scePadPortOpen(GA_PAD_PORT_1, GA_PAD_SLOT, pad_dma_buf1) != 0;
}
