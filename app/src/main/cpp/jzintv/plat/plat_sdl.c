/*
 * ============================================================================
 *  Title:    Platform-specific Initialization Functions for SDL1 and SDL2.
 *  Author:   J. Zbiciak
 * ============================================================================
 *  Platform-specific initialization for SDL.
 * ============================================================================
 *  PLAT_INIT -- Platform-specific initialization. Returns non-zero on fail.
 * ============================================================================
 */

#include "config.h"
#include "sdl_jzintv.h"
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

bool plat_is_batch_mode(void) { return false; }

#if GET_TIME_STRATEGY == GTS_WIN_PERF_COUNTERS
double win32_get_time_fallback(void)
{
    return SDL_GetTicks() / 1000.0;
}
#endif

#if GET_TIME_STRATEGY == GTS_SDL_GETTICKS
double get_time(void)
{
    return SDL_GetTicks() / 1000.0;
}
#endif

#if PLAT_DELAY_STRATEGY == PDS_SDL_DELAY
void plat_delay(unsigned delay)
{
    SDL_Delay(delay);
}
#endif


int plat_init(void)
{
#ifdef GP2X
    setenv("SDL_NOMOUSE", "1", 1);
#endif

    /* -------------------------------------------------------------------- */
    /*  Do a quick endian-check to ensure we were compiled correctly.       */
    /* -------------------------------------------------------------------- */
#ifndef __EMSCRIPTEN__
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
#endif
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    /* -------------------------------------------------------------------- */
    /*  Call SDL_Init and ask for Audio and Video.   This call is made      */
    /*  before we drop elevated privs (if we have any), so this should      */
    /*  allow us to get DGA access if we are suid-root.                     */
    /* -------------------------------------------------------------------- */
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK |
                 SDL_INIT_NOPARACHUTE) != 0)
    {
        fprintf(stderr, "plat_init:  SDL_Init failed!  Reason: '%s'\n",
                SDL_GetError());
        return -1;
    }

#if 0 && defined(N900)
    {
        int haa;
        if ((haa = HAA_Init(0)) != 0)
        {
            fprintf(stderr, "HAA_Init init failed."
                    "HA! HA! The error code: %d\n", haa);
            return -1;
        }
    }
#endif

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
#ifdef N900
    //HAA_Quit();
#endif
    SDL_Quit();
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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
