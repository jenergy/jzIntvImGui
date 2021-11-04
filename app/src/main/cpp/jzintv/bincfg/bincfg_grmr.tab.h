/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

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
#line 105 "bincfg/bincfg_grmr_real.y" /* yacc.c:1909  */

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

#line 110 "bincfg/bincfg_grmr.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE bc_lval;

int bc_parse (void);

#endif /* !YY_BC_BINCFG_BINCFG_GRMR_TAB_H_INCLUDED  */
