/* ======================================================================== */
/*  BINCFG   -- Routines for reading a configuration file for the BIN+CFG   */
/*              file format.  Includes debug functions which can generate   */
/*              a new configuration file from the parsed config file.       */
/*                                                                          */
/*  Parser interface functions, intended to be called for reading a .CFG:   */
/*                                                                          */
/*  BC_PARSE_CFG  -- Reads a configuration file using the lexer/grammar.    */
/*  BC_READ_DATA  -- Reads ROM segments and attaches them to bc_cfgfile_t.  */
/*  BC_DO_MACROS  -- Applies macros that can be safely applied statically.  */
/*  BC_FREE_CFG   -- Frees all memory associated with a bc_cfgfile_t.       */
/*                                                                          */
/*  Structure printing functions for generating a .CFG from a parsed CFG.   */
/*  The following functions are compiled out if BC_NOPRINT is defined.      */
/*                                                                          */
/*  BC_PRINT_CFG  -- Chase through a bc_cfgfile_t structure and print out   */
/*                   what we find therein.  It calls the following helpers: */
/*                                                                          */
/*  BC_PRINT_DIAG    -- Print all the collected diagnostics attached to cfg */
/*  BC_PRINT_MACRO   -- Print the [macro] section                           */
/*  BC_PRINT_MACRO_T -- Print a single bc_macro_t                           */
/*  BC_PRINT_VARLIKE -- Print [var],[keys],[joystick] sections              */
/*  BC_PRINT_VAR_T   -- Print a single <name>,<value> tuple                 */
/*  BC_PRINT_MEMSPAN -- Print out all the memory span information.          */
/* ======================================================================== */

#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#ifndef BC_NOMETADATA
#   include "metadata/metadata.h"
#   include "metadata/cfgvar_metadata.h"
#endif
#include "bincfg/bincfg.h"
#include "bincfg/bincfg_lex.h"
#include "bincfg/bincfg_grmr.tab.h"

bc_cfgfile_t *bc_parsed_cfg = NULL;

extern int bc_parse(void);  /* grrr... bison doesn't do this for us?!       */

