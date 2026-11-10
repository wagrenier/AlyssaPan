// game/ahRelocateSunPs2ObjectArry.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "sdk/eekernel.h"
#include "game/ahRelocateSunPs2ObjectArry.h"

#include <math.h>
#include <string.h>

#include "ga/gaBGT_DrawObject.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "ga/gaSearchNearLight4.h"
#include "ga/gaSinCos.h"
#include "game/Convert.h"
#include "game/LinkMatrixTree.h"
#include "game/QuatSlerp.h"
#include "game/ahalloc.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssCalcInversMatrix2.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"

// ---- the file relocator (CT3 0x0183e280..0x01842290) ----
//
// A CT3X file (a bank image, fileio.c's FSMltLoadProc) is linked for the EE address in its word +8; the
// relocator adds (the address it was loaded at - that base) to every pointer word, walking the image
// through the pointers it has just fixed, then stores the new base at +8 so a second call does nothing.
// PORT (open-items.md 1 h / bv): the image keeps its EE layout and is relocated to the EE address the
// host gave its bank (BankImageEE[]), so its words stay EE addresses; the walk maps each one back into
// the host image (RelocPtr). The ROM recomputes (image - image[+8]) at every word; +8 changes only at the
// end, so the host takes the difference once.

static u_char *sRelocImage;             /* PORT: the host image being relocated */
static u_int sRelocBase;                /* PORT: the EE address it is relocated to */

// PORT: the host address of an EE address inside the image being relocated.
static u_char *RelocPtr(u_int ee)
{
    return sRelocImage + (int)(ee - sRelocBase);
}
#define RW(ee) (*(u_int *)RelocPtr(ee))         /* a word of the image, by EE address */
#define RH(ee) (*(u_short *)RelocPtr(ee))       /* a halfword */
#define RB(ee) (*RelocPtr(ee))                  /* a byte */

// The ROM's inline step: a non-zero word is moved by `delta`; 0 stays 0. Answers the word.
static u_int RelocWord(u_int ee, int delta)
{
    u_int *w = (u_int *)RelocPtr(ee);
    if (*w != 0)
        *w += delta;
    return *w;
}

// The 0x10-byte records of the shape header's eight key tables (ALL_TABLE +0xec: {count, list} pairs
// from +0x1c): a key list at +0xc of u_short(+8) words, of which only two when +4 is 0.
static void RelocKeyTable(u_int list, int count, int delta)
{
    int i, k;
    for (i = 0; i < count; i++) {
        u_int e = list + i * 0x10;
        RelocWord(e + 4, delta);
        RelocWord(e + 0xc, delta);
        for (k = 0; k < RH(e + 8); k++) {
            RelocWord(RW(e + 0xc) + k * 4, delta);
            if (RW(e + 4) == 0 && k == 1)
                break;
        }
    }
}

// A 0x80-byte sun object list record as ahRelocateSunPs2ObjectArry relocates it: +0 the ga draw object
// (its +0x50 / +0x60..+0x6c words), +4, +8, +0x14, +0x18.
#define GA_OBJ_RELOC(g, R) do { R((g) + 0x50); R((g) + 0x60); R((g) + 0x64); R((g) + 0x68); R((g) + 0x6c); } while (0)

// CT3 0x0183e280
// A 0-ended list of sun object records (the ALL_TABLE draw lists objOpaque / objTrans).
// PORT: the list and both bases are EE addresses inside the image being relocated.
void ahRelocateSunPs2ObjectArry(u_int arry, u_int image, u_int base)
{
    int d = (int)(image - base);
#define R(ee) RelocWord((ee), d)
    for (; RW(arry) != 0; arry += 4) {
        u_int o = R(arry);
        u_int g;
        R(o + 4);
        R(o + 8);
        g = R(o);
        GA_OBJ_RELOC(g, R);
        R(o + 0x14);
        R(o + 0x18);
    }
#undef R
}

// The kinds of a behaviour track (its +9 byte, ahRelocateBehavior).
#define BEHAVIOR_TRACK_H 'H'            /* keys from (data + its +4) for u_short(+0xc) runs */
#define BEHAVIOR_TRACK_GT '>'           /* keys from data for u_short(+0xc) + 1 runs */
#define BEHAVIOR_TRACK_EQ '='           /* the same, the run length in +4's low 25 bits */
#define BEHAVIOR_TRACK_EQ_FLAG 0x4000000 /* '=' tracks: the ROM tests it, both paths do the same */
#define BEHAVIOR_TRACK_EQ_LEN 0x1ffffff
#define BEHAVIOR_KEY_SIZE 0x18          /* a key: +4 its length, +8 a pointer */

// CT3 0x0183e440
// A behaviour (ALL_TABLE +0xbc, or a BHVR file's entries): short(+8) groups of 8 bytes from +0x10, each
// u_short(+2) tracks of 0x20 bytes at its +4; a track's keys hold one pointer each (+8).
// PORT: EE addresses inside the image being relocated, as ahRelocateSunPs2ObjectArry.
void ahRelocateBehavior(u_int behavior, u_int image, u_int base)
{
    int d = (int)(image - base);
    int g, t, run;
#define R(ee) RelocWord((ee), d)
    for (g = 0; g < (short)RH(behavior + 8); g++) {
        u_int grp = behavior + g * 8;
        u_int tracks = R(grp + 0x14);
        for (t = 0; t < (short)RH(grp + 0x12); t++) {
            u_int tr = tracks + t * 0x20;
            u_int key = R(tr);
            char kind = (char)RB(tr + 9);
            if (kind == BEHAVIOR_TRACK_H) {
                key += RW(tr + 4);
                for (run = 0; run < RH(tr + 0xc); run++) {
                    int left;
                    for (left = (int)RW(tr + 4); left > 0; ) {
                        u_int len;
                        R(key + 8);
                        len = key + 4;
                        key += BEHAVIOR_KEY_SIZE;
                        left -= (int)RW(len);
                    }
                }
            } else if (kind == BEHAVIOR_TRACK_GT) {
                for (run = 0; run < RH(tr + 0xc) + 1; run++) {
                    int left;
                    for (left = (int)RW(tr + 4); left > 0; ) {
                        u_int len;
                        R(key + 8);
                        len = key + 4;
                        key += BEHAVIOR_KEY_SIZE;
                        left -= (int)RW(len);
                    }
                }
            } else if (kind == BEHAVIOR_TRACK_EQ) {
                // The ROM branches on BEHAVIOR_TRACK_EQ_FLAG into two identical loops.
                for (run = 0; run < RH(tr + 0xc) + 1; run++) {
                    u_int left = RW(tr + 4) & BEHAVIOR_TRACK_EQ_LEN;
                    if (left != 0) {
                        do {
                            R(key + 8);
                            left -= RW(key + 4);
                            key += BEHAVIOR_KEY_SIZE;
                        } while ((int)left > 0);
                    }
                }
            }
        }
    }
#undef R
}

// CT3 0x0183e710
// FSMltLoadProc's call: relocate a loaded file. BHVR: a 0-ended list of behaviours from +0x40. SHPE /
// ROOM: the ALL_TABLE at +0x80 and everything it points to; `persona` (a shape or behaviour load, not a
// scene) picks the shorter forms of its +0x20 and +0xc0 records and skips the behaviour.
// PORT: `image` is the host image; it is relocated to BankImageEE[] of the bank holding it.
void ahRelocateFileObject(u_char *image, int persona)
{
    u_int img = 0, base, t, l, e, o;
    int d, i, k, n, bank;

    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankImage[bank] == image) {
            img = BankImageEE[bank];
            break;
        }
    }
    if (bank == BANK_MAX)
        return;                                         /* PORT: not a bank image: nothing to map */
    sRelocImage = image;
    sRelocBase = img;
    base = RW(img + CT3X_BASE);
    if (base == img)
        return;
    d = (int)(img - base);
#define R(ee) RelocWord((ee), d)
#define AT(off) (img + BANK_ALL_TABLE_OFFSET + (off))   /* an ALL_TABLE word, by its EE offset */

    if (RW(img + CT3X_MAGIC) == CT3X_MAGIC_BHVR) {
        for (o = img + CT3X_BHVR_LIST; RW(o) != 0; o += 4) {
            R(o);
            ahRelocateBehavior(RW(o), img, base);
        }
        RW(img + CT3X_BASE) = img;
        return;
    }
    if (RW(img + CT3X_MAGIC) != CT3X_MAGIC_SHPE && RW(img + CT3X_MAGIC) != CT3X_MAGIC_ROOM)
        return;

    R(AT(0x10));                                        /* partsPos */
    R(AT(0x18));                                        /* objOpaque */
    ahRelocateSunPs2ObjectArry(RW(AT(0x18)), img, base);
    R(AT(0x1c));                                        /* objTrans */
    ahRelocateSunPs2ObjectArry(RW(AT(0x1c)), img, base);

    t = R(AT(0x28));                                    /* light (SCENE_LIGHT) */
    R(t);
    R(t + 4);
    for (i = 0; i < (int)RW(t + 0x14); i++) {
        e = RW(t + 4) + i * 0x14 + 0x10;
        if (RW(e) != (u_int)-1)
            R(e);
    }
    R(t + 0xc);
    R(t + 0x10);

    t = R(AT(0x2c));                                    /* partsHeader: short(+2) MODEL_PARTS */
    for (i = 0; i < (short)RH(t + 2); i++)
        R(t + i * 0x380 + 0x44);                        /* MODEL_PARTS.attr */

    R(AT(0x30));
    for (i = 0; i < (int)RW(AT(0x38)); i++) {
        e = R(RW(AT(0x30)) + i * 4);
        R(e + 0x1c);
        R(e + 0x20);
        R(e + 0x24);
        R(e + 0x28);
        R(e + 0x30);
        for (k = 0; (u_int)k < RW(e + 0x2c); k++)
            R(RW(e + 0x30) + k * 4);
    }
    R(AT(0x34));
    for (i = 0; i < (int)RW(AT(0x3c)); i++) {
        e = R(RW(AT(0x34)) + i * 4);
        R(e);
    }
    R(AT(0x40));
    R(AT(0x44));
    R(AT(0x48));
    for (i = 0; i < (int)RW(AT(0x14)); i++)             /* baseobjCount words */
        R(RW(AT(0x48)) + i * 4);
    R(AT(0x24));
    for (o = 0; RW(RW(AT(0x24)) + o) != 0; o += 4) {
        e = R(RW(AT(0x24)) + o);
        R(e + 0x10);
        R(e + 0x14);
    }

    t = R(AT(0x20));
    if (!persona) {
        l = R(t);
        for (o = 0; RW(l + o) != 0; o += 4)
            R(l + o);
        R(t + 4);
        for (i = 0; i < 0x40; i++)
            R(t + 0x10 + i * 4);
        l = R(t + 0x2a0);
        n = (int)RW(t + 0x2a4);
        for (i = 0; i < n; i++) {
            e = R(l + i * 4);
            R(e + 0xd0);
            R(e + 0xd4);
        }
        l = R(t + 0x2a8);
        n = (int)RW(t + 0x2ac);
        for (i = 0; i < n; i++)
            R(l + i * 4);
        R(t + 0x2b4);
        R(t + 0x2bc);
    } else {
        l = R(t);
        n = RB(t + 0x18);
        for (i = 0; i < n; i++)
            R(l + i * 4);
        R(t + 4);
        l = R(t + 8);
        n = RB(t + 0x19);
        for (i = 0; i < n; i++) {
            e = R(l + i * 4);
            R(e + 0xd0);
            R(e + 0xd4);
        }
        R(t + 0xc);
        R(t + 0x10);
        R(t + 0x14);
        R(t + 0x20);
        R(t + 0x28);
    }

    t = R(AT(0x4c));
    R(t + 4);
    R(t + 8);
    R(t);

    t = R(AT(0x50));
    if (t != 0) {
        R(t);
        for (i = 0; i < RH(t + 4); i++) {
            u_int r = RW(t) + i * 0x10;
            R(r + 0xc);
            if (RW(r + 0xc) != 0) {
                for (o = 0; RW(RW(r + 0xc) + o) != 0; o += 0x10) {
                    e = RW(r + 0xc) + o;
                    R(e);
                    R(e + 4);
                    R(e + 8);
                    R(e + 0xc);
                }
            }
        }
    }

    if (RW(AT(0x54)) != 0) {                            /* sound (SOUND_OBJ_TABLE) */
        t = R(AT(0x54));
        R(t + 4);                                       /* objs */
        R(t);                                           /* loadSE */
        for (o = 0; RW(RW(t) + o) != 0; o += sizeof(SE_LOAD_POINT))
            R(RW(t) + o);
    }

    R(AT(0x58));                                        /* particles */
    for (o = 0; RW(RW(AT(0x58)) + o) != 0; o += 4) {
        u_int p = R(RW(AT(0x58)) + o);
        u_int g = R(p);
        u_int s;
        GA_OBJ_RELOC(g, R);
        R(p + 4);
        R(p + 8);
        s = R(p + 0xc);
        if (RW(s + 0x2c) != 0) {
            u_int v = R(s + 0x2c);
            if (RW(v + 4) == 0) {                       /* shared: relocated once */
                R(v + 8);
                RW(v + 4) = 1;
            }
        }
        R(s + 0x30);
        R(s + 0x34);
        R(s + 0x38);
        R(s + 0x3c);
        R(p + 0x14);
        R(p + 0x18);
        R(p + 0x1c);
        R(p + 0x20);
        R(p + 0x24);
    }

    R(AT(0x5c));
    for (o = 0; RW(RW(AT(0x5c)) + o) != 0; o += 4) {
        e = R(RW(AT(0x5c)) + o);
        n = RB(e + 2);
        R(e + 4);
        for (i = 0; i < n; i++) {
            u_int r = RW(e + 4) + i * 0x1c;
            R(r);
            R(r + 0x14);
        }
    }

    if (RW(AT(0x60)) != 0) {                            /* objAfter */
        R(AT(0x60));
        for (o = 0; RW(RW(AT(0x60)) + o) != 0; o += 4) {
            u_int p = R(RW(AT(0x60)) + o);
            u_int g = R(p);
            R(g + 0x50);
            R(g + 0x58);
            R(g + 0x5c);
            R(g + 0x60);
            R(g + 0x64);
            R(g + 0x68);
            R(g + 0x6c);
            R(p + 4);
            R(p + 0x14);
            R(p + 0x18);
        }
    }
    if (RW(AT(0x64)) != 0)
        R(AT(0x64));
    if (RW(AT(0x68)) != 0) {
        l = R(AT(0x68));
        for (o = 0; RW(l + o) != 0; o += 4) {
            e = R(l + o);
            R(e + 0x10);
            R(e + 0x14);
            n = (int)RH(e + 4) * (int)RH(e + 6);
            for (i = 0; i < n; i++) {
                u_int c = R(RW(e + 0x10) + i * 4);
                if (c != 0) {
                    u_int a = R(c);
                    u_int b = R(c + 4);
                    int m = (int)RW(c + 8);
                    for (k = 0; k < m; k++) {
                        R(a + k * 4);
                        R(b + k * 4);
                    }
                }
            }
            for (k = 0; RW(RW(e + 0x14) + k) != 0; k += 4)
                R(RW(e + 0x14) + k);
        }
    }
    if (RW(AT(0x90)) != 0) {
        l = R(AT(0x90));
        for (o = 0; RW(l + o) != 0; o += 4) {
            e = R(l + o);
            R(e + 0xc);
            R(e + 0x10);
        }
    }
    if (RW(AT(0x6c)) != 0) {
        t = R(AT(0x6c));
        for (k = 0; k < 7; k++)
            R(t + k * 4);
        for (k = 0; k < 8; k++) {                       /* the key tables: {count +0x1c, list +0x20}... */
            u_int list = R(t + 0x20 + k * 8);
            RelocKeyTable(list, (int)RW(t + 0x1c + k * 8), d);
        }
    }
    if (RW(AT(0x70)) != 0)                              /* partsLight */
        R(AT(0x70));
    if (RW(AT(0x74)) != 0) {
        R(AT(0x74));
        for (o = 0; RW(RW(AT(0x74)) + o) != (u_int)-1; o += 0x28)
            for (k = 4; k <= 9; k++)
                R(RW(AT(0x74)) + o + k * 4);
    }
    if (RW(AT(0x80)) != 0) {
        R(AT(0x80));
        for (o = 0; RW(RW(AT(0x80)) + o) != (u_int)-1; o += 0x28)
            for (k = 5; k <= 9; k++)
                R(RW(AT(0x80)) + o + k * 4);
    }
    if (RW(AT(0x84)) != 0) {
        R(AT(0x84));
        for (o = 0; RW(RW(AT(0x84)) + o) != (u_int)-1; o += 0x24)
            for (k = 5; k <= 8; k++)
                R(RW(AT(0x84)) + o + k * 4);
    }
    for (k = 0; k < 2; k++) {                           /* particles2, particles2After */
        u_int slot = AT(0x78 + k * 4);
        if (RW(slot) == 0)
            continue;
        l = R(slot);
        for (o = 0; RW(l + o) != 0; o += 4) {
            u_int p = R(l + o);
            if (RW(p + 0x30) != 0) {
                u_int v = R(p + 0x30);
                if (RW(v + 0xc) == 0) {                 /* shared: relocated once */
                    R(v + 0x10);
                    R(v + 0x14);
                    RW(v + 0xc) = 1;
                }
            }
            R(p + 0x50);
            R(p + 0x54);
            R(p + 0x5c);
            R(p + 0x74);
            R(p + 0x78);
            n = RH(p + 0x44);
            for (i = 0; i < n; i++) {
                u_int r = RW(p + 0x5c) + i * 0x80;      /* PARTICLE2 ring records */
                u_int g = R(r + 0x68);
                GA_OBJ_RELOC(g, R);
                R(r + 0x6c);
            }
            R(p + 0x60);
            if (k == 1)                                 /* only the After list's records have +0x64 */
                R(p + 0x64);
            R(p + 0x68);
            R(p + 0x6c);
            R(p + 0x70);
        }
    }
    if (RW(AT(0x88)) != 0) {
        l = R(AT(0x88));
        for (o = 0; RW(l + o) != 0; o += 4) {
            u_int a;
            int m;
            e = R(l + o);
            a = R(e + 0x10);
            m = (int)RW(e + 0xc);
            for (i = 0; i < m; i++) {
                u_int r = a + i * 0x14;
                R(r);
                R(r + 4);
                R(r + 0x10);
            }
        }
    }
    if (RW(AT(0x8c)) != 0) {
        t = R(AT(0x8c));
        l = R(t);
        for (o = 0; RW(l + o) != 0; o += 0x14)
            R(l + o);
        l = R(t + 4);
        for (o = 0; RW(l + o) != (u_int)-1; o += 8) {
            u_int a = R(l + o + 4);
            for (i = 0; i < (int)RW(l + o); i++)
                R(a + i * 4);
        }
        l = R(t + 8);
        for (o = 0; RW(l + o) != (u_int)-1; o += 8) {
            R(l + o + 4);
            for (i = 0; i < (int)RW(l + o); i++) {
                u_int r = RW(l + o + 4) + i * 0x18;
                R(r + 4);
                R(r + 8);
                R(r + 0xc);
                R(r + 0x10);
                R(r + 0x14);
            }
        }
    }
    R(AT(0x94));
    for (o = 0; RW(RW(AT(0x94)) + o) != 0; o += 4) {
        e = R(RW(AT(0x94)) + o);
        R(e + 8);
    }
    if (RW(AT(0x98)) != 0) {
        l = R(AT(0x98));
        for (o = 0; RW(l + o) != 0; o += 4)
            R(l + o);
    }
    if (RW(AT(0xa0)) != 0) {
        l = R(AT(0xa0));
        for (o = 0; RW(l + o) != 0; o += 4) {
            e = R(l + o);
            R(e + 0x10);
            R(e + 8);
            n = (int)RH(e + 4) * (int)RH(e + 6);
            for (i = 0; i < n; i++) {
                u_int c = R(RW(e + 8) + i * 4);
                R(c);
                R(c + 4);
                for (k = 0; k < (int)RW(c + 8); k++) {
                    R(RW(c) + k * 4);
                    R(RW(c + 4) + k * 4);
                }
            }
        }
    }

    if (!persona) {
        R(AT(0xb8));                                    /* fog */
        R(AT(0xbc));                                    /* behavior */
        ahRelocateBehavior(RW(AT(0xbc)), img, base);
        t = R(AT(0xc4));
        R(t);
        l = R(t + 8);
        R(t + 0x10);
        for (i = 0; i < (int)RW(t + 0xc); i++) {
            R(l + i * 0x14);
            R(l + i * 0x14 + 0xc);
        }
        t = R(AT(0xc0));
        if (t != 0) {
            R(t);
            R(t + 4);
            R(t + 8);
            for (i = 0; i < RB(t + 0xe); i++)
                R(RW(t + 8) + i * 8);
        }
        t = R(AT(0xcc));
        if (t != 0)
            R(t);
        if (RW(AT(0xd8)) != 0) {
            t = R(AT(0xd8));
            R(t + 4);
            for (i = 0; i < (int)RW(t); i++) {
                u_int r = RW(t + 4) + i * 0x50;
                R(r);
                R(r + 4);
                R(r + 8);
                R(r + 0x2c);
            }
        }
        R(AT(0xd4));
        for (o = 0; RW(RW(AT(0xd4)) + o) != 0; o += 4) {
            u_int p = R(RW(AT(0xd4)) + o);
            R(p);
        }
        if (RW(AT(0xdc)) != 0) {
            t = R(AT(0xdc));
            R(t + 4);
            for (i = 0; i < (int)RW(t); i++) {
                R(RW(t + 4) + i * 0x10);
                R(RW(t + 4) + i * 0x10 + 4);
            }
        }
        R(AT(0xe0));
        for (o = 0; RW(RW(AT(0xe0)) + o) != 0; o += 4)
            R(RW(AT(0xe0)) + o);
        RW(AT(0xe8)) = 0;
    } else {
        R(AT(0xb8));                                    /* fog */
        t = R(AT(0xc0));
        l = R(t + 4);
        for (i = 0; i < (int)RW(t); i++) {
            u_int r = l + i * 0x10;
            u_int a;
            R(r + 4);
            a = R(r + 0xc);
            for (k = 0; k < (int)RW(r + 8); k++)
                R(a + k * 8 + 4);
        }
    }
    RW(img + CT3X_BASE) = img;
#undef AT
#undef R
}

// ---- the animation pass (CT3 0x018422e0..0x018427f0, slice 2, 2026-11-10) ----
//
// Every frame main runs asUpDateSceneAnimation (the scene banks' behaviours, then their matrix trees)
// and asUpDateGAobjectsAnimation (the ga objects' straws, through a SkinWork scratch block). A bank
// animates while it is drawn (or BANK_FLAG_DRAW_FIRST) and asCheckAnimationExecutable_Scene allows it:
// a type-0 bank always, the others by the world's pause state and asForceOperateAnimation_Scene's bits.

int iEASnum;                            /* sbss 0x0193f780 (symtab: global) */
void *lpEAS;                            /* sbss 0x0193f77c: the history records (_SunAlloc'd) */
int iEASidx;                            /* sbss 0x0193f784: records in lpEAS */
int FrameCounter;                       /* sbss 0x0194494c (symtab: global; owner unknown, defined with its one writer) */

// CT3 0x01842380
// 1 while the world moves: no pause (or a PAUSE_RUNNING one) and none reserved for the next frame.
int asCheckNowMoving(void)
{
    if (pause != 0 && !(pause & PAUSE_RUNNING))
        return 0;
    if (pause_reservation != 0)
        return 0;
    return 1;
}

// CT3 0x018422e0
int asCheckAnimationExecutable_Scene(int scene)
{
    BANK_INFO *bi;
    if (scene < 0)
        return 0;
    bi = &BankInfo[scene];
    if (bi->type == BANK_TYPE_0)
        return 1;
    if (asCheckNowMoving())
        return (bi->flags & BANK_FLAG_ANIM_FORCE_STOP) == 0;
    return (bi->flags & BANK_FLAG_ANIM_FORCE_RUN) != 0;
}

// CT3 0x01842290
// The character's bank (Character[].bank), as asCheckAnimationExecutable_Scene answers it.
int asCheckAnimationExecutable_Character(int chara)
{
    if (chara < 0)
        return 0;
    return asCheckAnimationExecutable_Scene(Character[chara].bank);
}

// CT3 0x018423c0
// The character's bank, with the caller's mode (the ROM leaves a1 as it came).
void asForceOperateAnimation_Character(int chara, int mode)
{
    if (chara >= 0)
        asForceOperateAnimation_Scene(Character[chara].bank, mode);
}

// CT3 0x01842400
void asForceOperateAnimation_Scene(int scene, int mode)
{
    u_int *flags;
    if (scene < 0 || BankInfo[scene].state != BANK_STATE_LOADED)
        return;
    flags = &BankInfo[scene].flags;
    *flags &= ~(BANK_FLAG_ANIM_FORCE_STOP | BANK_FLAG_ANIM_FORCE_RUN);
    if (mode == ANIM_FORCE_RUN)
        *flags |= BANK_FLAG_ANIM_FORCE_RUN;
    else if (mode == ANIM_FORCE_STOP)
        *flags |= BANK_FLAG_ANIM_FORCE_STOP;
    if (mode == ANIM_FORCE_STOP)
        *flags |= BANK_FLAG_ANIM_STOPPED;
    else if (mode == ANIM_FORCE_RUN || mode == ANIM_FORCE_NONE)
        *flags &= ~BANK_FLAG_ANIM_STOPPED;
}

// CT3 0x01842540
void asUpDateGAobjectsAnimation(void)
{
    int bank;
    short i;
    Execute_ExecuteAnimationHistory();
    SkinWork = _SunAlloc(&gHeapJunc, SKIN_WORK_SIZE, 0x10);
    for (bank = 0; bank < BANK_MAX; bank++) {
        BANK_INFO *bi = &BankInfo[bank];
        if (bi->state == BANK_STATE_LOADED
            && (bi->drawOn == DRAW_BANK_ON || (bi->flags & BANK_FLAG_DRAW_FIRST))
            && (bi->type == BANK_TYPE_SHADOW || bi->type == BANK_TYPE_0)
            && All_table_TBL[bank] != NULL && asCheckAnimationExecutable_Scene(bank)) {
            ALL_TABLE *tbl = All_table_TBL[bank];
            asMakeStraw(bank, tbl->straw, tbl->partsHeader);
        }
        if (bi->state > 0)
            bank += bi->groupCount - 1;
    }
    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *chr = &Character[i];
        if ((chr->flags & CHARACTER_FLAG_REFRACTION) && chr->bank > 0 && (chr->flags & CHARACTER_FLAG_SHADOW)) {
            ALL_TABLE *tbl = All_table_TBL[chr->bank];
            asMakeStraw(chr->bank, tbl->straw, tbl->partsHeader);
        }
    }
    _SunFree(&gHeapJunc, &SkinWork);
}

// CT3 0x018426e0
void asUpDateSceneAnimation(void)
{
    int bank;
    for (bank = 0; bank < BANK_MAX; bank++) {
        BANK_INFO *bi = &BankInfo[bank];
        if (bi->state != BANK_STATE_LOADED)
            continue;
        if ((bi->drawOn == DRAW_BANK_ON || (bi->flags & BANK_FLAG_DRAW_FIRST))
            && asCheckAnimationExecutable_Scene(bank)
            && (bi->type == BANK_TYPE_SHADOW || bi->type == BANK_TYPE_0)) {
            ALL_TABLE *tbl = All_table_TBL[bank];
            asSeekBarAnimation(bank);
            ExecuteAnimation(bank, tbl->behavior, tbl, 0, 1, 0, 0, 0);
            BuildMatrixTree(bank, 2, 0);
            asSearchObject(bank);
        }
        if (bi->state > 0)
            bank += bi->groupCount - 1;
    }
}

// CT3 0x0184a4b0
// The weights in single precision as the ROM rounds them, then vmulax / vmadday / vmaddaz / vmaddw.
void asGetBeziePosition(float t, float s, float out[4], const float ctrl[4][4])
{
    float t2 = t * t, s2 = s * s;
    float w[4];
    int i;
    w[0] = t2 * t;
    w[1] = 3.0f * (t2 * s);
    w[2] = 3.0f * (s2 * t);
    w[3] = s2 * s;
    for (i = 0; i < 4; i++)
        out[i] = ctrl[0][i] * w[0] + ctrl[1][i] * w[1] + ctrl[2][i] * w[2] + ctrl[3][i] * w[3];
}

// CT3 0x0184ef00
void ExecuteAnimationHistory_init(void)
{
    iEASnum = 0;
    lpEAS = NULL;
    iEASidx = 0;
}

// CT3 0x0184ec20
// Plays back the animations recorded since the last frame (lpEAS, iEASidx records), each at its
// recorded sign and step (the parts header's PARTS_HDR_ANIM_*, cleared after), then, for a record
// whose animation's first doubleword is all ones, copies each attached part's parent word into the
// scene's attach list (ALL_TABLE.ctrl +0); frees the records. FrameCounter counts every call.
// iEASidx is left as it is (the ROM's: the history adder sets it).
void Execute_ExecuteAnimationHistory(void)
{
    EAS_RECORD *base = (EAS_RECORD *)lpEAS;
    EAS_RECORD *rec;
    int n;
    FrameCounter++;
    if (base == NULL)
        return;
    for (rec = base, n = iEASidx; n != 0; n--, rec++) {
        int bank = rec->bank;
        u_char *hdr = BANK_PTR(u_char *, bank, rec->tbl->partsHeader);
        u_char *ctrl;
        u_long animWord;
        AbsSignI32(rec->sign, (int *)(hdr + PARTS_HDR_ANIM_ABS), (int *)(hdr + PARTS_HDR_ANIM_SIGN));
        *(u_int *)(hdr + PARTS_HDR_ANIM_STEP) = rec->step;
        ExecuteAnimation(bank, rec->anim, rec->tbl, 0, 0, 1, 0, 0);
        *(u_int *)(hdr + PARTS_HDR_ANIM_STEP) = 0;
        ctrl = BANK_PTR(u_char *, bank, rec->tbl->ctrl);
        if (ctrl == NULL)
            continue;
        memcpy(&animWord, BANK_PTR(u_char *, bank, rec->anim), sizeof(animWord));
        if (animWord == ~(u_long)0) {
            u_char *list = BANK_PTR(u_char *, bank, *(u_int *)(ctrl + OBJ_CTRL_ATTACH_LIST));
            int count = ctrl[OBJ_CTRL_ATTACH_COUNT];
            int i;
            for (i = 0; i < count; i++) {
                u_char *e = list + i * OBJ_CTRL_ATTACH_SIZE;
                u_int parent = *(u_int *)(PARTS_REC(hdr, *(u_short *)(e + OBJ_CTRL_ATTACH_PART)) + EAS_REC_PARENT);
                if (parent != 0)
                    *(u_int *)(e + OBJ_CTRL_ATTACH_PARENT) = parent;
            }
        }
    }
    _SunFree(&gHeapJunc, (void **)&base);
    lpEAS = NULL;
}

// ---- shadows (CT3 0x01847330..0x01847c30, slice 2) ----

char iShadowTransMode;                  /* sbss 0x0193fa7c */
char iShadowTransModeFin;               /* sbss 0x0193fa78 */
int iPleaseShadowDraw;                  /* sbss 0x0193fa74 */
char iShadowTransOrderBW;               /* sbss 0x0193fa70 */
void *ShadowBlock[BANK_MAX];           /* PORT: BankInfo +0x58, the bank's gHeapJunc shadow block */
// The symtab's local; its other readers are this TU's shadow code (assumed: move it if not).
alignas(16) static float matrix_global_view_for_shadow[4][4]; /* bss 0x0193c840 */

#define CAMERA_SHARED BANK_SCENE_MAX    /* Cam[6]: the banks past the scene ones share it (asCheckBankID) */

// CT3 0x01847330
void asGetShadowMatrixGlobalView(void)
{
    memcpy(matrix_global_view_for_shadow, Cam[CAMERA_SHARED].view, sizeof(matrix_global_view_for_shadow));
}

// CT3 0x01847460
void asShadowDrawing_DetachLightPosition(int scene)
{
    if (scene >= 0)
        BankInfo[scene].shadowLightPos = 0;
}

// CT3 0x018474f0
// The shadow's on word (shadowVec w; asShadowDrawing_Disable clears it).
void asShadowDrawing_Enable(int scene)
{
    if (scene >= 0)
        BankInfo[scene].shadowVec[3] = 1.0f;
}

// CT3 0x01847520
void asShadowDrawing_Disable(int scene)
{
    BankInfo[scene].shadowVec[3] = 0.0f;
    BankInfo[scene].shadowColor[3] = 0xff;
}

// CT3 0x01847610
void asShadowDrawing_SetTransOrderBW(char on)
{
    iShadowTransOrderBW = on != 0;
}

// CT3 0x01847b10
void asShadowDrawing_Init(int scene)
{
    BANK_INFO *bi;
    if (scene < 0)
        return;
    bi = &BankInfo[scene];
    asShadowDrawing_DetachLightPosition(scene);
    bi->shadowVec[0] = bi->shadowVec[1] = bi->shadowVec[2] = 0.0f; /* sqc2 vf0 */
    bi->shadowVec[3] = 1.0f;
    asShadowDrawing_Disable(scene);
    bi->shadow58 = 0;
    ShadowBlock[scene] = NULL;          /* PORT: the word +0x58 holds */
    bi->shadow70 = 0;
    bi->shadowLength = 1000.0f;         /* the ROM's three stores, the last one stays */
    bi->shadowLength = 250.0f;
    bi->shadowLength = 500.0f;
    bi->shadowColor[0] = bi->shadowColor[1] = bi->shadowColor[2] = bi->shadowColor[3] = 0xff;
    bi->shadowOrder = 100;
    bi->shadow6d = 0;
    bi->shadow64 = 200.0f;
    bi->shadow68 = 100.0f;
    bi->shadowDraw = 0;
}

