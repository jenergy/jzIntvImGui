/* ======================================================================== */
/*  INTELLICART ROM File I/O wrapper                    J. Zbiciak, 2003    */
/*                                                                          */
/*  This is a pretty simple "readfile"/"writefile" wrapper that handles     */
/*  reading and writing .ROM and .BIN+CFG files.                            */
/*                                                                          */
/*  The readfile wrapper implements the following rules:                    */
/*                                                                          */
/*   -- If the extension '.rom' or '.cc3' is given, try to read the file    */
/*      as a .ROM format file.                                              */
/*                                                                          */
/*   -- If the extension '.bin' is given, try to read the file as a .BIN    */
/*      + .CFG file pair.                                                   */
/*                                                                          */
/*   -- If neither of the above succeeds, try appending '.rom' and read     */
/*      the file as a .ROM file.                                            */
/*                                                                          */
/*   -- If none of those succeed, try appending '.bin' and read the file    */
/*      as a .BIN+CFG file.                                                 */
/*                                                                          */
/*  The writefile wrapper implements the following rules:                   */
/*                                                                          */
/*   -- If the extension '.rom' or '.cc3' is given, write a .ROM file.      */
/*   -- If the extension '.bin' is given, write a .BIN and .CFG file.       */
/*   -- If no extension is given, write all three variants.                 */
/*                                                                          */
/*  No check is made to ensure 'type' is consistent with the extension,     */
/*  except when type == ICART and the extension is .CC3, in which case      */
/*  the type silently gets changed to CC3_STD.                              */
/* ======================================================================== */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "config.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"
#include "icart/icartfile.h"


/* ======================================================================== */
/*  ICART_WRITEFILE -- Write combination of BIN+CFG or ROM from an icart.   */
/* ======================================================================== */
void icart_writefile(const char *fname, icartrom_t *icart, ictype_t type)
{
    char *bin_fn = NULL, *cfg_fn = NULL, *rom_fn = NULL;
    int name_len = strlen(fname);
    const char *ext = strrchr(fname, '.');  /* look for an extension */

    /* -------------------------------------------------------------------- */
    /*  Is it .ROM or .CC3?                                                 */
    /* -------------------------------------------------------------------- */
    if (ext && stricmp(ext, ".rom")==0)
    {
        rom_fn = strdup(fname);
    } else
    if (ext && stricmp(ext, ".cc3")==0)
    {
        rom_fn = strdup(fname);

        /*  Silent override:  Don't write CC3 in Intellicart format.        */
        if (type == ICART)
            type = CC3_STD;
    } else
    /* -------------------------------------------------------------------- */
    /*  Is it .BIN?                                                         */
    /* -------------------------------------------------------------------- */
    if (ext && stricmp(ext, ".bin")==0)
    {
        char *s;

        bin_fn = strdup(fname);
        cfg_fn = strdup(fname);
        if (!cfg_fn)
        {
            fprintf(stderr, "icart_writefile: Out of memory\n");
            exit(1);
        }

        s = cfg_fn + name_len - 4;
        strcpy(s, ".cfg");
    } else
    /* -------------------------------------------------------------------- */
    /*  Neither?  Write all three.                                          */
    /* -------------------------------------------------------------------- */
    {
        /* no extension or unknown extension */
        bin_fn = CALLOC(char, 3*(name_len + 5));
        if (!bin_fn)
        {
            fprintf(stderr, "icart_writefile: Out of memory\n");
            exit(1);
        }
        cfg_fn = bin_fn + name_len + 5;
        rom_fn = cfg_fn + name_len + 5;

        snprintf(bin_fn, name_len + 5, "%s.bin", fname);
        snprintf(cfg_fn, name_len + 5, "%s.cfg", fname);
        snprintf(rom_fn, name_len + 5, "%s.rom", fname);
    }

    /* -------------------------------------------------------------------- */
    /*  If we found a .ROM name, write a .ROM file.                         */
    /* -------------------------------------------------------------------- */
    if (rom_fn)
    {
        icartrom_writefile(rom_fn, icart, type);
    }

    /* -------------------------------------------------------------------- */
    /*  If we found a .BIN name, write a .BIN and CFG file.                 */
    /* -------------------------------------------------------------------- */
    if (bin_fn)
    {
        icb_writefile(bin_fn, cfg_fn, icart);
    }

    if (rom_fn) free(rom_fn);
    if (bin_fn) free(bin_fn);
    if (cfg_fn) free(cfg_fn);
}


