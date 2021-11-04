/*
 * ============================================================================
 *  OP_DECODE:      Decoders for all of the CP-1610 formats
 *
 *  Author:         J. Zbiciak
 *
 *
 * ============================================================================
 *
 *  This module is responsible for decoding CP-1610 instructions on the fly.
 *  The main function is "fn_decode" which is registered as the "execute"
 *  function for all instructions which require decoding.  Included in this
 *  module are lookup tables used in decoding, as well as specialized decoder
 *  functions for all of the CP-1610 instruction formats.
 *
 *  Wherever possible, the CP-1610 model tries to move complexity away from
 *  the execute functions and towards the decoder.  This is due to the fact
 *  that we cache decoded functions whenever possible, and thus most
 *  instructions will only be decoded once.
 *
 *  Self-modifying code is supported by setting the execute functions for
 *  locations that are modified to "fn_decode", forcing the instructions
 *  at those locations to be re-decoded.  (Actually, the two locations just
 *  before the modified location are also set to "fn_decode", because some
 *  instructions are 2 or 3 words long.)
 *
 *  Some perverse code may have MVII instructions that are *optionally*
 *  executed w/ an SDBD prefix.  This is handled by requiring the execute
 *  functions to update the program counter, and having MVII actually check
 *  to see if we're in DBD mode for the current cycle.  We always decode 3
 *  bytes for MVII, and calculate both possible immediate operands in the
 *  decoder.   The execute function picks the correct immediate based on the
 *  DBD mode at execute time.
 *
 * ============================================================================
 *  Decoder functions/tables:
 *
 *  FN_DECODE           -- Decode execute function. Uses DEC_DECODE for decode.
 *  DEC_DECODE          -- Decoder function pointer lookup table
 *  DEC_IMPL_1OP_A      -- Decodes Implied   -> Register 1-op   (part a)
 *  DEC_IMPL_1OP_B      -- Decodes Implied   -> Register 1-op   (part b)
 *  DEC_JUMP            -- Decodes jump instructions
 *  DEC_REG_1OP         -- Decodes Register 1-op
 *  DEC_GSWD            -- Decodes GSWD instructions
 *  DEC_NOP_SIN         -- Decodes NOP and SIN instructions
 *  DEC_ROT_1OP         -- Decodes Rotate/Shift 1-op
 *  DEC_REG_2OP         -- Decodes Register  -> Register 2-op
 *  DEC_COND_BR         -- Decodes Conditional branches
 *  DEC_DIR_2OP         -- Decodes Direct    -> Register 2-op
 *  DEC_IND_2OP         -- Decodes Indirect  -> Register 2-op
 *  DEC_IMM_2OP         -- Decodes Immediate -> Register 2-op
 * ============================================================================
 *  Execute function pointer lookup tables:
 *
 *  FN_IND_2OP          -- Indirect  -> Register 2-op
 *  FN_DIR_2OP          -- Direct    -> Register 2-op
 *  FN_IMM_2OP          -- Immediate -> Register 2-op
 *  FN_COND_BR          -- Conditional branches
 *  FN_REG_2OP          -- Register  -> Register 2-op
 *  FN_ROT_1OP          -- Rotate/Shift 1-op
 *  FN_REG_1OP          -- Register 1-op
 *  FN_IMPL_1OP_A       -- Implied   -> Register 1-op   (part a)
 *  FN_IMPL_1OP_B       -- Implied   -> Register 1-op   (part b)
 * ============================================================================
 *  Miscellaneous functions:
 *
 *  GET_INSTR           -- allocs mem for a new instruction.
 *  PUT_INSTR           -- frees mem for an old instruction.
 * ============================================================================
 */


#include "config.h"
#include "periph/periph.h"
#include "cp1600.h"
#include "op_decode.h"
#include "op_exec.h"
#include "op_exec_ext.h"
#include "op_tables.h"




