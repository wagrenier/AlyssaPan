// game/unassigned_data.h -- CT3 globals that game/main.c's slice uses and whose owning TU is not written
// yet (2026-11-09). Names, addresses and sizes are the symtab's; types are what the code reads. Each moves
// to its owner's header when that TU is written; lib/unassigned_data.cpp defines them until then.
// Pointers the ROM keeps as EE addresses are host pointers here (no EE memory behind them).
#pragma once

#include "sdk/scetypes.h"
#include "sdk/sce_gs.h"
#include "ga/gaInitDrawObject.h"
#include "game/ahalloc.h"
#include "game/ct3_types.h"
#include "ga/pad_button_read.h"

// ---- the ga library's frame (gaInit3dlib.c, gaInitDrawObject.c, gaInitInterrupt.c) ------------------
extern void *bgt_ringbuffer;                    /* sbss 0x01941808: 1 KB-aligned start in bgt_ringbuffer_work */
extern u_char bgt_ringbuffer_work[0x10000];     /* bss  0x01ad7c20 */
extern int ga_disp_fbuf_address;                /* sbss 0x0193e680 */
extern int ga_draw_fbuf_address;                /* sdata 0x019398c8 */
extern int ga_zbuf_address;                     /* sdata 0x019398cc */
extern sceGsDispEnv ga_disp;                    /* sbss 0x0193eec0 */
extern GA_GSINIT_DATA gsinit;                   /* sbss 0x01941e40 */
extern GA_DBUFFER_DATA drawbuffer_frameclear;   /* sbss 0x01941d40 */
extern GA_DBUFFER_DATA drawbuffer_0;            /* sbss 0x01941dc0 */
extern GA_DRAWCTRL_DATA drawctrl_frameclear;    /* sbss 0x01941c80 */
extern GA_DRAWCOMMAND_DATA drawcom_frameclear;  /* sdata 0x01939cc0 */
extern PAD_DATA pad[2];                         /* sbss 0x01941eb0: gaPadRead's two controllers */
extern u_char ga3dlibwork[0x400];               /* sbss 0x0193eac0: copied to the scratchpad each frame */
extern GA_FOGCTRL_DATA fogctrl[BANK_MAX];      /* bss  0x01ae8880: each bank's fog packet */
extern float screen_0[12];                     /* sdata 0x01939d10: gaSet3dlibStruct's screen (512 x 448, ...) */
extern float matrix_global_view[4][4];         /* sbss 0x01941840 */
extern float mrr_ReverseGlobalView[4][4];      /* bss  0x0193ca40: CharLightSet's gaSetLight view */
extern float mrr_ReverseMatrix[4][4];          /* bss  0x0193ca80 */
extern int FOG_mode;                           /* sbss 0x019405ec: set by main_rtn when the bank's fog part is a fog volume */
extern int drawobj_attr;                       /* sbss 0x0193f788: gaBGT_DrawObject's attr (main_rtn: 0) */
extern u_char **ParticleObject[BANK_MAX];      /* sbss 0x01940b70: per particle list entry, its slots' draw objects */

// ---- heaps (ahalloc.c) -------------------------------------------------------------------------------
extern SUN_HEAP gHeapJunc;                      /* sdata 0x0193bd30 */
extern SUN_HEAP gHeapSound;                     /* sdata 0x0193bcb0 */
extern SUN_HEAP gHeapGlobalScene;               /* sdata 0x0193bcf0 */
extern u_char gJuncAllocArea[0x96000];          /* bss  0x01bce3c0 */
extern u_char gSoundAllocArea[0xe1000];         /* bss  0x01aed3c0 */
extern u_char *gGlobalBankArea;                 /* sdata 0x01939c54: EE 0x00400000, 3 MB below the ELF */

