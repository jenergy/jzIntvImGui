#ifndef MISC_TYPES_H_
#define MISC_TYPES_H_

#include "misc/ll.h"

/* ------------------------------------------------------------------------ */
/*  Simple variable-resolution date structure                               */
/* ------------------------------------------------------------------------ */
typedef struct game_date_t
{
    uint16_t year;              /*  Year, or 0 if unknown                   */
    uint8_t  month;             /*  Month (1-12), or 0 if unknown           */
    uint8_t  day;               /*  Day (1-31), or 0 if unknown             */
    int8_t   hour;              /*  Hour (0-23), or -1 if unknown           */
    int8_t   min;               /*  Minutes (0-59), or -1 if unknown        */
    int8_t   sec;               /*  Seconds (0-60), or -1 if unknown        */
    int16_t  utc_delta;         /*  UTC delta (-720..720) or UTC_UNKNWON    */
} game_date_t;

#define UTC_DELTA_UNKNOWN (-32768)

/* ------------------------------------------------------------------------ */
/*  DATES_EQUAL  -- Returns true if two game_date_t* point to equal dates.  */
/* ------------------------------------------------------------------------ */
#define DATES_EQUAL(a,b)         \
    ((a) && (b)               && \
     (a)->year  == (b)->year  && \
     (a)->month == (b)->month && \
     (a)->day   == (b)->day)

/* ------------------------------------------------------------------------ */
/*  VAL_STRNUM_T -- A value that could be one of several types.             */
/* ------------------------------------------------------------------------ */
#define VAL_DECNUM  (1)
#define VAL_HEXNUM  (2)
#define VAL_STRING  (4)
#define VAL_DATE    (8)

typedef struct val_strnum_t
{
    uint32_t        flag;
    int32_t         dec_val;
    uint32_t        hex_val;
    char *          str_val;
    game_date_t     date_val;
} val_strnum_t;

#define VAL_HAS_DECNUM(v) (((v).flag & VAL_DECNUM) != 0)
#define VAL_HAS_HEXNUM(v) (((v).flag & VAL_HEXNUM) != 0)
#define VAL_HAS_STRING(v) (((v).flag & VAL_STRING) != 0)
#define VAL_HAS_DATE(v)   (((v).flag & VAL_DATE  ) != 0)

/* ------------------------------------------------------------------------ */
/*  CFG_VAR_T    -- <name,value> tuple.  Also stores 'type' for value.      */
/* ------------------------------------------------------------------------ */
typedef struct cfg_var_t
{
    ll_t            l;              /* linked list of variable defs.        */
    char           *name;           /* name of the variable.                */
    val_strnum_t    val;            /* Numeric, string or either.           */
} cfg_var_t;

/* ------------------------------------------------------------------------ */
/*  VAL_TRY_PARSE_DECNUM                                                    */
/*                                                                          */
/*  See if this variable has a numeric value that could be interpreted      */
/*  as decimal.  Only succeed if all characters are 0-9.                    */
/* ------------------------------------------------------------------------ */
void val_try_parse_decnum( val_strnum_t *const val );

/* ------------------------------------------------------------------------ */
/*  VAL_TRY_PARSE_HEXNUM                                                    */
/*                                                                          */
/*  See if this variable has a numeric value that could be interpreted      */
/*  as decimal.  Only succeed if all characters are 0-9, A-F, a-f, with     */
/*  the minor exception that '$' is allowed as a prefix.                    */
/* ------------------------------------------------------------------------ */
void val_try_parse_hexnum( val_strnum_t *const val );

/* ------------------------------------------------------------------------ */
/*  VAL_TRY_PARSE_DATE                                                      */
/*                                                                          */
/*  See if this variable has a date-like string in one of the following     */
/*  formats, or a subset.  Fields can be missing from the right.            */
/*                                                                          */
/*      YYYY/MM/DD HH:MM:SS +hh:mm                                          */
/*      YYYY-MM-DD HH:MM:SS +hh:mm                                          */
/*                                                                          */
/*  If so, populate the date_val structure and set the VAL_DATE flag.       */
/*  The VAL_DATE is not used for printing most places, but rather just      */
/*  provided as a convenience to date-consuming code.                       */
/* ------------------------------------------------------------------------ */
void val_try_parse_date( val_strnum_t *const val );

/* ------------------------------------------------------------------------ */
/*  GAME_DATE_TO_STRING                                                     */
/*  Convert a game_date_t to a string.  The string is malloc'd.  Returns    */
/*  NULL on failure.                                                        */
/* ------------------------------------------------------------------------ */
char *game_date_to_string( const game_date_t *const date );

/* ------------------------------------------------------------------------ */
/*  GAME_DATE_TO_UINT8_T -- Convert date to serialized .ROM/.LUIGI format.  */
/*  Note: buf[] must have room for up to 8 bytes.                           */
/* ------------------------------------------------------------------------ */
int game_date_to_uint8_t(const game_date_t *const d, uint8_t *const data);

/* ------------------------------------------------------------------------ */
/*  UINT8_T_TO_GAME_DATE -- Deserialize a serialized game date.             */
/*  Returns 0 on success, non-zero on failure.                              */
/* ------------------------------------------------------------------------ */
int uint8_t_to_game_date(game_date_t *const d, const uint8_t *const data,
                        const int length);

