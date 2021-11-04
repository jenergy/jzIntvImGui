/* ======================================================================== */
/*  Cartridge Reader Interfacr routines.                                    */
/*  By Joe Zbiciak                                                          */
/*  Using hardware designed by Scott Nudds                                  */
/*  (Based on routines from my own SUCK.BAS.)                               */
/* ======================================================================== */

#ifndef CART_H_
#define CART_H_

/* ------------------------------------------------------------------------ */
/*  Cart-reader structure                                                   */
/* ------------------------------------------------------------------------ */
typedef struct cart_rd_t
{
    unsigned        ctrl;               /* Current control signal status.   */
    unsigned        disp;               /* Current display pattern.         */
    unsigned long   port;               /* Current cart-reader port number. */
    int             reset_delay;        /* How long to hold reset.          */
    int             seg9;               /* Display segment 9.               */
    int             leds[8], lacc[8];   /* LED state for 'chaser'.          */
} cart_rd_t;

/* ------------------------------------------------------------------------ */
/*  Bus and Cart-reader Control Bits                                        */
/* ------------------------------------------------------------------------ */
typedef enum ctrl_bits_t
{
    DS9     = 128,                      /* Display Segment #9               */
    MSYNC   = 64,                       /* Machine SYNC control signal      */
    SL      = 32,                       /* Shift / Latch control input      */
    CLK     = 16,                       /* Shift clock.                     */
    OC      = 8,                        /* Output Control.                  */
    BDIR    = 4,                        /* Bus DIRection                    */
    BC2     = 2,                        /* Bus Control 2                    */
    BC1     = 1                         /* Bus Control 1                    */
} ctrl_bits_t;

/* ------------------------------------------------------------------------ */
/*  GI Bus Protocol Enumerations.                                           */
/* ------------------------------------------------------------------------ */
typedef enum gi_bus_phase_t
{
    RESET   =   0,                          /* Reset the bus.               */
    NACT    =   MSYNC,                      /* No ACTion                    */
    ADAR    =   MSYNC|              BC1,    /* Address Data to Addr Reg     */
    IAB     =   MSYNC|      BC2,            /* Interrupt Address to Bus     */
    DTB     =   MSYNC|      BC2 |   BC1,    /* Data To Bus                  */
    BAR     =   MSYNC|BDIR,                 /* Bus to Address Register      */
    DW      =   MSYNC|BDIR |            BC1,/* Data Write                   */
    DWS     =   MSYNC|BDIR |    BC2,        /* Data Write Strobe            */
    INTAK   =   MSYNC|BDIR |    BC2 |   BC1 /* INTerrupt AcKnowledge        */
} gi_bus_phase_t;

#define CTRL(b,x) ((0x47 & (unsigned)(b)) | (0xB8 & (unsigned)(x)))

/* ------------------------------------------------------------------------ */
/*  Give names to the various latch enables on the cart reader.             */
/* ------------------------------------------------------------------------ */
typedef enum lat_enab_t
{
    LAT_INH = 4,                        /* Inhibit the decoder              */
    LAT_CEN = 3,                        /* Control-signal latch.            */
    LAT_XEN = 2,                        /* Low-order Address/Data latch     */
    LAT_YEN = 1,                        /* High-order Address/Data latch    */
    LAT_DEN = 0,                        /* Display latch                    */
} lat_enab_t;

/* ------------------------------------------------------------------------ */
/*  CR_SET_DISP      -- Set the display register to an 8-bit value.         */
/*  CR_SET_CTRL      -- Set the bus control lines to a given state.         */
/*  CR_SET_DATA      -- Set the data bus lines to a given value.            */
/*  CR_GET_DATA      -- Read the current values that are on the data bus.   */
/*  CR_DESELECT      -- Make sure the Cartridge Reader is deselected.       */
/* ------------------------------------------------------------------------ */
void cr_set_disp(cart_rd_t *cr, unsigned display);
void cr_set_ctrl(cart_rd_t *cr, unsigned control);
void cr_set_data(cart_rd_t *cr, unsigned value);
unsigned cr_get_data(cart_rd_t *cr);
void cr_deselect(cart_rd_t *cr);

/* ------------------------------------------------------------------------ */
/*  CR_LOOPBACK      -- Writes, then reads, a given value on the bus.       */
/*                      Returns whether the value read matched what was     */
/*                      actually written.  (0 == Match, 1 == Mismatch.)     */
/* ------------------------------------------------------------------------ */
int cr_loopback(cart_rd_t *cr, unsigned value, unsigned *rd);

/* ------------------------------------------------------------------------ */
/*  CR_DETECT        -- Detect the cartridge reader on a given port.        */
/*                      Set the port to 0 to do an autodetect.              */
/* ------------------------------------------------------------------------ */
unsigned cr_detect(unsigned port);

/* ------------------------------------------------------------------------ */
/*  CR_SELFTEST      -- Perform a self-test on the cartridge reader.        */
/* ------------------------------------------------------------------------ */
int cr_selftest(cart_rd_t *cr, unsigned *w, unsigned *r);

/* ------------------------------------------------------------------------ */
/*  CR_SLEEP         -- Wrapper around nanosleep().                         */
/* ------------------------------------------------------------------------ */
void cr_sleep(long len);

/* ------------------------------------------------------------------------ */
/*  CR_DO_RESET      -- Pulse MSYNC low for awhile, then let it high.       */
/* ------------------------------------------------------------------------ */
void cr_do_reset(cart_rd_t *cr);

/* ------------------------------------------------------------------------ */
/*  CR_DO_READ       -- Do read via a BAR - NACT - DTB - NACT cycle         */
/* ------------------------------------------------------------------------ */
unsigned cr_do_read(cart_rd_t *cr, unsigned addr);

/* ------------------------------------------------------------------------ */
/*  CR_DO_WRITE      -- Do write via a BAR - NACT - DW - DWS - NACT cycle   */
/* ------------------------------------------------------------------------ */
void cr_do_write(cart_rd_t *cr, unsigned addr, unsigned data);


/* ------------------------------------------------------------------------ */
/*  CR_INIT_PORTS    -- Initialize port accesses, drop privs                */
/* ------------------------------------------------------------------------ */
void cr_init_ports(unsigned long base);
#endif /*_CART_H_*/

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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
