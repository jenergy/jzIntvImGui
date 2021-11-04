/* ======================================================================== */
/*  Cartridge Reader Interface routines.                                    */
/*  By Joe Zbiciak                                                          */
/*  Using hardware designed by Scott Nudds                                  */
/*  (Based on routines from my own SUCK.BAS.)                               */
/* ======================================================================== */

#include "config.h"
#include "cart.h"

#if (!defined(PLAT_LINUX) && !defined(WIN32)) || !defined(i386)
void cr_set_disp(cart_rd_t *cr, unsigned display) { UNUSED(cr); UNUSED(display); }
void cr_set_ctrl(cart_rd_t *cr, unsigned control) { UNUSED(cr); UNUSED(control); }
void cr_set_data(cart_rd_t *cr, unsigned value) { UNUSED(cr); UNUSED(value); }
void cr_deselect(cart_rd_t *cr) { UNUSED(cr); }
int cr_loopback(cart_rd_t *cr, unsigned value, unsigned *rd)
{
    UNUSED(cr); UNUSED(value); UNUSED(rd);
    return -1;
}
unsigned cr_detect(unsigned port)
{
    UNUSED(port);
    return 0;
}
int cr_selftest(cart_rd_t *cr, unsigned *w, unsigned *r)
{
    UNUSED(cr); UNUSED(w); UNUSED(r);
    return -1;
}
void cr_sleep(long len) { UNUSED(len); }
void cr_do_reset(cart_rd_t *cr) { UNUSED(cr); }
unsigned cr_do_read(cart_rd_t *cr, unsigned addr)
{
    UNUSED(cr); UNUSED(addr);
    return 0;
}
void cr_do_write(cart_rd_t *cr, unsigned addr, unsigned data)
{
    UNUSED(cr); UNUSED(addr); UNUSED(data);
}
void cr_init_ports(unsigned long base) { UNUSED(base); }


#else

static unsigned long ports[3] = { 0x378, 0x278, 0x3BC }; /* Printer ports  */

#define cr_outb(x,y) outb(y,x)
#define cr_inb(x)    inb(x)

/* ------------------------------------------------------------------------ */
/*  CR_SET_DISP      -- Set the display register to an 8-bit value.         */
/* ------------------------------------------------------------------------ */
void cr_set_disp(cart_rd_t *cr, unsigned display)
{
    /* -------------------------------------------------------------------- */
    /*  Remember this display word.                                         */
    /* -------------------------------------------------------------------- */
    cr->disp = display & 0xFF;

    /* -------------------------------------------------------------------- */
    /*  Output the control word to the DISPLAY latch.                       */
    /* -------------------------------------------------------------------- */
    cr_outb(cr->port + 2, LAT_DEN | LAT_INH);
    cr_outb(cr->port,     display & 0xFF);
    cr_outb(cr->port + 2, LAT_DEN);
    cr_outb(cr->port + 2, LAT_DEN | LAT_INH);
    cr_outb(cr->port + 2, LAT_INH);
}

/* ------------------------------------------------------------------------ */
/*  CR_SET_CTRL      -- Set the bus control lines to a given state.         */
/* ------------------------------------------------------------------------ */
void cr_set_ctrl(cart_rd_t *cr, unsigned control)
{
    /* -------------------------------------------------------------------- */
    /*  Remember this control word.                                         */
    /* -------------------------------------------------------------------- */
    cr->ctrl = (control & ~DS9) | (cr->seg9 ? DS9 : 0);

    /* -------------------------------------------------------------------- */
    /*  Output the control word to the CONTROL latch.                       */
    /* -------------------------------------------------------------------- */
    cr_outb(cr->port + 2, LAT_CEN | LAT_INH);
    cr_outb(cr->port,     control & 0xFF);
    cr_outb(cr->port + 2, LAT_CEN);
    cr_outb(cr->port + 2, LAT_CEN | LAT_INH);
    cr_outb(cr->port + 2, LAT_INH);
}

