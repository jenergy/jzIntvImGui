/* ======================================================================== */
/*  Title:    Intellicart Emulation                                         */
/*  Author:   J. Zbiciak                                                    */
/* ------------------------------------------------------------------------ */
/*  This module implements Intellicart Emulation.  We use this emulation    */
/*  to emulate ALL Intellivision cartridges because it's much more          */
/*  convenient that way.                                                    */
/* ======================================================================== */

#ifndef ICART_H_
#define ICART_H_
#include "icart/icartrom.h"

/* ======================================================================== */
/*  The Intellicart Structure.                                              */
/* ======================================================================== */
typedef struct icart_t
{
    periph_t    base;
    periph_t    r,   w,   rw;
    periph_t    rn,  wn,  rwn;
    periph_t    rb,  wb,  rwb;
    periph_t    rnb, wnb, rwnb;
    cp1600_t   *cpu;
    uint32_t    bs_tbl[32];
    icartrom_t  rom;
    int         cache_bs;
} icart_t;

/* ======================================================================== */
/*  Intellicart-vs-CPU cache enable bits:                                   */
/*                                                                          */
/*      IC_C_xxxx    -- When set, says CP-1600 is allowed to cache pages    */
/*                      whose attributes are the proper subset of these.    */
/*                                                                          */
/*      IC_S_xxxx    -- When set, says CP-1600 is must snoop cached pages   */
/*                      that have this attribute set.                       */
/* ======================================================================== */

#define IC_C_____    (0x00000001)
#define IC_C____R    (0x00000002)
#define IC_C___W_    (0x00000004)
#define IC_C___WR    (0x00000008)
#define IC_C__N__    (0x00000010)
#define IC_C__N_R    (0x00000020)
#define IC_C__NW_    (0x00000040)
#define IC_C__NWR    (0x00000080)
#define IC_C_B___    (0x00000100)
#define IC_C_B__R    (0x00000200)
#define IC_C_B_W_    (0x00000400)
#define IC_C_B_WR    (0x00000800)
#define IC_C_BN__    (0x00001000)
#define IC_C_BN_R    (0x00002000)
#define IC_C_BNW_    (0x00004000)
#define IC_C_BNWR    (0x00008000)

#define IC_S_____    (0x00010000)
#define IC_S____R    (0x00020000)
#define IC_S___W_    (0x00040000)
#define IC_S___WR    (0x00080000)
#define IC_S__N__    (0x00100000)
#define IC_S__N_R    (0x00200000)
#define IC_S__NW_    (0x00400000)
#define IC_S__NWR    (0x00800000)
#define IC_S_B___    (0x01000000)
#define IC_S_B__R    (0x02000000)
#define IC_S_B_W_    (0x04000000)
#define IC_S_B_WR    (0x08000000)
#define IC_S_BN__    (0x10000000)
#define IC_S_BN_R    (0x20000000)
#define IC_S_BNW_    (0x40000000)
#define IC_S_BNWR    (0x80000000)

#define IC_CACHE_CABS  (IC_C____R |                 \
                        IC_C___WR | IC_S___WR |     \
                        IC_C_B__R |                 \
                        IC_C_B_WR | IC_S_B_WR)

#define IC_CACHE_NOBS  (IC_C____R |                 \
                        IC_C___WR | IC_S___WR)

#define IC_CACHE_SAFE  (IC_C____R)
#define IC_CACHE_NONE  (0)

#define IC_CACHE_DFLT  (IC_CACHE_NOBS)

/* ======================================================================== */
/*  ICART_RD_F       -- Read flat 16-bit Intellicart memory.                */
/*  ICART_RD_FN      -- Read flat 8-bit Intellicart memory.                 */
/*  ICART_RD_B       -- Read bank-switched 16-bit Intellicart memory.       */
/*  ICART_RD_BN      -- Read bank-switched 8-bit Intellicart memory.        */
/* ======================================================================== */
uint32_t icart_rd_NULL(periph_t *const per, periph_t *const ign,
                       const uint32_t addr, const uint32_t data);
uint32_t icart_rd_f   (periph_t *const per, periph_t *const ign,
                       const uint32_t addr, const uint32_t data);
uint32_t icart_rd_fn  (periph_t *const per, periph_t *const ign,
                       const uint32_t addr, const uint32_t data);
uint32_t icart_rd_b   (periph_t *const per, periph_t *const ign,
                       const uint32_t addr, const uint32_t data);
uint32_t icart_rd_bn  (periph_t *const per, periph_t *const ign,
                       const uint32_t addr, const uint32_t data);

/* ======================================================================== */
/*  ICART_WR_F       -- Write flat 16-bit Intellicart memory.               */
/*  ICART_WR_FN      -- Write flat 8-bit Intellicart memory.                */
/*  ICART_WR_B       -- Write bank-switched 16-bit Intellicart memory.      */
/*  ICART_WR_BN      -- Write bank-switched 8-bit Intellicart memory.       */
/* ======================================================================== */
void icart_wr_NULL(periph_t *const per, periph_t *const ign,
                   const uint32_t addr, const uint32_t data);
void icart_wr_f   (periph_t *const per, periph_t *const ign,
                   const uint32_t addr, const uint32_t data);
void icart_wr_fn  (periph_t *const per, periph_t *const ign,
                   const uint32_t addr, const uint32_t data);
void icart_wr_b   (periph_t *const per, periph_t *const ign,
                   const uint32_t addr, const uint32_t data);
void icart_wr_bn  (periph_t *const per, periph_t *const ign,
                   const uint32_t addr, const uint32_t data);

/* ======================================================================== */
/*  ICART_RD_BS      -- Read from bankswitch registers.                     */
/*  ICART_WR_BS      -- Write to bankswitch registers.                      */
/* ======================================================================== */
uint32_t icart_rd_bs(periph_t *const per, periph_t *const ign,
                     const uint32_t addr, const uint32_t data);
void icart_wr_bs(periph_t *const per, periph_t *const ign,
                 const uint32_t addr, const uint32_t data);

/* ======================================================================== */
/*  ICART_INIT       -- Initialize an Intellicart from a ROM image.         */
/* ======================================================================== */
int icart_init
(
    icart_t *const ic,
    LZFILE  *const rom,
    int      const randomize
);

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
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
