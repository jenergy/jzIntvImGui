/* Cheat commands! */
#ifndef CHEAT_H_
#define CHEAT_H_

#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"

#define NUM_CHEATS (8)

typedef struct cheat_cmd {
    int         cmd;        /* Action to perform.   */
    uint32_t    arg[2];     /* Arguments.           */
} cheat_cmd_t;

typedef struct cheat {
    periph_t    periph;
    int         initialized;
    cheat_cmd_t *cheat[NUM_CHEATS];
    uint32_t    request;
    cp1600_t    *cpu;
} cheat_t;

/* ======================================================================== */
/*  CHEAT_ADD    -- Adds a cheat to cheat_t.                                */
/* ======================================================================== */
int cheat_add(cheat_t *const RESTRICT cheat, const char *const s);

/* ======================================================================== */
/*  CHEAT_COUNT  -- Returns number of active cheats.                        */
/* ======================================================================== */
int cheat_count(const cheat_t *const cheat);

/* ======================================================================== */
/*  CHEAT_INIT   -- Initializes the cheat peripheral if it isn't already.   */
/* ======================================================================== */
int cheat_init(cheat_t *const cheat, cp1600_t *const cpu);

#endif /* CHEAT_H_ */

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
/*         Copyright (c) 2019-+Inf, Joseph Zbiciak, Patrick Nadeau          */
/* ======================================================================== */
