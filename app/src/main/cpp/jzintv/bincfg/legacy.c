/* ======================================================================== */
/*  Title:    Legacy INTVPC BIN+CFG support                                 */
/*  Author:   J. Zbiciak, J. Tanner                                         */
/* ------------------------------------------------------------------------ */
/*  This module implements a memory peripheral with the semantics of        */
/*  INTVPC's BIN+CFG file, at least for the most part.  ECS paged ROM is    */
/*  supported indirectly by instantiating Paged ROMs from mem/mem.c.        */
/*                                                                          */
/*  The routines for reading BIN+CFG files are in bincfg/bincfg.h, not      */
/*  this file.                                                              */
/* ======================================================================== */


#include "config.h"
#include "periph/periph.h"
#include "mem/mem.h"
#include "metadata/metadata.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "cp1600/cp1600.h"
#include "bincfg.h"
#include "legacy.h"

extern int jlp_accel_on;

/* ======================================================================== */
/*  LEGACY_READ -- read from a legacy BIN+CFG.                              */
/* ======================================================================== */
uint32_t legacy_read (periph_t *per, periph_t *ign,
                      uint32_t addr, uint32_t data)
{
    legacy_t *const l = PERIPH_AS(legacy_t, per);
    uint32_t mask;

    UNUSED(ign);
    UNUSED(data);

    // Disallow reads to flat memory if JLP RAM / accelerators are on.
    if (jlp_accel_on && addr >= 0x8000 && addr <= 0x9FFF)
        return ~0U;

    // Disallow reads if range isn't marked readable.
    if ((l->loc[addr].flags & BC_SPAN_R) == 0)
        return ~0U;

    // Perform the read
    mask = ~(~0u << l->loc[addr].width);
    return l->loc[addr].data & mask;
}

/* ======================================================================== */
/*  LEGACY_WRITE -- write to a legacy BIN+CFG.                              */
/* ======================================================================== */
void  legacy_write(periph_t *per, periph_t *ign, uint32_t addr, uint32_t data)
{
    legacy_t *const l = PERIPH_AS(legacy_t, per);
    uint32_t mask;

    UNUSED(ign);
    UNUSED(data);

    // Disallow writes to flat memory if JLP RAM / accelerators are on.
    if (jlp_accel_on && addr >= 0x8000 && addr < 0x9FFF) return;

    // Disallow writes if range isn't marked writeable
    if ((l->loc[addr].flags & BC_SPAN_W) == 0)
        return;

    // Perform the write
    mask = ~(~0u << l->loc[addr].width);
    l->loc[addr].data = data & mask;
}

/* ======================================================================== */
/*  LEGACY_POKE  -- write to a legacy BIN+CFG, ignoring read-only status.   */
/* ======================================================================== */
void legacy_poke (periph_t *per, periph_t *ign, uint32_t addr, uint32_t data)
{
    legacy_t *const l = PERIPH_AS(legacy_t, per);
    uint32_t mask;

    UNUSED(ign);
    UNUSED(data);
    if (jlp_accel_on && addr >= 0x8000 && addr <= 0x9FFF)
        return;

    mask = ~(~0u << l->loc[addr].width);
    l->loc[addr].data = data & mask;
}


