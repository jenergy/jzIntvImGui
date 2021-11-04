/*
 * ============================================================================
 *  OP_EXEC:        Execute functions for the CP-1610 instructions
 *
 *  Author:         J. Zbiciak
 *
 * ============================================================================
 *  fn_invalid      -- Executed when a decoder failure happens
 *  fn_XXXX_i       -- Immediate operand instructions (relative branch, jump)
 *  fn_XXXX_r       -- Implied, register 2-ops (eg. ADCR, COMR, etc.)
 *  fn_XXXX_ir      -- Immediate, register 2-ops, JSR
 *  fn_XXXX_rr      -- Register, register 2-ops
 *  fn_XXXX_rp      -- Register, PC 2-ops
 *  fn_XXXX_dr      -- Direct, register 2-ops
 *  fn_XXXX_mr      -- Indirect ("memory"), register 2-ops, non-incrementing
 *  fn_XXXX_Mr      -- Indirect ("memory"), register 2-ops, post-incrementing
 *  fn_XXXX_Sr      -- Indirect ("stack"), register 2-ops, pre-decrementing
 * ============================================================================
 */


#ifndef OP_EXEC_H_
#define OP_EXEC_H_

int fn_invalid  (const instr_t *instr, cp1600_t *cpu);
int fn_breakpt  (const instr_t *instr, cp1600_t *cpu);
int fn_BEXT_i   (const instr_t *instr, cp1600_t *cpu);
int fn_B_i      (const instr_t *instr, cp1600_t *cpu);
int fn_BC_i     (const instr_t *instr, cp1600_t *cpu);
int fn_BOV_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BPL_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BEQ_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BLT_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BLE_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BUSC_i   (const instr_t *instr, cp1600_t *cpu);
int fn_NOPP_i   (const instr_t *instr, cp1600_t *cpu);
int fn_BNC_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BNOV_i   (const instr_t *instr, cp1600_t *cpu);
int fn_BMI_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BNEQ_i   (const instr_t *instr, cp1600_t *cpu);
int fn_BGE_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BGT_i    (const instr_t *instr, cp1600_t *cpu);
int fn_BESC_i   (const instr_t *instr, cp1600_t *cpu);
int fn_SIN_i    (const instr_t *instr, cp1600_t *cpu);
int fn_NOP_i    (const instr_t *instr, cp1600_t *cpu);
int fn_J_i      (const instr_t *instr, cp1600_t *cpu);
int fn_JSR_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_dr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_dr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_dr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_dr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_dr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_dr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_dr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_ir   (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_Ir   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_Ir   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_Ir   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_Ir   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_Ir   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_Ir   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_Ir   (const instr_t *instr, cp1600_t *cpu);
int fn_HLT      (const instr_t *instr, cp1600_t *cpu);
int fn_SDBD     (const instr_t *instr, cp1600_t *cpu);
int fn_EIS      (const instr_t *instr, cp1600_t *cpu);
int fn_DIS      (const instr_t *instr, cp1600_t *cpu);
int fn_TCI      (const instr_t *instr, cp1600_t *cpu);
int fn_CLRC     (const instr_t *instr, cp1600_t *cpu);
int fn_SETC     (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_mr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_mr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_mr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_mr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_mr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_mr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_mr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_Mr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_Mr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_Mr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_Mr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_Mr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_Mr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_Mr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_Nr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_Nr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_Nr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_Nr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_Nr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_Nr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_Nr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_nr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_nr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_nr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_nr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_nr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_nr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_nr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVO_Sr   (const instr_t *instr, cp1600_t *cpu);
int fn_MVI_Sr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_Sr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_Sr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_Sr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_Sr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_Sr   (const instr_t *instr, cp1600_t *cpu);
int fn_INCR_r   (const instr_t *instr, cp1600_t *cpu);
int fn_DECR_r   (const instr_t *instr, cp1600_t *cpu);
int fn_COMR_r   (const instr_t *instr, cp1600_t *cpu);
int fn_NEGR_r   (const instr_t *instr, cp1600_t *cpu);
int fn_ADCR_r   (const instr_t *instr, cp1600_t *cpu);
int fn_GSWD_r   (const instr_t *instr, cp1600_t *cpu);
int fn_RSWD_r   (const instr_t *instr, cp1600_t *cpu);
int fn_TST_rr   (const instr_t *instr, cp1600_t *cpu);
int fn_MOV_rr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_rr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_rr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_rr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_rr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_rr   (const instr_t *instr, cp1600_t *cpu);
int fn_MOV_pr   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_pr   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_pr   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_pr   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_pr   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_pr   (const instr_t *instr, cp1600_t *cpu);
int fn_MOV_rp   (const instr_t *instr, cp1600_t *cpu);
int fn_ADD_rp   (const instr_t *instr, cp1600_t *cpu);
int fn_SUB_rp   (const instr_t *instr, cp1600_t *cpu);
int fn_CMP_rp   (const instr_t *instr, cp1600_t *cpu);
int fn_AND_rp   (const instr_t *instr, cp1600_t *cpu);
int fn_XOR_rp   (const instr_t *instr, cp1600_t *cpu);
int fn_SWAP1_r  (const instr_t *instr, cp1600_t *cpu);
int fn_SLL1_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SLLC1_r  (const instr_t *instr, cp1600_t *cpu);
int fn_RLC1_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SLR1_r   (const instr_t *instr, cp1600_t *cpu);
int fn_RRC1_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SAR1_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SARC1_r  (const instr_t *instr, cp1600_t *cpu);
int fn_SWAP2_r  (const instr_t *instr, cp1600_t *cpu);
int fn_SLL2_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SLLC2_r  (const instr_t *instr, cp1600_t *cpu);
int fn_RLC2_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SLR2_r   (const instr_t *instr, cp1600_t *cpu);
int fn_RRC2_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SAR2_r   (const instr_t *instr, cp1600_t *cpu);
int fn_SARC2_r  (const instr_t *instr, cp1600_t *cpu);

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