/*
 * ============================================================================
 *  Decoder functions/tables:
 *
 *  FN_DECODE           -- Master decoder function
 *  DEC_DECODE          -- Decoder function pointer lookup tables
 *  DEC_IMPL_1OP_A      -- Decodes Implied   -> Register 1-op   (part a)
 *  DEC_IMPL_1OP_B      -- Decodes Implied   -> Register 1-op   (part b)
 *  DEC_JUMP            -- Decodes jump instructions
 *  DEC_REG_1OP         -- Decodes Register 1-op
 *  DEC_GSWD            -- Decodes GSWD instructions
 *  DEC_NOP_SIN         -- Decodes NOP and SIN instructions
 *  DEC_ROT_1OP         -- Decodes Rotate/Shift 1-op
 *  DEC_REG_2OP         -- Decodes Register  -> Register 2-op
 *  DEC_COND_BR         -- Decodes Conditional branches
 *  DEC_DIR_2OP         -- Decodes Direct    -> Register 2-op
 *  DEC_IND_2OP         -- Decodes Indirect  -> Register 2-op
 *  DEC_IMM_2OP         -- Decodes Immediate -> Register 2-op
 * ============================================================================
 */
static  int prev_is_sdbd = 0;

LOCAL void dec_impl_1op_a(instr_t *, cp1600_ins_t **);
LOCAL void dec_impl_1op_b(instr_t *, cp1600_ins_t **);
LOCAL void dec_jump      (instr_t *, cp1600_ins_t **);
LOCAL void dec_reg_1op   (instr_t *, cp1600_ins_t **);
LOCAL void dec_gswd      (instr_t *, cp1600_ins_t **);
LOCAL void dec_nop_sin   (instr_t *, cp1600_ins_t **);
LOCAL void dec_rot_1op   (instr_t *, cp1600_ins_t **);
LOCAL void dec_reg_2op   (instr_t *, cp1600_ins_t **);
LOCAL void dec_cond_br   (instr_t *, cp1600_ins_t **);
LOCAL void dec_dir_2op   (instr_t *, cp1600_ins_t **);
LOCAL void dec_ind_2op   (instr_t *, cp1600_ins_t **);
LOCAL void dec_imm_2op   (instr_t *, cp1600_ins_t **);

LOCAL op_decode_t *const dec_decode[] =
{
    dec_impl_1op_a,
    dec_impl_1op_b,
    dec_jump,
    dec_reg_1op,
    dec_gswd,
    dec_nop_sin,
    dec_rot_1op,
    dec_reg_2op,
    dec_cond_br,
    dec_dir_2op,
    dec_ind_2op,
    dec_imm_2op
};

LOCAL   const int dec_length[] =
{
    1, 1, 3, 1, 1, 1, 1, 1, 2, 2, 1, 2
};


/*
 * ============================================================================
 *  DEC_IMPL_1OP_A      -- Decodes Implied   -> Register 1-op   (part a)
 * ============================================================================
 */
LOCAL   void     dec_impl_1op_a  (instr_t *instr, cp1600_ins_t **execute)
{
    /* -------------------------------------------------------------------- */
    /*  Just look up the execute function in the table.  It's that simple.  */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_impl_1op_a[instr->opcode.impl_1op_a.op];
}

/*
 * ============================================================================
 *  DEC_IMPL_1OP_B      -- Decodes Implied   -> Register 1-op   (part b)
 * ============================================================================
 */
LOCAL   void     dec_impl_1op_b  (instr_t *instr, cp1600_ins_t **execute)
{
    /* -------------------------------------------------------------------- */
    /*  Just look up the execute function in the table.  It's that simple.  */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_impl_1op_b[instr->opcode.impl_1op_b.op];
}

/*
 * ============================================================================
 *  DEC_JUMP            -- Decodes jump instructions
 * ============================================================================
 */
