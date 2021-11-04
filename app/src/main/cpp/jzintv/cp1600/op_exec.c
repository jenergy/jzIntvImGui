/*
 * ============================================================================
 *  OP_EXEC:        Execute functions for the CP-1610 instructions
 *
 *  Author:         J. Zbiciak
 *
 *
 * ============================================================================
 *  fn_invalid      -- Executed when a decoder failure happens
 *  fn_XXXX_i       -- Immediate operand instructions (relative branch, jump)
 *  fn_XXXX_r       -- Implied, register 2-ops (eg. ADCR, COMR, etc.)
 *  fn_XXXX_ir      -- Immediate, register 2-ops, JSR
 *  fn_XXXX_Ir      -- Immediate (no DBD), register 2-ops, JSR
 *  fn_XXXX_rr      -- Register, register 2-ops
 *  fn_XXXX_dr      -- Direct, register 2-ops
 *  fn_XXXX_mr      -- Indirect ("memory"), register 2-ops, non-incrementing
 *  fn_XXXX_nr      -- Indirect (no DBD),   register 2-ops, non-incrementing
 *  fn_XXXX_Mr      -- Indirect ("memory"), register 2-ops, post-incrementing
 *  fn_XXXX_Nr      -- Indirect (no DBD),   register 2-ops, post-incrementing
 *  fn_XXXX_Sr      -- Indirect ("stack"),  register 2-ops, pre-decrementing
 * ============================================================================
 */


#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/op_decode.h"
#include "cp1600/op_exec.h"
#include "cp1600/emu_link.h"
#include "debug/debug_if.h"
#include <limits.h>


int first_dis = -1;
int last_dis  = -1;

int fn_invalid  (const instr_t *instr, cp1600_t *cp1600)
{
    jzp_printf("Invalid opcode @ 0x%.4X : %.4X %.4X %.4X\n",
            instr->address,
            CP1600_RD(cp1600, instr->address + 0),
            CP1600_RD(cp1600, instr->address + 1),
            CP1600_RD(cp1600, instr->address + 2));

    cp1600->r[7]++;

    return 0;
}


int fn_breakpt  (const instr_t *instr, cp1600_t *cp1600)
{
    const uint32_t pc = cp1600->r[7];
    uint32_t flags = instr->opcode.breakpt.flags;

    cp1600->hit_breakpoint =
        flags & CP1600_BKPT_ONCE ? BK_TRACEPOINT : BK_BREAKPOINT;
    cp1600->D <<= 1;

    flags &= ~CP1600_BKPT_ONCE;

    cp1600->execute[pc] = flags ? fn_decode_bkpt : fn_decode;

    /* ugly */
    cp1600->instr[pc]->opcode.breakpt.flags = flags;
    cp1600->instr[pc]->opcode.breakpt.cycles = 0;

    return CYC_MAX;
}


int fn_BEXT_i   (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    taken = cp1600->ext == instr->opcode.decoded.imm1;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}


/* "Positive" versions of branches */


int fn_B_i      (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7] += 2;

    cp1600->r[7] = instr->opcode.decoded.imm0;

    return 9;
}

int fn_BOV_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* O = 1 */
    taken = cp1600->O;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BC_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* C = 1 */
    taken = cp1600->C;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BPL_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* S = 0 */
    taken = !cp1600->S;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BEQ_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* Z = 1 */
    taken =  cp1600->Z;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BLT_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* S ^ O = 1 */
    taken = cp1600->S ^ cp1600->O;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}


int fn_BLE_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* Z | S ^ O = 1 */
    taken = (cp1600->Z | (cp1600->S ^ cp1600->O));

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BUSC_i   (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;

    cp1600->r[7] += 2;

    /* C ^ S = 1 */
    taken = cp1600->C ^ cp1600->S;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

/* "Negative" versions of branches. */

int fn_NOPP_i      (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7] += 2;

    UNUSED(instr);

    return 7;
}

int fn_BNOV_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* O = 0 */
    taken = !cp1600->O;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BNC_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* C = 0 */
    taken = !cp1600->C;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BMI_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* S = 1 */
    taken = cp1600->S;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BNEQ_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* Z = 0 */
    taken = !cp1600->Z;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BGE_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* S ^ O = 0 */
    taken = !(cp1600->S ^ cp1600->O);

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}


int fn_BGT_i    (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;
    cp1600->r[7] += 2;

    /* Z | (S ^ O) = 0 */
    taken = !(cp1600->Z | (cp1600->S ^ cp1600->O));

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}

