/* Grammar for parsing CFG files */

/*%defines*/
/*%verbose*/
%token-table
/*%name-prefix="bc_"*/
/*%file-prefix="bincfg/bincfg_grmr"*/
%error-verbose

%{
/* Clang doesn't like the unreachable code in Bison's generated output. */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif

#include "config.h"
#include "lzoe/lzoe.h"
#include "bincfg/bincfg.h"
#include "bincfg/bincfg_lex.h"
#include "misc/types.h"

static bc_diag_t *bc_diag_list = NULL;
static char bc_errbuf[1024];
static const char *bc_cursect = NULL;

static void yyerror(const char *msg);


static bc_diag_t bc_oom_error_struct =
{
    { NULL }, BC_DIAG_ERROR, 0, NULL, NULL
};


/* ------------------------------------------------------------------------ */
/*  BC_OOM_ERROR -- Out of memory error handling.                           */
/*  BC_CHKOOM    -- Check for out-of-memory condition.                      */
/*  BC_DIAG      -- Queue up a diagnostic message.                          */
/*                                                                          */
/*  I hate making these macros, but YYABORT expands to a 'goto' within the  */
/*  yyparse() function.                                                     */
/* ------------------------------------------------------------------------ */
#define BC_OOM_ERROR                                                        \
        do {                                                                \
            bc_oom_error_struct.line = bc_line_no;                          \
            bc_diag_list = &bc_oom_error_struct;                            \
            YYABORT;                                                        \
        } while (0)

#define BC_CHKOOM(m) do { if (!(m)) BC_OOM_ERROR; } while (0)

static void check_line(ll_t *l, void *opq)
{
    int *lineno = (int *)opq;
    bc_diag_t *diag = (bc_diag_t *)l;

    if (diag->line == *lineno)
        *lineno = -1;
}

#define BC_DIAG(diag, section, line_no, diagmsg)                            \
        do {                                                                \
            bc_diag_t *err;                                                 \
            int line_no_tmp = line_no;                                      \
                                                                            \
            bc_errbuf[sizeof(bc_errbuf)-1] = 0;                             \
                                                                            \
            if (yychar == TOK_ERROR_OOM)                                    \
                BC_OOM_ERROR;                                               \
                                                                            \
            LL_ACTON(bc_diag_list, check_line, (void*)&line_no_tmp);        \
                                                                            \
            if (line_no_tmp != -1)                                          \
            {                                                               \
                err = CALLOC(bc_diag_t, 1);                                 \
                BC_CHKOOM(err);                                             \
                                                                            \
                err->line = line_no;                                        \
                err->type = diag;                                           \
                err->sect = strdup(section);                                \
                err->msg  = strdup(diagmsg);                                \
                BC_CHKOOM(err->sect);                                       \
                BC_CHKOOM(err->msg);                                        \
                                                                            \
                LL_CONCAT(bc_diag_list, err, bc_diag_t);                    \
            }                                                               \
        } while (0)


static int bc_saved_tok = -1, bc_saved_lineno = 0, bc_dont_save = 0;
static const char *bc_saved_sec = NULL;

/* Ugh:  bison magic.  Moo. */
#define BC_TOK (bc_saved_tok < 0 || bc_saved_tok == YYEMPTY ? "<unknown>" : \
                yytname[yytranslate[bc_saved_tok]])

#define BC_SEC (bc_saved_sec ? bc_saved_sec : \
                bc_cursect   ? bc_cursect   : "<toplevel>")

#define S(s) bc_cursect = s

%}

%union
{
    int                 intv;
    char                *strv;
    bc_varlike_t        varlike;
    bc_varlike_types_t  varlike_type;
    cfg_var_t           *var_list;
    val_strnum_t        strnum;
    bc_mac_watch_t      mac_watch;
    bc_macro_t          macro;
    bc_macro_t          *macro_list;
    bc_memspan_t        *memspan_list;
    bc_cfgfile_t        *cfgfile;
    bc_memattr_page_t   memattr_page;
};

