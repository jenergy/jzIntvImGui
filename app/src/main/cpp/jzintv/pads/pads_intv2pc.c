/*
 * ============================================================================
 *  Title:    Controller pads
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements the controller pads.
 *  Pads are peripherals that extend periph_t.
 * ============================================================================
 */


#include "config.h"
#include "periph/periph.h"
#include "pads/pads_intv2pc.h"
#include "event/event.h"

#ifdef DIRECT_INTV2PC

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#ifndef WIN32
# include <sys/io.h>
#endif

int pads_intv2pc_ports_ok = 0;

/*
 * ===========================================================================
 *  INIT_PORTS   -- Get access to the desired I/O ports and drop root.
 *                  Don't bother returning an error if we fail.  Just abort.
 * ===========================================================================
 */
LOCAL int init_ports(uint32_t base)
{
    int bit = 0;

    /* -------------------------------------------------------------------- */
    /*  Convert the I/O base into a bitfield bit number.                    */
    /* -------------------------------------------------------------------- */
    switch (base)
    {
        case 0x378: bit = 1; break;
        case 0x278: bit = 2; break;
        case 0x3BC: bit = 4; break;
        default:    bit = 0; break;
    }

    /* -------------------------------------------------------------------- */
    /*  Complain if this seems to be an invalid port number.                */
    /* -------------------------------------------------------------------- */
    if (!bit)
    {
        fprintf(stderr, "direct_intv2pc:  Invalid base address 0x%.4X\n", base);
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  See if we were able to get permissions on the requested I/O port.   */
    /* -------------------------------------------------------------------- */
    if ((bit & pads_intv2pc_ports_ok) == 0)
    {
        fprintf(stderr, "direct_intv2pc:  Unable to set I/O permissions\n");
        return -1;
    }

    return 0;
}

/*
 * ============================================================================
 *  PAD_INTV2PC_TICK -- Reads from a dummy pad
 * ============================================================================
 */
uint32_t pad_intv2pc_tick(periph_t *p, uint32_t len)
{
    pad_intv2pc_t *const pad = PERIPH_AS(pad_intv2pc_t, p);
    uint32_t base  = pad->io_base;
    int      state = pad->rd_state++ & 7;
    int      side  = (state & 4) == 4;
    uint32_t val   = pad->rd_val;

    /* -------------------------------------------------------------------- */
    /*  Synchronous delays while we read the INTV2PC are bad.  Instead, we  */
    /*  have a four-phase state machine that moves between states at a low  */
    /*  enough rate while letting the rest of the emulator run in the       */
    /*  meantime.                                                           */
    /*                                                                      */
    /*  The state machine has four phases, and alternates between left and  */
    /*  right hand-controllers for a total of eight unique phases.          */
    /* -------------------------------------------------------------------- */
    switch (state & 3)
    {
        /* ---------------------------------------------------------------- */
        /*  Get low nybble.                                                 */
        /* ---------------------------------------------------------------- */
        case 0:
            /* ------------------------------------------------------------ */
            /*  Phase 0:  Select low-nybble for this controller side.       */
            /* ------------------------------------------------------------ */
            outb((2<<side)|1, base);
            break;

        case 1:
            /* ------------------------------------------------------------ */
            /*  Phase 1:  Read it back and mask it.                         */
            /* ------------------------------------------------------------ */
            val  = (inb(base + 1) >> 4) & 0x0F;
            break;

        /* ---------------------------------------------------------------- */
        /*  Get high nybble.                                                */
        /* ---------------------------------------------------------------- */
        case 2:
            /* ------------------------------------------------------------ */
            /*  Phase 2:  Select high-nybble for this controller side.      */
            /* ------------------------------------------------------------ */
            outb((2<<side)|0, base);
            break;

        case 3:
            /* ------------------------------------------------------------ */
            /*  Phase 3:  Read it back, mask & merge it.  Then update the   */
            /*            controller's value in our pad_intv2pc_t struct.   */
            /* ------------------------------------------------------------ */
            val |= (inb(base + 1)     ) & 0xF0;

            if (pad->io[side] == 0) pad->side[side] = val ^ 0x88;
            break;

        /* ---------------------------------------------------------------- */
        /*  Uhoh, something bad happened.                                   */
        /* ---------------------------------------------------------------- */
        default:
            fprintf(stderr, "Can't get here in pad_intv2pc_tick()\n");
            exit(1);
    }

    pad->rd_val = val;

    return len;
}


/* ======================================================================== */
/*  PAD_INTV2PC_READ -- Returns the current state of the pads.              */
/* ======================================================================== */
uint32_t pad_intv2pc_read(periph_t *p, periph_t *r, uint32_t a, uint32_t d)
{
    pad_intv2pc_t *const pad = PERIPH_AS(pad_intv2pc_t, p);

    UNUSED(r);
    UNUSED(d);

    /* -------------------------------------------------------------------- */
    /*  Ignore accesses that are outside our address space.                 */
    /* -------------------------------------------------------------------- */
    if (a < 14) return ~0U;

    /* -------------------------------------------------------------------- */
    /*  Ignore reads to ports config'd as output.  INTV2PC is input only.   */
    /* -------------------------------------------------------------------- */
    if (pad->io[a & 1]) return ~0U;

    return (pad->side[a & 1] & 0xFF);
}

/*
 * ============================================================================
 *  PAD_INTV2PC_WRITE    -- Looks for changes in I/O mode on PSG I/O ports.
 * ============================================================================
 */
void pad_intv2pc_write(periph_t *p, periph_t *r, uint32_t a, uint32_t d)
{
    pad_intv2pc_t *const pad = PERIPH_AS(pad_intv2pc_t, p);

    UNUSED(r);

    /* -------------------------------------------------------------------- */
    /*  Capture writes to the 'control' register in the PSG, looking for    */
    /*  I/O direction setup.                                                */
    /* -------------------------------------------------------------------- */
    if (a == 8)
    {
        int io_0 = (d >> 6) & 1;
        int io_1 = (d >> 7) & 1;

        pad->io[0] = io_0;
        pad->io[1] = io_1;
    }

    return;
}

/* ======================================================================== */
/*  PAD_INIT     -- Makes a dummy pad                                       */
/* ======================================================================== */
int pad_intv2pc_init
(
    pad_intv2pc_t  *pad,            /*  mem_t structure to initialize       */
    uint32_t        addr,           /*  Base address of pad.                */
    uint32_t        io_base         /*  I/O address of INTV2PC, or 0.       */
)
{
    if (io_base > 0)
    {
        /* ---------------------------------------------------------------- */
        /*  Note:  This will only really work reliably if the emulator is   */
        /*  running around 100% or slower on an otherwise unloaded machine. */
        /*  I really need to check the _actual_ elapsed time in the _tick   */
        /*  function since sudden bursts of reads might cause _tick to be   */
        /*  called too quickly in succession.                               */
        /* ---------------------------------------------------------------- */
        if (init_ports(io_base))
            return -1;

        pad->periph.read     = pad_intv2pc_read;
        pad->periph.write    = pad_intv2pc_write;
        pad->periph.peek     = pad_intv2pc_read;
        pad->periph.poke     = pad_intv2pc_write;
        pad->periph.tick     = pad_intv2pc_tick;
        pad->periph.min_tick = 3579545 / (4*4*480); /* 480Hz scanning rate. */
        pad->periph.max_tick = 3579545 / (4*4*480); /* (times 4 states)     */

        pad->io_base         = io_base;
    } else
    {
        /* ---------------------------------------------------------------- */
        /*  No I/O base, no INTV2PC.  We're not going to autodetect.        */
        /* ---------------------------------------------------------------- */
        fprintf(stderr, "ERROR:  INTV2PC with no io_base?\n");
        return -1;
    }
    jzp_printf("pads_intv2pc:  INTV2PC on port 0x%.3X mapped to $%.4X-$%.4X\n",
            io_base, addr + 0xE, addr + 0xF);

    pad->periph.addr_base = addr;
    pad->periph.addr_mask = 0xF;

    pad->rd_state         = 0;
    pad->rd_val           = 0;
    pad->side[0]          = 0xFF;
    pad->side[1]          = 0xFF;
    pad->io  [0]          = 0;
    pad->io  [1]          = 0;

    return 0;
}
#else

int pad_intv2pc_init
(
    pad_intv2pc_t  *pad,            /*  mem_t structure to initialize       */
    uint32_t        addr,           /*  Base address of pad.                */
    uint32_t        io_base         /*  I/O address of INTV2PC, or 0.       */
)
{
    UNUSED(pad); UNUSED(addr); UNUSED(io_base);

    fprintf(stderr, "Error:  INTV2PC support is not compiled in.\n");
    return -1;
}
#endif /* DIRECT_INTV2PC */


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