/* ------------------------------------------------------------------------ */
/*  VAL_ADD_DATE_STRING                                                     */
/*  If a val has a date but no string, generate the string if possible.     */
/* ------------------------------------------------------------------------ */
void val_add_date_string( val_strnum_t *const val );

/* ------------------------------------------------------------------------ */
/*  FREE_CFG_VAR         -- Free a single CFG_VAR.                          */
/*  FREE_CFG_VAR_LIST    -- Free a list of CFG_VAR_T.                       */
/* ------------------------------------------------------------------------ */
void free_cfg_var     ( cfg_var_t *var  );
void free_cfg_var_list( cfg_var_t *head );

/* ------------------------------------------------------------------------ */
/*  CFG_ESCAPE_STR       -- Add escape character sequences as needed.       */
/*                          Used for strings in vars and other CFG stuff.   */
/*                          Non-reentrant.  Shares buffer w/cfg_quote_str.  */
/* ------------------------------------------------------------------------ */
const char *cfg_escape_str( const char *const str );

/* ------------------------------------------------------------------------ */
/*  CFG_UNESCAPE_STR     -- Convert escape character sequences as needed.   */
/*                          Used for strings in vars and other CFG stuff.   */
/*                          Non-reentrant. Shares buffer w/cfg_unquote_str. */
/* ------------------------------------------------------------------------ */
const char *cfg_unescape_str( const char *const str );

/* ------------------------------------------------------------------------ */
/*  CFG_QUOTE_STR        -- Quote a string if necessary.  Non-reentrant.    */
/*                          Add escape character sequences as needed.       */
/*                          Used for strings in vars and other CFG stuff.   */
/* ------------------------------------------------------------------------ */
const char *cfg_quote_str( const char *const str );

/* ------------------------------------------------------------------------ */
/*  CFG_UNQUOTE_STR      -- Remove quotes from a string.  Non-reentrant.    */
/*                          Convert escape character sequences as needed.   */
/*                          Used for strings in vars and other CFG stuff.   */
/* ------------------------------------------------------------------------ */
const char *cfg_unquote_str( const char *const str );

/* ------------------------------------------------------------------------ */
/*  STRIP_BAD_UTF8       -- Replaces bad UTF-8 characters with '?'.         */
/*                          Destination may safely overlap source.          */
/*                          Output is never longer than the input.          */
/* ------------------------------------------------------------------------ */
void strip_bad_utf8( char *const dst, const char *const src );

/* ------------------------------------------------------------------------ */
/*  PRINT_CFG_VAR        -- Print <name> = <value> tuple.                   */
/*  PRINT_CFG_VAR_LIST   -- Print a list of cfg_vars                        */
/* ------------------------------------------------------------------------ */
struct printer_t;  /* Forward decl to avoid more compile deps. */
void print_cfg_var
(
    cfg_var_t        *RESTRICT const var,
    struct printer_t *RESTRICT const p
);
void print_cfg_var_list
(
    cfg_var_t        *RESTRICT const head,
    struct printer_t *RESTRICT const p
);

/* ------------------------------------------------------------------------ */
/*  CONS_CFG_VAR_DEC     -- Construct a decimal config variable.            */
/*  CONS_CFG_VAR_HEX     -- Construct a hexadecimal config variable.        */
/*  CONS_CFG_VAR_STRING  -- Construct a string config variable.             */
/*  CONS_CFG_VAR_DATE    -- Construct a date config variable.               */
/* ------------------------------------------------------------------------ */
cfg_var_t *cons_cfg_var_dec
(
    const char *RESTRICT const name,
    const int32_t              value
);
cfg_var_t *cons_cfg_var_hex
(
    const char *RESTRICT const name,
    const uint32_t             value
);
cfg_var_t *cons_cfg_var_string
(
    const char *RESTRICT const name,
    const char *RESTRICT const value
);
cfg_var_t *cons_cfg_var_date
(
    const char        *RESTRICT const name,
    const game_date_t *RESTRICT const value
);

/* ------------------------------------------------------------------------ */
/*  CONS_CFG_VAR_KV_STR  -- Construct a config variable from a key=value    */
/*                          string (such as get packed in LUIGIs).          */
/*                                                                          */
/*  This function has to guess at the value type, whether it's a date,      */
/*  number, or generic string.  It's meant for reconstituting cfg_var_t's   */
/*  that were serialized under a "misc" category, as they had no other      */
/*  category at the time of serialization.                                  */
/* ------------------------------------------------------------------------ */
cfg_var_t *cons_cfg_var_kv_str
(
    const char *RESTRICT kv_str
);

/* ------------------------------------------------------------------------ */
/*  APPEND_CFG_VAR       -- Appends a new config var to a list.             */
/*                          Returns the appended var.                       */
/*                                                                          */
/*  Intended to be used as follows:                                         */
/*                                                                          */
/*      var = append_cfg_var( &head, cons_cfg_var_XXX( "name", val ) );     */
/*                                                                          */
/*  If the cons failes, 'var' will be NULL and the list remains unchanged.  */
/* ------------------------------------------------------------------------ */
cfg_var_t *append_cfg_var
(
    cfg_var_t *RESTRICT *const head,
    cfg_var_t           *RESTRICT const var
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
