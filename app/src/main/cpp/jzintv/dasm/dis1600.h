/* ======================================================================== */
/*  DIS-1600  Advanced(?) CP-1600 Disassembler.                             */
/*  By Joseph Zbiciak                                                       */
/* ------------------------------------------------------------------------ */
/*                   Copyright (c) 2006, Joseph Zbiciak                     */
/* ======================================================================== */

#ifndef DIS1600_H_
#define DIS1600_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "config.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"
#include "icart/icartfile.h"
#include "util/symtab.h"

extern icartrom_t temp_icart, icart;
extern symtab_t   *symtab;

#define MASK_DATA   (0x0003)    /* lower two bits indicate data type        */
#define MASK_CODE   (0x00F4)    /* bits associated with valid code          */
#define FLAG_DATA   (0x0001)    /* data at this address is DECLE            */
#define FLAG_DBDATA (0x0002)    /* data at this and next addr is BIDECLE    */
#define FLAG_STRING (0x0003)    /* data at this address is STRING           */
#define FLAG_CODE   (0x0004)    /* this address is presumed to be CODE      */
#define FLAG_INVOP  (0x0008)    /* opcode at this addr is invalid.          */
#define FLAG_BRANCH (0x0010)    /* J, JSR, or B, or arith on PC             */
#define FLAG_CONDBR (0x0020)    /* This branch is conditional.              */
#define FLAG_JSR    (0x0040)    /* Further distinguish JSR from J, B        */
#define FLAG_SDBD   (0x0080)    /* Instruction is modified by SDBD          */
#define FLAG_BRTRG  (0x0100)    /* Instruction is known branch target.      */
#define FLAG_INTERP (0x0200)    /* This is data we know how to interpret    */
#define FLAG_FORCED (0x1000)    /* This instruction is forced valid.        */
#define FLAG_EMPTY  (0x80000000) /* nothing loaded at this address.         */

#define BTARG_NONE  (0)         /* no branch target for instruction         */
#define BTARG_MULT  (0x20000)   /* This instr. has multiple br targets.     */
#define BTARG_UNK   (0x40000)   /* branch target is unknown                 */

#define OPF_SRC     (0x0001)    /* This operand is a 'source' operand.      */
#define OPF_DST     (0x0002)    /* This operand is a 'destination' operand. */
#define OPF_SRCDST  (OPF_SRC|OPF_DST)
#define OPF_IND     (0x0004)    /* Value accessed thru operand indirectly   */
#define OPF_ADDR    (0x0008)    /* This operand represents an address.      */

/* ======================================================================== */
/*  INSTRUCTION MNEMONICS                                                   */
/* ======================================================================== */
typedef enum mnm_t
{
    M_err = 0,
    M_HLT,  M_SDBD, M_EIS,  M_DIS,          M_TCI,  M_CLRC, M_SETC,
    M_JSR,  M_JSRE, M_JSRD,         M_J,    M_JE,   M_JD,
    M_SWAP, M_SLL,  M_RLC,  M_SLLC, M_SLR,  M_SAR,  M_RRC,  M_SARC,
    M_B,    M_BC,   M_BOV,  M_BPL,  M_BEQ,  M_BLT,  M_BLE,  M_BUSC,
    M_NOPP, M_BNC,  M_BNOV, M_BMI,  M_BNEQ, M_BGE,  M_BGT,  M_BESC,
    M_BEXT,
            M_MVO,  M_MVI,  M_ADD,  M_SUB,  M_CMP,  M_AND,  M_XOR,
            M_MVOI, M_MVII, M_ADDI, M_SUBI, M_CMPI, M_ANDI, M_XORI,
            M_MVO_, M_MVI_, M_ADD_, M_SUB_, M_CMP_, M_AND_, M_XOR_,
                    M_MOVR, M_ADDR, M_SUBR, M_CMPR, M_ANDR, M_XORR,
            M_INCR, M_DECR, M_COMR, M_NEGR, M_ADCR,         M_RSWD,
    M_NOP,  M_SIN,  M_GSWD,

    M_PSHR, M_PULR, M_CLRR, M_TSTR,

    M_DECLE, M_BIDECLE, M_STRING,
    M_SKIP  /* pseudo-op for abused NOPP */
} mnm_t;

