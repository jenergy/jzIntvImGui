/*
 * ============================================================================
 *  Title:    Mouse Support via SDL
 *  Author:   J. Zbiciak
 * ============================================================================
 *
 *  jzIntv doesn't really support mice at this time.  This module is a
 *  simple module to convert xrel/yrel events into direction events, and
 *  capture mouse button events similarly to joystick buttons.
 *
 *  It should be considered highly experimental.
 *
 * ============================================================================
 */

#include "config.h"
#include "sdl_jzintv.h"
#include "event/event_tbl.h"
#include "event/event_plat.h"
#include "mouse/mouse.h"


/* yes, ugly & evil static vars */
static int         last_x = -1, last_y = -1;
static event_num_t last_dir = EVENT_BAD;
static double      to_release = 0;

/* ======================================================================== */
/*  MOUSE_DECODE_MOTION                                                     */
/* ======================================================================== */
LOCAL void mouse_decode_motion
(
    const SDL_Event *const ev,
    event_updn_t    *const ev_updn,
    event_num_t     *const ev_num
)
{
    const int x = ev->motion.x;
    const int y = ev->motion.y;

    if (last_x == -1)
    {
        last_x = x;
        last_y = y;
        return;
    }

    const int dx  = last_x - x;
    const int dy  = last_y - y;
    const int mag = dx * dx + dy * dy;

    if (mag < 16)   /* about 4 pixels in any direction before a dir event */
        return;

    /* For now just decode 4 directions.  Later, decode 16 like joy does. */
    const event_num_t dir =
          abs(dx) > abs(dy) ? ( dx > 0 ? EVENT_MOUSE_W : EVENT_MOUSE_E )
        :                     ( dy > 0 ? EVENT_MOUSE_N : EVENT_MOUSE_S );

    if (dir != last_dir)
    {
        if (last_dir != EVENT_BAD)
        {
            ev_updn[0] = EV_UP;   ev_num[0] = last_dir;
            ev_updn[1] = EV_DOWN; ev_num[1] = dir;
        } else
        {
            ev_updn[0] = EV_DOWN; ev_num[0] = dir;
        }
    }

    last_x   = x;
    last_y   = y;
    last_dir = dir;

    /* Release mouse after 0.1 seconds, unless a new event comes in */
    to_release = get_time() + 0.1;
    return;
}

/* ======================================================================== */
/*  MOUSE_DECODE_BUTTON                                                     */
/* ======================================================================== */
LOCAL void mouse_decode_button(const SDL_Event *const ev,
                               event_num_t *const ev_num)
{
    if (ev->button.button > 31)
    {
        *ev_num = EVENT_IGNORE;
        return;
    }

    *ev_num = EVENT_NUM_OFS(EVENT_MOUSE_BTN_00, ev->button.button);
    return;
}


/* ======================================================================== */
/*  MOUSE_DECODE_EVENT -- Pull apart an SDL_EVENT and turn it into our      */
/*                        internal event numbers.                           */
/* ======================================================================== */
void mouse_decode_event
(
    const SDL_Event *const ev,
    event_updn_t    *const ev_updn,
    event_num_t     *const ev_num)
{
    ev_num[0] = EVENT_IGNORE;
    ev_num[1] = EVENT_IGNORE;

    switch (ev->type)
    {
        case SDL_MOUSEMOTION:
        {
            mouse_decode_motion(ev, ev_updn, ev_num);
            return;
        }

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            ev_updn[0] = ev->type == SDL_MOUSEBUTTONDOWN ? EV_DOWN : EV_UP;
            mouse_decode_button(ev, ev_num);
            return;
        }

        default: return;
    }
}

/* ======================================================================== */
/*  MOUSE_PUMP          -- Decide whether to send a mouse up event          */
/* ======================================================================== */
void mouse_pump(event_updn_t *const ev_updn, event_num_t *const ev_num)
{
    if (last_dir != EVENT_BAD && get_time() > to_release)
    {
        *ev_num  = last_dir;
        *ev_updn = EV_UP;
        last_dir = EVENT_BAD;
    } else
    {
        *ev_num  = EVENT_IGNORE;
    }
}
