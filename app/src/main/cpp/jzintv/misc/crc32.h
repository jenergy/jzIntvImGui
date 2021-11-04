/* ======================================================================== */
/*  CRC-32 routines                                     J. Zbiciak, 2001    */
/* ------------------------------------------------------------------------ */
/*  This code is compatible with the CRC-32 that is used by the Zip file    */
/*  compression standard.  To use this code for that purpose, initialize    */
/*  your CRC to 0xFFFFFFFF, and XOR it with 0xFFFFFFFF afterwards.          */
/* ------------------------------------------------------------------------ */
/*  The contents of this file are hereby released into the public domain.   */
/*  This does not affect the rest of the program code in jzIntv, which      */
/*  remains under the GPL except where specific files state differently,    */
/*  such as this one.                                                       */
/*                                                                          */
/*  Programs are free to use the CRC-32 functions contained in this file    */
/*  for whatever purpose they desire, with no strings attached.             */
/* ======================================================================== */

#ifndef CRC32_H_
#define CRC32_H_

/* ======================================================================== */
/*  CRC32_TBL    -- Lookup table used for the CRC-32 code.                  */
/* ======================================================================== */
extern uint32_t crc32_tbl[256];

/* ======================================================================== */
/*  CRC32_UPDATE -- Updates a 32-bit CRC using the lookup table above.      */
/*                  Note:  The 32-bit CRC is set up as a right-shifting     */
/*                  CRC with no inversions.                                 */
/*                                                                          */
/*                  All-caps version is a macro for stuff that can use it.  */
/* ======================================================================== */
uint32_t crc32_update(uint32_t crc, uint8_t data);
#define CRC32_UPDATE(c, d) ((c) >> 8 ^ crc32_tbl[((c) ^ (d)) & 0xFF])

/* ======================================================================== */
/*  CRC32_UPD16  -- Updates a 32-bit CRC using the lookup table above.      */
/*                  This function updates the CRC with a 16-bit value,      */
/*                  little-endian.                                          */
/* ======================================================================== */
uint32_t crc32_upd16(uint32_t crc, uint16_t data);

/* ======================================================================== */
/*  CRC32_UPD32  -- Updates a 32-bit CRC using the lookup table above.      */
/*                  This function updates the CRC with a 32-bit value,      */
/*                  little-endian.                                          */
/* ======================================================================== */
uint32_t crc32_upd32(uint32_t crc, uint32_t data);

/* ======================================================================== */
/*  CRC32_BLOCK  -- Updates a 32-bit CRC on a block of 8-bit data.          */
/*                  Note:  The 32-bit CRC is set up as a right-shifting     */
/*                  CRC with no inversions.                                 */
/* ======================================================================== */
uint32_t crc32_block(uint32_t crc, const uint8_t *data, int len);

#endif
/* ======================================================================== */
/*     This specific file is placed in the public domain by its author,     */
/*                              Joseph Zbiciak.                             */
/* ======================================================================== */
