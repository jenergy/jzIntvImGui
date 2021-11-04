/* ======================================================================== */
/*  EMITTERS:  Functions that emit a record to the intermediate file.       */
/*                                                                          */
/*  This file is an attempt to collect up all the functions that write to   */
/*  the intermediate file in one place, increasing the abstraction level.   */
/*  The goal is to eventually eliminate the intermediate file altogether,   */
/*  once all direct references to it have been removed.                     */
/*                                                                          */
/*  Initially, though, this will just be a series of functions that wrap    */
/*  the fprintf statements collected from elsewhere.                        */
/* ======================================================================== */

#include "config.h"
#include "as1600_types.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "frasmdat.h"
#include "fragcon.h"
#include "protos.h"
#include "intermed.h"
#include "memo_string.h"
/*#include "malloc.h"*/

/* ------------------------------------------------------------------------ */
/*  Render buffer for emitters.                                             */
/* ------------------------------------------------------------------------ */
static char   *emit_buf = NULL;
static size_t  eb_len   = 0;

/* ------------------------------------------------------------------------ */
/*  Replace intermediate file with in-memory list.                          */
/* ------------------------------------------------------------------------ */

static irec_union *irec_tbl = NULL;
static unsigned irec_head = 0, irec_tail = 0, irec_total = 0;
static unsigned irec_mask = 0;

/* ------------------------------------------------------------------------ */
/*  ALLOC_IREC           -- Allocate an intermediate record that's already  */
/*                          appended to the list, but must be populated.    */
/* ------------------------------------------------------------------------ */
static intermed_rec *alloc_irec(const irec_type type)
{
    if (irec_tail - irec_head >= irec_total)
    {
        assert(irec_total < (1 << 29));
        irec_total = irec_total ? (irec_total << 2) : (1 << 16);
        irec_tbl   = (irec_union *)realloc(irec_tbl,
                                           irec_total * sizeof(irec_union));

        irec_mask  = irec_total - 1;
        assert((irec_total & (irec_total - 1)) == 0);
        assert(irec_tbl);
    }

    irec_union *const irec = &irec_tbl[irec_tail++ % irec_total];

    irec->irec.type = type;
    irec->irec.line = infilestk[currfstk].line;

    return (intermed_rec *)irec;
}

#define GET_IREC(type,TYPE) type *irec = (type *)(void *)alloc_irec(TYPE)


