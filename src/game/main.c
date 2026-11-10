// game/main.c -- CT3 main's object (0x01834440..0x0183aed8): the boot (main, FirstInitialization,
// game_init, system_init), the main loop, and the draw passes it picks by DrawMode. Built with
// main=gameMain (CMakeLists.txt): the process entry is the host's (src/main.cpp).
// Written 2026-11-09: slice 1 (the boot, the loop, the small draw passes), slice 2 (GlobalDrawObject,
// scene_draw_init, system_end, get_draw_obj), slice 3 (main_rtn, main_rtn_Particle), slice 4
// (DrawObjectRefraction, main_rtn_DOF), slice 5 (AllDrawObject, ssHQDOF_DrawAllObject, main_rtn_GHOST /
// _GHOST_DOF), slice 6 (scene_init), slice 7 (main_rtn_Particle2): the TU is complete.
#include "game/main.h"
#include "common/alyssapan_stub.h"

#include <string.h>

#include "sdk/eekernel.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"
#include "game/unassigned_data.h"
#include "ga/gaBGT_DrawObject.h"
#include "ga/gaDma.h"
#include "ga/gaObjectClippingCheck.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "ga/gaInitInterrupt.h"
#include "ga/gaVifInit.h"
#include "ga/pad_button_read.h"
#include "game/ArtoolDebugOpen.h"
#include "game/Character_Init.h"
#include "game/Clipping_Portal.h"
#include "game/LinkMatrixTree.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ahalloc.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/asMothPause.h"
#include "game/asMothRouteSearch.h"
#include "game/asUseArrow.h"
#include "game/asXcmd.h"
#include "game/fileio.h"
#include "game/gaInitMeter.h"
#include "game/main2.h"
#include "game/movie.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "game/tiAvoidObject.h"
#include "game/tkXDTrg_GetSceneHash.h"
#include "game/wideXcmd_DebugPrint.h"

u_int pad_key_button[4];                        /* sbss 0x01942ab0 */
u_int pad_trg_button[4];                        /* sbss 0x01942ac0 */
u_int pad_cnt_button[4];                        /* sbss 0x01942ad0 */
u_char Demo_Skip_Set;                           /* sbss 0x01942ae0 */
int DEMO_SKIP_FLAG;                             /* sbss 0x01942ae4 */
FILTER_CHANGE_WORK DemoFilterChangeWork;        /* sbss 0x01942af0 */
int DemoSkip;                                   /* sbss 0x01942b18 */
int DEMO_SKIP_SET;                              /* sbss 0x01942b1c */
int SkipMain;                                   /* sbss 0x01942b20 */
int MyFrame;                                    /* sbss 0x01942b24 */
int controller_type;                            /* sbss 0x01942b28 */
int controller_num2;                            /* sbss 0x01942b2c */
int controller_num;                             /* sbss 0x01942b30 */
int controller_mode;                            /* sbss 0x01942b34 */
int main_rtn_id;                                /* sbss 0x01942b38 */
int CntDrawnOut;                                /* sbss 0x01942b3c */
char **gArgv;                                   /* sbss 0x01942b40 */
int gArgc;                                      /* sbss 0x01942b44 */

// The heaps' names (@1555 / @1556 / @1557, 5 bytes each).
#define HEAP_NAME_JUNC "JUNC"
#define HEAP_NAME_SOUND "SUND"
#define HEAP_NAME_GLOBAL "GLBL"

#define BGT_RINGBUFFER_SIZE 0x8000
#define SPR_3DLIB_WORK SCRATCHPAD(0x400)      /* ga3dlibwork's copy in the scratchpad (EE 0x70000400) */
#define SPR_3DLIB_WORK_QWC 0x40

// main_rtn's flags: the low 15 bits name the pass (0 and 4 sort the list first), 0x8000 skips the
// clip check.
#define MAIN_RTN_PASS_MASK 0x7fff
#define MAIN_RTN_PASS_SORT 0
#define MAIN_RTN_PASS_SORT2 4
#define MAIN_RTN_NO_CLIP 0x8000
#define ALL_DRAW_DOF_FIRST 2                    /* AllDrawObject / main_rtn_DOF: the farthest DOF slice clip-checks */
#define ALL_DRAW_DOF_NEXT 3                     /* the nearer slices reuse the stored clip byte */
#define MAIN_RTN_CLIP_FAR 50000.0f              /* gaSet3dlibStruct's far clip */
#define DRAW_CTRL_NONE ((GA_DRAWCTRL_ARGS *)-1) /* main_rtn's "no draw control yet" (the ROM's -1) */
// FirstInitialization's frame-clear draw control (the ROM's 29 argument slots, 0x01835708).
static const GA_DRAWCTRL_ARGS FRAMECLEAR_DRAWCTRL_ARGS = {
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0}, 0};
#define SCENE_INIT_LISTS 3                      /* scene_init: objTrans, objOpaque, objAfter */
#define PARTS_HEADER_ANIMATED 1                 /* scene_init: the parts header's first byte: restart
                                                   the scene's behaviour animations (name from that use) */
#define BANK_NAME_MENU "MENU.BIN"               /* @2324: bank 0 is the menu -> every texture restarts the BGT ring */

#define FRAMECLEAR_RGBAQ 0x3f80000080000000ull  /* Q = 1.0f, A = 0x80, RGB = 0 */

// Trigger events main.c tests (ssGrpPortalCheck.c's table). Names from how main uses them; the script
// side that sets them is unread (open-items.md).
enum {
    EV_HOLD_MONSTER = 0x36,                     /* set: a time stop does not run MonsterAIUpdate */
    EV_HOLD_PLAYER = 0x37,                      /* set: a time stop does not run PlayerControlUpdate */
    EV_HOLD_MENU = 0x38,                        /* set: a time stop does not run ssMenuUpdate */
    EV_PLAY_TIME_STOP = 0x25,                   /* set: the play-time clock stands still */
    EV_DEMO = 0x9e,                             /* a demo (cutscene) runs: the skip button can end it */
    EV_DEMO_SKIPPED = 0xaf,                     /* registered while a skipped demo fast-forwards */
    EV_0xCA = 0xca,                             /* removed in a time stop unless 0xcc is set */
    EV_0xCC = 0xcc,
    EV_SHADOW_AFTER_MIRROR = 0x3b,              /* AllDrawObject: set, the shadow volumes draw after the
                                                   mirrored characters instead of before the characters */
};

enum DEMO_SKIP {                                /* DEMO_SKIP_FLAG: the demo skip's steps */
    DEMO_SKIP_IDLE = 0,                         /* waiting for the skip button */
    DEMO_SKIP_REQUEST = 1,                      /* asked for by script */
    DEMO_SKIP_FADE_START = 2,                   /* start the fade to black */
    DEMO_SKIP_FADING = 3,
    DEMO_SKIP_HOLD = 4,                         /* a few black frames */
    DEMO_SKIP_RUN = 5,                          /* fast-forward: DemoSkip = 1 */
    DEMO_SKIP_END = 10,                         /* the demo ended: fade back in */
    DEMO_SKIP_END_FADING = 11,
    DEMO_SKIP_END_HOLD = 12,
    DEMO_SKIP_END_DONE = 13,
};

#define DEMO_SKIP_FADE_FRAMES 10
#define DEMO_SKIP_HOLD_FRAMES 4

// scene_draw_init's bank names (@2360 "ARRW.BIN", @2361 "E_", @2362 "M") and the actress's light parts.
#define BANK_NAME_ARROW "ARRW.BIN"
#define BANK_PREFIX_E "E_"
#define BANK_PREFIX_M "M"
#define PARTS_ACTRESS_OMNI_1 "ACTRESS_OMNI_1"
#define PARTS_ACTRESS_OMNI_2 "ACTRESS_OMNI_2"
#define PARTS_ACTRESS_OMNI_BASE "ACTRESS_OMNI_BASE"

#define EV_TITLE_CANCEL 0x10                    /* system_end: TITLE_CANCEL_FLAG = the event's state */

#define KEY_MAP_SKIP 11                         /* KEY_MAP[11] (+0x16): skip a demo */
#define KEY_MAP_DRAWN_OUT 23                    /* KEY_MAP[23] (+0x2e): the trigger forced while CntDrawnOut */
#define PAD_BUTTON_0x100 0x100

// ──────────────────────────────────────────────────────────────────────
// main_rtn_DOF for a ghost: main_rtn_DOF's slice test and stored clip byte, with main_rtn_GHOST's
// differences (the current scene's character fog, one fixed draw control, GHOST_DRAWCTRL_ARGS).
// CT3 0x01834440

// gaSetupDrawCtrlData's settings for every ghost part (the ROM's 29 argument slots, 0x01834fd8).
static const GA_DRAWCTRL_ARGS GHOST_DRAWCTRL_ARGS = {
    {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 3, 0, 0, 1, 2, 2, 2, 2, 0, 0, 0, 1, 2, 1, 1, 0, 0}, 0};

void main_rtn_GHOST_DOF(int bank, int mode, int chara, int pass, int charaIndex, void **lastTex)
{
    alignas(16) GA_DRAWCTRL_DATA ctrl;
    short lightBank = -1;
    float lightFrames = 0.0f;
    int count;
    int blur;
    u_char *table;
    FOG_INFO *fog;
    TEX_INFO *texNow;
    u_int *timNow = NULL;
    int zonlyParts = -1;
    int i;

    if (chara == 1) {
        lightBank = Character[charaIndex].lightBank;
        lightFrames = (float)DirectLightCharaCounter[charaIndex];
    }
    if (asCheckBankType(bank) == 0)
        asChangeViewMatrix_forTheBank(asCheckBankID(bank));
    count = get_draw_obj(bank);
    if (count == 1)
        return;

    blur = ssGetNumBlurDrawObjForScene(bank, mode);
    texNow = (TEX_INFO *)*lastTex;
    table = osTotalObjTable[mode][bank];
    if (mode != 0 && count != 0) {
        if (blur == 0)
            objSort(bank, count, table);
        else
            objSortWithBlurIdx(bank, count, table);
        ALLobject[bank] = (void **)table;
    }
    if (blur != 0) {
        ssPartMotionBlurInsertObjToDrawTable(bank, table, ALLobject[bank], count, mode);
        ALLobject[bank] = (void **)table;
    }

    fog = (FOG_INFO *)FogCtrlAddr[idCurrentScene];
    gaSetupFogCtrlData(&fogctrl[bank], fog->charaFog[0], fog->charaFog[1], fog->charaFog[2],
                       fog->charaFogArg[0], fog->charaFogArg[1], fog->charaFogArg[2]);
    gaSet3dlibStruct(MAIN_RTN_CLIP_FAR, ga3dlibwork, &drawbuffer_0, &fogctrl[bank], screen_0);
    if (Clipping_System == 1)
        Clipping_Portal(ga3dlibwork, screen_0);
    tnInit3dlibWork(ga3dlibwork, SPR_3DLIB_WORK);
    gaBGT_ExecFogCtrlData_VIF1(&fogctrl[bank]);
    if (((MODEL_PARTS *)pModelPartsHeader[idCurrentScene])[fog->parts].flags & PARTS_FLAG_FOG)
        FOG_mode = 1;
    gaSetupDrawCtrlData(&ctrl, GHOST_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&ctrl);

    for (i = 0; ALLobject[bank][i] != NULL; i++) {
        DRAW_OBJ_ENTRY *ent = (DRAW_OBJ_ENTRY *)ALLobject[bank][i];
        u_char *obj = ent->obj;
        TEX_INFO *tex = ent->tex;
        MODEL_PARTS *parts = &((MODEL_PARTS *)pModelPartsHeader[bank])[ent->parts];

        if (parts->flags & PARTS_FLAG_FOG)
            continue;
        if (!(All_table_TBL[bank]->partsPos[ent->parts][3] > PARTS_POS_HIDDEN_W))
            continue;
        if (parts->kind == PARTS_KIND_NO_DOF)
            continue;
        if ((parts->flags & PARTS_FLAG_ZONLY_FIRST) && zonlyParts != ent->parts)
            zonlyParts = OneMaxObjectDrawZOnly(bank, i);
        if (pass == ALL_DRAW_DOF_FIRST)
            obj[DRAW_OBJ_CLIP] = (u_char)gaObjectClippingCheck(obj);
        if ((s_char)obj[DRAW_OBJ_CLIP] < 0)
            continue;

        if (texNow != tex && tex != NULL && tex->tim != timNow && tex->tim != NULL) {
            sunSetupTBufferData(ent->tbuffer, *tex);
            sunBGT_LoadGSTexClutData_VIF1(ent->tbuffer);
        }
        if (ssDOF_ClipObjectFromZRange(obj, (u_int)ent->dofMode) == 0)
            continue;
        *(int *)(obj + DRAW_OBJ_ATTR) = parts->attr;
        if (tex != NULL) {
            float *light = (float *)(obj + DRAW_OBJ_LIGHT);

            if (chara == 0) {
                if (tex->kind == TEX_KIND_LIGHT)
                    *light = All_table_TBL[bank]->partsLight[ent->parts];
            } else if (tex->kind == TEX_KIND_LIGHT && lightBank != -1 && BankInfo[lightBank].loaded == 1) {
                float target = All_table_TBL[lightBank]->light[2] * All_table_TBL[bank]->partsLight[ent->parts];

                if (lightFrames == 0.0f)
                    *light = target;
                else
                    *light = *light + (target - *light) / lightFrames;
            }
        }

        drawobj_attr = 0;
        if (texNow != tex && tex != NULL) {
            if (tex->tim != timNow && tex->tim != NULL) {
                if (tex->tim[0] <= TEX_SMALL_MAX && tex->tim[1] <= TEX_SMALL_MAX) {
                    gaBGT_End();
                    gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                }
                timNow = tex->tim;
            }
            sunBGT_ExecTBufferData_VIF1(ent->tbuffer);
            texNow = tex;
        }
        if (obj[DRAW_OBJ_CLIP] == 0 && !(obj[DRAW_OBJ_FLAGS] & DRAW_OBJ_FLAG_CLIP))
            gaBGT_DrawObject_NoClippingCheck2(obj, drawobj_attr);
        else
            gaBGT_DrawObject(obj, drawobj_attr);
    }
    *lastTex = texNow;
}