// CT3 0x01847c30
void Shadow_OnceInit(void)
{
    iShadowTransMode = 0;
    iShadowTransModeFin = 0;
    iPleaseShadowDraw = 0;
    iShadowTransOrderBW = 0;
}

// ---- mirrors (slice 2) ----

// CT3 0x0184fd00
// mrr_ReverseMatrix = identity (vf0 = (0,0,0,1), rotated by vmr32 into each row).
void asInitMirrorMatrix(void)
{
    memset(mrr_ReverseMatrix, 0, sizeof(mrr_ReverseMatrix));
    mrr_ReverseMatrix[0][0] = mrr_ReverseMatrix[1][1] = mrr_ReverseMatrix[2][2] = mrr_ReverseMatrix[3][3] = 1.0f;
}

// ---- seek bars (0x01842b20..0x01843270, slice 3, 2026-11-10) ----
//
// A scene's OBJ_CTRL records of CTRL_KIND_SEEKBAR become SEEKBARs at scene_init; every animated frame
// asSeekBarAnimation finds where a plane (a part's position: the bank's own, or a character's part by
// hash) crosses each bar, as t in [0, 1] along it, and starts the bar's animations at the frame t picks.

SEEKBAR_X *SeekBarX[BANK_MAX];          /* PORT: ALL_TABLE +0xe8 (the ROM's word) */

// out = m applied to v as the VU does it: m[0] v.x + m[1] v.y + m[2] v.z + m[3] v.w.
static void SeekBarApply(float out[4], const float m[4][4], const float v[4])
{
    int i;
    for (i = 0; i < 4; i++)
        out[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
}

// CT3 0x018430d0
void asSeekBarInitializeForX(int scene)
{
    ALL_TABLE *tbl = All_table_TBL[scene];
    u_char *ctrl;
    OBJ_CTRL *list;
    int i, n;
    if (scene < 0 || BankInfo[scene].type != BANK_TYPE_SHADOW || tbl == NULL || tbl->ctrl == 0)
        return;
    ctrl = BANK_PTR(u_char *, scene, tbl->ctrl);
    list = BANK_PTR(OBJ_CTRL *, scene, *(u_int *)(ctrl + OBJ_CTRL_LIST));
    if (list == NULL)
        return;
    n = 0;
    for (i = 0; i < ctrl[OBJ_CTRL_COUNT]; i++)
        if (list[i].kind == CTRL_KIND_SEEKBAR)
            n++;
    if (n < 1) {
        SeekBarX[scene] = NULL;
        return;
    }
    {
        SEEKBAR_X *x = (SEEKBAR_X *)_SunAlloc(&gHeapJunc, sizeof(SEEKBAR_X), 4);
        SeekBarX[scene] = x;
        x->count = n;
        x->bars = (SEEKBAR *)_SunAlloc(&gHeapJunc, n * (int)sizeof(SEEKBAR), 0x10);
        n = 0;
        for (i = 0; i < ctrl[OBJ_CTRL_COUNT]; i++) {
            SEEKBAR *bar;
            if (list[i].kind != CTRL_KIND_SEEKBAR)
                continue;
            bar = &x->bars[n++];
            bar->hash = 0;
            bar->planePart = 0;
            bar->chara = 0;
            bar->animCount = 0;
            bar->anims = NULL;
            bar->part = list[i].part;
            memcpy(bar->ends, BANK_PTR(float *, scene, list[i].ends), sizeof(bar->ends));
        }
    }
}

// CT3 0x01842fe0
void asSeekBarFreeForX(int scene)
{
    SEEKBAR_X *x;
    int i;
    if (scene < 0 || BankInfo[scene].type != BANK_TYPE_SHADOW || All_table_TBL[scene] == NULL)
        return;
    x = SeekBarX[scene];
    if (x == NULL)
        return;
    if (x->bars != NULL) {
        for (i = 0; i < x->count; i++)
            if (x->bars[i].anims != NULL)
                _SunFree(&gHeapJunc, (void **)&x->bars[i].anims);
        _SunFree(&gHeapJunc, (void **)&x->bars);
    }
    _SunFree(&gHeapJunc, (void **)&SeekBarX[scene]);
}

// CT3 0x01842b20
void asSeekBarAnimation(int scene)
{
    SEEKBAR_X *x;
    SEEKBAR *bar;
    u_char *hdr;
    int n;
    if (scene < 0 || BankInfo[scene].state != BANK_STATE_LOADED || BankInfo[scene].type != BANK_TYPE_SHADOW)
        return;
    x = SeekBarX[scene];
    if (x == NULL || x->count == 0)
        return;
    hdr = pModelPartsHeader[scene];
    for (n = x->count, bar = x->bars; n > 0; n--, bar++) {
        alignas(16) float a[4], b[4], d[4], dir[4], cross[4], plane[4];
        const MODEL_PARTS *part;
        SEEKBAR_ANIM *an;
        float len, t, s;
        int i, k;
        if (bar->anims == NULL)
            continue;
        part = &((const MODEL_PARTS *)hdr)[bar->part];
        if (part->flags & PARTS_FLAG_HIDDEN)
            continue;
        SeekBarApply(a, part->matrix, bar->ends[0]);
        SeekBarApply(b, part->matrix, bar->ends[1]);
        for (i = 0; i < 4; i++)
            d[i] = b[i] - a[i];
        len = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];  /* vmul, vaddbc twice; vrsqrt vf0w / sqrt */
        len = 1.0f / sqrtf(len);
        for (i = 0; i < 4; i++)
            dir[i] = d[i] * len;
        if (bar->chara == -1) {
            memcpy(plane, ((const MODEL_PARTS *)hdr)[bar->planePart].pos, sizeof(plane));
        } else {
            CHARACTER *chr = &Character[bar->chara];
            u_char *rec;
            if (!(chr->flags & CHARACTER_FLAG_REFRACTION))
                continue;
            rec = asFindModelParts(pModelPartsHeader[chr->bank], bar->hash);
            if (rec == NULL)
                continue;
            memcpy(plane, ((const MODEL_PARTS *)(rec - PARTS_RECORDS))->pos, sizeof(plane));
        }
        asGetCrossPointInPlaneWithLine(cross, a, b, dir, plane);
        for (i = 0; i < 4; i++)
            cross[i] -= a[i];
        if (d[0] != 0.0f)
            t = cross[0] / d[0];
        else if (d[1] != 0.0f)
            t = cross[1] / d[1];
        else if (d[2] != 0.0f)
            t = cross[2] / d[2];
        else
            t = 1.0f;
        if (t < 0.0f)
            t = 0.0f;
        else if (t > 1.0f)
            t = 1.0f;
        s = 1.0f - t;
        for (k = bar->animCount, an = bar->anims; k > 0; k--, an++) {
            int frame = (int)((float)an->frameB * t + (float)an->frameA * s); /* mula.s / madd.s */
            if (an->mode == SEEKBAR_ANIM_2)
                asSceneAnimationStart(1.0f, 0, All_table_TBL[scene], 0, frame, an->id, 0, 0, 1, 0, 1, 0, 1, 0);
            else if (an->mode == SEEKBAR_ANIM_1)
                asSceneAnimationStart(1.0f, 0, All_table_TBL[scene], 0, frame, an->id, 0, 1, 0, 0, 1, 0, 1, 0);
            else if (an->mode == SEEKBAR_ANIM_0)
                asSceneAnimationStart(1.0f, 0, All_table_TBL[scene], 0, frame, an->id, 1, 0, 0, 0, 1, 0, 1, 0);
        }
    }
}

// ---- seats (slice 3) ----

// CT3 0x018437c0
// Puts cell `cell` of row `row` on its grid square: the cell's part's translation (REC_BASE row 3) =
// the row's part's position + its matrix's x axis * (x + 1.5) * SEAT_SQUARE + its z axis * (y + 1.5)
// * SEAT_SQUARE (all four lanes). The ROM reads the current scene's table, not the caller's bank.
void asSeatArrange_ObjectMatchGlid(int row, int cell)
{
    int bank = idCurrentScene;
    u_int *seat = BANK_PTR(u_int *, bank, All_table_TBL[bank]->seat);
    u_char *header = BANK_PTR(u_char *, bank, All_table_TBL[bank]->partsHeader);
    u_char *r = BANK_PTR(u_char *, bank, seat[1]) + row * SEAT_ROW_SIZE;
    u_char *c = BANK_PTR(u_char *, bank, *(u_int *)(r + SEAT_ROW_CELLS)) + cell * SEAT_CELL_SIZE;
    u_char *base = PARTS_REC(header, *(u_short *)(r + SEAT_ROW_PART));
    float *t = (float *)(PARTS_REC(header, *(u_short *)(c + SEAT_CELL_PART)) + 0x2b0); /* REC_BASE row 3 */
    float fx = ((float)c[SEAT_CELL_X] + 1.5f) * SEAT_SQUARE;
    float fz = ((float)c[SEAT_CELL_Y] + 1.5f) * SEAT_SQUARE;
    const float *pos = (const float *)(base + 0xf0);    /* REC_POS */
    const float *ax = (const float *)(base + 0x180);    /* REC_MATRIX row 0 */
    const float *az = (const float *)(base + 0x1a0);    /* REC_MATRIX row 2 */
    int i;
    for (i = 0; i < 4; i++)
        t[i] = pos[i];
    for (i = 0; i < 4; i++)
        t[i] += ax[i] * fx;
    for (i = 0; i < 4; i++)
        t[i] += az[i] * fz;
}

// The 4 x 4 grid bytes from (x, y) take SEAT_GRID_TAKEN (asSeatArrange_FlameInit, unrolled in the ROM).
static void SeatMarkBlock(u_char *grid, int width, int x, int y)
{
    int at = x + y * width;
    int r, k;
    for (r = 0; r < 4; r++, at += width)
        for (k = 0; k < 4; k++)
            grid[at + k] |= SEAT_GRID_TAKEN;
}

// CT3 0x01843270
// The seat grid's frame: every grid byte keeps its low two bits (SEAT_GRID_KEEP), then each cell marks
// the 4 x 4 grid bytes at its position (SEAT_GRID_TAKEN), and a SEAT_CELL_TWO_PLACES cell those at its
// second position too. The marks go into row 0's grid with row 0's width, for every row (the ROM's).
void asSeatArrange_FlameInit(int scene)
{
    u_int *seat;
    u_char *rows;
    u_char *grid;
    int width;
    int i, j;
    if (BankInfo[scene].state != BANK_STATE_LOADED || All_table_TBL[scene]->seat == 0 || !asCheckBankType(scene))
        return;
    seat = BANK_PTR(u_int *, scene, All_table_TBL[scene]->seat);
    rows = BANK_PTR(u_char *, scene, seat[1]);
    grid = BANK_PTR(u_char *, scene, *(u_int *)(rows + SEAT_ROW_GRID));
    width = rows[SEAT_ROW_WIDTH];
    for (i = 0; i < (int)seat[0]; i++) {
        u_char *row = rows + i * SEAT_ROW_SIZE;
        u_char *g = BANK_PTR(u_char *, scene, *(u_int *)(row + SEAT_ROW_GRID));
        int n;
        for (n = *(u_short *)(row + SEAT_ROW_GRID_QWORDS) * 0x10; n != 0; n--, g++)
            *g &= SEAT_GRID_KEEP;
        for (j = 0; j < row[SEAT_ROW_COUNT]; j++) {
            u_char *cell = BANK_PTR(u_char *, scene, *(u_int *)(row + SEAT_ROW_CELLS)) + j * SEAT_CELL_SIZE;
            SeatMarkBlock(grid, width, cell[SEAT_CELL_X], cell[SEAT_CELL_Y]);
            if (cell[SEAT_CELL_PLACES] == SEAT_CELL_TWO_PLACES)
                SeatMarkBlock(grid, width, cell[SEAT_CELL_X2], cell[SEAT_CELL_Y2]);
        }
    }
}

// ---- searching parts (asSearchObject, slice 5, 2026-11-10) ----

#define REC_FLAGS 0x10                  /* u_int: MODEL_PARTS.flags (PARTS_FLAG_HIDDEN) */
#define REC_POS 0xf0                    /* float[4]: MODEL_PARTS.pos */

// v.xyz /= |v.xyz| (vrsqrt, vmulq.xyz: w is kept); answers |v.xyz| (the vdiv after it).
static float SearchNormalize(float v[4])
{
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    float q = 1.0f / len;
    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
    return 1.0f / q;
}

// SEARCH_IN_CONE when the direction from the eye is inside the cone about the axis (or there is no
// cone), SEARCH_IN_RANGE when the target is near enough (or there is no range).
static int SearchSees(const SEARCH_OBJ *so, float *axis, float *dir, float dist)
{
    float dot = gaInnerProductXYZ(axis, dir);
    float c = ahCos(so->halfAngle);
    int seen = 0;
    if (so->halfAngle == 0 || !(dot <= c))
        seen |= SEARCH_IN_CONE;
    if (so->range == 0 || dist < (float)so->range)
        seen |= SEARCH_IN_RANGE;
    return seen;
}

// CT3 0x01843950
// For each searching part whose part is shown and whose owner character is in use: gathers the
// targets (this bank's shown parts, the in-use characters' parts by hash), takes the nearest to the
// owner's eye part that is inside the cone and the range (gaSearchNearLight4, dropping each miss),
// else the rest part's position; moves the part towards it (straight at speed per 5 ticks, or turning
// about the eye by at most turnStep); sets the part's w to 1 while it sees its goal, else 0. The
// "nothing seen" flag is set once per call and never cleared (the ROM's: sp+0xb0 is zeroed once), so
// after one blind search every later part of the bank also gets w 0.
void asSearchObject(int scene)
{
    u_int *table;
    u_char *header;
    int blind = 0;
    int r;
    // callcheck: ahCos (SearchSees makes it)
    if (BankInfo[scene].state != BANK_STATE_LOADED)
        return;
    table = BANK_PTR(u_int *, scene, All_table_TBL[scene]->search);
    if (table == NULL)
        return;
    header = BANK_PTR(u_char *, scene, All_table_TBL[scene]->partsHeader);
    for (r = 0; r < (int)table[0]; r++) {
        SEARCH_OBJ *so = BANK_PTR(SEARCH_OBJ *, scene, table[1]) + r;
        u_char *self = PARTS_REC(header, so->part);
        float (*cand)[4] = BANK_PTR(float (*)[4], scene, so->candidates);
        CHARACTER *owner = &Character[so->chara];
        alignas(16) float axis[4], dir[4], toGoal[4];
        u_char *eye, *rest, *ownerHdr;
        u_char nearest[32];
        const short *list;
        int n = 0, k, seen = -1;
        float dist;

        if ((*(u_int *)(self + REC_FLAGS) & PARTS_FLAG_HIDDEN) || !(owner->flags & CHARACTER_FLAG_SHADOW))
            continue;
        list = BANK_PTR(const short *, scene, so->parts);
        for (k = 0; list[k] != -1; k++) {
            u_char *t = PARTS_REC(header, list[k]);
            if (!(*(u_int *)(t + REC_FLAGS) & PARTS_FLAG_HIDDEN)) {
                memcpy(cand[n], t + REC_POS, 0x10);
                *(u_int *)&cand[n][3] = SEARCH_CANDIDATE_W;
                n++;
            }
        }
        list = BANK_PTR(const short *, scene, so->charas);
        for (k = 0; list[k] != -1; k++) {
            CHARACTER *ch = &Character[list[k]];
            u_long hash;
            u_char *t;
            if (!(ch->flags & CHARACTER_FLAG_SHADOW))
                continue;
            memcpy(&hash, BANK_PTR(u_char *, scene, so->hashes) + k * 8, sizeof(hash));
            t = asFindModelParts(pModelPartsHeader[ch->bank], hash);
            if (t != NULL && !(*(u_int *)(t + REC_FLAGS) & PARTS_FLAG_HIDDEN)) {
                memcpy(cand[n], t + REC_POS, 0x10);
                *(u_int *)&cand[n][3] = SEARCH_CANDIDATE_W;
                n++;
            }
        }
        ownerHdr = pModelPartsHeader[owner->bank];
        eye = asFindModelParts(ownerHdr, so->eyeHash);
        rest = asFindModelParts(ownerHdr, so->restHash);
        if (eye == NULL || rest == NULL)
            continue;                   /* PORT: the ROM reads through them unchecked */
        for (k = 0; k < 4; k++)
            axis[k] = ((float *)(rest + REC_POS))[k] - ((float *)(eye + REC_POS))[k];
        SearchNormalize(axis);

        for (k = 0; n - k > 0; k++) {
            int i;
            gaSearchNearLight4(nearest, (float (*)[4])(eye + REC_POS), 1, cand, n - k);
            memcpy(so->goal, cand[nearest[0]], 0x10);
            so->goal[3] = 1.0f;
            for (i = 0; i < 4; i++)
                dir[i] = so->goal[i] - ((float *)(eye + REC_POS))[i];
            dist = SearchNormalize(dir);
            seen = SearchSees(so, axis, dir, dist);
            if (seen == SEARCH_SEEN)
                break;
            memcpy(cand[nearest[0]], cand[n - k - 1], 0x10);
        }
        if (seen != SEARCH_SEEN) {
            memcpy(so->goal, rest + REC_POS, 0x10);
            blind = 1;
        }

        if (so->turn == SEARCH_MOVE_STRAIGHT) {
            float step = gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
            int i;
            step = so->speed * step;
            for (i = 0; i < 4; i++)
                dir[i] = so->goal[i] - so->pos[i];
            if (SearchNormalize(dir) <= step) {
                memcpy(so->pos, so->goal, 0x10);
            } else {
                for (i = 0; i < 4; i++)
                    so->pos[i] += dir[i] * step;
            }
        } else {
            alignas(16) float m[4][4], cross[4], v[4];
            float la, lb;
            int angle, i;
            for (i = 0; i < 4; i++) {
                dir[i] = so->pos[i] - ((float *)(eye + REC_POS))[i];
                toGoal[i] = so->goal[i] - ((float *)(eye + REC_POS))[i];
            }
            la = SearchNormalize(dir);
            lb = SearchNormalize(toGoal);
            angle = ahAcos(gaInnerProductXYZ(dir, toGoal));
            (void)ahAcos(gaInnerProductXYZ(dir, axis)); /* the ROM's: its answer is not used */
            if (angle < so->turnStep) {
                memcpy(so->pos, so->goal, 0x10);
            } else {
                float len = la + (lb - la) / ((float)angle / (float)so->turnStep);
                // cross(toGoal, dir) (vopmula / vopmsub .xyz; w stays toGoal's), normalized
                cross[0] = toGoal[1] * dir[2] - dir[1] * toGoal[2];
                cross[1] = toGoal[2] * dir[0] - dir[2] * toGoal[0];
                cross[2] = toGoal[0] * dir[1] - dir[0] * toGoal[1];
                cross[3] = toGoal[3];
                SearchNormalize(cross);
                asMakeRotMatrixByQuartanion(m, cross, so->turnStep);
                for (i = 0; i < 4; i++)
                    v[i] = m[0][i] * dir[0] + m[1][i] * dir[1] + m[2][i] * dir[2] + m[3][i] * dir[3];
                for (i = 0; i < 4; i++)
                    so->pos[i] = ((float *)(eye + REC_POS))[i] + v[i] * len;
            }
        }
        memcpy(self + REC_POS, so->pos, 0x10);

        if (blind) {
            ((float *)(self + REC_POS))[3] = 0.0f;
        } else {
            int i;
            for (i = 0; i < 4; i++)
                dir[i] = so->pos[i] - ((float *)(eye + REC_POS))[i];
            dist = SearchNormalize(dir);
            ((float *)(self + REC_POS))[3] = SearchSees(so, axis, dir, dist) == SEARCH_SEEN ? 1.0f : 0.0f;
        }
    }
}

// CT3 0x018436a0
// Every cell of the scene's seat table back to its home position, matched to the grid, then the frame.
void asSeatArrange_Init(int scene)
{
    u_int *seat;
    int i, j;
    if (BankInfo[scene].state != BANK_STATE_LOADED)
        return;
    seat = BANK_PTR(u_int *, scene, All_table_TBL[scene]->seat);
    if (seat == NULL)
        return;
    for (i = 0; i < (int)seat[0]; i++) {
        u_char *row = BANK_PTR(u_char *, scene, seat[1]) + i * SEAT_ROW_SIZE;
        for (j = 0; j < row[SEAT_ROW_COUNT]; j++) {
            u_char *cell = BANK_PTR(u_char *, scene, *(u_int *)(row + SEAT_ROW_CELLS)) + j * SEAT_CELL_SIZE;
            cell[6] = cell[4] = cell[2];
            cell[7] = cell[5] = cell[3];
            asSeatArrange_ObjectMatchGlid(i, j);
        }
    }
    asSeatArrange_FlameInit(scene);
}

// ---- matrix trees (slice 3) ----

// The part record fields BMT_01 reads and writes (record offsets: PARTS_REC).
#define REC_KIND 0x2                    /* u_char: 1 = BMT_01 leaves the part alone */
#define REC_BITS 0x3                    /* u_char: BMT_01 clears REC_BIT_REBUILT */
#define REC_BIT_REBUILT 0x2              /* BuildMatrixTree: rebuilt this frame (its view pass clears it) */
#define REC_POS 0xf0                    /* float[4]: MODEL_PARTS.pos */
#define REC_MATRIX 0x180                /* float[4][4]: MODEL_PARTS.matrix */
#define REC_SCALE 0x2c0                 /* float[3]: the diagonal of the scale BMT_01 applies */
#define REC_GA_MATRIX 0x100             /* float[4][4]: the ga draw object's first matrix (DRAW_OBJ_MATRIX) */
#define REC_GA_MATRIX2 0x140            /* float[4][4]: its second (DRAW_OBJ_MATRIX2) */
#define REC_POS_PREV 0x2f0             /* float[4]: REC_POS copied by BMT_01 */
#define REC_DRAW_MATRIX 0x300           /* float[4][4]: view * matrix * scale */
#define REC_VIEW_MATRIX 0x340           /* float[4][4]: view * matrix */
#define REC_VIEW_HOLD 0x34c             /* float: != 0 keeps the view matrices (BMT_01 skips them);
                                           inside REC_VIEW_MATRIX's first row: its w */
#define PARTS_END(rec) (*(short *)(rec) < 0) /* the records end at a negative index */
#define PARTS_KIND_STATIC 1             /* REC_KIND: BMT_01 skips it */

// CT3 0x01850fc0
void BMT_01(u_char *partsHeader, int bank)
{
    alignas(16) float scale[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; /* 0x01937620 */
    u_char *rec;
    if (bank < 0)
        bank = -bank;
    for (rec = partsHeader + PARTS_RECORDS; !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        if (rec[REC_KIND] == PARTS_KIND_STATIC)
            continue;
        rec[REC_BITS] &= ~REC_BIT_REBUILT;
        memcpy(rec + REC_POS_PREV, rec + REC_POS, 0x10);
        if (*(float *)(rec + REC_VIEW_HOLD) == 0.0f) {
            scale[0][0] = ((float *)(rec + REC_SCALE))[0];
            scale[1][1] = ((float *)(rec + REC_SCALE))[1];
            scale[2][2] = ((float *)(rec + REC_SCALE))[2];
            gaMulMatrix((float (*)[4])(rec + REC_VIEW_MATRIX), Cam[asCheckBankID(bank)].view,
                        (float (*)[4])(rec + REC_MATRIX));
            gaMulMatrix((float (*)[4])(rec + REC_DRAW_MATRIX), (float (*)[4])(rec + REC_VIEW_MATRIX), scale);
        }
    }
}

// CT3 0x0184eed0
void ExecuteAnimationHistory_Free(void)
{
    if (lpEAS != NULL)
        _SunFree(&gHeapJunc, &lpEAS);
}

// ---- behaviours, shadows' end, mirrors, the force flag (slice 4, 2026-11-10) ----

// PORT: the bank whose host image holds `host` (the image starting nearest below it). The ROM passes
// a behaviour's address alone; its track words are EE addresses of that bank (decision h).
static int BankOfHostPtr(const void *host)
{
    const u_char *p = (const u_char *)host;
    int bank, best = -1;
    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankImage[bank] != NULL && BankImage[bank] <= p && (best < 0 || BankImage[bank] > BankImage[best]))
            best = bank;
    }
    return best;
}

// The track `t` of group `g` of a behaviour of bank `bank`.
static BEHAVIOR_TRACK *BehaviorTracks(int bank, const BEHAVIOR *bhv, int g)
{
    return BANK_PTR(BEHAVIOR_TRACK *, bank, bhv->groups[g].tracks);
}

// CT3 0x018497b0
// A track plays again from its first key; `resetTime` also clears its +0x1c.
void asStartAnimationFromTop(BEHAVIOR_TRACK *track, int resetTime)
{
    track->step = 1;
    track->playing = 1;
    track->time = 0;
    if (resetTime)
        track->time1c = 0;
}

// CT3 0x01847dc0
// Every track of the behaviour stops (NULL: none).
void asBehaviorStopAnimation(void *behavior)
{
    BEHAVIOR *bhv = (BEHAVIOR *)behavior;
    int bank, g, t;
    if (bhv == NULL)
        return;
    bank = BankOfHostPtr(bhv);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++)
            tr->playing = 0;
    }
}

// CT3 0x01847fd0
// scene_init's start: the tracks marked TRACK_FLAG_AUTO_START play from the top (and are marked
// TRACK_FLAG_STARTED); every track's +0xe becomes 100. The ROM tests no NULL here.
void asSceneAutoStartAnimation(void *behavior)
{
    BEHAVIOR *bhv = (BEHAVIOR *)behavior;
    int bank = BankOfHostPtr(bhv);
    int g, t;
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++) {
            if (tr->flags & TRACK_FLAG_AUTO_START) {
                if (!(tr->flags & TRACK_FLAG_STARTED))
                    tr->flags |= TRACK_FLAG_STARTED;
                asStartAnimationFromTop(tr, 1);
            }
            tr->playRate = TRACK_PLAY_RATE_FULL;
        }
    }
}

// CT3 0x01847bd0
// scene_end: the bank's shadow block goes back to gHeapJunc; the shadow is off.
void asShadowDrawing_Fin(int scene)
{
    BANK_INFO *bi = &BankInfo[scene];
    if (ShadowBlock[scene] != NULL)     /* ROM: the word BankInfo +0x58 */
        _SunFree(&gHeapJunc, &ShadowBlock[scene]);
    bi->shadow70 = 0;
}

// CT3 0x01851370
// 1 while matrices are forced (EV_FORCE_MATRIX_OPERATION, or time stopped): asDrawShadowVolumeLoop
// then draws no shadow volume.
int asGetForceMatrixOperationFlag(void)
{
    if (ssTrigCheckEvent(EV_FORCE_MATRIX_OPERATION, 1, 0) != 0 || time_stop_fream != 0)
        return 1;
    return 0;
}

// CT3 0x018443c0
// The shadow volumes of every bank asShadowDrawing_CheckTransMode hands out, once a frame
// (iPleaseShadowDraw) and only while no one else holds the effect buffer.
void asDrawShadowVolumeLoop(void)
{
    if (EffectBuffer_Use == 0 && iPleaseShadowDraw != 0) {
        int bank, last = -1;
        EffectBuffer_Use = 1;
        if (asShadowDrawing_CheckTransMode(SHADOW_TRANS_SORT) == 0) {
            asDrawShadowVolumeInit();
            asShadowStart();
        }
        bank = asShadowDrawing_CheckTransMode(SHADOW_TRANS_NEXT);
        while (bank != SHADOW_TRANS_END) {
            last = asGetForceMatrixOperationFlag();
            if (last == 0)
                asDrawShadowVolume(bank);
            last = bank;
            bank = asShadowDrawing_CheckTransMode(SHADOW_TRANS_NEXT);
        }
        if (asShadowDrawing_CheckTransMode(SHADOW_TRANS_FIN) == 0 && last != SHADOW_TRANS_END)
            asShadowEnd(last);
        EffectBuffer_Use = 0;
    }
    iPleaseShadowDraw = 0;
}

float vPVbk[4];                         /* sdata 0x01937740: the mirror's plane normal (asEnumMirror) */
float vPPbk[4];                         /* sdata 0x01937750: a point of the plane (asEnumMirror) */

// CT3 0x0184fb20
// The mirror plane asEnumMirror found last.
void asMirrorPlaneCalc(float *planeVec, float *planePoint)
{
    memcpy(planeVec, vPVbk, sizeof(vPVbk));
    memcpy(planePoint, vPPbk, sizeof(vPPbk));
}

// CT3 0x0184f330
// Flip every draw object's POLY_DISPLAY_ROTATE bit (its first byte, bit 0: the winding a mirror
// reverses) in ALL_TABLE +0x18's list and the 0-ended list that follows it.
// PORT: the lists and entries are EE words of the table's bank (decision h).
void asGAobjectControlPolygonDisplayRotateFlagReverse(ALL_TABLE *tbl)
{
    int bank = BankOfHostPtr(tbl);
    u_int *list = BANK_PTR(u_int *, bank, tbl->objOpaque);
    int pass;
    for (pass = 0; pass < 2; pass++) {
        for (; *list != 0; list++) {
            u_int *entry = BANK_PTR(u_int *, bank, *list);
            u_char *obj = BANK_PTR(u_char *, bank, entry[0]);
            *obj ^= POLY_DISPLAY_ROTATE;
        }
        list++;                         /* past the first list's 0 */
    }
}

// ---- the matrix tree (0x0184fd30, slice 4, 2026-11-10) ----
//
// A bank's parts are a tree in depth-first order (REC_DEPTH; LinkMatrixTree points each record's
// REC_PARENT at its parent's matrix, or at matUnit for a root). BuildMatrixTree walks it once a frame:
// every part's local matrix is parent * base; a part marked REC_BIT_DIRTY (or every part, mode 1)
// rebuilds its world matrix and position with the subtree under it; look-at parts turn to their target,
// linked parts take another bank's part as their base, lights keep their position / direction.
// Read from the disassembly (the decompiler drops gaMulMatrix arguments, hazards.md).

#define REC_DEPTH 0x0                   /* short: the part's depth in the tree, < 0 ends the records */
#define REC_FLAGS 0x10                  /* u_int: MODEL_PARTS.flags; bit 0 (REC_FLAG_HOLD) holds look-at,
                                           link and light parts as they are */
#define REC_FLAG_HOLD 0x1               /* the same bit as PARTS_FLAG_FOG; a link part sets it once */
#define REC_LIGHT_RANGE 0x1c            /* float: a light's w (BuildMatrixTree copies it to REC_LIGHT) */
#define REC_LPOS 0x20                   /* float[4]: the part's position in its parent's space */
#define REC_LINK_BANK 0x30              /* short: PARTS_KIND_LINK: the bank whose part it follows */
#define REC_LINK_PART 0x32              /* u_short: its part (0: look it up by REC_HASH once) */
#define REC_LINK_KEEP 0x34              /* u_short: PARTS_KIND_LINK: 0 = follow once (REC_FLAG_HOLD) */
#define REC_WATCH_AXIS 0x34             /* u_char: PARTS_KIND_WATCH: asMakeWatchMeMatrixToPosition's mode */
#define REC_WATCH_MODE 0x35             /* char: WATCH_* -- what the part looks at */
#define REC_WATCH_TARGET 0x36           /* short: WATCH_PART: a part of this bank; WATCH_CHARA_*: a
                                           character (WATCH_TARGET_CAMERA: the camera, -1: none) */
#define REC_HASH 0x38                   /* u_long: the part's name hash (MODEL_PARTS.hash) */
#define REC_PREV_SCALE_X 0x4c           /* float x3 (+0x10 apart): REC_SCALE as the last negative-bank
                                           pass left it; a link part's children draw with it */
#define REC_BLEND 0x7c                  /* float: 1 = world = local * REC_OFFSET, else world = local */
#define REC_PARENT 0x8c                 /* u_int: EE address of the parent's matrix (REC_PARENT_EE) */
#define REC_MATRIX_PREV 0x1c0           /* float[4][4]: REC_MATRIX as the last negative-bank pass left it */
#define REC_LOCAL 0x200                 /* float[4][4]: parent * REC_BASE */
#define REC_OFFSET 0x240                /* float[4][4]: applied after REC_LOCAL when REC_BLEND is 1 */
#define REC_BASE 0x280                  /* float[4][4]: the part's own transform (a link part: its target's) */
#define REC_LIGHT 0x2e0                 /* float[4]: a light's position (w its range) or direction (w 1) */
#define REC_BIT_DIRTY 0x1               /* REC_BITS: rebuild this part and its subtree */

#define PARTS_KIND_OMNI 4               /* a point light: REC_LIGHT = its position, OmniRangeChange */
#define PARTS_KIND_DIRECTIONAL 5        /* a directional light: REC_LIGHT = -(matrix row 1) */
#define PARTS_KIND_WATCH 0x14           /* turns to look at REC_WATCH_MODE's target */
#define PARTS_KIND_NO_POS 0x17          /* keeps REC_POS when rebuilt */
#define PARTS_KIND_LINK 0x1e            /* takes another bank's part as its base (REC_LINK_*) */

#define WATCH_PART 0                    /* REC_WATCH_TARGET: a part of this bank (its MODEL_PARTS.pos) */
#define WATCH_CHARA_PART_1 1            /* a character's part of the same hash (WATCH_TARGET_CAMERA: the camera) */
#define WATCH_CHARA_SELF 2              /* the character whose bank this is: its light bank's part of the
                                           same hash, if that part's w is not 0 */
