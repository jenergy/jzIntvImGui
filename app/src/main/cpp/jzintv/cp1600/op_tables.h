
/*
 * ============================================================================
 *  Execute function pointer lookup tables:
 *
 *  DEC_FORMAT          -- Decode format for entire instruction space
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
 *
 * ============================================================================
 */

#ifndef OP_TABLES_H_
#define OP_TABLES_H_

#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/op_decode.h"
#include "cp1600/op_exec.h"

extern const uint8_t dec_format[];
extern cp1600_ins_t *const fn_ind_2op[];
extern cp1600_ins_t *const fn_dir_2op[];
extern cp1600_ins_t *const fn_imm_2op[];
extern cp1600_ins_t *const fn_cond_br[];
extern cp1600_ins_t *const fn_reg_2op[];
extern cp1600_ins_t *const fn_rot_1op[];
extern cp1600_ins_t *const fn_reg_1op[];
extern cp1600_ins_t *const fn_impl_1op_a[];
extern cp1600_ins_t *const fn_impl_1op_b[];

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
