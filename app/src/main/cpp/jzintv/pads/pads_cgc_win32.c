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
#include "pads/pads_cgc_win32.h"

#ifdef CGC_DLL
/* ======================================================================== */
/*  CGC specific constants that should stay in this file only.              */
/* ======================================================================== */

enum
{
    CGC_OK = -100,
    ERR_GENERROR,
    ERR_THREAD,
    ERR_OPEN,
    ERR_PURGE,
    ERR_SEND,
    ERR_READ,
    ERR_TXTIMEOUT,
    ERR_RXTIMEOUT,
    ERR_SETBAUD,
    ERR_SETDATACHAR,
    ERR_SETFLOW,
    ERR_SETTIMEOUT,
    ERR_RESETDEV,
    ERR_CLOSE,
    ERR_ALREADYRUNNING
};

const uint8_t CONTROLLER_0   = 0;
const uint8_t CONTROLLER_1   = 1;
const uint8_t CONTROLLER_2   = 2;
const uint8_t CONTROLLER_3   = 3;
const uint8_t TYPE_INTY      = 0xA0;
const uint8_t TYPE_NES       = 0xA1;
const uint8_t TYPE_SNES      = 0xA2;
const uint8_t TYPE_ATARIJOY  = 0xA3;


/* ======================================================================== */
/*  WIN32 method for reading Classic Gaming Controller.                     */
/* ======================================================================== */
#include <windows.h>

typedef int f_cgc_open (uint8_t);
typedef int f_cgc_close(void);
typedef int f_cgc_read (uint8_t, uint16_t*);
typedef int f_cgc_error(int, char*, long);

LOCAL f_cgc_open  *cgc_open  = NULL;
LOCAL f_cgc_close *cgc_close = NULL;
LOCAL f_cgc_read  *cgc_read  = NULL;
LOCAL f_cgc_error *cgc_error = NULL;
LOCAL int did_cgc_init = 0;  /* 0 == need init, 1 == init, -1 == error */

LOCAL void *cgc_dll_handle = NULL;


LOCAL char cgc_err_buf[512];

/* ======================================================================== */
/*  INIT_CGC_DLL -- Look for the CGC's DLL and set up our CGC interface.    */
/* ======================================================================== */
LOCAL int init_cgc_dll()
{
    int ret;

    if (did_cgc_init)
        return did_cgc_init < 0 ? -1 : 0;

    /* -------------------------------------------------------------------- */
    /*  Attempt to open the CGC library.                                    */
    /* -------------------------------------------------------------------- */
    did_cgc_init = -1;  /* assume failed init until proven otherwise.       */

    cgc_dll_handle = LoadLibrary("cgc.dll");
    if (!cgc_dll_handle)
    {
        fprintf(stderr, "CGC Library 'cgc.dll' not found\n");
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Get function pointers for the four interface functions we need.     */
    /* -------------------------------------------------------------------- */
    cgc_open  = (f_cgc_open *)GetProcAddress(cgc_dll_handle,"CGCOpen");
    cgc_close = (f_cgc_close*)GetProcAddress(cgc_dll_handle,"CGCClose");
    cgc_read  = (f_cgc_read *)GetProcAddress(cgc_dll_handle,
                                             "CGCGetRawIntyData");
    cgc_error = (f_cgc_error*)GetProcAddress(cgc_dll_handle,
                                             "CGCGetErrorString");

    if (!cgc_open || !cgc_close || !cgc_read || !cgc_error)
    {
        fprintf(stderr,
                "init_cgc:  Couldn't get these functions from CGC.DLL:\n\n"
                "%s%s%s%s\n\n",
                !cgc_open  ? "    CGCOpen()\n":"",
                !cgc_close ? "    CGCClose()\n":"",
                !cgc_read  ? "    CGCGetRawIntyData()\n":"",
                !cgc_error ? "    CGCGetErrorString()\n":"");
        FreeLibrary(cgc_dll_handle);
        cgc_dll_handle = NULL;
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Now try to fire up the interface so we can talk!                    */
    /* -------------------------------------------------------------------- */
    if ((ret = cgc_open(TYPE_INTY)) != CGC_OK)
    {
        cgc_error(ret, cgc_err_buf, sizeof(cgc_err_buf));
        fprintf(stderr, "init_cgc:  Error initializing: %s\n", cgc_err_buf);
        cgc_close();

        FreeLibrary(cgc_dll_handle);
        cgc_dll_handle = NULL;
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Now that we've configured the CGC, report successful init!          */
    /* -------------------------------------------------------------------- */
    did_cgc_init = 1;

    return 0;
}

/* ======================================================================== */
/*  PAD_CGC_READ -- Returns the current state of the pads.                  */
/* ======================================================================== */
uint32_t pad_cgc_read(periph_t *p, periph_t *r, uint32_t a, uint32_t d)
{
    pad_cgc_t *const pad = PERIPH_AS(pad_cgc_t, p);
    int side = a & 1, ret;
    uint16_t value;

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

    /* -------------------------------------------------------------------- */
    /*  As long as this side is set to input, read from it.                 */
    /* -------------------------------------------------------------------- */
    value = 0x00FF;
    if (pad->io[side] == 0 && pad->num_errors < 5)
    {
        ret = cgc_read(side ? CONTROLLER_0 : CONTROLLER_1, &value);

        if (ret != CGC_OK)
        {
            cgc_error(ret, cgc_err_buf, sizeof(cgc_err_buf));
            fprintf(stderr, "cgc_tick:  Error reading CGC: %s\n",
                    cgc_err_buf);
            pad->num_errors++;
        }
    }
    return (value & 0xFF);
}

/* ======================================================================== */
/*  PAD_CGC_WRITE -- Looks for changes in I/O mode on PSG I/O ports.        */
/* ======================================================================== */
void pad_cgc_write(periph_t *p, periph_t *r, uint32_t a, uint32_t d)
{
    pad_cgc_t *const pad = PERIPH_AS(pad_cgc_t, p);

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
int pad_cgc_win32_init
(
    pad_cgc_t      *pad,            /*  pad_cgc_t structure to initialize   */
    uint32_t        addr,           /*  Base address of pad.                */
    int             cgc_num         /*  CGC number in system                */
)
{
    if (init_cgc_dll())
        return -1;

    pad->periph.read     = pad_cgc_read;
    pad->periph.write    = pad_cgc_write;
    pad->periph.peek     = pad_cgc_read;
    pad->periph.poke     = pad_cgc_write;
    pad->periph.tick     = NULL;
    pad->periph.min_tick = 0;
    pad->periph.max_tick = ~0U;
    pad->cgc_num         = cgc_num;

    jzp_printf("pads_cgc:  CGC #%d mapped to $%.4X-$%.4X\n",
            cgc_num, addr + 0xE, addr + 0xF);

    pad->periph.addr_base = addr;
    pad->periph.addr_mask = 0xF;

    pad->io  [0]          = 0;
    pad->io  [1]          = 0;

    return 0;
}
#else
int pad_cgc_win32_init
(
    pad_cgc_t      *pad,            /*  pad_cgc_t structure to initialize   */
    uint32_t        addr,           /*  Base address of pad.                */
    int             cgc_num         /*  CGC number in system                */
)
{
    UNUSED(pad);
    UNUSED(addr);
    UNUSED(cgc_num);

    return -1;
}

#endif /* CGC_DLL */


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

