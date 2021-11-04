/*
 * ============================================================================
 *  TEST_HCIF:      Test program for the Hand Controller Interface
 *
 *  Author:         J. Zbiciak
 *
 *
 * ============================================================================
 *  This program is a simple utility for testing the "Nudds/Moeller
 *  Hand Controller Interface", a device which allows connecting actual
 *  Intellivision Hand Controllers via a PC printer port.
 *
 *  This utility only works under Linux on a PC.  When built on unsupported
 *  platforms, this utility merely prints an error and exits.
 *
 *  Usage:  "test_hcif [LPT# [Rate]]"
 *
 *  LPT port # is 1, 2, or 3.  If omitted, 1 is assumed.
 *  Rate is the LPT query rate in HZ.
 * ===========================================================================
 */


#if (!defined(PLAT_LINUX) && !defined(WIN32)) || !defined(i386)

#include <stdio.h>

/*
 * ===========================================================================
 *  Handle unsupported platforms by providing a short 'main()' which
 *  prints an error message and exits.
 * ===========================================================================
 */
int main(void)
{
    printf
    (
        "Sorry, the Nudds/Moeller Hand Controller Interface is not\n"
        "supported this platform.  The Hand Controller Interface is \n"
        "supported under Linux or Windows on a PC only at this time.\n"
    );
    return 1;
}

#else

#include "config.h"
#include "plat/plat_lib.h"
#include <assert.h>
#include <ctype.h>


/*
 * ===========================================================================
 *  INIT_PORTS   -- Get access to the desired I/O ports and drop root.
 *                  Don't bother returning an error if we fail.  Just abort.
 * ===========================================================================
 */
