%{
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


%}
%union {
    int             intv;
    int             longv;
    char            *strng;
    struct symel    *symb;
    struct slidx    slidx;
    intvec_t        *intvec;
}

%token <intv> CP1600_REG 
%token <intv> KOC_BDEF
%token <intv> KOC_ELSE
%token <intv> KOC_END
%token <intv> KOC_ENDI
%token <intv> KOC_EQU
%token <intv> KOC_IF
%token <intv> KOC_INCLUDE
%token <intv> KOC_ORG
%token <intv> KOC_RESM
%token <intv> KOC_SDEF
%token <intv> KOC_SET
%token <intv> KOC_WDEF
%token <intv> KOC_CHSET
%token <intv> KOC_CHDEF
%token <intv> KOC_CHUSE
%token <intv> KOC_opcode
%token <intv> KOC_opcode_i
%token <intv> KOC_relbr
%token <intv> KOC_relbr_x
%token <intv> KOC_SDBD
%token <intv> KOC_ROMW
%token <intv> KOC_PROC
%token <intv> KOC_ENDP
%token <intv> KOC_STRUCT
%token <intv> KOC_ENDS
%token <intv> KOC_MEMATTR
%token <intv> KOC_DDEF
%token <intv> KOC_RPT
%token <intv> KOC_ENDR
%token <intv> KOC_USRERR
%token <intv> KOC_LIST
%token <intv> KOC_QEQU
%token <intv> KOC_QSET
%token <intv> KOC_MACERR  /* macro error */
%token <intv> KOC_BRKIF
%token <intv> KOC_CMSG      /* Comment message inserted into listing        */
%token <intv> KOC_SMSG      /* Status message printed to stdout + listing   */
%token <intv> KOC_WMSG      /* User warning                                 */
%token <intv> KOC_CFGVAR    /* Set a .CFG variable                          */
%token <intv> KOC_SRCFILE   /* Set the current source file and line #       */
%token <intv> KOC_LISTCOL   /* Control formatting of the listing file.      */
%token <intv> KOC_ERR_IF_OVERWRITTEN
%token <intv> KOC_FORCE_OVERWRITE

%token <longv> CONSTANT
%token EOL
%token KEOP_AND
%token KEOP_DEFINED
%token KEOP_EQ
%token KEOP_GE
%token KEOP_GT
%token KEOP_HIGH
%token KEOP_LE
%token KEOP_LOW
%token KEOP_LT
%token KEOP_MOD
%token KEOP_MUN
%token KEOP_NE
%token KEOP_NOT
%token KEOP_OR
%token KEOP_SHL
%token KEOP_SHR
%token KEOP_SHRU
%token <intv> KEOP_ROTL
%token <intv> KEOP_ROTR
%token KEOP_XOR
%token KEOP_locctr
%token <longv> KEOP_TODAY_STR
%token <longv> KEOP_TODAY_VAL
%token <longv> KEOP_STRLEN
%token <longv> KEOP_ASC
%token <longv> KEOP_CLASSIFY
%token <symb>  LABEL
%token <strng> STRING
%token <longv> QCHAR
%token <symb>  SYMBOL
%token <longv> FEATURE
%token KEOP_EXPMAC   /* _EXPMAC modifier for IF/ENDI.                */

%token KTK_invalid

%nonassoc EXPRLIST
%right  KEOP_HIGH KEOP_LOW
%left   KEOP_OR KEOP_XOR
%nonassoc FEATURE SYMBOL
%left   KEOP_AND
%right  KEOP_NOT
%nonassoc   KEOP_GT KEOP_GE KEOP_LE KEOP_LT KEOP_NE KEOP_EQ
%left   '+' '-'
%left   '*' '/' KEOP_MOD KEOP_SHL KEOP_SHR KEOP_SHRU KEOP_ROTL KEOP_ROTR
%right  KEOP_MUN
%nonassoc ')'
%right  KEOP_CLASSIFY


%type <intv> expr maybe_expmac
%type <intvec> exprlist symslice
%type <symb> labelcolon symbol label
%type <slidx> labelslice labelslicecolon
%type <strng> string

%start file

%%

file    :   file allline
        |   allline
        ;

allline :   line EOL
            {
                clrexpr();
                tempstrlen = 0;
            }
        |   EOL
        |   error EOL
            {
                clrexpr();
                tempstrlen = 0;
                yyerrok;
            }
        ;

