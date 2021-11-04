/* ======================================================================== */
/*  EMU_LINK                                                                */
/*  Allows programs running inside the emulator to invoke emulator-specific */
/*  functionality.  jzIntv performs this task via the SIN instruction.      */
/*                                                                          */
/*  The process is simple:                                                  */
/*   -- Program loads $4A5A into R0 as a magic signature                    */
/*   -- Program loads EMU_LINK call # into R1                               */
/*   -- Program puts whatever arguments in R2..R5                           */
/*   -- Program issues SIN                                                  */
/*   -- Emulator checks R0 for signature and R1 for call #                  */
/*       -- If no signature, treat SIN as NOP                               */
/*       -- If signature OK but call # invalid, set C=1, R0=FFFF            */
/*       -- If signature OK and call # valid, set C=0 and execute call      */
/*   -- Callee executes, returning pass/fail in C, result in R0..R5.        */
/*                                                                          */
/*  Programs can quickly detect EMU_LINK support like so:                   */
/*   -- R0 = $4A5A, R1 = 0, SIN                                             */
/*   -- If C=0, R0=0, then EMU_LINK supported                               */
/*   -- Otherwise, it's not.                                                */
/* ======================================================================== */

#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/emu_link.h"


static emu_link_api_t **emu_link_api = NULL;
static void **emu_link_opq = NULL;
static int emu_link_api_cnt = 0, emu_link_api_alloc = 0;

/* ======================================================================== */
/*  EMU_LINK_PING -- Simple API for presence detect.                        */
/* ======================================================================== */
LOCAL int emu_link_ping(cp1600_t *cpu, int *fail, void *opaque)
{
    UNUSED(cpu);
    UNUSED(opaque);
    *fail = 0;
    return 0;
}

/* ======================================================================== */
/*  EMU_LINK_INIT -- Initialize EMU_LINK subsystem.                         */
/* ======================================================================== */
int emu_link_init(void)
{
    emu_link_api        = CALLOC(emu_link_api_t *, 16);
    emu_link_api_cnt    = 1;
    emu_link_api_alloc  = 16;

    if (!emu_link_api)
    {
        fprintf(stderr, "emu_link: Out of memory\n");
        return -1;
    }

    emu_link_api[0]     = emu_link_ping;

    return 0;
}

/* ======================================================================== */
/*  EMU_LINK_REGISTER -- Register an API with EMU_LINK                      */
/* ======================================================================== */
int emu_link_register(emu_link_api_t *fn, int callno, void *opaque)
{
    int old_alloc = emu_link_api_alloc;
    int i;

    while (emu_link_api_alloc <= callno)
        emu_link_api_alloc <<= 1;

    emu_link_api = REALLOC(emu_link_api, emu_link_api_t *, emu_link_api_alloc);
    emu_link_opq = REALLOC(emu_link_opq, void *, emu_link_api_alloc);

    if (!emu_link_api || !emu_link_opq)
    {
        CONDFREE(emu_link_api);
        CONDFREE(emu_link_opq);
        fprintf(stderr, "emu_link: Out of memory\n");
        return -1;
    }

    for (i = old_alloc; i < emu_link_api_alloc; i++)
    {
        emu_link_api[i] = NULL;
        emu_link_opq[i] = NULL;
    }

    if (emu_link_api[callno] != NULL)
        fprintf(stderr, "emu_link: Warning: API %d reassigned\n", callno);

    emu_link_api[callno] = fn;
    emu_link_opq[callno] = opaque;

    if (callno > emu_link_api_cnt)
        emu_link_api_cnt = callno;

    return 0;
}

/* ======================================================================== */
/*  EMU_LINK_DISPATCH -- Dispatch to an EMU_LINK API                        */
/* ======================================================================== */
void emu_link_dispatch(cp1600_t *cpu)
{
    int fail = 0;

    if (cpu->r[0] != 0x4A5A)
        return;

/*Dprintf(("\nEMU-LINK %.4X %.4X %.4X\n", cpu->r[1], cpu->r[2], cpu->r[3]));*/
    if (cpu->r[1] > emu_link_api_cnt ||
        emu_link_api[cpu->r[1]] == NULL)
    {
        /*Dprintf(("EMU-LINK Invalid API\n"));*/
        cpu->C = 1;
        cpu->r[0] = 0xFFFF;
        return;
    }

    cpu->r[0] = emu_link_api[cpu->r[1]](cpu, &fail, emu_link_opq[cpu->r[1]]);
    /*Dprintf(("EMU-LINK Result:  %.4X %d\n", cpu->r[0], fail));*/

    cpu->C = fail != 0;
    return;
}

/* ======================================================================== */
/*  EMU_LINK_DTOR    -- Shut down emu_link                                  */
/* ======================================================================== */
void emu_link_dtor(void)
{
    CONDFREE(emu_link_api);
    emu_link_api_cnt   = 0;
    emu_link_api_alloc = 0;
}

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
/*                   Copyright (c) 2005, Joseph Zbiciak                     */
/* ======================================================================== */

