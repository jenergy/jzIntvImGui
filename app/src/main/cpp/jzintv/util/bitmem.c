/* ======================================================================== */
/*  BITMEM -- Bit-addressed memory routines.                                */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "bitmem.h"

/* ======================================================================== */
/*  BITREV -- Reverses the bits in a 32-bit number.                         */
/* ======================================================================== */
static uint32_t bitrev(uint32_t r)
{
    r = (0xFFFF0000 & (r << 16)) | (0x0000FFFF & (r >> 16));
    r = (0xFF00FF00 & (r <<  8)) | (0x00FF00FF & (r >>  8));
    r = (0xF0F0F0F0 & (r <<  4)) | (0x0F0F0F0F & (r >>  4));
    r = (0xCCCCCCCC & (r <<  2)) | (0x33333333 & (r >>  2));
    r = (0xAAAAAAAA & (r <<  1)) | (0x55555555 & (r >>  1));

    return r;
}

/* ======================================================================== */
/*  BITMEM_CREATE -- Creates a new bit-memory object.                       */
/* ======================================================================== */
bitmem_t *bitmem_create  (int org, int bits)
{
    bitmem_t *mem;
    int words = (bits + 31) >> 5;

    if (org & 31)
    {
        fprintf(stderr, "Error: bitmem requires origin to be mult of 32.\n");
        exit(1);
    }

    if (!(mem       = CALLOC(bitmem_t, 1        )) ||
        !(mem->data = CALLOC(uint32_t, words + 3)) ||
        !(mem->attr = CALLOC(uint8_t,  bits + 96)))
    {
        fprintf(stderr, "bitmem_create: Out of memory!\n");
        exit(1);
    }


    mem->data++;        /* We allow accesses to data[-1] */
    mem->org  = org >> 5;
    mem->size = words;

    return mem;
}

/* ======================================================================== */
/*  BITMEM_LOAD  -- Loads a memory image into a bit-memory.                 */
/* ======================================================================== */
#define CHUNK (1024)
void bitmem_load(bitmem_t *mem, FILE *f)
{
    uint8_t  readbuf[CHUNK + 4];
    int      bytes, i;
    int      short_rd = 0;
    uint32_t offset = 0;
    uint32_t word;

    /* -------------------------------------------------------------------- */
    /*  Keep reading until the file's empty or the memory's full.           */
    /* -------------------------------------------------------------------- */
    while (offset < mem->size && (bytes = fread(readbuf, 1, CHUNK, f)) > 0)
    {
        /* ---------------------------------------------------------------- */
        /*  This code needs to handle arbitrary byte counts, but really     */
        /*  wants multiples of four bytes.  As written, it can't handle     */
        /*  two short-reads in a row, if the first one was not a mult of 4. */
        /* ---------------------------------------------------------------- */
        if (bytes < CHUNK)
        {
            if (short_rd)
            {
                fprintf(stderr, "bitmem_load: Two short reads in a row\n");
                exit(1);
            }

            short_rd |= bytes & 3;
        }

        /* ---------------------------------------------------------------- */
        /*  Pad the end of the buffer with zeros, in case # of bytes is     */
        /*  not a multiple of 4.                                            */
        /* ---------------------------------------------------------------- */
        readbuf[bytes  ] = 0;
        readbuf[bytes+1] = 0;
        readbuf[bytes+2] = 0;
        readbuf[bytes+3] = 0;

        /* ---------------------------------------------------------------- */
        /*  Handle a read that read too much.                               */
        /* ---------------------------------------------------------------- */
        if ((bytes >> 2) + offset > mem->size)
            bytes = (mem->size - offset) << 2;

        /* ---------------------------------------------------------------- */
        /*  Put bytes together into words in big-endian order.              */
        /* ---------------------------------------------------------------- */
        for (i = 0; i < bytes; i += 4, offset++)
        {
            word = (readbuf[i+0] << 24) |
                   (readbuf[i+1] << 16) |
                   (readbuf[i+2] <<  8) |
                   (readbuf[i+3] <<  0);

            mem->data[offset] = word;
        }
    }
}

/* ======================================================================== */
/*  Handy macros which are very useful for bitstream assembly.              */
/*  These handle the case of shifts by more than 32, which is important.    */
/* ======================================================================== */
#define LSHIFT(d,s) (((s) & 32) == 0 ? (d) << (s) : 0)
#define RSHIFT(d,s) (((s) & 32) == 0 ? (d) >> (s) : 0)

/* ======================================================================== */
/*  BITMEM_READ_FWD  -- Read a bit field in the forward direction.          */
/* ======================================================================== */
uint32_t bitmem_read_fwd(bitmem_t *mem, uint32_t addr, int bits)
{
    int offset, lshift, rshift;
    uint32_t data;

    /* -------------------------------------------------------------------- */
    /*  Calculate our offset and shift values from the bit-address.         */
    /* -------------------------------------------------------------------- */
    offset = addr >> 5;
    lshift = addr & 31;
    rshift = 32 - lshift;

    /* -------------------------------------------------------------------- */
    /*  Adjust our offset according to our origin.  If the offset is        */
    /*  totally outside our memory, return 0.                               */
    /* -------------------------------------------------------------------- */
    offset -= mem->org;
    if (offset < -1 || offset >= (int)mem->size)
        return 0;

    /* -------------------------------------------------------------------- */
    /*  Extract the desired data as a left-justified series of bits.        */
    /* -------------------------------------------------------------------- */
    data = LSHIFT(mem->data[offset    ], lshift) |
           RSHIFT(mem->data[offset + 1], rshift);

    /* -------------------------------------------------------------------- */
    /*  Shift the data right to extract the field, and return it.           */
    /* -------------------------------------------------------------------- */
    return data >> (32 - bits);
}

