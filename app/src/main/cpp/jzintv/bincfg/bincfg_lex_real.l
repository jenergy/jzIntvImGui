/* Scanner for INTVPC .CFG files */
%option case-insensitive
%option never-interactive
%option noyywrap
%option noinput
%option batch
%option 8bit
%option nounput
%option prefix="bc_"
%option outfile="bincfg/bincfg_lex.c"

%x SEC_IGN
%s SEC_VAR
%s SEC_BSW
%s SEC_MAP
%s SEC_EBK
%s SEC_ATR
%s SEC_PRL
%x SEC_MC0
%x SEC_MC1
%x SEC_MC2

%{
/* Clang doesn't like the unreachable code in Flex's generated output. */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif

/* put any #includes here */
#include "config.h"
#include "lzoe/lzoe.h"
#include "bincfg/bincfg.h"
#include "bincfg/bincfg_grmr.tab.h"
#include "misc/types.h"

extern int      bc_lex(void);
int             bc_line_no = 1;
int             bc_dec; 
uint32_t         bc_hex;
char           *bc_txt = NULL;
static unsigned bc_txt_alloc = 0;

typedef enum nlsm_t 
{
    NLSM_START = 0,
    NLSM_SAWCR,
    NLSM_SAWEOF,
    NLSM_SAWEOF2,
    NLSM_DIDEOF
} nlsm_t;

static nlsm_t nlsm_state = NLSM_START;

/* Simple character-at-a-time state machine for reading input.  It serves
   three purposes:  

   1.  Convert Mac and Windows style NL to UNIX style.
   2.  Insert an extra NL at EOF to ensure the last line is NL terminated
   3.  Reads from an LZFILE* instead of a FILE*.
*/
static int nlsm_next_char(void)
{
    int next_char = EOF;

again: 
    if (nlsm_state == NLSM_SAWEOF)
    {
        nlsm_state = NLSM_SAWEOF2;
        return 10;
    }
    if (nlsm_state == NLSM_SAWEOF2)
    {
        nlsm_state = NLSM_DIDEOF;
        return EOF;
    }

    next_char = lzoe_fgetc( (LZFILE *)bc_in );

    if (nlsm_state == NLSM_DIDEOF)
    {
        if (next_char != EOF)
            nlsm_state = NLSM_START;
        else
            return EOF;
    }

    if (next_char == EOF)
    {
        nlsm_state = NLSM_SAWEOF;
        return 10; /* extra CR at EOF */
    }

    if (next_char == 13)
    {
        nlsm_state = NLSM_SAWCR;
        return 10;
    }

    if (next_char == 10 && nlsm_state == NLSM_SAWCR)
    {
        nlsm_state = NLSM_START;
        goto again; /* eat LF after CR, since we converted CR to LF */
    }

    nlsm_state = NLSM_START;
    return next_char;
}


#define YYTEXT_TO_BC_TXT(unquote) \
        do {                                                            \
            size_t uyyleng = yyleng > 0 ? yyleng : 0;                   \
            if (uyyleng >= bc_txt_alloc)                                \
            {                                                           \
                if (!bc_txt_alloc)                                      \
                    bc_txt_alloc = 256;                                 \
                                                                        \
                while (bc_txt_alloc <= uyyleng)                         \
                    bc_txt_alloc <<= 1;                                 \
                                                                        \
                bc_txt = (char *)realloc(bc_txt, bc_txt_alloc);         \
                                                                        \
                if (!bc_txt)                                            \
                    return TOK_ERROR_OOM;                               \
            }                                                           \
                                                                        \
            if (!unquote || uyyleng < 2)                                \
                strcpy(bc_txt, yytext);                                 \
            else                                                        \
                strcpy(bc_txt, cfg_unquote_str(yytext));                \
        } while (0)

#define YY_INPUT(buf,result,max_size)                                   \
        do {                                                            \
            int c_;                                                     \
            unsigned i_;                                                \
            unsigned ms = max_size > 0 ? max_size : 0;                  \
                                                                        \
            i_ = 0;                                                     \
            while (i_ < ms && (c_ = nlsm_next_char()) != EOF)           \
                buf[i_++] = c_;                                         \
                                                                        \
            result = i_ > 0 ? i_ : YY_NULL;                             \
                                                                        \
        } while (0);                                                    \


%}