// ---- game state --------------------------------------------------------------------------------------
extern u_int OMAKE_FLAG;                        /* sbss 0x0193f9f0: OMAKE_* (the extras unlocked) */
extern int pause;                               /* sbss 0x01941804: PAUSE_* */
extern int pause_reservation;                   /* sbss 0x01941800: next frame's pause */
extern void *SkinWork;                          /* sbss 0x01940200: asUpDateGAobjectsAnimation's scratch (gHeapJunc) */
extern int time_stop_fream;                     /* sbss 0x019417f8 (sic) */
extern int time_stop_flag;                      /* sbss 0x019417fc: TIME_STOP_* */
extern int fReset;                              /* sbss 0x0193f9d0 */
extern int load_continue;                       /* sbss 0x0193fa10 */
extern int progress_time;                       /* sbss 0x0193fa1c: play time in frames */
extern u_int AA_UidCount;                       /* sbss 0x0193fa0c */
extern u_int tnRandSeed;                        /* sbss 0x01942390 */
extern int PowerOn;                             /* sbss 0x0193fa08 */
extern int mcMain_ID;                           /* sbss 0x0193fa04 */
extern int mcSlot_ID;                           /* sbss 0x0193fa00 */
extern int mcFile_ID;                           /* sbss 0x0193f9fc */
extern int mcErr_ID;                            /* sbss 0x0193f9f8 */
extern int mcAccess_ID;                         /* sbss 0x0193f9f4 */
extern char SkipEventFlag[20];                  /* sbss 0x01941170 */
extern u_char SOUND_STEREO_MONAURAL_BUFF;       /* sbss 0x0193f9ec */
extern u_char BGM_MASTER_VOLUME_BUFF;           /* sbss 0x0193f9e8 */
extern u_char SE_MASTER_VOLUME_BUFF;            /* sbss 0x0193f9e4 */
extern u_char LOAD_SLOT_NUM;                    /* sbss 0x0193f9e0 */
extern u_char TITLE_SELECT_NUM;                 /* sbss 0x0193f9d8 */
extern u_char TITLE_CANCEL_FLAG;                /* sbss 0x0193f9d4 */
extern u_char fVibration;                       /* data 0x01939c80: the pad vibrates (option) */
extern u_short KEY_MAP[105];                    /* sdata 0x0193b9a0: button bits by action */
extern char Backup_Filename[255];               /* sbss 0x019404e0 */
extern u_char MoviePlayCheck;                   /* sbss 0x0194238c */
extern u_char MovieEndCheck;                    /* sbss 0x01942388 */
extern int ControlDisableflag;                  /* sbss 0x01944830 */
extern int TexFileSysInitStop;                  /* sbss 0x01941694 */
extern int Player_BankID;                       /* sbss 0x01941698: the player character's Character[] index */
extern int anim_play_slow;                      /* sdata 0x01937600: 100 */

// ---- scenes and banks --------------------------------------------------------------------------------
extern BANK_INFO BankInfo[BANK_MAX];            /* bss  0x01aea360 */
extern int idCurrentScene;                      /* sbss 0x01940e34 */
extern int ACTRESS_LIGHT_BASE_MAX_ID[BANK_MAX]; /* sbss 0x01941f00: < 0 = the scene has no actress light */
extern int Omni1_num[BANK_MAX];                 /* sbss 0x01942060 */
extern int Omni0_num[BANK_MAX];                 /* sbss 0x019421c0 */
extern char ActressLightFlag[BANK_MAX];         /* sbss 0x01942320 */
extern u_char *ActressMP;                       /* sbss 0x01942380: the actress's model (matrix at +0x1c0) */
extern ALL_TABLE *All_table_TBL[BANK_MAX];      /* sbss 0x019416a0 */
extern u_int *ALLobject[BANK_MAX];              /* sbss 0x01940cd0: the list main_rtn draws next (EE words) */
extern u_char *pModelPartsHeader[BANK_MAX];     /* sbss 0x01940380 */
extern u_char osTotalObjTable[3][BANK_MAX][OS_OBJ_TABLE_BYTES]; /* bss 0x01a59870: [1] = objSort's */
extern u_char *FogCtrlAddr[BANK_MAX];           /* sbss 0x019405f0: fog colour bytes at +4, +8, +0xc */
extern void *pBehaviorHeader[BANK_MAX];         /* sbss 0x01940208: ALL_TABLE.behavior (scene_init) */
extern int BaseobjCount[BANK_MAX];              /* sbss 0x01940750: ALL_TABLE.baseobjCount (scene_init) */
extern FOG_ANIME FogAnimeBuff[BANK_MAX];        /* bss  0x01a58040 */

