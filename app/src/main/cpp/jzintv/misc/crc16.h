/* ======================================================================== */
/*  CRC-16 routines                                     J. Zbiciak, 2001    */
/* ------------------------------------------------------------------------ */
/*  The contents of this file are hereby released into the public domain.   */
/*  This does not affect the rest of the program code in jzIntv, which      */
/*  remains under the GPL except where specific files state differently,    */
/*  such as this one.                                                       */
/*                                                                          */
/*  Programs are free to use the CRC-16 functions contained in this file    */
/*  for whatever purpose they desire, with no strings attached.             */
/* ======================================================================== */


#ifndef CRC_16_H_
#define CRC_16_H_ 1

/* ======================================================================== */
/*  CRC16_TBL    -- Lookup table used for the CRC-16 code.                  */
/* ======================================================================== */
extern const uint16_t crc16_tbl[256];

/* ======================================================================== */
/*  CRC16_UPDATE -- Updates a 16-bit CRC using the lookup table above.      */
/*                  Note:  The 16-bit CRC is set up as a left-shifting      */
/*                  CRC with no inversions.                                 */
/*                                                                          */
/*                  All-caps version is a macro for stuff that can use it.  */
/* ======================================================================== */
uint16_t crc16_update(uint16_t crc, uint8_t data);
#define CRC16_UPDATE(crc, d) (((crc) << 8) ^ crc16_tbl[((crc) >> 8) ^ (d)])

/* ======================================================================== */
/*  CRC16_BLOCK  -- Updates a 16-bit CRC on a block of 8-bit data.          */
/*                  Note:  The 16-bit CRC is set up as a left-shifting      */
/*                  CRC with no inversions.                                 */
/* ======================================================================== */
uint16_t crc16_block(uint16_t crc, const uint8_t *data, int len);

#endif
/* ======================================================================== */
/*     This specific file is placed in the public domain by its author,     */
/*                              Joseph Zbiciak.                             */
/* ======================================================================== */
