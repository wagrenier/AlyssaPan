// game/asMothOperate.h -- CT3 game/asMothOperate.c (symtab local chunk 84, 0x01882d60..0x01883ca0): the moths'
// state machine, run every frame by asCharacterOperate (wideXcmd_DebugPrint.c). MothInfo and the MOTH
// record (states, phases, arguments) are in asMothRouteSearch.h.
#pragma once

#include "sdk/scetypes.h"

// The trigger events and variables it reads (named from this use, 2026-11-12).
#define TRIG_EVENT_MOTHS_OUT 0x52       /* global: the moths are out (else the bank is parked out of sight) */
#define TRIG_EVENT_MOTHS_HIDE 0x257     /* global: park the moths; also lets the machine run while paused
                                           (= BRANCH_CLEAR.h's TRIG_EVENT_PANIC_ON_257) */
#define TRIG_EVENT_MOTHS_SPARE_3D4 0x3d4 /* global: with TRIG_VAR_STAGE 3, the moths stay out only while set */
#define TRIG_EVENT_MOTH_ON_ALYSSA 0xd1  /* registered when a moth lands on Alyssa (= tiAvoidObject.h's
                                           TRIG_EVENT_AI_APPEAR_NOW: the stalker appears at once) */
#define TRIG_VAR_MOTH_STATES 0x30       /* global: every moth's state after this frame, 4 bits each (moth i
                                           at bits (i & 7) * 4; 0 when it stays) */

#define MOTH_PARKED_Y 1.0e8f            /* 0x4cbebc20: part 0's drawOnPos y while the moths are parked */
#define MOTH_PAUSE_HOLD_FRAMES 2        /* paused for more than this: BANK_FLAG_ANIM_FORCE_STOP on the bank */
#define MOTH_RETURN_TICKS 54000         /* MOTH_STATE_WAIT: ticks after MothInfo.endTicks[stage] */
#define MOTH_FLY_SPEED 45               /* MOTH.speed once flying */
#define MOTH_LOOK_AHEAD 100.0f          /* 0x42c80000: the look part this far ahead of the body */
#define MOTH_ROUTE_REACHED 2500.0f      /* squared distance (50) at which a route point counts as reached */
#define MOTH_ALYSSA_HEIGHT 120.0f       /* MOTH_PHASE_SEEK: Alyssa's position + this in y */
#define MOTH_CIRCLE_RADIUS 50           /* MOTH.radius when it starts circling */
#define MOTH_CIRCLE_SWING 30            /* MOTH.swing */
#define MOTH_CIRCLE_STEP 0x400          /* MOTH.angle a frame */
#define MOTH_CIRCLE_TURN 16384.0f       /* ahSin(angle) * this: the circle's turn about Alyssa's facing */
#define MOTH_CIRCLE_NEAR 1.2f           /* on the circle: within radius^2 * this */
#define MOTH_CIRCLE_LAND_TICKS 600      /* nearTicks from here: it lands on Alyssa */
#define MOTH_CIRCLE_GIVE_UP_TICKS 3000  /* farTicks from here: back to its route */
#define MOTH_ON_ALYSSA_RISE 50.0f       /* the look2 part above Alyssa's part */
#define MOTH_SE_TICKS 120               /* timer: a flutter SE at most this often */
#define MOTH_HIT_TICKS 200              /* MOTH_STATE_HIT: then MOTH_STATE_DEAD */
#define MOTH_ROUTE_SHRINK 0.8f          /* 0x3f4ccccd: a blocked move is cut to this and tried again */
#define MOTH_ROUTE_STEP_AROUND 0x5ed0   /* asMothRouteSearch's stepAround / stepTilt */
#define MOTH_ROUTE_STEP_TILT 0x1555
// The rattle a landed moth opens (CLATTERING_SET): 120 frames, decay 10, 0x70, goal 10.
#define MOTH_RATTLE_TIME 0x78
#define MOTH_RATTLE_DECAY 10
#define MOTH_RATTLE_W08 0x70
#define MOTH_RATTLE_GOAL 10
// Alyssa's part the moths circle and land on (asFindModelParts; = CHARA_AXIS_HASH_ROW2, name unknown).
#define MOTH_ALYSSA_PART_HASH 0x61cb2103161c34f0ull
// Alyssa's motion record kind (MOTION_REC_KIND2) that shakes a landed moth off, unless her slot 0 motion
// is MOTH_SHAKE_NOT_MOTION.
#define MOTH_SHAKE_MOTION_KIND2 4
#define MOTH_SHAKE_NOT_MOTION 0xb3
// The SEs (SSPlaySE bank, number): Z_GA's 0 flutter while flying, 1 hit, 2 on Alyssa; C_0000's 0x3c when
// the rattle ends (won or lost) unless TRIG_EVENT_PANIC_ON_8A is set.
#define MOTH_SE_BANK "Z_GA"             /* rodata 0x01934430 (MOTH_SE_BANK_ARMED) */
#define MOTH_SE_FLUTTER 0
#define MOTH_SE_HIT 1
#define MOTH_SE_ON_ALYSSA 2
#define ALYSSA_SE_BANK "C_0000"         /* rodata 0x01934428 */
#define ALYSSA_SE_MOTH_OFF 0x3c

// Every frame: parks or shows the armed moth bank, steps each moth's state (route, circle, landing, hit)
// and moves its three parts, and publishes the states in TRIG_VAR_MOTH_STATES. Frees the moths when
// their bank is no longer the armed one, or Alyssa left the room of the first moth's route bank.
void asMothOperate(void);                                                                // CT3 0x01882d60
