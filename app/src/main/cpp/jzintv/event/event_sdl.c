/*
 * ============================================================================
 *  Title:    Event Handling Driver for both SDL1 and SDL2
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This is the platform-specific driver for SDL 1.2.x and SDL 2.x
 *  The SDL1 and SDL2 specific pieces are in event_sdl1.c and event_sdl2.c.
 * ============================================================================
 */

#include "config.h"
#include "sdl_jzintv.h"
#include "event/event_tbl.h"
#include "event/event_plat.h"
#include "event/event_sdl_pvt.h"
#include "joy/joy.h"
#include "joy/joy_sdl.h"
#include "mouse/mouse.h"

/* TODO:  Migrate Enscripten support to SDL2. */
#if defined(__EMSCRIPTEN__)
# define SDL_EventState(x,y) ((void)(x), (void)(y))
#endif

/* ======================================================================== */
/*  Some very minor platform-specific tweaks.                               */
/* ======================================================================== */
#ifdef WII
# define ENABLE_JOY_EVENTS SDL_IGNORE
#else
# define ENABLE_JOY_EVENTS SDL_ENABLE
#endif

#ifdef N900
# define ENABLE_SYSWM_EVENTS SDL_IGNORE
#else
# define ENABLE_SYSWM_EVENTS SDL_ENABLE
#endif

#ifdef USE_SDL2
# define WINDOW_EVENT_CATEGORY SDL_WINDOWEVENT
#else
# define WINDOW_EVENT_CATEGORY SDL_ACTIVEEVENT
#endif

typedef struct event_sdl_pvt
{
    bool mouse_enabled;
} event_sdl_pvt_t;

/* ======================================================================== */
/*  EVENT_PLAT_INIT  -- Initializes the SDL1 Event subsystem.               */
/* ======================================================================== */
int event_plat_init(
        const bool enable_mouse,  /*  Enable mouse events?                */
        evt_pvt_t *const evt_pvt,       /*  Event core private ptr, if needed.  */
        void **const ptr_plat_pvt   /*  Our allocated private struct.       */
)
{
    SDL_Event dummy;

    UNUSED(evt_pvt);

    /* -------------------------------------------------------------------- */
    /*  Set up our "private" structure.                                     */
    /* -------------------------------------------------------------------- */
    event_sdl_pvt_t *plat_pvt = CALLOC(event_sdl_pvt_t, 1);

    if (!plat_pvt)
    {
        fprintf(stderr, "event_sdl1: Unable to allocate private state.\n");
        return -1;
    }

    *ptr_plat_pvt = plat_pvt;

    /* -------------------------------------------------------------------- */
    /*  Set up SDL to filter out the events we're NOT interested in...      */
    /* -------------------------------------------------------------------- */
    if (!enable_mouse)
    {
        SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
        SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
        SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
    } else
    {
        plat_pvt->mouse_enabled = true;
        SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
        SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
        SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
    }
    SDL_EventState(SDL_SYSWMEVENT, ENABLE_SYSWM_EVENTS);

    /* -------------------------------------------------------------------- */
    /*  ...and leave us only with the events we ARE interested in.          */
    /* -------------------------------------------------------------------- */
    SDL_EventState(WINDOW_EVENT_CATEGORY, SDL_ENABLE);
    SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
    SDL_EventState(SDL_KEYUP, SDL_ENABLE);
    SDL_EventState(SDL_QUIT, SDL_ENABLE);
    SDL_EventState(SDL_JOYAXISMOTION, ENABLE_JOY_EVENTS);
    SDL_EventState(SDL_JOYHATMOTION, ENABLE_JOY_EVENTS);
    SDL_EventState(SDL_JOYBUTTONDOWN, ENABLE_JOY_EVENTS);
    SDL_EventState(SDL_JOYBUTTONUP, ENABLE_JOY_EVENTS);
    SDL_EventState(SDL_JOYBALLMOTION, ENABLE_JOY_EVENTS);
    SDL_JoystickEventState(ENABLE_JOY_EVENTS);

    /* -------------------------------------------------------------------- */
    /*  Drain the event queue right now to clear any initial events.        */
    /* -------------------------------------------------------------------- */
    while (SDL_PollEvent(&dummy))
        ;

    /* -------------------------------------------------------------------- */
    /*  Done!                                                               */
    /* -------------------------------------------------------------------- */
    return 0;
}

/* ======================================================================== */
/*  EVENT_PLAT_DTOR  -- Tear down the event engine.                         */
/* ======================================================================== */
void event_plat_dtor(void *const plat_pvt)
{
    if (plat_pvt) free(plat_pvt);
    joy_dtor();
}

/* ======================================================================== */
/*  EVENT_PLAT_PUMP  -- Pump the event engine before the full tick starts.  */
/* ======================================================================== */
void event_plat_pump(evt_pvt_t *const evt_pvt, void *const void_plat_pvt)
{
    UNUSED(evt_pvt);
    UNUSED(void_plat_pvt);

    SDL_PumpEvents();
}

evt_pvt_t* evt_pvt_inner;
void event_enqueue_custom(int press_status, const char *ev_name) {
    if (ev_name != NULL) {
        int ev_num = event_name_to_num(ev_name);
        event_num_t event_num = (event_num_t) ev_num;
        event_updn_t event_updn = press_status == SDL_KEYUP ? EV_UP : EV_DOWN;
        event_enqueue(evt_pvt_inner, event_updn, event_num);
    }
}