int fn_BESC_i   (const instr_t *instr, cp1600_t *cp1600)
{
    int taken;

    cp1600->r[7] += 2;

    /* C ^ S = 0 */
    taken = cp1600->C == cp1600->S;

    if (taken) cp1600->r[7] = instr->opcode.decoded.imm0;

    return 7 + (taken << 1);
}


int fn_SIN_i    (const instr_t *instr, cp1600_t *cp1600)
{

    cp1600->r[7]++;
    /* XXX: does SIN need to do anything?! */

    if (cp1600->r[0] == 0x656D && cp1600->r[1] == 0x753F)
    {
        cp1600->r[0] = 0x4A5A;
        cp1600->r[1] = JZINTV_VERSION;
        cp1600->C    = 0;
        return 6;
    }

    /* We use it as a hook for emulator-specific functionality. */
    emu_link_dispatch(cp1600);

    UNUSED(instr);
    /*cp1600->intr = 0;*/

    return 6;
}

int fn_NOP_i    (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7]++;

    UNUSED(instr);
    /*cp1600->intr = 0;*/

    return 6;
}

int fn_J_i      (const instr_t *instr, cp1600_t *cp1600)
{
    int intr = instr->opcode.decoded.imm1;

    if (intr)
    {
        cp1600->I = intr & 1;
        cp1600->intr = (intr & 1) ? CP1600_INT_ENABLE | CP1600_INT_INSTR : 0;
    }

    cp1600->r[7] = instr->opcode.decoded.imm0;

    return 13;
}

int fn_JSR_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    int intr = instr->opcode.decoded.imm1;

    if (intr)
    {
        cp1600->I = intr & 1;
        cp1600->intr = (intr & 1) ? CP1600_INT_ENABLE | CP1600_INT_INSTR : 0;
    }

    cp1600->r[instr->opcode.decoded.reg0] = cp1600->r[7] + 3;
    cp1600->r[7]                          = instr->opcode.decoded.imm0;

    return 13;
}

#define ADD_SZOC(a,b,c,cp1600)                             \
    do {                                                   \
        uint32_t op1 = (a);                                \
        uint32_t op2 = (b);                                \
        uint32_t op3;                                      \
        uint16_t res;                                      \
        op3 = op2 + op1;                                   \
        res = (uint16_t)op3;                               \
        (cp1600)->S = !!(op3 & 0x8000);                    \
        (cp1600)->C = !!(op3 & 0x10000);                   \
        (cp1600)->O = !!((op2^op3) & ~(op1^op2) & 0x8000); \
        (cp1600)->Z = !res;                                \
        (c) = res;                                         \
    } while(0);

#define SUB_SZOC(a,b,c,cp1600)                             \
    do {                                                   \
        uint32_t op1 = (a);                                \
        uint32_t op2 = (b);                                \
        uint32_t op3;                                      \
        uint16_t res;                                      \
        op3 = op2 + (0xFFFF ^ op1) + 1;                    \
        res = (uint16_t)op3;                               \
        (cp1600)->S = !!(op3 & 0x8000);                    \
        (cp1600)->C = !!(op3 & 0x10000);                   \
        (cp1600)->O = !!((op2^op3) & (op1^op2) & 0x8000);  \
        (cp1600)->Z = !res;                                \
        (c) = res;                                         \
    } while(0);


#define EXEC_SZ(a,b,c,o,cp1600)                            \
    do {                                                   \
        uint32_t op1 = (a);                                \
        uint32_t op2 = (b);                                \
        uint32_t op3;                                      \
        uint16_t res;                                      \
        op3 = op2 o op1;                                   \
        res = (uint16_t)op3;                               \
        (cp1600)->S = !!(op3 & 0x8000);                    \
        (cp1600)->Z = !res;                                \
        (c) = res;                                         \
    } while(0);

#define NEG(x) (0x10000-(uint32_t)(x))
#define EXTRA_IF_R6R7(r) ((instr->opcode.decoded.r) >= 6)

LOCAL uint16_t ext_addr_read(const instr_t *instr, cp1600_t *cp1600)
{
    extern int lto_isa_enabled;
    const int amode = instr->opcode.decoder.amode;
    const uint16_t reg0 = instr->opcode.decoder.reg0;
    const uint16_t imm0 = instr->opcode.decoder.imm0;
    const uint16_t imm1 = instr->opcode.decoder.imm1;
    const uint16_t xreg = instr->opcode.decoder.xreg0;
    const uint16_t base = cp1600->xr[xreg];
    const uint16_t addr = base + imm0;

    if ((!amode && !xreg) || !lto_isa_enabled)
        return CP1600_RD(cp1600, imm0);

    if (!amode)
    {
        if (reg0 == 7 && imm1 == 2)
            return base ? cp1600->r[7] + imm0 - 1 : cp1600->r[7];

        if (reg0 == 7 && imm1 == 7)
        {
            const uint16_t dest = (cp1600->r[7] - 1) + imm0;
            const uint16_t delt = cp1600->r[7] ^ dest;
            return --cp1600->xr[xreg] ? delt : 0;
        }

        return addr;
    }

    if (amode & 2)
        cp1600->xr[xreg] = addr;

    return CP1600_RD(cp1600, amode & 1 ? addr : base);
}

