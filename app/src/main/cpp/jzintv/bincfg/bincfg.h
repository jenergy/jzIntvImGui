#ifndef BINCFG_H_
#define BINCFG_H_

#include "misc/ll.h"
#include "misc/types.h"
#include "misc/printer.h"

/* ======================================================================== */
/*  BC_CFGFILE_T -- Struct that stores a parsed INTVPC .CFG file in a form  */
/*                  we can process.                                         */
/*                                                                          */
/*  The configuration file is actually interpreted as a sequence of         */
/*  commands for each section.  The [mapping], [preload], [bankswitch]      */
/*  and [memattr] sections configure the memory map and cause the primary   */
/*  binary image to be loaded into the memory map in some way.              */
/*                                                                          */
/*  The [vars] section produces a list of <name,value> tuples that can      */
/*  be interpreted prior to starting the game.  The [keys], [scrolllock],   */
/*  [capslock] and [numlock] sections are similar, only the 'name' and      */
/*  'value' pairs refer to key mappings in one of four different keymaps.   */
/*                                                                          */
/*  The [macro] section defines a sequence of operations the debugger       */
/*  shall perform at the "start of time."  At least, this was its original  */
/*  purpose.  Static tools can implement most of the commands -- they       */
/*  just can't execute the commands that have a temporal element.  My       */
/*  intention is to support simple "load" and "poke" macro sections, and    */
/*  not anything fancier.  In that context, [macro] is useful for patching  */
/*  a ROM, and so is processed after [mapping], [preload], [bankswitch]     */
/*  and [memattr].                                                          */
/*                                                                          */
/*  The BC_CFGFILE_T structure is built up of several other structures.     */
/* ======================================================================== */

#ifdef SMALLMEM
# define BC_MAXBIN   (1 << 16)   /* Maximum length .bin file */
#else
# define BC_MAXBIN   (1 << 20)   /* Maximum length .bin file */
#endif

/* ------------------------------------------------------------------------ */
/*  BC_MEMSPAN_T -- describes a span of memory to be configured.            */
/* ------------------------------------------------------------------------ */
#define BC_SPAN_R   (0x0001)    /* Readable                                 */
#define BC_SPAN_W   (0x0002)    /* Writeable                                */
#define BC_SPAN_N   (0x0004)    /* Narrow                                   */
#define BC_SPAN_B   (0x0008)    /* Bankswitched                             */
#define BC_SPAN_EP  (0x0040)    /* ECS-style Pageable.                      */
#define BC_SPAN_PL  (0x0010)    /* Load data into this segment              */
#define BC_SPAN_PK  (0x0020)    /* Poke a datum at s_addr. Datum in 'width' */
#define BC_SPAN_DEL (0x0080)    /* Internal: span can be deleted            */
#define BC_SPAN_ROM (BC_SPAN_R)
#define BC_SPAN_RAM (BC_SPAN_R | BC_SPAN_W)
#define BC_SPAN_WOM (BC_SPAN_W)

#define BC_SPAN_NOPAGE (-1)

typedef struct bc_memspan_t
{
    ll_t      l;                /* Linked list of memory spans.             */
    uint32_t  s_addr, e_addr;   /* Address range of the span.               */
    uint32_t  s_fofs, e_fofs;   /* Starting and ending file offset.         */
    uint32_t  flags;            /* Intellicart flags for span.              */
    int       width;            /* Width of span in bits (8...16)           */
    int       epage;            /* ECS Page Number (-1 for not-pageable).   */
    char     *f_name;           /* File to get span from (NULL if primary)  */
    uint16_t *data;             /* Actual data to put in memory.            */
} bc_memspan_t;


/* ------------------------------------------------------------------------ */
/*  BC_MEMATTR_PAGE_T -- Subset of bc_memspan_t used by grammar to capture  */
/*  memory attributes and ECS page number.  Used in both [mapping] and      */
/*  [memattr] sections.                                                     */
/* ------------------------------------------------------------------------ */
typedef struct bc_memattr_page_t
{
    uint32_t flags;             /* Intellicart flags for span.              */
    int      epage;             /* Width of span in bits (8...16)           */
    int      width;             /* ECS Page Number (-1 for not-pageable).   */
} bc_memattr_page_t;