/* ======================================================================== */
/*  LEGACY_APPLY_BINCFG -- Populates a legacy_t from a BIN+CFG.             */
/* ======================================================================== */
LOCAL int legacy_apply_bincfg(bc_cfgfile_t *bc, legacy_t *l, void *cpu,
                              int flag_jlp_accel, int flag_jlp_flash,
                              int rand_mem)
{
    uint32_t addr, ofs;
    bc_memspan_t *span;
    int num_ecs = 0;

    l->pg_rom = NULL;
    l->bc     = bc;
    l->loc    = CALLOC(struct legacy_loc_t, 65536);
    if (!l->loc)
        return -1;

    /* -------------------------------------------------------------------- */
    /*  If bincfg has no metadata, grab some defaults.                      */
    /* -------------------------------------------------------------------- */
    if (!bc->metadata)
        bc->metadata = default_game_metadata();

    /* -------------------------------------------------------------------- */
    /*  Reconcile the JLP flags with the bincfg metadata.                   */
    /* -------------------------------------------------------------------- */
    if (flag_jlp_accel != -1)
        bc->metadata->jlp_accel = (jlp_accel_t)flag_jlp_accel;

    if (flag_jlp_flash != -1)
        bc->metadata->jlp_flash = flag_jlp_flash;

    l->jlp_accel = bc->metadata->jlp_accel;

    /* -------------------------------------------------------------------- */
    /*  Traverse the memspan list, registering memory segments in legacy_t  */
    /* -------------------------------------------------------------------- */
    for (span = bc->span; span; span = (bc_memspan_t *)span->l.next)
    {
        /* ---------------------------------------------------------------- */
        /*  If this span has an ECS page, skip it for now.  We'll come      */
        /*  back to it in a minute.                                         */
        /* ---------------------------------------------------------------- */
        if (span->epage != BC_SPAN_NOPAGE || (span->flags & BC_SPAN_EP) != 0)
        {
            if ((span->flags & BC_SPAN_RAM) != BC_SPAN_WOM && span->epage < 16)
                num_ecs++;
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Sanity check addresses.                                         */
        /* ---------------------------------------------------------------- */
        if (span->s_addr > 0xFFFF || span->e_addr > 0xFFFF)
        {
            fprintf(stderr, "Address span %.8X-%.8X in CFG is illegal\n",
                    span->s_addr, span->e_addr);
            exit(1);
        }

        /* ---------------------------------------------------------------- */
        /*  Assertion:  If PRELOAD, then span->data.                        */
        /* ---------------------------------------------------------------- */
        assert(((span->flags & BC_SPAN_PL) == 0) || (span->data != 0));

        /* ---------------------------------------------------------------- */
        /*  Add the segment to the legacy_t.                                */
        /* ---------------------------------------------------------------- */
        for (addr = span->s_addr, ofs = 0; addr <= span->e_addr; addr++, ofs++)
        {
            l->loc[addr].flags |= span->flags;

            if (span->data)
                l->loc[addr].data  = span->data[ofs];
            else if (rand_mem && (span->flags & BC_SPAN_W))
                l->loc[addr].data  = rand_jz();

            if ((span->flags & BC_SPAN_PK) == 0)
                l->loc[addr].width = span->width;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Now allocate ECS Paged ROMs/RAMs/WOMs as needed.                    */
    /* -------------------------------------------------------------------- */
    if (num_ecs > 0)
    {
        int pg_flags[16][16] = {{0}};
        int pg_width[16][16] = {{0}};
        uint16_t *const all_data = CALLOC(uint16_t, 4096 * 16 * 16);
        bool *const all_init = CALLOC(bool, 4096 * 16 * 16);

        if (!all_data || !all_init)
        {
            fprintf(stderr, "legacy_bincfg_t: Out of memory\n");
            exit(1);
        }

        /* ---------------------------------------------------------------- */
        /*  Make page number the major index, so consecutive chapters in    */
        /*  the same page are consecutive in memory.                        */
        /* ---------------------------------------------------------------- */
        uint16_t *pg_data[16];
        bool *pg_init[16];
        for (int i = 0; i < 16; ++i)
        {
            pg_data[i] = all_data + i * 4096 * 16;
            pg_init[i] = all_init + i * 4096 * 16;
        }

        /* ---------------------------------------------------------------- */
        /*  Step over all the spans, copying them into pg_data.  Track      */
        /*  which words get initialized.  As a post-pass, we will fill the  */
        /*  uninitialized locations with either 0xFFFF or random, based on  */
        /*  whether it's ROM or RAM, and whether RAM randomization is on.   */
        /* ---------------------------------------------------------------- */
        for (span = bc->span; span; span = (bc_memspan_t *)span->l.next)
        {
            /* ------------------------------------------------------------ */
            /*  Skip spans that are not ECS pages or are paged WOM.         */
            /* ------------------------------------------------------------ */
            if ( span->epage == BC_SPAN_NOPAGE ||
                (span->flags & BC_SPAN_EP) == 0 ||
                (span->flags & BC_SPAN_RAM) == BC_SPAN_WOM )
            {
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  Ignore out-of-range pages, and print a warning.             */
            /* ------------------------------------------------------------ */
            const int page = span->epage;
            if (page >= 16)
            {
                jzp_printf(
                    "legacy_bincfg_t: Ignoring span $%.0X - $%.0X PAGE $%X."
                    "  Page out of range.\n",
                    span->s_addr, span->e_addr, span->epage);
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  Skip out-of-range starting address, and truncate out of     */
            /*  range ending addresses.                                     */
            /* ------------------------------------------------------------ */
            if (span->s_addr > 0xFFFF)
            {
                jzp_printf(
                    "legacy_bincfg_t: Ignoring span $%.0X - $%.0X PAGE $%X."
                    "  Addresses of range.\n",
                    span->s_addr, span->e_addr, span->epage);
                continue;
            }
            const uint32_t s_addr = span->s_addr;
            const uint32_t e_addr =
                span->e_addr > 0xFFFF ? 0xFFFF : span->e_addr;
            if (e_addr != span->e_addr)
            {
                jzp_printf(
                    "legacy_bincfg_t: Span $%.0X - $%.0X PAGE $%X:"
                    "  Truncating span to $%.0X - $FFFF.\n",
                    span->s_addr, span->e_addr, span->epage, span->s_addr);
            }

            /* ------------------------------------------------------------ */
            /*  Merge flags, and copy in any init data.                     */
            /* ------------------------------------------------------------ */
            const uint32_t s_chap = s_addr >> 12;
            const uint32_t e_chap = e_addr >> 12;
            for (uint32_t chap = s_chap; chap <= e_chap; chap++)
            {
                const int s_flags = span->flags & BC_SPAN_RAM;
                const int s_width = span->width;

                /* -------------------------------------------------------- */
                /*  Merge flags for overlapping ranges together, and warn   */
                /*  when we upgrade ROM to RAM.  Don't warn if we merge     */
                /*  ROM into an existing RAM as that's likely a macro poke. */
                /* -------------------------------------------------------- */
                if (pg_flags[page][chap] == BC_SPAN_ROM &&
                    s_flags == BC_SPAN_RAM)
                {
                    jzp_printf(
                        "legacy_bincfg_t: Upgrading $%X000 - $%XFFF PAGE $%X "
                        "from ROM to RAM.", chap, chap, page);
                }
                pg_flags[page][chap] |= s_flags;

                /* -------------------------------------------------------- */
                /*  Don't override original width.  Most likely cause is a  */
                /*  macro poke into narrow RAM, which is decidedly rare.    */
                /* -------------------------------------------------------- */
                if (pg_width[page][chap] && pg_width[page][chap] != s_width)
                {
                    jzp_printf(
                        "legacy_bincfg_t: Warning, $%X000 - $%XFFF PAGE $%X "
                        "Width mismatch: %d and %d.\n"
                        "                 Keeping original width %d.\n",
                        chap, chap, page, pg_width[page][chap], s_width);
                } else
                {
                    pg_width[page][chap] = s_width;
                }
            }

            if (span->data)
            {
                for (addr = s_addr; addr <= e_addr; ++addr)
                    pg_data[page][addr] = span->data[addr - s_addr];

                for (addr = s_addr; addr <= e_addr; ++addr)
                    pg_init[page][addr] = true;

                /* Since we've claimed span->data, we can free it now. */
                free(span->data);
                span->data = NULL;
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Now post-pass, filling in 0xFFFF, 0x0000, or random.  Also      */
        /*  count how many actual Mattel-style pages we actually have.      */
        /* ---------------------------------------------------------------- */
        num_ecs = 0;
        for (int page = 0; page < 16; ++page)
        {
            for (int chap = 0; chap < 16; ++chap)
            {
                if (!pg_flags[page][chap])
                    continue;

                num_ecs++;

                const uint32_t s_addr = chap << 12;
                const uint32_t e_addr = s_addr | 0x0FFF;
                if (pg_flags[page][chap] == BC_SPAN_ROM)
                {
                    for (addr = s_addr; addr <= e_addr; ++addr)
                        if (!pg_init[page][addr])
                            pg_data[page][addr] = 0xFFFF;
                } else if (!rand_mem)
                {
                    for (addr = s_addr; addr <= e_addr; ++addr)
                        if (!pg_init[page][addr])
                            pg_data[page][addr] = 0x0000;
                } else
                {
                    for (addr = s_addr; addr <= e_addr; ++addr)
                        if (!pg_init[page][addr])
                            pg_data[page][addr] = rand_jz();
                }
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Finally, allocate storage for paged memories and populate.      */
        /* ---------------------------------------------------------------- */
        l->pg_rom = CALLOC(mem_t, num_ecs);
        l->npg_rom = num_ecs;

        num_ecs = 0;
        bool failed = false;
        for (int page = 0; page < 16 && !failed; ++page)
        {
            for (int chap = 0; chap < 16 && !failed; ++chap)
            {
                if (!pg_flags[page][chap])
                    continue;

                const uint32_t s_addr = chap << 12;
                const int s_width = pg_width[page][chap];
                mem_t *const mem = l->pg_rom + num_ecs;
                uint16_t *image = CALLOC(uint16_t, 0x1000);
                if (!image)
                {
                    failed = true;
                    break;
                }
                memcpy(image, pg_data[page] + chap * 0x1000,
                       0x1000 * sizeof(uint16_t));
                if (pg_flags[page][chap] == BC_SPAN_ROM)
                {
                    if ( mem_make_prom( mem, s_width, s_addr, 12,
                                        page, image, cpu ) )
                        failed = true;
                } else
                {
                    if ( mem_make_pram( mem, s_width, s_addr, 12,
                                        page, image, cpu ) )
                        failed = true;
                }
                if (!failed)
                    num_ecs++;
            }
        }

        if (failed)
        {
            free(all_data);
            free(all_init);
            goto fail;
        }
    }


#if 0
    if (num_ecs > 0)
    {
        num_ecs = 0;

        l->pg_rom = CALLOC(mem_t, 256); /* theoretical maximum */

        for (span = bc->span; span; span = (bc_memspan_t *)span->l.next)
        {
            uint32_t new_s_addr, new_e_addr, new_slen, old_slen;
            uint32_t page_base, page_ofs;
            uint16_t *new_data;

            /* ------------------------------------------------------------ */
            /*  Skip spans that are not ECS pages.                          */
            /* ------------------------------------------------------------ */
            if ( span->epage == BC_SPAN_NOPAGE ||
                (span->flags & BC_SPAN_EP) == 0 )
            {
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  Skip spans that are write-only.                             */
            /* ------------------------------------------------------------ */
            if ( (span->flags & BC_SPAN_RAM) == BC_SPAN_WOM )
                continue;

            /* ------------------------------------------------------------ */
            /*  Assertions:  If ECS Paged, then page >= 0.                  */
            /* ------------------------------------------------------------ */
            assert(span->epage != BC_SPAN_NOPAGE);

            /* ------------------------------------------------------------ */
            /*  Realign the span to 4K boundaries and cut it up into 4K     */
            /*  chunks.                                                     */
            /* ------------------------------------------------------------ */
            new_s_addr = span->s_addr & 0xF000;
            new_e_addr = span->e_addr | 0x0FFF;
            new_slen   = new_e_addr   - new_s_addr + 1;
            old_slen   = span->e_addr - span->s_addr + 1;

            new_data   = CALLOC(uint16_t, new_slen);
            memset( (void *)new_data, 0xFF, new_slen * sizeof(uint16_t) );

            if ( span->data )
            {
                memcpy( new_data + span->s_addr - new_s_addr,
                        span->data, old_slen * sizeof(uint16_t) );
            } else
            {
                for ( addr = span->s_addr; addr <= span->e_addr; addr++ )
                    new_data[addr - span->s_addr] = rand_mem ? rand_jz() : 0;
            }

            /* ------------------------------------------------------------ */
            /*  Make a paged ROM out of each 4K segment.                    */
            /* ------------------------------------------------------------ */
            for ( page_ofs = 0,       page_base = new_s_addr;
                                      page_base < new_e_addr;
                  page_ofs += 0x1000, page_base += 0x1000 )
            {
                uint16_t *image = CALLOC(uint16_t, 0x1000);
                memcpy( image, new_data + page_ofs, 0x1000 * sizeof(uint16_t) );
                mem_t *mem = l->pg_rom + num_ecs;

                if ( (span->flags & BC_SPAN_RAM) == BC_SPAN_ROM )
                {
                    if ( mem_make_prom( mem, span->width, page_base, 12,
                                        span->epage, image, cpu ) )
                    {
                        free( new_data );
                        goto fail;
                    }
                } else
                {
                    if ( mem_make_pram( mem, span->width, page_base, 12,
                                        span->epage, image, cpu ) )
                    {
                        free( new_data );
                        goto fail;
                    }
                }

                if ( l->jlp_accel > 0 &&
                    (page_base == 0x8000 || page_base == 0x9000) )
                    l->pg_rom[num_ecs].chk_jlp = 1;

                num_ecs++;
            }

            /* ------------------------------------------------------------ */
            /*  Since we claimed span->data, null it out.                   */
            /* ------------------------------------------------------------ */
            free( new_data );
            CONDFREE( span->data );
            span->data = NULL;

        }
    }
    l->npg_rom = num_ecs;
#endif

    return 0;

fail:
    {
        int i;

        if (l->pg_rom)
            for (i = 0; i < num_ecs; i++)
                CONDFREE(l->pg_rom[num_ecs].image);

        CONDFREE(l->loc);
        CONDFREE(l->pg_rom);
    }
    return -1;
}

/* ======================================================================== */
/*  LEGACY_READ_BINCFG -- Reads a .BIN and optional .CFG file.              */
/* ======================================================================== */
LOCAL int legacy_read_bincfg(const char *bin_fn, const char *cfg_fn,
                             legacy_t *l, void *cpu, int jlp_accel,
                             int jlp_flash, int rand_mem)
{
    LZFILE *fc;
    bc_cfgfile_t *bc;

    /* -------------------------------------------------------------------- */
    /*  Read the .CFG file.  This process  open it, then parse it.          */
    /*  Otherwise, we skip it -- lack of .CFG file is non-fatal.            */
    /* -------------------------------------------------------------------- */
    if (cfg_fn && (fc = lzoe_fopen(cfg_fn, "r")) != NULL)
    {
        bc = bc_parse_cfg(fc, bin_fn, cfg_fn);
        lzoe_fclose(fc);
    } else
    {
        bc = bc_parse_cfg(NULL, bin_fn, NULL);
    }
    if (!bc || !bc->span)
    {
        if (cfg_fn)
            fprintf(stderr,
                   "Could not generate configuration from this BIN+CFG:\n"
                   "  \"%s\"\n  \"%s\"\n", bin_fn, cfg_fn);
        else
            fprintf(stderr,
                   "Could not generate configuration from this BIN:\n"
                   "  \"%s\"\n", bin_fn);

        return -1;
    }

#ifndef BC_NODOMACRO
    /* -------------------------------------------------------------------- */
    /*  Apply any statically safe macros.  Ignore errors.                   */
    /* -------------------------------------------------------------------- */
    bc_do_macros(bc, 0);
#endif

    /* -------------------------------------------------------------------- */
    /*  Populate the config with corresponding BIN data.                    */
    /* -------------------------------------------------------------------- */
    if (bc_read_data(bc))
    {
        fprintf(stderr, "Error reading data for CFG file.\n");
        goto err;
    }

    /* -------------------------------------------------------------------- */
    /*  Apply the configuration.  This generates the emulation image.       */
    /* -------------------------------------------------------------------- */
    if (legacy_apply_bincfg(bc, l, cpu, jlp_accel, jlp_flash, rand_mem))
    {
        fprintf(stderr, "Error applying CFG file\n");
        goto err;
    }

    return 0;

err:
#ifndef BC_NOFREE
    /* -------------------------------------------------------------------- */
    /*  Discard the parsed config.                                          */
    /* -------------------------------------------------------------------- */
    bc_free_cfg(bc);
    l->bc = NULL;
#endif
    return -1;

}

/* ======================================================================== */
/*  LEGACY_PRINT_LOADING -- Print what files it's trying to load.           */
/* ======================================================================== */
LOCAL void legacy_print_loading(char *f1, char *f2)
{
    jzp_printf("Loading:\n");
    if (f1 && file_exists(f1)) jzp_printf("  %s\n", f1);
    if (f2 && file_exists(f2)) jzp_printf("  %s\n", f2);
}

/* ======================================================================== */
/*  LEGACY_DTOR   -- Tear down a legacy_t, freeing its resources.           */
/* ======================================================================== */
LOCAL void legacy_dtor(periph_t *p)
{
    legacy_t *const l = PERIPH_AS(legacy_t, p);

    int i;

    if (l->pg_rom)
        for (i = 0; i < l->npg_rom; i++)
            CONDFREE(l->pg_rom[i].image);

    CONDFREE(l->pg_rom);
    CONDFREE(l->loc);

    if (l->bc)
        bc_free_cfg(l->bc);
}

/* ======================================================================== */
/*  LEGACY_BINCFG -- Try to determine if a file is BIN+CFG or ROM, and      */
/*                   read it in if it is BIN+CFG.                           */
/*                                                                          */
/*  The return value from this function requires explanation.  If we        */
/*  figure out a .ROM file associated with this fname, we will a distinct   */
/*  char * that points to its filename.  If we determine the file is a      */
/*  BIN+CFG file pair, we will try to load it.  On success, we will return  */
/*  an exact copy of fname directly.  Otherwise, we will return NULL.       */
/* ======================================================================== */
char *legacy_bincfg
(
    legacy_t        *l,         /*  Legacy BIN+CFG structure            */
    path_t          *path,      /*  Search path for games.              */
    const char      *fname_k,   /*  Basename to use for CFG/BIN         */
    int             *legacy_rom,
    void            *cpu,
    int             jlp_accel,
    int             jlp_flash,
    int             rand_mem
)
{
    char *rom1_fn = NULL, *bin1_fn = NULL, *cfg1_fn = NULL;
    char *rom2_fn = NULL, *bin2_fn = NULL, *cfg2_fn = NULL, *int2_fn = NULL;
    char *rom3_fn = NULL;
    char *itv2_fn = NULL;
    char *p_rom1_fn = NULL, *p_bin1_fn = NULL, *p_cfg1_fn = NULL;
    char *p_rom2_fn = NULL, *p_bin2_fn = NULL, *p_cfg2_fn = NULL;
    char *p_rom3_fn = NULL;
    char *p_itv2_fn = NULL, *p_int2_fn = NULL;
    char *fname = strdup(fname_k);
    int name_len = strlen(fname);
    char *ext = strrchr(fname, '.');  /* look for an extension */
    path_t dummy, *tmp;
    char *fn;
    int max_p_len = 1;
    char sep[2] = { PATH_SEP, 0 };

    *legacy_rom = 0;

    /* -------------------------------------------------------------------- */
    /*  Silly case:  If the filename ends in a '.', strip it off.           */
    /* -------------------------------------------------------------------- */
    if (ext && ext[1] == '\0')
    {
        *ext = '\0';
        ext = NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Is it .ROM or .CC3?                                                 */
    /* -------------------------------------------------------------------- */
    if (ext && (stricmp(ext, ".rom")==0 ||
                stricmp(ext, ".cc3")==0))
    {
        rom1_fn = strdup(fname);
    }
    /* -------------------------------------------------------------------- */
    /*  Is it .BIN or .INT or .ITV?                                         */
    /* -------------------------------------------------------------------- */
    else if (ext && (stricmp(ext, ".bin")==0 ||
                     stricmp(ext, ".int")==0 ||
                     stricmp(ext, ".itv")==0) )
    {
        char *s;

        bin1_fn = strdup(fname);
        cfg1_fn = strdup(fname);
        if (!cfg1_fn)
        {
            fprintf(stderr, "legacy_bincfg: Out of memory\n");
            exit(1);
        }

        s = cfg1_fn + name_len - 4;
        strcpy(s, ".cfg");
    }
    /* -------------------------------------------------------------------- */
    /*  In case those fail, have a backup plan.                             */
    /*                                                                      */
    /*  For instance, on the Mac version of "Intellivision Lives!", the     */
    /*  ROM images are equivalent to a .BIN, but they have no extension.    */
    /*  They do, occasionally, have a corresponding .CFG file, with ".cfg"  */
    /*  as the extension.  (Pointed out by JJT.)                            */
    /* -------------------------------------------------------------------- */
    else
    {
        /* ---------------------------------------------------------------- */
        /*  Handle the no-extension-but-matching-cfg case.                  */
        /* ---------------------------------------------------------------- */
        if (!ext)
        {
            bin1_fn = strdup(fname);
            cfg1_fn = CALLOC(char, name_len+5);
            if (!cfg1_fn)
            {
                fprintf(stderr, "legacy_bincfg: Out of memory\n");
                exit(1);
            }
            snprintf(cfg1_fn, name_len + 5, "%s.cfg", fname);
        }

        /* ---------------------------------------------------------------- */
        /*  Also search for all known extensions appended to the file name. */
        /*  This lets users type "jzintv foo" and it'll find "foo.rom" or   */
        /*  "foo.bin" or whatever.                                          */
        /* ---------------------------------------------------------------- */
        bin2_fn = CALLOC(char, 6*(name_len + 5));
        if (!bin2_fn)
        {
            fprintf(stderr, "legacy_bincfg: Out of memory\n");
            exit(1);
        }
        cfg2_fn = bin2_fn + name_len + 5;
        rom2_fn = cfg2_fn + name_len + 5;
        int2_fn = rom2_fn + name_len + 5;
        itv2_fn = int2_fn + name_len + 5;
        rom3_fn = itv2_fn + name_len + 5;

        snprintf(bin2_fn, name_len + 5, "%s.bin", fname);
        snprintf(cfg2_fn, name_len + 5, "%s.cfg", fname);
        snprintf(rom2_fn, name_len + 5, "%s.rom", fname);
        snprintf(int2_fn, name_len + 5, "%s.int", fname);
        snprintf(itv2_fn, name_len + 5, "%s.itv", fname);
        snprintf(rom3_fn, name_len + 5, "%s.cc3", fname);
    }

    /* -------------------------------------------------------------------- */
    /*  Now try out all of our options in each directory of the path.       */
    /*  If the filename is an absolute path, short out the path search.     */
    /* -------------------------------------------------------------------- */
    if (!path)
    {
        path        = &dummy;
        dummy.p_len = 1;
        dummy.name  = ".";
        dummy.next  = NULL;
    }

    if (is_absolute_path(fname))
    {
        path        = &dummy;
        dummy.p_len = 0;
        dummy.name  = "";
        dummy.next  = NULL;
        sep[0]      = 0;
    }

    for (tmp = path; tmp; tmp = tmp->next)
        if (max_p_len < tmp->p_len)
            max_p_len = tmp->p_len;

    p_rom1_fn = CALLOC(char, 9*(max_p_len + name_len + 7));

    if (!p_rom1_fn)
    {
        fprintf(stderr, "legacy_bincfg: Out of memory\n");
        exit(1);
    }

    p_bin1_fn = p_rom1_fn + max_p_len + name_len + 7;
    p_cfg1_fn = p_bin1_fn + max_p_len + name_len + 7;
    p_rom2_fn = p_cfg1_fn + max_p_len + name_len + 7;
    p_bin2_fn = p_rom2_fn + max_p_len + name_len + 7;
    p_cfg2_fn = p_bin2_fn + max_p_len + name_len + 7;
    p_int2_fn = p_cfg2_fn + max_p_len + name_len + 7;
    p_itv2_fn = p_int2_fn + max_p_len + name_len + 7;
    p_rom3_fn = p_itv2_fn + max_p_len + name_len + 7;

    tmp = path;

    while (path)
    {
        /* ---------------------------------------------------------------- */
        /*  Generate path-qualified versions of the filenames.              */
        /* ---------------------------------------------------------------- */
#define N (max_p_len+name_len+7)
        if (rom1_fn) snprintf(p_rom1_fn, N,"%s%s%s", path->name, sep, rom1_fn);
        if (bin1_fn) snprintf(p_bin1_fn, N,"%s%s%s", path->name, sep, bin1_fn);
        if (cfg1_fn) snprintf(p_cfg1_fn, N,"%s%s%s", path->name, sep, cfg1_fn);
        if (rom2_fn) snprintf(p_rom2_fn, N,"%s%s%s", path->name, sep, rom2_fn);
        if (bin2_fn) snprintf(p_bin2_fn, N,"%s%s%s", path->name, sep, bin2_fn);
        if (cfg2_fn) snprintf(p_cfg2_fn, N,"%s%s%s", path->name, sep, cfg2_fn);
        if (int2_fn) snprintf(p_int2_fn, N,"%s%s%s", path->name, sep, int2_fn);
        if (itv2_fn) snprintf(p_itv2_fn, N,"%s%s%s", path->name, sep, itv2_fn);
        if (rom3_fn) snprintf(p_rom3_fn, N,"%s%s%s", path->name, sep, rom3_fn);

        /* ---------------------------------------------------------------- */
        /*  See if path-qualified instances of the files exist.             */
        /* ---------------------------------------------------------------- */
        if (rom1_fn && file_exists(p_rom1_fn))
        {
            legacy_print_loading(p_rom1_fn, NULL);
            fn = strdup(p_rom1_fn);

            free(fname);
            free(p_rom1_fn);
            if (rom1_fn) free(rom1_fn);
            if (bin1_fn) free(bin1_fn);
            if (cfg1_fn) free(cfg1_fn);
            if (bin2_fn) free(bin2_fn);  /* also frees cfg2/int2/itv2/rom2 */

            return fn;
        }

        if (bin1_fn && file_exists(p_bin1_fn))
        {
            legacy_print_loading(p_bin1_fn, p_cfg1_fn);
            fn = strdup(p_bin1_fn);

            if (legacy_read_bincfg(p_bin1_fn, p_cfg1_fn, l, cpu, jlp_accel,
                                   jlp_flash, rand_mem))
            {
                free(fname);
                free(p_rom1_fn);
                if (rom1_fn) free(rom1_fn);
                if (bin1_fn) free(bin1_fn);
                if (cfg1_fn) free(cfg1_fn);
                if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/
                return NULL;
            }

            goto finish;
        }

        if (rom2_fn && file_exists(p_rom2_fn))
        {
            legacy_print_loading(p_rom2_fn, NULL);
            fn = strdup(p_rom2_fn);

            free(fname);
            free(p_rom1_fn);
            if (rom1_fn) free(rom1_fn);
            if (bin1_fn) free(bin1_fn);
            if (cfg1_fn) free(cfg1_fn);
            if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/
            return fn;
        }

        if (rom3_fn && file_exists(p_rom3_fn))
        {
            legacy_print_loading(p_rom3_fn, NULL);
            fn = strdup(p_rom3_fn);

            free(fname);
            free(p_rom1_fn);
            if (rom1_fn) free(rom1_fn);
            if (bin1_fn) free(bin1_fn);
            if (cfg1_fn) free(cfg1_fn);
            if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/
            return fn;
        }

        if (bin2_fn && file_exists(p_bin2_fn))
        {
            legacy_print_loading(p_bin2_fn, p_cfg2_fn);
            fn = strdup(p_bin2_fn);

            if (legacy_read_bincfg(p_bin2_fn, p_cfg2_fn, l, cpu, jlp_accel,
                                   jlp_flash, rand_mem))
            {
                free(fname);
                free(p_rom1_fn);
                if (rom1_fn) free(rom1_fn);
                if (bin1_fn) free(bin1_fn);
                if (cfg1_fn) free(cfg1_fn);
                if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/
                return NULL;
            }

            goto finish;
        }

        if (int2_fn && file_exists(p_int2_fn))
        {
            legacy_print_loading(p_int2_fn, p_cfg2_fn);
            fn = strdup(p_int2_fn);

            if (legacy_read_bincfg(p_int2_fn, p_cfg2_fn, l, cpu, jlp_accel,
                                   jlp_flash, rand_mem))
            {
                free(fname);
                free(p_rom1_fn);
                if (rom1_fn) free(rom1_fn);
                if (bin1_fn) free(bin1_fn);
                if (cfg1_fn) free(cfg1_fn);
                if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/
                return NULL;
            }
            goto finish;
        }

        if (itv2_fn && file_exists(p_itv2_fn))
        {
            legacy_print_loading(p_itv2_fn, p_cfg2_fn);
            fn = strdup(p_itv2_fn);

            if (legacy_read_bincfg(p_itv2_fn, p_cfg2_fn, l, cpu, jlp_accel,
                                   jlp_flash, rand_mem))
            {
                free(fname);
                free(p_rom1_fn);
                if (rom1_fn) free(rom1_fn);
                if (bin1_fn) free(bin1_fn);
                if (cfg1_fn) free(cfg1_fn);
                if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/
                return NULL;
            }

            goto finish;
        }

        path = path->next;
    }

    fprintf(stderr, "\nCould not find any of these candidate files:\n");
    if (rom1_fn) fprintf(stderr, "  %s\n", rom1_fn);
    if (bin1_fn) fprintf(stderr, "  %s\n", bin1_fn);
    if (rom2_fn) fprintf(stderr, "  %s\n", rom2_fn);
    if (rom3_fn) fprintf(stderr, "  %s\n", rom3_fn);
    if (bin2_fn) fprintf(stderr, "  %s\n", bin2_fn);
    if (int2_fn) fprintf(stderr, "  %s\n", int2_fn);
    if (itv2_fn) fprintf(stderr, "  %s\n", itv2_fn);

    free(fname);
    free(p_rom1_fn);
    if (rom1_fn) free(rom1_fn);
    if (bin1_fn) free(bin1_fn);
    if (cfg1_fn) free(cfg1_fn);
    if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/

    dump_search_path(tmp);

    return NULL;

finish:
    free(fname);
    *legacy_rom = 1;
    free(p_rom1_fn);
    if (rom1_fn) free(rom1_fn);
    if (bin1_fn) free(bin1_fn);
    if (cfg1_fn) free(cfg1_fn);
    if (bin2_fn) free(bin2_fn); /*also frees cfg2/int2/itv2/rom2*/

    /* -------------------------------------------------------------------- */
    /*  Set up peripheral function pointers to support reads of the right   */
    /*  width.  Ignore writes and explicitly disallow ticks.                */
    /* -------------------------------------------------------------------- */
    l->periph.read        = legacy_read;
    l->periph.write       = legacy_write;
    l->periph.peek        = legacy_read;
    l->periph.poke        = legacy_poke;
    l->periph.dtor        = legacy_dtor;

    l->periph.tick        = NULL;
    l->periph.min_tick    = ~0U;
    l->periph.max_tick    = ~0U;
    l->periph.addr_base   = 0;
    l->periph.addr_mask   = 0xFFFF;

    return fn;
}

/* ======================================================================== */
/*  LEGACY_REGISTER -- Actually registers the legacy ROMs.  Also frees      */
/*                     the saved bc_cfgfile_t.                              */
/* ======================================================================== */
int legacy_register
(
    legacy_t     *l,
    periph_bus_t *bus,
    cp1600_t     *cp
)
{
    bc_cfgfile_t *bc = l->bc;
    bc_memspan_t *span;
    int num_ecs = 0, need_register, did_register = 0;
    uint32_t addr;

    /* -------------------------------------------------------------------- */
    /*  Register the legacy ROM.  Eventually, we might 'optimize' this as   */
    /*  I did for the Intellicart, where I have multiple periph_t's based   */
    /*  on what flags are set for each range.                               */
    /* -------------------------------------------------------------------- */
    for (span = bc->span; span; span = (bc_memspan_t *)span->l.next)
    {

        if ( span->epage != BC_SPAN_NOPAGE ||
            (span->flags & BC_SPAN_EP) != 0)
            continue;

        /* ---------------------------------------------------------------- */
        /*  Handle other segments directly.  If all the addresses in a      */
        /*  span are already mapped, skip this span.  We could/should       */
        /*  optimize this at some point to only register areas not yet      */
        /*  mapped, but I'm lazy right now.  The optimization would rarely  */
        /*  if ever kick in.                                                */
        /* ---------------------------------------------------------------- */
        need_register = 0;
        for (addr = span->s_addr; addr <= span->e_addr; addr++)
        {
            if (!(l->loc[addr].flags & LOC_MAPPED))
                need_register = 1;

            l->loc[addr].flags |= LOC_MAPPED;
        }

        if (need_register)
        {
            periph_register(bus, &l->periph, span->s_addr, span->e_addr,
                            span->flags & BC_SPAN_PK ? "[macro poke]"
                                                     : "Legacy BIN+CFG");
            did_register = 1;
        }

        /* ---------------------------------------------------------------- */
        /*  Mark the span cacheable, with snoop if necessary.               */
        /* ---------------------------------------------------------------- */
        cp1600_cacheable(cp, span->s_addr, span->e_addr,
                         (span->flags & BC_SPAN_W) != 0);
    }

    /* -------------------------------------------------------------------- */
    /*  If no legacy segments got registered, register a dummy segment, so  */
    /*  our dtor gets called.  Otherwise we leak memory!                    */
    /* -------------------------------------------------------------------- */
    if (!did_register)
    {
        periph_register(bus, &l->periph, 0, 0, "[Legacy BIN+CFG]");
    }

    /* -------------------------------------------------------------------- */
    /*  Handle ECS-style pages differently.                                 */
    /* -------------------------------------------------------------------- */
    for (num_ecs = 0; num_ecs < l->npg_rom; num_ecs++)
    {
        char name[17];
        uint32_t s_addr = l->pg_rom[num_ecs].periph.addr_base;
        uint32_t e_addr = l->pg_rom[num_ecs].periph.addr_base + 0xFFF;

        snprintf(name, 17, "Paged ROM %d", num_ecs % 100000);
        periph_register(bus, &(l->pg_rom[num_ecs].periph),
                        s_addr, e_addr, name);

        cp1600_cacheable( cp, s_addr, e_addr, 0 );
        /* XXX:  for now paged ROM is never paged RAM.  Need to fix that.   */
        /* Note "snoopable" flag above set to 0.                            */
    }

#ifndef BC_NOFREE
    /* -------------------------------------------------------------------- */
    /*  Discard the parsed config.                                          */
    /* -------------------------------------------------------------------- */
    bc_free_cfg(l->bc);
    l->bc = NULL;
#endif

    assert(num_ecs == l->npg_rom);

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
/* ------------------------------------------------------------------------ */
/*           Copyright (c) 2003-+Inf, Joseph Zbiciak, John Tanner           */
/* ======================================================================== */
