/* ======================================================================== */
/*  INTELLICART ROM manipulation routines.              J. Zbiciak, 2001    */
/*                                                                          */
/*  These routines are intended for reading and writing Intellicart ROM     */
/*  images.  Portions of this code are based on Chad Schell's own routines  */
/*  for generating Intellicart ROM images.                                  */
/* ======================================================================== */

#include <stdio.h>
#include <assert.h>
#include "config.h"
#include "icart/icartrom.h"
#include "icart/icarttag.h"
#include "lzoe/lzoe.h"
#include "metadata/metadata.h"
#include "metadata/icarttag_metadata.h"
#include "misc/crc16.h"


#define SET_BIT(bv,idx) do {                                    \
                            unsigned int _ = (idx);             \
                            (bv)[_ >> 5] |= 1u << (_ & 31);     \
                        } while(0)

#define CLR_BIT(bv,idx) do {                                    \
                            unsigned int _ = (idx);             \
                            (bv)[_ >> 5] &= ~(1u << (_ & 31));  \
                        } while(0)

#define GET_BIT(bv,i,b) do {                                    \
                            unsigned int _ = (i);               \
                            b = ((bv)[_ >> 5] >> (_ & 31)) & 1; \
                        } while(0)


/* ======================================================================== */
/*  ICARTROM_NEW    -- Allocate and initialize an Intellicart image.        */
/* ======================================================================== */
icartrom_t *icartrom_new(void)
{
    return CALLOC(icartrom_t, 1);
}

/* ======================================================================== */
/*  ICARTROM_DELETE -- Free an icartrom_t.                                  */
/* ======================================================================== */
void icartrom_delete(icartrom_t *const rom)
{
    if (!rom) return;

    if (rom->metadata)
        free_game_metadata(rom->metadata);

    free(rom);
}

/* ======================================================================== */
/*  ICARTROM_INIT   -- Initialize an Intellicart image to defaults.         */
/* ======================================================================== */
void icartrom_init(icartrom_t *const rom)
{
    if (rom) memset(rom, 0, sizeof(icartrom_t));
}

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
    uint8_t           clr_attr  /* Attributes to clear                      */
)
{
/*  jzp_printf("ADDSEG: addr %.4X len %.4X set %.2X clr %.2X\n", addr, len, set_attr, clr_attr);*/

    /* -------------------------------------------------------------------- */
    /*  Sanity checks:                                                      */
    /* -------------------------------------------------------------------- */
    if (!rom)                   return -1;  /* Valid ROM pointer?           */
    if (addr + len > 0x10000)   return -1;  /* addr + len doesn't wrap?     */

    /* -------------------------------------------------------------------- */
    /*  Next, if any actual data was provided, memcpy() it into the ICart   */
    /* -------------------------------------------------------------------- */
    if (data)
    {
        set_attr |=  ICARTROM_PRELOAD; /* Force these pages to be preloaded */
        clr_attr &= ~ICARTROM_PRELOAD; /* Force these pages to be preloaded */
        memcpy(rom->image + addr, data, len * sizeof(uint16_t));
    }

    /* -------------------------------------------------------------------- */
    /*  Now, on the range specified, update the various attributes.         */
    /* -------------------------------------------------------------------- */
    for (uint32_t sa = addr; sa < addr + len; sa += 256)
    {
        const uint32_t sp = sa >> 8;
        if (set_attr & ICARTROM_PRELOAD) SET_BIT(rom->preload , sp);
        if (set_attr & ICARTROM_READ   ) SET_BIT(rom->readable, sp);
        if (set_attr & ICARTROM_WRITE  ) SET_BIT(rom->writable, sp);
        if (set_attr & ICARTROM_NARROW ) SET_BIT(rom->narrow  , sp);
        if (set_attr & ICARTROM_BANKSW ) SET_BIT(rom->dobanksw, sp);

        if (clr_attr & ICARTROM_PRELOAD) CLR_BIT(rom->preload , sp);
        if (clr_attr & ICARTROM_READ   ) CLR_BIT(rom->readable, sp);
        if (clr_attr & ICARTROM_WRITE  ) CLR_BIT(rom->writable, sp);
        if (clr_attr & ICARTROM_NARROW ) CLR_BIT(rom->narrow  , sp);
        if (clr_attr & ICARTROM_BANKSW ) CLR_BIT(rom->dobanksw, sp);
/*      jzp_printf("ATTR: set %.2X clr %.2X on %.2X00\n", set_attr,clr_attr,sp);*/

        const uint32_t ea = (sa + 255 > addr + len ? addr + len : sa + 255) - 1;
        const uint32_t ep = ea >> 8;
        if (set_attr & ICARTROM_PRELOAD) SET_BIT(rom->preload , ep);
        if (set_attr & ICARTROM_READ   ) SET_BIT(rom->readable, ep);
        if (set_attr & ICARTROM_WRITE  ) SET_BIT(rom->writable, ep);
        if (set_attr & ICARTROM_NARROW ) SET_BIT(rom->narrow  , ep);
        if (set_attr & ICARTROM_BANKSW ) SET_BIT(rom->dobanksw, ep);

        if (clr_attr & ICARTROM_PRELOAD) CLR_BIT(rom->preload , ep);
        if (clr_attr & ICARTROM_READ   ) CLR_BIT(rom->readable, ep);
        if (clr_attr & ICARTROM_WRITE  ) CLR_BIT(rom->writable, ep);
        if (clr_attr & ICARTROM_NARROW ) CLR_BIT(rom->narrow  , ep);
        if (clr_attr & ICARTROM_BANKSW ) CLR_BIT(rom->dobanksw, ep);
/*      jzp_printf("ATTR: set %.2X clr %.2X on %.2X00\n", set_attr,clr_attr,ep);*/
    }

    return 0; /*SUCCESS*/
}

