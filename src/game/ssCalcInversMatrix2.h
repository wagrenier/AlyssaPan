// game/ssCalcInversMatrix2.h -- CT3 game/ssCalcInversMatrix2.c: the declarations other TUs call
// (asMakeStraw first, 2026-11-10). Prototypes from the call sites; the TU that writes these confirms them.
#pragma once

#include "sdk/scetypes.h"

// out = the inverse of m (asMakeStraw keeps its translation row too).
void ssCalcInversMatrix2(float (*out)[4], float (*m)[4]);                                // CT3 0x018a88e0
// ---- from ssHitCheck*Attack's calls (ssHitCheckAttackToScn.c slice 3, 2026-11-11) ----
struct HIT_OBJ;
struct HIT_DATA;
struct HIT_DMG;
struct HIT_TABLE;
struct SS_HIT_ATTACK;
// The attack object's box from ssHitNPos / ssHitBPos and its rows (chara: 1 a character, 0 a scene attack).
void ssHitMakeAttackOBB(struct HIT_OBJ *obj, int chara);                                 // CT3 0x018a9570
// The character's body box (HIT_TABLE.bodyObb) from its damage shapes.
void ssHitMakeDamageOBB(struct HIT_TABLE *dmg);                                          // CT3 0x018a9390
// A struck damage shape: atkHits the attacker character's table (NULL for a scene attack), scnHits the
// attacking scene bank's (NULL for a character).
void ssHit_information(struct SS_HIT_ATTACK *a, struct HIT_OBJ *obj, struct HIT_DMG *d, struct HIT_TABLE *dmg,
                       struct HIT_TABLE *atkHits, struct HIT_TABLE *scnHits);            // CT3 0x018a91d0
// A struck scene shape h of scn at `pos` (w: ssHitNPos.w).
void ssHit_informationScn(struct SS_HIT_ATTACK *a, struct HIT_OBJ *obj, struct HIT_DATA *h, struct HIT_TABLE *scn,
                          struct HIT_TABLE *atkHits, const float *pos);                  // CT3 0x018a8e00
