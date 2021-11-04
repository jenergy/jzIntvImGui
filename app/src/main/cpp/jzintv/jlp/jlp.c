/*
 * ============================================================================
 *  Title:    Jean-Luc Project support
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module provides support for 3 JLP features:
 *
 *   -- 16-bit RAM from $8040 - $9F7F
 *   -- Save Game Arch V2 support at $8023 - $802F
 *   -- Random number generator at $9FFE
 *   -- CRC-16 accelerator at $9FFC / $9FFD (poly 0xAD52, right-shifting)
 *   -- Multiply / divide acceleration at $9F80 - $9F8F
 *      $9F8(0,1):  s16($9F80) x s16($9F81) -> s32($9F8F:$9F8E)
 *      $9F8(2,3):  s16($9F82) x u16($9F83) -> s32($9F8F:$9F8E)
 *      $9F8(4,5):  u16($9F84) x s16($9F85) -> s32($9F8F:$9F8E)
 *      $9F8(6,7):  u16($9F86) x u16($9F87) -> u32($9F8F:$9F8E)
 *      $9F8(8,9):  s16($9F88) / s16($9F89) -> quot($9F8E), rem($9F8F)
 *      $9F8(A,B):  u16($9F8A) / u16($9F8B) -> quot($9F8E), rem($9F8F)
 *   -- Extended registers at $9F90 - $9F97
 *
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "jlp/jlp.h"
#include "plat/plat_lib.h" /* rand_jz */

#ifdef BYTE_LE
# define    E(x) (x)
#else /* BYTE_BE */
# define    E(x) ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))
#endif

#define JLP_CRC_POLY (0xAD52)

static int jlp_accel_on_reset = 0;
int jlp_accel_on = 0;
int lto_isa_enabled = 0;

/* ======================================================================== */
/*  JLP_SG_VALIDATE_ARGS    -- Helper:  Make sure args are OK for fxn       */
/* ======================================================================== */
LOCAL int jlp_sg_validate_args(jlp_t *const jlp, const int check_addr)
{
    const uint32_t addr    = jlp->ram[0x25];
    const uint32_t row     = jlp->ram[0x26];
    const uint32_t min_row = jlp->sg_start;
    const uint32_t max_row = jlp->sg_end;

    const int have_flash = jlp->sg_img && jlp->sg_bytes;
    const int addr_ok = (check_addr == 0 || 
                         (addr >= 0x8040 && addr + 95 <= 0x9F7F));
    const int row_ok = (row >= min_row) && (row <= max_row);

    if (have_flash && addr_ok && row_ok)
        return 1;

    jzp_printf("JLP:  Invalid JLP command.  A=%.4X R=%.4X  [ %.4X, %.4X ]\n",
                addr, row, min_row, max_row);
    return 0;
}

/* ======================================================================== */
/*  JLP_SG_RAM_TO_FLASH     -- Copy 96 words from JLP RAM to Flash          */
/*  JLP_SG_FLASH_TO_RAM     -- Copy 96 words from Flash to JLP RAM          */
/*  JLP_SG_ERASE_SECTOR     -- Erase a 768 word flash sector to $FFFF       */
/* ======================================================================== */
LOCAL void jlp_sg_ram_to_flash(jlp_t *jlp)
{
    uint32_t addr = jlp->ram[0x25] - 0x8000;
    uint32_t row  = (jlp->ram[0x26] - jlp->ram[0x23]) * 192;
    int i, a;

    jlp->sleep = 400 + rand_jz() % 200;

    if (!jlp_sg_validate_args(jlp, 1)) return;

    jzp_printf("JLP:  RAM to Flash  A=%.4X R=%.4X\n",
                jlp->ram[0x25], jlp->ram[0x26]);

    /* Refuse to write if row isn't empty. */
    for (i = 0; i < 192; i++)
        if (jlp->sg_img[row + i] != 0xFF)
        {
            jzp_printf("JLP:  Dropped: Row not empty!\n");
            return;
        }

    /* Explicitly copy little-endian regardless of machine endian */
    for (i = a = 0; i < 192; i += 2, a++)
    {
        jlp->sg_img[row + i + 0] = jlp->ram[addr + a] & 0xFF;
        jlp->sg_img[row + i + 1] = jlp->ram[addr + a] >> 8;
    }

    if (jlp->sg_file)
    {
        fseek(jlp->sg_file, row, SEEK_SET);
        fwrite(&jlp->sg_img[row], 2, 96, jlp->sg_file);
        fflush(jlp->sg_file);
    }
}

