/* ======================================================================== */
/*  Dumps a range of addresses from an ECScable-enabled program.            */
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

uint16_t data[65536];

int main(int argc, char *argv[])
{
    int i, lo, hi, icart = 0, viddis = 0, page = -1;
    ecscable_t ec;
    FILE *f;

    if (argc < 4)
    {
usage:
        fprintf(stderr,
                "usage: ec_dump [-v] [-i] [-p] lo_addr hi_addr file.bin\n"
                "\n"
                "    -v   Blank video during dump (eg. for GRAM/GROM dump)\n"
                "    -i   Dump from Intellicart address space\n"
                "    -pX  Dump from ECS-style page number X\n"
        );

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

    while (*argv[1] && *argv[1] == '-')
    {
        switch (argv[1][1])
        {
            case 'v': viddis = 1; break;
            case 'i': icart  = 1; break;
            case 'p': page   = atoi(argv[1]+2); break;
            default:  printf("bad switch '%s'\n", argv[1]); goto usage;
        }
        argv++;
        argc--;
    }

    if (argc < 4)
        goto usage;

    if (sscanf(argv[1], "%x", &lo) != 1 ||
        sscanf(argv[2], "%x", &hi) != 1 || hi < lo)
    {
        fprintf(stderr, "Invalid address range.\n");
        goto usage;
    }

    if ((f = fopen(argv[3], "wb")) == NULL)
    {
        fprintf(stderr, "Couldn't open '%s' for writing\n", argv[3]);
        goto usage;
    }

    if (viddis) ec_video(&ec, 0);
    if (page >= 0 && page <= 15)
        for (i = lo & 0xF000; i <= hi; i += 0x1000)
            ec_ecs_setpage(&ec, i, page);

    if (ec_download(&ec, lo, hi - lo + 1, data, 16, icart))
    {
        fprintf(stderr, "Error during download\n");
        exit(1);
    }
    if (viddis) ec_video(&ec, 1);

#ifdef BYTE_LE
    for (i = 0; i < hi - lo + 1; i++)
        data[i] = (0xFF & (data[i] >> 8)) | ((data[i] & 0xFF) << 8);
#endif

    fwrite(data, 2, hi - lo + 1, f);
    fclose(f);

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