#define WATCH_CHARA_PART_3 3            /* as WATCH_CHARA_PART_1 */
#define WATCH_TARGET_CAMERA 1000
#define WATCH_TARGET_NONE (-1)

#define BMT_NO_LINK 99999999            /* the ROM's "no link part above" depth */

// PORT: a part's REC_PARENT word as a host matrix: matUnit for a root (LinkMatrixTree's), else a
// matrix of the same bank (decision h).
static float (*PartsParentMatrix(int bank, u_int ee))[4]
{
    if (ee == MATUNIT_EE)
        return matUnit;
    return BANK_PTR(float (*)[4], bank, ee);
}

#define RF(rec, off) ((float *)((rec) + (off)))
#define RM(rec, off) ((float (*)[4])((rec) + (off)))

// v' = m applied to v (vmulax / vmadday / vmaddaz / vmaddw), all four lanes.
static void BmtApply(float out[4], float (*m)[4], const float v[4])
{
    float r[4];
    int i;
    for (i = 0; i < 4; i++)
        r[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    memcpy(out, r, sizeof(r));
}

// The look-at target of a PARTS_KIND_WATCH part; 0 when there is none (the part keeps its matrix).
static int BmtWatchTarget(u_char *header, u_char *rec, int bank, float target[4])
{
    short t = *(short *)(rec + REC_WATCH_TARGET);
    u_long hash = *(u_long *)(rec + REC_HASH);
    u_char *part;
    int i, c;
    target[0] = target[1] = target[2] = 0.0f; /* 0x01937730 */
    target[3] = 1.0f;
    switch ((signed char)rec[REC_WATCH_MODE]) {
    case WATCH_CHARA_SELF:
        for (c = -1, i = 0; i < CHARACTER_MAX; i++) {
            if ((short)bank == Character[i].bank) {
                c = i;
                break;
            }
        }
        if (c == -1 || All_table_TBL[Character[c].lightBank] == NULL)
            return 0;
        part = asFindModelParts(BANK_PTR(u_char *, Character[c].lightBank,
                                         All_table_TBL[Character[c].lightBank]->partsHeader), hash);
        if (part == NULL)
            return 0;
        memcpy(target, part + REC_POS, 0x10);
        return target[3] != 0.0f;
    case WATCH_CHARA_PART_3:
    case WATCH_CHARA_PART_1:
        if (t == WATCH_TARGET_CAMERA) {
            asViewMatrix2PosRUF(&Cam[asCheckBankID(bank)].view[0][0], target, NULL, NULL, NULL);
            return 1;
        }
        if (t == WATCH_TARGET_NONE)
            return 1;
        if (t < 0 || Character[t].bank < 1 || All_table_TBL[Character[t].bank] == NULL)
            return 0;
        part = asFindModelParts(BANK_PTR(u_char *, Character[t].bank, All_table_TBL[Character[t].bank]->partsHeader),
                                hash);
        if (part == NULL)
            return 0;
        memcpy(target, part + REC_POS, 0x10);
        return 1;
    case WATCH_PART:
        memcpy(target, ((MODEL_PARTS *)header)[t].pos, 0x10);
        return 1;
    default:
        return 1;
    }
}

// CT3 0x0184fd30
// (scene, mode, pass): (0, 1) is BMT_01, (2, 0) BMT_20; otherwise mode 1 rebuilds every part and
// pass != 0 also refreshes the view / draw matrices of what moved. A negative scene keeps this
// frame's matrices and scale as the "previous" ones (REC_MATRIX_PREV, REC_PREV_SCALE_X).
void BuildMatrixTree(int scene, int mode, int pass)
{
    u_char *header = pModelPartsHeader[absi(scene)];
    alignas(16) float scale[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };     /* 0x019376b0 */
    alignas(16) float prevScale[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; /* 0x019376f0 */
    alignas(16) float savedPos[4] = { 0, 0, 0, 0 };
    short dirtyDepth = -1, movedDepth = -1;
    int linkDepth = BMT_NO_LINK;
    u_char *linkRec = NULL;
    int keep, rebuild, view, bank;
    u_char *rec;

    // callcheck: BMT_01, BMT_20, asFindModelParts, asViewMatrix2PosRUF (BmtWatchTarget makes the last two; the first two are the calls just below, which the parser does not pick up)
    if ((char)mode == 0 && (char)pass == 1) {
        BMT_01(header, scene);
        return;
    }
    if ((char)mode == 2 && (char)pass == 0) {
        BMT_20(header, scene);
        return;
    }
    keep = scene < 0;
    bank = keep ? -scene : scene;
    rebuild = (char)mode == 1;
    view = (char)pass == 1;
    for (rec = header + PARTS_RECORDS; !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        short depth = *(short *)(rec + REC_DEPTH);
        float (*local)[4] = RM(rec, REC_LOCAL);
        float (*world)[4] = RM(rec, REC_MATRIX);
        if ((char)pass != 0)
            memcpy(savedPos, rec + REC_POS, 0x10);
        if ((signed char)rec[REC_KIND] == PARTS_KIND_STATIC) {
            // A point on its parent: only its position, and only on a full rebuild.
            if ((rebuild || view) && (char)mode != 0)
                BmtApply(RF(rec, REC_POS), PartsParentMatrix(bank, *(u_int *)(rec + REC_PARENT)), RF(rec, REC_LPOS));
            continue;
        }
        scale[0][0] = RF(rec, REC_SCALE)[0];
        scale[1][1] = RF(rec, REC_SCALE)[1];
        scale[2][2] = RF(rec, REC_SCALE)[2];
        gaMulMatrix(local, PartsParentMatrix(bank, *(u_int *)(rec + REC_PARENT)), RM(rec, REC_BASE));
        if (rebuild && depth <= dirtyDepth)
            rebuild = 0;                /* past the dirty subtree */
        if (rec[REC_BITS] & REC_BIT_DIRTY) {
            rec[REC_BITS] &= ~REC_BIT_DIRTY;
            if (!rebuild) {
                dirtyDepth = depth;
                rebuild = 1;
            }
        }

        if ((signed char)rec[REC_KIND] == PARTS_KIND_WATCH && !(*(u_int *)(rec + REC_FLAGS) & REC_FLAG_HOLD)) {
            alignas(16) float target[4];
            alignas(16) float m[4][4];
            if (!rebuild) {
                dirtyDepth = depth;
                rebuild = 1;
            }
            if (BmtWatchTarget(header, rec, bank, target)) {
                asMakeWatchMeMatrixToPosition(m, local, target, rec[REC_WATCH_AXIS]);
                memcpy(local, m, sizeof(m));
            }
        }

        if ((signed char)rec[REC_KIND] == PARTS_KIND_LINK && !(*(u_int *)(rec + REC_FLAGS) & REC_FLAG_HOLD)) {
            short lb = *(short *)(rec + REC_LINK_BANK);
            if (!rebuild) {
                dirtyDepth = depth;
                rebuild = 1;
            }
            if (linkDepth == BMT_NO_LINK)
                linkDepth = depth;
            if (lb >= 0 && BankInfo[lb].state == BANK_STATE_LOADED) {
                u_char *lrec;
                if (*(u_short *)(rec + REC_LINK_PART) == 0)
                    *(u_short *)(rec + REC_LINK_PART) =
                        (u_short)asFindModelPartsID(pModelPartsHeader[lb], *(u_long *)(rec + REC_HASH));
                if (*(u_short *)(rec + REC_LINK_KEEP) == 0)
                    *(u_int *)(rec + REC_FLAGS) |= REC_FLAG_HOLD;
                lrec = pModelPartsHeader[lb] + *(u_short *)(rec + REC_LINK_PART) * sizeof(MODEL_PARTS) + PARTS_RECORDS;
                linkRec = lrec;
                memcpy(rec + REC_BASE, lrec + REC_MATRIX_PREV, 0x40);
                BmtApply(RM(rec, REC_BASE)[3], RM(lrec, REC_MATRIX_PREV), RF(lrec, REC_LPOS));
                gaMulMatrix(local, PartsParentMatrix(bank, *(u_int *)(rec + REC_PARENT)), RM(rec, REC_BASE));
            }
        } else if (linkDepth < depth && linkRec != NULL) {
            // Under a link part: world and view now, drawn with the previous scale.
            if (*RF(rec, REC_BLEND) == 1.0f)
                gaMulMatrix(world, local, RM(rec, REC_OFFSET));
            else
                memcpy(world, local, 0x40);
            prevScale[0][0] = *RF(rec, REC_PREV_SCALE_X);
            prevScale[1][1] = *RF(rec, REC_PREV_SCALE_X + 0x10);
            prevScale[2][2] = *RF(rec, REC_PREV_SCALE_X + 0x20);
            gaMulMatrix(RM(rec, REC_VIEW_MATRIX), Cam[asCheckBankID(bank)].view, world);
            gaMulMatrix(RM(rec, REC_DRAW_MATRIX), RM(rec, REC_VIEW_MATRIX), prevScale);
            if ((signed char)rec[REC_KIND] == PARTS_KIND_DIRECTIONAL) {
                int i;
                for (i = 0; i < 4; i++)
                    RF(rec, REC_LIGHT)[i] = world[1][i] * -1.0f;
                RF(rec, REC_LIGHT)[3] = 1.0f;
            } else if ((signed char)rec[REC_KIND] == PARTS_KIND_OMNI) {
                BmtApply(RF(rec, REC_LIGHT), world, RF(linkRec, REC_LPOS));
                RF(rec, REC_LIGHT)[3] = *RF(rec, REC_LIGHT_RANGE);
            }
        } else {
            linkDepth = BMT_NO_LINK;
        }

        if (rebuild) {
            rec[REC_BITS] |= REC_BIT_REBUILT;
            if (*RF(rec, REC_BLEND) == 1.0f)
                gaMulMatrix(world, local, RM(rec, REC_OFFSET));
            else
                memcpy(world, local, 0x40);
            if ((signed char)rec[REC_KIND] != PARTS_KIND_NO_POS)
                BmtApply(RF(rec, REC_POS), world, RF(rec, REC_LPOS));
            if ((signed char)rec[REC_KIND] == PARTS_KIND_DIRECTIONAL) {
                int i;
                for (i = 0; i < 4; i++)
                    RF(rec, REC_LIGHT)[i] = world[1][i] * -1.0f;
                RF(rec, REC_LIGHT)[3] = 1.0f;
            } else if ((signed char)rec[REC_KIND] == PARTS_KIND_OMNI) {
                memcpy(rec + REC_LIGHT, rec + REC_POS, 0x10);
                RF(rec, REC_LIGHT)[3] = *RF(rec, REC_LIGHT_RANGE);
            }
        }
        if (keep) {
            memcpy(rec + REC_MATRIX_PREV, world, 0x40);
            *RF(rec, REC_PREV_SCALE_X) = RF(rec, REC_SCALE)[0];
            *RF(rec, REC_PREV_SCALE_X + 0x10) = RF(rec, REC_SCALE)[1];
            *RF(rec, REC_PREV_SCALE_X + 0x20) = RF(rec, REC_SCALE)[2];
        }
        if ((signed char)rec[REC_KIND] == PARTS_KIND_OMNI && !(*(u_int *)(rec + REC_FLAGS) & REC_FLAG_HOLD))
            OmniRangeChange(rec);

        if (view && depth <= movedDepth)
            view = 0;
        if ((rec[REC_BITS] & REC_BIT_REBUILT) && (char)pass != 0) {
            rec[REC_BITS] &= ~REC_BIT_REBUILT;
            if (!view) {
                movedDepth = depth;
                view = 1;
            }
        }
        if (view) {
            memcpy(rec + REC_POS_PREV, savedPos, 0x10);
            if (*RF(rec, REC_VIEW_HOLD) == 0.0f) {
                gaMulMatrix(RM(rec, REC_VIEW_MATRIX), Cam[asCheckBankID(bank)].view, world);
                gaMulMatrix(RM(rec, REC_DRAW_MATRIX), RM(rec, REC_VIEW_MATRIX), scale);
            }
        }
    }
}
// CT3 0x01850770
// BuildMatrixTree(bank, 2, 0): the matrix tree without the view pass -- only what is dirty, a watch
// part or a link part (and its subtree) is rebuilt; parts under a link part also get their view and
// draw matrices (with the previous scale). A negative bank keeps this frame's matrices as the previous
// ones. Read from the disassembly beside BuildMatrixTree's; its one difference: a watch part turns
// only when its target's w is not 0 (0x01850af8), for every target kind.
void BMT_20(u_char *partsHeader, int bank)
{
    alignas(16) float prevScale[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; /* 0x01937660 */
    short dirtyDepth = -1;
    int linkDepth = BMT_NO_LINK;
    u_char *linkRec = NULL;
    int rebuild = 0;
    int keep = bank < 0;
    u_char *rec;

    // callcheck: asFindModelParts, asViewMatrix2PosRUF (BmtWatchTarget makes them)
    if (keep)
        bank = -bank;
    for (rec = partsHeader + PARTS_RECORDS; !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        short depth = *(short *)(rec + REC_DEPTH);
        float (*local)[4] = RM(rec, REC_LOCAL);
        float (*world)[4] = RM(rec, REC_MATRIX);
        if ((signed char)rec[REC_KIND] == PARTS_KIND_STATIC) {
            if (rebuild)
                BmtApply(RF(rec, REC_POS), PartsParentMatrix(bank, *(u_int *)(rec + REC_PARENT)), RF(rec, REC_LPOS));
            continue;
        }
        gaMulMatrix(local, PartsParentMatrix(bank, *(u_int *)(rec + REC_PARENT)), RM(rec, REC_BASE));
        if (rebuild && depth <= dirtyDepth)
            rebuild = 0;
        if (rec[REC_BITS] & REC_BIT_DIRTY) {
            rec[REC_BITS] &= ~REC_BIT_DIRTY;
            if (!rebuild) {
                dirtyDepth = depth;
                rebuild = 1;
            }
        }

        if ((signed char)rec[REC_KIND] == PARTS_KIND_WATCH && !(*(u_int *)(rec + REC_FLAGS) & REC_FLAG_HOLD)) {
            alignas(16) float target[4];
            alignas(16) float m[4][4];
            if (!rebuild) {
                dirtyDepth = depth;
                rebuild = 1;
            }
            if (BmtWatchTarget(partsHeader, rec, bank, target) && target[3] != 0.0f) {
                asMakeWatchMeMatrixToPosition(m, local, target, rec[REC_WATCH_AXIS]);
                memcpy(local, m, sizeof(m));
            }
        }

        if ((signed char)rec[REC_KIND] == PARTS_KIND_LINK && !(*(u_int *)(rec + REC_FLAGS) & REC_FLAG_HOLD)) {
            short lb = *(short *)(rec + REC_LINK_BANK);
            if (!rebuild) {
                dirtyDepth = depth;
                rebuild = 1;
            }
            if (linkDepth == BMT_NO_LINK)
                linkDepth = depth;
            if (lb >= 0 && BankInfo[lb].state == BANK_STATE_LOADED) {
                u_char *lrec;
                if (*(u_short *)(rec + REC_LINK_PART) == 0)
                    *(u_short *)(rec + REC_LINK_PART) =
                        (u_short)asFindModelPartsID(pModelPartsHeader[lb], *(u_long *)(rec + REC_HASH));
                if (*(u_short *)(rec + REC_LINK_KEEP) == 0)
                    *(u_int *)(rec + REC_FLAGS) |= REC_FLAG_HOLD;
                lrec = pModelPartsHeader[lb] + *(u_short *)(rec + REC_LINK_PART) * sizeof(MODEL_PARTS) + PARTS_RECORDS;
                linkRec = lrec;
                memcpy(rec + REC_BASE, lrec + REC_MATRIX_PREV, 0x40);
                BmtApply(RM(rec, REC_BASE)[3], RM(lrec, REC_MATRIX_PREV), RF(lrec, REC_LPOS));
                gaMulMatrix(local, PartsParentMatrix(bank, *(u_int *)(rec + REC_PARENT)), RM(rec, REC_BASE));
            }
        } else if (linkDepth < depth && linkRec != NULL) {
            if (*RF(rec, REC_BLEND) == 1.0f)
                gaMulMatrix(world, local, RM(rec, REC_OFFSET));
            else
                memcpy(world, local, 0x40);
            prevScale[0][0] = *RF(rec, REC_PREV_SCALE_X);
            prevScale[1][1] = *RF(rec, REC_PREV_SCALE_X + 0x10);
            prevScale[2][2] = *RF(rec, REC_PREV_SCALE_X + 0x20);
            gaMulMatrix(RM(rec, REC_VIEW_MATRIX), Cam[asCheckBankID(bank)].view, world);
            gaMulMatrix(RM(rec, REC_DRAW_MATRIX), RM(rec, REC_VIEW_MATRIX), prevScale);
            if ((signed char)rec[REC_KIND] == PARTS_KIND_DIRECTIONAL) {
                int i;
                for (i = 0; i < 4; i++)
                    RF(rec, REC_LIGHT)[i] = world[1][i] * -1.0f;
                RF(rec, REC_LIGHT)[3] = 1.0f;
            } else if ((signed char)rec[REC_KIND] == PARTS_KIND_OMNI) {
                BmtApply(RF(rec, REC_LIGHT), world, RF(linkRec, REC_LPOS));
                RF(rec, REC_LIGHT)[3] = *RF(rec, REC_LIGHT_RANGE);
            }
        } else {
            linkDepth = BMT_NO_LINK;
        }

        if (rebuild) {
            rec[REC_BITS] |= REC_BIT_REBUILT;
            if (*RF(rec, REC_BLEND) == 1.0f)
                gaMulMatrix(world, local, RM(rec, REC_OFFSET));
            else
                memcpy(world, local, 0x40);
            if ((signed char)rec[REC_KIND] != PARTS_KIND_NO_POS)
                BmtApply(RF(rec, REC_POS), world, RF(rec, REC_LPOS));
            if ((signed char)rec[REC_KIND] == PARTS_KIND_DIRECTIONAL) {
                int i;
                for (i = 0; i < 4; i++)
                    RF(rec, REC_LIGHT)[i] = world[1][i] * -1.0f;
                RF(rec, REC_LIGHT)[3] = 1.0f;
            } else if ((signed char)rec[REC_KIND] == PARTS_KIND_OMNI) {
                memcpy(rec + REC_LIGHT, rec + REC_POS, 0x10);
                RF(rec, REC_LIGHT)[3] = *RF(rec, REC_LIGHT_RANGE);
            }
            if (keep) {
                memcpy(rec + REC_MATRIX_PREV, world, 0x40);
                *RF(rec, REC_PREV_SCALE_X) = RF(rec, REC_SCALE)[0];
                *RF(rec, REC_PREV_SCALE_X + 0x10) = RF(rec, REC_SCALE)[1];
                *RF(rec, REC_PREV_SCALE_X + 0x20) = RF(rec, REC_SCALE)[2];
            }
        }
        if ((signed char)rec[REC_KIND] == PARTS_KIND_OMNI && !(*(u_int *)(rec + REC_FLAGS) & REC_FLAG_HOLD))
            OmniRangeChange(rec);
    }
}
#undef RF
#undef RM

// ---- the straws (CT3 0x018497d0, slice 6, 2026-11-10) ----
//
// A straw is a bendable joint between two parts (A, B) of a bank: asMakeStraw skins the ga objects
// hung on it every frame (asUpDateGAobjectsAnimation). Each vertex carries a weight byte w (0..255):
// w 0 follows A, w 255 follows B, the rest a frame between them -- the rotation slerped from A's to
// B's, the origin on a cubic Bezier from A's joint point to B's whose handles run along A's +Y and B's
// -Y (STRAW.bend / 4 long). A squeezed straw (STRAW_SQUEEZE) first takes the twist out of B
// (asDelSqueezeInformation answers it), then gives it back per vertex: a second weight byte scales
// it, turned about the blended frame's Y axis (asMakeRotMatrixByQuartanion).
// Read from the disassembly (the decompiler drops the VU0 broadcast fields). Every matrix product is
// the VU's: each row v of the left matrix becomes m[0] v.x + m[1] v.y + m[2] v.z + m[3] v.w.

#define REC_LPOS 0x20                   /* float[4]: the joint point (the part's position in its parent) */
#define REC_MATRIX_PREV 0x1c0           /* float[4][4]: the part's world matrix (BuildMatrixTree's keep) */
#define STRAW_SQUEEZE 0x8000            /* STRAW.partA: take B's twist out and spread it by weight */
#define STRAW_PART 0x7fff
#define STRAW_WEIGHTS 0x100             /* weight bytes: one cached matrix each (SkinWork, 0x40 apart) */
#define STRAW_W_A 0                     /* the weight that follows part A (identity, or A's matrix) */
#define STRAW_W_B 0xff                  /* the weight that follows part B */

// The bank's straw table (ALL_TABLE.straw): count records of 0x10 bytes. Bank data, EE layout.
typedef struct STRAW_TABLE {
    u_int straws;                       /* 0x0: EE address of the STRAW records */
    u_short count;                      /* 0x4 */
} STRAW_TABLE;
static_assert(offsetof(STRAW_TABLE, count) == 0x4, "STRAW_TABLE.count");
typedef struct STRAW {
    u_short partA;                      /* 0x0: part index | STRAW_SQUEEZE */
    u_short partB;                      /* 0x2 */
    float bend;                         /* 0x4: the Bezier handles are bend / 4 long */
    float stretch;                      /* 0x8: B's joint frame sits stretch along A's +Y */
    u_int skins;                        /* 0xc: EE address of STRAW_SKIN entries, ended by obj == 0 */
} STRAW;
static_assert(sizeof(STRAW) == 0x10, "STRAW");
// One ga object skinned by the straw: its source vertices and normals, one 4-byte weight per vertex
// (byte 1: the blend weight, byte 2: the squeeze share; both signed, + 0x80).
typedef struct STRAW_SKIN {
    u_int obj;                          /* 0x0: the ga draw object (count +0x48, out +0x60 / +0x64) */
    u_int srcPos;                       /* 0x4: float[4] per vertex */
    u_int srcNrm;                       /* 0x8: float[4] per vertex */
    u_int weights;                      /* 0xc: 4 bytes per vertex */
} STRAW_SKIN;
static_assert(sizeof(STRAW_SKIN) == 0x10, "STRAW_SKIN");
#define GAOBJ_VERTEX_COUNT 0x48         /* short */
#define GAOBJ_VERTICES 0x60             /* u_int EE: float[4] per vertex, written by asMakeStraw */
#define GAOBJ_NORMALS 0x64              /* u_int EE: a 0x10 header, then float[4] per vertex */

static void StrawApply(float out[4], const float m[4][4], const float v[4])
{
    float r[4];
    int i;
    for (i = 0; i < 4; i++)
        r[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    memcpy(out, r, sizeof(r));
}

// out = rows * m (each row through StrawApply); out may be rows.
static void StrawMulRows(float out[4][4], const float rows[4][4], const float m[4][4])
{
    int r;
    for (r = 0; r < 4; r++)
        StrawApply(out[r], m, rows[r]);
}

// The frame a weight between 0 and 255 exclusive follows: the rotation slerped from A's (qA) to B's
// (qB) at w / 255, the origin on the Bezier ctrl, then A's rows with the stretch row scaled by the weight.
static void StrawBlendFrame(float out[4][4], int w, const float (*matA)[4], const float qA[4],
                            const float qB[4], const float ctrl[4][4], const float stretch[4],
                            const float invRot[4][4], float scaledStretch[4])
{
    float ts[2], q[4], frame[4][4], bez[4], rows[4][4];
    int i;
    ts[1] = (float)w / 255.0f;
    ts[0] = 1.0f - ts[1];
    for (i = 0; i < 3; i++)
        scaledStretch[i] = stretch[i] * ts[1];
    scaledStretch[3] = stretch[3];
    QuatSlerp(q, (float *)qA, (float *)qB, ts);
    ConvertQuat2Matrix(frame, q);
    asGetBeziePosition(ts[0], ts[1], bez, ctrl);
    for (i = 0; i < 3; i++)
        frame[3][i] += bez[i];
    memcpy(rows, matA, 0x30);
    for (i = 0; i < 3; i++)
        rows[3][i] = 0.0f - scaledStretch[i];
    rows[3][3] = scaledStretch[3];
    StrawMulRows(out, rows, invRot);
    StrawMulRows(out, out, frame);
}

// CT3 0x018497d0
// PORT: bank in front (straw and partsHeader are its EE words, decision h).
void asMakeStraw(int bank, u_int straw, u_int partsHeader)
{
    STRAW_TABLE *tbl = BANK_PTR(STRAW_TABLE *, bank, straw);
    u_char *hdr = BANK_PTR(u_char *, bank, partsHeader);
    float (*cache)[4][4] = (float (*)[4][4])SkinWork;
    short s;
    // callcheck: ConvertQuat2Matrix, QuatSlerp, asGetBeziePosition (the ROM inlines the blend twice,
    // once per loop: StrawBlendFrame makes these calls)
    if (tbl == NULL)
        return;
    for (s = 0; s < (int)tbl->count; s++) {
        STRAW *st = BANK_PTR(STRAW *, bank, tbl->straws) + s;
        u_char *recA = PARTS_REC(hdr, st->partA & STRAW_PART);
        u_char *recB = PARTS_REC(hdr, st->partB);
        float (*matA)[4] = (float (*)[4])(recA + REC_MATRIX_PREV);
        float (*matB)[4] = (float (*)[4])(recB + REC_MATRIX_PREV);
        float ctrl[4][4], qA[4], qB[4], matB2[4][4], inv[4][4], invRot[4][4];
        float stretch[4], scaled[4], handle;
        u_char done[STRAW_WEIGHTS];
        int squeeze = (st->partA & STRAW_SQUEEZE) ? 1 : 0;
        STRAW_SKIN *skin;
        int i;

        StrawApply(ctrl[0], matA, (float *)(recA + REC_LPOS));
        StrawApply(ctrl[3], matB, (float *)(recB + REC_LPOS));
        ConvertMatrix2Quat(qA, matA);
        if (squeeze == 1)
            squeeze = -asDelSqueezeInformation(matB2, matA, matB);
        else
            memcpy(matB2, matB, sizeof(matB2));
        ConvertMatrix2Quat(qB, matB2);
        handle = st->bend * 0.25f;
        for (i = 0; i < 3; i++)
            stretch[i] = matA[1][i] * st->stretch;
        stretch[3] = 1.0f;
        memcpy(ctrl[1], ctrl[0], 0x10);
        memcpy(ctrl[2], ctrl[3], 0x10);
        for (i = 0; i < 3; i++) {
            ctrl[1][i] = matA[1][i] * handle + ctrl[0][i];
            ctrl[2][i] = (0.0f - matB2[1][i]) * handle + ctrl[3][i];
        }
        ssCalcInversMatrix2(inv, matA);
        memcpy(invRot, inv, sizeof(invRot));
        invRot[3][0] = invRot[3][1] = invRot[3][2] = 0.0f;
        invRot[3][3] = 1.0f;
        memset(done, 0, sizeof(done));

        for (skin = BANK_PTR(STRAW_SKIN *, bank, st->skins); skin->obj != 0; skin++) {
            STRAW_SKIN *sk = skin;
            u_char *obj = BANK_PTR(u_char *, bank, sk->obj);
            short n = *(short *)(obj + GAOBJ_VERTEX_COUNT);
            float (*dstPos)[4] = BANK_PTR(float (*)[4], bank, *(u_int *)(obj + GAOBJ_VERTICES));
            float (*dstNrm)[4] = (float (*)[4])(BANK_PTR(u_char *, bank, *(u_int *)(obj + GAOBJ_NORMALS)) + 0x10);
            float (*srcPos)[4] = BANK_PTR(float (*)[4], bank, sk->srcPos);
            float (*srcNrm)[4] = BANK_PTR(float (*)[4], bank, sk->srcNrm);
            signed char *wt = BANK_PTR(signed char *, bank, sk->weights);
            short v;
            for (v = 0; v < n; v++, wt += 4) {
                short w = (short)(wt[1] + 0x80);
                float (*m)[4] = cache[w];
                int twist = squeeze != 0 ? (squeeze * (wt[2] + 0x80)) / 0xff : 0;
                if (!done[w]) {
                    done[w] = 1;
                    if (w == STRAW_W_A) {
                        if (squeeze == 0) {
                            memcpy(m, matUnit, 0x40);
                        } else {
                            memcpy(m, matA, 0x40);
                        }
                    } else {
                        float rows[4][4];
                        if (w == STRAW_W_B) {
                            memcpy(rows, matA, 0x30);
                            for (i = 0; i < 3; i++)
                                rows[3][i] = 0.0f - stretch[i];
                            rows[3][3] = stretch[3];
                            StrawMulRows(m, rows, invRot);
                            StrawMulRows(m, m, matB2);
                        } else {
                            StrawBlendFrame(m, w, matA, qA, qB, ctrl, stretch, invRot, scaled);
                        }
                        if (squeeze == 0)
                            StrawMulRows(m, m, inv);
                    }
                }
                if (squeeze == 0) {
                    StrawApply(dstPos[v], m, srcPos[v]);
                    StrawApply(dstNrm[v], m, srcNrm[v]);
                } else if (twist == 0) {
                    float t[4];
                    if (w == STRAW_W_A) {
                        memcpy(dstPos[v], srcPos[v], 0x10);
                        memcpy(dstNrm[v], srcNrm[v], 0x10);
                    } else {
                        StrawApply(t, m, srcPos[v]);
                        StrawApply(dstPos[v], inv, t);
                        StrawApply(t, m, srcNrm[v]);
                        StrawApply(dstNrm[v], inv, t);
                    }
                } else {
                    float rot[4][4], t[4];
                    asMakeRotMatrixByQuartanion(rot, m[1], twist);
                    memcpy(t, srcPos[v], 0x10);
                    t[3] = 0.0f;                /* vsub.w: a direction through the matrix */
                    StrawApply(t, m, t);
                    StrawApply(t, rot, t);
                    for (i = 0; i < 4; i++)
                        t[i] += m[3][i];
                    StrawApply(dstPos[v], inv, t);
                    StrawApply(t, m, srcNrm[v]);
                    StrawApply(t, rot, t);
                    StrawApply(dstNrm[v], inv, t);
                }
            }
        }
    }
}

// CT3 0x0184a520
// The behaviour's playable length: its first group's first track's (the ROM reads +0x14, +0x10).
// PORT: (bank, anim): both words are the bank's EE addresses (decision h).
int asGetBehaviorPlayableTicks(int bank, u_int anim)
{
    return BehaviorTracks(bank, BANK_PTR(BEHAVIOR *, bank, anim), 0)->playable;
}

// ---- slice 7 (2026-11-10): the small animation helpers, the DOF tree, asEnumMirror ----

// CT3 0x0184a530
// The behaviour's time so far: its first group's first track's (the ROM reads +0x14, +0x18).
// PORT: (bank, anim), as asGetBehaviorPlayableTicks.
int asGetBehaviorPlayingTicks(int bank, u_int anim)
{
    return BehaviorTracks(bank, BANK_PTR(BEHAVIOR *, bank, anim), 0)->time;
}

// CT3 0x01849750
// A track runs on from where it is: step 1 (or its runs, TRACK_FLAG_STEP_FROM_RUNS); a track at its
// end (time == playable * 100) clears its +0xe.
void asStartAnimation(BEHAVIOR_TRACK *track)
{
    if (track->flags & TRACK_FLAG_STEP_FROM_RUNS)
        track->step = (short)track->runs;
    else
        track->step = 1;
    track->playing = TRACK_PLAYING;
    if (track->time == track->playable * TRACK_TIME_SCALE)
        track->playRate = 0;
}

// CT3 0x0184a540
void asDecInterpolateTicks(u_char *ctrl, int ticks)
{
    int *left = (int *)(ctrl + ANIM_CTRL_TICKS);
    *left -= ticks;
    if (*left < 0)
        *left = 0;
}

// CT3 0x01847d50
// Every running track of the behaviour is marked TRACK_ACT_END (NULL: none).
void asBehaviorActEndAnimation(void *behavior)
{
    BEHAVIOR *bhv = (BEHAVIOR *)behavior;
    int bank, g, t;
    if (bhv == NULL)
        return;
    bank = BankOfHostPtr(bhv);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++)
            if (tr->playing == TRACK_PLAYING)
                tr->playing = TRACK_ACT_END;
    }
}

// CT3 0x018424d0
// The character's bank is mirrored (on == 0) or not (BANK_FLAG_NO_MIRROR).
void asForceOperateMirroring_Character(int chara, int on)
{
    u_int *flags;
    if (chara < 0 || Character[chara].bank < 0)
        return;
    flags = &BankInfo[Character[chara].bank].flags;
    *flags &= ~BANK_FLAG_NO_MIRROR;
    if (on)
        *flags |= BANK_FLAG_NO_MIRROR;
}

// The part record fields asBuildDOFMatrixTree reads (record offsets, named from that one use).
#define REC_DOF_MATRIX 0x1c0            /* float[4][4]: the part's matrix for the DOF pass (rows 0..2 w
                                           taken as 0) */
#define REC_DOF_SCALE_X 0x4c            /* floats: the DOF scale's diagonal, 0x10 apart */
#define REC_DOF_SCALE_Y 0x5c
#define REC_DOF_SCALE_Z 0x6c
#define PARTS_KIND_3 3                  /* REC_KIND: asBuildDOFMatrixTree skips it (meaning unread) */

// CT3 0x0184f020
// The DOF slices' matrices: every part but PARTS_KIND_STATIC / PARTS_KIND_NO_POS / PARTS_KIND_3 gets
// REC_GA_MATRIX2 = matrix_global_view * its REC_DOF_MATRIX (w of rows 0..2 zeroed), REC_GA_MATRIX =
// that * the diagonal scale (the identity from sdata 0x019377e0 with REC_DOF_SCALE_*). From the
// disassembly: the decompiler dropped gaMulMatrix's third argument.
void asBuildDOFMatrixTree(u_char *partsHeader)
{
    alignas(16) float scale[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; /* 0x019377e0 */
    u_char *rec;
    for (rec = partsHeader + PARTS_RECORDS; !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        signed char kind = (signed char)rec[REC_KIND];
        alignas(16) float m[4][4];
        if (kind == PARTS_KIND_STATIC || kind == PARTS_KIND_NO_POS || kind == PARTS_KIND_3)
            continue;
        memcpy(m, rec + REC_DOF_MATRIX, sizeof(m));
        scale[0][0] = *(float *)(rec + REC_DOF_SCALE_X);
        scale[1][1] = *(float *)(rec + REC_DOF_SCALE_Y);
        scale[2][2] = *(float *)(rec + REC_DOF_SCALE_Z);
        m[0][3] = m[1][3] = m[2][3] = 0.0f;
        gaMulMatrix((float (*)[4])(rec + REC_GA_MATRIX2), matrix_global_view, m);
        gaMulMatrix((float (*)[4])(rec + REC_GA_MATRIX), (float (*)[4])(rec + REC_GA_MATRIX2), scale);
    }
}

// CT3 0x0184bc30
// ExecuteAnimation's case for one part (the group whose part has EA_001_PART_HASH): its running tracks
// build a local matrix (TRACK_KIND_ROT / _POS) and a base (TRACK_KIND_BASE_ROT / _POS), both from the
// identity; out = base * local. out's +0x3c word (row 3 w) takes four done bytes, one per kind, set
// when that track's InterpolateAnimationKey ran out: byte 0 ROT, 1 POS, 2 BASE_ROT, 3 BASE_POS.
int EA_001(void *behavior, ALL_TABLE *tbl, float (*out)[4])
{
    BEHAVIOR *bhv = (BEHAVIOR *)behavior;
    int bank = BankOfHostPtr(bhv);
    u_char *header = BANK_PTR(u_char *, bank, tbl->partsHeader);
    alignas(16) float local[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    alignas(16) float base[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    alignas(16) float rot[4][4];
    alignas(16) float q[4];
    u_char done[4] = { 0, 0, 0, 0 };
    int g, t;
    for (g = 0; g < bhv->groupCount; g++) {
        const BEHAVIOR_GROUP *grp = &bhv->groups[g];
        BEHAVIOR_TRACK *tr;
        if (grp->tracks == 0)
            continue;
        if (((MODEL_PARTS *)(header + grp->part * (int)sizeof(MODEL_PARTS)))->hash != EA_001_PART_HASH)
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = grp->trackCount; t > 0; t--, tr++) {
            u_char *keys;
            int more, slot;
            float *dst;
            if (tr->playing != TRACK_PLAYING)
                continue;
            switch (tr->kind) {
            case TRACK_KIND_BASE_POS: slot = 3; dst = base[3]; break;
            case TRACK_KIND_POS: slot = 1; dst = local[3]; break;
            case TRACK_KIND_BASE_ROT: slot = 2; dst = NULL; break;
            case TRACK_KIND_ROT: slot = 0; dst = NULL; break;
            default: continue;
            }
            more = InterpolateAnimationKey(tr, 0);
            keys = BANK_PTR(u_char *, bank, tr->keys); /* the ROM reads the word after the call */
            if (dst != NULL) {
                if (tr->length & TRACK_KEY_RAW)
                    memcpy(dst, keys + 0x10, 0x10);
                else
                    asZipTrans2Trans_XYZ1(dst, (const short *)(keys + 8), TRACK_KEY_SHIFT(tr->length));
            } else {
                asZipQuat2Quat(q, (const short *)(keys + 8));
                ConvertQuat2Matrix(rot, q);
                memcpy(slot == 2 ? base : local, rot, 0x30); /* rows 0..2 */
            }
            if (more == 0)
                done[slot] = 1;
        }
    }
    gaMulMatrix(out, base, local);
    memcpy(&out[3][3], done, 4);        /* ROM: sw of the done word into the matrix's +0x3c */
    return 0;
}

MIRROR *mrr_pActvMirrorMO;              /* sbss 0x0193cae0 */
u_char *mrr_pActvMirrorMP;              /* sbss 0x0193cb00 */

// out = m applied to v (vmulax / vmadday / vmaddaz / vmaddw): m[0] v.x + m[1] v.y + m[2] v.z + m[3] v.w.
static void MirrorApply(float out[4], const float m[4][4], const float v[4])
{
    int i;
    for (i = 0; i < 4; i++)
        out[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
}

// CT3 0x0184fb40
// The scene's first mirror whose part is shown (no PARTS_FLAG_HIDDEN) becomes the active one: its
// three points into world space by the part's matrix (REC_MATRIX), vPPbk = the third, vPVbk = the unit
// normal (a - b) x (b - c) (w: a's). 1 when one was found; mrr_ReverseMatrix is reset either way when
// the scene has a mirror list.
int asEnumMirror(ALL_TABLE *tbl)
{
    int bank, i;
    MIRROR_LIST *list;
    MIRROR *mirrors;
    u_char *header;
    if (tbl->mirror == 0)
        return 0;
    bank = BankOfHostPtr(tbl);
    list = BANK_PTR(MIRROR_LIST *, bank, tbl->mirror);
    mirrors = BANK_PTR(MIRROR *, bank, list->mirrors);
    header = BANK_PTR(u_char *, bank, tbl->partsHeader);
    asInitMirrorMatrix();
    mrr_pActvMirrorMP = NULL;
    mrr_pActvMirrorMO = NULL;
    for (i = 0; i < list->count; i++) {
        MIRROR *mo = &mirrors[i];
        MODEL_PARTS *part = (MODEL_PARTS *)(header + mo->part * (int)sizeof(MODEL_PARTS));
        u_char *rec = (u_char *)part + PARTS_RECORDS;
        const float (*m)[4] = (const float (*)[4])(rec + REC_MATRIX);
        alignas(16) float a[4], b[4], n[4];
        float len;
        if (part->flags & PARTS_FLAG_HIDDEN)
            continue;
        mrr_pActvMirrorMO = mo;         /* ROM: both set after the maths; nothing reads them between */
        mrr_pActvMirrorMP = rec;
        MirrorApply(a, m, mo->p[0]);
        MirrorApply(b, m, mo->p[1]);
        MirrorApply(vPPbk, m, mo->p[2]);
        for (int k = 0; k < 3; k++) {   /* vsub.xyz: w stays a's / b's */
            a[k] -= b[k];
            b[k] -= vPPbk[k];
        }
        n[0] = a[1] * b[2] - a[2] * b[1]; /* vopmula / vopmsub.xyz into a: w stays a's */
        n[1] = a[2] * b[0] - a[0] * b[2];
        n[2] = a[0] * b[1] - a[1] * b[0];
        n[3] = a[3];
        len = 1.0f / sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]); /* vrsqrt vf0w */
        vPVbk[0] = n[0] * len;
        vPVbk[1] = n[1] * len;
        vPVbk[2] = n[2] * len;
        vPVbk[3] = n[3];
        return 1;
    }
    return 0;
}

// CT3 0x0184ed90
// Records an animation for Execute_ExecuteAnimationHistory to play next frame: the history grows by
// 0x10 records at a time (from gHeapJunc), sign = a * b.
// PORT: the bank in front, host records (EAS_RECORD); memcpy for gaMemCopy128's qword copy.
void SetExecuteAnimationHistory(int bank, u_int anim, ALL_TABLE *tbl, int a, int b, u_int step)
{
    EAS_RECORD *rec;
    if (lpEAS == NULL) {
        iEASidx = 0;
        iEASnum = EAS_GROW;
        lpEAS = _SunAlloc(&gHeapJunc, iEASnum * (int)sizeof(EAS_RECORD), 0x10);
    }
    if (iEASidx >= iEASnum) {
        void *grown;
        iEASnum += EAS_GROW;
        grown = _SunAlloc(&gHeapJunc, iEASnum * (int)sizeof(EAS_RECORD), 0x10);
        memcpy(grown, lpEAS, iEASidx * sizeof(EAS_RECORD)); // callcheck: gaMemCopy128 (a qword copy)
        _SunFree(&gHeapJunc, &lpEAS);
        lpEAS = grown;
    }
    rec = (EAS_RECORD *)lpEAS + iEASidx;
    rec->anim = anim;
    rec->tbl = tbl;
    rec->sign = a * b;
    rec->step = step;
    rec->bank = bank;
    iEASidx++;
}

// ---- slice 8 (2026-11-10): ExecuteAnimation's dispatch and EA_100, the per-frame pass ----

// The two weights asInterpolateVector / QuatSlerp take: w[0] the share of the value the part has (the
// ticks left after this frame's, over the step), w[1] the key's; swapped, w[2] = 0, when dir < 0.
static void AnimWeights(float w[4], int ticks, u_int elapsed, int total, int dir)
{
    if (ticks == 0) {
        w[0] = 0.0f;
        w[1] = 1.0f;
    } else {
        w[0] = (float)(int)(ticks - elapsed) / (float)total;
        if (w[0] >= 0.0f) {
            w[1] = 1.0f - w[0];
        } else {
            w[0] = 0.0f;
            w[1] = 1.0f;
        }
    }
    if (dir < 0) {
        float s = w[0];
        w[0] = w[1];
        w[1] = s;
        w[2] = 0.0f;
    }
}

// The kinds EA_100 applies to the part's pose (PARTS_ANIM_*); a track of these stops (playing 0) at
// its end, any other is marked TRACK_ACT_END.
static int IsPoseTrack(int kind)
{
    return kind == TRACK_KIND_BASE_POS || kind == TRACK_KIND_BASE_ROT || kind == TRACK_KIND_VECTOR
        || kind == TRACK_KIND_POS || kind == TRACK_KIND_ROT;
}

// Rows 0..2 at rows: the key's quaternion (ticks 0), or the slerp from the rows' own by w. q is the
// caller's scratch vector (the ROM's one stack slot, which ExecuteAnimation's replay kinds read on).
static void PoseRotKey(float *rows, const short *key, int ticks, float *w, float *q)
{
    alignas(16) float q0[4], qs[4];
    alignas(16) float m[4][4];
    asZipQuat2Quat(q, key);
    if (ticks == 0) {
        ConvertQuat2Matrix(m, q);
    } else {
        memcpy(m, rows, 0x30);          /* lqc2 / sqc2 of the three rows to the stack */
        m[3][0] = m[3][1] = m[3][2] = 0.0f; /* PORT: the ROM's fourth row is stale stack */
        m[3][3] = 1.0f;
        ConvertMatrix2Quat(q0, m);
        QuatSlerp(qs, q0, q, w);
        ConvertQuat2Matrix(m, qs);
    }
    memcpy(rows, m, 0x30);
}

// A position key (TRACK_KEY_RAW: a float[4] at keys + 0x10) into dst, or blended in by w (v: the
// caller's scratch vector, as PoseRotKey's q).
static void PosePosKey(float *dst, const BEHAVIOR_TRACK *tr, const u_char *keys, int ticks, float *w, float *v)
{
    if (tr->length & TRACK_KEY_RAW) {
        if (ticks == 0)
            memcpy(dst, keys + 0x10, 0x10);
        else
            asInterpolateVector(dst, dst, (const float *)(keys + 0x10), w);
    } else if (ticks == 0) {
        asZipTrans2Trans_XYZ1(dst, (const short *)(keys + 8), TRACK_KEY_SHIFT(tr->length));
    } else {
        asZipTrans2Trans_XYZ1(v, (const short *)(keys + 8), TRACK_KEY_SHIFT(tr->length));
        asInterpolateVector(dst, dst, v, w);
    }
}

// A running pose track's key (where InterpolateAnimationKey left it) into its group's part: set when
// the parts header's ticks are 0, else blended by w; the part is marked PARTS_ANIM_DIRTY. Other kinds
// are left alone. tmp: the caller's scratch vector (PosePosKey).
static void PoseTrack(int bank, u_char *part, const BEHAVIOR_TRACK *tr, int ticks, float *w, float *tmp)
{
    const u_char *keys = BANK_PTR(const u_char *, bank, tr->keys); /* read after the call, as the ROM */
    switch (tr->kind) {
    case TRACK_KIND_VECTOR: {
        float *dst = (float *)(part + PARTS_ANIM_VECTOR);
        if (ticks == 0) {
            asZipVECTOR2VECTOR_XYZ1(dst, (const short *)(keys + 8));
        } else {
            asZipVECTOR2VECTOR_XYZ1(tmp, (const short *)(keys + 8));
            asInterpolateVector(dst, dst, tmp, w);
        }
        break;
    }
    case TRACK_KIND_BASE_POS:
    case TRACK_KIND_POS:
        PosePosKey((float *)(part + (tr->kind == TRACK_KIND_POS ? PARTS_ANIM_POS : PARTS_ANIM_BASE_POS)),
                   tr, keys, ticks, w, tmp);
        break;
    case TRACK_KIND_BASE_ROT:
    case TRACK_KIND_ROT:
        PoseRotKey((float *)(part + (tr->kind == TRACK_KIND_ROT ? PARTS_ANIM_ROT : PARTS_ANIM_BASE_ROT)),
                   (const short *)(keys + 8), ticks, w, tmp);
        break;
    default:
        return;
    }
    part[PARTS_ANIM_FLAGS] |= PARTS_ANIM_DIRTY;
}

// The EA_001_PART_HASH part's track at its key 0, as EA_001 builds it: BASE_POS row 3 of base (done
// byte 3), POS row 3 of local (1), BASE_ROT rows 0..2 of base (2), ROT rows 0..2 of local (0).
static void MatKey(int bank, BEHAVIOR_TRACK *tr, float (*base)[4], float (*local)[4], u_char *done, float *tmp)
{
    const u_char *keys;
    int slot;
    float *dst = NULL;
    switch (tr->kind) {
    case TRACK_KIND_BASE_POS: slot = 3; dst = base[3]; break;
    case TRACK_KIND_POS: slot = 1; dst = local[3]; break;
    case TRACK_KIND_BASE_ROT: slot = 2; break;
    case TRACK_KIND_ROT: slot = 0; break;
    default: return;
    }
    InterpolateAnimationKey(tr, 0);
    keys = BANK_PTR(const u_char *, bank, tr->keys);
    if (dst != NULL) {
        if (tr->length & TRACK_KEY_RAW)
            memcpy(dst, keys + 0x10, 0x10);
        else
            asZipTrans2Trans_XYZ1(dst, (const short *)(keys + 8), TRACK_KEY_SHIFT(tr->length));
    } else {
        alignas(16) float rot[4][4];
        asZipQuat2Quat(tmp, (const short *)(keys + 8));
        ConvertQuat2Matrix(rot, tmp);
        memcpy(slot == 2 ? base : local, rot, 0x30);
    }
    done[slot] = 1;
}

// CT3 0x0184d1a0
// Every running track of the behaviour takes its key at this frame's ticks (anim_play_slow *
// gGlobalNextTicksInterval * playRate / 100; atKey: where it stands) and, for a pose kind, puts it in
// its group's part (set when the parts header's ticks are 0, else blended by AnimWeights) and marks
// the part PARTS_ANIM_DIRTY. With mat, the EA_001_PART_HASH part's keys build base * local as EA_001
// does (every done byte set). Every track's time1c shifts up and takes time / 100. The call is
// recorded for the history first; the ticks are counted down after. 1 when no track runs on.
// PORT: (bank, anim) for the behaviour's EE word.
// callcheck: ConvertMatrix2Quat, QuatSlerp, ConvertQuat2Matrix, asInterpolateVector, asZipQuat2Quat,
// callcheck: asZipTrans2Trans_XYZ1, asZipVECTOR2VECTOR_XYZ1 (in PoseTrack, PoseRotKey, PosePosKey, MatKey)
int EA_100(int bank, u_int anim, ALL_TABLE *tbl, float (*mat)[4], int atKey)
{
    BEHAVIOR *bhv = BANK_PTR(BEHAVIOR *, bank, anim);
    u_char *hdr = BANK_PTR(u_char *, bank, tbl->partsHeader);
    int ticks = *(int *)(hdr + PARTS_HDR_ANIM_ABS);
    int dir = *(int *)(hdr + PARTS_HDR_ANIM_SIGN);
    int total = *(int *)(hdr + PARTS_HDR_ANIM_STEP);
    alignas(16) float local[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    alignas(16) float base[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    alignas(16) float w[4] = { 0, 0, 0, 0 };
    alignas(16) float tmp[4] = { 0, 0, 0, 0 };
    u_char done[4] = { 0, 0, 0, 0 };
    u_int elapsed = 0;
    int running = 0;
    int g, t;
    SetExecuteAnimationHistory(bank, anim, tbl, ticks, dir, total);
    for (g = 0; g < bhv->groupCount; g++) {
        const BEHAVIOR_GROUP *grp = &bhv->groups[g];
        u_char *part = hdr + grp->part * (int)sizeof(MODEL_PARTS);
        BEHAVIOR_TRACK *tr;
        if (grp->tracks == 0)
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = grp->trackCount; t > 0; t--, tr++) {
            elapsed = (u_int)anim_play_slow * (u_int)gGlobalNextTicksInterval * tr->playRate / 100;
            AnimWeights(w, ticks, elapsed, total, dir);
            if (tr->playing == TRACK_PLAYING) {
                int pose = IsPoseTrack(tr->kind);
                int end = InterpolateAnimationKey(tr, atKey ? 0 : (int)elapsed);
                if (hdr[0] == ANIM_CTRL_HOLD) {
                    running = 1;
                    if (end != 0) {
                        if (tr->flags & TRACK_FLAG_STARTED)
                            asStartAnimation(tr);
                        else
                            tr->playing = pose ? 0 : TRACK_ACT_END;
                    }
                } else if (end == 0) {
                    running++;
                } else {
                    tr->playing = pose ? 0 : TRACK_ACT_END;
                }
                PoseTrack(bank, part, tr, ticks, w, tmp);
                if (mat != NULL && ((MODEL_PARTS *)part)->hash == EA_001_PART_HASH)
                    MatKey(bank, tr, base, local, done, tmp);
            }
            tr->time1c = (int)((u_int)tr->time1c << 16) + ((tr->time / TRACK_TIME_SCALE) & 0xffff);
        }
    }
    if (mat != NULL) {
        gaMulMatrix(mat, base, local);
        memcpy(&mat[3][3], done, 4);    /* ROM: sw of the done word into the matrix's +0x3c */
    }
    asDecInterpolateTicks(hdr, (int)elapsed);
    if (running != 0)
        return 0;
    if (hdr[0] == ANIM_CTRL_HOLD || hdr[0] == ANIM_CTRL_ONCE)
        asBehaviorActEndAnimation(bhv);
    return 1;
}

static int IsReplayKind(int kind);
static void ReplayTrack(int bank, const ALL_TABLE *tbl, u_char *part, BEHAVIOR_TRACK *tr, int set,
                        float *w, float *held);

// CT3 0x0184a570
// The pass by the flags: play alone EA_100, replay alone EA_010, matOnly alone EA_001; any other mix
// runs the general body, the three passes in one per track: with play, every track's time1c shifts and
// a running track takes its key at this frame's ticks (atKey: where it stands) and, a pose kind, puts
// it in its part (EA_100); with replay, a running or ending replay kind is set or blended into what it
// animates (EA_010); with mat, the EA_001_PART_HASH part's keys build base * local (matOnly: weights at
// 0 ticks, no pose write). Without replay and matOnly the ticks are counted down and 1 is returned
// when no track runs on (the behaviour's act ended for HOLD / ONCE); else 0.
// PORT: (bank, anim) for the behaviour's EE word; mat a host pointer. The scratch vector `tmp` is the
// ROM's one stack slot shared by the pose and replay kinds (EA_010's `held`).
// callcheck: EA_100, EA_010, EA_001 (the dispatch above)
// callcheck: ConvertMatrix2Quat, QuatSlerp, ConvertQuat2Matrix, asZipQuat2Quat, asZipTrans2Trans_XYZ1 (in PoseTrack, MatKey)
// callcheck: asContInterpolateGA_VECTORZipNormal_XYZ0, asContInterpolateGA_VECTORZipTrans_XYZ1,
// callcheck: asContInterpolateGA_VECTORZipTrans_XYZ1_nw, asContInterpolateVector, asContInterpolateVector_nw,
// callcheck: asContZipNormal2Normal_XYZ0, asContZipTrans2Trans_XYZ1, asContZipTrans2Trans_XYZ1_nw,
// callcheck: asInterpolateNumber, asInterpolateVector, asInterpolateZipFloat1_1_14X, asMemCopy128c,
// callcheck: asMemCopy128c_nw, asZipFloat0_7_9X2Float, asZipVECTOR2VECTOR_XYZ1 (in ReplayTrack)
int ExecuteAnimation(int bank, u_int anim, ALL_TABLE *tbl, float (*mat)[4], int play, int replay,
                     int matOnly, int atKey)
{
    play = (u_char)play;                /* the ROM keeps the four flags as bytes */
    replay = (u_char)replay;
    matOnly = (u_char)matOnly;
    if (play == 1 && replay == 0 && matOnly == 0)
        return EA_100(bank, anim, tbl, mat, (u_char)atKey);
    if (play == 0 && replay == 1 && matOnly == 0)
        return EA_010(bank, anim, tbl);
    if (play == 0 && replay == 0 && matOnly == 1)
        return EA_001(BANK_PTR(void *, bank, anim), tbl, mat);
    {
        BEHAVIOR *bhv = BANK_PTR(BEHAVIOR *, bank, anim);
        u_char *hdr = BANK_PTR(u_char *, bank, tbl->partsHeader);
        int ticks = *(int *)(hdr + PARTS_HDR_ANIM_ABS);
        int dir = *(int *)(hdr + PARTS_HDR_ANIM_SIGN);
        int total = *(int *)(hdr + PARTS_HDR_ANIM_STEP);
        alignas(16) float local[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
        alignas(16) float base[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
        alignas(16) float w[4] = { 0, 0, 0, 0 };
        alignas(16) float tmp[4] = { 0, 0, 0, 0 };
        u_char done[4] = { 0, 0, 0, 0 };
        u_int elapsed = 0;
        int running = 0;
        int g, t;
        if (play != 0 && replay == 0 && matOnly == 0)   /* never true here: kept as the ROM has it */
            SetExecuteAnimationHistory(bank, anim, tbl, ticks, dir, total);
        for (g = 0; g < bhv->groupCount; g++) {
            const BEHAVIOR_GROUP *grp = &bhv->groups[g];
            u_char *part = hdr + grp->part * (int)sizeof(MODEL_PARTS);
            BEHAVIOR_TRACK *tr;
            if (grp->tracks == 0)
                continue;
            tr = BehaviorTracks(bank, bhv, g);
            for (t = grp->trackCount; t > 0; t--, tr++) {
                int pose = IsPoseTrack(tr->kind);
                int doReplay = IsReplayKind(tr->kind) && replay != 0;
                if (play != 0 || matOnly != 0) {
                    elapsed = matOnly != 0
                        ? 0u
                        : (u_int)anim_play_slow * (u_int)gGlobalNextTicksInterval * tr->playRate / 100;
                    AnimWeights(w, ticks, elapsed, total, dir);
                }
                if (play != 0)
                    tr->time1c = (int)((u_int)tr->time1c << 16) + ((tr->time / TRACK_TIME_SCALE) & 0xffff);
                if (!(tr->playing == TRACK_PLAYING || (tr->playing == TRACK_ACT_END && doReplay)))
                    continue;
                if (matOnly == 0) {
                    if (play != 0) {
                        int end = InterpolateAnimationKey(tr, atKey ? 0 : (int)elapsed);
                        if (hdr[0] == ANIM_CTRL_HOLD) {
                            running = 1;
                            if (end != 0) {
                                if (tr->flags & TRACK_FLAG_STARTED)
                                    asStartAnimation(tr);
                                else
                                    tr->playing = pose ? 0 : TRACK_ACT_END;
                            }
                        } else if (end == 0) {
                            running++;
                        } else {
                            tr->playing = pose ? 0 : TRACK_ACT_END;
                        }
                        PoseTrack(bank, part, tr, ticks, w, tmp);
                    }
                    if (doReplay)
                        ReplayTrack(bank, tbl, part, tr, ticks == 0 || tr->playing == TRACK_ACT_END, w, tmp);
                }
                if (mat != NULL && ((MODEL_PARTS *)part)->hash == EA_001_PART_HASH)
                    MatKey(bank, tr, base, local, done, tmp);
            }
        }
        if (mat != NULL) {
            gaMulMatrix(mat, base, local);
            memcpy(&mat[3][3], done, 4);    /* ROM: sw of the done word into the matrix's +0x3c */
        }
        if (matOnly != 0 || replay != 0)
            return 0;
        asDecInterpolateTicks(hdr, (int)elapsed);
        if (running != 0)
            return 0;
        if (hdr[0] == ANIM_CTRL_HOLD || hdr[0] == ANIM_CTRL_ONCE)
            asBehaviorActEndAnimation(bhv);
        return 1;
    }
}

// CT3 0x018427f0
// main, once a frame after the animations: the matrix trees of the drawn scene banks (and those drawn
// first) and of the refracting shadowed characters, each such character's axis from its model, then
// every drawn bank's world matrices kept as last frame's.
void asUpdateAnimation_Matrix(void)
{
    int i;
    short c;
    for (i = 0; i < BANK_MAX; i++) {
        BANK_INFO *bi = &BankInfo[i];
        if (bi->state == BANK_STATE_LOADED
            && (bi->drawOn == DRAW_BANK_ON || (bi->flags & BANK_FLAG_DRAW_FIRST) != 0)
            && (bi->type == BANK_TYPE_SHADOW || bi->type == BANK_TYPE_0))
            BuildMatrixTree(i, 0, 1);
        if (bi->state > 0)
            i += bi->groupCount - 1;
    }
    for (c = 0; c < CHARACTER_MAX; c++) {
        CHARACTER *ch = &Character[c];
        u_char *rec;
        if ((ch->flags & CHARACTER_FLAG_REFRACTION) == 0 || (ch->flags & CHARACTER_FLAG_SHADOW) == 0
            || ch->bank < 0)
            continue;
        BuildMatrixTree(ch->bank, 0, 1);
        rec = asFindModelParts(pModelPartsHeader[ch->bank], CHARA_AXIS_HASH_ROW1);
        if (rec != NULL) {
            memcpy(ch->refractAxis, rec + REC_MATRIX + 0x10, sizeof(ch->refractAxis));
        } else if ((rec = asFindModelParts(pModelPartsHeader[ch->bank], CHARA_AXIS_HASH_ROW2)) != NULL) {
            memcpy(ch->refractAxis, rec + REC_MATRIX + 0x20, sizeof(ch->refractAxis));
        } else {
            static const float vf0[4] = {0.0f, 0.0f, 0.0f, 1.0f}; /* sqc2 vf0 */
            memcpy(ch->refractAxis, vf0, sizeof(ch->refractAxis));
        }
    }
    for (i = 0; i < BANK_MAX; i++) {
        BANK_INFO *bi = &BankInfo[i];
        if (bi->state != BANK_STATE_LOADED)
            continue;
        if ((bi->drawOn == DRAW_BANK_ON || (bi->flags & BANK_FLAG_DRAW_FIRST) != 0)
            && (bi->type == BANK_TYPE_2 || bi->type == BANK_TYPE_SHADOW || bi->type == BANK_TYPE_0))
            asCopyDoubleBufferL2W(BANK_PTR(u_char *, i, All_table_TBL[i]->partsHeader));
        if (bi->state > 0)
            i += bi->groupCount - 1;
    }
    asResetCharacterRotation(0, 0, 0);
}

// PORT: CHARACTER +CHARA_MOTION_TABLE holds the EE address of the character's motion table, written
// by asInitPlayerBehaviorControler (wideXcmd_DebugPrint.c, still a stub), whose TU decides its host
// form; until then no character has one here (an empty table: the walk ends at once, index 0).
static const u_char *CharaMotionTable(const CHARACTER *ch)
{
    (void)ch;
    return NULL;
}

// CT3 0x018483d0
// Starts a scene animation over [from, to] (frames). SCENE_ANIM_SCENE: every track of the groups of
// the scene's behaviour that animate partId (or with byPart, the groups of part 0 too) whose kind the
// switches select: pose for the pose and vertex kinds, other for the number / vector kinds, byPart for
// the copy and UV kinds whose first key names partId. Each gets the range, the two flag bits, the play
// rate (speed * 100; toEnd: 0, to the end at once), its pass check and a start. SCENE_ANIM_CHARACTER:
// the character's motion by hash instead (toEnd: through its motion table, at frame `to`). 0 when a
// track started (or what the motion request answers), 1 otherwise.
u_long asSceneAnimationStart(float speed, u_long hash, ALL_TABLE *tbl, int from, int to, int partId,
                             int pose, int other, int byPart, int started, int toEnd, int stepFromRuns,
                             int target, int chara)
{
    int bank, g, t;
    BEHAVIOR *bhv;
    float rate;
    u_short part;
    u_char any;
    if (tbl == NULL)
        return 1;
    bank = BankOfHostPtr(tbl);
    rate = (char)toEnd == 0 ? 1.0f : 0.0f;
    if ((char)target == SCENE_ANIM_CHARACTER) {
        CHARACTER *ch = &Character[(u_char)chara];
        if ((char)toEnd == 0)
            return asCharacterMortionChangeRequestHash((u_char)chara, 0, hash);
        // The character's motion table (CHARACTER +CHARA_MOTION_TABLE, an EE word): 0x30-byte records
        // led by a hash, the motion's own hash at +8, a 0 hash ending it; the record's index goes to
        // the request.
        {
            const u_char *table = CharaMotionTable(ch);
            int i = 0;
            u_long h = 0;
            if (table != NULL)
                memcpy(&h, table, sizeof(h));
            while (h != 0) {
                if (h == hash) {
                    memcpy(&hash, table + i * CHARA_MOTION_SIZE + 8, sizeof(hash));
                    break;
                }
                i++;
                memcpy(&h, table + i * CHARA_MOTION_SIZE, sizeof(h));
            }
            asChangeBehaviorAnimationCurrent(asFindBehaviorByCharacter((u_char)chara, hash), to);
            return asCharacterMortionChangeRequest((u_char)chara, CHARA_MOTION_REQUEST_INDEX, i);
        }
    }
    if ((char)target != SCENE_ANIM_SCENE)
        return 1;
    bhv = BANK_PTR(BEHAVIOR *, bank, tbl->behavior);
    part = (u_short)partId;
    if (part == 0)
        part = (u_short)asFindModelPartsID(BANK_PTR(u_char *, bank, tbl->partsHeader), hash);
    any = 0;
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr;
        if (!(bhv->groups[g].part == (short)part || (byPart != 0 && bhv->groups[g].part == 0)))
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++) {
            int take = 0;
            switch (tr->kind) {
            case TRACK_KIND_BASE_POS:
            case TRACK_KIND_BASE_ROT:
            case TRACK_KIND_POS:
            case TRACK_KIND_ROT:
            case TRACK_KIND_VECTOR:
            case TRACK_KIND_NORMAL:
            case TRACK_KIND_VERTEX:
                take = (char)pose != 0;
                break;
            case TRACK_KIND_COPY:
                if (byPart != 0) {
                    u_short p;
                    memcpy(&p, BANK_PTR(const u_char *, bank, tr->keys) + 0x1a, sizeof(p));
                    take = p == (u_short)partId;
                }
                break;
            case TRACK_KIND_UV:
                if (byPart != 0) {
                    short p;
                    memcpy(&p, BANK_PTR(const u_char *, bank, tr->keys) + 0x1c, sizeof(p));
                    take = p == (int)(u_short)partId;
                }
                break;
            case TRACK_KIND_FLOAT0_7_9:
            case TRACK_KIND_SUB_NUMBER:
            case TRACK_KIND_NUMBER:
            case TRACK_KIND_FLOAT1_1_14:
            case TRACK_KIND_VECTOR_XYZ:
                take = (char)other != 0;
                break;
            }
            if (!take)
                continue;
            any = 1;
            asChangePlayRange(tr, from, to);
            tr->flags = (u_char)((tr->flags & ~TRACK_FLAG_STEP_FROM_RUNS) | (u_char)stepFromRuns);
            tr->flags = (u_char)((tr->flags & ~TRACK_FLAG_STARTED) | (u_char)((char)started << 1));
            tr->playRate = (u_char)(int)(speed * rate * 100.0f);
            asResetPassCheckarameter(tr, from);
            asStartAnimation(tr);
        }
    }
    return any ^ 1;
}

// ---- slice 12 (2026-11-10) ----

// asSceneAnimationStart's selection, as asSceneAnimationStop_S and asCheckAnimationDirection_X repeat
// it inline: pose for the pose and vertex kinds, other for the number / vector kinds, byPart for the
// copy and UV kinds whose first key names partId (u_short, a negative UV short never matches).
static int TrackSelected(int bank, const BEHAVIOR_TRACK *tr, u_short partId, int pose, int other, int byPart)
{
    switch (tr->kind) {
    case TRACK_KIND_BASE_POS:
    case TRACK_KIND_BASE_ROT:
    case TRACK_KIND_POS:
    case TRACK_KIND_ROT:
    case TRACK_KIND_VECTOR:
    case TRACK_KIND_NORMAL:
    case TRACK_KIND_VERTEX:
        return pose != 0;
    case TRACK_KIND_COPY:
        if (byPart != 0) {
            u_short p;
            memcpy(&p, BANK_PTR(const u_char *, bank, tr->keys) + 0x1a, sizeof(p));
            return p == partId;
        }
        return 0;
    case TRACK_KIND_UV:
        if (byPart != 0) {
            short p;
            memcpy(&p, BANK_PTR(const u_char *, bank, tr->keys) + 0x1c, sizeof(p));
            return p == (int)partId;
        }
        return 0;
    case TRACK_KIND_FLOAT0_7_9:
    case TRACK_KIND_SUB_NUMBER:
    case TRACK_KIND_NUMBER:
    case TRACK_KIND_FLOAT1_1_14:
    case TRACK_KIND_VECTOR_XYZ:
        return other != 0;
    }
    return 0;
}

// CT3 0x01848090
// Stops what asSceneAnimationStart started. SCENE_ANIM_SCENE: the running tracks the switches select
// in the groups of partId (0: the part named by hash; byPart: part 0's groups too) get
// TRACK_STOP_REQUEST. SCENE_ANIM_CHARACTER (a CHARACTER_FLAG_REFRACTION character only): the first
// motion slot whose motion table record has `hash` is stopped, and with hash 0 every used slot.
// 0 when something stopped (a track, or a slot matched by hash), 1 otherwise.
u_char asSceneAnimationStop_S(u_long hash, ALL_TABLE *tbl, int partId, int pose, int other, int byPart,
                              int target, int chara)
{
    int bank, g, t;
    BEHAVIOR *bhv;
    u_short part;
    u_char any;
    if (tbl == NULL)
        return 1;
    if ((char)target == SCENE_ANIM_CHARACTER) {
        CHARACTER *ch = &Character[(u_char)chara];
        const u_char *table;
        int slot;
        if (!(ch->flags & CHARACTER_FLAG_REFRACTION))
            return 1;
        // PORT: no character has a host motion table yet (CharaMotionTable): only hash 0 stops slots.
        table = CharaMotionTable(ch);
        for (slot = 0; slot < CHARA_MOTION_SLOT_COUNT; slot++) {
            short idx;
            memcpy(&idx, (const u_char *)ch + CHARA_MOTION_SLOTS + slot * 2, sizeof(idx));
            if (idx < 0)
                continue;
            if (table != NULL) {
                u_long h;
                memcpy(&h, table + idx * CHARA_MOTION_SIZE, sizeof(h));
                if (h == hash) {
                    asCharacterMortionStopRequest((u_char)chara, slot);
                    return 0;
                }
            }
            if (hash == 0)
                asCharacterMortionStopRequest((u_char)chara, slot);
        }
        return 1;
    }
    if ((char)target != SCENE_ANIM_SCENE)
        return 1;
    bank = BankOfHostPtr(tbl);
    bhv = BANK_PTR(BEHAVIOR *, bank, tbl->behavior);
    part = (u_short)partId;
    if (part == 0)
        part = (u_short)asFindModelPartsID(BANK_PTR(u_char *, bank, tbl->partsHeader), hash);
    any = 0;
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr;
        if (!((int)bhv->groups[g].part == (int)part || (byPart != 0 && bhv->groups[g].part == 0)))
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++) {
            if (tr->playing == 0)
                continue;
            if (TrackSelected(bank, tr, (u_short)partId, pose, other, byPart)) {
                any = 1;
                tr->playing = TRACK_STOP_REQUEST;
            }
        }
    }
    return any ^ 1;
}

// CT3 0x018487d0
// The direction of the first running track asSceneAnimationStop_S's selection takes (partId as
// given, no hash): its TRACK_FLAG_STEP_FROM_RUNS bit; -1 when none runs.
int asCheckAnimationDirection_X(ALL_TABLE *tbl, int partId, int pose, int other, int byPart)
{
    int bank, g, t;
    BEHAVIOR *bhv;
    u_short part = (u_short)partId;
    if (tbl == NULL)
        return -1;
    bank = BankOfHostPtr(tbl);
    bhv = BANK_PTR(BEHAVIOR *, bank, tbl->behavior);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr;
        if (!((int)bhv->groups[g].part == (int)part || (byPart != 0 && bhv->groups[g].part == 0)))
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++)
            if (TrackSelected(bank, tr, part, pose, other, byPart) && tr->playing == TRACK_PLAYING)
                return tr->flags & TRACK_FLAG_STEP_FROM_RUNS;
    }
    return -1;
}

