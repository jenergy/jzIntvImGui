/*
 * ============================================================================
 *  Title:    Controller pads
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements the controller pads.
 *  Pads are peripherals that extend periph_t.
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "pads/pads.h"

/* ------------------------------------------------------------------------ */
/*  Fake shift state machine states.                                        */
/*  These act as flags:  You can set PAD_FS_PENDING while PAD_FS_ENGAGED.   */
/* ------------------------------------------------------------------------ */
enum
{
    PAD_FS_IDLE,
    PAD_FS_ENGAGED = 1,     /*  Fake shift currently pressed.               */
    PAD_FS_PENDING = 2      /*  Engage fake shift on next keyboard scan.    */
};

/* ======================================================================== */
/*  PAD_EVAL_HAND    -- Interprets pad inputs as hand controllers.          */
/*                      Both I/Os must be in input mode.                    */
/* ======================================================================== */
LOCAL void pad_eval_hand(pad_t *const pad)
{
    /* -------------------------------------------------------------------- */
    /*  Iterate over both controllers.                                      */
    /* -------------------------------------------------------------------- */
    for (int side_idx = 0; side_idx < 2; side_idx++)
    {
        /* ---------------------------------------------------------------- */
        /*  Merge all of the keypad / action key inputs to the controller.  */
        /* ---------------------------------------------------------------- */
        const uint32_t *const event_inputs = side_idx == 0 ? pad->r : pad->l;
        uint32_t merged_inputs = 0;
        for (int i = 0; i < 15; i++)
            merged_inputs |= event_inputs[i];

        /* ---------------------------------------------------------------- */
        /*  Merge in the "raw bits" inputs.                                 */
        /* ---------------------------------------------------------------- */
        merged_inputs |= event_inputs[17];

        /* ---------------------------------------------------------------- */
        /*  Now, generate a disc dir # from E/NE/N/NW/W/SW/S/SE flags.      */
        /*                                                                  */
        /*  Input bits 0, 2, 4, and 6 give us pure E, N, W, and S.          */
        /*  Input bits 1, 3, 5, and 7 give us NE, NW, SE, SW.               */
        /*                                                                  */
        /*  Pad bit 0 is set for WSW through SE.                            */
        /*  Pad bit 1 is set for SSE through NE.                            */
        /*  Pad bit 2 is set for ENE through NW.                            */
        /*  Pad bit 3 is set for NNW through SW.                            */
        /*  Pad bit 4 is set for NE, NNE, NW, WNW, SW, SSW, SE, ESE.        */
        /*                                                                  */
        /*  Input bit to compass headings:                                  */
        /*                                                                  */
        /*      Compass         Pattern 1           Pattern 2               */
        /*         E            00000001                                    */
        /*         ENE          00000011                                    */
        /*         NE           00000010            00000101                */
        /*         NNE          00000110                                    */
        /*         N            00000100                                    */
        /*         NNW          00001100                                    */
        /*         NW           00001000            00010100                */
        /*         WNW          00011000                                    */
        /*         W            00010000                                    */
        /*         WSW          00110000                                    */
        /*         SW           00100000            01010000                */
        /*         SSW          01100000                                    */
        /*         S            01000000                                    */
        /*         SSE          11000000                                    */
        /*         SE           10000000            01000001                */
        /*         ESE          10000001                                    */
        /*                                                                  */
        /* ---------------------------------------------------------------- */
        uint32_t dir_flags = (0xFF & (event_inputs[15] | event_inputs[16]));
        dir_flags = (dir_flags << 4) | (dir_flags >> 4);

        /* ---------------------------------------------------------------- */
        /*  Step through the four major compass dirs and set bits 0..3      */
        /*  according to each range.  Also process bit 4 along the way.     */
        /*  We begin our analysis with 'south' as that is bit 0.            */
        /* ---------------------------------------------------------------- */
        for (int i = 0; i < 4; i++, dir_flags >>= 2)
        {
            /* ------------------------------------------------------------ */
            /*  Handle major-direction bit.  We set the bit for any dir     */
            /*  that is mostly in the direction of the major-direction, as  */
            /*  well as for one lop-sided case on the side.                 */
            /* ------------------------------------------------------------ */
            if ((dir_flags & 0x14) != 0x10 &&
                (dir_flags & 0x0F) > 0x01)
                merged_inputs |= 1u << i;

            /* ------------------------------------------------------------ */
            /*  Check for diagonal bit (bit 4) also.                        */
            /* ------------------------------------------------------------ */
            if ((0x64 >> (dir_flags & 0x07)) & 1)
                merged_inputs |= 0x10;
        }

        /* ---------------------------------------------------------------- */
        /*  Record the merged inputs, inverting because active-low.         */
        /* ---------------------------------------------------------------- */
        pad->side[side_idx] = 0xFF & ~merged_inputs;
    }
}

