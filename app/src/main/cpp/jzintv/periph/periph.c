/*
 * ============================================================================
 *  Title:    Peripheral Subsystem
 *  Author:   J. Zbiciak
 * ============================================================================
 *  PERIPH_NEW       -- Creates a new peripheral bus
 *  PERIPH_DELETE    -- Disposes a peripheral bus
 *  PERIPH_REGISTER  -- Registers a peripheral on the bus
 *  PERIPH_READ      -- Perform a read on a peripheral bus
 *  PERIPH_WRITE     -- Perform a write on a peripheral bus
 *  PERIPH_TICK      -- Perform a tick on a peripheral bus
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


#include "../config.h"
#include "periph.h"
#include "serializer/serializer.h"

#define DEBUG_TICK 0

/*
 * ============================================================================
 *  PERIPH_NEW       -- Creates a new peripheral bus
 * ============================================================================
 */
periph_bus_t *periph_new
(
    int addr_size,              /*  Address size (in bits)              */
    int data_size,              /*  Data size (in bits)                 */
    int decode_shift            /*  Decode granularity control          */
)
{
    periph_bus_t    *bus;
    int             bins, i;

    /* -------------------------------------------------------------------- */
    /*  Sanity check arguments:                                             */
    /*   -- addr_size, data_size must be <= 32 bits.                        */
    /*   -- decode_shift must be < addr_size.                               */
    /* -------------------------------------------------------------------- */
    if (addr_size > 32 || data_size > 32 || decode_shift >= addr_size)
    {
        fprintf(stderr,"FATAL:  invalid periph bus args: %d,%d,%d\n",
                addr_size, data_size, decode_shift);
    }

    /* -------------------------------------------------------------------- */
    /*  Allocate memory for the peripheral bus.                             */
    /* -------------------------------------------------------------------- */
    bus = CALLOC(periph_bus_t, 1);

    if (!bus)
    {
        fprintf(stderr,"FATAL:  cannot allocate memory for periph bus.\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Initialize the peripheral bus' private fields.                      */
    /* -------------------------------------------------------------------- */
    bus->addr_mask = ~(~0U << addr_size);
    bus->data_mask = ~(~0U << addr_size);

    bus->decode_shift = decode_shift;
    bus->pend_reset   = false;

    bus->list = 0;
    bins = addr_size - decode_shift;

    if (! (bus->rd[0] = CALLOC(periph_t *, MAX_PERIPH_BIN << bins)) ||
        ! (bus->wr[0] = CALLOC(periph_t *, MAX_PERIPH_BIN << bins)) )
    {
        fprintf(stderr,"FATAL:  cannot allocate memory for periph bus.\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Allocate memory for the decode arrays.                              */
    /* -------------------------------------------------------------------- */
    for (i = 1; i < MAX_PERIPH_BIN; i++)
    {
        bus->rd[i] = bus->rd[i - 1] + (1u << bins);
        bus->wr[i] = bus->wr[i - 1] + (1u << bins);
    }


    /* -------------------------------------------------------------------- */
    /*  Initialize the peripheral substructure of the bus.  Yes, peripheral */
    /*  busses can be peripherals on other busses.  Strange but true.       */
    /* -------------------------------------------------------------------- */
    bus->periph.busy        = 0;
    bus->periph.now         = 0;
    bus->periph.bus         = NULL;
    bus->periph.next        = NULL;
    bus->periph.min_tick    = 1;
    bus->periph.max_tick    = ~0U;
    bus->periph.next_tick   = 0;
    bus->periph.read        = periph_read;
    bus->periph.write       = periph_write;
    bus->periph.tick        = periph_tick;
    //bus->periph.dtor        = periph_delete;

    /* -------------------------------------------------------------------- */
    /*  Set the peripheral bus' default name.                               */
    /* -------------------------------------------------------------------- */
    strncpy(bus->periph.name, "Bus", sizeof(bus->periph.name));

    return bus;
}

/*
 * ============================================================================
 *  PERIPH_DELETE    -- Disposes a peripheral bus, destructing everything
 *                      that's attached to it.
 * ============================================================================
 */
void periph_delete
(
    periph_bus_t *bus               /*  Peripheral bus to dispose           */
)
{
    periph_t *curr, *next;

    /* -------------------------------------------------------------------- */
    /*  Avoid recursion by marking ourselves busy.                          */
    /* -------------------------------------------------------------------- */
    bus->periph.busy = 1;

    /* -------------------------------------------------------------------- */
    /*  Step through all attached periphs and call their dtors.             */
    /* -------------------------------------------------------------------- */
    for (curr = bus->list; curr; curr = next)
    {
        next = curr->next;
        if (curr->dtor && !curr->busy)
        {
            curr->busy = 1;
            curr->dtor(curr);       /* not safe to write to after this */
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Next free the periph_t* array ande bus itself.                      */
    /* -------------------------------------------------------------------- */
    free(bus->rd[0]);
    free(bus->wr[0]);
    free(bus);
}

/*
 * ============================================================================
 *  PERIPH_REGISTER  -- Registers a peripheral on the bus
 * ============================================================================
 */
void periph_register
(
    periph_bus_t    *bus,       /*  Peripheral bus being registered on. */
    periph_t        *periph,    /*  Peripheral being (re)registered.    */
    uint32_t        addr_lo,    /*  Low end of address range.           */
    uint32_t        addr_hi,    /*  High end of address range.          */
    const char      *name       /*  Name of peripheral.                 */
)
{
    uint32_t bin, addr;
    int i;
    static int unnamed = 0;
    char buf[32];

    /* -------------------------------------------------------------------- */
    /*  Make sure we're registering this peripheral on at most one bus.     */
    /* -------------------------------------------------------------------- */
    if (periph->bus != NULL && periph->bus != bus)
    {
        fprintf(stderr, "FATAL:  Registering a peripheral on multiple "
                        "busses!\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Make sure we're not registering ourself on ourself.                 */
    /* -------------------------------------------------------------------- */
    if ((periph_t *) bus == periph)
    {
        fprintf(stderr, "FATAL:  Loopback peripheral registry not allowed!\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If this is the first time we've registered this peripheral, add it  */
    /*  to our linked list of peripherals.  Since order isn't greatly       */
    /*  important, we just shove it in front.  Also, while we're here,      */
    /*  set the peripheral's name.                                          */
    /* -------------------------------------------------------------------- */
    if (periph->bus == NULL)
    {
        periph->bus  = bus;
        periph->next = bus->list;
        bus->list    = periph;

        /* ---------------------------------------------------------------- */
        /*  If the device has a 'tick' func, add it to our tickable list.   */
        /* ---------------------------------------------------------------- */
        if (periph->tick)
        {
            periph_t *tick;

            tick = bus->tickable;
            if (!tick)
            {
                bus->tickable = periph;
            } else
            {
                while (tick->tickable)
                    tick = tick->tickable;
                tick->tickable = periph;
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Poke in the user-readable name for the device, if supplied.     */
        /*  Otherwise name it 'Unnamed %d'.                                 */
        /* ---------------------------------------------------------------- */
        if (!name)
        {
            snprintf(buf, 32, "Unnamed %d\n", ++unnamed);
            name = buf;
        }
        int copy_amount = sizeof(periph->name);
        int name_bytes = strlen(name) + 1;
        if (copy_amount > name_bytes) copy_amount = name_bytes;
        memcpy(periph->name, name, copy_amount);
        periph->name[sizeof(periph->name) - 1] = 0;

        /* ---------------------------------------------------------------- */
        /*  Now register this guy for serialization.                        */
        /* ---------------------------------------------------------------- */
#ifndef NO_SERIALIZER
        if (periph->ser_init)
        {
            periph->ser_init(periph);
            periph->ser_init = NULL;  /* don't double-initialize */
        }
#endif
    }

    /* -------------------------------------------------------------------- */
    /*  Poke the device into our address decode structures.                 */
    /* -------------------------------------------------------------------- */
    if (periph->read)
    for (addr = addr_lo & ( -(1u << bus->decode_shift) );
         addr <= addr_hi; addr += 1u << bus->decode_shift)
    {
        bin = (addr & bus->addr_mask) >> bus->decode_shift;

        for (i = 0; i < MAX_PERIPH_BIN && bus->rd[i][bin]; i++)
            if (bus->rd[i][bin] == periph)
                break;

        if (i == MAX_PERIPH_BIN)
        {
            fprintf(stderr, "FATAL:  >%d read devices in address range "
                            "%.8x..%.8x\n",
                            MAX_PERIPH_BIN,
                            bin << bus->decode_shift,
                            ((bin + 1) << bus->decode_shift) - 1);
            exit(1);
        }

        bus->rd[i][bin] = periph;
    }

    if (periph->write)
    for (addr = addr_lo & ( -(1u << bus->decode_shift) );
         addr <= addr_hi; addr += 1u << bus->decode_shift)
    {
        bin = (addr & bus->addr_mask) >> bus->decode_shift;

        for (i = 0; i < MAX_PERIPH_BIN && bus->wr[i][bin]; i++)
            if (bus->wr[i][bin] == periph)
                break;

        if (i == MAX_PERIPH_BIN)
        {
            fprintf(stderr, "FATAL:  >%d write devices in address range "
                            "%.8x..%.8x\n",
                            MAX_PERIPH_BIN,
                            bin << bus->decode_shift,
                            ((bin + 1) << bus->decode_shift) - 1);
            exit(1);
        }

        bus->wr[i][bin] = periph;
    }

    jzp_printf("%-16s [0x%.4X...0x%.4X]\n", name,
            addr_lo & bus->addr_mask, addr_hi & bus->addr_mask);
}


/*
 * ============================================================================
 *  PERIPH_READ      -- Perform a read on a peripheral bus
 * ============================================================================
 */
uint32_t periph_read
(
    periph_t    *bus,       /*  Peripheral bus being read.          */
    periph_t    *req,       /*  Peripheral requesting the read.     */
    uint32_t    addr,       /*  Address being read.                 */
    uint32_t    data1
)
{
    periph_bus_t    *busp = (periph_bus_t *)bus;
    periph_t        *periph;
    uint32_t        bin;
    int             i;
    uint32_t        data = busp->data_mask;

    UNUSED(data1);

    /* -------------------------------------------------------------------- */
    /*  Any peripheral which forwards a read/write request is required to   */
    /*  set its busy and requestor fields around the forwarded request.     */
    /* -------------------------------------------------------------------- */
    bus->busy = 1;
    bus->req  = req;

    bin = (addr & busp->addr_mask) >> busp->decode_shift;

    /* -------------------------------------------------------------------- */
    /*  Perform the peripheral reads.  Peripherals which merely generate    */
    /*  side effects and don't actually drive the bus should return ~0U,    */
    /*  as we return the logical AND of all values received.  (TTL open-    */
    /*  collector behavior of driving the bus low or floating the bus hi.)  */
    /* -------------------------------------------------------------------- */
    for (i = 0; busp->rd[i][bin]; i++)
    {
        periph = busp->rd[i][bin];
        if (!periph->busy)
            data &= periph->read(periph, bus,
                    (addr - periph->addr_base) & periph->addr_mask, data);
    }

    bus->busy = 0;
    bus->req  = NULL;
    return data;
}

/*
 * ============================================================================
 *  PERIPH_PEEK      -- Perform a read on a peripheral bus w/out side effects
 * ============================================================================
 */
uint32_t periph_peek
(
    periph_t    *bus,       /*  Peripheral bus being read.          */
    periph_t    *req,       /*  Peripheral requesting the read.     */
    uint32_t    addr,       /*  Address being read.                 */
    uint32_t    data1
)
{
    periph_bus_t    *busp = (periph_bus_t *)bus;
    periph_t        *periph;
    uint32_t        bin;
    int             i;
    uint32_t        data = busp->data_mask;

    UNUSED(data1);

    /* -------------------------------------------------------------------- */
    /*  Any peripheral which forwards a read/write request is required to   */
    /*  set its busy and requestor fields around the forwarded request.     */
    /* -------------------------------------------------------------------- */
    bus->busy = 1;
    bus->req  = req;

    bin = (addr & busp->addr_mask) >> busp->decode_shift;

    /* -------------------------------------------------------------------- */
    /*  Perform the peripheral reads.  Peripherals which merely generate    */
    /*  side effects and don't actually drive the bus should return ~0U,    */
    /*  as we return the logical AND of all values received.  (TTL open-    */
    /*  collector behavior of driving the bus low or floating the bus hi.)  */
    /* -------------------------------------------------------------------- */
    for (i = 0; busp->rd[i][bin]; i++)
    {
        periph = busp->rd[i][bin];
        if (!periph->busy)
            data &= periph->peek(periph, bus,
                    (addr - periph->addr_base) & periph->addr_mask, data);
    }

    bus->busy = 0;
    bus->req  = NULL;
    return data;
}


/*
 * ============================================================================
 *  PERIPH_WRITE     -- Perform a write on a peripheral bus
 * ============================================================================
 */
void periph_write
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting the write.    */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
)
{
    periph_bus_t    *busp = (periph_bus_t *)bus;
    periph_t        *periph;
    uint32_t        bin;
    int             i;

    /* -------------------------------------------------------------------- */
    /*  Any peripheral which forwards a read/write request is required to   */
    /*  set its busy and requestor fields around the forwarded request.     */
    /* -------------------------------------------------------------------- */
    bus->busy = 1;
    bus->req  = req;

    bin = (addr & busp->addr_mask) >> busp->decode_shift;

    /* -------------------------------------------------------------------- */
    /*  Perform the peripheral writes.  Make sure we AND the data being     */
    /*  written with the actual width of the bus.                           */
    /* -------------------------------------------------------------------- */
    data &= busp->data_mask;
    for (i = 0; busp->wr[i][bin]; i++)
    {
        periph = busp->wr[i][bin];
        if (!periph->busy)
            periph->write(periph, bus,
                          (addr-periph->addr_base) & periph->addr_mask, data);

    }

    bus->busy = 0;
    bus->req  = NULL;
}

/*
 * ============================================================================
 *  PERIPH_POKE      -- Perform a write on a peripheral bus, incl to ROM
 * ============================================================================
 */
void periph_poke
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting the write.    */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
)
{
    periph_bus_t    *busp = (periph_bus_t *)bus;
    periph_t        *periph;
    uint32_t        bin;
    int             i;

    /* -------------------------------------------------------------------- */
    /*  Any peripheral which forwards a read/write request is required to   */
    /*  set its busy and requestor fields around the forwarded request.     */
    /* -------------------------------------------------------------------- */
    bus->busy = 1;
    bus->req  = req;

    bin = (addr & busp->addr_mask) >> busp->decode_shift;

    /* -------------------------------------------------------------------- */
    /*  Perform the peripheral writes.  Make sure we AND the data being     */
    /*  written with the actual width of the bus.                           */
    /* -------------------------------------------------------------------- */
    data &= busp->data_mask;
    for (i = 0; busp->wr[i][bin]; i++)
    {
        periph = busp->wr[i][bin];
        if (!periph->busy)
            periph->poke(periph, bus,
                          (addr-periph->addr_base) & periph->addr_mask, data);

    }

    bus->busy = 0;
    bus->req  = NULL;
}

/*
 * ============================================================================
 *  PERIPH_TICK      -- Perform a tick on a peripheral bus
 * ============================================================================
 */
uint32_t periph_tick
(
    periph_t        *bus,       /*  Peripheral bus being ticked.        */
    uint32_t        len         /*  How much time has passed.           */
)
{
    periph_bus_t    *busp = (periph_bus_t *)bus;
    periph_t        *tick;
    uint32_t        elapsed = 0, ticked;

    uint64_t        now  = bus->now;    /* Where we currently are       */
    uint64_t        soon = now + len;   /* What we're trying to get to */

    bus->busy = 1;

    /* -------------------------------------------------------------------- */
    /*  The Tick routine attempts to advance time a total of 'len' units    */
    /*  within the constraints imposed by the peripherals attached.         */
    /*  When calling a peripheral's tick function, it is informed of how    */
    /*  much time has passed since its last tick, and it is expected to     */
    /*  report back how much time it actually processed (since, for some    */
    /*  peripherals, the granularity of the passage of time is larger than  */
    /*  one cycle).                                                         */
    /*                                                                      */
    /*  Different peripherals have different "ticking" requirements.        */
    /*  For instance, the STIC must be ticked exactly 16 times a frame,     */
    /*  giving a tick rate of exactly 960 Hz.  Each tick represents the     */
    /*  passing of 1/960th of a second.  In contrast, the CP-1610 can be    */
    /*  ticked at any time, and said tick can be any duration.              */
    /*                                                                      */
    /*  In order to cope with these diverse requirements, each peripheral   */
    /*  defines a minimum and maximum tick size, the combination of         */
    /*  which specify the constraints on when the devices may be ticked.    */
    /*  For performance reasons, the peripheral bus tries to tick each      */
    /*  peripheral as infrequently as possible (eg. maximize the ticks),    */
    /*  but peripheral interactions will occasionally require more          */
    /*  frequent ticking.                                                   */
    /*                                                                      */
    /*  Ticking is performed in two passes.  The first pass identifies      */
    /*  the "minimum of the maximums" -- eg. the largest step forward in    */
    /*  time that the attached peripherals will let us acheive.  The        */
    /*  second pass will then tick all peripherals whose minimum tick       */
    /*  sizes allow them to be ticked with this size tick.  The process     */
    /*  is repeated (both passes) until the entire total tick size is       */
    /*  consumed.                                                           */
    /*                                                                      */
    /*  One side problem is the fact that each device has a slightly        */
    /*  different picture of what "now" is.  The way this code decides      */
    /*  who to tick and when is by looking at how far each peripheral's     */
    /*  view of "now" is from the peripheral bus' view of "now".  This      */
    /*  implies that the peripheral bus' view of "now" advances once at     */
    /*  the beginning of the process, and the peripherals then stagger      */
    /*  to catch up as they can.                                            */
    /* -------------------------------------------------------------------- */

    /* -------------------------------------------------------------------- */
    /*  Iterate until we've used up all of our time.                        */
    /* -------------------------------------------------------------------- */
    do
    {
        uint64_t until = soon;

        /* ---------------------------------------------------------------- */
        /*  Pass 1:  Iterate through the list of tickables looking for      */
        /*  the peripheral whose view of "now" is sufficiently behind ours  */
        /*  to warrant a tick.  Remember the size of the smallest such      */
        /*  differential.                                                   */
        /* ---------------------------------------------------------------- */
        for (tick = busp->tickable, ticked = 0; tick ; tick = tick->tickable)
        {
            /* ------------------------------------------------------------ */
            /*  If the peripheral's already busy, skip it.                  */
            /* ------------------------------------------------------------ */
            if (tick->busy)
                continue;

            /* ------------------------------------------------------------ */
            /*  Is this device tickable from the standpoint of its minimum  */
            /*  tick length?  If not, then don't even consider it.          */
            /* ------------------------------------------------------------ */
            if (tick->now + tick->min_tick >= until)
                continue;

            /* ------------------------------------------------------------ */
            /*  Does this peripheral represent a new constraint on our      */
            /*  tick size?  If so, then adjust our tick step size.          */
            /* ------------------------------------------------------------ */
            if (until > tick->now + tick->max_tick)
                until = tick->now + tick->max_tick;

#if DEBUG_TICK
jzp_printf("[%-16s] tick->now=%6llu min=%6u max=%6u | now=%6lld until=%6lld \n",tick->name,(unsigned long long)tick->now,(unsigned)tick->min_tick, (unsigned)tick->max_tick, (unsigned long long)now, (unsigned long long)until); jzp_flush();
#endif
            ticked++;
        }

#if DEBUG_TICK
jzp_printf("now=%llu until=%llu soon=%llu\n", (unsigned long long)now, (unsigned long long)until, (unsigned long long)soon);
#endif

        /* ---------------------------------------------------------------- */
        /*  If nobody was considered for ticking, get out of here.          */
        /* ---------------------------------------------------------------- */
        if (!ticked)
            break;

        /* ---------------------------------------------------------------- */
        /*  Pass 2:  Actually tick all peripherals as close as we can to    */
        /*  the cycle determined by the tick-step in pass 1.                */
        /* ---------------------------------------------------------------- */
        ticked = 0;
        for (tick = busp->tickable ; tick ; tick = tick->tickable)
        {
            uint32_t periph_step;

            /* ------------------------------------------------------------ */
            /*  If the peripheral's already busy, skip it.                  */
            /* ------------------------------------------------------------ */
            if (tick->busy)
                continue;

            /* ------------------------------------------------------------ */
            /*  Calculate the peripheral-specific step.  We need to do      */
            /*  this because each peripheral has a different concept of     */
            /*  'now', but we're trying to step all of time forward to a    */
            /*  fixed destination.                                          */
            /* ------------------------------------------------------------ */
            periph_step = until >= tick->now ? until - tick->now : 0;

            /* ------------------------------------------------------------ */
            /*  Is this tick step larger than the peripheral's min_tick?    */
            /*  And if it is, is its concept of 'now' far enough from ours  */
            /*  to allow us to tick it?                                     */
            /* ------------------------------------------------------------ */
            int condition = tick->now > soon;
            if (strcmp("[Sound]", tick->name)) {
                // Some Samsung devices need this, not sure about side effects
                condition |= tick->min_tick > periph_step;
            }
            if (condition) {
                continue;   /*  Nope:  Skip it. */
            }

            /* ------------------------------------------------------------ */
            /*  Bound the tick size by the peripheral's maximum tick value. */
            /* ------------------------------------------------------------ */
            if (periph_step > tick->max_tick) 
                periph_step = tick->max_tick;
#if DEBUG_TICK
jzp_printf("ticking %16s with step %u, tick->now=%llu\n", tick->name, periph_step, (unsigned long long)tick->now); jzp_flush();
#endif

            tick->busy++;
            periph_step = tick->tick(tick, periph_step);

            tick->now += periph_step;
#if DEBUG_TICK
jzp_printf("tick result: %u, tick->now=%llu\n", periph_step, (unsigned long long)tick->now); jzp_flush();
#endif
            tick->busy--;

            /* ------------------------------------------------------------ */
            /*  Record whether this peripheral really advanced time.  We    */
            /*  use this to detect the case that none of the peripherals    */
            /*  are able to get useful work done because insufficient time  */
            /*  has passed.                                                 */
            /* ------------------------------------------------------------ */
            ticked += periph_step != 0;
        }

        now = until;

    } while (now < soon && ticked);

    bus->busy = 0;

    elapsed = now - bus->now;
    bus->now = now;

    if (busp->pend_reset)
        periph_reset(busp);

    return elapsed;
}

/*
 * ============================================================================
 *  PERIPH_RESET     -- Resets all of the peripherals on the bus
 * ============================================================================
 */
void periph_reset
(
    periph_bus_t    *bus
)
{
    periph_t        *p;

    if (bus->periph.busy)
    {
        bus->pend_reset = true;
        return;
    }

    bus->periph.busy = 1;

    p = bus->list;

    while (p)
    {
        if (p->reset)
            p->reset(p);

        p = p->next;
    }

    bus->periph.busy = 0;
    bus->pend_reset  = false;
}


/*
 * ============================================================================
 *  PERIPH_SER_REGISTER -- registers a peripheral for serialization
 * ============================================================================
 */
void periph_ser_register
(
    periph_t    *per,
    ser_hier_t  *hier
)
{

#ifndef NO_SERIALIZER
#define SER_REG(x,t,l,f)\
        ser_register(hier, #x, &per->x, t, l, f)

    SER_REG(name,      ser_string, 1, SER_INFO);
    SER_REG(addr_base, ser_u32,    1, SER_INFO|SER_HEX);
    SER_REG(addr_mask, ser_u32,    1, SER_INFO|SER_HEX);
    SER_REG(min_tick,  ser_u32,    1, SER_INFO);
    SER_REG(max_tick,  ser_u32,    1, SER_INFO);
    SER_REG(now,       ser_u64,    1, SER_MAND);
    SER_REG(next_tick, ser_u32,    1, SER_MAND);
#else
    UNUSED(per);
    UNUSED(hier);
#endif
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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