// CT3 0x01847e20
// asRequestExecute's start: every track of the behaviour from where it stands (asStartAnimation),
// its flags cleared and, when the end (playable) lies before the range's start, the two swapped.
void asBehaviorStartAnimation(void *behavior, int go)
{
    BEHAVIOR *bhv = (BEHAVIOR *)behavior;
    int bank, g, t;
    if (bhv == NULL || go == 0)
        return;
    bank = BankOfHostPtr(bhv);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++) {
            int end;
            asStartAnimation(tr);
            tr->flags = 0;
            end = tr->playable;
            if (end < tr->rangeStart) {
                tr->playable = tr->rangeStart;
                tr->rangeStart = end;
            }
        }
    }
}

// CT3 0x01847ef0
// The same from the top (asStartAnimationFromTop with resetTime).
void asBehaviorStartAnimationFromTop(void *behavior, int go, int resetTime)
{
    BEHAVIOR *bhv = (BEHAVIOR *)behavior;
    int bank, g, t;
    if (bhv == NULL || go == 0)
        return;
    bank = BankOfHostPtr(bhv);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++) {
            int end;
            asStartAnimationFromTop(tr, resetTime);
            tr->flags = 0;
            end = tr->playable;
            if (end < tr->rangeStart) {
                tr->playable = tr->rangeStart;
                tr->rangeStart = end;
            }
        }
    }
}

