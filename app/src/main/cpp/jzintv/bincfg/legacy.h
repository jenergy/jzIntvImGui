/* ======================================================================== */
/*  Title:    Legacy INTVPC BIN+CFG support                                 */
/*  Author:   J. Zbiciak                                                    */
/* ------------------------------------------------------------------------ */
/*  This module implements a memory peripheral with the semantics of        */
/*  INTVPC's BIN+CFG file, at least for the most part.  ECS paged ROM is    */
/*  supported indirectly by instantiating Paged ROMs from mem/mem.c.        */
/*                                                                          */
/*  The routines for reading BIN+CFG files are in bincfg/bincfg.h, not      */
/*  this file.                                                              */
/* ======================================================================== */

#ifndef LEGACY_H_
#define LEGACY_H_

typedef struct legacy_loc_t
{
    uint8_t  width;      /* width of this location                       */
    uint8_t  flags;      /* flags associated with this location.         */
    uint16_t data;       /* data associated with this location.          */
} legacy_loc_t;

typedef struct legacy_t
{
    periph_t    periph;     /* This is a "peripheral".                      */
    legacy_loc_t *loc;
    mem_t       *pg_rom;    /* any paged ROMs that were in the CFG are here */
    int         npg_rom;    /* number of paged ROMs.                        */
    int         jlp_accel;  /* does this legacy game use JLP?               */
    bc_cfgfile_t *bc;       /* config file.  Valid between read & register  */
} legacy_t;

#define LOC_MAPPED (0x80)   /* flag not used by bc_cfgfile_t.               */

/* ======================================================================== */
/*  LEGACY_READ -- read from a legacy BIN+CFG.                              */
/* ======================================================================== */
uint32_t legacy_read (periph_t *per, periph_t *ign, uint32_t addr, uint32_t data);

/* ======================================================================== */
/*  LEGACY_WRITE -- write to a legacy BIN+CFG.                              */
/* ======================================================================== */
void  legacy_write(periph_t *per, periph_t *ign, uint32_t addr, uint32_t data);

/* ======================================================================== */
/*  LEGACY_POKE  -- write to a legacy BIN+CFG, ignoring read-only status.   */
/* ======================================================================== */
void legacy_poke (periph_t *per, periph_t *ign, uint32_t addr, uint32_t data);


/* ======================================================================== */
/*  LEGACY_BINCFG -- Try to determine if a file is BIN+CFG or ROM, and      */
/*                   read it in if it is BIN+CFG.                           */
/*                                                                          */
/*  The return value from this function requires explanation.  If we        */
/*  figure out a .ROM file associated with this fname, we will a distinct   */
/*  char * that points to its filename.  If we determine the file is a      */
/*  BIN+CFG file pair, we will try to load it.  On success, we will return  */
/*  fname directly.  Otherwise, we will return NULL.                        */
/* ======================================================================== */
char *legacy_bincfg
(
    legacy_t        *l,         /*  Legacy BIN+CFG structure        */
    path_t          *path,
    const char      *fname,     /*  Basename to use for CFG/BIN     */
    int             *legacy_rom,
    void            *cpu,
    int             flag_jlp_accel,
    int             flag_jlp_flash,
    int             rand_mem
);

/* ======================================================================== */
/*  LEGACY_REGISTER -- Actually registers the legacy ROMs.  Also frees      */
/*                     the saved bc_cfgfile_t.                              */
/* ======================================================================== */
int legacy_register
(
    legacy_t        *l,
    periph_bus_t    *bus,
    cp1600_t        *cp
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
/*                 Copyright (c) 2003-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
