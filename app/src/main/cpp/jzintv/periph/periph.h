/*
 * ============================================================================
 *  Title:    Peripheral Subsystem
 *  Author:   J. Zbiciak
 * ============================================================================
 *  PERIPH_NEW       -- Creates a new peripheral bus
 *  PERIPH_DELETE    -- Disposes a peripheral bus
 *  PERIPH_REGISTER  -- Registers a peripheral on the bus
 *  PERIPH_READ      -- Perform a read on a peripheral bus as a CPU
 *  PERIPH_PEEK      -- Perform a read on a peripheral bus via backdoor
 *  PERIPH_WRITE     -- Perform a write on a peripheral bus as a CPU
 *  PERIPH_POKE      -- Perform a write on a peripheral bus via backdoor
 *  PERIPH_TICK      -- Perform a tick on a peripheral bus
 * ============================================================================
 *  PERIPH_BUS_T     -- Peripheral bus information
 *  PERIPH_T         -- Per-peripheral information
 *  PERIPH_RD_T      -- Peripheral Read function pointer type
 *  PERIPH_WR_T      -- Peripheral Write function pointer type
 *  PERIPH_TICK_T    -- Peripheral Clock-Tick function pointer type
 * ============================================================================
 *  Peripheral bus information is divided into two sets of information:
 *
 *   -- Bus-wide information stored in a periph_bus_t
 *   -- Per-device information stored in a periph_t
 *
 *  Peripherals wishing to use the peripheral subsystem need to 'extend'
 *  periph_t by declaring a structure which has a periph_t as its first
 *  element.  This extended structure can then be passed into the periph_*
 *  routines as if it were a 'periph_t', and can be passed to each of the
 *  peripherals as a 'this' pointer.
 * ============================================================================
 */

#ifndef PERIPH_H_
#define PERIPH_H_

#include "serializer/serializer.h"
#define MAX_PERIPH_BIN (32)

/* ======================================================================== */
/*  PERIPH_RD_T      -- Peripheral Read function pointer type               */
/*  PERIPH_WR_T      -- Peripheral Write function pointer type              */
/*  PERIPH_TICK_T    -- Peripheral Clock-Tick function pointer type         */
/* ======================================================================== */
struct periph_t;    /* forward declaration */

typedef uint32_t periph_rd_t(struct periph_t *periph, struct periph_t *req,
                             uint32_t addr, uint32_t data);
typedef void     periph_wr_t(struct periph_t *periph, struct periph_t *req,
                             uint32_t addr, uint32_t data);
typedef uint32_t periph_tick_t(struct periph_t *periph, uint32_t len);
typedef void     periph_rst_t (struct periph_t *periph);
typedef void     periph_ser_t (struct periph_t *periph);
typedef void     periph_dtor_t(struct periph_t *periph);

/* ======================================================================== */
/*  PERIPH_T         -- Per-peripheral information                          */
/* ======================================================================== */
typedef struct periph_t
{
    char            name[16];   /*  Name of device                          */
    periph_rd_t     *read;      /*  Called for every read in addr space.    */
    periph_wr_t     *write;     /*  Called for every write in addr space.   */
    periph_rd_t     *peek;      /*  Reads memory without side-effects.      */
    periph_wr_t     *poke;      /*  Writes memory (including ROM.)          */
    periph_tick_t   *tick;      /*  Called every 'tick_per' ticks.          */
    periph_rst_t    *reset;     /*  Called when resetting the machine       */
    periph_ser_t    *ser_init;  /*  Called at reg time to init serializer   */
    periph_dtor_t   *dtor;      /*  Destructor; called when shutting down   */

    uint32_t        addr_base;  /*  Address base -- SUB'd from addrs on     */
                                /*  each read or write.                     */

    uint32_t        addr_mask;  /*  Address mask -- AND'd with addr after   */
                                /*  subtracting the address base.           */

    uint64_t        now;        /*  Peripheral's concept of 'now'.          */
    uint32_t        min_tick;   /*  Minimum number of cycles between ticks. */
    uint32_t        max_tick;   /*  Maximum number of cycles between ticks. */
    uint32_t        next_tick;  /*  Number of ticks until next tick call.   */

    struct periph_bus_t *bus;   /*  Peripheral bus registered on.           */
    struct periph_t *next;      /*  Next peripheral on peripheral bus.      */
    struct periph_t *tickable;  /*  Next periph on tickable list.           */

    int             busy;       /*  Busy flag to prevent infinite loops.    */
    struct periph_t *req;       /*  Requestor busying this peripheral.      */
    void            *parent;    /*  Optional pointer to parent structure.   */
} periph_t;

