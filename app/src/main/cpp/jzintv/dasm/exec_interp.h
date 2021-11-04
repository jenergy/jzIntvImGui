/* ======================================================================== */
/*  DIS-1600  Advanced(?) CP-1600 Disassembler.                             */
/*  By Joseph Zbiciak                                                       */
/* ------------------------------------------------------------------------ */
/*                   Copyright (c) 2006, Joseph Zbiciak                     */
/* ======================================================================== */

#ifndef EXEC_DECODE_H_
#define EXEC_DECODE_H_ 1


/* ======================================================================== */
/*  SETUP_EXEC_ROUTINE_SYM -- Mark down EXEC routines we know.              */
/* ======================================================================== */
void setup_exec_routine_sym(void);


/* ======================================================================== */
/* ======================================================================== */
/*  EXEC DECODERS                                                           */
/* ======================================================================== */
/* ======================================================================== */

extern uint32_t used_mob_pics[ 8];
extern uint32_t used_gfx_pics[16];

/* ======================================================================== */
/*  DECODE_GRAM_INIT                                                        */
/* ======================================================================== */
int decode_gram_init(uint32_t addr);

/* ======================================================================== */
/*  DECODE_TIMER_TABLE   -- Decode list of periodically-executed functions  */
/* ======================================================================== */
int decode_timer_table(uint32_t addr);


/* ======================================================================== */
/*  MARK_CART_HEADER_PRE -- Mark the cartridge header as known-data.        */
/*                          These are the steps we do "early."  The post    */
/*                          routine goes and backfills based on final info  */
/* ======================================================================== */
int mark_cart_header_pre(void);


/* ======================================================================== */
/*  DECODE_GFX_LIST                                                         */
/* ======================================================================== */
int decode_gfx_list(uint32_t addr);


/* ======================================================================== */
/*  MARK_CART_HEADER_POST -- Finish marking up data spec'd by header.       */
/* ======================================================================== */
int mark_cart_header_post(void);

/* ======================================================================== */
/*  DECODE_EXEC_MUSIC -- Try to decode music played by EXEC music engine    */
/* ======================================================================== */
int decode_exec_music(void);


/* ======================================================================== */
/*  DECODE_EXEC_SFX -- Try to decode music played by EXEC sound fx engine   */
/* ======================================================================== */
int decode_exec_sfx(void);

/* ======================================================================== */
/*  DECODE_PRINT_CALLS -- Scan for message-printing routines; Mark strings  */
/* ======================================================================== */
int decode_print_calls(void);

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