// ---- characters --------------------------------------------------------------------------------------
extern CHARACTER Character[CHARACTER_MAX];      /* bss  0x01a4bfc0 */
extern CAMERA Cam[CAMERA_MAX];                  /* bss  0x01ae7c40 */
extern u_char EffectBuffer_Use;                 /* sbss 0x0193f764: the effect frame buffer is taken */
extern u_char HQDOFCameraOffset;                /* sdata 0x01939c84: tenths: the HQ DOF eye jitter */
extern u_char HQDOFOpacity;                     /* sdata 0x01939c88: percent: each HQ DOF sample's weight */
// PORT: one more than the ROM's 35 shorts: CharacterSort writes its -1 at [count], up to [35].
extern short CharacterSortBuff[CHARACTER_MAX + 1]; /* sbss 0x01940170 (70 bytes): Character indices, -1 ends */
extern PART_BLUR_OBJ *ALLpartBlurObj[PART_BLUR_MODES][BANK_MAX][PART_BLUR_MAX]; /* data 0x01a43800 */
extern u_char NumPartBlurObj[PART_BLUR_MODES][BANK_MAX]; /* data 0x0193fa80: ALLpartBlurObj's slots in use */
// PORT: one more than the ROM's 48: objSortWithBlurIdx ends the list at [NumPartBlurObj], up to [48].
extern PART_BLUR_OBJ *aftSortALLpartBlurObj[PART_BLUR_MAX + 1]; /* data 0x0193fb30: matched, in sorted order */
extern int DirectLightCharaCounter[CHARACTER_MAX]; /* sbss 0x019400e0 */
extern short AI_flag[CHARACTER_MAX];            /* sbss 0x0193fca0 */
// Which side a character slot is on (ssHitCheckCharacterAttack picks its targets by it; only the low
// byte is read).
#define CHARA_ATTR_PLAYER 0             /* strikes ssAtkCharaList2 (the stalkers) */
#define CHARA_ATTR_STALKER 1            /* strikes ssAtkCharaList1 (the player) */
#define CHARA_ATTR_NONE 0xff            /* strikes nobody */
extern int charaAttr[CHARACTER_MAX];            /* data 0x01939d90 */
#define ATK_CHARA_LIST1_COUNT 1
extern short ssAtkCharaList1[ATK_CHARA_LIST1_COUNT]; /* data 0x01939d40: the player (slot 0) */
#define ATK_CHARA_LIST2_COUNT 9
extern short ssAtkCharaList2[ATK_CHARA_LIST2_COUNT]; /* data 0x01939d50: the stalker characters (MonsterAIUpdate) */
extern u_short CharDirectLightSet[CHARACTER_MAX][6]; /* sbss 0x0193ff30 */
extern u_short CharOmniEffect[CHARACTER_MAX][8];     /* sbss 0x0193fd00 */
extern u_short AllOmniEffect[8];                /* sbss 0x0193fcf0 */
extern int CharStopFlag;                        /* sbss 0x019401c8 */
extern int TrgCharStopFlag;                     /* sbss 0x019401c4: TRG_CHAR_STOP_* */
extern int PlayerPanicMode;                     /* sbss 0x019401c0 */
extern int MonsterHpDispCounter;                /* sbss 0x019401bc */
extern int MonsterHpDispCharID;                 /* sbss 0x019401b8 */
extern u_char *pBossInfo;                       /* sbss 0x0193f78c: byte +0x91 = Alyssa's own light belt */
extern int MC_NOW_LOADING;                      /* sbss 0x0193f9dc: != 0 while the memory card loads (SSBGMProc pauses the chase BGM rule) */
extern char sFirstCheck;                        /* sdata 0x01939c8c */
extern int save_count;                          /* sbss 0x0193fa20 */
extern int msg_num;                             /* sbss 0x0193fa14 */

