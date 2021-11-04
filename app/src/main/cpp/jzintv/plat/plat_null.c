/*
 * ============================================================================
 *  Title:    Platform-specific Initialization Functions without SDL
 *  Author:   J. Zbiciak
 * ============================================================================
 *  Platform-specific initialization without SDL.
 * ============================================================================
 *  PLAT_INIT -- Platform-specific initialization. Returns non-zero on fail.
 * ============================================================================
 */

#include "config.h"
#include "plat/plat.h"
#include "plat/plat_lib_config.h"


#ifdef DIRECT_INTV2PC
# include <unistd.h>
# ifndef WIN32
#  include <sys/io.h>
# endif
# include "periph/periph.h"
# include "pads/pads_intv2pc.h"
#endif

static void plat_quit(void);

bool plat_is_batch_mode(void) { return true; }

#if GET_TIME_STRATEGY == GTS_WIN_PERF_COUNTERS
#   if defined(CLOCK_MONOTONIC)
double win32_get_time_fallback(void)
{
    struct timespec now;
    double seconds;

    clock_gettime(CLOCK_MONOTONIC, &now);

    seconds = (double)now.tv_sec + (double)now.tv_nsec * 1e-9;
    return seconds;
}
#   else
double win32_get_time_fallback(void)
{
    return clock() / (1000.0 * CLOCKS_PER_SEC);
}
#   endif
#endif

#if GET_TIME_STRATEGY == GTS_SDL_GETTICKS
/* Uh-oh, we need a fallback.  Let's hope 'clock' and CLOCKS_PER_SEC work */
double get_time(void)
{
    return clock() / (1000.0 * CLOCKS_PER_SEC);
}
#endif

#if PLAT_DELAY_STRATEGY == PDS_SDL_DELAY
void plat_delay(unsigned delay)
{
    plat_delay_no_sdl(delay);
}
#endif

int plat_init(void)
{
    /* -------------------------------------------------------------------- */
    /*  Do a quick endian-check to ensure we were compiled correctly.       */
    /* -------------------------------------------------------------------- */
    {
        union { uint8_t byte[4]; uint32_t word; } endian;

        endian.word = 0x2A3A4A5A;

#ifdef BYTE_BE
        if (endian.byte[0] != 0x2A || endian.byte[1] != 0x3A ||
            endian.byte[2] != 0x4A || endian.byte[3] != 0x5A)
        {
            fprintf(stderr,
              "jzIntv built for Big Endian, but target gave following "
              "byte order:\n    %.2X %.2X %.2X %.2X\n",
              endian.byte[0], endian.byte[1], endian.byte[2], endian.byte[3]);
            exit(1);
        }
#endif
#ifdef BYTE_LE
        if (endian.byte[3] != 0x2A || endian.byte[2] != 0x3A ||
            endian.byte[1] != 0x4A || endian.byte[0] != 0x5A)
        {
            fprintf(stderr,
              "jzIntv built for Little Endian, but target gave following "
              "byte order:\n    %.2X %.2X %.2X %.2X\n",
              endian.byte[0], endian.byte[1], endian.byte[2], endian.byte[3]);
            exit(1);
        }
#endif
    }

    atexit(plat_quit);

#if defined(GP2X)
    {
        extern void gp2x_init(void);
        gp2x_init();
    }
#endif

#if defined(WII)
    {
        extern void wii_init(void);
        wii_init();
    }
#endif

#if defined(DIRECT_INTV2PC) && !defined(WIN32)
    /* -------------------------------------------------------------------- */
    /*  If direct hand-controller interface support is compiled in, try     */
    /*  to give ourself permission to the printer-port I/O address ranges.  */
    /*  Do this only if our EUID == 0.  We have to do this blindly,         */
    /*  because we do the platform init before we do argument and config    */
    /*  parsing since we need to drop privs as quickly as possible.         */
    /* -------------------------------------------------------------------- */
    if (!geteuid())
    {
        pads_intv2pc_ports_ok = 0;
        if (ioperm(0x378, 3, 1) == 0) pads_intv2pc_ports_ok |= 1;
        if (ioperm(0x278, 3, 1) == 0) pads_intv2pc_ports_ok |= 2;
        if (ioperm(0x3BC, 3, 1) == 0) pads_intv2pc_ports_ok |= 4;
    }
#endif

#if defined(DIRECT_INTV2PC) && defined(WIN32)
    pads_intv2pc_ports_ok = 7;
#endif


#ifndef NO_SETUID
    /* -------------------------------------------------------------------- */
    /*  If we have elevated privileges, drop them here, now, immediately.   */
    /*  Warn if it fails, but otherwise don't worry too much.  I have       */
    /*  seriously put very little thought into securing jzIntv, and it      */
    /*  should never be used SUID/SGID anyway.                              */
    /* -------------------------------------------------------------------- */
    if (getegid() != getgid())
    {
        if (setegid(getgid()) != 0)
            fprintf(stderr, "WARNING: Could not set EGID == GID.\n");
    }
    if (geteuid() != getuid()) 
    {
        if (seteuid(getuid()) != 0)
            fprintf(stderr, "WARNING: Could not set EUID == UID.\n");
    }
#endif

    return 0;
}

static void plat_quit(void)
{
}

/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License along */
/*  with this program; if not, write to the Free Software Foundation, Inc., */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             */
/* ======================================================================== */
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
