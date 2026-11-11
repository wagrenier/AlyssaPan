// game/asEyeForcusUP2CamMat.h -- CT3 game/asEyeForcusUP2CamMat.c: the camera (cuts, view matrices, the
// camera's globals). Prototypes first came from the call sites (game/main.c, 2026-11-09); slice 1
// (2026-11-10) wrote the functions marked "written" from the ROM.
#pragma once

#include "sdk/scetypes.h"
#include "ga/pad_button_read.h"

// ---- the camera's globals (symtab names and addresses; defined in asEyeForcusUP2CamMat.c until the
// object that owns them is known: none had a definition elsewhere, 2026-11-10) ---------------------------
extern float AlissaEyeHeight;               /* sdata 0x019386e0: 135 (camera_init resets it) */
extern float CameraBehind;                  /* sdata 0x019386e4: 250 */
extern int iLastCameraBank;                 /* sdata 0x019387c8: asChangeViewMatrix_forTheBank's last bank */
extern char TheFirstChangeCamera;           /* sbss 0x0193cf0c: set by the first scene-bank cut (SkipMain += 2) */
extern int iSm;                             /* sbss 0x0193cefc */
extern char szCamModeAdd[64];               /* sbss 0x0193d010: "" at init */
extern float NDAlyssaPos[4];                /* sbss 0x0193d090 */
extern int iCameraMode;                     /* sbss 0x0193d0a0: CAMERA_MODE_* (camera_view) */
extern int CameraAngle[4];                  /* sbss 0x0193d0c0 (static): [0] pitch, [1] yaw, 16-bit angle
                                               units (camera_view reads them with lw: ints) */
extern int iNeoCameraLock;                  /* sbss 0x0193cf00: != 0 -> CAMERA_MODE_SCENE also runs the ND camera */
extern char CameraError;                    /* sbss 0x0193cf04: != 0 -> camera_view sets matUnit as the view */
extern float NDAlissaEyeHeight;             /* sdata 0x01938764: 135 */
extern float NDCameraBehind;                /* sdata 0x01938768: 250 */
extern int NDCameraAngle[4];                /* sbss 0x0193d070: the ND camera's CameraAngle */
extern float CameGri_dvsZ;                  /* sbss 0x01941898: the stick's eased pitch deflection (-1..1) */
extern float CameGri_dvsX;                  /* sbss 0x0194189c: ... and yaw deflection */
extern u_int CameGri_DU;                    /* sbss 0x019418a0: yaw angle at full deflection (16-bit units) */
extern u_int CameGri_DR;                    /* sbss 0x019418a4: pitch angle at full deflection */

// iCameraMode (camera_view's cases; names from what each builds).
#define CAMERA_MODE_FOLLOW 0                /* Cam[6] behind Character[0] (CameraAngle, CameraBehind) */
#define CAMERA_MODE_FOLLOW1 1               /* the same */
#define CAMERA_MODE_ND 2                    /* Cam[6] behind NDAlyssaPos (the ND* set) */
#define CAMERA_MODE_SCENE 4                 /* a scene bank without a cut gets cut 0 */

// The look-around stick (camera_view, CameGri_Mode 0 / 1: pad[0]'s stick bytes 2, 3 or 0, 1).
#define CAMERA_STICK_CENTER 0x80
#define KEY_MAP_STICK_DEAD_BYTE 0x44        /* the s_char at KEY_MAP + 0x44 (KEY_MAP[34]'s low byte): the dead zone */

// One camera shake of CamQuake[3] (0x0193cf10, 0x14 bytes each; camera_view runs them on scene banks'
// cameras along the view's right / up / forward axis; names from that use).
typedef struct CAM_QUAKE {
    float amp;                              /* 0x00: the shake's amplitude */
    u_short period;                         /* 0x04: ticks per sine period */
    u_short fadeIn;                         /* 0x06: ticks to full amplitude */
    u_short h08;
    u_short fadeOut;                        /* 0x0a: ticks back to 0 at the end */
    u_short duration;                       /* 0x0c: 0 = off */
    u_short time;                           /* 0x0e: ticks run (gGlobalNextTicksInterval a frame) */
    u_short phase;                          /* 0x10: added to the sine's angle */
    u_short h12;
} CAM_QUAKE;
static_assert(sizeof(CAM_QUAKE) == 0x14, "CAM_QUAKE");
static_assert(offsetof(CAM_QUAKE, duration) == 0x0c, "CAM_QUAKE.duration");
static_assert(offsetof(CAM_QUAKE, phase) == 0x10, "CAM_QUAKE.phase");
#define CAM_QUAKE_AXES 3
extern CAM_QUAKE CamQuake[CAM_QUAKE_AXES];  /* sbss 0x0193cf10 */

