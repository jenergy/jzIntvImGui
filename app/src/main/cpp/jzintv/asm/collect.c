/* ======================================================================== */
/*  Collects all the ROM data for a given assembly, and then outputs it.    */
/*                                                                          */
/*  Intended to add some abstraction to the current ROM output code.        */
/* ======================================================================== */

#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "frasmdat.h"
#include "fragcon.h"
#include "as1600_types.h"
#include "protos.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"
#include "metadata/metadata.h"
#include "metadata/cfgvar_metadata.h"
#include "misc/printer.h"
#include "misc/types.h"
#include "collect.h"

/* ------------------------------------------------------------------------ */
/*  Flags assocated with each word in the ROM image.                        */
/*  Currently identical to the set used by ICARTROM.                        */
/*  Uninitialized spans set the word to 0xFFFF and do not set FLAG_HADATA.  */
/* ------------------------------------------------------------------------ */
#define FLAG_READ       ICARTROM_READ
#define FLAG_WRITE      ICARTROM_WRITE
#define FLAG_NARROW     ICARTROM_NARROW
#define FLAG_BANKSW     ICARTROM_BANKSW
#define FLAG_HASDATA    ICARTROM_PRELOAD
#define FLAG_PAGESW     (1 << 5)
#define FLAG_ERROVER    (1 << 6)    /* error on overwrite */

#define ICART_TO_MODE(x)    ((x) & 0xF);
typedef struct rom_data_t
{
    uint16_t word;
    uint16_t flag;
} rom_data_t;

#ifdef BYTE_LE
LOCAL INLINE uint16_t host_to_be16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}
#else
LOCAL INLINE uint16_t host_to_be16(uint16_t x)
{
    return x;
}
#endif


/* ------------------------------------------------------------------------ */
/*  ROM_DATA                                                                */
/*                                                                          */
/*  Allocate a worst-case structure for the largest ROM you could assemble. */
/*  This is far easier to get correct than a fancier data structure, and    */
/*  on any computer built this century (or even a smartphone built since    */
/*  2007), a relatively trivial amount of RAM to request.                   */
/* ------------------------------------------------------------------------ */
LOCAL rom_data_t rom_data[16][65536]; /* The ROM data.                      */
LOCAL int        rom_page[16];        /* pages present in each 4K window    */
LOCAL int        rom_err [16];
LOCAL int        rom_is_pagesw;       /* At least one page-switch segment   */
LOCAL int        rom_is_banksw;       /* At least one bank-switch segment   */
LOCAL int        rom_is_confused;
LOCAL cfg_var_t *rom_cfg_var = NULL;  /* Configuration variables            */

/* ------------------------------------------------------------------------ */
/*  These flags indicate whether it's safe to output a ROM of the           */
/*  indicated format.                                                       */
/*                                                                          */
/*  For example, Intellicart / CC3 do not support ECS-style paged ROMs.     */
/*  BIN + CFG doesn't *really* support Intellicart-style bankswitching.     */
/*  The CFG format allows specifying it, but jzIntv won't load it, so the   */
/*  assembler will output a warning when it sees it.                        */
/* ------------------------------------------------------------------------ */
int binfmt_ok = 1;      /* BIN + CFG                */
int romfmt_ok = 1;      /* Intellicart / CC3 .ROM   */

/* ------------------------------------------------------------------------ */
/*  ROM Overwrite Mode Flags.                                               */
/*                                                                          */
/*  These are used to control whether we warn when a portion of ROM gets    */
/*  overwritten.                                                            */
/*                                                                          */
/*  The bitmap 'ovw_err' contains overwrite error flags for the entire      */
/*  memory map, so we can report the errors in a clean sweep at the end.    */
/* ------------------------------------------------------------------------ */
LOCAL int err_if_overwritten = 0;
LOCAL int force_overwrite = 0;
LOCAL uint32_t ovw_err[16][65536 / 32];
LOCAL int ovw_errs = 0;

