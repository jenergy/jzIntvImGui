/* ======================================================================== */
/*  Decodes a .ROM file and then dumps out information about it.            */
/* ------------------------------------------------------------------------ */
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"

/* ======================================================================== */
/*  These are errors that can be reported by the Intellicart routines.      */
/* ======================================================================== */
const char *rom_errors[] =
{
    "No Error",
    "Bad Arguments",
    "Bad ROM Header",
    "CRC-16 Error in ROM Segments",
    "Bad ROM Segment Address Range",
    "Bad ROM Fine-Address Range",
    "CRC-16 Error in Enable Tables",
    "Short File / Truncated Structure",
    "Unknown Error"
};

icartrom_t the_icart;


/* ======================================================================== */
/*  MAIN                                                                    */
/*  This is the main program.  The action happens here.                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    int show_rom = 1, decoded;

    if (argc < 2 || argc > 3)
    {
usage:
        fprintf(stderr, "usage: rom2bin [-] foo.rom\n");
        exit(1);
    }

    if (argv[1][0] == '-')
    {
        show_rom = 1;
        argc--;
        argv++;
        if (argc < 2) goto usage;
    }

    icartrom_init(&the_icart);
    decoded = icartrom_readfile(argv[1], &the_icart, 0);

    if (decoded < 0)
    {
        if (decoded < -6) decoded = -7;

        fprintf(stderr, "Decoding error: %s\n", rom_errors[-decoded]);
        exit(1);
    }

    printf("Decoded %d bytes\n", decoded);

    icb_show_summary(&the_icart);

    if (show_rom)
    {
        FILE *fb, *fc;
        char bin_name[1024], cfg_name[1024];
        char *s;
        int ofs = 0;

        strncpy(bin_name, argv[1], 1020);
        bin_name[1020] = 0;
        s = strstr(bin_name, ".rom");
        if (!s) s = bin_name + strlen(bin_name);

        strcpy(s, ".cfg");
        strcpy(cfg_name, bin_name);
        strcpy(s, ".bin");

        printf("Config file: %s\nBIN file:    %s\n", cfg_name, bin_name);
        fc = fopen(cfg_name, "wb");
        fb = fopen(bin_name, "wb");
        if (!fc || !fb)
        {
            fprintf(stderr, "Can't open output files.\n");
            exit(1);
        }

        ofs = icb_write_bincfg(fb, fc, &the_icart, ofs);

        fclose(fb);
        fclose(fc);
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
