/*
 * ============================================================================
 *  Title:    Speed control
 *  Author:   J. Zbiciak
 * ============================================================================
 *  SPEED_T      -- Speed-control peripheral
 *  SPEED_TK     -- Main throttling agent.
 *  SPEED_SET    -- Set desired speed as a fraction out of 256.
 *  SPEED_GET    -- Query current average running speed, tick rate, etc.
 * ============================================================================
 *  Attempts to control speed of emulation.  Invoked as a peripheral.
 *  This code will tend to be very platform dependent, I'm guessing.
 *  The initial implementation will be a busy-loop that calls gettimeofday
 *  repeatedly
 * ============================================================================
 */



#include "config.h"
#include "periph/periph.h"
#include "gfx/gfx.h"
#include "demo/demo.h"
#include "stic/stic.h"
#include "speed.h"

#define LATE_TOLERANCE     (256   * 1.e-6)
#define MICROSEC_PER_FRAME (speed->pal ? (19968. * 1.e-6) : (16688. * 1.e-6))
#define DELAY_THRESH       (1000  * 1.e-6)
#define DELAY_THRESH2      (1000  * 1.e-6)
#define MIN_THRESH         (0)

/*
 * ============================================================================
 *  SPEED_TK         -- Main throttling agent.
 * ============================================================================
 */
uint32_t speed_tk(periph_t *p, uint32_t len)
{
    speed_t *const speed = PERIPH_AS(speed_t, p);
    double   sec, elapsed;
    double   now, then;


    /* -------------------------------------------------------------------- */
    /*  The value of 'len' says how long we've gone w/out a tick.  We       */
    /*  compare this against the amount of real time that's elapsed, and    */
    /*  act accordingly:                                                    */
    /*                                                                      */
    /*   -- If it appears "no" real time has passed, then we're being       */
    /*      called too often, so adjust our min tick rate outwards.         */
    /*      We don't let this get larger than max_tick, though, which is    */
    /*      set to correspond to 60Hz.                                      */
    /*                                                                      */
    /*   -- If too little real time has passed, sit in a busy loop and      */
    /*      for real time to catch up to simulation time.                   */
    /*                                                                      */
    /*   -- If too much time has passed, do nothing.  (We could adjust our  */
    /*      frame rate, audio quality, etc.)                                */
    /* -------------------------------------------------------------------- */

    now       = get_time() * speed->target_rate;
    then      = speed->last_time;    /*  When will THEN be NOW???  SOON!!!  */
    speed->last_time = now;
    sec       = speed->pal ? ((double)len / 1000000.)
                           : ((double)len * (4.0 / 3579545.0));
    elapsed   = now - then;

    if (speed->threshold < MIN_THRESH)
        speed->threshold = MIN_THRESH;

    /* -------------------------------------------------------------------- */
    /*  If more than 0.25 second has elapsed, assume that we've gotten      */
    /*  majorly pre-empted by the OS and just slip time so that we resync.  */
    /* -------------------------------------------------------------------- */
    if (now - then > 0.25 * speed->target_rate)
    {
        speed_resync(speed);
        return len;
    }

    /* -------------------------------------------------------------------- */
    /*  If we're warming up, then just record the time and move on.         */
    /* -------------------------------------------------------------------- */
    if (speed->warmup > 0)
    {
        speed->warmup--;
        return len;
    }

    /* -------------------------------------------------------------------- */
    /*  No time has passed.  Adjust and move on.                            */
    /* -------------------------------------------------------------------- */
    if (elapsed < 2e-6)
        elapsed = 2e-6;

    /* -------------------------------------------------------------------- */
    /*  If more real time has elapsed than simulation time, then just       */
    /*  return the amount of simulation time we were asked to burn.  Since  */
    /*  we get called many times a frame, but loading is 'lumpy', this is   */
    /*  OK to do.                                                           */
    /* -------------------------------------------------------------------- */
    if (elapsed >= sec)
    {
        double behind = (elapsed - sec);

/*jzp_printf("len = %-5d threshold = %-8.1f  behind = %-8.1f \n", len, speed->threshold*1e6, behind*1e6);jzp_flush();*/

        if (behind > MICROSEC_PER_FRAME + LATE_TOLERANCE)
        {
            speed->gfx->drop_frame  += behind / MICROSEC_PER_FRAME;
            speed->stic->drop_frame += behind / MICROSEC_PER_FRAME;
        }

        speed->threshold = speed->threshold + behind / 16.0;
    }
    /* -------------------------------------------------------------------- */
    /*  If more simulation time than real time has elapsed, burn off some   */
    /*  time.  We want to leave a small bit of unburnt time due to the      */
    /*  lumpyness of the rest of the emulation loading.                     */
    /* -------------------------------------------------------------------- */
    else if (elapsed < sec)
    {
        double ahead = sec - elapsed;
/*jzp_printf("len = %-5d threshold = %-8.1f   ahead = %-8.1f \n", len, speed->threshold*1e6, ahead*1e6);jzp_flush();*/

        speed->threshold = (0.875*speed->threshold) + (MIN_THRESH / 8.0);

        /* ---------------------------------------------------------------- */
        /*  If, somehow, we're ticked for more than a frame, ack only 1     */
        /*  frame.  Generally this shouldn't be much of an issue.           */
        /* ---------------------------------------------------------------- */
        if (sec - elapsed > MICROSEC_PER_FRAME)
        {
            sec = elapsed + MICROSEC_PER_FRAME;
            ahead = MICROSEC_PER_FRAME;
        }

        /* ---------------------------------------------------------------- */
        /*  Burn up to 'speed->threshold' microseconds.                     */
        /* ---------------------------------------------------------------- */
        if (ahead > speed->threshold)
        {
            double burn = (ahead - speed->threshold) / speed->target_rate;
            double until =   sec - speed->threshold;
/*jzp_printf("ahead=%8.6f burn=%8.6f DELAY_THRESH=%8.6f\n", ahead, burn, DELAY_THRESH);*/
            if (ahead > (speed->busywaits_ok ? DELAY_THRESH : DELAY_THRESH2))
                plat_delay(floor(burn * 1e3));
            else if (speed->busywaits_ok)
            {
                do
                {
                    elapsed = get_time() * speed->target_rate - then;
                } while (elapsed < until);
            }
        }
    }

    now              = get_time() * speed->target_rate;
    elapsed          = now - then;
    speed->last_time = now;
    len              = floor(elapsed * (speed->pal ? 1000000.0 : 894886.25)
                                                                        + 0.5);

    return len;
}

