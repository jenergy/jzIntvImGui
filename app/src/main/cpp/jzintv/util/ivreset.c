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

cart_rd_t cr, *crp = &cr;;

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
            ready = cr_do_read(crp, 0x80) & 0x8000;

    if (ready)
        cr_do_write(crp, 0x80, addr);

    return ready ? 0 : -1;
}

/* ======================================================================== */
/*  MAIN.                                                                   */
/* ======================================================================== */
main()
{
    int i, j, d, c, b, eof = 0;
    int started = 0;

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
    /*  Play a pause.                                                       */
    /* -------------------------------------------------------------------- */
    spc_do_ald(0x02, 0);    /* (pause)                      */

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