/* ======================================================================== */
/*  Helper functions for special key inputs: Fake shift and Synth.          */
/* ======================================================================== */
LOCAL INLINE uint32_t fake_shift_bits(const uint32_t row)
{
    return (row >> 8) & 0xFF;
}

LOCAL INLINE uint32_t synth_key_bits(const uint32_t row)
{
    return (row >> 16) & 0xFF;
}

/* ======================================================================== */
/*  BIT_TRANSPOSE_8X8    -- Transpose an 8x8 bit matrix.                    */
/*  This converts the keyboard bitmap from row-major to column-major.       */
/*                                                                          */
/*    src[0]  a7 a6 a5 a4 a3 a2 a1 a0     h0 g0 f0 e0 d0 c0 b0 a0  dst[0]   */
/*    src[1]  b7 b6 b5 b4 b3 b2 b1 b0     h1 g1 f1 e1 d1 c1 b1 a1  dst[1]   */
/*    src[2]  c7 c6 c5 c4 c3 c2 c1 c0     h2 g2 f2 e2 d2 c2 b2 a2  dst[2]   */
/*    src[3]  d7 d6 d5 d4 d3 d2 d1 d0 ==> h3 g3 f3 e3 d3 c3 b3 a3  dst[3]   */
/*    src[4]  e7 e6 e5 e4 e3 e2 e1 e0     h4 g4 f4 e4 d4 c4 b4 a4  dst[4]   */
/*    src[5]  f7 f6 f5 f4 f3 f2 f1 f0     h5 g5 f5 e5 d5 c5 b5 a5  dst[5]   */
/*    src[6]  g7 g6 g5 g4 g3 g2 g1 g0     h6 g6 f6 e6 d6 c6 b6 a6  dst[6]   */
/*    src[7]  h7 h6 h5 h4 h3 h2 h1 h0     h7 g7 f7 e7 d7 c7 b7 a7  dst[7]   */
/*                                                                          */
/* ======================================================================== */
LOCAL INLINE void bit_transpose_8x8(uint8_t *const dst,
                                    const uint8_t *const src)
{
#if CHAR_BIT != 8
    /* Slow reference version. */
    for (int col = 0; col < 8; col++)
    {
        uint32_t row_bits_for_col = 0;

        for (int row = 0; row < 8; row++)
            row_bits_for_col |= ((src[row] >> col) & 1) << row;

        dst[col] = row_bits_for_col;
    }
#else
    /* Tricky fast version, adapted from Hacker's Delight, v2, figure 7-6. */
    uint64_t x;
    memcpy((void *)&x, src, sizeof(uint64_t));

# ifdef BYTE_LE
    x = ((x     ) & 0xAA55AA55AA55AA55ull) |
        ((x << 7) & 0x5500550055005500ull) |
        ((x >> 7) & 0x00AA00AA00AA00AAull);

    x = ((x      ) & 0xCCCC3333CCCC3333ull) |
        ((x << 14) & 0x3333000033330000ull) |
        ((x >> 14) & 0x0000CCCC0000CCCCull);

    x = ((x      ) & 0xF0F0F0F00F0F0F0Full) |
        ((x << 28) & 0x0F0F0F0F00000000ull) |
        ((x >> 28) & 0x00000000F0F0F0F0ull);
# else /*BYTE_BE*/
    x = ((x     ) & 0x55AA55AA55AA55AAull) |
        ((x >> 9) & 0x0055005500550055ull) |
        ((x << 9) & 0xAA00AA00AA00AA00ull);

    x = ((x      ) & 0x3333CCCC3333CCCCull) |
        ((x >> 18) & 0x0000333300003333ull) |
        ((x << 18) & 0xCCCC0000CCCC0000ull);

    x = ((x      ) & 0x0F0F0F0FF0F0F0F0ull) |
        ((x >> 36) & 0x000000000F0F0F0Full) |
        ((x << 36) & 0xF0F0F0F000000000ull);
# endif

    memcpy(dst, (const void *)&x, sizeof(uint64_t));
#endif
}

