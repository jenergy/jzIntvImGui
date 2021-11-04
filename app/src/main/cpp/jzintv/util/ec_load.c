/* ======================================================================== */
/*  Downloads an Intellicart image to an ECScable enabled system.           */
/*                                                                          */
/*  Note, because this can't change the Intellicart memory map, the         */
/*  ECScable monitor must have a compatible memory map ahead of time for    */
/*  this to work.                                                           */
/* ------------------------------------------------------------------------ */
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "util/ecscable.h"
#include "icart/icartrom.h"

/* ======================================================================== */
/*  These are errors that can be reported by the Intellicart routines.      */
/* ======================================================================== */
char *rom_errors[] =
{
    "No Error",
    "Bad Arguments",
    "Bad ROM Header",
    "CRC-16 Error in ROM Segments",
    "Bad ROM Segment Address Range",
    "Bad ROM Fine-Address Range",
    "CRC-16 Error in Enable Tables",
    "Unknown Error"
};


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


void do_title(icartrom_t *ic, ecscable_t *ec)
{
    uint16_t addr;
    int year, i;
    uint16_t ctitle[20];
    char buf[21];

    addr = (ic->image[0x500A] & 0x00FF) | ((ic->image[0x500B] & 0x00FF) << 8);
    if (((ic->readable[2] >> 16) & 1) &&
        ((ic->readable[addr >> 13] >> ((addr >> 8) & 31)) & 1))
    {
        year = ic->image[addr++] + 1900;
        for (i = 0; i < 20; i++)
        {
            if (ic->image[addr + i] == 0) break;
            if (ic->image[addr + i] < 32 || ic->image[addr + i] > 127)
            {
                i = 20;
                break;
            }
        }

        if (i < 20)
        {
            for (i = 0; i < 20; i++)
            {
                if (!ic->image[addr + i]) break;
                ctitle[i] = 4 | ((ic->image[addr + i] - 0x20) << 3);
                buf[i] = ic->image[addr + i];
            }
            buf[i] = 0;

            printf("Title: %s  Year: %d\n", buf, year);
            ec_upload(ec, 0x200 + 6*20 + ((20 - i) >> 1), i, ctitle, -1, 0);
        }
    }
}

int main(int argc, char *argv[])
{
    FILE *f;
    int len, decoded, i, lo, hi;
    int ecs_rom_enable = -1;
    ecscable_t ec;
    uint8_t *rom_img;
    icartrom_t the_icart;

    if (argc < 2)
    {
usage:
        fprintf(stderr, "usage: ec_load [-e] foo.rom\n"
                        "\n"
                        "    -eX  Force ECS ROMs enabled/disabled\n"
                        "         (Default is \"no change\")\n"
                        );
        exit(1);
    }

    if (!strncmp(argv[1], "-e", 2))
    {
        ecs_rom_enable = atoi(argv[1]+2);
        argc--;
        argv++;
        if (argc < 2) goto usage;
    }

    signal( 2, die);
    signal(15, die);

    /* First, get set up. */
    ec_init_ports(0);
    if (!(ec.port = ec_detect(0)))
    {
        fprintf(stderr, "No ECS cable\n");
        exit(1);
    }
    ec_idle(&ec);


    if (!(f = fopen(argv[1], "r")))
    {
        fprintf(stderr, "ERROR:  Could not open '%s' for reading.\n", argv[1]);
        exit(1);
    }


    fseek(f, 0, SEEK_END);
    if ((len = ftell(f)) < 0)
    {
        fprintf(stderr, "Error seeking\n");
        exit(1);
    }
    rewind(f);

    if ((rom_img = malloc(len)) == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    fread(rom_img, 1, len, f);
    fclose(f);

    icartrom_init(&the_icart);
    decoded = icartrom_decode(&the_icart, rom_img, len, 0, 1);
    free(rom_img);

    if (decoded < 0)
    {
        if (decoded < -6) decoded = -7;

        fprintf(stderr, "Decoding error: %s\n", rom_errors[-decoded]);
        exit(1);
    }

    /* Actually load the game and reset to it. */
    if (ec_reset_intv(&ec, 1) < 0)
    {
        fprintf(stderr,"failed to reset\n"); exit(1);
    }
    if (ec_ping(&ec)<0) { fprintf(stderr,"failed to ping unit\n"); exit(1); }
    if (please_die) exit(0);

    do_title(&the_icart, &ec);
    if (please_die) exit(0);
    for (i = 0, lo = hi = -1; i <= 256; i++)
    {
        int idx = i >> 5;
        int shf = i & 31;

        if (i < 256 && ((the_icart.preload[idx] >> shf) & 1))
        {
            hi = i;
            if (lo == -1) { lo = i; }
        } else
        {
            if (lo != -1)
            {
                lo <<= 8;
                hi = (hi << 8) + 0x100;
                printf("Loading range %.4X - %.4X... ", lo, hi - 1);
                fflush(stdout);
                ec_upload(&ec, lo, hi - lo, the_icart.image + lo, -1, 1);
                printf("Done.\n");
            }
            hi = lo = -1;
        }
        if (please_die) exit(0);
    }

    if (ecs_rom_enable >= 0)
        ec_ecs_roms(&ec, ecs_rom_enable);

    ec_reset_intv(&ec, 0);

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
/* ------------------------------------------------------------------------ */
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