// ---- camera, clipping, drawing -----------------------------------------------------------------------
extern float CameraPosBefore[4];                /* sbss 0x01941810 */
extern float CameraPos[4];                      /* sbss 0x01941820 */
extern float Clipping_X1_pos;                   /* sbss 0x01940ea8 */
extern float Clipping_Y1_pos;                   /* sbss 0x01940ea0 */
extern float Clipping_X2_pos;                   /* sbss 0x01940ea4 */
extern float Clipping_Y2_pos;                   /* sbss 0x01940e9c */
extern CHAR_AMB_CHANGE CharAmbChangeFlag[CHAR_AMB_CHANGE_MAX]; /* sbss 0x0193fc00 */
extern int CharAmbChangeColor[CHAR_AMB_CHANGE_MAX]; /* sbss 0x0193fbf0 */
extern int VistaVisionNumber;                   /* sbss 0x0193fc90 */
extern int VistaVisionMode;                     /* sbss 0x0193fc94 */
extern int SS_fb_write_msb;                     /* sbss 0x019405e8 */
extern u_char SS_ZDrawMode;                     /* sbss 0x019405e4 */
extern u_char SS_ForceAlphaMode;                /* sbss 0x019405e0 */
extern int DrawMode;                            /* sbss 0x0194296c: DRAW_MODE_* */
extern int prevDrawMode;                        /* sbss 0x01942968 */
extern int ScreenHazyLevel;                     /* sbss 0x01942960 */
extern int HazeTransTime;                       /* sbss 0x0194295c */
extern int ScreenBlurLevel;                     /* sbss 0x01942964 */
extern FILTER_CHANGE_WORK FilterChangeWork;     /* sbss 0x0193fc60 */
extern char fFade;                              /* sbss 0x0193f74c */
extern char fXFade;                             /* sbss 0x0193f748 */
extern char fVibTimer;                          /* sbss 0x0193f768 */
extern char fShadeOff;                          /* sbss 0x0193f750 */
extern int fKeepScreen;                         /* sbss 0x0193f754 */
extern int Clipping_System;                     /* sbss 0x01940eac */
extern int nowShadeOffFrame;                    /* sbss 0x01942944 */
extern int ShadeOffFrame;                       /* sbss 0x01942940: the shade-off's length in frames */
extern char ShadeOff_CapLevel;                  /* sbss 0x0193f758: SHADE_OFF_LEVELS - the level argument */
extern float ShadeOff_SlidePixel;               /* sbss 0x0193f75c */
extern u_char ShadeOff_Level;                   /* sbss 0x0193f760: the level argument x 4 */
extern u_char XFade_Type;                       /* sbss 0x01942958: the cross-fade's kind (XFADE_TYPE_*) */
extern int FadeEndTicks;                        /* sbss 0x01942948: a fade is over once FadeStartTicks reaches it */
extern int FadeStartTicks;                      /* sbss 0x0194294c */
extern int XfadeEndTicks;                       /* sbss 0x01942950: the cross-fade is over once XfadeStartTicks reaches it */
extern int XfadeStartTicks;                     /* sbss 0x01942954 */
extern int VibTimeTicks;                        /* sbss 0x0193f76c */

// ---- items (tiItemInit) ------------------------------------------------------------------------------
extern int ItemList[12];                        /* sbss 0x0193f9a0 */
extern u_char ItemData[100][0x18];              /* data 0x0192e810: the item table (byte +1 = held) */
extern u_char KeyItemData[100][0x18];           /* data 0x0192f170: the key-item table, ItemData's shape */
extern int KeyItemList[24];                     /* sbss 0x0193f940 */
extern int LetterList[5][10];                   /* sbss 0x0193f870 */
extern int ArrowData[3];                        /* sbss 0x0193f858 */
extern int ArrowList[50];                       /* sbss 0x0193f790 */