/* ------------------------------------------------------------------------ */
/*  These destructors should only end up getting called if we ignore a      */
/*  token in a grammar.  This could happen along an error path, for         */
/*  example.  The destructors are not strictly necessary in this app.       */
/*  DISABLED FOR NOW -- too much potential for mayhem.                      */
/* ------------------------------------------------------------------------ */
/*
%destructor { free($$); } string
%destructor { ll_free(&($$->vars.l));            } var_list var_rec;
%destructor { ll_free(&($$->vars.l)); free($$);  } varlike;
%destructor { if ($$.str_val) free($$->str_val);    } strnum;
%destructor { free($$.name); free($$.addr);         } watch_list watch_span watch_range watch_addr;
%destructor { bc_free_macro(&$$);                   } macro_line;
%destructor { bc_free_macro_list($$);               } macro_list macro_rec sec_macro;
%destructor { bc_free_memspan_list($$);             } memspan_list
*/


/* ------------------------------------------------------------------------ */
/*  Lots of tokens coming from the lexer.                                   */
/* ------------------------------------------------------------------------ */
%token TOK_SEC_BANKSWITCH
%token TOK_SEC_MAPPING
%token TOK_SEC_ECSBANK
%token TOK_SEC_MEMATTR
%token TOK_SEC_PRELOAD
%token TOK_SEC_MACRO
%token TOK_SEC_VARS
%token TOK_SEC_JOYSTICK
%token TOK_SEC_KEYS
%token TOK_SEC_CAPSLOCK
%token TOK_SEC_NUMLOCK
%token TOK_SEC_SCROLLLOCK
%token TOK_SEC_DISASM
%token TOK_SEC_VOICES
%token TOK_SEC_UNKNOWN
%token TOK_RAM
%token TOK_ROM
%token TOK_WOM
%token TOK_PAGE
%token TOK_MAC_QUIET
%token TOK_MAC_REG
%token TOK_MAC_AHEAD
%token TOK_MAC_BLANK
%token TOK_MAC_INSPECT
%token TOK_MAC_LOAD
%token TOK_MAC_RUN
%token TOK_MAC_POKE
%token TOK_MAC_RUNTO
%token TOK_MAC_TRACE
%token TOK_MAC_VIEW
%token TOK_MAC_WATCH
%token TOK_DECONLY
%token TOK_DEC
%token TOK_HEX
%token TOK_NAME
%token TOK_STRING
%token TOK_ERROR_BAD
%token <strv> TOK_ERROR_OOM

%type <intv>            mac_reg hex_num dec_num memattr_tok
%type <strv>            string ident
%type <varlike>         sec_varlike
%type <varlike_type>    varlike_head
%type <var_list>        var_list var_rec
%type <strnum>          strnum
%type <mac_watch>       watch_list watch_span watch_range watch_addr
%type <macro_list>      macro_list macro_rec sec_macro
%type <macro>           macro_line
%type <memspan_list>    sec_memspan 
%type <memspan_list>    sec_mapping mapping_list mapping_rec
%type <memspan_list>    sec_ecsbank ecsbank_list ecsbank_rec
%type <memspan_list>    sec_memattr memattr_list memattr_rec
%type <memspan_list>    sec_preload preload_list preload_rec
%type <memspan_list>    sec_banksw  banksw_list  banksw_rec
%type <memattr_page>    memattr_page

%start config_file

%%

/* ------------------------------------------------------------------------ */
/*  top-level rule to clean up and grab any accumulated errors/warnings.    */
/* ------------------------------------------------------------------------ */
config_file :   config 
            {
                /* Before the chicken or egg came the rooster */
                if (!bc_parsed_cfg)
                {
                    bc_parsed_cfg = CALLOC(bc_cfgfile_t, 1);
                    BC_CHKOOM(bc_parsed_cfg);
                    bc_cursect = NULL;
                }

                bc_parsed_cfg->diags = bc_diag_list;
                bc_diag_list  = NULL;
            }
            ;