/* ------------------------------------------------------------------------ */
/*  CR_SET_DATA      -- Set the data bus lines to a given value.            */
/* ------------------------------------------------------------------------ */
void cr_set_data(cart_rd_t *cr, unsigned value)
{
    /* -------------------------------------------------------------------- */
    /*  Output the high-half of the value.                                  */
    /* -------------------------------------------------------------------- */
    cr_outb(cr->port + 2, LAT_XEN | LAT_INH);
    cr_outb(cr->port,     (value >> 8) & 0xFF);
    cr_outb(cr->port + 2, LAT_XEN);
    cr_outb(cr->port + 2, LAT_XEN | LAT_INH);

    /* -------------------------------------------------------------------- */
    /*  Output the low-half of the value.                                   */
    /* -------------------------------------------------------------------- */
    cr_outb(cr->port + 2, LAT_YEN | LAT_INH);
    cr_outb(cr->port,     value & 0xFF);
    cr_outb(cr->port + 2, LAT_YEN);
    cr_outb(cr->port + 2, LAT_YEN | LAT_INH);
}

/* ------------------------------------------------------------------------ */
/*  CR_GET_DATA      -- Read the current values that are on the data bus.   */
/* ------------------------------------------------------------------------ */
unsigned cr_get_data(cart_rd_t *cr)
{
    int i;
    unsigned data = 0, temp;
    unsigned old_ctrl = cr->ctrl;
    unsigned ctrl = (cr->ctrl | SL | OC | CLK);
    ctrl     = (    ctrl & ~DS9) | (cr->seg9 ? DS9 : 0);
    old_ctrl = (old_ctrl & ~DS9) | (cr->seg9 ? DS9 : 0);

    /* -------------------------------------------------------------------- */
    /*  Select the CONTROL latch.                                           */
    /* -------------------------------------------------------------------- */
    cr_outb(cr->port + 2, LAT_CEN | LAT_INH);
    cr_outb(cr->port,     ctrl);
    cr_outb(cr->port + 2, LAT_CEN);

    for (i = 0; i < 8; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Set the control latch directly (done this way for speed)        */
        /* ---------------------------------------------------------------- */
        cr_outb(cr->port, ctrl      );
        cr_outb(cr->port, ctrl ^ CLK);

        /* ---------------------------------------------------------------- */
        /*  Read the shift registers, and merge the bits into our data.     */
        /* ---------------------------------------------------------------- */
        temp = cr_inb(cr->port + 1);
        data <<= 1;
        if ((temp & 0x40) != 0) data |= 0x0001;
        if ((temp & 0x80) == 0) data |= 0x0100;
    }

    /* -------------------------------------------------------------------- */
    /*  Restore the control word and deselect the CONTROL latch.            */
    /* -------------------------------------------------------------------- */
    cr_outb(cr->port, cr->ctrl = old_ctrl);

    return data;
}

/* ------------------------------------------------------------------------ */
/*  CR_DESELECT      -- Make sure the Cartridge Reader is deselected.       */
/* ------------------------------------------------------------------------ */
void cr_deselect(cart_rd_t *cr)
{
    cr_outb(cr->port + 2, LAT_INH);    /* Inhibit all latches.              */
}

/* ------------------------------------------------------------------------ */
/*  CR_LOOPBACK      -- Writes, then reads, a given value on the bus.       */
/*                      Returns whether the value read matched what was     */
/*                      actually written.  (0 == Match, 1 == Mismatch.)     */
/* ------------------------------------------------------------------------ */
int cr_loopback(cart_rd_t *cr, unsigned value, unsigned *rd)
{
    unsigned read_back;
    value &= 0xFFFF;                /* limit to 16 bits (our bus width)     */
    cr_set_ctrl(cr, CTRL(NACT,0));  /* go to NACT phase during loopback     */
    cr_set_data(cr, value);         /* write value                          */
    read_back = cr_get_data(cr);
    if (rd) *rd = read_back;

    return read_back != value;                      /* Read back and test   */
}

