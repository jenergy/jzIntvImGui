/*
 * ============================================================================
 *  OP_DECODE:      CP-1610 Opcode Decoder
 *
 *  Author:         J. Zbiciak
 *
 * ============================================================================
 *  This header contains bitfield structures
 * ============================================================================
 */


#ifndef OP_DECODE_H
#define OP_DECODE_H

#include "cp1600.h"

/*
 * ============================================================================
 *  The CP-1610 is a 16-bit processor which stores its instructions in 10-bit
 *  wide quantities known as "decles".  (Rhymes w/ heckle.)  Instructions
 *  may be stored in 10-bit or 16-bit wide memory in the Intellivision.
 *
 *  The CP-1610 supports the following basic opcode formats:
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0000 000 0oo                               1    Implied 1-op insns
 *  0000 000 100  bbppppppii  pppppppppp       3    Jump insns
 *  0000 000 1oo                               1    Implied 1-op insns
 *  0000 ooo ddd                               1    1-op insns, comb src/dst
 *  0000 110 0dd                               1    GSWD
 *  0000 110 1om                               1    NOP(2), SIN(2)
 *  0001 ooo mrr                               1    Rotate/Shift insns
 *  0ooo sss ddd                               1    2-op arith, reg->reg
 *  1000 zxc ccc  pppppppppp                   2    Branch insns
 *  1001 000 sss  pppppppppp                   2    2-op MVO ,  reg->direct
 *  1001 mmm sss                               1*   2-op MVO@,  reg->ind.
 *  1001 111 sss  iiiiiiiiii                   2*   2-op MVOI,  reg->immed.
 *  1ooo 000 ddd  pppppppppp                   2    2-op arith, direct->reg
 *  1ooo mmm ddd                               1*   2-op arith, ind.  ->reg
 *  1ooo 111 ddd  iiiiiiiiii                   2*   2-op arith, immed.->reg
 *
 *
 *  Key
 *  ---
 *
 *  oo    -- Opcode field (dependent on format)
 *  sss   -- Source register,      R0 ... R7 (binary encoding)
 *  ddd   -- Destination register, R0 ... R7 (binary encoding)
 *  0dd   -- Destination register, R0 ... R3
 *  cccc  -- Condition codes (branches)
 *  x     -- External branch condition (0 == internal, 1 == examine BEXT)
 *  z     -- Branch displacement direction (1 == negative)
 *  m     -- Shift amount (0 == shift by 1, 1 == shift by 2)
 *  bb    -- Branch return register
 *  ii    -- Branch interrupt flag mode
 *
 *  Branch Condition Codes  (cccc)
 *  ----------------------
 *           n == 0                    n == 1
 *  n000  -- Always                    Never
 *  n001  -- Carry set/Greater than    Carry clear/Less than or equal
 *  n010  -- Overflow set              Overflow clear
 *  n011  -- Positive                  Negative
 *  n100  -- Equal                     Not equal
 *  n101  -- Less than                 Greater than or equal
 *  n110  -- Less than or equal        Greater than
 *  n111  -- Unequal sign and carry    Equal sign and carry
 *
 *  Branch Return Registers  (bb)
 *  -----------------------
 *
 *   00   -- R4
 *   01   -- R5
 *   10   -- R6
 *   11   -- none (do not save return address)
 *
 *  Branch Interrupt Modes   (ii)
 *  ----------------------
 *
 *   00   -- Do not change interrupt enable state
 *   01   -- Enable interrupts
 *   10   -- Disable interrupts
 *   11   -- Undefined/Reserved ?
 *
 *  SDBD notes:
 *
 *  -- SDBD is supported on "immediate" and "indirect" modes only.
 *
 *  -- An SDBD prefix on an immediate instruction sets the immediate constant
 *     to be 16 bits, stored in two adjacent 8-bit words.  The ordering is
 *     little-endian.
 *
 *  -- An SDBD prefix on an indirect instruction causes memory to be accessed
 *     twice, bringing in (or out) two 8-bit words, again in little-endian
 *     order.  If a non-incrementing data counter is used, both accesses are
 *     to the same address.  Otherwise, the counter is post-incremented with
 *     each access.  Indirect through R6 (stack addressing) is not allowed.
 *
 *  Interruptibility notes:
 *
 *  -- Interrupts always occur after an instruction completes.  No instruction
 *     is interrupted midstream and completed later.
 *
 *  -- Interrupts are taken after the completion of the next interruptible
 *     instruction.  A non-interruptible instruction masks interrupts through
 *     the transition from the current instruction to the next.
 *
 *  -- When the processor takes an interrupt, the current PC value is pushed
 *     on the stack.  The address jumped to for the interrupt is provided
 *     by external hardware (similar to an 8080, if I recall correctly).
 *
 *  Status register/GSWD/RSWD notes:
 *
 *  -- The status register has the following format:
 *
 *         3   2   1   0       S -- Sign
 *       +---+---+---+---+     Z -- Zero
 *       | S | Z | O | C |     O -- Overflow
 *       +---+---+---+---+     C -- Carry
 *
 *  -- The GSWD instruction stores the status word in the upper 4 bits of each
 *     byte of the destination register:
 *
 *         F   E   D   C   B   A   9   8   7   6   5   4   3   2   1   0
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *       | S | Z | O | C | - | - | - | - | S | Z | O | C | - | - | - | - |
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *
 *  -- The RSWD instruction reads the status word's new value from the upper
 *     four bits of the lower byte of the source register:
 *
 *         7   6   5   4   3   2   1   0
 *       +---+---+---+---+---+---+---+---+
 *       | S | Z | O | C | - | - | - | - |
 *       +---+---+---+---+---+---+---+---+
 *
 *  SWAP notes:
 *
 *  -- A "double" swap replicates the lower byte into both bytes, and sets
 *     S and Z status bits based on that byte.  This is based on the comments
 *     in the following code snippet from RUNNING.SRC in the dev kit:
 *
 *           SWAP    R0, 2                 ; check bit #7
 *           BMI     @@exit                ; max speed is 127
 *
 * ============================================================================
 */


