/*
 * ============================================================================
 *  Title:    Joystick Support via SDL
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements jzIntv's joystick support.  Specifically, it:
 *
 *   -- Enumerates available joysticks
 *   -- Discovers capabilities of available joysticks
 *   -- Binds joystick inputs to internal event tags
 *   -- Allows configuring joystick->event mapping
 *   -- Decodes analog inputs into Inty's 16-direction input
 *
 * ============================================================================
 */

#include "config.h"
#include "sdl_jzintv.h"
#include "event/event_tbl.h"
#include "event/event_plat.h"
#include "joy/joy.h"
#include "joy/joy_sdl.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/emu_link.h"


LOCAL joy_t *joy     = NULL;
LOCAL int    joy_cnt = 0;

#define DO_AC       1
#define AC_INIT_X   2
#define AC_INIT_Y   4
#define AC_INIT     (AC_INIT_X + AC_INIT_Y)

#define DIR_MAG     (32768)
#if defined(PLAT_LINUX)
# define PUSH_THRESH (128*DIR_MAG / 6)
# define RELS_THRESH (128*DIR_MAG /10)
# define AUTOCENTER  (0)
#else
# define PUSH_THRESH (128*DIR_MAG / 4)
# define RELS_THRESH (128*DIR_MAG / 6)
# define AUTOCENTER  (DO_AC + AC_INIT)
#endif

/* convert percentage to threshold and back */
#define P2T(x)  ((int)(((x)*128.*DIR_MAG + 50)/ 100.))
#define T2P(x)  ((int)(((100. * (x)) + 64.*DIR_MAG)/ (128.*DIR_MAG)))

/* convert percentage to direction magnitude and back */
#define P2M(x)  ((int)(((x)*DIR_MAG + 50.)/ 100.))
#define M2P(x)  ((int)(((100. * (x)) + 0.5*DIR_MAG)/ ((double)DIR_MAG)))

/*
SDL_NumJoysticks        Count available joysticks.
SDL_JoystickName        Get joystick name.
SDL_JoystickOpen        Opens a joystick for use.
SDL_JoystickIndex       Get the index of an SDL_Joystick.
SDL_JoystickNumAxes     Get the number of joystick axes
SDL_JoystickNumBalls    Get the number of joystick trackballs
SDL_JoystickNumHats     Get the number of joystick hats
SDL_JoystickNumButtons  Get the number of joysitck buttons
SDL_JoystickUpdate      Updates the state of all joysticks
SDL_JoystickGetAxis     Get the current state of an axis
SDL_JoystickGetHat      Get the current state of a joystick hat
SDL_JoystickGetButton   Get the current state of a given button on a given joystick
SDL_JoystickGetBall     Get relative trackball motion
SDL_JoystickClose       Closes a previously opened joystick
*/

static int dir_vect[16][2];
static const event_num_t joy_dir_map[MAX_JOY][MAX_STICKS] =
{
#define JOY_DIR_MAP_DECL(n) \
    {                                                                       \
        EVENT_JS##n##A_E, EVENT_JS##n##B_E, EVENT_JS##n##C_E,               \
        EVENT_JS##n##D_E, EVENT_JS##n##E_E, EVENT_JS##n##F_E,               \
        EVENT_JS##n##G_E, EVENT_JS##n##H_E, EVENT_JS##n##I_E,               \
        EVENT_JS##n##J_E,                                                   \
    }
    JOY_DIR_MAP_DECL(0), JOY_DIR_MAP_DECL(1), JOY_DIR_MAP_DECL(2),
    JOY_DIR_MAP_DECL(3), JOY_DIR_MAP_DECL(4), JOY_DIR_MAP_DECL(5),
    JOY_DIR_MAP_DECL(6), JOY_DIR_MAP_DECL(7), JOY_DIR_MAP_DECL(8),
    JOY_DIR_MAP_DECL(9),
};
static const event_num_t joy_btn_map[MAX_JOY] =
{
    EVENT_JS0_BTN_00, EVENT_JS1_BTN_00, EVENT_JS2_BTN_00, EVENT_JS3_BTN_00,
    EVENT_JS4_BTN_00, EVENT_JS5_BTN_00, EVENT_JS6_BTN_00, EVENT_JS7_BTN_00,
    EVENT_JS8_BTN_00, EVENT_JS9_BTN_00
};
static const event_num_t joy_hat_map[MAX_JOY] =
{
    EVENT_JS0_HAT0_E, EVENT_JS1_HAT0_E, EVENT_JS2_HAT0_E, EVENT_JS3_HAT0_E,
    EVENT_JS4_HAT0_E, EVENT_JS5_HAT0_E, EVENT_JS6_HAT0_E, EVENT_JS7_HAT0_E,
    EVENT_JS8_HAT0_E, EVENT_JS9_HAT0_E
};

LOCAL int joy_emu_link(cp1600_t *, int *, void *);

LOCAL void joy_config(int i, int j, char *cfg);
LOCAL void joy_print_config(int j, int s);
LOCAL void joy_cleanup_axes(const int js);