LOCAL void jlp_sg_flash_to_ram(jlp_t *jlp)
{
    uint32_t addr = jlp->ram[0x25] - 0x8000;
    uint32_t row  = (jlp->ram[0x26] - jlp->ram[0x23]) * 192;
    int i, a;

    jlp->sleep = 10 + rand_jz() % 20;

    if (!jlp_sg_validate_args(jlp, 1)) return;

    jzp_printf("JLP:  Flash to RAM  A=%.4X R=%.4X\n",
                jlp->ram[0x25], jlp->ram[0x26]);

    /* Explicitly copy little-endian regardless of machine endian */
    for (i = a = 0; i < 192; i += 2, a++)
    {
        uint16_t lo = jlp->sg_img[row + i + 0];
        uint16_t hi = jlp->sg_img[row + i + 1];
        jlp->ram[addr + a] = lo | (hi << 8);
    }
}

LOCAL void jlp_sg_erase_sector(jlp_t *jlp)
{
    uint32_t row = ((jlp->ram[0x26] - jlp->ram[0x23]) & -8) * 192;

    jlp->sleep = 800 + rand_jz() % 200;

    if (!jlp_sg_validate_args(jlp, 0)) return;

    jzp_printf("JLP:  Erase Flash          R=%.4X\n", jlp->ram[0x26]);

    memset((void *)&jlp->sg_img[row], 0xFF, 192 * 8);

    if (jlp->sg_file)
    {
        fseek(jlp->sg_file, row, SEEK_SET);
        fwrite(&jlp->sg_img[row], 2, 96 * 8, jlp->sg_file);
        fflush(jlp->sg_file);
    }
}

/* ======================================================================== */
/*  JLP_CRC16    -- Do a CRC-16 update.                                     */
/* ======================================================================== */
LOCAL uint32_t jlp_crc16(uint32_t data, uint32_t crc)
{
    int i;

    crc ^= data;

    for (i = 0; i < 16; i++)
        crc = (crc >> 1) ^ (crc & 1 ? JLP_CRC_POLY : 0);

    return crc;
}

/* ======================================================================== */
/*  JLP_READ     -- Read from a JLP location                                */
/* ======================================================================== */
LOCAL uint32_t jlp_read(periph_t *const per, periph_t *ign,
                        uint32_t addr, uint32_t data)
{
    jlp_t *const jlp = PERIPH_AS(jlp_t, per);

    UNUSED(ign); UNUSED(data);

    if (jlp->sleep > 0) { jlp->sleep--; return ~0u;     }
    if (!jlp_accel_on)  { return ~0u;                   }
    if (addr >= 0x1F90 && addr <= 0x1F9F) { return jlp->xreg[addr & 0xF]; }
    if (addr == 0x1FFE) { return rand_jz() >> 16;       }

    return jlp->ram[addr];
}

/* ======================================================================== */
/*  JLP_PEEK     -- Same as JLP read, except it ignores jlp->sleep          */
/* ======================================================================== */
LOCAL uint32_t jlp_peek(periph_t *const per, periph_t *ign,
                        uint32_t addr, uint32_t data)
{
    jlp_t *const jlp = PERIPH_AS(jlp_t, per);

    UNUSED(ign);
    UNUSED(data);

    if (!jlp_accel_on)  { return ~0u;                   }
    if (addr >= 0x1F90 && addr <= 0x1F9F) { return jlp->xreg[addr & 0xF]; }
    if (addr == 0x1FFE) { return rand_jz() >> 16;       }

    return jlp->ram[addr];
}