extern bool consume_special_event(int map, SDL_Event *event);
extern bool check_pause_event(SDL_Event *event, const event_num_t event_num[2], bool may_combo);
extern int get_current_map(evt_pvt_t *const pvt);
/* ======================================================================== */
/*  EVENT_PLAT_TICK  -- Performs the bulk of the tick cycle.                */
/*                                                                          */
/*  Returns true if we need to cork events, typically due to a combo.       */
/* ======================================================================== */
bool event_plat_tick(evt_pvt_t *const evt_pvt, void *const void_plat_pvt)
{
    const event_sdl_pvt_t *const plat_pvt = (event_sdl_pvt_t *) void_plat_pvt;
    SDL_Event event;

    /* -------------------------------------------------------------------- */
    /*  Now, process all pending events.                                    */
    /* -------------------------------------------------------------------- */
#ifdef WII
    getWiiJoyEvents();
#endif
    while (event_queue_has_room(evt_pvt, 4) && SDL_PollEvent(&event))
    {
        evt_pvt_inner = evt_pvt;
        if (consume_special_event(get_current_map(evt_pvt_inner), &event)){
            continue;
        }
        event_num_t send_event_num[2] = {EVENT_IGNORE, EVENT_IGNORE};
        switch (event.type)
        {
            /* ------------------------------------------------------------ */
            /*  Handle keypresses and releases by sending the decoded       */
            /*  keysym value as event #.                                    */
            /* ------------------------------------------------------------ */
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                const event_updn_t event_updn =
                        event.type == SDL_KEYUP ? EV_UP : EV_DOWN;

                const event_num_t event_num = event_sdl_translate_key(&event);
                send_event_num[0] = event_num;
                if (check_pause_event(&event, send_event_num, false)){
                    continue;
                }

                if (event_enqueue_check_combo(evt_pvt, event_updn, event_num))
                    return true;

                break;
            }

                /* ------------------------------------------------------------ */
                /*  Outsource all the joystick event decoding...                */
                /* ------------------------------------------------------------ */
            case SDL_JOYAXISMOTION:
            case SDL_JOYHATMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            {
                event_updn_t event_updn[2] = {EV_DOWN, EV_DOWN};
                event_num_t event_num[2] = {EVENT_IGNORE, EVENT_IGNORE};

                const bool may_combo =
                        joy_decode_event(&event, event_updn, event_num);

                assert(!may_combo || event_num[1] == EVENT_IGNORE);
                send_event_num[0] = event_num[0];
                send_event_num[1] = event_num[1];
                if (check_pause_event(&event, send_event_num, may_combo)){
                    continue;
                }
                if (may_combo)
                {
                    if (event_enqueue_check_combo(
                            evt_pvt, event_updn[0], event_num[0]))
                        return true;
                } else
                {
                    event_enqueue(evt_pvt, event_updn[0], event_num[0]);
                    event_enqueue(evt_pvt, event_updn[1], event_num[1]);
                }
                break;
            }

                /* ------------------------------------------------------------ */
                /*  Outsource all mouse event decoding...                       */
                /* ------------------------------------------------------------ */
            case SDL_MOUSEMOTION:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                if (plat_pvt->mouse_enabled)
                {
                    event_updn_t event_updn[2] = {EV_DOWN, EV_DOWN};
                    event_num_t event_num[2] = {EVENT_IGNORE, EVENT_IGNORE};

                    mouse_decode_event(&event, event_updn, event_num);

                    event_enqueue(evt_pvt, event_updn[0], event_num[0]);
                    event_enqueue(evt_pvt, event_updn[1], event_num[1]);
                }
                break;
            }

                /* ------------------------------------------------------------ */
                /*  And finally, handle the QUIT event.                         */
                /* ------------------------------------------------------------ */
            case SDL_QUIT:
            {
                event_enqueue(evt_pvt, EV_DOWN, EVENT_QUIT);
                break;
            }

                /* ------------------------------------------------------------ */
                /*  If it's unhandled, pass down to the SDL-version specific    */
                /*  handlers to see if they understand it.                      */
                /* ------------------------------------------------------------ */
            default:
            {
                /* If this returns 'true', that means it queued a combo. */
                if (event_sdl_unhandled_event(evt_pvt, &event))
                    return true;
                break;
            }
        }
    }

    return false;  /* did not need to cork. */
}

/* ======================================================================== */
/*  EVENT_PLAT_TICK_LATE -- Performs deferred tick cycle tasks, after we    */
/*                          have drained our internal event queue.          */
/*                                                                          */
/*  Currently this is only used by SDL's experimental mouse processing.     */
/*  Not sure if this is really necessary.                                   */
/* ======================================================================== */
void event_plat_tick_late(evt_pvt_t *const evt_pvt, void *const void_plat_pvt)
{
    const event_sdl_pvt_t *const plat_pvt = (event_sdl_pvt_t *) void_plat_pvt;

    /* -------------------------------------------------------------------- */
    /*  If the mouse is enabled, see if we need to do any delayed events.   */
    /* -------------------------------------------------------------------- */
    if (plat_pvt->mouse_enabled)
    {
        event_updn_t event_updn = EV_DOWN;
        event_num_t event_num = EVENT_IGNORE;

        mouse_pump(&event_updn, &event_num);
        event_enqueue(evt_pvt, event_updn, event_num);
    }
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
