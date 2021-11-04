/*
 * ============================================================================
 *  Title:    Memory Subsystem
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements RAMs and ROMs of various sizes and widths.
 *  Memories are peripherals that extend periph_t.
 *
 *  Currently, bank-switched ROMs aren't supported, but they will be
 *  eventually as I need to.
 * ============================================================================
 * ============================================================================
 */


#include "config.h"
#include "periph/periph.h"
#include "mem.h"
#include "cp1600/cp1600.h"
#include "serializer/serializer.h"

extern int jlp_accel_on;    /* A bit of a hack */

LOCAL void mem_ser_init(periph_t *const p);     /* forward decl */

/*
 * ============================================================================
 *  MEM_RD_8     -- Reads from an 8-bit memory.
 *  MEM_RD_10    -- Reads from a 10-bit memory.
 *  MEM_RD_16    -- Reads from a 16-bit memory.
 *  MEM_RD_G16   -- Reads from a 16-bit glitchy memory.
 *  MEM_RD_IC16  -- Reads from an Intellicart 16-bit memory
 *  MEM_WR_8     -- Writes to an 8-bit memory.
 *  MEM_WR_10    -- Writes to a 10-bit memory.
 *  MEM_WR_16    -- Writes to a 16-bit memory.
 *  MEM_WR_G16   -- Writes to a 16-bit glitchy memory.
 *  MEM_WR_IC16  -- Writes to an Intellicart 16-bit memory
 * ============================================================================
 */

LOCAL uint32_t mem_rd_8(periph_t *const per, periph_t *ign,
                        uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    UNUSED(data);
    if ( mem->chk_jlp && jlp_accel_on ) return ~0U;
    return mem->image[addr] & 0xFF;
}

LOCAL uint32_t mem_rd_10(periph_t *const per, periph_t *ign,
                         uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    UNUSED(data);
    if ( mem->chk_jlp && jlp_accel_on ) return ~0U;
    return mem->image[addr] & 0x3FF;
}

LOCAL uint32_t mem_rd_16(periph_t *const per, periph_t *ign,
                         uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    UNUSED(data);
    if ( mem->chk_jlp && jlp_accel_on ) return ~0U;
    return mem->image[addr];
}

LOCAL uint32_t mem_rd_g16(periph_t *const per, periph_t *ign,
                          uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    uint16_t glitch = 0;
    UNUSED(ign);
    UNUSED(data);
    if ( mem->chk_jlp && jlp_accel_on ) return ~0U;
    /*if ((rand_jz() & 131071) == 3) glitch = 1u << (0xF & rand_jz());*/
    if ((rand_jz() & 65535) == 3) glitch = 1u << (0xF & rand_jz());
    return glitch ^ mem->image[addr];
}

LOCAL uint32_t mem_rd_gen(periph_t *const per, periph_t *ign,
                          uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    UNUSED(data);
    if ( mem->chk_jlp && jlp_accel_on ) return ~0U;
    return mem->image[addr] & mem->data_mask;
}

LOCAL void mem_wr_8(periph_t *const per, periph_t *ign,
                    uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    if ( mem->chk_jlp && jlp_accel_on ) return;
    mem->image[addr] = data & 0xFF;
}

LOCAL void mem_wr_10(periph_t *const per, periph_t *ign,
                     uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    if ( mem->chk_jlp && jlp_accel_on ) return;
    mem->image[addr] = data & 0x3FF;
}

LOCAL void mem_wr_16(periph_t *const per, periph_t *ign,
                     uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    if ( mem->chk_jlp && jlp_accel_on ) return;
    mem->image[addr] = data;
}

LOCAL void mem_wr_g16(periph_t *const per, periph_t *ign,
                      uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    if ( mem->chk_jlp && jlp_accel_on ) return;
    if ((rand_jz() & 131071) == 3) data ^= 1u << (0xF & rand_jz());
    mem->image[addr] = data;
}

