// lib/unassigned_stubs.cpp -- Clock Tower 3 functions with no TU yet (tools/reverse/stubgen.py adds here
// what reconstructed code calls before its TU exists). Functions below are stubs until reconstructed.
#include "common/alyssapan_stub.h"
#include "sdk/libgraph.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01810de0
void sceGsSetDefDBuff(sceGsDBuff *dp, short psm, short w, short h,
                      short ztest, short zpsm, short clear)
{
    ALYSSAPAN_STUB("sceGsSetDefDBuff", 0x01810de0);
}