LOCAL   void     dec_jump        (instr_t *instr, cp1600_ins_t **execute)
{
    uint32_t imm0, imm1, reg0, op;

    /* -------------------------------------------------------------------- */
    /*  Set 'imm0' to be the destination address.  The dest address is      */
    /*  split across two opcode fields, so extract and merge together.      */
    /* -------------------------------------------------------------------- */
    imm0 = (instr->opcode.jump.dst_msb << 10) |
           (instr->opcode.jump.dst_lsb);

    /* -------------------------------------------------------------------- */
    /*  Set 'imm1' to be the "interrupt mode."  Note that we don't flag     */
    /*  the invalid case '11' at all.  Why bother?                          */
    /* -------------------------------------------------------------------- */
    imm1 = (instr->opcode.jump.intr_mode);

    /* -------------------------------------------------------------------- */
    /*  Set 'reg0' to be the return register (R4..R6).  If we decode R7,    */
    /*  then we pick a non-return-address saving Jump instead.              */
    /* -------------------------------------------------------------------- */
    reg0 = (instr->opcode.jump.save_reg) + 4;

    op   = reg0 == 7;   /* 1 == no save ret addr, 0 == save ret addr */

    /* -------------------------------------------------------------------- */
    /*  Set 'execute' to JSR if we have a valid return address, otherwise   */
    /*  set it to J.                                                        */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)(op ? fn_J_i : fn_JSR_ir);

    instr->opcode.decoder.imm0 = imm0;
    instr->opcode.decoder.imm1 = imm1;
    instr->opcode.decoder.reg0 = reg0;
}


/*
 * ============================================================================
 *  DEC_REG_1OP         -- Decodes Register 1-op
 * ============================================================================
 */
LOCAL   void     dec_reg_1op     (instr_t *instr, cp1600_ins_t **execute)
{
    /* -------------------------------------------------------------------- */
    /*  Look up the execute function in the table.                          */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_reg_1op[instr->opcode.reg_1op.op];

    /* -------------------------------------------------------------------- */
    /*  Set "reg0" to be the destination register for the 1-op.             */
    /* -------------------------------------------------------------------- */
    instr->opcode.decoder.reg0 = instr->opcode.reg_1op.dst;
}

/*
 * ============================================================================
 *  DEC_GSWD            -- Decodes GSWD instructions
 * ============================================================================
 */
LOCAL   void     dec_gswd        (instr_t *instr, cp1600_ins_t **execute)
{
    /* -------------------------------------------------------------------- */
    /*  This is hard-coded to the GSWD instruction.                         */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_GSWD_r;

    /* -------------------------------------------------------------------- */
    /*  Set "reg0" to the destination for GSWD.                             */
    /* -------------------------------------------------------------------- */
    instr->opcode.decoder.reg0 = instr->opcode.gswd.dst;
}

/*
 * ============================================================================
 *  DEC_NOP_SIN         -- Decodes NOP and SIN instructions
 * ============================================================================
 */
LOCAL   void     dec_nop_sin     (instr_t *instr, cp1600_ins_t **execute)
{
    /* -------------------------------------------------------------------- */
    /*  Pick between the two opcodes for this format -- NOP or SIN.         */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)(instr->opcode.nop_sin.op ? fn_SIN_i : fn_NOP_i);

    /* -------------------------------------------------------------------- */
    /*  Set 'imm0' to the one bit 'm' field value.  What is this used for?  */
    /* -------------------------------------------------------------------- */
    instr->opcode.decoder.imm0 = instr->opcode.nop_sin.imm;
}

/*
 * ============================================================================
 *  DEC_ROT_1OP         -- Decodes Rotate/Shift 1-op
 * ============================================================================
 */
LOCAL   void     dec_rot_1op     (instr_t *instr, cp1600_ins_t **execute)
{
    /* -------------------------------------------------------------------- */
    /*  Look up the rotate/shift function from the table.                   */
    /*  Note:  The ",2" bit is considered part of the opcode for speed.     */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_rot_1op[instr->opcode.rot_1op.op];

    /* -------------------------------------------------------------------- */
    /*  Set 'reg0' to the src/dst register.                                 */
    /* -------------------------------------------------------------------- */
    instr->opcode.decoder.reg0 = instr->opcode.rot_1op.dst;
}

/*
 * ============================================================================
 *  DEC_REG_2OP         -- Decodes Register  -> Register 2-op
 * ============================================================================
 */