/* ------------------------------------------------------------------------ */
/*  CR_DETECT        -- Detect the cartridge reader on a given port.        */
/*                      Set the port to 0 to do an autodetect.              */
/* ------------------------------------------------------------------------ */
unsigned cr_detect(unsigned port)
{
    unsigned patt[4] = { 0x0000, 0xFFFF, 0xAA55, 0x55AA };
    int i = 0, j, rd;
    cart_rd_t try;

    memset(&try, 0, sizeof(try));

    if (port) i = 4;
    do
    {
        if (!port) port = ports[i];
        printf("> Scanning for cartridge reader at port $%.4X\n", port);

        try.port = port;

        cr_set_disp(&try, 255);

        for (j = 0; j < 4; j++) if (cr_loopback(&try, patt[j], &rd)) break;
        cr_deselect(&try);


        if (j > 0 && j != 4)
        {
            printf("> Partial detect on port $%.4X: "
                   "Wrote %.4X, read %.4X\n", port, patt[j], rd);
        }

        if (j == 4)
        {
            cr_set_disp(&try, 0);
            cr_set_ctrl(&try, CTRL(NACT, 0));
            cr_set_data(&try, 0x0000);
            cr_deselect(&try);

            break;
        } else
            port = 0;
    } while (++i < 3);

    if (port)   printf("> Found cartridge reader at port $%.4X\n", port);
    else        printf("> No cartridge reader found!\n");

    return port;
}