HNUM    [0-9A-Fa-f]+
NAME    [A-Za-z_0-9\{\}"']+

%%
                   
%{ 
/* -------------------------------------------------------------------- */
/*  Rules for identifying section headers.                              */
/* -------------------------------------------------------------------- */
%} 
"[bankswitch]"      {   BEGIN(SEC_BSW); return  TOK_SEC_BANKSWITCH;     }
"[mapping]"         {   BEGIN(SEC_MAP); return  TOK_SEC_MAPPING;        }
"[ecsbank]"         {   BEGIN(SEC_EBK); return  TOK_SEC_ECSBANK;        }
"[memattr]"         {   BEGIN(SEC_ATR); return  TOK_SEC_MEMATTR;        }
"[preload]"         {   BEGIN(SEC_PRL); return  TOK_SEC_PRELOAD;        }
                        
"[macro]"           {   BEGIN(SEC_MC0); return  TOK_SEC_MACRO;          }

"[vars]"            {   BEGIN(SEC_VAR); return  TOK_SEC_VARS;           }
"[joystick]"        {   BEGIN(SEC_VAR); return  TOK_SEC_JOYSTICK;       }
"[keys]"            {   BEGIN(SEC_VAR); return  TOK_SEC_KEYS;           }
"[capslock]"        {   BEGIN(SEC_VAR); return  TOK_SEC_CAPSLOCK;       }
"[numlock]"         {   BEGIN(SEC_VAR); return  TOK_SEC_NUMLOCK;        }
"[scrolllock]"      {   BEGIN(SEC_VAR); return  TOK_SEC_SCROLLLOCK;     }
                        
"[disasm]"          {   BEGIN(SEC_IGN); return  TOK_SEC_DISASM;         }
"[voices]"          {   BEGIN(SEC_IGN); return  TOK_SEC_VOICES;         }
                        
"["[^\]]*"]"        {   BEGIN(SEC_IGN); return  TOK_SEC_UNKNOWN;        }

%{ 
/* -------------------------------------------------------------------- */
/*  Mini-scanner for ignored sections:  Just eat them right up.         */
/* -------------------------------------------------------------------- */
%} 
<SEC_IGN><<EOF>>    {   BEGIN(INITIAL); return '\n';                    }
<SEC_IGN>\n         {   bc_line_no++; /* eat newlines */                }
<SEC_IGN>;.*        {   /* eat comments */                              }
<SEC_IGN>[^\[;\n]+  {   /* eat non-section openers. */                  }
<SEC_IGN>"["        {   yyless(0);  BEGIN(INITIAL); return '\n';        }

%{ 
/* -------------------------------------------------------------------- */
/*  Keywords that are only valid in memory-attribute section.           */
/* -------------------------------------------------------------------- */
%} 
<SEC_ATR>"RAM"      {   return TOK_RAM;                                 }
<SEC_ATR>"ROM"      {   return TOK_ROM;                                 }
<SEC_ATR>"WOM"      {   return TOK_WOM;                                 }
<SEC_ATR>"PAGE"     {   return TOK_PAGE;                                }

%{ 
/* -------------------------------------------------------------------- */
/*  Keywords that are only valid in the mapping section.                */
/* -------------------------------------------------------------------- */
%} 
<SEC_MAP>"RAM"      {   return TOK_RAM;                                 }
<SEC_MAP>"ROM"      {   return TOK_ROM;                                 }
<SEC_MAP>"WOM"      {   return TOK_WOM;                                 }
<SEC_MAP>"PAGE"     {   return TOK_PAGE;                                }

%{ 
/* -------------------------------------------------------------------- */
/*  Keywords that are only valid in the ecsbank section.                */
/* -------------------------------------------------------------------- */
%} 
<SEC_EBK>":"        {   return ':';                                     }

%{ 
/* -------------------------------------------------------------------- */
/*  Keywords that are only valid in the macro section.                  */
/* -------------------------------------------------------------------- */
%} 
<SEC_MC0>"["        {   yyless(0); BEGIN(INITIAL);   /*]*/              }
<SEC_MC0>"@"        {   return TOK_MAC_QUIET;                           }
<SEC_MC0>"0"        {   BEGIN(SEC_MC2); bc_hex=0; return TOK_MAC_REG;   }
<SEC_MC0>"1"        {   BEGIN(SEC_MC2); bc_hex=1; return TOK_MAC_REG;   }
<SEC_MC0>"2"        {   BEGIN(SEC_MC2); bc_hex=2; return TOK_MAC_REG;   }
<SEC_MC0>"3"        {   BEGIN(SEC_MC1); bc_hex=3; return TOK_MAC_REG;   }
<SEC_MC0>"4"        {   BEGIN(SEC_MC2); bc_hex=4; return TOK_MAC_REG;   }
<SEC_MC0>"5"        {   BEGIN(SEC_MC2); bc_hex=5; return TOK_MAC_REG;   }
<SEC_MC0>"6"        {   BEGIN(SEC_MC2); bc_hex=6; return TOK_MAC_REG;   }
<SEC_MC0>"7"        {   BEGIN(SEC_MC2); bc_hex=7; return TOK_MAC_REG;   }
<SEC_MC0>"A"        {   BEGIN(SEC_MC2); return TOK_MAC_AHEAD;           }
<SEC_MC0>"B"        {   BEGIN(SEC_MC2); return TOK_MAC_BLANK;           }
<SEC_MC0>"I"        {   BEGIN(SEC_MC2); return TOK_MAC_INSPECT;         }
<SEC_MC0>"L"        {   BEGIN(SEC_MC1); return TOK_MAC_LOAD;            }
<SEC_MC0>"O"        {   BEGIN(SEC_MC2); return TOK_MAC_RUNTO;           }
<SEC_MC0>"P"        {   BEGIN(SEC_MC2); return TOK_MAC_POKE;            }
<SEC_MC0>"R"        {   BEGIN(SEC_MC2); return TOK_MAC_RUN;             }
<SEC_MC0>"T"        {   BEGIN(SEC_MC2); return TOK_MAC_TRACE;           }
<SEC_MC0>"V"        {   BEGIN(SEC_MC2); return TOK_MAC_VIEW;            }
<SEC_MC0>"W"        {   BEGIN(SEC_MC1); return TOK_MAC_WATCH;           }
<SEC_MC0>;.*        {   /* ignore comments.       */                    }
<SEC_MC0>[ \t\r]*   {   /* ignore whitespace.     */                    }
<SEC_MC0>\n         {   bc_line_no++; return '\n';                      }
<SEC_MC0>.          {   /* ignore unknown lines.  */                    }

%{ 
/* -------------------------------------------------------------------- */
/*  Special secondary, tertiary states for macro processing.  Grrr...   */
/*  We enter this state after parsing the first 'name' on a line.       */
/*  This keeps hex ranges that aren't $ adorned from turning into       */
/*  TOK_NAMEs.  Too much of the grammar comes into the lexer.  :-P      */
/* -------------------------------------------------------------------- */
%}

<SEC_MC1,SEC_MC2>[0-9]+         {   
                                    BEGIN(SEC_MC2);
                                    bc_dec = atoi(yytext);
                                    sscanf(yytext    , "%x", &bc_hex); 
                                    YYTEXT_TO_BC_TXT(0);
                                    return  TOK_DEC;    
                                }
<SEC_MC1,SEC_MC2>[A-F0-9]+      {   
                                    BEGIN(SEC_MC2);
                                    bc_dec = atoi(yytext);
                                    sscanf(yytext    , "%x", &bc_hex); 
                                    YYTEXT_TO_BC_TXT(0);
                                    return  TOK_HEX;    
                                }
<SEC_MC1,SEC_MC2>\$[A-F0-9]+    {   
                                    BEGIN(SEC_MC2);
                                    bc_dec = atoi(yytext + 1);
                                    sscanf(yytext + 1, "%x", &bc_hex); 
                                    YYTEXT_TO_BC_TXT(0);
                                    return  TOK_HEX;    
                                }
<SEC_MC1,SEC_MC2>"-"    {   BEGIN(SEC_MC2); return '-';                 }
<SEC_MC1,SEC_MC2>","    {   BEGIN(SEC_MC2); return ',';                 }
<SEC_MC1,SEC_MC2>":"    {   BEGIN(SEC_MC2); return ':';                 }
<SEC_MC1,SEC_MC2>"PAGE" {   BEGIN(SEC_MC2); return TOK_PAGE;            }
<SEC_MC1,SEC_MC2>;.*    {   BEGIN(SEC_MC2); /* eat comments.  */        }
<SEC_MC1,SEC_MC2>\n     {   bc_line_no++; BEGIN(SEC_MC0); return '\n';  }
<SEC_MC1,SEC_MC2>"["    {   yyless(0); BEGIN(INITIAL);   /*]*/          }

<SEC_MC1>\"([^\n\r"]*|\\\")*\"  {
                                    BEGIN(SEC_MC2);
                                    YYTEXT_TO_BC_TXT(1);
                                    return TOK_STRING;
                                }

<SEC_MC1>[^ \t\n\r;\[\]\$\=\-\,][^ \t\n\r;\[\]\$]* {   
                                    BEGIN(SEC_MC2);
                                    YYTEXT_TO_BC_TXT(0);                
                                    return  TOK_NAME;   
                                }

<SEC_MC1,SEC_MC2>[ \t\r]* { /* ignore whitespace. */                    }
<SEC_MC1,SEC_MC2>.        { YYTEXT_TO_BC_TXT(0); return TOK_ERROR_BAD;  }

%{ 
/* -------------------------------------------------------------------- */
/*  Main scanner with common rules across most sections.                */
/* -------------------------------------------------------------------- */
%}
-[0-9]+     {   
                bc_dec = atoi(yytext);
                sscanf(yytext + 1, "%x", &bc_hex); 
                YYTEXT_TO_BC_TXT(0);
                return  TOK_DECONLY;    
            }
[0-9]+      {   
                bc_dec = atoi(yytext);
                sscanf(yytext    , "%x", &bc_hex); 
                YYTEXT_TO_BC_TXT(0);
                return  TOK_DEC;    
            }
[A-F0-9]+   {   
                bc_dec = atoi(yytext);
                sscanf(yytext    , "%x", &bc_hex); 
                YYTEXT_TO_BC_TXT(0);
                return  TOK_HEX;    
            }
\$[A-F0-9]+ {   
                bc_dec = atoi(yytext + 1);
                sscanf(yytext + 1, "%x", &bc_hex); 
                YYTEXT_TO_BC_TXT(0);
                return  TOK_HEX;    
            }
"="         {   return '=';                                             }
"-"         {   return '-';                                             }
","         {   return ',';                                             }
;.*         {   /* eat comments.  */                                    }
\n          {   bc_line_no++;
                if (YY_START == SEC_MC1) BEGIN(SEC_MC0);
                return '\n';        /* comments and newlines are same */
            }

<SEC_VAR>[^ \t\n\r;\[\]\$\=\-\,\"]+ {   
                YYTEXT_TO_BC_TXT(0);                
                return TOK_NAME;   
            }

<SEC_VAR>\"([^\n\r"]*|\\\")*\" {
                YYTEXT_TO_BC_TXT(1);
                return TOK_STRING;
            }

[ \t\r]+    {   /* eat whitespace */                                    }
[^ \t\n\r\=\-\,\[\]A-Z0-9;]+ { 
    YYTEXT_TO_BC_TXT(0); 
#ifdef DEBUG
    fprintf(stderr, "BAD2: %d\n", yytext[0]);
#endif
    return TOK_ERROR_BAD;
}
.           {
    YYTEXT_TO_BC_TXT(0); 
#ifdef DEBUG
    fprintf(stderr, "BAD3: %d\n", yytext[0]);
#endif
    return TOK_ERROR_BAD;  
}

%%

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
