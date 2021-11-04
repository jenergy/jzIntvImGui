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

#ifndef MEM_H_
#define MEM_H_

/*
 * ============================================================================
 *  MEM_T        -- Memory Peripheral Structure
 * ============================================================================
 */

typedef struct mem_t
{
    periph_t    periph;     /*  Peripheral structure.                       */
    uint16_t   *image;      /*  Memory image.                               */
    uint32_t    img_base;   /*  address base of memory image.               */
    uint32_t    data_mask;  /*  Data mask for odd-sized memories.           */
    uint8_t     page;       /*  The page associated with this ROM, if any   */
    uint8_t     page_sel;   /*  The page selected in the ROM seg            */
    uint8_t     chk_jlp;    /*  Flag: If set, need to check jlp_accel_on    */
    uint32_t    img_length; /*  Actual length of memory image.              */
    void       *cpu;        /*  CPU pointer for handling caching.           */
} mem_t;


/*
 * ============================================================================
 *  MEM_MAKE_ROM -- Initializes a mem_t to be a ROM of a specified width
 *  MEM_MAKE_RAM -- Initializes a mem_t to be a RAM of a specified width
 *  MEM_MAKE_IC  -- Initializes a mem_t to emulate an Intellicart.
 * ============================================================================
 */

int mem_make_rom
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  width of ROM in bits.           */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size,       /*  Size of ROM in words.           */
    uint16_t       *image       /*  Memory image to use for ROM     */
);

int mem_make_prom
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  width of ROM in bits.           */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size,       /*  Size of ROM in words.           */
    uint32_t        page,       /*  ECS page number of ROM (0..15)  */
    uint16_t       *image,      /*  Memory image to use for ROM     */
    void           *cpu         /*  CPU struct, for managing cache  */
);

int mem_make_ram
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  Width of RAM in bits.           */
    uint32_t        addr,       /*  Base address of RAM.            */
    uint32_t        size,       /*  Size of RAM in words.           */
    int             randomize   /*  Flag: Randomize on init         */
);

int mem_make_pram
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    int             width,      /*  width of ROM in bits.           */
    uint32_t        addr,       /*  Base address of ROM.            */
    uint32_t        size,       /*  Size of ROM in words.           */
    uint32_t        page,       /*  ECS page number of ROM (0..15)  */
    uint16_t       *image,      /*  Memory image to use for ROM     */
    void           *cpu         /*  CPU struct, for managing cache  */
);

int mem_make_ic
(
    mem_t          *mem         /*  mem_t structure to initialize   */
);

int mem_make_9600a
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    uint32_t        addr,       /*  Base address                    */
    uint32_t        size        /*  Power-of-2 size                 */
);

int mem_make_glitch_ram
(
    mem_t          *mem,        /*  mem_t structure to initialize   */
    uint32_t        addr,       /*  Base address                    */
    uint32_t        size,       /*  Power-of-2 size                 */
    int             randomize   /*  Flag: Randomize on init         */
);

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
/*                 Copyright (c) 1998-2000, Joseph Zbiciak                  */
/* ======================================================================== */
