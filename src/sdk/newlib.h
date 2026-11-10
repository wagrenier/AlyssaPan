// sdk/newlib.h -- the newlib calls CT3 makes that the host's C library lacks or hides (2026-11-09).
// The ROM links newlib (architecture.md "Binary and toolchain"); MinGW keeps strupr behind its old names,
// which a strict C++ build does not declare. cpp-port.md's shim (e) (newlib rand) belongs here too.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// newlib strupr (CT3 0x01831720): upper-case s in place (ASCII), returns s.
char *strupr(char *s);

#ifdef __cplusplus
}
#endif
