/*
 * ============================================================================
 *  Title:    Jean-Luc Project support
 *  Author:   J. Zbiciak
 * ============================================================================
 */
#ifndef JLPSG_H_
#define JLPSG_H_

typedef struct jlp_t
{
    periph_t    periph;     /* Yup, this is a peripheral.  :-)              */
    uint16_t   *xreg;       /* Pointer to CPU X-Regs                        */
    uint16_t   *ram;        /* 16-bit RAM                                   */
    int         sleep;      /* Emulate "busy" time after a slot write.      */
    uint8_t    *sg_img;     /* Save game image                              */
    FILE       *sg_file;    /* Save-game filename                           */
    uint16_t    sg_start;   /* Initial flash row                            */
    uint16_t    sg_end;     /* Final flash row                              */
    uint32_t    sg_bytes;   /* Total size in bytes                          */
} jlp_t;


/* Approximate numbers for a 256K board and ~20Kw game. */
#define JLP_SG_OFS   (224)

/* ======================================================================== */
/*  JLP_INIT   -- Sets up JLP support                                       */
/* ======================================================================== */
int jlp_init
(
    jlp_t          *jlp,        /*  Structure to initialize.        */
    const char     *fname,      /*  Save-game file                  */
    uint16_t       *pc,         /*  Pointer to CPU program counter  */
    int             jlp_flags,  /*  Accel-enable, Flash-enable      */
    int             jlp_flash,  /*  Flash image size                */
    int             randomize   /*  Randomize memory on powerup.    */
);


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
/*                 Copyright (c) 2009-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
