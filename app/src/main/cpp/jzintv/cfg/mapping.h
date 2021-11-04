/*
 * ============================================================================
 *  Title:    Event binding tables.
 *  Author:   J. Zbiciak
 * ============================================================================
 *  These tables specify the bindable events and the default bindings.
 * ============================================================================
 *  CFG_INIT     -- Parse command line and get started
 *  CFG_FILE     -- Parse a config file and extend the state of the machine.
 * ============================================================================
 */

#ifndef MAPPING_H_
#define MAPPING_H_

/*
 * ============================================================================
 *  CFG_EVTACT_T     -- Human-readable name associations for various jzIntv
 *                      event actions.
 * ============================================================================
 */
typedef struct cfg_evtact_t
{
    const char  *name;          /* Event action name                        */
    uint32_t    *word;          /* Word modified by an input.               */
    uint32_t    and_mask[2];    /* Up/down AND masks.                       */
    uint32_t    or_mask [2];    /* Up/down OR masks.                        */
} cfg_evtact_t;

/*
 * ============================================================================
 *  CFG_KBD_T   -- Human-readable name associations all possible keyboard
 *                 inputs, along with the associated default bindings.
 *                 The user-specified config file can change these at
 *                 run-time.
 * ============================================================================
 */
typedef struct cfg_kbd_t
{
    const char  *key;               /* Name of input.                       */
    const char  *event_action[4];   /* Default actions to take for input.   */
} cfg_kbd_t;


/* ------------------------------------------------------------------------ */
/*  jzIntv internal event name table.  Keyboard and joystick inputs may be  */
/*  bound to any of these event names.  This table also ties the event      */
/*  names to the actual bits that the event fiddles with.                   */
/* ------------------------------------------------------------------------ */
extern cfg_evtact_t cfg_event_action[];
extern          int cfg_event_action_cnt;

/* ------------------------------------------------------------------------ */
/*  Default key bindings table.                                             */
/* ------------------------------------------------------------------------ */
extern cfg_kbd_t cfg_key_bind[];

/* ------------------------------------------------------------------------ */
/*  Some constants that need a better home:                                 */
/* ------------------------------------------------------------------------ */
#define PAUSE_NOP  (0)              /*  Nothing to do.                      */
#define PAUSE_TOG  (1)              /*  Toggle pause on/off.                */
#define PAUSE_ON   (2)              /*  Force pause on.                     */
#define PAUSE_OFF  (3)              /*  Force pause off.                    */
#define PAUSE_2SEC (4)              /*  2 second pause (window/fullsc flip) */

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
/*                 Copyright (c) 1998-2000, Joseph Zbiciak                  */
/* ======================================================================== */
