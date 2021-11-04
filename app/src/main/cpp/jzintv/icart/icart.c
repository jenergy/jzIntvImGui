/* ======================================================================== */
/*  Title:    Intellicart Emulation                                         */
/*  Author:   J. Zbiciak                                                    */
/* ------------------------------------------------------------------------ */
/*  This module implements Intellicart Emulation.  We use this emulation    */
/*  to emulate ALL Intellivision cartridges because it's much more          */
/*  convenient that way.                                                    */
/* ======================================================================== */

#include <assert.h>
#include "../config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "icart/icart.h"
#include "metadata/metadata.h"

/* ======================================================================== */
/*  ICART_CALC_BS    -- Calculate an address for a bank-switched address.   */
/* ======================================================================== */
LOCAL INLINE uint32_t icart_calc_bs(icart_t *const ic, uint32_t const addr)
{
    return 0xFFFF & (addr + ic->bs_tbl[addr >> 11]);
}

/* ======================================================================== */
/*  ICART_RD_F       -- Read flat 16-bit Intellicart memory.                */
/*  ICART_RD_FN      -- Read flat 8-bit Intellicart memory.                 */
/*  ICART_RD_B       -- Read bank-switched 16-bit Intellicart memory.       */
/*  ICART_RD_BN      -- Read bank-switched 8-bit Intellicart memory.        */
/* ======================================================================== */
uint32_t icart_rd_NULL(periph_t *const per, periph_t *const ign, 
                       const uint32_t addr, const uint32_t data)
{
    UNUSED(per); UNUSED(ign); UNUSED(addr); UNUSED(data);
    return ~0U;
}
uint32_t icart_rd_f(periph_t *const per, periph_t *const ign,
                    const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign); UNUSED(data);
    return ic->rom.image[addr];
}
uint32_t icart_rd_fn(periph_t *const per, periph_t *const ign,
                     const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign); UNUSED(data);
    return ic->rom.image[addr] & 0xFF;
}
uint32_t icart_rd_b(periph_t *const per, periph_t *const ign,
                    const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign); UNUSED(data);
    return ic->rom.image[icart_calc_bs(ic, addr)];
}
uint32_t icart_rd_bn(periph_t *const per, periph_t *const ign,
                     const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign); UNUSED(data);
    return ic->rom.image[icart_calc_bs(ic, addr)] & 0xFF;
}

/* ======================================================================== */
/*  ICART_WR_F       -- Write flat 16-bit Intellicart memory.               */
/*  ICART_WR_FN      -- Write flat 8-bit Intellicart memory.                */
/*  ICART_WR_B       -- Write bank-switched 16-bit Intellicart memory.      */
/*  ICART_WR_BN      -- Write bank-switched 8-bit Intellicart memory.       */
/*  ICART_WR_BI      -- Write bank-switch 16-bit I-cart mem, w/ invalidate  */
/*  ICART_WR_BNI     -- Write bank-switch 8-bit I-cart mem, w/ invalidate   */
/* ======================================================================== */

void icart_wr_NULL(periph_t *const per, periph_t *const ign,
                   const uint32_t addr, const uint32_t data)
{
    UNUSED(per); UNUSED(ign); UNUSED(addr); UNUSED(data);
}
void icart_wr_f(periph_t *const per, periph_t *const ign,
                const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign);
    ic->rom.image[addr] = data;
}
void icart_wr_fn(periph_t *const per, periph_t *const ign,
                 const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign);
    ic->rom.image[addr] = data & 0xFF;
}
void icart_wr_b(periph_t *const per, periph_t *const ign,
                const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign);
    ic->rom.image[icart_calc_bs(ic, addr)] = data;
}
void icart_wr_bn(periph_t *const per, periph_t *const ign,
                 const uint32_t addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);
    UNUSED(ign);
    ic->rom.image[addr] = data & 0xFF;
}

/* ======================================================================== */
/*  ICART_RD_BS      -- Read from bankswitch registers.                     */
/*  ICART_WR_BS      -- Write to bankswitch registers.                      */
/* ======================================================================== */
uint32_t icart_rd_bs(periph_t *const per, periph_t *const ign,
                     const uint32_t addr, const uint32_t data)
{
    UNUSED(per); UNUSED(ign); UNUSED(addr); UNUSED(data);
    return ~0U;
}

