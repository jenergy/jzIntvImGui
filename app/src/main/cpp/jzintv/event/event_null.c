/*
 * ============================================================================
 *  Title:    Null Event Handling Subsystem
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This backend generates no events, and has the event core short-circuit.
 * ============================================================================
 */

#include "config.h"
#include "event/event_tbl.h"
#include "event/event_plat.h"

/* ======================================================================== */
/*  EVENT_PLAT_INIT  -- Initializes the platform-specific code.             */
/* ======================================================================== */
int event_plat_init
(
    const bool       enable_mouse,  /*  Enable mouse events?                */
    evt_pvt_t *const evt_pvt,       /*  Pass back to event core.            */
    void     **const ptr_plat_pvt   /*  For plat-specifc private struct.    */
)
{
    UNUSED(enable_mouse);
    UNUSED(evt_pvt);
    *ptr_plat_pvt = NULL;
    return 0;
}

/* ======================================================================== */
/*  EVENT_PLAT_DTOR  -- Shuts down the platform-specific code.              */
/* ======================================================================== */
void event_plat_dtor(void *const plat_pvt) { UNUSED(plat_pvt); return; }

/* ======================================================================== */
/*  EVENT_PLAT_PUMP  -- Pump events, even if we're not going to consume.    */
/*  Called at the start of a tick, but before we decide to "cork."          */
/* ======================================================================== */
void event_plat_pump(evt_pvt_t *const pvt, void *const plat_pvt)
{
    UNUSED(pvt);
    UNUSED(plat_pvt);
}

/* ======================================================================== */
/*  EVENT_PLAT_TICK  -- Performs the bulk of the tick cycle.                */
/*                                                                          */
/*  Returns true if we should make an early exit before dequeuing.  This    */
/*  typically happens when we need to "cork" for a combo.                   */
/* ======================================================================== */
bool event_plat_tick(evt_pvt_t *const pvt, void *const plat_pvt)
{
    UNUSED(pvt);
    UNUSED(plat_pvt);
    return true;    /* always short-circuit the main event core. */
}

/* ======================================================================== */
/*  EVENT_PLAT_TICK_LATE -- Performs deferred tick cycle tasks, after we    */
/*                          have drained our internal event queue.          */
/*                                                                          */
/*  Currently this is only used by SDL's experimental mouse processing.     */
/*  Not sure if this is really necessary.                                   */
/* ======================================================================== */
void event_plat_tick_late(evt_pvt_t *const pvt, void *const plat_pvt)
{
    UNUSED(pvt);
    UNUSED(plat_pvt);
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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
