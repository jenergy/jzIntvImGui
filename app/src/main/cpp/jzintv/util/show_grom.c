/* ======================================================================== */
/*  SHOW_GROM -- Reads in a GROM file, and shows the pictures within it.    */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/*  ASCII OUTPUT MODE                                                       */
/*                                                                          */
/*      show_grom [-d #] [-o outfile.txt] grom.bin                          */
/*                                                                          */
/*  The -o flag specifies an output file for the dump to be written to.     */
/*  If this flag is omitted, then the output will be written directly to    */
/*  stdout.                                                                 */
/*                                                                          */
/*  The -r # flag determines how many cards are displayed per rom.  By      */
/*  default, only 8 are shown per row.  This number may vary from 1 to      */
/*  256.  Note that numbers larger than 8 will result in output that is     */
/*  wider than 80 columns.                                                  */
/*                                                                          */
/*  The -d# flag specifies the display mode for card numbers.  The default  */
/*  mode is decimal-only and is equivalent to -d1.  The flag -d2 displays   */
/*  hexadecimal, and -d3 displays both hexadecimal and decimal.             */
/*                                                                          */
/*  Each card in GROM is displayed in a giant 8 x 32 table of GROM cards.   */
/*  Each cell in the table is an 8x8 ASCII depiction of the GROM card that  */
/*  looks about like so:                                                    */
/*                                                                          */
/*                             7B   123                                     */
/*                             ..####..                                     */
/*                             .#....#.                                     */
/*                             #.#..#.#                                     */
/*                             #......#                                     */
/*                             #.#..#.#                                     */
/*                             #..##..#                                     */
/*                             .#....#.                                     */
/*                             ..####..                                     */
/*                                                                          */
/*  In the dump, '#' characters indicate pixels that are "ON", and '.'      */
/*  characters indicate pixels that are "OFF".  The card # displayed over   */
/*  the ASCII depiction is card # you'd use to display this picture on      */
/*  the display.                                                            */
/*                                                                          */
/*                                                                          */
/*  HTML OUTPUT MODE                                                        */
/*                                                                          */
/*  HTML mode is selected with the "-h" flag.  In this mode, SHOW_GROM is   */
/*  used as follows:                                                        */
/*                                                                          */
/*      show_grom -h [-d#] [-x#] [-y#] [-r#] [-f#] [-o out.html] grom.bin   */
/*                                                                          */
/*  The -o flag specifies an output file for the dump to be written to.     */
/*  If this flag is omitted, then the output will be written directly to    */
/*  stdout.                                                                 */
/*                                                                          */
/*  The -x # and -y # flags set the width and height for displaying the     */
/*  GIFs.  By default, the GIFs are displayed as 64x64 images.  Note that   */
/*  the GIFs are always generated as 8x8 images -- these flags only affect  */
/*  the HTML output.                                                        */
/*                                                                          */
/*  The -r # flag determines how many cards are displayed per rom.  By      */
/*  default, only 8 are shown per row.  This number may vary from 1 to      */
/*  256.                                                                    */
/*                                                                          */
/*  The -f # flag affects the relative font-size applied to the labels.     */
/*  The default size is 0.  Sizes from -4 to 4 are allowed.                 */
/*                                                                          */
/*  The -d# flag specifies the display mode for card numbers.  The default  */
/*  mode is decimal-only and is equivalent to -d1.  The flag -d2 displays   */
/*  hexadecimal, and -d3 displays both hexadecimal and decimal.             */
/*                                                                          */
/*  In HTML mode, SHOW_GROM outputs a single HTML file containing a         */
/*  series of tables, and a set of 256 GIF files containing the various     */
/*  GROM pictures.  Together, these produce an attractive HTML document     */
/*  showing the contents of the GROM.                                       */
/*                                                                          */
/*  The GIF files output in this mode will be named the same as the         */
/*  output filename given with the "-o" flag, except the ".html" (or        */
/*  ".htm") suffix will be replaced with "_XX.gif", where XX is the         */
/*  hexadecimal index for the GROM card.  If the original filename does     */
/*  not end in ".html", ".HTML", ".htm" or ".HTM", then the "_XX.gif"       */
/*  suffix is just tacked on the end.  If the you do not specify an         */
/*  output filename with "-o", then the GIF files will be named             */
/*  "grom_XX.gif".                                                          */
/*                                                                          */
/*  The full pathname of the output file specified by "-o" is used in       */
/*  the generated HTML.  This can cause strange things to happen, since     */
/*  the web-browser typically expects the GIF files to be specified with    */
/*  a path relative to the HTML file.  For this reason, I strongly          */
/*  encourage you to run show_grom in the directory where you want the      */
/*  output to appear.                                                       */
/*                                                                          */
/*  EXAMPLE:                                                                */
/*                                                                          */
/*      show_grom -h -o wxyz.html grom.bin                                  */
/*                                                                          */
/*  This will write an HTML file named "wxyz.html" containing the HTML      */
/*  dump of "grom.bin".  It will also write a series of files named         */
/*  "wxyz_00.gif" through "wxyz_FF.gif" which contain the individual        */
/*  pictures for each of the GROM cards.                                    */
/*                                                                          */
/*                                                                          */
/*  GIF PATENT ISSUES                                                       */
/*                                                                          */
/*  The GIF implementation in this program should NOT infringe on the       */
/*  Unisys LZW patent.  This code does NOT employ LZW compression.          */
/*  Rather, it entirely avoids compressing the data.  This is acceptible    */
/*  for small GIFs such as these.                                           */
/*                                                                          */
/*  The GIF encoder in this code outputs a syntactically valid GIF.  It     */
/*  does not, however, send any compression-specific codes other than       */
/*  "Clear dictionary" and "End of Information."  It does not construct,    */
/*  maintain, or even track the state of anything resembling an LZW         */
/*  dictionary.                                                             */
/*                                                                          */
/* ======================================================================== */


