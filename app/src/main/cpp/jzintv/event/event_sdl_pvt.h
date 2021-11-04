/* ======================================================================== */
/*  Private interface for event_sdl.c to event_sdl{1,2}.c                   */
/* ======================================================================== */

#ifndef EVENT_SDL_PVT_H_
#define EVENT_SDL_PVT_H_

/* ======================================================================== */
/*  EVENT_SDL_TRANSLATE_KEY  -- Translate an SDLKey to event_num_t.         */
/* ======================================================================== */
event_num_t event_sdl_translate_key(const SDL_Event *const event);

/* ======================================================================== */
/*  EVENT_SDL_UNHANDLED_EVENT -- Hook for SDL-version specific events.      */
/*  Returns true if the event ended up queuing a COMBO, false otherwise.    */
/* ======================================================================== */
bool event_sdl_unhandled_event(evt_pvt_t *const evt_pvt,
                               const SDL_Event *const event);

#endif /* EVENT_SDL_PVT_H_ */

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