line    :   labelcolon KOC_END 
            {
                endsymbol = $1;
                nextreadact = Nra_end;
            }
        |   KOC_USRERR string
            {
                usr_message(USRERR, $2);
            }
        |   KOC_WMSG string
            {
                usr_message(USRWARN, $2);
            }
        |   KOC_SMSG string
            {
                usr_message(USRSTAT, $2);
            }
        |   KOC_CMSG string
            {
                usr_message(USRCMT, $2);
            }
        |   KOC_MACERR
            {
                fraerror("Unexpected MACRO or ENDM directive");
            }
        |   KOC_END 
            {
                nextreadact = Nra_end;
            }
        |   KOC_INCLUDE STRING
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
                    infilestk[nextfstk].fnm  = memoize_string($2);
                    if( (infilestk[nextfstk].fpt = 
                        path_fopen(as1600_search_path,$2,"r")) ==(LZFILE*)NULL)
                    {
                        static char *incl_file = NULL;
                        static int   incl_file_size = 0;
                        int          incl_file_len  = strlen($2) + 80;
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
                                               "\"%s\"", $2);
                            
                            fraerror(incl_file);
                        }
                    }
                    else
                    {
                        nextreadact = Nra_new;
                    }
                }
            }
        |   labelcolon KOC_EQU exprlist
            {
                do_set_equ_list(FALSE, TRUE, 0, $1, $3, 0, $3->len - 1,
                               "noncomputable expression for EQU",
                               "cannot change symbol value with EQU");
                intvec_delete($3);
            }
        |   labelcolon KOC_QEQU exprlist
            {
                do_set_equ_list(FALSE, TRUE, SFLAG_QUIET, $1, $3, 0,
                                $3->len - 1,
                               "noncomputable expression for QEQU",
                               "cannot change symbol value with QEQU");
                intvec_delete($3);
            }
        |   labelcolon KOC_SET exprlist
            {
                do_set_equ_list(FALSE, FALSE, 0, $1, $3, 0, 
                               $3->len - 1,
                               "noncomputable expression for SET",
                               "cannot change symbol value with SET");
                intvec_delete($3);
            }
        |   labelcolon KOC_QSET exprlist
            {
                do_set_equ_list(FALSE, FALSE, SFLAG_QUIET, $1, $3, 0,
                                $3->len - 1,
                               "noncomputable expression for QSET",
                               "cannot change symbol value with QSET");
                intvec_delete($3);
            }
        |   labelslicecolon KOC_EQU exprlist
            {
                do_set_equ_list(TRUE, TRUE, 0, 
                                $1.sym, $3, $1.first, $1.last,
                               "noncomputable expression for EQU",
                               "cannot change symbol value with EQU");
                intvec_delete($3);
            }
        |   labelslicecolon KOC_QEQU exprlist
            {
                do_set_equ_list(TRUE, TRUE, SFLAG_QUIET, 
                                $1.sym, $3, $1.first, $1.last,
                               "noncomputable expression for QEQU",
                               "cannot change symbol value with QEQU");
                intvec_delete($3);
            }
        |   labelslicecolon KOC_SET exprlist
            {
                do_set_equ_list(TRUE, FALSE, 0, 
                                $1.sym, $3, $1.first, $1.last,
                               "noncomputable expression for SET",
                               "cannot change symbol value with SET");
                intvec_delete($3);
            }
        |   labelslicecolon KOC_QSET exprlist
            {
                do_set_equ_list(TRUE, FALSE, SFLAG_QUIET, 
                                $1.sym, $3, $1.first, $1.last,
                               "noncomputable expression for QSET",
                               "cannot change symbol value with QSET");
                intvec_delete($3);
            }
        |   KOC_RPT expr
            {
                pevalexpr(0, $2);
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
        |   KOC_ENDR    
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
        |   KOC_BRKIF expr
            {
                if (frarptcnt < 0)
                    fraerror("BRKIF without REPEAT");
                
                pevalexpr(0, $2);
                if (evalr[0].seg == SSG_ABS)
                {
                    if (evalr[0].value != 0)
                        frarptbreak();  /* skip rest of repeat block */
                } else
                {
                    fraerror("Computable expression required for BRKIF");
                }
            }
        |   KOC_LIST STRING
            {
                if      (stricmp($2, "ON"  )==0) emit_listing_mode(LIST_ON);
                else if (stricmp($2, "OFF" )==0) emit_listing_mode(LIST_OFF);
                else if (stricmp($2, "CODE")==0) emit_listing_mode(LIST_CODE);
                else if (stricmp($2, "PREV")==0) emit_listing_mode(LIST_PREV);
                else 
                {
                    fraerror("LISTING must be followed by \"ON\", \"OFF\" "
                             "or \"CODE\"");
                }
            }
        |   labelcolon KOC_LIST STRING
            {
                if      (stricmp($3, "ON"  )==0) emit_listing_mode(LIST_ON);
                else if (stricmp($3, "OFF" )==0) emit_listing_mode(LIST_OFF);
                else if (stricmp($3, "CODE")==0) emit_listing_mode(LIST_CODE);
                else if (stricmp($3, "PREV")==0) emit_listing_mode(LIST_PREV);
                else 
                {
                    fraerror("LISTING must be followed by \"ON\", \"OFF\" "
                             "or \"CODE\"");
                }

                if($1->seg == SSG_UNDEF)
                {
                    $1->seg   = SSG_ABS;
                    $1->value = labelloc;
                }
                else
                    fraerror( "multiple definition of label");
            }
        |   KOC_IF maybe_expmac expr
            {
                if((++ifstkpt) < IFSTKDEPTH)
                {
                    pevalexpr(0, $3);
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
                    fraexpmac = $2;
                }
                else
                {
                    fraerror("IF stack overflow");
                }
            }
                        
        |   KOC_IF maybe_expmac
            {
                if(fraifskip) 
                {
                    if((++ifstkpt) < IFSTKDEPTH)
                    {
                            elseifstk[ifstkpt] = If_Skip;
                            endifstk[ifstkpt] = If_Skip;
                            expmacstk[ifstkpt] = fraexpmac;
                            fraexpmac = $2;
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
                        
        |   KOC_ELSE 
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

        |   KOC_ENDI 
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
        |   labelcolon KOC_ORG expr 
            {
                pevalexpr(0, $3);
                if(evalr[0].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = -1;
                    currmode = memoize_string("+R");
                    if($1->seg == SSG_UNDEF)
                    {
                        $1->seg   = SSG_ABS;
                        $1->value = labelloc;
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
        |   KOC_ORG expr 
            {
                pevalexpr(0, $2);
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
        |   labelcolon KOC_ORG expr ',' expr
            {
                pevalexpr(0, $3);
                pevalexpr(1, $5);
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = chkover(evalr[1].value, 0) - labelloc;
                    currpag  = -1;
                    currmode = memoize_string(currseg ? "" : "+R");
                    if($1->seg == SSG_UNDEF)
                    {
                        $1->seg   = SSG_ABS;
                        $1->value = labelloc;
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
        |   KOC_ORG expr ',' expr
            {
                pevalexpr(0, $2);
                pevalexpr(1, $4);
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
        |   labelcolon KOC_ORG expr ',' expr ',' STRING
            {
                pevalexpr(0, $3);
                pevalexpr(1, $5);
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    char *s = $7;

                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = chkover(evalr[1].value, 0) - labelloc;
                    currpag  = -1;
                    currmode = memoize_string(s);

                    if($1->seg == SSG_UNDEF)
                    {
                        $1->seg = SSG_ABS;
                        $1->value = labelloc;
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
        |   KOC_ORG expr ',' expr ',' STRING
            {
                pevalexpr(0, $2);
                pevalexpr(1, $4);
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    char *s = $6;

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
        |   labelcolon KOC_ORG expr ':' expr
            {
                pevalexpr(0, $3);
                pevalexpr(1, $5);
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = evalr[1].value;
                    currmode = memoize_string("=R");
                    if($1->seg == SSG_UNDEF)
                    {
                        $1->seg   = SSG_ABS;
                        $1->value = labelloc;
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
        |   KOC_ORG expr ':' expr
            {
                pevalexpr(0, $2);
                pevalexpr(1, $4);
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
        |   labelcolon KOC_ORG expr ':' expr ',' STRING
            {
                pevalexpr(0, $3);
                pevalexpr(1, $5);
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = evalr[1].value;
                    currmode = memoize_string($7);
                    if($1->seg == SSG_UNDEF)
                    {
                        $1->seg   = SSG_ABS;
                        $1->value = labelloc;
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
        |   KOC_ORG expr ':' expr ',' STRING
            {
                pevalexpr(0, $2);
                pevalexpr(1, $4);
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    locctr   = 2 * chkover(labelloc = evalr[0].value, 0);
                    currseg  = 0;
                    currpag  = evalr[1].value;
                    currmode = memoize_string($6);
                    emit_set_equ(evalr[0].value);
                }
                else
                {
                    fraerror("noncomputable expression for ORG");
                }
            }
        |   KOC_MEMATTR expr ',' expr ',' STRING
            {
                pevalexpr(0, $2);
                pevalexpr(1, $4);
                if(evalr[0].seg == SSG_ABS && evalr[1].seg == SSG_ABS)
                {
                    const char *s = memoize_string($6);
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
        |   KOC_CFGVAR string KEOP_EQ string
            {
                const char *var   = memoize_string($2);
                const char *value = memoize_string($4);
                emit_cfgvar_str(var, value);
            }
        |   KOC_CFGVAR string KEOP_EQ expr
            {
                pevalexpr(0, $4);
                if(evalr[0].seg == SSG_ABS)
                {
                    const char *var = memoize_string($2);
                    emit_cfgvar_int(var, evalr[0].value);
                }
                else
                {
                    fraerror("noncomputable expression for CFGVAR");
                }
            }
        |   KOC_SRCFILE string ',' expr
            {
                /* set the current source file override and line number */
                pevalexpr(0, $4);
                if ( evalr[0].seg == SSG_ABS )
                {
                    if ( strlen( $2 ) == 0 || evalr[0].value < 1 )
                    {
                        emit_srcfile_override( NULL, 0 ); 
                    } else
                    {
                        emit_srcfile_override( memoize_string( $2 ), 
                                               evalr[0].value );
                    }
                }
                else
                {
                    fraerror("noncomputable expression for SRCFILE");
                }
            }
        |   KOC_LISTCOL expr ',' expr ',' expr
            {
                pevalexpr(0, $2);   /* Hex per line w/ source */
                pevalexpr(1, $4);   /* Hex per line w/out source */
                pevalexpr(2, $6);   /* Starting column of source */
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
        |   KOC_ERR_IF_OVERWRITTEN expr
            {
                pevalexpr(0, $2);
                if ( evalr[0].seg != SSG_ABS )
                {
                    fraerror("noncomputable expression for ERR_IF_OVERWITTEN");
                } else
                {
                    emit_err_if_overwritten(evalr[0].value != 0);
                }
            }
        |   KOC_FORCE_OVERWRITE expr
            {
                pevalexpr(0, $2);
                if ( evalr[0].seg != SSG_ABS )
                {
                    fraerror("noncomputable expression for FORCE_OVERWRITE");
                } else
                {
                    emit_force_overwrite(evalr[0].value != 0);
                }
            }
        |   labelcolon KOC_CHSET
            {
                if($1->seg == SSG_UNDEF)
                {
                    $1->seg = SSG_EQU;
                    if( ($1->value = chtcreate()) <= 0)
                    {
                        fraerror("cannot create character translation table");
                    }
                    emit_set_equ($1->value);
                }
                else
                {
                    fraerror("multiple definition of label");
                }
            }
        |   KOC_CHUSE
            {
                chtcpoint = (int *) NULL;
                emit_set_equ(0L);
            }
        |   KOC_CHUSE expr
            {
                pevalexpr(0, $2);
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
        |   KOC_CHDEF string ',' exprlist
            {
                chardef($2, $4);
                intvec_delete($4);
            }
        |   KOC_CHDEF QCHAR ',' exprlist
            {
                char st[2] = { $2, 0 };
                chardef(st, $4);
                intvec_delete($4);
            }
        |   labelcolon 
            {
                if($1->seg == SSG_UNDEF)
                {
                    $1->seg = SSG_ABS;
                    $1->value = chkover(labelloc, 0);
                    emit_set_equ(labelloc);

                }
                else
                    fraerror("multiple definition of label");
            }
        |   labeledline
        ;

maybe_expmac:   KEOP_EXPMAC { $$ = 1; }
            |               { $$ = 0; }
            ;

labeledline :   labelcolon genline
            {
                if (sdbd)
                    frawarn("label between SDBD and instruction");

                if($1->seg == SSG_UNDEF)
                {
                    $1->seg   = SSG_ABS;
                    $1->value = chkover(labelloc, 0);
                }
                else
                    fraerror("multiple definition of label");

                if (locctr & 1) fraerror("internal error: PC misaligned.");

                labelloc = locctr >> 1;

                sdbd    = is_sdbd;
                is_sdbd = 0;
                first   = 0;
            }
        |   genline
            {
                if (locctr & 1) fraerror("internal error: PC misaligned.");
                labelloc = locctr >> 1;

                sdbd    = is_sdbd;
                is_sdbd = 0;
                first   = 0;
            }
        ;

genline :   KOC_BDEF exprlist 
            {
                emit_location(currseg, currpag, labelloc, TYPE_DATA, currmode);
                evalr[2].seg   = SSG_ABS;
                evalr[2].value = 8;
                for( satsub = 0; satsub < $2->len; satsub++)
                {
                    pevalexpr(1, $2->data[satsub]);
                    locctr += geninstr(genbdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete($2);
            }
        |   KOC_DDEF exprlist
            {
                emit_location(currseg, currpag, labelloc, TYPE_DATA, currmode);
                evalr[2].seg   = SSG_ABS;
                evalr[2].value = romw;
                for( satsub = 0; satsub < $2->len; satsub++)
                {
                    pevalexpr(1, $2->data[satsub]);
                    locctr += geninstr(genbdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete($2);
            }

        |   KOC_SDEF exprlist 
            {
                emit_location(currseg, currpag, labelloc, TYPE_STRING, currmode);
                evalr[2].seg   = SSG_ABS;
                evalr[2].value = romw;
                for( satsub = 0; satsub < $2->len; satsub++)
                {
                    pevalexpr(1, $2->data[satsub]);
                    locctr += geninstr(genbdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete($2);
            }
        |   KOC_WDEF exprlist 
            {
                emit_location(currseg, currpag, labelloc,
                              TYPE_DBDATA|TYPE_DATA, currmode);
                for( satsub = 0; satsub < $2->len; satsub++)
                {
                    pevalexpr(1, $2->data[satsub]);
                    locctr += geninstr(genwdef);
                }
                chkover( locctr >> 1 , 1);
                intvec_delete($2);
            }   
        |   KOC_RESM expr 
            {
                pevalexpr(0, $2);
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
        ;

labelcolon: label
        |   label ':'
        ;

labelslicecolon: labelslice
             |   labelslice ':'
            ;

exprlist :  exprlist ',' expr
            {
                intvec_push($1, $3);
                $$ = $1;
            }
        |   exprlist ',' string
            {
                char *s = $3;
                int  accval = 0;

                while (*s)
                {
                    accval = chtran(&s);
                    intvec_push($1, 
                        exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL));
                }
                $$ = $1;
            }
        |   expr %prec EXPRLIST
            {
                intvec_t *const RESTRICT iv = intvec_new();
                intvec_push(iv, $1);
                $$ = iv;
            }
        |   string
            {
                intvec_t *const RESTRICT iv = intvec_new();
                char *s = $1;
                int  accval = 0;

                while (*s)
                {
                    accval = chtran(&s);
                    intvec_push(iv,
                        exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL));
                }
                $$ = iv;
            }
        |   KEOP_TODAY_VAL '(' string ')'
            {
                const struct tm *t = $1 ? &asm_time_gmt : &asm_time_local;
                $$ = unpack_time_exprs(t, &asm_time_gmt, $3);
            }
        |   exprlist ',' symslice
            {
                intvec_concat($1, $3);
                intvec_delete($3);
                $$ = $1;
            }
        |   symslice
            {
                $$ = $1;
            }
        ;

string  :   STRING
        |   '$' '(' exprlist ')'
            {   
                char *s = &tempstr[tempstrlen];

                if (chtcpoint != NULL)
                {
                    frawarn("Stringifying expression list while character "
                            "translation active");
                }

                tempstrlen += $3->len + 1;

                if (tempstrlen > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    $$ = "";
                } else
                {
                    int i;
                    $$ = s;
                    for (i = 0; i < $3->len; i++)
                    {
                        pevalexpr(0, $3->data[i]);

                        if (evalr[0].seg == SSG_ABS)
                            *s++ = evalr[0].value;
                        else
                            *s++ = '?';
                    }
                    *s = 0;
                }
                intvec_delete($3);
            }
        |   '$' '#' '(' expr ')'
            {   
                char *s = &tempstr[tempstrlen];

                if (tempstrlen + 32 > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    $$ = "";
                } else
                {
                    $$ = s;
                    pevalexpr(0, $4);

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
        |   '$' '$' '(' expr ')'
            {   
                char *s = &tempstr[tempstrlen];

                if (tempstrlen + 5 > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    $$ = "";
                } else
                {
                    $$ = s;
                    pevalexpr(0, $4);

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
        |   '$' '%' '(' expr ')'
            {   
                char *s = &tempstr[tempstrlen];

                if (tempstrlen + 5 > MAXTEMPSTR)
                {
                    fraerror("Temporary string buffer overflow");
                    $$ = "";
                } else
                {
                    $$ = s;
                    pevalexpr(0, $4);

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
        |   KEOP_TODAY_STR '(' string ')'
            {
                const struct tm *t = $1 ? &asm_time_gmt : &asm_time_local;
                char *const bufbeg = &tempstr[tempstrlen];
                const int avail = MAXTEMPSTR - tempstrlen;
                const int len = format_time_string(t, &asm_time_gmt,
                                                   $3, bufbeg, avail);
                tempstrlen += len;
                $$ = bufbeg;
            }
        ;

/* ======================================================================== */
/*  PROC/ENDP pseudo-ops                                                    */
/* ======================================================================== */
genline : labelcolon KOC_PROC
            {
                if (proc && struct_locctr != -1)
                    fraerror("PROC cannot nest inside STRUCT.");
                else if (proc && proc_stk_depth == MAX_PROC_STK)
                    fraerror("PROC nesting limit reached.");
                else if (($1->flags & SFLAG_ARRAY) != 0)
                    fraerror("array element can not be defined by PROC");
                else if ($1->seg != SSG_UNDEF)
                    fraerror("multiple definition of label");
                else
                {
                    if (proc)
                    {
                        char *old_proc     = proc;
                        int   old_proc_len = proc_len;
                        proc_stk[proc_stk_depth++] = proc;
                        proc_len = strlen(proc) + strlen($1->symstr) + 1;
                        proc     = (char *)malloc(proc_len + 1);
                        strcpy(proc, old_proc);
                        proc[old_proc_len] = '.';
                        strcpy(proc + old_proc_len + 1, $1->symstr);
                    } else
                    {
                        proc     = strdup($1->symstr);
                        proc_len = strlen(proc);
                    }

                    $1->seg   = SSG_ABS;
                    $1->value = labelloc;
                    emit_set_equ(labelloc);
                }
            }
        ;
genline : KOC_ENDP 
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
        ;
/* ======================================================================== */
/*  STRUCT/ENDS pseudo-ops                                                  */
/* ======================================================================== */
genline : labelcolon KOC_STRUCT expr
            {
                pevalexpr(0, $3);

                if (proc)
                    fraerror("STRUCT can not nest inside other STRUCTs or PROCs.");
                else if (($1->flags & SFLAG_ARRAY) != 0)
                    fraerror("array element can not be defined by STRUCT");
                else if (evalr[0].seg != SSG_ABS)
                    fraerror( "noncomputable expression for ORG");
                else if ($1->seg != SSG_UNDEF)
                    fraerror( "multiple definition of label");
                else
                {
                    proc     = strdup($1->symstr);
                    proc_len = strlen(proc);
                    struct_locctr = locctr;

                    locctr = 2 * chkover(labelloc = evalr[0].value, 0);

                    $1->seg = SSG_ABS;
                    $1->value = labelloc;

                    emit_set_equ(evalr[0].value);
                }
            }
        ;
genline : KOC_ENDS 
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
        ;
/* ======================================================================== */
/*  ROMWIDTH pseudo-op                                                      */
/* ======================================================================== */
genline : KOC_ROMW expr
            {
                emit_location(currseg, currpag, labelloc, TYPE_HOLE, currmode);
                pevalexpr(0, $2);
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
        | KOC_ROMW expr ',' expr
            {
                emit_location(currseg, currpag, labelloc, TYPE_HOLE, currmode);
                pevalexpr(0, $2);
                pevalexpr(1, $4);
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
        ;

/* ======================================================================== */
/*  SDBD instruction.                                                       */
/* ======================================================================== */
genline : KOC_SDBD
            {
                if (sdbd)
                    frawarn("Two SDBDs in a row.");

                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                locctr += geninstr(findgen($1, ST_IMP, 0));
                chkover(locctr >> 1, 1);
                is_sdbd = SDBD;
            }
        ;

/* ======================================================================== */
/*  Relative Branch instructions.                                           */
/* ======================================================================== */
genline : KOC_relbr expr
            {
                /*unsigned rel_addr = labelloc + 2;*/
                /*int dir;*/

                SDBD_CHK

                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, $2);

                evalr[3].seg   = SSG_ABS;
                evalr[3].value = romw;

                locctr += geninstr(findgen($1, ST_EXP, sdbd));
                chkover(locctr >> 1, 1);
            }
        ;
genline : KOC_relbr_x expr ',' expr
            {
                /*unsigned rel_addr = labelloc + 2;*/
                /*int dir;*/

                SDBD_CHK

                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, $2);
                pevalexpr(4, $4);

                if (evalr[4].seg != SSG_ABS)
                    fraerror("Must have constant expr for BEXT condition");

                evalr[3].seg   = SSG_ABS;
                evalr[3].value = romw;

                locctr += geninstr(findgen($1, ST_EXPEXP, sdbd));
                chkover(locctr >> 1, 1);
            }
        ;


/* ======================================================================== */
/*  Implied Operand instructions.                                           */
/* ======================================================================== */
genline : KOC_opcode 
            {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                locctr += geninstr(findgen($1, ST_IMP, sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Constant Operand instructions.                                          */
/* ======================================================================== */
genline : KOC_opcode  expr
            {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, $2);
                locctr += geninstr(findgen($1, ST_EXP, sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Register, Direct instructions (MVO)                                     */
/*  Register, 2 instructions (shifts)                                       */
/* ======================================================================== */
genline : KOC_opcode  CP1600_REG ',' expr
            {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = $2;
                pevalexpr(2, $4);
                evalr[3].seg    = SSG_ABS;
                evalr[3].value  = romw;
                locctr += geninstr(findgen($1, ST_REGEXP, reg_type[$2]|sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Register, Immediate --> MVOI                                            */
/* ======================================================================== */
genline : KOC_opcode  CP1600_REG ',' '#' expr
            {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value  = $2;
                evalr[3].seg    = SSG_ABS;
                evalr[3].value  = romw;
                pevalexpr(2, $5);
                locctr += geninstr(findgen($1, ST_REGCEX, reg_type[$2]|sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Direct, Register instructions                                           */
/* ======================================================================== */
genline : KOC_opcode  expr ',' CP1600_REG
            {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, $2);
                evalr[2].value = $4;
                evalr[3].seg   = SSG_ABS;
                evalr[3].value = romw;
                locctr += geninstr(findgen($1, ST_EXPREG, reg_type[$4]|sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Immediate, Register instructions                                        */
/* ======================================================================== */
genline : KOC_opcode  '#' expr ',' CP1600_REG
            {
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                pevalexpr(1, $3);
                evalr[2].value = $5;

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

                locctr += geninstr(findgen($1, ST_CEXREG, reg_type[$5]|sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Single-register instructions.                                           */
/* ======================================================================== */
genline : KOC_opcode  CP1600_REG
            {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = $2;
                locctr += geninstr(findgen($1, ST_REG, reg_type[$2]|sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Register, Register instructions.                                        */
/* ======================================================================== */
genline : KOC_opcode  CP1600_REG ',' CP1600_REG
            {
                SDBD_CHK
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = $2;
                evalr[2].value = $4;
                locctr += geninstr(findgen($1, ST_REGREG, reg_type[$2]|sdbd));
                chkover(locctr >> 1, 1);
            }
        ;

/* ======================================================================== */
/*  Register, Register instructions, indirect via first register.           */
/* ======================================================================== */
genline : KOC_opcode_i  CP1600_REG ',' CP1600_REG
            {
                emit_location(currseg, currpag, labelloc, TYPE_CODE, currmode);
                evalr[1].value = $2;
                evalr[2].value = $4;
                locctr += geninstr(findgen($1, ST_REGREG, reg_type[$2]|sdbd));
                chkover(locctr >> 1, 1);
            }
        ;


/* ======================================================================== */
/*  Expression definition.                                                  */
/* ======================================================================== */
expr    :   '+' expr %prec KEOP_MUN
            {
                $$ = $2;
            }
        |   '-' expr %prec KEOP_MUN
            {
                $$ = exprnode(PCCASE_UN,$2,IFC_NEG,0,0L, SYMNULL);
            }
        |   KEOP_NOT expr
            {
                $$ = exprnode(PCCASE_UN,$2,IFC_NOT,0,0L, SYMNULL);
            }
        |   KEOP_HIGH expr
            {
                $$ = exprnode(PCCASE_UN,$2,IFC_HIGH,0,0L, SYMNULL);
            }
        |   KEOP_LOW expr
            {
                $$ = exprnode(PCCASE_UN,$2,IFC_LOW,0,0L, SYMNULL);
            }
        |   expr '*' expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_MUL,$3,0L, SYMNULL);
            }
        |   expr '/' expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_DIV,$3,0L, SYMNULL);
            }
        |   expr '+' expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_ADD,$3,0L, SYMNULL);
            }
        |   expr '-' expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_SUB,$3,0L, SYMNULL);
            }
        |   expr KEOP_MOD expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_MOD,$3,0L, SYMNULL);
            }
        |   expr KEOP_SHL expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_SHL,$3,0L, SYMNULL);
            }
        |   expr KEOP_SHR expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_SHR,$3,0L, SYMNULL);
            }
        |   expr KEOP_SHRU expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_SHRU,$3,0L, SYMNULL);
            }
        |   expr KEOP_ROTL expr
            {
                const int ifc = $2 == 16 ? IFC_ROTL16 : IFC_ROTL32;
                $$ = exprnode(PCCASE_BIN,$1,ifc,$3,0L, SYMNULL);
            }
        |   expr KEOP_ROTR expr
            {
                const int ifc = $2 == 16 ? IFC_ROTL16 : IFC_ROTL32;
                const int neg = exprnode(PCCASE_UN,$3,IFC_NEG,0,0L, SYMNULL);
                $$ = exprnode(PCCASE_BIN,$1,ifc,neg,0L, SYMNULL);
            }
        |   expr KEOP_GT expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_GT,$3,0L, SYMNULL);
            }
        |   expr KEOP_GE expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_GE,$3,0L, SYMNULL);
            }
        |   expr KEOP_LT expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_LT,$3,0L, SYMNULL);
            }
        |   expr KEOP_LE expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_LE,$3,0L, SYMNULL);
            }
        |   expr KEOP_NE expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_NE,$3,0L, SYMNULL);
            }
        |   expr KEOP_EQ expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_EQ,$3,0L, SYMNULL);
            }
        |   expr KEOP_AND expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_AND,$3,0L, SYMNULL);
            }
        |   expr KEOP_OR expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_OR,$3,0L, SYMNULL);
            }
        |   expr KEOP_XOR expr
            {
                $$ = exprnode(PCCASE_BIN,$1,IFC_XOR,$3,0L, SYMNULL);
            }
        |   KEOP_DEFINED symbol
            {
                $$ = exprnode(PCCASE_DEF,0,IGP_DEFINED,0,0L,$2);
            }
        |   symbol %prec SYMBOL
            {
                $$ = exprnode(PCCASE_SYMB,0,IFC_SYMB,0,0L,$1);
            }
        |   KEOP_DEFINED FEATURE
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,$2, SYMNULL);
            }
        |   FEATURE
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,$1, SYMNULL);
            }
        |   '$'
            {
                $$ = exprnode(PCCASE_PROGC,0,IFC_PROGCTR,0,labelloc,SYMNULL);
            }
        |   CONSTANT
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,$1, SYMNULL);
            }
        |   QCHAR
            {
                char st[2] = { $1, 0 }, *s = st;
                int  accval = chtran(&s);
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL);
            }
        |   KEOP_CLASSIFY '(' ')' %prec KEOP_CLASSIFY
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_EMPTY,SYMNULL);
            }
        |   KEOP_CLASSIFY '(' CP1600_REG ')' %prec KEOP_CLASSIFY
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0, $3, SYMNULL);
            }
        |   KEOP_CLASSIFY '(' FEATURE ')' %prec KEOP_CLASSIFY
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_FEATURE,
                             SYMNULL);
            }
        |   KEOP_CLASSIFY '(' keyword ')' %prec KEOP_CLASSIFY
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_RESV,SYMNULL);
            }
        |   KEOP_CLASSIFY '(' string ')' %prec KEOP_CLASSIFY
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,CLASS_STRING,
                              SYMNULL);
            }
        |   KEOP_CLASSIFY '(' symbol ')' %prec KEOP_CLASSIFY
            {
                $$ = exprnode(PCCASE_CLASSSYM,0,IFC_CLASSIFY,0,0L,$3);
            }
        |   KEOP_CLASSIFY '(' expr ')' %prec KEOP_CLASSIFY
            {
                $$ = exprnode(PCCASE_UN,$3,IFC_CLASSIFY,0,0L, SYMNULL);
            }
        |   KEOP_STRLEN '(' string ')'
            {
                char *s = $3;
                int  accval = 0;
                int  length = 0;

                while (*s)
                {
                    accval = chtran(&s);
                    length++;
                }
                (void)accval;
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,length++,SYMNULL);
            }
        |   KEOP_STRLEN '(' QCHAR ')'
            {
                $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,1,SYMNULL);
            }
        |   KEOP_ASC '(' string ',' expr ')' 
            {
                char *s = $3;
                int  accval = 0;
                int  sindex = 0;

                pevalexpr(0, $5);
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

                    $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL);
                }
                else
                {
                    fraerror("noncomputable expression for index to ASC");
                }
            }
        |   KEOP_ASC '(' QCHAR ',' expr ')' 
            {
                char st[2] = { $3, 0 }, *s = st;
                int  accval = 0;

                pevalexpr(0, $5);
                if(evalr[0].seg == SSG_ABS)
                {
                    accval = evalr[0].value == 0 ? chtran(&s) : 0;
                    $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,accval,SYMNULL);
                }
                else
                {
                    fraerror("noncomputable expression for index to ASC");
                }
            }
        |   '(' expr ')' { $$ = $2; }
        |   '(' exprlist ')' '[' expr ']' %prec '('
            {
                pevalexpr(0, $5);
                if (evalr[0].seg == SSG_ABS)
                {
                    const int idx = evalr[0].value;
                    if (idx < $2->len)
                        $$ = $2->data[idx];
                    else
                        $$ = exprnode(PCCASE_CONS,0,IGP_CONSTANT,0,0,SYMNULL);
                }
                else
                {
                    fraerror("noncomputable expression for expr-list index");
                }
                intvec_delete($2);
            }
        ;