/*
 * ============================================================================
 *  OP_IMPL_1OP_A   -- Implied one-op instructions
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0000 000 0oo                               1    Implied 1-op insns
 * ============================================================================
 */
typedef struct op_impl_1op_a
{
    BFE(unsigned op         :  2,           /* Can have all four values */
    BFE(unsigned zero       :  8,           /* Must be zero!            */
        unsigned pad1       : 22));         /* Pad to 32-bit word       */
} op_impl_1op_a;

/*
 * ============================================================================
 *  OP_IMPL_1OP_B   -- Implied one-op instructions
 *
 *  Notes:
 *   -- Opcode field cannot be 00.  (That encoding is used by the Jump insns)
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0000 000 1oo                               1    Implied 1-op insns
 * ============================================================================
 */
typedef struct op_impl_1op_b
{
    BFE(unsigned op         :  2,           /* Can have all four values */
    BFE(unsigned one        :  1,           /* Must be one!             */
    BFE(unsigned zero       :  7,           /* Must be zero!            */
        unsigned pad1       : 22)));        /* Pad to 32-bit word       */
} op_impl_1op_b;

/*
 * ============================================================================
 *  OP_JUMP         -- Implied one-op instructions
 *
 *  Notes:
 *   -- Shares same 1st decle encoding as op_impl_1op_b, except op field == 0.
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0000 000 100  bbppppppii  pppppppppp       3    Jump insns
 * ============================================================================
 */
typedef struct op_jump
{
    /* Word #1  */
    BFE(unsigned op         :  2,           /* Can have all four values */
    BFE(unsigned one        :  1,           /* Must be one!             */
    BFE(unsigned zero       :  7,           /* Must be zero!            */
        unsigned pad1       : 22)));        /* Pad to 32-bit word       */

    /* Word #2 */
    BFE(unsigned intr_mode  :  2,           /* 11 is invalid            */
    BFE(unsigned dst_msb    :  6,           /* MSBs of destination      */
    BFE(unsigned save_reg   :  2,           /* Register to save to      */
        unsigned pad2       : 22)));        /* Pad to 32-bit word       */

    /* Word #3 */
    BFE(unsigned dst_lsb    : 10,           /* LSBs of destination      */
        unsigned pad3       : 22);          /* Pad to 32-bit word       */

} op_jump;

/*
 * ============================================================================
 *  OP_REG_1OP      -- Register-based one-op instructions
 *
 *  Notes:
 *   -- Opcode 000 is taken by jump and impl_1op encodings
 *   -- Opcode 110 aliases both GSWD encoding and NOP/SIN encoding.
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0000 ooo ddd                               1    1-op insns, comb src/dst
 * ============================================================================
 */
