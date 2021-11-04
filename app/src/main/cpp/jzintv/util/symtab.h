/* ======================================================================== */
/*  SYMTAB -- Symbol table routines for DASM0256.                           */
/* ======================================================================== */
#ifndef SYMTAB_H_
#define SYMTAB_H_

#include "misc/avl.h"

/* ------------------------------------------------------------------------ */
/*  SYMTAB_ENT_T -- Symbol table entry structure.                           */
/* ------------------------------------------------------------------------ */
typedef struct symtab_ent_t
{
    char       *symbol;
    uint32_t    address;
    int         addrseq;
    uint32_t   *xref;
    int         xrefs, xrsize;
    int         drefs;
} symtab_ent_t;

/* ------------------------------------------------------------------------ */
/*  SYMTAB_T -- Encapsulating structure for symbol table.                   */
/* ------------------------------------------------------------------------ */
typedef struct symtab_t
{
    TAVLTree    by_symbol;      /* AVL tree ordered by symbol   */
    TAVLTree    by_address;     /* AVL tree ordered by address  */
    int         num_symbols;    /* total number of symbols      */
} symtab_t;

/* ------------------------------------------------------------------------ */
/*  API Entries:                                                            */
/*                                                                          */
/*  SYMTAB_CREATE           -- Allocate and return a new symbol table.      */
/*  SYMTAB_DESTROY          -- Deallocate a symbol table.                   */
/*  SYMTAB_DEFSYM           -- Associate address with symbol.               */
/*  SYMTAB_GETSYM           -- Get symbol associated with address.          */
/*  SYMTAB_GETADDR          -- Get address associated with symbol.          */
/*  SYMTAB_XREF_ADDR        -- Say 'addr' is referenced from 'xref'.        */
/*  SYMTAB_DREF_ADDR        -- Say that we've directly referenced 'addr'.   */
/*  SYMTAB_DUMP_BY_SYMS     -- Write symbol table dump, sorted by symbol.   */
/*  SYMTAB_DUMP_BY_ADDR     -- Write symbol table dump, sorted by address.  */
/*  SYMTAB_DUMP_XREFS       -- Write cross-reference table.                 */
/*  SYMTAB_GREP_FOR_SYMBOL  -- Search for symbols containing 'string'       */
/* ------------------------------------------------------------------------ */
symtab_t *symtab_create      (void);
void      symtab_destroy     (symtab_t *symtab);
const char*symtab_defsym     (symtab_t *symtab, const char *sym, uint32_t addr);
const char*symtab_getsym     (symtab_t *symtab, uint32_t addr, int attr, int w);
int       symtab_getaddr     (symtab_t *symtab, const char *sym,uint32_t *addr);
void      symtab_xref_addr   (symtab_t *symtab, uint32_t addr, uint32_t xref);
void      symtab_dref_addr   (symtab_t *symtab, uint32_t addr);
void      symtab_dump_by_syms(symtab_t *symtab, FILE *f);
void      symtab_dump_by_addr(symtab_t *symtab, FILE *f);
void      symtab_dump_xrefs  (symtab_t *symtab, FILE *f);

typedef void (*symtab_grep_callback_t)(const char *, uint32_t, int);

void symtab_grep_for_symbol(symtab_t*, symtab_grep_callback_t, const char*);

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
