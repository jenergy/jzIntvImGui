/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         bc_parse
#define yylex           bc_lex
#define yyerror         bc_error
#define yydebug         bc_debug
#define yynerrs         bc_nerrs

#define yylval          bc_lval
#define yychar          bc_char

/* Copy the first part of user declarations.  */
#line 10 "bincfg/bincfg_grmr_real.y" /* yacc.c:339  */

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


#line 168 "bincfg/bincfg_grmr.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "bincfg_grmr.tab.h".  */
#ifndef YY_BC_BINCFG_BINCFG_GRMR_TAB_H_INCLUDED
# define YY_BC_BINCFG_BINCFG_GRMR_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int bc_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOK_SEC_BANKSWITCH = 258,
    TOK_SEC_MAPPING = 259,
    TOK_SEC_ECSBANK = 260,
    TOK_SEC_MEMATTR = 261,
    TOK_SEC_PRELOAD = 262,
    TOK_SEC_MACRO = 263,
    TOK_SEC_VARS = 264,
    TOK_SEC_JOYSTICK = 265,
    TOK_SEC_KEYS = 266,
    TOK_SEC_CAPSLOCK = 267,
    TOK_SEC_NUMLOCK = 268,
    TOK_SEC_SCROLLLOCK = 269,
    TOK_SEC_DISASM = 270,
    TOK_SEC_VOICES = 271,
    TOK_SEC_UNKNOWN = 272,
    TOK_RAM = 273,
    TOK_ROM = 274,
    TOK_WOM = 275,
    TOK_PAGE = 276,
    TOK_MAC_QUIET = 277,
    TOK_MAC_REG = 278,
    TOK_MAC_AHEAD = 279,
    TOK_MAC_BLANK = 280,
    TOK_MAC_INSPECT = 281,
    TOK_MAC_LOAD = 282,
    TOK_MAC_RUN = 283,
    TOK_MAC_POKE = 284,
    TOK_MAC_RUNTO = 285,
    TOK_MAC_TRACE = 286,
    TOK_MAC_VIEW = 287,
    TOK_MAC_WATCH = 288,
    TOK_DECONLY = 289,
    TOK_DEC = 290,
    TOK_HEX = 291,
    TOK_NAME = 292,
    TOK_STRING = 293,
    TOK_ERROR_BAD = 294,
    TOK_ERROR_OOM = 295
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 105 "bincfg/bincfg_grmr_real.y" /* yacc.c:355  */

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

#line 264 "bincfg/bincfg_grmr.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE bc_lval;

int bc_parse (void);

#endif /* !YY_BC_BINCFG_BINCFG_GRMR_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 281 "bincfg/bincfg_grmr.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   565

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  51
/* YYNRULES -- Number of rules.  */
#define YYNRULES  123
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  208

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   295

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      45,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    44,    41,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    43,     2,
       2,    42,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   204,   204,   223,   228,   233,   284,   285,   286,   299,
     308,   323,   324,   325,   326,   327,   333,   334,   340,   344,
     347,   360,   364,   378,   379,   385,   389,   393,   406,   410,
     425,   426,   432,   436,   440,   453,   457,   471,   472,   479,
     483,   486,   499,   503,   513,   519,   527,   541,   542,   548,
     552,   555,   568,   576,   603,   604,   610,   614,   619,   631,
     643,   647,   653,   659,   667,   673,   680,   687,   694,   699,
     704,   709,   710,   711,   712,   713,   723,   729,   748,   751,
     752,   755,   764,   784,   791,   792,   793,   794,   795,   796,
     799,   803,   806,   814,   815,   829,   831,   832,   833,   880,
     888,   896,   903,   921,   930,   931,   934,   935,   938,   939,
     940,   941,   944,   945,   946,   947,   948,   951,   952,   953,
     956,   957,   960,   963
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_SEC_BANKSWITCH", "TOK_SEC_MAPPING",
  "TOK_SEC_ECSBANK", "TOK_SEC_MEMATTR", "TOK_SEC_PRELOAD", "TOK_SEC_MACRO",
  "TOK_SEC_VARS", "TOK_SEC_JOYSTICK", "TOK_SEC_KEYS", "TOK_SEC_CAPSLOCK",
  "TOK_SEC_NUMLOCK", "TOK_SEC_SCROLLLOCK", "TOK_SEC_DISASM",
  "TOK_SEC_VOICES", "TOK_SEC_UNKNOWN", "TOK_RAM", "TOK_ROM", "TOK_WOM",
  "TOK_PAGE", "TOK_MAC_QUIET", "TOK_MAC_REG", "TOK_MAC_AHEAD",
  "TOK_MAC_BLANK", "TOK_MAC_INSPECT", "TOK_MAC_LOAD", "TOK_MAC_RUN",
  "TOK_MAC_POKE", "TOK_MAC_RUNTO", "TOK_MAC_TRACE", "TOK_MAC_VIEW",
  "TOK_MAC_WATCH", "TOK_DECONLY", "TOK_DEC", "TOK_HEX", "TOK_NAME",
  "TOK_STRING", "TOK_ERROR_BAD", "TOK_ERROR_OOM", "'-'", "'='", "':'",
  "','", "'\\n'", "$accept", "config_file", "config", "seed_config",
  "sec_memspan", "banksw_head", "sec_banksw", "banksw_list", "banksw_rec",
  "mapping_head", "sec_mapping", "mapping_list", "mapping_rec",
  "ecsbank_head", "sec_ecsbank", "ecsbank_list", "ecsbank_rec",
  "memattr_head", "sec_memattr", "memattr_list", "memattr_rec",
  "memattr_page", "preload_head", "sec_preload", "preload_list",
  "preload_rec", "macro_head", "sec_macro", "macro_list", "macro_rec",
  "macro_line", "mac_reg", "watch_list", "watch_span", "watch_addr",
  "watch_range", "sec_varlike", "varlike_head", "var_list", "var_rec",
  "sec_unsup", "unknown_head", "strnum", "dec_num", "hex_num", "ident",
  "string", "memattr_tok", "eolns", "eoln", "error_tok", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,    45,    61,    58,    44,    10
};
# endif