typedef struct op_reg_1op
{
    BFE(unsigned dst        :  3,           /* Can have all 8 values    */
    BFE(unsigned op         :  3,           /* 000, 110 invalid         */
    BFE(unsigned zero       :  4,           /* Must be zero!            */
        unsigned pad1       : 22)));        /* Pad to 32-bit word       */
} op_reg_1op;

/*
 * ============================================================================
 *  OP_GSWD         -- Get status word
 *
 *  Notes:
 *   -- Only one instruction (GSWD) has this encoding.
 *   -- Aliases reg_1op encoding's 110 opcode.
 *   -- Very similar to nop_sin encoding (below)
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0000 110 0dd                               1    GSWD
 * ============================================================================
 */
typedef struct op_gswd
{
    BFE(unsigned dst        :  2,           /* Can have all four values */
    BFE(unsigned id         :  4,           /* Must be 1100             */
    BFE(unsigned zero       :  4,           /* Must be zero!            */
        unsigned pad1       : 22)));        /* Pad to 32-bit word       */
} op_gswd;


/*
 * ============================================================================
 *  OP_NOP_SIN          -- NOP and SIN encoding
 *
 *  Notes:
 *   -- Only two instructions (NOP, SIN) have this encoding.
 *   -- Aliases reg_1op encoding's 110 opcode.
 *   -- Very similar to gswd encoding (above)
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0000 110 1om                               1    NOP(2), SIN(2)
 * ============================================================================
 */
typedef struct op_nop_sin
{
    BFE(unsigned imm        :  1,           /* 0 == '1', 1 == '2'       */
    BFE(unsigned op         :  1,           /* 0 == NOP, 1 == SIN       */
    BFE(unsigned id         :  4,           /* Must be 1101             */
    BFE(unsigned zero       :  4,           /* Must be zero!            */
        unsigned pad1       : 22))));       /* Pad to 32-bit word       */
} op_nop_sin;

/*
 * ============================================================================
 *  OP_ROT_1OP          -- Rotate/Shift instructions, one-op
 *
 *  Notes:
 *   -- Aliases reg_2op's 001 opcode.
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0001 ooo mrr                               1    Rotate/Shift insns
 * ============================================================================
 */
typedef struct op_rot_1op
{
    BFE(unsigned dst        :  2,           /* R0 .. R3                 */
    BFE(unsigned op         :  4,           /* All values valid         */
    BFE(unsigned id         :  4,           /* Must be 0001             */
        unsigned pad1       : 22)));        /* Pad to 32-bit word       */
} op_rot_1op;




/*
 * ============================================================================
 *  OP_REG_2OP          -- Register->Register 2OP instructions
 *
 *  Notes:
 *   -- The 000 opcode is used by other opcode formats above.
 *   -- The 001 opcode is used by rot_1op
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  0ooo sss ddd                               1    2-op arith, reg->reg
 * ============================================================================
 */

typedef struct op_reg_2op
{
    BFE(unsigned dst        :  3,           /* Dest. register  (R0..R7) */
    BFE(unsigned src        :  3,           /* Source register (R0..R7) */
    BFE(unsigned op         :  3,           /* 000, 001 invalid         */
    BFE(unsigned zero       :  1,           /* Must be 0                */
        unsigned pad1       : 22))));       /* Pad to 32-bit word       */
} op_reg_2op;


/*
 * ============================================================================
 *  OP_COND_BR          -- Conditional branch
 *
 *  Notes:
 *   -- Uses 000 opcode from imm_2op, ind_2op, and dir_2op spaces.
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  1000 zxc ccc  pppppppppp                   2    Branch insns
 * ============================================================================
 */
typedef struct op_cond_br
{
    /* Word #1 */
    BFE(unsigned cond       :  5,           /* Condition code + x bit   */
    BFE(unsigned dir        :  1,           /* Displacement direction   */
    BFE(unsigned id         :  4,           /* Must be 1000             */
        unsigned pad1       : 22)));        /* Pad to 32-bit word       */

    /* Word #2 */
    BFE(unsigned disp       : 16,           /* Displacement.            */
        unsigned pad2       : 16);          /* Pad to 32-bit word       */
} op_cond_br;