/* ======================================================================== */
/*  Array-indexed labels and symbols                                        */
/* ======================================================================== */
label:      LABEL
        |   label '[' expr ']'
            {   
                pevalexpr(0, $3);
                if (evalr[0].seg == SSG_ABS)
                {
                    $$ = symbentryidx($1->symstr, LABEL, 1, evalr[0].value);
                    $$->flags |= SFLAG_QUIET | SFLAG_ARRAY;
                   
                    /* track "high water mark" in LABEL's own value */
                    $1->seg    = SSG_SET;
                    $1->flags |= SFLAG_QUIET;
                    if ($1->value < evalr[0].value)
                        $1->value = evalr[0].value;
                } else
                {
                    fraerror("noncomputable expression for label array index");
                }
            }   
        ;

symbol:     SYMBOL
        |   symbol '[' expr ']'
            {   
                pevalexpr(0, $3);
                if (evalr[0].seg == SSG_ABS)
                {
                    $$ = symbentryidx($1->symstr, LABEL, 1, evalr[0].value);
                    $$->flags |= SFLAG_QUIET | SFLAG_ARRAY;
                    
                    /* track "high water mark" in LABEL's own value */
                    $1->seg    = SSG_SET;
                    $1->flags |= SFLAG_QUIET;
                    if ($1->value < evalr[0].value)
                        $1->value = evalr[0].value;
                } else
                {
                    fraerror("noncomputable expression for symbol array index");
                }
            }   
        ;