LOCAL void ext_addr_write(const instr_t *instr, cp1600_t *cp1600, uint16_t data)
{
    extern int lto_isa_enabled;
    const int amode = instr->opcode.decoder.amode;
    const uint16_t imm0 = instr->opcode.decoder.imm0;
    const uint16_t xreg = instr->opcode.decoder.xreg0;
    const uint16_t base = cp1600->xr[xreg];
    const uint16_t addr = base + imm0;

    if ((!amode && !xreg) || !lto_isa_enabled)
    {
        CP1600_WR(cp1600, imm0, data);
        return;
    }

    if (!amode)
    {
        cp1600->xr[xreg] = data + imm0;
        return;
    }

    if (amode & 2)
        cp1600->xr[xreg] = addr;

    CP1600_WR(cp1600, amode & 1 ? addr : base, data);
}


int fn_MVO_dr   (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7] += 2;

    ext_addr_write(instr, cp1600, cp1600->r[instr->opcode.decoded.reg0]);

    cp1600->intr = 0;
    return 11;
}

#define SDBD_DIRECT(data,cp1600,cycles) \
    if (cp1600->D)                                                  \
    {                                                               \
        uint16_t sdbd_data = CP1600_RD(cp1600, cp1600->r[7]);       \
        cp1600->r[7]++;                                             \
        data = (data & 0xFF) | ((sdbd_data & 0xFF) << 8);           \
        cycles += 3;                                                \
    }


int fn_MVI_dr   (const instr_t *instr, cp1600_t *cp1600)
{
    int cycles = 10 + EXTRA_IF_R6R7(reg0);
    uint16_t r0;
    cp1600->r[7] += 2;

    r0 = ext_addr_read(instr, cp1600);
    SDBD_DIRECT(r0,cp1600,cycles);
    cp1600->r[instr->opcode.decoded.reg0] = r0;

    return cycles;
}


int fn_ADD_dr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int cycles = 10 + EXTRA_IF_R6R7(reg0);
    cp1600->r[7] += 2;

    r0 = ext_addr_read(instr, cp1600);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SDBD_DIRECT(r0,cp1600,cycles);
    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return cycles;
}

int fn_SUB_dr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int cycles = 10 + EXTRA_IF_R6R7(reg0);
    cp1600->r[7] += 2;

    r0 = ext_addr_read(instr, cp1600);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SDBD_DIRECT(r0,cp1600,cycles);
    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return cycles;
}

int fn_CMP_dr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int cycles = 10 + EXTRA_IF_R6R7(reg0);
    cp1600->r[7] += 2;

    r0 = ext_addr_read(instr, cp1600);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SDBD_DIRECT(r0,cp1600,cycles);
    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);

    return cycles;
}

int fn_AND_dr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int cycles = 10 + EXTRA_IF_R6R7(reg0);
    cp1600->r[7] += 2;

    r0 = ext_addr_read(instr, cp1600);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SDBD_DIRECT(r0,cp1600,cycles);
    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return cycles;
}

int fn_XOR_dr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int cycles = 10 + EXTRA_IF_R6R7(reg0);
    cp1600->r[7] += 2;

    r0 = ext_addr_read(instr, cp1600);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SDBD_DIRECT(r0,cp1600,cycles);
    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return cycles;
}

#define DBDI(d,i) ((d)?(i)->opcode.decoded.imm1:(i)->opcode.decoded.imm0)

static uint16_t handle_atomic(const instr_t *instr, cp1600_t *cp1600,
                              uint16_t addr, uint16_t data)
{
    if (instr->opcode.decoded.amode == 0) return data;
    if (instr->opcode.decoded.amode == 1) return data + CP1600_RD(cp1600, addr);
    if (instr->opcode.decoded.amode == 2) return data & CP1600_RD(cp1600, addr);
    if (instr->opcode.decoded.amode == 3) return data | CP1600_RD(cp1600, addr);
    return data;
}

