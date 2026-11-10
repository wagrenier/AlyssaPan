// sdk/newlib.cpp -- newlib calls the host's C library lacks or hides (sdk/newlib.h).

#include "sdk/newlib.h"

extern "C" char *strupr(char *s)
{
    for (char *p = s; *p != '\0'; ++p) {
        if (*p >= 'a' && *p <= 'z') {
            *p = static_cast<char>(*p - 'a' + 'A');
        }
    }
    return s;
}