// CT3 0x018442d0
// The trigger's copy: dstChara's bank takes srcChara's shadow settings (BankInfo +0x30..+0x4f, the
// light position, length, colour, +0x64 / +0x68 and the four bytes from shadowOrder; not the stocked
// frame nor the block). Nothing when either character has no bank.
void asCopyShadowInformation(int dstChara, int srcChara)
{
    BANK_INFO *d, *s;
    if (Character[dstChara].bank < 0 || Character[srcChara].bank < 0)
        return;
    d = &BankInfo[Character[dstChara].bank];
    s = &BankInfo[Character[srcChara].bank];
    memcpy(d->shadowVec, s->shadowVec, sizeof(d->shadowVec));
    memcpy(d->shadowAnchor, s->shadowAnchor, sizeof(d->shadowAnchor));
    d->shadowLightPos = s->shadowLightPos;
    d->shadowLength = s->shadowLength;
    memcpy(d->shadowColor, s->shadowColor, sizeof(d->shadowColor));
    d->shadow64 = s->shadow64;
    d->shadow68 = s->shadow68;
    d->shadowOrder = s->shadowOrder;
    d->shadow6d = s->shadow6d;
    d->shadowDraw = s->shadowDraw;
    d->pad6f = s->pad6f;
}

// CT3 0x01847360
// Turns shadows off (asShadowDrawing_Disable): SHADOW_DISABLE_BANKS every drawn BANK_TYPE_SHADOW bank
// (groups skipped as BankInfo's walk does), SHADOW_DISABLE_CHARACTERS the bank of every character with
// both CHARACTER_FLAG_REFRACTION and _SHADOW.
void asShadowAllDisable(u_int which)
{
    int i;
    if (which & SHADOW_DISABLE_BANKS) {
        for (i = 0; i < BANK_MAX; i++) {
            BANK_INFO *bi = &BankInfo[i];
            if (bi->drawOn > 0 && bi->type == BANK_TYPE_SHADOW)
                asShadowDrawing_Disable(i);
            if (bi->state > 0)
                i += bi->groupCount - 1;
        }
    }
    if (which & SHADOW_DISABLE_CHARACTERS) {
        for (i = 0; i < CHARACTER_MAX; i++) {
            CHARACTER *ch = &Character[i];
            if ((ch->flags & CHARACTER_FLAG_REFRACTION) && (ch->flags & CHARACTER_FLAG_SHADOW) && ch->bank >= 0)
                asShadowDrawing_Disable(ch->bank);
        }
    }
}

// ---- slice 13 (2026-11-10) ----

// A character's motion: is `hash` playing (CHARA_MOTION_SLOTS) or queued (CHARA_MOTION_QUEUE)?
static int CharaMotionState(u_long hash, const CHARACTER *ch)
{
    const u_char *table;
    int state = ANIM_STATE_NONE, i;
    if (!(ch->flags & CHARACTER_FLAG_REFRACTION))
        return ANIM_STATE_NONE;
    // PORT: no character has a host motion table yet (CharaMotionTable): nothing matches until it does.
    table = CharaMotionTable(ch);
    if (table == NULL)
        return ANIM_STATE_NONE;
    for (i = 0; i < CHARA_MOTION_SLOT_COUNT; i++) {
        short idx;
        u_long h;
        memcpy(&idx, (const u_char *)ch + CHARA_MOTION_SLOTS + i * 2, sizeof(idx));
        if (idx == -1)
            continue;
        memcpy(&h, table + idx * CHARA_MOTION_SIZE, sizeof(h));
        if (h == hash) {
            state = ANIM_STATE_PLAYING;
            break;
        }
    }
    // the queue: the last entry with `hash` decides (the ROM walks on after a match)
    for (i = 0; i < CHARA_MOTION_QUEUE_COUNT; i++) {
        short idx, mode;
        u_long h;
        memcpy(&idx, (const u_char *)ch + CHARA_MOTION_QUEUE + i * 2, sizeof(idx));
        if (idx == -1)
            break;
        memcpy(&h, table + idx * CHARA_MOTION_SIZE, sizeof(h));
        if (h != hash)
            continue;
        memcpy(&mode, (const u_char *)ch + CHARA_MOTION_QUEUE_MODE + i * 2, sizeof(mode));
        switch (mode) {
        case 0: case 1: case 3: case 5:
            state = ANIM_STATE_PLAYING;
            break;
        case CHARA_MOTION_QUEUE_STOP:
            state = ANIM_STATE_NONE;
            break;
        }
    }
    return state;
}

// CT3 0x018489e0
// What asSceneAnimationStop_S would stop, asked instead of done. SCENE_ANIM_SCENE: the tracks it selects
// (running or not) mark their `playing` value; ANIM_STATE_PLAYING when one is TRACK_PLAYING, else
// ANIM_STATE_STOPPING when one is TRACK_STOP_REQUEST, else ANIM_STATE_NONE. SCENE_ANIM_CHARACTER:
// whether the motion `hash` plays in a slot or by the request queue.
int asCheckAnimationState_S(u_long hash, ALL_TABLE *tbl, int partId, int pose, int other, int byPart,
                            int target, int chara)
{
    u_char seen[5] = {0};       // sdata __2155: zeros
    int bank, g, t;
    BEHAVIOR *bhv;
    u_short part;
    if (tbl == NULL)
        return ANIM_STATE_NONE;
    if ((char)target == SCENE_ANIM_CHARACTER)
        return CharaMotionState(hash, &Character[(u_char)chara]);
    if ((char)target != SCENE_ANIM_SCENE)
        return ANIM_STATE_NONE;
    bank = BankOfHostPtr(tbl);
    bhv = BANK_PTR(BEHAVIOR *, bank, tbl->behavior);
    part = (u_short)partId;
    if (part == 0)
        part = (u_short)asFindModelPartsID(BANK_PTR(u_char *, bank, tbl->partsHeader), hash);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr;
        if (!((int)bhv->groups[g].part == (int)part || (byPart != 0 && bhv->groups[g].part == 0)))
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++)
            // PORT: the ROM indexes its 5-byte stack array by the signed byte unchecked
            if (TrackSelected(bank, tr, (u_short)partId, pose, other, byPart) && tr->playing < sizeof(seen))
                seen[tr->playing] = 1;
    }
    if (seen[TRACK_PLAYING])
        return ANIM_STATE_PLAYING;
    if (seen[TRACK_STOP_REQUEST])
        return ANIM_STATE_STOPPING;
    return ANIM_STATE_NONE;
}

// CT3 0x01848db0
// wideXcmd_PlayAnimeToFream2's question: the first track of `kind` in the groups of partId (0: the part
// named by hash); kinds TRACK_KIND_UV / _COPY also in part 0's groups, where the track's first key must
// name partId (as given). Its time in frames * 2, + 1 when TRACK_PLAYING; -1 when there is none.
int asGetSceneAnimationCurrent(u_long hash, ALL_TABLE *tbl, int partId, int kind)
{
    int bank, g, t;
    BEHAVIOR *bhv;
    u_short given = (u_short)partId, part;
    if (tbl == NULL)
        return 0;
    bank = BankOfHostPtr(tbl);
    bhv = BANK_PTR(BEHAVIOR *, bank, tbl->behavior);
    part = given;
    if (given == 0)
        part = (u_short)asFindModelPartsID(BANK_PTR(u_char *, bank, tbl->partsHeader), hash);
    for (g = 0; g < bhv->groupCount; g++) {
        int gp = bhv->groups[g].part;
        BEHAVIOR_TRACK *tr;
        if (!(gp == (int)part || ((char)kind == TRACK_KIND_UV || (char)kind == TRACK_KIND_COPY) && gp == 0))
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++) {
            int hit = 0;
            if (tr->kind != (char)kind)
                continue;
            if (gp != 0) {
                hit = 1;
            } else if ((char)kind == TRACK_KIND_UV) {
                short p;
                memcpy(&p, BANK_PTR(const u_char *, bank, tr->keys) + 0x1c, sizeof(p));
                hit = (int)p == (int)given;
            } else if ((char)kind == TRACK_KIND_COPY) {
                u_short p;
                memcpy(&p, BANK_PTR(const u_char *, bank, tr->keys) + 0x1a, sizeof(p));
                hit = p == given;
            }
            if (hit)
                return (tr->time / TRACK_TIME_SCALE) * 2 + (tr->playing == TRACK_PLAYING);
        }
    }
    return -1;
}

// ---- slice 14 (2026-11-10) ----

// Did the track's time pass `frame` between last frame and this one? time1c holds last frame's frame
// (high half) and this frame's (low half). Forward: (last, now], wrapping past the end; backward:
// [now, last), wrapping too. Equal halves: the frame it stands on. dir picks which play direction
// counts (PASS_DIR_*). romSceneQuirk: the scene branch's backward test repeats its first condition in
// place of the wrap test (the ROM's own code), so there a backward track that wrapped passes only the
// frame it came from.
static int TrackPassedFrame(const BEHAVIOR_TRACK *tr, u_int frame, int dir, int romSceneQuirk)
{
    u_int now = (u_int)tr->time1c & 0xffff, last = (u_int)tr->time1c >> 16;
    int reverse = (tr->flags & TRACK_FLAG_REVERSE) != 0;
    if (reverse ? !(dir & PASS_DIR_REVERSE) : !(dir & PASS_DIR_FORWARD))
        return 0;
    if (!reverse) {
        if (last < now)
            return last < frame && frame <= now;
        if (now < last)
            return frame <= now || last < frame;
        return last == frame;
    }
    if (now < last)
        return now <= frame && frame < last;
    if (last < now && !romSceneQuirk)
        return frame <= last || now < frame;
    return last == frame;
}

// CT3 0x01848fb0
// The triggers' "animation passed frame" condition. SCENE_ANIM_SCENE: any track in the groups of
// partId (0: the part named by hash) and of part 0 that the full selection picks (pose, other and
// byPart all on) whose time passed `frame` this frame in a direction dir accepts. SCENE_ANIM_CHARACTER
// (a CHARACTER_FLAG_REFRACTION character): the behaviour named by the motion slot playing `hash` (the
// last slot that matches), its group 0's first track; frame PASS_FRAME_ANY asks only whether one plays.
// The ROM returns distinct codes per case (0xa..0xc forward, 0x14..0x16 backward, 0x1e..0x23 either
// way) but only their being non-zero leaves the function.
int asCheckAnimationFlamePassing_S(u_long hash, ALL_TABLE *tbl, u_int frame, int partId, int dir, int target,
                                   int chara)
{
    int bank, g, t;
    BEHAVIOR *bhv;
    u_short part;
    if (tbl == NULL)
        return 0;
    if ((char)target == SCENE_ANIM_CHARACTER) {
        const CHARACTER *ch = &Character[(u_char)chara];
        const u_char *table;
        int i;
        if (!(ch->flags & CHARACTER_FLAG_REFRACTION))
            return 0;
        bhv = NULL;
        // PORT: no character has a host motion table yet (CharaMotionTable): nothing matches until it does.
        table = CharaMotionTable(ch);
        for (i = 0; table != NULL && i < CHARA_MOTION_SLOT_COUNT; i++) {
            short idx;
            u_long h, behaviorHash;
            memcpy(&idx, (const u_char *)ch + CHARA_MOTION_SLOTS + i * 2, sizeof(idx));
            if (idx == -1)
                continue;
            memcpy(&h, table + idx * CHARA_MOTION_SIZE, sizeof(h));
            if (h != hash)
                continue;
            memcpy(&behaviorHash, table + idx * CHARA_MOTION_SIZE + CHARA_MOTION_BEHAVIOR, sizeof(behaviorHash));
            bhv = asFindBehaviorByCharacter((u_char)chara, behaviorHash);
        }
        if (bhv == NULL)
            return 0;
        if (frame == (u_int)PASS_FRAME_ANY)
            return 1;
        return TrackPassedFrame(BehaviorTracks(BankOfHostPtr(bhv), bhv, 0), frame, dir, 0);
    }
    if ((char)target != SCENE_ANIM_SCENE)
        return 0;
    bank = BankOfHostPtr(tbl);
    bhv = BANK_PTR(BEHAVIOR *, bank, tbl->behavior);
    part = (u_short)partId;
    if (part == 0)
        part = (u_short)asFindModelPartsID(BANK_PTR(u_char *, bank, tbl->partsHeader), hash);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr;
        if (!((int)bhv->groups[g].part == (int)part || bhv->groups[g].part == 0))
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++)
            if (TrackSelected(bank, tr, (u_short)partId, 1, 1, 1) && TrackPassedFrame(tr, frame, dir, 1))
                return 1;
    }
    return 0;
}

// ---- stubs (tools/reverse/stubgen.py) ----