int fn_MVO_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;
    uint16_t addr = ++cp1600->r[7];
    r0 = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;

    if (cp1600->D)
        jzp_printf("WARNING: MVO@ w/ SDBD prefix @ %.4X\n", instr->address);

    CP1600_WR(cp1600, addr, r0);

    cp1600->intr = 0;
    return 9;
}

int fn_MVI_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;
    int d = cp1600->D;

    cp1600->r[7] += 2 + d;

    r0 = DBDI(d,instr);
    cp1600->r[instr->opcode.decoded.reg0] = r0;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_ADD_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int d = cp1600->D;

    cp1600->r[7] += 2 + cp1600->D;

    r0 = DBDI(d,instr);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_SUB_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int d = cp1600->D;

    cp1600->r[7] += 2 + cp1600->D;

    r0 = DBDI(d,instr);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_CMP_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int d = cp1600->D;

    cp1600->r[7] += 2 + cp1600->D;

    r0 = DBDI(d,instr);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_AND_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int d = cp1600->D;

    cp1600->r[7] += 2 + cp1600->D;

    r0 = DBDI(d,instr);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_XOR_ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int d = cp1600->D;

    cp1600->r[7] += 2 + cp1600->D;

    r0 = DBDI(d,instr);
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_MVO_Ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;
    cp1600->r[7] += 2;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    CP1600_WR(cp1600, instr->address+1, r0);

    cp1600->intr = 0;
    return 9;
}

int fn_MVI_Ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;

    cp1600->r[7] += 2;

    r0 = instr->opcode.decoded.imm0;
    cp1600->r[instr->opcode.decoded.reg0] = r0;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_ADD_Ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    cp1600->r[7] += 2;

    r0 = instr->opcode.decoded.imm0;
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_SUB_Ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    cp1600->r[7] += 2;

    r0 = instr->opcode.decoded.imm0;
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_CMP_Ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    cp1600->r[7] += 2;

    r0 = instr->opcode.decoded.imm0;
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_AND_Ir   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    cp1600->r[7] += 2;

    r0 = instr->opcode.decoded.imm0;
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_XOR_Ir   (const instr_t *instr, cp1600_t *cp1600)
{
    const uint32_t pc = cp1600->r[7];
    uint16_t r0,r1,r2;

    cp1600->r[7] += 2;

    r0 = instr->opcode.decoded.imm0;
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    /* Special hook: amode tells us if this was the 0xFFFF opcode. */
    if (instr->opcode.decoded.amode)
    {
        if (debug_fault_detected != DEBUG_CRASHING)
        {
            jzp_printf("\nCPU off in the weeds!\nPC: $%.4X   "
                       "Instr count: %" U64_FMT "   Cycles: %" U64_FMT "\n\n",
                        pc, cp1600->tot_instr, cp1600->tot_cycle);
        }

        debug_fault_detected = DEBUG_CRASHING;

        if (cp1600->instr_tick)
        {
            cp1600->instr[pc]->opcode.breakpt.cycles = 8 + EXTRA_IF_R6R7(reg0);
            return CYC_MAX;
        }
    }

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_HLT      (const instr_t *instr, cp1600_t *cp1600)
{
    const uint32_t pc = cp1600->r[7];
    UNUSED(instr);

    if (debug_fault_detected != DEBUG_HLT_INSTR)
    {
        jzp_printf("\nHALT!\nPC: $%.4X   "
                   "Instr count: %" U64_FMT "   Cycles: %" U64_FMT "\n\n",
                    pc, cp1600->tot_instr, cp1600->tot_cycle);
    }

    debug_fault_detected = DEBUG_HLT_INSTR;
    cp1600->instr[pc]->opcode.breakpt.cycles = 4;

    cp1600->intr = 0;
    return cp1600->instr_tick ? CYC_MAX : 4;
}

int fn_TCI      (const instr_t *instr, cp1600_t *cp1600)
{
    /* XXX: Uhm... NOP for now? */
    cp1600->r[7]++;
    cp1600->intr = 0;
    UNUSED(instr);

    return 4;
}

int fn_SDBD     (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7]++;
    cp1600->D = 2;
    cp1600->intr = 0;
    UNUSED(instr);
    return 4;
}


int fn_EIS      (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7]++;
    cp1600->I = 1;
    cp1600->intr = 0;
    UNUSED(instr);
    first_dis = last_dis = -1;
    return 4;
}

int fn_DIS      (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7]++;
    if (cp1600->I) first_dis = cp1600->r[7];
    last_dis = cp1600->r[7];
    cp1600->I = 0;
    cp1600->intr = 0;
    UNUSED(instr);
    return 4;
}