/* ------------------------------------------------------------------------ */
/*  Config files are lists of sections, with possible lexical errors.       */
/* ------------------------------------------------------------------------ */
config      :   config sec_memspan
            {
                if ($2) 
                    LL_CONCAT(bc_parsed_cfg->span, $2, bc_memspan_t);
            }
            |   config sec_macro
            {
                if ($2) 
                    LL_CONCAT(bc_parsed_cfg->macro, $2, bc_macro_t);
            }
            |   config sec_varlike
            {
                if ($2.vars) switch ($2.type)
                {
                    case BC_VL_VARS:
                    {
                        LL_CONCAT(bc_parsed_cfg->vars, $2.vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_JOYSTICK:
                    {
                        LL_CONCAT(bc_parsed_cfg->joystick, $2.vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_KEYS:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[0], $2.vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_CAPSLOCK:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[1], $2.vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_NUMLOCK:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[2], $2.vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_SCROLLLOCK:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[3], $2.vars, cfg_var_t);
                        break;
                    }

                    default:
                    {
                        /* report the error? */
                        BC_DIAG(BC_DIAG_ERROR, 
                                bc_cursect ? bc_cursect : "<toplevel>", 
                                bc_line_no, 
                                "Internal error processing variable lists");
                        break;
                    }
                }
            }
            |   config sec_unsup  
            |   seed_config       
            |   error_tok
            {
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                         "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, BC_SEC,
                        bc_saved_lineno, bc_errbuf);
            }
            ;

/* ------------------------------------------------------------------------ */
/*  We support several different section types.                             */
/* ------------------------------------------------------------------------ */
seed_config :   /* empty */
            {
                /* Before the chicken or egg came the rooster */
                if (!bc_parsed_cfg)
                {
                    bc_parsed_cfg = CALLOC(bc_cfgfile_t, 1);
                    BC_CHKOOM(bc_parsed_cfg);
                    bc_cursect = NULL;
                }
            }
            |   eolns
            {
                if (!bc_parsed_cfg)
                {
                    bc_parsed_cfg = CALLOC(bc_cfgfile_t, 1);
                    BC_CHKOOM(bc_parsed_cfg);
                    bc_cursect = NULL;
                }
            }
            ;
            

/* ------------------------------------------------------------------------ */
/*  SEC_MEMSPAN:  All of the memory span-like sections aggregate here.      */
/* ------------------------------------------------------------------------ */
sec_memspan :   sec_banksw
            |   sec_mapping
            |   sec_ecsbank
            |   sec_memattr
            |   sec_preload
            ;

/* ------------------------------------------------------------------------ */
/*  SEC_BANKSW:  BANKSW sections only contain lists of bankswitched addrs.  */
/* ------------------------------------------------------------------------ */
banksw_head :   TOK_SEC_BANKSWITCH { S("[bankswitch]"); } ;
sec_banksw  :   banksw_head eoln banksw_list
            {
                $$ = LL_REVERSE($3, bc_memspan_t);
            }
            ;

banksw_list :   banksw_list banksw_rec
            {
                $$ = LL_INSERT($1, $2, bc_memspan_t);
            }
            |   banksw_rec         { $$ = $1;   }
            ;

banksw_rec  :   hex_num '-' hex_num eoln
            {
                $$ = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM($$);
                $$->s_fofs = 0;
                $$->e_fofs = 0;
                $$->s_addr = $1;
                $$->e_addr = $3;
                $$->flags  = BC_SPAN_B | BC_SPAN_R;
                $$->width  = 16;
                $$->epage  = BC_SPAN_NOPAGE;
                $$->f_name = NULL;
            }
            |  eoln
            {
                $$ = NULL;
            }
            |   error_tok eoln
            {
                $$ = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                         "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[bankswitch]", 
                        bc_saved_lineno, bc_errbuf);
            }
            ;

/* ------------------------------------------------------------------------ */
/*  SEC_MAPPING: MAPPING sections contain file ranges that map to address   */
/*               ranges in the Intellivision and Intellicart memory maps.   */
/* ------------------------------------------------------------------------ */
mapping_head:   TOK_SEC_MAPPING { S("[mapping]"); } ;
sec_mapping :   mapping_head eoln mapping_list
            {
                $$ = LL_REVERSE($3, bc_memspan_t);
            }
            ;

mapping_list:   mapping_list mapping_rec
            {
                $$ = LL_INSERT($1, $2, bc_memspan_t);
            }
            |   mapping_rec { $$ = $1; }
            ;


