// sysmem.h -- IOP system memory, as RXSNDDRV includes it (macros onto iop_host.h).
#pragma once

#include "iop_host.h"

#define AllocSysMemory AlyssaPan_IopAllocSysMemory
#define FreeSysMemory AlyssaPan_IopFreeSysMemory
#define QueryMemSize AlyssaPan_IopQueryMemSize
#define QueryMaxFreeMemSize AlyssaPan_IopQueryMaxFreeMemSize
#define QueryTotalFreeMemSize AlyssaPan_IopQueryTotalFreeMemSize
