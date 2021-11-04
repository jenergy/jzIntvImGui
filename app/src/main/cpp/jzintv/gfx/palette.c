#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gfx/palette.h"
#include "lzoe/lzoe.h"

/* ======================================================================== */
/*  DEFAULT_NTSC         -- The STIC palette on NTSC machines.              */
/*                                                                          */
/*  This is a compromise palette derived with the assistance of a capture   */
/*  card and an eyeball.  NTSC has some out-of-gamut colors that are        */
/*  difficult to reproduce faithfully on computer displays.                 */
/* ======================================================================== */
static const palette_t default_ntsc =
{ {
    /* -------------------------------------------------------------------- */
    /*  This is an NTSC palette derived from a combination of screen cap    */
    /*  and eyeballing.  It's somewhat better in practice so far.           */
    /* -------------------------------------------------------------------- */
    { 0x00, 0x00, 0x00 },
    { 0x14, 0x38, 0xF7 },
    { 0xE3, 0x5B, 0x0E },
    { 0xCB, 0xF1, 0x68 },
    { 0x00, 0x94, 0x28 },
    { 0x07, 0xC2, 0x00 },
    { 0xFF, 0xFF, 0x01 },
    { 0xFF, 0xFF, 0xFF },
    { 0xC8, 0xC8, 0xC8 },
    { 0x23, 0xCE, 0xC3 },
    { 0xFD, 0x99, 0x18 },
    { 0x3A, 0x8A, 0x00 },
    { 0xF0, 0x46, 0x3C },
    { 0xD3, 0x83, 0xFF },
    { 0x48, 0xF6, 0x01 },
    { 0xB8, 0x11, 0x78 },
} };

/* ======================================================================== */
/*  DEFAULT_PAL          -- The STIC palette on NTSC machines.              */
/*                                                                          */
/*  This is a PAL pallete (used in PAL mode) derived with help from Oscar   */
/*  Toledo G. (nanochess).                                                  */
/* ======================================================================== */
static const palette_t default_pal =
{ {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x75, 0xFF },
    { 0xFF, 0x4C, 0x39 },
    { 0xD1, 0xB9, 0x51 },
    { 0x09, 0xB9, 0x00 },
    { 0x30, 0xDF, 0x10 },
    { 0xFF, 0xE5, 0x01 },
    { 0xFF, 0xFF, 0xFF },
    { 0x8C, 0x8C, 0x8C },
    { 0x28, 0xE5, 0xC0 },
    { 0xFF, 0xA0, 0x2E },
    { 0x64, 0x67, 0x00 },
    { 0xFF, 0x29, 0xFF },
    { 0x8C, 0x8F, 0xFF },
    { 0x7C, 0xED, 0x00 },
    { 0xC4, 0x2B, 0xFC },
} };

/* ======================================================================== */
/*  JZINTV_OLD_1         -- An old jzIntv palette.                          */
/*                                                                          */
/*  I generated these colors by directly eyeballing my television while it  */
/*  was next to my computer monitor.  I then tweaked each color until it    */
/*  was pretty close to my TV.  Bear in mind that NTSC (said to mean        */
/*  "Never The Same Color") is highly susceptible to Tint/Brightness/       */
/*  Contrast settings, so your mileage may vary with this particular        */
/*  palette setting.                                                        */
/* ======================================================================== */
static const palette_t jzintv_old_1 =
{ {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x2D, 0xFF },
    { 0xFF, 0x3D, 0x10 },
    { 0xC9, 0xCF, 0xAB },
    { 0x38, 0x6B, 0x3F },
    { 0x00, 0xA7, 0x56 },
    { 0xFA, 0xEA, 0x50 },
    { 0xFF, 0xFC, 0xFF },
    { 0xBD, 0xAC, 0xC8 },
    { 0x24, 0xB8, 0xFF },
    { 0xFF, 0xB4, 0x1F },
    { 0x54, 0x6E, 0x00 },
    { 0xFF, 0x4E, 0x57 },
    { 0xA4, 0x96, 0xFF },
    { 0x75, 0xCC, 0x80 },
    { 0xB5, 0x1A, 0x58 },
} };

/* ======================================================================== */
/*  JZINTV_OLD_2         -- An even older jzIntv palette.                   */
/* ======================================================================== */
static const palette_t jzintv_old_2 =
{ {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x16, 0xFE },
    { 0xFE, 0x5A, 0x02 },
    { 0xC0, 0xD8, 0x63 },
    { 0x00, 0xB7, 0x00 },
    { 0x00, 0xE6, 0x18 },
    { 0xF0, 0xFF, 0x56 },
    { 0xFD, 0xFD, 0xFF },
    { 0xBF, 0xC3, 0xCA },
    { 0x00, 0xC8, 0xF0 },
    { 0xFC, 0xCA, 0x23 },
    { 0x20, 0x80, 0x00 },
    { 0xFF, 0x5E, 0xA8 },
    { 0xA0, 0x90, 0xFF },
    { 0x90, 0xFF, 0x60 },
    { 0xC0, 0x10, 0x7A },
} };