LOCAL void mem_wr_gen(periph_t *const per, periph_t *ign,
                      uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    UNUSED(ign);
    if ( mem->chk_jlp && jlp_accel_on ) return;
    mem->image[addr] = data & mem->data_mask;
}

/*
 * ============================================================================
 *  MEM_RD_P16   -- Read a paged ROM
 *  MEM_WR_P16R  -- Write a paged ROM
 *  MEM_WR_P16W  -- Write a paged RAM
 *  MEM_PK_P16   -- Poke a paged ROM
 *  MEM_RS_P16   -- Reset a paged ROM
 * ============================================================================
 */
LOCAL uint32_t mem_rd_p16(periph_t *const per, periph_t *ign,
                          uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);

    UNUSED(ign);
    UNUSED(data);

    if ( mem->chk_jlp && jlp_accel_on ) return ~0U;
    if ( mem->page == mem->page_sel )
        return mem->image[addr] & mem->data_mask;

    return ~0U;
}

LOCAL void mem_wr_p16r(periph_t *const per, periph_t *ign,
                       uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    int range, page;

    UNUSED(ign);


    if ( mem->chk_jlp && jlp_accel_on ) return;
    if ((addr & 0x0FFF) != 0x0FFF) return;
    if (((addr | mem->periph.addr_base) & 0xFA50) != (data & 0xFFF0)) return;

    range = (data >> 12) & 0xF;
    page  = (data      ) & 0xF;

    if (mem->cpu)
        cp1600_invalidate((cp1600_t*)mem->cpu,
                          range << 12, (range << 12) | 0xFFF);
    mem->page_sel = page;
}

LOCAL void mem_wr_p16w(periph_t *const per, periph_t *ign,
                       uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    int range, page;

    UNUSED(ign);

    if ( mem->chk_jlp && jlp_accel_on ) return;

    if ( (addr & 0x0FFF) == 0x0FFF &&
        ((addr | mem->periph.addr_base) & 0xFA50) == (data & 0xFFF0))
    {
        range = (data >> 12) & 0xF;
        page  = (data      ) & 0xF;

        if (mem->cpu)
            cp1600_invalidate((cp1600_t*)mem->cpu,
                              range << 12, (range << 12) | 0xFFF);
        mem->page_sel = page;
        return;
    }

    if ( mem->page == mem->page_sel )
    {
        mem->image[addr] = data & mem->data_mask;
        if (mem->cpu)
        {
            const uint16_t full_addr = addr | mem->periph.addr_base;
            cp1600_invalidate((cp1600_t*)mem->cpu, full_addr, full_addr);
        }
    }
}

LOCAL void mem_pk_p16(periph_t *const per, periph_t *ign,
                      uint32_t addr, uint32_t data)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);

    UNUSED(ign);
    UNUSED(data);

    if ( mem->chk_jlp && jlp_accel_on ) return;
    if ( mem->page == mem->page_sel )
    {
        mem->image[addr] = data & mem->data_mask;
        if (mem->cpu)
        {
            const uint16_t full_addr = addr | mem->periph.addr_base;
            cp1600_invalidate((cp1600_t*)mem->cpu, full_addr, full_addr);
        }
    }
}

LOCAL void mem_rs_p16(periph_t *const per)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    mem->page_sel = 0;
    if (mem->cpu)
        cp1600_invalidate((cp1600_t*)mem->cpu,
                          per->addr_base, per->addr_base | per->addr_mask);
}

LOCAL void mem_ram_dtor(periph_t *const per)
{
    mem_t *const mem = PERIPH_AS(mem_t, per);
    CONDFREE(mem->image);
}

