/*
 * ============================================================================
 *  Title:    Generic Game Loading Logic
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This code attempts to detect the type of the program file being loaded,
 *  returning the binary image of the file and a type indicator if the
 *  detection is successful.
 *
 *  It is also responsible for searching the game search path and for trying
 *  different extensions (.ROM, .BIN, .INT) on the provided filename.
 *  In a sense, this is similar to what DOS's COMMAND.COM does for a command
 *  name, searching for "FOO.COM", "FOO.EXE" and "FOO.BAT" along all the
 *  directories in the search path in response to the command "FOO."
 * ============================================================================
 *
 * ============================================================================
 */


#ifndef LOADGAME_H_
#define LOADGAME_H_

#define GAMETYPE_ROM (0)
#define GAMETYPE_BIN (1)
#define GAMETYPE_BAD (-1)

/* ======================================================================== */
/*  READ_GAME                                                               */
/* ======================================================================== */
uint8_t *read_game
(
    const char *name,
    int        *type,
    char      **bin_name,
    char      **cfg_name
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
/*                 Copyright (c) 1998-2004, Joseph Zbiciak                  */
/* ======================================================================== */