#define YYPACT_NINF -150

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-150)))

#define YYTABLE_NINF -84

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       7,  -150,  -150,     1,   406,  -150,   -36,  -150,  -150,  -150,
    -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,
    -150,  -150,  -150,  -150,  -150,  -150,   -36,  -150,   -36,  -150,
     -36,  -150,   -36,  -150,   -36,  -150,   -36,  -150,  -150,   -36,
    -150,   -36,  -150,    24,    24,    24,    24,    24,   301,    78,
    -150,  -150,  -150,   348,  -150,   -14,  -150,   -36,   391,  -150,
     -10,  -150,   -36,   434,  -150,    -8,  -150,   -36,   477,  -150,
      -1,  -150,   -36,   520,  -150,    15,  -150,   -36,   343,  -150,
     -36,   -36,     2,    82,   -36,     2,     2,     2,   -36,    57,
     262,  -150,  -150,     2,  -150,   -36,  -150,  -150,  -150,  -150,
     305,  -150,    22,  -150,   -36,  -150,     2,  -150,  -150,     2,
    -150,  -150,     2,  -150,  -150,     2,  -150,  -150,     2,  -150,
    -150,  -150,  -150,  -150,   -36,  -150,  -150,  -150,  -150,  -150,
      20,  -150,    40,   -36,   -36,  -150,     2,  -150,   -36,  -150,
    -150,   124,  -150,   -36,    28,    30,    32,    43,  -150,  -150,
    -150,     2,     2,     2,   -36,  -150,  -150,    21,  -150,  -150,
    -150,    41,  -150,  -150,  -150,  -150,  -150,  -150,   -36,  -150,
       2,     2,   -16,     2,   -36,     2,     2,  -150,     2,  -150,
       2,  -150,   -16,    44,  -150,  -150,  -150,     2,  -150,    20,
    -150,   -36,  -150,   -36,   -36,  -150,  -150,  -150,     2,   -16,
     -16,  -150,  -150,  -150,   -36,  -150,  -150,  -150
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,   123,   122,     0,     2,     7,    10,   121,     8,     1,
      16,    23,    30,    37,    47,    54,    84,    85,    86,    87,
      88,    89,    96,    97,    98,     3,     0,    11,     0,    12,
       0,    13,     0,    14,     0,    15,     0,     4,     5,     0,
       6,     0,   120,     0,     0,     0,     0,     0,     0,     0,
      95,   107,   106,     0,    19,     0,    21,     0,     0,    26,
       0,    28,     0,     0,    33,     0,    35,     0,     0,    40,
       0,    42,     0,     0,    50,     0,    53,     0,     0,    76,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    57,    59,     0,    61,     0,   111,   110,   109,   108,
       0,    91,     0,    93,     0,    18,     0,    22,    25,     0,
      29,    32,     0,    36,    39,     0,    43,    49,     0,    52,
      58,    60,    71,    72,     0,   116,   115,   114,   112,   113,
       0,    73,     0,     0,     0,    74,     0,    56,     0,    75,
      90,     0,    94,     0,     0,     0,     0,     0,    68,   105,
     104,     0,     0,     0,     0,    69,    70,     0,    78,    79,
      80,    81,    62,   103,   102,   101,    99,   100,     0,    20,
       0,     0,     0,     0,     0,     0,     0,    65,     0,    64,
       0,    92,     0,     0,   117,   118,   119,     0,    41,     0,
      44,     0,    63,     0,     0,    77,    82,    27,     0,     0,
       0,    51,    66,    67,     0,    46,    45,    34
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,   -11,  -150,
    -150,  -150,    29,  -150,  -150,  -150,    26,  -150,  -150,  -150,
      31,  -149,  -150,  -150,  -150,    23,  -150,  -150,  -150,     8,
      19,  -150,  -150,   -77,  -150,  -150,  -150,  -150,  -150,     3,
    -150,  -150,  -150,   -87,    63,    16,  -150,  -150,  -150,     0,
     164
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     5,    25,    26,    27,    53,    54,    28,
      29,    58,    59,    30,    31,    63,    64,    32,    33,    68,
      69,   188,    34,    35,    73,    74,    36,    37,    90,    91,
      92,    93,   157,   158,   159,   160,    38,    39,   100,   101,
      40,    41,   168,   151,    55,   102,   130,   189,     6,   190,
      95
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
       7,     9,   184,   185,   186,   187,    42,    -9,     1,     2,
      -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,
      -9,    -9,    -9,    -9,    -9,     1,    43,   106,    44,     2,
      45,   109,    46,   197,    47,   112,    48,    51,    52,    49,
     115,    50,   105,    56,    61,    66,    71,    76,    94,   103,
     205,   206,     2,    56,   149,   150,   118,   107,    61,    51,
      52,   152,   110,    66,   141,   178,     2,   113,    71,     2,
     170,   171,   116,    76,   172,    51,    52,   119,   121,     1,
     122,   123,   180,   153,   131,   173,   198,   108,   135,   111,
      94,    96,    97,    98,    99,   139,   117,   120,   137,   114,
     103,   195,   200,   140,   142,   136,     0,    60,    65,    70,
      75,     0,    96,    97,    98,    99,   125,   126,   127,   128,
     129,    60,     0,     2,   148,     0,    65,     0,     0,     0,
       0,    70,     0,   155,   156,     0,    75,     0,   162,     0,
       0,     0,     0,   169,     0,   124,     0,     0,   132,   133,
     134,     0,     0,     0,   177,     0,   138,   179,   163,   164,
     165,   166,   167,     0,     8,     0,     0,     0,   181,   143,
       0,     0,   144,     0,   192,   145,     0,     0,   146,     0,
       0,   147,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   201,     0,   202,   203,   154,     0,     0,     0,   161,
       0,     0,     0,     0,   207,     0,     0,    57,    62,    67,
      72,    77,     0,   104,   174,   175,   176,    57,     0,     0,
       0,     0,    62,     0,     0,     0,     0,    67,     0,     0,
       0,     0,    72,   182,   183,     0,   191,    77,   193,   194,
       0,   161,     0,   196,     0,     0,     0,     0,     0,     0,
     199,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   204,   -55,     1,   104,   -55,   -55,   -55,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,
       0,     0,     0,     0,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,     0,     0,     0,     0,
       0,     0,     1,     0,     0,   -83,     1,     2,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,     0,     0,     0,     0,    96,
      97,    98,    99,     0,     1,     0,     2,     0,   -17,     1,
       2,   -17,   -17,   -17,   -17,   -17,   -17,   -17,   -17,   -17,
     -17,   -17,   -17,   -17,   -17,   -17,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,     0,     0,     0,
       0,     0,     0,    51,    52,     0,     0,     0,     2,     0,
       0,   -24,     1,     2,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,     0,    51,    52,     0,     0,
       0,     0,     0,     0,   -31,     1,     2,   -31,   -31,   -31,
     -31,   -31,   -31,   -31,   -31,   -31,   -31,   -31,   -31,   -31,
     -31,   -31,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    51,
      52,     0,     0,     0,     0,     0,     0,   -38,     1,     2,
     -38,   -38,   -38,   -38,   -38,   -38,   -38,   -38,   -38,   -38,
     -38,   -38,   -38,   -38,   -38,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    51,    52,     0,     0,     0,     0,     0,     0,
     -48,     1,     2,   -48,   -48,   -48,   -48,   -48,   -48,   -48,
     -48,   -48,   -48,   -48,   -48,   -48,   -48,   -48,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    51,    52,     0,     0,     0,
       0,     0,     0,     0,     0,     2
};

