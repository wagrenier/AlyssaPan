// game/Character_Init.c -- Clock Tower 3: the character table's reset, the IOP's boot, and the memory
// card (CT3 0x018b3a60..0x018bb610, the symtab's local chunk 105). Slice 1 reconstructed 2026-11-09:
// Character_All_Init, IOPInit, McInit, the work buffer (mc_alloc / mc_free / mc_save_data_init) and the
// card-present checks (mc_check, mc_check_always, MemorycardAutoLoadInsertedCheck3).
// Slice 2 (2026-11-10): MemorycardUpdate's flows and the format checks. Slice 3 (2026-11-10): the save
// path (mc_save_new, mc_save_normal, mc_save_set) and the system data (system_data_load_set, MC_HEADER).
//
// The file name is the tool's (the first keyed function's); the memory card code that fills most of the
// object (MemorycardUpdate, mc_load_new, mc_save_*, the icon "data/mc_icon/", "/BASLUS-20633") is still to
// come, so a later slice may rename it.

#include "common/alyssapan_stub.h"
#include "game/Character_Init.h"

#include "ga/gaInit3dlib.h"
#include "ga/gaPadInit.h"
#include "ga/gaSearchNearLight4.h"
#include "ga/gaInitInterrupt.h"
#include "game/ArtoolDebugOpen.h"
#include "game/Clipping_Portal.h"
#include "game/ssGrpPortalCheck.h"
#include "game/QuatSlerp.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ahalloc.h"
#include "game/LinkMatrixTree.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/asMothPause.h"
#include "game/tiAvoidObject.h"
#include "game/tkXDTrg_GetSceneHash.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/movie.h"
#include "game/sound.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "sdk/libgraph.h"
#include "sdk/libmc.h"

#include <stdio.h>
#include <string.h>

// ---- data ----

// Char_Init_MAX_HP (.data 0x01939000, 164 bytes): each Character[] slot's starting hit points. The ROM's
// values; the 41 words outlast the 35 slots (the rest are zero).
int Char_Init_MAX_HP[CHAR_INIT_MAX_HP_COUNT] = {
    800, 200, 500, 400, 500, 600, 700, 800, 300, 300, 300,
};

// The statics, as the symtab places them (.sbss 0x0193ef78..0x0193f0bc).
static int alloc_flag;                  /* 0x0193ef80: mc_alloc's buffer is held */
static int fream_counter;               /* 0x0193ef84: mc_check_always's frame count (sic) */
static int SlotErr[MC_SLOT_COUNT];      /* 0x0193efb0: per slot, set when mc_check3 finds no card */
static u_char *mc_databuff[MC_DATA_COUNT]; /* 0x0193f080: the save images in mc_data_buff */
static u_char *mc_header;               /* 0x0193f0a0 */
static u_char *mc_master;               /* 0x0193f0b0 */
static u_char *mc_data_buff;            /* 0x0193f0b4: the SdAlloc block */
static int mc_save_normal_num;          /* 0x0193ef90: mc_save_normal's step (mc_load_new bumps it too) */
static int mc_load_new_fd;              /* 0x0193efa4 */
static int mc_load_new_num;             /* 0x0193efa8: mc_load_new's step */
static int mc_load_new_end_size;        /* 0x0193efc4 */
static int mc_load_new_loop_count;      /* 0x0193efc8 */
static int mc_load_new_loop_count_max;  /* 0x0193efcc */
static int mc_broke_data[MC_BROKE_COUNT]; /* 0x0193f050 */
static int AA_New_McAlloc_Mode;         /* 0x0193ef74: != 0 keeps the buffer between flows (Set_AA_New_McAlloc_Mode) */
static int DrawWaitCounter;             /* 0x0193efb8: MemorycardAutoLoadInit: MC_AUTO_LOAD_DRAW_WAIT */
static int FadeOutCounter;              /* 0x0193efbc: MemorycardAutoLoad's frame count */
static int mc_menuid;                   /* 0x0193ef7c: a flow's sub-step */
static int AA_Phase_Step;               /* 0x0193ef88: the menus' format-check / save sub-step */
static int AA_UnFormatCk;               /* AA_UnFormatCk$1304, 0x0193efc0: the first format check said unformatted */
static int mc_type;                     /* 0x0193f0b8: 0 = the card holds the save, 1 = a new one is made */

// .data (0x01939aa0..0x01939b00), with the ROM's initial values.
static int AA_count = 1;                /* 0x01939aa0: vblanks left before the next card check */
static int mc_cursor[MC_SLOT_COUNT] = {MC_FILE_NONE, MC_FILE_NONE}; /* 0x01939af0: the file last used per slot */
static u_int mc_uidwork[MC_SLOT_COUNT]; /* 0x01939af8: that file's header uid per slot */

// @2405 (.rodata 0x01935150): the card SEs' bank. @2406 (0x01935160): MemorycardUpdate's debug line.
static const char s_seBank[] = "C_0000";
static const char s_check4Format[] = "%d=mc_check4()\n";

// mc_fname (.data 0x01939ab0, 64 bytes): the save's directory and file name.
static char mc_fname[0x40] = "/BASLUS-20633";

// @2706 (.rodata 0x01935270): SdAlloc's name for the block. @2707 (0x01935280): its failure message.
static const char s_mcAllocName[] = "MC";
static const char s_mcAllocError[] = "mc_alloc: SdAlloc error !!\n";
// @2749 (.rodata 0x019352a0): mc_load_new's path format.
static const char s_pathFormat[] = "%s%s";

// The VU's vf0, (0, 0, 0, 1.0f): what sqc2 $vf0 stores.
static const qword kVf0 = {0, 0, 0, 0x3f800000};

// ---- the character table ----

// CT3 0x018b4070: every Character[] slot back to free -- ids and indices -1, flags and counters zero,
// positions at the origin, hit points from Char_Init_MAX_HP -- and its junction block (if any) returned
// to gHeapJunc.
void Character_All_Init(void)
{
    int i;
    int j;
    int k;

    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *chr = &Character[i];

        chr->id = -1;
        chr->bank = -1;
        for (j = 0; j < 10; j++) {
            chr->slot004[j] = -1;
            for (k = 0; k < 10; k++) {
                chr->slot022[j][k] = 0;
            }
        }
        for (j = 0; j < 10; j++) {
            chr->slot018[j] = 0;
        }
        chr->flags = 0;
        chr->w090 = 0;
        chr->w098 = 0;
        chr->lightBank = -1;
        memcpy(chr->posBefore, kVf0, sizeof(qword));
        memcpy(chr->pos, kVf0, sizeof(qword));
        chr->rotBefore = 0;
        chr->rot = 0;
        chr->w300[0] = 0;
        chr->w300[1] = 0;
        chr->w300[2] = 0;
        chr->w300[3] = 0;
        chr->b2f1 = 1;
        chr->b2f2 = 0;
        asInitPlayerBehaviorControler(chr);
        asShadowDrawing_Init(chr->bank);
        CharacterSortBuff[i] = -1;
        chr->hp = Char_Init_MAX_HP[i];
        chr->hpMax = Char_Init_MAX_HP[i];
        chr->w0fc = 0;
        chr->h100 = -1;
        chr->b102 = -1;
        chr->translucent = 0;
        chr->translucentAlpha = 0.0f;   /* the ROM stores the word 0 */
        chr->b534 = 0;
        for (j = 0; j < 6; j++) {
            chr->h536[j] = 0;
        }
        if (chr->junc != NULL) {
            _SunFree(&gHeapJunc, &chr->junc->buf);
            _SunFree(&gHeapJunc, (void **)&chr->junc);
        }
    }
}

// ---- the IOP ----

// CT3 0x018b4430: reboot the IOP with the game's modules (IopLoadModules reads no argument), then bring up
// the pads, the memory card, the file and sound servers and the movie player's IOP side.
void IOPInit(char **argv)
{
    IopLoadModules();
    gaPadInit();
    McInit();
    FlInit();
    FSInit();
    SSInit();
    AA_MovieIOPInit();
}

