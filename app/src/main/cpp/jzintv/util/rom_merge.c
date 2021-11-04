/* ======================================================================== */
/*  ROM_MERGE -- Merge multiple .ROM files into a single .ROM file.         */
/*                                                                          */
/*  Usage:                                                                  */
/*      rom_merge [flags] file0.rom [file1.rom [file2.rom [...]]] out.rom   */
/*                                                                          */
/*  The output .ROM file may safely overlap one of the input ROMs if -f is  */
/*  specified.  Otherwise, the output file may not overlap any of the       */
/*  input ROMs as a safety precaution.                                      */
/*                                                                          */
/*  Flags:                                                                  */
/*      -f  --force     Allow output file to overwrite an input file.       */
/*      -r  --replace   Allow later .ROMs to replace portions mapped by     */
/*                      previous .ROMs.  (Default is error.)                */
/* ======================================================================== */
/*                 Copyright (c) 2002-2017, Joseph Zbiciak                  */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"
#include "icart/icartfile.h"
#include "metadata/metadata.h"

static struct option long_opts[] =
{
    {   "force",        0,      NULL,       'f'     },
    {   "replace",      0,      NULL,       'r'     },
    {   "help",         0,      NULL,       'h'     },
    {   "?",            0,      NULL,       '?'     },
    {   "license",      0,      NULL,       'l'     },
    {   "cc3",          0,      NULL,       '3'     },
    {   "adv",          0,      NULL,       3       },
    {   "strip",        0,      NULL,       's'     },

    {   NULL,           0,      NULL,       0       }
};

static const char *optchars = "frh?l3s";

/* -- should be defined in getopt.h --
extern char *optarg;
extern int  optind, opterr, optopt;
*/

int force_overwrite = 0;
int allow_replace   = 0;

icartrom_t final_icart, temp_icart;

#define GET_BIT(bv,i,b) do {                                    \
                            int _ = (i);                        \
                            b = ((bv)[_ >> 5] >> (_ & 31)) & 1; \
                        } while(0)

