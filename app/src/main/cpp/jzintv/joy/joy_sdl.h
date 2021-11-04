/*
 * ============================================================================
 *  Title:    Joystick Support via SDL
 *  Author:   J. Zbiciak
 * ============================================================================
 */

#ifndef JOY_SDL_H_
#define JOY_SDL_H_

/* ======================================================================== */
/*  JOY_DECODE_EVENT -- Pull apart an SDL_EVENT and turn it into our        */
/*                      internal event numbers.                             */
/*                                                                          */
/*  Returns non-zero if the event should be considered for 'combos.'        */
/* ======================================================================== */
bool joy_decode_event
(
    const SDL_Event *const ev,
    event_updn_t    *const ev_updn,
    event_num_t     *const ev_num
);

#endif  /* JOY_SDL_H_ */

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
/*                 Copyright (c) 2005-2020, Joseph Zbiciak                  */
/* ======================================================================== */