/*
 * ============================================================================
 *  MEM_MAKE_ROM -- Initializes a mem_t to be a ROM of a specified width
 *  MEM_MAKE_RAM -- Initializes a mem_t to be a RAM of a specified width
 *  MEM_MAKE_IC  -- Initializes a mem_t to be an Intellicart
 *  MEM_MAKE_PROM-- Initializes a mem_t to be a Paged ROM of specified width
 *  MEM_MAKE_9600A  Initializes a mem_t behave like RO-3-9600A at $360-$3FF
 * ============================================================================
 */

int mem_make_rom
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  width of ROM in bits.           */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size,       /*  Pwr of 2 size of ROM in words.  */
    uint16_t       *image       /*  Memory image to use for ROM     */
)
{
    /* -------------------------------------------------------------------- */
    /*  Set up peripheral function pointers to support reads of the right   */
    /*  width.  Ignore writes and explicitly disallow ticks.                */
    /* -------------------------------------------------------------------- */
    mem->periph.write   = NULL;
    mem->periph.read    = width ==  8 ? mem_rd_8  :
                          width == 10 ? mem_rd_10 :
                          width == 16 ? mem_rd_16 : mem_rd_gen;

    mem->periph.peek    = mem->periph.read;
    mem->periph.poke    = width ==  8 ? mem_wr_8  :
                          width == 10 ? mem_wr_10 :
                          width == 16 ? mem_wr_16 : mem_wr_gen;

    mem->periph.tick        = NULL;
    mem->periph.min_tick    = ~0U;
    mem->periph.max_tick    = ~0U;
    mem->periph.addr_base   = addr;
    mem->periph.addr_mask   = ~((~0U) << size);
    mem->periph.ser_init    = NULL; /* don't serialize ROMs */
    mem->periph.dtor        = NULL; /* don't destruct ROMs */

    /* -------------------------------------------------------------------- */
    /*  Set up the mem-specific fields.                                     */
    /* -------------------------------------------------------------------- */
    mem->image      = image;
    mem->img_length = 1u << size;
    mem->data_mask  = ~((~0U) << width);
    mem->chk_jlp    = 0;

    return 0;
}


int mem_make_ram
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  Width of RAM in bits.           */
    uint32_t        addr,       /*  Base address of RAM.            */
    uint32_t        size,       /*  Pwr of 2 size of RAM in words.  */
    int             randomize   /*  randomize memory on powerup?    */
)
{

    /* -------------------------------------------------------------------- */
    /*  Set up peripheral function pointers to support reads and writes of  */
    /*  the right width.  Explicitly disallow ticks.                        */
    /* -------------------------------------------------------------------- */
    mem->periph.write   = width ==  8 ? mem_wr_8  :
                          width == 10 ? mem_wr_10 :
                          width == 16 ? mem_wr_16 : mem_wr_gen;
    mem->periph.read    = width ==  8 ? mem_rd_8  :
                          width == 10 ? mem_rd_10 :
                          width == 16 ? mem_rd_16 : mem_rd_gen;

    mem->periph.peek    = mem->periph.read;
    mem->periph.poke    = mem->periph.write;

    mem->periph.tick        = NULL;
    mem->periph.min_tick    = ~0U;
    mem->periph.max_tick    = ~0U;
    mem->periph.addr_base   = addr;
    mem->periph.addr_mask   = ~((~0U) << size);
    mem->periph.ser_init    = mem_ser_init;
    mem->periph.dtor        = mem_ram_dtor;

    /* -------------------------------------------------------------------- */
    /*  Set up the mem-specific fields.                                     */
    /* -------------------------------------------------------------------- */
    mem->image      = CALLOC(uint16_t, 1u << size);
    mem->img_length = 1u << size;
    mem->data_mask  = ~((~0U) << width);
    mem->chk_jlp    = 0;

    /* -------------------------------------------------------------------- */
    /*  If set to randomize the memory, do so.                              */
    /* -------------------------------------------------------------------- */
    if (randomize && mem->image)
    {
        unsigned i;
        for (i = 0; i < mem->img_length; i++)
            mem->image[i] = rand_jz() & mem->data_mask;
    }

    return mem->image == NULL ? -1 : 0;
}

