/*
 * ============================================================================
 *  The STIC and System RAM control access to the GRAM, GROM and STIC
 *  registers.  The STIC also occasionally stalls the CPU with BUSRQ
 *  so it can fetch display cards from System RAM.  The STIC also
 *  interrupts the CPU every time it reaches the bottom of the frame.
 *
 *  The following parameters control the STIC timing for interrupts and
 *  bus requests.
 *
 *    FRAMCLKS          Cycles between interrupts.
 *
 *    INTRQ_HOLD        How long STIC asserts INTRQ for.  If the CPU
 *                      masks interrupts too long, it'll drop an interrupt.
 *
 *    BUSRQ_HOLD_*      How long the STIC holds BUSRQ for.  FIRST and NORMAL
 *                      refer to the first BUSRQ of the frame and the BUSRQs
 *                      in the body of the frame.  EXTRA refers to the 14th
 *                      BUSRQ that occurs when vertical delay == 0.
 *
 *    MUST_BUSAK_TIME   The cycle by which the CPU must respond to a BUSRQ.
 *                      If the CPU doesn't respond in time, the System RAM
 *                      won't advance the display FIFO and a display glitch
 *                      results.
 *
 *
 *  When the display's active, the STIC has three main phases:
 *
 *   -- VBlank 1, where CPU can access both GRAM/GROM and STIC registers
 *   -- VBlank 2, where CPU can access only GRAM/GROM
 *   -- Active display, where the CPU can access neither GRAM/GROM nor
 *      STIC registers.  The STIC periodically stalls the CPU for
 *      BACKTAB fetches from System RAM.
 *
 *  The following paramters define these periods:
 *
 *    STIC_ACCESSIBLE   Time window when STIC registers are accessible
 *    GMEM_ACCESSIBLE   Time window when GRAM/GROM are accessible
 *
 *
 *  And finally, the first frame after reset has different timing than the
 *  rest.  This actually affects a couple games with strange race conditions.
 *  INITIAL_OFFSET says how many cycles to start into the first frame.  This
 *  mainly controls where the first interrupt happens, since it's unlikely
 *  any program will get in there fast enough to enable display, unless you
 *  use a custom EXEC with "MVO R0, $20" in the first couple instructions.
 *
 * ============================================================================
 */
#ifndef STIC_TIMINGS_H_
#define STIC_TIMINGS_H_ 1

#define NTSC_FRAMCLKS           (14934)
/*#define NTSC_INTRQ_HOLD         (2920)*/
#define NTSC_INTRQ_HOLD         (2907)  /* from Kevin Horton */

#define NTSC_BUSRQ_HOLD_FIRST   (57)    /* First BUSRQ is really short?     */
#define NTSC_BUSRQ_HOLD_NORMAL  (110)   /* Typical BUSRQs during display    */
#define NTSC_BUSRQ_HOLD_EXTRA   (44)    /* 14th BUSRQ is half-length        */
#define NTSC_MUST_BUSAK_TIME    (68)    /* CPU must ACK a BUSRQ by here     */
#define NTSC_BUSRQ_MARGIN       (STIC_BUSRQ_HOLD_NORMAL - STIC_MUST_BUSAK_TIME)

#define NTSC_STIC_ACCESSIBLE    (2900)  /* STIC time window during vblank   */
#define NTSC_GMEM_ACCESSIBLE    (3796)  /* GRAM/GROM time window during vbl */

#define NTSC_INITIAL_OFFSET     (2782)  /* Starting point of first frame.   */

#define NTSC_SCANLINE           (57)

/* a stab at PAL timings */
#define PAL_FRAMCLKS            (19968)

#define PAL_INTRQ_HOLD          (6400)  /* Guess 100 scanlines              */
#define PAL_BUSRQ_HOLD_FIRST    (64)    /* First BUSRQ is really short?     */
#define PAL_BUSRQ_HOLD_NORMAL   (118)   /* Typical BUSRQs during display    */
#define PAL_BUSRQ_HOLD_EXTRA    (60)    /* 14th BUSRQ is half-length        */
#define PAL_MUST_BUSAK_TIME     (82)    /* CPU must ACK a BUSRQ by here     */
#define PAL_BUSRQ_MARGIN        (STIC_BUSRQ_HOLD_NORMAL - STIC_MUST_BUSAK_TIME)

#define PAL_STIC_ACCESSIBLE    (PAL_INTRQ_HOLD - 7)  /* A guess */
#define PAL_GMEM_ACCESSIBLE    (7456)   /* Guess 116.5 scanlines */

#define PAL_INITIAL_OFFSET     (PAL_INTRQ_HOLD - 75) /* total guess */

#define PAL_SCANLINE           (64)


#define STIC_FRAMCLKS           (stic->pal ? PAL_FRAMCLKS          : NTSC_FRAMCLKS         )
#define STIC_INTRQ_HOLD         (stic->pal ? PAL_INTRQ_HOLD        : NTSC_INTRQ_HOLD       )
#define STIC_BUSRQ_HOLD_FIRST   (stic->pal ? PAL_BUSRQ_HOLD_FIRST  : NTSC_BUSRQ_HOLD_FIRST )
#define STIC_BUSRQ_HOLD_NORMAL  (stic->pal ? PAL_BUSRQ_HOLD_NORMAL : NTSC_BUSRQ_HOLD_NORMAL)
#define STIC_BUSRQ_HOLD_EXTRA   (stic->pal ? PAL_BUSRQ_HOLD_EXTRA  : NTSC_BUSRQ_HOLD_EXTRA )
#define STIC_MUST_BUSAK_TIME    (stic->pal ? PAL_MUST_BUSAK_TIME   : NTSC_MUST_BUSAK_TIME  )
#define STIC_BUSRQ_MARGIN       (stic->pal ? PAL_BUSRQ_MARGIN      : NTSC_BUSRQ_MARGIN     )

#define STIC_STIC_ACCESSIBLE    (stic->pal ? PAL_STIC_ACCESSIBLE   : NTSC_STIC_ACCESSIBLE  )
#define STIC_GMEM_ACCESSIBLE    (stic->pal ? PAL_GMEM_ACCESSIBLE   : NTSC_GMEM_ACCESSIBLE  )

#define STIC_INITIAL_OFFSET     (stic->pal ? PAL_INITIAL_OFFSET    : NTSC_INITIAL_OFFSET   )
#define STIC_SCANLINE           (stic->pal ? PAL_SCANLINE          : NTSC_SCANLINE         )

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
/*                 Copyright (c) 1998-2011, Joseph Zbiciak                  */
/* ======================================================================== */