symslice:   symbol '[' expr ',' expr ']'
            {
                intvec_t *const RESTRICT iv = intvec_new();
                pevalexpr(0, $3);
                pevalexpr(1, $5);

                if (evalr[0].seg != SSG_ABS || evalr[1].seg != SSG_ABS)
                {
                    fraerror("noncomputable expression for symbol slice index");
                    $$ = iv;
                } else
                {
                    int i, s;

                    s = evalr[0].value > evalr[1].value ? -1 : 1;

                    for (i = evalr[0].value; i != evalr[1].value + s; i += s)
                    {
                        struct symel *sym;
                        intvec_push(iv,
                            exprnode(PCCASE_SYMB,0,IFC_SYMB,0,0L,
                                sym = symbentryidx($1->symstr, LABEL, 1, i)));

                        sym->flags |= SFLAG_ARRAY | SFLAG_QUIET;
                    }
                    $$ = iv;
                }
            }
        |   '(' exprlist ')' '[' expr ',' expr ']' %prec '('
            {
                pevalexpr(0, $5);
                pevalexpr(1, $7);
                if (evalr[0].seg != SSG_ABS || evalr[1].seg != SSG_ABS)
                {
                    fraerror(
                        "noncomputable expression for expr-list slice index");
                    intvec_resize($2, 0);
                    $$ = $2;
                } else if (evalr[0].value >= $2->len || 
                           evalr[1].value >= $2->len)
                {
                    fraerror("out of range index for expr-list slice");
                    intvec_resize($2, 0);
                    $$ = $2;
                } else
                {
                    const int rev = evalr[0].value > evalr[1].value;
                    const int lo = rev ? evalr[1].value : evalr[0].value;
                    const int hi = rev ? evalr[0].value : evalr[1].value;
                    const int cnt = hi - lo + 1;
                    int *const data = $2->data;

                    memmove(&data[0], &data[lo], sizeof(data[0]) * cnt);

                    if (rev)
                    {
                        int i, j;
                        for (i = 0, j = cnt - 1; i < j; i++, j--)
                        {
                            const int tmp = $2->data[i];
                            $2->data[i] = $2->data[j];
                            $2->data[j] = tmp;
                        }
                    }
                    $2->len = cnt;
                    $$ = $2;
                }
            }
        |   symslice '[' expr ']'
            {
                fraerror("array slice allowed on last index only");
                $$ = intvec_new();
            }
        |   symslice '[' expr ',' expr ']'
            {
                fraerror("array slice allowed on last index only");
                $$ = intvec_new();
            }
        ;                    


