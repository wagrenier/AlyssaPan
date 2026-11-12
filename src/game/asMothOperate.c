// game/asMothOperate.c -- Clock Tower 3: the moths' state machine (symtab local chunk 84, 0x01882d60..
// 0x01883ca0: asMothOperate and its static iPauseFrame$85). CodeWarrior C. Written 2026-11-12.
//
// A script arms a bank's parts as moths (asXcmd_I_have_x_Mothes: four numbers each, MOTH.arg) and gives
// each a route through another bank's parts (asXcmd_SetMoth). Each moth moves three parts of the moth
// bank -- the body and two look parts -- by their drawOnPos: it waits, flies its route, circles Alyssa
// when she is near, lands on her (a pad rattle shakes it off), and dies when an attack hits its hit shape.
// The VU0 macro code (vsub / vmul / vrsqrt) is done natively with the ROM's masks: xyzw adds and
// subtracts, xyz normalisation (w kept).

#include "game/asMothOperate.h"
#include "game/asMothRouteSearch.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ct3_types.h"
#include "game/LinkMatrixTree.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"

#include <math.h>
#include <string.h>

static void MothCopy(float *d, const float *a)
{
    d[0] = a[0];
    d[1] = a[1];
    d[2] = a[2];
    d[3] = a[3];
}

static void MothAdd(float *d, const float *a, const float *b)
{
    d[0] = a[0] + b[0];
    d[1] = a[1] + b[1];
    d[2] = a[2] + b[2];
    d[3] = a[3] + b[3];
}

static void MothSub(float *d, const float *a, const float *b)
{
    d[0] = a[0] - b[0];
    d[1] = a[1] - b[1];
    d[2] = a[2] - b[2];
    d[3] = a[3] - b[3];
}

static void MothScale(float *d, const float *a, float s)
{
    d[0] = a[0] * s;
    d[1] = a[1] * s;
    d[2] = a[2] * s;
    d[3] = a[3] * s;
}

// vmul.xyz / vaddy.x / vaddz.x / vrsqrt / vmulq.xyz: xyz to unit length, w kept.
static void MothNormalizeXYZ(float *v)
{
    float q = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
}

// Part `index` of a parts header (MODEL_PARTS is indexed from the header).
static MODEL_PARTS *MothPart(u_char *header, int index)
{
    return &((MODEL_PARTS *)header)[index];
}

// Sets a part's drawOnPos and marks it placed (PARTS_FLAG43_WARPED).
static void MothPlace(MODEL_PARTS *part, const float *pos)
{
    MothCopy(part->drawOnPos, pos);
    part->flags43 = PARTS_FLAG43_WARPED;
}

// Parks the whole moth bank (part 0) out of sight.
static void MothPark(MODEL_PARTS *root)
{
    root->drawOnPos[1] = MOTH_PARKED_Y;
    root->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
}

// PORT: CHARACTER +CHARA_MOTION_TABLE is an EE word of the character's bank (BankEePtr, as BRANCH_CLEAR.c's
// MotionTableOf); with no table (word 0) the record reads as not shaking, where the ROM would read address 0.
static int AlyssaShakesMothOff(void)
{
    const CHARACTER *ch = &Character[0];
    const u_char *table;
    u_int ee;
    short slot;

    memcpy(&ee, (const u_char *)ch + CHARA_MOTION_TABLE, sizeof(ee));
    memcpy(&slot, (const u_char *)ch + CHARA_MOTION_SLOTS, sizeof(slot));
    table = (const u_char *)BankEePtr(ch->bank, ee);
    if (table == NULL)
        return 0;
    return (s_char)table[slot * CHARA_MOTION_SIZE + MOTION_REC_KIND2] == MOTH_SHAKE_MOTION_KIND2 &&
           slot != MOTH_SHAKE_NOT_MOTION;
}

// A Z_GA SE at moth i's body, panned with Alyssa's listener every frame.
static void MothPlaySE(const MOTH *moth, int se, int i)
{
    int handle = SSPlaySE(MOTH_SE_BANK, se, i + 1);

    SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK | SD_SE_PARAM_CALLBACK, 0,
                 (int)(u_short)moth->arg[MOTH_ARG_BODY], (int)MothInfo.bank, SetPositionalParam_Alys);
    SetPositionalParam_Alys(handle);
}

