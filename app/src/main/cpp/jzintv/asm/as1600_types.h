#ifndef TYPES_H_
#define TYPES_H_

typedef enum { LIST_ON, LIST_CODE, LIST_OFF, LIST_PREV } listing_mode;
typedef enum { WARNING, ERROR } warnerr;

typedef enum
{
    REC_INVALID = 0,
    REC_LIST_MODE,
    REC_ERROR,
    REC_LIST_LINE,
    REC_SET_EQU,
    REC_COMMENT,
    REC_USER_COMMENT,
    REC_LOC_SET,
    REC_RESERVE_RANGE,
    REC_MARK_RANGE,
    REC_DATA_BLOCK,
    REC_FILE_START,
    REC_FILE_EXIT,
    REC_CFGVAR_INT,
    REC_CFGVAR_STR,
    REC_SRCFILE_OVER,
    REC_AS1600_CFG,
    REC_LISTING_COLUMN,
    REC_OVERWRITE
} irec_type;

typedef enum
{
    ACFG_INVALID = 0,
    ACFG_LIST_SRC_COLUMN,   /* Column to print source code in in listing    */
    ACFG_LIST_HEX_SOURCE,   /* Numb. of hex vals to list next to source     */
    ACFG_LIST_HEX_PER_LINE, /* Numb. of hex vals to list per line otheriwse */
} as1600_cfg_item;

typedef struct intermed_rec intermed_rec;

struct intermed_rec
{
    int             line : 24;
    irec_type       type :  8;
};

typedef struct irec_string
{
    intermed_rec    irec;
    const char      *string;
} irec_string;

typedef struct irec_list_mode
{
    intermed_rec    irec;
    listing_mode    mode;
} irec_list_mode;

typedef struct irec_set_equ
{
    intermed_rec    irec;
    unsigned int    value;
} irec_set_equ;

typedef struct irec_loc_set
{
    intermed_rec    irec;
    int             type;
    int             seg;
    int             pag;
    int             loc;
    const char      *mode;
} irec_loc_set;

typedef struct irec_mark_range
{
    intermed_rec    irec;
    const char      *mode;
    int             lo;
    int             hi;
    int             pag;
} irec_mark_range;

typedef struct irec_reserve_range
{
    intermed_rec    irec;
    int             endaddr;
} irec_reserve_range;

typedef struct irec_cfgvar_int
{
    intermed_rec    irec;
    const char      *var;
    int             value;
} irec_cfgvar_int;

typedef struct irec_cfgvar_str
{
    intermed_rec    irec;
    const char      *var;
    const char      *value;
} irec_cfgvar_str;

typedef struct irec_srcfile_over
{
    intermed_rec    irec;
    const char      *file;
    int             line;
} irec_srcfile_over;

typedef struct irec_as1600_cfg
{
    intermed_rec    irec;
    as1600_cfg_item item;
    int             int_value;
} irec_as1600_cfg;

typedef struct irec_listing_column
{
    intermed_rec    irec;
    int             hex_source;
    int             hex_no_src;
    int             source_col;
} irec_listing_column;

typedef struct irec_overwrite
{
    intermed_rec    irec;
    int             err_if_overwritten;     /* -1 means "no change" */
    int             force_overwrite;        /* -1 means "no change" */
} irec_overwrite;

typedef union irec_union
{
    intermed_rec        irec;
    irec_string         string;
    irec_set_equ        set_equ;
    irec_list_mode      list_mode;
    irec_loc_set        loc_set;
    irec_mark_range     mark_range;
    irec_reserve_range  reserve_range;
    irec_cfgvar_int     cfgvar_int;
    irec_cfgvar_str     cfgvar_str;
    irec_srcfile_over   srcfile_over;
    irec_as1600_cfg     as1600_cfg;
    irec_listing_column listing_column;
    irec_overwrite      overwrite;
} irec_union;

#endif