int fn_CLRC     (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7]++;
    cp1600->C = 0;
    cp1600->intr = 0;
    UNUSED(instr);
    return 4;
}

int fn_SETC     (const instr_t *instr, cp1600_t *cp1600)
{
    cp1600->r[7]++;
    cp1600->C = 1;
    cp1600->intr = 0;
    UNUSED(instr);
    return 4;
}

int fn_INCR_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];
    r1 = 1;

    EXEC_SZ(r0,r1,r2,+,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 6 + EXTRA_IF_R6R7(reg0);
}

int fn_DECR_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    cp1600->r[7]++;

    r0 = 1;
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    EXEC_SZ(r0,r1,r2,-,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 6 + EXTRA_IF_R6R7(reg0);
}

int fn_COMR_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    cp1600->r[7]++;

    r0 = 0xFFFF;
    r1 = cp1600->r[instr->opcode.decoded.reg0];

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 6 + EXTRA_IF_R6R7(reg0);
}

int fn_NEGR_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0, r2;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    SUB_SZOC(r0,0,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 6 + EXTRA_IF_R6R7(reg0);
}

int fn_ADCR_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];
    r1 = cp1600->C;

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg0] = r2;

    return 6 + EXTRA_IF_R6R7(reg0);
}

int fn_RSWD_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    cp1600->S = !!(r0 & 0x80);
    cp1600->Z = !!(r0 & 0x40);
    cp1600->O = !!(r0 & 0x20);
    cp1600->C = !!(r0 & 0x10);

    cp1600->intr = 0;
    return 6;
}

int fn_GSWD_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;
    cp1600->r[7]++;

    r0 = ((cp1600->S << 7) | (cp1600->Z << 6) |
          (cp1600->O << 5) | (cp1600->C << 4))&0xF0;
    r0 = r0 | (r0 << 8);

    cp1600->r[instr->opcode.decoded.reg0] = r0;

    cp1600->intr = 0;
    return 6;
}


int fn_MOV_rr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,rp;
    int reg0 = instr->opcode.decoded.reg0;
    int reg1 = instr->opcode.decoded.reg1;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[reg0];
    cp1600->r[reg1] = r0;
    cp1600->r[7] = rp;

    cp1600->S = !!(r0 & 0x8000);
    cp1600->Z = !(r0);

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_TST_rr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0, rp;
    int reg0 = instr->opcode.decoded.reg0;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[reg0] + (reg0 == 7);
    cp1600->r[7] = rp;

    cp1600->S = !!(r0 & 0x8000);
    cp1600->Z = !(r0);

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_ADD_rr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2,rp;
    int reg0 = instr->opcode.decoded.reg0;
    int reg1 = instr->opcode.decoded.reg1;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[reg0];
    r1 = cp1600->r[reg1];
    cp1600->r[7] = rp;

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_SUB_rr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2,rp;
    int reg0 = instr->opcode.decoded.reg0;
    int reg1 = instr->opcode.decoded.reg1;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[reg0];
    r1 = cp1600->r[reg1];
    cp1600->r[7] = rp;

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_CMP_rr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2,rp;
    int reg0 = instr->opcode.decoded.reg0;
    int reg1 = instr->opcode.decoded.reg1;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[reg0];
    r1 = cp1600->r[reg1];
    cp1600->r[7] = rp;

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_AND_rr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2,rp;
    int reg0 = instr->opcode.decoded.reg0;
    int reg1 = instr->opcode.decoded.reg1;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[reg0];
    r1 = cp1600->r[reg1];
    cp1600->r[7] = rp;

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_XOR_rr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2,rp;
    int reg0 = instr->opcode.decoded.reg0;
    int reg1 = instr->opcode.decoded.reg1;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[reg0];
    r1 = cp1600->r[reg1];
    cp1600->r[7] = rp;

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_MOV_pr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;
    int reg1 = instr->opcode.decoded.reg1;

    r0 = cp1600->r[7] + 1;
    cp1600->r[reg1] = r0;
    cp1600->r[7] = r0;

    cp1600->S = !!(r0 & 0x8000);
    cp1600->Z = !(r0);

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_ADD_pr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int reg1 = instr->opcode.decoded.reg1;

    r0 = cp1600->r[7] + 1;
    r1 = cp1600->r[reg1];
    cp1600->r[7] = r0;

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_SUB_pr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int reg1 = instr->opcode.decoded.reg1;

    r0 = cp1600->r[7] + 1;
    r1 = cp1600->r[reg1];
    cp1600->r[7] = r0;

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_CMP_pr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int reg1 = instr->opcode.decoded.reg1;

    r0 = cp1600->r[7] + 1;
    r1 = cp1600->r[reg1];
    cp1600->r[7] = r0;

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_AND_pr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int reg1 = instr->opcode.decoded.reg1;

    r0 = cp1600->r[7] + 1;
    r1 = cp1600->r[reg1];
    cp1600->r[7] = r0;

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_XOR_pr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    int reg1 = instr->opcode.decoded.reg1;

    r0 = cp1600->r[7] + 1;
    r1 = cp1600->r[reg1];
    cp1600->r[7] = r0;

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[reg1] = r2;

    return 6 + EXTRA_IF_R6R7(reg1);
}

