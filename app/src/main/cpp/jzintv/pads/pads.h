/*
 * ============================================================================
 *  Title:    Controller pads
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements the controller pads.
 *  Pads are peripherals that extend periph_t.
 * ============================================================================
 */

#ifndef PAD_H_
#define PAD_H_

typedef enum { PAD_INPUT_ONLY, PAD_BIDIR     } pad_io_cap_t;
typedef enum { PAD_DIR_INPUT, PAD_DIR_OUTPUT } pad_io_dir_t;

/* ======================================================================== */
/*  PAD_T        -- Controller Pad structure                                */
/* ======================================================================== */
typedef struct pad_t
{
    periph_t    periph;     /*  Peripheral structure.                       */
    uint8_t     side[2];    /*  Last read/written values on each port.      */
    uint8_t     io  [2];    /*  Flag bits:  Is this side set for output?    */
    uint8_t     io_cap;     /*  Flag: Can we change I/O modes?              */
    bool        stale;      /*  Flag: We need to reevaluate controllers.    */

    /* The following must be uint32_t for the event subsystem.              */
    uint32_t    l[18];      /*  Event inputs to left controllers.           */
    uint32_t    r[18];      /*  Event inputs to right controllers.          */
    uint32_t    k[8];       /*  Keyboard inputs for all 8 scanning rows.    */
    uint32_t    fake_shift; /*  We fake pressing the shift key sometimes.   */
} pad_t;

/* ======================================================================== */
/*  PAD_INIT     -- Makes an input pad device                               */
/* ======================================================================== */
int pad_init
(
    pad_t          *const pad,      /*  pad_t structure to initialize       */
    const uint32_t        addr,     /*  Base address of pad.                */
    const pad_io_cap_t    io_cap    /*  Input only, or bidirectional?       */
);

/* ======================================================================== */
/*  PAD_RESET_INPUTS -- Reset the input bitvectors.  Used when switching    */
/*                      keyboard input maps.                                */
/* ======================================================================== */
void pad_reset_inputs(pad_t *const pad);

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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