/* ======================================================================== */
/*  JOY_DTOR                                                                */
/* ======================================================================== */
void joy_dtor(void)
{
    int i;

    for (i = 0; i < joy_cnt; i++)
        CONDFREE(joy[i].name);

    CONDFREE(joy);
    joy_cnt = 0;
    memset(dir_vect, 0, sizeof(dir_vect));
}

/* ======================================================================== */
/*  JOY_INIT -- Enumerate available joysticks and features                  */
/* ======================================================================== */
int joy_init(int verbose, char *cfg[MAX_JOY][MAX_STICKS])
{
    double now = get_time();
    int jn;     /* Joy number */
    int sn;     /* Stick number */
    int an;     /* Axis number */

    /* -------------------------------------------------------------------- */
    /*  Initialize the direction vector table.                              */
    /* -------------------------------------------------------------------- */
    {
        int i;
        for (i = 0; i < 16; i++)
        {
            double ang = M_PI * (i / 8.);

            dir_vect[i][0] = DIR_MAG * cos(ang);
            dir_vect[i][1] = DIR_MAG *-sin(ang);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  How many do we have?                                                */
    /* -------------------------------------------------------------------- */
    joy_cnt = SDL_NumJoysticks();
    if (joy_cnt > MAX_JOY)
        joy_cnt = MAX_JOY;

    if (!joy_cnt)
        return 0;

    if (verbose)
        jzp_printf("joy:  Found %d joystick(s)\n", joy_cnt);

    if (!(joy = CALLOC(joy_t, joy_cnt)))
    {
        fprintf(stderr, "joy: out of memory\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Initialize default behavior.                                        */
    /* -------------------------------------------------------------------- */
    for (jn = 0; jn < joy_cnt; jn++)
    {
        for (sn = 0; sn < MAX_STICKS; sn++)
        {
            joy[jn].stick[sn].push_thresh = PUSH_THRESH;
            joy[jn].stick[sn].rels_thresh = RELS_THRESH;
            joy[jn].stick[sn].autocenter  = AUTOCENTER;
            joy[jn].stick[sn].dir_type    = 1;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Ok, what do they look like?                                         */
    /* -------------------------------------------------------------------- */
    for (jn = 0; jn < joy_cnt; jn++)
    {
        SDL_Joystick *sj;
            sj = SDL_JoystickOpen(jn);
        if (sj)
        {
#ifdef USE_SDL2
            joy[jn].name = strdup(SDL_JoystickName(sj));
#else
            joy[jn].name = strdup(SDL_JoystickName(jn));
#endif
            /* ------------------------------------------------------------ */
            /*  Get the info available from SDL.                            */
            /* ------------------------------------------------------------ */
            joy[jn].ptr         = (void*)sj;
            joy[jn].num_axes    = SDL_JoystickNumAxes   (sj);
            joy[jn].num_balls   = SDL_JoystickNumBalls  (sj);
            joy[jn].num_hats    = SDL_JoystickNumHats   (sj);
            joy[jn].num_buttons = SDL_JoystickNumButtons(sj);

            for (an = 0; an < MAX_AXES; an++)
            {
                struct joy_axis_t *const axis = &(joy[jn].axis[an]);

                axis->pos = SDL_JoystickGetAxis(sj, an);
                axis->ctr = axis->pos;

                if (abs(axis->pos) > 1000)
                    axis->ctr = 0; /* assume accidentally pressed */

                axis->prv = axis->ctr;
                axis->min = axis->ctr + P2M(-50.0);
                axis->max = axis->ctr + P2M(+50.0);
                axis->last = now;
                axis->stick = -1;  /* unbound, initially */
            }

            for (sn = 0; sn < MAX_STICKS; sn++)
            {
                struct joy_stick_t *const stick = &(joy[jn].stick[sn]);
                stick->x_axis = -1; /* unbound, initially */
                stick->y_axis = -1; /* unbound, initially */
            }

            if (verbose)
            {
                jzp_printf("joy:  Joystick JS%d \"%s\"\n", jn, joy[jn].name);
                jzp_printf("joy:     %d axes, %d trackball(s), "
                                "%d hat(s), %d button(s)\n",
                        joy[jn].num_axes, joy[jn].num_balls,
                        joy[jn].num_hats, joy[jn].num_buttons);
            }

            /* ------------------------------------------------------------ */
            /*  Parse configuration strings.  The following four aspects    */
            /*  can be configured in this way:                              */
            /*                                                              */
            /*   -- X/Y axis bindings                                       */
            /*   -- Autocentering on/off                                    */
            /*   -- Push/Release thresholds                                 */
            /*   -- X/Y axis range                                          */
            /* ------------------------------------------------------------ */
            for (sn = 0; sn < MAX_STICKS; sn++)
                if (cfg[jn][sn])
                    joy_config(jn, sn, cfg[jn][sn]);

            joy_cleanup_axes(jn);    /* binds any unbound axes <=> sticks */

            for (sn = 0; sn < MAX_STICKS; sn++)
                if (cfg[jn][sn])
                    joy_print_config(jn, sn);
        } else
        {
            if (verbose)
            {
                jzp_printf("joy:  Joystick JS%d \"%s\"\n", jn, joy[jn].name);
                jzp_printf("joy:     Unavailable:  Could not open.\n");
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Register our EMU_LINK API.  I'll put this on API #8.                */
    /* -------------------------------------------------------------------- */
    emu_link_register(joy_emu_link, 8, NULL);

    return 0;
}

/* ======================================================================== */
/*  JOY_CLEANUP_AXES                                                        */
/* ======================================================================== */
LOCAL void joy_cleanup_axes(const int jn)
{
    int sn, an;
    int axis_stick[MAX_AXES];

    /* -------------------------------------------------------------------- */
    /*  Assume all axes are not bound to sticks.                            */
    /* -------------------------------------------------------------------- */
    for (an = 0; an < MAX_AXES; an++)
        axis_stick[an] = -1;

    /* -------------------------------------------------------------------- */
    /*  Step through all sticks, and mark their axes as bound to them.      */
    /* -------------------------------------------------------------------- */
    for (sn = 0; sn < MAX_STICKS; sn++)
    {
        struct joy_stick_t *stick = &joy[jn].stick[sn];
        if (stick->x_axis >= 0) axis_stick[stick->x_axis] = sn;
        if (stick->y_axis >= 0 && !IS_DUMMY_AXIS(stick->y_axis))
            axis_stick[stick->y_axis] = sn;
    }

    /* -------------------------------------------------------------------- */
    /*  Fill in any unmapped Stick => Axis bindings.                        */
    /*  As long as MAX_AXES >= MAX_STICKS * 2, this always converges.       */
    /* -------------------------------------------------------------------- */
    for (sn = 0, an = 0; sn < MAX_STICKS; sn++)
    {
        struct joy_stick_t *stick = &joy[jn].stick[sn];
        if (stick->x_axis < 0)
        {
            while (an < MAX_AXES - 1 && axis_stick[an] >= 0)
                an++;
            stick->x_axis = an;
            axis_stick[an] = sn;
        }
        if (stick->y_axis < 0)
        {
            while (an < MAX_AXES - 1 && axis_stick[an] >= 0)
                an++;
            stick->y_axis = an;
            axis_stick[an] = sn;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Now map joy[jn].axis[an] back to the corresponding stick.           */
    /* -------------------------------------------------------------------- */
    for (an = 0; an < MAX_AXES; an++)
        joy[jn].axis[an].stick = axis_stick[an];
}

/* ======================================================================== */
/*  JOY_CONFIG -- Parse configuration string for a joystick.                */
/* ======================================================================== */
LOCAL void joy_config(int i, int st, char *cfg)
{
    char *s1, *s2;
    int v, j;
    int lo, hi;
    char *tmp = strdup(cfg);
    char *mem = tmp;

    /* -------------------------------------------------------------------- */
    /*  Strip off any quotes that get to us.                                */
    /* -------------------------------------------------------------------- */
    if (tmp[0] == '"' || tmp[0] == '\'')
        tmp++;

    if ((s1 = strrchr(tmp, '"' )) != NULL ||
        (s1 = strrchr(tmp, '\'')) != NULL)
        *s1 = 0;

    /* -------------------------------------------------------------------- */
    /*  Pull off comma-separated sections and parse them.                   */
    /* -------------------------------------------------------------------- */
    s1 = strtok(tmp, ",");

    while (s1)
    {
        s2 = s1;
        while (*s2 && *s2 != '=')
            s2++;

        if (*s2)
            *s2++ = '\0';

        v = atoi(s2);

        struct joy_stick_t *stick = &joy[i].stick[st];

        if      (!strcmp(s1, "ac")           ) stick->autocenter = AC_INIT;
        else if (!strcmp(s1, "noac")         ) stick->autocenter = 0;
        else if (!strcmp(s1, "push")   && *s2) stick->push_thresh=P2T(atoi(s2));
        else if (!strcmp(s1, "rels")   && *s2) stick->rels_thresh=P2T(atoi(s2));
        else if (!strcmp(s1, "button")       ) stick->dir_type = 8;
        else if (!strcmp(s1, "4diag")        ) stick->dir_type = -4;
        else if (!strcmp(s1, "4dir")         ) stick->dir_type = 4;
        else if (!strcmp(s1, "8dir")         ) stick->dir_type = 2;
        else if (!strcmp(s1, "16dir")        ) stick->dir_type = 1;
        else if ((!strcmp(s1, "xaxis" ) || !strcmp(s1, "yaxis")) && *s2)
        {
            if (v >= MAX_AXES)
            {
                fprintf(stderr,
                        "joy:  JS%d%c: axis %d out of range (max %d)\n",
                        i, 'A'+st, v, MAX_AXES);
                exit(1);
            }
            /* Go unmap any conflicts */
            for (j = 0; j < MAX_STICKS; j++)
            {
                if (joy[i].stick[j].x_axis == v) joy[i].stick[j].x_axis = -1;
                if (joy[i].stick[j].y_axis == v) joy[i].stick[j].y_axis = -1;
            }

            /* Stick => Axis map */
            if (s1[0] == 'x') stick->x_axis = v;
            else              stick->y_axis = v;

            /* Axis => Stick map */
            joy[i].axis[v].stick = st;
        }
        else if ((!strcmp(s1, "xrng") || !strcmp(s1, "yrng")) &&
                 *s2 && 2 == sscanf(s2, "%d:%d", &lo, &hi))
        {
            int ax = s1[0] == 'x' ? stick->x_axis : stick->y_axis;

            joy[i].axis[ax].min = P2M(lo);
            joy[i].axis[ax].max = P2M(hi);
        } else
        {
            fprintf(stderr, "joy:  Unknown joystick config key '%s'!\n", s1);
            exit(1);
        }
        s1 = strtok(NULL, ",");
    }

    /* -------------------------------------------------------------------- */
    /*  For "button" axes, force y_axis to DUMMY_AXIS.                      */
    /* -------------------------------------------------------------------- */
    for (j = 0; j < MAX_STICKS; j++)
    {
        if (joy[i].stick[j].dir_type == 8)
            joy[i].stick[j].y_axis = DUMMY_AXIS;
    }

    /* -------------------------------------------------------------------- */
    /*  Sanity check push/release                                           */
    /* -------------------------------------------------------------------- */
    for (j = 0; j < MAX_STICKS; j++)
    {
        if (joy[i].stick[j].push_thresh < joy[i].stick[j].rels_thresh)
        {
            joy[i].stick[j].push_thresh = joy[i].stick[j].rels_thresh;
            jzp_printf("joy:    "
                   "Warning: Push threshold below release on JS%d%c.  "
                   "Setting push = release.\n", i, 'A'+j);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Invert any axes for which inversion was requested.                  */
    /* -------------------------------------------------------------------- */
    for (j = 0; j < MAX_AXES; j++)
    {
        if (joy[i].axis[j].min > joy[i].axis[j].max)
        {
            int t;
            jzp_printf("joy:    Inverting axis %d because max < min\n",
                    j);

            t = joy[i].axis[j].min;
            joy[i].axis[j].min = joy[i].axis[j].max;
            joy[i].axis[j].max = t;
            joy[i].axis[j].inv = 1;
        }
    }

    free(mem);
}

/* ======================================================================== */
/*  JOY_GET_AXIS                                                            */
/* ======================================================================== */
LOCAL struct joy_axis_t *joy_get_axis(const int j, const int axis)
{
    if (IS_DUMMY_AXIS(axis))
        return &joy[j].axis[MAX_AXES];
    else
        return &joy[j].axis[axis];
}

/* ======================================================================== */
/*  JOY_PRINT_CONFIG                                                        */
/* ======================================================================== */
LOCAL void joy_print_config(int j, int s)
{
    /* -------------------------------------------------------------------- */
    /*  Summarize resulting configuration.                                  */
    /* -------------------------------------------------------------------- */
    struct joy_stick_t *stick  = &joy[j].stick[s];
    struct joy_axis_t  *x_axis = joy_get_axis(j, stick->x_axis);
    struct joy_axis_t  *y_axis = joy_get_axis(j, stick->y_axis);

    if (stick->dir_type != 8)
    {
        jzp_printf(
"joy:     JS%d%c: X-Axis = axis %d  Y-Axis = axis %d  Autocenter = %-3s\n"
"joy:           Push threshold = %d%%  Release threshold = %d%%\n"
"joy:           X range = [%d%%,%d%%]  Y range = [%d%%,%d%%]\n"
"joy:           Directions = %d%s\n"
"--js%d%c=\"xaxis=%d,yaxis=%d,%sac,push=%d,rels=%d,xrng=%d:%d,yrng=%d:%d,%s\"\n",
            j, 'A' + s,
            stick->x_axis, stick->y_axis,
            stick->autocenter ? "ON" : "off",
            T2P(stick->push_thresh), T2P(stick->rels_thresh),
            M2P(x_axis->min), M2P(x_axis->max),
            M2P(y_axis->min), M2P(y_axis->max),
            16 / abs(stick->dir_type), 
            stick->dir_type <  0 ? " diagonal bias" : "",
            j, 'a' + s, stick->x_axis, stick->y_axis,
            stick->autocenter ? "" : "no",
            T2P(stick->push_thresh), T2P(stick->rels_thresh),
            M2P(x_axis->min), M2P(x_axis->max),
            M2P(y_axis->min), M2P(y_axis->max),
            stick->dir_type ==-4 ? "4diag":
            stick->dir_type == 4 ? "4dir" :
            stick->dir_type == 2 ? "8dir" : "16dir");
    } else
    {
        /* "Button" mode omits yaxis info, as it is irrelevant. */
        jzp_printf(
"joy:     JS%d%c: X-Axis = axis %d  Y-Axis = None  Autocenter = %-3s\n"
"joy:           Push threshold = %d%%  Release threshold = %d%%\n"
"joy:           X range = [%d%%,%d%%]  Y range = N/A\n"
"joy:           Directions = 2\n"
"--js%d%c=\"xaxis=%d,%sac,push=%d,rels=%d,xrng=%d:%d,button\"\n",
            j, 'A' + s, stick->x_axis, stick->autocenter ? "ON" : "off",
            T2P(stick->push_thresh), T2P(stick->rels_thresh),
            M2P(x_axis->min), M2P(x_axis->max),
            j, 'a' + s, stick->x_axis,
            stick->autocenter ? "" : "no",
            T2P(stick->push_thresh), T2P(stick->rels_thresh),
            M2P(x_axis->min), M2P(x_axis->max));
    }
}

/* ======================================================================== */
/*  JOY_NORMALIZE_AXIS                                                      */
/* ======================================================================== */
LOCAL int joy_normalize_axis(struct joy_axis_t *axis)
{
    int pos, ctr, min, max;

    /* -------------------------------------------------------------------- */
    /*  Linearly interpolate the swing from ctr to edge to the range 0-128  */
    /* -------------------------------------------------------------------- */
    pos = axis->pos;
    ctr = axis->ctr;
    min = axis->min;
    max = axis->max;
    if (min == ctr) min--;
    if (max == ctr) max++;

    if (pos > ctr)  return   ((pos - ctr) * 128) / (max - ctr);
    else            return - ((ctr - pos) * 128) / (ctr - min);
}


/* ======================================================================== */
/*  JOY_DECODE_AXIS                                                         */
/* ======================================================================== */
LOCAL void joy_decode_axis
(
    const SDL_Event *const ev,
    event_updn_t    *const ev_updn,
    event_num_t     *const ev_num
)
{
    int a = ev->jaxis.axis;
    int i = ev->jaxis.which;
    int v = ev->jaxis.value;
    int u, f;
    int norm_x, norm_y;
    int j, dotp, best, best_dotp = 0;
    int ox, oy;
    struct joy_axis_t *axis, *x_axis, *y_axis;
    struct joy_stick_t *stick;
    int st;

    /* -------------------------------------------------------------------- */
    /*  Ignore axes and joysticks we can't track.                           */
    /* -------------------------------------------------------------------- */
    if (ev->jaxis.which >= joy_cnt || ev->jaxis.axis >= MAX_AXES)
    {
        ev_num[0] = EVENT_IGNORE;
#ifdef JOY_DEBUG
        printf("\rDROP axis event: %d %d      \n",
               ev->jaxis.which, ev->jaxis.axis);
        fflush(stdout);
#endif
        return;
    }

    axis = &joy[i].axis[a];

    /* -------------------------------------------------------------------- */
    /*  Update autoranging.                                                 */
    /* -------------------------------------------------------------------- */
    if (axis->inv)     v = -v;
    if (axis->min > v) axis->min = v;
    if (axis->max < v) axis->max = v;
    axis->pos = v;

    /* -------------------------------------------------------------------- */
    /*  The remaining code is specific to X/Y axis updates.                 */
    /* -------------------------------------------------------------------- */
    if (axis->stick < 0 || axis->stick >= MAX_STICKS)
    {
#ifdef JOY_DEBUG
        printf("\rDROP axis event: %d %d (not X/Y axis)      \n",
               ev->jaxis.which, ev->jaxis.axis);
        fflush(stdout);
#endif
        ev_num[0] = EVENT_IGNORE;
        return;
    }

    /* Map back to this axis' stick */
    st = axis->stick;
    stick = &joy[i].stick[axis->stick];

    /* Get both axes associated with the stick */
    x_axis = joy_get_axis(i, stick->x_axis);
    y_axis = joy_get_axis(i, stick->y_axis);

    /* -------------------------------------------------------------------- */
    /*  Update autocentering.  Do a decaying average of all samples that    */
    /*  are less than 1/8th the current release-threshold away from what    */
    /*  we consider as the current center.  An enterprising gamer might     */
    /*  fool this algorithm by rocking the joystick just off center. BFD.   */
    /*                                                                      */
    /*  The autoweighting function actually looks at the previous sample    */
    /*  and how long it was held for.  Samples that were held longer are    */
    /*  more likely to be "center."  The adaptation is simple:  Assume      */
    /*  a desired sample rate and corresponding exponent.  For samples      */
    /*  held longer, convert the held sample into multiple equal-valued     */
    /*  samples.                                                            */
    /*                                                                      */
    /*  In this case, the exponent is 1/64, and the assumed sample rate     */
    /*  is 32Hz.  (Powers of two, ya know.)  If more than 1 sec elapsed     */
    /*  we clamp at 1 sec.                                                  */
    /* -------------------------------------------------------------------- */
    f = axis == x_axis ? AC_INIT_X : AC_INIT_Y;

    /* ugh, gotta get the float out of this someday. */
    ox = x_axis->prv - x_axis->ctr;
    oy = y_axis->prv - y_axis->ctr;
    dotp = ((double)ox*128./SHRT_MAX) * ((double)ox*DIR_MAG/SHRT_MAX) +
           ((double)oy*128./SHRT_MAX) * ((double)oy*DIR_MAG/SHRT_MAX);

/*  jzp_printf("dotp=%d rt=%d\n",dotp*8, stick->rels_thresh); */

    if ((stick->autocenter & f) == f && v != 0)
    {
        /* ---------------------------------------------------------------- */
        /*  If it's a digital joystick, it'll snap to a dir.  Otherwise,    */
        /*  if it's a slow motion, use it as initial centering estimate.    */
        /* ---------------------------------------------------------------- */
        if (abs(v) * 8 < SHRT_MAX) axis->ctr = v;
        else                       axis->ctr = 0;

        stick->autocenter &= ~f;
    } else
    if ((stick->autocenter & DO_AC) && dotp*8 < stick->rels_thresh &&
        abs(v) * 8 < SHRT_MAX)
    {
        double now  = get_time();
        double then = axis->last;
        int iters;

        u = axis->prv;

        iters = 32 * (now - then) + 0.5;
        if (iters > 32) iters = 32;
        if (iters <= 0) iters = 1;

/*jzp_printf("iters=%d axis=%d then=%f now=%f\n", iters, a, then*16, now*16);*/
        while (iters-- > 0)
            axis->ctr = (axis->ctr * 63 + 31 + u) >> 6;

        axis->last = now;
    }

    axis->prv = v;

    /* -------------------------------------------------------------------- */
    /*  Ok, if this was either the X or Y axis, determine if we generate a  */
    /*  DISC event.                                                         */
    /*                                                                      */
    /*  Decoding strategy:                                                  */
    /*                                                                      */
    /*   -- Normalize the input to a +/- 128 range based on our autocenter  */
    /*      and autoranging.                                                */
    /*                                                                      */
    /*   -- Decide whether disc is up or down based on hysteresis.          */
    /*                                                                      */
    /*       -- The disc gets pressed when the joystick is more than        */
    /*          1/4 of the way to the edge from the center along its        */
    /*          closest direction line.                                     */
    /*                                                                      */
    /*       -- The disc gets released when the joystick is less than       */
    /*          1/6 along its closest direction line.                       */
    /*                                                                      */
    /*   -- If the disc is pressed, decode the direction into one of 16.    */
    /*      Rather than do trig with arctan and all that jazz, I instead    */
    /*      take the dot product of the joystick position with 16           */
    /*      different normalized direction vectors, and return the largest  */
    /*      as the best match.  Yay vector algebra.                         */
    /*                                                                      */
    /* -------------------------------------------------------------------- */

    /* -------------------------------------------------------------------- */
    /*  Normalize the X/Y.  This returns stuff in a +/- 128 range.          */
    /* -------------------------------------------------------------------- */
    norm_x = joy_normalize_axis(x_axis);
    norm_y = joy_normalize_axis(y_axis);

    /* -------------------------------------------------------------------- */
    /*  Figure out which of the 16 directions is closest to the dir we're   */
    /*  pointing.  We apply the "press" and "release" thresholds to the     */
    /*  dot product we calculate below.                                     */
    /* -------------------------------------------------------------------- */
    best = stick->disc_dir;
    for (j = stick->dir_type < 0 ? 2 : 0; j < 16; j += abs(stick->dir_type))
    {
        dotp = dir_vect[j][0] * norm_x + dir_vect[j][1] * norm_y;
        if (best_dotp < dotp)
        {
            best_dotp = dotp;
            best = j;
        }
    }

    ev_updn[0] = stick->disc_dir == -1 ? EV_UP : EV_DOWN;

    if (best_dotp < stick->rels_thresh && stick->disc_dir != -1)
    {
        ev_updn[0] = EV_UP;
        ev_num[0]  = EVENT_NUM_OFS(joy_dir_map[i][st], stick->disc_dir);
        stick->disc_dir = -1;
        return;
    }

    if (best_dotp <= stick->push_thresh && stick->disc_dir == -1)
    {
        ev_updn[0] = EV_UP;
        ev_num[0]  = EVENT_IGNORE;
        return;
    }

    if (((best_dotp > stick->push_thresh && stick->disc_dir == -1) ||
         (best_dotp > stick->rels_thresh && stick->disc_dir != best)) &&
         best != -1)
    {
        ev_updn[0] = EV_DOWN;
        ev_num[0]  = EVENT_NUM_OFS(joy_dir_map[i][st], best);
        stick->disc_dir = best;
        return;
    }

    ev_num[0] = EVENT_IGNORE;
#ifdef JOY_DEBUG
    printf("\rDROP axis event: %d %d (reached end of decode axis)    \n",
           ev->jaxis.which, ev->jaxis.axis);
    fflush(stdout);
#endif
    return;
}

/* ======================================================================== */
/*  JOY_DECODE_HAT                                                          */
/* ======================================================================== */
LOCAL void joy_decode_hat
(
    const SDL_Event *const ev,
    event_updn_t    *const ev_updn,
    event_num_t     *const ev_num
)
{
    if (ev->jhat.which >= joy_cnt || ev->jhat.hat >= MAX_HATS)
    {
        ev_num[0] = EVENT_IGNORE;
#ifdef JOY_DEBUG
        printf("\rDROP hat event: %d %d      \n",
               ev->jhat.which, ev->jhat.hat);
        fflush(stdout);
#endif
        return;
    }

    const event_num_t base = joy_hat_map[ev->jhat.which] + 8*ev->jhat.hat;
    ev_updn[0] = EV_DOWN;

    /* Offsets to the cardinal directions */
    enum { E = 0, NE, N, NW, W, SW, S, SE };

    if (ev->jhat.value != joy[ev->jhat.which].hat_dir[ev->jhat.hat])
    {
        ev_updn[1] = EV_UP;
        switch (joy[ev->jhat.which].hat_dir[ev->jhat.hat])
        {
            case SDL_HAT_RIGHT:     ev_num[1] = EVENT_NUM_OFS(base, E ); break;
            case SDL_HAT_RIGHTUP:   ev_num[1] = EVENT_NUM_OFS(base, NE); break;
            case SDL_HAT_UP:        ev_num[1] = EVENT_NUM_OFS(base, N ); break;
            case SDL_HAT_LEFTUP:    ev_num[1] = EVENT_NUM_OFS(base, NW); break;
            case SDL_HAT_LEFT:      ev_num[1] = EVENT_NUM_OFS(base, W ); break;
            case SDL_HAT_LEFTDOWN:  ev_num[1] = EVENT_NUM_OFS(base, SW); break;
            case SDL_HAT_DOWN:      ev_num[1] = EVENT_NUM_OFS(base, S ); break;
            case SDL_HAT_RIGHTDOWN: ev_num[1] = EVENT_NUM_OFS(base, SE); break;
            case SDL_HAT_CENTERED:  ev_num[1] = EVENT_IGNORE; break;
            default: 
               ev_num[1] = EVENT_IGNORE;
               jzp_printf("Warning: Unknown hat input %d\n", ev->jhat.value);
        }
    }

    switch (ev->jhat.value)
    {
        case SDL_HAT_RIGHT:     ev_num[0] = EVENT_NUM_OFS(base, E ); break;
        case SDL_HAT_RIGHTUP:   ev_num[0] = EVENT_NUM_OFS(base, NE); break;
        case SDL_HAT_UP:        ev_num[0] = EVENT_NUM_OFS(base, N ); break;
        case SDL_HAT_LEFTUP:    ev_num[0] = EVENT_NUM_OFS(base, NW); break;
        case SDL_HAT_LEFT:      ev_num[0] = EVENT_NUM_OFS(base, W ); break;
        case SDL_HAT_LEFTDOWN:  ev_num[0] = EVENT_NUM_OFS(base, SW); break;
        case SDL_HAT_DOWN:      ev_num[0] = EVENT_NUM_OFS(base, S ); break;
        case SDL_HAT_RIGHTDOWN: ev_num[0] = EVENT_NUM_OFS(base, SE); break;
        case SDL_HAT_CENTERED:  ev_num[0] = EVENT_IGNORE; break;
        default: 
            ev_num[0] = EVENT_IGNORE;
            jzp_printf("Warning: Unknown hat input %d\n", ev->jhat.value);
    }

    joy[ev->jhat.which].hat_dir[ev->jhat.hat] = ev->jhat.value;

    return;
}

/* ======================================================================== */
/*  JOY_DECODE_BUTTON                                                       */
/* ======================================================================== */
LOCAL void joy_decode_button(const SDL_Event *const ev,
                             event_num_t *const ev_num)
{
    if (ev->jbutton.which >= joy_cnt || ev->jbutton.button > 31)
    {
#ifdef JOY_DEBUG
        printf("\rDROP button event: %d %d      \n",
               ev->jbutton.which, ev->jbutton.button);
        fflush(stdout);
#endif
        *ev_num = EVENT_IGNORE;
        return;
    }

    *ev_num = EVENT_NUM_OFS(joy_btn_map[ev->jbutton.which], ev->jbutton.button);
    return;
}



/* ======================================================================== */
/*  JOY_DECODE_EVENT -- Pull apart an SDL_EVENT and turn it into our        */
/*                      internal event numbers.                             */
/* ======================================================================== */
bool joy_decode_event
(
    const SDL_Event *const ev,
    event_updn_t    *const ev_updn,
    event_num_t     *const ev_num
)
{
    bool may_combo = false;
    SDL_JoystickID *ptr = (SDL_JoystickID *)&ev->jaxis.which;
    if (*ptr >= joy_cnt) {
        *ptr = *ptr % joy_cnt;
    }

    switch (ev->type)
    {
        case SDL_JOYAXISMOTION:
        {
            joy_decode_axis(ev, ev_updn, ev_num);
            break;
        }
        case SDL_JOYHATMOTION:
        {
            joy_decode_hat(ev, ev_updn, ev_num);
            break;
        }

        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
            may_combo = true;
            *ev_updn = ev->type == SDL_JOYBUTTONDOWN ? EV_DOWN : EV_UP;
            joy_decode_button(ev, ev_num);
            break;
        }

        case SDL_JOYBALLMOTION:
        {
            /* ignored */
#ifdef JOY_DEBUG
            printf("\rDROP ball event: %d %d [%4d,%4d]     \n",
                   ev->jball.which, ev->jball.ball,
                   ev->jball.xrel, ev->jball.yrel);
            fflush(stdout);
#endif

            *ev_num = EVENT_IGNORE;
            break;
        }

        default: *ev_num = EVENT_IGNORE; break;
    }

    if (ev_num[0] == EVENT_IGNORE || ev_num[1] != EVENT_IGNORE)
        may_combo = false;

    return may_combo;
}

/* ======================================================================== */
/*  JOY_EMU_LINK -- Allow programs to get analog joystick info.             */
/* ======================================================================== */
LOCAL int joy_emu_link(cp1600_t *cpu, int *fail, void *opaque)
{
    int js, st;
    int api;
    struct joy_t *joyp;
    struct joy_stick_t *stick;
    struct joy_axis_t *x_axis, *y_axis;

    UNUSED(opaque);

    /* -------------------------------------------------------------------- */
    /*  Sub-APIs we export:                                                 */
    /*                                                                      */
    /*    R2:  Sub-API number (table below)                                 */
    /*    R3:  LSB is Joy #, MSB is Stick #                                 */
    /*                                                                      */
    /*  00: Number of joysticks.  Result in R0.  Ignores R3.                */
    /*  01: Get geometry: Returns # of axes, balls, hats, buttons in R0..R3 */
    /*  02: Get X/Y raw pos:  Returns 16-bit X/Y pos in R1, R2.             */
    /*  03: Get X/Y raw min:  Returns 16-bit X/Y min in R1, R2.             */
    /*  04: Get X/Y raw max:  Returns 16-bit X/Y max in R1, R2.             */
    /*  05: Get X/Y raw ctr:  Returns 16-bit X/Y max in R1, R2.             */
    /*  06: Get X/Y cooked:   Norm'd 8-bit X/Y in R1, R2. Disc Dir in R0.   */
    /*  07: Get buttons.  Returns 32-bit bitmap in R1, R2.                  */
    /*  08: Get hats.  Returns hats 0..3 in 4 x 4-bit fields in R0.         */
    /* -------------------------------------------------------------------- */
    if (cpu->r[2] == 0x00)
    {
        *fail = 0;
        return joy_cnt;
    }

    js = cpu->r[3] & 0xFF;
    st = (cpu->r[3] >> 8) & 0xFF;
    api = cpu->r[2];

    if (api > 0x08 || js >= joy_cnt || st >= MAX_STICKS)
    {
        *fail = 1;
        return 0xFFFF;
    }

    joyp = joy + js;
    stick = &joyp->stick[st];
    x_axis = joy_get_axis(js, stick->x_axis);
    y_axis = joy_get_axis(js, stick->y_axis);

    switch (api)
    {
        case 0x01:
        {
            *fail = 0;
            cpu->r[1] = joyp->num_balls;
            cpu->r[2] = joyp->num_hats;
            cpu->r[3] = joyp->num_buttons;
            return      joyp->num_axes;
        }

        case 0x02:
        {
            *fail = 0;
            cpu->r[1] = x_axis->pos;
            cpu->r[2] = y_axis->pos;
            return 0;
        }

        case 0x03:
        {
            *fail = 0;
            cpu->r[1] = x_axis->min;
            cpu->r[2] = y_axis->min;
            return 0;
        }

        case 0x04:
        {
            *fail = 0;
            cpu->r[1] = x_axis->max;
            cpu->r[2] = y_axis->max;
            return 0;
        }

        case 0x05:
        {
            *fail = 0;
            cpu->r[1] = x_axis->ctr;
            cpu->r[2] = y_axis->ctr;
            return 0;
        }

        case 0x06:
        {
            *fail = 0;
            cpu->r[1] = joy_normalize_axis(x_axis);
            cpu->r[2] = joy_normalize_axis(y_axis);
            return stick->disc_dir;
        }

        case 0x07:
        {
            uint32_t buttons = 0;
            int i;

            *fail = 0;
            for (i = 0; i < joyp->num_buttons; i++)
                buttons |= (SDL_JoystickGetButton(
                                (SDL_Joystick *)joyp->ptr,i) != 0) << i;

            cpu->r[1] =  buttons        & 0xFFFF;
            cpu->r[2] = (buttons >> 16) & 0xFFFF;

            return 0;
        }

        case 0x08:
        {
            uint32_t hats = 0;
            int i;

            *fail = 0;
            for (i = 0; i < joyp->num_hats; i++)
            {
                int p;

                switch (joyp->hat_dir[i])
                {
                    case SDL_HAT_RIGHT:         p = 0;  break; /* E    */
                    case SDL_HAT_RIGHTUP:       p = 1;  break; /* NE   */
                    case SDL_HAT_UP:            p = 2;  break; /* N    */
                    case SDL_HAT_LEFTUP:        p = 3;  break; /* NW   */
                    case SDL_HAT_LEFT:          p = 4;  break; /* W    */
                    case SDL_HAT_LEFTDOWN:      p = 5;  break; /* SW   */
                    case SDL_HAT_DOWN:          p = 6;  break; /* S    */
                    case SDL_HAT_RIGHTDOWN:     p = 7;  break; /* SE   */
                    case SDL_HAT_CENTERED:      p = 15; break; /* center */
                    default:                    p = 15; break;
                }
                hats |= p << (4 * i);
            }

            return hats & 0xFFFF;
        }

        default:
            break;
    }

    *fail = 1;
    return 0xFFFF;
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
/*                 Copyright (c) 2005-2020, Joseph Zbiciak                  */
/* ======================================================================== */
