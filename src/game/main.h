// game/main.h -- CT3 game/main.c (0x01834440..0x0183aed8): the boot, the main loop and its draw passes.
// main.c's globals are the .sbss run 0x01942ab0..0x01942b48 (pad_key_button .. gArgc): contiguous, and
// every one is read or written by main.c's functions (a guess from placement: architecture.md).
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

// ---- main.c's globals ---------------------------------------------------------------------------------
extern u_int pad_key_button[4];                 /* sbss 0x01942ab0: [0] pad 1, [1] pad 2 */
extern u_int pad_trg_button[4];                 /* sbss 0x01942ac0 */
extern u_int pad_cnt_button[4];                 /* sbss 0x01942ad0 */
extern u_char Demo_Skip_Set;                    /* sbss 0x01942ae0 */
extern int DEMO_SKIP_FLAG;                      /* sbss 0x01942ae4: DEMO_SKIP_* */
extern FILTER_CHANGE_WORK DemoFilterChangeWork; /* sbss 0x01942af0 */
extern int DemoSkip;                            /* sbss 0x01942b18 */
extern int DEMO_SKIP_SET;                       /* sbss 0x01942b1c */
extern int SkipMain;                            /* sbss 0x01942b20: frames main runs without drawing */
extern int MyFrame;                             /* sbss 0x01942b24 */
extern int controller_type;                     /* sbss 0x01942b28 */
extern int controller_num2;                     /* sbss 0x01942b2c */
extern int controller_num;                      /* sbss 0x01942b30 */
extern int controller_mode;                     /* sbss 0x01942b34: CONTROLLER_MODE_* */
extern int main_rtn_id;                         /* sbss 0x01942b38 */
extern int CntDrawnOut;                         /* sbss 0x01942b3c */
extern char **gArgv;                            /* sbss 0x01942b40 */
extern int gArgc;                               /* sbss 0x01942b44 */

// ---- modes and flags main.c names ---------------------------------------------------------------------
enum DRAW_MODE {                                /* DrawMode: main's draw pass this frame */
    DRAW_MODE_NORMAL = 0,
    DRAW_MODE_DOF = 1,
    DRAW_MODE_REFLECT_MAP = 2,
    DRAW_MODE_BLUR = 3,
    DRAW_MODE_ULTIMA = 4,
    DRAW_MODE_HAZY = 5,
    DRAW_MODE_WAVE = 6,
    DRAW_MODE_VASELINE = 7,
    DRAW_MODE_HQDOF = 8,
    DRAW_MODE_GLOBAL_ONLY = 9,
};

enum CONTROLLER_MODE {                          /* controller_mode */
    CONTROLLER_MODE_NORMAL = 0,
    CONTROLLER_MODE_PANIC = 2,
    CONTROLLER_MODE_100 = 100,                  /* main reads the pad with gaPadRead mode 1 (unread) */
};

#define PAUSE_RUNNING 0x2                       /* pause bit: paused, but the world still runs */

enum TIME_STOP_FLAG {                           /* time_stop_flag: what runs during a time stop */
    TIME_STOP_PAUSE_OFF = 0x1,
    TIME_STOP_PLAYER = 0x2,
    TIME_STOP_MONSTER = 0x4,
    TIME_STOP_MENU = 0x8,
};

enum TRG_CHAR_STOP {                            /* TrgCharStopFlag */
    TRG_CHAR_STOP_KEYS = 0x1,
    TRG_CHAR_STOP_CHAR = 0x8,
};

enum OMAKE {                                    /* OMAKE_FLAG */
    OMAKE_1 = 0x1,                              /* event 0xa7 registered at system_init */
    OMAKE_2 = 0x2,                              /* event 0xc5 */
    OMAKE_BOOTED = 0x4,                         /* set once by main after the display is up */
};

#define PROGRESS_TIME_MAX 0xa4cb62              /* 10,800,226 frames: the play-time clock's cap */
#define AA_UID_STEP 0x10

// ---- functions ----------------------------------------------------------------------------------------
int main(int argc, char **argv);
void tiItemInit(void);                                                                  // CT3 0x01835420
void FirstInitialization(void);                                                         // CT3 0x01835660
void game_init(void);                                                                   // CT3 0x01835800
void ControllerMain(void);                                                              // CT3 0x018359f0
void ssVaselineDrawAllObject(void);                                                     // CT3 0x01835ad0
void ssWaveDrawAllObject(void);                                                         // CT3 0x01835b10
void ssHazyDrawAllObject(void);                                                         // CT3 0x01835b50
void ssUltimaDrawAllObject(void);                                                       // CT3 0x01835b90
void ssBlurDrawAllObject(void);                                                         // CT3 0x01835bf0
void ssDOF_DrawAllObject(int level);                                                    // CT3 0x01835c40
void DrawAllObject_Normal(void);                                                        // CT3 0x01835d00
void ssHQDOF_DrawAllObject(void);                                                       // CT3 0x01835d40
void CaptureReflectMap(void);                                                           // CT3 0x018362a0
void AllDrawObject(int mode);                                                           // CT3 0x018362b0
void DrawObjectRefraction(int mode);                                                    // CT3 0x01836f30
void GlobalDrawObject(void);                                                            // CT3 0x01837210
void main_rtn_DOF(int bank, int mode, int chara, int pass, int charaIndex, void **lastTex); // CT3 0x01837d90
void main_rtn_Particle2(int bank, int fog, int after, void **lastTex);                  // CT3 0x01837570
void main_rtn_Particle(int bank, int fog, void **lastTex);                              // CT3 0x018379d0
void main_rtn(int bank, int mode, int chara, u_int flags, int charaIndex, void **lastTex); // CT3 0x01838600
void main_rtn_GHOST_DOF(int bank, int mode, int chara, int pass, int charaIndex, void **lastTex); // CT3 0x01834440
void main_rtn_GHOST(int bank, int mode, int chara, u_int flags, int charaIndex, void **lastTex); // CT3 0x01834c40
void scene_end(int scene);                                                              // CT3 0x01838f20
void scene_draw_init(int scene);                                                        // CT3 0x01838fa0
void system_end(void);                                                                  // CT3 0x01839670
void system_init(void);                                                                 // CT3 0x01839720
int get_draw_obj(int bank);                                                             // CT3 0x01839cd0