/*
 * ============================================================================
 *  SPEED_RESYNC     -- Resynchronizes speed-control, slipping time.
 * ============================================================================
 */
void speed_resync(speed_t *speed)
{
    speed->warmup    = 10;
    speed->last_time = get_time() * speed->target_rate;
}

/*
 * ============================================================================
 *  SPEED_INIT       -- Initializes a speed-control object.
 * ============================================================================
 */
int speed_init(speed_t *speed, gfx_t *gfx, stic_t *stic,
               int busywaits, double target, int pal_mode)
{
    /* -------------------------------------------------------------------- */
    /*  Set up tick values.  Who knows if these work?                       */
    /* -------------------------------------------------------------------- */
    if (target < 0.01)
        target = 0.01;

    speed->target_rate      = target;
    speed->last_time        = get_time() * speed->target_rate;
    speed->pal              = pal_mode;

    /* -------------------------------------------------------------------- */
    /*  Set up the speed_t structure.                                       */
    /* -------------------------------------------------------------------- */
    speed->periph.read      = NULL;
    speed->periph.write     = NULL;
    speed->periph.peek      = NULL;
    speed->periph.poke      = NULL;
    speed->periph.tick      = speed_tk;
    speed->periph.min_tick  = pal_mode ? 19968 : 14934;
    speed->periph.max_tick  = pal_mode ? 19968 : 14934;
    speed->gfx              = gfx;
    speed->stic             = stic;
    speed->threshold        = 0;
    speed->warmup           = 10;
    speed->busywaits_ok     = busywaits;

    return 0;
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
/*                 Copyright (c) 1998-2016, Joseph Zbiciak                  */
/* ======================================================================== */