// CamChange's request (camera_view builds one on the stack: {0, idCurrentScene, 0, 1}).
typedef struct CAM_CHANGE {
    int arg;                                /* 0x0: CAMERA.moveTicks */
    int bank;                               /* 0x4 */
    short cut;                              /* 0x8 */
    u_char mode;                            /* 0xa: CAMERA.changeMode */
    u_char pad;
} CAM_CHANGE;
static_assert(sizeof(CAM_CHANGE) == 0xc, "CAM_CHANGE");
static_assert(offsetof(CAM_CHANGE, mode) == 0xa, "CAM_CHANGE.mode");

// CamStock: a cut by part hash asked for while its bank is not drawn yet (CamChange_Hash_S_Hash stocks
// it, camera_view retries it each frame).
#define CAMSTOCK_HASH 0                     /* [0..1]: the u_long hash */
#define CAMSTOCK_BANK 2                     /* -1 = nothing stocked (init, and every cut change) */
#define CAMSTOCK_ARG 3
#define CAMSTOCK_MODE 4                     /* read as a short */
extern float FOV_deg;                       /* sbss 0x01941830: the field of view in degrees (40 at init) */
extern float ZelCam[6];                     /* sbss 0x01941880: reset for a scene bank by camera_init0
                                               (400, 200, 180, 30, 0, 0) */
extern u_char CameGri_Mode;                 /* sbss 0x019418a8: 0xff at init */
extern int CamStock[6];                     /* sbss 0x019418b0: [2] = -1 at init and on a cut change */
extern u_char *lpCopyMP[35];                /* sbss 0x0193cf80: per Character[] slot, the model part a cut's
                                               eye / target follows (asFindModelParts' answer, a host
                                               pointer); a cut clears them all */

// A bank's camera control table (ALL_TABLE.ctrl, bank data, layout: ee; CamChangeSub).
#define CAM_CTRL_CUTS 0x0                   /* u_int: EE address of the CAM_CUT records */
#define CAM_CTRL_TARGETS 0x4                /* u_int: EE address of the CAM_TRG records */
#define CAM_CTRL_CUT_COUNT 0xc              /* u_char: the cuts (0 = the bank has no camera) */
// A cut record (0x20 bytes; names from CamChangeSub and asCopyCharacterParameterEYE, 2026-11-10).
#define CAM_CUT_SIZE 0x20
#define CAM_CUT_HASH 0x00                   /* u_long: the model part the eye follows */
#define CAM_CUT_CHARA 0x10                  /* u_short: the Character[] slot that part belongs to */
#define CAM_CUT_PART 0x0e                   /* u_short: the eye's part in the bank (asFindCamera; not on a character) */
#define CAM_CUT_ON_CHARA 0x12              /* u_char: 1 = the eye follows the character's part */
#define CAM_CUT_TARGETS 0x13                /* u_char[3]: CAM_TRG record indices */
#define CAM_CUT_TRG_MODE 0x17               /* s_char: how the focus comes from the targets (CAM_TRG_MODE_*) */
#define CAM_CUT_EYE_KIND 0x18               /* u_char: asGetDestinationEYE's kind */
#define CAM_CUT_DOF 0x1a                    /* u_char: CAMERA.dofLevel (4 = no DOF) */
#define CAM_CUT_MOVE_SPEED 0x1b             /* u_char: CAMERA.moveSpeed */
#define CAM_EYE_MODE_MOVE 0xff              /* CAMERA.eyeMode / trgMode: the cut's move runs (camera_view0) */
#define CAM_CHANGE_MODE_EASE 2              /* CAMERA.changeMode: the move eases (1 - cos) / 2, else linear */
#define CAM_EYE_KIND_ORBIT 2                /* camera_view0: the eye keeps ZelCam's distance range from the
                                               focus raised by ZelCam[3] (name from that use) */