// ──────────────────────────────────────────────────────────────────────
// main_rtn for a ghost (CHARA_TRANS_GHOST, AllDrawObject's last pass): the same list, light and texture
// handling, but every list with entries sorted (mode 1), the current scene's character fog whatever
// chara says, one fixed draw control for every part (GHOST_DRAWCTRL_*) instead of the parts' own, no
// camera position update, and only the small-texture rule restarting the BGT ring.
// CT3 0x01834c40

void main_rtn_GHOST(int bank, int mode, int chara, u_int flags, int charaIndex, void **lastTex)
{
    alignas(16) GA_DRAWCTRL_DATA ctrl;
    short lightBank = -1;
    float lightFrames = 0.0f;
    int count;
    int blur;
    u_char *table;
    MODEL_PARTS *parts;
    FOG_INFO *fog;
    TEX_INFO *texNow;
    u_int *timNow = NULL;
    int zonlyParts = -1;
    int i;

    if (chara == 1) {
        lightBank = Character[charaIndex].lightBank;
        lightFrames = (float)DirectLightCharaCounter[charaIndex];
    }
    if (asCheckBankType(bank) == 0)
        asChangeViewMatrix_forTheBank(asCheckBankID(bank));
    count = get_draw_obj(bank);
    if (count == 1)
        return;

    blur = ssGetNumBlurDrawObjForScene(bank, mode);
    texNow = (TEX_INFO *)*lastTex;
    table = osTotalObjTable[mode][bank];
    if (mode != 0 && count != 0) {
        if (blur == 0)
            objSort(bank, count, table);
        else
            objSortWithBlurIdx(bank, count, table);
        ALLobject[bank] = (void **)table;
    }
    if (blur != 0) {
        ssPartMotionBlurInsertObjToDrawTable(bank, table, ALLobject[bank], count, mode);
        ALLobject[bank] = (void **)table;
    }

    fog = (FOG_INFO *)FogCtrlAddr[idCurrentScene];
    gaSetupFogCtrlData(&fogctrl[bank], fog->charaFog[0], fog->charaFog[1], fog->charaFog[2],
                       fog->charaFogArg[0], fog->charaFogArg[1], fog->charaFogArg[2]);
    gaSet3dlibStruct(MAIN_RTN_CLIP_FAR, ga3dlibwork, &drawbuffer_0, &fogctrl[bank], screen_0);
    if (Clipping_System == 1)
        Clipping_Portal(ga3dlibwork, screen_0);
    tnInit3dlibWork(ga3dlibwork, SPR_3DLIB_WORK);
    gaBGT_ExecFogCtrlData_VIF1(&fogctrl[bank]);
    if (((MODEL_PARTS *)pModelPartsHeader[idCurrentScene])[fog->parts].flags & PARTS_FLAG_FOG)
        FOG_mode = 1;
    gaSetupDrawCtrlData(&ctrl, GHOST_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&ctrl);

    for (i = 0; ALLobject[bank][i] != NULL; i++) {
        DRAW_OBJ_ENTRY *ent = (DRAW_OBJ_ENTRY *)ALLobject[bank][i];
        u_char *obj = ent->obj;
        TEX_INFO *tex = ent->tex;
        int clip;

        parts = &((MODEL_PARTS *)pModelPartsHeader[bank])[ent->parts];
        if (parts->flags & PARTS_FLAG_FOG)
            continue;
        if (!(All_table_TBL[bank]->partsPos[ent->parts][3] > PARTS_POS_HIDDEN_W))
            continue;
        if ((parts->flags & PARTS_FLAG_ZONLY_FIRST) && zonlyParts != ent->parts)
            zonlyParts = OneMaxObjectDrawZOnly(bank, i);
        clip = (flags & MAIN_RTN_NO_CLIP) == 0 ? gaObjectClippingCheck(obj) : 0;
        if (clip < 0)
            continue;

        if (texNow != tex && tex != NULL && tex->tim != timNow && tex->tim != NULL) {
            sunSetupTBufferData(ent->tbuffer, *tex);
            sunBGT_LoadGSTexClutData_VIF1(ent->tbuffer);
        }
        *(int *)(obj + DRAW_OBJ_ATTR) = parts->attr;
        if (tex != NULL) {
            float *light = (float *)(obj + DRAW_OBJ_LIGHT);

            if (chara == 0) {
                if (tex->kind == TEX_KIND_LIGHT)
                    *light = All_table_TBL[bank]->partsLight[ent->parts];
            } else if (tex->kind == TEX_KIND_LIGHT && lightBank != -1 && BankInfo[lightBank].loaded == 1) {
                float target = All_table_TBL[lightBank]->light[2] * All_table_TBL[bank]->partsLight[ent->parts];

                if (lightFrames == 0.0f)
                    *light = target;
                else
                    *light = *light + (target - *light) / lightFrames;
            }
        }

        drawobj_attr = 0;
        if (texNow != tex && tex != NULL) {
            if (tex->tim != timNow && tex->tim != NULL) {
                if (tex->tim[0] <= TEX_SMALL_MAX && tex->tim[1] <= TEX_SMALL_MAX) {
                    gaBGT_End();
                    gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                }
                timNow = tex->tim;
            }
            sunBGT_ExecTBufferData_VIF1(ent->tbuffer);
            texNow = tex;
        }
        if (clip == 0 && !(obj[DRAW_OBJ_FLAGS] & DRAW_OBJ_FLAG_CLIP)) {
            gaBGT_DrawObject_NoClippingCheck2(obj, drawobj_attr);
        } else {
            obj[DRAW_OBJ_CLIP] = (u_char)clip;
            gaBGT_DrawObject(obj, drawobj_attr);
        }
    }
    *lastTex = texNow;
}

// ──────────────────────────────────────────────────────────────────────
// Clears the item, key item, letter and arrow lists, and every item's held flag.
// CT3 0x01835420