/* ======================================================================== */
/*  PAD_EVAL_KEYBOARD    -- Interprets pad inputs as a keyboard.            */
/* ======================================================================== */
LOCAL void pad_eval_keyboard(pad_t *const pad)
{
    /* -------------------------------------------------------------------- */
    /*  The I/O ports can have one of four settings.  The following truth   */
    /*  table indicates the meaning of these four modes.                    */
    /*                                                                      */
    /*      io[1]   io[0]       Meaning                                     */
    /*        0       0         Both sides read -- scan hand controllers    */
    /*        0       1         Normal scanning mode.  0 drives, 1 reads    */
    /*        1       0         Transposed scanning.   1 drives, 0 reads    */
    /*        1       1         Illegal:  Both sides driving.               */
    /*                                                                      */
    /*  It's worth noting that ECS BASIC does not employ transposed scan.   */
    /*  Future software might, because it gives better opportunities for    */
    /*  disambiguating key aliases, esp. wrt to the SHIFT key.              */
    /*                                                                      */
    /*  The QWERTY keyboard has no diodes and therefore can be scanned      */
    /*  both normally and in a transposed manner.  It is also subject to    */
    /*  buffer fights due to ghost paths, limiting what keys the Inty can   */
    /*  resolve simultaneously.                                             */
    /*                                                                      */
    /*  The synthesizer keyboard, on the other hand, has a full set of      */
    /*  diodes.  As a result, it can only be scanned in the normal          */
    /*  direction, but there will never be any issues with ghosting or      */
    /*  buffer fights.  The Inty can resolve any key combination.           */
    /*                                                                      */
    /*  We handle the undefined "both driven" state by ignoring it.  If     */
    /*  we want to, we can use that state as a magic handshake so that      */
    /*  apps can request higher-quality keyboard input from an emulator.    */
    /* -------------------------------------------------------------------- */
    if (pad->io[0] == PAD_DIR_OUTPUT &&
        pad->io[1] == PAD_DIR_OUTPUT)   /* both sides driving; do nothing. */
    {
        return;
    }

    /* Keyboard matrices, in normal (by row) and transposed (by col) order. */
    uint8_t keys_by_row[8];         /* Each row has 1 bit per column.       */
    uint8_t keys_by_col[8];         /* Each colum has 1 bit per row.        */

    /* -------------------------------------------------------------------- */
    /*  Fold the 'fake-shift' data down into the real data if we need it.   */
    /* -------------------------------------------------------------------- */
    if (pad->fake_shift & PAD_FS_ENGAGED)
        for (int row = 0; row < 8; row++)
            keys_by_row[row] =
                (pad->k[row] | fake_shift_bits(pad->k[row])) & 0xFF;
    else
        for (int row = 0; row < 8; row++)
            keys_by_row[row] = pad->k[row] & 0xFF;

    /* -------------------------------------------------------------------- */
    /*  Determine if we'll need fake-shift during an upcoming scan.  Some   */
    /*  of our keybindings press a key AND shift.  We handle that by        */
    /*  looking at the upper bits of the value table and seeing if we need  */
    /*  to push shift in addition to the key.  Pending shifts move to       */
    /*  Engaged when the keyboard scan reads the shift key state.           */
    /* -------------------------------------------------------------------- */
    bool need_fake_shift = false;

    for (int row = 0; row < 8; row++)
        if (fake_shift_bits(pad->k[row]))
            need_fake_shift = true;

    if (need_fake_shift) { pad->fake_shift |= PAD_FS_PENDING; }
    else                 { pad->fake_shift  = PAD_FS_IDLE; }

    /* -------------------------------------------------------------------- */
    /*  Compute the transpose of the keyboard.  We need this to identify    */
    /*  "buffer fight paths", so that we correctly mask away keys in the    */
    /*  same way a real ECS keyboard will.                                  */
    /* -------------------------------------------------------------------- */
    bit_transpose_8x8(/* dst = */ keys_by_col, /* src = */ keys_by_row);

    /* -------------------------------------------------------------------- */
    /*  Merge any synth keys into the "by-row" keyboard image, but not the  */
    /*  "by-col" keyboard image.  The synth has blocking diodes that cause  */
    /*  the transpose to read as all zeros.  That is why we do this step    */
    /*  after computing the transpose, rather than before.                  */
    /* -------------------------------------------------------------------- */
    for (int row = 0; row < 8; row++)
        keys_by_row[row] |= synth_key_bits(pad->k[row]);

    /* -------------------------------------------------------------------- */
    /*  Handle normal scanning:                                             */
    /* -------------------------------------------------------------------- */
    if (pad->io[0] == PAD_DIR_OUTPUT)
    {
        /* ---------------------------------------------------------------- */
        /*  Ok, merge together the data from the rows selected by side 0.   */
        /* ---------------------------------------------------------------- */
        const uint32_t row_select_n = pad->side[0];   /* Active Low */
        uint32_t merged_cols = 0;

        for (int row = 0; row < 8; row++)
        {
            if ((row_select_n & (1u << row)) == 0)
                merged_cols |= keys_by_row[row];
        }

        /* ---------------------------------------------------------------- */
        /*  Handle pushing the fake-shift.  We look for row 6 requested in  */
        /*  the scan, and the fake-shift flag being set.  If both are met,  */
        /*  assert the shift in col 7.                                      */
        /* ---------------------------------------------------------------- */
        if ((row_select_n & (1u << 6)) == 0 &&
            (pad->fake_shift & PAD_FS_PENDING) != 0)
        {
            merged_cols |= 1u << 7;
            pad->fake_shift = PAD_FS_ENGAGED;
        }

        /* ---------------------------------------------------------------- */
        /*  Go back and clear out bits that would get zapped by ghost       */
        /*  paths in the scanning matrix.  These happen wherever there's a  */
        /*  1 in the transpose that isn't matched up to a 0 in the scan val */
        /* ---------------------------------------------------------------- */
        for (int col = 0; col < 8; col++)
        {
            const uint32_t col_ghost_bits = keys_by_col[col] & row_select_n;
            if (col_ghost_bits)
                merged_cols &= ~(1u << col);
        }

        pad->side[1] = 0xFF & ~merged_cols;
    }
    /* -------------------------------------------------------------------- */
    /*  Handle transposed scanning:                                         */
    /* -------------------------------------------------------------------- */
    else if (pad->io[1] == PAD_DIR_OUTPUT)
    {
        /* ---------------------------------------------------------------- */
        /*  Ok, merge together the data from the cols selected by side 1.   */
        /* ---------------------------------------------------------------- */
        const uint32_t col_select_n = pad->side[1];  /* Active Low */

        uint32_t merged_rows = 0;
        for (int col = 0; col < 8; col++)
        {
            if ((col_select_n & (1u << col)) == 0)
                merged_rows |= keys_by_col[col];
        }

        /* ---------------------------------------------------------------- */
        /*  Handle pushing the fake-shift.  We look for col 7 requested in  */
        /*  the scan, and the fake-shift flag being set.  If both are met,  */
        /*  assert the shift in row 6.                                      */
        /* ---------------------------------------------------------------- */
        if ((col_select_n & (1u << 7)) == 0 &&
            (pad->fake_shift & PAD_FS_PENDING) != 0)
        {
            merged_rows |= 1u << 6;
            pad->fake_shift = PAD_FS_ENGAGED;
        }

        /* ---------------------------------------------------------------- */
        /*  Go back and clear out bits that would get zapped by ghost       */
        /*  paths in the scanning matrix.  These happen wherever there's a  */
        /*  1 in the transpose that isn't matched up to a 0 in the scan val */
        /* ---------------------------------------------------------------- */
        for (int row = 0; row < 8; row++)
        {
            const uint32_t row_ghost_bits = keys_by_row[row] & col_select_n;
            if (row_ghost_bits)
                merged_rows &= ~(1u << row);
        }

        pad->side[0] = 0xFF & ~merged_rows;
    }
}

