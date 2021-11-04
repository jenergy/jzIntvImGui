/*
 * ============================================================================
 *  Title:    Input support for GP2X
 *  Author:   J. Zbiciak
 * ============================================================================
 *
 * ============================================================================
 */

#include "config.h"
#include "sdl_jzintv.h"
#include "periph/periph.h"
#include "pads/pads.h"
#include "event/event.h"
#include "event/event_tbl_sdl.h"
#include "joy/joy.h"

int gp2x_joystick_mode = 0;

/* ======================================================================== */
/*  JOY_INIT -- Enumerate available joysticks and features                  */
/* ======================================================================== */
int joy_init(int verbose, char *cfg[])
{
    if (cfg && cfg[0])
        gp2x_joystick_mode = atoi(cfg[0]);


    if (gp2x_joystick_mode > 6)
        gp2x_joystick_mode = 0;

    SDL_JoystickOpen(0);
    return 0;
}

static uint8_t gp2x_down[8] = { UP, UP, UP, UP, UP, UP, UP, UP };
static uint8_t virt_down[8] = { UP, UP, UP, UP, UP, UP, UP, UP };
static uint8_t virt_next[8] = { UP, UP, UP, UP, UP, UP, UP, UP };


/* ======================================================================== */
/*  JOY_DECODE_EVENT -- Pull apart an SDL_EVENT and turn it into our        */
/*                      internal event numbers.                             */
/* ======================================================================== */
void joy_decode_event(SDL_Event *ev, int *ev_updn, uint32_t *ev_num,
                                     int *ex_updn, uint32_t *ex_num)
{
    int gp2x, i, vdn = -1, vup = -1;

    if (ev->type != SDL_JOYBUTTONDOWN && ev->type != SDL_JOYBUTTONUP)
    {
        *ev_num  = EVENT_IGNORE;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  If there is no bias or this is not a thumb pad button, just return  */
    /*  the recoded event as-is.                                            */
    /* -------------------------------------------------------------------- */
    if (gp2x_joystick_mode < 2 || ev->jbutton.button >= 8)
    {
        *ev_updn = ev->type == SDL_JOYBUTTONDOWN ? DOWN : UP;
        *ev_num  = EVENT_JS0_BTN_00 + ev->jbutton.button;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Use the button event to drive a filter state machine.  The filter   */
    /*  tracks which buttons are actually down.  It generates filtered      */
    /*  "virtual buttons" from these.  It then sends events based on the    */
    /*  virtual buttons.  We will sometimes send pairs of events.           */
    /* -------------------------------------------------------------------- */
    gp2x = ev->jbutton.button;

    gp2x_down[gp2x] = ev->type == SDL_JOYBUTTONDOWN ? DOWN : UP;

    switch (gp2x_joystick_mode)
    {
        /* ---------------------------------------------------------------- */
        /*  8 Direction, UD/LR Bias.                                        */
        /*  When UDLR or UDLR + diag pressed, set UDLR.  Otherwise diag.    */
        /* ---------------------------------------------------------------- */
        case 2:
        {
            if (gp2x_down[0] || gp2x_down[2] || gp2x_down[4] || gp2x_down[6])
            {
                virt_next[1] = virt_next[3] = virt_next[5] = virt_next[7] = UP;
                virt_next[0] = gp2x_down[0];
                virt_next[2] = gp2x_down[2];
                virt_next[4] = gp2x_down[4];
                virt_next[6] = gp2x_down[6];
            } else
            {
                virt_next[0] = virt_next[2] = virt_next[4] = virt_next[6] = UP;
                virt_next[1] = gp2x_down[1];
                virt_next[3] = gp2x_down[3];
                virt_next[5] = gp2x_down[5];
                virt_next[7] = gp2x_down[7];
            }

            break;
        }

        /* ---------------------------------------------------------------- */
        /*  8 Direction, diagonal bias                                      */
        /* ---------------------------------------------------------------- */
        case 3:
        {
            if (gp2x_down[1] || gp2x_down[3] || gp2x_down[5] || gp2x_down[7])
            {
                virt_next[0] = virt_next[2] = virt_next[4] = virt_next[6] = UP;
                virt_next[1] = gp2x_down[1];
                virt_next[3] = gp2x_down[3];
                virt_next[5] = gp2x_down[5];
                virt_next[7] = gp2x_down[7];
            } else
            {
                virt_next[1] = virt_next[3] = virt_next[5] = virt_next[7] = UP;
                virt_next[0] = gp2x_down[0];
                virt_next[2] = gp2x_down[2];
                virt_next[4] = gp2x_down[4];
                virt_next[6] = gp2x_down[6];
            }

            break;
        }

        /* ---------------------------------------------------------------- */
        /*  8 Direction, dead zones                                         */
        /* ---------------------------------------------------------------- */
        case 4:
        {
            int e,o;

            e = gp2x_down[0] || gp2x_down[2] || gp2x_down[4] || gp2x_down[6];
            o = gp2x_down[1] || gp2x_down[3] || gp2x_down[5] || gp2x_down[7];

            if (e && !o)
            {
                virt_next[1] = virt_next[3] = virt_next[5] = virt_next[7] = UP;
                virt_next[0] = gp2x_down[0];
                virt_next[2] = gp2x_down[2];
                virt_next[4] = gp2x_down[4];
                virt_next[6] = gp2x_down[6];
            } else if (o & !e)
            {
                virt_next[0] = virt_next[2] = virt_next[4] = virt_next[6] = UP;
                virt_next[1] = gp2x_down[1];
                virt_next[3] = gp2x_down[3];
                virt_next[5] = gp2x_down[5];
                virt_next[7] = gp2x_down[7];
            } else
            {
                virt_next[0] = virt_next[2] = virt_next[4] = virt_next[6] = UP;
                virt_next[1] = virt_next[3] = virt_next[5] = virt_next[7] = UP;
            }

            break;
        }

        /* ---------------------------------------------------------------- */
        /*  4 direction, UD/LR bias                                         */
        /* ---------------------------------------------------------------- */
        case 5:
        {
            virt_next[0] = gp2x_down[0] | (gp2x_down[1] & !gp2x_down[2]);
            virt_next[2] = gp2x_down[2] | (gp2x_down[3] & !gp2x_down[4]);
            virt_next[4] = gp2x_down[4] | (gp2x_down[5] & !gp2x_down[6]);
            virt_next[6] = gp2x_down[6] | (gp2x_down[7] & !gp2x_down[0]);
            break;
        }

        /* ---------------------------------------------------------------- */
        /*  4 direction, diagonal bias                                      */
        /* ---------------------------------------------------------------- */
        case 6:
        {
            virt_next[1] = gp2x_down[1] | (gp2x_down[2] & !gp2x_down[3]);
            virt_next[3] = gp2x_down[3] | (gp2x_down[4] & !gp2x_down[5]);
            virt_next[5] = gp2x_down[5] | (gp2x_down[6] & !gp2x_down[7]);
            virt_next[7] = gp2x_down[7] | (gp2x_down[0] & !gp2x_down[1]);
            break;
        }

        default:
        {
            fprintf(stderr, "Bad GP2X pad mode\n");
            exit(1);
        }
    }


    /* -------------------------------------------------------------------- */
    /*  Look for virtual buttons that got depressed.                        */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 8; i++)
    {
        if (virt_next[i] && !virt_down[i])
        {
            virt_down[i] = DOWN;
            *ev_num  = EVENT_JS0_BTN_00 + i;
            *ev_updn = DOWN;
            break;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Look for virtual buttons that got released.                         */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 8; i++)
    {
        if (!virt_next[i] && virt_down[i])
        {
            virt_down[i] = UP;
            *ex_num  = EVENT_JS0_BTN_00 + i;
            *ex_updn = UP;
            break;
        }
    }
}