// CT3 0x0184f220
// The mirror pass's matrices (main, a character in a mirrored scene): asBuildDOFMatrixTree's loop with
// the reflection put in: REC_GA_MATRIX2 = Cam[CAMERA_SHARED].view2 * (mrr_ReverseMatrix * the part's
// REC_DOF_MATRIX, w of rows 0..2 zeroed), REC_GA_MATRIX = that * the diagonal scale (the identity from
// sdata 0x01937760 with REC_DOF_SCALE_*). From the disassembly (the decompiler dropped the first
// gaMulMatrix's third argument).
void asBuildMirrorMatrixTree(u_char *partsHeader)
{
    alignas(16) float scale[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; /* 0x01937760 */
    u_char *rec;
    for (rec = partsHeader + PARTS_RECORDS; !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        signed char kind = (signed char)rec[REC_KIND];
        float (*m2)[4] = (float (*)[4])(rec + REC_GA_MATRIX2);
        alignas(16) float m[4][4];
        if (kind == PARTS_KIND_STATIC || kind == PARTS_KIND_NO_POS || kind == PARTS_KIND_3)
            continue;
        memcpy(m, rec + REC_DOF_MATRIX, sizeof(m));
        scale[0][0] = *(float *)(rec + REC_DOF_SCALE_X);
        scale[1][1] = *(float *)(rec + REC_DOF_SCALE_Y);
        scale[2][2] = *(float *)(rec + REC_DOF_SCALE_Z);
        m[0][3] = m[1][3] = m[2][3] = 0.0f;
        gaMulMatrix(m2, mrr_ReverseMatrix, m);
        gaMulMatrix(m2, Cam[CAMERA_SHARED].view2, m2);
        gaMulMatrix((float (*)[4])(rec + REC_GA_MATRIX), m2, scale);
    }
}

// CT3 0x0184f120
// After the mirror pass: the parts' matrices back to the unreflected ones, the same body as
// asBuildDOFMatrixTree (matrix_global_view * REC_DOF_MATRIX, then the scale from sdata 0x019377a0).
void asBuildMirrorMatrixTreeBack(u_char *partsHeader)
{
    alignas(16) float scale[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; /* 0x019377a0 */
    u_char *rec;
    for (rec = partsHeader + PARTS_RECORDS; !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        signed char kind = (signed char)rec[REC_KIND];
        alignas(16) float m[4][4];
        if (kind == PARTS_KIND_STATIC || kind == PARTS_KIND_NO_POS || kind == PARTS_KIND_3)
            continue;
        memcpy(m, rec + REC_DOF_MATRIX, sizeof(m));
        scale[0][0] = *(float *)(rec + REC_DOF_SCALE_X);
        scale[1][1] = *(float *)(rec + REC_DOF_SCALE_Y);
        scale[2][2] = *(float *)(rec + REC_DOF_SCALE_Z);
        m[0][3] = m[1][3] = m[2][3] = 0.0f;
        gaMulMatrix((float (*)[4])(rec + REC_GA_MATRIX2), matrix_global_view, m);
        gaMulMatrix((float (*)[4])(rec + REC_GA_MATRIX), (float (*)[4])(rec + REC_GA_MATRIX2), scale);
    }
}


// CT3 0x0184f910
// The mirror's reflection from its plane (normal planeVec, w cleared here; a point planePoint):
// mrr_ReverseMatrix = RY^-1 * (RX^-1 * (flipY * (RX * RY))), RY turning the normal about Y
// (asRadianByO axis 1) and RX the turned normal about X (axis 0, negated), flipY = diag(1, -1, 1, 1)
// built from vf0; row 3 = the normal * 2 (n . p) / (n . n), w 1. 1 for a zero normal (nothing made).
int asMakeMirrorMatrixByPlane(float *planeVec, float *planePoint)
{
    alignas(16) float flip[4][4] = { { 1, 0, 0, 0 }, { 0, -1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    alignas(16) float ry[4][4];
    alignas(16) float rx[4][4];
    alignas(16) float v[4];
    alignas(16) float w[4];
    float a, d;
    int i;
    if (planeVec[0] == 0.0f && planeVec[1] == 0.0f && planeVec[2] == 0.0f)
        return 1;
    planeVec[3] = 0.0f;
    memcpy(v, planeVec, sizeof(v));
    a = asRadianByO(v, 1);
    gaSetRotMatrixY(ry, (int)((a * 65536.0f) / 6.2831855f) & 0xffff);
    for (i = 0; i < 4; i++) /* vmulabc / vmaddabc / vmaddbc: v * ry */
        w[i] = ry[0][i] * v[0] + ry[1][i] * v[1] + ry[2][i] * v[2] + ry[3][i] * v[3];
    a = asRadianByO(w, 0);
    gaSetRotMatrixX(rx, -((int)((a * 65536.0f) / 6.2831855f) & 0xffff));
    gaMulMatrix(mrr_ReverseMatrix, rx, ry);
    gaMulMatrix(mrr_ReverseMatrix, flip, mrr_ReverseMatrix);
    gaInversMatrix(rx, rx);
    gaMulMatrix(mrr_ReverseMatrix, rx, mrr_ReverseMatrix);
    gaInversMatrix(ry, ry);
    gaMulMatrix(mrr_ReverseMatrix, ry, mrr_ReverseMatrix);
    d = gaInnerProductXYZ(planeVec, planePoint);
    d = (d / gaInnerProductXYZ(planeVec, planeVec)) * 2.0f;
    for (i = 0; i < 3; i++)
        mrr_ReverseMatrix[3][i] = planeVec[i] * d;
    mrr_ReverseMatrix[3][3] = 1.0f;
    return 0;
}

// asMirrorCulling's modes (main: 0 before the mirror pass, 1 after it).
#define MIRROR_CULL_BEGIN 0
#define MIRROR_CULL_END 1
// screen_0's floats asMirrorCulling reads (gaSet3dlibStruct's screen; main.c has SCREEN_PROJ too).
#define SCREEN_X 0                      /* the screen's left edge */
#define SCREEN_Y 1                      /* its top edge */
#define SCREEN_W 2                      /* its width (512) */
#define SCREEN_H 3                      /* its height (448) */
#define SCREEN_CX 4                     /* the projection centre */
#define SCREEN_CY 5
#define SCREEN_PROJ_D 8                 /* the projection distance (512) */
#define CLIPPING_PORTAL 1               /* Clipping_System: main_rtn clips by Clipping_Portal */

float mrr_MirrorCullingRect[4];         /* sbss 0x0193cad0: the clip rectangle the mirror pass replaced
                                           (X1, X2, Y1, Y2) */
int mrr_MirrorClip_bk;                  /* sbss 0x0193cac0: Clipping_System before the mirror pass */

// CT3 0x0184f390
// The mirror pass draws only inside the mirror: MIRROR_CULL_BEGIN saves the clip rectangle and
// Clipping_System, turns the portal clip on and sets the rectangle to the screen bounds of the active
// mirror's four corners (each through its part's REC_GA_MATRIX, then projected: centre + d * x / z;
// y flipped about the screen's bottom edge), clamped to the screen. Every corner behind the eye
// (z < 0): an empty rectangle; some behind: the whole screen. MIRROR_CULL_END puts the saved ones back.
// From the disassembly. The rectangle is in screen_0's 512 x 448 space; the host's wide aspect comes
// from the renderer, not from here.
void asMirrorCulling(int on)
{
    signed char mode = (signed char)on;
    alignas(16) float v[4][4];
    float x0, y0, x1, y1, fx[4], fy[4], lo, hi, t;
    int i, j;
    if (mode == MIRROR_CULL_END) {
        Clipping_X1_pos = mrr_MirrorCullingRect[0];
        Clipping_X2_pos = mrr_MirrorCullingRect[1];
        Clipping_Y1_pos = mrr_MirrorCullingRect[2];
        Clipping_Y2_pos = mrr_MirrorCullingRect[3];
        Clipping_System = mrr_MirrorClip_bk;
        return;
    }
    if (mode != MIRROR_CULL_BEGIN)
        return;
    mrr_MirrorClip_bk = Clipping_System;
    Clipping_System = CLIPPING_PORTAL;
    mrr_MirrorCullingRect[0] = Clipping_X1_pos;
    mrr_MirrorCullingRect[1] = Clipping_X2_pos;
    mrr_MirrorCullingRect[2] = Clipping_Y1_pos;
    mrr_MirrorCullingRect[3] = Clipping_Y2_pos;
    {
        const float (*m)[4] = (const float (*)[4])(mrr_pActvMirrorMP + REC_GA_MATRIX);
        for (i = 0; i < 4; i++) {
            const float *p = mrr_pActvMirrorMO->p[i];
            for (j = 0; j < 4; j++)
                v[i][j] = m[0][j] * p[0] + m[1][j] * p[1] + m[2][j] * p[2] + m[3][j] * p[3];
        }
    }
    x0 = screen_0[SCREEN_X];
    y0 = screen_0[SCREEN_Y];
    x1 = screen_0[SCREEN_X] + screen_0[SCREEN_W];
    y1 = screen_0[SCREEN_Y] + screen_0[SCREEN_H];
    if (v[0][2] < 0.0f && v[1][2] < 0.0f && v[2][2] < 0.0f && v[3][2] < 0.0f) {
        Clipping_Y2_pos = Clipping_Y1_pos = Clipping_X2_pos = Clipping_X1_pos = 0.0f;
        return;
    }
    if (v[0][2] < 0.0f || v[1][2] < 0.0f || v[2][2] < 0.0f || v[3][2] < 0.0f) {
        Clipping_X1_pos = x0;
        Clipping_Y1_pos = y0;
        Clipping_Y2_pos = y1;
        Clipping_X2_pos = x1;
        return;
    }
    for (i = 0; i < 4; i++) {
        fx[i] = screen_0[SCREEN_CX] + (screen_0[SCREEN_PROJ_D] * v[i][0]) / v[i][2];
        fy[i] = screen_0[SCREEN_CY] + (screen_0[SCREEN_PROJ_D] * v[i][1]) / v[i][2];
    }
    /* x: min / max, clamped to the screen, ordered */
    lo = hi = fx[0];
    for (i = 1; i < 4; i++) {
        if (fx[i] < lo)
            lo = fx[i];
        else if (hi < fx[i])
            hi = fx[i];
    }
    if (lo < x0) lo = x0; else if (x1 < lo) lo = x1;
    if (hi < x0) hi = x0; else if (x1 < hi) hi = x1;
    if (hi < lo) { t = lo; lo = hi; hi = t; }
    Clipping_X1_pos = lo;
    Clipping_X2_pos = hi;
    /* y: min / max, flipped about the bottom edge, clamped, ordered */
    lo = hi = fy[0];
    for (i = 1; i < 4; i++) {
        if (fy[i] < lo)
            lo = fy[i];
        else if (hi < fy[i])
            hi = fy[i];
    }
    lo = y1 - lo;
    hi = y1 - hi;
    if (lo < y0) lo = y0; else if (y1 < lo) lo = y1;
    if (hi < y0) hi = y0; else if (y1 < hi) hi = y1;
    if (hi < lo) { t = lo; lo = hi; hi = t; }
    Clipping_Y1_pos = lo;
    Clipping_Y2_pos = hi;
}



// ---- the shadow volume pass (slice 11, 2026-11-10) ----
//
// A shadowed bank's volume is a list of ga draw objects in its bank data (ALL_TABLE.shadow's word 0),
// stocked once a frame by asStockShadowVolume. asShadowStart points drawing at the shadow buffer
// (SHADOW_FBUF_ADDRESS) and clears it with a full-screen sprite; the volume's faces then go in with
// SDW_drawctrl_A (adds) or SDW_drawctrl_S (subtracts), switched per object by its kind byte;
// asShadowEnd lays the result over the frame.

#define SHADOW_FBUF_ADDRESS 0xc0000     /* the shadow buffer: ga_draw_fbuf_address during the pass */
#define SHADOW_FBUF_WIDTH 0x200
#define SHADOW_SCISSOR_X1 0x1ff         /* 512 x 448 (screen_0's space) */
#define SHADOW_SCISSOR_Y1 0x1bf

// A shadow volume object's kind byte (+1; asStockShadowVolume writes it, asDrawShadowVolume clears it).
#define SV_OBJ_KIND 1
#define SV_KIND_END 0                   /* the list ends */
#define SV_KIND_SUB 1                   /* two plus quads (ShadowVolume2), drawn subtracting (SDW_drawctrl_S) */
#define SV_KIND_ADD 2                   /* two minus quads (ShadowVolume2), drawn adding (SDW_drawctrl_A) */
#define SV_KIND_SUB_5 5                 /* a lone plus quad (ShadowVolume1), the flush's; as SV_KIND_SUB */
#define SV_KIND_ADD_LAST 6              /* a lone minus quad, the flush's last object (no end mark after
                                           it); as SV_KIND_ADD */
#define SV_OBJ_SIZE 0x80
#define SV_CTRL_NONE 0                  /* asDrawShadowVolume: no draw control sent yet */
#define SV_CTRL_SUB 1
#define SV_CTRL_ADD (-1)

// The draw controls (gaSetupDrawCtrlData's 29 argument slots, from the disassembly): flat, UV, alpha
// blended, untextured; alpha test always passing, depth tested GEQUAL (the shadow buffer's ZBUF is
// masked: d_buffer_A's zMsk 1, so depth is read, never written). A: Cs * As + Cd; S: Cd - Cs * As.
static const GA_DRAWCTRL_ARGS SDW_DRAWCTRL_A_ARGS = {
    0, 0, 0, 1, 0, 1, 0, 0, /* PRMODE: ABE, FST */
    0, 1, 0, 1, 0, 0, 1, 2, /* TEST_1: ATST 1, AFAIL 1, ZTE, ZTST 2 */
    0, 2, 0, 1, 0,          /* ALPHA_1: A 0 (Cs), B 2 (0), C 0 (As), D 1 (Cd) */
    0,                      /* FBA_1 */
    0, 0, 1, 1, 0, 0,       /* TEX1_1: MMAG 1, MMIN 1 */
    0};                     /* TEXA */
static const GA_DRAWCTRL_ARGS SDW_DRAWCTRL_S_ARGS = {
    0, 0, 0, 1, 0, 1, 0, 0, /* PRMODE: ABE, FST */
    0, 1, 0, 1, 0, 0, 1, 2, /* TEST_1: as A */
    2, 0, 0, 1, 0,          /* ALPHA_1: A 2 (0), B 0 (Cs), C 0 (As), D 1 (Cd) */
    0,                      /* FBA_1 */
    0, 0, 1, 1, 0, 0,       /* TEX1_1 */
    0};                     /* TEXA */
// The shadow buffer's clear: flat, UV, no blend, alpha test always, no depth test (ZTST 1 ALWAYS).
static const GA_DRAWCTRL_ARGS EBCLEAR_B_ARGS = {
    0, 0, 0, 0, 0, 1, 0, 0, /* PRMODE: FST */
    0, 1, 0, 0, 0, 0, 0, 1, /* TEST_1: ATST 1, ZTST 1 */
    0, 1, 0, 1, 0,          /* ALPHA_1: A 0, B 1, C 0, D 1 */
    0,                      /* FBA_1 */
    0, 0, 1, 1, 0, 0,       /* TEX1_1: MMAG 1, MMIN 1 */
    0};                     /* TEXA */

alignas(16) static GA_DRAWCTRL_DATA SDW_drawctrl_S;     /* bss 0x0193c8c0 */
alignas(16) static GA_DRAWCTRL_DATA SDW_drawctrl_A;     /* bss 0x0193c980 */
static int asSD_BackBufferAdd;                          /* sbss 0x0193c880: ga_draw_fbuf_address before
                                                           the pass (asShadowEnd puts it back) */
alignas(16) static u_char tex_A_2748[0x100];            /* bss 0x0193c480: sunSetupTBufferData's buffer */
alignas(16) static GA_DBUFFER_DATA d_buffer_A_2747;     /* bss 0x0193c580 */
alignas(16) static GA_DRAWCTRL_DATA gaDC_EBclear_B_2753; /* bss 0x0193c3c0 */
static char iCngFlag_2749;                              /* sbss 0x0193bd70: tex_A / d_buffer_A set up */
static char iCngFlag_2754;                              /* sbss 0x0193bd74: gaDC_EBclear_B set up */
static char iCngFlag_2863;                              /* sbss 0x0193bd8c: the SDW_drawctrl_* set up */

/* sdata 0x01937d00: dc_EBclear_C$2757, the shadow buffer's clear: one sprite, RGBA 0x80808080, Q 1,
   (0x8000, 0x8000) .. (0xa000, 0x9c00) -- 512 x 448 at the GS offset. */
static const u_char dc_EBclear_C_init[0x50] = {
    0x04, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x50,
    0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x10, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x3f,
    0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x00,
};
static GA_DRAWCOMMAND_DATA dc_EBclear_C_make()
{
    GA_DRAWCOMMAND_DATA d;
    memcpy(&d, dc_EBclear_C_init, sizeof d);
    return d;
}
alignas(16) static GA_DRAWCOMMAND_DATA dc_EBclear_C_2757 = dc_EBclear_C_make();

/* data 0x01937830: ShadowVolume_light_work_data (0x190 bytes): a VIF UNPACK V4-32 of 16 qwords to
   0x180, zeros, and an identity matrix at +0x150; gaSetLight fills the rest. */
static u_char *ShadowVolumeLightWorkMake()
{
    alignas(16) static u_char work[0x190];
    static const u_int unpack = 0x6c100180u;
    static const float ident[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    memcpy(work + 0xc, &unpack, 4);
    memcpy(work + 0x150, ident, sizeof(ident));
    return work;
}
static u_char *ShadowVolume_light_work_data = ShadowVolumeLightWorkMake();

// CT3 0x018461f0
// The volume's light under the shadow view; the two draw controls once. The ROM first stores the
// addresses of ShadowVolume1/2_tex_st_data and _vertex_color_data into SDW_drawctrl_A +0x10..+0x1c,
// which gaSetupDrawCtrlData's packet head overwrites at once: not kept.
void asDrawShadowVolumeInit(void)
{
    gaSetLight(ShadowVolume_light_work_data, matrix_global_view_for_shadow);
    if (iCngFlag_2863 == 0) {
        gaSetupDrawCtrlData(&SDW_drawctrl_A, SDW_DRAWCTRL_A_ARGS);
        gaSetupDrawCtrlData(&SDW_drawctrl_S, SDW_DRAWCTRL_S_ARGS);
        iCngFlag_2863 = 1;
    }
}

// CT3 0x018471c0
// Drawing goes to the shadow buffer (the old address kept for asShadowEnd), its frame / texture
// buffers set up once and sent, then the clear sprite under gaDC_EBclear_B.
// PORT: sunSetupTBufferData's TEX_INFO.tim is the EE address of tex_attr_A$2750 (data 0x01937c80:
// 512 x 448, log2 9 x 9, +0x18 0x000e0000 -- the shadow buffer read as a texture); sunSetupTBufferData
// (QuatSlerp.c) keeps the host copy and resolves the word.
void asShadowStart(void)
{
    asSD_BackBufferAdd = ga_draw_fbuf_address;
    ga_draw_fbuf_address = SHADOW_FBUF_ADDRESS;
    if (iCngFlag_2749 == 0) {
        TEX_INFO tex;
        memset(&tex, 0, sizeof(tex));
        tex.tim = 0x01937c80u;                  /* tex_attr_A$2750 */
        tex.tbp = SHADOW_FBUF_ADDRESS;
        tex.wms = TEX_WRAP_CLAMP;
        tex.wmt = TEX_WRAP_CLAMP;
        sunSetupTBufferData(tex_A_2748, tex);
        gaSetupDBufferData(&d_buffer_A_2747, ga_draw_fbuf_address, SHADOW_FBUF_WIDTH, 0, 0, 0, 0, 1, 0, 0,
                           SHADOW_SCISSOR_X1, SHADOW_SCISSOR_Y1, 0, 0, 0);
        iCngFlag_2749 = 1;
    }
    FlushCache(0);
    gaBGT_ExecDBufferData_VIF1(&d_buffer_A_2747);
    sunBGT_ExecTBufferData_VIF1(tex_A_2748);
    if (iCngFlag_2754 == 0) {
        gaSetupDrawCtrlData(&gaDC_EBclear_B_2753, EBCLEAR_B_ARGS);
        iCngFlag_2754 = 1;
    }
    gaBGT_ExecDrawCtrlData_VIF1(&gaDC_EBclear_B_2753);
    gaBGT_ExecDrawCommandData_VIF1(&dc_EBclear_C_2757);
}

// CT3 0x018444b0
// One bank's shadow volume: only when its shadow is on (shadowVec w > 0), it has a shadow table, its
// shadowDraw bit 0 is set (cleared here) and the volume was stocked this frame. In the plain order
// (asShadowDrawing_CheckTransMode(SHADOW_TRANS_FIN) != 0) the bank starts and ends its own pass.
// Each object is drawn under the control its kind names, sent only when it changes; the kind is
// cleared (the stock is one frame's); SV_KIND_ADD_LAST ends the list after its draw.
void asDrawShadowVolume(int bank)
{
    int more = 1, ctrl = SV_CTRL_NONE;
    u_char *obj;
    u_int *table;
    BANK_INFO *bi;
    if (bank < 0)
        return;
    bi = &BankInfo[bank];
    table = All_table_TBL[bank]->shadow ? BANK_PTR(u_int *, bank, All_table_TBL[bank]->shadow) : NULL;
    if (bi->shadowVec[3] <= 0.0f || table == NULL)
        return;
    if ((bi->shadowDraw & 1) == 0)
        return;
    bi->shadowDraw &= 0xfe;
    if (bi->shadowFrame != FrameCounter)
        return;
    if (asShadowDrawing_CheckTransMode(SHADOW_TRANS_FIN) != 0) {
        asDrawShadowVolumeInit();
        asShadowStart();
    }
    for (obj = BANK_PTR(u_char *, bank, table[0]); obj[SV_OBJ_KIND] != SV_KIND_END && more; obj += SV_OBJ_SIZE) {
        switch (obj[SV_OBJ_KIND]) {
        case SV_KIND_ADD_LAST:
            more = 0;
            /* fall through */
        case SV_KIND_ADD:
            obj[SV_OBJ_KIND] = 0;
            if (ctrl != SV_CTRL_ADD) {
                gaBGT_ExecDrawCtrlData_VIF1(&SDW_drawctrl_A);
                ctrl = SV_CTRL_ADD;
            }
            break;
        case SV_KIND_SUB:
        case SV_KIND_SUB_5:
            obj[SV_OBJ_KIND] = 0;
            if (ctrl != SV_CTRL_SUB) {
                gaBGT_ExecDrawCtrlData_VIF1(&SDW_drawctrl_S);
                ctrl = SV_CTRL_SUB;
            }
            break;
        }
        gaBGT_DrawObject(obj, 0);
    }
    if (asShadowDrawing_CheckTransMode(SHADOW_TRANS_FIN) != 0)
        asShadowEnd(bank);
}

// ---- the volume's stock (slice 13, 2026-11-10) ----
//
// asShadowPolygonPositions hands asStockShadowVolume the volume's quads one by one (four position
// qwords each), as plus or minus faces. Two of a sign make one ga object (ShadowVolume2: 4 vertices
// in, 8 indices); a lone one left at the flush makes a ShadowVolume1 object (2 / 4). The bank's shadow
// table gives the three buffers: the objects (SV_OBJ_SIZE each), the positions (8 qwords an object)
// and the normals (a VIF header, then 8 qwords: one normal per vertex).

// The shadow table's words (ALL_TABLE.shadow): the buffers the stock fills.
#define SV_TABLE_OBJECTS 0
#define SV_TABLE_POSITIONS 1
#define SV_TABLE_NORMALS 2
#define SV_POSITIONS_SIZE 0x80          /* per object: 2 quads x 4 qwords */
#define SV_NORMALS_SIZE 0x90            /* per object: the UNPACK header, then 8 qwords */
#define SV_QUAD_VERTS 4
// ga draw object words the stock sets (the gaBGT_DrawObject TU owns the layout).
#define GA_OBJ_VERTS 0x44               /* vertices in */
#define GA_OBJ_INDICES 0x48             /* indices */
#define GA_OBJ_INDEX_DATA 0x50
#define GA_OBJ_POSITIONS 0x60
#define GA_OBJ_NORMALS 0x64
#define GA_OBJ_TEX_ST 0x68
#define GA_OBJ_COLORS 0x6c

// The ROM's templates (sdata). PORT: the index, ST and colour data are left as their EE addresses in
// the objects (asShadowStart's tex_attr_A precedent): the gaBGT_DrawObject TU resolves them.
#define SV1_VERTEX_INDEX_DATA 0x01937a90u       /* ShadowVolume1_vertex_index_data */
#define SV1_TEX_ST_DATA_VIFCODE 0x019379c0u     /* ShadowVolume1_tex_st_data_vifcode */
#define SV1_VERTEX_COLOR_DATA_VIFCODE 0x01937a40u /* ShadowVolume1_vertex_color_data_vifcode */
#define SV2_VERTEX_INDEX_DATA 0x01937ab0u       /* ShadowVolume2_vertex_index_data */
#define SV2_TEX_ST_DATA_VIFCODE 0x019379f0u     /* ShadowVolume2_tex_st_data_vifcode */
#define SV2_VERTEX_COLOR_DATA_VIFCODE 0x01937a60u /* ShadowVolume2_vertex_color_data_vifcode */
// sdata 0x01937b80 / 0x01937b90: ShadowVolume1/2_normal_data_vifcode, UNPACK V4-32 of 4 / 8 qwords.
static const u_int ShadowVolume1_normal_data_vifcode[4] = { 0, 0, 0, 0x6c048000u };
static const u_int ShadowVolume2_normal_data_vifcode[4] = { 0, 0, 0, 0x6c088000u };
// sdata 0x01937ba0: the volume object's template (byte 0 = 2; +0x1c 1.0f; +0x20 / +0x28 500.0f; +0x30
// 127.0f; +0x38 / +0x3c 0x01937820 / 0x01937830 -- the second ShadowVolume_light_work_data; +0x44 2,
// +0x48 4; +0x50 0x01937a90; +0x58 / +0x5c 0x01941840; +0x60..+0x6c the ShadowVolume1 buffers).
static const u_int ShadowVolumeObjectTemplate[GA_OBJECT_SIZE / 4] = {
    0x00000002, 0, 0, 0,
    0, 0, 0, 0x3f800000,
    0x43fa0000, 0, 0x43fa0000, 0,
    0x42fe0000, 0, 0x01937820, 0x01937830,
    0, 2, 4, 0,
    0x01937a90, 0, 0x01941840, 0x01941840,
    0x01937b00, 0x01937b80, 0x019379c0, 0x01937a40,
    0, 0, 0, 0,
};

static int iPcnt_3177;                  /* sbss 0x0193bf04: plus quads waiting (0 or 1) */
static int iMcnt_3178;                  /* sbss 0x0193bf00: minus quads waiting */
static int iFBflag_3179;                /* sbss 0x0193bd90: cleared at the start, never read here */
static u_char *lpGA_3180;               /* sbss 0x0193bda0: the next object */
static u_char *lpSVWP_3181;             /* sbss 0x0193bdc0: the next positions */
static u_char *lpSVWN_3182;             /* sbss 0x0193bdd0: the next normals */
alignas(16) static float gMP_3183[2][SV_QUAD_VERTS][4]; /* bss 0x0193be80: the waiting plus quads */
alignas(16) static float gMM_3184[2][SV_QUAD_VERTS][4]; /* bss 0x0193be00: the waiting minus quads */
static int svStockBank;                 /* PORT: the bank the buffers lie in (the objects keep EE words) */

// One quad's face normal, its four vertices' copies: (q2 - q1) x (q0 - q1), normalised (VU0
// OPMULA / OPMSUB, RSQRT); w = q2.w - q1.w (OPMSUB and MULQ write xyz only).
static void SvQuadNormals(float (*dst)[4], const float (*q)[4])
{
    float a[4], b[4], n[4], s, r;
    int i;
    for (i = 0; i < 4; i++) {
        a[i] = q[0][i] - q[1][i];
        b[i] = q[2][i] - q[1][i];
    }
    n[0] = b[1] * a[2] - a[1] * b[2];
    n[1] = b[2] * a[0] - a[2] * b[0];
    n[2] = b[0] * a[1] - a[0] * b[1];
    n[3] = b[3];
    s = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
    r = s > 0.0f ? 1.0f / sqrtf(s) : 0.0f;  // VU RSQRT of 0 gives +max: the zero normal stays zero
    n[0] *= r;
    n[1] *= r;
    n[2] *= r;
    for (i = 0; i < SV_QUAD_VERTS; i++)
        memcpy(dst[i], n, sizeof(n));
}

// One object at lpGA from quads[0..count-1] (1: ShadowVolume1, 2: ShadowVolume2) of kind `kind`.
static void SvStockObject(const float (*quads)[SV_QUAD_VERTS][4], int count, int kind)
{
    u_int verts = count == 2 ? 4 : 2, indices = count == 2 ? 8 : 4;
    gaCopyObject2(lpGA_3180, ShadowVolumeObjectTemplate);
    memcpy(lpSVWN_3182, count == 2 ? ShadowVolume2_normal_data_vifcode : ShadowVolume1_normal_data_vifcode, 16);
    memcpy(lpGA_3180 + GA_OBJ_VERTS, &verts, 4);
    memcpy(lpGA_3180 + GA_OBJ_INDICES, &indices, 4);
    {
        u_int idx = count == 2 ? SV2_VERTEX_INDEX_DATA : SV1_VERTEX_INDEX_DATA;
        u_int st = count == 2 ? SV2_TEX_ST_DATA_VIFCODE : SV1_TEX_ST_DATA_VIFCODE;
        u_int col = count == 2 ? SV2_VERTEX_COLOR_DATA_VIFCODE : SV1_VERTEX_COLOR_DATA_VIFCODE;
        u_int pos = BankEeAddr(svStockBank, lpSVWP_3181);
        u_int nrm = BankEeAddr(svStockBank, lpSVWN_3182);
        memcpy(lpGA_3180 + GA_OBJ_INDEX_DATA, &idx, 4);
        memcpy(lpGA_3180 + GA_OBJ_POSITIONS, &pos, 4);
        memcpy(lpGA_3180 + GA_OBJ_NORMALS, &nrm, 4);
        memcpy(lpGA_3180 + GA_OBJ_TEX_ST, &st, 4);
        memcpy(lpGA_3180 + GA_OBJ_COLORS, &col, 4);
    }
    lpGA_3180[SV_OBJ_KIND] = (u_char)kind;
    for (int i = 0; i < count; i++) {
        SvQuadNormals((float (*)[4])(lpSVWN_3182 + 0x10 + i * SV_QUAD_VERTS * 16), quads[i]);
        memcpy(lpSVWP_3181 + i * SV_QUAD_VERTS * 16, quads[i], SV_QUAD_VERTS * 16);
    }
}

// The next object: past this one, the list's end marked after it.
static void SvStockAdvance(void)
{
    lpSVWN_3182 += SV_NORMALS_SIZE;
    lpSVWP_3181 += SV_POSITIONS_SIZE;
    lpGA_3180 += SV_OBJ_SIZE;
    lpGA_3180[SV_OBJ_KIND] = SV_KIND_END;
}

// CT3 0x01844650
// SV_STOCK_BEGIN points the stock at the bank's shadow table buffers; SV_STOCK_PLUS / _MINUS take one
// quad (4 position qwords), a pair making an object (SV_KIND_SUB / _ADD); SV_STOCK_FLUSH turns a lone
// plus quad into an SV_KIND_SUB_5 object and a lone minus one into the last, SV_KIND_ADD_LAST (no end
// mark after it). A negative bank does nothing.
void asStockShadowVolume(int bank, float (*quad)[4], int mode)
{
    // callcheck: gaCopyObject2 (SvStockObject makes it)
    if (bank < 0)
        return;
    switch (mode) {
    case SV_STOCK_FLUSH:
        if (iPcnt_3177 != 0) {
            SvStockObject(gMP_3183, 1, SV_KIND_SUB_5);
            SvStockAdvance();
        }
        if (iMcnt_3178 != 0)
            SvStockObject(gMM_3184, 1, SV_KIND_ADD_LAST);
        break;
    case SV_STOCK_MINUS:
        memcpy(gMM_3184[iMcnt_3178++], quad, sizeof(gMM_3184[0]));
        if ((iMcnt_3178 & 1) == 0) {
            iMcnt_3178 = 0;
            SvStockObject(gMM_3184, 2, SV_KIND_ADD);
            SvStockAdvance();
        }
        break;
    case SV_STOCK_PLUS:
        memcpy(gMP_3183[iPcnt_3177++], quad, sizeof(gMP_3183[0]));
        if ((iPcnt_3177 & 1) == 0) {
            iPcnt_3177 = 0;
            SvStockObject(gMP_3183, 2, SV_KIND_SUB);
            SvStockAdvance();
        }
        break;
    case SV_STOCK_BEGIN: {
        ALL_TABLE *tbl = All_table_TBL[bank];
        const u_int *table = BANK_PTR(const u_int *, bank, tbl->shadow);
        iPcnt_3177 = 0;
        iMcnt_3178 = 0;
        iFBflag_3179 = 0;
        svStockBank = bank;
        lpGA_3180 = BANK_PTR(u_char *, bank, table[SV_TABLE_OBJECTS]);
        lpSVWP_3181 = BANK_PTR(u_char *, bank, table[SV_TABLE_POSITIONS]);
        lpSVWN_3182 = BANK_PTR(u_char *, bank, table[SV_TABLE_NORMALS]);
        break;
    }
    }
}

// ---- the shadow polygons (slice 13, 2026-11-10) ----
//
// Once a frame (main) each shadowed bank's casters become volume quads: the caster's vertices are
// placed by its part, pushed away from the light by shadowLength (the far cap), and every silhouette
// edge of a lit face (or of a segment's outline) gives a quad (far a, near a, near b, far b) under the
// plane through the light and the edge. A quad is a plus face when the camera and the caster's centre
// lie on the same side of that plane, a minus face otherwise.

// The shadow table's work words after the stock's three (ALL_TABLE.shadow).
#define SP_TABLE_NEAR 3                 /* float[][4]: the caster's vertices, placed */
#define SP_TABLE_FAR 4                  /* ... pushed away from the light */
#define SP_TABLE_RAYS 5                 /* ... minus the light */
#define SP_TABLE_PLANES 6               /* per edge: the plane's normal, w = its offset */
#define SP_TABLE_LEVELS 7               /* 8 x (count, SHADOW_CASTER list), by shadowColor[3] */
#define SP_LEVELS 8
#define SP_RING_MAX 70                  /* the ROM's stack index list (u_short[70]) */
#define SP_ANCHOR_HASH 0x74dda77d0cbcf0c2ull /* a character bank's shadow anchor part (name not known) */
#define SP_PARALLEL_DOT 0.99f           /* |axis . light| at or above it: the segment is end-on */
#define SP_SEGMENT_RADIUS_SCALE 10.0f   /* a segment end's w: low 16 bits radius * 10, high 16 points */
#define REC_SHADOW_SCALE 0x2c0          /* float[3]: REC_SCALE, the caster part's diagonal */
#define REC_SHADOW_MATRIX 0x1c0         /* float[4][4]: REC_MATRIX_PREV, the caster part's matrix */

// One caster of a level's list (0x10 bytes, bank data). layout: ee.
typedef struct SHADOW_CASTER {
    u_short part;                       /* 0x0: the part placing it (its matrix * its scale) */
    u_short polyCount;                  /* 0x2: 0: a segment (two qwords at verts) */
    u_int polys;                        /* 0x4: u_short lists: n, then n vertex indices */
    u_short vertCount;                  /* 0x8 */
    u_short hidePart;                   /* 0xa: skipped while this part's REC_FLAGS bit 0 is set */
    u_int verts;                        /* 0xc: float[vertCount][4] */
} SHADOW_CASTER;
static_assert(sizeof(SHADOW_CASTER) == 0x10, "SHADOW_CASTER");

// PORT: the bank whose relocated EE range holds `ee` (BankOfHostPtr's counterpart): the light position
// a bank's shadow is attached to lies in another bank's parts.
static int BankOfEeAddr(u_int ee)
{
    int bank, best = -1;
    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankImage[bank] != NULL && BankImageEE[bank] <= ee && (best < 0 || BankImageEE[bank] > BankImageEE[best]))
            best = bank;
    }
    return best;
}

// VU0 helpers, fields as the ROM's: rows-as-vectors matrix * v (MULAx .. MADDw, xyzw).
static void SpMulMatVec(float *dst, const float (*m)[4], const float *v)
{
    float r[4];
    for (int i = 0; i < 4; i++)
        r[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    memcpy(dst, r, sizeof(r));
}
// xyz scaled to unit length (RSQRT; w kept).
static void SpNormalizeXYZ(float *v)
{
    float s = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    float r = s > 0.0f ? 1.0f / sqrtf(s) : 0.0f;
    v[0] *= r;
    v[1] *= r;
    v[2] *= r;
}
// dst.xyz = a x b (OPMULA a, b; OPMSUB b, a); dst.w = a.w.
static void SpCross(float *dst, const float *a, const float *b)
{
    float r[4] = { a[1] * b[2] - b[1] * a[2], a[2] * b[0] - b[2] * a[0], a[0] * b[1] - b[0] * a[1], a[3] };
    memcpy(dst, r, sizeof(r));
}
static float SpDot(const float *a, const float *b)
{
    return gaInnerProductXYZ((float *)a, (float *)b);
}
// The ROM's side test: 0 for +-0, -1 below, 1 otherwise (NaN too).
static float SpSign(float f)
{
    if (f == 0.0f)
        return 0.0f;
    return f < 0.0f ? -1.0f : 1.0f;
}

// far = (near - light) * (length / |near - light| + 1) + light, xyz (w: near's), for n vertices.
static void SpExtrude(float (*nearV)[4], float (*farV)[4], float (*rays)[4], int n, const float *light, float length)
{
    for (int i = 0; i < n; i++) {
        float k;
        rays[i][0] = nearV[i][0] - light[0];
        rays[i][1] = nearV[i][1] - light[1];
        rays[i][2] = nearV[i][2] - light[2];
        rays[i][3] = nearV[i][3];
        k = length / asVectorLength3D(rays[i]) + 1.0f;
        farV[i][0] = rays[i][0] * k + light[0];
        farV[i][1] = rays[i][1] * k + light[1];
        farV[i][2] = rays[i][2] * k + light[2];
        farV[i][3] = rays[i][3];
    }
}

// The plane through the light and edge (a, b): normalize(ray a x ray b), w = -(n . light).
static void SpEdgePlane(float *plane, const float *rayA, const float *rayB, const float *light)
{
    SpCross(plane, rayA, rayB);
    SpNormalizeXYZ(plane);
    plane[3] = -SpDot(plane, light);
}

// Edge (a, b)'s quad to the stock: plus when the camera and `ref` are on the same side of a's plane.
static void SpStockEdge(int bank, float (*nearV)[4], float (*farV)[4], const float (*planes)[4], int a, int b,
                        const float *cam, const float *ref)
{
    alignas(16) float quad[SV_QUAD_VERTS][4];
    float sCam, sRef;
    memcpy(quad[0], farV[a], 16);
    memcpy(quad[1], nearV[a], 16);
    memcpy(quad[2], nearV[b], 16);
    memcpy(quad[3], farV[b], 16);
    sCam = SpSign(SpDot(cam, planes[a]) + planes[a][3]);
    sRef = SpSign(SpDot(ref, planes[a]) + planes[a][3]);
    asStockShadowVolume(bank, quad, sCam == sRef ? SV_STOCK_PLUS : SV_STOCK_MINUS);
}

// A segment caster's outline into nearV: two half rings (end-on: one full ring about the nearer
// looking end). Answers the vertex count, -1 when nothing is cast; *split the ring index from which
// refB replaces refA as the side test's reference.
static int SpSegmentOutline(float (*nearV)[4], const float (*m)[4], const float *src, const float *light,
                            float *refA, float *refB, int *split)
{
    alignas(16) float p0[4], p1[4], axis[4], toL[4], axisN[4], toLN[4], d[4], c[4], rot[4][4];
    u_int w0, w1;
    float r0, r1, dot;
    short n0, n1;
    int count, i;
    memcpy(p0, src, 16);
    memcpy(p1, src + 4, 16);
    memcpy(&w0, &p0[3], 4);
    memcpy(&w1, &p1[3], 4);
    r0 = (float)(w0 & 0xffff) / SP_SEGMENT_RADIUS_SCALE;
    r1 = (float)(w1 & 0xffff) / SP_SEGMENT_RADIUS_SCALE;
    n0 = (short)(w0 >> 16);
    n1 = (short)(w1 >> 16);
    p0[3] = 1.0f;
    p1[3] = 1.0f;
    SpMulMatVec(p0, m, p0);
    SpMulMatVec(p1, m, p1);
    for (i = 0; i < 4; i++) {
        axis[i] = p1[i] - p0[i];
        toL[i] = light[i] - p0[i];
    }
    memcpy(axisN, axis, 16);
    SpNormalizeXYZ(axisN);
    memcpy(toLN, toL, 16);
    SpNormalizeXYZ(toLN);
    dot = SpDot(axisN, toLN);
    if (dot < SP_PARALLEL_DOT && dot > -SP_PARALLEL_DOT) {
        // side by side: a half ring of n + 2 points about each end, turned about the light ray
        SpCross(d, toL, axis);
        SpNormalizeXYZ(d);
        for (i = 0; i < 4; i++) {
            c[i] = d[i] * -r1;
            d[i] *= r0;
        }
        asMakeRotMatrixByQuartanion(rot, toL, 0x10000 - 0x10000 / ((n0 + 1) * 2));
        for (i = 0; i < 4; i++)
            nearV[0][i] = p0[i] + d[i];
        for (count = 1; count < n0 + 2; count++) {
            SpMulMatVec(d, rot, d);
            for (i = 0; i < 4; i++)
                nearV[count][i] = p0[i] + d[i];
        }
        asMakeRotMatrixByQuartanion(rot, toL, 0x10000 - 0x10000 / ((n1 + 1) * 2));
        for (i = 0; i < 4; i++)
            nearV[count][i] = p1[i] + c[i];
        for (int k = 1; k < n1 + 2; k++) {
            SpMulMatVec(c, rot, c);
            count++;
            for (i = 0; i < 4; i++)
                nearV[count][i] = p1[i] + c[i];
        }
        count = (u_short)(n0 + n1 + 4);
        *split = n0 + 2;
    } else {
        // end-on: a full ring about the end whose radius looks the larger from the light
        float e0[4], e1[4], f[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        for (i = 0; i < 4; i++) {
            e0[i] = p0[i] - light[i];
            e1[i] = p1[i] - light[i];
        }
        {
            float l0 = asVectorLength3D(e0), l1 = asVectorLength3D(e1);
            if ((r0 / l0 <= r1 / l1 && n1 != 0) || n0 == 0) {
                memcpy(p0, p1, 16);
                r0 = r1;
                n0 = n1;
            }
        }
        if (n0 == 0)
            return -1;
        count = (n0 + 1) * 2;
        if (SpDot(f, toLN) >= SP_PARALLEL_DOT || SpDot(f, toLN) <= -SP_PARALLEL_DOT) {
            f[0] = 0.0f;
            f[1] = 1.0f;
        }
        SpCross(f, f, toLN);
        SpNormalizeXYZ(f);
        for (i = 0; i < 4; i++)
            f[i] *= r0;
        asMakeRotMatrixByQuartanion(rot, toLN, 0x10000 / count);
        for (i = 0; i < 4; i++)
            nearV[0][i] = p0[i] + f[i];
        for (int k = 1; k < count; k++) {
            SpMulMatVec(f, rot, f);
            for (i = 0; i < 4; i++)
                nearV[k][i] = p0[i] + f[i];
        }
        *split = count;
    }
    // the side test's references: an end without points is replaced by the middle
    for (i = 0; i < 4; i++) {
        refA[i] = n0 == 0 ? (p0[i] + p1[i]) * 0.5f : p0[i];
        refB[i] = n1 == 0 ? (p0[i] + p1[i]) * 0.5f : p1[i];
    }
    return count;
}

// CT3 0x01844df0
// main, every shadowed bank (and each refracting shadowed character's bank) once a frame: marks the
// bank's shadow for this frame (shadowFrame, shadowDraw bit 0, iPleaseShadowDraw), takes the light
// (the attached position, for a character bank the angle-locked one above its anchor), and stocks the
// volume of every caster of the level shadowColor[3] picks whose hide part is shown -- last caster
// first. Nothing while the shadow is off, the table missing or the level empty; no quads while the
// light's w is not positive.
void asShadowPolygonPositions(int bank)
{
    BANK_INFO *bi;
    ALL_TABLE *tbl;
    const u_int *st;
    u_char *parts;
    const SHADOW_CASTER *list = NULL;
    alignas(16) float cam[4], lockLight[4];
    const float *light;
    int count = 0, c;
    // callcheck: asMakeRotMatrixByQuartanion, asVectorLength3D, gaInnerProductXYZ (SpSegmentOutline, SpExtrude and SpDot make them)
    if (bank < 0)
        return;
    bi = &BankInfo[bank];
    tbl = All_table_TBL[bank];
    if (!(bi->shadowVec[3] > 0.0f) || tbl == NULL || tbl->shadow == 0)
        return;
    st = BANK_PTR(const u_int *, bank, tbl->shadow);
    bi->shadowFrame = FrameCounter;
    iPleaseShadowDraw = 1;
    bi->shadowDraw |= 1;
    if (bi->shadowColor[3] < SP_LEVELS) {
        count = (int)st[SP_TABLE_LEVELS + bi->shadowColor[3] * 2];
        list = BANK_PTR(const SHADOW_CASTER *, bank, st[SP_TABLE_LEVELS + bi->shadowColor[3] * 2 + 1]);
    }
    if (count == 0)
        return;
    asViewMatrix2PosRUF(&matrix_global_view_for_shadow[0][0], cam, cam, cam, cam);
    if (bi->shadowLightPos != 0) {
        int lb = BankOfEeAddr(bi->shadowLightPos);
        if (lb >= 0)
            memcpy(bi->shadowVec, BankEePtr(lb, bi->shadowLightPos), sizeof(bi->shadowVec));
    }
    light = bi->shadowVec;
    {
        float *anchor = bi->shadowAnchor;
        if (bi->type == FS_LOAD_SHAPE) {
            u_char *hdr = BANK_PTR(u_char *, bank, tbl->partsHeader);
            u_char *rec = asFindModelParts(hdr, SP_ANCHOR_HASH);
            if (rec == NULL) {
                rec = asFindModelParts(hdr, PARTS_HASH_SOUND_ORIGIN);
                memcpy(anchor, PARTS_REC(hdr, 0) + REC_POS, 16);
                if (rec != NULL) {
                    memcpy(&anchor[0], rec + REC_POS, 4);
                    memcpy(&anchor[2], rec + REC_POS + 8, 4);
                }
            } else {
                memcpy(anchor, rec + REC_POS, 16);
            }
            if (bi->shadow6d != 0) {
                // the angle lock: the light at shadow68 from a point shadow64 above the anchor, level with it
                float p[4], d[4];
                memcpy(p, anchor, 16);
                p[1] += bi->shadow64;
                d[0] = light[0] - p[0];
                d[1] = 0.0f;                    // (light.x, p.y, light.z) - p
                d[2] = light[2] - p[2];
                d[3] = light[3] - p[3];
                SpNormalizeXYZ(d);
                lockLight[0] = p[0] + d[0] * bi->shadow68;
                lockLight[1] = p[1] + d[1] * bi->shadow68;
                lockLight[2] = p[2] + d[2] * bi->shadow68;
                lockLight[3] = bi->shadowVec[3];
                light = lockLight;
            }
        } else {
            memcpy(anchor, bi->shadowVec, 16);
        }
    }
    parts = PARTS_REC(BANK_PTR(u_char *, bank, tbl->partsHeader), 0);
    if (!(light[3] > 0.0f))
        return;
    asStockShadowVolume(bank, NULL, SV_STOCK_BEGIN);
    for (c = count - 1; c >= 0; c--) {
        const SHADOW_CASTER *e = &list[c];
        u_char *rec = parts + e->part * 0x380;
        alignas(16) float m[4][4];
        float (*nearV)[4] = BANK_PTR(float (*)[4], bank, st[SP_TABLE_NEAR]);
        float (*farV)[4] = BANK_PTR(float (*)[4], bank, st[SP_TABLE_FAR]);
        float (*rays)[4] = BANK_PTR(float (*)[4], bank, st[SP_TABLE_RAYS]);
        float (*planes)[4] = BANK_PTR(float (*)[4], bank, st[SP_TABLE_PLANES]);
        const float (*verts)[4] = BANK_PTR(const float (*)[4], bank, e->verts);
        u_int hide;
        memcpy(&hide, parts + e->hidePart * 0x380 + REC_FLAGS, 4);
        if (hide & 1)
            continue;
        // the part's matrix * its scale (matUnit with the diagonal from REC_SHADOW_SCALE)
        memcpy(m, matUnit, sizeof(m));
        memcpy(&m[0][0], rec + REC_SHADOW_SCALE, 4);
        memcpy(&m[1][1], rec + REC_SHADOW_SCALE + 4, 4);
        memcpy(&m[2][2], rec + REC_SHADOW_SCALE + 8, 4);
        gaMulMatrix(m, (float (*)[4])(rec + REC_SHADOW_MATRIX), m);
        if (e->polys == 0) {
            // a segment: its outline is the ring the quads go round
            alignas(16) float refA[4], refB[4];
            u_short ring[SP_RING_MAX + 1];
            int split, n = SpSegmentOutline(nearV, (const float (*)[4])m, verts[0], light, refA, refB, &split), i;
            const float *ref = refA;
            if (n < 0)
                continue;
            if (n > SP_RING_MAX)
                n = SP_RING_MAX;        // PORT: the ROM's stack list would overflow
            SpExtrude(nearV, farV, rays, n, light, bi->shadowLength);
            for (i = 0; i < n; i++)
                ring[i] = (u_short)i;
            ring[n] = 0;
            for (i = 0; i < n; i++)
                SpEdgePlane(planes[ring[i]], rays[ring[i]], rays[ring[i + 1]], light);
            for (i = 0; i < n; i++) {
                if (i == split)
                    ref = refB;
                SpStockEdge(bank, nearV, farV, (const float (*)[4])planes, ring[i], ring[i + 1], cam, ref);
            }
        } else {
            // polygons: each lit face's edges
            const u_short *poly = BANK_PTR(const u_short *, bank, e->polys);
            u_int at = 0;
            int i, k;
            for (i = 0; i < e->vertCount; i++)
                SpMulMatVec(nearV[i], (const float (*)[4])m, verts[i]);
            SpExtrude(nearV, farV, rays, e->vertCount, light, bi->shadowLength);
            for (i = 0; i < e->polyCount; i++) {
                int n = poly[at];
                const u_short *v = &poly[at + 1];
                alignas(16) float centre[4], a[4], b[4], fn[4];
                at += 1 + n;
                for (k = 0; k < 4; k++)
                    centre[k] = nearV[v[0]][k] + nearV[v[1]][k] + nearV[v[2]][k];
                for (k = 0; k < 3; k++)
                    centre[k] *= 1.0f / 3.0f;   // VMULx.xyz: w stays the sum (unread)
                for (k = 0; k < 3; k++) {
                    a[k] = nearV[v[0]][k] - nearV[v[1]][k];
                    b[k] = nearV[v[0]][k] - nearV[v[2]][k];
                }
                a[3] = b[3] = nearV[v[0]][3];
                SpCross(fn, a, b);
                SpNormalizeXYZ(fn);
                if (!(SpDot(fn, rays[v[0]]) > 0.0f))
                    continue;
                for (k = 0; k < n - 1; k++)
                    SpEdgePlane(planes[v[k]], rays[v[k]], rays[v[k + 1]], light);
                for (k = 0; k < n - 1; k++)
                    SpStockEdge(bank, nearV, farV, (const float (*)[4])planes, v[k], v[k + 1], cam, centre);
            }
        }
    }
    asStockShadowVolume(bank, NULL, SV_STOCK_FLUSH);
}

// ---- asShadowEnd (slice 14, 2026-11-10) ----
//
// The shadow buffer counted the volume faces from 0x80 (asShadowStart's clear): +0x60 under D, then
// -0xe0 under F folds it to "how far into shadow" (0 outside). Then drawing goes back to the frame and
// the shadow buffer, read as a texture (asShadowStart's tex_attr_A), is laid over it, coloured by the
// bank's shadowColor * |shadowOrder| / 100: a negative value lightens (W: Cd + Cs * 2), a positive one
// darkens (B: Cd - Cs * 2). The split pass covers the screen with 8 x 7 sprites of 64 x 64; the plain
// pass draws four Gouraud fans about the anchor's screen point, coloured there and black at the edges.
// All in screen_0's 512 x 448 space (the shadow buffer's): the host widens the 3D, not this pass.

#define SHADOW_TILE 64                  /* the split pass's sprites: 8 x 7 of them over 512 x 448 */
#define SHADOW_TILES_X 8
#define SHADOW_TILES_Y 7
#define SHADOW_SCREEN_W 0x200
#define SHADOW_SCREEN_H 0x1c0
#define SCREEN_SCALE_X 6                /* screen_0[6]: x's projection scale (0.832) */
#define SCREEN_SCALE_Y 7                /* screen_0[7]: y's (0.94) */
#define SHADOW_FAN_SIDE_LOW 0           /* the anchor clamped to the screen's low edge */
#define SHADOW_FAN_SIDE_IN 1
#define SHADOW_FAN_SIDE_HIGH 2          /* clamped to the high edge */
#define SHADOW_FAN_COUNT 4
#define SHADOW_FAN_SIZE 0xa0            /* one fan's packet: 10 qwords (ShadowBlock[bank] holds 4) */
#define SHADOW_FAN_RINGS 1              /* the ROM's ring loop runs once (ring 0: the full colour) */

// The fold's two controls: untextured, alpha test always, no depth test; D adds Cs * FIX, F
// subtracts it (FIX 0x80 = 1.0). From the disassembly.
static const GA_DRAWCTRL_ARGS EBCLEAR_D_ARGS = {
    0, 0, 0, 1, 0, 0, 0, 0, /* PRMODE: ABE */
    0, 1, 0, 1, 0, 0, 0, 1, /* TEST_1: ATST 1, AFAIL 1, ZTST 1 */
    0, 2, 2, 1, 0x80,       /* ALPHA_1: Cs * FIX + Cd */
    0,                      /* FBA_1 */
    1, 0, 1, 1, 0, 0,       /* TEX1_1 */
    0};                     /* TEXA */
static const GA_DRAWCTRL_ARGS EBCLEAR_F_ARGS = {
    0, 0, 0, 1, 0, 0, 0, 0, /* PRMODE: ABE */
    0, 1, 0, 1, 0, 0, 0, 1, /* TEST_1: as D */
    2, 0, 2, 1, 0x80,       /* ALPHA_1: Cd - Cs * FIX */
    0,                      /* FBA_1 */
    1, 0, 1, 1, 0, 0,       /* TEX1_1 */
    0};                     /* TEXA */
// The overlay's controls: textured (TME, FST), blended, alpha test always, no depth test; W adds
// Cs * FIX, B subtracts it (FIX 0xff: about 2.0). The fans' (2808 / 2807) add Gouraud (IIP).
#define EBCLEAR_OVERLAY_ARGS(iip, a, b) {                                     \
    iip, 1, 0, 1, 0, 1, 0, 0, /* PRMODE: IIP?, TME, ABE, FST */               \
    0, 1, 0, 0, 0, 0, 0, 1,   /* TEST_1: ATST 1, ZTST 1 */                    \
    a, b, 2, 1, 0xff,         /* ALPHA_1: A, B, C 2 (FIX), D 1 (Cd), FIX */   \
    0,                        /* FBA_1 */                                     \
    0, 0, 1, 1, 0, 0,         /* TEX1_1: MMAG 1, MMIN 1 */                    \
    0}                        /* TEXA */
static const GA_DRAWCTRL_ARGS EBCLEAR_W_ARGS = EBCLEAR_OVERLAY_ARGS(0, 0, 2);
static const GA_DRAWCTRL_ARGS EBCLEAR_B_OVERLAY_ARGS = EBCLEAR_OVERLAY_ARGS(0, 2, 0);
static const GA_DRAWCTRL_ARGS EBCLEAR_W_FAN_ARGS = EBCLEAR_OVERLAY_ARGS(1, 0, 2);
static const GA_DRAWCTRL_ARGS EBCLEAR_B_FAN_ARGS = EBCLEAR_OVERLAY_ARGS(1, 2, 0);

alignas(16) static GA_DRAWCTRL_DATA gaDC_EBclear_D_2764;      /* bss 0x0193c300 */
alignas(16) static GA_DRAWCTRL_DATA gaDC_EBclear_F_2769;      /* bss 0x0193c240 */
alignas(16) static GA_DRAWCTRL_DATA gaDC_EBclearW_2780;       /* bss 0x0193c0c0 */
alignas(16) static GA_DRAWCTRL_DATA gaDC_EBclearB_2779;       /* bss 0x0193c180 */
alignas(16) static GA_DRAWCTRL_DATA gaDC_EBclearW_2808;       /* bss 0x0193bf40 */
alignas(16) static GA_DRAWCTRL_DATA gaDC_EBclearB_2807;       /* bss 0x0193c000 */
static char iCngFlag_2765;                                    /* sbss 0x0193bd78 */
static char iCngFlag_2770;                                    /* sbss 0x0193bd7c */
static char iCngFlag_2781;                                    /* sbss 0x0193bd80 */
static int iInit_2788;                                        /* sbss 0x0193bd84 */
static char iCntFlag_2809;                                    /* sbss 0x0193bd88 */

/* sdata 0x01937d50 dc_EBclear_E$2768 / 0x01937da0 dc_EBclear_G$2773: dc_EBclear_C's sprite (512 x 448)
   with RGBA 0x60606080 / 0xe0e0e080. */
static GA_DRAWCOMMAND_DATA dc_EBclear_make(u_char rgb)
{
    GA_DRAWCOMMAND_DATA d;
    memcpy(&d, dc_EBclear_C_init, sizeof d);
    ((u_char *)&d)[0x38] = ((u_char *)&d)[0x39] = ((u_char *)&d)[0x3a] = rgb;
    return d;
}
alignas(16) static GA_DRAWCOMMAND_DATA dc_EBclear_E_2768 = dc_EBclear_make(0x60);
alignas(16) static GA_DRAWCOMMAND_DATA dc_EBclear_G_2773 = dc_EBclear_make(0xe0);

/* bss 0x0194bec0 asFS$2787: the split pass's 56 sprites, 0x60 bytes each (DMAtag CNT 5, VIF DIRECT 4,
   GIFtag reglist PRIM RGBAQ UV XYZ2 UV XYZ2). */
#define ASFS_QW 12                      /* u_longs per sprite */
#define ASFS_RGBAQ 7
alignas(16) static u_long asFS_2787[SHADOW_TILES_X * SHADOW_TILES_Y][ASFS_QW];

// GIF words of the two packet kinds (from the ROM's stores).
#define SHADOW_DMATAG_CNT(qwc) (0x10000000ull | (qwc))
#define SHADOW_VIF_DIRECT(qwc) ((u_long)(0x50000000u | (qwc)) << 32)
#define SHADOW_GIFTAG_SPRITE 0x6400000000008001ull      /* NLOOP 1, EOP, reglist, NREG 6 */
#define SHADOW_GIFREGS_SPRITE 0x434310ull               /* PRIM RGBAQ UV XYZ2 UV XYZ2 */
#define SHADOW_GIFTAG_FAN 0xe400000000008001ull         /* NLOOP 1, EOP, reglist, NREG 14 */
#define SHADOW_GIFREGS_FAN 0xd4314314314310ull          /* PRIM, 4 x (RGBAQ UV XYZ2), XYZ3 */
#define GS_PRIM_TRIFAN 5
#define GS_PRIM_SPRITE 6
#define SHADOW_RGBAQ_BLACK 0x3f80000000000000ull        /* RGBA 0, Q 1.0 */
#define SHADOW_Q_ONE 1.0f

// One fan: centre, three corners (screen pixels / UV in 1/16 texel), the last corner again as XYZ3.
static void ShadowFan(u_long *p, u_long rgbaq, u_int uvC, u_long xyzC, u_int uv1, u_long xyz1, u_int uv2,
                      u_long xyz2, u_int uv3, u_long xyz3, u_long xyz3End)
{
    p[7] = rgbaq;
    p[8] = uvC;
    p[9] = xyzC;
    p[11] = uv1;
    p[12] = xyz1;
    p[14] = uv2;
    p[15] = xyz2;
    p[17] = uv3;
    p[18] = xyz3;
    p[19] = xyz3End;
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)p);
}

// CT3 0x01846360
// The end of a bank's shadow pass (comment above). A negative bank sends drawbuffer_0 only.
// callcheck: __fixdfdi, dpmul, fptodp (the sprite set-up's double maths: the compiler's runtime)
void asShadowEnd(int bank)
{
    BANK_INFO *bi;
    int level, lighten, i, j;
    if (bank >= 0) {
        bi = &BankInfo[bank];
        FlushCache(0);
        if (iCngFlag_2765 == 0) {
            gaSetupDrawCtrlData(&gaDC_EBclear_D_2764, EBCLEAR_D_ARGS);
            iCngFlag_2765 = 1;
        }
        gaBGT_ExecDrawCtrlData_VIF1(&gaDC_EBclear_D_2764);
        gaBGT_ExecDrawCommandData_VIF1(&dc_EBclear_E_2768);
        if (iCngFlag_2770 == 0) {
            gaSetupDrawCtrlData(&gaDC_EBclear_F_2769, EBCLEAR_F_ARGS);
            iCngFlag_2770 = 1;
        }
        gaBGT_ExecDrawCtrlData_VIF1(&gaDC_EBclear_F_2769);
        gaBGT_ExecDrawCommandData_VIF1(&dc_EBclear_G_2773);
        ga_draw_fbuf_address = asSD_BackBufferAdd;
        gaBGT_ExecDBufferData_VIF1(&drawbuffer_0);
        level = bi->shadowOrder;
        lighten = level < 0;
        if (lighten)
            level = -level;
        if (asShadowDrawing_CheckTransMode(SHADOW_TRANS_FIN) < 2) {
            u_long rgbaq;
            if (iCngFlag_2781 == 0) {
                gaSetupDrawCtrlData(&gaDC_EBclearW_2780, EBCLEAR_W_ARGS);
                gaSetupDrawCtrlData(&gaDC_EBclearB_2779, EBCLEAR_B_OVERLAY_ARGS);
                iCngFlag_2781 = 1;
            }
            gaBGT_ExecDrawCtrlData_VIF1(lighten ? &gaDC_EBclearW_2780 : &gaDC_EBclearB_2779);
            if (iInit_2788 == 0) {
                iInit_2788 = 1;
                for (i = 0; i < SHADOW_TILES_Y; i++) {
                    for (j = 0; j < SHADOW_TILES_X; j++) {
                        u_long *p = asFS_2787[j + i * SHADOW_TILES_X];
                        int x1 = j * SHADOW_TILE + SHADOW_TILE, y1 = i * SHADOW_TILE + SHADOW_TILE;
                        p[0] = SHADOW_DMATAG_CNT(5);
                        p[1] = 0;
                        p[2] = 0;
                        p[3] = SHADOW_VIF_DIRECT(4);
                        p[4] = SHADOW_GIFTAG_SPRITE;
                        p[5] = SHADOW_GIFREGS_SPRITE;
                        p[6] = GS_PRIM_SPRITE;
                        p[8] = (u_long)(u_short)(i << 10) << 16 | (u_short)(j << 10);
                        // the ROM goes through doubles: (64-bit int)(x * 16.0) + 0x8000
                        p[9] = (u_long)(u_short)((int64_t)((double)(j * SHADOW_TILE) * 16.0) + 0x8000)
                             | (u_long)(u_short)((int64_t)((double)(i * SHADOW_TILE) * 16.0) + 0x8000) << 16;
                        p[10] = (u_long)(u_short)(y1 * 0x10) << 16 | (u_short)(x1 * 0x10);
                        p[11] = (u_long)(u_short)((int64_t)((double)x1 * 16.0) + 0x8000)
                              | (u_long)(u_short)((int64_t)((double)y1 * 16.0) + 0x8000) << 16;
                    }
                }
            }
            rgbaq = asDR_RGBAQ((u_char)(bi->shadowColor[0] * level / 100), (u_char)(bi->shadowColor[1] * level / 100),
                               (u_char)(bi->shadowColor[2] * level / 100), 0x80, SHADOW_Q_ONE);
            for (i = 0; i < SHADOW_TILES_X * SHADOW_TILES_Y; i++) {
                asFS_2787[i][ASFS_RGBAQ] = rgbaq;
                gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)asFS_2787[i]);
            }
        } else {
            alignas(16) float v[4];
            float sx, sy, hi;
            int xSide = SHADOW_FAN_SIDE_IN, ySide = SHADOW_FAN_SIDE_IN, ring;
            int ix, iy, u, v16;
            u_char *block;
            if (iCntFlag_2809 == 0) {
                gaSetupDrawCtrlData(&gaDC_EBclearW_2808, EBCLEAR_W_FAN_ARGS);
                gaSetupDrawCtrlData(&gaDC_EBclearB_2807, EBCLEAR_B_FAN_ARGS);
                iCntFlag_2809 = 1;
            }
            gaBGT_ExecDrawCtrlData_VIF1(lighten ? &gaDC_EBclearW_2808 : &gaDC_EBclearB_2807);
            if (ShadowBlock[bank] == NULL) {    /* ROM: BankInfo +0x58 */
                ShadowBlock[bank] = _SunAlloc(&gHeapJunc, SHADOW_FAN_COUNT * SHADOW_FAN_SIZE, 0x10);
                bi->shadow70 = 1;
            }
            for (i = 0; i < SHADOW_FAN_COUNT; i++) {
                u_long *p = (u_long *)((u_char *)ShadowBlock[bank] + i * SHADOW_FAN_SIZE);
                p[0] = SHADOW_DMATAG_CNT(9);
                p[1] = 0;
                p[2] = 0;
                p[3] = SHADOW_VIF_DIRECT(8);
                p[4] = SHADOW_GIFTAG_FAN;
                p[5] = SHADOW_GIFREGS_FAN;
                p[6] = GS_PRIM_TRIFAN;
                p[10] = SHADOW_RGBAQ_BLACK;
                p[13] = SHADOW_RGBAQ_BLACK;
                p[16] = SHADOW_RGBAQ_BLACK;
            }
            // the anchor to the screen: VU0 matrix_global_view * anchor, then centre + scale * d * x / z
            for (j = 0; j < 4; j++)
                v[j] = matrix_global_view[0][j] * bi->shadowAnchor[0] + matrix_global_view[1][j] * bi->shadowAnchor[1]
                     + matrix_global_view[2][j] * bi->shadowAnchor[2] + matrix_global_view[3][j] * bi->shadowAnchor[3];
            sy = screen_0[SCREEN_CY] - screen_0[SCREEN_SCALE_Y] * ((screen_0[SCREEN_PROJ_D] * v[1]) / v[2]);
            sx = screen_0[SCREEN_CX] + screen_0[SCREEN_SCALE_X] * ((screen_0[SCREEN_PROJ_D] * v[0]) / v[2]);
            hi = screen_0[SCREEN_X] + screen_0[SCREEN_W] / screen_0[SCREEN_SCALE_X];
            if (sx < screen_0[SCREEN_X]) {
                sx = screen_0[SCREEN_X];
                xSide = SHADOW_FAN_SIDE_LOW;
            } else if (hi < sx) {
                sx = hi;
                xSide = SHADOW_FAN_SIDE_HIGH;
            }
            hi = screen_0[SCREEN_Y] + screen_0[SCREEN_H] / screen_0[SCREEN_SCALE_Y];
            if (sy < screen_0[SCREEN_Y]) {
                sy = screen_0[SCREEN_Y];
                ySide = SHADOW_FAN_SIDE_LOW;
            } else if (hi < sy) {
                sy = hi;
                ySide = SHADOW_FAN_SIDE_HIGH;
            }
            block = (u_char *)ShadowBlock[bank];
            ix = (int)sx;
            iy = (int)sy;
            u = ix * 16;
            v16 = iy * 16;
            for (ring = 0; ring < SHADOW_FAN_RINGS; ring++) {
                // ring r would pull the outer corners r% of the way in and halve the colour per ring
                int u0 = u * ring * 4 / 100, u1 = 0x2000 - (0x2000 - ix * 16) * ring / 100;
                int v0 = v16 * ring * 4 / 100, v1 = 0x1c00 - (0x1c00 - iy * 16) * ring / 100;
                u_long col = asDR_RGBAQ((u_char)((bi->shadowColor[0] * level / 100) >> ring),
                                        (u_char)((bi->shadowColor[1] * level / 100) >> ring),
                                        (u_char)((bi->shadowColor[2] * level / 100) >> ring), 0x80, SHADOW_Q_ONE);
                u_int uvC = asDR_UV(u, v16);
                u_long xyzC = asDR_XYZ2(ix, iy, 0);
                if (xSide != SHADOW_FAN_SIDE_LOW && ySide != SHADOW_FAN_SIDE_LOW) {        /* top left */
                    ShadowFan((u_long *)block, col, uvC, xyzC, asDR_UV(u0, v16), asDR_XYZ2(0, iy, 0),
                              asDR_UV(u0, v0), asDR_XYZ2(0, 0, 0), asDR_UV(u, v0), asDR_XYZ2(ix, 0, 0),
                              asDR_XYZ3(ix, 0, 0));
                    block += SHADOW_FAN_SIZE;
                }
                if (xSide != SHADOW_FAN_SIDE_HIGH && ySide != SHADOW_FAN_SIDE_LOW) {       /* top right */
                    ShadowFan((u_long *)block, col, uvC, xyzC, asDR_UV(u, v0), asDR_XYZ2(ix, 0, 0),
                              asDR_UV(u1, v0), asDR_XYZ2(SHADOW_SCREEN_W, 0, 0), asDR_UV(u1, v16),
                              asDR_XYZ2(SHADOW_SCREEN_W, iy, 0), asDR_XYZ3(SHADOW_SCREEN_W, iy, 0));
                    block += SHADOW_FAN_SIZE;
                }
                if (xSide != SHADOW_FAN_SIDE_LOW && ySide != SHADOW_FAN_SIDE_HIGH) {       /* bottom left */
                    ShadowFan((u_long *)block, col, uvC, xyzC, asDR_UV(u, v1), asDR_XYZ2(ix, SHADOW_SCREEN_H, 0),
                              asDR_UV(u0, v1), asDR_XYZ2(0, SHADOW_SCREEN_H, 0), asDR_UV(u0, v16),
                              asDR_XYZ2(0, iy, 0), asDR_XYZ3(0, iy, 0));
                    block += SHADOW_FAN_SIZE;
                }
                if (xSide != SHADOW_FAN_SIDE_HIGH && ySide != SHADOW_FAN_SIDE_HIGH) {      /* bottom right */
                    ShadowFan((u_long *)block, col, uvC, xyzC, asDR_UV(u1, v16), asDR_XYZ2(SHADOW_SCREEN_W, iy, 0),
                              asDR_UV(u1, v1), asDR_XYZ2(SHADOW_SCREEN_W, SHADOW_SCREEN_H, 0), asDR_UV(u, v1),
                              asDR_XYZ2(ix, SHADOW_SCREEN_H, 0), asDR_XYZ3(ix, SHADOW_SCREEN_H, 0));
                    block += SHADOW_FAN_SIZE;
                }
            }
        }
    }
    gaBGT_ExecDBufferData_VIF1(&drawbuffer_0);
}


