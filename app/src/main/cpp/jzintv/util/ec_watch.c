/* ======================================================================== */
/*  "Watches" values in an ECScable-enabled program.                        */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "config.h"
#include "util/ecscable.h"

int comp_int(const void *a, const void *b)
{
    return *((const int*)a) - *((const int*)b);
}

volatile int please_die = 0;

void die(int x)
{
    UNUSED(x);

    if (please_die > 2)
    {
        fprintf(stderr, "Ok, fine, exiting.\n");
        exit(1);
    }
    please_die++;
    fprintf(stderr, "Scheduling exit...\n");
}


int main(int argc, char *argv[])
{
    int i, lo, hi, tot;
    ecscable_t ec;
    int addr[32];
    uint16_t data[32];

    if (argc < 2)
    {
usage:
        fprintf(stderr, "usage: ec_watch addr [addr [addr]]\n");
        exit(1);
    }

    signal(2,  die);
    signal(15, die);

    /* First, get set up. */
    ec_init_ports(0);
    if (!(ec.port = ec_detect(0)))
    {
        fprintf(stderr, "No ECS cable\n");
        exit(1);
    }
    ec_idle(&ec);

    for (i = 1; i < argc && i <= 32; i++)
        if (sscanf(argv[i], "%x", &addr[i - 1]) != 1)
             goto usage;

    tot = i - 1;
    qsort(addr, tot, sizeof(addr[0]), comp_int);

    while (!please_die)
    {
        ec_ping(&ec);
        if (please_die) exit(0);
        printf("\r");
        for (i = 1, lo = hi = 0; i <= tot; i++)
        {
            if (i < tot && addr[i] == addr[i-1]+1)
            {
                hi = i;
            } else
            {
                printf("[%.4X-%.4X] ", addr[lo], addr[hi]);
                ec_download(&ec, addr[lo], addr[hi] - addr[lo] + 1,
                            data + lo, 16, 0);
                if (please_die) exit(0);
                while (lo < i)
                    printf("%.4X ", data[lo++]);
                lo = hi = i;
            }
        }
        ec_sleep(20000);
        fflush(stdout);
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
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