void tiItemInit(void)
{
    int i, j;

    for (i = 0; i < 12; i++)
        ItemList[i] = 0;
    for (i = 0; i < 100; i++)
        ItemData[i][1] = 0;
    for (i = 0; i < 24; i++)
        KeyItemList[i] = 0;
    for (i = 0; i < 5; i++)
        for (j = 0; j < 10; j++)
            LetterList[i][j] = 0;
    ArrowData[0] = 0;
    ArrowData[1] = 0;
    ArrowData[2] = 0;
    for (i = 0; i < 50; i++)
        ArrowList[i] = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Once at boot: the three SunAlloc heaps, the frame's double-buffer / draw-control packets, the demo
// skip state (SkipMain = 4: the first frames run without drawing), the sound system.
// CT3 0x01835660

void FirstInitialization(void)
{
    SunAllocInit(&gHeapJunc, gJuncAllocArea, sizeof gJuncAllocArea, HEAP_NAME_JUNC);
    SunAllocInit(&gHeapSound, gSoundAllocArea, sizeof gSoundAllocArea, HEAP_NAME_SOUND);
    SunAllocInit(&gHeapGlobalScene, gGlobalBankArea, 0x300000, HEAP_NAME_GLOBAL);
    gaSetupDBufferData(&drawbuffer_frameclear, 0, 0x200, 0, 0, 0, 0, 0);
    gaSetupDrawCtrlData(&drawctrl_frameclear, FRAMECLEAR_DRAWCTRL_ARGS);
    gaSetupDBufferData(&drawbuffer_0, 0, 0x200, 0, 0, 0, 0, 0);
    DemoFilterChangeWork.active = 0;
    SkipMain = 4;
    DemoSkip = 0;
    DEMO_SKIP_FLAG = DEMO_SKIP_IDLE;
    Demo_Skip_Set = 0;
    Sound_FirstInitialization();
}

// ──────────────────────────────────────────────────────────────────────
// Once per power-on: the random seed, the memory card ids, file I/O, the per-scene light tables (-1 =
// none), the characters, the door triggers, the options' defaults (BGM 20, SE 18, stereo).
// CT3 0x01835800

void game_init(void)
{
    int i;

    tnRandSeed = 0x19700208;
    main_rtn_id = 0;
    PowerOn = 0;
    fReset = 0;
    SelfDiscardSceneLoadInit();
    WarpObjectUpdateInit();
    mcMain_ID = -1;
    mcSlot_ID = -1;
    mcFile_ID = -1;
    mcErr_ID = -1;
    mcAccess_ID = -1;
    FS_work_init();
    MyFrame = 0;
    for (i = 0; i < BANK_MAX; i++) {
        ACTRESS_LIGHT_BASE_MAX_ID[i] = -1;
        Omni1_num[i] = -1;
        Omni0_num[i] = -1;
    }
    Character_All_Init();
    tkXcmd_DoorTrigger_Init();
    load_continue = 0;
    asInitArrowInfo();
    asInitMothInfo();
    asXcmd_AddForLockToTargChara(0, 0, 0);
    DEMO_SKIP_SET = 0;
    OMAKE_FLAG = 0;
    for (i = 0; i < 20; i++)
        SkipEventFlag[i] = 0;
    SOUND_STEREO_MONAURAL_BUFF = 0;
    BGM_MASTER_VOLUME_BUFF = 20;
    SE_MASTER_VOLUME_BUFF = 18;
    LOAD_SLOT_NUM = 0;
    TITLE_SELECT_NUM = 0;
    TITLE_CANCEL_FLAG = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The pad's per-frame hand-off to the characters and the camera (skipped while main fast-forwards).
// CT3 0x018359f0

void ControllerMain(void)
{
    if (SkipMain != 0 || DemoSkip != 0)
        return;
    asSetCharacterKeyEnable(1);
    CharStopFlag = 0;
    if (PlayerPanicMode != 0) {
        asSetCharacterKeyEnable(0);
        controller_mode = CONTROLLER_MODE_PANIC;
    }
    ControllerClatteringUpdate();
    if (TrgCharStopFlag & TRG_CHAR_STOP_KEYS)
        asSetCharacterKeyEnable(0);
    if (TrgCharStopFlag & TRG_CHAR_STOP_CHAR)
        CharStopFlag = 1;
    if (controller_mode == CONTROLLER_MODE_PANIC)
        camera_main(pad, 4);
    else if (controller_mode == CONTROLLER_MODE_NORMAL)
        camera_main(pad, 4);
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_VASELINE: the scene, then a soft "vaseline" diamond over it.
// CT3 0x01835ad0

void ssVaselineDrawAllObject(void)
{
    SS_fb_write_msb = 1;
    DrawAllObject_Normal();
    ssBGT_VaselineSetDrawEnvPostRend();
    ssBGT_VaselineDrawDiamond();
    SS_fb_write_msb = 0;
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_WAVE: the scene and the 2D particles, then the wave diamond.
// CT3 0x01835b10

void ssWaveDrawAllObject(void)
{
    SS_fb_write_msb = 1;
    DrawAllObject_Normal();
    ss2DParticle_main();
    ssBGT_WaveSetDrawEnvPostRend();
    ssBGT_WaveDrawDiamond();
    SS_fb_write_msb = 0;
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_HAZY: the scene, then the haze sprite at ScreenHazyLevel.
// CT3 0x01835b50

void ssHazyDrawAllObject(void)
{
    SS_fb_write_msb = 1;
    DrawAllObject_Normal();
    ssBGT_HazySetDrawEnvPostRend();
    ssBGT_HazyDrawSprite(ScreenHazyLevel, HazeTransTime);
    SS_fb_write_msb = 0;
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_ULTIMA: five frames of the plain scene, then only the "ultima" sprite (its frame counter is
// the function's static a, passed by address).
// CT3 0x01835b90

void ssUltimaDrawAllObject(void)
{
    static int a;                               /* sbss 0x0193bc80, a$1034 */

    SS_fb_write_msb = 1;
    if (a < 5) {
        a++;
        DrawAllObject_Normal();
    } else {
        a++;
        ssBGT_UltimaSetDrawEnvPostRend();
        ssBGT_UltimaDrawSprite(&a);
    }
    SS_fb_write_msb = 0;
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_BLUR: the whole frame (scene, shading, filter, global objects), then the blur sprite.
// CT3 0x01835bf0

void ssBlurDrawAllObject(void)
{
    SS_fb_write_msb = 1;
    DrawAllObject_Normal();
    tiDrawShadingOff();
    XcmdScreenFilter();
    GlobalDrawObject();
    ssBGT_BlurSetDrawEnvPostRend();
    ssBGT_BlurDrawSprite(ScreenBlurLevel);
    SS_fb_write_msb = 0;
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_DOF: the scene in depth slices from 4 down to the focus level, each slice past the focus
// blurred onto the frame; then refraction, lens flares and the panic filter as in a normal frame.
// CT3 0x01835c40

void ssDOF_DrawAllObject(int level)
{
    int pass = 4;

    SS_fb_write_msb = 1;
    while (pass >= level) {
        ssBGT_DOFSetDrawEnvPreRend(pass, level);
        if (pass == 4)
            AllDrawObject(2);
        else
            AllDrawObject(3);
        if (level < pass) {
            ssBGT_DOFSetDrawEnvPostRend(pass, level);
            ssBGT_DOFDrawSprite(pass, level);
        }
        pass--;
        if (pass == 0)
            break;
    }
    SS_fb_write_msb = 0;
    DrawObjectRefraction(0);
    tiDrawLensFlares();
    PanicFilter();
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_NORMAL: every object, refraction, lens flares, the panic filter.
// CT3 0x01835d00

void DrawAllObject_Normal(void)
{
    AllDrawObject(0);
    DrawObjectRefraction(0);
    tiDrawLensFlares();
    PanicFilter();
}

#define HQDOF_SAMPLES 8
#define HQDOF_FBUF 0xa8000                      /* ga_draw_fbuf_address while a sample draws */
#define SCREEN_PROJ 8                           /* screen_0[8]: the projection distance (512) */

// The eye's offset for sample i: v = r * right + u * up, xyz only; w from the vector the ROM loads first.
static void HqdofEyeOffset(float *out, const float *right, const float *up, float r, float u)
{
    const float *wsrc = (u != 0.0f) ? up : right;
    int k;

    for (k = 0; k < 3; k++)
        out[k] = right[k] * r + up[k] * u;
    out[3] = wsrc[3];
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_HQDOF: the normal frame, then (unless the effect buffer is taken) HQDOF_SAMPLES more renders
// of the scene into the effect frame buffer (HQDOF_FBUF), each from an eye moved in the camera's
// right / up plane by HQDOFCameraOffset tenths (+-R, +-U, then the four half diagonals) and still looking
// at focusPos, each blended back over the frame at HQDOFOpacity percent: what is far from the focus
// blurs. focusPos then takes the shared camera's focus for the next frame.
// CT3 0x01835d40

void ssHQDOF_DrawAllObject(void)
{
    static const signed char sampleR[HQDOF_SAMPLES] = { 2, 0, -2, 0, 1, -1, 1, -1 }; /* in halves */
    static const signed char sampleU[HQDOF_SAMPLES] = { 0, 2, 0, -2, 1, 1, -1, -1 };
    alignas(16) static float focusPos[4];       /* focusPos$905, sdata 0x0193bc90 */
    alignas(16) float pos[4];
    alignas(16) float right[4];
    alignas(16) float up[4];
    alignas(16) float forward[4];
    alignas(16) float offset[4];
    alignas(16) float eye[4];
    float step;
    int fbuf;
    int i;
    int k;
    short c;

    AllDrawObject(0);
    DrawObjectRefraction(0);
    tiDrawLensFlares();
    PanicFilter();
    if (EffectBuffer_Use != 0)
        return;

    gaBGT_End();
    EffectBuffer_Use = 1;
    asViewMatrix2CamRUF(matrix_global_view, pos, right, up, forward);
    step = (float)HQDOFCameraOffset / 10.0f;
    for (i = 0; i < HQDOF_SAMPLES; i++) {
        HqdofEyeOffset(offset, right, up, step * sampleR[i] / 2.0f, step * sampleU[i] / 2.0f);
        for (k = 0; k < 3; k++)
            eye[k] = offset[k] + pos[k];
        eye[3] = offset[3];
        asEyeForcusUP2CamMat(screen_0[SCREEN_PROJ], matrix_global_view, eye, focusPos, up);

        for (k = 0; k < BANK_MAX; k++) {
            if (BankInfo[k].loaded != 0 && BankInfo[k].type == BANK_TYPE_SHADOW)
                asBuildDOFMatrixTree(pModelPartsHeader[k]);
            if (BankInfo[k].groupCount != 0)
                k += BankInfo[k].groupCount - 1;
        }
        for (c = 0; c < CHARACTER_MAX; c++) {
            if ((Character[c].flags & CHARACTER_FLAG_REFRACTION) && (Character[c].flags & CHARACTER_FLAG_SHADOW)
                && Character[c].bank >= 0)
                asBuildDOFMatrixTree(pModelPartsHeader[Character[c].bank]);
        }

        fbuf = ga_draw_fbuf_address;
        ga_draw_fbuf_address = HQDOF_FBUF;
        FlushCache(0);
        gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
        ssBGT_HQDOF_SetDrawEnvPreRend();
        gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_frameclear);
        gaBGT_ExecDrawCommandData_VIF1(&drawcom_frameclear);
        AllDrawObject(0);
        DrawObjectRefraction(0);
        gaBGT_End();
        ga_draw_fbuf_address = fbuf;
        gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
        ssBGT_HQDOF_WriteBackFB(((int)HQDOFOpacity << 7) / 100);
        gaBGT_End();
    }
    memcpy(focusPos, Cam[asCheckBankID(BANK_SCENE_MAX)].focus, sizeof(focusPos));
    EffectBuffer_Use = 0;
    gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
}

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_REFLECT_MAP: empty in the retail build.
// CT3 0x018362a0

void CaptureReflectMap(void)
{
}

// A bank AllDrawObject's scene passes draw: loaded, of BANK_TYPE_SHADOW (open-items.md item 1 (j)).
static int AllDrawSceneBank(int bank)
{
    return BankInfo[bank].loaded != 0 && BankInfo[bank].type == BANK_TYPE_SHADOW;
}

// A character's light bank is loaded. The ROM reads BankInfo[lightBank].loaded without testing
// lightBank >= 0 in the opaque and translucent passes (BankInfo[-1] is the memory before the table); the
// port treats -1 as not loaded (hazards.md "AlyssaPan").
static int AllDrawLightBankLoaded(short lightBank)
{
    return lightBank >= 0 && BankInfo[lightBank].loaded != 0;
}

// One of AllDrawObject's lists of a bank: main_rtn_DOF in the depth-of-field slices, main_rtn otherwise.
static void AllDrawList(int bank, int list, int chara, int mode, int charaIndex, void **lastTex)
{
    if (mode == ALL_DRAW_DOF_FIRST || mode == ALL_DRAW_DOF_NEXT)
        main_rtn_DOF(bank, list, chara, mode, charaIndex, lastTex);
    else
        main_rtn(bank, list, chara, (u_int)mode, charaIndex, lastTex);
}

// A character's bank as a character: its light (not in the nearer DOF slices), the opaque list, the
// translucent list.
static void AllDrawCharacter(short c, int mode, void **lastTex)
{
    short bank = Character[c].bank;
    ALL_TABLE *tbl = All_table_TBL[bank];

    if (mode != ALL_DRAW_DOF_NEXT)
        CharLightSet(c);
    ALLobject[bank] = tbl->objOpaque;
    AllDrawList(bank, 0, 1, mode, c, lastTex);
    ALLobject[bank] = tbl->objTrans;
    AllDrawList(bank, 1, 1, mode, c, lastTex);
}

// A sorted character the mirror passes reflect: opaque, its bank not BANK_FLAG_NO_MIRROR, its light
// bank loaded with a table, and a mirror there (asEnumMirror).
static int AllDrawMirrored(short c)
{
    CHARACTER *chr = &Character[c];
    short lightBank = chr->lightBank;

    return chr->translucent == 0 && !(BankInfo[chr->bank].flags & BANK_FLAG_NO_MIRROR)
        && AllDrawLightBankLoaded(lightBank) && All_table_TBL[lightBank] != NULL
        && asEnumMirror(All_table_TBL[lightBank]) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// The 3D scene's draw (mode: 0 normal, ALL_DRAW_DOF_FIRST / _NEXT for the depth-of-field slices):
// 1. the opaque list of each scene bank (6..85, a group's banks skipped after its head), under its
//    scene light;
// 2. the shadow volumes here unless event 0x3b is set (then after the mirrors);
// 3. the sorted characters' opaque and translucent lists (CharacterSortBuff, front to back), those
//    with CHARA_TRANS_* bits left for the end;
// 4. back to front, each character standing by a mirror once more through the mirror matrix, culling
//    reversed; then the mirror trees undone in the same order;
// 5. the scene banks' translucent lists, BANK_FLAG_DRAW_FIRST banks last; their particles, then those
//    of the REFRACTION + SHADOW characters;
// 6. the translucent characters (tiChara_Translucence_*) and the ghosts (main_rtn_GHOST), each with
//    its particles.
// lastTex restarts at the passes the ROM restarts it.
// CT3 0x018362b0

void AllDrawObject(int mode)
{
    int fbuf = ga_draw_fbuf_address;
    void *lastTex = NULL;
    float planeVec[4];
    float planePoint[4];
    int count;
    int i;
    short c;

    // 1. the scene banks' opaque lists
    asChangeViewMatrix_forTheBank(asCheckBankID(BANK_SCENE_MAX));
    for (i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (AllDrawSceneBank(i)) {
            if (mode != ALL_DRAW_DOF_NEXT)
                SceneLightSet(i);
            ALLobject[i] = All_table_TBL[i]->objOpaque;
            AllDrawList(i, 0, 0, mode, 0, &lastTex);
        }
        if (BankInfo[i].groupCount != 0)
            i += BankInfo[i].groupCount - 1;
    }
    // (The ROM has an empty loop over CHARACTER_MAX here: a body compiled out of the retail build.)

    // 2. shadow volumes before the characters
    if (ssTrigCheckEvent(EV_SHADOW_AFTER_MIRROR, 1, 0) == 0)
        asDrawShadowVolumeLoop();

    // 3. the sorted characters
    lastTex = NULL;
    asInitMirrorMatrix();
    count = 0;
    for (i = 0; CharacterSortBuff[i] != -1; i++) {
        c = CharacterSortBuff[i];
        if (Character[c].translucent == 0 && AllDrawLightBankLoaded(Character[c].lightBank))
            AllDrawCharacter(c, mode, &lastTex);
        count++;
    }

    // 4. the mirrored characters, back to front
    lastTex = NULL;
    for (i = count - 1; i >= 0; i--) {
        c = CharacterSortBuff[i];
        if (AllDrawMirrored(c)) {
            ALL_TABLE *tbl = All_table_TBL[Character[c].bank];

            asMirrorPlaneCalc(planeVec, planePoint);
            asMakeMirrorMatrixByPlane(planeVec, planePoint);
            asMirrorCulling(0);
            asBuildMirrorMatrixTree(tbl->partsHeader);
            asGAobjectControlPolygonDisplayRotateFlagReverse(tbl);
            AllDrawCharacter(c, mode, &lastTex);
            asMirrorCulling(1);
        }
    }
    asChangeViewMatrix_forTheBank(asCheckBankID(BANK_SCENE_MAX));
    for (i = count - 1; i >= 0; i--) {
        c = CharacterSortBuff[i];
        if (AllDrawMirrored(c)) {
            ALL_TABLE *tbl = All_table_TBL[Character[c].bank];

            asGAobjectControlPolygonDisplayRotateFlagReverse(tbl);
            asBuildMirrorMatrixTreeBack(tbl->partsHeader);
        }
    }
    if (ssTrigCheckEvent(EV_SHADOW_AFTER_MIRROR, 1, 0) != 0)
        asDrawShadowVolumeLoop();

    // 5. the scene banks' translucent lists (BANK_FLAG_DRAW_FIRST last), then the particles
    lastTex = NULL;
    for (i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (AllDrawSceneBank(i) && !(BankInfo[i].flags & BANK_FLAG_DRAW_FIRST)) {
            ALLobject[i] = All_table_TBL[i]->objTrans;
            AllDrawList(i, 1, 0, mode, 0, &lastTex);
        }
        if (BankInfo[i].groupCount != 0)
            i += BankInfo[i].groupCount - 1;
    }
    for (i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (AllDrawSceneBank(i) && (BankInfo[i].flags & BANK_FLAG_DRAW_FIRST)) {
            ALLobject[i] = All_table_TBL[i]->objTrans;
            AllDrawList(i, 1, 0, mode, 0, &lastTex);
        }
        if (BankInfo[i].groupCount != 0)
            i += BankInfo[i].groupCount - 1;
    }
    lastTex = NULL;
    for (i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (AllDrawSceneBank(i)) {
            main_rtn_Particle(i, 0, &lastTex);
            main_rtn_Particle2(i, 0, 0, &lastTex);
        }
        if (BankInfo[i].groupCount != 0)
            i += BankInfo[i].groupCount - 1;
    }
    for (c = 0; c < CHARACTER_MAX; c++) {
        if ((Character[c].flags & CHARACTER_FLAG_REFRACTION) && (Character[c].flags & CHARACTER_FLAG_SHADOW)
            && Character[c].translucent == 0 && Character[c].bank >= 0) {
            main_rtn_Particle(Character[c].bank, 1, &lastTex);
            main_rtn_Particle2(Character[c].bank, 1, 0, &lastTex);
        }
    }

    // 6. the translucent characters and the ghosts
    lastTex = NULL;
    asInitMirrorMatrix();
    for (i = 0; CharacterSortBuff[i] != -1; i++) {
        CHARACTER *chr;
        short bank;
        ALL_TABLE *tbl;

        c = CharacterSortBuff[i];
        chr = &Character[c];
        bank = chr->bank;
        if (chr->translucent & CHARA_TRANS_TRANSLUCENT) {
            tiChara_Translucence_Start();
            if (AllDrawLightBankLoaded(chr->lightBank)) {
                AllDrawCharacter(c, mode, &lastTex);
                tiChara_Translucence_End(chr->translucentAlpha, fbuf);
                main_rtn_Particle(bank, 1, &lastTex);
                main_rtn_Particle2(bank, 1, 0, &lastTex);
            }
        } else if (chr->translucent & CHARA_TRANS_GHOST) {
            tiChara_Translucence_Start2();
            if (AllDrawLightBankLoaded(chr->lightBank)) {
                tbl = All_table_TBL[bank];
                ALLobject[bank] = tbl->objOpaque;
                if (mode == ALL_DRAW_DOF_FIRST || mode == ALL_DRAW_DOF_NEXT)
                    main_rtn_GHOST_DOF(bank, 0, 1, mode, c, &lastTex);
                else
                    main_rtn_GHOST(bank, 0, 1, (u_int)mode, c, &lastTex);
                ALLobject[bank] = tbl->objTrans;
                if (mode == ALL_DRAW_DOF_FIRST || mode == ALL_DRAW_DOF_NEXT)
                    main_rtn_GHOST_DOF(bank, 1, 1, mode, c, &lastTex);
                else
                    main_rtn_GHOST(bank, 1, 1, (u_int)mode, c, &lastTex);
                tiChara_Translucence_End2(fbuf, c);
                main_rtn_Particle(bank, 1, &lastTex);
                main_rtn_Particle2(bank, 1, 0, &lastTex);
            }
        }
    }
}

// One loaded scene bank of GlobalDrawObject's: its view matrix, a cleared Z buffer, its lights, then the
// lists main_rtn and the particle passes draw. The first non-empty "after" list exports the frame to
// the refraction texture once (*exported). The ROM has this body twice, inlined in both loops.
static void GlobalDrawBank(int bank, int *exported, void **lastTex)
{
    ALL_TABLE *tbl = All_table_TBL[bank];

    asChangeViewMatrix_forTheBank(asCheckBankID(bank));
    FlushCache(0);
    ssBGT_ZBufferClear();
    SceneLightSet(bank);
    ALLobject[bank] = tbl->objOpaque;
    main_rtn(bank, 0, 0, 0, 0, lastTex);
    ALLobject[bank] = tbl->objTrans;
    main_rtn(bank, 1, 0, 0, 0, lastTex);
    main_rtn_Particle(bank, 0, lastTex);
    main_rtn_Particle2(bank, 0, 0, lastTex);
    ALLobject[bank] = tbl->objAfter;
    if (!*exported && *ALLobject[bank] != NULL) {
        ExportDrawToTex();
        *exported = 1;
    }
    main_rtn(bank, 0, 0, 0, 0, lastTex);
    if (!*exported && *tbl->particles2After != NULL) {
        ExportDrawToTex();
        *exported = 1;
    }
    main_rtn_Particle2(bank, 0, 1, lastTex);
}

// ──────────────────────────────────────────────────────────────────────
// Every draw mode's last pass: the loaded scene banks 0..5 over the frame, those flagged
// BANK_FLAG_DRAW_FIRST (arrows, "E_" / "M<n>" banks) first, then the rest; the view matrix back to the
// shared bank 6's.
// CT3 0x01837210

void GlobalDrawObject(void)
{
    void *lastTex = NULL;
    int exported = 0;
    int i;

    Clipping_System = 0;
    for (i = 0; i < BANK_SCENE_MAX; i++) {
        if (BankInfo[i].loaded != 0 && (BankInfo[i].flags & BANK_FLAG_DRAW_FIRST))
            GlobalDrawBank(i, &exported, &lastTex);
    }
    for (i = 0; i < BANK_SCENE_MAX; i++) {
        if (BankInfo[i].loaded != 0 && !(BankInfo[i].flags & BANK_FLAG_DRAW_FIRST))
            GlobalDrawBank(i, &exported, &lastTex);
    }
    asChangeViewMatrix_forTheBank(BANK_SCENE_MAX);
}

// ──────────────────────────────────────────────────────────────────────
// A bank's second particle list -- ALL_TABLE.particles2, or particles2After (after != 0, whose textures
// are not uploaded: the caller's ExportDrawToTex pass) -- under the bank's fog, or the current scene's
// character fog (fog != 0). Each entry draws its count live records from head (wrapping at ringSize),
// each at the entry's model part position plus the record's offset, its w scaled around
// PARTS_POS_W_CENTER; draw control and texture switch as in main_rtn_Particle. *lastTex as in main_rtn.
// CT3 0x01837570

void main_rtn_Particle2(int bank, int fog, int after, void **lastTex)
{
    ALL_TABLE *tbl;
    TEX_INFO *texNow;
    int ctrlNow = -1;
    u_int *timNow = NULL;
    void **list;
    FOG_INFO *info;
    int i;

    if (asCheckBankType(bank) == 0)
        asChangeViewMatrix_forTheBank(asCheckBankID(bank));
    tbl = All_table_TBL[bank];
    texNow = (TEX_INFO *)*lastTex;
    list = after == 0 ? tbl->particles2 : tbl->particles2After;
    if (fog == 0) {
        info = (FOG_INFO *)FogCtrlAddr[bank];
        gaSetupFogCtrlData(&fogctrl[bank], info->fog[0], info->fog[1], info->fog[2],
                           info->fogArg[0], info->fogArg[1], info->fogArg[2]);
    } else {
        info = (FOG_INFO *)FogCtrlAddr[idCurrentScene];
        gaSetupFogCtrlData(&fogctrl[bank], info->charaFog[0], info->charaFog[1], info->charaFog[2],
                           info->charaFogArg[0], info->charaFogArg[1], info->charaFogArg[2]);
    }
    gaSet3dlibStruct(MAIN_RTN_CLIP_FAR, ga3dlibwork, &drawbuffer_0, &fogctrl[bank], screen_0);
    tnInit3dlibWork(ga3dlibwork, SPR_3DLIB_WORK);
    gaBGT_ExecFogCtrlData_VIF1(&fogctrl[bank]);
    {
        int fogBank = fog == 0 ? bank : idCurrentScene;
        MODEL_PARTS *fogParts = (MODEL_PARTS *)pModelPartsHeader[fogBank];

        if (fogParts[((FOG_INFO *)FogCtrlAddr[fogBank])->parts].flags & PARTS_FLAG_FOG)
            FOG_mode = 1;
    }

    for (i = 0; list[i] != NULL; i++) {
        PARTICLE2_ENTRY *ent = (PARTICLE2_ENTRY *)list[i];
        TEX_INFO *tex = ent->tex;
        const float *base;
        int j;

        if (ent->count == 0)
            continue;
        if (ent->drawCtrlId != 0 && ent->drawCtrlId != ctrlNow)
            gaBGT_ExecDrawCtrlData_VIF1((GA_DRAWCTRL_DATA *)ent->drawCtrl);
        if (after == 0 && tex != NULL && tex != texNow) {
            if (tex->tim != timNow) {
                sunBGT_LoadGSTexClutData_VIF1(ent->tbuffer);
                gaBGT_End();
                gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                timNow = tex->tim;
            }
            sunBGT_ExecTBufferData_VIF1(ent->tbuffer);
        }
        base = tbl->partsPos[ent->parts];
        for (j = 0; j < ent->count; j++) {
            PARTICLE2 *rec = &ent->ring[(ent->head + j) % ent->ringSize];
            u_char *obj = rec->obj;

            obj[DRAW_OBJ_CLIP] = (u_char)gaObjectClippingCheck(obj);
            if ((s_char)obj[DRAW_OBJ_CLIP] >= 0) {
                float *pos = (float *)BankEePtr(bank, *(u_int *)(obj + DRAW_OBJ_POS));

                pos[0] = base[0] + rec->offset[0];
                pos[1] = base[1] + rec->offset[1];
                pos[2] = base[2] + rec->offset[2];
                pos[3] = (base[3] + PARTS_POS_W_CENTER) * rec->offset[3] - PARTS_POS_W_CENTER;
                gaBGT_DrawObject(obj, 0);
            }
        }
        ctrlNow = ent->drawCtrlId;
        texNow = tex;
    }
    *lastTex = texNow;
}

// ──────────────────────────────────────────────────────────────────────
// A bank's particle systems (ALL_TABLE.particles) under the bank's fog, or the current scene's
// character fog (fog != 0): each system's live ring slots head .. tail, clip-checked, texture and draw
// control switched when they change (a new texture restarts the BGT ring). *lastTex as in main_rtn.
// CT3 0x018379d0

void main_rtn_Particle(int bank, int fog, void **lastTex)
{
    TEX_INFO *texNow = (TEX_INFO *)*lastTex;
    int ctrlNow = -1;
    u_int *timNow = NULL;
    void **list = All_table_TBL[bank]->particles;
    FOG_INFO *info;
    int i;

    if (asCheckBankType(bank) == 0)
        asChangeViewMatrix_forTheBank(asCheckBankID(bank));
    if (fog == 0) {
        info = (FOG_INFO *)FogCtrlAddr[bank];
        gaSetupFogCtrlData(&fogctrl[bank], info->fog[0], info->fog[1], info->fog[2],
                           info->fogArg[0], info->fogArg[1], info->fogArg[2]);
    } else {
        info = (FOG_INFO *)FogCtrlAddr[idCurrentScene];
        gaSetupFogCtrlData(&fogctrl[bank], info->charaFog[0], info->charaFog[1], info->charaFog[2],
                           info->charaFogArg[0], info->charaFogArg[1], info->charaFogArg[2]);
    }
    gaSet3dlibStruct(MAIN_RTN_CLIP_FAR, ga3dlibwork, &drawbuffer_0, &fogctrl[bank], screen_0);
    tnInit3dlibWork(ga3dlibwork, SPR_3DLIB_WORK);
    gaBGT_ExecFogCtrlData_VIF1(&fogctrl[bank]);
    {
        int fogBank = fog == 0 ? bank : idCurrentScene;
        MODEL_PARTS *fogParts = (MODEL_PARTS *)pModelPartsHeader[fogBank];

        if (fogParts[((FOG_INFO *)FogCtrlAddr[fogBank])->parts].flags & PARTS_FLAG_FOG)
            FOG_mode = 1;
    }

    for (i = 0; list[i] != NULL; i++) {
        PARTICLE_ENTRY *ent = (PARTICLE_ENTRY *)list[i];
        PARTICLE_STATE *state = ent->state;
        TEX_INFO *tex = ent->tex;
        u_int slot = state->head;
        u_int tail = state->tail;

        if (slot == tail)
            continue;
        if (ent->drawCtrlId != 0 && ent->drawCtrlId != ctrlNow)
            gaBGT_ExecDrawCtrlData_VIF1((GA_DRAWCTRL_DATA *)ent->drawCtrl);
        if (tex != NULL && tex != texNow) {
            if (tex->tim != timNow) {
                sunBGT_LoadGSTexClutData_VIF1(ent->tbuffer);
                gaBGT_End();
                gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                timNow = tex->tim;
            }
            sunBGT_ExecTBufferData_VIF1(ent->tbuffer);
        }
        texNow = tex;
        ctrlNow = ent->drawCtrlId;

        while (slot != tail) {
            u_char *obj = ParticleObject[bank][i] + slot * PARTICLE_OBJ_SIZE;

            obj[DRAW_OBJ_CLIP] = (u_char)gaObjectClippingCheck(obj);
            if ((s_char)obj[DRAW_OBJ_CLIP] >= 0) {
                drawobj_attr = 0;
                gaBGT_DrawObject(obj, 0);
            }
            slot++;
            if (slot == state->ringSize)
                slot = 0;
        }
    }
    *lastTex = texNow;
}

// ──────────────────────────────────────────────────────────────────────
// One draw list of a bank (ALLobject[bank]), sorted for the passes that sort it (mode 1 lists into
// osTotalObjTable[mode][bank], motion-blurred parts inserted), under the bank's fog -- or, for a
// character pass (chara != 0), the current scene's character fog and the light of the character's
// light bank, eased in over DirectLightCharaCounter frames. Each part: skipped when it is a fog volume
// or hidden (partsPos w), clip-checked unless MAIN_RTN_NO_CLIP, its texture uploaded and its draw
// control switched only when they change; *lastTex carries the last texture across calls.
// CT3 0x01838600

void main_rtn(int bank, int mode, int chara, u_int flags, int charaIndex, void **lastTex)
{
    short lightBank = -1;
    float lightFrames = 0.0f;
    int count;
    int blur;
    u_char *table;
    MODEL_PARTS *parts;
    FOG_INFO *fog;
    TEX_INFO *texNow;
    u_int *timNow = NULL;
    GA_DRAWCTRL_ARGS *ctrlNow = DRAW_CTRL_NONE;
    int zonlyParts = -1;
    u_int pass = flags & MAIN_RTN_PASS_MASK;
    int i;

    if (chara != 0) {
        lightBank = Character[charaIndex].lightBank;
        lightFrames = (float)DirectLightCharaCounter[charaIndex];
    }
    if (asCheckBankType(bank) == 0)
        asChangeViewMatrix_forTheBank(asCheckBankID(bank));
    asViewMatrix2PosRUF(&matrix_global_view[0][0], CameraPos, NULL, NULL, NULL);
    count = get_draw_obj(bank);
    if (count == 1)
        return;

    blur = ssGetNumBlurDrawObjForScene(bank, mode);
    texNow = (TEX_INFO *)*lastTex;
    table = osTotalObjTable[mode][bank];
    if (mode != 0 && count != 0) {
        if (pass == MAIN_RTN_PASS_SORT || pass == MAIN_RTN_PASS_SORT2) {
            if (blur == 0)
                objSort(bank, count, table);
            else
                objSortWithBlurIdx(bank, count, table);
        }
        ALLobject[bank] = (void **)table;
    }
    if ((pass == MAIN_RTN_PASS_SORT || pass == MAIN_RTN_PASS_SORT2) && blur != 0) {
        ssPartMotionBlurInsertObjToDrawTable(bank, table, ALLobject[bank], count, mode);
        ALLobject[bank] = (void **)table;
    }

    if (chara == 0) {
        fog = (FOG_INFO *)FogCtrlAddr[bank];
        gaSetupFogCtrlData(&fogctrl[bank], fog->fog[0], fog->fog[1], fog->fog[2],
                           fog->fogArg[0], fog->fogArg[1], fog->fogArg[2]);
    } else {
        fog = (FOG_INFO *)FogCtrlAddr[idCurrentScene];
        gaSetupFogCtrlData(&fogctrl[bank], fog->charaFog[0], fog->charaFog[1], fog->charaFog[2],
                           fog->charaFogArg[0], fog->charaFogArg[1], fog->charaFogArg[2]);
    }
    gaSet3dlibStruct(MAIN_RTN_CLIP_FAR, ga3dlibwork, &drawbuffer_0, &fogctrl[bank], screen_0);
    if (Clipping_System == 1)
        Clipping_Portal(ga3dlibwork, screen_0);
    tnInit3dlibWork(ga3dlibwork, SPR_3DLIB_WORK);
    gaBGT_ExecFogCtrlData_VIF1(&fogctrl[bank]);

    {
        int fogBank = chara == 0 ? bank : idCurrentScene;
        MODEL_PARTS *fogParts = (MODEL_PARTS *)pModelPartsHeader[fogBank];

        if (fogParts[((FOG_INFO *)FogCtrlAddr[fogBank])->parts].flags & PARTS_FLAG_FOG)
            FOG_mode = 1;
    }

    for (i = 0; ALLobject[bank][i] != NULL; i++) {
        DRAW_OBJ_ENTRY *ent = (DRAW_OBJ_ENTRY *)ALLobject[bank][i];
        u_char *obj = ent->obj;
        TEX_INFO *tex = ent->tex;
        int clip;

        parts = &((MODEL_PARTS *)pModelPartsHeader[bank])[ent->parts];
        if (parts->flags & PARTS_FLAG_FOG)
            continue;
        if (!(All_table_TBL[bank]->partsPos[ent->parts][3] > PARTS_POS_HIDDEN_W))
            continue;
        if ((parts->flags & PARTS_FLAG_ZONLY_FIRST) && zonlyParts != ent->parts) {
            zonlyParts = OneMaxObjectDrawZOnly(bank, i);
            ctrlNow = DRAW_CTRL_NONE;
        }
        clip = (flags & MAIN_RTN_NO_CLIP) == 0 ? gaObjectClippingCheck(obj) : 0;
        if (clip < 0)
            continue;

        if (texNow != tex && tex != NULL && tex->tim != timNow && tex->tim != NULL) {
            sunSetupTBufferData(ent->tbuffer, *tex);
            sunBGT_LoadGSTexClutData_VIF1(ent->tbuffer);
        }
        if (ctrlNow != ent->drawCtrlArgs && ent->drawCtrlArgs != NULL) {
            gaBGT_ExecDrawCtrlData_VIF1((GA_DRAWCTRL_DATA *)ent->drawCtrl);
            ctrlNow = ent->drawCtrlArgs;
        }
        *(int *)(obj + DRAW_OBJ_ATTR) = parts->attr;
        if (tex != NULL) {
            float *light = (float *)(obj + DRAW_OBJ_LIGHT);

            if (chara == 0) {
                if (tex->kind == TEX_KIND_LIGHT)
                    *light = All_table_TBL[bank]->partsLight[ent->parts];
            } else if (tex->kind == TEX_KIND_LIGHT && lightBank != -1 && BankInfo[lightBank].loaded == 1) {
                float target = All_table_TBL[lightBank]->light[2] * All_table_TBL[bank]->partsLight[ent->parts];

                if (lightFrames == 0.0f)
                    *light = target;
                else
                    *light = *light + (target - *light) / lightFrames;
            }
        }

        drawobj_attr = 0;
        if (texNow != tex && tex != NULL) {
            if (tex->tim != timNow && tex->tim != NULL) {
                if (bank < BANK_SCENE_MAX && strcmp(BANK_NAME_MENU, BankInfo[0].name) == 0) {
                    gaBGT_End();
                    gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                } else if (tex->tim[0] <= TEX_SMALL_MAX && tex->tim[1] <= TEX_SMALL_MAX) {
                    gaBGT_End();
                    gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                }
                timNow = tex->tim;
            }
            sunBGT_ExecTBufferData_VIF1(ent->tbuffer);
            texNow = tex;
        }
        if (clip == 0 && !(obj[DRAW_OBJ_FLAGS] & DRAW_OBJ_FLAG_CLIP)) {
            gaBGT_DrawObject_NoClippingCheck2(obj, drawobj_attr);
        } else {
            obj[DRAW_OBJ_CLIP] = (u_char)clip;
            gaBGT_DrawObject(obj, drawobj_attr);
        }
    }
    *lastTex = texNow;
}

// ──────────────────────────────────────────────────────────────────────
// The refraction pass: each loaded bank of BANK_TYPE_SHADOW past the scene banks (6..85, a group's
// banks skipped after its head), then the bank of each character flagged REFRACTION and SHADOW, as
// characters: its "after" list (main_rtn_DOF in the DOF slices, main_rtn otherwise) and its
// after-particles. The first non-empty list exports the frame to the refraction texture once.
// RefractionDrawBank is one bank of it (the ROM has this body twice, inlined in both loops).

static void RefractionDrawBank(int bank, int chara, int mode, int charaIndex, int *exported, void **lastTex)
{
    ALL_TABLE *tbl = All_table_TBL[bank];

    ALLobject[bank] = tbl->objAfter;
    if (!*exported && *ALLobject[bank] != NULL) {
        ExportDrawToTex();
        *exported = 1;
    }
    if (mode == ALL_DRAW_DOF_FIRST || mode == ALL_DRAW_DOF_NEXT)
        main_rtn_DOF(bank, 0, chara, mode, charaIndex, lastTex);
    else
        main_rtn(bank, 0, chara, (u_int)mode, charaIndex, lastTex);
    if (!*exported && *tbl->particles2After != NULL) {
        ExportDrawToTex();
        *exported = 1;
    }
    main_rtn_Particle2(bank, chara, 1, lastTex);
}

// CT3 0x01836f30

void DrawObjectRefraction(int mode)
{
    void *lastTex = NULL;
    int exported = 0;
    int i;
    short c;

    for (i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (BankInfo[i].loaded != 0 && BankInfo[i].type == BANK_TYPE_SHADOW)
            RefractionDrawBank(i, 0, mode, 0, &exported, &lastTex);
        if (BankInfo[i].groupCount != 0)
            i += BankInfo[i].groupCount - 1;
    }
    for (c = 0; c < CHARACTER_MAX; c++) {
        if ((Character[c].flags & CHARACTER_FLAG_REFRACTION) && (Character[c].flags & CHARACTER_FLAG_SHADOW)
            && Character[c].bank >= 0)
            RefractionDrawBank(Character[c].bank, 1, mode, c, &exported, &lastTex);
    }
}

// ──────────────────────────────────────────────────────────────────────
// main_rtn for one depth-of-field slice (pass ALL_DRAW_DOF_FIRST / _NEXT): the same list, sort, fog
// and per-part texture / light / draw control as main_rtn, but no camera position update, parts of
// PARTS_KIND_NO_DOF skipped, the clip check run only in the first (farthest) slice and its byte kept
// for the nearer ones, and each part drawn only when ssDOF_ClipObjectFromZRange puts it in the slice.
// CT3 0x01837d90

void main_rtn_DOF(int bank, int mode, int chara, int pass, int charaIndex, void **lastTex)
{
    short lightBank = -1;
    float lightFrames = 0.0f;
    int count;
    int blur;
    u_char *table;
    FOG_INFO *fog;
    TEX_INFO *texNow;
    u_int *timNow = NULL;
    GA_DRAWCTRL_ARGS *ctrlNow = DRAW_CTRL_NONE;
    int zonlyParts = -1;
    int i;

    if (chara == 1) {
        lightBank = Character[charaIndex].lightBank;
        lightFrames = (float)DirectLightCharaCounter[charaIndex];
    }
    if (asCheckBankType(bank) == 0)
        asChangeViewMatrix_forTheBank(asCheckBankID(bank));
    count = get_draw_obj(bank);
    if (count == 1)
        return;

    blur = ssGetNumBlurDrawObjForScene(bank, mode);
    texNow = (TEX_INFO *)*lastTex;
    table = osTotalObjTable[mode][bank];
    if (mode != 0 && count != 0) {
        if (blur == 0)
            objSort(bank, count, table);
        else
            objSortWithBlurIdx(bank, count, table);
        ALLobject[bank] = (void **)table;
    }
    if (blur != 0) {
        ssPartMotionBlurInsertObjToDrawTable(bank, table, ALLobject[bank], count, mode);
        ALLobject[bank] = (void **)table;
    }

    if (chara == 0) {
        fog = (FOG_INFO *)FogCtrlAddr[bank];
        gaSetupFogCtrlData(&fogctrl[bank], fog->fog[0], fog->fog[1], fog->fog[2],
                           fog->fogArg[0], fog->fogArg[1], fog->fogArg[2]);
    } else {
        fog = (FOG_INFO *)FogCtrlAddr[idCurrentScene];
        gaSetupFogCtrlData(&fogctrl[bank], fog->charaFog[0], fog->charaFog[1], fog->charaFog[2],
                           fog->charaFogArg[0], fog->charaFogArg[1], fog->charaFogArg[2]);
    }
    gaSet3dlibStruct(MAIN_RTN_CLIP_FAR, ga3dlibwork, &drawbuffer_0, &fogctrl[bank], screen_0);
    if (Clipping_System == 1)
        Clipping_Portal(ga3dlibwork, screen_0);
    tnInit3dlibWork(ga3dlibwork, SPR_3DLIB_WORK);
    gaBGT_ExecFogCtrlData_VIF1(&fogctrl[bank]);

    {
        int fogBank = chara == 0 ? bank : idCurrentScene;
        MODEL_PARTS *fogParts = (MODEL_PARTS *)pModelPartsHeader[fogBank];

        if (fogParts[((FOG_INFO *)FogCtrlAddr[fogBank])->parts].flags & PARTS_FLAG_FOG)
            FOG_mode = 1;
    }

    for (i = 0; ALLobject[bank][i] != NULL; i++) {
        DRAW_OBJ_ENTRY *ent = (DRAW_OBJ_ENTRY *)ALLobject[bank][i];
        u_char *obj = ent->obj;
        TEX_INFO *tex = ent->tex;
        MODEL_PARTS *parts = &((MODEL_PARTS *)pModelPartsHeader[bank])[ent->parts];

        if (parts->flags & PARTS_FLAG_FOG)
            continue;
        if (!(All_table_TBL[bank]->partsPos[ent->parts][3] > PARTS_POS_HIDDEN_W))
            continue;
        if (parts->kind == PARTS_KIND_NO_DOF)
            continue;
        if ((parts->flags & PARTS_FLAG_ZONLY_FIRST) && zonlyParts != ent->parts) {
            zonlyParts = OneMaxObjectDrawZOnly(bank, i);
            ctrlNow = DRAW_CTRL_NONE;
        }
        if (pass == ALL_DRAW_DOF_FIRST)
            obj[DRAW_OBJ_CLIP] = (u_char)gaObjectClippingCheck(obj);
        if ((s_char)obj[DRAW_OBJ_CLIP] < 0)
            continue;

        if (texNow != tex && tex != NULL && tex->tim != timNow && tex->tim != NULL) {
            sunSetupTBufferData(ent->tbuffer, *tex);
            sunBGT_LoadGSTexClutData_VIF1(ent->tbuffer);
        }
        if (ssDOF_ClipObjectFromZRange(obj, (u_int)ent->dofMode) == 0)
            continue;
        if (ctrlNow != ent->drawCtrlArgs && ent->drawCtrlArgs != NULL) {
            gaBGT_ExecDrawCtrlData_VIF1((GA_DRAWCTRL_DATA *)ent->drawCtrl);
            ctrlNow = ent->drawCtrlArgs;
        }
        *(int *)(obj + DRAW_OBJ_ATTR) = parts->attr;
        if (tex != NULL) {
            float *light = (float *)(obj + DRAW_OBJ_LIGHT);

            if (chara == 0) {
                if (tex->kind == TEX_KIND_LIGHT)
                    *light = All_table_TBL[bank]->partsLight[ent->parts];
            } else if (tex->kind == TEX_KIND_LIGHT && lightBank != -1 && BankInfo[lightBank].loaded == 1) {
                float target = All_table_TBL[lightBank]->light[2] * All_table_TBL[bank]->partsLight[ent->parts];

                if (lightFrames == 0.0f)
                    *light = target;
                else
                    *light = *light + (target - *light) / lightFrames;
            }
        }

        drawobj_attr = 0;
        if (texNow != tex && tex != NULL) {
            if (tex->tim != timNow && tex->tim != NULL) {
                if (tex->tim[0] <= TEX_SMALL_MAX && tex->tim[1] <= TEX_SMALL_MAX) {
                    gaBGT_End();
                    gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                }
                timNow = tex->tim;
            }
            sunBGT_ExecTBufferData_VIF1(ent->tbuffer);
            texNow = tex;
        }
        if (obj[DRAW_OBJ_CLIP] == 0 && !(obj[DRAW_OBJ_FLAGS] & DRAW_OBJ_FLAG_CLIP))
            gaBGT_DrawObject_NoClippingCheck2(obj, drawobj_attr);
        else
            gaBGT_DrawObject(obj, drawobj_attr);
    }
    *lastTex = texNow;
}

// ──────────────────────────────────────────────────────────────────────
// A scene's teardown: triggers, particles, motion blur, its camera, shadows, wind, the seek bars, the
// lock-on list (100000000 = all) and its animations.
// CT3 0x01838f20

void scene_end(int scene)
{
    Trigger_deinit();
    ssParticleDeInit(scene);
    ssPartMotionBlurAllEnd(scene);
    camera_destroy(scene, 0);
    asShadowDrawing_Fin(scene);
    tiWindEnd(scene);
    asSeekBarFreeForX(scene);
    asXcmd_AddForLockToTargChara(100000000, 0, scene);
    asForceOperateAnimation_Scene(scene, 0);
}

// ──────────────────────────────────────────────────────────────────────
// A loaded scene's draw set-up (TrgSceneDraw, TrgGLBLSceneDraw): matrices, hits, AI; the draw-first flag
// by bank name; the actress's three light parts; the second object list sorted into osTotalObjTable[1].
// CT3 0x01838fa0

void scene_draw_init(int scene)
{
    BANK_INFO *bank = &BankInfo[scene];
    char name[16];
    char head[16];
    int n;

    BuildMatrixTree(scene, 1, 1);
    ssHitInit(scene);
    ssAI_SceneInit(scene);

    if (bank->type == 1 || bank->type == 0) {
        strcpy(name, bank->name);
        if (strcmp(name, BANK_NAME_ARROW) == 0)
            bank->flags |= BANK_FLAG_DRAW_FIRST;
        strcpy(name, bank->name);
        name[2] = '\0';
        if (strcmp(name, BANK_PREFIX_E) == 0)
            bank->flags |= BANK_FLAG_DRAW_FIRST;
    }
    if (bank->type == 0) {
        strcpy(head, bank->name);
        head[1] = '\0';
        if (strcmp(head, BANK_PREFIX_M) == 0 && bank->name[1] >= '0' && bank->name[1] <= '9')
            bank->flags |= BANK_FLAG_DRAW_FIRST;
    }

    Omni0_num[scene] = asFindModelPartsID(pModelPartsHeader[scene], ahGetHash(PARTS_ACTRESS_OMNI_1));
    Omni1_num[scene] = asFindModelPartsID(pModelPartsHeader[scene], ahGetHash(PARTS_ACTRESS_OMNI_2));
    ACTRESS_LIGHT_BASE_MAX_ID[scene] =
        asFindModelPartsID(pModelPartsHeader[scene], ahGetHash(PARTS_ACTRESS_OMNI_BASE));

    for (n = 0; All_table_TBL[scene]->objTrans[n] != NULL; n++)
        ;
    if (ssGetNumBlurDrawObjForScene(scene, 1) != 0)
        objSortWithBlurIdx(scene, n + 1, osTotalObjTable[1][scene]);
    else
        objSort(scene, n + 1, osTotalObjTable[1][scene]);
    asForceOperateAnimation_Scene(scene, 0);
}

static u_long FogClearColor(const u_char *fog);

// ──────────────────────────────────────────────────────────────────────
// A bank's set-up once its file is loaded (FSMltLoadProc): the triggers, the bank's tables out of its
// ALL_TABLE (object count, parts header, behaviours, fog), its fog packet (cleared) and the 3dlib, the
// matrix tree, the scene's animations, then each subsystem's init, and every draw list entry's texture
// buffer and draw control packets (the transparent, opaque and after-particle lists).
// CT3 0x01839270

void scene_init(int scene)
{
    ALL_TABLE **tbl;
    int list, i;
    void **objs = NULL;

    Trigger_init();
    tbl = &All_table_TBL[scene];
    BaseobjCount[scene] = (*tbl)->baseobjCount;
    pModelPartsHeader[scene] = (*tbl)->partsHeader;
    pBehaviorHeader[scene] = (*tbl)->behavior;
    gaSetupFogCtrlData(&fogctrl[scene], 0.0f, 0.0f, 0.0f, 0, 0, 0);
    if (BankInfo[scene].type == BANK_TYPE_SHADOW || BankInfo[scene].type == BANK_TYPE_0)
        FogCtrlAddr[scene] = (*tbl)->fog;
    if (BankInfo[scene].type == BANK_TYPE_SHADOW)
        drawcom_frameclear.rgbaq = FogClearColor(FogCtrlAddr[scene]);
    gaSet3dlibStruct(MAIN_RTN_CLIP_FAR, ga3dlibwork, &drawbuffer_0, &fogctrl[scene], screen_0);
    InitMatrixTree(pModelPartsHeader[scene]);
    LinkMatrixTree((*tbl)->objOpaque, pModelPartsHeader[scene]);
    if ((s_char)pModelPartsHeader[scene][0] == PARTS_HEADER_ANIMATED) {
        asBehaviorStopAnimation(pBehaviorHeader[scene]);
        asSceneAutoStartAnimation(pBehaviorHeader[scene]);
    }
    SwapDrawingDoor();
    BuildMatrixTree(scene, 1, 1);
    ssParticleInit(scene);
    ssSetAmbColorToAllObj(scene);
    tiHurikoInit(scene);
    tiParticleInit(scene);
    tiLensFlaresInit(scene);
    asShadowDrawing_Init(scene);
    asSeatArrange_Init(scene);

    for (list = 0; list < SCENE_INIT_LISTS; list++) {
        if (list == 2)
            objs = (*tbl)->objAfter;
        else if (list == 1)
            objs = (*tbl)->objOpaque;
        else if (list == 0)
            objs = (*tbl)->objTrans;
        for (i = 0; objs[i] != NULL; i++) {
            DRAW_OBJ_ENTRY *ent = (DRAW_OBJ_ENTRY *)objs[i];
            TEX_INFO *tex = ent->tex;
            GA_DRAWCTRL_DATA *ctrl = (GA_DRAWCTRL_DATA *)ent->drawCtrl;
            GA_DRAWCTRL_ARGS *args = ent->drawCtrlArgs;

            if (tex != NULL)
                sunSetupTBufferData(ent->tbuffer, *tex);
            gaSetupDrawCtrlData(ctrl, *args);
        }
    }
    AmbientAlphaInit(scene);
    InitTriggerDetectorShape(scene);
    TexFileSetInit(scene);
    tiWindInit(scene);
    tiLightBelt2Init(scene);
    asSeekBarInitializeForX(scene);
}

// ──────────────────────────────────────────────────────────────────────
// Each game's end (main's outer loop, before the heaps are rebuilt): the title's cancel flag from
// event 0x10, scene loads, warps, files, characters, wind, fades, sound, the boss, animation history,
// the monsters' panic stack, arrows, moths, vibration and the demo skip.
// CT3 0x01839670

void system_end(void)
{
    fReset = 0;
    TITLE_CANCEL_FLAG = (u_char)ssTrigCheckEvent(EV_TITLE_CANCEL, 1, 0);
    SelfDiscardSceneLoadInit();
    WarpObjectUpdateInit();
    FS_work_init();
    Character_All_Init();
    tiWindEnd_All();
    fFade = 0;
    fShadeOff = 0;
    fKeepScreen = 0;
    Sound_system_end();
    tiFreeBossData();
    ExecuteAnimationHistory_Free();
    asMonsterDistanceStack_inPanic(1, 0, 0);
    asDestroyArrowInfo();
    asDestroyMothInfo(0);
    DEMO_SKIP_SET = 0;
    tiVibrationOff(0);
    SkipMain = 0;
    DemoSkip = 0;
    DEMO_SKIP_FLAG = 0;
    DemoFilterChangeWork.active = 0;
    Demo_Skip_Set = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Each game start (main's outer loop): pause and time stop off, shadows, items, AI flags, the
// controllers, the camera, the clipping window (512 x 448), draw modes, the hit / particle / trigger /
// AI systems, the lights (-1 = no omni effect), panic, fog animation, the sound system, and the trigger
// variables the options and the title screen set.
// CT3 0x01839720

void system_init(void)
{
    int i;

    pause = 0;
    pause_reservation = 0;
    time_stop_fream = 0;
    time_stop_flag = 0;
    sFirstCheck = 1;
    CharStopFlag = 0;
    TrgCharStopFlag = 0;
    Shadow_OnceInit();
    controller_mode = CONTROLLER_MODE_NORMAL;
    save_count = 0;
    progress_time = 0;
    msg_num = 0;
    MonsterHpDispCounter = 0;
    MonsterHpDispCharID = -1;
    tiItemInit();
    for (i = 0; i < CHARACTER_MAX; i++)
        AI_flag[i] = 0;
    VistaVisionNumber = 0;
    controller_num = 0;
    controller_num2 = 0;
    ChrMoveControllerMode_init();
    camera_init();
    ExecuteAnimationHistory_init();
    Clipping_X1_pos = 0.0f;
    Clipping_Y1_pos = 0.0f;
    Clipping_X2_pos = 512.0f;
    Clipping_Y2_pos = 448.0f;
    VistaVisionMode = 1;
    SS_fb_write_msb = 0;
    SS_ZDrawMode = 0;
    SS_ForceAlphaMode = 0;
    DrawMode = DRAW_MODE_NORMAL;
    prevDrawMode = DRAW_MODE_NORMAL;
    ssHitGlobalInit();
    ssPartMotionBlurGlobalInit();
    TriggerGlobalInit();
    ssAI_GlobalInit();
    ssTrigSetVariable(0x12, 3, 1, 0);
    ssTrigSetVariable(0x21, 5, 1, 0);
    for (i = 0; i < CHARACTER_MAX; i++) {
        int k;

        DirectLightCharaCounter[i] = 0;
        CharDirectLightSet[i][0] = 0;
        CharDirectLightSet[i][1] = 0;
        for (k = 0; k < 8; k++)
            CharOmniEffect[i][k] = 0xffff;
    }
    for (i = 0; i < 8; i++)
        AllOmniEffect[i] = 0xffff;
    PanicModeInit();
    asPanicStatusCheck(0);
    asMonsterDistanceStack_inPanic(-1, 0, 0);
    ControllerClatteringInit();
    for (i = 0; i < BANK_MAX; i++) {
        FogAnimeBuff[i].a = 0;
        FogAnimeBuff[i].b = 0;
        FogAnimeBuff[i].c = 0;
        FogAnimeBuff[i].d = 0;
    }
    asResetCharacterRotation(-1, 0, 0);
    ssTrigSetVariable(0x3e, 3, 1, 0);
    ssTrigSetVariable(0x40, 3, 1, 0);
    ssTrigSetVariable(0x3d, 0, 1, 0);
    ssTrigSetVariable(0x1d, 1, 1, 0);
    FilterChangeWork.active = 0;
    ssTrigSetVariable(0x25, 0, 1, 0);
    ssTrigSetVariable(0x24, 0, 1, 0);
    asInitArrowInfo();
    asInitMothInfo();
    asXcmd_AddForLockToTargChara(0, 0, 0);
    asAlyssaLockedByAnything(2);
    asRapidLeverChangeOperate(1, 0, 0, 0, 0);
    asKatchuKekkaiCheck(-1);
    TexFileSysInitStop = 0;
    anim_play_slow = 100;
    Backup_Filename[0] = 0;
    Sound_system_init();
    if (SOUND_STEREO_MONAURAL_BUFF == 0)
        ssTrigRemoveEvent(0xa8, 1, 0);
    else
        ssTrigRegisterEvent(0xa8, 1, 0);
    ssTrigSetVariable(0x4e, BGM_MASTER_VOLUME_BUFF, 1, 0);
    ssTrigSetVariable(0x4f, SE_MASTER_VOLUME_BUFF, 1, 0);
    SSSetSoundMode(0);
    ssTrigRegisterEvent(0x54, 1, 0);
    ssTrigSetVariable(0x45, 1, 1, 0);
    ssTrigSetVariable(0x46, 0x40, 1, 0);
    ssTrigSetVariable(0x47, 0x80, 1, 0);
    ssTrigSetVariable(0x48, 0xc0, 1, 0);
    ssTrigSetVariable(0x49, 0xff, 1, 0);
    MoviePlayCheck = 0;
    MovieEndCheck = 0;
    ControlDisableflag = 0;
    if (OMAKE_FLAG & OMAKE_1)
        ssTrigRegisterEvent(0xa7, 1, 0);
    if ((OMAKE_FLAG & OMAKE_2) == 0)
        ssTrigRemoveEvent(0xc5, 1, 0);
    else
        ssTrigRegisterEvent(0xc5, 1, 0);
    if (LOAD_SLOT_NUM == 0)
        ssTrigRemoveEvent(0xf, 1, 0);
    else
        ssTrigRegisterEvent(0xf, 1, 0);
    ssTrigSetVariable(0x4c, TITLE_SELECT_NUM, 1, 0);
    if (TITLE_CANCEL_FLAG == 0)
        ssTrigRemoveEvent(0x10, 1, 0);
    else
        ssTrigRegisterEvent(0x10, 1, 0);
    ssTrigRegisterEvent(0xab, 1, 0);
    ssTrigRegisterEvent(0xac, 1, 0);
}

// The frame's clear colour: the scene's fog colour (R +4, G +8, B +0xc) under Q 1.0 / A 0x80.
// Host: before a scene loads its fog table the pointer is null, which the EE reads harmlessly (kernel RAM
// at 0x4..0xc); the host clears to black instead.
static u_long FogClearColor(const u_char *fog)
{
    if (fog == 0)
        return FRAMECLEAR_RGBAQ;
    return (u_long)(u_int)(fog[0xc] << 16 | fog[8] << 8 | fog[4]) | FRAMECLEAR_RGBAQ;
}

// ──────────────────────────────────────────────────────────────────────
// The objects in the bank's current list (ALLobject[bank]) plus one: its terminator.
// CT3 0x01839cd0

int get_draw_obj(int bank)
{
    int n = 0;

    while (ALLobject[bank][n] != NULL)
        n++;
    return n + 1;
}

// ──────────────────────────────────────────────────────────────────────
// The game: GS / DMA / VIF reset and the ga library's first packets, two cleared frames, the display
// (512 x 448 interlaced), the IOP, then forever: system_init, main2 (the title and the game's scene
// set-up), and the frame loop until a reset or a load -- pad, controllers, animation, triggers, the time
// stop, the demo skip, characters, effects, the camera, the draw pass DrawMode picks, VSync, movies,
// time -- and the scene's teardown with the heaps rebuilt.
// CT3 0x01839d10

int main(int argc, char **argv)
{
    static int skip_wait_temp;                  /* sbss 0x0193bca0, skip_wait_temp$61 */
    int timer;
    int i;
    short c;

    gArgv = argv;
    gArgc = argc;
    sceGsResetPath();
    sceDmaReset(1);
    sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME);
    gaDmaInit();
    gaVifInit();
    gaSetupGSInitData(&gsinit);
    InitThread();
    FlushCache(WRITEBACK_DCACHE);
    // The ROM's constant 0x01ad8000: bgt_ringbuffer_work's first 1 KB boundary.
    bgt_ringbuffer = (void *)(((uintptr_t)bgt_ringbuffer_work + 0x3ff) & ~(uintptr_t)0x3ff);
    gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
    gaBGT_ExecGSInitData_VIF1(&gsinit);
    gaBGT_End();
    ga_disp_fbuf_address = 0;
    ga_draw_fbuf_address = 0x38000;
    ga_zbuf_address = 0x70000;
    gaInitInterrupt();
    gaInitMeter();
    FirstInitialization();
    drawcom_frameclear.rgbaq = FRAMECLEAR_RGBAQ;
    for (i = 0; i < 2; i++) {
        FlushCache(WRITEBACK_DCACHE);
        gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
        gaBGT_ExecDBufferData_VIF1(&drawbuffer_frameclear);
        gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_frameclear);
        gaBGT_ExecDrawCommandData_VIF1(&drawcom_frameclear);
        gaBGT_End();
        gaInitDrawObject();
        gaWaitVSync();
    }
    // PMODE EN2; SMODE2 INT (frame mode); DISPFB FBW 8 (512); DISPLAY DX 636 DY 50 MAGH 4, DW 2559
    // DH 447 (512 x 448); BGCOLOR black.
    *(u_long *)&ga_disp.pmode = 2;
    *(u_long *)&ga_disp.smode2 = 1;
    *(u_long *)&ga_disp.dispfb = 0x1000;
    OMAKE_FLAG |= OMAKE_BOOTED;
    *(u_long *)&ga_disp.display = 0x001bf9ff0203227cull;
    *(u_long *)&ga_disp.bgcolor = 0;
    sceGsPutDispEnv(&ga_disp);
    IOPInit(argv);
    ArtoolDebugOpen();
    game_init();
    for (;;) {
        system_init();
        main2();
        main_rtn_id = 1;
        FlushCache(WRITEBACK_DCACHE);
        ahTimeInit();
        do {
            timer = ga_main_timer;
            FileSoundServer();
            asChangeViewMatrix_forTheBank(6);
            gaInit3dlib();

            // ---- the pad -----------------------------------------------------------------------
            if (SkipMain == 0 && DemoSkip == 0) {
                if (controller_mode == CONTROLLER_MODE_100) {
                    gaPadRead(pad, 1);
                } else {
                    gaPadRead(pad, 0);
                    if (CntDrawnOut != 0) {
                        pad[0].cnt = 0;
                        pad[0].trg = 0;
                        pad[0].key = 0;
                        if (strcmp("GAME.BIN", BankInfo[0].name) == 0 && (pause == 0 || (pause & PAUSE_RUNNING))) {
                            if (pad_trg_button[0] & PAD_BUTTON_0x100)
                                pad[0].trg = 0;
                            else
                                pad[0].trg = KEY_MAP[KEY_MAP_DRAWN_OUT];
                        }
                    }
                }
                pad_cnt_button[0] = pad[0].cnt;
                pad_trg_button[0] = pad[0].trg;
                pad_key_button[0] = pad[0].key;
                pad_cnt_button[1] = pad[1].cnt;
                pad_trg_button[1] = pad[1].trg;
                pad_key_button[1] = pad[1].key;
            } else {
                pad_cnt_button[0] = 0;
                pad_trg_button[0] = 0;
                pad_key_button[0] = 0;
                pad_cnt_button[1] = 0;
                pad_trg_button[1] = 0;
                pad_key_button[1] = 0;
            }
            ControllerMain();
            FlushCache(WRITEBACK_DCACHE);
            if (SkipMain == 0 && DemoSkip == 0) {
                gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                gaBGT_ExecDBufferData_VIF1(&drawbuffer_frameclear);
                gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_frameclear);
                gaBGT_ExecDrawCommandData_VIF1(&drawcom_frameclear);
                gaBGT_ExecDBufferData_VIF1(&drawbuffer_0);
                gaBGT_End();
            }
            gaVifcodeFlush_VIF1();
            while (gaVIFSync_VIF1() != 0)
                ;
            gaInitDrawObject();
            gaDmaSendN_toSPR(SPR_3DLIB_WORK, ga3dlibwork, SPR_3DLIB_WORK_QWC, 0);
            gaDmaSync_toSPR();
            gaInit3dlibWork(SPR_3DLIB_WORK);
            gaSetZRollMatrix(0);

            // ---- animation and shadows ---------------------------------------------------------
            asUpDateGAobjectsAnimation();
            for (i = 0; i < BANK_MAX; i++) {
                if (BankInfo[i].loaded != 0 && BankInfo[i].type == BANK_TYPE_SHADOW)
                    asShadowPolygonPositions(i);
                if (BankInfo[i].count > 0)
                    i += BankInfo[i].groupCount - 1;
            }
            for (c = 0; c < CHARACTER_MAX; c++) {
                if (Character[c].bank >= 0 && (Character[c].flags & CHARACTER_FLAG_SHADOW))
                    asShadowPolygonPositions(Character[c].bank);
            }
            asWarpObjectBody(1, 0);
            ProcessPortal();
            MemorycardUpdate();
            SelfDiscardSceneLoadUpdate();
            WarpObjectUpdate();
            for (i = 0; i < CHARACTER_MAX; i++) {
                if (DirectLightCharaCounter[i] != 0)
                    DirectLightCharaCounter[i]--;
            }
            Trigger_main();
            PanicUpdate();
            asPanicStatusCheck(2);

            // ---- the time stop -----------------------------------------------------------------
            if (time_stop_fream < 1) {
                if (time_stop_flag & TIME_STOP_PAUSE_OFF)
                    pause_reservation = 0;
                if ((time_stop_flag & TIME_STOP_PLAYER) && ssTrigCheckEvent(EV_HOLD_PLAYER, 1, -1) == 0)
                    PlayerControlUpdate(0);
                if (time_stop_flag & TIME_STOP_MONSTER) {
                    if (ssTrigCheckEvent(EV_HOLD_MONSTER, 1, -1) == 0)
                        MonsterAIUpdate(0);
                    if (ssTrigCheckEvent(EV_0xCC, 1, -1) == 0)
                        ssTrigRemoveEvent(EV_0xCA, 1, -1);
                    asMothPause(0);
                }
                if ((time_stop_flag & TIME_STOP_MENU) && ssTrigCheckEvent(EV_HOLD_MENU, 1, -1) == 0)
                    ssMenuUpdate(0);
                time_stop_flag = 0;
            } else {
                time_stop_fream--;
            }

            // ---- the demo skip -----------------------------------------------------------------
            if (ssTrigCheckEvent(EV_DEMO, 1, 0) != 0) {
                switch (DEMO_SKIP_FLAG) {
                case DEMO_SKIP_IDLE:
                    if (pad_trg_button[0] & KEY_MAP[KEY_MAP_SKIP]) {
                        pause_reservation = 1;
                        DEMO_SKIP_FLAG = DEMO_SKIP_FADE_START;
                    }
                    break;
                case DEMO_SKIP_REQUEST:
                    pause_reservation = 1;
                    DEMO_SKIP_FLAG = DEMO_SKIP_FADE_START;
                    break;
                case DEMO_SKIP_FADE_START:
                    DemoFilterChangeWork.timer = DEMO_SKIP_FADE_FRAMES;
                    pause_reservation = 1;
                    DemoFilterChangeWork.from[0] = 0xff;
                    DemoFilterChangeWork.active = 1;
                    DEMO_SKIP_FLAG = DEMO_SKIP_FADING;
                    DemoFilterChangeWork.mode = 1;
                    DemoFilterChangeWork.to[1] = 0;
                    DemoFilterChangeWork.to[2] = 0;
                    DemoFilterChangeWork.to[3] = 0;
                    DemoFilterChangeWork.to[0] = 0;
                    DemoFilterChangeWork.from[1] = 0;
                    DemoFilterChangeWork.from[2] = 0;
                    DemoFilterChangeWork.from[3] = 0;
                    tiVibrationOff(0);
                    break;
                case DEMO_SKIP_FADING:
                    pause_reservation = 1;
                    if (DemoFilterChangeWork.timer == 5)
                        ssTrigScreenBlurEnd();
                    if (DemoFilterChangeWork.timer == 0) {
                        skip_wait_temp = DEMO_SKIP_HOLD_FRAMES;
                        DEMO_SKIP_FLAG = DEMO_SKIP_HOLD;
                    }
                    break;
                case DEMO_SKIP_HOLD:
                    if (--skip_wait_temp == 0)
                        DEMO_SKIP_FLAG = DEMO_SKIP_RUN;
                    break;
                case DEMO_SKIP_RUN:
                    pause_reservation = 0;
                    DemoSkip = 1;
                    DEMO_SKIP_FLAG = DEMO_SKIP_END;
                    ssTrigRegisterEvent(EV_DEMO_SKIPPED, 1, 0);
                    break;
                }
            }
            if (ssTrigCheckEvent(EV_DEMO, 1, 0) == 0 && DEMO_SKIP_FLAG >= DEMO_SKIP_END) {
                if (DEMO_SKIP_FLAG == DEMO_SKIP_END_DONE) {
                    pause_reservation = 0;
                    DemoFilterChangeWork.active = 0;
                    DEMO_SKIP_FLAG = DEMO_SKIP_IDLE;
                } else if (DEMO_SKIP_FLAG == DEMO_SKIP_END_HOLD) {
                    if (--skip_wait_temp == 0)
                        DEMO_SKIP_FLAG = DEMO_SKIP_END_DONE;
                } else if (DEMO_SKIP_FLAG == DEMO_SKIP_END_FADING) {
                    pause_reservation = 1;
                    if (DemoFilterChangeWork.timer == 0) {
                        DEMO_SKIP_FLAG = DEMO_SKIP_END_HOLD;
                        skip_wait_temp = DEMO_SKIP_HOLD_FRAMES;
                    }
                } else if (DEMO_SKIP_FLAG == DEMO_SKIP_END) {
                    DemoFilterChangeWork.timer = DEMO_SKIP_FADE_FRAMES;
                    DemoSkip = 0;
                    pause_reservation = 1;
                    DemoFilterChangeWork.to[0] = 0xff;
                    DEMO_SKIP_FLAG = DEMO_SKIP_END_FADING;
                    DemoFilterChangeWork.active = 1;
                    DemoFilterChangeWork.mode = 1;
                    DemoFilterChangeWork.to[1] = 0;
                    DemoFilterChangeWork.to[2] = 0;
                    DemoFilterChangeWork.to[3] = 0;
                    DemoFilterChangeWork.from[1] = 0;
                    DemoFilterChangeWork.from[2] = 0;
                    DemoFilterChangeWork.from[3] = 0;
                    DemoFilterChangeWork.from[0] = 0;
                    ssTrigRemoveEvent(EV_DEMO_SKIPPED, 1, 0);
                }
            }

            // ---- characters --------------------------------------------------------------------
            asUpDateSceneAnimation();
            for (i = 0; i < CHARACTER_MAX; i++) {
                if (Character[i].id != -1) {
                    memcpy(Character[i].posBefore, Character[i].pos, sizeof Character[i].pos);
                    Character[i].rotBefore = Character[i].rot;
                    asCharacterForceMoveOperate(&Character[i]);
                }
            }
            memcpy(CameraPosBefore, CameraPos, sizeof CameraPos);
            asCharacterOperate(Character, 1, 0, 0);
            asCharacterOperate(Character, 0, 1, 1);
            asCharacterPositioning(Character, 0, 1);
            asMonsterDistanceStack_inPanic(1, 0, 0);
            ssAI_main();
            asCharacterOperate(Character, 1, 0, 1);
            asCharacterPositioning(Character, 1, 0);
            if (pause == 0 || (pause & PAUSE_RUNNING))
                ssProcessHit();
            asCharacterPositionReviewInPausing(Character);

            // ---- effects -----------------------------------------------------------------------
            tiCharaFootAdjust();
            tiWindUpdate();
            ssParticle_main();
            ssPartMotionBlurUpdate();
            ssChangeCharAmbientColor();
            ReflectMapUpdate();
            tiRefractionMapUpdate();
            tiBumpSTValueUpdate();
            tiHurikoHitUpdate();
            tiHurikoUpdate();
            tiParticleUpdate();
            FogAnimeUpdate();
            AmbientAlphaUpdate();
            tiPipeJointUpdate();
            if (pBossInfo != 0) {
                tiLightBelt2UpdateForBOSS_Battle();
                if (pBossInfo[0x91] != 0)
                    tiLightBelt2UpdateForALYSSA_Battle();
            }
            tiLightBelt2Update();
            asCopyDoubleBuffer();
            camera_view();
            asGetShadowMatrixGlobalView();
            CharacterSort();
            gaEndCpuMeter();

            // ---- the actress light: her position through her model's matrix (VU0 in the ROM) ---
            if (ACTRESS_LIGHT_BASE_MAX_ID[idCurrentScene] < 0) {
                ActressLightFlag[idCurrentScene] = 0;
            } else {
                const float *v = (const float *)(ActressMP + 0x20);
                const float *m = (const float *)(ActressMP + 0x1c0);
                alignas(16) float pos[4];
                int k;

                for (k = 0; k < 4; k++)
                    pos[k] = m[k] * v[0] + m[4 + k] * v[1] + m[8 + k] * v[2] + m[12 + k] * v[3];
                asWarpSceneObject(All_table_TBL[idCurrentScene], ACTRESS_LIGHT_BASE_MAX_ID[idCurrentScene], pos);
                ActressLightFlag[idCurrentScene] = 1;
            }

            // ---- the draw pass -----------------------------------------------------------------
            if (SkipMain == 0 && DemoSkip == 0) {
                gaBGT_Start(bgt_ringbuffer, BGT_RINGBUFFER_SIZE);
                drawcom_frameclear.rgbaq = FogClearColor(FogCtrlAddr[idCurrentScene]);
                switch (DrawMode) {
                case DRAW_MODE_NORMAL:
                    DrawAllObject_Normal();
                    tiDrawShadingOff();
                    XcmdScreenFilter();
                    GlobalDrawObject();
                    break;
                case DRAW_MODE_DOF:
                    ssDOF_DrawAllObject(CamGetNowDOFLevel());
                    tiDrawShadingOff();
                    XcmdScreenFilter();
                    GlobalDrawObject();
                    break;
                case DRAW_MODE_REFLECT_MAP:
                    CaptureReflectMap();
                    break;
                case DRAW_MODE_BLUR:
                    ssBlurDrawAllObject();
                    break;
                case DRAW_MODE_ULTIMA:
                    ssUltimaDrawAllObject();
                    tiDrawShadingOff();
                    XcmdScreenFilter();
                    GlobalDrawObject();
                    break;
                case DRAW_MODE_HAZY:
                    ssHazyDrawAllObject();
                    tiDrawShadingOff();
                    XcmdScreenFilter();
                    GlobalDrawObject();
                    break;
                case DRAW_MODE_WAVE:
                    ssWaveDrawAllObject();
                    tiDrawShadingOff();
                    XcmdScreenFilter();
                    GlobalDrawObject();
                    break;
                case DRAW_MODE_VASELINE:
                    ssVaselineDrawAllObject();
                    tiDrawShadingOff();
                    XcmdScreenFilter();
                    GlobalDrawObject();
                    break;
                case DRAW_MODE_HQDOF:
                    ssHQDOF_DrawAllObject();
                    tiDrawShadingOff();
                    XcmdScreenFilter();
                    GlobalDrawObject();
                    break;
                case DRAW_MODE_GLOBAL_ONLY:
                    drawcom_frameclear.rgbaq = FogClearColor(FogCtrlAddr[0]);
                    tiDrawShadingOff();
                    GlobalDrawObject();
                    break;
                }
                if (fFade)
                    tiFade_Draw();
                if (fXFade)
                    tiXFadeOut_Draw();
                if (fVibTimer)
                    tiVibrationTimer();
            } else {
                // Fast-forward: no drawing; the fades' clocks move with the skipped time.
                if (fShadeOff)
                    nowShadeOffFrame--;
                XcmdScreenFilterSkip();
                if (fFade)
                    FadeStartTicks += gGlobalNextTicksInterval;
                if (fXFade)
                    XfadeStartTicks += gGlobalNextTicksInterval;
                if (fVibTimer)
                    VibTimeTicks += gGlobalNextTicksInterval;
                tiVibrationOff(0);
            }
            pause = pause_reservation;
            asUpdateAnimation_Matrix();
            if (SkipMain == 0 && DemoSkip == 0)
                gaBGT_End();
            VistaVisionMain();
            gaEndGSMeter();

            // ---- VSync and time ----------------------------------------------------------------
            if (SkipMain == 0 && DemoSkip == 0) {
                while (timer == ga_main_timer)
                    ;
            }
            if (SkipMain == 0 && DemoSkip == 0) {
                gaWaitVSync();
                AA_MovieProc();
            } else {
                if (SkipMain != 0) {
                    gNextTicksInterval = 5;
                    gGlobalNextTicksInterval = 5;
                    SkipMain--;
                    ga_main_timer = ga_main_timer + 1;
                    gNextTicks = gTotalTicks + 10;
                    gGlobalNextTicks = gGlobalTotalTicks + 10;
                    gGlobalTotalTicks += 5;
                    gTotalTicks += 5;
                }
                if (DemoSkip != 0) {
                    gNextTicksInterval = 10;
                    gGlobalNextTicksInterval = 10;
                    ga_main_timer = ga_main_timer + 1;
                    gNextTicks = gTotalTicks + 20;
                    gGlobalNextTicks = gGlobalTotalTicks + 20;
                    gGlobalTotalTicks += 10;
                    gTotalTicks += 10;
                }
            }
            if (pause == 0 || (pause & PAUSE_RUNNING))
                ahTimeMaintenance(0);
            else
                ahTimeMaintenance(1);
            MyFrame++;
            if (SkipMain == 0 && DemoSkip == 0 && (char)ssTrigCheckEvent(EV_PLAY_TIME_STOP, 1, 0) == 0) {
                if (++progress_time > PROGRESS_TIME_MAX - 1)
                    progress_time = PROGRESS_TIME_MAX;
            }
            AA_UidCount = (AA_UidCount + AA_UID_STEP) & ~(u_int)(AA_UID_STEP - 1);
            if (AA_UidCount == 0)
                AA_UidCount = AA_UID_STEP;
        } while (fReset == 0 && load_continue == 0);

        // ---- the scene's teardown: banks freed, the heaps rebuilt ------------------------------
        FSMltLoadEndWaitAll();
        main_rtn_id = 0;
        CharacterAllFree();
        for (i = 0; i < BANK_MAX; i++)
            BankInfo[i].loaded = 0;
        SceneEndBank(0, BANK_MAX);
        system_end();
        CheckAllocNum0(&gHeapJunc);
        SunAllocInit(&gHeapJunc, gJuncAllocArea, sizeof gJuncAllocArea, HEAP_NAME_JUNC);
        CheckAllocNum0(&gHeapSound);
        SunAllocInit(&gHeapSound, gSoundAllocArea, sizeof gSoundAllocArea, HEAP_NAME_SOUND);
        CheckAllocNum0(&gHeapGlobalScene);
        SunAllocInit(&gHeapGlobalScene, gGlobalBankArea, 0x300000, HEAP_NAME_GLOBAL);
    }
}