extern const char *mnemonic[];
extern mnm_t mnm_dir_2op[8];
extern mnm_t mnm_ind_2op[8];
extern mnm_t mnm_imm_2op[8];
extern mnm_t mnm_reg_2op[8];
extern mnm_t mnm_rot_1op[8];
extern mnm_t mnm_reg_1op[8];
extern mnm_t mnm_cond_br[16];
extern mnm_t mnm_impl_1op_a[4];
extern mnm_t mnm_impl_1op_b[4];
extern mnm_t mnm_jsr[8];

/* ======================================================================== */
/*  Note:  Some attempt is made to avoid namespace collision w/ jzIntv      */
/*  source code names, in case I re-integrate this w/ jzIntv someday.       */
/* ======================================================================== */
typedef enum oper_type
{
    OP_NONE = 0,                /* No operand.                              */
    OP_REG,                     /* CPU register.                            */
    OP_IMMED,                   /* Immediate constant.                      */
    OP_DIRADDR,                 /* Operand stored at indicated address.     */
    OP_BRTRG                    /* Branch target (special immediate).       */
} oper_type_t;

typedef struct dis_oper_t
{
    oper_type_t type;
    uint16_t    op;             /* address, reg# or value                   */
    uint16_t    flags;          /* flags associated w/ operand.             */
} dis_oper_t;

typedef enum { CMT_DFLT = 0, CMT_MED, CMT_LONG } cmt_len_t;

typedef struct dis_instr_t
{
    int          len;           /* length of this instruction               */
    mnm_t        mnemonic;      /* Mnemonic for this instruction.           */
    char        *label;         /* label applied to this instruction        */
    const char  *pre_cmt;       /* comment block preceeding instr           */
    const char  *cmt;           /* comment(s) associated w/ address.        */
    cmt_len_t    cmt_len;       /* Allow 40/32/24 spaces for instrs.        */
    char        *fmtline;       /* formatted line for this instruction.     */
    dis_oper_t   op1;           /* first operand.                           */
    dis_oper_t   op2;           /* second operand.                          */
    unsigned     flags;         /* Other information about this instruction */
    int          br_target;     /* branch target address, if known          */
    /* todo -- have a way of recording a list of branch targets if multiple */
    int         *target_of;     /* branches this instr is a target of.      */
    int          tg_of_cnt;     /* length of target_of array                */
    int          tg_of_max;     /* max length allocated to target_of        */
} dis_instr_t;


/* ======================================================================== */
/*  INSTRUCTION PRINTER TABLE (forward references)                          */
/* ======================================================================== */
typedef char *ins_prt_t(uint32_t);

extern char *prt_err(uint32_t);
extern char *prt_imp(uint32_t);
extern char *prt_jsr(uint32_t);
extern char *prt_cbr(uint32_t);
extern char *prt_bxt(uint32_t);
extern char *prt_dir(uint32_t);
extern char *prt_imm(uint32_t);
extern char *prt_dro(uint32_t);
extern char *prt_imo(uint32_t);
extern char *prt_2rg(uint32_t);
extern char *prt_1rg(uint32_t);
extern char *prt_plr(uint32_t);
extern char *prt_dcl(uint32_t);
extern char *prt_bid(uint32_t);
extern char *prt_rot(uint32_t);
extern char *prt_str(uint32_t);

extern ins_prt_t *const instr_printer[];

/* ======================================================================== */
/*  Declare storage for the ROM image.                                      */
/* ======================================================================== */
extern dis_instr_t instr[65539];

/* ======================================================================== */
/*  HELPER FUNCTIONS AND MACROS                                             */
/* ======================================================================== */


#define INSTR_ADDR(i) ((i) - instr)
#define GET_WORD(a)   (icart.image[(a) & 0xFFFF])
#define GET_DWORD(a)  (((0xFF & GET_WORD(1+(a))) << 8) | (0xFF & GET_WORD(a)))
#define NOT_CODE(a)   ((instr[(a)].flags & (FLAG_EMPTY|FLAG_INVOP|MASK_DATA)))
#define IS_EMPTY(a)   ((instr[(a)].flags & (FLAG_EMPTY)) != 0)
#define MAYBE_CODE(a) (!NOT_CODE(a))
#define BAD_ARG10(a)  ((GET_WORD(a) & 0xFC00) || IS_EMPTY(a))
#define IS_BRTRG(a)   ((instr[(a)].flags & FLAG_BRTRG ) != 0)
#define IS_BRANCH(a)  ((instr[(a)].flags & FLAG_BRANCH) != 0)
#define IS_JSR(a)     ((instr[(a)].flags & FLAG_JSR)    != 0)
#define IS_CONDBR(a)  ((instr[(a)].flags & FLAG_CONDBR) != 0)
#define IS_INTERP(a)  ((instr[(a)].flags & FLAG_INTERP) != 0)

