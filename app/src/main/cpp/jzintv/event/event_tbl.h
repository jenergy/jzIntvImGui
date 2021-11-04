/*
 * ============================================================================
 *  Title:    Event enumeration and related tables and helpers.
 *  Author:   J. Zbiciak
 * ============================================================================
 */
#ifndef EVENT_TBL_H_
#define EVENT_TBL_H_

/* ======================================================================== */
/*  EVENT_NUM_T  -- An enumeration of all of the event numbers supported.   */
/*                                                                          */
/*  These event numbers are internal to jzIntv.  Any platform-specific      */
/*  code must map its event numbers to this enumeration before calling      */
/*  into the event core.                                                    */
/* ======================================================================== */
typedef enum
{
    /* -------------------------------------------------------------------- */
    /*  Report errors with EVENT_BAD, the only negative event number.       */
    /* -------------------------------------------------------------------- */
    EVENT_BAD = -1,

#define EVT_DECL(name, event) event,
#define EVT_DECL_A(name, event) /* empty */
#include "event/event_tbl.inc"
#undef EVT_DECL_A
#undef EVT_DECL

    /* -------------------------------------------------------------------- */
    /*  The total number of valid event numbers.  All valid event numbers   */
    /*  are below this value.                                               */
    /* -------------------------------------------------------------------- */
    EVENT_COUNT,

    /* -------------------------------------------------------------------- */
    /*  And if we just want to ignore an event, we set it to this.          */
    /* -------------------------------------------------------------------- */
    EVENT_IGNORE
} event_num_t;

/* ======================================================================== */
/*  EVENT_NAME_TO_NUM -- Convert an event name to an event_num_t.           */
/*                                                                          */
/*  For now, a slow linear search.  Do not place this in a critical path.   */
/*  Returns EVENT_BAD if not found.  EVENT_BAD is the only negative event.  */
/* ======================================================================== */
int event_name_to_num(const char *const event_name);

/* ======================================================================== */
/*  EVENT_NUM_TO_NAME -- Convert an event number to a name, or NULL.        */
/* ======================================================================== */
const char *event_num_to_name(const event_num_t event_num);

/* ======================================================================== */
/*  EVENT_VALID  -- Returns true if the event number is valid.              */
/* ======================================================================== */
#define EVENT_VALID(ev)  ((ev) >= 0 && (ev) < EVENT_COUNT)

#endif /*EVENT_TBL_H*/
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