// ---- the double buffer (asCopyDoubleBuffer and its L2V, slice 5, 2026-11-10) ----
//
// Once a frame (main, after the draw) every part's view matrices (REC_DRAW_MATRIX, REC_VIEW_MATRIX)
// become the matrices its ga draw object reads (REC_GA_MATRIX, REC_GA_MATRIX2). A scene bank whose
// drawing has just been switched on, or a character that has just taken CHARACTER_FLAG_SHADOW, gets
// the gNoDraw matrices instead: last frame's matrices are not drawn.

#define REC_GA_VIEW_HOLD 0x14c          /* float: REC_GA_MATRIX2 row 0 w = the copied REC_VIEW_HOLD;
                                           != 0 leaves the part's ga matrices alone */
#define L2V_COPY 0                      /* asCopyDoubleBufferL2V: the view matrices to the ga ones */
#define L2V_NO_DRAW 1                   /* ... the ga ones cleared, gNoDraw as the first one's last row */

// The symtab's local gNoDraw$117: the last row of a "nothing to draw" matrix (translation 1e8).
alignas(16) static const float gNoDraw[4] = { 100000000.0f, 100000000.0f, 100000000.0f, 0.0f }; /* sdata 0x01937610 */

// CT3 0x018513b0
// Every part but a static one or one whose ga view hold is set: L2V_COPY copies REC_DRAW_MATRIX and
// REC_VIEW_MATRIX to the ga matrices (lqc2 / sqc2); L2V_NO_DRAW zeroes both ga matrices but the first
// one's last row, gNoDraw. Other modes do nothing.
void asCopyDoubleBufferL2V(u_char *partsHeader, int mode)
{
    u_char *rec;
    if (mode != L2V_COPY && mode != L2V_NO_DRAW)
        return;
    for (rec = PARTS_REC(partsHeader, 0); !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        if ((signed char)rec[REC_KIND] == PARTS_KIND_STATIC || *(float *)(rec + REC_GA_VIEW_HOLD) != 0.0f)
            continue;
        if (mode == L2V_NO_DRAW) {
            memset(rec + REC_GA_MATRIX, 0, 0x30);
            memcpy(rec + REC_GA_MATRIX + 0x30, gNoDraw, sizeof(gNoDraw));
            memset(rec + REC_GA_MATRIX2, 0, 0x40);
        } else {
            memcpy(rec + REC_GA_MATRIX, rec + REC_DRAW_MATRIX, 0x80);
        }
    }
}

// CT3 0x01851150
// The scene banks (BANK_TYPE_SHADOW and BANK_TYPE_0) that are drawn or BANK_FLAG_DRAW_FIRST, then
// every character's bank; drawOnPrev / flagsPrev keep this frame's state for the next.
void asCopyDoubleBuffer(void)
{
    int bank;
    int c;
    for (bank = 0; bank < BANK_MAX; bank++) {
        BANK_INFO *bi = &BankInfo[bank];
        if (bi->state != BANK_STATE_LOADED)
            continue;
        if ((bi->drawOn == DRAW_BANK_ON || (bi->flags & BANK_FLAG_DRAW_FIRST))
            && (bi->type == BANK_TYPE_SHADOW || bi->type == BANK_TYPE_0)
            && All_table_TBL[bank] != NULL) { /* PORT: the ROM reads the table unchecked */
            u_char *hdr = BANK_PTR(u_char *, bank, All_table_TBL[bank]->partsHeader);
            if (bi->drawOnPrev == 0 && bi->drawOn == DRAW_BANK_ON)
                asCopyDoubleBufferL2V(hdr, L2V_NO_DRAW);
            else
                asCopyDoubleBufferL2V(hdr, L2V_COPY);
        }
        bi->drawOnPrev = bi->drawOn;
        bank += bi->groupCount - 1;     /* state is BANK_STATE_LOADED (> 0): skip the group */
    }
    for (c = 0; c < CHARACTER_MAX; c++) {
        CHARACTER *chr = &Character[c];
        short b = chr->bank;
        if (b >= 0 && All_table_TBL[b] != NULL) { /* PORT: the same check */
            u_char *hdr = BANK_PTR(u_char *, b, All_table_TBL[b]->partsHeader);
            if (!(chr->flagsPrev & CHARACTER_FLAG_SHADOW) && (chr->flags & CHARACTER_FLAG_SHADOW))
                asCopyDoubleBufferL2V(hdr, L2V_NO_DRAW);
            else
                asCopyDoubleBufferL2V(hdr, L2V_COPY);
        }
        chr->flagsPrev = chr->flags;
    }
}

// ---- the keys (slice 9, 2026-11-10) ----

// A key's frame: its first int.
static int KeyFrame(const u_char *key)
{
    return *(const int *)key;
}

// The key at time t into key 0 (out): prev's copied when t is on it or there is no next key, else prev
// and next blended by w = {1 - f, f}, f how far t is between their frames. The ROM has this body twice,
// inline, in IAK_0 and InterpolateAnimationKey. PORT: the bank resolves the vertex kinds' EE words.
static void IAK_WriteKey(int bank, const BEHAVIOR_TRACK *tr, u_char *out, const u_char *prev,
                         const u_char *next, int t)
{
    u_int stride = tr->length & TRACK_KEY_STRIDE;
    int raw = (tr->length & TRACK_KEY_RAW) != 0;
    ANIM_SUBKEY *o = (ANIM_SUBKEY *)out;
    const ANIM_SUBKEY *p = (const ANIM_SUBKEY *)prev;
    const ANIM_SUBKEY *q = (const ANIM_SUBKEY *)next;
    int n;
    if (t == KeyFrame(prev) * TRACK_TIME_SCALE || next == NULL) {
        switch (tr->kind) {
        case TRACK_KIND_UV: {
            // The ROM gives asZipUV2UV a 4-byte stack copy of the packed UV (it reads 4 shorts: the
            // last two are stack garbage, unused) and stores the first 8 bytes of its float answer --
            // u and v as floats -- over key 0's packed UV (+8..+0xf). Kept: hazards.md (2026-11-10).
            short zip[4] = { *(const short *)(prev + 8), *(const short *)(prev + 10), 0, 0 };
            float uv[4];
            asZipUV2UV(uv, zip);
            memcpy(out + 8, uv, 8);
            break;
        }
        case TRACK_KIND_NORMAL:
            for (n = (int)stride; n > 0; n -= o->size, o++, p++)
                asMemCopy16c(BANK_PTR(void *, bank, o->data), BANK_PTR(const void *, bank, p->data), o->count * 3);
            break;
        case TRACK_KIND_VERTEX:
            for (n = (int)stride; n > 0; n -= o->size, o++, p++) {
                if (raw)
                    asMemCopy128c(BANK_PTR(void *, bank, o->data), BANK_PTR(const void *, bank, p->data), o->count);
                else
                    asMemCopy16c(BANK_PTR(void *, bank, o->data), BANK_PTR(const void *, bank, p->data), o->count * 3);
            }
            break;
        default:
            memmove(out, prev, (size_t)((int)stride >> 2) * 4); // callcheck: gaMemCopy32 (words)
            break;
        }
        return;
    }
    {
        float w[2];
        w[1] = ((float)t / 100.0f - (float)KeyFrame(prev)) / (float)(KeyFrame(next) - KeyFrame(prev));
        w[0] = 1.0f - w[1];
        switch (tr->kind) {
        case TRACK_KIND_BASE_ROT:
        case TRACK_KIND_ROT:
            asQuatSlerpZipVECTOR((short *)(out + 8), (const short *)(prev + 8), (const short *)(next + 8), w);
            break;
        case TRACK_KIND_BASE_POS:
        case TRACK_KIND_POS:
            if (!raw)
                asInterpolateZipTrans_XYZ((short *)(out + 8), (const short *)(prev + 8), (const short *)(next + 8), w,
                                          TRACK_KEY_SHIFT(tr->length));
            else
                asInterpolateVector((float *)(out + 0x10), (const float *)(prev + 0x10), (const float *)(next + 0x10), w);
            break;
        case TRACK_KIND_FLOAT0_7_9:
            asInterpolateZipFloat0_7_9X((short *)(out + 8), (const short *)(prev + 8), (const short *)(next + 8), w);
            break;
        case TRACK_KIND_UV:
            asInterpolateZipUV((short *)(out + 8), (const short *)(prev + 8), (const short *)(next + 8), w);
            *(short *)(out + 0xc) = *(const short *)(prev + 0xc);
            break;
        case TRACK_KIND_NORMAL:
            for (n = (int)stride; n > 0; n -= o->size, o++, p++, q++)
                asContInterpolateZipNormal_XYZ(BANK_PTR(short *, bank, o->data), BANK_PTR(const short *, bank, p->data),
                                               BANK_PTR(const short *, bank, q->data), w, o->count);
            break;
        case TRACK_KIND_VERTEX:
            for (n = (int)stride; n > 0; n -= o->size, o++, p++, q++) {
                if (!raw)
                    asContInterpolateZipTrans_XYZ(BANK_PTR(short *, bank, o->data), BANK_PTR(const short *, bank, p->data),
                                                  BANK_PTR(const short *, bank, q->data), w, o->count,
                                                  TRACK_KEY_SHIFT(tr->length));
                else
                    asContInterpolateVector(BANK_PTR(float *, bank, o->data), BANK_PTR(const float *, bank, p->data),
                                            BANK_PTR(const float *, bank, q->data), w, o->count);
            }
            break;
        case TRACK_KIND_VECTOR_XYZ:
        case TRACK_KIND_VECTOR:
            asInterpolateZipVECTOR_XYZ((short *)(out + 8), (const short *)(prev + 8), (const short *)(next + 8), w);
            break;
        case TRACK_KIND_COPY:
            memmove(out, prev, (size_t)((int)stride >> 2) * 4); // callcheck: gaMemCopy32 (words)
            break;
        case TRACK_KIND_NUMBER:
            *(short *)(out + 8) = (short)(int)asInterpolateNumber((float)*(const short *)(prev + 8),
                                                                  (float)*(const short *)(next + 8), w);
            break;
        case TRACK_KIND_FLOAT1_1_14:
            asInterpolateZipFloat1_1_14X((short *)(out + 8), (const short *)(prev + 8), (const short *)(next + 8), w);
            break;
        case TRACK_KIND_SUB_NUMBER:
            // The ROM counts the whole length word down here, flag bits and all.
            for (n = (int)tr->length; n > 0; n -= o->size, o++, p++, q++) {
                o->number = (short)(int)asInterpolateNumber((float)p->number, (float)q->number, w);
                o->data = p->data;
            }
            break;
        default:
            break;
        }
    }
}