/* ======================================================================== */
/*  Up and down cast helper macros.                                         */
/* ======================================================================== */
#define AS_PERIPH(p)           ((periph_t *)(p))
#define PERIPH_AS(t, p)        ((t *)(p))
#define PERIPH_PARENT_AS(t, p) ((t *)((p)->parent))

/* ======================================================================== */
/*  Convenience macros declaring a peripheral.                              */
/*  PERIPH_HZ       Approximate period in Hz for NTSC.  PAL is ~11% fast.   */
/*  PERIPH_NO_RDWR  Use to declare a periph_t with no memory read/write.    */
/* ======================================================================== */
#define PERIPH_HZ(h) (3579545 / (4 * (h)))
#define PERIPH_NO_RDWR  \
    .read = NULL, .write = NULL, .peek = NULL, .poke = NULL, \
    .addr_base = ~0U, .addr_mask = 0

/* ======================================================================== */
/*  PERIPH_BUS_T     -- Peripheral bus information                          */
/* ======================================================================== */
typedef struct periph_bus_t
{
    periph_t    periph;         /*  Peripheral busses are also peripherals  */

    uint32_t    addr_mask;      /*  Address mask (up to 32 bits).           */
    uint32_t    data_mask;      /*  Data mask (up to 32 bits).              */

    uint32_t    decode_shift;   /*  Controls granularity of addr. decode.   */

    periph_t    **rd[MAX_PERIPH_BIN];  /* Pointers to readable peripherals  */
    periph_t    **wr[MAX_PERIPH_BIN];  /* Pointers to writable peripherals  */

    periph_t    *list;          /*  Linked list of peripherals on this bus  */
    periph_t    *tickable;      /*  Linked list of periph. w/ tick fxns.    */

    bool        pend_reset;     /*  Pending reset flag set by a periph.     */
} periph_bus_t;


/* ======================================================================== */
/*  PERIPH_NEW       -- Creates a new peripheral bus                        */
/* ======================================================================== */
periph_bus_t *periph_new
(
    int addr_size,              /*  Address size (in bits)              */
    int data_size,              /*  Data size (in bits)                 */
    int decode_shift            /*  Decode granularity control          */
);


/* ======================================================================== */
/*  PERIPH_DELETE    -- Disposes a peripheral bus                           */
/* ======================================================================== */
void periph_delete
(
    periph_bus_t *bus           /*  Peripheral bus to dispose           */
);

/* ======================================================================== */
/*  PERIPH_REGISTER  -- Registers a peripheral on the bus                   */
/* ======================================================================== */
void periph_register
(
    periph_bus_t    *bus,       /*  Peripheral bus being registered on. */
    periph_t        *periph,    /*  Peripheral being (re)registered.    */
    uint32_t        addr_lo,    /*  Low end of address range.           */
    uint32_t        addr_hi,    /*  High end of address range.          */
    const char      *name       /*  Name to give device.                */
);

/* ======================================================================== */
/*  PERIPH_READ      -- Perform a read on a peripheral bus as a CPU.        */
/*  PERIPH_PEEK      -- Perform a read on a peripheral bus via backdoor.    */
/* ======================================================================== */
unsigned periph_read
(
    periph_t        *bus,       /*  Peripheral bus being read.          */
    periph_t        *req,       /*  Peripheral requesting read.         */
    uint32_t        addr,       /*  Address being read.                 */
    uint32_t        data        /*  Current state of data being read.   */
);

unsigned periph_peek
(
    periph_t        *bus,       /*  Peripheral bus being read.          */
    periph_t        *req,       /*  Peripheral requesting read.         */
    uint32_t        addr,       /*  Address being read.                 */
    uint32_t        data        /*  Current state of data being read.   */
);

/* ======================================================================== */
/*  PERIPH_WRITE     -- Perform a write on a peripheral bus as a CPU.       */
/*  PERIPH_POKE      -- Perform a write on a peripheral bus via backdoor.   */
/* ======================================================================== */
void periph_write
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting write.        */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
);

void periph_poke
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting write.        */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
);

/* ======================================================================== */
/*  PERIPH_TICK      -- Perform a tick on a peripheral bus                  */
/* ======================================================================== */
uint32_t periph_tick
(
    periph_t        *bus,       /*  Peripheral bus being ticked.        */
    uint32_t        len
);

/* ======================================================================== */
/*  PERIPH_RESET     -- Resets all of the peripherals on the bus            */
/* ======================================================================== */
void periph_reset
(
    periph_bus_t    *bus
);

/* ======================================================================== */
/*  PERIPH_SER_REGISTER -- registers a peripheral for serialization         */
/* ======================================================================== */
void periph_ser_register
(
    periph_t   *per,
    ser_hier_t *hier
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
/*                 Copyright (c) 1998-2019, Joseph Zbiciak                  */
/* ======================================================================== */