/* ------------------------------------------------------------------------ */
/*  BC_MACRO_T   -- describes a macro entry.                                */
/*      BC_MAC_ADDR_T    -- arg struct for 'I'nspect, 'T'race, Runt'O'      */
/*      BC_MAC_LOAD_T    -- arg struct for 'L'oad command.                  */
/*      BC_MAC_POKE_T    -- arg struct for 'P'oke command.                  */
/*      BC_MAC_REG_T     -- arg struct for '0'..'7' register update cmds.   */
/*      BC_MAC_WATCH_T   -- arg struct for 'W'atch command.                 */
/*                                                                          */
/*  Note:  I collapse 'trace' and 'run to' into one command.  Oversight.    */
/* ------------------------------------------------------------------------ */
typedef struct bc_mac_addr_t
{
    uint16_t addr;
} bc_mac_addr_t;

typedef struct bc_mac_load_t
{
    char    *name;
    int     width;
    uint16_t addr;
} bc_mac_load_t;

typedef struct bc_mac_poke_t
{
    uint16_t addr;
    uint16_t value;
    int      epage;
} bc_mac_poke_t;

typedef struct bc_mac_reg_t
{
    uint16_t reg;
    uint16_t value;
} bc_mac_reg_t;

typedef struct bc_mac_watch_t
{
    char    *name;
    int     spans;
    uint16_t *addr;
} bc_mac_watch_t;

typedef enum bc_macro_cmd_t
{
    BC_MAC_NONE,
    BC_MAC_REG,
    BC_MAC_AHEAD,
    BC_MAC_BLANK,
    BC_MAC_INSPECT,
    BC_MAC_LOAD,
    BC_MAC_POKE,
    BC_MAC_RUN,
    BC_MAC_RUNTO,
    BC_MAC_TRACE,
    BC_MAC_VIEW,
    BC_MAC_WATCH,
    BC_MAC_ERROR
} bc_macro_cmd_t;

typedef struct bc_macro_t
{
    ll_t            l;              /* Linked list of macros.               */
    int             quiet;          /* quiet flag for this macro line.      */
    bc_macro_cmd_t  cmd;            /* What macro command is this?          */
    union
    {
        bc_mac_addr_t       inspect;
        bc_mac_load_t       load;
        bc_mac_poke_t       poke;
        bc_mac_reg_t        reg;
        bc_mac_addr_t       runto;
        bc_mac_watch_t      watch;
    } arg;
} bc_macro_t;



/* ------------------------------------------------------------------------ */
/*  BC_VARLIKE_T -- Structure for encapsulating a [var]-like section.       */
/* ------------------------------------------------------------------------ */
typedef enum   bc_varlike_types_t
{
    BC_VL_VARS,
    BC_VL_JOYSTICK,
    BC_VL_KEYS,
    BC_VL_CAPSLOCK,
    BC_VL_NUMLOCK,
    BC_VL_SCROLLLOCK
} bc_varlike_types_t;

typedef struct bc_varlike_t
{
    bc_varlike_types_t type;
    cfg_var_t          *vars;
} bc_varlike_t;

/* ------------------------------------------------------------------------ */
/*  BC_DIAG_T     -- List of errors/warnings encountered within the CFG.    */
/* ------------------------------------------------------------------------ */
typedef enum bc_diagtype_t
{
    BC_DIAG_ERROR,
    BC_DIAG_WARNING
} bc_diagtype_t;

typedef struct bc_diag_t
{
    ll_t            l;              /* We can have more than one error.     */
    bc_diagtype_t   type;           /* Warning or error?                    */
    int             line;           /* Line number (if any) of the error    */
    char            *sect;          /* Section error occurred in.           */
    char            *msg;           /* Textual error string.                */
} bc_diag_t;


