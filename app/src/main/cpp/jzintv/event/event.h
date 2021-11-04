/*
 * ============================================================================
 *  Title:    Event interface.
 *  Author:   J. Zbiciak
 * ============================================================================
 *
 * ============================================================================
 *
 * ============================================================================
 */
#ifndef EVENT_H_
#define EVENT_H_

/* ======================================================================== */
/*  EVENT_T          -- Event Subsystem object                              */
/* ======================================================================== */
typedef struct evt_pvt_t evt_pvt_t;
typedef struct event_t
{
    periph_t    periph;         /* Yes, it's a peripheral.  Surprise!       */
    evt_pvt_t  *pvt;            /* Private structure                        */
} event_t;

/* ======================================================================== */
/*  EVENT_INIT       -- Initializes the Event subsystem.                    */
/* ======================================================================== */
int event_init
(
    event_t *const event,
    const bool     enable_mouse,
    const int      initial_event_map
);

/* ======================================================================== */
/*  EVENT_MAP        -- Maps an event to a particular AND/OR mask set       */
/* ======================================================================== */
int event_map
(
    event_t    *event,          /* Event_t structure being set up.          */
    const char *name,           /* Name of event to map.                    */
    int         map_num,        /* Keyboard mapping number                  */
    const char* jzintv_event_name,
    uint32_t   *word,           /* Word modified by event, (NULL to ignore) */
    uint32_t    and_mask[2],    /* AND masks for event up/down.             */
    uint32_t    or_mask[2]      /* OR masks for event up/down.              */
);

/* ======================================================================== */
/*  EVENT_COMBINE    -- Register a combo event as COMBOxx                   */
/* ======================================================================== */
int event_combine
(
    event_t     *const event,
    const char  *const event_name1,
    const char  *const event_name2,
    const int          combo_num
);

/* ======================================================================== */
/*  EVENT_SET_COMBO_COALESCE                                                */
/*  Adjust the coalesce timer for combo matching.                           */
/* ======================================================================== */
void event_set_combo_coalesce
(
    event_t     *const event,
    const double       coalesce_time
);

typedef enum
{
    EV_MAP_NOP = 0,
    EV_MAP_SET_0, EV_MAP_SET_1, EV_MAP_SET_2, EV_MAP_SET_3, 
    EV_MAP_NEXT,  EV_MAP_PREV,  EV_MAP_POP,
    EV_MAP_PSH_0, EV_MAP_PSH_1, EV_MAP_PSH_2, EV_MAP_PSH_3
} ev_map_change_req;

/* ======================================================================== */
/*  EVENT_CHANGE_ACTIVE_MAP  -- Change the current input mapping.           */
/* ======================================================================== */
void event_change_active_map(event_t *const event, 
                             const ev_map_change_req map_change_req);

#endif /*EVENT_H*/
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