static const yytype_int16 yycheck[] =
{
       0,     0,    18,    19,    20,    21,     6,     0,     1,    45,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,     1,    26,    41,    28,    45,
      30,    41,    32,   182,    34,    43,    36,    35,    36,    39,
      41,    41,    53,    43,    44,    45,    46,    47,    48,    49,
     199,   200,    45,    53,    34,    35,    41,    57,    58,    35,
      36,    21,    62,    63,    42,    44,    45,    67,    68,    45,
      42,    41,    72,    73,    42,    35,    36,    77,    78,     1,
      80,    81,    41,    43,    84,    42,    42,    58,    88,    63,
      90,    34,    35,    36,    37,    95,    73,    78,    90,    68,
     100,   178,   189,   100,   104,    89,    -1,    44,    45,    46,
      47,    -1,    34,    35,    36,    37,    34,    35,    36,    37,
      38,    58,    -1,    45,   124,    -1,    63,    -1,    -1,    -1,
      -1,    68,    -1,   133,   134,    -1,    73,    -1,   138,    -1,
      -1,    -1,    -1,   143,    -1,    82,    -1,    -1,    85,    86,
      87,    -1,    -1,    -1,   154,    -1,    93,   157,    34,    35,
      36,    37,    38,    -1,     0,    -1,    -1,    -1,   168,   106,
      -1,    -1,   109,    -1,   174,   112,    -1,    -1,   115,    -1,
      -1,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   191,    -1,   193,   194,   132,    -1,    -1,    -1,   136,
      -1,    -1,    -1,    -1,   204,    -1,    -1,    43,    44,    45,
      46,    47,    -1,    49,   151,   152,   153,    53,    -1,    -1,
      -1,    -1,    58,    -1,    -1,    -1,    -1,    63,    -1,    -1,
      -1,    -1,    68,   170,   171,    -1,   173,    73,   175,   176,
      -1,   178,    -1,   180,    -1,    -1,    -1,    -1,    -1,    -1,
     187,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,     0,     1,   100,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    -1,    -1,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,     0,     1,    45,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    -1,     1,    -1,    45,    -1,     0,     1,
      45,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    35,    36,    -1,    -1,    -1,    45,    -1,
      -1,     0,     1,    45,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    -1,    35,    36,    -1,    -1,
      -1,    -1,    -1,    -1,     0,     1,    45,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,
      36,    -1,    -1,    -1,    -1,    -1,    -1,     0,     1,    45,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    35,    36,    -1,    -1,    -1,    -1,    -1,    -1,
       0,     1,    45,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    35,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    45
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    45,    47,    48,    49,    94,    95,    96,     0,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    50,    51,    52,    55,    56,
      59,    60,    63,    64,    68,    69,    72,    73,    82,    83,
      86,    87,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    35,    36,    53,    54,    90,    95,    96,    57,    58,
      90,    95,    96,    61,    62,    90,    95,    96,    65,    66,
      90,    95,    96,    70,    71,    90,    95,    96,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      74,    75,    76,    77,    95,    96,    34,    35,    36,    37,
      84,    85,    91,    95,    96,    54,    41,    95,    58,    41,
      95,    62,    43,    95,    66,    41,    95,    71,    41,    95,
      76,    95,    95,    95,    90,    34,    35,    36,    37,    38,
      92,    95,    90,    90,    90,    95,    91,    75,    90,    95,
      85,    42,    95,    90,    90,    90,    90,    90,    95,    34,
      35,    89,    21,    43,    90,    95,    95,    78,    79,    80,
      81,    90,    95,    34,    35,    36,    37,    38,    88,    95,
      42,    41,    42,    42,    90,    90,    90,    95,    44,    95,
      41,    95,    90,    90,    18,    19,    20,    21,    67,    93,
      95,    90,    95,    90,    90,    79,    90,    67,    42,    90,
      89,    95,    95,    95,    90,    67,    67,    95
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    46,    47,    48,    48,    48,    48,    48,    48,    49,
      49,    50,    50,    50,    50,    50,    51,    52,    53,    53,
      54,    54,    54,    55,    56,    57,    57,    58,    58,    58,
      59,    60,    61,    61,    62,    62,    62,    63,    64,    65,
      65,    66,    66,    66,    67,    67,    67,    68,    69,    70,
      70,    71,    71,    71,    72,    73,    74,    74,    75,    75,
      75,    75,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    77,    78,    78,    79,
      79,    80,    81,    82,    83,    83,    83,    83,    83,    83,
      84,    84,    85,    85,    85,    86,    87,    87,    87,    88,
      88,    88,    88,    88,    89,    89,    90,    90,    91,    91,
      91,    91,    92,    92,    92,    92,    92,    93,    93,    93,
      94,    94,    95,    96
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     2,     2,     1,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     3,     2,     1,
       4,     1,     2,     1,     3,     2,     1,     6,     1,     2,
       1,     3,     2,     1,     8,     1,     2,     1,     3,     2,
       1,     5,     1,     2,     1,     3,     3,     1,     3,     2,
       1,     6,     2,     1,     1,     3,     2,     1,     2,     1,
       2,     1,     3,     5,     4,     4,     6,     6,     3,     3,
       3,     2,     2,     2,     2,     2,     1,     3,     1,     1,
       1,     1,     3,     3,     1,     1,     1,     1,     1,     1,
       2,     1,     4,     1,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 205 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
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
#line 1602 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 224 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                if ((yyvsp[0].memspan_list)) 
                    LL_CONCAT(bc_parsed_cfg->span, (yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1611 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 229 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                if ((yyvsp[0].macro_list)) 
                    LL_CONCAT(bc_parsed_cfg->macro, (yyvsp[0].macro_list), bc_macro_t);
            }
#line 1620 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 234 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                if ((yyvsp[0].varlike).vars) switch ((yyvsp[0].varlike).type)
                {
                    case BC_VL_VARS:
                    {
                        LL_CONCAT(bc_parsed_cfg->vars, (yyvsp[0].varlike).vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_JOYSTICK:
                    {
                        LL_CONCAT(bc_parsed_cfg->joystick, (yyvsp[0].varlike).vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_KEYS:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[0], (yyvsp[0].varlike).vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_CAPSLOCK:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[1], (yyvsp[0].varlike).vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_NUMLOCK:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[2], (yyvsp[0].varlike).vars, cfg_var_t);
                        break;
                    }

                    case BC_VL_SCROLLLOCK:
                    {
                        LL_CONCAT(bc_parsed_cfg->keys[3], (yyvsp[0].varlike).vars, cfg_var_t);
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
#line 1675 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 287 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                         "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, BC_SEC,
                        bc_saved_lineno, bc_errbuf);
            }
#line 1686 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 299 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                /* Before the chicken or egg came the rooster */
                if (!bc_parsed_cfg)
                {
                    bc_parsed_cfg = CALLOC(bc_cfgfile_t, 1);
                    BC_CHKOOM(bc_parsed_cfg);
                    bc_cursect = NULL;
                }
            }
#line 1700 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 309 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                if (!bc_parsed_cfg)
                {
                    bc_parsed_cfg = CALLOC(bc_cfgfile_t, 1);
                    BC_CHKOOM(bc_parsed_cfg);
                    bc_cursect = NULL;
                }
            }
#line 1713 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 333 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[bankswitch]"); }
#line 1719 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 335 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_REVERSE((yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1727 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 341 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_INSERT((yyvsp[-1].memspan_list), (yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1735 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 344 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.memspan_list) = (yyvsp[0].memspan_list);   }
#line 1741 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 348 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM((yyval.memspan_list));
                (yyval.memspan_list)->s_fofs = 0;
                (yyval.memspan_list)->e_fofs = 0;
                (yyval.memspan_list)->s_addr = (yyvsp[-3].intv);
                (yyval.memspan_list)->e_addr = (yyvsp[-1].intv);
                (yyval.memspan_list)->flags  = BC_SPAN_B | BC_SPAN_R;
                (yyval.memspan_list)->width  = 16;
                (yyval.memspan_list)->epage  = BC_SPAN_NOPAGE;
                (yyval.memspan_list)->f_name = NULL;
            }
#line 1758 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 361 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
            }