mapping_rec :   hex_num '-' hex_num '=' hex_num memattr_page
            {
                $$ = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM($$);
                $$->s_fofs = $1;
                $$->e_fofs = $3;
                $$->s_addr = $5;
                $$->e_addr = $5 + $3 - $1;
                $$->flags  = $6.flags | BC_SPAN_PL;
                $$->width  = $6.width;
                $$->epage  = $6.epage;
                $$->f_name = NULL;
            }
            |   eoln
            {
                $$ = NULL;
            }
            |   error_tok eoln
            {
                $$ = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[mapping]", 
                        bc_saved_lineno, bc_errbuf);
            }
            ;

/* ------------------------------------------------------------------------ */
/*  SEC_ECSBANK: ECSBANK sections look a lot like MAPPING, except they only */
/*               contain ECS-style bankswitched ROMs segments.  The syntax  */
/*               is also different.                                         */
/* ------------------------------------------------------------------------ */
ecsbank_head:   TOK_SEC_ECSBANK { S("[ecsbank]"); } ;
sec_ecsbank :   ecsbank_head eoln ecsbank_list
            {
                $$ = LL_REVERSE($3, bc_memspan_t);
            }
            ;

ecsbank_list:   ecsbank_list ecsbank_rec
            {
                $$ = LL_INSERT($1, $2, bc_memspan_t);
            }
            |   ecsbank_rec { $$ = $1; }
            ;


ecsbank_rec :   hex_num ':' hex_num '-' hex_num '=' hex_num eoln
            {
                $$ = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM($$);
                $$->s_fofs = $3;
                $$->e_fofs = $5;
                $$->s_addr = $7;
                $$->e_addr = $7 + $5 - $3;
                $$->flags  = BC_SPAN_PL | BC_SPAN_R | BC_SPAN_EP;
                $$->width  = 16;
                $$->epage  = $1;
                $$->f_name = NULL;
            }
            |   eoln
            {
                $$ = NULL;
            }
            |   error_tok eoln
            {
                $$ = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[ecsbank]", 
                        bc_saved_lineno, bc_errbuf);
            }
            ;


/* ------------------------------------------------------------------------ */
/*  SEC_MEMATTR: MEMATTR sections define regions of memory as RAM or WOM.   */
/* ------------------------------------------------------------------------ */
memattr_head:   TOK_SEC_MEMATTR { S("[memattr]"); } ;
sec_memattr :   memattr_head eoln memattr_list
            {
                $$ = LL_REVERSE($3, bc_memspan_t);
            }
            ;


memattr_list:   memattr_list memattr_rec 
            {
                $$ = LL_INSERT($1, $2, bc_memspan_t);
            }
            |   memattr_rec { $$ = $1; }
            ;

memattr_rec :   hex_num '-' hex_num '=' memattr_page
            {
                $$ = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM($$);
                $$->s_fofs = 0;
                $$->e_fofs = 0;
                $$->s_addr = $1;
                $$->e_addr = $3;
                $$->flags  = $5.flags;
                $$->width  = $5.width;
                $$->epage  = $5.epage;
                $$->f_name = NULL;
            }
            |   eoln
            {   
                $$ = NULL;
            }
            |   error_tok eoln
            {
                $$ = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'\n", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[memattr]", 
                        bc_saved_lineno, bc_errbuf);
            }
            ;

memattr_page:   eoln
            {
                $$.flags = BC_SPAN_R;
                $$.width = 16;
                $$.epage = BC_SPAN_NOPAGE;
            }
            |   memattr_tok dec_num memattr_page
            {
                $$.flags =  $1 
                         | ($2 < 16 ? BC_SPAN_N : 0)
                         | ($3.flags & BC_SPAN_EP);
                $$.width = $2;
                $$.epage = $3.epage;
            }
            |   TOK_PAGE hex_num memattr_page
            {
                $$.flags = $3.flags | BC_SPAN_EP;
                $$.width = $3.width;
                $$.epage = $2;
            }
            ;


/* ------------------------------------------------------------------------ */
/*  SEC_PRELOAD: PRELOAD sections initialize Intellicart address ranges     */
/*               without necessarily implying a mapping in Intellivision    */
/*               address space.                                             */
/* ------------------------------------------------------------------------ */
preload_head:   TOK_SEC_PRELOAD { S("[preload]"); } ;
sec_preload :   preload_head eoln preload_list
            {
                $$ = LL_REVERSE($3, bc_memspan_t);
            }
            ;