// The light helpers SceneLightSet / CharLightSet inline (one quadword move or VU0 ease each in the ROM).
static void LightCopy(float *dst, const float *src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

static void LightClear(float *dst)
{
    dst[0] = dst[1] = dst[2] = dst[3] = 0.0f;
}

// vf0: (0, 0, 0, 1), an unused point light's position.
static void LightClearPos(float *dst)
{
    dst[0] = dst[1] = dst[2] = 0.0f;
    dst[3] = 1.0f;
}

// VU0 vdiv / vsub / vmulq / vadd: v += (target - v) / frames.
static void LightEase(float *v, const float *target, float frames)
{
    float q = 1.0f / frames;
    int k;

    for (k = 0; k < 4; k++) {
        v[k] = (target[k] - v[k]) * q + v[k];
    }
}

static const float s_lightZero[4] = {0.0f, 0.0f, 0.0f, 0.0f};

// The point-light overrides of AllOmniEffect (and CharOmniEffect): [bank, part] pairs; a pair whose bank
// is gone is cleared, a live one replaces point light slot i.
static void LightOmniOverride(GA_LIGHT_WORK *lw, u_short *pair, int i)
{
    short bank = (short)pair[0];

    if (bank == LIGHT_NONE || BankInfo[bank].loaded != 1) {
        pair[0] = 0xffff;
        pair[1] = 0xffff;
    } else {
        MODEL_PARTS *src = &((MODEL_PARTS *)pModelPartsHeader[bank])[(short)pair[1]];

        LightCopy(lw->omniPos[i], src->lightPos);
        LightCopy(lw->omniColor[i], src->lightColor);
    }
}

// CT3 0x018b4480: give every lit part of a scene bank its lights (SCENE_LIGHT.pairs): up to four
// directional and four point lights from the set's parts, a set with an omni list choosing its
// LIGHT_OMNI_NEAREST slots by gaSearchNearLight4; a switched-off light part (PARTS_FLAG_FOG, bit 0) is
// skipped; banks of type other than 0 take AllOmniEffect's overrides. Then gaSetLight under the view.
void SceneLightSet(int scene)
{
    SCENE_LIGHT *sl = All_table_TBL[scene]->light;
    short *pairs = (short *)BankEePtr(scene, sl->pairs);
    LIGHT_SET *sets = (LIGHT_SET *)BankEePtr(scene, sl->sets);
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[scene];
    int maxCount = -1;
    int *off = NULL;
    float (*pos)[4] = NULL;
    float (*col)[4] = NULL;
    u_char nearest[32];
    int i;
    int k;

    // the longest omni list sizes the work buffers
    for (i = 0; pairs[i] != LIGHT_PAIRS_END; i += 2) {
        short set = pairs[i + 1];

        if (set != LIGHT_NONE && sets[set].omniList != LIGHT_LIST_NONE) {
            short *list = (short *)BankEePtr(scene, sets[set].omniList);

            if (maxCount < list[0]) {
                maxCount = list[0];
            }
        }
    }
    if (maxCount > 0) {
        off = (int *)_SunAlloc(&gHeapJunc, maxCount * 4, 0x40);
        pos = (float (*)[4])_SunAlloc(&gHeapJunc, maxCount * 16, 0x40);
        col = (float (*)[4])_SunAlloc(&gHeapJunc, maxCount * 16, 0x40);
    }

    for (i = 0; pairs[i] != LIGHT_PAIRS_END; i += 2) {
        short p = pairs[i];
        short s = pairs[i + 1];
        GA_LIGHT_WORK *lw = (GA_LIGHT_WORK *)BankEePtr(scene, (u_int)parts[p].attr);

        for (k = 0; k < LIGHT_SLOTS; k++) {
            LightClear(lw->dirPos[k]);
            LightClear(lw->dirColor[k]);
            LightClearPos(lw->omniPos[k]);
            LightClear(lw->omniColor[k]);
        }

        if (s != LIGHT_NONE) {
            LIGHT_SET *set = &sets[s];

            for (k = 0; k < LIGHT_SLOTS; k++) {
                short d = set->dir[k];

                if (d != LIGHT_NONE && !(parts[d].flags & PARTS_FLAG_FOG)) {
                    LightCopy(lw->dirPos[k], parts[d].lightPos);
                    LightCopy(lw->dirColor[k], parts[d].lightColor);
                }
            }
            if (set->omniList == LIGHT_LIST_NONE) {
                for (k = 0; k < LIGHT_SLOTS; k++) {
                    short o = set->omni[k];

                    if (o != LIGHT_NONE && !(parts[o].flags & PARTS_FLAG_FOG)) {
                        LightCopy(lw->omniPos[k], parts[o].lightPos);
                        LightCopy(lw->omniColor[k], parts[o].lightColor);
                    }
                }
            } else {
                short *list = (short *)BankEePtr(scene, set->omniList);
                int n;

                for (n = 0; n < list[0]; n++) {
                    MODEL_PARTS *l = &parts[list[n + 1]];

                    off[n] = l->flags & PARTS_FLAG_FOG;
                    LightCopy(pos[n], l->lightPos);
                    LightCopy(col[n], l->lightColor);
                }
                memset(nearest, NEAR_LIGHT_NONE, sizeof(nearest)); // PORT: a stubbed search writes nothing
                gaSearchNearLight4(nearest, (float (*)[4])parts[p].pos, 1, pos, list[0]);
                n = 0;
                for (k = 0; k < LIGHT_SLOTS; k++) {
                    u_int m = nearest[n];
                    short o = set->omni[k];

                    if (o == LIGHT_NONE) {
                        continue;
                    }
                    if (o == LIGHT_OMNI_NEAREST) {
                        if (m != NEAR_LIGHT_NONE && off[m] == 0) {
                            LightCopy(lw->omniPos[k], pos[m]);
                            LightCopy(lw->omniColor[k], col[m]);
                        }
                        n++;
                    } else if (!(parts[o].flags & PARTS_FLAG_FOG)) {
                        LightCopy(lw->omniPos[k], parts[o].lightPos);
                        LightCopy(lw->omniColor[k], parts[o].lightColor);
                    }
                }
            }
        }

        if (BankInfo[scene].type != BANK_TYPE_0) {
            for (k = 0; k < LIGHT_SLOTS; k++) {
                LightOmniOverride(lw, &AllOmniEffect[k * 2], k);
            }
        }
        gaSetLight(lw, matrix_global_view);
    }

    if (maxCount > 0) {
        _SunFree(&gHeapJunc, (void **)&off);
        _SunFree(&gHeapJunc, (void **)&pos);
        _SunFree(&gHeapJunc, (void **)&col);
    }
}

// One directional light slot of CharLightSet: toward target (NULL: dark), at once, or eased over
// DirectLightCharaCounter's frames, or over CharDirectLightSet's fade while a set is on.
static void CharDirLight(GA_LIGHT_WORK *lw, int k, const float *pos, const float *color, float charaFrames,
                         int setOn, float setFrames)
{
    if (charaFrames == 0.0f) {
        if (!setOn || setFrames == 0.0f) {
            LightCopy(lw->dirPos[k], pos);
            LightCopy(lw->dirColor[k], color);
        } else {
            LightEase(lw->dirPos[k], pos, setFrames);
            LightEase(lw->dirColor[k], color, setFrames);
        }
    } else {
        LightEase(lw->dirPos[k], pos, charaFrames);
        LightEase(lw->dirColor[k], color, charaFrames);
    }
}

// CharLightSet's point light slot from the nearest search: light m, or off when there is none or the
// light part is switched off.
static void CharOmniNearest(GA_LIGHT_WORK *lw, int k, u_int m, const int *off, float (*pos)[4],
                            float (*col)[4])
{
    if (m == NEAR_LIGHT_NONE || off[m] == 1) {
        LightClearPos(lw->omniPos[k]);
        LightClear(lw->omniColor[k]);
    } else {
        LightCopy(lw->omniPos[k], pos[m]);
        LightCopy(lw->omniColor[k], col[m]);
    }
}

// CT3 0x018b49f0: light a character's parts from its light bank (CHARACTER.lightBank, -1 = none): the
// directional lights of the bank's charaDir (or the character's CharDirectLightSet, its LIGHT_NONE slots
// filled from charaDir), eased over DirectLightCharaCounter / the set's fade; the four point lights
// nearest the part from the bank's omniList. Actress lighting (ActressLightFlag[bank], the player only)
// keeps the bank's Omni0_num / Omni1_num lights in slots 0 and 1 and weakens the others' falloff.
// CharOmniEffect and AllOmniEffect override the point lights; gaSetLight runs under the mirror's view.
void CharLightSet(int chara)
{
    CHARACTER *chr = &Character[chara];
    short lightBank = chr->lightBank;
    int bank = chr->bank;
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    short *pairs = (short *)BankEePtr(bank, All_table_TBL[bank]->light->pairs);
    LIGHT_SET *sets = (LIGHT_SET *)BankEePtr(bank, All_table_TBL[bank]->light->sets);
    u_short *dlSet = CharDirectLightSet[chara];
    int actress;
    short *dir;
    short *list;
    int count;
    int *off = NULL;
    float (*pos)[4] = NULL;
    float (*col)[4] = NULL;
    float (*posFar)[4] = NULL;
    u_char nearest[32];
    MODEL_PARTS *lparts;
    SCENE_LIGHT *lsl;
    int i;
    int k;

    if (lightBank < 0) {
        return;
    }
    lsl = All_table_TBL[lightBank]->light;
    lparts = (MODEL_PARTS *)pModelPartsHeader[lightBank];
    if (dlSet[CHAR_DLS_ON] == 0) {
        dir = (short *)BankEePtr(lightBank, lsl->charaDir);
    } else {
        short *charaDir = (short *)BankEePtr(lightBank, lsl->charaDir);

        dir = (short *)&dlSet[CHAR_DLS_DIR];
        for (k = 0; k < LIGHT_SLOTS; k++) {
            if (dir[k] == LIGHT_NONE) {
                dir[k] = charaDir[k];
            }
        }
    }
    list = (short *)BankEePtr(lightBank, lsl->omniList);
    count = list[0];
    actress = ActressLightFlag[lightBank] != 0 && chara == 0;
    if (count != 0) {
        off = (int *)_SunAlloc(&gHeapJunc, count * 4, 0x40);
        pos = (float (*)[4])_SunAlloc(&gHeapJunc, count * 16, 0x40);
        col = (float (*)[4])_SunAlloc(&gHeapJunc, count * 16, 0x40);
        if (actress) {
            // PORT: the ROM never allocates this buffer (sp+0x100 is written and freed uninitialised;
            // hazards.md "AlyssaPan" 2026-11-10); the host gives it the same size.
            posFar = (float (*)[4])_SunAlloc(&gHeapJunc, count * 16, 0x40);
        }
    }

    for (i = 0; pairs[i] != LIGHT_PAIRS_END; i += 2) {
        short p = pairs[i];
        short s = pairs[i + 1];
        GA_LIGHT_WORK *lw = (GA_LIGHT_WORK *)BankEePtr(bank, (u_int)parts[p].attr);

        if (s == LIGHT_NONE) {
            for (k = 0; k < LIGHT_SLOTS; k++) {
                LightClear(lw->dirPos[k]);
                LightClear(lw->dirColor[k]);
                LightClearPos(lw->omniPos[k]);
                LightClear(lw->omniColor[k]);
            }
        } else {
            float charaFrames = (float)DirectLightCharaCounter[chara];
            float setFrames = (float)(short)dlSet[CHAR_DLS_FADE];

            for (k = 0; k < LIGHT_SLOTS; k++) {
                MODEL_PARTS *l = NULL;

                if (dir[k] != LIGHT_NONE && sets[s].dir[k] != LIGHT_NONE) {
                    l = &lparts[dir[k]];
                    if (l->flags & PARTS_FLAG_FOG) {
                        l = NULL;
                    }
                }
                // a missing or switched-off light fades to dark the same way
                CharDirLight(lw, k, l != NULL ? l->lightPos : s_lightZero, l != NULL ? l->lightColor : s_lightZero,
                             charaFrames, dlSet[CHAR_DLS_ON] != 0, setFrames);
            }

            if (count == 0) {
                for (k = 0; k < LIGHT_SLOTS; k++) {
                    LightClearPos(lw->omniPos[k]);
                    LightClear(lw->omniColor[k]);
                }
            } else {
                int omni0 = -1;
                int omni1 = -1;
                int n;

                for (n = 0; n < count; n++) {
                    short lp = list[n + 1];
                    MODEL_PARTS *l = &lparts[lp];

                    off[n] = l->flags & PARTS_FLAG_FOG;
                    LightCopy(pos[n], l->lightPos);
                    if (!actress) {
                        if (lp == Omni0_num[lightBank]) {
                            pos[n][3] = 1.0f;
                        }
                        if (lp == Omni1_num[lightBank]) {
                            pos[n][3] = 1.0f;
                        }
                    } else {
                        LightCopy(posFar[n], pos[n]);
                        posFar[n][3] = 1.0f / (1.0f / posFar[n][3] + LIGHT_ACTRESS_FALLOFF);
                        if (lp == Omni0_num[lightBank]) {
                            posFar[n][3] = 1.0f;
                            omni0 = n;
                        }
                        if (lp == Omni1_num[lightBank]) {
                            posFar[n][3] = 1.0f;
                            omni1 = n;
                        }
                    }
                    LightCopy(col[n], l->lightColor);
                }
                memset(nearest, NEAR_LIGHT_NONE, sizeof(nearest)); // PORT: a stubbed search writes nothing
                gaSearchNearLight4(nearest, (float (*)[4])parts[p].pos, 1, actress ? posFar : pos, count);

                if (!actress) {
                    for (k = 0; k < LIGHT_SLOTS; k++) {
                        CharOmniNearest(lw, k, nearest[k], off, pos, col);
                    }
                } else {
                    float frames = (float)DirectLightCharaCounter[chara];
                    int slot = 0;

                    // slots 0 and 1: the actress lights when the bank lists them, else the nearest
                    if (omni0 < 0) {
                        CharOmniNearest(lw, 0, nearest[slot], off, pos, col);
                        slot = 1;
                    } else {
                        LightCopy(lw->omniPos[0], pos[omni0]);
                        if (frames == 0.0f) {
                            LightCopy(lw->omniColor[0], col[omni0]);
                        } else {
                            LightEase(lw->omniColor[0], col[omni0], frames);
                        }
                    }
                    if (omni1 < 0) {
                        CharOmniNearest(lw, 1, nearest[slot], off, pos, col);
                        slot++;
                    } else {
                        LightCopy(lw->omniPos[1], pos[omni1]);
                        if (frames == 0.0f) {
                            LightCopy(lw->omniColor[1], col[omni1]);
                        } else {
                            LightEase(lw->omniColor[1], col[omni1], frames);
                        }
                    }
                    // slots 2 and 3: the next nearest (the ROM advances only past a light it used)
                    CharOmniNearest(lw, 2, nearest[slot], off, pos, col);
                    if (nearest[slot] != NEAR_LIGHT_NONE && off[nearest[slot]] != 1) {
                        slot++;
                    }
                    CharOmniNearest(lw, 3, nearest[slot], off, pos, col);
                }
            }

            for (k = 0; k < LIGHT_SLOTS; k++) {
                LightOmniOverride(lw, &CharOmniEffect[chara][k * 2], k);
                LightOmniOverride(lw, &AllOmniEffect[k * 2], k);
            }
            gaMulMatrix(mrr_ReverseGlobalView, matrix_global_view, mrr_ReverseMatrix);
            gaSetLight(lw, mrr_ReverseGlobalView);
        }
    }

    if ((short)dlSet[CHAR_DLS_FADE] != 0) {
        dlSet[CHAR_DLS_FADE]--;
    }
    if (count != 0) {
        _SunFree(&gHeapJunc, (void **)&off);
        _SunFree(&gHeapJunc, (void **)&pos);
        _SunFree(&gHeapJunc, (void **)&col);
        if (actress) {
            _SunFree(&gHeapJunc, (void **)&posFar);
        }
    }
}

// ---- the memory card ----

// CT3 0x018bb5c0: sceMcInit once; answers its result (0 when done before).
int McInit(void)
{
    static int initflag;                /* initflag$64, .sbss 0x0193ef78 */
    int ret = 0;

    if (initflag == 0) {
        ret = sceMcInit();
        if (ret == 0) {
            initflag = 1;
        }
    }
    return ret;
}

// CT3 0x018b89b0: give the work buffer back to the sound heap and forget its pieces.
void mc_free(void)
{
    int i;

    if (alloc_flag != 0) {
        SdFree((void **)&mc_data_buff);
        mc_data_buff = NULL;
        mc_master = NULL;
        mc_header = NULL;
        for (i = 0; i < MC_DATA_COUNT; i++) {
            mc_databuff[i] = NULL;
        }
        alloc_flag = 0;
    }
}

// CT3 0x018b8a10: once every file, SE and SQ load is done, borrow the sound heap's slot for the work
// buffer, zero it and cut it up. PORT: the ROM answers the block's address on success (non-zero) and
// printf's count on failure; the host answers 1 on success.
int mc_alloc(void)
{
    int ret;
    int i;

    FSMltLoadEndWaitAll();
    SdLoadSEEndWaitAll();
    SdLoadSQEndWaitAll();
    mc_data_buff = (u_char *)SdAlloc(MC_BUFF_ALLOC, MC_BUFF_ALIGN, s_mcAllocName);
    if (mc_data_buff == NULL) {
        ret = printf(s_mcAllocError);
    } else {
        for (i = 0; i < MC_BUFF_CLEAR; i++) {
            mc_data_buff[i] = 0;
        }
        alloc_flag = 1;
        mc_master = mc_data_buff + MC_MASTER_OFFSET;
        mc_header = mc_data_buff + MC_HEADER_OFFSET;
        for (i = 0; i < MC_DATA_COUNT; i++) {
            mc_databuff[i] = mc_data_buff + MC_DATA_OFFSET + i * MC_DATA_SIZE;
        }
        ret = 1;
    }
    return ret;
}

// CT3 0x018bacb0: zero one save image (data), then mark every record of every image in mc_databuff empty.
// The ROM clears data but marks the eight images, not data alone: kept.
void mc_save_data_init(u_char *data)
{
    int i;
    int n;

    for (i = 0; i < MC_DATA_SIZE; i++) {
        data[i] = 0;
    }
    for (i = 0; i < MC_DATA_COUNT; i++) {
        MC_SAVE_RECORD *rec = (MC_SAVE_RECORD *)mc_databuff[i];

        for (n = 0; n < MC_SAVE_RECORD_MAX; n++) {
            rec[n].id = MC_SAVE_RECORD_EMPTY;
        }
    }
}

// CT3 0x018bb460: ask the card in slot mcSlot_ID its type until four answers in a row agree (or the
// library gives up), one vblank between asks. Answers 0 for a PS2 card, 1 for anything else (none, PS1,
// PDA, or a card that never answered).
int mc_check(void)
{
    int type = 0;
    int result;
    int tries;
    int fails = 0;
    int same = 0;
    int seen[MC_CHECK_STABLE_READS];

    for (;;) {
        tries = 0;
        while (sceMcGetInfo(mcSlot_ID, 0, &type, NULL, NULL) != 0) {
            if (tries > MC_CHECK_INFO_RETRIES) {
                type = 0;
                goto settled;
            }
            tries++;
            fails = 0;
            same = 0;
            sceGsSyncV(0);
        }
        sceMcSync(0, NULL, &result);
        if (result < 0) {
            type = 0;
            if (fails > MC_CHECK_SYNC_RETRIES) {
                goto settled;
            }
            fails++;
            same = 0;
        } else {
            /* The ROM's order: a mismatch restarts the count at 1 without moving the new answer to
               seen[0], so the next answer is compared with the old first one. Kept. */
            seen[same] = type;
            if (same > 0) {
                if (seen[same - 1] == seen[same]) {
                    if (same == MC_CHECK_STABLE_READS - 1) {
                        goto settled;
                    }
                } else {
                    fails = 0;
                    same = 0;
                }
            }
            same++;
        }
        sceGsSyncV(0);
    }

settled:
    /* The ROM tests PDA, then PS1, then PS2: only a PS2 card answers 0. */
    return type == sceMcTypePS2 ? 0 : 1;
}

// CT3 0x018b78d0: every eleventh call, check the card: none -> MC_ERR_NO_CARD and back to the card menu's
// first state (mcMain_ID 0); present -> the error is cleared when clearError says so.
void mc_check_always(int clearError)
{
    fream_counter++;
    if (fream_counter > MC_CHECK_ALWAYS_PERIOD) {
        fream_counter = 0;
        if (mc_check() == 0) {
            if (clearError != 0) {
                mcErr_ID = MC_ERR_NONE;
            }
        } else {
            mcErr_ID = MC_ERR_NO_CARD;
            mcMain_ID = 0;
        }
    }
}

// CT3 0x018b5d00: the auto-load's third insertion check: mark the slot when mc_check3 finds no card.
int MemorycardAutoLoadInsertedCheck3(void)
{
    int result = mc_check3();

    if (result != 0) {
        SlotErr[mcSlot_ID] = MC_SLOT_ERR_NO_CARD;
    }
    return result;
}

// CT3 0x018b5cc0: the auto-load's format check: mark the slot when the card is unformatted. Answers
// mc_formatcheck's answer XOR 1 (0 for a formatted card, as the ROM computes it).
int MemorycardAutoLoadFormatCheck(void)
{
    int formatted = mc_formatcheck();

    if (formatted == 0) {
        SlotErr[mcSlot_ID] = MC_SLOT_ERR_UNFORMATTED;
    }
    return formatted ^ 1;
}

// CT3 0x018bac60: reset the header (mc_save_system_init) and every save image.
void mc_save_init(void)
{
    int i;

    mc_save_system_init();
    for (i = 0; i < MC_DATA_COUNT; i++) {
        mc_save_data_init(mc_databuff[i]);
    }
}

// CT3 0x018bb150: ask the card's format flag until four answers in a row agree (mc_check's loop, on
// sceMcGetInfo's format word). Answers the flag, or 0 when the library gives up.
int mc_formatcheck(void)
{
    int format = 0;
    int result;
    int tries;
    int fails = 0;
    int same = 0;
    int seen[MC_CHECK_STABLE_READS];

    for (;;) {
        tries = 0;
        while (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
            if (tries > MC_CHECK_INFO_RETRIES) {
                return 0;
            }
            tries++;
            fails = 0;
            same = 0;
            sceGsSyncV(0);
        }
        sceMcSync(0, NULL, &result);
        if (result < 0) {
            format = 0;
            if (fails > MC_CHECK_SYNC_RETRIES) {
                return 0;
            }
            fails++;
            same = 0;
        } else {
            /* mc_check's order (a mismatch restarts the count at 1), kept. */
            seen[same] = format;
            if (same > 0) {
                if (seen[same - 1] == seen[same]) {
                    if (same == MC_CHECK_STABLE_READS - 1) {
                        return format;
                    }
                } else {
                    fails = 0;
                    same = 0;
                }
            }
            same++;
        }
        sceGsSyncV(0);
    }
}

// CT3 0x018bb070: one quick format check, asked twice: MC_FORMAT_YES / MC_FORMAT_NO, MC_FORMAT_BUSY when
// the library refused. The ROM answers YES after a first "formatted" whatever the second ask says.
int mc_formatcheck2(void)
{
    int format;
    int result;

    if (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
        return MC_FORMAT_BUSY;
    }
    sceMcSync(0, NULL, &result);
    if (format == 0) {
        if (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
            return MC_FORMAT_BUSY;
        }
        sceMcSync(0, NULL, &result);
        return format != 0 ? MC_FORMAT_BUSY : MC_FORMAT_NO;
    }
    if (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
        return MC_FORMAT_BUSY;
    }
    sceMcSync(0, NULL, &result);
    return MC_FORMAT_YES;
}

// CT3 0x018bb280: the card's state in one call: sceMcGetInfo's refusal - 200, or sceMcSync's result.
int mc_check4(void)
{
    int result;
    int ret = sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, NULL);

    if (ret < 0) {
        result = ret - 200;
    } else {
        sceMcSync(0, NULL, &result);
    }
    return result;
}