// CT3 0x01882d60
void asMothOperate(void)
{
    static int iPauseFrame;             /* iPauseFrame$85 (0x0193ceb8): frames paused */
    BANK_INFO *bi = &BankInfo[MothInfo.bank];
    u_char *header;
    MODEL_PARTS *root;
    u_int hitsWord;
    u_int states;
    int i;

    if (MothInfo.paused == 0) {
        bi->flags &= ~BANK_FLAG_ANIM_FORCE_STOP;
        iPauseFrame = 0;
    } else {
        iPauseFrame++;
        if (iPauseFrame > MOTH_PAUSE_HOLD_FRAMES)
            bi->flags |= BANK_FLAG_ANIM_FORCE_STOP;
    }

    if (MothInfo.active == 0 || bi->drawOn <= 0)
        return;
    if (MothInfo.paused != 0 && ssTrigCheckEvent(TRIG_EVENT_MOTHS_HIDE, 1, 0) == 0)
        return;

    if (strcmp(bi->name, MothInfo.bankName) != 0 ||
        (MothInfo.list[0].bank != 0 && Character[0].lightBank != MothInfo.list[0].bank)) {
        asDestroyMothInfo(1);
        return;
    }

    header = pModelPartsHeader[MothInfo.bank];
    root = MothPart(header, 0);
    MothInfo.shown = (u_char)ssTrigCheckEvent(TRIG_EVENT_MOTHS_OUT, 1, 0);
    if (MothInfo.shown == 0 || ssTrigCheckEvent(TRIG_EVENT_MOTHS_ARMED, 1, 0) != 0 ||
        ssTrigCheckEvent(TRIG_EVENT_MOTHS_HIDE, 1, 0) != 0 ||
        (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0) == 3 && ssTrigCheckEvent(TRIG_EVENT_MOTHS_SPARE_3D4, 1, 0) == 0)) {
        MothPark(root);
        return;
    }

    root->drawOnPos[1] = 0.0f;
    root->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
    states = 0;
    hitsWord = All_table_TBL[MothInfo.bank]->hits;
    for (i = 0; i < MothInfo.count; i++) {
        MOTH *moth = &MothInfo.list[i];
        u_char *routeHdr = pModelPartsHeader[moth->bank];
        MODEL_PARTS *look = MothPart(header, (u_short)moth->arg[MOTH_ARG_LOOK]);
        MODEL_PARTS *body = MothPart(header, (u_short)moth->arg[MOTH_ARG_BODY]);
        MODEL_PARTS *look2 = MothPart(header, (u_short)moth->arg[MOTH_ARG_LOOK2]);
        const u_short *route = (const u_short *)moth->mem;
        float d[4];
        float dist;

        switch (moth->state) {
        case MOTH_STATE_REST:
            if (moth->next == MOTH_STATE_WAIT) {
                MothPlace(body, MothPart(routeHdr, (u_short)moth->home[0])->pos);
                MothPlace(look, MothPart(routeHdr, (u_short)moth->home[1])->pos);
                MothPlace(look2, MothPart(routeHdr, (u_short)moth->home[2])->pos);
                moth->frames = 0;
            }
            break;

        case MOTH_STATE_WAIT:
            if (moth->frames == 1 && MothInfo.endTicks[MothInfo.stage] + MOTH_RETURN_TICKS <= gTotalTicks) {
                moth->next = MOTH_STATE_FLY;
                moth->speed = MOTH_FLY_SPEED;
                moth->frames = 0;
                moth->routeStep = 0;
                moth->phase = MOTH_PHASE_AIM;
                MothInfo.endTicks[MothInfo.stage] = 0;
            } else {
                moth->frames = 0;
                moth->next = 0;
            }
            break;

        case MOTH_STATE_FLY: {
            const float *point;

            if (moth->frames == 1)
                moth->timer = MOTH_SE_TICKS;
            if (moth->phase != MOTH_PHASE_AWAY && moth->phase != MOTH_PHASE_SEEK) {
                if (moth->phase != MOTH_PHASE_AIM_AWAY && moth->phase != MOTH_PHASE_AIM)
                    break;
                // Turn the look parts towards the route point.
                MothSub(d, MothPart(routeHdr, route[(u_short)moth->routeStep])->pos, body->drawOnPos);
                MothNormalizeXYZ(d);
                MothScale(d, d, MOTH_LOOK_AHEAD);
                MothAdd(look->drawOnPos, body->drawOnPos, d);
                look->flags43 = PARTS_FLAG43_WARPED;
                MothPlace(look2, look->drawOnPos);
                moth->phase = moth->phase == MOTH_PHASE_AIM ? MOTH_PHASE_SEEK : MOTH_PHASE_AWAY;
            }
            point = MothPart(routeHdr, route[(u_short)moth->routeStep])->pos;
            MothCopy(moth->target, point);
            MothSub(d, moth->target, body->drawOnPos);
            dist = gaInnerProductXYZ(d, d);
            if (dist <= MOTH_ROUTE_REACHED) {
                moth->routeStep++;
                if ((u_short)moth->routeCount <= (u_short)moth->routeStep)
                    moth->routeStep = 0;
                moth->phase = MOTH_PHASE_AIM;
            }
            if (moth->phase == MOTH_PHASE_SEEK) {
                float alys[4];

                MothCopy(alys, (const float *)Character[0].pos);
                alys[1] += MOTH_ALYSSA_HEIGHT;
                MothSub(alys, body->drawOnPos, alys);
                if (gaInnerProductXYZ(alys, alys) <= dist && ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0) == 0) {
                    moth->next = MOTH_STATE_CIRCLE;
                    moth->angle = 0;
                    moth->radius = MOTH_CIRCLE_RADIUS;
                    moth->swing = MOTH_CIRCLE_SWING;
                    moth->frames = 0;
                }
            }
            moth->routeSearch = 1;
            break;
        }

        case MOTH_STATE_CIRCLE: {
            float m[4][4];
            float pos[4];
            const MODEL_PARTS *anchor;
            float s, c;

            if (moth->frames == 1) {
                moth->nearTicks = 0;
                moth->farTicks = 0;
            }
            anchor = (const MODEL_PARTS *)asFindModelParts(pModelPartsHeader[Character[0].bank],
                                                           MOTH_ALYSSA_PART_HASH);
            pos[0] = anchor->transInit[0];
            pos[1] = anchor->transInit[1];
            pos[2] = anchor->transInit[2];
            pos[3] = 1.0f;
            s = ahSin((u_short)moth->angle);
            gaSetRotMatrixY(m, Character[0].rot + (int)(s * MOTH_CIRCLE_TURN));
            MothScale(d, m[2], (float)(u_short)moth->radius);
            MothAdd(pos, pos, d);
            c = ahCos((u_short)moth->angle << 2);
            MothScale(d, m[1], ((float)(u_short)moth->swing * c) / 2.0f);
            MothAdd(moth->target, pos, d);
            moth->angle += MOTH_CIRCLE_STEP;
            MothSub(d, moth->target, body->drawOnPos);
            dist = gaInnerProductXYZ(d, d);
            if (dist <= (float)(int)((u_short)moth->radius * (u_short)moth->radius) * MOTH_CIRCLE_NEAR)
                moth->nearTicks += gGlobalNextTicksInterval;
            else
                moth->farTicks += gGlobalNextTicksInterval;
            if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0) != 0) {
                moth->next = MOTH_STATE_FLY;
                moth->frames = 0;
                moth->phase = MOTH_PHASE_AIM_AWAY;
            } else if (moth->nearTicks >= MOTH_CIRCLE_LAND_TICKS) {
                moth->next = MOTH_STATE_ON_ALYSSA;
                moth->frames = 0;
            } else if (moth->farTicks >= MOTH_CIRCLE_GIVE_UP_TICKS) {
                moth->next = MOTH_STATE_FLY;
                moth->frames = 0;
                moth->phase = MOTH_PHASE_AIM_AWAY;
            }
            moth->routeSearch = 1;
            break;
        }

        case MOTH_STATE_ON_ALYSSA: {
            int release = 0;
            int ended = 0;

            if (moth->frames == 1) {
                const MODEL_PARTS *anchor = (const MODEL_PARTS *)asFindModelParts(
                    pModelPartsHeader[Character[0].bank], MOTH_ALYSSA_PART_HASH);

                MothPlace(look, anchor->transInit);
                MothPlace(look2, anchor->transInit);
                look2->drawOnPos[1] += MOTH_ON_ALYSSA_RISE;
                if (MothInfo.clatter == -1) {
                    CLATTERING_SET set = {MOTH_RATTLE_TIME, MOTH_RATTLE_DECAY, MOTH_RATTLE_W08, MOTH_RATTLE_GOAL};

                    MothInfo.clatter = (short)ControllerClatteringSet(&set);
                }
                ssTrigRegisterEvent(TRIG_EVENT_MOTH_ON_ALYSSA, 1, 0);
                moth->timer = MOTH_SE_TICKS;
            }
            if (MothInfo.clatter == -1) {
                release = 1;
            } else if (ControllerClatteringGet(MothInfo.clatter) != CLATTERING_RUNNING) {
                release = 1;
                ended = 1;
            } else if (AlyssaShakesMothOff()) {
                release = 1;
                if (MothInfo.clatter != -1)
                    ControllerClatteringFree(MothInfo.clatter);
            }
            if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0) != 0) {
                release = 1;
                if (MothInfo.clatter != -1)
                    ControllerClatteringFree(MothInfo.clatter);
            }
            if (release) {
                MothInfo.clatter = -1;
                moth->next = MOTH_STATE_FLY;
                moth->frames = 0;
                moth->phase = MOTH_PHASE_AIM_AWAY;
                if (ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_8A, 1, 0) == 0 && ended)
                    SSPlaySE(ALYSSA_SE_BANK, ALYSSA_SE_MOTH_OFF, 1);
            } else {
                asAlyssaLockedByAnything(0);
            }
            if (moth->timer >= MOTH_SE_TICKS) {
                MothPlaySE(moth, MOTH_SE_ON_ALYSSA, i);
                moth->timer = 0;
            }
            break;
        }

        case MOTH_STATE_HIT:
            if (moth->frames == 1) {
                MothPlaySE(moth, MOTH_SE_HIT, i);
                moth->timer = 0;
            }
            if (moth->timer >= MOTH_HIT_TICKS) {
                moth->next = MOTH_STATE_DEAD;
                moth->frames = 0;
                moth->phase = 0;
            }
            break;
        }

        if (moth->state == MOTH_STATE_CIRCLE || moth->state == MOTH_STATE_FLY) {
            if (moth->routeSearch != 0) {
                float to[4], dir[4], out[4], step[4];
                float f;

                // The move: towards target, cut short until the line clears the route bank's collision.
                MothCopy(to, moth->target);
                MothSub(dir, to, body->drawOnPos);
                while (asMothRouteSearch(moth->bank, body->drawOnPos, to, out, MOTH_ROUTE_STEP_AROUND,
                                         MOTH_ROUTE_STEP_TILT) != 0) {
                    MothScale(dir, dir, MOTH_ROUTE_SHRINK);
                    MothAdd(to, dir, body->drawOnPos);
                }
                MothSub(dir, out, body->drawOnPos);
                MothNormalizeXYZ(dir);
                f = gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
                MothScale(step, dir, ((float)anim_play_slow * (((float)(u_short)moth->speed * f) / 10.0f)) / 100.0f);
                MothAdd(body->drawOnPos, body->drawOnPos, step);
                body->flags43 = PARTS_FLAG43_WARPED;
                MothAdd(look->drawOnPos, out, step);
                look->flags43 = PARTS_FLAG43_WARPED;
                MothPlace(look2, look->drawOnPos);
                look2->drawOnPos[1] = body->drawOnPos[1];
                if (moth->timer >= MOTH_SE_TICKS && tnRand(2) == 0) {
                    MothPlaySE(moth, MOTH_SE_FLUTTER, i);
                    moth->timer = 0;
                }
            }
        } else if (moth->state == MOTH_STATE_WAIT || moth->state == MOTH_STATE_REST) {
            MothPark(root);
        }
        moth->routeSearch = 0;

        // An attack on its hit shape this frame kills it.
        if (hitsWord != 0 && moth->state != MOTH_STATE_HIT && moth->state != MOTH_STATE_DEAD) {
            const HIT_TABLE *hits = BANK_PTR(const HIT_TABLE *, MothInfo.bank, hitsWord);
            const HIT_ATK_REC *recs = BANK_PTR(const HIT_ATK_REC *, MothInfo.bank, hits->scnHits);
            int j;

            for (j = 0; j < hits->w2b8; j++) {
                const HIT_DATA *data = BANK_PTR(const HIT_DATA *, MothInfo.bank, recs[j].object);

                if ((int)(u_short)moth->arg[MOTH_ARG_HIT] == (int)data->part) {
                    moth->next = MOTH_STATE_HIT;
                    moth->frames = 0;
                    moth->phase = 0;
                    moth->timer = 0;
                }
            }
        }

        states |= (u_int)moth->next << ((i & 7) << 2);
        if (moth->next != 0)
            moth->state = moth->next;
        moth->next = 0;
        moth->timer += gGlobalNextTicksInterval;
        moth->frames++;
        if (moth->frames == -1)
            moth->frames = -2;
    }
    ssTrigSetVariable(TRIG_VAR_MOTH_STATES, (int)states, 1, 0);
}
