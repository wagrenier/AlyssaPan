// thevent.h -- IOP event flags, as RXSNDDRV includes them (macros onto iop_host.h).
#pragma once

#include "iop_host.h"

#define EventFlagParam IopEventFlagParam
#define CreateEventFlag AlyssaPan_IopCreateEventFlag
#define SetEventFlag AlyssaPan_IopSetEventFlag
// The interrupt-context form: the host has no separate one.
#define iSetEventFlag AlyssaPan_IopSetEventFlag
#define WaitEventFlag AlyssaPan_IopWaitEventFlag
