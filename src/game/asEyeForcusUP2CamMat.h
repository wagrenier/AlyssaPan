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
extern int iCameraMode;                     /* sbss 0x0193d0a0 */
extern float CameraAngle[4];                /* sbss 0x0193d0c0 */
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
#define CAM_CUT_ON_CHARA 0x12               /* u_char: 1 = the eye follows the character's part */
#define CAM_CUT_TARGETS 0x13                /* u_char[3]: CAM_TRG record indices */
#define CAM_CUT_TRG_MODE 0x17               /* s_char: how the focus comes from the targets (CAM_TRG_MODE_*) */
#define CAM_CUT_EYE_KIND 0x18               /* u_char: asGetDestinationEYE's kind */
#define CAM_CUT_DOF 0x1a                    /* u_char: CAMERA.dofLevel (4 = no DOF) */
#define CAM_CUT_1B 0x1b                     /* u_char: CAMERA.b181 */
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
void camera_view(void);                                                                  // CT3 0x018875d0
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