/* ======================================================================== */
/*  ICARTROM_GENROM -- Generate a .ROM image from an icartrom_t.            */
/* ======================================================================== */
uint8_t *icartrom_genrom(icartrom_t *const rom, uint32_t *const rom_size, 
                         const ictype_t type)
{
    uint32_t size;
    uint8_t  bank_attr[32];
    uint8_t  fine_addr[32];
    uint8_t  seg_lo[128], seg_hi[128];
    int      num_seg;
    uint8_t  *rom_img;
    uint16_t crc;
    uint8_t  *md_img = NULL;
    size_t   md_size = 0;

    /* -------------------------------------------------------------------- */
    /*  Sanity checks.                                                      */
    /* -------------------------------------------------------------------- */
    if (!rom || !rom_size) return NULL;
    *rom_size = 0;

    /* -------------------------------------------------------------------- */
    /*  Scan the memory map looking for segments of memory image to dl.     */
    /* -------------------------------------------------------------------- */
    num_seg = 0;
    for (int i = 0, lo = -1, hi = -1; i <= 256; i++)
    {
        int b;
        if (i != 256) { GET_BIT(rom->preload, i, b); } else { b = 0; }
        if (b)
        {
            hi = i;                     /* extend upper end of segment  */
            if (lo == -1) { lo = i; }   /* detect start of a segment    */
        } else
        {
            if (lo >= 0)
            {
                seg_lo[num_seg] = lo;
                seg_hi[num_seg] = hi;
                num_seg++;
            }
            hi = lo = -1;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Now, build up the attribute flags and fine-address ranges.          */
    /* -------------------------------------------------------------------- */
    for (int i = 0; i < 256; i += 8)
    {
        uint8_t page[8];     /* attrs for pages within 2K bank */

        /* ---------------------------------------------------------------- */
        /*  First, generate the union of all attributes in this 2K bank.    */
        /* ---------------------------------------------------------------- */
        int b, j, attr = 0;
        for (j = 0, attr = 0; j < 8; j++)
        {
            int tmp = 0;
            GET_BIT(rom->readable, (i+j), b); if (b) tmp |= ICARTROM_READ;
            GET_BIT(rom->writable, (i+j), b); if (b) tmp |= ICARTROM_WRITE;
            GET_BIT(rom->narrow,   (i+j), b); if (b) tmp |= ICARTROM_NARROW;
            GET_BIT(rom->dobanksw, (i+j), b); if (b) tmp |= ICARTROM_BANKSW;
            attr |= tmp;
            page[j] = tmp;
        }

        /* ---------------------------------------------------------------- */
        /*  No attributes?  Go to next 2K bank.                             */
        /* ---------------------------------------------------------------- */
        if (!attr)
        {
            fine_addr[i >> 3] = 0x07;
            bank_attr[i >> 3] = 0x00;
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Next, determine the range of addresses that have any bits set.  */
        /*  Warn if some pages don't have all of the attributes that the    */
        /*  bank will end up having.                                        */
        /* ---------------------------------------------------------------- */
        int lo, hi;
        for (j = 0, lo = -1, hi = -1; j < 8; j++)
        {
            if (page[j])
            {
                if (lo == -1) lo = j;
                hi = j;

                if ((page[j] & attr) != attr)
                {
                    /* put warning here */
                }
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Store out the final fine-address range and bank attributes.     */
        /* ---------------------------------------------------------------- */
        assert (lo != -1 && hi != -1);
        fine_addr[i >> 3] = (lo << 4) | hi;
        bank_attr[i >> 3] = attr;
    }

    /* -------------------------------------------------------------------- */
    /*  Next, calculate the size of the .ROM image.  The .ROM format has    */
    /*  the following fixed and variable costs:                             */
    /*                                                                      */
    /*   -- Header:              3 bytes (fixed)                            */
    /*   -- Attribute Table:    16 bytes (fixed)                            */
    /*   -- Fine Addr Table:    32 bytes (fixed)                            */
    /*   -- Table Checksum:      2 bytes (fixed)                            */
    /*                         ------------------                           */
    /*                          53 bytes total fixed overhead               */
    /*                                                                      */
    /*   -- ROM segments:       4 + 2*num_words per segment.                */
    /* -------------------------------------------------------------------- */
    size = 53;

    for (int i = 0; i < num_seg; i++)
        size += 512 * (seg_hi[i] - seg_lo[i] + 1) + 4;

    /* -------------------------------------------------------------------- */
    /*  If we have metadata, go encode that real quick if possible, and     */
    /*  add that to our total size.                                         */
    /* -------------------------------------------------------------------- */
    if (rom->metadata)
    {
        int err = 0;

        md_img = icarttag_encode(rom->metadata, &md_size, &err);
        if (err)
        {
            if (md_img)
                free(md_img);
            return NULL;
        }

        size += md_size;
    }

    *rom_size = size;   /* Report the size to the caller.                   */

    /* -------------------------------------------------------------------- */
    /*  Allocate a hunk of memory for the .ROM file.                        */
    /* -------------------------------------------------------------------- */
    if ((rom_img = CALLOC(uint8_t, size)) == NULL)
        return NULL;  /* fail if out-of-memory. */

    /* -------------------------------------------------------------------- */
    /*  Now, construct the .ROM image.                                      */
    /*  First:  The header.                                                 */
    /* -------------------------------------------------------------------- */
    switch (type)
    {
        case ICART:
            rom_img[0] = 0xA8;      /*  Autobaud Rate-Detection Byte        */
            break;
        case CC3_STD:
            rom_img[0] = 0x41;      /*  Autobaud Rate-Detection Byte        */
            break;
        case CC3_ADV:
            rom_img[0] = 0x61;      /*  Autobaud Rate-Detection Byte        */
            break;
        default:
            free(rom_img);          /*  Fail on unknown type.               */
            return NULL;
    }
    rom_img[1] = num_seg;           /*  Number of ROM segments in image.    */
    rom_img[2] = num_seg ^ 0xFF;    /*  1s Complement of # of segments.     */

    /* -------------------------------------------------------------------- */
    /*  Next:   The ROM segments.                                           */
    /* -------------------------------------------------------------------- */
    int ofs = 3;
    for (int i = 0; i < num_seg; i++)
    {
        const int start_ofs = ofs;

        /* ---------------------------------------------------------------- */
        /*  Inclusive segment range.  (upper 8 of addresses)                */
        /* ---------------------------------------------------------------- */
        rom_img[ofs++] = seg_lo[i];
        rom_img[ofs++] = seg_hi[i];

        const int lo  =  seg_lo[i] << 8;
        const int hi  = (seg_hi[i] << 8) + 0x100;
        const int len = 2*(hi - lo) + 2;            /* +2 to include header */

        /* ---------------------------------------------------------------- */
        /*  Actual ROM image data in big-endian format.                     */
        /* ---------------------------------------------------------------- */
        for (int j = lo; j < hi; j++)
        {
            rom_img[ofs++] = rom->image[j] >> 8;
            rom_img[ofs++] = rom->image[j] & 0xFF;
        }

        /* ---------------------------------------------------------------- */
        /*  ROM segment checksum (CRC-16), also big-endian.                 */
        /* ---------------------------------------------------------------- */
        crc = crc16_block(0xFFFF, rom_img + start_ofs, len);

        rom_img[ofs++] = crc >> 8;
        rom_img[ofs++] = crc & 0xFF;
    }

    /* -------------------------------------------------------------------- */
    /*  Last:   The attribute and fine-address tables.                      */
    /* -------------------------------------------------------------------- */
    for (int i = 0; i < 16; i++)
        rom_img[ofs++] = bank_attr[i*2 + 0] | (bank_attr[i*2 + 1] << 4);

    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 16; i++)
            rom_img[ofs++] = fine_addr[i*2 + j];

    crc = crc16_block(0xFFFF, rom_img + ofs - 48, 48);
    rom_img[ofs++] = crc >> 8;
    rom_img[ofs++] = crc & 0xFF;

    /* -------------------------------------------------------------------- */
    /*  If we had metadata, now tack it on the end and delete temp copy.    */
    /* -------------------------------------------------------------------- */
    if (md_img)
    {
        memcpy((void *)&rom_img[ofs], md_img, md_size);
        free(md_img);
    }

    /* -------------------------------------------------------------------- */
    /*  Return the completed .ROM image.                                    */
    /* -------------------------------------------------------------------- */
    return rom_img;
}

/* ======================================================================== */
/*  ICARTROM_DECODE -- Decode a .ROM image into an icartrom_t.              */
/* ======================================================================== */
int icartrom_decode
(
    icartrom_t    *const rom,
    const uint8_t *const rom_img,
    int            const img_len,
    int            const ignore_crc,
    int            const ignore_metadata
)
{
    int num_seg;
    uint16_t crc_expect, crc_actual;

    /* -------------------------------------------------------------------- */
    /*  Sanity checks.                                                      */
    /* -------------------------------------------------------------------- */
    if (!rom_img)
        return IC_BAD_ARGS;

    if (img_len < 53)
        return IC_SHORT_FILE;

    /* -------------------------------------------------------------------- */
    /*  First, check to see if the header passes initial muster.            */
    /* -------------------------------------------------------------------- */
    if ((rom_img[0] != 0xA8 && (rom_img[0] & ~0x20) != 0x41) ||
        (rom_img[1] ^ rom_img[2]) != 0xFF)
        return IC_BAD_ROM_HEADER;

    /* -------------------------------------------------------------------- */
    /*  Step through the ROM segments.                                      */
    /* -------------------------------------------------------------------- */
    num_seg = rom_img[1];
    int ofs = 3;
    for (int i = 0; i < num_seg; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Get the range of addresses for the ROM segment.                 */
        /* ---------------------------------------------------------------- */
        if (ofs + 2 > img_len) return IC_SHORT_FILE;

        const int start = ofs;
        const int lo    =  rom_img[ofs++] << 8;
        const int hi    = (rom_img[ofs++] << 8) + 0x100;

        if (hi < lo)
            return IC_BAD_SEG_ADDR_RANGE;

        if (!rom)
            ofs += 2 * (hi - lo);
        else
            for (int j = lo; j < hi; j++)
            {
                if (ofs + 2 > img_len) return IC_SHORT_FILE;

                rom->image[j] = (rom_img[ofs] << 8) | (rom_img[ofs + 1] & 0xFF);
                ofs += 2;
            }

        /* ---------------------------------------------------------------- */
        /*  Mark the "preload" bits for this ROM segment.                   */
        /* ---------------------------------------------------------------- */
        if (rom)
            for (int j = lo >> 8; j < (hi >> 8); j++)
                SET_BIT(rom->preload, j);

        /* ---------------------------------------------------------------- */
        /*  Check the CRC-16, unless instructed not to.  (We might do this  */
        /*  to try to rescue a corrupt or tweaked ROM or something.)        */
        /* ---------------------------------------------------------------- */
        if (ofs + 2 > img_len) return IC_SHORT_FILE;

        if (!ignore_crc)
        {
            crc_expect = (rom_img[ofs] << 8) | (rom_img[ofs + 1] & 0xFF);
            crc_actual = crc16_block(0xFFFF, rom_img + start, 2 * (hi-lo) + 2);
            if (crc_expect != crc_actual)
                return IC_CRC_ERROR_ROM_DATA;
        }
        ofs += 2; /* step over CRC */
    }

    /* -------------------------------------------------------------------- */
    /*  Unpack the attributes into our bitmaps.                             */
    /* -------------------------------------------------------------------- */
    for (int i = 0; i < 32; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Get our attribute and fine-address bits from the right nibbles. */
        /* ---------------------------------------------------------------- */
        if (ofs + (i >> 1) >= img_len) return IC_SHORT_FILE;

        const int attr = 0xF & (rom_img[ofs + (i >> 1)] >> ((i & 1) * 4));
        const int lohi = rom_img[ofs + 16 + ((i >> 1) | ((i & 1) << 4))];
        const int lo   = (lohi >> 4) & 0x7;
        const int hi   = (lohi & 0x7) + 1;

        /* ---------------------------------------------------------------- */
        /*  Sanity checks.                                                  */
        /* ---------------------------------------------------------------- */
        if (hi < lo)
            return IC_BAD_FINE_ADDR;

        if (!attr || !rom)
            continue;

        /* ---------------------------------------------------------------- */
        /*  Apply attributes to our fine-address range.                     */
        /* ---------------------------------------------------------------- */
        for (int j = lo; j < hi; j++)
        {
            const int p = (i << 3) + j;

            if (attr & ICARTROM_READ  ) SET_BIT(rom->readable, p);
            if (attr & ICARTROM_WRITE ) SET_BIT(rom->writable, p);
            if (attr & ICARTROM_NARROW) SET_BIT(rom->narrow  , p);
            if (attr & ICARTROM_BANKSW) SET_BIT(rom->dobanksw, p);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Check the CRC on our attributes.                                    */
    /* -------------------------------------------------------------------- */
    if (ofs + 50 > img_len) return IC_SHORT_FILE;
    if (!ignore_crc)
    {
        crc_expect = (rom_img[ofs + 48] << 8) | (rom_img[ofs + 49] & 0xFF);
        crc_actual = crc16_block(0xFFFF, rom_img + ofs, 48);
        if (crc_expect != crc_actual)
            return IC_CRC_ERROR_ENABLE_TBL;
    }

    ofs += 50;

    /* -------------------------------------------------------------------- */
    /*  If there's more to decode, it's metadata tags. Go get them, unless  */
    /*  we were told not to.  Don't bother if there's no ROM to populate.   */
    /* -------------------------------------------------------------------- */
    if (img_len == ofs || !rom || ignore_metadata)
    {
        if (rom)
            rom->metadata = NULL;
    } else
    {
        int ict_v_err = 0;

        icarttag_visitor_t *const ict_v = get_game_metadata_icarttag_visitor();
        const int new_ofs = icarttag_decode(rom_img, img_len, ignore_crc, ofs,
                                            ict_v, &ict_v_err);

        if (new_ofs < 0)
            return new_ofs;

        rom->metadata = put_game_metadata_icarttag_visitor(ict_v);

        assert(new_ofs >= ofs);
        ofs = new_ofs;
    }

    /* -------------------------------------------------------------------- */
    /*  Return the total number of bytes decoded.                           */
    /* -------------------------------------------------------------------- */
    return ofs;
}

/* ======================================================================== */
/*  ICARTROM_READFILE -- Reads a file into an icartrom_t.                   */
/* ======================================================================== */
int icartrom_readfile(const char *const fname, icartrom_t *const the_icart,
                      const int silent)
{
    LZFILE *const f = lzoe_fopen(fname, "rb");

    if (!f)
    {
        if (silent) return -1;
        perror("fopen()");
        fprintf(stderr, "ERROR: Couldn't open '%s' for reading\n", fname);
        exit(1);
    }

    lzoe_fseek(f, 0, SEEK_END);
    const int len = lzoe_ftell(f);
    if (len < 0)
    {
        if (silent) return -1;
        perror("fseek()");
        fprintf(stderr, "ERROR:  Error seeking while reading '%s'\n", fname);
        exit(1);
    }
    lzoe_rewind(f);

    uint8_t *const rom_img = CALLOC(uint8_t, len);
    if (!rom_img)
    {
        if (silent) return -1;
        perror("malloc()");
        fprintf(stderr, "ERROR:  Out of memory decoding '%s'\n", fname);
        exit(1);
    }

    if ((int)lzoe_fread(rom_img, 1, len, f) != len)
    {
        if (silent) return -1;
        fprintf(stderr, "ERROR:  Short read while reading '%s'\n", fname);
        exit(1);
    }
    lzoe_fclose(f);

    const int decoded = icartrom_decode(the_icart, rom_img, len, 0, 0);
    free(rom_img);

    return decoded;
}


/* ======================================================================== */
/*  ICARTROM_WRITEFILE -- Writes a file from an icartrom_t.                 */
/* ======================================================================== */
uint32_t icartrom_writefile(const char *const fname,
                            icartrom_t *const the_icart,
                            const ictype_t type)
{
    uint32_t rom_size = 0;
    size_t tag_size = 0;
    uint8_t *rom_img = NULL;
    FILE *fr;

    fr = fopen(fname, "wb");
    if (!fr)
    {
        fprintf(stderr, "ERROR:  Could not open '%s' for writing\n", fname);
        exit(1);
    }

    rom_img = icartrom_genrom(the_icart, &rom_size, type);

    if (!rom_img)
    {
        fprintf(stderr, "ERROR:  No ROM image generated?\n");
        exit(1);
    }

    fwrite(rom_img, 1, rom_size, fr);
    free(rom_img);

    fclose(fr);
    return rom_size + tag_size;
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
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