int fn_MOV_rp   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0;
    int reg0 = instr->opcode.decoded.reg0;

    r0 = cp1600->r[reg0];
    cp1600->r[7] = r0;

    cp1600->S = !!(r0 & 0x8000);
    cp1600->Z = !(r0);

    return 7;
}

int fn_ADD_rp   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    r0 = cp1600->r[instr->opcode.decoded.reg0];
    r1 = cp1600->r[7] + 1;

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[7] = r2;

    return 7;
}

int fn_SUB_rp   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    r0 = cp1600->r[instr->opcode.decoded.reg0];
    r1 = cp1600->r[7] + 1;

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[7] = r2;

    return 7;
}

int fn_CMP_rp   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2,rp;

    rp = cp1600->r[7] + 1;
    r0 = cp1600->r[instr->opcode.decoded.reg0];
    r1 = cp1600->r[7] + 1;
    cp1600->r[7] = rp;

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);

    return 7;
}

int fn_AND_rp   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    r0 = cp1600->r[instr->opcode.decoded.reg0];
    r1 = cp1600->r[7] + 1;

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[7] = r2;

    return 7;
}

int fn_XOR_rp   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;

    r0 = cp1600->r[instr->opcode.decoded.reg0];
    r1 = cp1600->r[7] + 1;

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[7] = r2;

    return 7;
}


int fn_SWAP1_r  (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = (0xFF & (r0 >> 8)) | ((0xFF & r0) << 8);

    cp1600->Z = !r0;
    cp1600->S = !!(r0 & 0x8000);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}

int fn_SLL1_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = r0 << 1;

    cp1600->S = !!(r0 & 0x4000);
    cp1600->Z = !r1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}

int fn_SLLC1_r  (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = r0 << 1;

    cp1600->Z = !r1;
    cp1600->C = !!(r0 & 0x8000);
    cp1600->S = !!(r0 & 0x4000);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}

int fn_RLC1_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = (r0 << 1) | cp1600->C;

    cp1600->Z = !r1;
    cp1600->C = !!(r0 & 0x8000);
    cp1600->S = !!(r0 & 0x4000);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}


int fn_SLR1_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = 0x7FFF & (r0 >> 1);

    cp1600->S = !!(r0 & 0x0100);
    cp1600->Z = !r1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}

int fn_RRC1_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = (0x7FFF & (r0 >> 1)) | (cp1600->C << 15);

    cp1600->S = !!(r0 & 0x0100);
    cp1600->Z = !r1;
    cp1600->C = r0 & 1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}

int fn_SAR1_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = (r0 >> 1) | (r0 & 0x8000);

    cp1600->S = !!(r0 & 0x0100);
    cp1600->Z = !r1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}

int fn_SARC1_r  (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = (r0 >> 1) | (r0 & 0x8000);

    cp1600->S = !!(r0 & 0x0100);
    cp1600->Z = !r1;
    cp1600->C = r0 & 1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 6;
}

int fn_SWAP2_r  (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0, r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0] & 0xFF;
    r1 = r0 | (r0 << 8);

    cp1600->S = !!(r0 & 0x0080);
    cp1600->Z = !r1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}

int fn_SLL2_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = r0 << 2;

    cp1600->Z = !r1;
    cp1600->S = !!(r0 & 0x2000);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}

int fn_SLLC2_r  (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = r0 << 2;

    cp1600->Z = !r1;
    cp1600->C = !!(r0 & 0x8000);
    cp1600->O = !!(r0 & 0x4000);
    cp1600->S = !!(r0 & 0x2000);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}

int fn_RLC2_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = (r0 << 2) | (cp1600->C << 1) | cp1600->O;

    cp1600->Z = !r1;
    cp1600->C = !!(r0 & 0x8000);
    cp1600->O = !!(r0 & 0x4000);
    cp1600->S = !!(r0 & 0x2000);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}

int fn_SLR2_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = 0x3FFF & (r0 >> 2);

    cp1600->S = !!(r0 & 0x0200);
    cp1600->Z = !r1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}

