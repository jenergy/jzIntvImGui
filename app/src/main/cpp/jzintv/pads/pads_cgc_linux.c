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


#include "config.h"
#include "periph/periph.h"
#include "pads/pads_cgc.h"
#include "pads/pads_cgc_linux.h"

#ifdef CGC_THREAD

#include "sdl_jzintv.h"
#include <termios.h>
#include <fcntl.h>

#define MAX_CGC (4)

static pad_cgc_t  *cgc_struct[MAX_CGC];
static SDL_Thread *cgc_thread[MAX_CGC];
static int         cgc_threads = 0;

/* ======================================================================== */
/*  PAD_CGC_SCANNER -- scan the CGC periodically from a separate thread.    */
/* ======================================================================== */
LOCAL int pad_cgc_scanner(void *opaque)
{
    pad_cgc_t *pad = (pad_cgc_t *)opaque;
    char i_byte[2], o_byte[2];
    int scans = 0, start, end;

    start = SDL_GetTicks();
    o_byte[0] = 3;
    o_byte[1] = 2;
    i_byte[0] = i_byte[1] = 0xFF;

    /* -------------------------------------------------------------------- */
    /*  Repeatedly read the CGC until we're asked to die.                   */
    /* -------------------------------------------------------------------- */
    while (!pad->die)
    {
        /* ---------------------------------------------------------------- */
        /*  Protocol is simple:                                             */
        /*   -- Write '3', read back right side.                            */
        /*   -- Write '2', read back left side.                             */
        /* ---------------------------------------------------------------- */
        scans++;

        tcflush(pad->fd, TCIOFLUSH);

        if (write(pad->fd, &o_byte[0], 1) != 1) goto error;
        if (read (pad->fd, &i_byte[0], 1) != 1) goto error;
        if (write(pad->fd, &o_byte[1], 1) != 1) goto error;
        if (read (pad->fd, &i_byte[1], 1) != 1) goto error;

        pad->val[0] = i_byte[0];    // right side
        pad->val[1] = i_byte[1];    // left side

        if (pad->die)
            break;

        continue;

error:
        /* ------------------------------------------------------------ */
        /*  If for some reason we get errors on the reads or writes,    */
        /*  record them.  IF a cascade error happens, abort.            */
        /* ------------------------------------------------------------ */
        perror("pad_cgc");
        fprintf(stderr, "pad_cgc: Error reading CGC fd %d\n", pad->fd);
        pad->num_errors++;

        if (pad->num_errors > 5)
        {
            fprintf(stderr, "pad_cgc: Too many errors:  Exiting thread\n");
            return -1;
        }
    }
    end = SDL_GetTicks();

#if 1
    jzp_flush();
    jzp_printf("\npad_cgc_scanner:  "
           "Approx scanning rate for fd %d:  %7.2fHz / ctrlr\n",
           pad->fd, 1000.0 * scans / (end - start));
    jzp_flush();
#endif

    return 0;
}

/* ======================================================================== */
/*  PAD_CGC_REAPER -- Ask all the CGC worker threads to die, politely.      */
/* ======================================================================== */
LOCAL void pad_cgc_reaper(periph_t *const unused)
{
    int i;

    UNUSED(unused);

    jzp_printf("\nReaping CGC threads...\n");
    jzp_flush();

    for (i = 0; i < cgc_threads; i++)
        cgc_struct[i]->die = 1;

    for (i = 0; i < cgc_threads; i++)
        SDL_WaitThread(cgc_thread[i], NULL);
}



/* ======================================================================== */
/*  PAD_CGC_READ -- Returns the current state of the pads.                  */
/* ======================================================================== */
uint32_t pad_cgc_read(periph_t *p, periph_t *r, uint32_t a, uint32_t d)
{
    pad_cgc_t *pad = PERIPH_AS(pad_cgc_t, p);
    int side = a & 1;
    uint16_t value;

    UNUSED(r);
    UNUSED(d);

    /* -------------------------------------------------------------------- */
    /*  Ignore accesses that are outside our address space.                 */
    /* -------------------------------------------------------------------- */
    if (a < 14) return ~0U;

    /* -------------------------------------------------------------------- */
    /*  Ignore reads to ports config'd as output.  CGC is input only.       */
    /* -------------------------------------------------------------------- */
    if (pad->io[a & 1]) return ~0U;

    /* -------------------------------------------------------------------- */
    /*  As long as this side is set to input, read from it.                 */
    /* -------------------------------------------------------------------- */
    value = 0x00FF;
    if (pad->io[side] == 0 && pad->num_errors < 5)
        value = pad->val[side];

    return (value & 0xFF);
}