#include "config.h"

uint8_t grom[256 * 8];

/* ======================================================================== */
/*  READ_GROM -- Reads in a GROM image.  Detects whether it's a 16-bit or   */
/*               8-bit GROM image and handles accordingly.                  */
/* ======================================================================== */
LOCAL void read_grom(FILE *f)
{
    size_t len;

    /* -------------------------------------------------------------------- */
    /*  Seek to the end of the file to determine its length.                */
    /* -------------------------------------------------------------------- */
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);

    /* -------------------------------------------------------------------- */
    /*  Standard GROM image is 2048 bytes (8 * 256).                        */
    /* -------------------------------------------------------------------- */
    if (len == 2048)
    {
        if (fread(grom, 1, 2048, f) != 2048)
        {
            perror("fread()");
            fprintf(stderr, "Unable to read GROM\n");
            exit(1);
        }
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Non-standard 16-bit GROM image is 4096 bytes.                       */
    /* -------------------------------------------------------------------- */
    if (len == 4096)
    {
        int i, nz = 0;
        uint8_t temp[4096];

        if (fread(temp, 1, 4096, f) != 4096)
        {
            perror("fread()");
            fprintf(stderr, "Unable to read GROM\n");
            exit(1);
        }


        /* ---------------------------------------------------------------- */
        /*  Try to decode GROM as a big-endian ROM first.                   */
        /* ---------------------------------------------------------------- */
        for (i = 1; i < 4096; i += 2)
        {
            grom[i >> 1] = temp[i];
            nz += temp[i];
        }

        /* ---------------------------------------------------------------- */
        /*  If we didn't find any non-zero bytes, try decoding as little-   */
        /*  endian ROM next.                                                */
        /* ---------------------------------------------------------------- */
        if (nz == 0)
            for (i = 0; i < 4096; i += 2)
                grom[i >> 1] = temp[i];

        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Die if we don't recognize this as a GROM image.                     */
    /* -------------------------------------------------------------------- */
    fprintf(stderr, "ERROR:  GROM image is non-standard size (%ld bytes)\n",
            (long)len);
    exit(1);
}

/* ======================================================================== */
/*  ASCII_DUMP -- Outputs the ASCII dump of the GROM to a file.             */
/* ======================================================================== */
LOCAL void ascii_dump(FILE *f, int num_per_row, int digit_mode)
{
    int i, j, pix_row, pix_col;

    /* -------------------------------------------------------------------- */
    /*  The outermost loop steps through sets of 8 characters.              */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 256; i += num_per_row)
    {
        /* ---------------------------------------------------------------- */
        /*  Insert a spacer between rows.                                   */
        /* ---------------------------------------------------------------- */
        if (i > 0)
            fputc('\n', f);

        /* ---------------------------------------------------------------- */
        /*  Each row of 8 starts with a header giving the card #s.          */
        /* ---------------------------------------------------------------- */
        for (j = 0; j < num_per_row; j++)
            if (i + j < 256)
                switch (digit_mode)
                {
                    case 1: fprintf(f, " CARD %3d",   i + j);        break;
                    case 2: fprintf(f, " CARD  %.2X", i + j);        break;
                    case 3: fprintf(f, " %.2X   %3d", i + j, i + j); break;
                }

        fputc('\n', f);

        /* ---------------------------------------------------------------- */
        /*  Output the 8 rows of characters.                                */
        /* ---------------------------------------------------------------- */
        for (pix_row = 0; pix_row < 8; pix_row++)
        {
            for (j = 0; j < num_per_row; j++)
            {
                if (i + j > 255)
                    break;

                fputc(' ', f);
                for (pix_col = 0; pix_col < 8; pix_col++)
                {
                    int idx = 8 * (i + j) + pix_row;
                    fputc((grom[idx] << pix_col) & 0x80 ? '#' : '.', f);
                }
            }
            fputc('\n', f);
        }
    }
}