/*
 * ============================================================================
 *  OP_DIR_2OP          -- Direct->Register 2OP instructions
 *
 *  Notes:
 *   -- The 000 opcode is used by cond_br
 *   -- The 001 opcode interprets 'dst' as a src (MVOx instruction).
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  1ooo 000 ddd  pppppppppp                   2    2-op arith, direct->reg
 * ============================================================================
 */

typedef struct op_dir_2op
{
    /* Word #1 */
    BFE(unsigned dst        :  3,           /* Dest. register  (R0..R7) */
    BFE(unsigned zero       :  3,           /* Must be 0                */
    BFE(unsigned op         :  3,           /* 000 invalid              */
    BFE(unsigned id         :  1,           /* Must be 1                */
    BFE(unsigned xreg       :  3,           /* Extended register        */
    BFE(unsigned amode      :  2,           /* Extended address mode    */
        unsigned pad1       : 17))))));     /* Pad to 32-bit word       */

    /* Word #2 */
    BFE(unsigned addr       : 16,           /* Address (ls-bits)        */
        unsigned pad2       : 16);          /* Pad to 32-bit word       */

} op_dir_2op;


/*
 * ============================================================================
 *  OP_IND_2OP          -- Indirect->Register 2OP instructions
 *
 *  Notes:
 *   -- The 000 opcode is used by cond_br
 *   -- The 001 opcode interprets 'dst' as a src (MVOx instruction).
 *
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  1ooo mmm ddd                               1*   2-op arith, ind.  ->reg
 * ============================================================================
 */

typedef struct op_ind_2op
{
    BFE(unsigned reg        :  3,           /* Dest. register  (R0..R7) */
    BFE(unsigned addr       :  3,           /* Indirect address reg.    */
    BFE(unsigned op         :  3,           /* 000 invalid              */
    BFE(unsigned id         :  1,           /* Must be 1                */
    BFE(unsigned ext        :  6,           /* Extended opcode          */
        unsigned pad1       : 16)))));      /* Pad to 32-bit word       */
} op_ind_2op;



/*
 * ============================================================================
 *  OP_IMM_2OP          -- Register->Register 2OP instructions
 *
 *  Notes:
 *   -- The 000 opcode is used by cond_br
 *   -- The 001 opcode interprets 'dst' as a src (MVOx instruction).
 *
 *  Format                                   Words  Description
 *  ---------------------------------------  -----  -------------------------
 *  1ooo 111 ddd  iiiiiiiiii                   2*   2-op arith, immed.->reg
 * ============================================================================
 */

typedef struct op_imm_2op
{
    /* Word #1 */
    BFE(unsigned dst        :  3,           /* Dest. register  (R0..R7) */
    BFE(unsigned one        :  3,           /* Must be 111              */
    BFE(unsigned op         :  3,           /* 000 invalid              */
    BFE(unsigned id         :  1,           /* Must be 1                */
    BFE(unsigned ext        :  6,           /* Extended opcode          */
        unsigned pad1       : 16)))));      /* Pad to 32-bit word       */

    /* Word #2 */
    BFE(unsigned data_lsb   : 16,           /* Data (ls-bits)           */
        unsigned pad2       : 16);          /* Pad to 32-bit word       */

    /* Word #3 -- only when prefixed by SDBD */
    BFE(unsigned data_msb   :  8,           /* Data (ms-bits)           */
        unsigned pad3       : 24);          /* Pad to 32-bit word       */

} op_imm_2op;


/*
 * ============================================================================
 *  OP_DECODED          -- General form for decoded operands
 *
 *  All of the execute routines operate on the data contained in op_decoded.
 *  Decoder functions pick apart the data in the other op_XXX types and
 *  store it in an op_decoded structure.
 *
 *  The exact meaning ascribed to the various fields is dependent on the
 *  original opcode format as well as the instruction in question.  This
 *  format exists only because it's more convenient for the CPU to deal with
 *  than the packed bitfields.
 * ============================================================================
 */

typedef struct op_decoded
{
    uint16_t        imm0;               /* immediate value 0        */
    uint16_t        imm1;               /* immediate value 1        */
    uint8_t         reg0;               /* First register           */
    uint8_t         reg1;               /* Second register          */
    uint8_t         xreg0;              /* First xregister          */
    uint8_t         amode;              /* Extended addressing mode */
} op_decoded;

