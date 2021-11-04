/*
 * ============================================================================
 *  Title:    CGC Interface for controller pads.
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements the controller pads.
 *  Pads are peripherals that extend periph_t.
 * ============================================================================
 */

#ifndef PAD_CGC_H_
#define PAD_CGC_H_

/*
 * ============================================================================
 *  PAD_CGC_T -- CGC interface structure
 * ============================================================================
 */
typedef struct pad_cgc_t
{
    periph_t    periph;     /*  Peripheral structure.                       */
#ifdef CGC_SUPPORTED
    int         num_errors; /*  Number of errors reading CGC.               */
    uint8_t     io  [2];    /*  Flag bits:  Is this side set for output?    */
#else
    char        unused;     /*  nearly empty struct if compiled out.        */
#endif

#ifdef CGC_DLL
    uint32_t    cgc_num;    /*  Which CGC are we hooked to?                 */
#endif

#ifdef CGC_THREAD
    int         fd;             /*  File descriptor of CGC.                 */
    volatile uint8_t val[2];    /*  Last values read from CGC.              */
    volatile uint8_t die;       /*  Flag telling CGC thread to die.         */
#endif
} pad_cgc_t;

#ifdef CGC_SUPPORTED

/*
 * ============================================================================
 *  PAD_CGC_READ -- Get the current state of the CGC controller.
 * ============================================================================
 */
uint32_t pad_cgc_read(periph_t *, periph_t *, uint32_t, uint32_t);

/*
 * ============================================================================
 *  PAD_CGC_WRITE -- We need to monitor the I/O state for the pads' ports.
 * ============================================================================
 */
void    pad_cgc_write(periph_t *, periph_t *, uint32_t, uint32_t);

#endif /*CGC_SUPPORTED*/

/*
 * ============================================================================
 *  PAD_CGC_INIT  -- Initializes an Classic Game Controller
 * ============================================================================
 */
int pad_cgc_init
(
    pad_cgc_t  *pad,            /*  CGC structure to initialize             */
    uint32_t    addr,           /*  Base address of pad.                    */
    int         cgc_num,        /*  Which CGC in system to hook to  (win32) */
    const char *cgc_dev         /*  Device node associated w/ CGC.  (linux) */
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
/*                 Copyright (c) 1998-2004, Joseph Zbiciak                  */
/* ======================================================================== */