/* ======================================================================== */
/*  PAD_TICK     -- Just mark the controller inputs stale.                  */
/* ======================================================================== */
LOCAL uint32_t pad_tick(periph_t *per, uint32_t len)
{
    pad_t *const pad = PERIPH_AS(pad_t, per);
    pad->stale = true;
    return len;
}

/* ======================================================================== */
/*  PAD_READ     -- Returns the current state of the pads.                  */
/* ======================================================================== */
LOCAL uint32_t pad_read(periph_t *per, periph_t *req,
                        uint32_t addr, uint32_t data)
{
    pad_t *const pad = PERIPH_AS(pad_t, per);
    UNUSED(req);
    UNUSED(data);

    if (addr < 14) return ~0U;

    if (pad->stale)
    {
        pad->stale = false;

        if (pad->io_cap == PAD_INPUT_ONLY ||
            (pad->io[0] == PAD_DIR_INPUT && pad->io[1] == PAD_DIR_INPUT))
            pad_eval_hand(pad);
        else
            pad_eval_keyboard(pad);
    }

    const int pad_idx = addr & 1;
    return pad->side[pad_idx];
}

/* ======================================================================== */
/*  PAD_WRITE    -- Looks for changes in I/O mode on PSG I/O ports.         */
/* ======================================================================== */
LOCAL void pad_write(periph_t *per, periph_t *req, uint32_t addr, uint32_t data)
{
    pad_t *const pad = PERIPH_AS(pad_t, per);
    UNUSED(req);

    /* -------------------------------------------------------------------- */
    /*  Only look at lower 8 bits of the write.                             */
    /* -------------------------------------------------------------------- */
    data &= 0xFF;

    /* -------------------------------------------------------------------- */
    /*  Capture writes to the 'control' register in the PSG, looking for    */
    /*  I/O direction setup.  Re-evaluate pad if I/O direction changes.     */
    /* -------------------------------------------------------------------- */
    if (addr == 8 && pad->io_cap == PAD_BIDIR)
    {
        const pad_io_dir_t io_0 = (data >> 6) & 1 ? PAD_DIR_OUTPUT
                                                  : PAD_DIR_INPUT;
        const pad_io_dir_t io_1 = (data >> 7) & 1 ? PAD_DIR_OUTPUT
                                                  : PAD_DIR_INPUT;

        pad->stale |= io_0 != pad->io[0] || io_1 != pad->io[1];

        pad->io[0] = io_0;
        pad->io[1] = io_1;
    }

    /* -------------------------------------------------------------------- */
    /*  Look for writes to I/O port 0 and 1.  If they're set to output,     */
    /*  record the writes.  Re-evaluate pad if I/O output changes.          */
    /* -------------------------------------------------------------------- */
    const int pad_idx = addr & 1;
    if (addr >= 14 && pad->io[pad_idx] == PAD_DIR_OUTPUT)
    {
        pad->stale |= pad->side[pad_idx] != data;
        pad->side[pad_idx] = data;
    }

    return;
}