// The XOR of count little-endian halfwords from p (mc_make_checksum's sum, written out once per use in
// the ROM). The buffers keep the EE's layout and byte order, which the host shares.
static u_short mc_xor_halfs(const u_char *p, int count)
{
    const u_short *h = (const u_short *)p;
    u_short sum = 0;
    int i;

    for (i = 0; i < count; i++) {
        sum ^= h[i];
    }
    return sum;
}

// CT3 0x018ba6d0: the master record's checksums. save != 0: (before writing) store the header's sum, and
// every image's except the one being saved (mcFile_ID) is checked as on a load. save == 0: (after
// reading) check them all; a part whose sum is wrong is reset (the header by mc_save_system_init, an
// image by mc_save_data_init), its sum stored again and mc_broke_data says which.
void mc_make_checksum(int save)
{
    MC_MASTER *master;
    u_char *base;
    u_short sum;
    int i;

    for (i = 0; i < MC_BROKE_COUNT; i++) {
        mc_broke_data[i] = 0;
    }
    master = (MC_MASTER *)mc_master;
    base = mc_master;
    sum = mc_xor_halfs(base + MC_HEADER_OFFSET, MC_HEADER_SUM_HALFS);
    if (save == 0) {
        if (master->headerSum != sum) {
            mc_save_system_init();
            master->headerSum = mc_xor_halfs(base + MC_HEADER_OFFSET, MC_HEADER_SUM_HALFS);
            mc_broke_data[MC_BROKE_SYSTEM] = 1;
        }
    } else {
        master->headerSum = sum;
    }
    for (i = 0; i < MC_DATA_COUNT; i++) {
        u_char *data = base + MC_DATA_OFFSET + i * MC_DATA_SIZE;

        sum = mc_xor_halfs(data, MC_DATA_SUM_HALFS);
        if (save == 0 || mcFile_ID != i) {
            if (master->dataSum[i] != sum) {
                mc_save_data_init(mc_databuff[i]);
                master->dataSum[i] = mc_xor_halfs(data, MC_DATA_SUM_HALFS);
                mc_broke_data[i] = 1;
            }
        } else {
            master->dataSum[i] = sum;
        }
    }
}

// CT3 0x018b8b90: read the save file (mc_fname twice: "/BASLUS-20633/BASLUS-20633") from the card in
// slot mcSlot_ID into the whole work buffer, then check it (mc_make_checksum(0)). Runs its steps until
// done, one vblank after each refused call; answers MC_RESULT_OK or MC_RESULT_FAIL. The steps fall
// through as the ROM's do: a read refused mid-file resumes at the chunk it stopped on.
int mc_load_new(void)
{
    char path[0x40c];
    int result;
    int ret = MC_RESULT_BUSY;

    mc_load_new_fd = 0;
    mc_load_new_num = 0;
    for (;;) {
        switch (mc_load_new_num) {
        case 0:
            sprintf(path, s_pathFormat, mc_fname, mc_fname);
            ret = MC_RESULT_BUSY;
            if (sceMcOpen(mcSlot_ID, 0, path, MC_OPEN_RDONLY) != 0) {
                break;
            }
            sceMcSync(0, NULL, &mc_load_new_fd);
            if (mc_load_new_fd < 0) {
                ret = MC_RESULT_FAIL;
                mc_load_new_num = MC_STEP_ERROR;
                break;
            }
            mc_load_new_num++;
            /* fall through */
        case 1:
            ret = MC_RESULT_BUSY;
            if (sceMcSeek(mc_load_new_fd, 0, 0) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            if (result < 0) {
                mc_load_new_num = MC_STEP_ERROR;
                break;
            }
            mc_load_new_loop_count = 0;
            mc_load_new_loop_count_max = MC_LOAD_CHUNKS;
            mc_load_new_end_size = MC_LOAD_TAIL;
            mc_load_new_num++;
            /* fall through */
        case 2:
            ret = MC_RESULT_OK;
            for (; mc_load_new_loop_count != mc_load_new_loop_count_max; mc_load_new_loop_count++) {
                if (sceMcRead(mc_load_new_fd, mc_data_buff + mc_load_new_loop_count * MC_LOAD_CHUNK,
                              MC_LOAD_CHUNK) != 0) {
                    ret = MC_RESULT_BUSY;
                    break;
                }
                sceMcSync(0, NULL, &result);
                if (result < 0) {
                    ret = MC_RESULT_BUSY;
                    mc_load_new_num = MC_STEP_ERROR;
                    break;
                }
            }
            if (ret != MC_RESULT_OK) {
                break;
            }
            /* The ROM bumps mc_save_normal_num here and after the tail read, not mc_load_new_num (the
               step stays 2): a slip copied from mc_save_normal, kept. */
            mc_save_normal_num++;
            /* fall through */
        case 3:
            ret = MC_RESULT_BUSY;
            if (sceMcRead(mc_load_new_fd, mc_data_buff + mc_load_new_loop_count * MC_LOAD_CHUNK,
                          mc_load_new_end_size) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            mc_save_normal_num++;
            if (result < 0) {
                mc_load_new_num = MC_STEP_ERROR;
                break;
            }
            /* fall through */
        case 4:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_load_new_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;
            if (result >= 0) {
                mc_load_new_num = 0;
                ret = MC_RESULT_OK;
                mc_make_checksum(0);
            }
            break;
        case MC_STEP_ERROR:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_load_new_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;       /* the ROM tests the result and fails either way */
            break;
        }
        if (ret == MC_RESULT_FAIL || ret == MC_RESULT_OK) {
            mc_load_new_num = 0;
            return ret;
        }
        sceGsSyncV(0);
    }
}

// ---- MemorycardUpdate: the card flows, one step per frame ----
//
// SE arguments: the ROM leaves a2 unset on some SSStopSE / SSPlaySE(MC_SE_ACCESS) calls; where the
// register can be traced it holds 1 (the value just compared or stored), so every call passes 1.

// The save menu's answer to mc_save_new / mc_save_normal (inlined four times in the ROM).
static void mc_save_menu_result(int ret)
{
    if (ret == MC_RESULT_FAIL) {
        mcErr_ID = MC_ERR_SAVE_FAILED;
        mcMain_ID = 5;
        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
    } else if (ret == MC_RESULT_OK) {
        mcErr_ID = MC_ERR_NONE;
        mcMain_ID = 5;
        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
        SSPlaySE(s_seBank, MC_SE_DONE, 1);
    }
}

// The slot the save now lives in: LOAD_SLOT_NUM and TRIG_EVENT_LOAD_SLOT2 (inlined twice in the ROM).
static void mc_set_load_slot(void)
{
    if (mcSlot_ID == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_LOAD_SLOT2, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_LOAD_SLOT2, 1, 0);
    }
    LOAD_SLOT_NUM = mcSlot_ID != 0;
}

// The file to point at after a load: the one last used in this slot when the card's uid still matches,
// else the header's last file.
static int mc_pick_file(void)
{
    const u_int *header = (const u_int *)mc_header;

    if (mc_cursor[mcSlot_ID] == MC_FILE_NONE ||
        (header[1] & MC_HEADER_UID_MASK) != mc_uidwork[mcSlot_ID]) {
        return (int)(header[1] & MC_HEADER_FILE_MASK);
    }
    return mc_cursor[mcSlot_ID];
}

// MC_ACCESS_SAVE (mcAccess_ID 4).
static void mc_update_save_menu(void)
{
    int ret;
    int state;
    int result;

    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        AA_Phase_Step = 0;
        if (AA_New_McAlloc_Mode == 0 && alloc_flag != 0) {
            mc_free();
        }
        break;
    case 1:
        if (alloc_flag == 0) {
            if (AA_New_McAlloc_Mode == 0) {
                mc_alloc();
            }
            mcErr_ID = MC_ERR_CHECKING;
        } else if (mc_check() == 0) {
            mcErr_ID = MC_ERR_NONE;
            mcMain_ID = 5;
        } else {
            mcErr_ID = MC_ERR_NO_CARD;
            mcMain_ID = 2;
            AA_count = MC_RETRY_WAIT;
            mc_cursor[mcSlot_ID] = MC_FILE_NONE;
        }
        break;
    case 2:
        if (AA_count < 0) {
            AA_count = MC_RETRY_WAIT;
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_NONE;
                mc_menuid = 1;
                mcMain_ID = 3;
            }
        } else {
            AA_count--;
        }
        break;
    case 4:
        switch (AA_Phase_Step) {
        case 0:
            AA_Phase_Step = 0;
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 3;
            }
            break;
        case 1:
            ret = mc_formatcheck2();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = 2;
                AA_count = MC_FORMAT_WAIT;
                AA_UnFormatCk = 0;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    AA_UnFormatCk = 1;
                    AA_Phase_Step = 2;
                    AA_count = MC_FORMAT_WAIT;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
            break;
        case 2:
            if (AA_count < 0) {
                AA_Phase_Step = 3;
            } else {
                AA_count--;
            }
            break;
        case 3:
            ret = mc_formatcheck2();
            state = mc_check4();
            printf(s_check4Format, state);
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = AA_UnFormatCk == 0 ? 4 : 1;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    if (AA_UnFormatCk == 1) {
                        mcErr_ID = MC_ERR_UNFORMATTED;
                        mcMain_ID = 5;
                    } else {
                        AA_Phase_Step = 1;
                    }
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
            break;
        case 4:
            mc_free();
            mc_alloc();
            ret = mc_load_new();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    system_data_load_set(0);
                    mcFile_ID = mc_pick_file();
                    if (mcFile_ID < 0) {
                        mcFile_ID = 0;
                    }
                    mcErr_ID = MC_ERR_NONE;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                    mc_type = 0;
                    mcMain_ID = 5;
                } else {
                    mc_save_init();
                    mcFile_ID = 0;
                    mcErr_ID = MC_ERR_NONE;
                    mc_type = 0;
                    mcMain_ID = 5;
                }
            } else if (ret == MC_RESULT_FAIL) {
                if (mc_check() == 0) {
                    mc_save_init();
                    mcFile_ID = 0;
                    AA_Phase_Step = 5;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                    mc_type = 1;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 5;
                }
            }
            break;
        case 5:
            ret = mc_free_area();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == 1) {
                mcErr_ID = MC_ERR_MSG_08;
                mcMain_ID = 5;
            } else if (ret == 0) {
                mc_type = 1;
                mcFile_ID = 0;
                fream_counter = MC_FRAME_COUNTER_NOW;
                mcErr_ID = MC_ERR_NONE;
                mcMain_ID = 5;
            }
            break;
        }
        break;
    case 5:
    case 9:
        mc_check_always2(0);
        AA_Phase_Step = 0;
        break;
    case 7:
        if (AA_Phase_Step == 1) {
            if (sceMcFormat(mcSlot_ID, 0) >= 0 && sceMcSync(0, NULL, &result) != 0) {
                if (result == 0) {
                    mc_cursor[mcSlot_ID] = 0;
                    mc_uidwork[mcSlot_ID] = 0;
                    mc_save_init();
                    mcFile_ID = 0;
                    mc_type = 1;
                    mcErr_ID = MC_ERR_NONE;
                    mcMain_ID = 5;
                    AA_Phase_Step = 0;
                } else {
                    AA_Phase_Step = 0;
                    mcErr_ID = MC_ERR_MSG_04;
                    mcMain_ID = 5;
                }
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
            } else {
                AA_Phase_Step = 0;
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            }
        }
        break;
    case 8:
        if (AA_Phase_Step == 1) {
            int alt;

            mc_set_load_slot();
            mc_cursor[mcSlot_ID] = mcFile_ID;
            mc_uidwork[mcSlot_ID] = 0;
            alt = ssTrigCheckEvent(TRIG_EVENT_SAVE_ALT, 1, 0) != 0;
            if (mc_type == 1) {
                mc_free();
                mc_alloc();
                system_data_load_set(2);
                ret = mc_save_new(alt);
            } else {
                system_data_load_set(2);
                ret = mc_save_normal(alt);
            }
            if (ret != MC_RESULT_BUSY) {
                mc_save_menu_result(ret);
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
                SSPlaySE(s_seBank, MC_SE_ACCESS, 1);
            } else {
                AA_Phase_Step = 0;
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            }
        }
        break;
    }
}

