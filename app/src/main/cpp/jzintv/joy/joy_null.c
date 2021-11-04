/*
 * ============================================================================
 *  Title:    Null Joystick Support
 *  Author:   J. Zbiciak
 * ============================================================================
 */

#include "config.h"
#include "joy/joy.h"

/* ======================================================================== */
/*  JOY_DTOR                                                                */
/* ======================================================================== */
void joy_dtor(void)
{
}

/* ======================================================================== */
/*  JOY_INIT -- Enumerate available joysticks and features                  */
/* ======================================================================== */
int joy_init(int verbose, char *cfg[MAX_JOY][MAX_STICKS])
{
    UNUSED(verbose);
    UNUSED(cfg);
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
/*                 Copyright (c) 2005-2020, Joseph Zbiciak                  */
/* ======================================================================== */