/* ------------------------------------------------------------------------ */
/*  COLLECT_INIT     -- Initialize the collector.                           */
/* ------------------------------------------------------------------------ */
void collect_init(void)
{
    int a, p, s;

    /* Initialize entire ROM image to "not present" */
    for (p = 0; p < 16; p++)
        for (a = 0; a < 65536; a++)
        {
            rom_data[p][a].word = 0xFFFF;
            rom_data[p][a].flag = 0;
        }

    /* Set entire ROM image to "unpaged", in the ECS-style paging sense */
    for (s = 0; s < 16; s++)
    {
        rom_page[s] = 0;
        rom_err [s] = 0;
    }

    rom_is_pagesw   = 0;
    rom_is_banksw   = 0;
    rom_is_confused = 0;

    /* Initialize overwrite errors to "no error" */
    for (p = 0; p < 16; p++)
        for (a = 0; a < 65536/32; a++)
            ovw_err[p][a] = 0;

    ovw_errs = 0;
    err_if_overwritten = 0;
    force_overwrite = 0;
}

/* ------------------------------------------------------------------------ */
/*  COLLECT_ADDSEG   -- Add a segment of data to the ROM image.             */
/* ------------------------------------------------------------------------ */
const char *collect_addseg
(
    uint16_t *data,
    uint32_t  addr,
    uint32_t  len,
    int8_t    page,
    uint8_t   set_attr,
    uint8_t   clr_attr
)
{
    uint16_t addr_lo, addr_hi;
    uint16_t slot_lo, slot_hi, s;
    uint16_t flag, cum_flag;
    int p, i, a;

    /* -------------------------------------------------------------------- */
    /*  Overflow sanity check                                               */
    /* -------------------------------------------------------------------- */
    if (addr + len > 0x10000)
    {
        static int ao_err = 0;
        return ao_err++ ? NULL : "Address overflow (collect)";
    }

    /* -------------------------------------------------------------------- */
    /*  Page == -1 means unpaged; otherwise only allow 0 .. 15              */
    /* -------------------------------------------------------------------- */
    if (page > 15 || page < -1)
        return "Page number out of range";

    /* -------------------------------------------------------------------- */
    /*  Bankswitched (Intellicart-style) or Paged (ECS-style)?              */
    /* -------------------------------------------------------------------- */
    if ((set_attr & ICARTROM_BANKSW) != 0) rom_is_banksw = 1;
    if (page >= 0)                         rom_is_pagesw = 1;

    if (rom_is_banksw && rom_is_pagesw)
    {
        return rom_is_confused++
                ? NULL
                : "Cannot mix page-switching and bank-switching";
    }

    addr_lo = addr;
    addr_hi = addr + len - 1;
    slot_lo = addr_lo >> 12;
    slot_hi = addr_hi >> 12;

    /* -------------------------------------------------------------------- */
    /*  Check for page / unpaged consistency within 4K slots.               */
    /* -------------------------------------------------------------------- */
    for (s = slot_lo; s <= slot_hi; s++)
    {
        if ((rom_page[s] == -1 && page >= 0) ||
            (rom_page[s] > 0   && page <  0))
            return rom_err[s]++
                     ? NULL
                     : "Mixture of paged and unpaged ROM in same 4K range";
    }

    /* -------------------------------------------------------------------- */
    /*  If it's paged, go ahead and set the page-present. If it's unpaged,  */
    /*  mark it unpaged.                                                    */
    /* -------------------------------------------------------------------- */
    for (s = slot_lo; s <= slot_hi; s++)
    {
        if (page >= 0)  rom_page[s] |= 1 << page;
        else            rom_page[s]  = -1;
    }

    if (page >= 0)
        set_attr |= FLAG_PAGESW;

    /* -------------------------------------------------------------------- */
    /*  Copy the data in, if any, and update flags.                         */
    /* -------------------------------------------------------------------- */
    p = page < 0 ? 0 : page;

    if (data)
    {
        for (i = 0, a = addr_lo; a <= addr_hi; a++, i++)
            rom_data[p][a].word = data[i];

        set_attr |= FLAG_HASDATA;

        /* ---------------------------------------------------------------- */
        /*  Check for data-overwrite errors unless told not to.             */
        /* ---------------------------------------------------------------- */
        if (!force_overwrite)
            for (i = 0, a = addr_lo; a <= addr_hi; a++, i++)
                if ((rom_data[p][a].flag & FLAG_ERROVER) != 0)
                {
                    const uint32_t idx = a >> 5;
                    const uint32_t shf = a & 31;
                    ovw_err[p][idx] |= 1u << shf;
                    ovw_errs++;
                }

        /* ---------------------------------------------------------------- */
        /*  Set the warn-overwrite flag if asked to.                        */
        /* ---------------------------------------------------------------- */
        if (err_if_overwritten)
            set_attr |= FLAG_ERROVER;
    }

    cum_flag = 0;
    for (a = addr_lo; a <= addr_hi; a++)
    {
        flag = rom_data[p][a].flag;
        flag &= ~clr_attr;
        flag |=  set_attr;
        rom_data[p][a].flag = flag;

        cum_flag |= flag;
    }

    /* -------------------------------------------------------------------- */
    /*  BIN+CFG warnings                                                    */
    /* -------------------------------------------------------------------- */
    if (binoutf && cfgoutf)
    {
        /* ---------------------------------------------------------------- */
        /*  Warn if we see bankswitched ROM, and BIN+CFG requested.         */
        /* ---------------------------------------------------------------- */
        if (rom_is_banksw)
        {
            static int bs_warn = 0;

            if (!bs_warn)
            {
                bs_warn = 1;
                frp2warn("Bankswitched memory is not well supported "
                         "in BIN+CFG format");
            }
            binfmt_ok = 0;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  ROM warnings                                                        */
    /* -------------------------------------------------------------------- */
    if (romoutf)
    {
        static int ps_warn = 0;

        if (rom_is_pagesw & !ps_warn)
        {
            ps_warn = 1;
            frp2warn("ROM format does not support page-switched output.");

            romfmt_ok = 0;
        }
    }

    if (!binfmt_ok && (!romoutf || !romfmt_ok) && !rom_is_confused)
    {
        return rom_is_confused++
                ? NULL
                : "No compatible executable output format";
    }

    return NULL;
}

/* ------------------------------------------------------------------------ */
/*  COLLECT_SET_OVERWRITE_FLAGS -- Modulate set-if-overwritten/force-over   */
/* ------------------------------------------------------------------------ */
void collect_set_overwrite_flags
(
    const int new_err_if_overwritten,  /* -1 means no change */
    const int new_force_overwrite      /* -1 means no change */
)
{
    if (new_err_if_overwritten >= 0)
        err_if_overwritten = new_err_if_overwritten;

    if (new_force_overwrite >= 0)
        force_overwrite = new_force_overwrite;
}

/* ------------------------------------------------------------------------ */
/*  COLLECT_CFG_VAR  -- Collect configuration variables for BIN+CFG format  */
/* ------------------------------------------------------------------------ */
void collect_cfg_var(const char *name, const char *string, int value )
{
    cfg_var_t *new_var = CALLOC( cfg_var_t, 1 );

    if ( !new_var )
    {
        frp2error("Out of memory in collect_cfg_var");
        return;
    }

    new_var->name = strdup( name );

    if ( string )
    {
        new_var->val.flag    = VAL_STRING;
        new_var->val.str_val = strdup( string );
    } else
    {
        char buf[32];
        int is_decimal = value <= 4095;     /* including negative values */
        if ( is_decimal )
            sprintf(buf, "%d", value);
        else
            sprintf(buf, "$%.4X", value);

        char *val_string = strdup( buf );

        if ( !val_string )
        {
            free(new_var);
            frp2error("Out of memory in collect_cfg_var");
            return;
        }

        if ( is_decimal )
        {
            new_var->val.flag    = VAL_STRING | VAL_DECNUM;
            new_var->val.str_val = val_string;
            new_var->val.dec_val = value;
        } else
        {
            new_var->val.flag    = VAL_STRING | VAL_HEXNUM;
            new_var->val.str_val = val_string;
            new_var->val.hex_val = value;
        }
    }

    val_try_parse_date( &(new_var->val) );

    LL_CONCAT( rom_cfg_var, new_var, cfg_var_t );
}

LOCAL void output_romfmt(int and_bincfg);
LOCAL void output_bincfg(void);

LOCAL void emit_overwrite_error(uint32_t lo, uint32_t hi, int page)
{
    char buf[256];
    snprintf(buf, sizeof(buf) - 1, 
             page < 0 ? "ROM overwrite error on $%04X - $%04X"
                      : "ROM overwrite error on $%04X - $%04X PAGE %d",
             lo, hi, page);
    frp2error(buf);
}

/* ------------------------------------------------------------------------ */
/*  COLLECT_FLUSH    -- Write out everything that was collected.            */
/* ------------------------------------------------------------------------ */
void collect_flush(void)
{
    int want_bincfg = binoutf && cfgoutf;

    /* Did we have ROM overwrite errors? */
    if (ovw_errs)
    {
        uint32_t err_lo = 0, err_hi = 0;
        int in_err = 0;

        /* Look for overwrite errors on non-page-flipped segments */
        for (uint32_t addr = 0; addr <= 0xFFFF; addr++)
        {
            const uint32_t slot = addr >> 12;
            const uint32_t idx = addr >> 5;
            const uint32_t shf = addr & 31;

            if (rom_page[slot] != -1)
            {
                if (in_err)
                {
                    emit_overwrite_error(err_lo, err_hi, -1);
                    in_err = err_lo = err_hi = 0;
                }
                addr = ((slot + 1) << 12) - 1;
                continue;
            }

            if ((ovw_err[0][idx] >> shf) & 1)
            {
                if (!in_err)
                {
                    in_err = 1;
                    err_lo = addr;
                }
                err_hi = addr;
            } else
            {
                if (in_err)
                {
                    emit_overwrite_error(err_lo, err_hi, -1);
                    in_err = err_lo = err_hi = 0;
                }
            }
        }
        if (in_err)
        {
            emit_overwrite_error(err_lo, err_hi, -1);
            in_err = err_lo = err_hi = 0;
        }

        /* Look for overwrite errors in page-flipped segments */
        for (int page = 0; page < 16; page++)
        {
            for (uint32_t addr = 0; addr <= 0xFFFF; addr++)
            {
                const uint32_t slot = addr >> 12;
                const uint32_t idx = addr >> 5;
                const uint32_t shf = addr & 31;

                if (rom_page[slot] == -1 || ((rom_page[slot] >> page) & 1) == 0)
                {
                    if (in_err)
                    {
                        emit_overwrite_error(err_lo, err_hi, page);
                        in_err = err_lo = err_hi = 0;
                    }
                    addr = ((slot + 1) << 12) - 1;
                    continue;
                }

                if ((ovw_err[page][idx] >> shf) & 1)
                {
                    if (!in_err)
                    {
                        in_err = 1;
                        err_lo = addr;
                    }
                    err_hi = addr;
                } else
                {
                    if (in_err)
                    {
                        emit_overwrite_error(err_lo, err_hi, page);
                        in_err = err_lo = err_hi = 0;
                    }
                }
            }
            if (in_err)
            {
                emit_overwrite_error(err_lo, err_hi, page);
                in_err = err_lo = err_hi = 0;
            }
        }
    }

    if (rom_is_confused)
    {
        frp2error("Unable to generate output file");
        return;
    }

    if (romfmt_ok && romoutf)
    {
        /* If we flagged BIN+CFG as faulty, use icb_write_bincfg() to write
           an Intellicart-compatible BIN+CFG at least. */
        output_romfmt(binfmt_ok == 0 && want_bincfg);
    }

    if (binfmt_ok && want_bincfg)
        output_bincfg();
}

LOCAL void emit_map_span
(
    const uint32_t  span_lo,
    const uint32_t  span_hi,
    const int       page,
    const uint32_t  span_flags,
    uint32_t *const tot_size
)
{
    char addr_buf[40];
    char flags_buf[6];

    snprintf(addr_buf, sizeof(addr_buf)-1, 
            page < 0 ? "$%04X - $%04X" : "$%04X - $%04X PAGE $%X",
            span_lo, span_hi, page);

    flags_buf[0] = span_flags & FLAG_READ    ? 'R' : '-';
    flags_buf[1] = span_flags & FLAG_WRITE   ? 'W' : '-';
    flags_buf[2] = span_flags & FLAG_NARROW  ? 'N' : '-';
    flags_buf[3] = span_flags & FLAG_BANKSW  ? 'B' : '-';
    flags_buf[4] = span_flags & FLAG_HASDATA ? 'P' : '-';
    flags_buf[5] = 0;
    
    printf("      %-24s $%04X            %s\n", 
           addr_buf, span_hi - span_lo + 1, flags_buf);

    if ((span_flags & FLAG_HASDATA) && tot_size)
        *tot_size += span_hi - span_lo + 1;
}

/* ------------------------------------------------------------------------ */
/*  COLLECT_SHOW_MAP                                                        */
/* ------------------------------------------------------------------------ */
void collect_show_map(void)
{
    const uint32_t sig_flags = FLAG_READ | FLAG_WRITE | FLAG_NARROW 
                             | FLAG_BANKSW | FLAG_HASDATA;
    uint32_t span_lo = 0, span_hi = 0, span_flags = 0, addr;
    uint32_t tot_size = 0;

    printf("\n MEMORY MAP SUMMARY\n");
    printf("===========================================================\n");
    printf("      %-24s %-16s %s\n", "Address Range", "Size", "Flags");
    printf("-----------------------------------------------------------\n");

    /* -------------------------------------------------------------------- */
    /*  Print out non-paged segments first.                                 */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        const uint32_t slot = addr >> 12;

        if (rom_page[slot] != -1)
        {
            if (span_flags)
                emit_map_span(span_lo, span_hi, -1, span_flags, &tot_size);
            span_lo = span_hi = span_flags = 0;
            addr = ((slot + 1) << 12) - 1;
            continue;
        }

        if (((span_flags ^ rom_data[0][addr].flag) & sig_flags) == 0)
        {
            span_hi = addr;
            continue;
        }

        if (span_flags)
            emit_map_span(span_lo, span_hi, -1, span_flags, &tot_size);

        span_flags = rom_data[0][addr].flag & sig_flags;
        span_lo = addr;
        span_hi = addr;
    }

    if (span_flags)
        emit_map_span(span_lo, span_hi, -1, span_flags, &tot_size);

    /* -------------------------------------------------------------------- */
    /*  Print out paged segments next.                                      */
    /* -------------------------------------------------------------------- */
    span_flags = span_lo = span_hi = 0;
    for (int page = 0; page < 16; page++)
    {
        int span_slot = -1;

        for (addr = 0; addr <= 0xFFFF; addr++)
        {
            const int slot = addr >> 12;

            if (rom_page[slot] == -1 || ((rom_page[slot] >> page) & 1) == 0)
            {
                if (span_flags)
                    emit_map_span(span_lo, span_hi, page, span_flags,
                                  &tot_size);
                span_lo = span_hi = span_flags = 0;
                addr = ((slot + 1) << 12) - 1;
                continue;
            }

            if (span_slot == slot && 
                ((span_flags ^ rom_data[page][addr].flag) & sig_flags) == 0)
            {
                span_hi = addr;
                continue;
            }

            if (span_flags)
                emit_map_span(span_lo, span_hi, page, span_flags, &tot_size);

            span_flags = rom_data[page][addr].flag & sig_flags;
            span_lo = addr;
            span_hi = addr;
            span_slot = slot;
        }

        if (span_flags)
            emit_map_span(span_lo, span_hi, page, span_flags, &tot_size);
    }
    printf("===========================================================\n");
    printf(" TOTAL INITIALIZED SIZE:  $%04X words\n", tot_size);
}

LOCAL uint16_t binbuf[4096];

/* ------------------------------------------------------------------------ */
/*  OUTPUT_BINCFG                                                           */
/* ------------------------------------------------------------------------ */
LOCAL void output_bincfg(void)
{
    int addr, addr_lo, addr_hi, span_lo, span_hi, span_len, slot, page;
    int span_flag;
    int fileofs = 0, i;
    int memattr_hdr = 0;

    fprintf(cfgoutf, "[mapping]\015\012");

    addr_lo = addr_hi = -1;
    span_lo = span_hi = -1;
    span_len = -1;

    /* -------------------------------------------------------------------- */
    /*  Scan through by 4K chunks, outputting any ROM/RAM/WOM segments      */
    /*  that have data.  For paged segments, output exactly 4K sized        */
    /*  chunks.  For non-paged segments, output the exact words defined by  */
    /*  the program image.                                                  */
    /* -------------------------------------------------------------------- */
    for (page = 0; page < 16; page++)
    {
        for (slot = 0; slot < 16; slot++)
        {
            span_flag = 0;

            if (rom_page[slot] == 0 || rom_page[slot] == -1)
                continue;

            addr_lo = slot << 12;
            addr_hi = addr_lo | 0xFFF;

            /* if we get here, we're in a paged segment. */
            if (((rom_page[slot] >> page) & 1) == 0)
                continue;

            for (i = 0, addr = addr_lo; addr <= addr_hi; addr++, i++)
            {
                binbuf[i] = host_to_be16( rom_data[page][addr].word );
                span_flag |= rom_data[page][addr].flag;
            }

            if ( (span_flag & FLAG_HASDATA) == 0 )
                continue;

            fprintf(cfgoutf, "$%.4X - $%.4X = $%.4X PAGE %.1X",
                    fileofs, fileofs + 0xFFF, addr_lo, page);

            switch (span_flag & (FLAG_READ | FLAG_WRITE))
            {
                case FLAG_READ | FLAG_WRITE:
                    fprintf(cfgoutf, " RAM %d",
                            span_flag & FLAG_NARROW ? 8 : 16);
                    break;
                case FLAG_WRITE:
                    fprintf(cfgoutf, " WOM %d",
                            span_flag & FLAG_NARROW ? 8 : 16);
                    break;
                case FLAG_READ:
                    if ( span_flag & FLAG_NARROW )
                        fprintf(cfgoutf, " ROM 8");
                    break;
            }
            fputs("\015\012", cfgoutf);

            fwrite((void *)binbuf, 2, 0x1000, binoutf);

            fileofs += 0x1000;
        }
    }

    for (slot = 0; slot < 16; slot++)
    {
        if (rom_page[slot] != -1)
            continue;

        addr_lo = slot << 12;
        addr_hi = addr_lo | 0xFFF;

        span_lo = span_hi = -1;
        span_len  = 0;
        span_flag = 0;

        /* Find HASDATA spans and output them exactly */
        for (addr = addr_lo; addr <= addr_hi; addr++)
        {
            int this_flag = rom_data[0][addr].flag &
                            (FLAG_READ|FLAG_WRITE|FLAG_NARROW|FLAG_HASDATA);

            /* Continue our current state */
            if (this_flag == span_flag)
            {
                if (span_hi != -1)
                {
                    span_hi = addr;
                    binbuf[span_len++] = host_to_be16(rom_data[0][addr].word);
                }

                if (addr != addr_hi)
                    continue;
            }

            /* Something changed, or end of 4K segment.  Flush current seg. */
            if (span_len)
            {
                fprintf(cfgoutf, "$%.4X - $%.4X = $%.4X",
                        fileofs, fileofs + span_len - 1, span_lo);

                switch (span_flag & (FLAG_READ | FLAG_WRITE))
                {
                    case FLAG_READ | FLAG_WRITE:
                        fprintf(cfgoutf, " RAM %d",
                                span_flag & FLAG_NARROW ? 8 : 16);
                        break;
                    case FLAG_WRITE:
                        fprintf(cfgoutf, " WOM %d",
                                span_flag & FLAG_NARROW ? 8 : 16);
                        break;
                    case FLAG_READ:
                        if ( span_flag & FLAG_NARROW )
                            fprintf(cfgoutf, " ROM 8");
                        break;
                }
                fputs("\015\012", cfgoutf);

                fwrite((void *)binbuf, 2, span_len, binoutf);

                fileofs += span_len;
                span_lo  = -1;
                span_hi  = -1;
                span_len = 0;
                span_flag = 0;

                if (addr == addr_hi)
                    break;
            }

            /* Start a new segment, if HASDATA */
            if ((this_flag & FLAG_HASDATA) != 0 &&
                (this_flag & (FLAG_READ|FLAG_WRITE)) != 0)
            {
                span_lo = span_hi = addr;
                span_len = 0;
                span_flag = this_flag;
                binbuf[span_len++] = host_to_be16(rom_data[0][addr].word);
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Scan through by 4K chunks, outputting any ROM/RAM/WOM segments      */
    /*  that do NOT have data.  As before, round paged segments to exact    */
    /*  4K chunks, while leaving non-paged segments exact-sized.            */
    /* -------------------------------------------------------------------- */
    for (page = 0; page < 16; page++)
    {
        for (slot = 0; slot < 16; slot++)
        {
            int flag = 0;

            if (rom_page[slot] == 0 || rom_page[slot] == -1)
                continue;

            addr_lo = slot << 12;
            addr_hi = addr_lo | 0xFFF;

            /* if we get here, we're in a paged segment. */
            if (((rom_page[slot] >> page) & 1) == 0)
                continue;

            for (i = 0, addr = addr_lo; addr <= addr_hi; addr++, i++)
                flag |= rom_data[page][addr].flag;

            if ( (flag & FLAG_HASDATA) != 0 )
                continue;

            if (!memattr_hdr)
            {
                fprintf(cfgoutf, "\015\012[memattr]\015\012");
                memattr_hdr = 1;
            }

            fprintf(cfgoutf, "$%.4X - $%.4X = PAGE %.1X",
                    addr_lo, addr_hi, page);

            switch (flag & (FLAG_READ | FLAG_WRITE))
            {
                case FLAG_READ | FLAG_WRITE:
                    fprintf(cfgoutf, " RAM %d", flag & FLAG_NARROW ? 8 : 16);
                    break;
                case FLAG_WRITE:
                    fprintf(cfgoutf, " WOM %d", flag & FLAG_NARROW ? 8 : 16);
                    break;
                case FLAG_READ:
                    fprintf(cfgoutf, " ROM %d", flag & FLAG_NARROW ? 8 : 16);
                    break;
                default:
                    fprintf(cfgoutf, " ; unknown!");
                    break;
            }
            fputs("\015\012", cfgoutf);
        }
    }

    for (slot = 0; slot < 16; slot++)
    {
        if (rom_page[slot] != -1)
            continue;

        addr_lo = slot << 12;
        addr_hi = addr_lo | 0xFFF;

        span_lo = span_hi = -1;
        span_len  = 0;
        span_flag = 0;

        /* Find !HASDATA spans and output them exactly */
        for (addr = addr_lo; addr <= addr_hi; addr++)
        {
            int this_flag = rom_data[0][addr].flag &
                            (FLAG_READ|FLAG_WRITE|FLAG_NARROW|FLAG_HASDATA);

            /* Continue our current state */
            if (this_flag == span_flag)
            {
                if (span_hi != -1)
                    span_hi = addr;

                if (addr != addr_hi)
                    continue;
            }

            /* Something changed, or end of 4K segment.  Flush current seg. */
            if (span_hi != -1)
            {
                if (!memattr_hdr)
                {
                    fprintf(cfgoutf, "\015\012[memattr]\015\012");
                    memattr_hdr = 1;
                }

                fprintf(cfgoutf, "$%.4X - $%.4X =", span_lo, span_hi);

                switch (span_flag & (FLAG_READ | FLAG_WRITE))
                {
                    case FLAG_READ | FLAG_WRITE:
                        fprintf(cfgoutf, " RAM %d",
                                span_flag & FLAG_NARROW ? 8 : 16);
                        break;
                    case FLAG_WRITE:
                        fprintf(cfgoutf, " WOM %d",
                                span_flag & FLAG_NARROW ? 8 : 16);
                        break;
                    case FLAG_READ:
                        fprintf(cfgoutf, " ROM %d",
                                span_flag & FLAG_NARROW ? 8 : 16);
                        break;
                    default:
                        fprintf(cfgoutf, " ; unknown!");
                }
                fputs("\015\012", cfgoutf);

                span_lo  = -1;
                span_hi  = -1;
                span_len = 0;
                span_flag = 0;

                if (addr == addr_hi)
                    break;
            }

            /* Start a new segment, if HASDATA */
            if ((this_flag & FLAG_HASDATA) == 0 &&
                (this_flag & (FLAG_READ|FLAG_WRITE)) != 0)
            {
                span_lo = span_hi = addr;
                span_len = 0;
                span_flag = this_flag;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Output any configuration variables.                                 */
    /* -------------------------------------------------------------------- */
    if (rom_cfg_var)
    {
        printer_t cfgoutp = printer_to_file( cfgoutf );
        fprintf(cfgoutf, "\015\012[vars]\015\012");
        print_cfg_var_list( rom_cfg_var, &cfgoutp );
    }

    fflush(binoutf);
    fflush(cfgoutf);
}

extern ictype_t icart_type;

/* ------------------------------------------------------------------------ */
/*  OUTPUT_ROMFMT                                                           */
/* ------------------------------------------------------------------------ */
LOCAL void output_romfmt(int and_bincfg)
{
    icartrom_t *icart_rom;
    int addr;
    uint32_t size;
    uint8_t *rom_img;
    int overlaps_jlp = 0, requires_jlp = 0;
    int overlap_jlp_lo = -1, overlap_jlp_hi = -1;

    /* -------------------------------------------------------------------- */
    /*  Allocate an icart_rom, and populate it with the ROM data we've      */
    /*  collected.  We only need to scan page 0, because icart doesn't      */
    /*  support paged ROM.                                                  */
    /* -------------------------------------------------------------------- */
    icart_rom = icartrom_new();

    for (addr = 0; addr < 0x10000; addr++)
    {
        uint16_t  flag = rom_data[0][addr].flag;
        uint16_t *data = flag & FLAG_HASDATA ? &rom_data[0][addr].word : 0;
        if (data || (flag & 0xF) != 0)
        {
            icartrom_addseg(icart_rom, data, addr, 1, flag & 0xF, 0);
            if ((flag & ICARTROM_READ) == ICARTROM_READ && 
                addr >= 0x8000 && addr <= 0x9FFF)
            {
                overlaps_jlp = 1;
                overlap_jlp_hi = addr;
                if (overlap_jlp_lo < 0)
                    overlap_jlp_lo = addr;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Add metadata to the .ROM image.                                     */
    /* -------------------------------------------------------------------- */
    if ( rom_cfg_var )
        icart_rom->metadata = game_metadata_from_cfgvars( rom_cfg_var );

    /* -------------------------------------------------------------------- */
    /*  Check to see whether any of the readable ROM space overlaps JLP.    */
    /* -------------------------------------------------------------------- */
    if (icart_rom->metadata)
        requires_jlp = REQ_JLP(icart_rom->metadata->jlp_accel);

    if (overlaps_jlp && requires_jlp)
    {
        char buf[100];
        snprintf(buf, 100, 
                 ".ROM requests JLP, but has overlapping "
                 "readable section at $%.4X - $%.4X\n",
                 overlap_jlp_lo, overlap_jlp_hi);
        frp2warn(buf);
    }

    /* -------------------------------------------------------------------- */
    /*  Write out the ROM image next.                                       */
    /* -------------------------------------------------------------------- */
    rom_img = icartrom_genrom(icart_rom, &size, icart_type);

    if (rom_img)
    {
        fwrite(rom_img, 1, size, romoutf);
        fflush(romoutf);
        free(rom_img);
    } else
        frp2error("Internal Error:  Could not generate ROM image");

    /* -------------------------------------------------------------------- */
    /*  If we were asked to also write an Intellicart-specific BIN+CFG,     */
    /*  then do that also.                                                  */
    /* -------------------------------------------------------------------- */
    if (and_bincfg)
    {
        icb_write_bincfg(binoutf, cfgoutf, icart_rom, 0);
    }

    icartrom_delete(icart_rom);
}


