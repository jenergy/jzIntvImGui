/* ======================================================================== */
/*  Routines for writing a .BIN and .CFG from an icartrom_t.                */
/* ======================================================================== */
#ifndef ICARTBIN_H_
#define ICARTBIN_H_ 1
#include "icart/icartrom.h"

/* ======================================================================== */
/*  ROUTINES FOR WRITING BIN+CFG                                            */
/* ======================================================================== */

/* ======================================================================== */
/*  ICB_SHOW_RANGES                                                         */
/*  Shows a list of ranges of addresses represented by a bit-vector.        */
/* ======================================================================== */
void icb_show_ranges(uint32_t *bv);

/* ======================================================================== */
/*  ICB_WRITE_MAPPINGS                                                      */
/*  Writes the [mappings] section of a .CFG file, based on the icartrom.    */
/* ======================================================================== */
int icb_write_mappings(FILE *fb, FILE *fc, icartrom_t *icart, int ofs);

/* ======================================================================== */
/*  ICB_WRITE_PRELOADS                                                      */
/*  Writes the [preload] section of a .CFG, which addresses ranges of       */
/*  address that are preloaded, but not readable.                           */
/* ======================================================================== */
int icb_write_preloads(FILE *fb, FILE *fc, icartrom_t *icart, int ofs);

/* ======================================================================== */
/*  ICB_WRITE_BANKSW                                                        */
/*  Writes the [bankswitch] section.  These are sections marked for         */
/*  Intellicart-style bankswitching.                                        */
/* ======================================================================== */
void icb_write_banksw(FILE *fc, icartrom_t *icart);

/* ======================================================================== */
/*  ICB_WRITE_MEMATTR                                                       */
/*  Writes the [memattr] section.  These are sections marked as RAM.        */
/* ======================================================================== */
void icb_write_memattr(FILE *fc, icartrom_t *icart);

/* ======================================================================== */
/*  ICB_WRITE_CFGVARS    -- Write [vars] section from metadata, if any.     */
/* ======================================================================== */
void icb_write_cfgvars(FILE *fc, icartrom_t *icart);

/* ======================================================================== */
/*  ICB_WRITE_BINCFG                                                        */
/*  Write out an entire BIN+CFG.                                            */
/* ======================================================================== */
int icb_write_bincfg(FILE *fb, FILE *fc, icartrom_t *icart, int ofs);
int icb_writefile   (char *fb, char *fc, icartrom_t *icart);

/* ======================================================================== */
/*  ICB_READ_BINCFG -- Reads a .BIN and optional .CFG file.                 */
/* ======================================================================== */
struct game_metadata_t;     /* fwd reference */
void icb_read_bincfg(char *bin_fn, char *cfg_fn, icartrom_t *the_icart,
                     int loud);

/* ======================================================================== */
/*  ICB_SHOW_SUMMARY                                                        */
/*  Show a bunch of human-readable info about an icartrom.                  */
/* ======================================================================== */
void icb_show_summary(icartrom_t *icart);

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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
