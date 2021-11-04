/*
 * ============================================================================
 *  Title:    Demo Recorder
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements a "demo recorder", which records updates to GRAM,
 *  BACKTAB and the PSG to a file.  Currently it does not record Intellivoice.
 *
 *  The output file can be fed through a post-processor, which then reformats
 *  and compresses the data, for use in a demo-player on a real Intellivision.
 *
 *  Rather than snoop the bus for everything or implement a "tickable"
 *  interface, the demo recorder gets ticked by the STIC directly, and the
 *  STIC hands the demo recorder its current vision of the STIC control
 *  registers, BACKTAB and GRAM.  The demo recorder then only has to snoop
 *  the PSG registers.
 *
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "gfx/gfx.h"
#include "snd/snd.h"
#include "periph/periph.h"
#include "demo/demo.h"
#include "ay8910/ay8910.h"
#include "stic/stic.h"

/* ======================================================================== */
/*  SIGBIT_PSG   -- Significant bits in PSG  registers.                     */
/*  SIGBIT_STIC  -- Significant bits in STIC registers.                     */
/*  STIC_REGS    -- List of STIC register numbers to examine.               */
/* ======================================================================== */
LOCAL const uint16_t sigbit_psg [14]   =
{
    0x00FF, 0x00FF, 0x00FF, 0x00FF, /* Lower 8 bits of channel periods.     */
    0x000F, 0x000F, 0x000F,         /* Upper 4 bits of channel periods.     */
    0x00FF,                         /* Upper 8 bits of envelope period.     */
    0x003F,                         /* Channel enables.                     */
    0x001F,                         /* Noise period.                        */
    0x000F,                         /* Envelope characteristics             */
    0x003F, 0x003F, 0x003F          /* Volume controls.                     */
};

LOCAL const uint16_t sigbit_stic[0x40] =
{
    /* MOB X Registers                                  0x00 - 0x07 */
    0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF,

    /* MOB Y Registers                                  0x08 - 0x0F */
    0x0FFF, 0x0FFF, 0x0FFF, 0x0FFF, 0x0FFF, 0x0FFF, 0x0FFF, 0x0FFF,

    /* MOB A Registers                                  0x10 - 0x17 */
    0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF,

    /* MOB C Registers                                  0x18 - 0x1F */
    0x03FE, 0x03FD, 0x03FB, 0x03F7, 0x03EF, 0x03DF, 0x03BF, 0x037F,

    /* Display enable, Mode select                      0x20 - 0x21 */
    0x0000, 0x0000,

    /* Unimplemented registers                          0x22 - 0x27 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    /* Color stack, border color                        0x28 - 0x2C */
    0x000F, 0x000F, 0x000F, 0x000F, 0x000F,

    /* Unimplemented registers                          0x2D - 0x2F */
    0x0000, 0x0000, 0x0000,

    /* Horiz delay, vertical delay, border extension    0x30 - 0x32 */
    0x0007, 0x0007, 0x0003,

    /* Unimplemented registers                          0x33 - 0x3F */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

LOCAL const int stic_regs[32] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x30, 0x31, 0x32,
};


/* ======================================================================== */
/*  DEMO_BUF     -- Buffer we construct each frame in.                      */
/*                                                                          */
/*  Frame format:                                                           */
/*                                                                          */
/*      4 bytes     0x2A3A4A5A  Frame header                                */
/*      4 bytes     Bitmap of changed STIC registers                        */
/*      8 bytes     Bitmap of changed GRAM cards                            */
/*      30 bytes    Bitmap of changed BTAB cards                            */
/*      2 bytes     Bitmap of changed PSG0 registers                        */
/*      2 bytes     Bitmap of changed PSG1 registers                        */
/*      N bytes     STIC register values (2 bytes each)                     */
/*      N bytes     GRAM tiles (8 bytes each)                               */
/*      N bytes     BTAB cards (2 bytes each)                               */
/*      N bytes     PSG0 registers (1 byte each)                            */
/*      N bytes     PSG1 registers (1 byte each)                            */
/*                                                                          */
/* ======================================================================== */
LOCAL uint8_t demo_buf[54 + 32*2 + 64*8 + 240*2 + 16 + 16];