void icart_wr_bs(periph_t *const per, periph_t *const ign,
                 const uint32_t w_addr, const uint32_t data)
{
    icart_t *const ic = PERIPH_PARENT_AS(icart_t, per);

    UNUSED(ign);

    /* Convert the register write address to an Intellivision address */
    const uint32_t i_addr = ((w_addr & 0xF) << 12) | ((w_addr & 0x10) << 7);

    const uint32_t b_idx =  i_addr >> 13;
    const uint32_t b_shf = (i_addr >> 8) & 31;
    if (((ic->rom.dobanksw[b_idx] >> b_shf) & 1) == 0)
        return;

    /* Convert the write data + Intellivision address to a bankswitch entry. */
    const uint32_t t_idx = i_addr >> 11;
    const uint32_t bs    = ((data << 8) - (i_addr & ~0x0700)) & 0xFFFF;

    ic->bs_tbl[t_idx] = bs;

    if (ic->cache_bs && ((ic->rom.readable[b_idx] >> b_shf) & 1) == 1)
        cp1600_invalidate(ic->cpu, i_addr, i_addr + 0x07FF);
}

/* ======================================================================== */
/*  These handy tables simplify initializing the Intellicart.               */
/* ======================================================================== */
typedef struct
{
    periph_rd_t *read;
    periph_wr_t *write;
    periph_rd_t *peek;
    periph_wr_t *poke;
} ic_init_t;

LOCAL const ic_init_t ic_init[] =
{
    {   icart_rd_f,     icart_wr_NULL,  icart_rd_f,     icart_wr_f      },
    {   icart_rd_NULL,  icart_wr_f,     icart_rd_f,     icart_wr_f      },
    {   icart_rd_f,     icart_wr_f,     icart_rd_f,     icart_wr_f      },

    {   icart_rd_fn,    icart_wr_NULL,  icart_rd_fn,    icart_wr_fn     },
    {   icart_rd_NULL,  icart_wr_fn,    icart_rd_fn,    icart_wr_fn     },
    {   icart_rd_fn,    icart_wr_fn,    icart_rd_fn,    icart_wr_fn     },

    {   icart_rd_b,     icart_wr_NULL,  icart_rd_b,     icart_wr_b      },
    {   icart_rd_NULL,  icart_wr_b,     icart_rd_b,     icart_wr_b      },
    {   icart_rd_b,     icart_wr_b,     icart_rd_b,     icart_wr_b      },

    {   icart_rd_bn,    icart_wr_NULL,  icart_rd_bn,    icart_wr_bn     },
    {   icart_rd_NULL,  icart_wr_bn,    icart_rd_bn,    icart_wr_bn     },
    {   icart_rd_bn,    icart_wr_bn,    icart_rd_bn,    icart_wr_bn     },
};

/* Legend: R == Readable, W == Writable, N == Narrow, B == Bankswitchable */

LOCAL const int ic_attr_map[16] =
{
/*       R       R         */
/*           W   W         */
    -1,  0,  1,  2, /*     */
    -1,  3,  4,  5, /* N   */
    -1,  6,  7,  8, /*   B */
    -1,  9, 10, 11, /* N B */
};

/* ======================================================================== */
/*  ICART_DTOR   -- Tear down the ICART_T                                   */
/* ======================================================================== */
LOCAL void icart_dtor(periph_t *const p)
{
    icart_t *const icart = PERIPH_AS(icart_t, p);

    if (icart->rom.metadata)
    {
        free_game_metadata(icart->rom.metadata);
        icart->rom.metadata = NULL;
    }
}