// MC_ACCESS_LOAD (mcAccess_ID 3).
static void mc_update_load_menu(void)
{
    int ret;
    int state;
    int i;
    int files;

    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        if (AA_New_McAlloc_Mode == 0 && alloc_flag != 0) {
            mc_free();
        }
        break;
    case 1:
        if (alloc_flag == 0) {
            if (AA_New_McAlloc_Mode == 0) {
                mc_alloc();
            }
            mcErr_ID = MC_ERR_CHECKING;
        } else {
            ret = mc_check();
            if (ret == 0) {
                mcErr_ID = MC_ERR_NONE;
                mcMain_ID = 5;
            } else if (ret == 1) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 2;
                AA_count = MC_RETRY_WAIT;
                mc_cursor[mcSlot_ID] = MC_FILE_NONE;
                mc_uidwork[mcSlot_ID] = 0;
            }
        }
        break;
    case 2:
        if (AA_count < 0) {
            AA_count = MC_RETRY_WAIT;
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_NONE;
                mc_menuid = 1;
                AA_Phase_Step = 0;
                mcMain_ID = 3;
            }
        } else {
            AA_count--;
        }
        break;
    case 4:
        if (AA_Phase_Step == 4) {
            mc_free();
            mc_alloc();
            ret = mc_load_new();
            state = mc_check4();
            printf(s_check4Format, state);
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    const u_int *header = (const u_int *)mc_header;

                    system_data_load_set(0);
                    files = 0;
                    for (i = 0; i < MC_INFO_COUNT; i++) {
                        if (mc_Getinfo_count(i) != 0) {
                            files++;
                        }
                    }
                    if (files == 0) {
                        mc_save_init();
                        mcFile_ID = 0;
                        mcErr_ID = MC_ERR_NEW_DATA;
                        mc_type = 0;
                        mcMain_ID = 5;
                    } else {
                        if (mc_cursor[mcSlot_ID] == MC_FILE_NONE ||
                            (header[1] & MC_HEADER_UID_MASK) != mc_uidwork[mcSlot_ID]) {
                            mcFile_ID = (int)(header[1] & MC_HEADER_FILE_MASK);
                            mc_uidwork[mcSlot_ID] = 0;
                        } else if (mc_Getinfo_count(mc_cursor[mcSlot_ID]) == 0) {
                            mcFile_ID = (int)(header[1] & MC_HEADER_FILE_MASK);
                        } else {
                            mcFile_ID = mc_cursor[mcSlot_ID];
                        }
                        if (mcFile_ID < 0) {
                            mcFile_ID = 0;
                        }
                        mcErr_ID = MC_ERR_NONE;
                        fream_counter = MC_FRAME_COUNTER_NOW;
                        mc_type = 0;
                        mcMain_ID = 5;
                    }
                } else {
                    mc_save_init();
                    mcFile_ID = 0;
                    mcErr_ID = MC_ERR_NEW_DATA;
                    mc_type = 0;
                    mcMain_ID = 5;
                }
            } else if (ret == MC_RESULT_FAIL) {
                if (mc_check() == 0) {
                    mc_save_init();
                    mcFile_ID = 0;
                    mc_type = 1;
                    mcErr_ID = MC_ERR_NEW_DATA;
                    mcMain_ID = 5;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
        } else if (AA_Phase_Step == 3) {
            ret = mc_formatcheck2();
            state = mc_check4();
            /* No else here in the ROM (the save menu has one): a card error still lets the format
               answer move the step. Kept. */
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            }
            if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = AA_UnFormatCk == 0 ? 4 : 1;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    if (AA_UnFormatCk == 1) {
                        mcErr_ID = MC_ERR_UNFORMATTED;
                        mcMain_ID = 5;
                    } else {
                        AA_Phase_Step = 1;
                    }
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
        } else if (AA_Phase_Step == 2) {
            if (AA_count < 0) {
                AA_Phase_Step = 3;
            } else {
                AA_count--;
            }
        } else if (AA_Phase_Step == 1) {
            ret = mc_formatcheck2();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = 2;
                AA_count = MC_FORMAT_WAIT;
                AA_UnFormatCk = 0;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    AA_UnFormatCk = 1;
                    AA_Phase_Step = 2;
                    AA_count = MC_FORMAT_WAIT;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 3;
            }
        }
        break;
    case 5:
    case 7:
        mc_check_always2(0);
        AA_Phase_Step = 0;
        break;
    case 6:
        if (AA_Phase_Step == 1) {
            ret = mc_load_new();
            if (ret == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    if (mc_Getinfo_count(mcFile_ID) == 0) {
                        mcErr_ID = MC_ERR_MSG_06;
                        mcMain_ID = 7;
                        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                    } else {
                        mcErr_ID = MC_ERR_NONE;
                        mcMain_ID = 7;
                        mc_type = 0;
                        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                        SSPlaySE(s_seBank, MC_SE_DONE, 1);
                    }
                } else {
                    mcErr_ID = MC_ERR_MSG_06;
                    mcMain_ID = 7;
                    mc_type = 0;
                    SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                }
            } else if (ret == MC_RESULT_FAIL) {
                if (mc_check() == 0) {
                    mcErr_ID = MC_ERR_MSG_06;
                    mcMain_ID = 7;
                    mc_type = 1;
                    SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                    SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                }
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
                SSPlaySE(s_seBank, MC_SE_ACCESS, 1);
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 3;
            }
        }
        break;
    case 8:
        tiFade_Start(10.0f, 0); // PORT: the ROM leaves a0 (the fade type) unset here (open-items.md)
        mcMain_ID = 9;
        break;
    case 9:
        if (FadeEndTicks <= FadeStartTicks) {
            AA_Phase_Step = 0;
            mcMain_ID = 10;
        }
        break;
    case 10:
        if (AA_Phase_Step == 2) {
            mc_cursor[mcSlot_ID] = mcFile_ID;
            mc_uidwork[mcSlot_ID] = ((const u_int *)mc_header)[1] & MC_HEADER_UID_MASK;
            mc_free();
            mcMain_ID = 0;
            mc_set_load_slot();
            SceneSELoad();
            ssTrigRegisterEvent(TRIG_EVENT_MC_LOADED, 1, 0);
        } else if (AA_Phase_Step == 1) {
            system_data_load_set(1);
            mc_load_set();
            AA_Phase_Step = 2;
        } else if (AA_Phase_Step == 0) {
            AA_Phase_Step = 1;
        }
        break;
    }
}

// MC_ACCESS_AUTO_SAVE (mcAccess_ID 1).
static void mc_update_auto_save(void)
{
    int ret;
    int result;

    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        mc_free();
        break;
    case 1:
        if (mc_menuid == 2) {
            if (mc_load_new() == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    system_data_load_set(0);
                    mcErr_ID = MC_ERR_NONE;
                    mcMain_ID = 2;
                    mc_type = 0;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                } else {
                    mcErr_ID = MC_ERR_MSG_11;
                    mcMain_ID = 0;
                }
            } else if (mc_check() == 0) {
                mc_save_init();
                mcErr_ID = MC_ERR_CHECKING;
                mcMain_ID = 2;
                fream_counter = MC_FRAME_COUNTER_NOW;
                mc_type = 1;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 1) {
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_CHECKING;
                mc_menuid = 2;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 0) {
            mc_alloc();
            mcErr_ID = MC_ERR_NONE;
            mc_menuid = 1;
        }
        break;
    case 2:
        mc_check_always(1);
        break;
    case 3:
        if (mc_type == 0) {
            mc_menuid = 4;
            if (*(const int *)(mc_databuff[mcFile_ID] + MC_DATA_USED) == 0) {
                mcMain_ID = 6;
            } else {
                mcErr_ID = MC_ERR_OVERWRITE_ASK;
                mcMain_ID = 5;
                fream_counter = MC_FRAME_COUNTER_NOW;
            }
        } else if (mc_formatcheck() == 0) {
            mc_menuid = 0;
            mcErr_ID = MC_ERR_MSG_06;
            mcMain_ID = 5;
            fream_counter = MC_FRAME_COUNTER_NOW;
        } else {
            ret = mc_free_area();
            if (ret == 2) {
                mcErr_ID = MC_ERR_MSG_0E;
                mcMain_ID = 0;
            } else if (ret == 1) {
                mcErr_ID = MC_ERR_MSG_11;
                mcMain_ID = 0;
            } else if (ret == 0) {
                mc_menuid = 2;
                fream_counter = MC_FRAME_COUNTER_NOW;
                mcErr_ID = MC_ERR_NEW_DATA;
                mcMain_ID = 5;
            }
        }
        break;
    case 5:
        mc_check_always(0);
        break;
    case 6:
        switch (mc_menuid) {
        case 0:
            mc_menuid = 1;
            mcErr_ID = MC_ERR_FORMAT_ASK;
            break;
        case 1:
            if (sceMcFormat(mcSlot_ID, 0) < 0) {
                mcErr_ID = MC_ERR_MSG_08;
                mcMain_ID = 0;
            } else if (sceMcSync(0, NULL, &result) != 0) {
                if (result == 0) {
                    mc_menuid = 2;
                } else {
                    mcErr_ID = MC_ERR_MSG_08;
                    mcMain_ID = 0;
                }
            }
            break;
        case 2:
        case 4:
            SSPlaySE(s_seBank, MC_SE_ACCESS, 1);
            mcErr_ID = MC_ERR_SAVING;
            mc_menuid++;
            break;
        case 3:
            system_data_load_set(2);
            ret = mc_save_new(0);
            if (ret == MC_RESULT_FAIL) {
                mc_menuid = 0;
                mcErr_ID = MC_ERR_MSG_0E;
                mcMain_ID = 0;
            } else if (ret == MC_RESULT_OK) {
                mcErr_ID = MC_ERR_SAVED;
                mcMain_ID = -1;
                mc_free();
            }
            SSStopSE(s_seBank, MC_SE_ACCESS, 1);
            break;
        case 5:
            system_data_load_set(2);
            if (mc_save_normal(0) == MC_RESULT_OK) {
                mcErr_ID = MC_ERR_SAVED;
                mcMain_ID = -1;
                mc_free();
            } else {
                mcErr_ID = MC_ERR_MSG_0E;
                mcMain_ID = 0;
            }
            SSStopSE(s_seBank, MC_SE_ACCESS, 1);
            break;
        }
        break;
    }
}

// MC_ACCESS_AUTO_LOAD (mcAccess_ID 0).
static void mc_update_auto_load(void)
{
    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        mc_free();
        break;
    case 1:
        if (mc_menuid == 2) {
            if (mc_load_new() == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    system_data_load_set(0);
                    mcErr_ID = MC_ERR_NONE;
                    mcMain_ID = 2;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                } else {
                    mcErr_ID = MC_ERR_MSG_11;
                    mcMain_ID = 0;
                }
            } else if (mc_check() == 0) {
                mcErr_ID = MC_ERR_MSG_04;
                mcMain_ID = 0;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 1) {
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_CHECKING;
                mc_menuid = 2;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 0) {
            mc_alloc();
            mcErr_ID = MC_ERR_CHECKING;
            mc_menuid = 1;
        }
        break;
    case 2:
        mc_check_always(1);
        mc_menuid = 0;
        break;
    case 3:
        tiFade_Start(10.0f, 0); // PORT: the ROM leaves a0 (the fade type) unset here (open-items.md)
        mcMain_ID = 4;
        break;
    case 4:
        if (FadeEndTicks <= FadeStartTicks) {
            mcMain_ID = 5;
        }
        break;
    case 5:
        if (mc_menuid == 2) {
            mcErr_ID = MC_ERR_LOADED;
            mcMain_ID = -1;
            mc_free();
            ssTrigRegisterEvent(TRIG_EVENT_MC_LOADED, 1, 0);
        } else if (mc_menuid == 1) {
            mc_load_set();
            mc_menuid = 2;
        } else if (mc_menuid == 0) {
            mc_menuid = 1;
            mcErr_ID = MC_ERR_MSG_0E;
        }
        break;
    }
}

// CT3 0x018b6080: run one frame of the card flow mcAccess_ID names (main calls it every frame). The card
// calls wait vblanks inside, so the frame timer is put back as it was; the ROM returns early (timer not
// restored) for an mcAccess_ID it does not know. The ROM inlines the four flows; they are split here.
void MemorycardUpdate(void)
{
    // callcheck: SSPlaySE, SSStopSE, SceneSELoad, mc_Getinfo_count, mc_alloc, mc_check, mc_check4, mc_check_always, mc_check_always2, mc_formatcheck, mc_formatcheck2, mc_free, mc_free_area, mc_load_new, mc_load_set, mc_save_init, mc_save_new, mc_save_normal, printf, sceMcFormat, sceMcSync, ssTrigCheckEvent, ssTrigRegisterEvent, ssTrigRemoveEvent, system_data_load_set, tiFade_Start (the ROM inlines the four flows: the mc_update_* functions above make these calls)
    int timer = ga_main_timer;

    switch (mcAccess_ID) {
    case MC_ACCESS_SAVE:
        mc_update_save_menu();
        break;
    case MC_ACCESS_LOAD:
        mc_update_load_menu();
        break;
    case MC_ACCESS_AUTO_SAVE:
        mc_update_auto_save();
        break;
    case MC_ACCESS_AUTO_LOAD:
        mc_update_auto_load();
        break;
    case MC_ACCESS_NONE:
        mc_menuid = 0;
        break;
    default:
        return;
    }
    ga_main_timer = timer;
}

// ---- slice 3 (2026-11-10): the save path and the system data ----

// mc_save_normal's state (.sbss, the symtab's statics).
static int mc_save_normal_fd;             /* 0x0193ef8c */
static int mc_save_normal_end_size;       /* 0x0193f030 */
static int mc_save_normal_loop_count;     /* 0x0193f034 */
static int mc_save_normal_loop_count_max; /* 0x0193f038 */
static void *game_end_clear_buff;         /* 0x0193f040: the game-end image while mc_save_set copies it */

u_char continue_backup[MC_DATA_SIZE];     /* bss 0x01a31450 */

static const char s_mcIconDir[] = "data/mc_icon/";                       /* @3051 */
static const char s_gameEndFile[] = "game_end.dat";                      /* @3052 */
static const char s_gameEndError[] = "game_end.dat file open error!!\n"; /* @3053 */

// mc_check_always2 polls once every MC_CHECK_ALWAYS2_PERIOD + 1 calls.
#define MC_CHECK_ALWAYS2_PERIOD 2

