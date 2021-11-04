/*
 * ============================================================================
 *  Title:    Simple Debugger
 *  Author:   J. Zbiciak
 * ============================================================================
 *
 * ============================================================================
 *
 * ============================================================================
 */

#ifndef DEBUG_H_
#define DEBUG_H_

/* Forward declarations */
struct cp1600_t;
struct speed_t;
struct gfx_t;
struct stic_t;
struct event_t;

/*
 * ============================================================================
 *  DEBUG_T          -- Debugger peripheral.  Yes, it's a peripheral.
 * ============================================================================
 */
#define DEBUG_MAX_FILESTK (16)
typedef struct debug_t
{
    periph_t        periph;         /*  Debugger looks like a peripheral.   */
    int             show_rd;        /*  FLAG: show reads being performed.   */
    int             show_wr;        /*  FLAG: show writes being performed.  */
    int             show_ins;       /*  FLAG: show instructions / regs .    */
    struct cp1600_t *cp1600;        /*  Pointer to actual CPU.              */
    struct speed_t  *speed;         /*  Rate control (needed for resync)    */
    struct gfx_t    *gfx;           /*  So we can toggle windowed mode.     */
    struct stic_t   *stic;          /*  So we can toggle STIC breakpoints   */
    struct event_t  *event;         /*  So we can pump/inject events.       */
    int             step_count;     /*  Number of CPU instrs to step thru.  */
    int             step_over;      /*  FLAG:  Are we stepping over JSRs?   */
    uint64_t        tot_instr;      /*  Last observed tot_instr value.      */
    uint8_t         *vid_enable;
    LZFILE          *filestk[DEBUG_MAX_FILESTK];
    int             filestk_depth;
    int             symb_addr_format;
    uint32_t        *exit_flag;     /*  Pointer to exit request flag.       */
} debug_t;

/*
 * ============================================================================
 *  DEBUG_RD         -- Capture/print a read event.
 *  DEBUG_WR         -- Capture/print a write event.
 *  DEBUG_TK         -- Debugger 'tick' function.  This where the user command
 *                      line called from.
 * ============================================================================
 */
uint32_t debug_rd(periph_t *p, periph_t *r, uint32_t a, uint32_t d);
void     debug_wr(periph_t *p, periph_t *r, uint32_t a, uint32_t d);
uint32_t debug_tk(periph_t *p, uint32_t len);

/*
 * ============================================================================
 *  DEBUG_DISASM      -- Disassembles one instruction, returning a pointer
 *                       to the disassembled text.  Uses the disassembly
 *                       cache if possible.
 * ============================================================================
 */
char * debug_disasm(periph_t *p, cp1600_t *cp, uint16_t addr,
                    uint32_t *len, int dbd);

const char * debug_disasm_src(periph_t *p, cp1600_t *cp, uint16_t addr,
                              uint32_t *len, int dbd, int disasm_width);
/*
 * ============================================================================
 *  DEBUG_DISASM_MEM  -- Disassembles a range of memory locations.
 * ============================================================================
 */
void debug_disasm_mem(periph_t *p, cp1600_t *cp, uint16_t *paddr, uint32_t cnt);

/*
 * ============================================================================
 *  DEBUG_DISPMEM     -- Displays ten lines of "hex dump" memory information.
 *                       The first arg is the address to start dumping at.
 *                       The second arg is the number of addresses to dump.
 * ============================================================================
 */
void debug_dispmem(periph_t *p, uint16_t addr, uint16_t len);

/*
 * ============================================================================
 *  DEBUG_INIT       -- Initializes a debugger object and registers a CPU
 *                      pointer.
 * ============================================================================
 */
int debug_init(debug_t *debug,
               struct cp1600_t *cp1600,
               struct speed_t *speed,
               struct gfx_t *gfx,
               struct stic_t *stic,
               struct event_t *event,
               const char *symtbl,
               uint8_t *vid_enable,
               const char *script,
               uint32_t *exit_flag);

/* ======================================================================== */
/*  DEBUG_SYMB_FOR_ADDR  -- Returns symbol associated with and address, or  */
/*                          NULL if there is none.  Performs no formatting. */
/*                                                                          */
/*  Prefers symbols that start w/out a '.' if available.                    */
/* ======================================================================== */
const char *debug_symb_for_addr
(
    const uint32_t addr
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
/*                    Copyright (c) 2006, Joseph Zbiciak                    */
/* ======================================================================== */
