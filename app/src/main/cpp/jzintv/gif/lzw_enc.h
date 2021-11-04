/* ======================================================================== */
/*  LZW Encode                                                              */
/*                                                                          */
/*  This code compresses an input buffer using LZW compression, as defined  */
/*  by the GIF standard.  This includes dividing the compressed output      */
/*  into 256-byte blocks.                                                   */
/*                                                                          */
/*  My data structure is entirely uncreative.  I use an N-way tree to       */
/*  represent the current code table.  It's dirt simple to implement, but   */
/*  it's a memory pig.  Since the longest code is 12 bits, I use indices    */
/*  instead of pointers, and use a static table of codes.                   */
/* ======================================================================== */

#ifndef LZW_ENC_H_
#define LZW_ENC_H_ 1

int lzw_encode (const uint8_t *i_buf, uint8_t *o_buf, int i_len, int max_o_len);
int lzw_encode2(const uint8_t *i_buf, const uint8_t *i_buf_alt,
                      uint8_t *o_buf, int i_len, int max_o_len);

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
/*                   Copyright (c) 2005, Joseph Zbiciak                     */
/* ======================================================================== */