// CT3 0x018bb350: is the card in slot mcSlot_ID gone or not a PS2 card? Answers 1 when so. A refused
// sceMcGetInfo answers 0 (no news); a failed sync falls back to mc_check's slow loop. A PS2 card is asked
// twice more (the second answer is the one that counts; the ROM ignores the middle sync's result).
int mc_check2(void)
{
    int result;
    int type = 0;

    if (sceMcGetInfo(mcSlot_ID, 0, &type, NULL, NULL) < 0) {
        return 0;
    }
    sceMcSync(0, NULL, &result);
    if (result < 0) {
        return mc_check();
    }
    if (type == sceMcTypePS2) {
        if (sceMcGetInfo(mcSlot_ID, 0, &type, NULL, NULL) < 0) {
            return 0;
        }
        sceMcSync(0, NULL, &result);
        if (type == sceMcTypePS2) {
            return 0;
        }
    }
    if (sceMcGetInfo(mcSlot_ID, 0, &type, NULL, NULL) < 0) {
        return 0;
    }
    sceMcSync(0, NULL, &result);
    return (type == sceMcTypePS2) ^ 1;
}

// CT3 0x018b7870: mc_check_always on mc_check2, every third call.
void mc_check_always2(int clearError)
{
    fream_counter++;
    if (fream_counter > MC_CHECK_ALWAYS2_PERIOD) {
        fream_counter = 0;
        if (mc_check2() == 0) {
            if (clearError != 0) {
                mcErr_ID = MC_ERR_NONE;
            }
        } else {
            mcErr_ID = MC_ERR_NO_CARD;
            mcMain_ID = 0;
        }
    }
}

// CT3 0x018b5d40: the auto-load's second insertion check: mark the slot when mc_check2 finds no card.
int MemorycardAutoLoadInsertedCheck2(void)
{
    int result = mc_check2();

    if (result != 0) {
        SlotErr[mcSlot_ID] = MC_SLOT_ERR_NO_CARD;
    }
    return result;
}

// CT3 0x018b6020: save image file's MC_DATA_USED word (0 for a file out of range or no work buffer).
int mc_Getinfo_count(int file)
{
    if (file < 0 || file > MC_INFO_COUNT - 1) {
        return 0;
    }
    if (alloc_flag == 0) {
        return 0;
    }
    return *(const int *)(mc_databuff[file] + MC_DATA_USED);
}

// CT3 0x018b9e90: set event bit event in the image being saved (mcFile_ID).
void McRegisterEvent(u_int event)
{
    u_char *bits = mc_databuff[mcFile_ID] + MC_DATA_EVENTS + ((int)event >> 3);

    *bits |= (u_char)(1 << (event & 7));
}

// CT3 0x018b5e70: keep the game's state as a save image in continue_backup (the continue after a death).
void ContinueBackupSet(void)
{
    data_save_set(continue_backup);
}

// CT3 0x018b5e80: whether mc_alloc's work buffer is held (PicPaz.c).
int mc_CheckMcWorkAllocc(void)
{
    return alloc_flag;
}

// CT3 0x018b5e90: the last card check's answer: 0 = the card holds the save, 1 = a new one is made.
int mc_Getinfo_mc_type(void)
{
    return mc_type;
}

// CT3 0x018b9a80: fill the work buffer for a save into file mcFile_ID: the header (id, file, the extras,
// the skip-event flags, the screen position, the key map and the options -- the sound options also go to
// the trigger system), then the image: the game's state (data_save_set), or with alt the game-end image
// from the disc (data/mc_icon/game_end.dat, OMAKE_1 set). Then the checksums, and the continue backup.
void mc_save_set(int alt)
{
    MC_HEADER *header = (MC_HEADER *)mc_header;
    int i;

    header->id = MC_HEADER_ID;
    save_count++;
    header->uidFile &= ~MC_HEADER_FILE_MASK;
    header->uidFile |= mcFile_ID;
    if (alt != 0) {
        OMAKE_FLAG |= OMAKE_1;
        ssTrigRegisterEvent(TRIG_EVENT_OMAKE_1, 1, 0);
    }
    header->omake = OMAKE_FLAG;
    for (i = 0; i < MC_SKIP_EVENT_COUNT; i++) {
        header->skipEvent[i] = SkipEventFlag[i];
    }
    memcpy(&header->display, &ga_disp.display, sizeof(header->display));
    for (i = 0; i < MC_KEY_MAP_COUNT; i++) {
        header->keyMap[i] = KEY_MAP[i];
    }
    header->vibration = fVibration;
    header->stereo = SOUND_STEREO_MONAURAL_BUFF;
    header->bgmVolume = BGM_MASTER_VOLUME_BUFF;
    header->seVolume = SE_MASTER_VOLUME_BUFF;
    if (SOUND_STEREO_MONAURAL_BUFF == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_MONAURAL, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_MONAURAL, 1, 0);
    }
    ssTrigSetVariable(TRIG_VAR_BGM_VOLUME, BGM_MASTER_VOLUME_BUFF, 1, 0);
    ssTrigSetVariable(TRIG_VAR_SE_VOLUME, SE_MASTER_VOLUME_BUFF, 1, 0);
    /* The ROM keeps the low 4 bits and ORs the uid in shifted by 8 (not 4): kept. */
    header->uidFile &= MC_HEADER_FILE_MASK;
    header->uidFile |= AA_UidCount << MC_HEADER_UID_SHIFT;
    if (alt == 0) {
        data_save_set(mc_databuff[mcFile_ID]);
    } else {
        game_end_clear_buff = _SunAlloc(&gHeapSound, MC_GAME_END_SIZE, MC_GAME_END_ALIGN);
        if (FlLoad(s_mcIconDir, s_gameEndFile, game_end_clear_buff) < 0) {
            printf(s_gameEndError);
            for (;;) {
                /* the ROM hangs here */
            }
        }
        memcpy(mc_databuff[mcFile_ID], game_end_clear_buff, MC_DATA_SIZE);
        *(int *)(mc_databuff[mcFile_ID] + MC_DATA_USED) = 1;
        *(int *)(mc_databuff[mcFile_ID] + MC_DATA_C714) = 0;
        McRegisterEvent(MC_EVENT_GAME_END);
        _SunFree(&gHeapSound, &game_end_clear_buff);
    }
    mc_make_checksum(1);
    ContinueBackupSet();
}

// CT3 0x018b9790: write the work buffer (mc_save_set(alt) fills it after the open) over the save file
// (mc_fname twice) on the card in slot mcSlot_ID: mc_load_new's steps with sceMcWrite. Runs until done,
// one vblank after each refused call; answers MC_RESULT_OK or MC_RESULT_FAIL.
int mc_save_normal(int alt)
{
    char path[0x40c];
    int result;
    int ret = MC_RESULT_OK;

    mc_save_normal_fd = 0;
    mc_save_normal_num = 0;
    for (;;) {
        switch (mc_save_normal_num) {
        case 0:
            mc_save_normal_fd = 0;
            sprintf(path, s_pathFormat, mc_fname, mc_fname);
            ret = MC_RESULT_BUSY;
            if (sceMcOpen(mcSlot_ID, 0, path, MC_OPEN_WRITE) != 0) {
                break;
            }
            mc_save_normal_num++;
            sceMcSync(0, NULL, &mc_save_normal_fd);
            ret = MC_RESULT_FAIL;
            if (mc_save_normal_fd < 0) {
                break;
            }
            /* fall through */
        case 1:
            mc_save_set(alt);
            mc_save_normal_loop_count = 0;
            mc_save_normal_loop_count_max = MC_LOAD_CHUNKS;
            mc_save_normal_end_size = MC_LOAD_TAIL;
            mc_save_normal_num++;
            /* fall through */
        case 2:
            ret = MC_RESULT_OK;
            for (; mc_save_normal_loop_count != mc_save_normal_loop_count_max; mc_save_normal_loop_count++) {
                if (sceMcWrite(mc_save_normal_fd, mc_data_buff + mc_save_normal_loop_count * MC_LOAD_CHUNK,
                               MC_LOAD_CHUNK) != 0) {
                    ret = MC_RESULT_BUSY;
                    break;
                }
                sceMcSync(0, NULL, &result);
                if (result < 0) {
                    ret = MC_RESULT_BUSY;
                    mc_save_normal_num = MC_STEP_ERROR;
                    break;
                }
            }
            if (ret != MC_RESULT_OK) {
                break;
            }
            mc_save_normal_num++;
            /* fall through */
        case 3:
            ret = MC_RESULT_BUSY;
            if (sceMcWrite(mc_save_normal_fd, mc_data_buff + mc_save_normal_loop_count * MC_LOAD_CHUNK,
                           mc_save_normal_end_size) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            mc_save_normal_num++;
            if (result < 0) {
                mc_save_normal_num = MC_STEP_ERROR;
                break;
            }
            /* fall through */
        case 4:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_save_normal_fd) != 0) {
                break;
            }
            mc_save_normal_num++;
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;
            if (result >= 0) {
                mc_save_normal_num = 0;
                ret = MC_RESULT_OK;
            }
            break;
        case MC_STEP_ERROR:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_save_normal_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;       /* the ROM tests the result and fails either way */
            break;
        }
        if (ret == MC_RESULT_FAIL || ret == MC_RESULT_OK) {
            mc_save_normal_num = 0;
            return ret;
        }
        sceGsSyncV(0);
    }
}

// CT3 0x018b8680: take the system data back from the header after a load. SYSTEM_LOAD_FILE_ID: only the
// last file; SYSTEM_LOAD_ALL: the screen position, key map and options (pushed to the trigger system and
// the sound driver), OMAKE_2, then as SYSTEM_LOAD_UNLOCKS: OMAKE_1 and the skip-event flags OR-ed in.
void system_data_load_set(int mode)
{
    const MC_HEADER *header = (const MC_HEADER *)mc_header;
    u_int omake1;
    int i;

    if (mode == SYSTEM_LOAD_FILE_ID) {
        mcFile_ID = header->uidFile & MC_HEADER_FILE_MASK;
        return;
    }
    if (mode != SYSTEM_LOAD_ALL && mode != SYSTEM_LOAD_UNLOCKS) {
        return;
    }
    if (mode == SYSTEM_LOAD_ALL) {
        memcpy(&ga_disp.display, &header->display, sizeof(header->display));
        for (i = 0; i < MC_KEY_MAP_COUNT; i++) {
            KEY_MAP[i] = header->keyMap[i];
        }
        KEY_MAP[KEY_MAP_LOAD_DST] = KEY_MAP[KEY_MAP_LOAD_SRC];
        KEY_MAP[KEY_MAP_LOAD_DST + 1] = KEY_MAP[KEY_MAP_LOAD_SRC + 1];
        fVibration = header->vibration;
        SOUND_STEREO_MONAURAL_BUFF = header->stereo;
        BGM_MASTER_VOLUME_BUFF = header->bgmVolume;
        SE_MASTER_VOLUME_BUFF = header->seVolume;
        if (SOUND_STEREO_MONAURAL_BUFF == 0) {
            ssTrigRemoveEvent(TRIG_EVENT_MONAURAL, 1, 0);
        } else {
            ssTrigRegisterEvent(TRIG_EVENT_MONAURAL, 1, 0);
        }
        ssTrigSetVariable(TRIG_VAR_BGM_VOLUME, BGM_MASTER_VOLUME_BUFF, 1, 0);
        ssTrigSetVariable(TRIG_VAR_SE_VOLUME, SE_MASTER_VOLUME_BUFF, 1, 0);
        SSSetSoundMode(SOUND_MODE_STEREO);
        SSSetSoundMode(SOUND_MODE_BGM_VOLUME);
        SSSetSoundMode(SOUND_MODE_SE_VOLUME);
        /* The ROM keeps only OMAKE_2 here (OMAKE_1 and OMAKE_BOOTED drop until the OR below). */
        OMAKE_FLAG = (OMAKE_FLAG & OMAKE_2) | (header->omake & OMAKE_2);
        if (OMAKE_FLAG == 0) {
            ssTrigRemoveEvent(TRIG_EVENT_OMAKE_2, 1, 0);
        } else {
            ssTrigRegisterEvent(TRIG_EVENT_OMAKE_2, 1, 0);
        }
    }
    omake1 = header->omake & OMAKE_1;
    if ((OMAKE_FLAG & OMAKE_1) == 0 && omake1 == 0) {
        OMAKE_FLAG |= omake1;
        ssTrigRemoveEvent(TRIG_EVENT_OMAKE_1, 1, 0);
    } else {
        OMAKE_FLAG |= omake1;
        ssTrigRegisterEvent(TRIG_EVENT_OMAKE_1, 1, 0);
    }
    for (i = 0; i < MC_SKIP_EVENT_COUNT; i++) {
        SkipEventFlag[i] |= header->skipEvent[i];
    }
}

// ---- mc_save_new: a new save directory, its icons, the save and icon.sys ----

// icon.sys's colours and lights (.data 0x019399f0..0x01939aa0, the ROM's values).
int mc_bgcolor[4][4] = {{42, 42, 42, 0}, {82, 75, 50, 0}, {82, 75, 50, 0}, {53, 40, 1, 0}};   /* 0x019399f0 */
float mc_lightdir[3][4] = {{11.72f, 15.19f, 12.64f, 0.0f}, {-9.7f, -6.29f, 4.33f, 0.0f},    /* 0x01939a30 */
                           {-7.88f, 7.49f, -7.55f, 0.0f}};
float mc_lightcol[3][4] = {{50.0f / 255, 50.0f / 255, 50.0f / 255, 0.0f},                    /* 0x01939a60 */
                           {61.0f / 255, 63.0f / 255, 111.0f / 255, 0.0f},
                           {40.0f / 255, 48.0f / 255, 30.0f / 255, 0.0f}};
float mc_ambient[4] = {100.0f / 255, 100.0f / 255, 100.0f / 255, 0.0f};                      /* 0x01939a90 */

// mc_save_new's state (.sbss, the symtab's statics).
static int mc_save_new_count;             /* 0x0193ef98: the icon file being written (0..2) */
static int mc_save_new_num;               /* 0x0193ef9c: the step */
static int mc_save_new_fd;                /* 0x0193ef94 */
static int mc_save_new_size;              /* 0x0193efa0: the icon file's size */
static int mc_save_new_end_size;          /* 0x0193efd0 */
static int mc_save_new_loop_count;        /* 0x0193efd4 */
static int mc_save_new_loop_count_max;    /* 0x0193efd8 */
static void *mc_save_new_rwbuff;          /* 0x0193efdc: the icon file, from gHeapJunc */
static int mc_save_new_len;               /* 0x0193efe0: strlen(mc_fname) in mc_save_new_icon_fname */
static char mc_save_new_icon_fname[0x40]; /* 0x0193eff0 */

// mc_save_new's constants (.data 0x01939b00..0x01939b8c, the function's locals' initialisers).
static const char s_iconHead[4] = {'P', 'S', '2', 'D'};                /* @836 */
// @837: "ＣｌｏｃｋＴｏｗｅｒ３" (Shift-JIS, full-width), the title the PS2 browser shows.
static const char s_iconTitle[0x44] = {
    '\x82', '\x62', '\x82', '\x8c', '\x82', '\x8f', '\x82', '\x83', '\x82', '\x8b', /* ＣｌｏｃｋＴ */
    '\x82', '\x73', '\x82', '\x8f', '\x82', '\x97', '\x82', '\x85', '\x82', '\x92', /* ｏｗｅｒ   */
    '\x82', '\x52',                                                                 /* ３          */
};
static const char s_iconDir[0x20] = "data/mc_icon/";                   /* @838 */
// @839: the icons the browser shows -- FnameView, FnameCopy (the same file), FnameDel.
static const char *const s_iconFiles[MC_ICON_FILES] = {"alyssas.ico", "alyssas.ico", "alyssad.ico"};
static const char s_iconSysName[] = "/icon.sys";                       /* @2952 */

