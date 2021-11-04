/*
 * ============================================================================
 *  Title:    Demo Recorder
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements a "demo recorder", which records updates to GRAM,
 *  BACKTAB and the PSG to a file.  Currently it does not record Intellivoice.
 * ============================================================================
 */

#ifndef DEMO_H_
#define DEMO_H_ 1

#ifndef STIC_T_
#define STIC_T_ 1
typedef struct stic_t stic_t;
#endif

#ifndef AY8910_T_
#define AY8910_T_ 1
typedef struct ay8910_t ay8910_t;
#endif

typedef struct demo_t
{
    FILE       *f;

    uint16_t    btab[240];
    uint8_t     gram[512];
    uint16_t    stic[32];

    ay8910_t   *psg0, *psg1;
    uint16_t    psg0_reg[16];
    uint16_t    psg1_reg[16];
} demo_t;

/* ======================================================================== */
/*  DEMO_TICK    -- Called from STIC_TICK at the start of VBlank.           */
/* ======================================================================== */
void demo_tick
(
    demo_t        *demo,
    struct stic_t *stic
);

/* ======================================================================== */
/*  DEMO_INIT    -- Initialize the demo recorder.                           */
/* ======================================================================== */
int demo_init
(
    demo_t      *demo,
    char        *demo_file,
    ay8910_t    *psg0,
    ay8910_t    *psg1
);

void demo_dtor(demo_t *demo);
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
/*                 Copyright (c) 2005-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