int mem_make_prom
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  width of ROM in bits.           */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size,       /*  Pwr of 2 size of ROM in words.  */
    uint32_t        page,       /*  ECS Page number to map to.      */
    uint16_t       *image,      /*  Memory image to use for ROM     */
    void           *cpu         /*  CPU pointer for handling cache  */
)
{
    assert( size == 12 && (addr & 0x0FFF) == 0 );

    /* -------------------------------------------------------------------- */
    /*  Set up peripheral function pointers to support reads of the right   */
    /*  width.  Ignore writes and explicitly disallow ticks.                */
    /* -------------------------------------------------------------------- */
    mem->periph.read    = mem_rd_p16;
    mem->periph.write   = mem_wr_p16r;
    mem->periph.peek    = mem->periph.read;
    mem->periph.poke    = mem_pk_p16;
    mem->periph.reset   = mem_rs_p16;

    mem->periph.tick        = NULL;
    mem->periph.min_tick    = ~0U;
    mem->periph.max_tick    = ~0U;
    mem->periph.addr_base   = addr;
    mem->periph.addr_mask   = 0xFFF;
    mem->periph.ser_init    = mem_ser_init;
    mem->periph.dtor        = NULL;

    /* -------------------------------------------------------------------- */
    /*  Set up the mem-specific fields.                                     */
    /* -------------------------------------------------------------------- */
    mem->image      = image;
    mem->page       = page;
    mem->page_sel   = 0;
    mem->data_mask  = ~((~0U) << width);
    mem->chk_jlp    = 0;
    mem->cpu        = cpu;

    return 0;
}

int mem_make_pram
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  width of ROM in bits.           */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size,       /*  Pwr of 2 size of ROM in words.  */
    uint32_t        page,       /*  ECS Page number to map to.      */
    uint16_t       *image,      /*  Memory image to use for ROM     */
    void           *cpu         /*  CPU pointer for handling cache  */
)
{
    assert( size == 12 && (addr & 0x0FFF) == 0 );

    /* -------------------------------------------------------------------- */
    /*  Set up peripheral function pointers to support reads of the right   */
    /*  width.  Allow writes and explicitly disallow ticks.                 */
    /* -------------------------------------------------------------------- */
    mem->periph.read    = mem_rd_p16;
    mem->periph.write   = mem_wr_p16w;
    mem->periph.peek    = mem->periph.read;
    mem->periph.poke    = mem_pk_p16;
    mem->periph.reset   = mem_rs_p16;

    mem->periph.tick        = NULL;
    mem->periph.min_tick    = ~0U;
    mem->periph.max_tick    = ~0U;
    mem->periph.addr_base   = addr;
    mem->periph.addr_mask   = 0xFFF;
    mem->periph.ser_init    = mem_ser_init;
    mem->periph.dtor        = NULL;

    /* -------------------------------------------------------------------- */
    /*  Set up the mem-specific fields.                                     */
    /* -------------------------------------------------------------------- */
    mem->image      = image;
    mem->page       = page;
    mem->page_sel   = 0;
    mem->data_mask  = ~((~0U) << width);
    mem->chk_jlp    = 0;
    mem->cpu        = cpu;

    return 0;
}


LOCAL uint16_t ra3_9600a_pat[8] =
{
    0x0204, 0x0255, 0x4104, 0x0020,
    0xF460, 0x0080, 0x0120, 0x0404
};