/* ======================================================================== */
/*  ICART_INIT       -- Initialize the Intellicart w/ a ROM image.          */
/* ======================================================================== */
int icart_init
(
    icart_t *const ic,
    LZFILE  *const rom,
    int      const randomize
)
{
    uint8_t    *rom_img;
    periph_t   *p[12];
    long        size;
    int         err;

    /* -------------------------------------------------------------------- */
    /*  Sanity checks.                                                      */
    /* -------------------------------------------------------------------- */
    if (!ic || !rom)
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Clean things up.                                                    */
    /* -------------------------------------------------------------------- */
    memset(ic, 0, sizeof(icart_t));

    /* -------------------------------------------------------------------- */
    /*  If asked to randomize memory, do so.                                */
    /* -------------------------------------------------------------------- */
    if (randomize)
        for (int i = 0; i <= 0xFFFF; i++)
            ic->rom.image[i] = rand_jz();

    /* -------------------------------------------------------------------- */
    /*  The Intellicart offers multiple "periph" interfaces, each tuned     */
    /*  for a different type of memory.  This is done for performance.      */
    /*                                                                      */
    /*                  Read        Write       8-bit       Bankswitch      */
    /*      ic->r       Yes         No          No          No              */
    /*      ic->w       No          Yes         No          No              */
    /*      ic->rw      Yes         Yes         No          No              */
    /*      ic->rn      Yes         No          Yes         No              */
    /*      ic->wn      No          Yes         Yes         No              */
    /*      ic->rwn     Yes         Yes         Yes         No              */
    /*      ic->rb      Yes         No          No          Yes             */
    /*      ic->wb      No          Yes         No          Yes             */
    /*      ic->rwb     Yes         Yes         No          Yes             */
    /*      ic->rnb     Yes         No          Yes         Yes             */
    /*      ic->wnb     No          Yes         Yes         Yes             */
    /*      ic->rwnb    Yes         Yes         Yes         Yes             */
    /* -------------------------------------------------------------------- */
    p[ 0] = &(ic->r);               p[ 6] = &(ic->rb);
    p[ 1] = &(ic->w);               p[ 7] = &(ic->wb);
    p[ 2] = &(ic->rw);              p[ 8] = &(ic->rwb);
    p[ 3] = &(ic->rn);              p[ 9] = &(ic->rnb);
    p[ 4] = &(ic->wn);              p[10] = &(ic->wnb);
    p[ 5] = &(ic->rwn);             p[11] = &(ic->rwnb);

    for (int i = 0; i < 12; i++)
    {
        p[i]->read      = ic_init[i].read;
        p[i]->write     = ic_init[i].write;
        p[i]->peek      = ic_init[i].peek;
        p[i]->poke      = ic_init[i].poke;
        p[i]->tick      = NULL;
        p[i]->min_tick  = ~0U;
        p[i]->max_tick  = ~0U;
        p[i]->addr_base = 0x000;
        p[i]->addr_mask = 0xFFFF;
        p[i]->parent    = (void*)ic;
    }

    /* -------------------------------------------------------------------- */
    /*  Now, read in the .ROM file.                                         */
    /* -------------------------------------------------------------------- */
    size = file_length(rom);
    if (size <= 52)
    {
        fprintf(stderr, "icart:  Short file?\n");
        return -1;      /* Short file, or seek failed */
    }

    if ((rom_img = CALLOC(uint8_t, size)) == NULL)
    {
        fprintf(stderr, "icart:  Out of memory.\n");
        return -1;
    }

    lzoe_rewind(rom);
    if ((long)lzoe_fread(rom_img, 1, size, rom) != size)
    {
        fprintf(stderr, "icart:  Short read while reading ROM.\n");
        return -1;
    }

    if ((err = icartrom_decode(&(ic->rom), rom_img, size, 0, 0)) < 0)
    {
        fprintf(stderr, "icart:  Error %d while decoding ROM.\n", -err);
        return -1;
    }

    free(rom_img);

    /* -------------------------------------------------------------------- */
    /*  Base Intellicart peripheral, responsible for cleanup.               */
    /* -------------------------------------------------------------------- */
    ic->base.read      = NULL;
    ic->base.write     = NULL;
    ic->base.peek      = NULL;
    ic->base.poke      = NULL;
    ic->base.tick      = 0;
    ic->base.max_tick  = ~0U;
    ic->base.min_tick  = ~0U;
    ic->base.addr_base = 0;
    ic->base.addr_mask = 0;
    ic->base.parent    = ic;
    ic->base.dtor      = icart_dtor;

    /* -------------------------------------------------------------------- */
    /*  If asked to randomize, also randomize the bankswitch table.         */
    /*  This happens /after/ loading the ROM, since we need to know which   */
    /*  segments are bankswitched.                                          */
    /* -------------------------------------------------------------------- */
    if (randomize)
        for (int i = 0x00; i < 0x1F; i++)
            icart_wr_bs(AS_PERIPH(ic), NULL, i, rand_jz());

    return 0;
}

