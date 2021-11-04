/*
 * ============================================================================
 *  CP_1600:        CP-1600 internal status structures and functions
 *
 *  Author:         J. Zbiciak
 *
 *
 * ============================================================================
 *  CP1600_T             -- CP-1600 internal state structure
 *  CP1600_P             -- pointer to CP1600 object
 * ============================================================================
 *  CP1600_INIT          -- Initializes a CP1600_T structure
 *  CP1600_RUN           -- Runs the CP1600 for some number of microcycles
 *  CP1600_CACHEABLE     -- Marks a region of instruction space as
 *                          cacheable in the decoder, so that we can cache
 *                          the decoded instructions.  It doesn't actually
 *                          trigger the decode of the instructions though.
 *  CP1600_INVALIDATE    -- Invalidates a region of cached instructions.
 * ============================================================================
 */


#ifndef CP_1600_H_
#define CP_1600_H_

#include "cp1600/req_q.h"

/*
 * ============================================================================
 *  CP1600_T             -- CP-1600 internal state structure
 *  CP1600_P             -- pointer to CP1600 object
 *
 *  This structure contains all the state information related to an instance
 *  of a CP-1600 processor.  This includes the state of the register file,
 *  current cycle count, and pointers to the memory subsystem and peripherals.
 *
 *  Since the CP1600 tries to cache decoded instructions, the CP1600 state
 *  structure also includes "cacheable" bits for each 'page' of memory.  If a
 *  page is not marked "cacheable", then no instruction spanning that page
 *  is allowed to be "cached".  The page size for the cacheability bits is
 *  identical to the decoding page size.
 * ============================================================================
 */

#define CYC_MAX            (1 << 24)
#define CP1600_MEMSIZE     (16)
#define CP1600_DECODE_PAGE (4)

struct cp1600_t;    /* forward decl. */
struct instr_t;     /* forward decl. */

typedef int cp1600_ins_t(const struct instr_t *, struct cp1600_t *);

typedef struct cp1600_t
{
    periph_t        periph;         /* The CP-1600 is a peripheral.         */
    periph_t        snoop;          /* The CP-1600 is a peripheral.         */

    uint16_t        r[8];           /* CP1600 registers                     */
    uint16_t        xr[16];         /* Extended registers (JLP/Locutus)     */
    uint16_t        oldpc;          /* Program counter at start of insn.    */
    uint16_t        ext;            /* EXT[0-3] pin state                   */
    uint16_t        int_vec;        /* Interrupt vector.                    */

    bool            pend_reset;     /* flag: Pending reset.                 */
    bool            rand_mem;       /* flag: Randomize on reset.            */

    int             S,C,O,Z,I,D;    /* status bits.                         */
    int             intr;           /* Current instr is interruptible       */
    int             req_ack_state;  /* INTRQ/INTAK/BUSRQ/BUSAK for debugger */
    req_q_t         req_q;          /* INTRQ and BUSRQ inputs to CPU.       */

    uint32_t        cacheable [1 << (CP1600_MEMSIZE-CP1600_DECODE_PAGE-5)];
    cp1600_ins_t   *execute   [1 << CP1600_MEMSIZE];  /* Decoded instrs     */
    struct instr_t *instr     [1 << CP1600_MEMSIZE];  /* Decoded instrs     */
    char *          disasm    [1 << CP1600_MEMSIZE];  /* Disassembled instrs*/

#ifdef DEBUG_DECODE_CACHE
    int             decoded   [1 <<  CP1600_MEMSIZE];
#endif

    periph_tick_t  *instr_tick;         /* Per-instruction external ticker  */
    periph_t       *instr_tick_periph;  /* Periph ptr to pass along.        */
    int             step_count;         /* Number of instructions to run.   */
    int             steps_remaining;    /* Step down-counter.               */

    uint64_t        tot_cycle;
    uint64_t        tot_instr;
    uint64_t        tot_cache;
    uint64_t        tot_noncache;

    uint8_t         hit_breakpoint;
} cp1600_t;

#define CP1600_PK(c,a)   (periph_peek (AS_PERIPH(c->periph.bus),         \
                                       AS_PERIPH(c),a,~0u))
#define CP1600_RD(c,a)   (periph_read (AS_PERIPH(c->periph.bus),         \
                                       AS_PERIPH(c),a,~0u))
#define CP1600_WR(c,a,d) (periph_write(AS_PERIPH(c->periph.bus),         \
                                       AS_PERIPH(c),a,d))

/* Types of 'breakpoint' */
enum
{
    BK_NONE = 0,        /* No breakpoint hit */
    BK_BREAKPOINT = 1,  /* A standard breakpoint: hit it every time */
    BK_TRACEPOINT       /* Tracepoint: hit once, then cancel. */
};