void init_ports(unsigned long base)
{
    /* -------------------------------------------------------------------- */
    /*  First, sanity check the port number.  If it is not one of the       */
    /*  standard printer port #'s, then abort.                              */
    /* -------------------------------------------------------------------- */
    if (base != 0x378 && base != 0x278 && base != 0x3BC)
    {
        fprintf(stderr, "test_hcif:  Invalid base address 0x%.4lX\n", base);
        exit(1);
    }

#ifndef NO_SETUID
    /* -------------------------------------------------------------------- */
    /*  Grant ourself permission to access ports 'base' through 'base+2'.   */
    /* -------------------------------------------------------------------- */
    if (ioperm(base, 3, 1))
    {
        fprintf(stderr, "test_hcif:  Unable to set I/O permissions\n");
        perror("test_hcif: ioperm()");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Drop elevated privs if we have them.                                */
    /* -------------------------------------------------------------------- */
    if (getegid() != getgid()) setgid(getgid());
    if (geteuid() != getuid()) setuid(getuid());
#endif

    return;
}

#ifndef NO_NANOSLEEP
/*
 * ===========================================================================
 *  HCIF_DELAY   -- Amount of time to sleep when reading the HCIF, in
 *                  nanoseconds.
 * ===========================================================================
 */
struct timespec hcif_delay = { 0, 10000000 };   /* 10ms */

/*
 * ===========================================================================
 *  HCIF_RATE    -- Set the (approximate) HCIF query rate.
 * ===========================================================================
 */
void hcif_rate(unsigned rate)
{
    hcif_delay.tv_nsec = 500000000 / rate - 10000000;
}

/*
 * ===========================================================================
 *  HCIF_SLEEP   -- Sleep for HCIF_DELAY by calling nanosleep.
 * ===========================================================================
 */
inline void hcif_sleep(void)
{
    struct timespec delay = hcif_delay, remain;

    nanosleep(&delay, &remain); /* don't bother looping. */

#if 0
    while (nanosleep(&delay, &remain) == -1 && errno == EINTR)
        delay = remain;
#endif
}


#else /* NO_NANOSLEEP */

long hcif_delay = 1000000 / 100; /* 10ms == 100Hz */

void hcif_rate(unsigned rate)
{
    hcif_delay = 500000 / rate - 10000;
}

/* ------------------------------------------------------------------------ */
/*  HCIF_SLEEP   -- Sleep for HCIF_DELAY by busywaiting                     */
/* ------------------------------------------------------------------------ */
inline void hcif_sleep(void)
{
    struct timeval x, y;
    long diff = 0;

    gettimeofday(&x, 0);
    while (diff < hcif_delay)
    {
        gettimeofday(&y, 0);
        diff = (y.tv_sec  - x.tv_sec ) * 1000000 +
               (y.tv_usec - x.tv_usec);
    }
    return;
}
#endif

/*
 * ===========================================================================
 *  HCIF_READ    -- Reads one side of the hand-controller interface.
 * ===========================================================================
 */
unsigned hcif_read(unsigned long base, int side)
{
    unsigned val;

    assert(side == 0 || side == 1);

    /* -------------------------------------------------------------------- */
    /*  Get low nybble.                                                     */
    /* -------------------------------------------------------------------- */
    outb((2<<side)|1, base); hcif_sleep();
    val  = (inb(base + 1) >> 4) & 0x0F;

    /* -------------------------------------------------------------------- */
    /*  Get high nybble.                                                    */
    /* -------------------------------------------------------------------- */
    outb((2<<side)|0, base); hcif_sleep();
    val |= (inb(base + 1)     ) & 0xF0;

    /* -------------------------------------------------------------------- */
    /*  Return value w/ inverted bits fixed.                                */
    /* -------------------------------------------------------------------- */
    return val ^ 0x77;
}

#define HC_EXACT    (0x80000000)        /* Decoding is exact.           */
#define HC_DISC     (0x0000000F)        /* Disc direction (0 - 15)      */
#define HC_DISC_R   (0x00100000)        /* Disc "right"                 */
#define HC_DISC_UR  (0x00200000)        /* Disc "up + right"            */
#define HC_DISC_U   (0x00400000)        /* Disc "up"                    */
#define HC_DISC_UL  (0x00800000)        /* Disc "up + left"             */
#define HC_DISC_L   (0x01000000)        /* Disc "left"                  */
#define HC_DISC_DL  (0x02000000)        /* Disc "down + left"           */
#define HC_DISC_D   (0x04000000)        /* Disc "down"                  */
#define HC_DISC_DR  (0x08000000)        /* Disc "down + right"          */
#define HC_KEY_1    (0x00000010)        /* Keypad key 1                 */
#define HC_KEY_2    (0x00000020)        /* Keypad key 2                 */
#define HC_KEY_3    (0x00000040)        /* Keypad key 3                 */
#define HC_KEY_4    (0x00000080)        /* Keypad key 4                 */
#define HC_KEY_5    (0x00000100)        /* Keypad key 5                 */
#define HC_KEY_6    (0x00000200)        /* Keypad key 6                 */
#define HC_KEY_7    (0x00000400)        /* Keypad key 7                 */
#define HC_KEY_8    (0x00000800)        /* Keypad key 8                 */
#define HC_KEY_9    (0x00001000)        /* Keypad key 9                 */
#define HC_KEY_C    (0x00002000)        /* Keypad key Clear             */
#define HC_KEY_0    (0x00004000)        /* Keypad key 0                 */
#define HC_KEY_E    (0x00008000)        /* Keypad key Enter             */
#define HC_ACT_0    (0x00010000)        /* Upper action button          */
#define HC_ACT_1    (0x00020000)        /* Left lower action button     */
#define HC_ACT_2    (0x00040000)        /* Right lower action button    */

/*
 * ===========================================================================
 *  HCIF_DECODE  -- Attempts to decode the inputs from a hand-controller.
 *                  The results are placed in a bit-field.
 * ===========================================================================
 */


/*
 * ===========================================================================
 *  DISP_INFO    -- Displays the status for a controller.
 * ===========================================================================
 */
void disp_info(unsigned info)
{
    int i;

    for (i = 0x80; i; i >>= 1)
        putchar((i & info) ? '1' : '0');

    printf(" [%.2X]   ", info);
}

/*
 * ============================================================================
 *  ELAPSED      -- Returns amount of time that's elapsed since the program
 *                  started, in seconds.
 * ============================================================================
 */
double elapsed(void)
{
    static struct timeval start;
    static int init = 0;
    struct timeval now;
    uint32_t usec, sec;

    if (!init)
    {
        gettimeofday(&start, NULL);
        init = 1;
    }

    gettimeofday(&now, NULL);

    if (now.tv_usec < start.tv_usec)
    {
        now.tv_usec += 1000000;
        now.tv_sec--;
    }

    usec = now.tv_usec - start.tv_usec;
    sec  = now.tv_sec  - start.tv_sec;

    return (sec + usec/1000000.);
}


/*
 * ===========================================================================
 *  MAIN         -- In the beginning...  (ok, so that joke's getting old.)
 * ===========================================================================
 */
int main(int argc, char *argv[])
{
    const unsigned long ports[3] = { 0x378, 0x278, 0x3BC };
    unsigned long port = ports[0];
    int lpt  = 1;       /* LPTx port        */
    int rate = 25;      /* Query rate (HZ)  */
    unsigned l, r;
    double actual_rate = 0., now;
    unsigned samples;

    printf("==============================================================\n");
    printf(" Test Utility for the Nudds/Moeller Hand Controller Interface\n");
    printf(" By Joseph Zbiciak.\n");
    printf("==============================================================\n");
    printf("\n");

    if (argc == 3)
    {
        int len = 8;
        char *s;

        s = argv[2];

        while (len-->0 && *s && isdigit(*s)) s++;

        if (*s || sscanf(argv[2], "%d", &rate) != 1 || rate < 1 || rate > 100)
        {
            fprintf(stderr, "test_hcif:  Rate must be 1 <= rate <= 100\n");
            exit(1);
        }
    }

    if (argc == 2)
    {
        int len = 8;
        char *s;

        s = argv[1];

        while (len-->0 && *s && isdigit(*s)) s++;

        if (*s || sscanf(argv[1], "%d", &lpt) != 1 || lpt < 1 || lpt > 3)
        {
            fprintf(stderr, "test_hcif:  LPT# must 1, 2, or 3.\n");
            exit(1);
        }

        port = ports[lpt - 1];
    }

    printf("Attempting to access HCIF on LPT%d [port 0x%.3lX], rate = %d Hz\n",
            lpt, port, rate);

    fflush(stdout);
    init_ports(port);

    printf("Setting desired query rate to %d Hz\n", rate);
    hcif_rate(rate);

    printf("Timing port reads to get actual rate... ");
    fflush(stdout);

    elapsed();
    for (samples = 0; samples < 100; samples += 2)
    {
        l = hcif_read(port, 0);
        r = hcif_read(port, 1);
    }
    now = elapsed();
    actual_rate = samples / now;
    printf("  %7.1f Hz\n\n", actual_rate);

    printf("Press 'INTR' to terminate test.\n");
    fflush(stdout);

    while (1)
    {
        l = hcif_read(port, 0);
        r = hcif_read(port, 1);

        printf("\rStatus:  ");
        disp_info(l);
        disp_info(r);

        putchar('\r');
        fflush(stdout);
    }

    return 0;
}

#endif

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