labelslice: label '[' expr ',' expr ']'
            {
                pevalexpr(0, $3);
                pevalexpr(1, $5);

                if (evalr[0].seg != SSG_ABS || evalr[1].seg != SSG_ABS)
                {
                    fraerror("noncomputable expression for label slice index");
                } else
                {
                    $$.first = evalr[0].value;
                    $$.last  = evalr[1].value;
                    $$.sym   = $1;
                }
            }
        |   labelslice '[' expr ']'
            {
                fraerror("array slice allowed on last index only");
            }
        |   labelslice '[' expr ',' expr ']'
            {
                fraerror("array slice allowed on last index only");
            }
        ;                    

/* Keyword catch-all for CLASSIFY operator */
keyword :   KEOP_AND
        |   KEOP_DEFINED
        |   KEOP_EQ
        |   KEOP_GE
        |   KEOP_GT
        |   KEOP_HIGH
        |   KEOP_LE
        |   KEOP_LOW
        |   KEOP_LT
        |   KEOP_MOD
        |   KEOP_MUN
        |   KEOP_NE
        |   KEOP_NOT
        |   KEOP_OR
        |   KEOP_SHL
        |   KEOP_SHR
        |   KEOP_SHRU
        |   KEOP_XOR
        |   KEOP_locctr
        |   KEOP_STRLEN
        |   KEOP_ASC
        |   KEOP_CLASSIFY
        ;
%%

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