/* Breakpoint flags, stored in the encoded opcode */
enum
{
    CP1600_BKPT       = 1,
    CP1600_BKPT_ONCE  = 2
};

/* Interruptibilty, as indicated in cp1600->intr */
enum
{
    CP1600_INT_ENABLE   = 1,    /* Interrupts enabled           */
    CP1600_INT_INSTR    = 2,    /* Interruptible instruction    */
};

#define CP1600_CAN_BUSAK(c) (((c)->intr & CP1600_INT_INSTR) != 0)
#define CP1600_CAN_INTAK(c) \
    (((c)->intr & (CP1600_INT_INSTR | CP1600_INT_ENABLE)) == \
                  (CP1600_INT_INSTR | CP1600_INT_ENABLE))

/* INTRQ/BUSRQ state for debugger */
enum
{
    CP1600_INTRQ = 1 << 0,
    CP1600_INTAK = 1 << 1,
    CP1600_BUSRQ = 1 << 2,
    CP1600_BUSAK = 1 << 3
};

/*
 * ============================================================================
 *  CP1600_INIT          -- Initializes a CP1600_T structure
 *
 *  Currently, this sets up the CP1600 structure w/ 64K of 16-bit RAM.  This
 *  is not ideal and will be changed later.
 * ============================================================================
 */
int
cp1600_init
(
    cp1600_t *const cp1600,
    uint16_t  const rst_vec,
    uint16_t  const int_vec,
    bool      const rand_mem
);

/*
 * ============================================================================
 *  CP1600_INSTR_TICK    -- Sets/unsets an per-instruction ticker
 * ============================================================================
 */
void
cp1600_instr_tick
(
    cp1600_t      *const cp1600,
    periph_tick_t *const instr_tick,
    periph_t      *const instr_tick_periph
);

/*
 * ============================================================================
 *  CP1600_RUN           -- Runs the CP1600 for some number of microcycles
 *
 *  This is the main CP1600 loop.  It is responsible for fetching instructions,
 *  decoding them if necessary (or using predecoded instructions if possible)
 *  and calling the required execute functions.
 *
 *  The cp1600_run function will run as many instructions as are necessary to
 *  just barely exceed the specified number of microcycles.  eg.  It will
 *  execute a new instruction if the specified total number of microcycles
 *  has not yet been exceeded.  The new instruction may exceed the specified
 *  number of microcycles.  The total number of microcycles exhausted is
 *  returned as an int.
 * ============================================================================
 */
uint32_t
cp1600_run
(
    periph_t *const periph,
    uint32_t  const microcycles
);


/*
 * ============================================================================
 *  CP1600_RESET         -- Resets the CP1600.
 * ============================================================================
 */
void cp1600_reset(periph_t *const p);

/*
 * ============================================================================
 *  CP1600_CACHEABLE     -- Marks a region of instruction space as
 *                          cacheable in the decoder, so that we can cache
 *                          the decoded instructions.  It doesn't actually
 *                          trigger the decode of the instructions though.
 * ============================================================================
 */

void
cp1600_cacheable
(
    cp1600_t *const cp1600,
    uint32_t  const addr_lo,
    uint32_t  const addr_hi,
    int       const need_snoop
);

/*
 * ============================================================================
 *  CP1600_INVALIDATE    -- Invalidates a region of cached instructions.
 * ============================================================================
 */
void
cp1600_invalidate
(
    cp1600_t *const cp1600,
    uint32_t  const addr_lo,
    uint32_t  const addr_hi
);

/*
 * ============================================================================
 *  CP1600_WRITE         -- Snoop writes to the bus to do cache invalidates.
 * ============================================================================
 */
void
cp1600_write
(
    periph_t *const p,
    periph_t *const req,
    uint32_t  const addr,
    uint32_t  const data
);

/*
 * ============================================================================
 *  CP1600_SET_BREAKPT   -- Sets a breakpoint at a given address.
 *
 *  Note:  Instructions which overlap a breakpoint address won't trigger the
 *  breakpoint.
 * ============================================================================
 */
int
cp1600_set_breakpt
(
    cp1600_t *const cp1600,
    uint16_t  const addr,
    uint16_t  const flags
);

/*
 * ============================================================================
 *  CP1600_CLR_BREAKPT   -- Clears a breakpoint at a given address.
 * ============================================================================
 */
void
cp1600_clr_breakpt
(
    cp1600_t *const cp1600,
    uint16_t  const addr,
    uint16_t  const flags
);

/*
 * ============================================================================
 *  CP1600_LIST_BREAKPTS -- Lists all matching breakpoints via callback.
 * ============================================================================
 */
typedef void cp1600_addr_callback_fn(void *, uint32_t);

void cp1600_list_breakpts
(
    const cp1600_t          *const cp1600,
    uint16_t                 const flags,
    cp1600_addr_callback_fn *const callback,
    void                    *const opaque
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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