LOCAL   void     dec_reg_2op     (instr_t *instr, cp1600_ins_t **execute)
{
    uint32_t reg0, reg1, pc0, pc1, op;


    /* -------------------------------------------------------------------- */
    /*  Set 'reg0' to the source and 'reg1' to the destination.             */
    /* -------------------------------------------------------------------- */
    reg0 = instr->opcode.reg_2op.src;
    reg1 = instr->opcode.reg_2op.dst;
    pc0  = reg0 == 7;
    pc1  = reg1 == 7 && reg0 != 7;

    /* -------------------------------------------------------------------- */
    /*  Look up the opcode in the reg_2op table.  Set bit 3 if dst == PC.   */
    /*  Handle special case of TSTR Rx directly for minor speedup.          */
    /* -------------------------------------------------------------------- */
    op   = instr->opcode.reg_2op.op | (pc1<<3) | (pc0<<4);
    *execute = (reg0 == reg1 && op == 2) ?
                        (cp1600_ins_t *)fn_TST_rr :
                        (cp1600_ins_t *)fn_reg_2op[op];

    instr->opcode.decoder.reg0 = reg0;
    instr->opcode.decoder.reg1 = reg1;
}

/*
 * ============================================================================
 *  DEC_COND_BR         -- Decodes Conditional branches
 * ============================================================================
 */
LOCAL   void     dec_cond_br     (instr_t *instr, cp1600_ins_t **execute)
{
    uint32_t imm0, imm1;

    /* -------------------------------------------------------------------- */
    /*  Look up the opcode in the cond_br table.  This opcode word has      */
    /*  the 'x' and 'nccc' bits in it, for a 5-bit field.  The 'n' bit is   */
    /*  treated as a don't-care here for now, although I'll eventually      */
    /*  specialize the branches too.                                        */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_cond_br[instr->opcode.cond_br.cond];

    /* -------------------------------------------------------------------- */
    /*  Set 'imm0' to be the branch destination address.  Negative offsets  */
    /*  are achieved by taking the 1s complement of the displacement before */
    /*  adding it to the address.  The address *after* the instruction      */
    /*  (instr->address+2) is used for the calculation.                     */
    /* -------------------------------------------------------------------- */
    imm0 = instr->opcode.cond_br.disp;

    if (instr->opcode.cond_br.dir)
        imm0 = ~imm0;

    imm0 += instr->address + 2 ;

    /* -------------------------------------------------------------------- */
    /*  Set 'imm1' to the branch condition code.  Only used by BEXT.        */
    /* -------------------------------------------------------------------- */
    imm1 = instr->opcode.cond_br.cond & 0xF;

    instr->opcode.decoder.imm0 = imm0;
    instr->opcode.decoder.imm1 = imm1;
}

/*
 * ============================================================================
 *  DEC_DIR_2OP         -- Decodes Direct    -> Register 2-op
 * ============================================================================
 */
LOCAL   void     dec_dir_2op     (instr_t *instr, cp1600_ins_t **execute)
{
    uint32_t reg0, imm0, xreg0, amode, imm1;

    /* -------------------------------------------------------------------- */
    /*  Look up the opcode in the Direct->Register 2op table.               */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_dir_2op[instr->opcode.dir_2op.op];

    /* -------------------------------------------------------------------- */
    /*  Set 'reg0' to the src/dst register, and 'imm0' to the address.      */
    /* -------------------------------------------------------------------- */
    reg0  = instr->opcode.dir_2op.dst;
    imm0  = instr->opcode.dir_2op.addr;
    imm1  = instr->opcode.dir_2op.op;
    xreg0 = instr->opcode.dir_2op.xreg;
    amode = instr->opcode.dir_2op.amode;

    instr->opcode.decoder.reg0  = reg0;
    instr->opcode.decoder.imm0  = imm0;
    instr->opcode.decoder.imm1  = imm1;
    instr->opcode.decoder.xreg0 = xreg0;
    instr->opcode.decoder.amode = amode;
}

/*
 * ============================================================================
 *  DEC_IND_2OP         -- Decodes Indirect  -> Register 2-op
 * ============================================================================
 */
