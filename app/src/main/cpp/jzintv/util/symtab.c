/* ======================================================================== */
/*  SYMTAB -- Symbol table routines for DASM0256.                           */
/* ======================================================================== */

#define ADDR(x)  (x) >> 3, (x) & 7

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "symtab.h"

char symtab_cmt_char = '#';

/* ------------------------------------------------------------------------ */
/*  Internal static comparison functions for AVL trees.                     */
/* ------------------------------------------------------------------------ */
static int compare_symbol(symtab_ent_t *a, symtab_ent_t *b)
{
    return -strcmp(a->symbol, b->symbol);
}

static int compare_address(symtab_ent_t *a, symtab_ent_t *b)
{
    return  (a->address > b->address) ? -1 :
            (a->address < b->address) ?  1 :
            (a->addrseq > b->addrseq) ? -1 :
            (a->addrseq < b->addrseq) ?  1 : 0;
}


/* ------------------------------------------------------------------------ */
/*  SYMTAB_CREATE           -- Allocate and return a new symbol table.      */
/* ------------------------------------------------------------------------ */
symtab_t* symtab_create(void)
{
    symtab_t *new_symtab;

    new_symtab = CALLOC(symtab_t, 1);

    if (!new_symtab) { fprintf(stderr, "symtab: Out of memory\n"); exit(1); }

    AVL_InitTree(&new_symtab->by_symbol );
    AVL_InitTree(&new_symtab->by_address);

    AVL_SetTreeComp(&new_symtab->by_symbol,  (PAVLCompFxn) compare_symbol );
    AVL_SetTreeComp(&new_symtab->by_address, (PAVLCompFxn) compare_address);

    symtab_cmt_char = '#';

    return new_symtab;
}

/* ------------------------------------------------------------------------ */
/*  SYMTAB_DESTROY          -- Deallocate an existing symbol table.         */
/* ------------------------------------------------------------------------ */
static int free_data(void *a)
{
    symtab_ent_t *aa = (symtab_ent_t *)a;

    CONDFREE(aa->symbol);   /* free the symbol's name   */
    CONDFREE(aa->xref);     /* free the symbol's xrefs  */
    CONDFREE(aa);           /* free the entry itself    */

    return 0;
}

void symtab_destroy(symtab_t *symtab)
{
    if (!symtab)
        return;

    AVL_Traverse    (&symtab->by_symbol, InOrder, free_data);
    AVL_KillJustTree(&symtab->by_symbol);
    AVL_KillJustTree(&symtab->by_address);
}

