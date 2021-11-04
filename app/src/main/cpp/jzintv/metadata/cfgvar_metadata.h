#ifndef CFGVAR_METADATA_H_
#define CFGVAR_METADATA_H_

/* ======================================================================== */
/*  GAME_METADATA_FROM_CFGVARS   -- Get all metadata from list of cfg vars  */
/*                                                                          */
/*  The 'default_compat' flags returns true if all the metadata returned    */
/*  represents the assumed defaults, as opposed to explicitly-set values.   */
/* ======================================================================== */
game_metadata_t *game_metadata_from_cfgvars
(
    cfg_var_t *RESTRICT const vars
);

/* ======================================================================== */
/*  CFGVARS_FROM_GAME_METADATA                                              */
/*  Create a list of configuration variables from game_metadata_t.          */
/* ======================================================================== */
cfg_var_t *cfgvars_from_game_metadata
(
    game_metadata_t *RESTRICT const m
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
/* ------------------------------------------------------------------------ */
/*                 Copyright (c) 2016-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