/* ======================================================================== */
/*  MERGE_ICARTS     -- Given two icartrom_t's, copy the second into the    */
/*                      first.  The "replace" argument controls whether     */
/*                      the second ROM is allowed to replace segments in    */
/*                      the first.                                          */
/* ======================================================================== */
LOCAL void merge_icarts(icartrom_t *dst, icartrom_t *src, int replace,
                        int merge_metadata)
{
    uint32_t a, p, attr_src, attr_dst, b;

    /* -------------------------------------------------------------------- */
    /*  Look through 256-word pages of 'src' for preload hunks to copy to   */
    /*  'dst'.  Preload is orthogonal to memory attribute settings.         */
    /* -------------------------------------------------------------------- */
    for (p = 0; p < 256; p++)
    {
        GET_BIT(src->preload, p, b);

        if (!b)
            continue;

        a = p << 8;

        if (!replace)
        {
            GET_BIT(dst->preload, p, b);
            if (b)
            {
                fprintf(stderr, "ERROR:  [preload] hunk conflict at "
                                "$%.4X - $%.4x\n"
                                "        Use '-r' to override\n",
                                a, a + 255);
                exit(1);
            }
        }

        icartrom_addseg(dst, &src->image[a], a, 256, 0, 0);
    }

    /* -------------------------------------------------------------------- */
    /*  Now look through 256-word segments of 'src' for various memory      */
    /*  attributes and try to set them in 'dst'.  We allow non-empty flags  */
    /*  in 'src' to merge into 'dst' in the following circumstances:        */
    /*                                                                      */
    /*   -- The 'replace' flag is set, or                                   */
    /*   -- The corresponding 'dst' flags are empty, or                     */
    /*   -- The corresponding 'dst' flags are equal to the 'src' flags.     */
    /*                                                                      */
    /* -------------------------------------------------------------------- */
    for (p = 0; p < 256; p++)
    {
        a = p << 8;

        attr_src = 0;
        GET_BIT(src->readable, p, b); if (b) attr_src |= ICARTROM_READ;
        GET_BIT(src->writable, p, b); if (b) attr_src |= ICARTROM_WRITE;
        GET_BIT(src->narrow,   p, b); if (b) attr_src |= ICARTROM_NARROW;
        GET_BIT(src->dobanksw, p, b); if (b) attr_src |= ICARTROM_BANKSW;

        attr_dst = 0;
        GET_BIT(dst->readable, p, b); if (b) attr_dst |= ICARTROM_READ;
        GET_BIT(dst->writable, p, b); if (b) attr_dst |= ICARTROM_WRITE;
        GET_BIT(dst->narrow,   p, b); if (b) attr_dst |= ICARTROM_NARROW;
        GET_BIT(dst->dobanksw, p, b); if (b) attr_dst |= ICARTROM_BANKSW;

        if (!attr_src)
            continue;

        if (replace || !attr_dst || attr_dst == attr_src)
        {
            icartrom_addseg(dst, NULL, a, 256, attr_src, 0);
        } else
        {
            fprintf(stderr, "ERROR:  Cannot merge incompatible attributes on "
                            "$%.4X - $%.4X\n"
                            "        Use '-r' to override\n",
                            a, a + 255);
            exit(1);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  If we're asked to merge metadata, do so.                            */
    /* -------------------------------------------------------------------- */
    if (merge_metadata)
    {
        if (!dst->metadata)
        {
            dst->metadata = src->metadata;
            src->metadata = NULL;
        } else if (dst->metadata && src->metadata)
        {
            game_metadata_t *new_metadata =
                merge_game_metadata(dst->metadata, src->metadata);
            free_game_metadata(dst->metadata);
            dst->metadata = new_metadata;
        }
    }
}

/* ======================================================================== */
/*  USAGE            -- Just give usage info and exit.                      */
/* ======================================================================== */
LOCAL void usage(void)
{
    fprintf(stderr,
                                                                          "\n"
    "ROM_MERGE"                                                           "\n"
    "Copyright 2018, Joseph Zbiciak"                                      "\n"
                                                                          "\n"
    "Usage: \n"
    "    rom_merge [flags] file0.rom [file1.rom [file2.rom [...]]] out.rom\n"
    "\n"
    "The output .ROM file may safely overlap one of the input ROMs if -f is\n"
    "specified.  Otherwise, the output file may not overlap any of the"   "\n"
    "input ROMs as a safety precaution."                                  "\n"
                                                                          "\n"
    "Flags:"                                                              "\n"
    "    -f  --force        Allow output file to overwrite an input file.""\n"
    "    -r  --replace      Allow later .ROMs to replace portions mapped by\n"
    "                       previous .ROMs.  (Default is error.)"         "\n"
    "    -3  --cc3          Write a CC3 ROM header."                      "\n"
    "    -l  --license      License information"                          "\n"
    " -h -?  --help         This usage info"                              "\n"
    "    -s  --strip        Strip metadata from the result."              "\n"
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
    "ROM_MERGE"                                                           "\n"
    "Copyright 2018, Joseph Zbiciak"                                      "\n"
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
    "Run \"rom_merge --help\" for usage information."                     "\n"
                                                                          "\n"
    );

    exit(0);
}


/* ======================================================================== */
/*  MAIN             -- In The Beginning, there was MAIN, and C was with    */
/*                      CONST and VOID, and Darkness was on the face of     */
/*                      the Programmer.                                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    int c, option_idx = 0;
    char *output_file;
    int i;
    ictype_t icart_type = ICART;
    int merge_metadata = 1;
    int last_input;

    /* -------------------------------------------------------------------- */
    /*  Parse command-line arguments.                                       */
    /* -------------------------------------------------------------------- */
    while ((c = getopt_long(argc, argv, optchars, long_opts, &option_idx))
            != EOF)
    {
        switch (c)
        {
            case 'f': force_overwrite = 1;              break;
            case 'r': allow_replace   = 1;              break;
            case 'h': case '?': usage();                break;
            case 'l': license();                        break;
            case '3': icart_type = CC3_STD;             break;
            case  3 : icart_type = CC3_ADV;             break;
            case 's': merge_metadata = 0;               break;

            default:
            {
                fprintf(stderr, "Unrecognized argument: '%c'\n", c);
                exit(1);
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  If "rom_merge -s" with exactly one file, turn on "force_overwrite"  */
    /* -------------------------------------------------------------------- */
    last_input = argc - 2;
    if (!merge_metadata && optind == argc - 1)
    {
        last_input = argc - 1;
        force_overwrite = 1;
    }

    /* -------------------------------------------------------------------- */
    /*  Must have at least two additional arguments:  One input file and    */
    /*  one output file.                                                    */
    /*                                                                      */
    /*  Exception:  Allow "rom_merge -s foo.rom" to strip metadata.         */
    /* -------------------------------------------------------------------- */
    if (optind > last_input)
    {
        if (merge_metadata)
            fprintf(stderr, "ERROR:  Must provide at least one input file and "
                            "one output file\n");
        else
            fprintf(stderr, "ERROR:  Must provide input file name with "
                            "rom_merge -s\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Initialize our main Intellicart image.  We work by merging all      */
    /*  others into this one.                                               */
    /* -------------------------------------------------------------------- */
    icartrom_init(&final_icart);

    /* -------------------------------------------------------------------- */
    /*  Remaining arguments are .ROM image filenames.  The first N-1 names  */
    /*  are input files.  The last name is the output file.  The output     */
    /*  file is not allowed to be the same as any of the input files names  */
    /*  unless the overwrite flag is used.                                  */
    /*                                                                      */
    /*  Note that I just do a simple string compare, and so this mechanism  */
    /*  is easy to fool.  Under UNIX, I could do safer checks (eg. stat()   */
    /*  and look at device and inode number), but that's overkill, and not  */
    /*  very portable.                                                      */
    /* -------------------------------------------------------------------- */
    output_file = argv[argc - 1];

    for (i = optind; i <= last_input; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Check for accidental over write of output file.                 */
        /* ---------------------------------------------------------------- */
        if (!force_overwrite && !strcmp(argv[i], output_file))
        {
            fprintf(stderr, "ERROR:  Input file '%s' overwrites output file\n"
                            "        Use \"-f\" flag to force overwrite.\n",
                            argv[i]);
            exit(1);
        }

        /* ---------------------------------------------------------------- */
        /*  Re-initialize the temporary Intellicart.                        */
        /* ---------------------------------------------------------------- */
        icartrom_init(&temp_icart);

        /* ---------------------------------------------------------------- */
        /*  Read in the requested file.                                     */
        /* ---------------------------------------------------------------- */
        icart_readfile(argv[i], &temp_icart, 1);

        /* ---------------------------------------------------------------- */
        /*  Merge this image into our final image.                          */
        /* ---------------------------------------------------------------- */
        merge_icarts(&final_icart, &temp_icart, allow_replace, merge_metadata);
    }

    /* -------------------------------------------------------------------- */
    /*  Now, write the final file.                                          */
    /* -------------------------------------------------------------------- */
    icart_writefile(output_file, &final_icart, icart_type);

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
/*                 Copyright (c) 2002-2018, Joseph Zbiciak                  */
/* ======================================================================== */