/*
 * ============================================================================
 *  OP_BREAKPT          -- Flags for breakpoints
 *
 *  These flags are cleverly stored in the last 16 bits of the union and
 *  so should never be overwritten.
 * ============================================================================
 */
typedef struct op_breakpt
{
    uint32_t        unused0;
    uint32_t        unused1;
    BFE(unsigned    cycles  : 16,       /* optional cycles to adv by    */
        unsigned    flags   : 16);      /* breakpoint flags             */
} op_breakpt;

/*
 * ============================================================================
 *  OP_ENCODED          -- Structure for the encoded instruction bits
 * ============================================================================
 */
typedef struct op_encoded
{
    BFE(unsigned word0      : 16,           /* Encoded instr word #1    */
        unsigned pad0       : 16);          /* Pad to 32-bit word       */
    BFE(unsigned word1      : 16,           /* Encoded instr word #2    */
        unsigned pad1       : 16);          /* Pad to 32-bit word       */
    BFE(unsigned word2      : 16,           /* Encoded instr word #3    */
        unsigned pad2       : 16);          /* Pad to 32-bit word       */
} op_encoded;

/*
 * ============================================================================
 *  INSTR_T             -- Instruction record
 *  INSTR_FMT_T         -- Enumeration of instruction formats
 * ============================================================================
 */

typedef enum instr_fmt_t
{
    fmt_impl_1op_a  = 0,
    fmt_impl_1op_b  = 1,
    fmt_jump        = 2,
    fmt_reg_1op     = 3,
    fmt_gswd        = 4,
    fmt_nop_sin     = 5,
    fmt_rot_1op     = 6,
    fmt_reg_2op     = 7,
    fmt_cond_br     = 8,
    fmt_dir_2op     = 9,
    fmt_ind_2op     = 10,
    fmt_imm_2op     = 11
} instr_fmt_t;

typedef union opcode_t
{
    op_encoded          encoded;        /* Actual raw words             */
    op_impl_1op_a       impl_1op_a;     /* Implied 1-op instruction     */
    op_impl_1op_b       impl_1op_b;     /* Implied 1-op instruction     */
    op_jump             jump;           /* Jump instruction             */
    op_reg_1op          reg_1op;        /* Register 1-op instruction    */
    op_gswd             gswd;           /* GSWD instruction             */
    op_nop_sin          nop_sin;        /* NOP or SIN instruction       */
    op_rot_1op          rot_1op;        /* Rotate/Shift instruction     */
    op_reg_2op          reg_2op;        /* Register->Register insn.     */
    op_cond_br          cond_br;        /* Conditional branch           */
    op_dir_2op          dir_2op;        /* Direct addressing insn.      */
    op_ind_2op          ind_2op;        /* Indirect addressing insn.    */
    op_imm_2op          imm_2op;        /* Immediate mode instruction   */
    op_decoded          decoder;        /* "Decoded" operation          */
    const op_decoded    decoded;        /* "Decoded" operation (const)  */
    op_breakpt          breakpt;        /* Breakpoint-related flags.    */
} opcode_t;

typedef struct instr_t
{
    unsigned        address;            /* Address this insn exists at  */
    opcode_t        opcode;
} instr_t;

typedef void op_decode_t(instr_t *, cp1600_ins_t **);

/*
 * ============================================================================
 *  FN_DECODE           -- Decode and execute an instruction for the CPU.
 *  FN_DECODE_1ST       -- Variant used for tracking first-time-decoded.
 *  FN_DECODE_BKPT      -- Variant used for making breakpoints persist.
 * ============================================================================
 */
int
fn_decode
(
    const instr_t *instr,
    cp1600_t      *cp1600
);

int
fn_decode_1st
(
    const instr_t *instr,
    cp1600_t      *cp1600
);

int
fn_decode_bkpt
(
    const instr_t *instr,
    cp1600_t      *cp1600
);

/*
 * ============================================================================
 *  GET_INSTR           -- allocs mem for a new instruction, and copies
 *                         in an existing instruction, if any.
 *  PUT_INSTR           -- frees mem for an old instruction.
 * ============================================================================
 */

instr_t *   get_instr(void);
void        put_instr(instr_t *instr);

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
/* ======================================================================== */
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
