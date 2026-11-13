// sdk/newlib.h -- the newlib calls CT3 makes that the host's C library lacks or hides (2026-11-09).
// The ROM links newlib (architecture.md "Binary and toolchain"). strupr is not standard C, and each host
// library has its own: MinGW keeps it behind its old names, which a strict C++ build does not declare, and
// the UCRT declares it __declspec(dllimport) under /MD, which a plain declaration here contradicts (C2375,
// or calls bound to the CRT's locale-dependent one when <string.h> came first). So the host's <string.h>
// goes first, with whatever strupr it has, and the game's calls are renamed onto this file's own: one
// ASCII strupr on every compiler, as newlib's C locale has it. cpp-port.md's shim (e) (newlib rand)
// belongs here too.
#pragma once

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// newlib strupr (CT3 0x01831720): upper-case s in place (ASCII), returns s.
char *newlib_strupr(char *s);

#ifdef __cplusplus
}
#endif

#define strupr newlib_strupr
