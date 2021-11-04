#ifndef GFX_PALETTE_H_
#define GFX_PALETTE_H_

/* ======================================================================== */
/*  Base palette datatypes.                                                 */
/* ======================================================================== */
typedef struct palette_t
{
    uint8_t color[16][3];
} palette_t;

typedef struct palette_info_t
{
    const char *name;
    const char *desc;
    const palette_t *palette;
} palette_info_t;

/* ======================================================================== */
/*  INTV_PALETTES    -- List of available palettes.                         */
/*  UTIL_PALETTE     -- Utlity palette used by gfx_t.                       */
/* ======================================================================== */
extern const palette_info_t intv_palettes[];   /* null-terminated */

extern const palette_t util_palette;

/* ======================================================================== */
/*  PALETTE_GET_BY_NAME  -- Get a palette by name.                          */
/*  PALETTE_GET_BY_IDX   -- Get a palette by index.                         */
/*                                                                          */
/*  Returns -1 on failure, 0 on success.                                    */
/* ======================================================================== */
int palette_get_by_name(const char *const name, palette_t *const palette);
int palette_get_by_idx(const int idx, palette_t *const palette);

/* ======================================================================== */
/*  PALETTE_LOAD_FILE -- Load an alternate palette from a file.             */
/*                       Returns 0 on success, non-zero otherwise.          */
/* ======================================================================== */
struct LZFILE;  /* forward decl */
int palette_load_file(struct LZFILE *const f, palette_t *const palette);

/* ======================================================================== */
/*  Names for the default palettes used by gfx_t.                           */
/* ======================================================================== */
enum
{
    PALETTE_DEFAULT_NTSC = 0,
    PALETTE_DEFAULT_PAL,
    PALETTE_JZINTV_OLD_1,
    PALETTE_JZINTV_OLD_2,
    
    PALETTE_IDX_MAX
};

#endif  // GFX_PALETTE_H_
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
