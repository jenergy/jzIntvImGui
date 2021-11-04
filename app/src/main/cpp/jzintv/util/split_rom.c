/* ======================================================================== */
/*  SPLIT_ROM -- Split apart a 16-bit image into 8-bit images suitable for  */
/*               burning on 2732 EPROMs.                                    */
/*                                                                          */
/*  Usage:                                                                  */
/*      split_rom [flags] file0.rom [file1.rom [file2.rom [...]]] prefix    */
/*                                                                          */
/*  The output .ROM file may safely overlap one of the input ROMs if -f is  */
/*  specified.  Otherwise, the output file may not overlap any of the       */
/*  input ROMs as a safety precaution.                                      */
/*                                                                          */
/*  Flags:                                                                  */
/*      -h  --help      Usage info.                                         */
/*      -l  --license   Show the GPL.                                       */
/* ======================================================================== */
/*                    Copyright (c) 2004, Joseph Zbiciak                    */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"
#include "icart/icartfile.h"



static struct option long_opts[] =
{
    {   "help",         0,      NULL,       'h'     },
    {   "?",            0,      NULL,       '?'     },
    {   "license",      0,      NULL,       'l'     },

    {   NULL,           0,      NULL,       0       }
};

static const char *optchars = "h?l";
/*
extern char *optarg;
extern int  optind, opterr, optopt;
*/


icartrom_t icart;

#define GET_BIT(bv,i,b) do {                                    \
                            int _ = (i);                        \
                            b = ((bv)[_ >> 5] >> (_ & 31)) & 1; \
                        } while(0)


uint8_t bin_h[4096];
uint8_t bin_l[4096];


/* ======================================================================== */
/*  USAGE            -- Just give usage info and exit.                      */
/* ======================================================================== */
LOCAL void usage(void)
{
    fprintf(stderr,
                                                                          "\n"
    "SPLIT_ROM"                                                           "\n"
    "Copyright 2004, Joseph Zbiciak"                                      "\n"
                                                                          "\n"
    "This utility splits a 16-bit ROM image into 8-bit 4K segments that"  "\n"
    "are suitable for burning onto 2732 EPROMs.  This is only useful if"  "\n"
    "you have access to something like a T-Card, an Activision proto,"    "\n"
    "or a Foomboard."                                                     "\n"
                                                                          "\n"
    "Usage:"                                                              "\n"
    "    split_rom [flags] input.(rom|bin) [output_prefix]"               "\n"
                                                                          "\n"
    "Input files can be in either .ROM or .BIN+CFG formats."              "\n"
                                                                          "\n"
    "The output prefix will be prepended to each of the output files."    "\n"
    "If no output prefix is given, then split_rom will use the input file""\n"
    "name sans suffix as the prefix.  All output files are of the form:"  "\n"
                                                                          "\n"
    "    PREFIX_#h.bin        Upper 8-bit half for $#000 - $#FFF"         "\n"
    "    PREFIX_#l.bin        Lower 8-bit half for $#000 - $#FFF"         "\n"
                                                                          "\n"
    "In addition to writing the files, the program will output the 20-bit""\n"
    "2s complement checksum, similar to what most EPROM burning software" "\n"
    "generates.  This allows labeling and verifying the EPROM in the"     "\n"
    "future."                                                             "\n"
                                                                          "\n"
    "Flags:"                                                              "\n"
    "    -l  --license   License information"                             "\n"
    " -h -?  --help      This usage info"                                 "\n"
                                                                          "\n"
    );

    exit(0);
}

/* ======================================================================== */
/*  LICENSE          -- Just give license/authorship info and exit.         */
/* ======================================================================== */
LOCAL void license(void)
{
    fprintf(stderr,
                                                                          "\n"
    "SPLIT_ROM"                                                           "\n"
    "Copyright 2004, Joseph Zbiciak"                                      "\n"
                                                                          "\n"
    "This program is free software; you can redistribute it and/or modify""\n"
    "it under the terms of the GNU General Public License as published by""\n"
    "the Free Software Foundation; either version 2 of the License, or"   "\n"
    "(at your option) any later version."                                 "\n"
                                                                          "\n"
    "This program is distributed in the hope that it will be useful,"     "\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of"      "\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU"   "\n"
    "General Public License for more details."                            "\n"
                                                                          "\n"
"You should have received a copy of the GNU General Public License along" "\n"
"with this program; if not, write to the Free Software Foundation, Inc.," "\n"
"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA."             "\n"
                                                                          "\n"
    "Run \"split_rom --help\" for usage information."                     "\n"
                                                                          "\n"
    );

    exit(0);
}

#define MAX_FNAME  (1024)
#define MAX_PREFIX (MAX_FNAME - 8)
#define SUFFIX     "_%x%c.bin"

