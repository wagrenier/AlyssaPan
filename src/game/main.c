// game/main.c -- CT3 main's object (0x01834440..0x0183aed8): the boot (main, FirstInitialization,
// game_init, system_init), the main loop, and the draw passes it picks by DrawMode. Built with
// main=gameMain (CMakeLists.txt): the process entry is the host's (src/main.cpp).
// Written 2026-11-09 (slice 1: the boot, the loop, the small draw passes); main_rtn*, AllDrawObject,
// DrawObjectRefraction, GlobalDrawObject, ssHQDOF_DrawAllObject, scene_init / scene_draw_init / system_end
// and get_draw_obj are the TU's later slices.
#include "game/main.h"
#include "common/alyssapan_stub.h"

#include <string.h>

#include "sdk/eekernel.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"
#include "game/unassigned_data.h"
#include "ga/gaDma.h"
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
#define SPR_3DLIB_WORK ((void *)0x70000400)    /* ga3dlibwork's copy in the scratchpad */
#define SPR_3DLIB_WORK_QWC 0x40

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

#define KEY_MAP_SKIP 11                         /* KEY_MAP[11] (+0x16): skip a demo */
#define KEY_MAP_DRAWN_OUT 23                    /* KEY_MAP[23] (+0x2e): the trigger forced while CntDrawnOut */
#define PAD_BUTTON_0x100 0x100

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
    gaSetupDrawCtrlData(&drawctrl_frameclear, 0, 0, 0, 1, 0, 0, 0);
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

// ──────────────────────────────────────────────────────────────────────
// DRAW_MODE_REFLECT_MAP: empty in the retail build.
// CT3 0x018362a0

void CaptureReflectMap(void)
{
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
        gaWaitVSync(0);
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
                gaWaitVSync(0);
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018362b0
void AllDrawObject(int mode)
{
    ALYSSAPAN_STUB("AllDrawObject", 0x018362b0);
}

// stub of CT3 0x01836f30
void DrawObjectRefraction(int mode)
{
    ALYSSAPAN_STUB("DrawObjectRefraction", 0x01836f30);
}

// stub of CT3 0x01837210
void GlobalDrawObject(void)
{
    ALYSSAPAN_STUB("GlobalDrawObject", 0x01837210);
}

// stub of CT3 0x01835d40
void ssHQDOF_DrawAllObject(void)
{
    ALYSSAPAN_STUB("ssHQDOF_DrawAllObject", 0x01835d40);
}

// stub of CT3 0x01839670
void system_end(void)
{
    ALYSSAPAN_STUB("system_end", 0x01839670);
}