preload_list:   preload_list preload_rec
            {
                $$ = LL_INSERT($1, $2, bc_memspan_t);
            }
            |   preload_rec { $$ = $1; }
            ;

preload_rec :   hex_num '-' hex_num '=' hex_num eoln
            {
                $$ = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM($$);
                $$->s_fofs = $1;
                $$->e_fofs = $3;
                $$->s_addr = $5;
                $$->e_addr = $5 + $3 - $1;
                $$->flags  = BC_SPAN_PL;
                $$->width  = 16;
                $$->epage  = BC_SPAN_NOPAGE;
                $$->f_name = NULL;
            }
            |   error_tok eoln
            {
                $$ = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[preload]", 
                        bc_saved_lineno, bc_errbuf);
            }
            |   eoln
            {
                $$ = NULL;
            }
            ;

/* ------------------------------------------------------------------------ */
/*  SEC_MACRO:  MACRO sections define key sequences that INTVPC's built-in  */
/*              debugger would execute.  We implement an ersatz interpreter */
/*              that is sufficient to apply ROM patches.                    */
/*                                                                          */
/*  Several different macro commands are parsed, even if we can't support   */
/*  all of them in the typical case.                                        */
/*                                                                          */
/*    [0-7] <value>             Set register <n> to <value>                 */
/*      A                       Shows the instructions 'A'head.             */
/*      B                       Run until next vertical 'B'lank.            */
/*      I <addr>                'I'nspect data/code at <addr>.              */
/*      L <file> <width> <addr> 'L'oad <file> of <width> at <addr>.         */
/*      P <addr> <data>         'P'oke <data> at <addr>.                    */
/*      R                       'R'un emulation.                            */
/*      T <addr>                'T'race to <addr>.                          */
/*      O <addr>                Run t'O' <addr>.                            */
/*      V                       'V'iew emulation display                    */
/*      W <name> <list>         'W'atch a set of values with label <name>.  */
/*                                                                          */
/* ------------------------------------------------------------------------ */
macro_head  :   TOK_SEC_MACRO { S("[macro]"); } ;
sec_macro   :   macro_head eoln macro_list
            {
                $$ = LL_REVERSE($3, bc_macro_t);
            }
            ;

macro_list  :   macro_list macro_rec
            {
                $$ = LL_INSERT($1, $2, bc_macro_t);
            }
            |   macro_rec { $$ = $1; }
            ;



macro_rec   :   TOK_MAC_QUIET macro_line
            {
                if ($2.cmd == BC_MAC_ERROR)
                    $$ = NULL;
                else
                {
                    $$  = CALLOC(bc_macro_t, 1);
                    BC_CHKOOM($$);
                    *$$ = $2;
                    $$->quiet = 1;
                }
            }
            |   macro_line
            {
                if ($1.cmd == BC_MAC_ERROR)
                    $$ = NULL;
                else
                {
                    $$  = CALLOC(bc_macro_t, 1);
                    BC_CHKOOM($$);
                    *$$ = $1;
                    $$->quiet = 0;
                }
            }
            |   TOK_MAC_QUIET eoln
            {
                $$ = NULL;
            }
            |   eoln
            {
                $$ = NULL;
            }
            ;

