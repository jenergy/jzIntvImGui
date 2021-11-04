/* ======================================================================== */
/*  INTELLICART ROM File I/O wrapper                    J. Zbiciak, 2003    */
/*                                                                          */
/*  This is a pretty simple "readfile"/"writefile" wrapper that handles     */
/*  reading and writing .ROM and .BIN+CFG files.                            */
/*                                                                          */
/*  The readfile wrapper implements the following rules:                    */
/*                                                                          */
/*   -- If the extension '.rom' is given, try to read the file as a .ROM    */
/*      format file.                                                        */
/*                                                                          */
/*   -- If the extension '.bin' is given, try to read the file as a .BIN    */
/*      + .CFG file pair.                                                   */
/*                                                                          */
/*   -- If neither of the above succeeds, try appending '.rom' and read     */
/*      the file as a .ROM file.                                            */
/*                                                                          */
/*   -- If none of those succeed, try appending '.bin' and read the file    */
/*      as a .BIN+CFG file.                                                 */
/*                                                                          */
/*  The writefile wrapper implements the following rules:                   */
/*                                                                          */
/*   -- If the extension '.rom' is given, write a .ROM file.                */
/*   -- If the extension '.bin' is given, write a .BIN and .CFG file.       */
/*   -- If no extension is given, write all three variants.                 */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*                 Copyright (c) 2003-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */

/* ======================================================================== */
/*  ICART_WRITEFILE -- Write combination of BIN+CFG or ROM from an icart.   */
/* ======================================================================== */
void icart_writefile(const char *fname, icartrom_t *icart, ictype_t type);

/* ======================================================================== */
/*  ICART_READFILE -- Make a best effort, trying to read a ROM/BIN+CFG.     */
/* ======================================================================== */
void icart_readfile(const char *fname, icartrom_t *icart, int loud);

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
/*                 Copyright (c) 2003-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