/* ======================================================================== */
/*  MINI_GIF -- writes out an 8x8 1bpp GIF87a file.  Does not employ LZW    */
/*              compression (to avoid patent issues).                       */
/* ======================================================================== */
LOCAL const uint8_t mini_gif_header[] =
{
    /* GIF Header */
    'G', 'I', 'F', '8', '7', 'a', /* GIF87a signature                       */
    0x08, 0x00,                   /* Screen width  = 0x0008                 */
    0x08, 0x00,                   /* Screen height = 0x0008                 */
    0xF0,                         /* 1-bpp pixels, 8-bpp color map          */
    0x00,                         /* Background color = index #0            */
    0x00,                         /* Filler byte                            */

    /* Global Color Map */
    0xFF, 0xFF, 0xFF,             /* Index #0 == WHITE                      */
    0x00, 0x00, 0x00,             /* Index #1 == BLACK                      */

    /* Image Descriptor */
    0x2C,                         /* Image separator character              */
    0x00, 0x00,                   /* Image left    = 0x0000                 */
    0x00, 0x00,                   /* Image top     = 0x0000                 */
    0x08, 0x00,                   /* Image width   = 0x0008                 */
    0x08, 0x00,                   /* Image height  = 0x0008                 */
    0x00,                         /* Use global color, sequential, 1bpp     */

    /* Start of compressed image */
    0x02,                         /* Initial code length = 2                */
    37                            /* "Compressed" block is always 37 bytes. */
};