char output_prefix_buf[MAX_PREFIX];
char output_name      [MAX_FNAME];

/* ======================================================================== */
/*  MAIN             -- In The Beginning, there was MAIN, and C was with    */
/*                      CONST and VOID, and Darkness was on the face of     */
/*                      the Programmer.                                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    int c, option_idx = 0;
    char *output_prefix = NULL;
    int i;
    int sect, addr;
    FILE *f;

    /* -------------------------------------------------------------------- */
    /*  Parse command-line arguments.                                       */
    /* -------------------------------------------------------------------- */
    while ((c = getopt_long(argc, argv, optchars, long_opts, &option_idx))
            != EOF)
    {
        switch (c)
        {
            case 'h': case '?': usage();                break;
            case 'l': license();                        break;
            default:
            {
                fprintf(stderr, "Unrecognized argument: '%c'\nTry '-h'.\n", c);
                exit(1);
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Must have precisely one or two additional arguments.                */
    /* -------------------------------------------------------------------- */
    if (optind == argc)
    {
        fprintf(stderr, "ERROR:  No input file provided\n");
        exit(1);
    }
    if (optind < argc - 2)
    {
        fprintf(stderr, "ERROR:  Too many file names provided\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Initialize our main Intellicart image.  We work by merging all      */
    /*  others into this one.                                               */
    /* -------------------------------------------------------------------- */
    icartrom_init(&icart);

    /* -------------------------------------------------------------------- */
    /*  We should have one or two filenames left at this point.             */
    /*                                                                      */
    /*  Optind should point to the input file name.  If there's another     */
    /*  name after that, take it as the output ROM prefix.  Otherwise,      */
    /*  generate the prefix from the input filename.                        */
    /* -------------------------------------------------------------------- */
    if (optind < argc - 1)      /* Prefix provided? */
        output_prefix = argv[argc - 1];
    else
    {
        char *ext;

        assert(optind == argc - 1);

        output_prefix = output_prefix_buf;
        strncpy(output_prefix, argv[optind], MAX_PREFIX - 1);
        output_prefix[MAX_PREFIX - 1] = 0;
        ext = strchr(output_prefix, '.');   /* strip ALL extensions */

        if (*ext)
            *ext = '\0';
        else
            fprintf(stderr,
                    "Warning: Could not find extension(s) on '%s'\n",
                    argv[optind]);
    }

    /* -------------------------------------------------------------------- */
    /*  Read in the requested file.                                         */
    /* -------------------------------------------------------------------- */
    icart_readfile(argv[optind], &icart, 1);

    /* -------------------------------------------------------------------- */
    /*  Scan the cart, looking for PRELOAD && READ on each 4K chunk.        */
    /* -------------------------------------------------------------------- */
    for (sect = 0; sect < 16; sect++)
    {
        int preload, readable;
        int csum_h, csum_l;

        /* ---------------------------------------------------------------- */
        /*  Scan for preload/readable bits.  Take it if it has both at      */
        /*  least somewhere in this 4K range.                               */
        /* ---------------------------------------------------------------- */
        preload = readable = 0;
        for (addr = sect << 4; addr < (sect + 1) << 4; addr++)
        {
            int p, r;

            GET_BIT(icart.preload,  addr, p);  preload  += p;
            GET_BIT(icart.readable, addr, r);  readable += r;
        }

        if (!(preload && readable))
            continue;

        /* ---------------------------------------------------------------- */
        /*  Found a segment!  Split it into high/low, then write it out.    */
        /* ---------------------------------------------------------------- */
        csum_h = csum_l = 0;
        for (i = 0, addr = sect << 12; i < 0x1000; addr++, i++)
        {
            csum_h += bin_h[i] = 0xFF & (icart.image[addr] >> 8);
            csum_l += bin_l[i] = 0xFF & (icart.image[addr]     );
        }

        snprintf(output_name, MAX_FNAME, "%s" SUFFIX, output_prefix,
                 sect & 0xF, 'h');
        output_name[MAX_FNAME-1] = 0;
        printf("%s: %.5X\n", output_name, csum_h);
        if (!(f = fopen(output_name, "wb"))) { perror("fopen"); exit(1); }
        fwrite(bin_h, 1, 4096, f);
        fclose(f);

        snprintf(output_name, MAX_FNAME, "%s" SUFFIX, output_prefix,
                 sect & 0xF, 'l');
        output_name[MAX_FNAME-1] = 0;
        printf("%s: %.5X\n", output_name, csum_l);
        if (!(f = fopen(output_name, "wb"))) { perror("fopen"); exit(1); }
        fwrite(bin_l, 1, 4096, f);
        fclose(f);
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
/*                 Copyright (c) 2002-2003, Joseph Zbiciak                  */
/* ======================================================================== */
