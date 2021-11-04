/* ======================================================================== */
/*  Intellivoice Player                                                     */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/io.h>
#include "cart.h"

#define SLEEP_TIME (200000)

cart_rd_t cr, *crp = &cr;

/* ======================================================================== */
/*  SPC_DO_ALD   -- Do address load.  Wait for LRQ and load new address.    */
/*                  Returns -1 immediately if "no_block" is set, and LRQ    */
/*                  isn't asserted.                                         */
/* ======================================================================== */
int spc_do_ald(int addr, int no_block)
{
    int ready;

    ready = cr_do_read(crp, 0x80) & 0x8000;
    if (!no_block)
        while (!ready)
        {
            usleep(SLEEP_TIME);
            ready = cr_do_read(crp, 0x80) & 0x8000;
        }

    if (ready)
        cr_do_write(crp, 0x80, addr);

    return ready ? 0 : -1;
}

/* ======================================================================== */
/*  SPC_DO_FIFO  -- Do FIFO load.  Wait for FIFO to have room and load a    */
/*                  new decle.  Return -1 immediately if "no_block" set     */
/*                  and FIFO is full.                                       */
/* ======================================================================== */
int spc_do_fifo(int data, int no_block)
{
    int ready;

    ready = (cr_do_read(crp, 0x81) & 0x8000) == 0;
    if (!no_block)
        while (!ready)
        {
            usleep(SLEEP_TIME);
            ready = (cr_do_read(crp, 0x81) & 0x8000) == 0;
        }

    if (ready)
        cr_do_write(crp, 0x81, data);

    return ready ? 0 : -1;
}

unsigned unpacked[4096];

/* ======================================================================== */
/*  MAIN.                                                                   */
/* ======================================================================== */
main()
{
    int i, j, d, c, b, eof = 0;
    int started = 0;
    char buf[1024];

    cr_init_ports(0);
    cr.port = cr_detect(0);
    cr.reset_delay = 1000000;

    if (!cr.port)
    {
        printf("No reader found. Aborting.\n");
        exit(1);
    }

    cr_do_reset(crp);
    cr_set_disp(crp, 0xFF);

    /* -------------------------------------------------------------------- */
    /*  Reset the FIFO, ready it for data.                                  */
    /* -------------------------------------------------------------------- */
    cr_do_write(crp, 0x81, 0x400);
    cr_do_write(crp, 0x81, 0x400);
    while (cr_do_read(crp, 0x81) & 0x8000)
        ;
    while ((cr_do_read(crp, 0x80) & 0x8000) == 0)
        ;

    /* -------------------------------------------------------------------- */
    /*  Play the Mattel Electronics Presents stream, just because we can.   */
    /* -------------------------------------------------------------------- */
    /*spc_do_ald(0x06, 0);  /* Mattel Electronics Presents  */
    spc_do_ald(0x02, 0);    /* (pause)                      */


    /* -------------------------------------------------------------------- */
    /*  Read byte stream from stdin, and convert into decle stream.         */
    /* -------------------------------------------------------------------- */
#if 1
    d = b = j = eof = 0;
    while (!eof)
    {
        c = getchar();
        if (c == EOF) { c = 0; eof = 1; }

        for (i = 7; i >= 0; i--)
        {
            d |= ((c >> i) & 1) << b;
            b++;

            if (b == 10)
            {
                unpacked[j++] = d;
                d = b = 0;
            }
        }
    }
    for (i = 0; i < j; i++)
    {
        printf(" %.3X", unpacked[i]);
        fflush(stdout);
        if (spc_do_fifo(unpacked[i], 1))
        {
            if (!started)
            {
                spc_do_ald(0x00, 0);
                started = 1;
                printf("\nALD\n");
            }
            spc_do_fifo(unpacked[i], 0);
        }
        d = 0;
        b = 0;
    }
#else
    while (fgets(buf, 1024, stdin))
    {
        sscanf(buf, "%x", &d);
        printf("Writing %.3X\n", d);
        fflush(stdout);
        while (spc_do_fifo(d, !started))
        {
            printf("Wrote 0x00 to ALD\n");
            started = 1;
            spc_do_ald(0x00, 0);
        }
    }
#endif
    printf("\n\n");
    if (!started) spc_do_ald(0x00, 0);

    /* -------------------------------------------------------------------- */
    /*  Done.  Wait a couple seconds, and then RESET the bus.               */
    /* -------------------------------------------------------------------- */
    spc_do_ald(0x02, 0);    /* Send a pause */
    while (cr_do_read(crp, 0x81) & 0x8000)
        usleep(SLEEP_TIME);
    while ((cr_do_read(crp, 0x80) & 0x8000) == 0)
        usleep(SLEEP_TIME);
    cr_do_reset(crp);

    return 0;
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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