// CT3 0x018b8eb0: make a new save on the card in slot mcSlot_ID: the directory mc_fname (made when
// sceMcChdir says it is missing), the three icon files read from the disc (data/mc_icon/), the save file
// (mc_save_set(alt), then mc_save_normal's chunked writes) and icon.sys. Steps as the ROM numbers them;
// runs until done, one vblank after each refused call; answers MC_RESULT_OK or MC_RESULT_FAIL.
int mc_save_new(int alt)
{
    char path[0x400];
    sceMcIconSys icon;
    int result;
    int ret = MC_RESULT_OK;
    size_t len;

    mc_save_new_fd = 0;
    mc_save_new_count = 0;
    mc_save_new_num = 0;
    mc_save_new_size = 0;
    for (;;) {
        switch (mc_save_new_num) {
        case MC_NEW_CHDIR:
            result = sceMcChdir(mcSlot_ID, 0, mc_fname, NULL);
            ret = MC_RESULT_BUSY;
            if (result != 0) {
                break;
            }
            mc_save_new_num = MC_NEW_OPEN_SAVE;
            sceMcSync(0, NULL, &result);
            if (result == sceMcResNoEntry) {
                mc_save_new_num = MC_NEW_MKDIR;
            } else if (result < 0) {
                ret = MC_RESULT_FAIL;
            } else {
                /* The directory exists: the ROM still writes the icons again (step 2, not 10). */
                mc_save_new_num = MC_NEW_ICONS;
            }
            break;
        case MC_NEW_MKDIR:
            ret = MC_RESULT_BUSY;
            if (sceMcMkdir(mcSlot_ID, 0, mc_fname) != 0) {
                break;
            }
            mc_save_new_num++;
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;
            if (result < 0) {
                break;
            }
            /* fall through */
        case MC_NEW_ICONS:
            strcpy(mc_save_new_icon_fname, mc_fname);
            mc_save_new_len = (int)strlen(mc_save_new_icon_fname);
            mc_save_new_count = 0;
            mc_save_new_num = MC_NEW_LOAD_ICON;
            /* fall through */
        case MC_NEW_LOAD_ICON:
            mc_save_new_size = FlGetLen(s_iconDir, s_iconFiles[mc_save_new_count], 1);
            mc_save_new_rwbuff = _SunAlloc(&gHeapJunc, mc_save_new_size + MC_ICON_PAD, MC_ICON_ALIGN);
            FlLoad(s_iconDir, s_iconFiles[mc_save_new_count], mc_save_new_rwbuff);
            mc_save_new_icon_fname[mc_save_new_len] = '/';
            strcpy(mc_save_new_icon_fname + mc_save_new_len + 1, s_iconFiles[mc_save_new_count]);
            mc_save_new_num++;
            /* fall through */
        case MC_NEW_OPEN_ICON:
            ret = MC_RESULT_BUSY;
            if (sceMcOpen(mcSlot_ID, 0, mc_save_new_icon_fname, MC_OPEN_CREATE) != 0) {
                break;
            }
            mc_save_new_num++;
            sceMcSync(0, NULL, &mc_save_new_fd);
            if (mc_save_new_fd < 0) {
                _SunFree(&gHeapJunc, &mc_save_new_rwbuff);
                ret = MC_RESULT_FAIL;
                break;
            }
            /* fall through */
        case MC_NEW_WRITE_ICON:
            ret = MC_RESULT_BUSY;
            if (sceMcWrite(mc_save_new_fd, mc_save_new_rwbuff, mc_save_new_size) != 0) {
                break;
            }
            mc_save_new_num += 2;       /* the ROM skips step 8 */
            sceMcSync(0, NULL, &result);
            if (result < 0) {
                _SunFree(&gHeapJunc, &mc_save_new_rwbuff);
                mc_save_new_num = MC_STEP_ERROR;
                break;
            }
            /* fall through */
        case MC_NEW_CLOSE_ICON:
            /* Freed before the close: a refused close frees again on the retry (null by then). */
            _SunFree(&gHeapJunc, &mc_save_new_rwbuff);
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_save_new_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;
            if (result < 0) {
                break;
            }
            mc_save_new_count++;
            if (mc_save_new_count < MC_ICON_FILES) {
                ret = MC_RESULT_BUSY;
                mc_save_new_icon_fname[mc_save_new_len] = '\0';
                mc_save_new_num = MC_NEW_LOAD_ICON;
                break;
            }
            mc_save_new_num++;
            /* fall through */
        case MC_NEW_OPEN_SAVE:
            sprintf(path, s_pathFormat, mc_fname, mc_fname);
            ret = MC_RESULT_BUSY;
            if (sceMcOpen(mcSlot_ID, 0, path, MC_OPEN_CREATE) != 0) {
                break;
            }
            sceMcSync(0, NULL, &mc_save_new_fd);
            ret = MC_RESULT_FAIL;
            if (mc_save_new_fd < 0) {
                break;
            }
            mc_save_set(alt);
            mc_save_new_loop_count = 0;
            mc_save_new_loop_count_max = MC_LOAD_CHUNKS;
            mc_save_new_end_size = MC_LOAD_TAIL;
            mc_save_new_num++;
            /* fall through */
        case MC_NEW_WRITE_SAVE:
            ret = MC_RESULT_OK;
            for (; mc_save_new_loop_count != mc_save_new_loop_count_max; mc_save_new_loop_count++) {
                if (sceMcWrite(mc_save_new_fd, mc_data_buff + mc_save_new_loop_count * MC_LOAD_CHUNK,
                               MC_LOAD_CHUNK) != 0) {
                    ret = MC_RESULT_BUSY;
                    break;
                }
                sceMcSync(0, NULL, &result);
                if (result < 0) {
                    ret = MC_RESULT_BUSY;
                    mc_save_new_num = MC_STEP_ERROR;
                    break;
                }
            }
            if (ret != MC_RESULT_OK) {
                break;
            }
            mc_save_new_num++;
            /* fall through */
        case MC_NEW_WRITE_TAIL:
            ret = MC_RESULT_BUSY;
            if (sceMcWrite(mc_save_new_fd, mc_data_buff + mc_save_new_loop_count * MC_LOAD_CHUNK,
                           mc_save_new_end_size) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            mc_save_new_num++;
            if (result < 0) {
                mc_save_new_num = MC_STEP_ERROR;
                break;
            }
            /* fall through */
        case MC_NEW_CLOSE_SAVE:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_save_new_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;
            if (result < 0) {
                break;
            }
            mc_save_new_num = MC_NEW_OPEN_ICONSYS;
            /* fall through */
        case MC_NEW_OPEN_ICONSYS:
            strcpy(mc_save_new_icon_fname, mc_fname);
            mc_save_new_len = (int)strlen(mc_save_new_icon_fname);
            strcpy(mc_save_new_icon_fname + mc_save_new_len, s_iconSysName);
            ret = MC_RESULT_BUSY;
            if (sceMcOpen(mcSlot_ID, 0, mc_save_new_icon_fname, MC_OPEN_CREATE) != 0) {
                break;
            }
            sceMcSync(0, NULL, &mc_save_new_fd);
            ret = MC_RESULT_FAIL;
            if (mc_save_new_fd < 0) {
                break;
            }
            mc_save_new_num++;
            /* fall through */
        case MC_NEW_BUILD_ICONSYS:
            memset(&icon, 0, sizeof(icon));
            memcpy(icon.Head, s_iconHead, sizeof(icon.Head));
            len = strlen(s_iconTitle);
            memcpy(icon.TitleName, s_iconTitle, len);
            icon.TransRate = MC_ICON_TRANS_RATE;
            icon.OffsLF = (unsigned short)len;  /* one line: the break is at the title's end */
            memcpy(icon.BgColor, mc_bgcolor, sizeof(icon.BgColor));
            memcpy(icon.LightDir, mc_lightdir, sizeof(icon.LightDir));
            memcpy(icon.LightColor, mc_lightcol, sizeof(icon.LightColor));
            memcpy(icon.Ambient, mc_ambient, sizeof(icon.Ambient));
            memcpy(icon.FnameView, s_iconFiles[0], strlen(s_iconFiles[0]));
            memcpy(icon.FnameCopy, s_iconFiles[1], strlen(s_iconFiles[1]));
            memcpy(icon.FnameDel, s_iconFiles[2], strlen(s_iconFiles[2]));
            mc_save_new_num++;
            /* fall through */
        case MC_NEW_WRITE_ICONSYS:
            ret = MC_RESULT_BUSY;
            if (sceMcWrite(mc_save_new_fd, &icon, sizeof(icon)) != 0) {
                break;
            }
            mc_save_new_num++;
            sceMcSync(0, NULL, &result);
            if (result < 0) {
                mc_save_new_num = MC_STEP_ERROR;
                break;
            }
            /* fall through */
        case MC_NEW_CLOSE_ICONSYS:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_save_new_fd) != 0) {
                break;
            }
            mc_save_new_num++;
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;
            if (result >= 0) {
                mc_save_new_num = 0;
                ret = MC_RESULT_OK;
            }
            break;
        case MC_STEP_ERROR:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_save_new_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;       /* the ROM tests the result and fails either way */
            break;
        }
        if (ret == MC_RESULT_FAIL || ret == MC_RESULT_OK) {
            mc_save_new_num = 0;
            return ret;
        }
        sceGsSyncV(0);
    }
}

// ---- stubs (tools/reverse/stubgen.py) ----


// CT3 0x018bb2d0: whether slot mcSlot_ID lacks a usable card (1): the library refused, the sync failed,
// or a card that is not a PS2 card; a PS2 card is then checked through mc_check.
int mc_check3(void)
{
    int type = 0;
    int result;

    if (sceMcGetInfo(mcSlot_ID, 0, &type, NULL, NULL) < 0) {
        return 1;
    }
    sceMcSync(0, NULL, &result);
    if (result < 0 || type != sceMcTypePS2) {
        return 1;
    }
    return mc_check() != 0;
}

// CT3 0x018badd0: write the system data into the header without touching the uid or the save count
// (mc_save_set's header half): id, last file, the extras, the skip-event flags, the screen position, the
// key map and the options; the sound options also go to the trigger system.
void mc_save_system_init(void)
{
    MC_HEADER *header = (MC_HEADER *)mc_header;
    int i;

    header->id = MC_HEADER_ID;
    header->uidFile &= ~MC_HEADER_FILE_MASK;
    header->uidFile |= mcFile_ID;
    header->omake = OMAKE_FLAG;
    for (i = 0; i < MC_SKIP_EVENT_COUNT; i++) {
        header->skipEvent[i] = SkipEventFlag[i];
    }
    memcpy(&header->display, &ga_disp.display, sizeof(header->display));
    for (i = 0; i < MC_KEY_MAP_COUNT; i++) {
        header->keyMap[i] = KEY_MAP[i];
    }
    header->vibration = fVibration;
    header->stereo = SOUND_STEREO_MONAURAL_BUFF;
    header->bgmVolume = BGM_MASTER_VOLUME_BUFF;
    header->seVolume = SE_MASTER_VOLUME_BUFF;
    if (SOUND_STEREO_MONAURAL_BUFF == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_MONAURAL, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_MONAURAL, 1, 0);
    }
    ssTrigSetVariable(TRIG_VAR_BGM_VOLUME, BGM_MASTER_VOLUME_BUFF, 1, 0);
    ssTrigSetVariable(TRIG_VAR_SE_VOLUME, SE_MASTER_VOLUME_BUFF, 1, 0);
}

// CT3 0x018baac0: whether slot mcSlot_ID has too little room for a new save (1): its free clusters (1 KB),
// read with mc_check's retries and settled once MC_CHECK_STABLE_READS reads agree, against the two icon
// files rounded up to clusters plus MC_SAVE_CLUSTERS. A refused or failed read counts as no room.
int mc_free_area(void)
{
    int iconS = (FlGetLen(s_mcIconDir, s_iconFiles[0], 1) + MC_CLUSTER_SIZE - 1) / MC_CLUSTER_SIZE;
    int iconD = (FlGetLen(s_mcIconDir, s_iconFiles[2], 1) + MC_CLUSTER_SIZE - 1) / MC_CLUSTER_SIZE;
    int freeClusters = 0;
    int result;
    int tries;
    int fails = 0;
    int same = 0;
    int seen[MC_CHECK_STABLE_READS];

    for (;;) {
        tries = 0;
        while (sceMcGetInfo(mcSlot_ID, 0, NULL, &freeClusters, NULL) != 0) {
            if (tries > MC_CHECK_INFO_RETRIES) {
                freeClusters = 0;
                goto settled;
            }
            tries++;
            fails = 0;
            same = 0;
            sceGsSyncV(0);
        }
        sceMcSync(0, NULL, &result);
        if (result < 0) {
            freeClusters = 0;
            if (fails > MC_CHECK_SYNC_RETRIES) {
                goto settled;
            }
            fails++;
            same = 0;
        } else {
            /* mc_check's order, kept (a mismatch restarts the count without moving the answer). */
            seen[same] = freeClusters;
            if (same > 0) {
                if (seen[same - 1] == seen[same]) {
                    if (same == MC_CHECK_STABLE_READS - 1) {
                        goto settled;
                    }
                } else {
                    fails = 0;
                    same = 0;
                }
            }
            same++;
        }
        sceGsSyncV(0);
    }

settled:
    return freeClusters < iconS + iconD + MC_SAVE_CLUSTERS;
}

// CT3 0x018b5d80: the auto-load's insertion check: mark the slot when mc_check finds no PS2 card.
int MemorycardAutoLoadInsertedCheck(void)
{
    int result = mc_check();

    if (result != 0) {
        SlotErr[mcSlot_ID] = MC_SLOT_ERR_NO_CARD;
    }
    return result;
}

// CT3 0x018b5dc0: the auto-load's room check: mark the slot when a new save would not fit.
int MemorycardAutoLoadFreeCheck(void)
{
    int result = mc_free_area();

    if (result != 0) {
        SlotErr[mcSlot_ID] = MC_SLOT_ERR_NO_ROOM;
    }
    return result;
}