#define IS_DATA(a)    (((instr[(a)].flags & FLAG_INVOP) != 0) |  \
                       (((instr[(a)].flags & MASK_DATA) != 0) &  \
                        ((instr[(a)].flags & FLAG_CODE) == 0)))

#define GET_BIT(bv,i) (((bv)[(i) >> 5] >> ((i) & 31)) & 1)
#define SET_BIT(bv,i) ((bv)[(i) >> 5] |= 1u << ((i) & 31))

#define VB_PRINTF(v,x)  do {                                    \
                            if (verbose > (v))                  \
                            {                                   \
                                printf x;                       \
                                fflush(stdout);                 \
                            }                                   \
                        } while(0)

/* ======================================================================== */
/* ======================================================================== */
/*  DEFAULT SYMBOL TABLE                                                    */
/* ======================================================================== */
/* ======================================================================== */
struct defsym_t
{
    const char *name;
    uint16_t addr, len, width;
};


/* ======================================================================== */
/*  MAYBE_DEFSYM     -- Helper:  Define a symbol if it's not def'd yet      */
/* ======================================================================== */
void maybe_defsym(const char *sym, uint32_t addr);

/* ======================================================================== */
/*  SETUP_DEFSYM     -- Set up the default symbol table.                    */
/* ======================================================================== */
void setup_defsym(void);

/* ======================================================================== */
/* ======================================================================== */
/*  ANALYSIS PASSES AND DECODER FUNCTIONS                                   */
/* ======================================================================== */
/* ======================================================================== */
extern int hlt_is_invalid               ;
extern int rare_ops_are_invalid         ;
extern int allow_branch_target_wrap     ;
extern int allow_branch_to_bad_addr     ;
extern int suspicious_pc_math_is_invalid;
extern int verbose                      ;
extern int skip_advanced_analysis       ;
extern int skip_mark_cart_header        ;
extern int skip_funky_branch_detect     ;
extern int skip_propagate_invalid       ;
extern int skip_kill_bad_branches       ;
extern int skip_brtrg_vs_sdbd           ;
extern int skip_find_jsr_data           ;
extern int skip_mark_args_invalid       ;
extern int skip_exec_sound_interp       ;
extern int dont_loop_analysis           ;

extern int allow_global_branches        ;
extern int no_exec_branches             ;

extern int debug_find_jsr_data          ;
extern int debug_show_instr_flags       ;
extern int no_default_symbols           ;
extern int no_exec_routine_symbols      ;
extern int forced_entry_points          ;

#define MAX_ENTRY (1024)

extern uint32_t entry_point[MAX_ENTRY];

/* ======================================================================== */
/*  MARK_INVALID -- Mark an address as invalid.                             */
/* ======================================================================== */
int mark_invalid(uint32_t addr);

/* ======================================================================== */
/*  MARK_VALID -- Mark an address as valid, w/ optional add'l flags.        */
/* ======================================================================== */
int mark_valid(uint32_t addr, uint32_t flags);

/* ======================================================================== */
/*  MARK_INTERP -- Mark address range as interpreted, w/ opt flags & cmts.  */
/* ======================================================================== */
int mark_interp(uint32_t addr, uint32_t flags, int len, const char *cmt);

/* ======================================================================== */
/*  ADD_ENTRY_POINT -- List a location as a ROM entry point.                */
/* ======================================================================== */
int add_entry_point(uint32_t addr);

/* ======================================================================== */
/*  MARK_EMPTY -- Mark an address range as not holding "local code."        */
/* ======================================================================== */
void mark_empty(int addr_lo, int addr_hi);

/* ======================================================================== */
/*  MARK_STRING -- Given a starting address, mark locations until a NUL.    */
/* ======================================================================== */
int mark_string(int addr);

/* ======================================================================== */
/*  MARK_BRANCH_TARGET -- Mark a location as a branch target.               */
/* ======================================================================== */
int mark_branch_target(int target, int target_of);

/* ======================================================================== */
/*  ADD_COMMENT  -- Add / replace comment on an instruction                 */
/* ======================================================================== */
int add_comment(uint32_t addr, const char *cmt);


#include "dasm/exec_interp.h"

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
/*                   Copyright (c) 2006, Joseph Zbiciak                     */
/* ======================================================================== */