#line 1766 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 365 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                         "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[bankswitch]", 
                        bc_saved_lineno, bc_errbuf);
            }
#line 1778 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 378 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[mapping]"); }
#line 1784 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 380 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_REVERSE((yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1792 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 386 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_INSERT((yyvsp[-1].memspan_list), (yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1800 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 389 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.memspan_list) = (yyvsp[0].memspan_list); }
#line 1806 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 394 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM((yyval.memspan_list));
                (yyval.memspan_list)->s_fofs = (yyvsp[-5].intv);
                (yyval.memspan_list)->e_fofs = (yyvsp[-3].intv);
                (yyval.memspan_list)->s_addr = (yyvsp[-1].intv);
                (yyval.memspan_list)->e_addr = (yyvsp[-1].intv) + (yyvsp[-3].intv) - (yyvsp[-5].intv);
                (yyval.memspan_list)->flags  = (yyvsp[0].memattr_page).flags | BC_SPAN_PL;
                (yyval.memspan_list)->width  = (yyvsp[0].memattr_page).width;
                (yyval.memspan_list)->epage  = (yyvsp[0].memattr_page).epage;
                (yyval.memspan_list)->f_name = NULL;
            }
#line 1823 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 407 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
            }
#line 1831 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 411 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[mapping]", 
                        bc_saved_lineno, bc_errbuf);
            }