int mem_make_9600a
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size        /*  Pwr of 2 size of ROM in words.  */
)
{
    unsigned i;

    /* -------------------------------------------------------------------- */
    /*  Set up peripheral function pointers to support reads of the right   */
    /*  width.  Ignore writes and explicitly disallow ticks.                */
    /* -------------------------------------------------------------------- */
    mem->periph.write   = NULL;
    mem->periph.read    = mem_rd_g16;
    mem->periph.peek    = mem_rd_g16;
    mem->periph.poke    = mem_wr_16;

    mem->periph.tick        = NULL;
    mem->periph.min_tick    = ~0U;
    mem->periph.max_tick    = ~0U;
    mem->periph.addr_base   = addr;
    mem->periph.addr_mask   = ~((~0U) << size);
    mem->periph.ser_init    = mem_ser_init;
    mem->periph.dtor        = mem_ram_dtor;

    /* -------------------------------------------------------------------- */
    /*  Set up the mem-specific fields.                                     */
    /* -------------------------------------------------------------------- */
    mem->image      = CALLOC(uint16_t, 1u << size);
    mem->img_length = 1u << size;
    mem->data_mask  = 0xFFFF;
    mem->chk_jlp    = 0;

    /* -------------------------------------------------------------------- */
    /*  Write out a pattern similar to what I've observed on RA-3-9600A.    */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < (1u << size); i++)
        mem->image[i] = ra3_9600a_pat[i & 7];

    return 0;
}

int mem_make_glitch_ram
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size,       /*  Pwr of 2 size of ROM in words.  */
    int             randomize   /*  Randomize on init?              */
)
{
    /* -------------------------------------------------------------------- */
    /*  Set up peripheral function pointers to support reads of the right   */
    /*  width.  Ignore writes and explicitly disallow ticks.                */
    /* -------------------------------------------------------------------- */
    mem->periph.write   = mem_wr_g16;
    mem->periph.read    = mem_rd_g16;
    mem->periph.peek    = mem_rd_g16;
    mem->periph.poke    = mem_wr_g16;

    mem->periph.tick        = NULL;
    mem->periph.min_tick    = ~0U;
    mem->periph.max_tick    = ~0U;
    mem->periph.addr_base   = addr;
    mem->periph.addr_mask   = ~((~0U) << size);
    mem->periph.ser_init    = mem_ser_init;
    mem->periph.dtor        = mem_ram_dtor;

    /* -------------------------------------------------------------------- */
    /*  Set up the mem-specific fields.                                     */
    /* -------------------------------------------------------------------- */
    mem->image      = CALLOC(uint16_t, 1u << size);
    mem->img_length = 1u << size;
    mem->data_mask  = 0xFFFF;
    mem->chk_jlp    = 0;

    /* -------------------------------------------------------------------- */
    /*  If set to randomize the memory, do so.                              */
    /* -------------------------------------------------------------------- */
    if (randomize && mem->image)
    {
        unsigned i;
        for (i = 0; i < mem->img_length; i++)
            mem->image[i] = rand_jz() & mem->data_mask;
    }

    return 0;
}


/*
 * ============================================================================
 *  MEM_SER_INIT
 * ============================================================================
 */
LOCAL void mem_ser_init(periph_t *const p)
{
#ifdef NO_SERIALIZER
    UNUSED(p);
#else
    ser_hier_t *hier, *phier;
    mem_t *mem = PERIPH_AS(mem_t, p);


    hier  = ser_new_hierarchy(NULL, p->name);
    phier = ser_new_hierarchy(hier, "periph");

    periph_ser_register(p, phier);

    ser_register(hier, "data_mask", &mem->data_mask, ser_u32, 1,
                 SER_MAND|SER_HEX);

    if (p->read == mem_rd_p16)
    {
        ser_register(hier, "page", &mem->page, ser_u8, 1,
                     SER_MAND|SER_HEX);
        ser_register(hier, "page_sel", &mem->page_sel, ser_u8, 1,
                     SER_MAND|SER_HEX);
    } else
    {
        ser_register(hier, "image", mem->image, ser_u16, mem->img_length,
                     SER_MAND|SER_HEX);
    }
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
/*                 Copyright (c) 1998-2000, Joseph Zbiciak                  */
/* ======================================================================== */