/* ------------------------------------------------------------------------ */
/*  CR_SELFTEST      -- Perform a self-test on the cartridge reader.        */
/* ------------------------------------------------------------------------ */
int cr_selftest(cart_rd_t *cr, unsigned *w, unsigned *r)
{
    unsigned value, xvalue, rd;

    cr_set_ctrl(cr, CTRL(NACT, DS9));
    for (value = 0x0000; value <= 0xFFFF; value++)
    {
        if ((value & 0xFF) == 0)
        {
            cr_set_disp(cr, (value >> 8) ^ 0xFF);
            cr_deselect(cr);
        }
        xvalue = (0x00FF & (value >> 8)) | (0xFF00 & (value << 8));
        if (cr_loopback(cr, value,  &rd))
        {
            *w = value;
            *r = rd;
            return -1;
        }
        if (cr_loopback(cr, xvalue, &rd))
        {
            *w = xvalue;
            *r = rd;
            return -1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------ */
/*  CR_SLEEP         -- Wrapper around nanosleep().                         */
/* ------------------------------------------------------------------------ */
inline void cr_sleep(long len)
{
#ifndef NO_NANOSLEEP
    struct timespec delay, remain;

    delay.tv_sec  = 0;
    delay.tv_nsec = len * 1000000;

    while (nanosleep(&delay, &remain) == -1 && errno == EINTR)
    {
        delay = remain;
        if (remain.tv_nsec < 10000) break;
    }
#else
    struct timeval x, y;
    long diff = 0;

    gettimeofday(&x, 0);
    while (diff < len)
    {
        gettimeofday(&y, 0);
        diff = (y.tv_sec  - x.tv_sec ) * 1000000 +
               (y.tv_usec - x.tv_usec);
    }
#endif
    return;
}


/* ------------------------------------------------------------------------ */
/*  CR_DO_RESET      -- Pulse MSYNC low for awhile, then let it high.       */
/* ------------------------------------------------------------------------ */
void cr_do_reset(cart_rd_t *cr)
{
    unsigned cur_disp = cr->disp;

    cr_set_ctrl(cr, CTRL(RESET, cr->ctrl));     /* Go into RESET.           */
    cr_set_disp(cr, 0);

    if (cr->reset_delay)
        cr_sleep(cr->reset_delay);

    cr_set_ctrl(cr, CTRL(NACT,  cr->ctrl));     /* Go into NACT.            */
    cr_set_disp(cr, cur_disp);
}

/* ------------------------------------------------------------------------ */
/*  CR_DO_READ       -- Do read via a BAR - NACT - DTB - NACT cycle         */
/* ------------------------------------------------------------------------ */
unsigned cr_do_read(cart_rd_t *cr, unsigned addr)
{
    unsigned data;

    cr_set_ctrl(cr, CTRL(NACT, OC));    /* Force NACT initially     */
    cr_set_data(cr, addr);              /* Send out the address     */
    cr_set_ctrl(cr, CTRL(BAR,   0));    /* Enter BAR phase          */
    cr_set_ctrl(cr, CTRL(NACT,  0));    /* Enter NACT phase         */
    cr_set_ctrl(cr, CTRL(NACT, OC));    /* Let address fade         */
    cr_set_ctrl(cr, CTRL(DTB,  OC));    /* Enter DTB phase          */
    cr_set_ctrl(cr, cr->ctrl |  CLK);   /* Latch data during DTB    */
    cr_set_ctrl(cr, cr->ctrl & ~CLK);   /* (Second half of clock)   */
    data = cr_get_data(cr);             /* Read the shift regs.     */
    cr_set_ctrl(cr, CTRL(NACT, OC));    /* End in the NACT phase.   */
    cr_deselect(cr);                    /* Deselect the device.     */

    return data;
}

/* ------------------------------------------------------------------------ */
/*  CR_DO_WRITE      -- Do write via a BAR - NACT - DW - DWS - NACT cycle   */
/* ------------------------------------------------------------------------ */
void cr_do_write(cart_rd_t *cr, unsigned addr, unsigned data)
{
    cr_set_ctrl(cr, CTRL(NACT, OC));    /* Force NACT initially     */
    cr_set_data(cr, addr);              /* Send out the address     */
    cr_set_ctrl(cr, CTRL(BAR,   0));    /* Enter BAR phase          */
    cr_set_ctrl(cr, CTRL(NACT,  0));    /* Enter NACT phase         */
    cr_set_ctrl(cr, CTRL(NACT, OC));    /* Let address fade         */
    cr_set_data(cr, data);              /* Send out the write data  */
    cr_set_ctrl(cr, CTRL(NACT,  0));    /* Let data be asserted     */
    cr_set_ctrl(cr, CTRL(DW,    0));    /* Enter DW phase           */
    cr_set_ctrl(cr, CTRL(DWS,   0));    /* Enter DWS phase          */
    cr_set_ctrl(cr, CTRL(NACT, OC));    /* End in the NACT phase.   */
    cr_deselect(cr);                    /* Deselect the device.     */
}


/* ======================================================================== */
/*  CR_INIT_PORTS    -- Get access to the desired I/O ports and drop        */
/*                      root.  Don't bother returning an error if we fail.  */
/*                      Just abort.                                         */
/* ======================================================================== */
void cr_init_ports(unsigned long base)
{
    int i;

    /* -------------------------------------------------------------------- */
    /*  First, sanity check the port number.  If it is not one of the       */
    /*  standard printer port #'s, then abort.                              */
    /* -------------------------------------------------------------------- */
    if (base && base != 0x378 && base != 0x278 && base != 0x3BC)
    {
        fprintf(stderr, "cr_init_ports:  Invalid base address 0x%.4lX\n", base);
        exit(1);
    }

#ifndef NO_SETUID
    if (base)
    {
        /* ---------------------------------------------------------------- */
        /*  Grant ourself perms to access ports 'base' through 'base+2'.    */
        /* ---------------------------------------------------------------- */
        if (ioperm(base, 3, 1))
        {
            fprintf(stderr, "cr_init_ports:  Unable to set I/O permissions\n");
            perror("cr_init_ports: ioperm()");
            exit(1);
        }
        printf("> Granted access to $%.4X (%d)\n", (int)base, (int)base);
    } else
    {
        /* ---------------------------------------------------------------- */
        /*  Grant ourself perms to access ports 'base' through 'base+2'.    */
        /* ---------------------------------------------------------------- */
        for (i = 0; i < 3; i++)
        {
            if (ioperm(ports[i], 3, 1))
            {
                fprintf(stderr, "cr_init_ports: "
                                " Unable to set I/O permissions\n");
                perror("cr_init_ports: ioperm()");
                exit(1);
            }
            printf("> Granted access to $%.4X (%d)\n",
                   (int)ports[i], (int)ports[i]);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Drop elevated privs if we have them.                                */
    /* -------------------------------------------------------------------- */
    if (getegid() != getgid()) setgid(getgid());
    if (geteuid() != getuid()) setuid(getuid());
#endif

    return;
}

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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