#line 1843 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 425 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[ecsbank]"); }
#line 1849 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 427 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_REVERSE((yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1857 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 433 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_INSERT((yyvsp[-1].memspan_list), (yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1865 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 436 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.memspan_list) = (yyvsp[0].memspan_list); }
#line 1871 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 441 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM((yyval.memspan_list));
                (yyval.memspan_list)->s_fofs = (yyvsp[-5].intv);
                (yyval.memspan_list)->e_fofs = (yyvsp[-3].intv);
                (yyval.memspan_list)->s_addr = (yyvsp[-1].intv);
                (yyval.memspan_list)->e_addr = (yyvsp[-1].intv) + (yyvsp[-3].intv) - (yyvsp[-5].intv);
                (yyval.memspan_list)->flags  = BC_SPAN_PL | BC_SPAN_R | BC_SPAN_EP;
                (yyval.memspan_list)->width  = 16;
                (yyval.memspan_list)->epage  = (yyvsp[-7].intv);
                (yyval.memspan_list)->f_name = NULL;
            }
#line 1888 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 454 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
            }
#line 1896 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 458 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[ecsbank]", 
                        bc_saved_lineno, bc_errbuf);
            }
#line 1908 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 471 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[memattr]"); }
#line 1914 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 473 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_REVERSE((yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1922 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 480 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_INSERT((yyvsp[-1].memspan_list), (yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 1930 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 483 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.memspan_list) = (yyvsp[0].memspan_list); }
#line 1936 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 487 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM((yyval.memspan_list));
                (yyval.memspan_list)->s_fofs = 0;
                (yyval.memspan_list)->e_fofs = 0;
                (yyval.memspan_list)->s_addr = (yyvsp[-4].intv);
                (yyval.memspan_list)->e_addr = (yyvsp[-2].intv);
                (yyval.memspan_list)->flags  = (yyvsp[0].memattr_page).flags;
                (yyval.memspan_list)->width  = (yyvsp[0].memattr_page).width;
                (yyval.memspan_list)->epage  = (yyvsp[0].memattr_page).epage;
                (yyval.memspan_list)->f_name = NULL;
            }
