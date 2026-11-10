// game/ahRelocateSunPs2ObjectArry.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/ahRelocateSunPs2ObjectArry.h"

#include <math.h>
#include <string.h>

#include "ga/gaInit3dlib.h"
#include "ga/gaSearchNearLight4.h"
#include "game/Convert.h"
#include "game/LinkMatrixTree.h"
#include "game/QuatSlerp.h"
#include "game/ahalloc.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/ssCalcInversMatrix2.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"

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
            tr->value0e = TRACK_VALUE0E_START;
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
            asDrawShadowVolumeInit(2);
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
        track->value0e = 0;
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

// Rows 0..2 at rows: the key's quaternion (ticks 0), or the slerp from the rows' own by w.
static void PoseRotKey(float *rows, const short *key, int ticks, float *w)
{
    alignas(16) float q[4], q0[4], qs[4];
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

// A position key (TRACK_KEY_RAW: a float[4] at keys + 0x10) into dst, or blended in by w.
static void PosePosKey(float *dst, const BEHAVIOR_TRACK *tr, const u_char *keys, int ticks, float *w)
{
    alignas(16) float v[4];
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

// CT3 0x0184d1a0
// Every running track of the behaviour takes its key at this frame's ticks (anim_play_slow *
// gGlobalNextTicksInterval * value0e / 100; atKey: where it stands) and, for a pose kind, puts it in
// its group's part (set when the parts header's ticks are 0, else blended by AnimWeights) and marks
// the part PARTS_ANIM_DIRTY. With mat, the EA_001_PART_HASH part's keys build base * local as EA_001
// does (every done byte set). Every track's time1c shifts up and takes time / 100. The call is
// recorded for the history first; the ticks are counted down after. 1 when no track runs on.
// PORT: (bank, anim) for the behaviour's EE word.
// callcheck: ConvertMatrix2Quat, QuatSlerp (in PoseRotKey)
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
            elapsed = (u_int)anim_play_slow * (u_int)gGlobalNextTicksInterval * tr->value0e / 100;
            AnimWeights(w, ticks, elapsed, total, dir);
            if (tr->playing == TRACK_PLAYING) {
                int pose = IsPoseTrack(tr->kind);
                int end = InterpolateAnimationKey(tr, atKey ? 0 : (int)elapsed);
                const u_char *keys;
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
                keys = BANK_PTR(const u_char *, bank, tr->keys); /* read after the call, as the ROM */
                switch (tr->kind) {
                case TRACK_KIND_VECTOR: {
                    alignas(16) float v[4];
                    float *dst = (float *)(part + PARTS_ANIM_VECTOR);
                    if (ticks == 0) {
                        asZipVECTOR2VECTOR_XYZ1(dst, (const short *)(keys + 8));
                    } else {
                        asZipVECTOR2VECTOR_XYZ1(v, (const short *)(keys + 8));
                        asInterpolateVector(dst, dst, v, w);
                    }
                    part[PARTS_ANIM_FLAGS] |= PARTS_ANIM_DIRTY;
                    break;
                }
                case TRACK_KIND_BASE_POS:
                case TRACK_KIND_POS:
                    PosePosKey((float *)(part + (tr->kind == TRACK_KIND_POS ? PARTS_ANIM_POS : PARTS_ANIM_BASE_POS)),
                               tr, keys, ticks, w);
                    part[PARTS_ANIM_FLAGS] |= PARTS_ANIM_DIRTY;
                    break;
                case TRACK_KIND_BASE_ROT:
                case TRACK_KIND_ROT:
                    PoseRotKey((float *)(part + (tr->kind == TRACK_KIND_ROT ? PARTS_ANIM_ROT : PARTS_ANIM_BASE_ROT)),
                               (const short *)(keys + 8), ticks, w);
                    part[PARTS_ANIM_FLAGS] |= PARTS_ANIM_DIRTY;
                    break;
                default:
                    break;
                }
                if (mat != NULL && ((MODEL_PARTS *)part)->hash == EA_001_PART_HASH) {
                    int slot = -1;
                    float *dst = NULL;
                    switch (tr->kind) {
                    case TRACK_KIND_BASE_POS: slot = 3; dst = base[3]; break;
                    case TRACK_KIND_POS: slot = 1; dst = local[3]; break;
                    case TRACK_KIND_BASE_ROT: slot = 2; break;
                    case TRACK_KIND_ROT: slot = 0; break;
                    default: break;
                    }
                    if (slot >= 0) {
                        InterpolateAnimationKey(tr, 0);
                        keys = BANK_PTR(const u_char *, bank, tr->keys);
                        if (dst != NULL) {
                            if (tr->length & TRACK_KEY_RAW)
                                memcpy(dst, keys + 0x10, 0x10);
                            else
                                asZipTrans2Trans_XYZ1(dst, (const short *)(keys + 8), TRACK_KEY_SHIFT(tr->length));
                        } else {
                            alignas(16) float q[4], rot[4][4];
                            asZipQuat2Quat(q, (const short *)(keys + 8));
                            ConvertQuat2Matrix(rot, q);
                            memcpy(slot == 2 ? base : local, rot, 0x30);
                        }
                        done[slot] = 1;
                    }
                }
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

// CT3 0x0184a570
// The pass by the flags: play alone EA_100, replay alone EA_010, matOnly alone EA_001; any other mix
// runs the general body (the three passes in one; not reached by the callers written so far).
// PORT: (bank, anim) for the behaviour's EE word; mat a host pointer.
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
    ALYSSAPAN_STUB("ExecuteAnimation (the general body)", 0x0184a688);
    return 0;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018483d0
u_long asSceneAnimationStart(float speed, int a0, ALL_TABLE *tbl, int a2, int frame, int id, int a6,
                             int a7, int a8, int a9, int a10, int a11, int a12, int a13)
{
    ALYSSAPAN_STUB("asSceneAnimationStart", 0x018483d0);
    return {};
}

// stub of CT3 0x0184bf60
int EA_010(int bank, u_int anim, ALL_TABLE *tbl)
{
    ALYSSAPAN_STUB("EA_010", 0x0184bf60);
    return {};
}

// stub of CT3 0x01844df0
void asShadowPolygonPositions(int bank)
{
    ALYSSAPAN_STUB("asShadowPolygonPositions", 0x01844df0);
}

// stub of CT3 0x018427f0
void asUpdateAnimation_Matrix(void)
{
    ALYSSAPAN_STUB("asUpdateAnimation_Matrix", 0x018427f0);
}

// stub of CT3 0x0184f220
void asBuildMirrorMatrixTree(void *tree)
{
    ALYSSAPAN_STUB("asBuildMirrorMatrixTree", 0x0184f220);
}

// stub of CT3 0x0184f120
void asBuildMirrorMatrixTreeBack(void *tree)
{
    ALYSSAPAN_STUB("asBuildMirrorMatrixTreeBack", 0x0184f120);
}


// stub of CT3 0x0184f910
int asMakeMirrorMatrixByPlane(float *planeVec, float *planePoint)
{
    ALYSSAPAN_STUB("asMakeMirrorMatrixByPlane", 0x0184f910);
    return {};
}

// stub of CT3 0x0184f390
void asMirrorCulling(int on)
{
    ALYSSAPAN_STUB("asMirrorCulling", 0x0184f390);
}



// stub of CT3 0x018444b0
void asDrawShadowVolume(int bank)
{
    ALYSSAPAN_STUB("asDrawShadowVolume", 0x018444b0);
}

// stub of CT3 0x018461f0
void asDrawShadowVolumeInit(int a0)
{
    ALYSSAPAN_STUB("asDrawShadowVolumeInit", 0x018461f0);
}

// stub of CT3 0x01847660
int asShadowDrawing_CheckTransMode(int mode)
{
    ALYSSAPAN_STUB("asShadowDrawing_CheckTransMode", 0x01847660);
    return {};
}

// stub of CT3 0x01846360
void asShadowEnd(int bank)
{
    ALYSSAPAN_STUB("asShadowEnd", 0x01846360);
}

// stub of CT3 0x018471c0
void asShadowStart(void)
{
    ALYSSAPAN_STUB("asShadowStart", 0x018471c0);
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

// stub of CT3 0x0184dbc0
int InterpolateAnimationKey(BEHAVIOR_TRACK *track, int dir)
{
    ALYSSAPAN_STUB("InterpolateAnimationKey", 0x0184dbc0);
    return {};
}
