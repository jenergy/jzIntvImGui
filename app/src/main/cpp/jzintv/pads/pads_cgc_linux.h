/*
 * ============================================================================
 *  Title:    Controller pads via Joe Fisher's Classic Gaming Controller
 *  Author:   J. Zbiciak
 * ============================================================================
 *  Some code in this module comes from Joe Fisher's reference code.
 * ============================================================================
 *  This module implements the controller pads.
 *  Pads are peripherals that extend periph_t.
 * ============================================================================
 */

#ifndef PAD_CGC_LINUX_H_
# define PAD_CGC_LINUX_H_
# if defined(PLAT_LINUX) || defined(PLAT_MACOS)

/* ======================================================================== */
/*  PAD_CGC_INIT -- Initializes a Classic Gaming Controller interface.      */
/* ======================================================================== */
int pad_cgc_linux_init
(
    pad_cgc_t      *pad,            /*  pad_cgc_t structure to initialize   */
    uint32_t        addr,           /*  Base address of pad.                */
    const char      *cgc_dev        /*  path to CGC device.                 */
);

# endif /* PLAT_LINUX || PLAT_MACOS */
#endif /* PAD_CGC_LINUX_H_ */

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
/*                 Copyright (c) 2004-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