/* ------------------------------------------------------------------------ */
/*  SYMTAB_DEFSYM           -- Associate address with symbol.               */
/* ------------------------------------------------------------------------ */
const char *symtab_defsym(symtab_t *symtab, const char *symbol, uint32_t addr)
{
    symtab_ent_t *entry;
    char *sym_copy;
    int err;

    /* -------------------------------------------------------------------- */
    /*  Allocate an entry and set it up.                                    */
    /* -------------------------------------------------------------------- */
    entry    = CALLOC(symtab_ent_t, 1);
    sym_copy = strdup(symbol);
    if (!entry || !sym_copy)
    {
        fprintf(stderr, "symtab_defsym: Out of memory\n");
        exit(1);
    }

    entry->symbol  = sym_copy;
    entry->address = addr;

    /* -------------------------------------------------------------------- */
    /*  Add the address to the "by symbol" tree.  If we find a duplicate,   */
    /*  generate an error if the addresses differ.  If the dup has the      */
    /*  same address, drop this addition.                                   */
    /* -------------------------------------------------------------------- */
    err = AVL_AddNode(&symtab->by_symbol, entry);

    if (err != EAVL_NOERR)
    {
        if (err == EAVL_DUPREC)
        {
            symtab_ent_t *dupl = NULL;
            void         *duplv = NULL;

            if (AVL_SearchTree(&symtab->by_symbol,
                               (void*)entry, &duplv) != EAVL_NOERR)
            {
                fprintf(stderr, "symtab_defsym: Internal error\n");
                exit(1);
            }
            dupl = (symtab_ent_t *)duplv;
            if (dupl->address == entry->address)
            {
                /* This is a harmless duplicate.  Just return. */
                free(sym_copy);
                free(entry);
                return dupl->symbol;
            } else
            {
                fprintf(stderr, "ERROR:  Symbol redefined with different "
                                "addresses!\n %s at %.4X.%.1X vs. %.4X.%.1X\n",
                                entry->symbol,
                                ADDR(entry->address), ADDR(dupl->address));
                exit(1);
            }
        } else
        {
            fprintf(stderr, "symtab_defsym: AVL error %d\n", err);
            exit(1);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Increment our symbol counter.                                       */
    /* -------------------------------------------------------------------- */
    symtab->num_symbols++;

    /* -------------------------------------------------------------------- */
    /*  Add the address to the "by address" tree.  Here, if we find a dup,  */
    /*  just keep increasing our sequence number until we aren't a dup      */
    /*  any more.  This resolves collisions, in a somewhat clunky way.      */
    /* -------------------------------------------------------------------- */
    while (AVL_AddNode(&symtab->by_address, entry) == EAVL_DUPREC)
        entry->addrseq++;

    return sym_copy;
}


/* ------------------------------------------------------------------------ */
/*  SYMTAB_GETSYM           -- Get symbol associated with address.          */
/* ------------------------------------------------------------------------ */
const char *symtab_getsym(symtab_t *symtab, uint32_t addr,
                          int attrib, int which)
{
    symtab_ent_t key, *find = NULL;
    void *findv = NULL;
    int err;
    char buf[64];
    const char *symbol;

    /* -------------------------------------------------------------------- */
    /*  Set up our key structure, and then search the AVL tree for it.      */
    /* -------------------------------------------------------------------- */
    key.address = addr;
    key.addrseq = which;

    err  = AVL_SearchTree(&symtab->by_address, (void*)&key, &findv);
    find = (symtab_ent_t *)findv;

    if (err == EAVL_NULLTREE) err = EAVL_NOTFOUND;

    if (err != EAVL_NOERR && err != EAVL_NOTFOUND)
    {
        fprintf(stderr, "symtab_getsym: AVL error %d\n", err);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If we found it, return it.                                          */
    /* -------------------------------------------------------------------- */
    if (err != EAVL_NOTFOUND)
        return find->symbol;

    /* -------------------------------------------------------------------- */
    /*  If we didn't find it, generate a generic label for this address     */
    /*  if we're allowed to, otherwise just return NULL.                    */
    /* -------------------------------------------------------------------- */
    if (!attrib || which)
        return NULL;

    snprintf(buf, sizeof(buf), "%c_%.4X", attrib, addr >> 3);

    symbol = symtab_defsym(symtab, buf, addr);

    return symbol;
}

/* ------------------------------------------------------------------------ */
/*  SYMTAB_GETADDR          -- Get address associated with symbol.          */
/* ------------------------------------------------------------------------ */
int symtab_getaddr(symtab_t *symtab, const char *symbol, uint32_t *addr)
{
    symtab_ent_t key, *find = NULL;
    void *findv = NULL;
    int err;

    /* -------------------------------------------------------------------- */
    /*  Set up our key structure, and then search the AVL tree for it.      */
    /* -------------------------------------------------------------------- */
    key.symbol = strdup(symbol);

    err  = AVL_SearchTree(&symtab->by_symbol, (void*)&key, &findv);
    find = (symtab_ent_t *)findv;

    free(key.symbol);

    if (err == EAVL_NULLTREE) err = EAVL_NOTFOUND;
    if (err != EAVL_NOERR && err != EAVL_NOTFOUND)
    {
        fprintf(stderr, "symtab_getsym: AVL error %d\n", err);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If we didn't find it, return an error.                              */
    /* -------------------------------------------------------------------- */
    if (err == EAVL_NOTFOUND)
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Otherwise, return the address.                                      */
    /* -------------------------------------------------------------------- */
    *addr = find->address;
    return 0;
}


/* ------------------------------------------------------------------------ */
/*  SYMTAB_XREF_ADDR        -- Say 'addr' is referenced from 'xref'.        */
/* ------------------------------------------------------------------------ */
void symtab_xref_addr(symtab_t *symtab, uint32_t addr, uint32_t xref)
{
    symtab_ent_t key, *find = NULL;
    void *findv = NULL;
    int err, i;

    /* -------------------------------------------------------------------- */
    /*  Set up our key structure, and then search the AVL tree for it.      */
    /* -------------------------------------------------------------------- */
    key.address = addr;
    key.addrseq = 0;

    err  = AVL_SearchTree(&symtab->by_address, (void*)&key, &findv);
    find = (symtab_ent_t *)findv;

    if (err == EAVL_NULLTREE) err = EAVL_NOTFOUND;
    if (err != EAVL_NOERR && err != EAVL_NOTFOUND)
    {
        fprintf(stderr, "symtab_getsym: AVL error %d\n", err);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If we found it, add the cross-reference and return.                 */
    /* -------------------------------------------------------------------- */
    if (err != EAVL_NOTFOUND)
    {
        /* ---------------------------------------------------------------- */
        /*  Avoid adding redundant cross-references.                        */
        /* ---------------------------------------------------------------- */
        for (i = 0; i < find->xrefs; i++)
            if (find->xref[i] == xref)
                return;

        /* ---------------------------------------------------------------- */
        /*  Allocate memory for the cross-reference if needed.              */
        /* ---------------------------------------------------------------- */
        if (find->xrsize <= find->xrefs)
        {
            if (!find->xrsize)  find->xrsize = 4;
            else                find->xrsize <<= 1;

            find->xref = (uint32_t *)realloc(find->xref,
                                            find->xrsize * sizeof(uint32_t));
        }

        /* ---------------------------------------------------------------- */
        /*  Add the cross-reference.                                        */
        /* ---------------------------------------------------------------- */
        find->xref[find->xrefs++] = xref;
    }
}


/* ------------------------------------------------------------------------ */
/*  SYMTAB_DREF_ADDR        -- Say that we've directly referenced 'addr'.   */
/* ------------------------------------------------------------------------ */
void symtab_dref_addr(symtab_t *symtab, uint32_t addr)
{
    symtab_ent_t key, *find = NULL;
    void *findv = NULL;
    int err;

    /* -------------------------------------------------------------------- */
    /*  Set up our key structure, and then search the AVL tree for it.      */
    /* -------------------------------------------------------------------- */
    key.address = addr;
    key.addrseq = 0;

    err  = AVL_SearchTree(&symtab->by_address, (void*)&key, &findv);
    find = (symtab_ent_t *)findv;

    if (err == EAVL_NULLTREE) err = EAVL_NOTFOUND;
    if (err != EAVL_NOERR && err != EAVL_NOTFOUND)
    {
        fprintf(stderr, "symtab_getsym: AVL error %d\n", err);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If we found it, increment the direct-reference counter and return.  */
    /* -------------------------------------------------------------------- */
    if (err != EAVL_NOTFOUND)
    {
        find->drefs++;
    }
}

static int ord_num = 0;
static symtab_ent_t **ord_list = NULL;

/* ------------------------------------------------------------------------ */
/*  PUT_IN_LIST             -- Internal: builds linear list from tree.      */
/* ------------------------------------------------------------------------ */
static int put_in_list(void *sym)
{
    ord_list[ord_num++] = (symtab_ent_t*)sym;
    return 0;
}

/* ------------------------------------------------------------------------ */
/*  DISP_SYM                -- Internal helper:  displays a symbol.         */
/* ------------------------------------------------------------------------ */
static void  disp_sym(FILE *f, symtab_ent_t *sym, int col)
{
    char buf[4] = "## ";
    buf[0] = symtab_cmt_char;

    fprintf(f, "%s%14s:%c %.4X.%.1X%s",
            col == 0 ? buf : "  ",
            sym->symbol,
            sym->addrseq == 0 && sym->drefs == 0 ? '!' :
            sym->addrseq != 0                    ? 'a' : ' ',
            ADDR(sym->address),
            col == 2 ? "\n" : "");
}

/* ------------------------------------------------------------------------ */
/*  DUMP_XREF               -- Internal helper:  displays cross-refs.       */
/* ------------------------------------------------------------------------ */
static FILE *xref_file = NULL;
static int   disp_xref(void *symv)
{
    symtab_ent_t *sym = (symtab_ent_t *)symv;
    int i;
    FILE *f = xref_file;

    if (!f) return -1;

    if (sym->xrefs == 0) return 0;

    for (i = 0; i < sym->xrefs; i++)
    {
        if ((i & 7) == 0)
        {
            if (i == 0)
                fprintf(f, "%c# %14s:%c",
                            symtab_cmt_char, sym->symbol,
                            sym->addrseq == 0 && sym->drefs == 0 ? '!' :
                            sym->addrseq != 0                    ? 'a' : ' ');
            else
                fprintf(f, "\n%c#                 ", symtab_cmt_char);
        }
        fprintf(f, " %.4X.%.1X", ADDR(sym->xref[i]));
    }

    fputc('\n', f);

    return 0;
}

/* ------------------------------------------------------------------------ */
/*  DUMP_GENERIC            -- Internal helper used to dump symbol tbls.    */
/* ------------------------------------------------------------------------ */
static void dump_generic(symtab_t *symtab, PAVLTree tree,
                         const char * sort, FILE *f)
{
    int i, j, k, third;

    fprintf(f, "\n%c#-------------------------------------"
               "---------------------------------------\n", symtab_cmt_char);
    fprintf(f, "%c# Symbol Table, sorted by %s", symtab_cmt_char, sort);
    fprintf(f, "\n%c#-------------------------------------"
               "---------------------------------------\n", symtab_cmt_char);

    ord_num  = 0;
    ord_list = CALLOC(symtab_ent_t *, symtab->num_symbols + 4);
    if (!ord_list)
    {
        fprintf(f, "%c# --> warning, not enough memory for operation\n",
                symtab_cmt_char);
        return;
    }

    AVL_Traverse(tree, InOrder, put_in_list);

    third = (symtab->num_symbols + 2) / 3;

    for (i = j = 0; i < third; i++)
    {
        for (j = 0, k = i; j < 3; j++, k += third)
        {
            if (ord_list[k]) disp_sym(f, ord_list[k], j);
            else
                break;
        }

        if (j != 3) break;
    }

    if (j != 3) fputc('\n', f);

    fprintf(f, "%c#-------------------------------------"
               "---------------------------------------\n", symtab_cmt_char);

    free(ord_list);
}

/* ------------------------------------------------------------------------ */
/*  SYMTAB_DUMP_BY_SYMS     -- Write symbol table dump, sorted by symbol.   */
/* ------------------------------------------------------------------------ */
void     symtab_dump_by_syms(symtab_t *symtab, FILE *f)
{
    dump_generic(symtab, &symtab->by_symbol, "Symbol", f);
}

/* ------------------------------------------------------------------------ */
/*  SYMTAB_DUMP_BY_ADDR     -- Write symbol table dump, sorted by address.  */
/* ------------------------------------------------------------------------ */
void     symtab_dump_by_addr(symtab_t *symtab, FILE *f)
{
    dump_generic(symtab, &symtab->by_address, "Address", f);
}

/* ------------------------------------------------------------------------ */
/*  SYMTAB_DUMP_XREFS       -- Write cross-reference table.                 */
/* ------------------------------------------------------------------------ */
void     symtab_dump_xrefs  (symtab_t *symtab, FILE *f)
{
    fprintf(f, "\n%c#-------------------------------------"
               "---------------------------------------\n", symtab_cmt_char);
    fprintf(f, "%c# Cross Reference Table", symtab_cmt_char);
    fprintf(f, "\n%c#-------------------------------------"
               "---------------------------------------\n", symtab_cmt_char);

    xref_file = f;
    AVL_Traverse(&symtab->by_address, InOrder, disp_xref);

    fprintf(f, "%c#-------------------------------------"
               "---------------------------------------\n", symtab_cmt_char);
}

/* ------------------------------------------------------------------------ */
/*  SYMTAB_GREP_FOR_SYMBOL   -- Step through symbol table looking for       */
/*                              symbols containing substring.               */
/* ------------------------------------------------------------------------ */
static symtab_grep_callback_t grep_callback = NULL;
static const char *grep_string;

static int match_string(void *p)
{
    symtab_ent_t *ent = (symtab_ent_t *)p;
    if (strstr(ent->symbol, grep_string) != NULL)
        grep_callback(ent->symbol, ent->address, ent->addrseq);

    return 0;
}

void symtab_grep_for_symbol(symtab_t *symtab, symtab_grep_callback_t callback,
                            const char *search_string)
{
    grep_callback = callback;
    grep_string   = search_string;

    AVL_Traverse(&symtab->by_symbol, InOrder, match_string);
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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