#line 1953 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 500 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {   
                (yyval.memspan_list) = NULL;
            }
#line 1961 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 504 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'\n", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[memattr]", 
                        bc_saved_lineno, bc_errbuf);
            }
#line 1973 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 514 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memattr_page).flags = BC_SPAN_R;
                (yyval.memattr_page).width = 16;
                (yyval.memattr_page).epage = BC_SPAN_NOPAGE;
            }
#line 1983 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 520 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memattr_page).flags =  (yyvsp[-2].intv) 
                         | ((yyvsp[-1].intv) < 16 ? BC_SPAN_N : 0)
                         | ((yyvsp[0].memattr_page).flags & BC_SPAN_EP);
                (yyval.memattr_page).width = (yyvsp[-1].intv);
                (yyval.memattr_page).epage = (yyvsp[0].memattr_page).epage;
            }
#line 1995 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 528 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memattr_page).flags = (yyvsp[0].memattr_page).flags | BC_SPAN_EP;
                (yyval.memattr_page).width = (yyvsp[0].memattr_page).width;
                (yyval.memattr_page).epage = (yyvsp[-1].intv);
            }
#line 2005 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 541 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[preload]"); }
#line 2011 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 543 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_REVERSE((yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 2019 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 549 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = LL_INSERT((yyvsp[-1].memspan_list), (yyvsp[0].memspan_list), bc_memspan_t);
            }
#line 2027 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 552 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.memspan_list) = (yyvsp[0].memspan_list); }
#line 2033 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 556 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = CALLOC(bc_memspan_t, 1);
                BC_CHKOOM((yyval.memspan_list));
                (yyval.memspan_list)->s_fofs = (yyvsp[-5].intv);
                (yyval.memspan_list)->e_fofs = (yyvsp[-3].intv);
                (yyval.memspan_list)->s_addr = (yyvsp[-1].intv);
                (yyval.memspan_list)->e_addr = (yyvsp[-1].intv) + (yyvsp[-3].intv) - (yyvsp[-5].intv);
                (yyval.memspan_list)->flags  = BC_SPAN_PL;
                (yyval.memspan_list)->width  = 16;
                (yyval.memspan_list)->epage  = BC_SPAN_NOPAGE;
                (yyval.memspan_list)->f_name = NULL;
            }
#line 2050 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 569 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_ERROR, "[preload]", 
                        bc_saved_lineno, bc_errbuf);
            }
#line 2062 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 577 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.memspan_list) = NULL;
            }
#line 2070 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 603 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[macro]"); }
#line 2076 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 605 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro_list) = LL_REVERSE((yyvsp[0].macro_list), bc_macro_t);
            }
#line 2084 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 611 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro_list) = LL_INSERT((yyvsp[-1].macro_list), (yyvsp[0].macro_list), bc_macro_t);
            }
#line 2092 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 614 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.macro_list) = (yyvsp[0].macro_list); }
#line 2098 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 620 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                if ((yyvsp[0].macro).cmd == BC_MAC_ERROR)
                    (yyval.macro_list) = NULL;
                else
                {
                    (yyval.macro_list)  = CALLOC(bc_macro_t, 1);
                    BC_CHKOOM((yyval.macro_list));
                    *(yyval.macro_list) = (yyvsp[0].macro);
                    (yyval.macro_list)->quiet = 1;
                }
            }
#line 2114 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 632 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                if ((yyvsp[0].macro).cmd == BC_MAC_ERROR)
                    (yyval.macro_list) = NULL;
                else
                {
                    (yyval.macro_list)  = CALLOC(bc_macro_t, 1);
                    BC_CHKOOM((yyval.macro_list));
                    *(yyval.macro_list) = (yyvsp[0].macro);
                    (yyval.macro_list)->quiet = 0;
                }
            }
#line 2130 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 644 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro_list) = NULL;
            }
#line 2138 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 648 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro_list) = NULL;
            }
#line 2146 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 654 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd            = BC_MAC_REG;
                (yyval.macro).arg.reg.reg    = (yyvsp[-2].intv);
                (yyval.macro).arg.reg.value  = (yyvsp[-1].intv);
            }