int fn_RRC2_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    r1 = (0x3FFF & (r0 >> 2)) | (cp1600->C << 14) | (cp1600->O << 15);

    cp1600->S = !!(r0 & 0x0200);
    cp1600->Z = !r1;
    cp1600->C = r0 & 1;
    cp1600->O = !!(r0 & 2);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}

int fn_SAR2_r   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,s;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    s  = r0 & 0x8000;
    r1 = (r0 >> 2) + s + (s >> 1);

    cp1600->S = !!(r0 & 0x0200);
    cp1600->Z = !r1;

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}

int fn_SARC2_r  (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,s;
    cp1600->r[7]++;

    r0 = cp1600->r[instr->opcode.decoded.reg0];

    s  = r0 & 0x8000;
    r1 = (r0 >> 2) + s + (s >> 1);

    cp1600->S = !!(r0 & 0x0200);
    cp1600->Z = !r1;
    cp1600->C = r0 & 1;
    cp1600->O = !!(r0 & 2);

    cp1600->r[instr->opcode.decoded.reg0] = r1;
    cp1600->intr = 0;

    return 8;
}


int fn_MVO_mr   (const instr_t *instr, cp1600_t *cp1600)
{
    int d = cp1600->D;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    uint16_t data;

    cp1600->r[7]++;

    data = cp1600->r[instr->opcode.decoded.reg1];
    data = handle_atomic(instr, cp1600, addr, data);

    if (d)
    {
        jzp_printf("WARNING: MVO@ w/ SDBD prefix @ %.4X\n", instr->address);
        /*
        CP1600_WR(cp1600, addr, data & 0xFF);
        CP1600_WR(cp1600, addr, (data >> 8) & 0xFF);
        */
        CP1600_WR(cp1600, addr, data);
    } else
    {
        CP1600_WR(cp1600, addr, data);
    }

    cp1600->intr = 0;
    return 9;
}

int fn_MVI_mr   (const instr_t *instr, cp1600_t *cp1600)
{
    int d = cp1600->D;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    uint16_t r0;

    cp1600->r[7]++;

    r0 = CP1600_RD(cp1600, addr);
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8;
    }

    cp1600->r[instr->opcode.decoded.reg1] = r0;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_ADD_mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8;
    }

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_SUB_mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8;
    }

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_CMP_mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8;
    }

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);   /* r2's value is discarded */

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_AND_mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8;
    }

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_XOR_mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;

    r0  = CP1600_RD(cp1600, addr);
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8;
    }

    r1 = cp1600->r[instr->opcode.decoded.reg1];

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

#define R0R1CHK(x) if (instr->opcode.decoded.reg0==instr->opcode.decoded.reg1 && d) jzp_printf("WARNING: " #x " w/ op1==op2 and SDBD @ %.4X\n", instr->address);


int fn_MVO_Mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    uint16_t data;
    int d = cp1600->D;

    cp1600->r[7]++;
R0R1CHK(MVO@)

    data = cp1600->r[instr->opcode.decoded.reg1];
    data = handle_atomic(instr, cp1600, addr, data);

    if (d)
        jzp_printf("WARNING: MVO@ w/ SDBD prefix @ %.4X\n", instr->address);

    CP1600_WR(cp1600, addr, data); addr++;

    cp1600->r[instr->opcode.decoded.reg0] = addr;
    cp1600->intr = 0;
    return 9;
}

int fn_MVI_Mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    uint16_t r0 = 0;
    int d = cp1600->D;

    cp1600->r[7]++;
R0R1CHK(MVI@)
    r0 = CP1600_RD(cp1600, addr); addr++;
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8; addr++;
    }

    cp1600->r[instr->opcode.decoded.reg0] = addr;
    cp1600->r[instr->opcode.decoded.reg1] = r0;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_ADD_Mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;
R0R1CHK(ADD@)
    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr); addr++;
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8; addr++;
    }

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;
    cp1600->r[instr->opcode.decoded.reg0] = addr;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_SUB_Mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;
R0R1CHK(SUB@)
    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr); addr++;
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8; addr++;
    }

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;
    cp1600->r[instr->opcode.decoded.reg0] = addr;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_CMP_Mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;
R0R1CHK(CMP@)
    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr); addr++;
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8; addr++;
    }

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);   /* r2's value is discarded */

    cp1600->r[instr->opcode.decoded.reg0] = addr;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_AND_Mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;
R0R1CHK(AND@)
    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr); addr++;
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8; addr++;
    }

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;
    cp1600->r[instr->opcode.decoded.reg0] = addr;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}