// The save image's state (Character_Init.c's data_save_set / data_load_set, 2026-11-10).
#define GLOBAL_VARIABLE_COUNT 0x100
#define GLOBAL_EVENT_FLAG_BYTES 0x101         /* event e is bit e & 7 of byte e >> 3 (McRegisterEvent) */
extern int idNextScene;                         /* sbss 0x01940e38 */
extern int backup_progress_time;                /* sbss 0x0193fa18: the play time ContinueBackupGet restores */
extern int idCameraScene;                       /* sbss 0x01940e30: the bank whose camera is live */
extern u_long haCameraRoom;                     /* sbss 0x01940e28: ahGetHash of that bank's 4-letter room name
                                                   (ssTrigExecuteActCmd's camera command) */
extern int TransparentBodyTime;                 /* sbss 0x01940e48: data_load_set: 360 */
extern int KekkaiOn;                            /* sbss 0x0193f9a4 (no symbol; game-inferred): cleared when
                                                   Kekkai_Check spends the last barrier charm */
extern PANIC_INFO PanicInfoData;                /* sbss 0x01940e50 */
extern int vsBossMuteki;                        /* sbss 0x0194485c: Alyssa's invulnerable ticks in a boss fight
                                                   (asCheckCharacterHited counts it down) */
extern int AfterPanicStartMuteki;               /* sbss 0x01944860: her invulnerable ticks after a panic starts */
extern int GlobalCameraMode;                    /* sbss 0x0193cf08: CAMERA_MODE_* (wideXcmd_DebugPrint.h) */
extern int bVerticalMove;                       /* sbss 0x0194180c: camera_main writes its mode 0 / 1 (no reader seen) */
extern u_char act_small;                        /* sbss 0x0193f740: the pad's small motor is running (vibration TU) */
extern u_char act_large;                        /* sbss 0x0193f744: its large motor */
extern int GlobalVariable[GLOBAL_VARIABLE_COUNT]; /* sbss 0x01941190 */
extern u_char GlobalEventFlag[GLOBAL_EVENT_FLAG_BYTES]; /* sbss 0x01941590 */
extern qword DrawOnPosSetVectorTemp;            /* sbss 0x01942540: TrgSceneDraw(.., -2, ..) places the bank here */
extern float DrawOnPosSetMatrixTemp[4][4];      /* sbss 0x01942500: asJointParts_byMatrix's matrix (main2.c) */
extern u_long DrawOnPosSetHashTemp;             /* sbss 0x019424e0: != 0 -> TrgSceneDraw(.., -2, ..) joints by matrix */
extern int CharacterDisable;                    /* sbss 0x01942384: main2 clears it */
extern float matUnit[4][4];                     /* sdata 0x01939e40: the unit matrix */
#define MATUNIT_EE 0x01939e40u           /* matUnit's EE address (a root part's REC_PARENT) */
extern float vecOne[4];                         /* sdata 0x01939e80: (1, 1, 1, 1) */
extern u_char LetterData[100][0xc];             /* data 0x0192fad0: the letter table */

// ---- sound (game/sound.c's slice 1 uses them; the Sd driver TU or the scene TU owns them) -----------
extern u_int gBfoScene;                         /* sdata 0x0193a28c: a 4-char scene code, "Bxxx" at boot */
extern u_int gNowScene;                         /* sdata 0x0193a290: a 4-char scene code, "Nxxx" at boot */
extern int gStereo;                             /* sdata 0x0193a4dc: 1 = stereo (the menu's buffer 0) */
extern int gMasterVolSE;                        /* sdata 0x0193a4e0: 0..255 */
extern int gMasterVolSQ;                        /* sdata 0x0193a4e4: 0..255 */
extern int gMasterVolADS;                       /* sdata 0x0193a4e8: 0..255 */
// gPlaySESp1, gSQLd, gSdSE, gSELd: declared in game/SdMasterVolumeSetSE.h, defined in game/SdMasterVolumeSetSE.c (their owner).