LOCAL void mini_gif(FILE *f, uint8_t *bitmap)
{
    uint32_t bitbuf = 0, bit;
    int     bitptr = 0;
    int     i;

    /* -------------------------------------------------------------------- */
    /*  Write the GIF87a header, colormap, and image descriptor.            */
    /* -------------------------------------------------------------------- */
    fwrite(mini_gif_header, 1, sizeof(mini_gif_header), f);

    /* -------------------------------------------------------------------- */
    /*  Scan through the bitmap and output 64 pixels.  We only output the   */
    /*  two codes "000" and "001" for the pixels.  The Clear Code is "100"  */
    /*  and the End of Information code is "101".                           */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 64; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Every 2 pixels, issue a clear code.  This keeps our code size   */
        /*  at 3 and effectively shuts down the LZW compression.            */
        /* ---------------------------------------------------------------- */
        if ((i & 1) == 0)
        {
            bitbuf |= 0x04 << bitptr;  /* Clear code = 100 */
            bitptr += 3;
        }

        /* ---------------------------------------------------------------- */
        /*  Pull out a single pixel as code 000 or 001 and insert it.       */
        /* ---------------------------------------------------------------- */
        bit     = 1 & (bitmap[i >> 3] >> (7 - (i & 7)));
        bitbuf |= bit << bitptr;
        bitptr += 3;

        /* ---------------------------------------------------------------- */
        /*  Flush out full bytes as they occur.                             */
        /* ---------------------------------------------------------------- */
        if (bitptr >= 8)
        {
            fputc(0xFF & bitbuf, f);
            bitbuf >>= 8;
            bitptr  -= 8;
        }
    }

    bitbuf |= 0x05 << bitptr;   /* End of Information code = 101 */
    bitptr += 3;

    /* -------------------------------------------------------------------- */
    /*  Flush output.                                                       */
    /* -------------------------------------------------------------------- */
    while (bitptr >= 0)
    {
        fputc(0xFF & bitbuf, f);
        bitbuf >>= 8;
        bitptr  -= 8;
    }

    /* -------------------------------------------------------------------- */
    /*  Terminate compressed data.                                          */
    /* -------------------------------------------------------------------- */
    fputc(0, f);

    /* -------------------------------------------------------------------- */
    /*  Terminate GIF.                                                      */
    /* -------------------------------------------------------------------- */
    fputc(0x3B, f);
}

const char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', 
                       '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

/* ======================================================================== */
/*  HTML_DUMP -- Outputs the HTML dump of the GROM to a file.               */
/* ======================================================================== */
LOCAL void html_dump(FILE *f, const char *filename, int x_dim, int y_dim,
                     int num_per_row, int font_size, int digit_mode)
{
    int i, j;
    char *gif_fn, *sfx_ofs;
    FILE *gif;

    /* -------------------------------------------------------------------- */
    /*  Generate the GIF filename template.                                 */
    /* -------------------------------------------------------------------- */
    if (!filename) filename = "grom";

    i = strlen(filename);
    gif_fn = (char *)malloc(i + 8);  /* Extra room for _XX.gif */

    if (!gif_fn)
    {
        fprintf(stderr, "Out of memory in html_dump.\n");
        exit(1);
    }

    strcpy(gif_fn, filename);

    if      (!strcmp(gif_fn + i - 4, ".htm" )) sfx_ofs = gif_fn + i - 4;
    else if (!strcmp(gif_fn + i - 4, ".HTM" )) sfx_ofs = gif_fn + i - 4;
    else if (!strcmp(gif_fn + i - 5, ".html")) sfx_ofs = gif_fn + i - 5;
    else if (!strcmp(gif_fn + i - 5, ".HTML")) sfx_ofs = gif_fn + i - 5;
    else                                       sfx_ofs = gif_fn + i;

    strcpy(sfx_ofs, "_XX.gif");

    if (font_size)
        fprintf(f, "<FONT SIZE=%d>", font_size);

    /* -------------------------------------------------------------------- */
    /*  The outermost loop steps through sets of 8 characters.              */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 256; i += num_per_row)
    {
        /* ---------------------------------------------------------------- */
        /*  Each row of 8 is its own <TABLE>.                               */
        /* ---------------------------------------------------------------- */
        fprintf(f, "<TABLE BORDER=1>");

        /* ---------------------------------------------------------------- */
        /*  Output the row of 8 characters.                                 */
        /* ---------------------------------------------------------------- */
        fprintf(f, "<TR>");
        for (j = 0; j < num_per_row; j++)
        {
            const int tile = i + j;
            char label[32];

            if (tile > 255)
                break;

            sfx_ofs[1] = hex[0xF & (tile >> 4)];
            sfx_ofs[2] = hex[0xF & (tile >> 0)];

            switch (digit_mode)
            {
                case 1: snprintf(label, sizeof(label), "%d", tile); break;
                case 2: snprintf(label, sizeof(label), "%.2X", tile); break;
                case 3: snprintf(label, sizeof(label), "%.2X (%d)", tile, tile);
                        break;
                default: label[0] = 0;
            }

            if (font_size == 0)
            {
                fprintf(f,
                    "<TD ALIGN=center BGCOLOR=#CCCCCC>%s<BR>"
                    "<IMG SRC=\"%s\" WIDTH=%d HEIGHT=%d></TD>",
                    label, gif_fn, x_dim, y_dim);
            } else
            {
                fprintf(f,
                    "<TD ALIGN=center BGCOLOR=#CCCCCC>"
                    "<FONT SIZE=%d>%s<BR>"
                    "<IMG SRC=\"%s\" WIDTH=%d HEIGHT=%d></TD>",
                    font_size, label, gif_fn, x_dim, y_dim);
            }

            if ((gif = fopen(gif_fn, "wb")) == NULL)
            {
                fprintf(stderr, "ERROR:  Could not open GIF file \"%s\" "
                        "for writing!\n", gif_fn);
                exit(1);
            }

            mini_gif(gif, &grom[8 * tile]);

            fclose(gif);
        }
        fprintf(f, "</TR>");

        /* ---------------------------------------------------------------- */
        /*  Close this row of 8's <TABLE>                                   */
        /* ---------------------------------------------------------------- */
        fprintf(f, "</TABLE>");
    }
}