LOCAL   void     dec_ind_2op     (instr_t *instr, cp1600_ins_t **execute)
{
    uint32_t reg0, reg1, amode = 0;
    uint32_t op;
    uint32_t no_dbd = 0;

    /* -------------------------------------------------------------------- */
    /*  Set 'reg0' to be the address register, and 'reg1' to be the src/    */
    /*  dest registers.                                                     */
    /* -------------------------------------------------------------------- */
    reg0 = instr->opcode.ind_2op.addr;
    reg1 = instr->opcode.ind_2op.reg;

    /* -------------------------------------------------------------------- */
    /*  If the previous word was not an SDBD, then we can use the non-DBD   */
    /*  versions of the instructions.                                       */
    /* -------------------------------------------------------------------- */
    if (!prev_is_sdbd)
        no_dbd = 32;

    /* -------------------------------------------------------------------- */
    /*  Encode the ms-bits of the register number into 'op', so that we     */
    /*  can distinguish non-incrementing, incrementing, and stack modes     */
    /*  in our lookup table.                                                */
    /* -------------------------------------------------------------------- */
    op   = instr->opcode.ind_2op.op | ((reg0 & 6) << 2) | no_dbd;


    /* -------------------------------------------------------------------- */
    /*  Extended opcodes:  If op == 1 && ext == 1..3, it's an atomic.       */
    /* -------------------------------------------------------------------- */
    if (instr->opcode.ind_2op.op == 1 && instr->opcode.ind_2op.ext <= 3)
        amode = instr->opcode.ind_2op.ext;

    /* -------------------------------------------------------------------- */
    /*  Look up the modified opcode in the Indirect->Register 2op table.    */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_ind_2op[op];

    instr->opcode.decoder.reg0  = reg0;
    instr->opcode.decoder.reg1  = reg1;
    instr->opcode.decoder.amode = amode;
}

/*
 * ============================================================================
 *  DEC_IMM_2OP         -- Decodes Immediate -> Register 2-op
 * ============================================================================
 */

LOCAL   void     dec_imm_2op     (instr_t *instr, cp1600_ins_t **execute)
{
    uint32_t reg0, reg1 = 0, xreg0 = 0, imm0, imm1, amode = 0, no_dbd = 0;
    extern int lto_isa_enabled;

    /* -------------------------------------------------------------------- */
    /*  Consider the "dead air" opcode 0xFFFF as "off in the weeds."  It    */
    /*  looks like XORI #xxxx, R7, but with bits 16:10 set to 1 also.       */
    /* -------------------------------------------------------------------- */
    const bool in_the_weeds = instr->opcode.encoded.word0 == 0xFFFF;

    /* -------------------------------------------------------------------- */
    /*  Peek at the previous word and see if it's SDBD.  If not, we will    */
    /*  never have our D bit set, so we can use the faster version of the   */
    /*  execute functions.                                                  */
    /* -------------------------------------------------------------------- */
    if (!prev_is_sdbd)
    {
        no_dbd = 8;
    }

    /* -------------------------------------------------------------------- */
    /*  Look up the opcode in the Immediate->Register 2op table.  If we     */
    /*  determined we don't need to support SDBD, look up the faster        */
    /*  version of the instruction (encoded as bit 3 of the opcode).        */
    /* -------------------------------------------------------------------- */
    *execute = (cp1600_ins_t *)fn_imm_2op[instr->opcode.imm_2op.op | no_dbd];

    /* -------------------------------------------------------------------- */
    /*  Set 'reg0' to be the src/dst register.                              */
    /* -------------------------------------------------------------------- */
    reg0 = instr->opcode.imm_2op.dst;

    /* -------------------------------------------------------------------- */
    /*  Set 'imm0' to the non-DBD version of the immediate constant.        */
    /* -------------------------------------------------------------------- */
    imm0 = instr->opcode.imm_2op.data_lsb;

    /* -------------------------------------------------------------------- */
    /*  If we need to support DBD, then set 'imm1' to be the DBD version    */
    /*  of the immediate constant.  The correct constant is selected at     */
    /*  execute time.                                                       */
    /* -------------------------------------------------------------------- */
    imm1 = (instr->opcode.imm_2op.data_msb << 8) | (imm0 & 0xFF);

    /* -------------------------------------------------------------------- */
    /*  If this is an extended opcode, rewrite a few details.               */
    /* -------------------------------------------------------------------- */
    if (lto_isa_enabled && !in_the_weeds &&
        instr->opcode.imm_2op.op == 1 && instr->opcode.imm_2op.ext > 0)
    {
        *execute = (cp1600_ins_t *)fn_ext_isa;
        amode = instr->opcode.imm_2op.ext;  // Extended opcode field
        reg1  = (imm0 >> 4) & 0xF;          // src2 field, if xregister
        imm1  = ((imm0 >> 4) & 0xF) ^ ((amode & 2) ? 0xFFFFu : 0); // src2 cst
        xreg0 = (imm0 >> 0) & 0xF;          // dst field

        // Get extra bit for src1xr if bit 13 and bit 14 set
        if ((amode & 0x18) == 0x18)
            reg0 |= 0x8;
    }

    /* -------------------------------------------------------------------- */
    /*  If it turns out this was simply an 0xFFFF opcode, set amode = 1     */
    /*  to let fn_XOR_Ir know to do the right thing.                        */
    /* -------------------------------------------------------------------- */
    if (in_the_weeds)
        amode = 1;

    instr->opcode.decoder.reg0  = reg0;
    instr->opcode.decoder.reg1  = reg1;
    instr->opcode.decoder.xreg0 = xreg0;
    instr->opcode.decoder.amode = amode;
    instr->opcode.decoder.imm0  = imm0;
    instr->opcode.decoder.imm1  = imm1;
}