/* ======================================================================== */
/*  BC_PARSE_CFG  -- Invokes the lexer and grammar to parse the config.     */
/* ======================================================================== */
bc_cfgfile_t *bc_parse_cfg
(
    LZFILE *f,
    const char *const binfile,
    const char *const cfgfile
)
{
    bc_memspan_t *span, **prev;
    int num_preload = 0, num_memattr = 0, need_default = 0, ma, pl;
    bc_memspan_t **preloads, **memattrs;

    bc_parsed_cfg = NULL;

    /* -------------------------------------------------------------------- */
    /*  Scan in the file, ignoring errors returned directly from bc_parse.  */
    /*  All the diagnostics will be attached to whatever cfg we generate.   */
    /* -------------------------------------------------------------------- */
    if (f)
    {
        bc_restart( (FILE*)f ); /* register the file with the lexer.        */
        bc_parse();             /* run the grammar.  It calls the bc_lex(). */
    }

    /* -------------------------------------------------------------------- */
    /*  If not parsing a configuration file, or the file was empty, just    */
    /*  make an empty config as a starting point.                           */
    /* -------------------------------------------------------------------- */
    if (!bc_parsed_cfg)
    {
        bc_parsed_cfg = CALLOC(bc_cfgfile_t, 1);
    }

    /* -------------------------------------------------------------------- */
    /*  Leave if we get here without a valid config.                        */
    /* -------------------------------------------------------------------- */
    if (!bc_parsed_cfg)
        return NULL;

    if (binfile) bc_parsed_cfg->binfile = strdup(binfile);
    if (cfgfile) bc_parsed_cfg->cfgfile = strdup(cfgfile);

    /* -------------------------------------------------------------------- */
    /*  Scan the memory spans counting up preload sections and non-preload  */
    /*  (ie. memattr) sections.  We need to apply memattr spans to any      */
    /*  preload spans they overlap, but before we do so, we need to         */
    /*  allocate some storage.                                              */
    /* -------------------------------------------------------------------- */
    for (span = bc_parsed_cfg->span; span;
         span = (bc_memspan_t*)(span->l.next))
    {
        if (span->flags & BC_SPAN_PL) num_preload++;
        else                          num_memattr++;
    }

    /* -------------------------------------------------------------------- */
    /*  If there were no preload sections, we'll need a default config.     */
    /*  The default config gets instantiated after we allocate arrays.      */
    /* -------------------------------------------------------------------- */
    if (num_preload == 0)
    {
        num_preload = 3;
        need_default = 1;
    }

    /* -------------------------------------------------------------------- */
    /*  Allocate storage for the preload and memattr lists and fill them.   */
    /* -------------------------------------------------------------------- */
    preloads = CALLOC(bc_memspan_t*, num_preload);
    memattrs = CALLOC(bc_memspan_t*, num_memattr);
    assert((preloads && memattrs) || "Out of memory");
    for (span = bc_parsed_cfg->span, ma = pl = 0; span;
         span = (bc_memspan_t*)(span->l.next))
    {
        if (span->flags & BC_SPAN_PL) preloads[pl++] = span;
        else                          memattrs[ma++] = span;
    }
    assert( pl == num_preload || (need_default && pl == 0));
    assert( ma == num_memattr );

    /* -------------------------------------------------------------------- */
    /*  If there were no preload sections, create a default mapping.        */
    /* -------------------------------------------------------------------- */
    if (need_default)
    {
        bc_memspan_t *s0, *s1, *s2;

        if (!(s0 = CALLOC(bc_memspan_t, 1)) ||
            !(s1 = CALLOC(bc_memspan_t, 1)) ||
            !(s2 = CALLOC(bc_memspan_t, 1)))
        {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }

        s0->s_fofs = 0x0000;
        s0->e_fofs = 0x1FFF;
        s0->s_addr = 0x5000;
        s0->e_addr = 0x6FFF;
        s0->flags  = BC_SPAN_R | BC_SPAN_PL;
        s0->width  = 16;
        s0->epage  = BC_SPAN_NOPAGE;
        s0->f_name = NULL;
        s0->l.next = NULL;

        s1->s_fofs = 0x2000;
        s1->e_fofs = 0x2FFF;
        s1->s_addr = 0xD000;
        s1->e_addr = 0xDFFF;
        s1->flags  = BC_SPAN_R | BC_SPAN_PL;
        s1->width  = 16;
        s1->epage  = BC_SPAN_NOPAGE;
        s1->f_name = NULL;
        s1->l.next = NULL;

        s2->s_fofs = 0x3000;
        s2->e_fofs = 0x3FFF;
        s2->s_addr = 0xF000;
        s2->e_addr = 0xFFFF;
        s2->flags  = BC_SPAN_R | BC_SPAN_PL;
        s2->width  = 16;
        s2->epage  = BC_SPAN_NOPAGE;
        s2->f_name = NULL;
        s2->l.next = NULL;

        assert( preloads );
        assert( num_preload == 3 );
        preloads[0] = s0;
        preloads[1] = s1;
        preloads[2] = s2;
    }

    /* -------------------------------------------------------------------- */
    /*  Step through all of the memattr sections, applying their attribute  */
    /*  flags to overlapping preload sections.  This is O(N^2), but who     */
    /*  cares, as the total number of spans is at most dozens.  (Worse      */
    /*  than O(N^2) if we have to split a span.)                            */
    /* -------------------------------------------------------------------- */
    for ( ma = 0 ; ma < num_memattr ; ma++ )
    {
        bc_memspan_t *m = memattrs[ma];

        for ( pl = 0 ; pl < num_preload ; pl++ )
        {
            bc_memspan_t *p = preloads[pl];

            /* Skip if spans don't overlap */
            if (m->e_addr < p->s_addr) continue;
            if (m->s_addr > p->e_addr) continue;
            if (m->epage != p->epage)  continue;

            /* Is p completely inside m? No: split p. Fragments go to end.  */
            if (p->s_addr < m->s_addr)
            {
                int np = num_preload++;
                bc_memspan_t *n;
                preloads = (bc_memspan_t**)realloc((void*)preloads,
                               num_preload * sizeof(bc_memspan_t*));
                assert(preloads || "Out of memory");

                n = preloads[np] = CALLOC(bc_memspan_t, 1);
                assert(preloads[np] || "Out of memory");

                *n = *p;

                n->e_addr = m->s_addr - 1;
                p->s_addr = m->s_addr;
                p->s_fofs = p->e_fofs - p->e_addr + p->s_addr;
                n->e_fofs = n->s_fofs + n->e_addr - n->s_addr;
            }

            if (p->e_addr > m->e_addr)
            {
                int np = num_preload++;
                bc_memspan_t *n;
                preloads = (bc_memspan_t**)realloc((void*)preloads,
                               num_preload * sizeof(bc_memspan_t*));
                assert(preloads || "Out of memory");

                n = preloads[np] = CALLOC(bc_memspan_t, 1);
                assert(preloads[np] || "Out of memory");

                *n = *p;

                n->s_addr = m->e_addr + 1;
                p->e_addr = m->e_addr;
                n->s_fofs = n->e_fofs - n->e_addr + n->s_addr;
                p->e_fofs = p->s_fofs + p->e_addr - p->s_addr;
            }

            /* Is m completely inside p? No: split m. Fragments go to end.  */
            if (m->s_addr < p->s_addr)
            {
                int nm = num_memattr++;
                bc_memspan_t *n;
                memattrs = (bc_memspan_t**)realloc((void*)memattrs,
                               num_memattr * sizeof(bc_memspan_t*));
                assert(memattrs || "Out of memory");

                n = memattrs[nm] = CALLOC(bc_memspan_t, 1);
                assert(memattrs[nm] || "Out of memory");

                *n = *m;

                n->e_addr = p->s_addr - 1;
                m->s_addr = p->s_addr;
            }

            if (m->e_addr > p->e_addr)
            {
                int nm = num_memattr++;
                bc_memspan_t *n;
                memattrs = (bc_memspan_t**)realloc((void*)memattrs,
                               num_memattr * sizeof(bc_memspan_t*));
                assert(memattrs || "Out of memory");

                n = memattrs[nm] = CALLOC(bc_memspan_t, 1);
                assert(memattrs[nm] || "Out of memory");

                *n = *m;

                n->s_addr = p->e_addr + 1;
                m->e_addr = p->e_addr;
            }

            assert(m->s_addr == p->s_addr);
            assert(m->e_addr == p->e_addr);

            /* Apply the memattr to the preload */
            p->width = m->width;
            p->flags = (m->flags | BC_SPAN_PL) & ~BC_SPAN_DEL;
            m->flags |= BC_SPAN_DEL;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Now reassemble the span list.  For memattr spans, only keep the     */
    /*  ones not marked BC_SPAN_DEL, as those didn't overlap any preload.   */
    /* -------------------------------------------------------------------- */
    prev = &(bc_parsed_cfg->span);

    for (pl = 0; pl < num_preload; pl++)
    {
        *prev = preloads[pl];
        prev = (bc_memspan_t**)&(preloads[pl]->l.next);
        assert( (preloads[pl]->flags & BC_SPAN_DEL) == 0 );
    }

    for (ma = 0; ma < num_memattr; ma++)
    {
        if (memattrs[ma]->flags & BC_SPAN_DEL)
        {
            free(memattrs[ma]);
            continue;
        }
        *prev = memattrs[ma];
        prev = (bc_memspan_t**)&(memattrs[ma]->l.next);
    }
    *prev = NULL;

    free(preloads);
    free(memattrs);

    /* -------------------------------------------------------------------- */
    /*  By default, assume no decoded metadata.                             */
    /* -------------------------------------------------------------------- */
    bc_parsed_cfg->metadata = NULL;

#ifndef BC_NOMETADATA
    /* -------------------------------------------------------------------- */
    /*  Parse config variables into a metadata structure.                   */
    /* -------------------------------------------------------------------- */
    bc_parsed_cfg->metadata =
        bc_parsed_cfg->vars ? game_metadata_from_cfgvars(bc_parsed_cfg->vars)
                            : default_game_metadata();
#endif
    return bc_parsed_cfg;
}

/* ======================================================================== */
/*  BC_READ_HELPER -- helper function for bc_read_data.                     */
/* ======================================================================== */
LOCAL int bc_read_helper(char *f_name, uint16_t *buf, int width)
{
    LZFILE *f;
    int len;

    /* -------------------------------------------------------------------- */
    /*  Open and read up to 64K words from a file.  Return how much we      */
    /*  actually read from the file to the caller.                          */
    /* -------------------------------------------------------------------- */
    if ((f = lzoe_fopen(f_name, "rb")) == NULL)
    {

        perror("fopen()");
        fprintf(stderr, "Could not open binary file '%s' for reading.\n",
                f_name);
        return -1;
    }

    len = width > 8 ? file_read_rom16(f, BC_MAXBIN, buf)
                    : file_read_rom8 (f, BC_MAXBIN, buf);

    if (len < 1)
    {
        fprintf(stderr, "Unable to read binary file '%s'\n", f_name);
        return -1;
    }

    lzoe_fclose(f);

    return len;
}

/* ======================================================================== */
/*  BC_READ_DATA  -- Reads ROM segments and attaches them to bc_cfgfile_t.  */
/*                                                                          */
/*  This pass will adjust, or remove and free memspans that are partially   */
/*  or completely outside the bounds of the associated binfile.             */
/* ======================================================================== */
int bc_read_data(bc_cfgfile_t *bc)
{
    bc_memspan_t *span, *prev;
    uint16_t *pbuf, *lbuf, *buf = NULL;
    size_t plen, llen, len = 0;
    int slen, i;

    /* -------------------------------------------------------------------- */
    /*  Allocate a temporary buffer for the primary BIN file.  Note that    */
    /*  we can't just parcel our BIN segments out of this buffer with       */
    /*  pointer manipulation unless there is only 1 such segment.  This     */
    /*  is due to the 'free()' semantics for the ->data field in each       */
    /*  memspan.                                                            */
    /* -------------------------------------------------------------------- */
    if ((pbuf = CALLOC(uint16_t, BC_MAXBIN*2)) == NULL)
    {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    lbuf = pbuf + BC_MAXBIN;

    for (i = 0; i < BC_MAXBIN; i += 2)
    {
        pbuf[i + 0] = 0xDEAD;
        pbuf[i + 1] = 0xBEEF;
        lbuf[i + 0] = 0xDEAD;
        lbuf[i + 1] = 0xBEEF;
    }

    /* -------------------------------------------------------------------- */
    /*  Load the primary file up front.                                     */
    /* -------------------------------------------------------------------- */
    if (!bc->binfile || !file_exists(bc->binfile))
    {
        plen = 0;
    } else
    {
        int tmp;

        /* Assume primary binfile is width >= 9. */
        tmp = bc_read_helper(bc->binfile, pbuf, 16);

        if (tmp < 0) { free(pbuf); return -1; }

        plen = (size_t)tmp;
    }

    /* -------------------------------------------------------------------- */
    /*  Chase down the spans, attaching ->data to each span.                */
    /* -------------------------------------------------------------------- */
    for (prev = NULL, span = bc->span; span;
         prev = span, span = (bc_memspan_t *)(span->l.next))
    {
        bc_memspan_t dummy;

        /* ---------------------------------------------------------------- */
        /*  Skip spans that already have data attached -- assume they are   */
        /*  correct.  These are usually POKE spans.                         */
        /* ---------------------------------------------------------------- */
        if (span->data)
            continue;

        assert((span->flags & BC_SPAN_PK) == 0);

        /* ---------------------------------------------------------------- */
        /*  Skip spans that dopn't preload a data segment.  These would be  */
        /*  banksw/memattr spans.                                           */
        /* ---------------------------------------------------------------- */
        if ((span->flags & BC_SPAN_PL) == 0)
            continue;

        /* ---------------------------------------------------------------- */
        /*  Load any file that might be associated with this span.          */
        /* ---------------------------------------------------------------- */
        if (span->f_name != NULL)
        {
            int tmp;

            tmp = bc_read_helper(span->f_name, lbuf, 16);

            if (tmp < 0) { free(pbuf); return -1; }

            llen = (size_t)tmp;
            len = llen;
            buf = lbuf;
        } else if (span->f_name == NULL) /* true if primary-file span. */
        {
            len = plen;
            buf = pbuf;
        }

        /* ---------------------------------------------------------------- */
        /*  Now attach data from the file to the memspan.  We handle spans  */
        /*  that go outside the file specially, and differently depending   */
        /*  on whether they're writeable.                                   */
        /*   -- Non-writeable, entirely beyond EOF:  delete span            */
        /*   -- Non-writeable, partially beyond EOF:  trim span             */
        /*   -- Writeable, entirely beyond EOF:  drop preload flag          */
        /*   -- Writeable, partially beyond EOF:  split and drop preload    */
        /*      flag on part beyond EOF.                                    */
        /* ---------------------------------------------------------------- */

        /* ---------------------------------------------------------------- */
        /*   -- Non-writeable, entirely beyond EOF:  delete span            */
        /* ---------------------------------------------------------------- */
        if (span->s_fofs >= len && (span->flags & BC_SPAN_W) == 0)
        {
            ll_t *dead = (ll_t *)span;

            if (!prev)
            {
                bc->span     = (bc_memspan_t *)span->l.next;
                dummy.l.next = span->l.next;
                span         = &dummy;
            } else
            {
                prev->l.next = span->l.next;
                span         = prev;
            }
#ifndef BC_NOFREE
            bc_free_memspan_t(dead, NULL);
#endif
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*   -- Non-writeable, partially beyond EOF:  trim span             */
        /* ---------------------------------------------------------------- */
        if (span->e_fofs >= len && (span->flags & BC_SPAN_W) == 0)
            span->e_fofs = len - 1;

        /* ---------------------------------------------------------------- */
        /*   -- Writeable, entirely beyond EOF:  drop preload flag          */
        /* ---------------------------------------------------------------- */
        if (span->s_fofs >= len && (span->flags & BC_SPAN_W) != 0)
        {
            span->flags &= ~BC_SPAN_PL;
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*   -- Writeable, partially beyond EOF:  split and drop preload    */
        /*      flag on part beyond EOF.                                    */
        /* ---------------------------------------------------------------- */
        if (span->e_fofs >= len && (span->flags & BC_SPAN_W) != 0)
        {
            bc_memspan_t *part = CALLOC(bc_memspan_t, 1);
            *part = *span;
            part->flags &= BC_SPAN_PL;
            part->s_fofs = 0;
            part->e_fofs = 0;
            part->s_addr = span->s_addr + len - span->s_fofs;
            span->e_fofs = len - 1;
            span->e_addr = span->s_addr + span->e_fofs - span->s_fofs;
            part->l.next = (ll_t *)span;
            prev->l.next = (ll_t *)part;
        }

        /* ---------------------------------------------------------------- */
        /*  Allocate span->data and copy over the data from the file.       */
        /* ---------------------------------------------------------------- */
        slen         = span->e_fofs - span->s_fofs + 1;
        span->e_addr = span->s_addr + slen - 1;
        span->data   = CALLOC(uint16_t, slen);

        if (!span->data) { fprintf(stderr, "out of memory\n"); exit(1); }

        assert(buf);
        memcpy(span->data, buf + span->s_fofs, slen * sizeof(uint16_t));
    }

    free(pbuf);
    return 0;
}



#ifndef BC_NODOMACRO /* BC_NODOMACRO if you don't want to interpret macros */
/* ======================================================================== */
/*  BC_DO_MACROS  -- Applies macros that can be safely applied statically.  */
/*                                                                          */
/*  I offer two modes here:                                                 */
/*                                                                          */
/*      1.  Execute up until the first macro we can't do statically, and    */
/*                                                                          */
/*      2.  Scan the macros, and execute them only if they're all safe      */
/*          to execute or ignore.  Special case:  We'll go ahead and        */
/*          execute a macro set that ends in a "run" command as long as     */
/*          it's the last macro in the list.                                */
/*                                                                          */
/*  Macros that are safe to execute statically:                             */
/*                                                                          */
/*      L <file> <width> <addr> 'L'oad <file> of <width> at <addr>.         */
/*      P <addr> <data>         'P'oke <data> at <addr>.                    */
/*                                                                          */
/*  Macros that are safe to treat as NOPs during this pass:                 */
/*                                                                          */
/*    <n/a>                     Null macro lines                            */
/*      A                       Shows the instructions 'A'head.             */
/*      I <addr>                'I'nspect data/code at <addr>.              */
/*      W <name> <list>         'W'atch a set of values with label <name>.  */
/*      V                       'V'iew emulation display                    */
/*                                                                          */
/*  Macros that are NOT safe to execute statically:                         */
/*                                                                          */
/*    [0-7] <value>             Set register <n> to <value>                 */
/*      B                       Run until next vertical 'B'lank.            */
/*      R                       'R'un emulation.                            */
/*      T <addr>                'T'race to <addr>.                          */
/*      O <addr>                Run t'O' <addr>.                            */
/*    <unk>                     Any unknown macros that are in the list.    */
/*                                                                          */
/*  We implement L and P by tacking additional ROM segments to the          */
/*  memspan list.  Load and Poke macros implicitly set "Preload" and        */
/*  "Read" attributes.  Load may also set 'Narrow' if the ROM is <= 8 bits  */
/*  wide.  Poke will set the "POKE" attribute.  No others are set.          */
/* ======================================================================== */
int  bc_do_macros(bc_cfgfile_t *cfg, int partial_ok)
{
    bc_macro_t *macro;
    bc_memspan_t *newspan = NULL;

    /* -------------------------------------------------------------------- */
    /*  Trivial case:  Zero macros.                                         */
    /* -------------------------------------------------------------------- */
    if (!cfg->macro)
        return 0;

    /* -------------------------------------------------------------------- */
    /*  If given an all-or-nothing directive, prescan macro list to make    */
    /*  sure it is acceptable.                                              */
    /* -------------------------------------------------------------------- */
    if (!partial_ok)
    {
        for (macro = bc_parsed_cfg->macro; macro;
             macro = (bc_macro_t*)macro->l.next)
        {
            if (macro->cmd != BC_MAC_LOAD    &&
                macro->cmd != BC_MAC_POKE    &&
                macro->cmd != BC_MAC_NONE    &&
                macro->cmd != BC_MAC_AHEAD   &&
                macro->cmd != BC_MAC_INSPECT &&
                macro->cmd != BC_MAC_WATCH   &&
                macro->cmd != BC_MAC_VIEW)
                break;
        }

        /* ---------------------------------------------------------------- */
        /*  If we exited before end of list, we're at an unsupported macro. */
        /*  If it's not a "run" macro at the end of the list, refuse it.    */
        /* ---------------------------------------------------------------- */
        if (macro != NULL &&
            (macro->l.next != NULL || macro->cmd != BC_MAC_RUN))
            return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Chase down the macro list looking for LOAD/POKE commands.           */
    /* -------------------------------------------------------------------- */
    for (macro = bc_parsed_cfg->macro; macro;
         macro = (bc_macro_t*)macro->l.next)
    {
        if (macro->cmd == BC_MAC_LOAD || macro->cmd == BC_MAC_POKE)
        {
            newspan = CALLOC(bc_memspan_t, 1);
            if (!newspan)
            {
                fprintf(stderr, "like, out of memory or something\n");
                exit(1);
            }

            newspan->l.next = NULL;

            /* Inefficient, but called very rarely. */
            LL_CONCAT(bc_parsed_cfg->span, newspan, bc_memspan_t);
        }

        switch (macro->cmd)
        {
            case BC_MAC_LOAD :
            {
                newspan->s_fofs = 0;
                newspan->e_fofs = 0xFFFF;
                newspan->s_addr = macro->arg.load.addr;
                newspan->e_addr = 0; /* calculated on the fly. */
                newspan->width  = macro->arg.load.width;
                newspan->flags  = BC_SPAN_PL | BC_SPAN_R;
                newspan->epage  = BC_SPAN_NOPAGE;
                newspan->f_name = strdup(macro->arg.load.name);

                if (newspan->width <= 8)
                    newspan->flags |= BC_SPAN_N;

                break;
            }

            case BC_MAC_POKE :
            {
                if (!(newspan->data = CALLOC(uint16_t, 1)))
                {
                    fprintf(stderr, "out of memory\n");
                    exit(1);
                }

                newspan->s_fofs  = 0;
                newspan->e_fofs  = 0;
                newspan->s_addr  = macro->arg.poke.addr;
                newspan->e_addr  = macro->arg.poke.addr;
                newspan->width   = 16;
                newspan->flags   = BC_SPAN_PL | BC_SPAN_R | BC_SPAN_PK;
                newspan->epage   = macro->arg.poke.epage;
                newspan->f_name  = NULL;
                newspan->data[0] = macro->arg.poke.value;

                if (macro->arg.poke.epage != BC_SPAN_NOPAGE)
                    newspan->flags |= BC_SPAN_EP;

                break;
            }

            case BC_MAC_NONE:
            case BC_MAC_AHEAD:
            case BC_MAC_WATCH:
            case BC_MAC_VIEW:
            {
                /* ignore */
                break;
            }

            default:
            {
                macro = NULL; /* force loop to terminate. */
                break;
            }
        }
        newspan = NULL;
    }


    return 0;
}
#endif


#ifndef BC_NOFREE /* BC_NOFREE if you have no intention of freeing a cfg */
/* ======================================================================== */
/*  BC_FREE_CFG   -- Release storage held by bc_cfgfile_t.                  */
/*                                                                          */
/*  Helper functions for FREE_CFG:                                          */
/*                                                                          */
/*  BC_FREE_MEMSPAN_T    -- Releases storage associated with bc_memspan_t.  */
/*  BC_FREE_MACRO_T      -- Releases storage associated with bc_macro_t.    */
/*  BC_FREE_DIAG_T       -- Releases storage associated with bc_diag_t      */
/* ======================================================================== */
#ifndef CONDFREE
# define CONDFREE(x) do { if (x) free(x); } while (0)
#endif

/* ======================================================================== */
/*  BC_FREE_MEMSPAN_T    -- Releases storage associated with bc_memspan_t.  */
/* ======================================================================== */
void bc_free_memspan_t(ll_t *l_mem, void *unused)
{
    bc_memspan_t *mem = (bc_memspan_t*)l_mem;
    UNUSED(unused);

    CONDFREE(mem->f_name);
    CONDFREE(mem->data);
    free(mem);
}

/* ======================================================================== */
/*  BC_FREE_MACRO_T      -- Releases storage associated with bc_macro_t.    */
/* ======================================================================== */
void bc_free_macro_t(ll_t *l_mac, void *unused)
{
    bc_macro_t *mac = (bc_macro_t *)l_mac;

    UNUSED(unused);

    switch (mac->cmd)
    {
        case BC_MAC_LOAD:   CONDFREE(mac->arg.load.name);   break;

        case BC_MAC_WATCH:  CONDFREE(mac->arg.watch.name);
                            CONDFREE(mac->arg.watch.addr);  break;

        default: /* nothing */ ;
    }
    free(mac);
}

/* ======================================================================== */
/*  BC_FREE_DIAG_T       -- Releases storage associated with bc_diag_t      */
/* ======================================================================== */
void bc_free_diag_t(ll_t *l_diag, void *unused)
{
    bc_diag_t *diag = (bc_diag_t *)l_diag;

    UNUSED(unused);

    CONDFREE(diag->sect);
    CONDFREE(diag->msg);
    free(diag);
}


/* ======================================================================== */
/*  BC_FREE_CFG   -- Release storage held by bc_cfgfile_t.                  */
/* ======================================================================== */
void bc_free_cfg(bc_cfgfile_t *cfg)
{
    CONDFREE(cfg->cfgfile);
    CONDFREE(cfg->binfile);

    if (cfg->span    ) LL_ACTON(cfg->span,  bc_free_memspan_t, NULL);
    if (cfg->macro   ) LL_ACTON(cfg->macro, bc_free_macro_t,   NULL);
    if (cfg->vars    ) free_cfg_var_list( cfg->vars     );
    if (cfg->keys[0] ) free_cfg_var_list( cfg->keys[0]  );
    if (cfg->keys[1] ) free_cfg_var_list( cfg->keys[1]  );
    if (cfg->keys[2] ) free_cfg_var_list( cfg->keys[2]  );
    if (cfg->keys[3] ) free_cfg_var_list( cfg->keys[3]  );
    if (cfg->joystick) free_cfg_var_list( cfg->joystick );
    if (cfg->diags   ) LL_ACTON(cfg->diags, bc_free_diag_t,    NULL);

#ifndef BC_NOMETADATA
    if (cfg->metadata) free_game_metadata( cfg->metadata );
#endif

    free(cfg);
}
#endif


#ifndef BC_NOPRINT
/* ======================================================================== */
/*  BC_PRINT_CFG  -- Chase through a bc_cfgfile_t structure and print out   */
/*                   what we find therein.                                  */
/*                                                                          */
/*  Helper functions for PRINT_CFG:                                         */
/*                                                                          */
/*  BC_PRINT_DIAG    -- Print all the collected diagnostics attached to cfg */
/*  BC_PRINT_MACRO   -- Print the [macro] section                           */
/*  BC_PRINT_VARLIKE -- Print [var],[keys],[joystick] sections              */
/*  BC_PRINT_MEMSPAN -- Print out all the memory span information.          */
/* ======================================================================== */

/* ======================================================================== */
/*  BC_PRINT_DIAG    -- Print all the collected diagnostics attached to cfg */
/* ======================================================================== */
void bc_print_diag
(
    printer_t       *RESTRICT const p,
    const char      *RESTRICT const fname,
    const bc_diag_t *RESTRICT const diag,
    const int                       cmt
)
{
    const bc_diag_t *RESTRICT d;
    int tot_warn = 0, tot_err = 0;

    /* -------------------------------------------------------------------- */
    /*  Step through the list and dump these out.  If cmt != 0, put a       */
    /*  semicolon before each line.                                         */
    /* -------------------------------------------------------------------- */
    for (d = diag; d; d = (const bc_diag_t *)d->l.next)
    {
        p->fxn(p->opq, "%s%s:%d:%s %s: %s\015\012",
                cmt ? "; " : "", fname, d->line,
                d->sect ? d->sect : "<toplevel?>",
                d->type == BC_DIAG_WARNING ? "warning" : "error",
                d->msg  ? d->msg  : "out of memory?");
        if (d->type == BC_DIAG_WARNING) tot_warn++; else tot_err++;
    }

    /* -------------------------------------------------------------------- */
    /*  If we had any warnings or errors, print summary information.        */
    /* -------------------------------------------------------------------- */
    if (tot_warn || tot_err)
    {
        p->fxn(p->opq, "%s%d warnings, %d errors\015\012",
                cmt ? "; " : "", tot_warn, tot_err);
    }
}

/* ======================================================================== */
/*  BC_PRINT_MACRO_T -- Print a single macro_t structure.                   */
/* ======================================================================== */
LOCAL void bc_print_macro_t(ll_t *const l_mac, void *RESTRICT const v_p)
{
    const bc_macro_t *RESTRICT const mac = (bc_macro_t*)l_mac;
    const printer_t *RESTRICT const p = (printer_t*)v_p;

    if (mac->quiet)
        p->fxn(p->opq, "@");

    switch (mac->cmd)
    {
        case BC_MAC_NONE:       { p->fxn(p->opq, ";"); break; }
        case BC_MAC_AHEAD:      { p->fxn(p->opq, "A"); break; }
        case BC_MAC_BLANK:      { p->fxn(p->opq, "B"); break; }
        case BC_MAC_RUN:        { p->fxn(p->opq, "R"); break; }
        case BC_MAC_VIEW:       { p->fxn(p->opq, "V"); break; }

        case BC_MAC_REG:
        {
            p->fxn(p->opq, "%d $%.4X", mac->arg.reg.reg, mac->arg.reg.value);
            break;
        }

        case BC_MAC_INSPECT:
        {
            p->fxn(p->opq, "I $%.4X", mac->arg.inspect.addr);
            break;
        }

        case BC_MAC_POKE:
        {
            p->fxn(p->opq, "P $%.4X $%.4X",
                    mac->arg.poke.addr, mac->arg.poke.value);
            break;
        }

        case BC_MAC_LOAD:
        {
            p->fxn(p->opq, "L %s %d $%.4X",
                    cfg_quote_str( mac->arg.load.name ),
                    mac->arg.load.width,
                    mac->arg.load.addr);
            break;
        }

        case BC_MAC_RUNTO:
        {
            p->fxn(p->opq, "O $%.4X", mac->arg.runto.addr);
            break;
        }

        case BC_MAC_TRACE:
        {
            p->fxn(p->opq, "T $%.4X", mac->arg.runto.addr);
            break;
        }

        case BC_MAC_WATCH:
        {
            int i, lo, hi;

            p->fxn(p->opq, "W %s ", mac->arg.watch.name);
            for (i = 0; i < mac->arg.watch.spans; i++)
            {
                if (i)
                    p->fxn(p->opq, ",");

                lo = mac->arg.watch.addr[2*i + 0];
                hi = mac->arg.watch.addr[2*i + 1];

                if (lo == hi) p->fxn(p->opq, "$%.4X", lo);
                else          p->fxn(p->opq, "$%.4X-$%.4X", lo, hi);
            }
            break;
        }

        case BC_MAC_ERROR:
        default:
        {
            p->fxn(p->opq, "; unknown macro\015\012");
            break;
        }
    }

    p->fxn(p->opq, "\015\012");
}

/* ======================================================================== */
/*  BC_PRINT_MACRO   -- Print the [macro] section                           */
/* ======================================================================== */
void bc_print_macro(printer_t  *RESTRICT const p,
                    bc_macro_t *RESTRICT const mac)
{
    /* -------------------------------------------------------------------- */
    /*  Step through the macro list, regenerating each macro.               */
    /* -------------------------------------------------------------------- */
    p->fxn(p->opq, "\015\012[macro]\015\012");
    LL_ACTON(mac, bc_print_macro_t, (void *)p);
}

/* ======================================================================== */
/*  BC_PRINT_VARLIKE -- Print [var],[keys],[joystick] sections              */
/* ======================================================================== */
void bc_print_varlike
(
    printer_t   *RESTRICT const p,
    cfg_var_t   *RESTRICT const varlike,
    const char  *RESTRICT const sectname
)
{
    /* -------------------------------------------------------------------- */
    /*  Real easy:  Just step thru the list and print all the tuples.       */
    /* -------------------------------------------------------------------- */
    p->fxn(p->opq, "\015\012%s\015\012", sectname);
    print_cfg_var_list( varlike, p );
}

/* ======================================================================== */
/*  BC_PRINT_MEMSPAN -- Print out all the memory span information.          */
/* ======================================================================== */
void bc_print_memspan
(
    printer_t           *RESTRICT const p,
    const bc_memspan_t  *RESTRICT const mem
)
{
    const bc_memspan_t *RESTRICT m;
    int need_hdr;

    /* -------------------------------------------------------------------- */
    /*  First print out a 'raw' list as a comment.                          */
    /* -------------------------------------------------------------------- */
    m = mem;
    while (m)
    {
        p->fxn(p->opq,
           "; $%.4X - $%.4X => $%.4X - $%.4X PAGE %X "
           "FLAGS %c%c%c%c%c%c%c from \"%s\"\015\012",
           m->s_fofs, m->e_fofs, m->s_addr, m->e_addr, m->epage,
           m->flags & BC_SPAN_R  ? 'R' : '-',
           m->flags & BC_SPAN_W  ? 'W' : '-',
           m->flags & BC_SPAN_N  ? 'N' : '-',
           m->flags & BC_SPAN_B  ? 'B' : '-',
           m->flags & BC_SPAN_PL ? 'L' : '-',
           m->flags & BC_SPAN_PK ? 'K' : '-',
           m->flags & BC_SPAN_EP ? 'E' : '-',
           m->f_name ? m->f_name : "(primary)"
       );

        m = (bc_memspan_t *)m->l.next;
    }

    /* -------------------------------------------------------------------- */
    /*  Next, try to print it out as CFG sections.  The CFG format is       */
    /*  kinda odd, but since we just parsed the config, it should be easy.  */
    /*                                                                      */
    /*  The following truth table indicates how the attributes map back     */
    /*  to different sections.                                              */
    /*                                                                      */
    /*  R W N B PL PK EP   Section       Format                             */
    /*  R - x x PL -  -    [mapping]     $xxxx - $xxxx = $xxxx              */
    /*  - W x x PL -  -    [mapping]     $xxxx - $xxxx = $xxxx WOM w        */
    /*  R W x x PL -  -    [mapping]     $xxxx - $xxxx = $xxxx RAM w        */
    /*  R - x x PL -  EP   [mapping]     $xxxx - $xxxx = $xxxx PAGE p       */
    /*  - W x x PL -  EP   [mapping]     $xxxx - $xxxx = $xxxx PAGE p WOM w */
    /*  R W x x PL -  EP   [mapping]     $xxxx - $xxxx = $xxxx PAGE p RAM w */
    /*  - - x x PL -  -    [preload]     $xxxx - $xxxx = $xxxx              */
    /*  x x x B x  -  x    [bankswitch]  $xxxx - $xxxx                      */
    /*  R - x x -  -  -    [memattr]     $xxxx - $xxxx = ROM d              */
    /*  - W x x -  -  -    [memattr]     $xxxx - $xxxx = WOM d              */
    /*  R W x x -  -  -    [memattr]     $xxxx - $xxxx = RAM d              */
    /*  R - x x -  -  EP   [memattr]     $xxxx - $xxxx = PAGE p ROM d       */
    /*  - W x x -  -  EP   [memattr]     $xxxx - $xxxx = PAGE p WOM d       */
    /*  R W x x -  -  EP   [memattr]     $xxxx - $xxxx = PAGE p RAM d       */
    /* -------------------------------------------------------------------- */


    /* -------------------------------------------------------------------- */
    /*  Get all the [mapping] sections.                                     */
    /* -------------------------------------------------------------------- */
    need_hdr = 1;
    for (m = mem; m; m = (bc_memspan_t *)m->l.next)
    {
        if (
            !(
                (m->flags & BC_SPAN_PL) &&
                (m->flags & (BC_SPAN_R | BC_SPAN_W))
            ) ||
            (m->flags & BC_SPAN_PK) ||
            (m->f_name != NULL))
            continue;

        if (need_hdr)
        {
            p->fxn(p->opq, "\015\012[mapping]\015\012");
            need_hdr = 0;
        }

        p->fxn(p->opq, "$%.4X - $%.4X = $%.4X",
               m->s_fofs, m->e_fofs, m->s_addr);

        if (m->flags & BC_SPAN_EP)
            p->fxn(p->opq, " PAGE %X", m->epage);

        switch (m->flags & (BC_SPAN_R | BC_SPAN_W))
        {
            case BC_SPAN_ROM:
                if ( m->width != 16 )
                    p->fxn(p->opq, " ROM %d", m->width); /* Weird... */
                break;
            case BC_SPAN_WOM:
                p->fxn(p->opq, " WOM %d", m->width);
                break;
            case BC_SPAN_RAM:
                p->fxn(p->opq, " RAM %d", m->width);
                break;
            default:
                p->fxn(p->opq, " ; unknown!");
                break;
        }
        p->fxn(p->opq, "\015\012");
    }

    /* -------------------------------------------------------------------- */
    /*  Get all the [preload] sections.                                     */
    /* -------------------------------------------------------------------- */
    need_hdr = 1;
    for (m = mem; m; m = (bc_memspan_t *)m->l.next)
    {
        if ((m->flags & (BC_SPAN_R|BC_SPAN_W|BC_SPAN_PL)) != (BC_SPAN_PL) ||
            (m->flags & BC_SPAN_PK) ||
            (m->f_name != NULL))
            continue;

        if (need_hdr)
        {
            p->fxn(p->opq, "\015\012[preload]\015\012");
            need_hdr = 0;
        }

        if (m->flags & BC_SPAN_EP)
            p->fxn(p->opq, "$%.4X - $%.4X = $%.4X PAGE %X\015\012",
                   m->s_fofs, m->e_fofs, m->s_addr, m->epage);
        else
            p->fxn(p->opq, "$%.4X - $%.4X = $%.4X\015\012",
                   m->s_fofs, m->e_fofs, m->s_addr);
    }

    /* -------------------------------------------------------------------- */
    /*  Get all the [bankswitch] sections.                                  */
    /* -------------------------------------------------------------------- */
    need_hdr = 1;
    for (m = mem; m; m = (bc_memspan_t *)m->l.next)
    {
        if ((m->flags & (BC_SPAN_B)) != (BC_SPAN_B) ||
            (m->flags & BC_SPAN_PK))
            continue;

        if (need_hdr)
        {
            p->fxn(p->opq, "\015\012[bankswitch]\015\012");
            need_hdr = 0;
        }

        p->fxn(p->opq, "$%.4X - $%.4X\015\012", m->s_addr, m->e_addr);
    }


    /* -------------------------------------------------------------------- */
    /*  Get all the [memattr] sections.                                     */
    /* -------------------------------------------------------------------- */
    need_hdr = 1;
    for (m = mem; m; m = (bc_memspan_t *)m->l.next)
    {
        if ((m->flags & (BC_SPAN_R | BC_SPAN_W)) == 0 ||
            (m->flags & BC_SPAN_PL) ||
            (m->flags & BC_SPAN_PK))
            continue;

        if (need_hdr)
        {
            p->fxn(p->opq, "\015\012[memattr]\015\012");
            need_hdr = 0;
        }

        p->fxn(p->opq, "$%.4X - $%.4X =", m->s_addr, m->e_addr);

        if (m->flags & BC_SPAN_EP)
            p->fxn(p->opq, " PAGE %X", m->epage);

        switch (m->flags & (BC_SPAN_R | BC_SPAN_W))
        {
            case BC_SPAN_ROM:
                p->fxn(p->opq, " ROM %d", m->width);
                break;
            case BC_SPAN_WOM:
                p->fxn(p->opq, " WOM %d", m->width);
                break;
            case BC_SPAN_RAM:
                p->fxn(p->opq, " RAM %d", m->width);
                break;
            default:
                p->fxn(p->opq, " ; unknown!");
                break;
        }
        p->fxn(p->opq, "\015\012");
    }
}

/* ======================================================================== */
/*  BC_PRINT_CFG -- Chase through a bc_cfgfile_t structure and print out    */
/*                  what we find therein.                                   */
/* ======================================================================== */
void bc_print_cfg
(
    printer_t           *RESTRICT const p,
    const bc_cfgfile_t  *RESTRICT const bc
)
{
    if (bc->diags)      bc_print_diag   (p, bc->cfgfile,  bc->diags, 1   );
    if (bc->span)       bc_print_memspan(p, bc->span                     );
    if (bc->macro)      bc_print_macro  (p, bc->macro                    );
    if (bc->vars)       bc_print_varlike(p, bc->vars,     "[vars]"       );
    if (bc->keys[0])    bc_print_varlike(p, bc->keys[0],  "[keys]"       );
    if (bc->keys[1])    bc_print_varlike(p, bc->keys[1],  "[capslock]"   );
    if (bc->keys[2])    bc_print_varlike(p, bc->keys[2],  "[numlock]"    );
    if (bc->keys[3])    bc_print_varlike(p, bc->keys[3],  "[scrolllock]" );
    if (bc->joystick)   bc_print_varlike(p, bc->joystick, "[joystick]"   );
}

#endif /* BC_NOPRINT */


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