/* ======================================================================== */
/*  ICART_READFILE -- Make a best effort, trying to read a ROM/BIN+CFG.     */
/* ======================================================================== */
void icart_readfile(const char *fname, icartrom_t *icart, int loud)
{
    char *rom1_fn = NULL, *bin1_fn = NULL, *cfg1_fn = NULL;
    char *rom2_fn = NULL, *bin2_fn = NULL, *cfg2_fn = NULL;
    char *rom3_fn = NULL;
    int name_len = strlen(fname);
    const char *ext = strrchr(fname, '.');  /* look for an extension */
    FILE *f;

    /* -------------------------------------------------------------------- */
    /*  Is it .ROM?                                                         */
    /* -------------------------------------------------------------------- */
    if (ext && (stricmp(ext, ".rom")==0 ||
                stricmp(ext, ".cc3")==0))
    {
        rom1_fn = strdup(fname);
    }

    /* -------------------------------------------------------------------- */
    /*  Is it .BIN?                                                         */
    /* -------------------------------------------------------------------- */
    if (ext && stricmp(ext, ".bin")==0)
    {
        char *s;

        bin1_fn = strdup(fname);
        cfg1_fn = strdup(fname);
        if (!cfg1_fn)
        {
            fprintf(stderr, "icart_readfile: Out of memory\n");
            exit(1);
        }

        s = cfg1_fn + name_len - 4;
        strcpy(s, ".cfg");
    }

    /* -------------------------------------------------------------------- */
    /*  In case those fail, have a backup plan.                             */
    /* -------------------------------------------------------------------- */
    {
        /* no extension or unknown extension */
        bin2_fn = CALLOC(char, 4*(name_len + 5));
        if (!bin2_fn)
        {
            fprintf(stderr, "icart_readfile: Out of memory\n");
            exit(1);
        }
        cfg2_fn = bin2_fn + name_len + 5;
        rom2_fn = cfg2_fn + name_len + 5;
        rom3_fn = rom2_fn + name_len + 5;

        snprintf(bin2_fn, name_len + 5, "%s.bin", fname);
        snprintf(cfg2_fn, name_len + 5, "%s.cfg", fname);
        snprintf(rom2_fn, name_len + 5, "%s.rom", fname);
        snprintf(rom3_fn, name_len + 5, "%s.cc3", fname);
    }

    /* -------------------------------------------------------------------- */
    /*  Now try out all of our options.                                     */
    /* -------------------------------------------------------------------- */
    if (rom1_fn && (f = fopen(rom1_fn, "rb")))
    {
        fclose(f);
        icartrom_readfile(rom1_fn, icart, 0);
        goto done;
    }

    if (bin1_fn && (f = fopen(bin1_fn, "rb")))
    {
        fclose(f);
        icb_read_bincfg(bin1_fn, cfg1_fn, icart, loud);
        goto done;
    }

    if (rom2_fn && (f = fopen(rom2_fn, "rb")))
    {
        fclose(f);
        icartrom_readfile(rom2_fn, icart, 0);
        goto done;
    }

    if (rom3_fn && (f = fopen(rom3_fn, "rb")))
    {
        fclose(f);
        icartrom_readfile(rom3_fn, icart, 0);
        goto done;
    }

    if (bin2_fn && (f = fopen(bin2_fn, "rb")))
    {
        fclose(f);
        icb_read_bincfg(bin2_fn, cfg2_fn, icart, loud);
        goto done;
    }

    fprintf(stderr, "ERROR: Could not read a .ROM or .BIN for '%s'\n", fname);
    exit(1);

done:
    if (rom1_fn) free(rom1_fn);
    if (bin1_fn) free(bin1_fn);
    if (bin2_fn) free(bin2_fn);
    if (cfg1_fn) free(cfg1_fn);

    return;
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
/*                 Copyright (c) 2003-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
