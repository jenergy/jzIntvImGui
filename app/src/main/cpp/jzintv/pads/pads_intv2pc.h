/*
 * ============================================================================
 *  Title:    INTV2PC Interface for controller pads.
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements the controller pads.
 *  Pads are peripherals that extend periph_t.
 * ============================================================================
 */

#ifndef PAD_INTV2PC_H_
#define PAD_INTV2PC_H_

/*
 * ============================================================================
 *  PAD_INTV2PC_T -- INTV2PC interface structure
 * ============================================================================
 */
typedef struct pad_intv2pc_t
{
    periph_t    periph;     /*  Peripheral structure.                       */
#ifdef DIRECT_INTV2PC
    uint32_t    io_base;    /*  I/O port that INTV2PC is on.                */
    uint32_t    rd_state;   /*  State-machine counter.                      */
    uint32_t    rd_val;     /*  Value being read from INTV2PC.              */
    uint8_t     side[2];    /*  Last read values from each controller.      */
    uint8_t     io  [2];    /*  Flag bits:  Is this side set for output?    */
#endif
} pad_intv2pc_t;

#ifdef DIRECT_INTV2PC
extern int pads_intv2pc_ports_ok;

/*
 * ============================================================================
 *  PAD_INTV2PC_READ -- Get the current state of the INTV2PC controller.
 * ============================================================================
 */
uint32_t pad_intv2pc_read(periph_t *, periph_t *, uint32_t, uint32_t);

/*
 * ============================================================================
 *  PAD_INTV2PC_WRITE -- We need to monitor the I/O state for the pads ports.
 * ============================================================================
 */
void pad_intv2pc_write(periph_t *, periph_t *, uint32_t, uint32_t);

/*
 * ============================================================================
 *  PAD_INTV2PC_TICK  -- Update the INTV2PC reading state machine.
 * ============================================================================
 */
uint32_t pad_intv2pc_tick(periph_t *p, uint32_t len);
#endif /*DIRECT_INTV2PC*/

/*
 * ============================================================================
 *  PAD_INTV2PC_INIT  -- Initializes an INTV2PC device.
 * ============================================================================
 */
int pad_intv2pc_init
(
    pad_intv2pc_t  *pad,        /*  INTV2PC structure to initialize     */
    uint32_t        addr,       /*  Base address of pad.                */
    uint32_t        io_base     /*  Hand contr interface IO base.       */
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