/* ======================================================================== */
/*  PAD_CGC_WRITE -- Looks for changes in I/O mode on PSG I/O ports.        */
/* ======================================================================== */
void pad_cgc_write(periph_t *p, periph_t *r, uint32_t a, uint32_t d)
{
    pad_cgc_t *pad = PERIPH_AS(pad_cgc_t, p);

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
/*  PAD_CGC_INIT -- Initializes a Classic Gaming Controller interface.      */
/* ======================================================================== */
int pad_cgc_linux_init
(
    pad_cgc_t      *pad,            /*  pad_cgc_t structure to initialize   */
    uint32_t        addr,           /*  Base address of pad.                */
    const char     *cgc_dev         /*  path to CGC device.                 */
)
{
    static int reaper = 0;
    SDL_Thread *th;
    struct termios tio;
    int fd, i;
    char o_byte, i_byte = 0;

    /* -------------------------------------------------------------------- */
    /*  Make sure we don't have too many CGC's registered.                  */
    /* -------------------------------------------------------------------- */
    if (cgc_threads == MAX_CGC)
    {
        fprintf(stderr, "Too many CGCs registered!\n");
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Establish descriptor to the device node.                            */
    /* -------------------------------------------------------------------- */
    fd = open(cgc_dev, O_RDWR);

    if (fd < 0)
    {
        perror("open()");
        fprintf(stderr, "Could not open CGC device \"%s\".\n", cgc_dev);
        return -1;
    }

    pad->fd = fd;

    /* -------------------------------------------------------------------- */
    /*  Ugh.  CGC is over a tty, so we need to set the terminal attribs.    */
    /* -------------------------------------------------------------------- */
    if (tcgetattr(fd, &tio))
    {
        perror("tcgetattr()");
        fprintf(stderr, "Could not control CGC device \"%s\".\n", cgc_dev);
        return -1;
    }

    /*
    tio.c_iflag = IGNBRK | IGNPAR | IGNCR;
    tio.c_oflag = 0;
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_lflag = NOFLSH;
    tio.c_cc[VINTR   ] = -1;
    tio.c_cc[VQUIT   ] = -1;
    tio.c_cc[VERASE  ] = -1;
    tio.c_cc[VKILL   ] = -1;
    tio.c_cc[VEOF    ] = -1;
    tio.c_cc[VEOL    ] = -1;
    tio.c_cc[VEOL2   ] = -1;
    tio.c_cc[VSUSP   ] = -1;
    tio.c_cc[VLNEXT  ] = -1;
    tio.c_cc[VWERASE ] = -1;
    tio.c_cc[VDISCARD] = -1;
    */
    cfmakeraw(&tio);
    tio.c_cc[VMIN    ] = 1;
    tio.c_cc[VTIME   ] = 0;
    tio.c_cflag |= CRTSCTS;

    if (cfsetispeed(&tio, B9600) ||
        cfsetospeed(&tio, B9600) ||
        tcsetattr(fd, TCSANOW, &tio))
    {
        perror("tcsetattr()");
        fprintf(stderr, "Could not control CGC device \"%s\".\n", cgc_dev);
        return -1;
    }

    tcflow(fd, TCOON);
    tcflow(fd, TCION);

    tcflush(fd, TCOFLUSH);
    tcflush(fd, TCIFLUSH);

    /* -------------------------------------------------------------------- */
    /*  Synchronize with the CGC.                                           */
    /* -------------------------------------------------------------------- */
    jzp_printf("pads_cgc:  Synchronizing with \"%s\" (fd %d)\n", cgc_dev, fd);
    jzp_flush();
    o_byte = 0;
    for (i = 0; i < 10; i++)
    {
        tcflush(pad->fd, TCIOFLUSH);
        if (write(fd, &o_byte, 1) != 1 ||
            read (fd, &i_byte, 1) != 1)
        {
            fprintf(stderr, "Could not synchronize with CGC %s\n", cgc_dev);
            return -1;
        }
    }
    if (i_byte != 0x52)
    {
        fprintf(stderr, "Unexpected sync byte %.2X synchronizing with %s\n",
                0xFF & i_byte, cgc_dev);
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Establish a scanner thread.                                         */
    /* -------------------------------------------------------------------- */
#ifndef USE_SDL2
    th = SDL_CreateThread(pad_cgc_scanner, (void*)pad);
#else
    th = SDL_CreateThread(pad_cgc_scanner, "jzintv CGC scanner", (void*)pad);
#endif

    if (!th)
    {
        fprintf(stderr, "Could not fork CGC scanning thread.\n");
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Remember the scanning thread details for later reaping.             */
    /* -------------------------------------------------------------------- */
    cgc_struct[cgc_threads  ] = pad;
    cgc_thread[cgc_threads++] = th;

    /* -------------------------------------------------------------------- */
    /*  Set up the emulator "peripheral."                                   */
    /* -------------------------------------------------------------------- */
    pad->periph.read      = pad_cgc_read;
    pad->periph.write     = pad_cgc_write;
    pad->periph.peek      = pad_cgc_read;
    pad->periph.poke      = pad_cgc_write;
    pad->periph.dtor      = reaper ? NULL 
                                   : ((void)(reaper = 1), pad_cgc_reaper);
    pad->periph.tick      = NULL;
    pad->periph.min_tick  = 0;
    pad->periph.max_tick  = ~0U;

    jzp_printf("pads_cgc:  CGC @ \"%s\" (fd %d) mapped to $%.4X-$%.4X\n",
            cgc_dev, fd, addr + 0xE, addr + 0xF);

    pad->periph.addr_base = addr;
    pad->periph.addr_mask = 0xF;

    pad->io [0]           = 0;
    pad->io [1]           = 0;
    pad->val[0]           = 0xFF;
    pad->val[1]           = 0xFF;
    return 0;
}
#endif /* CGC_THREAD */


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