macro_line  :   mac_reg hex_num eoln
            {
                $$.cmd            = BC_MAC_REG;
                $$.arg.reg.reg    = $1;
                $$.arg.reg.value  = $2;
            }
            |   TOK_MAC_LOAD string dec_num hex_num eoln
            {
                $$.cmd            = BC_MAC_LOAD;
 //fprintf(stderr, "name = %s\n", $2);
                $$.arg.load.name  = $2;
                $$.arg.load.width = $3;
                $$.arg.load.addr  = $4;
            }
            |   TOK_MAC_WATCH ident watch_list eoln
            {
                $$.cmd            = BC_MAC_WATCH;
                $$.arg.watch      = $3;
                $$.arg.watch.name = $2;
            }
            |   TOK_MAC_POKE hex_num hex_num eoln
            {
                $$.cmd = BC_MAC_POKE;
                $$.arg.poke.addr  = $2;
                $$.arg.poke.epage = BC_SPAN_NOPAGE;
                $$.arg.poke.value = $3;
            }
            |   TOK_MAC_POKE hex_num TOK_PAGE hex_num hex_num eoln
            {
                $$.cmd = BC_MAC_POKE;
                $$.arg.poke.addr  = $2;
                $$.arg.poke.epage = $4;
                $$.arg.poke.value = $5;
            }
            |   TOK_MAC_POKE hex_num ':' hex_num hex_num eoln
            {
                $$.cmd = BC_MAC_POKE;
                $$.arg.poke.addr  = $2;
                $$.arg.poke.epage = $4;
                $$.arg.poke.value = $5;
            }
            |   TOK_MAC_INSPECT hex_num eoln
            {
                $$.cmd = BC_MAC_INSPECT;
                $$.arg.inspect.addr = $2;
            }
            |   TOK_MAC_RUNTO hex_num eoln
            {
                $$.cmd = BC_MAC_RUNTO;
                $$.arg.runto.addr = $2;
            }
            |   TOK_MAC_TRACE hex_num eoln
            {
                $$.cmd = BC_MAC_TRACE;
                $$.arg.runto.addr = $2;
            }
            |   TOK_MAC_AHEAD eoln  {   $$.cmd  = BC_MAC_AHEAD;     }
            |   TOK_MAC_BLANK eoln  {   $$.cmd  = BC_MAC_BLANK;     }
            |   TOK_MAC_RUN   eoln  {   $$.cmd  = BC_MAC_RUN;       }
            |   TOK_MAC_VIEW  eoln  {   $$.cmd  = BC_MAC_VIEW;      }
            |   error_tok eoln
            {
                $$.cmd = BC_MAC_ERROR;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_WARNING, "[macro]", 
                        bc_saved_lineno, bc_errbuf);
            }
            ;

mac_reg     :   TOK_MAC_REG { $$ = bc_hex; } 
            ;

/* ------------------------------------------------------------------------ */
/*  WATCH_LIST:  These states assemble a watch list.  It's kinda messy.     */
/* ------------------------------------------------------------------------ */
watch_list  :   watch_list ',' watch_span
            {
                size_t new_size;
                int new_spans, i;

                new_spans = $1.spans + $3.spans;
                new_size  = new_spans*sizeof(uint16_t)*2;

                $$.addr  = (uint16_t *)realloc($1.addr, new_size);
                $$.spans = new_spans;
                BC_CHKOOM($$.addr);

                for (i = 0; i < $3.spans*2; i++)
                    $$.addr[i + 2*$1.spans] = $3.addr[i];

                free($3.addr);
                $3.addr = NULL;
                $1.addr = NULL;
            }
            |   watch_span  { $$ = $1; }
            ;

watch_span  :   watch_addr
            |   watch_range
            ;

watch_addr  :   hex_num
            {
                $$.spans   = 1;
                $$.addr    = CALLOC(uint16_t, 2); BC_CHKOOM($$.addr);
                $$.addr[0] = $1;
                $$.addr[1] = $1;
            }
            ;

watch_range :   hex_num '-' hex_num
            {
                $$.spans   = 1;
                $$.addr    = CALLOC(uint16_t, 2); BC_CHKOOM($$.addr);
                $$.addr[0] = $1;
                $$.addr[1] = $3;
            }
            ;

/* ------------------------------------------------------------------------ */
/*  SEC_VARLIKE:  Captures all the "foo = bar" sections.  This includes     */
/*                the variable definitions and the key mapping sections.    */
/*                                                                          */
/*  VARLIKE_HEAD tracks what section type this actually is.  The list that  */
/*  we produce in of the form of "name = value" tuples.  The names are      */
/*  always interpreted as strings.  The values are semi-lazily interpreted: */
/*  we keep track of their string interpretation, as well as their numeric  */
/*  values, if they have any.  It's up to the recipient of the data to      */
/*  decide if the right data types are being handed about.                  */
/* ------------------------------------------------------------------------ */
sec_varlike :   varlike_head eoln var_list
            {
                $$.type = $1;
                $$.vars = LL_REVERSE($3, cfg_var_t);
            }
            ;