/* ======================================================================== */
/*  UTIL_PALETTE         -- Utility palette used by gfx_t.                  */
/* ======================================================================== */
const palette_t util_palette =
{ {
    /* -------------------------------------------------------------------- */
    /*  This pink color is used for drawing rectangles around sprites.      */
    /*  It's a temporary hack.                                              */
    /* -------------------------------------------------------------------- */
    { 0xFF, 0x80, 0x80 },
    /* -------------------------------------------------------------------- */
    /*  Grey shades used for misc tasks (not currently used).               */
    /* -------------------------------------------------------------------- */
    { 0x11, 0x11, 0x11 },
    { 0x22, 0x22, 0x22 },
    { 0x33, 0x33, 0x33 },
    { 0x44, 0x44, 0x44 },
    { 0x55, 0x55, 0x55 },
    { 0x66, 0x66, 0x66 },
    { 0x77, 0x77, 0x77 },
    { 0x88, 0x88, 0x88 },
    { 0x99, 0x99, 0x99 },
    { 0xAA, 0xAA, 0xAA },
    { 0xBB, 0xBB, 0xBB },
    { 0xCC, 0xCC, 0xCC },
    { 0xDD, 0xDD, 0xDD },
    { 0xEE, 0xEE, 0xEE },
    { 0xFF, 0xFF, 0xFF }
} };

/* ======================================================================== */
/*  INTV_PALETTES    -- List of available palettes.                         */
/* ======================================================================== */
const palette_info_t intv_palettes[] =  /* null-terminated */
{
    {   "default_ntsc", "The default NTSC palette", &default_ntsc   },
    {   "default_pal",  "The default PAL palette",  &default_pal    },
    {   "jzintv_old_1", "Pre-2018 jzIntv palette",  &jzintv_old_1   },
    {   "jzintv_old_2", "Pre-20?? jzIntv palette",  &jzintv_old_2   },
    {   NULL,           NULL,                       NULL            }
};

/* ======================================================================== */
/*  PALETTE_GET_BY_NAME  -- Get a palette by name.                          */
/* ======================================================================== */
int palette_get_by_name(const char *const name, palette_t *const palette)
{
    for (int i = 0; intv_palettes[i].name; ++i)
    {
        if (!strcmp(intv_palettes[i].name, name))
        {
            *palette = *intv_palettes[i].palette;
            return 0;
        }
    }

    return -1;
}

/* ======================================================================== */
/*  PALETTE_GET_BY_IDX   -- Get a palette by index.                         */
/* ======================================================================== */
int palette_get_by_idx(const int idx, palette_t *const palette)
{
    if (idx < 0 || idx >= PALETTE_IDX_MAX) return -1;
    *palette = *intv_palettes[idx].palette;
    return 0;
}

/* ======================================================================== */
/*  PALETTE_LOAD_FILE -- Load an alternate palette from a file.             */
/* ======================================================================== */
int palette_load_file(LZFILE *const f, palette_t *const palette)
{
    int i;
    unsigned r, g, b;
    char buf[256], *s1, *s2, *const end = buf + sizeof(buf);

    /* -------------------------------------------------------------------- */
    /*  Format:                                                             */
    /*   -- 16 non-empty lines, one for each of the 16 INTV colors.         */
    /*   -- Empty lines ignored.                                            */
    /*   -- Anything after a ';' ignored                                    */
    /*   -- Each line must either have:                                     */
    /*       #rrggbb        HTML-style hexadecimal color triple             */
    /*       dec dec dec    Three decimal numbers separated by spaces       */
    /*   -- Lines must be less than 256 characters.                         */
    /* -------------------------------------------------------------------- */
    i = 0;
    while (i < 16 && lzoe_fgets(buf, sizeof(buf), f) != NULL)
    {
        int prev_was_ws = 1, curr_is_ws;

        s1 = s2 = buf;

        while (s1 != end && *s1 && *s1 != ';')
        {
            curr_is_ws = isspace(*s1);

            if (prev_was_ws && curr_is_ws)
            {
                s1++;
                continue;
            }
            *s2++ = *s1++;
            prev_was_ws = curr_is_ws;
        }
        *s2 = 0;

        if (s2 == buf)
            continue;

        if (buf[0] == '#') sscanf(buf + 1, "%2x%2x%2x", &r, &g, &b);
        else               sscanf(buf,     "%d %d %d",  &r, &g, &b);

        r &= 0xFF;
        g &= 0xFF;
        b &= 0xFF;

        palette->color[i][0] = r;
        palette->color[i][1] = g;
        palette->color[i][2] = b;
        i++;
    }

    return i != 16;
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
/*                   Copyright (c) 2019, Joseph Zbiciak                     */
/* ======================================================================== */
