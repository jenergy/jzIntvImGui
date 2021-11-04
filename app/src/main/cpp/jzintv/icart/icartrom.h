/* ======================================================================== */
/*  INTELLICART ROM manipulation routines.              J. Zbiciak, 2001    */
/*                                                                          */
/*  These routines are intended for reading and writing Intellicart ROM     */
/*  images.  Portions of this code are based on Chad Schell's own routines  */
/*  for generating Intellicart ROM images.                                  */
/* ======================================================================== */
#ifndef ICARTROM_H_
#define ICARTROM_H_ 1

#define ICARTROM_READ      (0x01)
#define ICARTROM_WRITE     (0x02)
#define ICARTROM_NARROW    (0x04)   /* Ignored by Intellicart.          */
#define ICARTROM_BANKSW    (0x08)
#define ICARTROM_PRELOAD   (0x10)   /* Not sent to Intellicart.         */

struct game_metadata_t;             /* forward declaration.             */

typedef struct icartrom_t
{
    uint32_t preload  [256 >> 5];   /* Pages to download to Intellicart */
    uint32_t readable [256 >> 5];   /* Pages readable by Intellivision  */
    uint32_t narrow   [256 >> 5];   /* Pages writable by Intellivision  */
    uint32_t writable [256 >> 5];   /* Pages with 8-bit memory.         */
    uint32_t dobanksw [256 >> 5];   /* Pages with bankswitched memory.  */
    uint16_t image    [65536];      /* Intellicart memory image.        */
    
    /*  Optional decoded metadata from ICARTTAGs. (May be NULL.)            */
    struct game_metadata_t *metadata;
} icartrom_t;

typedef enum ictype_t { ICART, CC3_STD, CC3_ADV } ictype_t;

enum icartrom_errors
{
    IC_BAD_ARGS             = -1,
    IC_BAD_ROM_HEADER       = -2,
    IC_CRC_ERROR_ROM_DATA   = -3,
    IC_BAD_SEG_ADDR_RANGE   = -4,   /* lo > hi */
    IC_BAD_FINE_ADDR        = -5,
    IC_CRC_ERROR_ENABLE_TBL = -6,
    IC_SHORT_FILE           = -7 
};

/* ======================================================================== */
/*  ICARTROM_NEW    -- Allocate a new icartrom_t initialized to defaults.   */
/* ======================================================================== */
icartrom_t *icartrom_new(void);

/* ======================================================================== */
/*  ICARTROM_DELETE -- Deallocate an icartrom_t.                            */
/* ======================================================================== */
void icartrom_delete(icartrom_t *const rom);

/* ======================================================================== */
/*  ICARTROM_INIT   -- Initialize an Intellicart image to defaults.         */
/* ======================================================================== */
void icartrom_init(icartrom_t *const rom);

/* ======================================================================== */
/*  ICARTROM_ADDSEG -- Adds a segment to an Intellcart ROM structure.       */
/* ======================================================================== */
int icartrom_addseg
(
    icartrom_t *const rom,      /* Intellicart ROM structure.               */
    uint16_t   *const data,     /* Data to insert into ROM.  May be NULL.   */
    uint32_t    const addr,     /* Address to insert the data at.           */
    uint32_t    const len,      /* Length of the data to insert.            */
    uint8_t           set_attr, /* Attributes to set (read, write, banksw). */
    uint8_t           clr_attr  /* Attributes to clear.                     */
);

/* ======================================================================== */
/*  ICARTROM_GENROM -- Generate a .ROM image from an icartrom_t.            */
/* ======================================================================== */
uint8_t *icartrom_genrom(icartrom_t *const rom, uint32_t *const rom_size,
                         const ictype_t type);

/* ======================================================================== */
/*  ICARTROM_DECODE -- Decode a .ROM image into an icartrom_t.              */
/* ======================================================================== */
int icartrom_decode
(
    icartrom_t      *const rom,
    const uint8_t   *const rom_img,
    int              const length,
    int              const ignore_crc,
    int              const ignore_metadata
);

/* ======================================================================== */
/*  ICARTROM_READFILE -- Reads a file into an icartrom_t.                   */
/* ======================================================================== */
int icartrom_readfile(const char *const fname, icartrom_t *const the_icart,
                      const int silent);

/* ======================================================================== */
/*  ICARTROM_WRITEFILE -- Writes a file from an icartrom_t.                 */
/* ======================================================================== */
uint32_t icartrom_writefile(const char *const fname,
                            icartrom_t *const the_icart, const ictype_t type);

#endif
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
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