#line 2156 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 660 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd            = BC_MAC_LOAD;
 //fprintf(stderr, "name = %s\n", $2);
                (yyval.macro).arg.load.name  = (yyvsp[-3].strv);
                (yyval.macro).arg.load.width = (yyvsp[-2].intv);
                (yyval.macro).arg.load.addr  = (yyvsp[-1].intv);
            }
#line 2168 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 668 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd            = BC_MAC_WATCH;
                (yyval.macro).arg.watch      = (yyvsp[-1].mac_watch);
                (yyval.macro).arg.watch.name = (yyvsp[-2].strv);
            }
#line 2178 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 674 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd = BC_MAC_POKE;
                (yyval.macro).arg.poke.addr  = (yyvsp[-2].intv);
                (yyval.macro).arg.poke.epage = BC_SPAN_NOPAGE;
                (yyval.macro).arg.poke.value = (yyvsp[-1].intv);
            }
#line 2189 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 681 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd = BC_MAC_POKE;
                (yyval.macro).arg.poke.addr  = (yyvsp[-4].intv);
                (yyval.macro).arg.poke.epage = (yyvsp[-2].intv);
                (yyval.macro).arg.poke.value = (yyvsp[-1].intv);
            }
#line 2200 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 688 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd = BC_MAC_POKE;
                (yyval.macro).arg.poke.addr  = (yyvsp[-4].intv);
                (yyval.macro).arg.poke.epage = (yyvsp[-2].intv);
                (yyval.macro).arg.poke.value = (yyvsp[-1].intv);
            }
#line 2211 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 695 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd = BC_MAC_INSPECT;
                (yyval.macro).arg.inspect.addr = (yyvsp[-1].intv);
            }
#line 2220 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 700 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd = BC_MAC_RUNTO;
                (yyval.macro).arg.runto.addr = (yyvsp[-1].intv);
            }
#line 2229 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 705 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd = BC_MAC_TRACE;
                (yyval.macro).arg.runto.addr = (yyvsp[-1].intv);
            }
#line 2238 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 709 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {   (yyval.macro).cmd  = BC_MAC_AHEAD;     }
#line 2244 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 710 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {   (yyval.macro).cmd  = BC_MAC_BLANK;     }
#line 2250 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 711 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {   (yyval.macro).cmd  = BC_MAC_RUN;       }
#line 2256 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 712 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {   (yyval.macro).cmd  = BC_MAC_VIEW;      }
#line 2262 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 714 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.macro).cmd = BC_MAC_ERROR;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'", BC_TOK);
                BC_DIAG(BC_DIAG_WARNING, "[macro]", 
                        bc_saved_lineno, bc_errbuf);
            }
#line 2274 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 723 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = bc_hex; }
#line 2280 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 730 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                size_t new_size;
                int new_spans, i;

                new_spans = (yyvsp[-2].mac_watch).spans + (yyvsp[0].mac_watch).spans;
                new_size  = new_spans*sizeof(uint16_t)*2;

                (yyval.mac_watch).addr  = (uint16_t *)realloc((yyvsp[-2].mac_watch).addr, new_size);
                (yyval.mac_watch).spans = new_spans;
                BC_CHKOOM((yyval.mac_watch).addr);

                for (i = 0; i < (yyvsp[0].mac_watch).spans*2; i++)
                    (yyval.mac_watch).addr[i + 2*(yyvsp[-2].mac_watch).spans] = (yyvsp[0].mac_watch).addr[i];

                free((yyvsp[0].mac_watch).addr);
                (yyvsp[0].mac_watch).addr = NULL;
                (yyvsp[-2].mac_watch).addr = NULL;
            }
#line 2303 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 748 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.mac_watch) = (yyvsp[0].mac_watch); }
#line 2309 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 756 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.mac_watch).spans   = 1;
                (yyval.mac_watch).addr    = CALLOC(uint16_t, 2); BC_CHKOOM((yyval.mac_watch).addr);
                (yyval.mac_watch).addr[0] = (yyvsp[0].intv);
                (yyval.mac_watch).addr[1] = (yyvsp[0].intv);
            }
#line 2320 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 765 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.mac_watch).spans   = 1;
                (yyval.mac_watch).addr    = CALLOC(uint16_t, 2); BC_CHKOOM((yyval.mac_watch).addr);
                (yyval.mac_watch).addr[0] = (yyvsp[-2].intv);
                (yyval.mac_watch).addr[1] = (yyvsp[0].intv);
            }
#line 2331 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 785 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.varlike).type = (yyvsp[-2].varlike_type);
                (yyval.varlike).vars = LL_REVERSE((yyvsp[0].var_list), cfg_var_t);
            }