/* ------------------------------------------------------------------------ */
/*  BC_CFGFILE_T  -- Top level structure that ties the others together.     */
/* ------------------------------------------------------------------------ */
struct metadata_t;                  /* Forward declaration.             */
typedef struct bc_cfgfile_t
{
    char            *cfgfile;       /* name of configuration file.          */
    char            *binfile;       /* name of primary binary file.         */

    bc_memspan_t    *span;          /* linked list of memory spans to load  */
    bc_macro_t      *macro;         /* linked list of macros to execute.    */
    cfg_var_t       *vars;          /* linked list of variable defs.        */
    cfg_var_t       *keys[4];       /* linked list of key mappings.         */
    cfg_var_t       *joystick;      /* linked list of joystick mappings.    */

    bc_diag_t       *diags;         /* linked list of errors/warnings       */

    /*  Optional decoded metadata from CFG variables. (May be NULL.)        */
    struct game_metadata_t *metadata; 
} bc_cfgfile_t;

/* ======================================================================== */
/*  BC_PARSED_CFG -- Interface between grammar and parser.                  */
/* ======================================================================== */
extern bc_cfgfile_t *bc_parsed_cfg;

#ifdef LZOE_H_
/* ======================================================================== */
/*  BC_PARSE_CFG  -- Invokes the lexer and grammar to parse the config.     */
/* ======================================================================== */
bc_cfgfile_t *bc_parse_cfg
(
    LZFILE *f,
    const char *const binfile,
    const char *const cfgfile
);
#endif

/* ======================================================================== */
/*  BC_READ_DATA  -- Reads ROM segments and attaches them to bc_cfgfile_t.  */
/*                                                                          */
/*  This pass will adjust, or remove and free memspans that are partially   */
/*  or completely outside the bounds of the associated binfile.             */
/* ======================================================================== */
int bc_read_data(bc_cfgfile_t *bc);

#ifndef BC_NODOMACRO /* BC_NODOMACRO if you don't want to interpret macros */
/* ======================================================================== */
/*  BC_DO_MACROS         -- Parse macros that are safe to do statically.    */
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
/*  Macros that are safe to tread as NOPs during this pass:                 */
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
int  bc_do_macros(bc_cfgfile_t *cfg, int partial_ok);
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

/* ======================================================================== */
/*  BC_FREE_MEMSPAN_T    -- Releases storage associated with bc_memspan_t.  */
/* ======================================================================== */
void bc_free_memspan_t(ll_t *l_mem, void *unused);

/* ======================================================================== */
/*  BC_FREE_MACRO_T      -- Releases storage associated with bc_macro_t.    */
/* ======================================================================== */
void bc_free_macro_t(ll_t *l_mac, void *unused);

/* ======================================================================== */
/*  BC_FREE_DIAG_T       -- Releases storage associated with bc_diag_t      */
/* ======================================================================== */
void bc_free_diag_t(ll_t *l_diag, void *unused);

/* ======================================================================== */
/*  BC_FREE_CFG   -- Release storage held by bc_cfgfile_t.                  */
/* ======================================================================== */
void bc_free_cfg(bc_cfgfile_t *cfg);
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
    printer_t       *RESTRICT const printer,
    const char      *RESTRICT const fname,
    const bc_diag_t *RESTRICT const diag,
    const int                       cmt
);

/* ======================================================================== */
/*  BC_PRINT_MACRO   -- Print the [macro] section                           */
/* ======================================================================== */
void bc_print_macro  (printer_t  *RESTRICT const printer, 
                      bc_macro_t *RESTRICT const mac);

/* ======================================================================== */
/*  BC_PRINT_VARLIKE -- Print [var],[keys],[joystick] sections              */
/* ======================================================================== */
void bc_print_varlike
(
    printer_t  *RESTRICT const printer,
    cfg_var_t  *RESTRICT const varlike,
    const char *RESTRICT const sectname
);

/* ======================================================================== */
/*  BC_PRINT_MEMSPAN -- Print out all the memory span information.          */
/* ======================================================================== */
void bc_print_memspan
(
    printer_t           *RESTRICT const printer,
    const bc_memspan_t  *RESTRICT const mem
);

/* ======================================================================== */
/*  BC_PRINT_CFG -- Chase through a bc_cfgfile_t structure and print out    */
/*                  what we find therein.                                   */
/* ======================================================================== */
void bc_print_cfg
(
    printer_t           *RESTRICT const printer,
    const bc_cfgfile_t  *RESTRICT const bc
);

#endif /* BC_NOPRINT */

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
