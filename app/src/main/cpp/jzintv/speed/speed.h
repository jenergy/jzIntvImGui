/*
 * ============================================================================
 *  Title:
 *  Author:   J. Zbiciak
 * ============================================================================
 *
 * ============================================================================
 *
 * ============================================================================
 */

#ifndef SPEED_H_
#define SPEED_H_

typedef struct speed_t
{
    periph_t        periph;
    double          last_time;
    double          threshold;
    double          target_rate;
    uint64_t        tick;
    uint32_t        warmup;
    uint8_t         busywaits_ok;
    uint8_t         pal;
    gfx_t          *gfx;
    stic_t         *stic;
} speed_t;

/*
 * ============================================================================
 *  SPEED_TK         -- Main throttling agent.
 *  SPEED_INIT       -- Initializes a speed-control object.
 *  SPEED_RESYNC     -- Slips time to resync speed-control
 * ============================================================================
 */
uint32_t speed_tk    (periph_t *p, uint32_t len);
int      speed_init  (speed_t *speed, gfx_t *gfx, stic_t *stic,
                      int busywaits, double target, int pal_mode);
void     speed_resync(speed_t *speed);

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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */


