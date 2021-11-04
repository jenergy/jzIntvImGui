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




/* Copy the first part of user declarations.  */
#line 1 "asm/as1600_real.y" /* yacc.c:339  */

/* Clang doesn't like the unreachable code in Bison's generated output. */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif

#if 0
#   define YYDEBUG 1
#   define YYERROR_VERBOSE 1
#   define YYTOKEN_TABLE   1
    int yydebug = 1;
#endif

/*  NOTICE:  This code is based on the Public Domain AS2650.Y that comes
 *           with the Frankenstein Assembler, by Mark Zenier.  The changes
 *           that I, Joseph Zbiciak, have made are being placed under GPL.
 *           See GPL notice immediately below. 
 */

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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */


/*
HEADER:     ;
TITLE:      Frankenstein Cross Assemblers;
VERSION:    2.0;
DESCRIPTION: "  Reconfigurable Cross-assembler producing Intel (TM)
                Hex format object records.  ";
KEYWORDS:   cross-assemblers, 1600, 1805, 2650, 6301, 6502, 6805, 6809, 
            6811, tms7000, 8048, 8051, 8096, z8, z80;
SYSTEM:     UNIX, MS-Dos ;
FILENAME:   as1600.y;
WARNINGS:   "This software is in the public domain.  
             Any prior copyright claims are relinquished.  

             This software is distributed with no warranty whatever.  
             The author takes no responsibility for the consequences 
             of its use.

             Yacc (or Bison) required to compile."  ;
SEE-ALSO:   as1600.ps, frasmain.c;  
AUTHORS:    Mark Zenier; Joe Zbiciak 
COMPILERS:  GCC
*/

/* 1600 instruction generation file, GI standard syntax */
/* September 25, 1999 */

/*
    description frame work parser description for framework cross assemblers
    history     February 2, 1988
                September 11, 1990 - merge table definition
                September 12, 1990 - short file names
                September 14, 1990 - short variable names
                September 17, 1990 - use yylex as external
*/

/* ======================================================================== *\

The CP-1610 supports the following basic opcode formats:

 ---------------------------------------  -------  --------------------------
  Format                                   Words    Description
 ---------------------------------------  -------  --------------------------
  0000 000 0oo                               1      Implied 1-op insns
  0000 000 100  bbppppppii  pppppppppp       3      Jump insns
  0000 000 1oo                               1      Implied 1-op insns
  0000 ooo ddd                               1      1-op insns, comb src/dst
  0000 110 0dd                               1      GSWD
  0000 110 1om                               1      NOP(2), SIN(2)
  0001 ooo mrr                               1      Rotate/Shift insns
  0ooo sss ddd                               1      2-op arith, reg->reg
  1000 zxc ccc  pppppppppp                   2      Branch insns
  1ooo 000 ddd  pppppppppp                   2      2-op arith, direct, reg
  1ooo mmm ddd                               1*     2-op arith, ind., reg
  1ooo 111 ddd  iiiiiiiiii                   2*     2-op arith, immed., reg
 ---------------------------------------  -------  --------------------------


 -----
  Key
 -----

  oo    -- Opcode field (dependent on format)
  sss   -- Source register,      R0 ... R7 (binary encoding)
  ddd   -- Destination register, R0 ... R7 (binary encoding)
  0dd   -- Destination register, R0 ... R3
  cccc  -- Condition codes (branches)
  x     -- External branch condition (0 == internal, 1 == examine BEXT)
  z     -- Branch displacement direction (1 == negative)
  m     -- Shift amount (0 == shift by 1, 1 == shift by 2)
  bb    -- Branch return register
  ii    -- Branch interrupt flag mode

 --------------------------------
  Branch Condition Codes  (cccc)
 --------------------------------
           n == 0                    n == 1
  n000  -- Always                    Never
  n001  -- Carry set/Greater than    Carry clear/Less than or equal
  n010  -- Overflow set              Overflow clear
  n011  -- Positive                  Negative
  n100  -- Equal                     Not equal
  n101  -- Less than                 Greater than or equal
  n110  -- Less than or equal        Greater than
  n111  -- Unequal sign and carry    Equal sign and carry


 -------------------------------
  Branch Return Registers  (bb)
 -------------------------------

  00   -- R4
  01   -- R5
  10   -- R6
  11   -- none (do not save return address)

 -------------------------------
  Branch Interrupt Modes   (ii)
 -------------------------------

  00   -- Do not change interrupt enable state
  01   -- Enable interrupts
  10   -- Disable interrupts
  11   -- Undefined/Reserved ?

 ------------
  SDBD notes
 ------------

  -- SDBD is supported on "immediate" and "indirect" modes only.

  -- An SDBD prefix on an immediate instruction sets the immediate constant
     to be 16 bits, stored in two adjacent 8-bit words.  The ordering is
     little-endian.

  -- An SDBD prefix on an indirect instruction causes memory to be
     accessed twice, bringing in (or out) two 8-bit words, again in
     little-endian order.  If a non-incrementing data counter is used,
     both accesses are to the same address.  Otherwise, the counter
     is post-incremented with each access.  Indirect through R6
     (stack addressing) is not allowed, although I suspect it works
     as expected (performing two accesses through R6).

 ------------------------
  General encoding notes
 ------------------------

  -- "Immediate" mode is encoded the same as "Indirect" mode, except that
     R7 is given as the indirect register.  I'm guessing R7 is implemented
     the same as R4 and R5, especially since MVOI does what you'd
     expect -- it (attempts to) write over its immediate operand!!!

  -- The PC value (in R7) used for arithmetic always points to the first
     byte after the instruction for purposes of arithmetic.  This is
     consistent with the observation that immediate mode is really
     indirect mode in disguise, with the instruction being only one word
     long initially.

  -- Several instructions are just special cases of other instructions,
     and therefore do not need special decoder treatment:

      -- TSTR Rx  -->  MOVR Rx, Rx
      -- JR Rx    -->  MOVR Rx, R7
      -- CLRR Rx  -->  XORR Rx, Rx
      -- B        -->  Branch with condition code 0000 ("Always")
      -- NOPP     -->  Branch with condition code 1000 ("Never")
      -- PSHR Rx  -->  MVO@ Rx, R6
      -- PULR Rx  -->  MVI@ R6, Rx

  -- "Direct" mode is encoded the same as "Indirect" mode, except 000
     (which corresponds to R0) is encoded in the indirect register field.
     This is why R0 cannot be used as a data counter, and why it has no
     "special use."

  -- Relative branches encode their sign bit in the opcode word, rather
     than relying on a sign-extended relative offset in their second word.
     This allows +/- 10-bit range in a 10-bit wide memory, or +/-
     16-bit range in a 16-bit memory.  To avoid redundant encoding, the
     offset is calculated slightly differently for negative vs. positive
     offset:

      -- Negative: address_of_branch + 1 - offset
      -- Positive: address_of_branch + 2 + offset

     I'm guessing it is implemented about like so in hardware:

      -- offset == pc + (offset ^ (sign ? -1 : 0))

 ---------------
  Opcode Spaces
 ---------------

  I've divided the CP-1610 opcode map into 12 different opcode
  spaces.  (I really should merge the two separate Implied 1-op
  spaces into one space.  Oh well...)  In the descriptions below,
  "n/i" means "not interruptible".  Defined flags: Sign, Zero, Carry,
  Overflow, Interrupt-enable, Double-byte-data.  Interrupt-enable and
  Double-byte-data are not user visible.

  -- Implied 1-op instructions, part A:     0000 000 0oo
     Each has a single, implied operand, if any.

         opcode   mnemonic n/i  SZCOID  description
      --   00       HLT                 Halts the CPU (until next interrupt?)
      --   01       SDBD    *        1  Set Double Byte Data
      --   10       EIS     *       1   Enable Interrupt System
      --   11       DIS     *       1   Disable Interrupt System

  -- Implied 1-op instructions, part B:     0000 000 1oo
     Each has a single, implied operand, if any.

         opcode   mnemonic n/i  SZCOID  description
      --   00       n/a                 Aliases the "Jump" opcode space
      --   01       TCI     *           Terminate Current Interrupt.
      --   10       CLRC    *           Clear carry flag
      --   11       SETC    *           Set carry flag

  -- Jump Instructions:                     0000 000 100 bbppppppii pppppppppp
     Unconditional jumps with optional return-address save and
     interrupt enable/disable.

          bb  ii   mnemonic n/i  SZCOID description
      --  11  00    J                   Jump.
      --  xx  00    JSR                 Jump.  Save return address in R4..R6
      --  11  01    JE              1   Jump and enable ints.
      --  xx  01    JSRE            1   Jump and enable ints.  Save ret addr.
      --  11  10    JD              0   Jump and disable ints
      --  xx  10    JSRD            0   Jump and disable ints.  Save ret addr.
      --  xx  11    n/a                 Invalid opcode.

  -- Register 1-op instructions             0000 ooo rrr
     Each has one register operand, encoded as 000 through 111.

         opcode   mnemonic n/i  SZCOID  description
      --   000      n/a                 Aliases "Implied", "Jump" opcode space
      --   001      INCR        XX      INCrement register
      --   010      DECR        XX      DECrement register
      --   011      COMR        XX      COMplement register (1s complement)
      --   100      NEGR        XXXX    NEGate register     (2s complement)
      --   101      ADCR        XXXX    ADd Carry to Register
      --   110      n/a                 Aliases "GSWD", "NOP/SIN" opcode space
      --   111      RSWD        XXXX    Restore Status Word from Register


  -- Get Status WorD                        0000 110 0rr
     This was given its own opcode space due to limited encoding on its
     destination register and complication with the NOP/SIN encodings.

  -- NOP/SIN                                0000 110 1om
     I don't know what the "m" bit is for.  I don't know what to do with SIN.

         opcode   mnemonic n/i  SZCOID  description
      --    0       NOP                 No operation
      --    1       SIN                 Software Interrupt (pulse PCIT pin) ?

  -- Shift/Rotate 1-op instructions         0001 ooo mrr
     These can operate only on R0...R3.  The "m" bit specifies whether the
     operation is performed once or twice.  The overflow bit is used for
     catching the second bit on the rotates/shifts that use the carry.

         opcode   mnemonic n/i  SZCOID  description
      --   000      SWAP    *   XX      Swaps bytes in word once or twice.
      --   001      SLL     *   XX      Shift Logical Left
      --   010      RLC     *   XXX2    Rotate Left through Carry/overflow
      --   011      SLLC    *   XXX2    Shift Logical Left thru Carry/overflow
      --   100      SLR     *   XX      Shift Logical Right
      --   101      SAR     *   XX      Shift Arithmetic Right
      --   110      RRC     *   XXX2    Rotate Left through Carry/overflow
      --   111      SARC    *   XXX2    Shift Arithmetic Right thru Carry/over

  -- Register/Register 2-op instructions    0ooo sss ddd
     Register to register arithmetic.  Second operand acts as src2 and dest.

         opcode   mnemonic n/i  SZCOID  description
      --   00x      n/a                 Aliases other opcode spaces
      --   010      MOVR        XX      Move register to register
      --   011      ADDR        XXXX    Add src1 to src2->dst
      --   100      SUBR        XXXX    Sub src1 from src2->dst
      --   101      CMPR        XXXX    Sub src1 from src2, don't store
      --   110      ANDR        XX      AND src1 with src2->dst
      --   111      XORR        XX      XOR src1 with src2->dst

  -- Conditional Branch instructions        1000 zxn ccc pppppppppppppppp
     The "z" bit specifies the direction for the offset.  The "x" bit
     specifies using an external branch condition instead of using flag
     bits.  Conditional brances are interruptible.  The "n" bit specifies
     branching on the opposite condition from 'ccc'.

          cond      n=0         Condition       n=1         Condition
      --  n000      B           always          NOPP        never
      --  n001      BC          C = 1           BNC         C = 0
      --  n010      BOV         O = 1           BNOV        O = 0
      --  n011      BPL         S = 0           BMI         S = 1
      --  n100      BZE/BEQ     Z = 1           BNZE/BNEQ   Z = 0
      --  n101      BLT/BNGE    S^O = 1         BGE/BNLT    S^O = 0
      --  n110      BLE/BNGT    Z|(S^O) = 1     BGT/BNLE    Z|(S^O) = 0
      --  n111      BUSC        S^C = 1         BESC        S^C = 0

  -- Direct/Register 2-op instructions      1ooo 000 rrr  pppppppppppppppp
     Direct memory to register arithmetic.  MVO uses direct address as
     a destination, all other operations use it as a source, with
     the register as the destination.

         opcode   mnemonic n/i  SZCOID  description
      --   000      n/a                 Aliases conditional branch opcodes
      --   001      MVO     *           Move register to direct address
      --   010      MVI                 Move direct address to register
      --   011      ADD         XXXX    Add src1 to src2->dst
      --   100      SUB         XXXX    Sub src1 from src2->dst
      --   101      CMP         XXXX    Sub src1 from src2, don't store
      --   110      AND         XX      AND src1 with src2->dst
      --   111      XOR         XX      XOR src1 with src2->dst


  -- Indirect/Register 2-op instructions    1ooo sss ddd
     A source of "000" is actually a Direct/Register opcode.
     A source of "111" is actually a Immediate/Register opcode.
     R4, R5 increment after each access.  If the D bit is set, two
     accesses are made through the indirect register, updating the
     address if R4 or R5.  R6 increments after writes, decrements
     before reads.

         opcode   mnemonic n/i  SZCOID  description
      --   000      n/a                 Aliases conditional branch opcodes
      --   001      MVO@    *           Move register to indirect address
      --   010      MVI@                Move indirect address to register
      --   011      ADD@        XXXX    Add src1 to src2->dst
      --   100      SUB@        XXXX    Sub src1 from src2->dst
      --   101      CMP@        XXXX    Sub src1 from src2, don't store
      --   110      AND@        XX      AND src1 with src2->dst
      --   111      XOR@        XX      XOR src1 with src2->dst

  -- Immediate/Register 2-op instructions   1ooo 111 ddd  pppp
     If DBD is set, the immediate value spans two adjacent bytes, little
     endian order.  Otherwise the immediate value spans one word.  This
     instruction really looks like indirect through R7, and I suspect
     that's how the silicon implements it.

         opcode   mnemonic n/i  SZCOID  description
      --   000      n/a                 Aliases conditional branch opcodes
      --   001      MVOI    *           Move register to immediate field!
      --   010      MVII                Move immediate field to register
      --   011      ADDI        XXXX    Add src1 to src2->dst
      --   100      SUBI        XXXX    Sub src1 from src2->dst
      --   101      CMPI        XXXX    Sub src1 from src2, don't store
      --   110      ANDI        XX      AND src1 with src2->dst
      --   111      XORI        XX      XOR src1 with src2->dst

\* ======================================================================== */

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "as1600_types.h"
#include "intermed.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "asm/frasmdat.h"
#include "asm/fragcon.h"
#include "asm/intvec.h"
#include "asm/protos.h"
#include "asm/as1600.tab.h"
#include "asm/memo_string.h"

#define yylex lexintercept

#define JSR_RG    0x0001
#define SHF_RG    0x0002
#define IND_RG    0x0004
#define SDBD      0x0008

#define ST_REGREG 0x0001
#define ST_REGEXP 0x0002
#define ST_EXPREG 0x0004
#define ST_REGCEX 0x0008
#define ST_CEXREG 0x0010
#define ST_REG    0x0020
#define ST_EXP    0x0040
#define ST_IMP    0x0080
#define ST_EXPEXP 0x0100
    
/* ======================================================================== */
/*  R0 .. R7 can be used as general-purpose registers.                      */
/*  R0 .. R3 can be used for shifts and GSWD.                               */
/*  R1 .. R6 can be used for indirect addressing.                           */
/*  R4 .. R6 can be used for JSR.                                           */
/* ======================================================================== */
static int  reg_type[8] = 
{ 
    SHF_RG,
    SHF_RG | IND_RG,
    SHF_RG | IND_RG,
    SHF_RG | IND_RG,
    JSR_RG | IND_RG,
    JSR_RG | IND_RG,
    JSR_RG | IND_RG,
    0
};
    
/* ======================================================================== */
/*  BDEF outputs a number as a ROMW width word directly.  Allowed width is  */
/*       determined by argument #2 to the expression.                       */
/*  WDEF outputs a 16-bit word as a Double Byte Data.                       */
/* ======================================================================== */
static char genbdef[] = "[1=].[2#]I$[1=]x";
static char genwdef[] = "[1=].10I$[1=].FF&x[1=].8}.FF&x"; 

/*static char gensdbd[] = "0001x";*/

char ignosyn[] = "[Xinvalid syntax for instruction";
char ignosel[] = "[Xinvalid operands";

/* ======================================================================== */
/*  Truth table:                                                            */
/*                                                                          */
/*      ifskip      rptskip     expmac  |   Expand macros   Parse line      */
/*      FALSE       FALSE       any     |   TRUE            TRUE            */
/*      TRUE        any         FALSE   |   FALSE           FALSE           */
/*      TRUE        any         TRUE    |   TRUE            FALSE           */
/*      any         TRUE        FALSE   |   FALSE           FALSE           */
/*      any         TRUE        TRUE    |   TRUE            FALSE           */
/*                                                                          */
/*  Only IF/ENDI modify expmac.  RPT 0 does not modify expmac, and the      */
/*  default state of expmac is TRUE.  Therefore, by default, macros will    */
/*  get expanded in RPT 0 / ENDR blocks.  If you need RPT 0 to terminate    */
/*  macro recursion, include an IF / ENDI around it.  Sorry.                */
/* ======================================================================== */
int fraifskip = FALSE, frarptskip = FALSE, fraexpmac = TRUE;

int labelloc;
static int satsub;
int ifstkpt = 0;
int frarptact = 0,     frarptcnt = -1;
int struct_locctr = -1;

#define MAX_PROC_STK (64)

static char *proc_stk[MAX_PROC_STK];
static int   proc_stk_depth = 0;

extern char *proc;
extern int   proc_len;
static const char *currmode = "";


static int sdbd = 0, is_sdbd = 0;
static int romw = 16; 
static unsigned romm = 0xFFFF;
static int first = 1;

static int fwd_sdbd = 0;

struct symel * endsymbol = SYMNULL;


#define SDBD_CHK \
    if (sdbd) { sdbd = 0; frawarn("SDBD not allowed with this instruction."); }

LOCAL void do_set_equ_(int isequ, int flags, 
                       struct symel *sym, int value, 
                       const char *equerr)
{
    if (sym->seg == SSG_UNDEF
        || (sym->seg == SSG_SET && isequ == FALSE))
    {
        sym->seg    = isequ ? SSG_EQU : SSG_SET;
        sym->value  = value;
        sym->flags |= flags;
        emit_set_equ(value);
    } else
    {
        fraerror(equerr);
    }
}

LOCAL void do_set_equ_list(int isslice,
                           int isequ,         
                           int flags, 
                           struct symel   *const RESTRICT sym, 
                           const intvec_t *const RESTRICT exprs, 
                           int firstidx,      
                           int lastidx,
                           const char *const RESTRICT ncerr, 
                           const char *const RESTRICT equerr)
{
    static long *exprvals = NULL;
    static int exprvals_size = 0;
    struct symel *newsym;
    int i, idx, stp;

    if (exprvals_size < exprs->len)
    {
        exprvals = REALLOC(exprvals, long, exprs->alloc);
        exprvals_size = exprs->alloc;
    }

    for (i = 0; i < exprs->len; i++)
    {
        pevalexpr(0, exprs->data[i]);

        exprvals[i] = evalr[0].value;

        if (evalr[0].seg != SSG_ABS) 
            fraerror(ncerr);
    }

    if (exprs->len == 1 && !isslice)
    {
        do_set_equ_(isequ, flags, sym, exprvals[0], equerr);
    } 
    else if (abs(lastidx - firstidx) + 1 != exprs->len)
    {
        fraerror("Array slice length doesn't match expression list length");
    } 
    else if (sym->seg == SSG_UNDEF || sym->seg == SSG_SET)
    {
        if (sym->value < firstidx)
            sym->value = firstidx;

        if (sym->value < lastidx)
            sym->value = lastidx;

        stp = firstidx > lastidx ? -1 : 1;
        sym->seg    = SSG_SET;
        sym->flags |= SFLAG_QUIET;

        for (i = 0, idx = firstidx; i < exprs->len; i++, idx += stp)
        {
            newsym         = symbentryidx(sym->symstr, LABEL, 1, idx);
            newsym->flags |= SFLAG_QUIET | SFLAG_ARRAY;

            do_set_equ_(isequ, flags, newsym, exprvals[i], equerr);
        }
    } else
    {
        fraerror("Cannot convert symbol to array");
    }
}

typedef enum { USRERR, USRWARN, USRSTAT, USRCMT } usrmsg;

LOCAL void usr_message(usrmsg type, const char *msg)
{
    char *copy = strdup((msg && *msg) ? msg : " "), *s, *ss;

    /* force all characters to be printing characters */
    for (s = copy; *s; s++)
        if (!(isprint(*s) || isspace(*s)))
            *s = '?';

    /* Print all the lines of the message, breaking at newlines */
    for (s = copy; s && *s; s = ss)
    {
        ss = strpbrk(s, "\n\r");
        
        if (ss)
            *ss++ = 0;

        switch (type)
        {
            case USRERR:   fraerror(s); break;
            case USRWARN:  frawarn (s); break;
            case USRSTAT:  puts(s);     FALLTHROUGH_INTENDED;
            case USRCMT:   emit_comment(1, "%s", s); break;
            default:       fraerror("internal error in usr_message");
        }
    }

    free(copy);
}

LOCAL void chardef(char *sourcestr, 
                   const intvec_t *const RESTRICT defs)
{
    int findrv, numret, *charaddr;
    char *before;

    if(chtnpoint != (int *)NULL)
    {
        for(satsub = 0; satsub < defs->len; satsub++)
        {
            before = sourcestr;

            pevalexpr(0, defs->data[satsub]);
            findrv = chtcfind(chtnpoint, &sourcestr, &charaddr, &numret);

            if(findrv == CF_END)
            {
                fraerror("more expressions than characters");
                break;
            }

            if(evalr[0].seg == SSG_ABS)
            {
                switch(findrv)
                {
                case CF_UNDEF:
                    {
                        if(evalr[0].value < 0 || evalr[0].value > 255)
                            frawarn("character translation value truncated");
                        
                        *charaddr = evalr[0].value & 0xff;
                        emit_set_equ(evalr[0].value);
                    }
                    break;

                case CF_INVALID:
                case CF_NUMBER:
                    fracherror("invalid character to define", 
                                before, sourcestr);
                    break;

                case CF_CHAR:
                    fracherror("character already defined", 
                               before, sourcestr);
                    break;
                }
            }
            else
                fraerror("noncomputable expression");
        }

        if( *sourcestr != '\0')
            fraerror("more characters than expressions");
    }
    else
        fraerror("no CHARSET statement active");
}

LOCAL int chkover(int value, int bias)
{
    if (value > 0xFFFF + bias || value < 0)
        fraerror("Address overflow");

    return value;
}

#define MAXTEMPSTR (65536)
static char tempstr[MAXTEMPSTR];
static int  tempstrlen = 0;



#line 727 "asm/as1600.tab.c" /* yacc.c:339  */

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
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "as1600.tab.h".  */
#ifndef YY_YY_ASM_AS1600_TAB_H_INCLUDED
# define YY_YY_ASM_AS1600_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    CP1600_REG = 258,
    KOC_BDEF = 259,
    KOC_ELSE = 260,
    KOC_END = 261,
    KOC_ENDI = 262,
    KOC_EQU = 263,
    KOC_IF = 264,
    KOC_INCLUDE = 265,
    KOC_ORG = 266,
    KOC_RESM = 267,
    KOC_SDEF = 268,
    KOC_SET = 269,
    KOC_WDEF = 270,
    KOC_CHSET = 271,
    KOC_CHDEF = 272,
    KOC_CHUSE = 273,
    KOC_opcode = 274,
    KOC_opcode_i = 275,
    KOC_relbr = 276,
    KOC_relbr_x = 277,
    KOC_SDBD = 278,
    KOC_ROMW = 279,
    KOC_PROC = 280,
    KOC_ENDP = 281,
    KOC_STRUCT = 282,
    KOC_ENDS = 283,
    KOC_MEMATTR = 284,
    KOC_DDEF = 285,
    KOC_RPT = 286,
    KOC_ENDR = 287,
    KOC_USRERR = 288,
    KOC_LIST = 289,
    KOC_QEQU = 290,
    KOC_QSET = 291,
    KOC_MACERR = 292,
    KOC_BRKIF = 293,
    KOC_CMSG = 294,
    KOC_SMSG = 295,
    KOC_WMSG = 296,
    KOC_CFGVAR = 297,
    KOC_SRCFILE = 298,
    KOC_LISTCOL = 299,
    KOC_ERR_IF_OVERWRITTEN = 300,
    KOC_FORCE_OVERWRITE = 301,
    CONSTANT = 302,
    EOL = 303,
    KEOP_AND = 304,
    KEOP_DEFINED = 305,
    KEOP_EQ = 306,
    KEOP_GE = 307,
    KEOP_GT = 308,
    KEOP_HIGH = 309,
    KEOP_LE = 310,
    KEOP_LOW = 311,
    KEOP_LT = 312,
    KEOP_MOD = 313,
    KEOP_MUN = 314,
    KEOP_NE = 315,
    KEOP_NOT = 316,
    KEOP_OR = 317,
    KEOP_SHL = 318,
    KEOP_SHR = 319,
    KEOP_SHRU = 320,
    KEOP_ROTL = 321,
    KEOP_ROTR = 322,
    KEOP_XOR = 323,
    KEOP_locctr = 324,
    KEOP_TODAY_STR = 325,
    KEOP_TODAY_VAL = 326,
    KEOP_STRLEN = 327,
    KEOP_ASC = 328,
    KEOP_CLASSIFY = 329,
    LABEL = 330,
    STRING = 331,
    QCHAR = 332,
    SYMBOL = 333,
    FEATURE = 334,
    KEOP_EXPMAC = 335,
    KTK_invalid = 336,
    EXPRLIST = 337
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 661 "asm/as1600_real.y" /* yacc.c:355  */

    int             intv;
    int             longv;
    char            *strng;
    struct symel    *symb;
    struct slidx    slidx;
    intvec_t        *intvec;

#line 859 "asm/as1600.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_ASM_AS1600_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 876 "asm/as1600.tab.c" /* yacc.c:358  */

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
#define YYFINAL  108
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2206

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  96
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  17
/* YYNRULES -- Number of rules.  */
#define YYNRULES  176
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  353

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   337

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    92,    91,    93,     2,     2,
      90,    87,    85,    83,    88,    84,     2,    86,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    89,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    94,     2,    95,     2,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   776,   776,   777,   780,   785,   786,   794,   799,   803,
     807,   811,   815,   819,   823,   866,   873,   881,   889,   897,
     905,   913,   921,   929,   948,   959,   974,   986,  1006,  1040,
    1063,  1081,  1101,  1125,  1142,  1167,  1185,  1213,  1232,  1257,
    1274,  1299,  1316,  1335,  1341,  1354,  1374,  1410,  1421,  1432,
    1448,  1453,  1478,  1483,  1489,  1501,  1504,  1505,  1508,  1529,
    1540,  1553,  1567,  1580,  1592,  1610,  1611,  1614,  1615,  1618,
    1623,  1636,  1642,  1656,  1661,  1667,  1673,  1674,  1707,  1730,
    1757,  1788,  1803,  1837,  1858,  1885,  1902,  1927,  1964,  1979,
    1996,  2022,  2034,  2048,  2064,  2080,  2096,  2132,  2145,  2159,
    2173,  2177,  2181,  2185,  2189,  2193,  2197,  2201,  2205,  2209,
    2213,  2217,  2221,  2225,  2230,  2236,  2240,  2244,  2248,  2252,
    2256,  2260,  2264,  2268,  2272,  2276,  2280,  2284,  2288,  2292,
    2296,  2302,  2306,  2310,  2315,  2319,  2324,  2328,  2332,  2346,
    2350,  2375,  2391,  2392,  2414,  2415,  2435,  2436,  2457,  2485,
    2525,  2530,  2538,  2553,  2557,  2564,  2565,  2566,  2567,  2568,
    2569,  2570,  2571,  2572,  2573,  2574,  2575,  2576,  2577,  2578,
    2579,  2580,  2581,  2582,  2583,  2584,  2585
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CP1600_REG", "KOC_BDEF", "KOC_ELSE",
  "KOC_END", "KOC_ENDI", "KOC_EQU", "KOC_IF", "KOC_INCLUDE", "KOC_ORG",
  "KOC_RESM", "KOC_SDEF", "KOC_SET", "KOC_WDEF", "KOC_CHSET", "KOC_CHDEF",
  "KOC_CHUSE", "KOC_opcode", "KOC_opcode_i", "KOC_relbr", "KOC_relbr_x",
  "KOC_SDBD", "KOC_ROMW", "KOC_PROC", "KOC_ENDP", "KOC_STRUCT", "KOC_ENDS",
  "KOC_MEMATTR", "KOC_DDEF", "KOC_RPT", "KOC_ENDR", "KOC_USRERR",
  "KOC_LIST", "KOC_QEQU", "KOC_QSET", "KOC_MACERR", "KOC_BRKIF",
  "KOC_CMSG", "KOC_SMSG", "KOC_WMSG", "KOC_CFGVAR", "KOC_SRCFILE",
  "KOC_LISTCOL", "KOC_ERR_IF_OVERWRITTEN", "KOC_FORCE_OVERWRITE",
  "CONSTANT", "EOL", "KEOP_AND", "KEOP_DEFINED", "KEOP_EQ", "KEOP_GE",
  "KEOP_GT", "KEOP_HIGH", "KEOP_LE", "KEOP_LOW", "KEOP_LT", "KEOP_MOD",
  "KEOP_MUN", "KEOP_NE", "KEOP_NOT", "KEOP_OR", "KEOP_SHL", "KEOP_SHR",
  "KEOP_SHRU", "KEOP_ROTL", "KEOP_ROTR", "KEOP_XOR", "KEOP_locctr",
  "KEOP_TODAY_STR", "KEOP_TODAY_VAL", "KEOP_STRLEN", "KEOP_ASC",
  "KEOP_CLASSIFY", "LABEL", "STRING", "QCHAR", "SYMBOL", "FEATURE",
  "KEOP_EXPMAC", "KTK_invalid", "EXPRLIST", "'+'", "'-'", "'*'", "'/'",
  "')'", "','", "':'", "'('", "'$'", "'#'", "'%'", "'['", "']'", "$accept",
  "file", "allline", "line", "maybe_expmac", "labeledline", "genline",
  "labelcolon", "labelslicecolon", "exprlist", "string", "expr", "label",
  "symbol", "symslice", "labelslice", "keyword", YY_NULLPTR
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
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,    43,    45,    42,    47,    41,    44,    58,
      40,    36,    35,    37,    91,    93
};
# endif

#define YYPACT_NINF -88

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-88)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-1)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     507,   -44,  1042,   -88,   -88,   -88,   -67,   -57,  1156,  1156,
    1042,  1042,   -53,  1156,    23,    19,  1156,  1156,   -88,  1156,
     -88,   -88,  1156,  1042,  1156,   -88,   -16,   -25,   -88,  1156,
     -16,   -16,   -16,   -16,   -16,  1156,  1156,  1156,   -88,   -88,
     459,   -88,     7,   -88,   -88,   550,     4,   -87,   -74,   -88,
     -88,   -43,  1156,  1156,  1156,   -23,   -21,   -19,   -14,   -12,
     -88,   -88,   -88,   -88,  1156,  1156,  1042,    26,    -5,   -88,
    2010,   -31,   -20,   -88,  1156,   -88,  1042,   -88,  1199,    -9,
    2010,    -5,    -5,     1,    26,    15,  2010,    16,  1156,  1281,
      20,  2010,  1319,  1357,  1395,    -5,  2010,   -88,   -88,  2010,
     -88,   -88,   -88,    14,    22,  1433,  2010,  2010,   -88,   -88,
     -88,   -88,  1042,  1156,  1042,   -88,   -88,  1156,    35,  1042,
    1042,   -88,    34,   -52,  1042,  1042,  1042,  1042,   -88,  1156,
     -88,  1156,   -88,    -9,  2010,  2010,  2084,   -16,   -16,   -29,
     -27,   371,   -88,   -88,    -6,  1737,  1042,    32,    36,    37,
    1080,  1156,  1156,  1156,  1156,  1156,  1156,  1156,  1156,  1156,
    1156,  1156,  1156,  1156,  1156,  1156,  1156,  1156,  1156,  1156,
    1156,  1156,  2010,     0,  1156,  1156,  1156,  1042,  1042,   299,
    1471,   109,   121,  1156,  1156,  1156,  1118,  1156,  1156,    -5,
    1240,    -5,  2010,   -88,    -5,    -5,  1156,    -5,    -5,    -5,
      -5,   538,   579,    38,    43,    44,    45,    46,    47,    49,
     -88,   -43,   -88,   -88,   -88,  1156,   -88,  1156,   -88,   -88,
     -88,   -88,  1156,   -88,   -88,   -88,   -88,   -88,   -88,   -19,
     -14,   -12,    50,   -88,    51,  1776,   -84,    52,    70,   -88,
      11,  1156,  1156,  1156,   -88,  2010,   -20,  2084,  2120,  2120,
    2120,  2120,  2120,   -88,  2120,  2048,   -88,   -88,   -88,   -88,
     -88,  2048,   -33,   -33,   -88,   -88,   620,   661,    71,  1509,
    1547,   743,    -5,    -5,   -88,  1156,  2010,   137,   -88,   -88,
    2010,  2010,  1585,   -88,  2010,  2010,  1623,  1156,  1156,   782,
    1156,   -88,  1156,   -88,   -88,   -88,   -88,   -88,  1156,  1156,
     -88,   -88,   -88,   -88,   -88,   -88,  1156,   -88,  1815,  1854,
    1893,  1156,   -88,  1156,   -88,  1156,    94,    96,  2010,   -88,
      97,  1156,  1661,  1699,   821,   860,  1932,  1971,   702,   -88,
     -88,   -88,   899,   938,   977,   -88,   -88,   -88,  2010,    98,
     105,   -88,   -88,   -88,   -88,  1156,   -88,   -88,   -88,   -88,
     -88,  1016,   -88
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,    30,    13,    31,    57,     0,     0,     0,
       0,     0,     0,    50,    91,     0,     0,     0,    88,     0,
      83,    85,     0,     0,     0,    24,     0,     0,    12,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     5,   144,
       0,     3,     0,    55,    59,    54,     0,    65,    67,     6,
     129,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      76,   130,   146,   127,     0,     0,     0,   128,    60,    72,
      71,   125,    75,    56,    29,    14,     0,   128,    33,   125,
      64,    62,    63,     0,     0,     0,    51,    97,     0,    92,
       0,    89,     0,    86,     0,    61,    23,     8,    26,    25,
      11,    10,     9,     0,     0,     0,    47,    48,     1,     2,
       4,     7,     0,     0,     0,    49,    82,     0,     0,     0,
       0,    58,     0,    65,     0,     0,     0,     0,    66,     0,
      68,     0,   126,   124,   103,   104,   102,     0,     0,     0,
       0,     0,   100,   101,     0,    71,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    28,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    15,
      32,    17,    84,    27,    16,    18,     0,    19,    21,    20,
      22,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   127,   131,     0,     0,   125,     0,     0,   142,
       0,     0,     0,     0,    70,    69,    74,   121,   120,   116,
     115,   118,   117,   109,   119,   122,   110,   111,   112,   113,
     114,   123,   107,   108,   105,   106,     0,     0,     0,    35,
      39,     0,    53,    52,    98,     0,    93,     0,    95,    99,
      90,    87,     0,    43,    44,    45,     0,     0,     0,     0,
       0,   145,     0,   153,    81,    73,   139,   138,     0,     0,
     132,   133,   135,   137,   136,   134,     0,    77,     0,     0,
       0,     0,   147,     0,   150,     0,     0,     0,    94,    96,
       0,     0,    34,    38,     0,     0,     0,     0,     0,    79,
      78,    80,     0,     0,     0,    37,    41,    42,    46,     0,
       0,   152,   154,   141,   140,     0,   143,   148,   151,    36,
      40,     0,   149
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -88,   -88,   142,   -88,   -88,   -88,   138,   140,   -88,   238,
      60,    -8,   141,   118,    39,   -88,   -88
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    40,    41,    42,    74,    43,    44,    45,    46,    68,
      69,    70,    47,    79,    72,    48,   237
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      78,    80,   128,   304,    49,    86,    89,   129,    91,    92,
     176,    93,   124,    73,    94,   130,    96,    55,   125,    75,
     131,    99,    90,    60,    83,   157,    87,   105,   106,   107,
     160,   161,   162,   163,   164,    62,   132,   128,    84,   126,
     127,    55,   196,    55,   134,   135,   136,    60,   205,    60,
     207,    98,   168,   169,    55,   110,   142,   143,   145,   116,
      60,   117,    84,   170,    84,   186,   172,   137,   145,   138,
      50,   139,    85,    51,   171,    84,   140,    52,   141,    53,
     180,   238,   150,   150,    54,   176,    97,   268,   150,   177,
     100,   101,   102,   103,   104,    57,    58,    59,   307,   150,
      61,    62,    63,   178,   179,   190,    64,    65,   182,   192,
     187,   193,   278,    76,    77,    88,   146,   147,   148,   149,
      71,   201,   241,   202,   279,   294,   242,   243,    71,    71,
     295,   296,   297,   235,   298,   299,   300,   301,   302,   305,
     319,    71,   245,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   306,   315,   269,   270,   271,   133,
     335,   276,   336,   337,   349,   280,   281,   282,   284,   285,
     286,   350,   109,   121,    71,   122,   123,     0,   289,   246,
       0,     0,     0,     0,    71,     0,     0,   203,   204,   206,
     208,   234,     0,     0,     0,     0,     0,   134,     0,   135,
     244,     0,     0,     0,   136,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      71,     0,    71,   308,   309,   310,     0,    71,    71,     0,
       0,     0,    71,    71,    71,    71,   283,     0,    81,    82,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   236,
       0,    95,     0,     0,    71,     0,     0,   318,    71,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   322,
     323,     0,   324,     0,   325,     0,     0,     0,     0,     0,
     326,   327,     0,     0,     0,    71,    71,     0,   328,     0,
       0,     0,   274,   332,   144,   333,     0,   334,     0,     0,
       0,     0,     0,   338,   173,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   133,
       0,     0,     0,     0,     0,     0,     0,   351,     0,     0,
       0,     0,     0,     0,     0,     0,    50,     0,     0,    51,
     189,     0,   191,    52,     0,    53,     0,   194,   195,     0,
      54,     0,   197,   198,   199,   200,     0,     0,     0,     0,
       0,    57,    58,    59,   209,     0,    61,    62,    63,     0,
       0,     0,    64,    65,   240,     0,     0,     0,     0,    76,
      77,   275,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   272,   273,     0,    50,     0,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,     0,     0,   227,
     228,    55,     0,   229,   230,   231,     0,    60,    61,    62,
     232,     0,     0,     0,    64,    65,     0,     0,   233,   108,
       1,    76,    67,     2,     3,     4,     5,     0,     6,     7,
       8,     9,    10,     0,    11,     0,    12,    13,    14,    15,
      16,    17,    18,    19,     0,    20,     0,    21,    22,    23,
      24,    25,    26,    27,     0,     0,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,     0,    38,     1,     0,
       0,     2,     3,     4,     5,     0,     6,     7,     8,     9,
      10,     0,    11,     0,    12,    13,    14,    15,    16,    17,
      18,    19,     0,    20,    39,    21,    22,    23,    24,    25,
      26,    27,     0,     0,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,     2,    38,   111,     0,   112,     0,
       0,   113,     9,    10,   114,    11,   115,     0,     0,    14,
      15,    16,    17,    18,    19,   116,    20,   117,    21,     0,
      23,     0,    39,     0,   118,   119,   120,   151,     0,   152,
     153,   154,     0,   155,     0,   156,   157,     0,   158,     0,
     159,   160,   161,   162,   163,   164,   165,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   166,   167,   168,   169,    39,   290,     0,   151,     0,
     152,   153,   154,   291,   155,     0,   156,   157,     0,   158,
       0,   159,   160,   161,   162,   163,   164,   165,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   166,   167,   168,   169,     0,   292,     0,   151,
       0,   152,   153,   154,   293,   155,     0,   156,   157,     0,
     158,     0,   159,   160,   161,   162,   163,   164,   165,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   166,   167,   168,   169,     0,   311,     0,
     151,     0,   152,   153,   154,   312,   155,     0,   156,   157,
       0,   158,     0,   159,   160,   161,   162,   163,   164,   165,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   166,   167,   168,   169,     0,   313,
       0,   151,     0,   152,   153,   154,   314,   155,     0,   156,
     157,     0,   158,     0,   159,   160,   161,   162,   163,   164,
     165,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   166,   167,   168,   169,     0,
     345,     0,   151,     0,   152,   153,   154,   346,   155,     0,
     156,   157,     0,   158,     0,   159,   160,   161,   162,   163,
     164,   165,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   166,   167,   168,   169,
       0,   151,     0,   152,   153,   154,     0,   155,   312,   156,
     157,     0,   158,     0,   159,   160,   161,   162,   163,   164,
     165,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   166,   167,   168,   169,     0,
     151,     0,   152,   153,   154,     0,   155,   291,   156,   157,
       0,   158,     0,   159,   160,   161,   162,   163,   164,   165,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   166,   167,   168,   169,     0,   151,
       0,   152,   153,   154,     0,   155,   341,   156,   157,     0,
     158,     0,   159,   160,   161,   162,   163,   164,   165,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   166,   167,   168,   169,     0,   151,     0,
     152,   153,   154,     0,   155,   342,   156,   157,     0,   158,
       0,   159,   160,   161,   162,   163,   164,   165,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   166,   167,   168,   169,     0,   151,     0,   152,
     153,   154,     0,   155,   347,   156,   157,     0,   158,     0,
     159,   160,   161,   162,   163,   164,   165,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   166,   167,   168,   169,     0,   151,     0,   152,   153,
     154,     0,   155,   348,   156,   157,     0,   158,     0,   159,
     160,   161,   162,   163,   164,   165,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     166,   167,   168,   169,     0,   151,     0,   152,   153,   154,
       0,   155,   346,   156,   157,     0,   158,     0,   159,   160,
     161,   162,   163,   164,   165,     0,     0,     0,     0,    50,
       0,     0,    51,     0,     0,     0,    52,     0,    53,   166,
     167,   168,   169,    54,     0,     0,     0,     0,     0,     0,
       0,   352,    55,    56,    57,    58,    59,     0,    60,    61,
      62,    63,     0,     0,     0,    64,    65,    50,     0,     0,
      51,     0,    66,    67,    52,     0,    53,     0,     0,     0,
       0,    54,     0,     0,     0,     0,     0,     0,     0,     0,
      55,     0,    57,    58,    59,     0,    60,    61,    62,    63,
       0,     0,     0,    64,    65,    50,     0,     0,    51,     0,
      66,    67,    52,     0,    53,     0,     0,     0,     0,    54,
       0,     0,     0,     0,     0,     0,     0,     0,    55,     0,
      57,    58,    59,     0,    60,    61,    62,    63,     0,     0,
       0,    64,    65,    50,     0,     0,    51,     0,    76,    67,
      52,     0,    53,     0,     0,     0,     0,    54,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    57,    58,
      59,     0,     0,    61,    62,    63,     0,     0,     0,    64,
      65,     0,     0,     0,     0,     0,    76,    77,   151,     0,
     152,   153,   154,     0,   155,     0,   156,   157,     0,   158,
       0,   159,   160,   161,   162,   163,   164,   165,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   166,   167,   168,   169,     0,   174,   175,   151,
       0,   152,   153,   154,     0,   155,     0,   156,   157,     0,
     158,     0,   159,   160,   161,   162,   163,   164,   165,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   166,   167,   168,   169,     0,   287,   288,
     151,     0,   152,   153,   154,     0,   155,     0,   156,   157,
       0,   158,     0,   159,   160,   161,   162,   163,   164,   165,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   166,   167,   168,   169,   151,   181,
     152,   153,   154,     0,   155,     0,   156,   157,     0,   158,
       0,   159,   160,   161,   162,   163,   164,   165,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   166,   167,   168,   169,   151,   183,   152,   153,
     154,     0,   155,     0,   156,   157,     0,   158,     0,   159,
     160,   161,   162,   163,   164,   165,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     166,   167,   168,   169,   151,   184,   152,   153,   154,     0,
     155,     0,   156,   157,     0,   158,     0,   159,   160,   161,
     162,   163,   164,   165,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   166,   167,
     168,   169,   151,   185,   152,   153,   154,     0,   155,     0,
     156,   157,     0,   158,     0,   159,   160,   161,   162,   163,
     164,   165,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   166,   167,   168,   169,
     151,   188,   152,   153,   154,     0,   155,     0,   156,   157,
       0,   158,     0,   159,   160,   161,   162,   163,   164,   165,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   166,   167,   168,   169,   151,   277,
     152,   153,   154,     0,   155,     0,   156,   157,     0,   158,
       0,   159,   160,   161,   162,   163,   164,   165,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   166,   167,   168,   169,   151,   316,   152,   153,
     154,     0,   155,     0,   156,   157,     0,   158,     0,   159,
     160,   161,   162,   163,   164,   165,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     166,   167,   168,   169,   151,   317,   152,   153,   154,     0,
     155,     0,   156,   157,     0,   158,     0,   159,   160,   161,
     162,   163,   164,   165,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   166,   167,
     168,   169,   151,   320,   152,   153,   154,     0,   155,     0,
     156,   157,     0,   158,     0,   159,   160,   161,   162,   163,
     164,   165,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   166,   167,   168,   169,
     151,   321,   152,   153,   154,     0,   155,     0,   156,   157,
       0,   158,     0,   159,   160,   161,   162,   163,   164,   165,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   166,   167,   168,   169,   151,   339,
     152,   153,   154,     0,   155,     0,   156,   157,     0,   158,
       0,   159,   160,   161,   162,   163,   164,   165,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   166,   167,   168,   169,   151,   340,   152,   153,
     154,     0,   155,     0,   156,   157,     0,   158,     0,   159,
     160,   161,   162,   163,   164,   165,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     166,   167,   168,   169,   239,   151,     0,   152,   153,   154,
       0,   155,     0,   156,   157,     0,   158,     0,   159,   160,
     161,   162,   163,   164,   165,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   166,
     167,   168,   169,   303,   151,     0,   152,   153,   154,     0,
     155,     0,   156,   157,     0,   158,     0,   159,   160,   161,
     162,   163,   164,   165,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   166,   167,
     168,   169,   329,   151,     0,   152,   153,   154,     0,   155,
       0,   156,   157,     0,   158,     0,   159,   160,   161,   162,
     163,   164,   165,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   166,   167,   168,
     169,   330,   151,     0,   152,   153,   154,     0,   155,     0,
     156,   157,     0,   158,     0,   159,   160,   161,   162,   163,
     164,   165,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   166,   167,   168,   169,
     331,   151,     0,   152,   153,   154,     0,   155,     0,   156,
     157,     0,   158,     0,   159,   160,   161,   162,   163,   164,
     165,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   166,   167,   168,   169,   343,
     151,     0,   152,   153,   154,     0,   155,     0,   156,   157,
       0,   158,     0,   159,   160,   161,   162,   163,   164,   165,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   166,   167,   168,   169,   344,   151,
       0,   152,   153,   154,     0,   155,     0,   156,   157,     0,
     158,     0,   159,   160,   161,   162,   163,   164,   165,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   166,   167,   168,   169,   151,     0,   152,
     153,   154,     0,   155,     0,   156,   157,     0,   158,     0,
       0,   160,   161,   162,   163,   164,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   166,   167,   168,   169,   152,   153,   154,     0,   155,
       0,   156,   157,     0,   158,     0,     0,   160,   161,   162,
     163,   164,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   166,   167,   168,
     169,    -1,    -1,    -1,     0,    -1,     0,    -1,   157,     0,
      -1,     0,     0,   160,   161,   162,   163,   164,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   166,   167,   168,   169
};

static const yytype_int16 yycheck[] =
{
       8,     9,    89,    87,    48,    13,    14,    94,    16,    17,
      94,    19,     8,    80,    22,    89,    24,    70,    14,    76,
      94,    29,     3,    76,    77,    58,     3,    35,    36,    37,
      63,    64,    65,    66,    67,    78,    79,    89,    91,    35,
      36,    70,    94,    70,    52,    53,    54,    76,    77,    76,
      77,    76,    85,    86,    70,    48,    64,    65,    66,    25,
      76,    27,    91,    94,    91,    51,    74,    90,    76,    90,
      47,    90,    12,    50,    94,    91,    90,    54,    90,    56,
      88,    87,    88,    88,    61,    94,    26,    87,    88,    88,
      30,    31,    32,    33,    34,    72,    73,    74,    87,    88,
      77,    78,    79,    88,    88,   113,    83,    84,    88,   117,
      88,    76,     3,    90,    91,    92,    90,    91,    92,    93,
       2,   129,    90,   131,     3,    87,    90,    90,    10,    11,
      87,    87,    87,   141,    88,    88,    87,    87,    87,    87,
       3,    23,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    94,    94,   174,   175,   176,    51,
      76,   179,    76,    76,    76,   183,   184,   185,   186,   187,
     188,    76,    40,    45,    66,    45,    45,    -1,   196,   150,
      -1,    -1,    -1,    -1,    76,    -1,    -1,   137,   138,   139,
     140,   141,    -1,    -1,    -1,    -1,    -1,   215,    -1,   217,
     150,    -1,    -1,    -1,   222,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     112,    -1,   114,   241,   242,   243,    -1,   119,   120,    -1,
      -1,    -1,   124,   125,   126,   127,   186,    -1,    10,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
      -1,    23,    -1,    -1,   146,    -1,    -1,   275,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   287,
     288,    -1,   290,    -1,   292,    -1,    -1,    -1,    -1,    -1,
     298,   299,    -1,    -1,    -1,   177,   178,    -1,   306,    -1,
      -1,    -1,     3,   311,    66,   313,    -1,   315,    -1,    -1,
      -1,    -1,    -1,   321,    76,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   211,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   345,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    50,
     112,    -1,   114,    54,    -1,    56,    -1,   119,   120,    -1,
      61,    -1,   124,   125,   126,   127,    -1,    -1,    -1,    -1,
      -1,    72,    73,    74,     3,    -1,    77,    78,    79,    -1,
      -1,    -1,    83,    84,   146,    -1,    -1,    -1,    -1,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   177,   178,    -1,    47,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    -1,    -1,    68,
      69,    70,    -1,    72,    73,    74,    -1,    76,    77,    78,
      79,    -1,    -1,    -1,    83,    84,    -1,    -1,    87,     0,
       1,    90,    91,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    -1,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    -1,    48,     1,    -1,
      -1,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    15,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    -1,    26,    75,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,     4,    48,     6,    -1,     8,    -1,
      -1,    11,    12,    13,    14,    15,    16,    -1,    -1,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    -1,    75,    -1,    34,    35,    36,    49,    -1,    51,
      52,    53,    -1,    55,    -1,    57,    58,    -1,    60,    -1,
      62,    63,    64,    65,    66,    67,    68,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    83,    84,    85,    86,    75,    88,    -1,    49,    -1,
      51,    52,    53,    95,    55,    -1,    57,    58,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    84,    85,    86,    -1,    88,    -1,    49,
      -1,    51,    52,    53,    95,    55,    -1,    57,    58,    -1,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    84,    85,    86,    -1,    88,    -1,
      49,    -1,    51,    52,    53,    95,    55,    -1,    57,    58,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    84,    85,    86,    -1,    88,
      -1,    49,    -1,    51,    52,    53,    95,    55,    -1,    57,
      58,    -1,    60,    -1,    62,    63,    64,    65,    66,    67,
      68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,    -1,
      88,    -1,    49,    -1,    51,    52,    53,    95,    55,    -1,
      57,    58,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,
      -1,    49,    -1,    51,    52,    53,    -1,    55,    95,    57,
      58,    -1,    60,    -1,    62,    63,    64,    65,    66,    67,
      68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,    -1,
      49,    -1,    51,    52,    53,    -1,    55,    95,    57,    58,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    84,    85,    86,    -1,    49,
      -1,    51,    52,    53,    -1,    55,    95,    57,    58,    -1,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    84,    85,    86,    -1,    49,    -1,
      51,    52,    53,    -1,    55,    95,    57,    58,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    84,    85,    86,    -1,    49,    -1,    51,
      52,    53,    -1,    55,    95,    57,    58,    -1,    60,    -1,
      62,    63,    64,    65,    66,    67,    68,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    83,    84,    85,    86,    -1,    49,    -1,    51,    52,
      53,    -1,    55,    95,    57,    58,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    84,    85,    86,    -1,    49,    -1,    51,    52,    53,
      -1,    55,    95,    57,    58,    -1,    60,    -1,    62,    63,
      64,    65,    66,    67,    68,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    50,    -1,    -1,    -1,    54,    -1,    56,    83,
      84,    85,    86,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    95,    70,    71,    72,    73,    74,    -1,    76,    77,
      78,    79,    -1,    -1,    -1,    83,    84,    47,    -1,    -1,
      50,    -1,    90,    91,    54,    -1,    56,    -1,    -1,    -1,
      -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    -1,    72,    73,    74,    -1,    76,    77,    78,    79,
      -1,    -1,    -1,    83,    84,    47,    -1,    -1,    50,    -1,
      90,    91,    54,    -1,    56,    -1,    -1,    -1,    -1,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      72,    73,    74,    -1,    76,    77,    78,    79,    -1,    -1,
      -1,    83,    84,    47,    -1,    -1,    50,    -1,    90,    91,
      54,    -1,    56,    -1,    -1,    -1,    -1,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    72,    73,
      74,    -1,    -1,    77,    78,    79,    -1,    -1,    -1,    83,
      84,    -1,    -1,    -1,    -1,    -1,    90,    91,    49,    -1,
      51,    52,    53,    -1,    55,    -1,    57,    58,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    84,    85,    86,    -1,    88,    89,    49,
      -1,    51,    52,    53,    -1,    55,    -1,    57,    58,    -1,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    84,    85,    86,    -1,    88,    89,
      49,    -1,    51,    52,    53,    -1,    55,    -1,    57,    58,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    84,    85,    86,    49,    88,
      51,    52,    53,    -1,    55,    -1,    57,    58,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    84,    85,    86,    49,    88,    51,    52,
      53,    -1,    55,    -1,    57,    58,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    84,    85,    86,    49,    88,    51,    52,    53,    -1,
      55,    -1,    57,    58,    -1,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,    84,
      85,    86,    49,    88,    51,    52,    53,    -1,    55,    -1,
      57,    58,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,
      49,    88,    51,    52,    53,    -1,    55,    -1,    57,    58,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    84,    85,    86,    49,    88,
      51,    52,    53,    -1,    55,    -1,    57,    58,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    84,    85,    86,    49,    88,    51,    52,
      53,    -1,    55,    -1,    57,    58,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    84,    85,    86,    49,    88,    51,    52,    53,    -1,
      55,    -1,    57,    58,    -1,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,    84,
      85,    86,    49,    88,    51,    52,    53,    -1,    55,    -1,
      57,    58,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,
      49,    88,    51,    52,    53,    -1,    55,    -1,    57,    58,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    84,    85,    86,    49,    88,
      51,    52,    53,    -1,    55,    -1,    57,    58,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    84,    85,    86,    49,    88,    51,    52,
      53,    -1,    55,    -1,    57,    58,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    84,    85,    86,    87,    49,    -1,    51,    52,    53,
      -1,    55,    -1,    57,    58,    -1,    60,    -1,    62,    63,
      64,    65,    66,    67,    68,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,
      84,    85,    86,    87,    49,    -1,    51,    52,    53,    -1,
      55,    -1,    57,    58,    -1,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,    84,
      85,    86,    87,    49,    -1,    51,    52,    53,    -1,    55,
      -1,    57,    58,    -1,    60,    -1,    62,    63,    64,    65,
      66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,
      86,    87,    49,    -1,    51,    52,    53,    -1,    55,    -1,
      57,    58,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,
      87,    49,    -1,    51,    52,    53,    -1,    55,    -1,    57,
      58,    -1,    60,    -1,    62,    63,    64,    65,    66,    67,
      68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,    87,
      49,    -1,    51,    52,    53,    -1,    55,    -1,    57,    58,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    84,    85,    86,    87,    49,
      -1,    51,    52,    53,    -1,    55,    -1,    57,    58,    -1,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    84,    85,    86,    49,    -1,    51,
      52,    53,    -1,    55,    -1,    57,    58,    -1,    60,    -1,
      -1,    63,    64,    65,    66,    67,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    83,    84,    85,    86,    51,    52,    53,    -1,    55,
      -1,    57,    58,    -1,    60,    -1,    -1,    63,    64,    65,
      66,    67,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,
      86,    51,    52,    53,    -1,    55,    -1,    57,    58,    -1,
      60,    -1,    -1,    63,    64,    65,    66,    67,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    84,    85,    86
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     4,     5,     6,     7,     9,    10,    11,    12,
      13,    15,    17,    18,    19,    20,    21,    22,    23,    24,
      26,    28,    29,    30,    31,    32,    33,    34,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    48,    75,
      97,    98,    99,   101,   102,   103,   104,   108,   111,    48,
      47,    50,    54,    56,    61,    70,    71,    72,    73,    74,
      76,    77,    78,    79,    83,    84,    90,    91,   105,   106,
     107,   109,   110,    80,   100,    76,    90,    91,   107,   109,
     107,   105,   105,    77,    91,   106,   107,     3,    92,   107,
       3,   107,   107,   107,   107,   105,   107,   106,    76,   107,
     106,   106,   106,   106,   106,   107,   107,   107,     0,    98,
      48,     6,     8,    11,    14,    16,    25,    27,    34,    35,
      36,   102,   103,   108,     8,    14,    35,    36,    89,    94,
      89,    94,    79,   109,   107,   107,   107,    90,    90,    90,
      90,    90,   107,   107,   105,   107,    90,    91,    92,    93,
      88,    49,    51,    52,    53,    55,    57,    58,    60,    62,
      63,    64,    65,    66,    67,    68,    83,    84,    85,    86,
      94,    94,   107,   105,    88,    89,    94,    88,    88,    88,
     107,    88,    88,    88,    88,    88,    51,    88,    88,   105,
     107,   105,   107,    76,   105,   105,    94,   105,   105,   105,
     105,   107,   107,   106,   106,    77,   106,    77,   106,     3,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    68,    69,    72,
      73,    74,    79,    87,   106,   107,   109,   112,    87,    87,
     105,    90,    90,    90,   106,   107,   110,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,    87,   107,
     107,   107,   105,   105,     3,    92,   107,    88,     3,     3,
     107,   107,   107,   106,   107,   107,   107,    88,    89,   107,
      88,    95,    88,    95,    87,    87,    87,    87,    88,    88,
      87,    87,    87,    87,    87,    87,    94,    87,   107,   107,
     107,    88,    95,    88,    95,    94,    88,    88,   107,     3,
      88,    88,   107,   107,   107,   107,   107,   107,   107,    87,
      87,    87,   107,   107,   107,    76,    76,    76,   107,    88,
      88,    95,    95,    87,    87,    88,    95,    95,    95,    76,
      76,   107,    95
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    96,    97,    97,    98,    98,    98,    99,    99,    99,
      99,    99,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,    99,    99,    99,   100,   100,   101,   101,
     102,   102,   102,   102,   102,   103,   103,   104,   104,   105,
     105,   105,   105,   105,   105,   105,   106,   106,   106,   106,
     106,   106,   102,   102,   102,   102,   102,   102,   102,   102,
     102,   102,   102,   102,   102,   102,   102,   102,   102,   102,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   108,   108,   109,   109,   110,   110,
     110,   110,   111,   111,   111,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     2,     1,     2,     2,     2,     2,
       2,     2,     1,     1,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     1,     2,     2,     3,     3,     2,
       1,     1,     3,     2,     5,     4,     7,     6,     5,     4,
       7,     6,     6,     4,     4,     4,     6,     2,     2,     2,
       1,     2,     4,     4,     1,     1,     1,     0,     2,     1,
       2,     2,     2,     2,     2,     1,     2,     1,     2,     3,
       3,     1,     1,     4,     3,     1,     1,     4,     5,     5,
       5,     4,     2,     1,     3,     1,     2,     4,     1,     2,
       4,     1,     2,     4,     5,     4,     5,     2,     4,     4,
       2,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     1,     2,     1,     1,     1,
       1,     3,     4,     4,     4,     4,     4,     4,     4,     4,
       6,     6,     3,     6,     1,     4,     1,     4,     6,     8,
       4,     6,     6,     4,     6,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1
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
        case 4:
#line 781 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                clrexpr();
                tempstrlen = 0;
            }
#line 2577 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 787 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                clrexpr();
                tempstrlen = 0;
                yyerrok;
            }
#line 2587 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 795 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                endsymbol = (yyvsp[-1].symb);
                nextreadact = Nra_end;
            }
#line 2596 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 800 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                usr_message(USRERR, (yyvsp[0].strng));
            }
#line 2604 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 804 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                usr_message(USRWARN, (yyvsp[0].strng));
            }
#line 2612 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 808 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                usr_message(USRSTAT, (yyvsp[0].strng));
            }
#line 2620 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 812 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                usr_message(USRCMT, (yyvsp[0].strng));
            }
#line 2628 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 816 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                fraerror("Unexpected MACRO or ENDM directive");
            }
#line 2636 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 820 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                nextreadact = Nra_end;
            }
#line 2644 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 824 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if(frarptact)
                {
                    fraerror("INCLUDE not permitted inside REPEAT");
                } else if(nextfstk >= FILESTKDPTH)
                {
                    fraerror("include file nesting limit exceeded");
                }
                else
                {
                    infilestk[nextfstk].line = 0;
                    infilestk[nextfstk].fnm  = memoize_string((yyvsp[0].strng));
                    if( (infilestk[nextfstk].fpt = 
                        path_fopen(as1600_search_path,(yyvsp[0].strng),"r")) ==(LZFILE*)NULL)
                    {
                        static char *incl_file = NULL;
                        static int   incl_file_size = 0;
                        int          incl_file_len  = strlen((yyvsp[0].strng)) + 80;
                        if (incl_file_size < incl_file_len)
                        {
                            incl_file_size = incl_file_len << 1;
                            if (incl_file) free(incl_file);
                            incl_file      = (char *)malloc(incl_file_size);
                            if (!incl_file)
                                incl_file_size = 0;
                        }
                        if (incl_file_size == 0)
                            fraerror("cannot open include file");
                        else
                        {
                            sprintf(incl_file, "cannot open include file "
                                               "\"%s\"", (yyvsp[0].strng));
                            
                            fraerror(incl_file);
                        }
                    }
                    else
                    {
                        nextreadact = Nra_new;
                    }
                }
            }
#line 2691 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 867 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(FALSE, TRUE, 0, (yyvsp[-2].symb), (yyvsp[0].intvec), 0, (yyvsp[0].intvec)->len - 1,
                               "noncomputable expression for EQU",
                               "cannot change symbol value with EQU");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2702 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 874 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(FALSE, TRUE, SFLAG_QUIET, (yyvsp[-2].symb), (yyvsp[0].intvec), 0,
                                (yyvsp[0].intvec)->len - 1,
                               "noncomputable expression for QEQU",
                               "cannot change symbol value with QEQU");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2714 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 882 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(FALSE, FALSE, 0, (yyvsp[-2].symb), (yyvsp[0].intvec), 0, 
                               (yyvsp[0].intvec)->len - 1,
                               "noncomputable expression for SET",
                               "cannot change symbol value with SET");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2726 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 890 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(FALSE, FALSE, SFLAG_QUIET, (yyvsp[-2].symb), (yyvsp[0].intvec), 0,
                                (yyvsp[0].intvec)->len - 1,
                               "noncomputable expression for QSET",
                               "cannot change symbol value with QSET");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2738 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 898 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(TRUE, TRUE, 0, 
                                (yyvsp[-2].slidx).sym, (yyvsp[0].intvec), (yyvsp[-2].slidx).first, (yyvsp[-2].slidx).last,
                               "noncomputable expression for EQU",
                               "cannot change symbol value with EQU");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2750 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 906 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(TRUE, TRUE, SFLAG_QUIET, 
                                (yyvsp[-2].slidx).sym, (yyvsp[0].intvec), (yyvsp[-2].slidx).first, (yyvsp[-2].slidx).last,
                               "noncomputable expression for QEQU",
                               "cannot change symbol value with QEQU");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2762 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 914 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(TRUE, FALSE, 0, 
                                (yyvsp[-2].slidx).sym, (yyvsp[0].intvec), (yyvsp[-2].slidx).first, (yyvsp[-2].slidx).last,
                               "noncomputable expression for SET",
                               "cannot change symbol value with SET");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2774 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 922 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                do_set_equ_list(TRUE, FALSE, SFLAG_QUIET, 
                                (yyvsp[-2].slidx).sym, (yyvsp[0].intvec), (yyvsp[-2].slidx).first, (yyvsp[-2].slidx).last,
                               "noncomputable expression for QSET",
                               "cannot change symbol value with QSET");
                intvec_delete((yyvsp[0].intvec));
            }
#line 2786 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 930 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if (evalr[0].seg == SSG_ABS)
                {
                    if (evalr[0].value < 0)
                    {
                        fraerror("REPEAT count must be >= 0");
                        frarptpush(0);  /* treat it as a 0 count. */
                    } else
                    {
                        frarptpush(frarptskip ? 0 : evalr[0].value);
                    }
                } else
                {
                    fraerror("Computable expression required for REPEAT block");
                    frarptpush(0);  /* treat it as a 0 count. */
                }
            }
#line 2809 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 949 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (frarptact == 0)
                {
                    fraerror("ENDR without REPEAT");
                    frarptreset();  /* make sure repeat stack is reset. */
                } else
                {
                    frarptendr();   /* loop back to most recent REPEAT */
                }
            }
#line 2824 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 960 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (frarptcnt < 0)
                    fraerror("BRKIF without REPEAT");
                
                pevalexpr(0, (yyvsp[0].intv));
                if (evalr[0].seg == SSG_ABS)
                {
                    if (evalr[0].value != 0)
                        frarptbreak();  /* skip rest of repeat block */
                } else
                {
                    fraerror("Computable expression required for BRKIF");
                }
            }
#line 2843 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 975 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if      (stricmp((yyvsp[0].strng), "ON"  )==0) emit_listing_mode(LIST_ON);
                else if (stricmp((yyvsp[0].strng), "OFF" )==0) emit_listing_mode(LIST_OFF);
                else if (stricmp((yyvsp[0].strng), "CODE")==0) emit_listing_mode(LIST_CODE);
                else if (stricmp((yyvsp[0].strng), "PREV")==0) emit_listing_mode(LIST_PREV);
                else 
                {
                    fraerror("LISTING must be followed by \"ON\", \"OFF\" "
                             "or \"CODE\"");
                }
            }
#line 2859 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 987 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if      (stricmp((yyvsp[0].strng), "ON"  )==0) emit_listing_mode(LIST_ON);
                else if (stricmp((yyvsp[0].strng), "OFF" )==0) emit_listing_mode(LIST_OFF);
                else if (stricmp((yyvsp[0].strng), "CODE")==0) emit_listing_mode(LIST_CODE);
                else if (stricmp((yyvsp[0].strng), "PREV")==0) emit_listing_mode(LIST_PREV);
                else 
                {
                    fraerror("LISTING must be followed by \"ON\", \"OFF\" "
                             "or \"CODE\"");
                }

                if((yyvsp[-2].symb)->seg == SSG_UNDEF)
                {
                    (yyvsp[-2].symb)->seg   = SSG_ABS;
                    (yyvsp[-2].symb)->value = labelloc;
                }
                else
                    fraerror( "multiple definition of label");
            }
#line 2883 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 1007 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if((++ifstkpt) < IFSTKDEPTH)
                {
                    pevalexpr(0, (yyvsp[0].intv));
                    if(evalr[0].seg == SSG_ABS)
                    {
                        if(evalr[0].value != 0)
                        {
                            elseifstk[ifstkpt] = If_Skip;
                            endifstk[ifstkpt] = If_Active;
                        }
                        else
                        {
                            fraifskip = TRUE;
                            elseifstk[ifstkpt] = If_Active;
                            endifstk[ifstkpt] = If_Active;
                        }
                    }
                    else
                    {
                        fraifskip = TRUE;
                        elseifstk[ifstkpt] = If_Active;
                        endifstk[ifstkpt] = If_Active;
                    }
                    expmacstk[ifstkpt] = fraexpmac;
                    fraexpmac = (yyvsp[-1].intv);
                }
                else
                {
                    fraerror("IF stack overflow");
                }
            }
#line 2920 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 1041 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if(fraifskip) 
                {
                    if((++ifstkpt) < IFSTKDEPTH)
                    {
                            elseifstk[ifstkpt] = If_Skip;
                            endifstk[ifstkpt] = If_Skip;
                            expmacstk[ifstkpt] = fraexpmac;
                            fraexpmac = (yyvsp[0].intv);
                    }
                    else
                    {
                        fraerror("IF stack overflow");
                    }
                }
                else
                {
                    yyerror("syntax error");
                    YYERROR;
                }
            }
#line 2946 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 1064 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                switch(elseifstk[ifstkpt])
                {
                case If_Active:
                    fraifskip = FALSE;
                    break;
                
                case If_Skip:
                    fraifskip = TRUE;
                    break;
                
                case If_Err:
                    fraerror("ELSE with no matching if");
                    break;
                }
            }
#line 2967 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 1082 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                fraexpmac = expmacstk[ifstkpt];
                switch(endifstk[ifstkpt])
                {
                case If_Active:
                    fraifskip = FALSE;
                    ifstkpt--;
                    break;
                
                case If_Skip:
                    fraifskip = TRUE;
                    ifstkpt--;
                    break;
                
                case If_Err:
                    fraerror("ENDI with no matching if");
                    break;
                }
            }
#line 2991 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 1102 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = -1;
                    currmode = memoize_string("+R");
                    if((yyvsp[-2].symb)->seg == SSG_UNDEF)
                    {
                        (yyvsp[-2].symb)->seg   = SSG_ABS;
                        (yyvsp[-2].symb)->value = labelloc;
                    }
                    else
                        fraerror( "multiple definition of label");

                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror( "noncomputable expression for ORG");
                }
            }
#line 3019 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 1126 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = -1;
                    currmode = memoize_string("+R");
                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror(
                     "noncomputable expression for ORG");
                }
            }
#line 3040 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 1143 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-2].intv));
                pevalexpr(1, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = chkover(evalr[1].value, 0) - labelloc;
                    currpag  = -1;
                    currmode = memoize_string(currseg ? "" : "+R");
                    if((yyvsp[-4].symb)->seg == SSG_UNDEF)
                    {
                        (yyvsp[-4].symb)->seg   = SSG_ABS;
                        (yyvsp[-4].symb)->value = labelloc;
                    }
                    else
                        fraerror( "multiple definition of label");

                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror( "noncomputable expression for ORG");
                }
            }
#line 3069 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 1168 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-2].intv));
                pevalexpr(1, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = chkover(evalr[1].value, 0) - labelloc;
                    currpag  = -1;
                    currmode = memoize_string(currseg ? "" : "+R");
                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror(
                     "noncomputable expression for ORG");
                }
            }
#line 3091 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 1186 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-4].intv));
                pevalexpr(1, (yyvsp[-2].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    char *s = (yyvsp[0].strng);

                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = chkover(evalr[1].value, 0) - labelloc;
                    currpag  = -1;
                    currmode = memoize_string(s);

                    if((yyvsp[-6].symb)->seg == SSG_UNDEF)
                    {
                        (yyvsp[-6].symb)->seg = SSG_ABS;
                        (yyvsp[-6].symb)->value = labelloc;
                    }
                    else
                        fraerror( "multiple definition of label");

                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror( "noncomputable expression for ORG");
                }
            }
#line 3123 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 1214 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-4].intv));
                pevalexpr(1, (yyvsp[-2].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    char *s = (yyvsp[0].strng);

                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = chkover(evalr[1].value, 0) - labelloc;
                    currpag  = -1;
                    currmode = memoize_string(s);
                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror("noncomputable expression for ORG");
                }
            }
#line 3146 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 1233 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-2].intv));
                pevalexpr(1, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = evalr[1].value;
                    currmode = memoize_string("=R");
                    if((yyvsp[-4].symb)->seg == SSG_UNDEF)
                    {
                        (yyvsp[-4].symb)->seg   = SSG_ABS;
                        (yyvsp[-4].symb)->value = labelloc;
                    }
                    else
                        fraerror( "multiple definition of label");

                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror( "noncomputable expression for ORG");
                }
            }
#line 3175 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 1258 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-2].intv));
                pevalexpr(1, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = evalr[1].value;
                    currmode = memoize_string("=R");
                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror("noncomputable expression for ORG");
                }
            }
#line 3196 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 1275 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-4].intv));
                pevalexpr(1, (yyvsp[-2].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = evalr[1].value;
                    currmode = memoize_string((yyvsp[0].strng));
                    if((yyvsp[-6].symb)->seg == SSG_UNDEF)
                    {
                        (yyvsp[-6].symb)->seg   = SSG_ABS;
                        (yyvsp[-6].symb)->value = labelloc;
                    }
                    else
                        fraerror( "multiple definition of label");

                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror( "noncomputable expression for ORG");
                }
            }
#line 3225 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 1300 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-4].intv));
                pevalexpr(1, (yyvsp[-2].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = evalr[1].value;
                    currmode = memoize_string((yyvsp[0].strng));
                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror("noncomputable expression for ORG");
                }
            }
#line 3246 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 1317 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-4].intv));
                pevalexpr(1, (yyvsp[-2].intv));
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    const char *s = memoize_string((yyvsp[0].strng));
                    chkover(evalr[0].value, 0);
                    chkover(evalr[1].value, 0);
                    emit_location(0, -1, labelloc, TYPE_HOLE, s);
                    emit_mark_with_mode(evalr[0].value, evalr[1].value, s);
                    emit_location(currseg, currpag, labelloc, TYPE_HOLE, 
                                  currmode);
                }
                else
                {
                    fraerror("noncomputable expression for MEMATTR");
                }
            }
#line 3269 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 1336 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                const char *var   = memoize_string((yyvsp[-2].strng));
                const char *value = memoize_string((yyvsp[0].strng));
                emit_cfgvar_str(var, value);
            }
#line 3279 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 1342 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    const char *var = memoize_string((yyvsp[-2].strng));
                    emit_cfgvar_int(var, evalr[0].value);
                }
                else
                {
                    fraerror("noncomputable expression for CFGVAR");
                }
            }
#line 3296 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 1355 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                /* set the current source file override and line number */
                pevalexpr(0, (yyvsp[0].intv));
                if ( evalr[0].seg == SSG_ABS )
                {
                    if ( strlen( (yyvsp[-2].strng) ) == 0 || evalr[0].value < 1 )
                    {
                        emit_srcfile_override( NULL, 0 ); 
                    } else
                    {
                        emit_srcfile_override( memoize_string( (yyvsp[-2].strng) ), 
                                               evalr[0].value );
                    }
                }
                else
                {
                    fraerror("noncomputable expression for SRCFILE");
                }
            }
#line 3320 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 1375 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-4].intv));   /* Hex per line w/ source */
                pevalexpr(1, (yyvsp[-2].intv));   /* Hex per line w/out source */
                pevalexpr(2, (yyvsp[0].intv));   /* Starting column of source */
                if ( evalr[0].seg != SSG_ABS || evalr[1].seg != SSG_ABS ||
                     evalr[2].seg != SSG_ABS )
                {
                    fraerror("noncomputable expression for LISTCOL");
                } else
                {
                    const int new_hex_source = evalr[0].value;
                    const int new_hex_no_src = evalr[1].value;
                    const int new_source_ofs = evalr[2].value;
                    const int new_source_ofs_min = 7 + 5*new_hex_source;

                    if (new_hex_source < 1 || new_hex_source > 256 ||
                        new_hex_no_src < 1 || new_hex_no_src > 256 || 
                        new_source_ofs < 1 || new_source_ofs > 2048)
                    {
                        fraerror("value out of range value for LISTCOL");
                    } else
                    {
                        if (new_source_ofs < new_source_ofs_min)
                        {
                            fraerror("source column too small compared to "
                                     "hex-per-source-line for LISTCOL");
                        } else
                        {
                            emit_listing_column(new_hex_source,
                                                new_hex_no_src,
                                                new_source_ofs);
                        }
                    }
                }
            }
#line 3360 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 1411 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if ( evalr[0].seg != SSG_ABS )
                {
                    fraerror("noncomputable expression for ERR_IF_OVERWITTEN");
                } else
                {
                    emit_err_if_overwritten(evalr[0].value != 0);
                }
            }
#line 3375 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 1422 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if ( evalr[0].seg != SSG_ABS )
                {
                    fraerror("noncomputable expression for FORCE_OVERWRITE");
                } else
                {
                    emit_force_overwrite(evalr[0].value != 0);
                }
            }
#line 3390 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 1433 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if((yyvsp[-1].symb)->seg == SSG_UNDEF)
                {
                    (yyvsp[-1].symb)->seg = SSG_EQU;
                    if( ((yyvsp[-1].symb)->value = chtcreate()) <= 0)
                    {
                        fraerror("cannot create character translation table");
                    }
                    emit_set_equ((yyvsp[-1].symb)->value);
                }
                else
                {
                    fraerror("multiple definition of label");
                }
            }
#line 3410 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 1449 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                chtcpoint = (int *) NULL;
                emit_set_equ(0L);
            }
#line 3419 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 1454 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if( evalr[0].seg == SSG_ABS)
                {
                    if( evalr[0].value == 0)
                    {
                        chtcpoint = (int *)NULL;
                        emit_set_equ(0L);
                    }
                    else if(evalr[0].value < chtnxalph)
                    {
                        chtcpoint = chtatab[evalr[0].value];
                        emit_set_equ(evalr[0].value);
                    }
                    else
                    {
                        fraerror("nonexistent character translation table");
                    }
                }
                else
                {
                    fraerror("noncomputable expression");
                }
            }
#line 3448 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 1479 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                chardef((yyvsp[-2].strng), (yyvsp[0].intvec));
                intvec_delete((yyvsp[0].intvec));
            }
#line 3457 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 1484 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                char st[2] = { (yyvsp[-2].longv), 0 };
                chardef(st, (yyvsp[0].intvec));
                intvec_delete((yyvsp[0].intvec));
            }
#line 3467 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 1490 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if((yyvsp[0].symb)->seg == SSG_UNDEF)
                {
                    (yyvsp[0].symb)->seg = SSG_ABS;
                    (yyvsp[0].symb)->value = chkover(labelloc, 0);
                    emit_set_equ(labelloc);

                }
                else
                    fraerror("multiple definition of label");
            }
#line 3483 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 1504 "asm/as1600_real.y" /* yacc.c:1646  */
    { (yyval.intv) = 1; }
#line 3489 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 1505 "asm/as1600_real.y" /* yacc.c:1646  */
    { (yyval.intv) = 0; }
#line 3495 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 1509 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (sdbd)
                    frawarn("label between SDBD and instruction");

                if((yyvsp[-1].symb)->seg == SSG_UNDEF)
                {
                    (yyvsp[-1].symb)->seg   = SSG_ABS;
                    (yyvsp[-1].symb)->value = chkover(labelloc, 0);
                }
                else
                    fraerror("multiple definition of label");

                if (locctr & 1) fraerror("internal error: PC misaligned.");

                labelloc = locctr >> 1;

                sdbd    = is_sdbd;
                is_sdbd = 0;
                first   = 0;
            }
#line 3520 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 1530 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (locctr & 1) fraerror("internal error: PC misaligned.");
                labelloc = locctr >> 1;

                sdbd    = is_sdbd;
                is_sdbd = 0;
                first   = 0;
            }
#line 3533 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 1541 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc, TYPE_DATA, currmode);
                evalr[2].seg   = SSG_ABS;
                evalr[2].value = 8;
                for( satsub = 0; satsub < (yyvsp[0].intvec)->len; satsub++)
                {
                    pevalexpr(1, (yyvsp[0].intvec)->data[satsub]);
                    locctr += geninstr(genbdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete((yyvsp[0].intvec));
            }
#line 3550 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 1554 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc, TYPE_DATA, currmode);
                evalr[2].seg   = SSG_ABS;
                evalr[2].value = romw;
                for( satsub = 0; satsub < (yyvsp[0].intvec)->len; satsub++)
                {
                    pevalexpr(1, (yyvsp[0].intvec)->data[satsub]);
                    locctr += geninstr(genbdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete((yyvsp[0].intvec));
            }
#line 3567 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 1568 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc, TYPE_STRING, currmode);
                evalr[2].seg   = SSG_ABS;
                evalr[2].value = romw;
                for( satsub = 0; satsub < (yyvsp[0].intvec)->len; satsub++)
                {
                    pevalexpr(1, (yyvsp[0].intvec)->data[satsub]);
                    locctr += geninstr(genbdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete((yyvsp[0].intvec));
            }
#line 3584 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 1581 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc,
                              TYPE_DBDATA|TYPE_DATA, currmode);
                for( satsub = 0; satsub < (yyvsp[0].intvec)->len; satsub++)
                {
                    pevalexpr(1, (yyvsp[0].intvec)->data[satsub]);
                    locctr += geninstr(genwdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete((yyvsp[0].intvec));
            }
#line 3600 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 1593 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    locctr = 2 * chkover(labelloc + evalr[0].value, 1);
                    emit_set_equ(labelloc);
                    emit_location(currseg, currpag, labelloc, TYPE_HOLE, 
                                  currmode);
                    emit_reserve(labelloc + evalr[0].value - 1);
                }
                else
                {
                    fraerror("noncomputable expression for RMB");
                }
            }
#line 3620 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 1619 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                intvec_push((yyvsp[-2].intvec), (yyvsp[0].intv));
                (yyval.intvec) = (yyvsp[-2].intvec);
            }
#line 3629 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 1624 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                char *s = (yyvsp[0].strng);
                int  accval = 0;

                while (*s)
                {
                    accval = chtran(&s);
                    intvec_push((yyvsp[-2].intvec), 
                        exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL));
                }
                (yyval.intvec) = (yyvsp[-2].intvec);
            }
#line 3646 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 1637 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                intvec_t *const RESTRICT iv = intvec_new();
                intvec_push(iv, (yyvsp[0].intv));
                (yyval.intvec) = iv;
            }
#line 3656 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 1643 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                intvec_t *const RESTRICT iv = intvec_new();
                char *s = (yyvsp[0].strng);
                int  accval = 0;

                while (*s)
                {
                    accval = chtran(&s);
                    intvec_push(iv,
                        exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL));
                }
                (yyval.intvec) = iv;
            }
#line 3674 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 1657 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                const struct tm *t = (yyvsp[-3].longv) ? &asm_time_gmt : &asm_time_local;
                (yyval.intvec) = unpack_time_exprs(t, &asm_time_gmt, (yyvsp[-1].strng));
            }
#line 3683 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 1662 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                intvec_concat((yyvsp[-2].intvec), (yyvsp[0].intvec));
                intvec_delete((yyvsp[0].intvec));
                (yyval.intvec) = (yyvsp[-2].intvec);
            }
#line 3693 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 1668 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intvec) = (yyvsp[0].intvec);
            }
#line 3701 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 1675 "asm/as1600_real.y" /* yacc.c:1646  */
    {   
                char *s = &tempstr[tempstrlen];

                if (chtcpoint != NULL)
                {
                    frawarn("Stringifying expression list while character "
                            "translation active");
                }

                tempstrlen += (yyvsp[-1].intvec)->len + 1;

                if (tempstrlen > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    (yyval.strng) = "";
                } else
                {
                    int i;
                    (yyval.strng) = s;
                    for (i = 0; i < (yyvsp[-1].intvec)->len; i++)
                    {
                        pevalexpr(0, (yyvsp[-1].intvec)->data[i]);

                        if (evalr[0].seg == SSG_ABS)
                            *s++ = evalr[0].value;
                        else
                            *s++ = '?';
                    }
                    *s = 0;
                }
                intvec_delete((yyvsp[-1].intvec));
            }
#line 3738 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 1708 "asm/as1600_real.y" /* yacc.c:1646  */
    {   
                char *s = &tempstr[tempstrlen];

                if (tempstrlen + 32 > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    (yyval.strng) = "";
                } else
                {
                    (yyval.strng) = s;
                    pevalexpr(0, (yyvsp[-1].intv));

                    if (evalr[0].seg == SSG_ABS)
                        sprintf(s, "%d", (int)evalr[0].value);
                    else
                    {
                        s[0] = '?';
                        s[1] = 0;
                    }
                    tempstrlen += strlen(s) + 1;
                }
            }
#line 3765 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 1731 "asm/as1600_real.y" /* yacc.c:1646  */
    {   
                char *s = &tempstr[tempstrlen];

                if (tempstrlen + 5 > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    (yyval.strng) = "";
                } else
                {
                    (yyval.strng) = s;
                    pevalexpr(0, (yyvsp[-1].intv));

                    if (evalr[0].seg == SSG_ABS)
                        sprintf(s, "%4.4X", 
                                (unsigned int)(0xFFFF & evalr[0].value));
                    else
                    {
                        s[0] = '?';
                        s[1] = '?';
                        s[2] = '?';
                        s[3] = '?';
                        s[4] = 0;
                    }
                    tempstrlen += 5;
                }
            }
#line 3796 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 1758 "asm/as1600_real.y" /* yacc.c:1646  */
    {   
                char *s = &tempstr[tempstrlen];

                if (tempstrlen + 5 > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    (yyval.strng) = "";
                } else
                {
                    (yyval.strng) = s;
                    pevalexpr(0, (yyvsp[-1].intv));

                    if (evalr[0].seg == SSG_ABS)
                        sprintf(s, "%8.8X", 
                                (unsigned int)(0xFFFFFFFF & evalr[0].value));
                    else
                    {
                        s[0] = '?';
                        s[1] = '?';
                        s[2] = '?';
                        s[3] = '?';
                        s[4] = '?';
                        s[5] = '?';
                        s[6] = '?';
                        s[7] = '?';
                        s[8] = 0;
                    }
                    tempstrlen += 5;
                }
            }
#line 3831 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 1789 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                const struct tm *t = (yyvsp[-3].longv) ? &asm_time_gmt : &asm_time_local;
                char *const bufbeg = &tempstr[tempstrlen];
                const int avail = MAXTEMPSTR - tempstrlen;
                const int len = format_time_string(t, &asm_time_gmt,
                                                   (yyvsp[-1].strng), bufbeg, avail);
                tempstrlen += len;
                (yyval.strng) = bufbeg;
            }
#line 3845 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 1804 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (proc && struct_locctr != -1)
                    fraerror("PROC cannot nest inside STRUCT.");
                else if (proc && proc_stk_depth == MAX_PROC_STK)
                    fraerror("PROC nesting limit reached.");
                else if (((yyvsp[-1].symb)->flags & SFLAG_ARRAY) != 0)
                    fraerror("array element can not be defined by PROC");
                else if ((yyvsp[-1].symb)->seg != SSG_UNDEF)
                    fraerror("multiple definition of label");
                else
                {
                    if (proc)
                    {
                        char *old_proc     = proc;
                        int   old_proc_len = proc_len;
                        proc_stk[proc_stk_depth++] = proc;
                        proc_len = strlen(proc) + strlen((yyvsp[-1].symb)->symstr) + 1;
                        proc     = (char *)malloc(proc_len + 1);
                        strcpy(proc, old_proc);
                        proc[old_proc_len] = '.';
                        strcpy(proc + old_proc_len + 1, (yyvsp[-1].symb)->symstr);
                    } else
                    {
                        proc     = strdup((yyvsp[-1].symb)->symstr);
                        proc_len = strlen(proc);
                    }

                    (yyvsp[-1].symb)->seg   = SSG_ABS;
                    (yyvsp[-1].symb)->value = labelloc;
                    emit_set_equ(labelloc);
                }
            }
#line 3882 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 1838 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (!proc || struct_locctr != -1)
                    fraerror("ENDP w/out PROC.");

                free(proc);

                if (proc_stk_depth > 0)
                {
                    proc     = proc_stk[--proc_stk_depth];
                    proc_len = strlen(proc);
                } else
                {
                    proc     = NULL;
                    proc_len = 0;
                }
            }
#line 3903 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 1859 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[0].intv));

                if (proc)
                    fraerror("STRUCT can not nest inside other STRUCTs or PROCs.");
                else if (((yyvsp[-2].symb)->flags & SFLAG_ARRAY) != 0)
                    fraerror("array element can not be defined by STRUCT");
                else if (evalr[0].seg != SSG_ABS)
                    fraerror( "noncomputable expression for ORG");
                else if ((yyvsp[-2].symb)->seg != SSG_UNDEF)
                    fraerror( "multiple definition of label");
                else
                {
                    proc     = strdup((yyvsp[-2].symb)->symstr);
                    proc_len = strlen(proc);
                    struct_locctr = locctr;

                    locctr = 2 * chkover(labelloc = evalr[0].value, 0);

                    (yyvsp[-2].symb)->seg = SSG_ABS;
                    (yyvsp[-2].symb)->value = labelloc;

                    emit_set_equ(evalr[0].value);
                }
            }
#line 3933 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 1886 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (!proc || struct_locctr == -1)
                    fraerror("ENDS w/out STRUCT.");
                else
                {
                    free(proc);
                    proc     = NULL;
                    proc_len = 0;
                    locctr = struct_locctr;
                    struct_locctr = -1;
                }
            }
#line 3950 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 1903 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc, TYPE_HOLE, currmode);
                pevalexpr(0, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    romw = evalr[0].value;

                    if (romw < 8 || romw > 16)
                        fraerror("ROMWIDTH out of range");

                    romm = 0xFFFFU >> (16 - romw);
                }
                else
                {
                    fraerror("noncomputable expression for ROMWIDTH");
                }

                if (!first)
                {
                    frawarn("Code appears before ROMW directive.");
                }

                fwd_sdbd = 0;
            }
#line 3979 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 1928 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc, TYPE_HOLE, currmode);
                pevalexpr(0, (yyvsp[-2].intv));
                pevalexpr(1, (yyvsp[0].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    romw = evalr[0].value;

                    if (romw < 8 || romw > 16)
                        fraerror("ROMWIDTH out of range");

                    romm = 0xFFFFU >> (16 - romw);
                }
                else
                    fraerror("noncomputable expression for ROMWIDTH");

                if (!first)
                {
                    frawarn("Code appears before ROMW directive.");
                }

                if (evalr[1].seg == SSG_ABS)
                {
                    fwd_sdbd = evalr[1].value;

                    if (fwd_sdbd > 1 || fwd_sdbd < 0)
                        fraerror("SDBD mode flag must be 0 or 1.");
                } else
                    fraerror("noncomputable expression for ROMWIDTH");

            }
#line 4015 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 1965 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                if (sdbd)
                    frawarn("Two SDBDs in a row.");

                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                locctr += geninstr(findgen((yyvsp[0].intv), ST_IMP, 0));
                chkover(locctr >> 1, 1);
                is_sdbd = SDBD;
            }
#line 4029 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 1980 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                /*unsigned rel_addr = labelloc + 2;*/
                /*int dir;*/

                SDBD_CHK

                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, (yyvsp[0].intv));

                evalr[3].seg   = SSG_ABS;
                evalr[3].value = romw;

                locctr += geninstr(findgen((yyvsp[-1].intv), ST_EXP, sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4049 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 1997 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                /*unsigned rel_addr = labelloc + 2;*/
                /*int dir;*/

                SDBD_CHK

                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, (yyvsp[-2].intv));
                pevalexpr(4, (yyvsp[0].intv));

                if (evalr[4].seg != SSG_ABS)
                    fraerror("Must have constant expr for BEXT condition");

                evalr[3].seg   = SSG_ABS;
                evalr[3].value = romw;

                locctr += geninstr(findgen((yyvsp[-3].intv), ST_EXPEXP, sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4073 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 2023 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                locctr += geninstr(findgen((yyvsp[0].intv), ST_IMP, sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4084 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 2035 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, (yyvsp[0].intv));
                locctr += geninstr(findgen((yyvsp[-1].intv), ST_EXP, sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4096 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 2049 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = (yyvsp[-2].intv);
                pevalexpr(2, (yyvsp[0].intv));
                evalr[3].seg    = SSG_ABS;
                evalr[3].value  = romw;
                locctr += geninstr(findgen((yyvsp[-3].intv), ST_REGEXP, reg_type[(yyvsp[-2].intv)]|sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4111 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 2065 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value  = (yyvsp[-3].intv);
                evalr[3].seg    = SSG_ABS;
                evalr[3].value  = romw;
                pevalexpr(2, (yyvsp[0].intv));
                locctr += geninstr(findgen((yyvsp[-4].intv), ST_REGCEX, reg_type[(yyvsp[-3].intv)]|sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4126 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 2081 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, (yyvsp[-2].intv));
                evalr[2].value = (yyvsp[0].intv);
                evalr[3].seg   = SSG_ABS;
                evalr[3].value = romw;
                locctr += geninstr(findgen((yyvsp[-3].intv), ST_EXPREG, reg_type[(yyvsp[0].intv)]|sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4141 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 2097 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, (yyvsp[-2].intv));
                evalr[2].value = (yyvsp[0].intv);

                evalr[3].seg   = SSG_ABS;
                evalr[3].value = romw;

                if (sdbd == 0 && romw != 16)
                {
                    if (evalr[1].seg == SSG_ABS && 
                        (0xFFFF & evalr[1].value & ~romm) != 0)
                    {
                        /*frawarn("Constant is wider than ROM width.  "
                                "Inserting SDBD.");*/
                        locctr += geninstr("0001x");
                        sdbd = SDBD;
                    }

                    if (evalr[1].seg != SSG_ABS && fwd_sdbd)
                    {   
                        frawarn("Inserting SDBD due to forward reference.");
                        locctr += geninstr("0001x");
                        sdbd = SDBD;
                    }
                }

                locctr += geninstr(findgen((yyvsp[-4].intv), ST_CEXREG, reg_type[(yyvsp[0].intv)]|sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4176 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 2133 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = (yyvsp[0].intv);
                locctr += geninstr(findgen((yyvsp[-1].intv), ST_REG, reg_type[(yyvsp[0].intv)]|sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4188 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 2146 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = (yyvsp[-2].intv);
                evalr[2].value = (yyvsp[0].intv);
                locctr += geninstr(findgen((yyvsp[-3].intv), ST_REGREG, reg_type[(yyvsp[-2].intv)]|sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4201 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 2160 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = (yyvsp[-2].intv);
                evalr[2].value = (yyvsp[0].intv);
                locctr += geninstr(findgen((yyvsp[-3].intv), ST_REGREG, reg_type[(yyvsp[-2].intv)]|sdbd));
                chkover(locctr >> 1, 1);
            }
#line 4213 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 2174 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = (yyvsp[0].intv);
            }
#line 4221 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 2178 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_UN,(yyvsp[0].intv),IFC_NEG,0,0L, SYMNULL);
            }
#line 4229 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 2182 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_UN,(yyvsp[0].intv),IFC_NOT,0,0L, SYMNULL);
            }
#line 4237 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 2186 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_UN,(yyvsp[0].intv),IFC_HIGH,0,0L, SYMNULL);
            }
#line 4245 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 2190 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_UN,(yyvsp[0].intv),IFC_LOW,0,0L, SYMNULL);
            }
#line 4253 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 2194 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_MUL,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4261 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 2198 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_DIV,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4269 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 2202 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_ADD,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4277 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 2206 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_SUB,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4285 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 2210 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_MOD,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4293 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 2214 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_SHL,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4301 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 2218 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_SHR,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4309 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 2222 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_SHRU,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4317 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 2226 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                const int ifc = (yyvsp[-1].intv) == 16 ? IFC_ROTL16 : IFC_ROTL32;
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),ifc,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4326 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 2231 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                const int ifc = (yyvsp[-1].intv) == 16 ? IFC_ROTL16 : IFC_ROTL32;
                const int neg = exprnode(PCCASE_UN,(yyvsp[0].intv),IFC_NEG,0,0L, SYMNULL);
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),ifc,neg,0L, SYMNULL);
            }
#line 4336 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 2237 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_GT,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4344 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 2241 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_GE,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4352 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 2245 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_LT,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4360 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 2249 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_LE,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4368 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 2253 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_NE,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4376 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 2257 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_EQ,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4384 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 2261 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_AND,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4392 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 2265 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_OR,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4400 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 2269 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_BIN,(yyvsp[-2].intv),IFC_XOR,(yyvsp[0].intv),0L, SYMNULL);
            }
#line 4408 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 2273 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_DEF,0,IGP_DEFINED,0,0L,(yyvsp[0].symb));
            }
#line 4416 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 2277 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_SYMB,0,IFC_SYMB,0,0L,(yyvsp[0].symb));
            }
#line 4424 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 2281 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,(yyvsp[0].longv), SYMNULL);
            }
#line 4432 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 2285 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,(yyvsp[0].longv), SYMNULL);
            }
#line 4440 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 2289 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_PROGC,0,IFC_PROGCTR,0,labelloc,SYMNULL);
            }
#line 4448 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 2293 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,(yyvsp[0].longv), SYMNULL);
            }
#line 4456 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 2297 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                char st[2] = { (yyvsp[0].longv), 0 }, *s = st;
                int  accval = chtran(&s);
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL);
            }
#line 4466 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 2303 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_EMPTY,SYMNULL);
            }
#line 4474 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 2307 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0, (yyvsp[-1].intv), SYMNULL);
            }
#line 4482 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 2311 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_FEATURE,
                             SYMNULL);
            }
#line 4491 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 2316 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_RESV,SYMNULL);
            }
#line 4499 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 2320 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_STRING,
                              SYMNULL);
            }
#line 4508 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 2325 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CLASSSYM,0,IFC_CLASSIFY,0,0L,(yyvsp[-1].symb));
            }
#line 4516 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 2329 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_UN,(yyvsp[-1].intv),IFC_CLASSIFY,0,0L, SYMNULL);
            }
#line 4524 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 2333 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                char *s = (yyvsp[-1].strng);
                int  accval = 0;
                int  length = 0;

                while (*s)
                {
                    accval = chtran(&s);
                    length++;
                }
                (void)accval;
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,length++,SYMNULL);
            }
#line 4542 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 2347 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,1,SYMNULL);
            }
#line 4550 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 2351 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                char *s = (yyvsp[-3].strng);
                int  accval = 0;
                int  sindex = 0;

                pevalexpr(0, (yyvsp[-1].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    sindex = evalr[0].value;
                    while (*s && sindex >= 0)
                    {
                        accval = chtran(&s);
                        sindex--;
                    }
                    if (sindex >= 0) 
                        accval = 0;

                    (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL);
                }
                else
                {
                    fraerror("noncomputable expression for index to ASC");
                }
            }
#line 4579 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 2376 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                char st[2] = { (yyvsp[-3].longv), 0 }, *s = st;
                int  accval = 0;

                pevalexpr(0, (yyvsp[-1].intv));
                if(evalr[0].seg == SSG_ABS)
                {
                    accval = evalr[0].value == 0 ? chtran(&s) : 0;
                    (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL);
                }
                else
                {
                    fraerror("noncomputable expression for index to ASC");
                }
            }
#line 4599 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 2391 "asm/as1600_real.y" /* yacc.c:1646  */
    { (yyval.intv) = (yyvsp[-1].intv); }
#line 4605 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 2393 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-1].intv));
                if (evalr[0].seg == SSG_ABS)
                {
                    const int idx = evalr[0].value;
                    if (idx < (yyvsp[-4].intvec)->len)
                        (yyval.intv) = (yyvsp[-4].intvec)->data[idx];
                    else
                        (yyval.intv) = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,0,SYMNULL);
                }
                else
                {
                    fraerror("noncomputable expression for expr-list index");
                }
                intvec_delete((yyvsp[-4].intvec));
            }
#line 4626 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 2416 "asm/as1600_real.y" /* yacc.c:1646  */
    {   
                pevalexpr(0, (yyvsp[-1].intv));
                if (evalr[0].seg == SSG_ABS)
                {
                    (yyval.symb) = symbentryidx((yyvsp[-3].symb)->symstr, LABEL, 1, evalr[0].value);
                    (yyval.symb)->flags |= SFLAG_QUIET | SFLAG_ARRAY;
                   
                    /* track "high water mark" in LABEL's own value */
                    (yyvsp[-3].symb)->seg    = SSG_SET;
                    (yyvsp[-3].symb)->flags |= SFLAG_QUIET;
                    if ((yyvsp[-3].symb)->value < evalr[0].value)
                        (yyvsp[-3].symb)->value = evalr[0].value;
                } else
                {
                    fraerror("noncomputable expression for label array index");
                }
            }
#line 4648 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 2437 "asm/as1600_real.y" /* yacc.c:1646  */
    {   
                pevalexpr(0, (yyvsp[-1].intv));
                if (evalr[0].seg == SSG_ABS)
                {
                    (yyval.symb) = symbentryidx((yyvsp[-3].symb)->symstr, LABEL, 1, evalr[0].value);
                    (yyval.symb)->flags |= SFLAG_QUIET | SFLAG_ARRAY;
                    
                    /* track "high water mark" in LABEL's own value */
                    (yyvsp[-3].symb)->seg    = SSG_SET;
                    (yyvsp[-3].symb)->flags |= SFLAG_QUIET;
                    if ((yyvsp[-3].symb)->value < evalr[0].value)
                        (yyvsp[-3].symb)->value = evalr[0].value;
                } else
                {
                    fraerror("noncomputable expression for symbol array index");
                }
            }
#line 4670 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 2458 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                intvec_t *const RESTRICT iv = intvec_new();
                pevalexpr(0, (yyvsp[-3].intv));
                pevalexpr(1, (yyvsp[-1].intv));

                if (evalr[0].seg != SSG_ABS || evalr[1].seg != SSG_ABS)
                {
                    fraerror("noncomputable expression for symbol slice index");
                    (yyval.intvec) = iv;
                } else
                {
                    int i, s;

                    s = evalr[0].value > evalr[1].value ? -1 : 1;

                    for (i = evalr[0].value; i != evalr[1].value + s; i += s)
                    {
                        struct symel *sym;
                        intvec_push(iv,
                            exprnode(PCCASE_SYMB,0,IFC_SYMB,0,0L,
                                sym = symbentryidx((yyvsp[-5].symb)->symstr, LABEL, 1, i)));

                        sym->flags |= SFLAG_ARRAY | SFLAG_QUIET;
                    }
                    (yyval.intvec) = iv;
                }
            }
#line 4702 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 2486 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-3].intv));
                pevalexpr(1, (yyvsp[-1].intv));
                if (evalr[0].seg != SSG_ABS || evalr[1].seg != SSG_ABS)
                {
                    fraerror(
                        "noncomputable expression for expr-list slice index");
                    intvec_resize((yyvsp[-6].intvec), 0);
                    (yyval.intvec) = (yyvsp[-6].intvec);
                } else if (evalr[0].value >= (yyvsp[-6].intvec)->len || 
                           evalr[1].value >= (yyvsp[-6].intvec)->len)
                {
                    fraerror("out of range index for expr-list slice");
                    intvec_resize((yyvsp[-6].intvec), 0);
                    (yyval.intvec) = (yyvsp[-6].intvec);
                } else
                {
                    const int rev = evalr[0].value > evalr[1].value;
                    const int lo = rev ? evalr[1].value : evalr[0].value;
                    const int hi = rev ? evalr[0].value : evalr[1].value;
                    const int cnt = hi - lo + 1;
                    int *const data = (yyvsp[-6].intvec)->data;

                    memmove(&data[0], &data[lo], sizeof(data[0]) * cnt);

                    if (rev)
                    {
                        int i, j;
                        for (i = 0, j = cnt - 1; i < j; i++, j--)
                        {
                            const int tmp = (yyvsp[-6].intvec)->data[i];
                            (yyvsp[-6].intvec)->data[i] = (yyvsp[-6].intvec)->data[j];
                            (yyvsp[-6].intvec)->data[j] = tmp;
                        }
                    }
                    (yyvsp[-6].intvec)->len = cnt;
                    (yyval.intvec) = (yyvsp[-6].intvec);
                }
            }
#line 4746 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 2526 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                fraerror("array slice allowed on last index only");
                (yyval.intvec) = intvec_new();
            }
#line 4755 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 2531 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                fraerror("array slice allowed on last index only");
                (yyval.intvec) = intvec_new();
            }
#line 4764 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 2539 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                pevalexpr(0, (yyvsp[-3].intv));
                pevalexpr(1, (yyvsp[-1].intv));

                if (evalr[0].seg != SSG_ABS || evalr[1].seg != SSG_ABS)
                {
                    fraerror("noncomputable expression for label slice index");
                } else
                {
                    (yyval.slidx).first = evalr[0].value;
                    (yyval.slidx).last  = evalr[1].value;
                    (yyval.slidx).sym   = (yyvsp[-5].symb);
                }
            }
#line 4783 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 2554 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                fraerror("array slice allowed on last index only");
            }
#line 4791 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 2558 "asm/as1600_real.y" /* yacc.c:1646  */
    {
                fraerror("array slice allowed on last index only");
            }
#line 4799 "asm/as1600.tab.c" /* yacc.c:1646  */
    break;


#line 4803 "asm/as1600.tab.c" /* yacc.c:1646  */
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
#line 2587 "asm/as1600_real.y" /* yacc.c:1906  */


int lexintercept(void)
/*
    description intercept the call to yylex (the lexical analyzer)
            and filter out all unnecessary tokens when skipping
            the input between a failed IF and its matching ENDI or
            ELSE
    globals     fraifskip   the enable flag
*/
{
#undef yylex
    int rv;

    if (!(frarptskip | fraifskip))
    {
        int token = yylex();
        return token;
    }

    if(frarptskip)
    {
        for(;;)
        {

            switch(rv = yylex())
            {
            case 0:
            case KOC_END:
            case KOC_ENDR:
            case EOL:
                return rv;
            case KOC_RPT:
                frarptpush(0);  /* push a dummy loop */
            default:
                break;
            }
        }
    } else if(fraifskip)
    {
        for(;;)
        {

            switch(rv = yylex())
            {
            case 0:
            case KOC_END:
            case KOC_IF:
            case KOC_ELSE:
            case KOC_ENDI:
            case KEOP_EXPMAC:
            case EOL:
                return rv;
            default:
                break;
            }
        }
    } else
    {
        int token = yylex();
        return token;
    }

#define yylex lexintercept
}



void setreserved(void)
{
    reservedsym("and",           KEOP_AND,       0);
    reservedsym("defined",       KEOP_DEFINED,   0);
    reservedsym("ge",            KEOP_GE,        0);
    reservedsym("high",          KEOP_HIGH,      0);
    reservedsym("le",            KEOP_LE,        0);
    reservedsym("low",           KEOP_LOW,       0);
    reservedsym("mod",           KEOP_MOD,       0);
    reservedsym("ne",            KEOP_NE,        0);
    reservedsym("not",           KEOP_NOT,       0);
    reservedsym("or",            KEOP_OR,        0);
    reservedsym("shl",           KEOP_SHL,       0);
    reservedsym("shr",           KEOP_SHR,       0);
    reservedsym("shru",          KEOP_SHRU,      0);
    reservedsym("xor",           KEOP_XOR,       0);
    reservedsym("AND",           KEOP_AND,       0);
    reservedsym("DEFINED",       KEOP_DEFINED,   0);
    reservedsym("GE",            KEOP_GE,        0);
    reservedsym("HIGH",          KEOP_HIGH,      0);
    reservedsym("LE",            KEOP_LE,        0);
    reservedsym("LOW",           KEOP_LOW,       0);
    reservedsym("MOD",           KEOP_MOD,       0);
    reservedsym("NE",            KEOP_NE,        0);
    reservedsym("NOT",           KEOP_NOT,       0);
    reservedsym("OR",            KEOP_OR,        0);
    reservedsym("SHL",           KEOP_SHL,       0);
    reservedsym("SHR",           KEOP_SHR,       0);
    reservedsym("SHRU",          KEOP_SHRU,      0);
    reservedsym("_ROTL16",       KEOP_ROTL,      16);
    reservedsym("_ROTL32",       KEOP_ROTL,      32);
    reservedsym("_ROTR16",       KEOP_ROTR,      16);
    reservedsym("_ROTR32",       KEOP_ROTR,      32);
    reservedsym("XOR",           KEOP_XOR,       0);
    reservedsym("STRLEN",        KEOP_STRLEN,    0);
    reservedsym("ASC",           KEOP_ASC,       0);
    reservedsym("CLASSIFY",      KEOP_CLASSIFY,  0);
    reservedsym("TODAY_STR_LOC", KEOP_TODAY_STR, 0);
    reservedsym("TODAY_STR_GMT", KEOP_TODAY_STR, 1);
    reservedsym("TODAY_VAL_LOC", KEOP_TODAY_VAL, 0);
    reservedsym("TODAY_VAL_GMT", KEOP_TODAY_VAL, 1);
    reservedsym("_EXPMAC",       KEOP_EXPMAC,    1);

    /* machine specific token definitions */
    reservedsym("r0",       CP1600_REG,     0);
    reservedsym("r1",       CP1600_REG,     1);
    reservedsym("r2",       CP1600_REG,     2);
    reservedsym("r3",       CP1600_REG,     3);
    reservedsym("r4",       CP1600_REG,     4);
    reservedsym("r5",       CP1600_REG,     5);
    reservedsym("r6",       CP1600_REG,     6);
    reservedsym("r7",       CP1600_REG,     7);
    reservedsym("sp",       CP1600_REG,     6);
    reservedsym("pc",       CP1600_REG,     7);
    reservedsym("R0",       CP1600_REG,     0);
    reservedsym("R1",       CP1600_REG,     1);
    reservedsym("R2",       CP1600_REG,     2);
    reservedsym("R3",       CP1600_REG,     3);
    reservedsym("R4",       CP1600_REG,     4);
    reservedsym("R5",       CP1600_REG,     5);
    reservedsym("R6",       CP1600_REG,     6);
    reservedsym("R7",       CP1600_REG,     7);
    reservedsym("SP",       CP1600_REG,     6);
    reservedsym("PC",       CP1600_REG,     7);

    reservedsym("__FEATURE.MACRO",     FEATURE, 99);
    reservedsym("__FEATURE.CFGVAR",    FEATURE, 99);
    reservedsym("__FEATURE.SRCFILE",   FEATURE, 99);
    reservedsym("__FEATURE.CLASSIFY",  FEATURE, 99);
    reservedsym("__FEATURE.TODAY",     FEATURE, 99);
    reservedsym("__FEATURE.ROTATE",    FEATURE, 99);
    reservedsym("__FEATURE.EXPMAC",    FEATURE, 99);
    reservedsym("__FEATURE.LISTCOL",   FEATURE, 99);
    reservedsym("__FEATURE.OVERWRITE", FEATURE, 99);
    reservedsym("MACRO",               FEATURE, 99);
    reservedsym("ENDM",                FEATURE, 99);
}

int cpumatch(char *str)
{
    (void)str;
    return TRUE;
}


/* ======================================================================== */
/*  Opcode and Instruction Generation Tables                                */
/*                                                                          */
/*  These tables are used by the assembler framework to generate            */
/*  instructions from the parsed input.                                     */
/*                                                                          */
/*  OPTAB    -- OPcode TABle.  Contains the set of supported mnemonics.     */
/*  OSTAB    -- Opcode Syntax TABle.  Syntax definition sets for instrs.    */
/*  IGTAB    -- Instruction Generation TABle.  Contains RPN code for        */
/*              generating the instructions.                                */
/* ======================================================================== */



/* ======================================================================== */
/*  OPTAB    -- OPcode TABle.  Contains the set of supported mnemonics.     */
/* ======================================================================== */
struct opsym optab[] =
{
    {   "invalid",  KOC_opcode,     2,  0   },

    {   "MVO",      KOC_opcode,     1,  2   },
    {   "MVI",      KOC_opcode,     1,  3   },
    {   "ADD",      KOC_opcode,     1,  4   },
    {   "SUB",      KOC_opcode,     1,  5   },
    {   "CMP",      KOC_opcode,     1,  6   },
    {   "AND",      KOC_opcode,     1,  7   },
    {   "XOR",      KOC_opcode,     1,  8   },

    {   "MVO@",     KOC_opcode,     1,  9   },
    {   "MVI@",     KOC_opcode_i,   1,  10  },
    {   "ADD@",     KOC_opcode_i,   1,  11  },
    {   "SUB@",     KOC_opcode_i,   1,  12  },
    {   "CMP@",     KOC_opcode_i,   1,  13  },
    {   "AND@",     KOC_opcode_i,   1,  14  },
    {   "XOR@",     KOC_opcode_i,   1,  15  },

    {   "MVOI",     KOC_opcode,     1,  16  },
    {   "MVII",     KOC_opcode,     1,  17  },
    {   "ADDI",     KOC_opcode,     1,  18  },
    {   "SUBI",     KOC_opcode,     1,  19  },
    {   "CMPI",     KOC_opcode,     1,  20  },
    {   "ANDI",     KOC_opcode,     1,  21  },
    {   "XORI",     KOC_opcode,     1,  22  },

    {   "MOVR",     KOC_opcode,     1,  24  },
    {   "ADDR",     KOC_opcode,     1,  25  },
    {   "SUBR",     KOC_opcode,     1,  26  },
    {   "CMPR",     KOC_opcode,     1,  27  },
    {   "ANDR",     KOC_opcode,     1,  28  },
    {   "XORR",     KOC_opcode,     1,  29  },

    {   "B",        KOC_relbr,      1,  30  },
    {   "BC",       KOC_relbr,      1,  31  },
    {   "BOV",      KOC_relbr,      1,  32  },
    {   "BPL",      KOC_relbr,      1,  33  },
    {   "BZE",      KOC_relbr,      1,  34  },
    {   "BEQ",      KOC_relbr,      1,  34  },
    {   "BLT",      KOC_relbr,      1,  35  },
    {   "BNGE",     KOC_relbr,      1,  35  },
    {   "BLE",      KOC_relbr,      1,  36  },
    {   "BNGT",     KOC_relbr,      1,  36  },
    {   "BUSC",     KOC_relbr,      1,  37  },

    {   "NOPP",     KOC_opcode,     2,  92  },
    {   "BNC",      KOC_relbr,      1,  39  },
    {   "BNOV",     KOC_relbr,      1,  40  },
    {   "BMI",      KOC_relbr,      1,  41  },
    {   "BNZE",     KOC_relbr,      1,  42  },
    {   "BNZ",      KOC_relbr,      1,  42  },
    {   "BNEQ",     KOC_relbr,      1,  42  },
    {   "BNE",      KOC_relbr,      1,  42  },
    {   "BGE",      KOC_relbr,      1,  43  },
    {   "BNLT",     KOC_relbr,      1,  43  },
    {   "BGT",      KOC_relbr,      1,  44  },
    {   "BNLE",     KOC_relbr,      1,  44  },
    {   "BESC",     KOC_relbr,      1,  45  },

    {   "BEXT",     KOC_relbr_x,    1,  96  },

    {   "SWAP",     KOC_opcode,     2,  46  },
    {   "SLL",      KOC_opcode,     2,  48  },
    {   "RLC",      KOC_opcode,     2,  50  },
    {   "SLLC",     KOC_opcode,     2,  52  },
    {   "SLR",      KOC_opcode,     2,  54  },
    {   "SAR",      KOC_opcode,     2,  56  },
    {   "RRC",      KOC_opcode,     2,  58  },
    {   "SARC",     KOC_opcode,     2,  60  },

    {   "NOP",      KOC_opcode,     1,  62  },
    {   "NOP2",     KOC_opcode,     1,  94  },
    {   "SIN",      KOC_opcode,     1,  63  },
    {   "SIN2",     KOC_opcode,     1,  95  },

    {   "J",        KOC_opcode,     1,  64  },
    {   "JE",       KOC_opcode,     1,  65  },
    {   "JD",       KOC_opcode,     1,  66  },
    {   "JSR",      KOC_opcode,     1,  67  },
    {   "JSRE",     KOC_opcode,     1,  68  },
    {   "JSRD",     KOC_opcode,     1,  69  },

    {   "INCR",     KOC_opcode,     1,  70  },
    {   "DECR",     KOC_opcode,     1,  71  },
    {   "COMR",     KOC_opcode,     1,  72  },
    {   "NEGR",     KOC_opcode,     1,  73  },
    {   "ADCR",     KOC_opcode,     1,  74  },
    {   "GSWD",     KOC_opcode,     1,  75  },
    {   "RSWD",     KOC_opcode,     1,  76  },

    {   "HLT",      KOC_opcode,     1,  77  },
    {   "SDBD",     KOC_SDBD,       1,  78  },
    {   "EIS",      KOC_opcode,     1,  79  },
    {   "DIS",      KOC_opcode,     1,  80  },
    {   "TCI",      KOC_opcode,     1,  81  },
    {   "CLRC",     KOC_opcode,     1,  82  },
    {   "SETC",     KOC_opcode,     1,  83  },

    {   "TSTR",     KOC_opcode,     1,  84  },  /*  MOVR  Rx, Rx    */
    {   "CLRR",     KOC_opcode,     1,  85  },  /*  XORR  Rx, Rx    */
    {   "PSHR",     KOC_opcode,     1,  86  },  /*  MVO@  Rx, SP    */
    {   "PULR",     KOC_opcode,     1,  87  },  /*  MVI@  SP, Rx    */
    {   "JR",       KOC_opcode,     1,  88  },  /*  MOVR  Rx, PC    */
    {   "CALL",     KOC_opcode,     1,  89  },  /*  JSR   R5, addr  */
    {   "BEGIN",    KOC_opcode,     1,  90  },  /*  MVO@  R5, SP    */
    {   "RETURN",   KOC_opcode,     1,  91  },  /*  MVI@  SP, PC    */

    {   "DECLE",    KOC_DDEF,       0,  0   },  /* Generates ROMW values  */
    {   "DCW",      KOC_DDEF,       0,  0   },  /* Generates ROMW values  */
    {   "BIDECLE",  KOC_WDEF,       0,  0   },  /* Generates SDBD values  */
    {   "ROMWIDTH", KOC_ROMW,       0,  0   },
    {   "ROMW",     KOC_ROMW,       0,  0   },
    {   "PROC",     KOC_PROC,       0,  0   },
    {   "ENDP",     KOC_ENDP,       0,  0   },

    {   "BYTE",     KOC_BDEF,       0,  0   },  /* Generates 8-bit values */
    {   "CHARDEF",  KOC_CHDEF,      0,  0   },
    {   "CHARSET",  KOC_CHSET,      0,  0   },
    {   "CHARUSE",  KOC_CHUSE,      0,  0   },
    {   "CHD",      KOC_CHDEF,      0,  0   },
    {   "DATA",     KOC_DDEF,       0,  0   },  /* Generates ROMW values  */
    {   "DB",       KOC_BDEF,       0,  0   },  /* Generates 8-bit values */
    {   "DW",       KOC_WDEF,       0,  0   },  /* Generates SDBD values  */
    {   "ELSE",     KOC_ELSE,       0,  0   },
    {   "END",      KOC_END,        0,  0   },
    {   "ENDI",     KOC_ENDI,       0,  0   },
    {   "EQU",      KOC_EQU,        0,  0   },
    {   "FCB",      KOC_BDEF,       0,  0   },  /* Generates 8-bit values */
    {   "FCC",      KOC_SDEF,       0,  0   },
    {   "FDB",      KOC_WDEF,       0,  0   },  /* Generates SDBD values  */
    {   "IF",       KOC_IF,         0,  0   },
    {   "INCL",     KOC_INCLUDE,    0,  0   },
    {   "INCLUDE",  KOC_INCLUDE,    0,  0   },
    {   "ORG",      KOC_ORG,        0,  0   },
    {   "RES",      KOC_RESM,       0,  0   },
    {   "RESERVE",  KOC_RESM,       0,  0   },
    {   "RMB",      KOC_RESM,       0,  0   },
    {   "SET",      KOC_SET,        0,  0   },
    {   "STRING",   KOC_SDEF,       0,  0   },
    {   "WORD",     KOC_WDEF,       0,  0   },  /* Generates SDBD values  */

    {   "STRUCT",   KOC_STRUCT,     0,  0   },  /* Opens a struct def'n */
    {   "ENDS",     KOC_ENDS,       0,  0   },  /* Closes a struct def'n */

    {   "MEMATTR",  KOC_MEMATTR,    0,  0   },  /* Set memory attributes */

    {   "RPT",      KOC_RPT,        0,  0   },  /* Repeat a block of code */
    {   "REPEAT",   KOC_RPT,        0,  0   },  /* Repeat a block of code */
    {   "ENDR",     KOC_ENDR,       0,  0   },  /* End repeated block     */

    {   "ERR",      KOC_USRERR,     0,  0   },  /* User-designated error */

    {   "STRLEN",   KEOP_STRLEN,    0,  0   },  /* Returns length of string */
    {   "ASC",      KEOP_ASC,       0,  0   },  /* ASCII val of char in str */

    {   "LISTING",  KOC_LIST,       0,  0   },  /* Assembler listing control */
    {   "QEQU",     KOC_QEQU,       0,  0   },  /* EQU and mark as "quiet" */
    {   "QSET",     KOC_QSET,       0,  0   },  /* SET and mark as "quiet" */

    {   "MACRO",    KOC_MACERR,     0,  0   },  /* We shouldn't see MACRO   */
    {   "ENDM",     KOC_MACERR,     0,  0   },  /* We shouldn't see ENDM    */

    {   "BRKIF",    KOC_BRKIF,      0,  0   },  /* Break out of RPT if true */

    {   "CMSG",     KOC_CMSG,       0,  0   },  /* Comment message in listing */
    {   "SMSG",     KOC_SMSG,       0,  0   },  /* Status message to stdout */
    {   "WMSG",     KOC_WMSG,       0,  0   },  /* Warning message */ 
    
    {   "CFGVAR",   KOC_CFGVAR,     0,  0   },  /* Configuration variable */

    {   "SRCFILE",  KOC_SRCFILE,    0,  0   },  /* HLL source file / line */
    {   "LISTCOL",  KOC_LISTCOL,    0,  0   },  /* Listing formatting */
    {   "ERR_IF_OVERWRITTEN", 
          KOC_ERR_IF_OVERWRITTEN,   0,  0   },  /* ROM overwrite control */
    {   "FORCE_OVERWRITE",     
          KOC_FORCE_OVERWRITE,      0,  0   },  /* ROM overwrite control */
    {   "",         0,              0,  0   }
};


/* ======================================================================== */
/*  OSTAB    -- Opcode Syntax TABle.  Syntax definition sets for instrs.    */
/*                                                                          */
/*  Legend:                                                                 */
/*      REG      Register.                                                  */
/*      EXP      EXPression                                                 */
/*      CEX      Constant EXpression (eg. exp. prefixed w/ #).              */
/*      IMP      Implied operand.                                           */
/* ======================================================================== */
struct opsynt ostab[] = 
{
    /*  invalid 0   */  {   0,          1,  0   },
    /*  invalid 1   */  {   0xFFFF,     1,  1   },

    /*  MVO     2   */  {   ST_REGEXP,  1,  2   },
    /*  MVI     3   */  {   ST_EXPREG,  1,  3   },
    /*  ADD     4   */  {   ST_EXPREG,  1,  4   },
    /*  SUB     5   */  {   ST_EXPREG,  1,  5   },
    /*  CMP     6   */  {   ST_EXPREG,  1,  6   },
    /*  AND     7   */  {   ST_EXPREG,  1,  7   },
    /*  XOR     8   */  {   ST_EXPREG,  1,  8   },

    /*  MVO@    9   */  {   ST_REGREG,  1,  9   },
    /*  MVI@    10  */  {   ST_REGREG,  1,  10  },
    /*  ADD@    11  */  {   ST_REGREG,  1,  11  },
    /*  SUB@    12  */  {   ST_REGREG,  1,  12  },
    /*  CMP@    13  */  {   ST_REGREG,  1,  13  },
    /*  AND@    14  */  {   ST_REGREG,  1,  14  },
    /*  XOR@    15  */  {   ST_REGREG,  1,  15  },

    /*  MVOI    16  */  {   ST_REGCEX,  1,  16  },
    /*  MVII    17  */  {   ST_CEXREG,  2,  17  },
    /*  ADDI    18  */  {   ST_CEXREG,  2,  19  },
    /*  SUBI    19  */  {   ST_CEXREG,  2,  21  },
    /*  CMPI    20  */  {   ST_CEXREG,  2,  23  },
    /*  ANDI    21  */  {   ST_CEXREG,  2,  25  },
    /*  XORI    22  */  {   ST_CEXREG,  2,  27  },

    /*  unused  23  */  {   0,          1,  0   },  /* oops */
    /*  MOVR    24  */  {   ST_REGREG,  1,  29  },
    /*  ADDR    25  */  {   ST_REGREG,  1,  30  },
    /*  SUBR    26  */  {   ST_REGREG,  1,  31  },
    /*  CMPR    27  */  {   ST_REGREG,  1,  32  },
    /*  ANDR    28  */  {   ST_REGREG,  1,  33  },
    /*  XORR    29  */  {   ST_REGREG,  1,  34  },

    /*  B       30  */  {   ST_EXP,     1,  35  },
    /*  BC      31  */  {   ST_EXP,     1,  36  },
    /*  BOV     32  */  {   ST_EXP,     1,  37  },
    /*  BPL     33  */  {   ST_EXP,     1,  38  },
    /*  BEQ     34  */  {   ST_EXP,     1,  39  },
    /*  BLT     35  */  {   ST_EXP,     1,  40  },
    /*  BLE     36  */  {   ST_EXP,     1,  41  },
    /*  BUSC    37  */  {   ST_EXP,     1,  42  },

    /*  unused  38  */  {   0,          1,  0   },  /* oops */
    /*  BNC     39  */  {   ST_EXP,     1,  44  },
    /*  BNOV    40  */  {   ST_EXP,     1,  45  },
    /*  BMI     41  */  {   ST_EXP,     1,  46  },
    /*  BNEQ    42  */  {   ST_EXP,     1,  47  },
    /*  BGE     43  */  {   ST_EXP,     1,  48  },
    /*  BGT     44  */  {   ST_EXP,     1,  49  },
    /*  BESC    45  */  {   ST_EXP,     1,  50  },

    /*  SWAP    46  */  {   ST_REG,     1,  51  },
    /*  SWAP    47  */  {   ST_REGEXP,  1,  52  },
    /*  SLL     48  */  {   ST_REG,     1,  53  },
    /*  SLL     49  */  {   ST_REGEXP,  1,  54  },
    /*  RLC     50  */  {   ST_REG,     1,  55  },
    /*  RLC     51  */  {   ST_REGEXP,  1,  56  },
    /*  SLLC    52  */  {   ST_REG,     1,  57  },
    /*  SLLC    53  */  {   ST_REGEXP,  1,  58  },
    /*  SLR     54  */  {   ST_REG,     1,  59  },
    /*  SLR     55  */  {   ST_REGEXP,  1,  60  },
    /*  SAR     56  */  {   ST_REG,     1,  61  },
    /*  SAR     57  */  {   ST_REGEXP,  1,  62  },
    /*  RRC     58  */  {   ST_REG,     1,  63  },
    /*  RRC     59  */  {   ST_REGEXP,  1,  64  },
    /*  SARC    60  */  {   ST_REG,     1,  65  },
    /*  SARC    61  */  {   ST_REGEXP,  1,  66  },

    /*  NOP     62  */  {   ST_IMP,     1,  67  },
    /*  SIN     63  */  {   ST_IMP,     1,  68  },

    /*  J       64  */  {   ST_EXP,     1,  69  },
    /*  JE      65  */  {   ST_EXP,     1,  70  },
    /*  JD      66  */  {   ST_EXP,     1,  71  },
    /*  JSR     67  */  {   ST_REGEXP,  1,  72  },
    /*  JSRE    68  */  {   ST_REGEXP,  1,  73  },
    /*  JSRD    69  */  {   ST_REGEXP,  1,  74  },

    /*  INCR    70  */  {   ST_REG,     1,  75  },
    /*  DECR    71  */  {   ST_REG,     1,  76  },
    /*  COMR    72  */  {   ST_REG,     1,  77  },
    /*  NEGR    73  */  {   ST_REG,     1,  78  },
    /*  ADCR    74  */  {   ST_REG,     1,  79  },
    /*  GSWD    75  */  {   ST_REG,     1,  80  },
    /*  RSWD    76  */  {   ST_REG,     1,  81  },

    /*  HLT     77  */  {   ST_IMP,     1,  82  },
    /*  SDBD    78  */  {   ST_IMP,     1,  83  },
    /*  EIS     79  */  {   ST_IMP,     1,  84  },
    /*  DIS     80  */  {   ST_IMP,     1,  85  },
    /*  TCI     81  */  {   ST_IMP,     1,  86  },
    /*  CLRC    82  */  {   ST_IMP,     1,  87  },
    /*  SETC    83  */  {   ST_IMP,     1,  88  },

    /*  TSTR    84  */  {   ST_REG,     1,  89  },
    /*  CLRR    85  */  {   ST_REG,     1,  90  },
    /*  PSHR    86  */  {   ST_REG,     1,  91  },
    /*  PULR    87  */  {   ST_REG,     1,  92  },
    /*  JR      88  */  {   ST_REG,     1,  93  },
    /*  CALL    89  */  {   ST_EXP,     1,  94  },
    /*  BEGIN   90  */  {   ST_IMP,     1,  95  },
    /*  RETURN  91  */  {   ST_IMP,     1,  96  },

    /*  NOPP    92  */  {   ST_EXP,     1,  43  },
    /*  NOPP    93  */  {   ST_IMP,     1,  97  },

    /*  NOP2    94  */  {   ST_IMP,     1,  98  },
    /*  SIN2    95  */  {   ST_IMP,     1,  99  },

    /*  BEXT    95  */  {   ST_EXPEXP,  1,  100 },

    /*  end         */  {   0,          0,  0   }
};


/* ======================================================================== */
/*  Helper macros.                                                          */
/*  MVO_OK  Tests arg 2 to make sure it's R1 .. R6.                         */
/*  SH_OK   Tests if shift amount is ok.                                    */
/*  CST_OK  Tests if constant is ok (within field width).                   */
/*  DBD     Generate double-byte-data.                                      */
/*  RR      Register/Register generator. Reused for Direct, Immediate.      */
/*  BR      Branch Relative generator.                                      */
/*  SH      Shift generator.                                                */
/*  SR      Single-register generator                                       */
/*  JSR     Jump/JSR generator                                              */
/*  CST     Constant arg generator (eg. immediate argument)                 */
/* ======================================================================== */
#define MVO_OK      "[2#].0=.[2#].7=+T$"
#define SH_OK(n)    #n ".1>T$" #n ".<0T$"
#define CST_OK(w,c) #c "." #w"I$"
#define DBD(x)      #x ".FF&x" #x ".8}.FF&x"
#define RR(o,x,y)   #o "." #x ".3{|." #y "|x"
#define BRDIR(a)    "P.2+." #a ">."
#define BROFS(a)    #a ".P.2+-." BRDIR(a) "!_^"
#define BR(c,a,w)   "0200." #c "|." BRDIR(a) "5{|x" BROFS(a) "~x" #w "I$"
#define BX(c,a,w)   #c ".4I$" \
                    "0210." #c "|." BRDIR(a) "5{|x" BROFS(a) "~x" #w "I$"
/*#define BR(c,a,m)   "0200." #c "|." BRDIR(a) "5{|x" #a "x"*/
#define CST(c,m)    CST_OK(m,c) #c "x"
#define SH(o,n,r)   SH_OK(n) "0040." #o ".3{|." #n ".1&.2{|." #r "|x"
#define SR(o,r)     "0000." #o ".3{|." #r "|x"
#define JSR(r,e,a)  "0004x" #r ".3&.8{." #a ".8}.FC&|." #e "|x" #a ".3FF&x"


/* ======================================================================== */
/*  IGTAB    -- Instruction Generator Table.                                */
/* ======================================================================== */
struct igel igtab[] = 
{
    /* inv  0   */  {   SDBD,       0,      "[Xnullentry"                   },
    /* inv  1   */  {   SDBD,       0,      "[Xinvalid opcode"              },

    /* MVO  2   */  {   SDBD,       0,      RR(0240,0,[1#]) CST([2=],[3#])  },
    /* MVI  3   */  {   SDBD,       0,      RR(0280,0,[2#]) CST([1=],[3#])  },
    /* ADD  4   */  {   SDBD,       0,      RR(02C0,0,[2#]) CST([1=],[3#])  },
    /* SUB  5   */  {   SDBD,       0,      RR(0300,0,[2#]) CST([1=],[3#])  },
    /* CMP  6   */  {   SDBD,       0,      RR(0340,0,[2#]) CST([1=],[3#])  },
    /* AND  7   */  {   SDBD,       0,      RR(0380,0,[2#]) CST([1=],[3#])  },
    /* XOR  8   */  {   SDBD,       0,      RR(03C0,0,[2#]) CST([1=],[3#])  },
    
    /* MVO@ 9   */  {   SDBD,       0,      MVO_OK
                                            RR(0240,[2#],[1#])              },
    /* MVI@ 10  */  {   IND_RG,     IND_RG, RR(0280,[1#],[2#])              },
    /* ADD@ 11  */  {   IND_RG,     IND_RG, RR(02C0,[1#],[2#])              },
    /* SUB@ 12  */  {   IND_RG,     IND_RG, RR(0300,[1#],[2#])              },
    /* CMP@ 13  */  {   IND_RG,     IND_RG, RR(0340,[1#],[2#])              },
    /* AND@ 14  */  {   IND_RG,     IND_RG, RR(0380,[1#],[2#])              },
    /* XOR@ 15  */  {   IND_RG,     IND_RG, RR(03C0,[1#],[2#])              },

    /* MVOI 16  */  {   SDBD,       0,      RR(0240,7,[1#]) CST([2=],[3#])  },
    /* MVII 17  */  {   SDBD,       0,      RR(0280,7,[2#]) CST([1=],[3#])  },
    /* MVII 18  */  {   SDBD,       SDBD,   RR(0280,7,[2#]) DBD([1=])       },
    /* ADDI 19  */  {   SDBD,       0,      RR(02C0,7,[2#]) CST([1=],[3#])  },
    /* ADDI 20  */  {   SDBD,       SDBD,   RR(02C0,7,[2#]) DBD([1=])       },
    /* SUBI 21  */  {   SDBD,       0,      RR(0300,7,[2#]) CST([1=],[3#])  },
    /* SUBI 22  */  {   SDBD,       SDBD,   RR(0300,7,[2#]) DBD([1=])       },
    /* CMPI 23  */  {   SDBD,       0,      RR(0340,7,[2#]) CST([1=],[3#])  },
    /* CMPI 24  */  {   SDBD,       SDBD,   RR(0340,7,[2#]) DBD([1=])       },
    /* ANDI 25  */  {   SDBD,       0,      RR(0380,7,[2#]) CST([1=],[3#])  },
    /* ANDI 26  */  {   SDBD,       SDBD,   RR(0380,7,[2#]) DBD([1=])       },
    /* XORI 27  */  {   SDBD,       0,      RR(03C0,7,[2#]) CST([1=],[3#])  },
    /* XORI 28  */  {   SDBD,       SDBD,   RR(03C0,7,[2#]) DBD([1=])       },

    /* MOVR 29  */  {   SDBD,       0,      RR(0080,[1#],[2#])              },
    /* ADDR 30  */  {   SDBD,       0,      RR(00C0,[1#],[2#])              },
    /* SUBR 31  */  {   SDBD,       0,      RR(0100,[1#],[2#])              },
    /* CMPR 32  */  {   SDBD,       0,      RR(0140,[1#],[2#])              },
    /* ANDR 33  */  {   SDBD,       0,      RR(0180,[1#],[2#])              },
    /* XORR 34  */  {   SDBD,       0,      RR(01C0,[1#],[2#])              },

    /* B    35  */  {   SDBD,       0,      BR(0,[1=],[3#])                 },
    /* BC   36  */  {   SDBD,       0,      BR(1,[1=],[3#])                 },
    /* BOV  37  */  {   SDBD,       0,      BR(2,[1=],[3#])                 },
    /* BPL  38  */  {   SDBD,       0,      BR(3,[1=],[3#])                 },
    /* BEQ  39  */  {   SDBD,       0,      BR(4,[1=],[3#])                 },
    /* BLT  40  */  {   SDBD,       0,      BR(5,[1=],[3#])                 },
    /* BLE  41  */  {   SDBD,       0,      BR(6,[1=],[3#])                 },
    /* BUSC 42  */  {   SDBD,       0,      BR(7,[1=],[3#])                 },
    /* NOPP 43  */  {   SDBD,       0,      BR(8,[1=],[3#])                 },
    /* BNC  44  */  {   SDBD,       0,      BR(9,[1=],[3#])                 },
    /* BNOV 45  */  {   SDBD,       0,      BR(A,[1=],[3#])                 },
    /* BMI  46  */  {   SDBD,       0,      BR(B,[1=],[3#])                 },
    /* BNEQ 47  */  {   SDBD,       0,      BR(C,[1=],[3#])                 },
    /* BGE  48  */  {   SDBD,       0,      BR(D,[1=],[3#])                 },
    /* BGT  49  */  {   SDBD,       0,      BR(E,[1=],[3#])                 },
    /* BESC 50  */  {   SDBD,       0,      BR(F,[1=],[3#])                 },

    /* SWAP 51  */  {   SDBD|SHF_RG,SHF_RG, SH(0,0,[1#])                    },
    /* SWAP 52  */  {   SDBD|SHF_RG,SHF_RG, SH(0,[2=].1-,[1#])              },
    /* SLL  53  */  {   SDBD|SHF_RG,SHF_RG, SH(1,0,[1#])                    },
    /* SLL  54  */  {   SDBD|SHF_RG,SHF_RG, SH(1,[2=].1-,[1#])              },
    /* RLC  55  */  {   SDBD|SHF_RG,SHF_RG, SH(2,0,[1#])                    },
    /* RLC  56  */  {   SDBD|SHF_RG,SHF_RG, SH(2,[2=].1-,[1#])              },
    /* SLLC 57  */  {   SDBD|SHF_RG,SHF_RG, SH(3,0,[1#])                    },
    /* SLLC 58  */  {   SDBD|SHF_RG,SHF_RG, SH(3,[2=].1-,[1#])              },
    /* SLR  59  */  {   SDBD|SHF_RG,SHF_RG, SH(4,0,[1#])                    },
    /* SLR  60  */  {   SDBD|SHF_RG,SHF_RG, SH(4,[2=].1-,[1#])              },
    /* SAR  61  */  {   SDBD|SHF_RG,SHF_RG, SH(5,0,[1#])                    },
    /* SAR  62  */  {   SDBD|SHF_RG,SHF_RG, SH(5,[2=].1-,[1#])              },
    /* RRC  63  */  {   SDBD|SHF_RG,SHF_RG, SH(6,0,[1#])                    },
    /* RRC  64  */  {   SDBD|SHF_RG,SHF_RG, SH(6,[2=].1-,[1#])              },
    /* SARC 65  */  {   SDBD|SHF_RG,SHF_RG, SH(7,0,[1#])                    },
    /* SARC 66  */  {   SDBD|SHF_RG,SHF_RG, SH(7,[2=].1-,[1#])              },

    /* NOP  67  */  {   SDBD,       0,      "0034x"                         },
    /* SIN  68  */  {   SDBD,       0,      "0036x"                         },

    /* J    69  */  {   SDBD,       0,      JSR(3,0,[1=])                   },
    /* JE   70  */  {   SDBD,       0,      JSR(3,1,[1=])                   },
    /* JD   71  */  {   SDBD,       0,      JSR(3,2,[1=])                   },
    /* JSR  72  */  {   SDBD|JSR_RG,JSR_RG, JSR([1#],0,[2=])                },
    /* JSRE 73  */  {   SDBD|JSR_RG,JSR_RG, JSR([1#],1,[2=])                },
    /* JSRD 74  */  {   SDBD|JSR_RG,JSR_RG, JSR([1#],2,[2=])                },

    /* INCR 75  */  {   SDBD,       0,      SR(1,[1#])                      },
    /* DECR 76  */  {   SDBD,       0,      SR(2,[1#])                      },
    /* COMR 77  */  {   SDBD,       0,      SR(3,[1#])                      },
    /* NEGR 78  */  {   SDBD,       0,      SR(4,[1#])                      },
    /* ADCR 79  */  {   SDBD,       0,      SR(5,[1#])                      },
    /* GSWD 80  */  {   SDBD|SHF_RG,SHF_RG, SR(6,[1#])                      },
    /* RSWD 81  */  {   SDBD,       0,      SR(7,[1#])                      },

    /* HLT  82  */  {   SDBD,       0,      "0000x"                         },
    /* SDBD 83  */  {   SDBD,       0,      "0001x"                         },
    /* EIS  84  */  {   SDBD,       0,      "0002x"                         },
    /* DIS  85  */  {   SDBD,       0,      "0003x"                         },
    /* TCI  86  */  {   SDBD,       0,      "0005x"                         },
    /* CLRC 87  */  {   SDBD,       0,      "0006x"                         },
    /* SETC 88  */  {   SDBD,       0,      "0007x"                         },

    /* TSTR 89  */  {   SDBD,       0,      RR(0080,[1#],[1#])              },
    /* CLRR 90  */  {   SDBD,       0,      RR(01C0,[1#],[1#])              },
    /* PSHR 91  */  {   SDBD,       0,      RR(0240,6,[1#])                 },
    /* PULR 92  */  {   0,          0,      RR(0280,6,[1#])                 },
    /* JR   93  */  {   SDBD,       0,      RR(0080,[1#],7)                 },
    /* CALL 94  */  {   SDBD,       0,      JSR(5,0,[1=])                   },
    /* BEGIN 95 */  {   SDBD,       0,      RR(0240,6,5)                    },
    /* RETURN 96*/  {   SDBD,       0,      RR(0280,6,7)                    },

    /* NOPP 97  */  {   SDBD,       0,      "0208x0000x"                    },
    /* NOP2 98  */  {   SDBD,       0,      "0035x"                         },
    /* SIN2 99  */  {   SDBD,       0,      "0037x"                         },

    /* BESC 100 */  {   SDBD,       0,      BX([4#],[1=],[3#])              },

    /* end      */  {   0,          0,      "[Xinvalid opcode"              },
};

#define NUMOPCODE (sizeof(optab)/sizeof(struct opsym))

int gnumopcode = NUMOPCODE;
int ophashlnk[NUMOPCODE];

/* ======================================================================== */
/*  End of file:  fraptabdef.c                                              */
/* ======================================================================== */