varlike_head:   TOK_SEC_VARS       { $$=BC_VL_VARS;       S("[vars]"      ); }
            |   TOK_SEC_JOYSTICK   { $$=BC_VL_JOYSTICK;   S("[joystick]"  ); }
            |   TOK_SEC_KEYS       { $$=BC_VL_KEYS;       S("[keys]"      ); }
            |   TOK_SEC_CAPSLOCK   { $$=BC_VL_CAPSLOCK;   S("[capslock]"  ); }
            |   TOK_SEC_NUMLOCK    { $$=BC_VL_NUMLOCK;    S("[numlock]"   ); }
            |   TOK_SEC_SCROLLLOCK { $$=BC_VL_SCROLLLOCK; S("[scrolllock]"); }
            ;

var_list    :   var_list var_rec
            {
                $$ = LL_INSERT($1, $2, cfg_var_t);
            }
            |   var_rec { $$ = $1; }
            ;

var_rec     :   ident '=' strnum eoln
            {
                $$ = CALLOC(cfg_var_t, 1);
                BC_CHKOOM($$);

                $$->name = $1;
                $$->val  = $3;
            }
            |   eoln { $$ = NULL; }
            |   error_tok eoln
            {
                $$ = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'\n", BC_TOK);
                BC_DIAG(BC_DIAG_WARNING, "[vars]", 
                        bc_saved_lineno, bc_errbuf);
            }
            ;

/* ------------------------------------------------------------------------ */
/*  SEC_UNSUP:  Clean up the remaining section types that we might have     */
/*              heard of, but don't do anything valuable with.              */
/* ------------------------------------------------------------------------ */
sec_unsup   :   unknown_head eoln ;

unknown_head:   TOK_SEC_DISASM   { S("[disasm]"); }
            |   TOK_SEC_VOICES   { S("[voices]"); }
            |   TOK_SEC_UNKNOWN
            {
                static char bc_unknown[256];

                if (bc_txt)
                {
                    strncpy(bc_unknown, bc_txt, 255);
                    bc_unknown[255] = 0;
                }

                S(bc_unknown);
            }
            ;



/* ------------------------------------------------------------------------ */
/*  Helper states.  These are mostly here to deal with ambiguous tokens,    */
/*  or in the case of MEMATTR, to provide a wild card.                      */
/*                                                                          */
/*  STRNUM  :   Token that could be a string or a number -- we won't know   */
/*              until the emulator gets the value.                          */
/*                                                                          */
/*  DEC_NUM :   Token that (by context) is unambiguously a decimal number.  */
/*                                                                          */
/*  HEX_NUM :   Token that (by context) is unambiguously a hex number.      */
/*                                                                          */
/*  STRING  :   Token that (by context) is unambiguously a string, and may  */
/*              be in quotes.                                               */
/*                                                                          */
/*  IDENT   :   Token that (by context) is unambiguously a string, and may  */
/*              not be in quotes.                                           */
/*                                                                          */
/*  MEMATTR :   RAM, ROM or WOM.                                            */
/*                                                                          */
/*  An example of an ambiguous token would be the text "100".  The lexer    */
/*  returns this as "TOK_DEC."  But, it could be the decimal value 100,     */
/*  the hex value $100, or the string "100", depending on where it is in    */
/*  an operand.  For instance, consider the following 3 config snippets:    */
/*                                                                          */
/*  [mapping]                                                               */
/*  100 - 0FFF = 5100  ; offset $100 thru $0FFF map to $5100 thru $5FFF.    */
/*                                                                          */
/*  [vars]                                                                  */
/*  handdelay = 100    ; delay for reading hand controllers on LPT.         */
/*  inp = 100          ; read ECS text input from file "100".               */
/* ------------------------------------------------------------------------ */
strnum      :   TOK_NAME
            {
                $$.flag    = VAL_STRING;
                $$.str_val = strdup(bc_txt);  BC_CHKOOM($$.str_val);
                $$.dec_val = 0;
                $$.hex_val = 0;
                val_try_parse_date( &($$) );
            }
            |   TOK_STRING
            {
                $$.flag    = VAL_STRING;
                $$.str_val = strdup(bc_txt);  BC_CHKOOM($$.str_val);
                $$.dec_val = 0;
                $$.hex_val = 0;
                val_try_parse_date( &($$) );
            }
            |   TOK_HEX
            {
                $$.flag    = VAL_STRING | VAL_HEXNUM;
                $$.str_val = strdup(bc_txt);  BC_CHKOOM($$.str_val);
                $$.dec_val = 0;
                $$.hex_val = bc_hex;
            }
            |   TOK_DEC
            {
                $$.flag    = VAL_STRING | VAL_HEXNUM | VAL_DECNUM;
                $$.str_val = strdup(bc_txt);  BC_CHKOOM($$.str_val);
                $$.dec_val = bc_dec;
                $$.hex_val = bc_hex;

                /* It *may* be a year, so try putting it in date too */
                if ( bc_dec > 0 && bc_dec < 100 )
                {
                    $$.flag |= VAL_DATE;
                    $$.date_val.year = bc_dec + 1900;
                } else if ( bc_dec > 1900 && bc_dec < 1900 + 255 )
                {
                    $$.flag |= VAL_DATE;
                    $$.date_val.year = bc_dec;
                }
            }
            |   TOK_DECONLY
            {
                $$.flag    = VAL_STRING | VAL_DECNUM;
                $$.str_val = strdup(bc_txt);  BC_CHKOOM($$.str_val);
                $$.dec_val = bc_dec;
                $$.hex_val = 0;
            }
            ;

