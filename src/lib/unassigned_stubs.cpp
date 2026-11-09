// lib/unassigned_stubs.cpp -- Clock Tower 3 functions with no TU yet (tools/reverse/stubgen.py adds here
// what reconstructed code calls before its TU exists). Functions below are stubs until reconstructed.
#include "common/alyssapan_stub.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// CT3 0x01839d10
// The game's main (built as gameMain: the process entry is the host's, src/main.cpp). Its TU is the
// first one to reconstruct: open-items.md "Next up".
int gameMain(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    ALYSSAPAN_STUB("main", 0x01839d10);
    return 0;
}
