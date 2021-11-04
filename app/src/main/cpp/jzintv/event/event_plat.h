/*
 * ============================================================================
 *  Title:    Private platform event interface
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This describes the interface between Event and Event Plat.  It is a
 *  bidirectional interface.
 *
 *  Functions that start with "event_plat_" are implemented by the platform
 *  specific code.  Functions that start with just "event_" are implemented in
 *  event.c as part of the core.
 * ============================================================================
 */

#ifndef EVENT_PLAT_H_
#define EVENT_PLAT_H_

#ifndef EVENT_H_
typedef struct evt_pvt_t evt_pvt_t;
#endif /* EVENT_H_ */

typedef enum { EV_DOWN = 1, EV_UP = 0 } event_updn_t;

/* ======================================================================== */
/*  EVENT_PLAT_INIT  -- Initializes the platform-specific code.             */
/* ======================================================================== */
int event_plat_init
(
    const bool       enable_mouse,  /*  Enable mouse events?                */
    evt_pvt_t *const evt_pvt,       /*  Pass back to event core.            */
    void     **const ptr_plat_pvt   /*  For plat-specifc private struct.    */
);

/* ======================================================================== */
/*  EVENT_PLAT_DTOR  -- Shuts down the platform-specific code.              */
/* ======================================================================== */
void event_plat_dtor(void *const plat_pvt);

/* ======================================================================== */
/*  EVENT_PLAT_PUMP  -- Pump events, even if we're not going to consume.    */
/*  Called at the start of a tick, but before we decide to "cork."          */
/* ======================================================================== */
void event_plat_pump(evt_pvt_t *const pvt, void *const plat_pvt);

/* ======================================================================== */
/*  EVENT_PLAT_TICK  -- Performs the bulk of the tick cycle.                */
/*                                                                          */
/*  Returns true if we should make an early exit before dequeuing.  This    */
/*  typically happens when we need to "cork" for a combo.                   */
/* ======================================================================== */
bool event_plat_tick(evt_pvt_t *const pvt, void *const plat_pvt);

/* ======================================================================== */
/*  EVENT_PLAT_TICK_LATE -- Performs deferred tick cycle tasks, after we    */
/*                          have drained our internal event queue.          */
/*                                                                          */
/*  Currently this is only used by SDL's experimental mouse processing.     */
/*  Not sure if this is really necessary.                                   */
/* ======================================================================== */
void event_plat_tick_late(evt_pvt_t *const pvt, void *const plat_pvt);

/* ======================================================================== */
/*  EVENT_QUEUE_HAS_ROOM -- Returns true if there's room for N events.      */
/* ======================================================================== */
int event_queue_has_room(evt_pvt_t *const pvt, const int count);

/* ======================================================================== */
/*  EVENT_ENQUEUE    -- Internal event queue containing expanded events.    */
/* ======================================================================== */
void event_enqueue
(
    evt_pvt_t *const   pvt,
    const event_updn_t event_updn,
    const event_num_t  event_num
);

/* ======================================================================== */
/*  EVENT_ENQUEUE_CHECK_COMBO                                               */
/*                                                                          */
/*  Enqueues an event, or combo-related events associated with this event.  */
/*  Returns "true" if we need to cork further inputs.  Callers should stop  */
/*  draining event queue at this time and wait until the next "tick."       */
/* ======================================================================== */
bool event_enqueue_check_combo
(
    evt_pvt_t *const   pvt,
    const event_updn_t event_updn,
    const event_num_t  event_num
);

/* ======================================================================== */
/*  EVENT_NUM_OFS    -- Compute an event_num_t as an offset from a base.    */
/* ======================================================================== */
#define EVENT_NUM_OFS(base, ofs) ((event_num_t)((base) + (ofs)))

#endif /* EVENT_PLAT_H_ */

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
/*                 Copyright (c) 2020-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