// CT3 0x018b5c60: start the auto-load (xcmd_tn.c): take the work buffer, slot 1 and the first state,
// the "checking" message after MC_AUTO_LOAD_DRAW_WAIT frames; the title selection is cleared.
void MemorycardAutoLoadInit(void)
{
    mc_alloc();
    mcMain_ID = 0;
    mcSlot_ID = 0;
    DrawWaitCounter = MC_AUTO_LOAD_DRAW_WAIT;
    mcErr_ID = MC_ERR_CHECKING;
    ssTrigSetVariable(TRIG_VAR_TITLE_SELECT, 0, 1, 0);
    TITLE_SELECT_NUM = 0;
    ssTrigRemoveEvent(TRIG_EVENT_TITLE_CANCEL, 1, 0);
    TITLE_CANCEL_FLAG = 0;
}

// CT3 0x018b5e00: continue after a death: the game comes back from continue_backup (data_load_set) with
// the play time kept, the panic mode reset, the AI, menu and player control restarted, the trigger
// system told (TRIG_EVENT_MC_RELOADED), and a fade in while main skips four frames (mc_load_set's end).
void ContinueBackupGet(void)
{
    data_load_set(continue_backup);
    progress_time = backup_progress_time;
    PanicModeInit();
    MonsterAIUpdate(1);
    ssMenuUpdate(1);
    PlayerControlUpdate(1);
    ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
    asMothPause(1);
    tiFade_Start(0.0f, FADE_TYPE_MC_LOAD);
    SkipMain = MC_LOAD_SKIP_FRAMES;
}

// CT3 0x018b5ea0: whether save image file was saved after the game was cleared once (its event
// TRIG_EVENT_CLEARED; PicPaz.c's file list). 0 for a file out of range or no work buffer.
int mc_Getinfo_ClearFlag(int file)
{
    const MC_SAVE_DATA *save;

    if (file < 0 || file > MC_INFO_COUNT - 1 || alloc_flag == 0) {
        return 0;
    }
    save = (const MC_SAVE_DATA *)mc_databuff[file];
    return (save->globalEventFlag[TRIG_EVENT_CLEARED >> 3] >> (TRIG_EVENT_CLEARED & 7)) & 1;
}

// CT3 0x018b5f00: save image file's branch (its trigger variable TRIG_VAR_STAGE); 1 for a file out of
// range or no work buffer.
int mc_Getinfo_BranchNo(int file)
{
    if (file < 0 || file > MC_INFO_COUNT - 1 || alloc_flag == 0) {
        return 1;
    }
    return ((const MC_SAVE_DATA *)mc_databuff[file])->globalVariable[TRIG_VAR_STAGE];
}

// CT3 0x018b5f60: save image file's area message (GetAreaNumber(0) when saved); 0 if out of range.
int mc_Getinfo_msg_num(int file)
{
    if (file < 0 || file > MC_INFO_COUNT - 1 || alloc_flag == 0) {
        return 0;
    }
    return ((const MC_SAVE_DATA *)mc_databuff[file])->area;
}

// CT3 0x018b5fc0: save image file's play time (progress_time, frames); 0 if out of range.
int mc_Getinfo_time(int file)
{
    if (file < 0 || file > MC_INFO_COUNT - 1 || alloc_flag == 0) {
        return 0;
    }
    return ((const MC_SAVE_DATA *)mc_databuff[file])->progressTime;
}

// CT3 0x018b5740: the auto-load at boot (xcmd_tn.c's tnXcmd_AutoLoad, once per frame; answer: the
// player's answer to the message on screen, MC_AUTO_ANSWER_*). Each slot in turn is checked (card,
// format, the save loaded); a save with the game's header is taken (its system data), from slot 1 first.
// Without one, the message says why (no card, no room, broken data) and waits: for a card to appear,
// for the player to go on without saving (the system data written to the header) or to check again.
// Answers 1 once the flow ends (the work buffer is freed then).
int MemorycardAutoLoad(int answer)
{
    if (mcMain_ID == MC_AUTO_END) {
        mcMain_ID = MC_FLOW_IDLE;
        mcSlot_ID = -1;
        mcFile_ID = -1;
        mcErr_ID = -1;
        mcAccess_ID = MC_ACCESS_NONE;
    } else if (mcMain_ID == MC_AUTO_FADE) {
        if (FadeOutCounter == 0) {
            mcMain_ID = MC_AUTO_END;
        } else {
            FadeOutCounter--;
        }
    } else if (mcMain_ID == MC_AUTO_SHOW) {
        mcMain_ID = MC_AUTO_FADE;
        FadeOutCounter = MC_AUTO_SHOW_FRAMES;
    } else if (mcMain_ID == MC_AUTO_LOADED) {
        if (SlotErr[0] == MC_SLOT_ERR_LOADED) {
            mcErr_ID = MC_AUTO_ERR_LOADED_1;
            LOAD_SLOT_NUM = 0;
            ssTrigRemoveEvent(TRIG_EVENT_LOAD_SLOT2, 1, 0);
        }
        if (SlotErr[1] == MC_SLOT_ERR_LOADED) {
            mcErr_ID = MC_AUTO_ERR_LOADED_2;
            LOAD_SLOT_NUM = 1;
            ssTrigRegisterEvent(TRIG_EVENT_LOAD_SLOT2, 1, 0);
        }
        mcMain_ID = MC_AUTO_SHOW;
    } else if (mcMain_ID == MC_AUTO_BROKEN) {
        if (SlotErr[0] == MC_SLOT_ERR_BROKEN) {
            mcErr_ID = MC_AUTO_ERR_BROKEN_1;
        }
        if (SlotErr[1] == MC_SLOT_ERR_BROKEN) {
            mcErr_ID = MC_AUTO_ERR_BROKEN_2;
        }
        mcMain_ID = MC_AUTO_SHOW;
    } else if (mcMain_ID == MC_AUTO_RESTART) {
        mcSlot_ID = 0;
        mcErr_ID = MC_ERR_CHECKING;
        mcMain_ID = MC_AUTO_WAIT;
    } else if (mcMain_ID == MC_AUTO_NO_SAVE) {
        mc_save_system_init();
        mcMain_ID = MC_AUTO_END;
    } else if (mcMain_ID == MC_AUTO_ASK) {
        if (answer == MC_AUTO_ANSWER_CONTINUE) {
            mcMain_ID = MC_AUTO_NO_SAVE;
        }
        if (answer == MC_AUTO_ANSWER_RETRY) {
            mcMain_ID = MC_AUTO_RESTART;
        }
        if (mcErr_ID == MC_AUTO_ERR_NO_CARD) {
            // No card anywhere: look in each slot in turn, every MC_AUTO_POLL_WAIT + 1 frames.
            if (FadeOutCounter == 0) {
                FadeOutCounter = MC_AUTO_POLL_WAIT;
                mcSlot_ID ^= 1;
                if (MemorycardAutoLoadInsertedCheck3() == 0) {
                    mcMain_ID = MC_AUTO_RESTART;
                }
            } else {
                FadeOutCounter--;
            }
        } else {
            if (mcErr_ID == MC_AUTO_ERR_FULL_1) {
                mcSlot_ID = 0;
            }
            if (mcErr_ID == MC_AUTO_ERR_FULL_2) {
                mcSlot_ID = 1;
            }
            if (mcErr_ID == MC_AUTO_ERR_FULL_BOTH) {
                mcSlot_ID ^= 1;
            }
            if (mcErr_ID == MC_AUTO_ERR_FULL_1 || mcErr_ID == MC_AUTO_ERR_FULL_2 ||
                mcErr_ID == MC_AUTO_ERR_FULL_BOTH) {
                // A full card: start again once it is taken out.
                if (FadeOutCounter == 0) {
                    FadeOutCounter = MC_AUTO_POLL_WAIT;
                    if (MemorycardAutoLoadInsertedCheck2() != 0) {
                        mcMain_ID = MC_AUTO_RESTART;
                    }
                } else {
                    FadeOutCounter--;
                }
            }
        }
    } else if (mcMain_ID == MC_AUTO_DECIDE) {
        mcMain_ID = MC_AUTO_NO_SAVE;
        if (SlotErr[0] == MC_SLOT_ERR_NO_CARD && SlotErr[1] == MC_SLOT_ERR_NO_CARD) {
            FadeOutCounter = 0;
            mcErr_ID = MC_AUTO_ERR_NO_CARD;
            mcMain_ID = MC_AUTO_ASK;
        }
        if (SlotErr[0] == MC_SLOT_ERR_NO_ROOM && SlotErr[1] == MC_SLOT_ERR_NO_CARD) {
            mcErr_ID = MC_AUTO_ERR_FULL_1;
            mcMain_ID = MC_AUTO_ASK;
        }
        if (SlotErr[0] == MC_SLOT_ERR_NO_CARD && SlotErr[1] == MC_SLOT_ERR_NO_ROOM) {
            mcErr_ID = MC_AUTO_ERR_FULL_2;
            mcMain_ID = MC_AUTO_ASK;
        }
        if (SlotErr[0] == MC_SLOT_ERR_NO_ROOM && SlotErr[1] == MC_SLOT_ERR_NO_ROOM) {
            mcErr_ID = MC_AUTO_ERR_FULL_BOTH;
            mcMain_ID = MC_AUTO_ASK;
        }
    } else if (mcMain_ID == MC_AUTO_NEXT_SLOT) {
        if (mcSlot_ID == 0) {
            mcSlot_ID = 1;
            mcErr_ID = MC_ERR_NO_CARD;
            mcMain_ID = MC_AUTO_WAIT;
        } else {
            mcMain_ID = MC_AUTO_DECIDE;
        }
    } else if (mcMain_ID == MC_AUTO_CHECK) {
        SlotErr[mcSlot_ID] = 0;
        if (MemorycardAutoLoadInsertedCheck() != 0) {
            mcMain_ID = MC_AUTO_NEXT_SLOT;
        } else if (MemorycardAutoLoadFormatCheck() != 0) {
            MemorycardAutoLoadInsertedCheck();
            mcMain_ID = MC_AUTO_NEXT_SLOT;
        } else if (mc_load_new() != 0) {
            // No save on the card: is there room for one?
            if (MemorycardAutoLoadInsertedCheck() == 0) {
                MemorycardAutoLoadFreeCheck();
            }
            mcMain_ID = MC_AUTO_NEXT_SLOT;
        } else if (mc_broke_data[MC_BROKE_SYSTEM] == 0 && *(const int *)mc_header == MC_HEADER_ID) {
            system_data_load_set(SYSTEM_LOAD_ALL);
            SlotErr[mcSlot_ID] = MC_SLOT_ERR_LOADED;
            mcMain_ID = MC_AUTO_LOADED;
            ssTrigSetVariable(TRIG_VAR_TITLE_SELECT, 1, 1, 0);
            TITLE_SELECT_NUM = 1;
        } else {
            if (MemorycardAutoLoadInsertedCheck() == 0) {
                SlotErr[mcSlot_ID] = MC_SLOT_ERR_BROKEN;
            }
            mcMain_ID = MC_AUTO_BROKEN;
        }
    } else if (mcMain_ID == MC_AUTO_WAIT) {
        if (DrawWaitCounter == 0) {
            mcMain_ID = MC_AUTO_CHECK;
            DrawWaitCounter = MC_AUTO_LOAD_DRAW_WAIT;
        } else {
            DrawWaitCounter--;
        }
    }
    if (mcMain_ID == MC_AUTO_END) {
        mc_free();
        return 1;
    }
    return 0;
}

// CT3 0x018bb600: keep the card work buffer between flows (PicPaz.c).
void Set_AA_New_McAlloc_Mode(void)
{
    AA_New_McAlloc_Mode = 1;
}

// CT3 0x018b7930: after a load, take the world down and build it again from the save: each drawn
// character and every loaded bank past the scene banks is unregistered (its draw / scene teardown by
// bank type), all banks are marked unloaded and ended, then system_init, the system data, the chosen
// save image (data_load_set), the continue backup; the AI, menu and player control restart, the trigger
// system hears TRIG_EVENT_MC_RELOADED, and the screen fades in while main skips four frames.
void mc_load_set(void)
{
    int i;

    for (i = 0; i < CHARACTER_MAX; i++) {
        if (Character[i].flags & (CHARACTER_FLAG_REFRACTION | CHARACTER_FLAG_SHADOW)) {
            TrgCharacterDraw(0, i);
        }
    }
    for (i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (BankInfo[i].loaded == 1) {
            if (BankInfo[i].type == BANK_TYPE_NO_ROOM_SOUND) {
                FSMltSetDrawBank(i, BankInfo[i].name, 0);
            } else if (BankInfo[i].type == BANK_TYPE_2) {
                FSMltSetDrawBank(i, BankInfo[i].name, 0);
            } else if (BankInfo[i].type == BANK_TYPE_SHADOW) {
                TrgSceneDraw(0, 0, 0, BankInfo[i].name, i);
            }
            BankInfo[i].count = 0;
        }
    }
    for (i = 0; i < BANK_MAX; i++) {
        BankInfo[i].loaded = 0;
    }
    SceneEndBank(0, BANK_MAX);
    system_init();
    system_data_load_set(SYSTEM_LOAD_ALL);
    data_load_set(mc_databuff[mcFile_ID]);
    ContinueBackupSet();
    MonsterAIUpdate(1);
    ssMenuUpdate(1);
    PlayerControlUpdate(1);
    ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
    asMothPause(1);
    tiFade_Start(0.0f, FADE_TYPE_MC_LOAD);
    SkipMain = MC_LOAD_SKIP_FRAMES;
}

// The global banks' names (the symtab's locals @2606..@2609, 5 bytes each at 0x01935250..0x01935268).
static char s_glblInfo[5] = "INFO";
static char s_glblEGl[5] = "E_GL";
static char s_glblOmh0[5] = "0MH0";
static char s_glblGame[5] = "GAME";

// Trigger events a load removes (data_load_set; numbers only, named from this use: none yet read).
#define TRIG_EVENT_LOAD_CLEAR_9E 0x9e   /* removed right after the scene variable is set */
#define TRIG_EVENT_LOAD_CLEAR_15 0x15   /* removed after the title selection is set */
#define TRIG_EVENT_LOAD_CLEAR_1A 0x1a   /* removed last, after TransparentBodyTime */
static const int s_loadClearEvents[] = {9, 7, 8, 0x19, 10, 0x97}; /* removed after the camera returns */

// Copy one CHARACTER field (by its offset) out of a save's character image (data_load_set's restore
// after the banks are loaded again).
#define MC_CHARA_FIELD(dst, img, field) \
    memcpy(&(dst).field, (img) + offsetof(CHARACTER, field), sizeof((dst).field))