// CT3 0x0184dbc0
// Moves the track's time on by dir ticks (backwards with TRACK_FLAG_REVERSE; at anim_play_slow's speed
// snapped to TRACK_SLOW_STEP; playRate 0 jumps to the end), applies the play mode at the ends (1 when
// a once track reached playable), finds the keys either side and writes key 0 (IAK_WriteKey). dir 0
// is IAK_0's.
// callcheck: IAK_0 (all-caps: callcheck reads it as a macro)
// callcheck: asContInterpolateVector, asContInterpolateZipNormal_XYZ, asContInterpolateZipTrans_XYZ, asInterpolateNumber, asInterpolateVector, asInterpolateZipFloat0_7_9X, asInterpolateZipFloat1_1_14X, asInterpolateZipTrans_XYZ, asInterpolateZipUV, asInterpolateZipVECTOR_XYZ, asMemCopy128c, asMemCopy16c, asQuatSlerpZipVECTOR, asZipUV2UV, gaMemCopy32 (in IAK_WriteKey; gaMemCopy32 is memmove)
int InterpolateAnimationKey(BEHAVIOR_TRACK *track, int dir)
{
    int bank, end, lastFrame, last, before, t, ret = 0;
    u_int stride, k;
    u_char flags, *keys;
    const u_char *prev, *next;
    if (dir == 0)
        return IAK_0(track);
    bank = BankOfHostPtr(track);
    keys = BANK_PTR(u_char *, bank, track->keys);
    flags = track->flags;
    stride = track->length & TRACK_KEY_STRIDE;
    before = track->time;
    end = track->playable * TRACK_TIME_SCALE;
    lastFrame = KeyFrame(keys + track->runs * stride);
    last = lastFrame * TRACK_TIME_SCALE;
    t = dir * ((flags & TRACK_FLAG_REVERSE) ? -1 : 1);
    if (track->playRate == 0) {
        track->time = end;
        t = end;
    } else if ((u_int)track->playRate == (u_int)(anim_play_slow * 100) / 100) {
        if (!(flags & TRACK_FLAG_REVERSE))
            t = ((t + before - 1) / TRACK_SLOW_STEP + 1) * TRACK_SLOW_STEP;
        else
            t = ((t + before) / TRACK_SLOW_STEP) * TRACK_SLOW_STEP;
    } else {
        t += before;
    }
    switch (flags & TRACK_FLAG_PLAY_MASK) {
    case TRACK_PLAY_LOOP_REVERSE:
        if (t < end) {
            while (t < end)
                t += track->rangeStart * TRACK_TIME_SCALE - track->playable * TRACK_TIME_SCALE;
            track->step = (short)track->runs;
        }
        break;
    case TRACK_PLAY_LOOP:
        if (end < t) {
            while (end < t)
                t -= end - track->rangeStart * TRACK_TIME_SCALE;
            track->step = 1;
        }
        break;
    case TRACK_PLAY_ONCE_REVERSE:
        if (end < before && t <= end) {
            ret = 1;
            t = end;
        } else if (before >= 0 && t < 1) {
            t += last;
            track->step = (short)track->runs;
        }
        break;
    default: /* TRACK_PLAY_ONCE */
        if (before < end && end <= t) {
            ret = 1;
            t = end;
        } else if (before <= last && last <= t && end <= t) {
            track->step = 1;
            t -= lastFrame * TRACK_TIME_SCALE;
        }
        break;
    }
    if (!(flags & TRACK_FLAG_REVERSE)) {
        if (t < last) {
            k = 1;
            while (KeyFrame(keys + (k + 1) * stride) * TRACK_TIME_SCALE <= t)
                k = (k + 1) & 0xffff;
            next = keys + (k + 1) * stride;
            prev = keys + k * stride;
        } else {
            k = track->runs;
            next = NULL;
            prev = keys + k * stride;
        }
    } else if (t < 1) {
        k = 1;
        next = keys + 2 * stride;
        prev = keys + stride;
    } else {
        k = (u_short)track->step;
        while (t <= KeyFrame(keys + k * stride) * TRACK_TIME_SCALE)
            k = (k - 1) & 0xffff;
        prev = keys + k * stride;
        next = keys + (k + 1) * stride;
    }
    track->step = (short)k;
    track->time = t;
    IAK_WriteKey(bank, track, keys, prev, next, t);
    return ret;
}

// CT3 0x0184e520
// The key at the track's time as it stands (playRate 0: the end), clamped to playable -- 1 when it is
// there (at or past it forwards, at or before it backwards).
// callcheck: asContInterpolateVector, asContInterpolateZipNormal_XYZ, asContInterpolateZipTrans_XYZ, asInterpolateNumber, asInterpolateVector, asInterpolateZipFloat0_7_9X, asInterpolateZipFloat1_1_14X, asInterpolateZipTrans_XYZ, asInterpolateZipUV, asInterpolateZipVECTOR_XYZ, asMemCopy128c, asMemCopy16c, asQuatSlerpZipVECTOR, asZipUV2UV, gaMemCopy32 (in IAK_WriteKey; gaMemCopy32 is memmove)
int IAK_0(BEHAVIOR_TRACK *track)
{
    int bank = BankOfHostPtr(track);
    u_char *keys = BANK_PTR(u_char *, bank, track->keys);
    u_int stride = track->length & TRACK_KEY_STRIDE;
    int last = KeyFrame(keys + track->runs * stride) * TRACK_TIME_SCALE;
    int end = track->playable * TRACK_TIME_SCALE;
    int t, ret;
    u_int k;
    const u_char *prev, *next;
    if (track->playRate == 0) {
        track->time = end;
        t = end;
    } else {
        t = track->time;
    }
    if (!(track->flags & TRACK_FLAG_REVERSE)) {
        ret = end <= t;
        if (ret)
            t = end;
        if (t < last) {
            k = 1;
            while (KeyFrame(keys + (k + 1) * stride) * TRACK_TIME_SCALE <= t)
                k = (k + 1) & 0xffff;
            next = keys + (k + 1) * stride;
            prev = keys + k * stride;
        } else {
            k = track->runs;
            next = NULL;
            prev = keys + k * stride;
        }
    } else {
        ret = t <= end;
        if (ret)
            t = end;
        k = (u_short)track->step;
        while (t <= KeyFrame(keys + k * stride) * TRACK_TIME_SCALE)
            k = (k - 1) & 0xffff;
        prev = keys + k * stride;
        next = keys + (k + 1) * stride;
    }
    track->step = (short)k;
    track->time = t;
    IAK_WriteKey(bank, track, keys, prev, next, t);
    return ret;
}

// ---- play range, behaviour lookup, shadow setters (slice 9, 2026-11-10) ----

// CT3 0x01849700
void asChangePlayRange(BEHAVIOR_TRACK *track, int from, int to)
{
    if (from >= 0) {
        track->rangeStart = from;
        track->time = from * TRACK_TIME_SCALE;
    }
    if (to < 0)
        track->playable = TRACK_RANGE_OPEN;
    else
        track->playable = to;
}

// CT3 0x018496c0
void asResetPassCheckarameter(BEHAVIOR_TRACK *track, int frame)
{
    if (frame < 0)
        track->time1c = (int)((u_int)(u_short)track->time1c * 0x10001);
    else
        track->time1c = frame * 0x10001;
}

// CT3 0x0184ef10
// PORT: the list's words are EE addresses of the image's bank; the answer is a host pointer.
BEHAVIOR *asFindBehavior(u_char *image, u_long hash)
{
    int bank = BankOfHostPtr(image);
    const u_int *list = (const u_int *)(image + CT3X_BHVR_LIST);
    for (; *list != 0; list++) {
        BEHAVIOR *bhv = BANK_PTR(BEHAVIOR *, bank, *list);
        if (bhv->hash == hash)
            return bhv;
    }
    return NULL;
}

// CT3 0x0184ef50
BEHAVIOR *asFindBehaviorByCharacter(int chara, u_long hash)
{
    BEHAVIOR *bhv = NULL;
    int i;
    for (i = 0; i < 10; i++) {
        short b = Character[chara].behaviorBank[i];
        if (b < 0 || BankInfo[b].state != BANK_STATE_LOADED)
            continue;
        bhv = asFindBehavior(BankImage[b], hash); /* PORT: BankInfo[b].image's host side */
        if (bhv != NULL)
            break;
    }
    return bhv;
}

// CT3 0x01847c50
void asChangeBehaviorAnimationCurrent(BEHAVIOR *bhv, int frame)
{
    int bank, g, t;
    if (bhv == NULL)
        return;
    bank = BankOfHostPtr(bhv);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++) {
            if (frame < tr->playable)
                tr->time = frame * TRACK_TIME_SCALE;
            else
                tr->time = tr->playable * TRACK_TIME_SCALE - 1;
        }
    }
}

// CT3 0x01847490
void asShadowDrawing_AttachLightPosition(int bank, ALL_TABLE *tbl, int part)
{
    if (bank < 0)
        return;
    BankInfo[bank].shadowLightPos = tbl->partsHeader + PARTS_RECORDS + (u_int)(part & 0xffff) * 0x380 + SHADOW_LIGHT_POS;
    asShadowDrawing_Enable(bank);
}

// CT3 0x01847550
void asShadowDrawing_SetShadowAngleLockParameter(int bank, float lock64, float lock68)
{
    if (bank >= 0) {
        BankInfo[bank].shadow64 = lock64;
        BankInfo[bank].shadow68 = lock68;
    }
}

// CT3 0x01847580
void asShadowDrawing_SetShadowAngleLock(int bank, u_char on)
{
    if (bank >= 0)
        BankInfo[bank].shadow6d = on;
}

// CT3 0x018475b0
void asShadowDrawing_SetShadowValue(int bank, s_char value)
{
    if (bank >= 0)
        BankInfo[bank].shadowOrder = value;
}

// CT3 0x018475e0
// The shadow's alpha (shadowColor[3]; asShadowDrawing_Disable puts 0xff there).
void asShadowDrawing_SetShadowLevel(int bank, u_char level)
{
    if (bank >= 0)
        BankInfo[bank].shadowColor[3] = level;
}

// CT3 0x01847630
void asShadowDrawing_SetTransMode(char mode)
{
    iShadowTransMode = mode == SHADOW_TRANS_MODE_PLAIN;
}

// ---- the shadow pass order (CT3 0x01847660, slice 10, 2026-11-10) ----
//
// SHADOW_TRANS_SORT lists the banks whose shadow is drawn: the drawn scene banks (type
// BANK_TYPE_SHADOW, shadowDraw on; a group's later banks skipped), then the shadowed characters'
// banks (CHARACTER_FLAG_SHADOW, not translucent). shadowOrder > 0 puts a bank at the front of BID
// (from 0 up), <= 0 at the back (from 0xff down); 0xff marks an empty slot. Each SHADOW_TRANS_NEXT
// then hands out the next bank: the front ones in order, then the back ones from the end (with
// iShadowTransOrderBW the other way round), SHADOW_TRANS_END when both are spent.

#define BID_SLOTS 256
#define BID_EMPTY 0xff
static u_short BID[BID_SLOTS];          /* bss  0x0193c610 (BID$2565) */
static u_short BidxB;                   /* sbss 0x0193c604 (BidxB$2566): the next front slot */
static u_short BidxW;                   /* sbss 0x0193c600 (BidxW$2567): the next back slot */

// Whether a bank is one SHADOW_TRANS_SORT lists from the scene banks.
static int ShadowSortSceneBank(const BANK_INFO *bi)
{
    return bi->drawOn != 0 && bi->type == BANK_TYPE_SHADOW && bi->shadowDraw != 0;
}

// Whether a character's bank is one it lists: shadowed, opaque, and its bank's shadow on.
static int ShadowSortCharacter(const CHARACTER *ch)
{
    return ch->bank >= 0 && (ch->flags & CHARACTER_FLAG_SHADOW) != 0 && ch->translucent == 0
        && BankInfo[ch->bank].shadowDraw != 0;
}

// CT3 0x01847660
int asShadowDrawing_CheckTransMode(int mode)
{
    int i, b, w;
    if (mode == SHADOW_TRANS_SORT) {
        for (i = 0; i < BID_SLOTS; i++)
            BID[i] = BID_EMPTY;
        BidxW = BID_EMPTY;
        BidxB = 0;
        if (iShadowTransMode == 0) {
            // One counter does two jobs here, as the ROM has it: the character loop's index also
            // counts the changes of shadowOrder, so a change skips the next character, and the loop
            // always ends with it >= 2 (iShadowTransModeFin 1). The bank loop's count is unused.
            s_char order = 0x7f;
            int n = 0;
            b = 0;
            w = BID_SLOTS - 1;
            for (i = 0; i < BANK_MAX; i++) {
                BANK_INFO *bi = &BankInfo[i];
                if (ShadowSortSceneBank(bi)) {
                    if (order != bi->shadowOrder) {
                        order = bi->shadowOrder;
                        n++;
                    }
                    if (bi->shadowOrder > 0) {
                        BID[b++] = (u_short)i;
                        BidxB++;
                    } else {
                        BID[w--] = (u_short)i;
                        BidxW--;
                    }
                }
                if (bi->state > 0)
                    i += bi->groupCount - 1;
            }
            (void)n;
            b = BidxB;
            w = BidxW;
            for (n = 0; n < CHARACTER_MAX; n++) {
                CHARACTER *ch = &Character[n];
                if (ShadowSortCharacter(ch)) {
                    s_char o = BankInfo[ch->bank].shadowOrder;
                    if (order != o) {
                        order = o;
                        n++;
                    }
                    if (o > 0) {
                        BID[b++] = (u_short)ch->bank;
                        BidxB++;
                    } else {
                        BID[w--] = (u_short)ch->bank;
                        BidxW--;
                    }
                }
            }
            iShadowTransModeFin = n >= 2;
        } else {
            iShadowTransModeFin = 2;
            b = 0;
            w = BID_SLOTS - 1;
            for (i = 0; i < BANK_MAX; i++) {
                BANK_INFO *bi = &BankInfo[i];
                if (ShadowSortSceneBank(bi)) {
                    if (bi->shadowOrder > 0) {
                        BID[b++] = (u_short)i;
                        BidxB++;
                    } else {
                        BID[w--] = (u_short)i;
                        BidxW--;
                    }
                }
                if (bi->state > 0)
                    i += bi->groupCount - 1;
            }
            b = BidxB;
            w = BidxW;
            for (i = 0; i < CHARACTER_MAX; i++) {
                CHARACTER *ch = &Character[i];
                if (ShadowSortCharacter(ch)) {
                    if (BankInfo[ch->bank].shadowOrder > 0) {
                        BID[b++] = (u_short)ch->bank;
                        BidxB++;
                    } else {
                        BID[w--] = (u_short)ch->bank;
                        BidxW--;
                    }
                }
            }
        }
        BidxB = 0;
        BidxW = BID_EMPTY;
    }
    if (mode < SHADOW_TRANS_NEXT)
        return (u_char)iShadowTransModeFin;
    if (iShadowTransOrderBW == 0) {
        if (BID[BidxB] != BID_EMPTY)
            return BID[BidxB++];
        if (BID[BidxW] != BID_EMPTY)
            return BID[BidxW--];
    } else {
        if (BID[BidxW] != BID_EMPTY)
            return BID[BidxW--];
        if (BID[BidxB] != BID_EMPTY)
            return BID[BidxB++];
    }
    return SHADOW_TRANS_END;
}

// CT3 0x01847cf0
// Every track of the behaviour plays at rate percent (BEHAVIOR_TRACK.playRate).
void asChangeBehaviorAnimationPlayRate(BEHAVIOR *bhv, u_char rate)
{
    int bank, g, t;
    if (bhv == NULL)
        return;
    bank = BankOfHostPtr(bhv);
    for (g = 0; g < bhv->groupCount; g++) {
        BEHAVIOR_TRACK *tr = BehaviorTracks(bank, bhv, g);
        for (t = bhv->groups[g].trackCount; t > 0; t--, tr++)
            tr->playRate = rate;
    }
}

// The record fields asCopyDoubleBufferL2W keeps (LinkMatrixTree.c names them the same).
#define REC_MATRIX_PREV 0x1c0           /* float[4][4]: last frame's REC_MATRIX */
#define REC_PREV_SCALE_X 0x4c           /* float: last frame's REC_SCALE, one per row w of the offset */
#define REC_PREV_SCALE_Y 0x5c
#define REC_PREV_SCALE_Z 0x6c

// CT3 0x018510d0
// Every part but a static one keeps this frame's world matrix (REC_MATRIX, four lqc2 / sqc2) and
// scale (REC_SCALE's xyz) as last frame's.
void asCopyDoubleBufferL2W(u_char *partsHeader)
{
    u_char *rec;
    for (rec = PARTS_REC(partsHeader, 0); !PARTS_END(rec); rec += sizeof(MODEL_PARTS)) {
        if ((signed char)rec[REC_KIND] == PARTS_KIND_STATIC)
            continue;
        memcpy(rec + REC_MATRIX_PREV, rec + REC_MATRIX, 0x40);
        memcpy(rec + REC_PREV_SCALE_X, rec + REC_SCALE + 0, 4);
        memcpy(rec + REC_PREV_SCALE_Y, rec + REC_SCALE + 4, 4);
        memcpy(rec + REC_PREV_SCALE_Z, rec + REC_SCALE + 8, 4);
    }
}

// ---- slice 14 (2026-11-10): EA_010, the replay pass ----

#define TRACK_KEY_SPARSE 0x2000000      /* BEHAVIOR_TRACK.length ('='): the _nw writers, which skip vertices
                                           marked ANIM_SKIP_SHORT (packed) or w == 0 (TRACK_KEY_RAW) */
#define ANIM_SKIP_SHORT (-0x8000)
#define GA_OBJ_UV_OFFSET 0x78           /* float[2]: TRACK_KIND_UV's key (+8, +0xc) */
#define GA_OBJ_COLOR_ALPHA(i) (0x10 + (i) * 4 + 3) /* GA_OBJ_COLORS' data: vertex i's alpha byte */
#define SUN_OBJ_COPY_TARGET 2           /* a sun object record's word 2: where TRACK_KIND_COPY writes */
#define PARTS_ANIM_VECTOR_XYZ 0x310     /* float[4]: TRACK_KIND_VECTOR_XYZ's value */
#define PARTS_ANIM_NUMBER 0x5a          /* short: TRACK_KIND_NUMBER's value */
#define PARTS_ANIM_FLOAT1_1_14 0x58     /* short: TRACK_KIND_FLOAT1_1_14's packed value */
#define SUB_NUMBER_MAX 0x7f

// The ga draw object of entry idx of the bank's objOpaque list (a sun object record's word 0).
static u_char *ObjOpaqueObject(int bank, const ALL_TABLE *tbl, int idx)
{
    const u_int *list = BANK_PTR(const u_int *, bank, tbl->objOpaque);
    const u_int *entry = BANK_PTR(const u_int *, bank, list[idx]);
    return BANK_PTR(u_char *, bank, entry[0]);
}

// A ga draw object's relocated word at `off` as a host pointer.
static u_char *ObjWord(int bank, const u_char *obj, int off)
{
    u_int ee;
    memcpy(&ee, obj + off, sizeof(ee));
    return BANK_PTR(u_char *, bank, ee);
}

// objGroups[group]: (first, count) into objGroupIndex.
static void ObjGroup(int bank, const ALL_TABLE *tbl, int group, int *first, int *count)
{
    const short *g = BANK_PTR(const short *, bank, tbl->objGroups) + group * 2;
    *first = g[0];
    *count = g[1];
}

static int ObjGroupIndex(int bank, const ALL_TABLE *tbl, int i)
{
    return BANK_PTR(const short *, bank, tbl->objGroupIndex)[i];
}

// The kinds EA_010 writes (the pose kinds are EA_100's: an ending pose track keeps TRACK_ACT_END here).
static int IsReplayKind(int kind)
{
    switch (kind) {
    case TRACK_KIND_FLOAT0_7_9: case TRACK_KIND_UV: case TRACK_KIND_NORMAL: case TRACK_KIND_VERTEX:
    case TRACK_KIND_SUB_NUMBER: case TRACK_KIND_NUMBER: case TRACK_KIND_FLOAT1_1_14: case TRACK_KIND_VECTOR_XYZ:
    case TRACK_KIND_COPY:
        return 1;
    }
    return 0;
}

// A replay-kind track's key (where it stands) into what it animates: set, or blended by w (the
// kinds below; the pose kinds are left alone). A COPY track's key word goes to its group's objects
// either way. An ending (TRACK_ACT_END) track of these kinds stops (playing 0). held: the caller's
// scratch vector, kept across the tracks as the ROM's one stack slot (EA_010's comment).
static void ReplayTrack(int bank, const ALL_TABLE *tbl, u_char *part, BEHAVIOR_TRACK *tr, int set,
                        float *w, float *held)
{
    const u_char *keys = BANK_PTR(const u_char *, bank, tr->keys);
    if (set) {
        /* set */
        switch (tr->kind) {
        case TRACK_KIND_FLOAT0_7_9:
            asZipFloat0_7_9X2Float((float *)(part + PARTS_ANIM_VECTOR), (const u_short *)(keys + 8));
            break;
        case TRACK_KIND_UV: {
            int first, count, i;
            short group;
            memcpy(&held[0], keys + 8, 8);
            memcpy(&group, keys + 0x1c, sizeof(group));
            ObjGroup(bank, tbl, group, &first, &count);
            for (i = 0; i < count; i++)
                memcpy(ObjOpaqueObject(bank, tbl, ObjGroupIndex(bank, tbl, first + i)) + GA_OBJ_UV_OFFSET,
                       held, 8);
            break;
        }
        case TRACK_KIND_NORMAL: {
            const ANIM_SUBKEY *sub = (const ANIM_SUBKEY *)keys;
            int n = (int)(tr->length & TRACK_KEY_STRIDE);
            if (n != 0) {
                do {
                    u_char *obj = ObjOpaqueObject(bank, tbl, sub->obj);
                    asContZipNormal2Normal_XYZ0((float *)(ObjWord(bank, obj, GA_OBJ_NORMALS) + 0x10),
                                                BANK_PTR(const short *, bank, sub->data), sub->count);
                    n -= sub->size;
                    sub++;
                } while (n > 0);
            }
            break;
        }
        case TRACK_KIND_VERTEX: {
            const ANIM_SUBKEY *sub = (const ANIM_SUBKEY *)keys;
            u_int len = tr->length;
            int n = (int)(len & TRACK_KEY_STRIDE);
            if (n != 0) {
                do {
                    u_char *obj = ObjOpaqueObject(bank, tbl, sub->obj);
                    float *dst = (float *)ObjWord(bank, obj, GA_OBJ_POSITIONS);
                    const void *src = BankEePtr(bank, sub->data);
                    if (len & TRACK_KEY_RAW) {
                        if (len & TRACK_KEY_SPARSE)
                            asMemCopy128c_nw(0.0f, dst, src, sub->count);
                        else
                            asMemCopy128c(dst, src, (u_int)sub->count);
                    } else if (len & TRACK_KEY_SPARSE) {
                        asContZipTrans2Trans_XYZ1_nw(dst, (const short *)src, sub->count,
                                                     TRACK_KEY_SHIFT(len), ANIM_SKIP_SHORT);
                    } else {
                        asContZipTrans2Trans_XYZ1(dst, (const short *)src, sub->count, TRACK_KEY_SHIFT(len));
                    }
                    n -= sub->size;
                    sub++;
                } while (n > 0);
            }
            break;
        }
        case TRACK_KIND_SUB_NUMBER: {
            const ANIM_SUBKEY *sub = (const ANIM_SUBKEY *)keys;
            int n;
            for (n = (int)tr->length; n > 0; n -= sub->size, sub++) {
                u_char *colors = ObjWord(bank, ObjOpaqueObject(bank, tbl, sub->obj), GA_OBJ_COLORS);
                const signed char *pct = BANK_PTR(const signed char *, bank, sub->data);
                int i;
                for (i = 0; i < sub->count; i++) {
                    short a = (short)(sub->number * pct[i] / 100);
                    if (a > SUB_NUMBER_MAX)
                        a = SUB_NUMBER_MAX;
                    colors[GA_OBJ_COLOR_ALPHA(i)] = (u_char)a;
                }
            }
            break;
        }
        case TRACK_KIND_NUMBER:
            memcpy(part + PARTS_ANIM_NUMBER, keys + 8, 2);
            break;
        case TRACK_KIND_FLOAT1_1_14:
            memcpy(part + PARTS_ANIM_FLOAT1_1_14, keys + 8, 2);
            break;
        case TRACK_KIND_VECTOR_XYZ:
            asZipVECTOR2VECTOR_XYZ1((float *)(part + PARTS_ANIM_VECTOR_XYZ), (const short *)(keys + 8));
            break;
        default:
            break;
        }
    } else {
        /* blend */
        switch (tr->kind) {
        case TRACK_KIND_FLOAT0_7_9:
            asZipFloat0_7_9X2Float(held, (const u_short *)(keys + 8));
            asInterpolateVector((float *)(part + PARTS_ANIM_VECTOR), (float *)(part + PARTS_ANIM_VECTOR), held, w);
            break;
        case TRACK_KIND_UV: {
            int first, count, i;
            short group;
            memcpy(&held[0], keys + 8, 8);
            memcpy(&group, keys + 0x1c, sizeof(group));
            ObjGroup(bank, tbl, group, &first, &count);
            for (i = 0; i < count; i++) {
                u_char *obj = ObjOpaqueObject(bank, tbl, ObjGroupIndex(bank, tbl, first + i));
                alignas(16) float cur[4] = { 0, 0, 0, 0 }, out[4];
                memcpy(cur, obj + GA_OBJ_UV_OFFSET, 8);
                asInterpolateVector(out, cur, held, w);
                memcpy(obj + GA_OBJ_UV_OFFSET, out, 8);
            }
            break;
        }
        case TRACK_KIND_NORMAL: {
            const ANIM_SUBKEY *sub = (const ANIM_SUBKEY *)keys;
            int n;
            for (n = (int)tr->length; n > 0; n -= sub->size, sub++) {
                u_char *obj = ObjOpaqueObject(bank, tbl, sub->obj);
                float *dst = (float *)(ObjWord(bank, obj, GA_OBJ_NORMALS) + 0x10);
                asContInterpolateGA_VECTORZipNormal_XYZ0(dst, dst, BANK_PTR(const short *, bank, sub->data), w,
                                                         sub->count);
            }
            break;
        }
        case TRACK_KIND_VERTEX: {
            const ANIM_SUBKEY *sub = (const ANIM_SUBKEY *)keys;
            u_int len = tr->length;
            int n = (int)(len & TRACK_KEY_STRIDE);
            if (n != 0) {
                do {
                    u_char *obj = ObjOpaqueObject(bank, tbl, sub->obj);
                    float *dst = (float *)ObjWord(bank, obj, GA_OBJ_POSITIONS);
                    const void *src = BankEePtr(bank, sub->data);
                    if (len & TRACK_KEY_RAW) {
                        if (len & TRACK_KEY_SPARSE)
                            asContInterpolateVector_nw(0.0f, dst, dst, (const float *)src, w, sub->count);
                        else
                            asContInterpolateVector(dst, dst, (const float *)src, w, sub->count);
                    } else if (len & TRACK_KEY_SPARSE) {
                        asContInterpolateGA_VECTORZipTrans_XYZ1_nw(dst, dst, (const short *)src, w, sub->count,
                                                                   TRACK_KEY_SHIFT(len), ANIM_SKIP_SHORT);
                    } else {
                        asContInterpolateGA_VECTORZipTrans_XYZ1(dst, dst, (const short *)src, w, sub->count,
                                                                TRACK_KEY_SHIFT(len));
                    }
                    n -= sub->size;
                    sub++;
                } while (n > 0);
            }
            break;
        }
        case TRACK_KIND_SUB_NUMBER: {
            const ANIM_SUBKEY *sub = (const ANIM_SUBKEY *)keys;
            int n;
            for (n = (int)tr->length; n > 0; n -= sub->size, sub++) {
                u_char *colors = ObjWord(bank, ObjOpaqueObject(bank, tbl, sub->obj), GA_OBJ_COLORS);
                const signed char *pct = BANK_PTR(const signed char *, bank, sub->data);
                int i;
                for (i = 0; i < sub->count; i++) {
                    short a = (short)(sub->number * pct[i] / 100);
                    if (a > SUB_NUMBER_MAX)
                        a = SUB_NUMBER_MAX;
                    colors[GA_OBJ_COLOR_ALPHA(i)] = (u_char)(int)asInterpolateNumber(
                        (float)(signed char)colors[GA_OBJ_COLOR_ALPHA(i)], (float)a, w);
                }
            }
            break;
        }
        case TRACK_KIND_NUMBER: {
            short cur, key;
            memcpy(&cur, part + PARTS_ANIM_NUMBER, 2);
            memcpy(&key, keys + 8, 2);
            cur = (short)(int)asInterpolateNumber((float)cur, (float)key, w);
            memcpy(part + PARTS_ANIM_NUMBER, &cur, 2);
            break;
        }
        case TRACK_KIND_FLOAT1_1_14:
            asInterpolateZipFloat1_1_14X((short *)(part + PARTS_ANIM_FLOAT1_1_14),
                                         (const short *)(part + PARTS_ANIM_FLOAT1_1_14),
                                         (const short *)(keys + 8), w);
            break;
        case TRACK_KIND_VECTOR_XYZ:
            asZipVECTOR2VECTOR_XYZ1(held, (const short *)(keys + 8));
            asInterpolateVector((float *)(part + PARTS_ANIM_VECTOR_XYZ),
                                (float *)(part + PARTS_ANIM_VECTOR_XYZ), held, w);
            break;
        default:
            break;
        }
    }
    if (tr->kind == TRACK_KIND_COPY) {
        // set either way: the word copyWords[key +8] to each object of group key +0xa
        u_short which, group;
        u_int word;
        int first, count, i;
        memcpy(&which, keys + 8, 2);
        memcpy(&group, keys + 0xa, 2);
        word = BANK_PTR(const u_int *, bank, tbl->copyWords)[which];
        {
            const u_short *pair = BANK_PTR(const u_short *, bank, tbl->objGroups) + group * 2;
            first = pair[0];
            count = pair[1];
        }
        for (i = 0; i < count; i++) {
            const u_int *list = BANK_PTR(const u_int *, bank, tbl->objOpaque);
            const u_int *entry = BANK_PTR(const u_int *, bank, list[ObjGroupIndex(bank, tbl, first + i)]);
            *BANK_PTR(u_int *, bank, entry[SUN_OBJ_COPY_TARGET]) = word;
        }
    }
    if (tr->playing == TRACK_ACT_END && IsReplayKind(tr->kind))
        tr->playing = 0;        /* the ROM resets it in each kind's branch: pose kinds stay */
}

// CT3 0x0184bf60
// Puts the key each running (TRACK_PLAYING) or ending (TRACK_ACT_END) track stands on into what it
// animates, without moving the time: set when the parts header's ticks are 0 or the track is ending,
// else blended by AnimWeights. The pose kinds are EA_100's; here the rest: the part's FLOAT0_7_9 /
// VECTOR_XYZ / NUMBER / FLOAT1_1_14 values, the UV offset of a group of ga objects, the vertex
// positions, normals and alphas of the objects the key's sub-records name; a COPY track's key word
// goes to its group's objects either way. An ending track of these kinds stops (playing 0). Always 0.
// PORT: (bank, anim) for the behaviour's EE word. The ROM keeps one stack vector across the tracks
// (FLOAT0_7_9 writes its x only, UV its xy): `held` is that vector, zeroed where the ROM's stack is stale.
// callcheck: asContInterpolateGA_VECTORZipNormal_XYZ0, asContInterpolateGA_VECTORZipTrans_XYZ1,
// callcheck: asContInterpolateGA_VECTORZipTrans_XYZ1_nw, asContInterpolateVector, asContInterpolateVector_nw,
// callcheck: asContZipNormal2Normal_XYZ0, asContZipTrans2Trans_XYZ1, asContZipTrans2Trans_XYZ1_nw,
// callcheck: asInterpolateNumber, asInterpolateVector, asInterpolateZipFloat1_1_14X, asMemCopy128c,
// callcheck: asMemCopy128c_nw, asZipFloat0_7_9X2Float, asZipVECTOR2VECTOR_XYZ1 (in ReplayTrack)
int EA_010(int bank, u_int anim, ALL_TABLE *tbl)
{
    BEHAVIOR *bhv = BANK_PTR(BEHAVIOR *, bank, anim);
    u_char *hdr = BANK_PTR(u_char *, bank, tbl->partsHeader);
    int ticks = *(int *)(hdr + PARTS_HDR_ANIM_ABS);
    int dir = *(int *)(hdr + PARTS_HDR_ANIM_SIGN);
    int total = *(int *)(hdr + PARTS_HDR_ANIM_STEP);
    alignas(16) float held[4] = { 0, 0, 0, 0 };
    alignas(16) float w[4] = { 0, 0, 0, 0 };
    int g, t;
    for (g = 0; g < bhv->groupCount; g++) {
        const BEHAVIOR_GROUP *grp = &bhv->groups[g];
        u_char *part = hdr + grp->part * (int)sizeof(MODEL_PARTS);
        BEHAVIOR_TRACK *tr;
        if (grp->tracks == 0)
            continue;
        tr = BehaviorTracks(bank, bhv, g);
        for (t = grp->trackCount; t > 0; t--, tr++) {
            if (tr->playing != TRACK_PLAYING && tr->playing != TRACK_ACT_END)
                continue;
            if (ticks != 0 && tr->playing != TRACK_ACT_END) {
                u_int elapsed = (u_int)anim_play_slow * (u_int)gGlobalNextTicksInterval * tr->playRate / 100;
                AnimWeights(w, ticks, elapsed, total, dir);
            }
            ReplayTrack(bank, tbl, part, tr, ticks == 0 || tr->playing == TRACK_ACT_END, w, held);
        }
    }
    return 0;
}
