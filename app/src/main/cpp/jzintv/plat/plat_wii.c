/* ======================================================================== */
/*  WII Platform Support                                                    */
/*  Adapted to jzIntv by Daniele Moglia.                                    */
/* ======================================================================== */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

void  wii_init()
{
#ifndef USE_AS_BACKEND
    fatInitDefault();
#endif
}

/* http://lists.ozlabs.org/pipermail/linuxppc-dev/1999-October/003889.html */
LOCAL uint64_t wii_get_timebase()
{
    unsigned long long retval;
    unsigned long junk;
    __asm__ __volatile__ ("\n\
1:  mftbu   %1\n\
    mftb    %L0\n\
    mftbu   %0\n\
    cmpw    %0,%1\n\
    bne     1b"
    : "=r" (retval), "=r" (junk));
    return retval;
}

double wii_get_time()
{
    return (double)wii_get_timebase() / (1000.0 * (double)TB_TIMER_CLOCK);
}