/* ======================================================================== */
/*  PAD_RESET_INPUTS -- Reset the input bitvectors.  Used when switching    */
/*                      keyboard input maps.                                */
/* ======================================================================== */
void pad_reset_inputs(pad_t *const pad)
{
    memset(pad->l, 0, sizeof(pad->l));
    memset(pad->r, 0, sizeof(pad->r));
    memset(pad->k, 0, sizeof(pad->k));
    pad->fake_shift = PAD_FS_IDLE;
    pad->stale = true;
}

/* ======================================================================== */
/*  PAD_INIT     -- Makes a pair of input controller pads                   */
/* ======================================================================== */
int pad_init
(
    pad_t          *const pad,      /*  pad_t structure to initialize       */
    const uint32_t        addr,     /*  Base address of pad.                */
    const pad_io_cap_t    io_cap    /*  Input only, or bidirectional?       */
)
{
    pad->periph.read      = pad_read;
    pad->periph.write     = pad_write;
    pad->periph.peek      = pad_read;
    pad->periph.poke      = pad_write;
    pad->periph.tick      = pad_tick;
    pad->periph.min_tick  = 3579545 / (4*240);  /* 240Hz scanning rate. */
    pad->periph.max_tick  = 3579545 / (4*120);  /* 120Hz scanning rate. */

    pad->periph.addr_base = addr;
    pad->periph.addr_mask = 0xF;

    pad->side[0]          = 0xFF;
    pad->side[1]          = 0xFF;
    pad->io  [0]          = PAD_DIR_INPUT;
    pad->io  [1]          = PAD_DIR_INPUT;
    pad->io_cap           = io_cap;

    pad_reset_inputs(pad);

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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