/* ======================================================================== */
/*  USAGE -- Show program usage information.                                */
/* ======================================================================== */
LOCAL void usage(char *argv0)
{
    fprintf(stderr,
      "Usage:  %s [flags] grom.bin\n"
      "\n"
      "        Recognized flags:\n"
      "\n"
      "        -o outfile  Specify output file (writes to stdout by default)\n"
      "        -d #        Digit display mode for card #'s\n"
      "                    1  Decimal only (default)\n"
      "                    2  Hexadecimal only \n"
      "                    3  Hexadecimal and Decimal\n"
      "        -h          HTML output (default is ASCII)\n"
      "        -x #        Displayed width for GIFs       (HTML mode only)\n"
      "        -y #        Displayed height for GIFs      (HTML mode only)\n"
      "        -r #        Cards per row                  (HTML mode only)\n"
      "        -f #        Relative font size for labels  (HTML mode only)\n"
      "        grom.bin    Pathname for GROM.BIN file to show contents of\n",
      argv0);
    exit(0);
}

/* ======================================================================== */
/*  MAIN -- Where the action happens.                                       */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    FILE *fg, *fo;
    int html_output = 0;
    char *inp_fn = NULL;
    char *out_fn = NULL;
    int n = 0;
    int x_dim = 64, y_dim = 64, num_per_row = 8, font_size = 0, digit_mode = 1;

    /* -------------------------------------------------------------------- */
    /*  No args:  Show usage information.                                   */
    /* -------------------------------------------------------------------- */
    if (argc == 1)
        usage(argv[0]);

    /* -------------------------------------------------------------------- */
    /*  Parse the commandline arguments.                                    */
    /* -------------------------------------------------------------------- */
    while (argc-->1)
    {
        n++;

        if (argv[n][0] == '-')
        {
            switch(argv[n][1])
            {
                case 'h' : html_output = 1;  break;
                case 'o' :
                {
                    if (out_fn != NULL)
                    {
                        fprintf(stderr, "ERROR:  Too many output files!\n\n");
                        usage(argv[0]);
                    }

                    if (argv[n][2] != '\0')
                    {
                        out_fn = &argv[n][2];
                        break;
                    }

                    if (argc <= 1 || argv[n + 1][0] == '\0')
                    {
                        fprintf(stderr, "ERROR:  -o requires a filename\n\n");
                        usage(argv[0]);
                    }
                    out_fn = argv[++n];
                    argc--;

                    break;
                }
                case 'd' :
                case 'x' :
                case 'y' :
                case 'r' :
                case 'f' :
                {
                    int num = 0, c = argv[n][1];

                    if (argv[n][2] != '\0')
                    {
                        num = atoi(&argv[n][2]);
                    } else if (argc > 1 &&
                               (isdigit(argv[n + 1][0]) ||
                                (argv[n + 1][0] == '-' &&
                                 isdigit(argv[n + 1][1]))))
                    {
                        num = atoi(argv[++n]);
                        argc--;
                    } else
                    {
                        fprintf(stderr, "ERROR:  -%c requires a numeric"
                                " argument\n\n", c);
                        usage(argv[0]);
                    }
                    switch (c)
                    {
                        case 'd' : digit_mode = num; break;
                        case 'x' : x_dim = num; break;
                        case 'y' : y_dim = num; break;
                        case 'r' : num_per_row = num; break;
                        case 'f' : font_size = num; break;
                    }

                    break;
                }
                case '?' : usage(argv[0]); break;
                default:
                {
                    fprintf(stderr, "ERROR:  Unrecognized flag -%c\n\n",
                            argv[n][1]);
                    usage(argv[0]);
                }
            }
        } else
        {
            if (inp_fn != NULL)
            {
                fprintf(stderr, "ERROR:  Too many input files!\n\n");
                usage(argv[0]);
            }
            inp_fn = argv[n];
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Make sure we got an input file.                                     */
    /* -------------------------------------------------------------------- */
    if (inp_fn == NULL)
    {
        fprintf(stderr, "ERROR:  No input file specified!\n\n");
        usage(argv[0]);
    }

    /* -------------------------------------------------------------------- */
    /*  If we're in HTML mode, check validity of x_dim, etc.                */
    /* -------------------------------------------------------------------- */
    if (html_output)
    {
        if (x_dim < 1) { fprintf(stderr, "GIF width too small\n"); exit(1); }
        if (y_dim < 1) { fprintf(stderr, "GIF height too small\n"); exit(1); }
        if (font_size < -4 || font_size > 4)
        {
            fprintf(stderr, "Font size must be between -4 and 4.\n");
            exit(1);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Make sure number-per-row is valid.                                  */
    /* -------------------------------------------------------------------- */
    if (num_per_row < 1 || font_size > 256)
    {
        fprintf(stderr, "Number per row must be between 1 and 256.\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Make sure our digit mode is valid.                                  */
    /* -------------------------------------------------------------------- */
    if (digit_mode < 1 || digit_mode > 3)
    {
        fprintf(stderr, "ERROR:  Digit mode must be 1, 2, or 3\n");
        usage(argv[0]);
    }

    /* -------------------------------------------------------------------- */
    /*  Read in the GROM image.                                             */
    /* -------------------------------------------------------------------- */
    if ((fg = fopen(inp_fn, "rb")) == NULL)
    {
        fprintf(stderr, "ERROR:  Could not open GROM image '%s' for reading\n",
                inp_fn);
        exit(1);
    }

    read_grom(fg);
    fclose(fg);

    /* -------------------------------------------------------------------- */
    /*  Open our output file if one was given, otherwise default to stdout. */
    /* -------------------------------------------------------------------- */
    if (out_fn)
    {
        if ((fo = fopen(out_fn, "w")) == NULL)
        {
            fprintf(stderr, "ERROR:  Could not open '%s' for writing\n",
                    out_fn);
            exit(1);
        }
    } else
    {
        fo = stdout;
    }

    /* -------------------------------------------------------------------- */
    /*  Dump the GROM to the output file in the requested format (HTML or   */
    /*  ASCII).                                                             */
    /* -------------------------------------------------------------------- */
    if (html_output) html_dump(fo, out_fn, x_dim, y_dim, num_per_row,
                               font_size, digit_mode);
    else             ascii_dump(fo, num_per_row, digit_mode);

    fclose(fo);

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
/*                 Copyright (c) 2002-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