dec_num     :   TOK_DEC     { $$ = bc_dec; }
            |   TOK_DECONLY { $$ = bc_dec; }
            ;

hex_num     :   TOK_HEX     { $$ = bc_hex; }
            |   TOK_DEC     { $$ = bc_hex; }
            ;

ident       :   TOK_NAME    { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            |   TOK_HEX     { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            |   TOK_DEC     { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            |   TOK_DECONLY { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            ;

string      :   TOK_NAME    { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            |   TOK_STRING  { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            |   TOK_HEX     { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            |   TOK_DEC     { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            |   TOK_DECONLY { $$ = strdup(bc_txt); BC_CHKOOM($$); }
            ;

memattr_tok :   TOK_RAM     { $$ = BC_SPAN_RAM; }
            |   TOK_ROM     { $$ = BC_SPAN_ROM; }
            |   TOK_WOM     { $$ = BC_SPAN_WOM; }
            ;

eolns       :   eolns eoln
            |   eoln
            ;

eoln        :   '\n'        { bc_dont_save = 0; bc_saved_tok = YYEMPTY; }
            ;

error_tok   :   error
            {
                if (!bc_dont_save)
                {
                    if (yychar > 0 && yychar != YYEMPTY)
                    {
                        bc_dont_save    = 1;
                        bc_saved_tok    = yychar;
                        bc_saved_lineno = bc_line_no;
                        bc_saved_sec    = bc_cursect ? bc_cursect : "<toplevel>";
                    }
                }
            }
            ;
%%

/* ------------------------------------------------------------------------ */
/*  YYERROR -- required by Bison/YACC.                                      */
/* ------------------------------------------------------------------------ */
static void yyerror(const char *diagmsg)
{
    bc_diag_t *err;
    const char *cursect = bc_cursect ? bc_cursect : "<internal>";

    err = CALLOC(bc_diag_t, 1);
    if (!err) return;

    if (bc_txt && yychar == TOK_ERROR_BAD)
        snprintf(bc_errbuf, sizeof(bc_errbuf),
                "%s.  Text at error: '%s'", diagmsg, bc_txt);
    else
        snprintf(bc_errbuf, sizeof(bc_errbuf), "%s.", diagmsg);

    err->line = bc_line_no;
    err->type = BC_DIAG_ERROR;
    err->sect = strdup(cursect);    if (!err->sect) goto oom;
    err->msg  = strdup(bc_errbuf);  if (!err->msg ) goto oom;

    LL_CONCAT(bc_diag_list, err, bc_diag_t);
    return;
oom:
    if (err)
    {
        CONDFREE(err->msg);
        CONDFREE(err->sect);
    }
    CONDFREE(err);
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
/*                 Copyright (c) 2003-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