#line 2340 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 791 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.varlike_type)=BC_VL_VARS;       S("[vars]"      ); }
#line 2346 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 792 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.varlike_type)=BC_VL_JOYSTICK;   S("[joystick]"  ); }
#line 2352 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 793 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.varlike_type)=BC_VL_KEYS;       S("[keys]"      ); }
#line 2358 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 794 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.varlike_type)=BC_VL_CAPSLOCK;   S("[capslock]"  ); }
#line 2364 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 795 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.varlike_type)=BC_VL_NUMLOCK;    S("[numlock]"   ); }
#line 2370 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 796 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.varlike_type)=BC_VL_SCROLLLOCK; S("[scrolllock]"); }
#line 2376 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 800 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.var_list) = LL_INSERT((yyvsp[-1].var_list), (yyvsp[0].var_list), cfg_var_t);
            }
#line 2384 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 803 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.var_list) = (yyvsp[0].var_list); }
#line 2390 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 807 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.var_list) = CALLOC(cfg_var_t, 1);
                BC_CHKOOM((yyval.var_list));

                (yyval.var_list)->name = (yyvsp[-3].strv);
                (yyval.var_list)->val  = (yyvsp[-1].strnum);
            }
#line 2402 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 814 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.var_list) = NULL; }
#line 2408 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 816 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.var_list) = NULL;
                snprintf(bc_errbuf, sizeof(bc_errbuf),
                        "Unexpected token/state '%s'\n", BC_TOK);
                BC_DIAG(BC_DIAG_WARNING, "[vars]", 
                        bc_saved_lineno, bc_errbuf);
            }
#line 2420 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 831 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[disasm]"); }
#line 2426 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 832 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { S("[voices]"); }
#line 2432 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 834 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                static char bc_unknown[256];

                if (bc_txt)
                {
                    strncpy(bc_unknown, bc_txt, 255);
                    bc_unknown[255] = 0;
                }

                S(bc_unknown);
            }
#line 2448 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 881 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.strnum).flag    = VAL_STRING;
                (yyval.strnum).str_val = strdup(bc_txt);  BC_CHKOOM((yyval.strnum).str_val);
                (yyval.strnum).dec_val = 0;
                (yyval.strnum).hex_val = 0;
                val_try_parse_date( &((yyval.strnum)) );
            }
#line 2460 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 889 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.strnum).flag    = VAL_STRING;
                (yyval.strnum).str_val = strdup(bc_txt);  BC_CHKOOM((yyval.strnum).str_val);
                (yyval.strnum).dec_val = 0;
                (yyval.strnum).hex_val = 0;
                val_try_parse_date( &((yyval.strnum)) );
            }
#line 2472 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 897 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.strnum).flag    = VAL_STRING | VAL_HEXNUM;
                (yyval.strnum).str_val = strdup(bc_txt);  BC_CHKOOM((yyval.strnum).str_val);
                (yyval.strnum).dec_val = 0;
                (yyval.strnum).hex_val = bc_hex;
            }
#line 2483 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 904 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.strnum).flag    = VAL_STRING | VAL_HEXNUM | VAL_DECNUM;
                (yyval.strnum).str_val = strdup(bc_txt);  BC_CHKOOM((yyval.strnum).str_val);
                (yyval.strnum).dec_val = bc_dec;
                (yyval.strnum).hex_val = bc_hex;

                /* It *may* be a year, so try putting it in date too */
                if ( bc_dec > 0 && bc_dec < 100 )
                {
                    (yyval.strnum).flag |= VAL_DATE;
                    (yyval.strnum).date_val.year = bc_dec + 1900;
                } else if ( bc_dec > 1900 && bc_dec < 1900 + 255 )
                {
                    (yyval.strnum).flag |= VAL_DATE;
                    (yyval.strnum).date_val.year = bc_dec;
                }
            }
#line 2505 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 922 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    {
                (yyval.strnum).flag    = VAL_STRING | VAL_DECNUM;
                (yyval.strnum).str_val = strdup(bc_txt);  BC_CHKOOM((yyval.strnum).str_val);
                (yyval.strnum).dec_val = bc_dec;
                (yyval.strnum).hex_val = 0;
            }
#line 2516 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 930 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = bc_dec; }
#line 2522 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 931 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = bc_dec; }
#line 2528 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 934 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = bc_hex; }
#line 2534 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 935 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = bc_hex; }
#line 2540 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 938 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2546 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 939 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2552 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 940 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2558 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 941 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2564 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 944 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2570 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 945 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2576 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 946 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2582 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 947 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2588 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 948 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.strv) = strdup(bc_txt); BC_CHKOOM((yyval.strv)); }
#line 2594 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 951 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = BC_SPAN_RAM; }
#line 2600 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 952 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = BC_SPAN_ROM; }
#line 2606 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 953 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { (yyval.intv) = BC_SPAN_WOM; }
#line 2612 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 960 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
    { bc_dont_save = 0; bc_saved_tok = YYEMPTY; }
#line 2618 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 964 "bincfg/bincfg_grmr_real.y" /* yacc.c:1646  */
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
#line 2635 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
    break;


#line 2639 "bincfg/bincfg_grmr.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 977 "bincfg/bincfg_grmr_real.y" /* yacc.c:1906  */


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
