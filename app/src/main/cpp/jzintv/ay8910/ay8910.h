/*
 * ============================================================================
 *  Title:    AY-8910 family Programmable Sound Generator
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements the AY-8910 sound chip.
 * ============================================================================
 *
 * ============================================================================
 */

#ifndef AY8910_H_
#define AY8910_H_

/*
 * ============================================================================
 *  AY8910_T         -- PSG structure
 * ============================================================================
 */

#define AY8910_STCK     (4)        /* Samples per tick.                    */

struct ay8910_t
{
    periph_t    periph;             /* Yup, it's a peripheral.  Go figure.  */
    uint16_t    reg[14];            /* The AY-891x's 14 internal registers. */
    int         max[5];             /* Up-count cutoff: A, B, C, N, E.      */
    int         cnt[6];             /* Counter registers.  These behave as  */
                                    /*  upcounters that compare against     */
                                    /*  cnt_max[], but really they're down  */
                                    /*  counters.                           */

    snd_buf_t   snd_buf;            /* Sound circular buffer.               */

    int16_t    *cur_buf;            /* Current sound buffer.                */
    int         cur_len;            /* Fullness of current sound buffer.    */

    uint32_t    noise_rng;          /* Random-number generator for noise.   */

    int         env_cont;           /* Envelope CONTINUE flag               */
    int         env_atak;           /* Envelope ATTACK flag                 */
    int         env_altr;           /* Envelope ALTERNATE flag              */
    int         env_hold;           /* Envelope HOLD flag                   */
    int         env_vol;            /* Last envelope volume.                */
    int         env_samp;           /* Envelope sample count remainder.     */
    int         demo_env_hit;       /* Flag for demo rec:  Hit env reg.     */

    int         chan[3];            /* Last channel state (on/off)          */

    int        *window;             /* Sliding Window.                      */
    int         wind_sum;           /* Window sum.                          */
    int         wind_ptr;           /* Window pointer.                      */
    int         rate, wind;         /* Sample rate, Window size.            */
    int         sys_clock;          /* System clock rate                    */
    double      time_scale;
    double      scale_frc;

    /* Dynamic Digital Analyzer approach for matching sample rates.         */
    int         sample_frc;         /* Fractional error term.               */

    uint64_t    sound_current;      /* Sound is calc'd up until this time.  */
    uint64_t    unaccounted;
    uint64_t    accutick;           /* min time when simulating on write    */
    uint64_t   *cpu_time;           /* don't get ahead of CPU (HACK!)       */

    char       *trace_filename;     /* Register trace file name             */
    FILE       *trace;              /* Register trace file pointer          */
};


#ifndef AY8910_T_
#define AY8910_T_ 1
typedef struct ay8910_t ay8910_t;
#endif


/*
 * ============================================================================
 *  AY8910_READ      -- Read from device.
 * ============================================================================
 */
uint32_t ay8910_read
(
    periph_t        *bus,       /*  Peripheral bus being read.          */
    periph_t        *req,       /*  Peripheral requesting read.         */
    uint32_t        addr,       /*  Address being read.                 */
    uint32_t        data        /*  Current state of data being read.   */
);

/*
 * ============================================================================
 *  AY8910_WRITE     -- Write to device.
 * ============================================================================
 */
void ay8910_write
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting write.        */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
);

/*
 * ============================================================================
 *  AY8910_TICK      -- Tick the device.
 * ============================================================================
 */
uint32_t ay8910_tick
(
    periph_t        *bus,       /*  Peripheral bus being ticked.        */
    uint32_t        len
);


/*
 * ============================================================================
 *  AY8910_INIT          -- Makes a new PSG.
 * ============================================================================
 */

int ay8910_init
(
    ay8910_t       *ay8910,     /*  Structure to initialize.        */
    uint32_t        addr,       /*  Base address of ay8910.         */
    snd_t          *snd,        /*  Sound device to register w/.    */
    int             rate,       /*  Sampling rate.                  */
    int             wind,       /*  Averaging window.               */
    int             accutick,   /*  Averaging window.               */
    double          time_scale, /*  for --macho                     */
    int             pal_mode,   /*  0 == NTSC, 1 == PAL             */
    uint64_t       *cpu_time    /*  HACK: don't get ahead of CPU    */
);


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