/* ------------------------------------------------------------------------ */
/*  EMIT_LISTING_MODE    -- This changes the output listing display mode    */
/* ------------------------------------------------------------------------ */
void emit_listing_mode(const listing_mode m)
{
    GET_IREC(irec_list_mode, REC_LIST_MODE);
    irec->mode = m;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_SET_EQU         -- Output value associated with a SET or EQU.      */
/* ------------------------------------------------------------------------ */
void emit_set_equ(const unsigned int value)
{
    GET_IREC(irec_set_equ, REC_SET_EQU);
    irec->value = value;
}


/* ------------------------------------------------------------------------ */
/*  EMIT_COMMENT         -- Emit a comment either for SET/EQU or a user's   */
/*                          CMSG/SMSG.                                      */
/* ------------------------------------------------------------------------ */
void emit_comment(const int is_user, const char *const format, ...)
{
    GET_IREC(irec_string, is_user ? REC_USER_COMMENT : REC_COMMENT);
    va_list ap;

    assert(emit_buf);

again:
    va_start(ap, format);
    vsnprintf(emit_buf, eb_len, format, ap);
    va_end(ap);

    if (eb_len < 65536 && strlen(emit_buf) >= eb_len - 1)
    {
        eb_len <<= 2;
        emit_buf = (char *)realloc(emit_buf, eb_len);
        goto again;
    }

    irec->string = memoize_string(emit_buf);
}

/* ------------------------------------------------------------------------ */
/*  EMIT_LOCATION        -- Establish the current address we're assembling  */
/*                          at, and what sort of memory mode.               */
/* ------------------------------------------------------------------------ */
void emit_location(const int seg, const int pag, const int loc,
                   const int type, const char *const mode)
/*
       description output to the intermediate file, a 'P' record
                   giving the current location counter.  Segment
                   is not used at this time.
*/
{
    GET_IREC(irec_loc_set, REC_LOC_SET);

    irec->seg  = seg;
    irec->pag  = pag;
    irec->loc  = loc;
    irec->type = type;
    irec->mode = mode;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_MARK_WITH_MODE  -- Mark addresses lo to hi with a given mode.      */
/* ------------------------------------------------------------------------ */
void emit_mark_with_mode(const int lo, const int hi, const char *const mode)
{
    GET_IREC(irec_mark_range, REC_MARK_RANGE);

    irec->lo   = lo;
    irec->hi   = hi;
    irec->mode = mode;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_RESERVE         -- Reserve 'len' addresses.                        */
/* ------------------------------------------------------------------------ */
void emit_reserve(const int endaddr)
{
    GET_IREC(irec_reserve_range, REC_RESERVE_RANGE);
    irec->endaddr = endaddr;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_ENTERING_FILE   -- Record that we're entering a new file.          */
/* ------------------------------------------------------------------------ */
void emit_entering_file(const char *const name)
{
    GET_IREC(irec_string, REC_FILE_START);
    irec->string = name;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_EXITING_FILE    -- Record that we're leaving a file.               */
/* ------------------------------------------------------------------------ */
void emit_exiting_file(const char *const name)
{
    GET_IREC(irec_string, REC_FILE_EXIT);
    irec->string = name;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_WARNERR         -- Emit a warning or an error                      */
/* ------------------------------------------------------------------------ */
void emit_warnerr(const char *const file, const int line, const warnerr type,
                  const char *const format, ...)
{
    GET_IREC(irec_string, REC_ERROR);
    va_list ap;

    const size_t file_len = strlen(file);

    if (file_len + 100 > eb_len)
    {
        free(emit_buf);
        eb_len   = 2 * (file_len + 100);
        emit_buf = (char *)malloc(eb_len);
    }

    sprintf(emit_buf,
            type == WARNING ? "%s:%d: WARNING - " : "%s:%d: ERROR - ",
            file, line);

    const size_t pf_len = strlen(emit_buf);

    if (type == WARNING) warncnt++;
    else                 errorcnt++;

    size_t emit_len;
again:
    va_start(ap, format);
    vsnprintf(emit_buf + pf_len, eb_len - pf_len, format, ap);
    va_end(ap);

    emit_len = strlen(emit_buf);
    if (eb_len < 65536 && emit_len >= eb_len - 1)
    {
        eb_len <<= 2;
        emit_buf = (char *)realloc(emit_buf, eb_len);
        goto again;
    }

    char *const final = (char *)malloc(emit_len + 2);
    strcpy(final, emit_buf);
    final[emit_len] = 0;

    irec->string = final;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_RAW_ERROR       -- Emit a raw error (typically as reported by      */
/*                          the macro preprocessor).                        */
/* ------------------------------------------------------------------------ */
void emit_raw_error(const char *const raw_error)
{
    GET_IREC(irec_string, REC_ERROR);
    irec->string = memoize_string(raw_error);
    errorcnt++;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_LISTED_LINE     -- A line to put directly in the listing file.     */
/* ------------------------------------------------------------------------ */
void emit_listed_line(const char *const buf)
{
    GET_IREC(irec_string, REC_LIST_LINE);
    irec->string = memoize_string(buf);
}

/* ------------------------------------------------------------------------ */
/*  EMIT_UNLISTED_LINE   -- Keep track of source lines consumed, even if    */
/*                          they won't be replayed in the listing file.     */
/* ------------------------------------------------------------------------ */
void emit_unlisted_line(void)
{
}

/* ------------------------------------------------------------------------ */
/*  EMIT_GENERATED_INSTR -- Output the "polish notation" string for this    */
/*                          instruction.                                    */
/* ------------------------------------------------------------------------ */
void emit_generated_instr(const char *const buf)
{
    GET_IREC(irec_string, REC_DATA_BLOCK);
    irec->string = memoize_string(buf);
}

/* ------------------------------------------------------------------------ */
/*  EMIT_CFGVAR_INT       -- Output a .CFG variable with an integer value.  */
/* ------------------------------------------------------------------------ */
void emit_cfgvar_int(const char *const var, const int value)
{
    GET_IREC(irec_cfgvar_int, REC_CFGVAR_INT);
    irec->var   = var;
    irec->value = value;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_CFGVAR_STR       -- Output a .CFG variable with a string value.    */
/* ------------------------------------------------------------------------ */
void emit_cfgvar_str(const char *const var, const char *const value)
{
    GET_IREC(irec_cfgvar_str, REC_CFGVAR_STR);
    irec->var   = var;
    irec->value = value;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_SRCFILE_OVERRIDE -- Allow HLLs to indicate orig source file, line  */
/* ------------------------------------------------------------------------ */
void emit_srcfile_override(const char *const file, const int line)
{
    GET_IREC(irec_srcfile_over, REC_SRCFILE_OVER);
    irec->file = file;
    irec->line = line;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_AS1600_CFG_INT   -- Emit an AS1600 configuration parameter.        */
/* ------------------------------------------------------------------------ */
void emit_as1600_cfg_int(const as1600_cfg_item item, const int value)
{
    GET_IREC(irec_as1600_cfg, REC_AS1600_CFG);
    irec->item      = item;
    irec->int_value = value;
}

/* ------------------------------------------------------------------------ */
/*  EMIT_LISTING_COLUMN   -- Emit listing colunm configuration.             */
/* ------------------------------------------------------------------------ */
void emit_listing_column
(
    const int hex_source,   /*  Num hex values on a line with source        */
    const int hex_no_src,   /*  Num hex values on a line w/ no source       */
    const int source_col    /*  Column number of source on lines w/source   */
)
{
    GET_IREC(irec_listing_column, REC_LISTING_COLUMN);
    irec->hex_source = hex_source;
    irec->hex_no_src = hex_no_src;
    irec->source_col = source_col;
}

/* ------------------------------------------------------------------------ */
/*  Overwrite warning truth table:                                          */
/*                                                                          */
/*               Force                                                      */
/*              off   ON                                                    */
/*     Err off  ok    ok                                                    */
/*     Err ON   ERR   ok                                                    */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/*  EMIT_ERR_IF_OVERWRITTEN  -- Boolean flag indicating whether it's        */
/*                              unsafe to overwrite this data.              */
/* ------------------------------------------------------------------------ */
void emit_err_if_overwritten(const int enable_warning)
{
    GET_IREC(irec_overwrite, REC_OVERWRITE);
    irec->err_if_overwritten = !!enable_warning;
    irec->force_overwrite = -1;  /* no change */
}

/* ------------------------------------------------------------------------ */
/*  EMIT_FORCE_OVERWRITE     -- Boolean flag indicating whether subsequent  */
/*                              writes can overwrite without warnings.      */
/* ------------------------------------------------------------------------ */
void emit_force_overwrite(const int force_overwrite)
{
    GET_IREC(irec_overwrite, REC_OVERWRITE);
    irec->err_if_overwritten = -1;  /* no change */
    irec->force_overwrite = !!force_overwrite;
}

/* ------------------------------------------------------------------------ */
/*  INTERMED_START_PASS_1 -- Initialize the intermediate file (and later,   */
/*                           other data structures) for pass 1.             */
/* ------------------------------------------------------------------------ */
void intermed_start_pass_1(void)
{
    intermed_finish(0);

    if (!emit_buf)
    {
        emit_buf = (char *)malloc(4096);
        eb_len   = 4096;
    }
}

/* ------------------------------------------------------------------------ */
/*  INTERMED_START_PASS_2 -- Initialize the intermediate file (and later,   */
/*                           other data structures) for pass 2.             */
/* ------------------------------------------------------------------------ */
void intermed_start_pass_2(void)
{
    //fprintf(stderr, "total allocs = %d\n", allocs);
#if 0
    struct mallinfo m = mallinfo();

#   define M(x)    printf("%-15s %ld\n", #x, (long)m.x);
    M(arena)
    M(ordblks)
    M(smblks)
    M(hblks)
    M(hblkhd)
    M(usmblks)
    M(fsmblks)
    M(uordblks)
    M(fordblks)
    M(keepcost)
#endif
}

/* ------------------------------------------------------------------------ */
/*  INTERMED_FINISH       -- Clean up all the intermediate file/data/etc.   */
/*                           Optionally output debug info.                  */
/* ------------------------------------------------------------------------ */
void intermed_finish(const int debugmode)
{
    UNUSED(debugmode);

    if (irec_tbl)
    {
        while (irec_head < irec_tail)
            pass2_release_rec(&irec_tbl[irec_head++ % irec_total]);

        free(irec_tbl);
        irec_head = irec_tail = irec_total = 0;
    }
}

/* ------------------------------------------------------------------------ */
/*  PASS2_NEXT_LINE      -- Gets the next line of the intermediate data.    */
/* ------------------------------------------------------------------------ */
irec_union *pass2_next_rec(void)
{
    if (irec_head > irec_total)
    {
        irec_head -= irec_total;
        irec_tail -= irec_total;
    }
    return irec_head < irec_tail ? &irec_tbl[irec_head++ % irec_total] : NULL;
}

void pass2_release_rec(irec_union *const irec)
{
    UNUSED(irec);
}