/* ======================================================================== */
/*  ICART_REGISTER   -- The Intellicart is unique in that it will register  */
/*                      itself on the peripheral bus.                       */
/* ======================================================================== */
int icart_register
(
    icart_t      *const ic,
    periph_bus_t *const bus,
    cp1600_t     *const cpu,
    uint32_t      const cache_flags
)
{
    int         attr, p_attr;
    char        buf[17];
    int         has_banksw = 0;
    uint32_t    addr_lo, addr_hi;
    periph_t   *p[12];
    int         warn_jlp = 0;
    const char *name = "ICart";

    /* -------------------------------------------------------------------- */
    /*  Sanity checks.                                                      */
    /* -------------------------------------------------------------------- */
    if (!ic || !bus)
        return -1;

    /* -------------------------------------------------------------------- */
    /*  See table above in icart_init for explanation.                      */
    /* -------------------------------------------------------------------- */
    p[ 0] = &(ic->r);               p[ 6] = &(ic->rb);
    p[ 1] = &(ic->w);               p[ 7] = &(ic->wb);
    p[ 2] = &(ic->rw);              p[ 8] = &(ic->rwb);
    p[ 3] = &(ic->rn);              p[ 9] = &(ic->rnb);
    p[ 4] = &(ic->wn);              p[10] = &(ic->wnb);
    p[ 5] = &(ic->rwn);             p[11] = &(ic->rwnb);

    /* -------------------------------------------------------------------- */
    /*  If someone selected JLP support, at least warn when the memory map  */
    /*  may do something bad to us.                                         */
    /* -------------------------------------------------------------------- */
    const int has_jlp = 
        (ic->rom.metadata && ic->rom.metadata->jlp_accel > JLP_DISABLED);

    /* -------------------------------------------------------------------- */
    /*  Register ourself on the peripheral bus.  Also, set up the CPU       */
    /*  cacheability of each range we register.                             */
    /* -------------------------------------------------------------------- */
    has_banksw = 0;
    ic->cpu = cpu;
    p_attr  = -1;
    addr_lo = addr_hi = -1;
    for (unsigned i = 0; i <= 256; i++)
    {
        const unsigned idx = i >> 5;
        const unsigned shf = i & 31;

        attr = i >= 256 ? -1
                        : (int)((1 & ((ic->rom.readable[idx] >> shf) << 0)) |
                                (2 & ((ic->rom.writable[idx] >> shf) << 1)) |
                                (4 & ((ic->rom.narrow  [idx] >> shf) << 2)) |
                                (8 & ((ic->rom.dobanksw[idx] >> shf) << 3)));

        /* Check for overlap between ICart and JLP in JLP's special RAM area */
        if (has_jlp && (attr & 1) == 1 && i >= 0x80 && i <= 0x9F)
            warn_jlp = 1;

        if (attr != p_attr)
        {
            if (p_attr > 0 && ic_attr_map[p_attr] >= 0)
            {
                snprintf(buf, sizeof(buf),
                        "ICart   [%c%c%c%c]",
                        p_attr & 1 ? 'R' : ' ',
                        p_attr & 2 ? 'W' : ' ',
                        p_attr & 4 ? 'N' : ' ',
                        p_attr & 8 ? 'B' : ' ');

                periph_register(bus, p[ic_attr_map[p_attr]],
                                addr_lo, addr_hi, buf);
                if (p_attr & ICARTROM_BANKSW)
                    has_banksw = 1;

                if (cache_flags & (1u << p_attr))
                {
                    int snoop;

                    snoop = (cache_flags >> (p_attr + 16)) & 1;
                    cp1600_cacheable(cpu, addr_lo, addr_hi, snoop);
                }
            }
            addr_lo = i << 8;
            addr_hi = addr_lo + 0xFF;
            p_attr  = attr;
        } else
        {
            addr_hi = (i << 8) + 0xFF;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Did we see a problem with JLP?  Print a warning to stderr.          */
    /* -------------------------------------------------------------------- */
    if (warn_jlp)
        fprintf(stderr, 
"icart: JLP is enabled, but Intellicart ROM mapped part of $8000 - $9FFF\n"
"       as readable memory.  This may result in incorrect JLP behavior.\n"
"       Remove the readable Intellicart ROM mapping in this address range\n"
"       when using JLP accelerators.  Mark this region's memory attributes\n"
"       as \"-R\" or \"-RWBN\", for example.\n"
"\n"
"       See http://spatula-city.org/~im14u2c/intv/icart_jlp.html .\n");

    /* -------------------------------------------------------------------- */
    /*  If this cartridge image uses bankswitching, reconfigure the base    */
    /*  peripheral to monitor the bankswitch registers.                     */
    /* -------------------------------------------------------------------- */
    if (has_banksw)
    {
        if (cache_flags & (IC_C_B__R | IC_C_B_WR | IC_C_BN_R | IC_C_BNWR))
            ic->cache_bs = 1;

        ic->base.read      = icart_rd_bs;
        ic->base.write     = icart_wr_bs;
        ic->base.peek      = icart_rd_bs;
        ic->base.poke      = icart_wr_bs;
        ic->base.tick      = 0;
        ic->base.max_tick  = ~0U;
        ic->base.min_tick  = ~0U;
        ic->base.addr_base = 0x40;
        ic->base.addr_mask = 0x1F;
        name = "ICart BankSw";
    }

    periph_register(bus, &(ic->base), ic->base.addr_base,
                    ic->base.addr_base + ic->base.addr_mask, name);

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
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