/* ======================================================================== */
/*  JLP_WRITE    -- Write to JLP                                            */
/* ======================================================================== */
LOCAL void jlp_write(periph_t *const per, periph_t *ign,
                     uint32_t addr, uint32_t data)
{
    jlp_t *const jlp = PERIPH_AS(jlp_t, per);

    UNUSED(ign);

    if (jlp->sleep == 0 && !jlp_accel_on && addr == 0x33 && data == 0x4A5A)
    {
        jlp_accel_on = 1;
        return;
    }

    if (jlp->sleep == 0 &&  jlp_accel_on && addr == 0x34 && data == 0x6A7A)
    {
        jlp_accel_on = 0;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Ignore writes while sleeping.  Otherwise, handle the fast case      */
    /*  (normal RAM) up front.                                              */
    /* -------------------------------------------------------------------- */
    if (jlp->sleep > 0)                  { jlp->sleep--;            return; }
    if (!jlp_accel_on)                   { return;                          }
    if (addr >= 0x040 && addr <= 0x1F7F) { jlp->ram[addr] = data;   return; }

    /* -------------------------------------------------------------------- */
    /*  Check for mult/div writes                                           */
    /*      $9F8(0,1):  s16($9F80) x s16($9F81) -> s32($9F8F:$9F8E)         */
    /*      $9F8(2,3):  s16($9F82) x u16($9F83) -> s32($9F8F:$9F8E)         */
    /*      $9F8(4,5):  u16($9F84) x s16($9F85) -> s32($9F8F:$9F8E)         */
    /*      $9F8(6,7):  u16($9F86) x u16($9F87) -> u32($9F8F:$9F8E)         */
    /*      $9F8(8,9):  s16($9F88) / s16($9F89) -> quot($9F8E), rem($9F8F)  */
    /*      $9F8(A,B):  u16($9F8A) / u16($9F8B) -> quot($9F8E), rem($9F8F)  */
    /* -------------------------------------------------------------------- */
    if (addr >= 0x1F80 && addr <= 0x1F8F)
    {
        uint32_t prod, quot, rem;

        jlp->ram[addr] = data;

        switch (addr)
        {
            case 0x1F80: case 0x1F81:
                prod = (uint32_t)((int32_t)(int16_t)jlp->ram[0x1F80] *
                                  (int32_t)(int16_t)jlp->ram[0x1F81]);
                jlp->ram[0x1F8E] = prod;
                jlp->ram[0x1F8F] = prod >> 16;
                break;
            case 0x1F82: case 0x1F83:
                prod = (uint32_t)((int32_t)(int16_t)jlp->ram[0x1F82] *
                                  (int32_t)(uint16_t)jlp->ram[0x1F83]);
                jlp->ram[0x1F8E] = prod;
                jlp->ram[0x1F8F] = prod >> 16;
                break;
            case 0x1F84: case 0x1F85:
                prod = (uint32_t)((int32_t)(uint16_t)jlp->ram[0x1F84] *
                                  (int32_t)(int16_t)jlp->ram[0x1F85]);
                jlp->ram[0x1F8E] = prod;
                jlp->ram[0x1F8F] = prod >> 16;
                break;
            case 0x1F86: case 0x1F87:
                prod = (uint32_t)((uint32_t)(uint16_t)jlp->ram[0x1F86] *
                                  (uint32_t)(uint16_t)jlp->ram[0x1F87]);
                jlp->ram[0x1F8E] = prod;
                jlp->ram[0x1F8F] = prod >> 16;
                break;

            case 0x1F88: case 0x1F89:
                if (jlp->ram[0x1F89])
                {
                    quot = (uint32_t)((int16_t)jlp->ram[0x1F88] /
                                      (int16_t)jlp->ram[0x1F89]);
                    rem  = (uint32_t)((int16_t)jlp->ram[0x1F88] %
                                      (int16_t)jlp->ram[0x1F89]);
                    jlp->ram[0x1F8E] = quot;
                    jlp->ram[0x1F8F] = rem;
                }
                break;
            case 0x1F8A: case 0x1F8B:
                if (jlp->ram[0x1F8B])
                {
                    quot = (uint32_t)((uint16_t)jlp->ram[0x1F8A] /
                                      (uint16_t)jlp->ram[0x1F8B]);
                    rem  = (uint32_t)((uint16_t)jlp->ram[0x1F8A] %
                                      (uint16_t)jlp->ram[0x1F8B]);
                    jlp->ram[0x1F8E] = quot;
                    jlp->ram[0x1F8F] = rem;
                }
                break;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Check for JLP X-Register writes                                     */
    /* -------------------------------------------------------------------- */
    if (addr >= 0x1F90 && addr <= 0x1F9F)
    {
        jlp->xreg[addr & 15] = data;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Check for Save Game Arch V2 writes                                  */
    /*                                                                      */
    /*      $8025   JLP RAM address to operate on                           */
    /*      $8026   Flash row to operate on                                 */
    /*                                                                      */
    /*      $802D   Copy JLP RAM to flash row (unlock: $C0DE)               */
    /*      $802E   Copy flash row to JLP RAM (unlock: $DEC0)               */
    /*      $802F   Erase flash sector        (unlock: $BEEF)               */
    /* -------------------------------------------------------------------- */
    if (addr == 0x25 || addr == 0x26)
    {
        jlp->ram[addr] = data;
        return;
    }

    if (addr == 0x2D && data == 0xC0DE) jlp_sg_ram_to_flash(jlp);
    if (addr == 0x2E && data == 0xDEC0) jlp_sg_flash_to_ram(jlp);
    if (addr == 0x2F && data == 0xBEEF) jlp_sg_erase_sector(jlp);

    /* -------------------------------------------------------------------- */
    /*  Check for CRC-16 accelerator writes                                 */
    /* -------------------------------------------------------------------- */
    if (addr == 0x1FFC) jlp->ram[0x1FFD] = jlp_crc16(data, jlp->ram[0x1FFD]);
    if (addr == 0x1FFD) jlp->ram[0x1FFD] = data;
}

/* ======================================================================== */
/*  JLP_POKE     -- Write to JLP w/out side effects                         */
/* ======================================================================== */
LOCAL void jlp_poke(periph_t *const per, periph_t *ign,
                    uint32_t addr, uint32_t data)
{
    jlp_t *const jlp = PERIPH_AS(jlp_t, per);

    if (!jlp_accel_on && addr == 0x33 && data == 0x4A5A)
    {
        jlp_accel_on = 1;
        return;
    }

    if (jlp_accel_on && addr == 0x34 && data == 0x6A7A)
    {
        jlp_accel_on = 0;
        return;
    }

    UNUSED(ign);
    jlp->ram[addr] = data;
}

/* ======================================================================== */
/*  JLP_RESET    -- Reset behaviors.                                        */
/* ======================================================================== */
LOCAL void jlp_reset(periph_t *const per)
{
    jlp_t *const jlp = PERIPH_AS(jlp_t, per);
    jlp->sleep = 0;
    jlp_accel_on = jlp_accel_on_reset;
}

/* ======================================================================== */
/*  JLP_DTOR     -- Tear down the JLP                                       */
/* ======================================================================== */
LOCAL void jlp_dtor(periph_t *const p)
{
    jlp_t *const jlp = PERIPH_AS(jlp_t, p);

    if (jlp->sg_file) fclose(jlp->sg_file);

    CONDFREE(jlp->ram   );
    CONDFREE(jlp->sg_img);
}

/* ======================================================================== */
/*  JLP_INIT     -- Sets up the JLP support                                 */
/* ======================================================================== */
int jlp_init
(
    jlp_t          *jlp,        /*  Structure to initialize.        */
    const char     *fname,      /*  Save-game file                  */
    uint16_t       *xreg,       /*  Pointer to CPU X-Regs           */
    int             jlp_flags,  /*  JLP Accelerator flags           */
    int             jlp_flash,  /*  JLP flash storage size          */
    int             randomize   /*  Randomize on power-up.          */
)
{
    int i;

    jlp->xreg             = xreg;
    jlp->periph.write     = jlp_write;
    jlp->periph.read      = jlp_read;
    jlp->periph.peek      = jlp_peek;
    jlp->periph.poke      = jlp_poke;
    jlp->periph.reset     = jlp_reset;
    jlp->periph.tick      = NULL;
    jlp->periph.min_tick  = ~0U;
    jlp->periph.max_tick  = ~0U;
    jlp->periph.addr_base = 0x8000;
    jlp->periph.addr_mask = 0x1FFF;
    jlp->periph.ser_init  = 0;
    jlp->periph.dtor      = jlp_dtor;

    jlp_accel_on_reset    = (jlp_flags & 1) != 0;
    jlp_accel_on          = jlp_accel_on_reset;
    lto_isa_enabled       = jlp_flags != 0;

    /* -------------------------------------------------------------------- */
    /*  Set up flash image, if enabled.                                     */
    /* -------------------------------------------------------------------- */
    if ( (jlp_flags & 2) != 0 && jlp_flash > 0 )
    {
        jlp->sg_start = JLP_SG_OFS;
        jlp->sg_end   = JLP_SG_OFS + jlp_flash * 8 - 1;
        jlp->sg_bytes = jlp_flash * 192 * 8;
        jzp_printf("JLP: Flash from $%04X to $%04X (%d bytes) (flash=%d)\n",
                    jlp->sg_start, jlp->sg_end, jlp->sg_bytes, jlp_flash);
    } else
    {
        if (jlp_flags & 2)
            jzp_printf("JLP: Warning: Flash requested, but size set to 0.\n");
        jlp->sg_start = 0;
        jlp->sg_end   = 0;
        jlp->sg_bytes = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Allocate and set up RAM image                                       */
    /* -------------------------------------------------------------------- */
    if (!(jlp->ram = CALLOC(uint16_t, 8192)))
    {
        jzp_printf("JLP: out of memory\n");
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Allocate and set up flash image                                     */
    /* -------------------------------------------------------------------- */
    if (jlp->sg_bytes > 0)
    {
        if (!(jlp->sg_img = CALLOC(uint8_t, jlp->sg_bytes)))
        {
            jzp_printf("JLP: out of memory\n");
            return -1;
        }

        memset(jlp->sg_img, 0xFF, jlp->sg_bytes);
    }

    /* -------------------------------------------------------------------- */
    /*  Locations 0x000 - 0x03F and 0xF90 - 0xFFE aren't implemented, so    */
    /*  we fill them with 0xFFFF.   0x1FFF always reads as 0.               */
    /* -------------------------------------------------------------------- */
    for (i = 0x0000; i < 0x0040; i++) jlp->ram[i] = 0xFFFF;
    for (i = 0x1F90; i < 0x1FFF; i++) jlp->ram[i] = 0xFFFF;
    jlp->ram[0x1FFF] = 0;

    /* -------------------------------------------------------------------- */
    /*  If asked to randomize memory on startup, do so now.                 */
    /* -------------------------------------------------------------------- */
    if (randomize)
    {
        for (i = 0x0040; i < 0x1F90; i++)
            jlp->ram[i] = rand_jz();
    }

    /* -------------------------------------------------------------------- */
    /*  The JLP Save Game feature borrows locations 0x23, 0x24, 0x2D-0x2F   */
    /*  for read-only settings and command registers.                       */
    /* -------------------------------------------------------------------- */
    jlp->ram[0x23] = jlp->sg_start;             /* First valid flash sector */
    jlp->ram[0x24] = jlp->sg_end;               /* Last  valid flash sector */
    jlp->ram[0x2D] = 0;                         /* Command regs read as 0   */
    jlp->ram[0x2E] = 0;                         /* Command regs read as 0   */
    jlp->ram[0x2F] = 0;                         /* Command regs read as 0   */

    /* -------------------------------------------------------------------- */
    /*  If a save-game file was specified, set up a backing file for the    */
    /*  save-game image, and make a backup file by appending a '~' at the   */
    /*  end of the save-game filename.                                      */
    /* -------------------------------------------------------------------- */
    if (fname &&
        ( (jlp->sg_file = fopen(fname, "r+b")) != NULL ||
          (jlp->sg_file = fopen(fname, "w+b")) != NULL ) )
    {
        FILE   *f;
        size_t len = strlen(fname);
        char   *fnb;

        if (!(fnb = CALLOC(char, len + 2)))
        {
            jzp_printf("JLP: out of memory\n");
            return -1;
        }

        strcpy(fnb, fname);
        fnb[len    ]  = '~';
        fnb[len + 1]  = 0;

        fseek(jlp->sg_file, 0, SEEK_SET);
        if (fread(jlp->sg_img, 1, jlp->sg_bytes, jlp->sg_file) != jlp->sg_bytes)
            /* ignore */{}

        /* Try to write a backup file */
        if ((f = fopen(fnb, "wb")) != NULL)
        {
            fwrite(jlp->sg_img, 1, jlp->sg_bytes, f);
            fclose(f);
        }

        free(fnb);

        fseek(jlp->sg_file, 0, SEEK_SET);
        if (fwrite(jlp->sg_img, 1, jlp->sg_bytes, jlp->sg_file)
                                                            != jlp->sg_bytes)
        {
            jzp_printf("JLP: Unable to initialize backing file\n");
            return -1;
        }
    }

    return 0;
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
/*                 Copyright (c) 2009-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
