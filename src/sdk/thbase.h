// thbase.h -- the IOP thread manager, as RXSNDDRV includes it. The SDK's names are macros onto
// iop_host.h: the EE kernel (eekernel.h) and Win32 have a CreateThread of their own.
#pragma once

#include "iop_host.h"

#define TH_C 0x02000000

#define ThreadParam IopThreadParam
#define CreateThread AlyssaPan_IopCreateThread
#define StartThread AlyssaPan_IopStartThread
#define GetThreadId AlyssaPan_IopGetThreadId
