/*
 * ============================================================================
 *  Title:    Platform-specific Initialization Functions
 *  Author:   J. Zbiciak
 * ============================================================================
 *  All platform-specific initialization should be handled in plat_init().
 *  The default supported platform is "SDL".
 * ============================================================================
 *  PLAT_INIT -- Platform-specific initialization. Returns non-zero on fail.
 * ============================================================================
 */
#ifndef PLAT_H_
#define PLAT_H_

int plat_init(void);

bool plat_is_batch_mode(void);    /* Returns true if running in batch mode. */

#endif /*PLAT_H*/
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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