int fn_XOR_Mr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    int d = cp1600->D;
    cp1600->r[7]++;
R0R1CHK(XOR@)
    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr); addr++;
    if (d)
    {
        r0 &= 0xFF;
        r0 |= CP1600_RD(cp1600, addr) << 8; addr++;
    }

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;
    cp1600->r[instr->opcode.decoded.reg0] = addr;

    return (d ? 10 : 8) + EXTRA_IF_R6R7(reg0);
}


int fn_MVO_Nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    uint16_t data;

    cp1600->r[7]++;

    data = cp1600->r[instr->opcode.decoded.reg1];
    data = handle_atomic(instr, cp1600, addr, data);

    CP1600_WR(cp1600, addr, data); addr++;

    cp1600->r[instr->opcode.decoded.reg0] = addr;
    cp1600->intr = 0;
    return 9;
}

int fn_MVI_Nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0]++;
    uint16_t r0 = 0;

    cp1600->r[7]++;

    r0 = CP1600_RD(cp1600, addr);

    cp1600->r[instr->opcode.decoded.reg1] = r0;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_ADD_Nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0]++;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_SUB_Nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0]++;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_CMP_Nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0]++;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);   /* r2's value is discarded */

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_AND_Nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0]++;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_XOR_Nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0]++;
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_MVO_nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    uint16_t data;

    cp1600->r[7]++;

    data = cp1600->r[instr->opcode.decoded.reg1];
    data = handle_atomic(instr, cp1600, addr, data);
    CP1600_WR(cp1600, addr, data);

    cp1600->intr = 0;
    return 9;
}

int fn_MVI_nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    uint16_t r0 = 0;

    cp1600->r[7]++;

    r0 = CP1600_RD(cp1600, addr);

    cp1600->r[instr->opcode.decoded.reg1] = r0;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_ADD_nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_SUB_nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_CMP_nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);   /* r2's value is discarded */

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_AND_nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}

int fn_XOR_nr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;

    r1 = cp1600->r[instr->opcode.decoded.reg1];
    r0 = CP1600_RD(cp1600, addr);

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 8 + EXTRA_IF_R6R7(reg0);
}


int fn_MVI_Sr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0] - 1;
    uint16_t r0 = 0;

    cp1600->r[7]++;

    r0  = CP1600_RD(cp1600, addr);
    cp1600->r[instr->opcode.decoded.reg0] = addr;
    cp1600->r[instr->opcode.decoded.reg1] = r0;

    return 11 + EXTRA_IF_R6R7(reg0);
}

int fn_ADD_Sr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;


    r0  = CP1600_RD(cp1600, addr - 1);
    cp1600->r[instr->opcode.decoded.reg0]--;
    r1 = cp1600->r[instr->opcode.decoded.reg1];

    ADD_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 11 + EXTRA_IF_R6R7(reg0);
}

int fn_SUB_Sr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;


    r0  = CP1600_RD(cp1600, addr - 1);
    cp1600->r[instr->opcode.decoded.reg0]--;
    r1 = cp1600->r[instr->opcode.decoded.reg1];

    SUB_SZOC(r0,r1,r2,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 11 + EXTRA_IF_R6R7(reg0);
}

int fn_CMP_Sr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;


    r0  = CP1600_RD(cp1600, addr - 1);
    cp1600->r[instr->opcode.decoded.reg0]--;
    r1 = cp1600->r[instr->opcode.decoded.reg1];

    SUB_SZOC(r0,r1,r2,cp1600);

    UNUSED(r2);

    return 11 + EXTRA_IF_R6R7(reg0);
}

int fn_AND_Sr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;


    r0  = CP1600_RD(cp1600, addr - 1);
    cp1600->r[instr->opcode.decoded.reg0]--;
    r1 = cp1600->r[instr->opcode.decoded.reg1];

    EXEC_SZ(r0,r1,r2,&,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 11 + EXTRA_IF_R6R7(reg0);
}

int fn_XOR_Sr   (const instr_t *instr, cp1600_t *cp1600)
{
    uint16_t r0,r1,r2;
    uint16_t addr = cp1600->r[instr->opcode.decoded.reg0];
    cp1600->r[7]++;


    r0  = CP1600_RD(cp1600, addr - 1);
    cp1600->r[instr->opcode.decoded.reg0]--;
    r1 = cp1600->r[instr->opcode.decoded.reg1];

    EXEC_SZ(r0,r1,r2,^,cp1600);

    cp1600->r[instr->opcode.decoded.reg1] = r2;

    return 11 + EXTRA_IF_R6R7(reg0);
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