/* ======================================================================== */
/*  BITMEM_READ_FWD  -- Read a bit field in the reverse direction.          */
/* ======================================================================== */
uint32_t bitmem_read_rev(bitmem_t *mem, uint32_t addr, int bits)
{
    int offset, lshift, rshift;
    uint32_t data;

    /* -------------------------------------------------------------------- */
    /*  Calculate our offset and shift values from the bit-address.         */
    /* -------------------------------------------------------------------- */
    offset = addr >> 5;
    lshift = addr & 31;
    rshift = 32 - lshift;

    /* -------------------------------------------------------------------- */
    /*  Adjust our offset according to our origin.  If the offset is        */
    /*  totally outside our memory, return 0.                               */
    /* -------------------------------------------------------------------- */
    offset -= mem->org;
    if (offset < -1 || offset >= (int)mem->size)
        return 0;

    /* -------------------------------------------------------------------- */
    /*  Extract the desired data as a left-justified series of bits.        */
    /* -------------------------------------------------------------------- */
    data = LSHIFT(mem->data[offset    ], lshift) |
           RSHIFT(mem->data[offset + 1], rshift);

    /* -------------------------------------------------------------------- */
    /*  Mask away the bits we're not interested in with a left-mask.        */
    /* -------------------------------------------------------------------- */
    data &= LSHIFT(~0U, 32-bits);

    /* -------------------------------------------------------------------- */
    /*  Return the bit-reversed field, right-justified.                     */
    /* -------------------------------------------------------------------- */
    return bitrev(data);
}

/* ======================================================================== */
/*  BITMEM_SETATTR   -- Set attributes on a range of memory addresses.      */
/* ======================================================================== */
void bitmem_setattr(bitmem_t *mem, uint32_t addr, int len, uint8_t attr)
{
    uint32_t org32 = mem->org << 5;

    /* -------------------------------------------------------------------- */
    /*  Never let the user set AT_LOCAL.  We handle that ourselves.         */
    /* -------------------------------------------------------------------- */
    attr &= ~AT_LOCAL;

    /* -------------------------------------------------------------------- */
    /*  Make sure address and len parameters are inside our bit-memory.     */
    /*  Clip the arguments to the memory boundaries if necessary.           */
    /* -------------------------------------------------------------------- */
    if (addr < org32)
    {
        len -= org32 - addr;
        addr = 0;
    } else
        addr -= org32;

    if (len < 0 || addr > (mem->size << 5))
        return;

    if (addr + len > (mem->size << 5))
        len = (mem->size << 5) - addr;

    /* -------------------------------------------------------------------- */
    /*  Step through the desired range of addresses, setting attributes.    */
    /* -------------------------------------------------------------------- */
    while (len > 0)
    {
        mem->attr[addr++] |= attr;
        len--;
    }
}

/* ======================================================================== */
/*  BITMEM_CLRATTR   -- Clear attributes on a range of memory addresses.    */
/* ======================================================================== */
void bitmem_clrattr(bitmem_t *mem, uint32_t addr, int len, uint8_t attr)
{
    uint32_t org32 = mem->org << 5;

    /* -------------------------------------------------------------------- */
    /*  Make sure address and len parameters are inside our bit-memory.     */
    /*  Clip the arguments to the memory boundaries if necessary.           */
    /* -------------------------------------------------------------------- */
    if (addr < org32)
    {
        len -= org32 - addr;
        addr = 0;
    } else
        addr -= org32;

    if (len < 0 || addr > (mem->size << 5))
        return;

    if (addr + len > (mem->size << 5))
        len = (mem->size << 5) - addr;

    /* -------------------------------------------------------------------- */
    /*  Step through the desired range of addresses, clearing attributes.   */
    /* -------------------------------------------------------------------- */
    while (len > 0)
    {
        mem->attr[addr++] &= ~attr;
        len--;
    }
}

/* ======================================================================== */
/*  BITMEM_GETATTR   -- Get merged attributes for a range of addresses.     */
/* ======================================================================== */
uint8_t bitmem_getattr(bitmem_t *mem, uint32_t addr, int len)
{
    uint32_t org32  = mem->org  << 5;
    uint32_t size32 = mem->size << 5;
    uint8_t  attr   = AT_LOCAL;

    /* -------------------------------------------------------------------- */
    /*  Make sure address and len parameters are inside our bit-memory.     */
    /*  Clip the arguments to the memory boundaries if necessary.           */
    /* -------------------------------------------------------------------- */
    if (addr < org32)
    {
        len -= org32 - addr;
        addr = 0;
    } else
        addr -= org32;

    if (len <= 0 || addr >= size32)
        return 0;

    if (addr + len > size32)
        len = size32 - addr;

    /* -------------------------------------------------------------------- */
    /*  Step through the desired range of addresses, merging attributes.    */
    /* -------------------------------------------------------------------- */
    while (len > 0)
    {
        attr |= mem->attr[addr++];
        len--;
    }

    /* -------------------------------------------------------------------- */
    /*  Return the merged attribute mask.                                   */
    /* -------------------------------------------------------------------- */
    return attr;
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
