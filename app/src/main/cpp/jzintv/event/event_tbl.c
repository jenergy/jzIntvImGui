/*
 * ============================================================================
 *  Title:    Event Subsystem Tables
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This file contains lookup tables used by the event subsystem, and
 *  lookup functions on those tables.
 * ============================================================================
 *  Private tables:
 *
 *  NAME_TO_NUM -- Maps event names to event numbers.
 *  NUM_TO_NAME -- Maps event numbers to event names.
 *
 *  External APIs:
 *
 *  EVENT_NAME_TO_NUM   Convert an event name to an event number.
 *  EVENT_NUM_TO_NAME   Convert an event number to an event name.
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "event/event.h"
#include "event/event_tbl.h"

/* ======================================================================== */
/*  Sanity check that the following events go in exactly this order, and    */
/*  are contiguous:                                                         */
/*                                                                          */
/*                LMETA, RMETA, LSUPER, RSUPER, LGUI, RGUI                  */
/*                                                                          */
/*  This ensures that files that include event_tbl.h see a consistent set   */
/*  of enum values regardless of whether we build for SDL1 or SDL2, so the  */
/*  same .o files could be used for both links.                             */
/* ======================================================================== */
STATIC_ASSERT(EVENT_RMETA  == EVENT_LMETA + 1 &&
              EVENT_LSUPER == EVENT_LMETA + 2 &&
              EVENT_RSUPER == EVENT_LMETA + 3 &&
              EVENT_LGUI   == EVENT_LMETA + 4 &&
              EVENT_RGUI   == EVENT_LMETA + 5,
              "Check declaration order for events "
              "LMETA/RMETA/LSUPER/RSUPER/LGUI/RGUI "
              "in event/event_tbl.inc");

/* ======================================================================== */
/*  EVENT_NAME_T     -- Structure used to map event names to numbers.       */
/* ======================================================================== */
typedef struct event_name_to_num
{
    const char *name;           /* Printable name for event.        */
    event_num_t num;            /* Event index number into mask_tbl */
} event_name_to_num_t;

/* ======================================================================== */
/*  NAME_TO_NUM -- Maps event names to event numbers.                       */
/* ======================================================================== */
#define EVT_DECL(name, num)   { name, num },
#define EVT_DECL_A(name, num) EVT_DECL(name, num)
static const event_name_to_num_t name_to_num[] =
{
#include "event_tbl.inc"
};
#undef EVT_DECL
#undef EVT_DECL_A

static const int name_count = sizeof(name_to_num) / sizeof(name_to_num[0]);

/* ======================================================================== */
/*  NUM_TO_NAME -- Maps event numbers to event names.                       */
/* ======================================================================== */
#define EVT_DECL(name, num) [num] = name,
#define EVT_DECL_A(name, num) /* drop aliases */
static const char *const num_to_name[] =
{
#include "event_tbl.inc"
};
#undef EVT_DECL
#undef EVT_DECL_A

/* ======================================================================== */
/*  EVENT_NAME_TO_NUM -- Convert an event name to an event_num_t.           */
/*                                                                          */
/*  For now, a slow linear search.  Do not place this in a critical path.   */
/*  Returns EVENT_BAD if not found.  EVENT_BAD is the only negative event.  */
/* ======================================================================== */
event_num_t event_name_to_num(const char *const event_name)
{
    /* "!BAD!" is an internal name.  Don't let it leak out. */
    if (!stricmp("!BAD!", event_name))
        return EVENT_BAD;

    for (int i = 0; i < name_count; ++i)
        if (!stricmp(event_name, name_to_num[i].name))
            return name_to_num[i].num;

    return EVENT_BAD;
}

/* ======================================================================== */
/*  EVENT_NUM_TO_NAME -- Convert an event number to a name, or NULL.        */
/* ======================================================================== */
const char *event_num_to_name(const event_num_t event_num)
{
    if (event_num < 0 || event_num >= EVENT_COUNT)
        return NULL;

    /* "!BAD!" is an internal name.  Don't let it leak out. */
    if (!stricmp("!BAD!", num_to_name[(int)event_num]))
        return NULL;

    return num_to_name[(int)event_num];
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
