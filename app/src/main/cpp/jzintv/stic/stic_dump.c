/*
 * ============================================================================
 *  Title:    STIC DUMP
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This quick and dirty program generates a STIC image from a memory dump.
 *  I'm using this to post-mortem the emulator in the absence of a STIC
 *  implementation.
 * ============================================================================
 */


#include "config.h"

uint16_t backtab[240];
uint16_t grfx   [512 * 8];
uint16_t regs   [32];

uint8_t  bitmap [(160 + 16) * (96 + 16)];

/* These need _alot_ of work.  They're just rough guesses */
uint8_t  colors[16][3] =
{
#if 0
    { 0x00, 0x00, 0x00 },   /* Black            */
    { 0x00, 0x00, 0xDC },   /* Blue             */
    { 0xE8, 0x00, 0x00 },   /* Red              */
    { 0xC0, 0xD0, 0x74 },   /* Tan              */
    { 0x00, 0x84, 0x00 },   /* Dark Green       */
    { 0x00, 0xD0, 0x00 },   /* Green            */
    { 0xFC, 0xE0, 0x18 },   /* Yellow           */
    { 0xFF, 0xFF, 0xFF },   /* White            */
    { 0xD0, 0xD0, 0xD0 },   /* Gray             */
    { 0x00, 0xD0, 0xDC },   /* Cyan             */
    { 0xD0, 0x80, 0x00 },   /* Orange           */
    { 0x76, 0x77, 0x20 },   /* Brown            */
    { 0xFF, 0xD0, 0xD0 },   /* Pink             */
    { 0xD0, 0xD0, 0xFF },   /* Light Blue       */
    { 0xD0, 0xFF, 0x00 },   /* Yellow-Green     */
    { 0xD0, 0x00, 0xD0 },   /* Purple           */
#else
    { 0x01, 0x01, 0x01 },
    { 0x00, 0x91, 0xff },
    { 0xff, 0x00, 0x00 },
    { 0x8a, 0x73, 0x00 },
    { 0x00, 0x91, 0x00 },
    { 0x00, 0xf3, 0x00 },
    { 0x7d, 0xff, 0x00 },
    { 0xff, 0xff, 0xff },
    { 0x5e, 0x5e, 0x5e },
    { 0x00, 0xe7, 0x6f },
    { 0xff, 0x33, 0x00 },
    { 0x30, 0x29, 0x00 },
    { 0xff, 0x02, 0xff },
    { 0x31, 0x3d, 0xff },
    { 0x04, 0xcf, 0x00 },
    { 0xff, 0x00, 0xff },
#endif
};


void efix(uint16_t *ary, int len);

int main(int argc, char *argv[])
{
    FILE *f_i, *f_o;
    int i, j, csq = 0;
    uint32_t x, y, xx, yy, pat, fg, bg, col_stack = 0, card;
    uint32_t cs0=0, cs1=0, cs2=0, cs3=0;
    int mode;

    if (argc != 4)
    {
        fprintf(stderr,"stic_dump mode memory_image ppmfile\n");
        exit(1);
    }

    mode = argv[1][0] == 'f';

    f_i = fopen(argv[2], "rb");
    if (!f_i)
    {
        fprintf(stderr, "Couldn't open input file.\n");
        exit(1);
    }

    f_o = fopen(argv[3], "wb");
    if (!f_o)
    {
        fprintf(stderr, "Couldn't open output file.\n");
        exit(1);
    }

    fseek(f_i, 0x200 * 2, SEEK_SET);
    if (fread(backtab, 2, 0xF0, f_i) != 0xF0)
    {
        fprintf(stderr, "Error reading BACKTAB\n");
        exit(1);
    }

    fseek(f_i, 0x3000 * 2, SEEK_SET);
    if (fread(grfx, 2, 0xA00, f_i) != 0xA00)
    {
        fprintf(stderr, "Error reading GROM/GRAM\n");
        exit(1);
    }

    fseek(f_i, 0x0020 * 2, SEEK_SET);
    if (fread(regs, 2, 0x20, f_i) != 0x20)
    {
        fprintf(stderr, "Error reading registers\n");
        exit(1);
    }

    fclose(f_i);


    efix(backtab, sizeof(backtab) / 2);
    efix(grfx,    sizeof(grfx) / 2);
    efix(regs,    sizeof(regs) / 2);


    /* Set the background color */
    for (i = 0; i < (signed)sizeof(bitmap); i++)
        bitmap[i] = regs[0x0C];

    /* now draw the 'cards' */
    for (y = 0; y < 12; y++)
    {
        col_stack = 0;
        for (x = 0; x < 20; x++)
        {
            csq = 0;
            card = backtab[x + y*20];
            pat = (card >> 3) & 0x1FF;

            if (pat > 0xFF || mode == 1) pat &= 0x13F;
            pat <<= 3;

            fg = card & 0x7;

            if (mode == 0)
            {
                fg |= (card >> 9) & 0x8;
                bg = regs[0x8 + (col_stack & 3)];
                csq = ((card >> 11) & 3) == 2;
                if (!csq)
                    col_stack += (card >> 13) & 1;
                else
                {
                    cs0 = card & 7;
                    cs1 = (card >> 3) & 7;
                    cs2 = (card >> 6) & 7;
                    cs3 = ((card >> 9) & 3) | ((card >> 11) & 4);
                    if (cs0 == 7)
                        cs0 = regs[0x8 + (col_stack&3)];
                }
            } else
            {
                bg = ((card >> 8) & 0xB) | ((card >> 11) & 0x4);
            }

            if (csq)
            {
                for (yy = 8 + (y << 3), j = 0; j < 4; yy++, j++)
                    for (xx = 8 + (x << 3), i = 0; i < 4; xx++, i++)
                    {
                        bitmap[ yy      * (160+16) + xx    ] = cs0;
                        bitmap[ yy      * (160+16) + xx + 4] = cs1;
                        bitmap[(yy + 4) * (160+16) + xx    ] = cs2;
                        bitmap[(yy + 4) * (160+16) + xx + 4] = cs3;
                    }
            } else
            {
                for (yy = 8 + (y << 3), j = 0; j < 8; yy++, j++)
                    for (xx = 8 + (x << 3), i = 0; i < 8; xx++, i++)
                    {
                        bitmap[yy * (160+16) + xx] =
                            (0x80 & (grfx[pat + j] << i)) ? fg : bg;
                    }
            }
        }
    }

    /* Finally, output the PPM file */
    fprintf(f_o, "P6\n176 112 255\n");
    for (i = 0; i < (signed)sizeof(bitmap); i++)
        fprintf(f_o, "%c%c%c",
                colors[bitmap[i]][0],
                colors[bitmap[i]][1],
                colors[bitmap[i]][2]);

    fclose(f_o);

    return 0;
}



void efix(uint16_t *ary, int size)
{
#ifdef BYTE_BE
    UNUSED(ary);
    UNUSED(size);
    return;
#else
    int i;

    for (i = 0; i < size; i++)
        ary[i] = ((0xFF00 & ary[i]) >> 8) | ((0x00FF & ary[i]) << 8);
#endif
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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