void asCameraPositionTranser(int speed, float *out, const float *to, const float *from); // CT3 0x01883fd0 written
void asCamModeChange(int cam);                                                           // CT3 0x01884610 written
// A target record (0x10 bytes; asCopyCharacterParameterTRG).
#define CAM_TRG_SIZE 0x10
#define CAM_TRG_HASH 0x00                   /* u_long: the model part looked at */
#define CAM_TRG_PART 0x08                   /* u_short: the part's index in the camera's bank (not on a character) */
#define CAM_TRG_CHARA 0x0a                  /* u_short: its Character[] slot */
#define CAM_TRG_ON_CHARA 0x0c               /* u_char: 1 = a character's part (lpCopyMP), 0 = the bank's */
#define CAM_PART_POS 0xf0                   /* a part record's world position (qword; SHADOW_LIGHT_POS) */
// CAM_CUT_TRG_MODE (asGetDstinationFromTargetList; CamChangeSub: 0 = no target list).
#define CAM_TRG_MODE_NONE 0
#define CAM_TRG_MODE_ONE 1                  /* the focus is target 0's position */
#define CAM_TRG_MODE_2 2                    /* leaves the focus */
#define CAM_TRG_MODE_AXES 3                 /* x, y, z each from targets 0, 1, 2 */
#define CAM_TRG_MODE_4 4                    /* leaves the focus */
#define CAM_EYE_KIND_1 1                    /* CamChangeSub: kinds 1 and 6 need a target list */
#define CAM_EYE_KIND_6 6
#define CAM_TRG_KIND_LIST 0                 /* asGetDestinationTRG: look at the cut's target list */
#define CAM_TRG_KIND_NONE 4
#define CAM_DOF_NONE 4                      /* CAMERA.dofLevel: no DOF pass */
#define CAM_CHANGE_NEEDS_TARGETS 4          /* CamChangeSub: an eye kind 1 / 6 cut without a target list */
#define TRIG_EVENT_CAMERA_CUT 0xb3          /* CamChangeSub: ssTrigRegisterEvent on a scene bank's cut */

#define CAMERA_EYE_HEIGHT_INIT 135.0f       /* camera_init: AlissaEyeHeight */
#define CAMERA_BEHIND_INIT 250.0f           /* camera_init: CameraBehind */
#define CAMERA_FOV_DEG_INIT 40.0f           /* camera_init: FOV_deg */
#define CAMERA_FOCAL_INIT 2.88f             /* camera_init0: CAMERA.focal[] (LEICA units) */
#define CAMERA_FIRST_CUT_SKIP 2             /* CamChange_S: SkipMain frames for the first scene-bank cut */
#define CAMERA_CHANGE_NONE 1                /* CamChange_S: no bank (< 0) */
#define CAMERA_CHANGE_SAME_CUT 5            /* CamChange_S: the bank already shows this cut */

void asChangeViewMatrix_forTheBank(int bank);                                            // CT3 0x01886690 written
u_char CamGetNowDOFLevel(void);                                                          // CT3 0x01886770 written
void camera_destroy(int scene, int mode);                                                // CT3 0x018875a0 written
void camera_view(void);                                                                  // CT3 0x018875d0 written
void camera_view0(int cam);                                                              // CT3 0x01885d20
int CamChange(CAM_CHANGE *req);                                                          // CT3 0x01886f30 written
int CamChange_Hash_S_Hash(int bank, u_long hash, short mode, int arg);                   // CT3 0x01886b70 written
short asFindCamera(int bank, u_long hash);                                               // CT3 0x01886cf0 written
void asCopyCameraMatrix(void);                                                           // CT3 0x01887040 written
void camera_main(PAD_DATA *pad, int mode);                                               // CT3 0x018881a0
void camera_init(void);                                                                  // CT3 0x01889c50 written
// From data_load_set's calls (Character_Init.c, 2026-11-10).
void camera_init0(int cam, int mode);                                                    // CT3 0x018870c0 written
int CamChange_S(int cam, short cut, u_char mode, int arg);                               // CT3 0x01886dc0 written
int CamChangeSub(int cam);                                                               // CT3 0x018867d0
int CamChangeFOV_S(void);                                                                // CT3 0x018867c0 written
int asEyeForcusUP2CamMat(float proj, float (*view)[4], float *eye, float *focus, float *up); // CT3 0x01883db0 written
void asCameraPositionTranserUP(int step, float *out, const float *from, const float *to); // CT3 0x01883f00 written
void asCopyCharacterParameterTRG(u_int target, int id);                                  // CT3 0x018841f0 written
void asCopyCharacterParameterEYE(int id);                                                // CT3 0x01884300 written
int asGetDstinationFromTargetList(float *focus, u_char *partsHeader, u_char *cutRec, int id); // CT3 0x01884410
int asCheckInnerPositions(float a, float x, float b);                                    // CT3 0x01884e60 written
u_int asCheckThePositionInRectAngle(float (*quad)[4], float *p, float *n);               // CT3 0x01884680 written
int asGetPlaneRadTargetPos(float *out, float *p, float *dir, float (*quad)[4]);          // CT3 0x018847f0 written
int asGetLineRadTargetPos(float *a, float *b, float *p, float *out, float *foot, int angle); // CT3 0x01884c20 written
int asGetDestinationTRG(int kind, float *focus, int id);                                 // CT3 0x01884eb0 written
int asGetDestinationEYE(int kind, float *eye, float *focus, float *up, float *focal, int id); // CT3 0x01884f50