/*
 * ============================================================================
 *  FN_DECODE           -- Decodes and execute an instruction
 * ============================================================================
 */
int
fn_decode
(
    const instr_t *ins,
    cp1600_t *cp1600
)
{
    uint16_t w, pw, pc, pc2, dpc, dpc2;
    int cycles, words;
    cp1600_ins_t *fn_execute = (cp1600_ins_t *)fn_invalid;
    instr_fmt_t format;
    instr_t *instr;

    UNUSED(ins);

    /* -------------------------------------------------------------------- */
    /*  Grab our PC, and set its value in the new instruction.  Register    */
    /*  the instruction with the CP1600.                                    */
    /* -------------------------------------------------------------------- */
    pc = cp1600->r[7];

    if (!(instr = cp1600->instr[pc]))
        instr = get_instr();

    instr->address     = pc;
    cp1600->instr [pc] = instr;
    cp1600->disasm[pc] = NULL;

    /* -------------------------------------------------------------------- */
    /*  Read the first word of the instruction, so that we can determine    */
    /*  what format instruction this is.  Go ahead and store this word in   */
    /*  the encoded-opcode field of the decoder union.  Also look up the    */
    /*  previous word.  We use this to determine if an immediate-mode       */
    /*  instruction might need to support SDBD.                             */
    /* -------------------------------------------------------------------- */
    w  = CP1600_RD(cp1600, pc + 0);
    pw = cp1600->D ? 0x0001 : CP1600_PK(cp1600, 0xFFFF & (pc - 1));
    instr->opcode.encoded.word0 = w;

#if 0
    /*if (w & 0xFC00) *//* Is this instruction longer than 10 bits? */
    if (w == 0xFFFF) /* Jumped into null memory? */
    {
        fprintf(stderr, "CPU off in the weeds @ PC == %.4x, w = %.4x\n", pc, w);
        fprintf(stderr, "instruction count: %" U64_FMT "\n", cp1600->tot_instr);
        dump_state();
        if (cp1600->instr_tick)
            cp1600->instr_tick(cp1600->instr_tick_periph, (unsigned)-INT_MAX);
        else
            exit(1);
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  Look up the format and length of the instruction.  Set 'pc2' to     */
    /*  the last address this instruction occupies.  If the format is       */
    /*  "immediate" and the previous word was "SDBD", add 1 to the length.  */
    /* -------------------------------------------------------------------- */
    format = (instr_fmt_t) dec_format[w & 0x03FF];
    words  = dec_length[(int)format] + (format == fmt_imm_2op && pw == 0x0001);
    pc2    = pc + words - 1;

    /* -------------------------------------------------------------------- */
    /*  Read the next 1 or 2 words if necessaary.                           */
    /*                                                                      */
    /*  Subtle:  Increment PC only during fetch, so that debugger marks     */
    /*  these as code fetches.  Don't *actually* increment PC in the end.   */
    /*  The actual PC update happens when we execute the instruction.       */
    /* -------------------------------------------------------------------- */
    if (words > 1)
    {
        cp1600->r[7] = pc + 1;
        instr->opcode.encoded.word1 = CP1600_RD(cp1600, pc + 1);
    }
    if (words > 2)
    {
        cp1600->r[7] = pc + 2;
        instr->opcode.encoded.word2 = CP1600_RD(cp1600, pc + 2);
    }
    cp1600->r[7] = pc;

    /* -------------------------------------------------------------------- */
    /*  Call the specific decoder function, and based on the result, call   */
    /*  the execute function for the instruction.                           */
    /* -------------------------------------------------------------------- */
    prev_is_sdbd = (pw == 0x0001);
    dec_decode[(int)format](instr, &fn_execute);
    cycles = fn_execute(instr,cp1600);

    /* -------------------------------------------------------------------- */
    /*  If the decoded instruction is at a "cacheable" address, store the   */
    /*  decoded function pointer in the CP1600 structure.  If this is a     */
    /*  breakpoint, don't store anything.                                   */
    /* -------------------------------------------------------------------- */
    if (cp1600->execute[pc] != fn_breakpt)
    {
        dpc  = pc  >> CP1600_DECODE_PAGE;
        dpc2 = pc2 >> CP1600_DECODE_PAGE;

        if ( 1 & (cp1600->cacheable[dpc  >> 5] >> (dpc  & 31)) &
                 (cp1600->cacheable[dpc2 >> 5] >> (dpc2 & 31)) )
        {
            cp1600->execute[pc] = fn_execute;
            cp1600->tot_cache++;
        } else
        {
            cp1600->execute[pc] = fn_decode;
            cp1600->tot_noncache++;
        }
    }

#ifdef DEBUG_DECODE_CACHE
    /* -------------------------------------------------------------------- */
    /*  Mark down that we decoded the locations covered by this instr.      */
    /* -------------------------------------------------------------------- */
    cp1600->decoded[pc]++;
    if (words>1) cp1600->decoded[pc+1]++;
    if (words>2) cp1600->decoded[pc+2]++;
#endif

    /* -------------------------------------------------------------------- */
    /*  Return the number of cycles spent in the execute function.          */
    /* -------------------------------------------------------------------- */
    return cycles;
}

int
fn_decode_1st
(
    const instr_t *instr,
    cp1600_t *cp1600
)
{
    return fn_decode(instr, cp1600);
}

int
fn_decode_bkpt
(
    const instr_t *instr,
    cp1600_t *cp1600
)
{
    cp1600->execute[cp1600->r[7]] = fn_breakpt;
    return fn_decode(instr, cp1600);
}

/*
 * ============================================================================
 *  GET_INSTR           -- allocs mem for a new instruction.
 *  PUT_INSTR           -- frees mem for an old instruction.
 * ============================================================================
 */

typedef union instr_list_t
{
    union instr_list_t *next;
    instr_t instr;
} instr_list_t;

instr_list_t *instr_list_head = NULL;
#define INSTR_CHUNK (16)

instr_t * get_instr (void)
{
    instr_t * new_instr;

    if (instr_list_head == NULL)
    {
        int i;

        instr_list_head = CALLOC(instr_list_t, INSTR_CHUNK);

        if (!instr_list_head)
        {
            fprintf(stderr,"Out of memory in get_instr!!\n");
            exit(1);
        }

        /* build linked list */

        for (i = 1; i < INSTR_CHUNK; i++)
        {
            instr_list_head[i - 1].next = &instr_list_head[i];
        }

        instr_list_head[INSTR_CHUNK - 1].next = NULL;
    }

    new_instr = & (instr_list_head->instr);

    instr_list_head = instr_list_head->next;

    memset(new_instr, 0, sizeof(instr_t));

    return new_instr;
}

void
put_instr
(
    instr_t * instr
)
{
    ((instr_list_t*)(void *)instr)->next = instr_list_head;
    instr_list_head = (instr_list_t*)(void *)instr;
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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
