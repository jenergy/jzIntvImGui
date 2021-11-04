/* ======================================================================== */
/*  Cart Reader Tester                                                      */
/* ======================================================================== */

#include "config.h"
#include "cart.h"

cart_rd_t cr;

int main()
{
    unsigned w, r;

    cr_init_ports(0);
    cr.port = cr_detect(0);
    cr.reset_delay = 10000;

    if (!cr.port)
    {
        printf("No reader found. Aborting.\n");
        exit(1);
    }

    cr_do_reset(&cr);

    if (cr_selftest(&cr, &w, &r))
    {
        printf("Selftest failed:  Wrote %.4X, Read %.4X\n", w, r);
        exit(1);
    } else
    {
        printf("Selftest succeeded\n");
    }

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
