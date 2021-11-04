/* ======================================================================== */
/*  Takes a BIN (and optional CFG) and generates a .ROM from it.            */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"
#include "icart/icarttag.h"
#include "lzoe/lzoe.h"
#include "metadata/metadata.h"
#include "metadata/cfgvar_metadata.h"
#include "file/file.h"

icartrom_t the_icart;


/* ======================================================================== */
/*  MAIN                                                                    */
/*  This is the main program.  The action happens here.                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    char bin_fn[1024], cfg_fn[1024], rom_fn[1024];
    int fn_len;
    ictype_t icart_type = ICART;
    int add_meta = 1;

    if (argc > 2)
    {
        argc--;

        if      (!strcmp(argv[1], "--cc3")) { icart_type = CC3_STD; argv++; }
        else if (!strcmp(argv[1], "-3"))    { icart_type = CC3_STD; argv++; }
        else if (!strcmp(argv[1], "--adv")) { icart_type = CC3_ADV; argv++; }
        else if (!strcmp(argv[1], "--metadata"))    { add_meta = 1; argv++; }
        else if (!strcmp(argv[1], "--no-metadata")) { add_meta = 0; argv++; }
        else
            argc++;
    }

    if (argc != 2)
    {
        fprintf(stderr, "usage: bin2rom [--cc3] foo[.bin]\n");
        exit(1);
    }
    /* -------------------------------------------------------------------- */
    /*  Initialize the icartrom.                                            */
    /* -------------------------------------------------------------------- */
    icartrom_init(&the_icart);

    /* -------------------------------------------------------------------- */
    /*  Generate .BIN, .CFG, and .ROM filenames from argument filename.     */
    /*  If the argument lacks a .BIN extension, add one.                    */
    /* -------------------------------------------------------------------- */
    strncpy(bin_fn, argv[1], 1019);
    bin_fn[1019] = 0;

    fn_len = strlen(bin_fn);
    if (stricmp(bin_fn + fn_len - 4, ".bin") != 0 &&
        stricmp(bin_fn + fn_len - 4, ".int") != 0 &&
        stricmp(bin_fn + fn_len - 4, ".itv") != 0 &&
        !file_exists(bin_fn))
    {
        strcpy(bin_fn + fn_len, ".bin");
        fn_len += 4;
        if (!file_exists(bin_fn))
        {
            fprintf(stderr, "Could not find '%s' or '%s'\n", argv[1], bin_fn);
            exit(1);
        }
    }

    strcpy(cfg_fn, bin_fn);
    strcpy(rom_fn, bin_fn);

    strcpy(cfg_fn + fn_len - 4, ".cfg");
    strcpy(rom_fn + fn_len - 4, ".rom");

    /* -------------------------------------------------------------------- */
    /*  Read the BIN+CFG into the icartrom_t.                               */
    /* -------------------------------------------------------------------- */
    icb_read_bincfg(bin_fn, cfg_fn, &the_icart, 1);

    /* -------------------------------------------------------------------- */
    /*  Strip metadata, if asked to do so.                                  */
    /* -------------------------------------------------------------------- */
    if (!add_meta)
    {
        free_game_metadata(the_icart.metadata);
        the_icart.metadata = NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Finally, generate the ROM file and write it out.                    */
    /* -------------------------------------------------------------------- */
    icartrom_writefile(rom_fn, &the_icart, icart_type);

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