// CT3 0x018b7b50: rebuild the game from a save image (mc_load_set: the card's; ContinueBackupGet: the
// continue backup). The counters, scenes, items, letters, arrows, event flags and variables come back
// first, then the trigger system's view of the system data, the round, the sound state and the
// characters (Character_All_Init, then each image through mc_char_set). The global banks are ended and
// loaded again (INFO 5, E_GL 3, 0MH0 1, GAME 0), the cameras reset, the current scene's bank is loaded
// (and drawn at its saved place when it was drawn), then every other saved bank by its type. Once the
// loads end, the fields the shape / behaviour set-up reset come back from the images, the camera returns
// to its saved cut, the panic state comes back, and a few events are cleared.
void data_load_set(u_char *data)
{
    MC_SAVE_DATA *save = (MC_SAVE_DATA *)data;
    MC_SAVE_RECORD *rec;
    char name[0x100];
    char glblName[0x10];
    int i;
    int j;
    int bank;
    int chara;
    int round;

    MC_NOW_LOADING = 1;
    save_count = save->saveCount;
    progress_time = save->progressTime;
    idNextScene = save->idNextScene;
    idCurrentScene = save->idCurrentScene;
    idCameraScene = save->idCameraScene;
    memcpy(ItemList, save->itemList, sizeof(ItemList));
    memcpy(ItemData, save->itemData, sizeof(ItemData));
    memcpy(KeyItemList, save->keyItemList, sizeof(KeyItemList));
    memcpy(LetterList, save->letterList, sizeof(LetterList));
    memcpy(LetterData, save->letterData, sizeof(LetterData));
    memcpy(ArrowList, save->arrowList, sizeof(ArrowList));
    memcpy(ArrowData, save->arrowData, sizeof(ArrowData));
    memcpy(GlobalEventFlag, save->globalEventFlag, sizeof(GlobalEventFlag));
    memcpy(GlobalVariable, save->globalVariable, sizeof(GlobalVariable));

    ssTrigRegisterEvent(TRIG_EVENT_MC_LOADING, 1, 0);
    ssTrigSetVariable(TRIG_VAR_CURRENT_SCENE, idCurrentScene, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_LOAD_CLEAR_9E, 1, 0);
    if ((OMAKE_FLAG & OMAKE_1) == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_OMAKE_1, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_OMAKE_1, 1, 0);
    }
    if ((OMAKE_FLAG & OMAKE_2) == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_OMAKE_2, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_OMAKE_2, 1, 0);
    }
    if (SOUND_STEREO_MONAURAL_BUFF == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_MONAURAL, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_MONAURAL, 1, 0);
    }
    ssTrigSetVariable(TRIG_VAR_BGM_VOLUME, BGM_MASTER_VOLUME_BUFF, 1, 0);
    ssTrigSetVariable(TRIG_VAR_SE_VOLUME, SE_MASTER_VOLUME_BUFF, 1, 0);
    ssTrigSetVariable(TRIG_VAR_TITLE_SELECT, TITLE_SELECT_NUM, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_LOAD_CLEAR_15, 1, 0);
    if (ssTrigCheckEvent(TRIG_EVENT_CLEARED, 1, 0) == 0) {
        Round2ChangeChar(0);
    } else {
        round = ssTrigGetVariable(TRIG_VAR_ROUND, 1, 0);
        if (round < 1) {
            round = 1;
        }
        Round2ChangeChar(round);
    }
    Sound_data_load_set();
    Character_All_Init();
    for (i = 0; i < CHARACTER_MAX; i++) {
        mc_char_set((u_char *)&Character[i], save->chara[i]);
    }

    // The global banks: end what is loaded, then load and draw them again.
    if (BankInfo[MC_GLBL_BANK_GAME].loaded != 0) {
        strcpy(glblName, BankInfo[MC_GLBL_BANK_GAME].name);
        glblName[4] = '\0';
        TrgGLBLSceneDraw(0, glblName, MC_GLBL_BANK_GAME);
    }
    if (BankInfo[MC_GLBL_BANK_INFO].loaded != 0) {
        TrgGLBLSceneDraw(0, s_glblInfo, MC_GLBL_BANK_INFO);
        SceneEndBank(MC_GLBL_BANK_INFO, 1);
    }
    if (BankInfo[MC_GLBL_BANK_E_GL].loaded != 0) {
        TrgGLBLSceneDraw(0, s_glblEGl, MC_GLBL_BANK_E_GL);
        SceneEndBank(MC_GLBL_BANK_E_GL, 1);
    }
    if (BankInfo[MC_GLBL_BANK_0MH0].loaded != 0) {
        TrgGLBLSceneDraw(0, s_glblOmh0, MC_GLBL_BANK_0MH0);
        SceneEndBank(MC_GLBL_BANK_0MH0, 1);
    }
    ssTrigRegisterEvent(TRIG_EVENT_GLBL_LOADED, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_GLBL_UNLOADED, 1, 0);
    TrgGLBLSceneLoad(s_glblInfo, MC_GLBL_BANK_INFO);
    TrgGLBLSceneDraw(1, s_glblInfo, MC_GLBL_BANK_INFO);
    TrgGLBLSceneLoad(s_glblEGl, MC_GLBL_BANK_E_GL);
    TrgGLBLSceneDraw(1, s_glblEGl, MC_GLBL_BANK_E_GL);
    TrgGLBLSceneLoad(s_glblOmh0, MC_GLBL_BANK_0MH0);
    TrgGLBLSceneDraw(1, s_glblOmh0, MC_GLBL_BANK_0MH0);
    TrgGLBLSceneLoad(s_glblGame, MC_GLBL_BANK_GAME);
    TrgGLBLSceneDraw(1, s_glblGame, MC_GLBL_BANK_GAME);

    for (i = 0; i < MC_LOAD_CAMERAS; i++) {
        camera_init0(i, 0);
        CamChange_S(i, 0, 0, 0);
    }
    camera_init0(MC_LOAD_CAMERAS, 1);
    BranchSELoad();

    // The current scene's bank first.
    for (i = 0; i < MC_SAVE_RECORD_MAX; i++) {
        rec = &save->banks[i];
        bank = rec->id;
        if (bank == idCurrentScene) {
            SceneDoorFlagClear(GetRoomName_file(rec->name));
            strcpy(name, rec->name);
            name[4] = '\0';
            TrgSceneLoad(name, bank, -1);
            if (rec->loaded == 1) {
                memcpy(DrawOnPosSetVectorTemp, rec->pos, sizeof(DrawOnPosSetVectorTemp));
                TrgSceneDraw(1, 0, -2, name, bank);
            }
            break;
        }
    }
    // Then every other saved bank, by its type.
    for (i = 0; i < MC_SAVE_RECORD_MAX; i++) {
        rec = &save->banks[i];
        bank = rec->id;
        if (bank == MC_SAVE_RECORD_EMPTY) {
            break;
        }
        if (idCurrentScene == bank) {
            continue;
        }
        chara = rec->chara;
        strcpy(name, rec->name);
        name[4] = '\0';
        if (rec->type == MC_BANK_BEHAVIOR) {
            TrgCharacterBehaviorLoad(chara, name, bank, -1);
            if (rec->loaded == 1) {
                for (j = 0; j < 10; j++) {
                    if (bank == Character[chara].slot004[j]) {
                        TrgCharacterBehaviorSet(chara, name, -1);
                        break;
                    }
                }
            }
        } else if (rec->type == MC_BANK_SHAPE) {
            TrgCharacterShapeLoad(chara, name, bank, -1);
            if (rec->loaded == 1) {
                Character[chara].bank = -1;
                Character[chara].w090 = 0;
                Character[chara].w098 = 0;
                Character[chara].lightBank = -1;
                TrgCharacterShapeSet(chara, name, -1, 0, 0);
                InitMatrixTree(pModelPartsHeader[bank]);
                LinkMatrixTree(All_table_TBL[bank]->objOpaque, pModelPartsHeader[bank]);
            }
        } else if (rec->type == MC_BANK_SCENE || rec->type == MC_BANK_SCENE0) {
            TrgSceneLoad(name, bank, -1);
            if (rec->loaded == 1) {
                memcpy(DrawOnPosSetVectorTemp, rec->pos, sizeof(DrawOnPosSetVectorTemp));
                TrgSceneDraw(1, 0, -2, name, bank);
            }
        }
    }
    FSMltLoadEndWaitAll();

    // The fields the shape set-up reset come back from the images.
    for (i = 0; i < CHARACTER_MAX; i++) {
        MC_CHARA_FIELD(Character[i], save->chara[i], w090);
        MC_CHARA_FIELD(Character[i], save->chara[i], w098);
        MC_CHARA_FIELD(Character[i], save->chara[i], lightBank);
        MC_CHARA_FIELD(Character[i], save->chara[i], h100);
    }
    pad[0].cnt = 0;
    pad[0].pad02 = 0;
    pad[0].trg = 0;
    pad[0].key = 0;
    camera_main(&pad[0], 4);
    CamChange_S(idCameraScene, (short)save->cameraCut, 0, 0);
    memcpy(&PanicInfoData, &save->panic, sizeof(PanicInfoData));
    for (i = 0; i < (int)(sizeof(s_loadClearEvents) / sizeof(s_loadClearEvents[0])); i++) {
        ssTrigRemoveEvent(s_loadClearEvents[i], 1, 0);
    }
    TransparentBodyTime = MC_LOAD_TRANSPARENT_TIME;
    ssTrigRemoveEvent(TRIG_EVENT_LOAD_CLEAR_1A, 1, 0);
    PanicInfoData.b01 = 0xff;
    PanicInfoData.h02 = 0;
    ssAI_MemLoadInit(data);
    MC_NOW_LOADING = 0;
}

// The byte ranges of a CHARACTER a save keeps (mc_char_set). The ROM copies them field by field (bytes,
// shorts, words, quadwords: 0x00, 0x02, 0x04[10], 0x18[10], 0x22[10][10], 0x88, 0x90, 0x98, 0xa0, 0xb0..
// 0xec, 0xf8, 0xfc, 0x100, 0x102, 0x310, 0x320, 0x340..0x359, 0x38c[6], 0x398[6], 0x3a4[6], 0x3aa[6],
// 0x3b0[42], 0x404[42], 0x458[6], 0x4d0, 0x4e0, 0x524, 0x528); adjacent fields are merged here.
static const struct {
    u_short offset;
    u_short size;
} s_mcCharFields[] = {
    {0x000, 0x01}, {0x002, 0x84}, {0x088, 0x04}, {0x090, 0x12}, {0x0b0, 0x40}, {0x0f8, 0x0b},
    {0x310, 0x20}, {0x340, 0x1a}, {0x38c, 0xe4}, {0x4d0, 0x04}, {0x4e0, 0x10}, {0x524, 0x08},
};

// CT3 0x018ba4c0: copy the saved fields of one character (dst and src: CHARACTER records or their
// images in a save; data_load_set / data_save_set).
void mc_char_set(u_char *dst, u_char *src)
{
    u_int i;

    for (i = 0; i < sizeof(s_mcCharFields) / sizeof(s_mcCharFields[0]); i++) {
        memcpy(dst + s_mcCharFields[i].offset, src + s_mcCharFields[i].offset, s_mcCharFields[i].size);
    }
}


// One bank's save record (data_save_set): its name, type and state; characters' banks take their
// character (no character: not set again on a load), scene banks part 0's place.
static void mc_save_bank_record(MC_SAVE_RECORD *rec, int bank)
{
    rec->id = bank;
    rec->chara = -1;
    strcpy(rec->name, BankInfo[bank].name);
    rec->type = BankInfo[bank].type;
    rec->loaded = BankInfo[bank].loaded;
}

// CT3 0x018b9ee0: keep the game's state as a save image (mc_save_set: the card's; ContinueBackupSet: the
// continue backup): the counters and scenes with the live camera's cut, the items, letters, arrows,
// event flags and variables; the banks loaded (bank 2 first, then banks 6..85 group by group, ended by
// id -1); every character through mc_char_set; the panic state; the AI's part; then the image's door
// flags are cleared.
void data_save_set(u_char *data)
{
    MC_SAVE_DATA *save = (MC_SAVE_DATA *)data;
    MC_SAVE_RECORD *rec;
    u_int count;
    int i;

    // callcheck: BankID_TO_CharID, FSMltLoadEndWaitAll, GetAreaNumber, asCheckBankID, mc_char_set, ssAI_MemSaveData, strcpy, tkXcmd_DoorFlagClear (the ROM inlines the record twice: mc_save_bank_record makes the strcpy)
    FSMltLoadEndWaitAll();
    save->saveCount = save_count;
    save->progressTime = progress_time;
    save->area = GetAreaNumber(0);
    save->idNextScene = idNextScene;
    save->idCurrentScene = idCurrentScene;
    save->idCameraScene = idCameraScene;
    save->cameraCut = Cam[asCheckBankID(idCameraScene)].cut;
    memcpy(save->itemList, ItemList, sizeof(save->itemList));
    memcpy(save->itemData, ItemData, sizeof(save->itemData));
    memcpy(save->keyItemList, KeyItemList, sizeof(save->keyItemList));
    memcpy(save->letterList, LetterList, sizeof(save->letterList));
    memcpy(save->letterData, LetterData, sizeof(save->letterData));
    memcpy(save->arrowList, ArrowList, sizeof(save->arrowList));
    memcpy(save->arrowData, ArrowData, sizeof(save->arrowData));
    memcpy(save->globalEventFlag, GlobalEventFlag, sizeof(save->globalEventFlag));
    memcpy(save->globalVariable, GlobalVariable, sizeof(save->globalVariable));

    count = 0;
    if (BankInfo[MC_SAVE_GLOBAL_BANK].count > 0) {
        rec = &save->banks[0];
        mc_save_bank_record(rec, MC_SAVE_GLOBAL_BANK);
        memcpy(rec->pos, ((MODEL_PARTS *)pModelPartsHeader[MC_SAVE_GLOBAL_BANK])->drawOnPos, sizeof(rec->pos));
        count = 1;
    }
    // The ROM does not bound count by MC_SAVE_RECORD_MAX (no more than 35 groups are ever loaded).
    for (i = MC_SAVE_BANK_FIRST; i < BANK_MAX; i++) {
        if (BankInfo[i].count > 0) {
            rec = save->banks + count;
            mc_save_bank_record(rec, i);
            if (rec->type == MC_BANK_BEHAVIOR || rec->type == MC_BANK_SHAPE) {
                rec->chara = BankID_TO_CharID((short)i);
                if (rec->chara < 0) {
                    rec->loaded = 0;
                }
            } else if (rec->type == MC_BANK_SCENE) {
                memcpy(rec->pos, ((MODEL_PARTS *)pModelPartsHeader[i])->drawOnPos, sizeof(rec->pos));
            }
            count++;
            if (BankInfo[i].groupCount != 0) {
                i += BankInfo[i].groupCount - 1;
            }
        }
    }
    save->banks[count].id = MC_SAVE_RECORD_EMPTY;

    for (i = 0; i < CHARACTER_MAX; i++) {
        mc_char_set(save->chara[i], (u_char *)&Character[i]);
    }
    memcpy(&save->panic, &PanicInfoData, sizeof(save->panic));
    ssAI_MemSaveData(data);
    tkXcmd_DoorFlagClear(save->globalEventFlag);
}