#define EMIT_32(buf, word)  do {\
                                buf[0]   = ((word) >>  0) & 0xFF;           \
                                buf[1]   = ((word) >>  8) & 0xFF;           \
                                buf[2]   = ((word) >> 16) & 0xFF;           \
                                buf[3]   = ((word) >> 24) & 0xFF;           \
                                buf += 4;                                   \
                            } while (0)

#define EMIT_16(buf, word)  do {\
                                buf[0]   = ((word) >>  0) & 0xFF;           \
                                buf[1]   = ((word) >>  8) & 0xFF;           \
                                buf += 2;                                   \
                            } while (0)

#define EMIT_8(buf, word)   *(buf)++ = word;


/* ======================================================================== */
/*  DEMO_TICK    -- Called from STIC_TICK at the start of VBlank.           */
/* ======================================================================== */
void demo_tick
(
    demo_t      *demo,
    stic_t      *stic
)
{
    int i, j, c;
    uint32_t stic_chg    = { 0 };
    uint32_t gram_chg[2] = { 0, 0 };
    uint32_t btab_chg[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint32_t psg0_chg    = { 0 };
    uint32_t psg1_chg    = { 0 };
    uint32_t mask;
    uint8_t  *buf;


    /* -------------------------------------------------------------------- */
    /*  Scan the STIC registers and look for changes.  Ignore "dontcares".  */
    /*  Note:  We encode FGBG/CSTK and VidEn in extra bits of the border    */
    /*  extension register, 0x32.                                           */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 32; i++)
    {
        int reg = stic_regs[i];
        uint32_t stic_reg = stic->raw[reg] & sigbit_stic[reg];

        if (i == 31)    /* last reg is 0x32 */
        {
            stic_reg |= stic->vid_enable ? 4 : 0;
            stic_reg |= stic->mode << 3;
        }

        if (stic_reg != demo->stic[i])
        {
            stic_chg |= 1 << i;
            demo->stic[i] = stic_reg;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Scan the GRAM image and find out what changed.                      */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 512; i++)
    {
        if (stic->gmem[0x800 + i] != demo->gram[i])
        {
            c = i >> 3;
            gram_chg[c >> 5] |= 1 << (c & 31);
        }
    }
    memcpy(demo->gram, stic->gmem + 0x800, 512 * sizeof(stic->gmem[0]));

    /* -------------------------------------------------------------------- */
    /*  Scan the BACKTAB and find what's changed.  Ignore "dontcare" bits.  */
    /* -------------------------------------------------------------------- */
    mask = stic->mode ? 0x3FFF : 0x39FF;

    for (i = 0; i < 240; i++)
    {
        uint32_t demo_btab = demo->btab[i];
        uint32_t stic_btab = stic->btab[i];

        stic_btab &= (stic_btab & 0x800) ? mask : 0x3FFF;

        if (stic_btab != demo_btab)
        {
            btab_chg[i >> 5] |= 1 << (i & 31);
            demo->btab[i] = stic_btab;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Scan the PSG registers and see what's changed.                      */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 14; i++)
    {
        uint32_t psg0_reg = demo->psg0 ? demo->psg0->reg[i] & sigbit_psg[i] : 0;
        uint32_t psg1_reg = demo->psg1 ? demo->psg1->reg[i] & sigbit_psg[i] : 0;

        if (demo->psg0_reg[i] != psg0_reg)
        {
            demo->psg0_reg[i] =  psg0_reg;
            psg0_chg |= 1 << i;
        }

        if (demo->psg1_reg[i] != psg1_reg)
        {
            demo->psg1_reg[i] =  psg1_reg;
            psg1_chg |= 1 << i;
        }
    }

    if (demo->psg0 && demo->psg0->demo_env_hit)
    {
        demo->psg0->demo_env_hit = 0;
        psg0_chg |= 1 << 10;
    }

    if (demo->psg1 && demo->psg1->demo_env_hit)
    {
        demo->psg1->demo_env_hit = 0;
        psg1_chg |= 1 << 10;
    }

    /* -------------------------------------------------------------------- */
    /*  Encode the frame.                                                   */
    /*  Frame format:                                                       */
    /*                                                                      */
    /*      4 bytes     0x2A3A4A5A  Frame header                            */
    /*      4 bytes     Bitmap of changed STIC registers                    */
    /*      8 bytes     Bitmap of changed GRAM cards                        */
    /*      30 bytes    Bitmap of changed BTAB cards                        */
    /*      2 bytes     Bitmap of changed PSG0 registers                    */
    /*      2 bytes     Bitmap of changed PSG1 registers                    */
    /*                                                                      */
    /*      N bytes     STIC register values (2 bytes each)                 */
    /*      N bytes     GRAM tiles (8 bytes each)                           */
    /*      N bytes     BTAB cards (2 bytes each)                           */
    /*      N bytes     PSG0 registers (1 byte each)                        */
    /*      N bytes     PSG1 registers (1 byte each)                        */
    /* -------------------------------------------------------------------- */
    buf = demo_buf;

    EMIT_32(buf, 0x2A3A4A5A);
    EMIT_32(buf, stic_chg);
    EMIT_32(buf, gram_chg[0]);
    EMIT_32(buf, gram_chg[1]);
    for (i = 0; i < 7; i++)
        EMIT_32(buf, btab_chg[i]);
    EMIT_16(buf, btab_chg[7]);
    EMIT_16(buf, psg0_chg);
    EMIT_16(buf, psg1_chg);

    for (i = 0; i < 32; i++)
        if ((stic_chg >> i) & 1)
            EMIT_16(buf, demo->stic[i]);

    for (i = 0; i < 64; i++)
        if ((gram_chg[i >> 5] >> (i & 31)) & 1)
            for (j = 0; j < 8; j++)
                EMIT_8(buf, demo->gram[i*8 + j]);

    for (i = 0; i < 240; i++)
        if ((btab_chg[i >> 5] >> (i & 31)) & 1)
            EMIT_16(buf, demo->btab[i]);

    for (i = 0; i < 14; i++)
        if ((psg0_chg >> i) & 1)
            EMIT_8(buf, demo->psg0_reg[i]);

    for (i = 0; i < 14; i++)
        if ((psg1_chg >> i) & 1)
            EMIT_8(buf, demo->psg1_reg[i]);

    assert((size_t)(buf - demo_buf) <= sizeof(demo_buf));

    fwrite(demo_buf, 1, buf - demo_buf, demo->f);

    return;
}


/* ======================================================================== */
/*  DEMO_INIT    -- Initialize the demo recorder.                           */
/* ======================================================================== */
int demo_init
(
    demo_t      *demo,
    char        *demo_file,
    ay8910_t    *psg0,
    ay8910_t    *psg1
)
{
    memset(demo, 0, sizeof(*demo));

    if (!(demo->f = fopen(demo_file, "wb")))
    {
        perror("fopen()");
        fprintf(stderr, "Could not open demo file '%s' for writing.\n",
                demo_file);

        return -1;
    }

    demo->psg0 = psg0;
    demo->psg1 = psg1;

    return 0;
}

/* ======================================================================== */
/*  DEMO_DTOR    -- Shut down the demo recorder.                            */
/* ======================================================================== */
void demo_dtor(demo_t *demo)
{
    if (demo && demo->f)
        fclose(demo->f);
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
/*                 Copyright (c) 2005-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
