/* ======================================================================== */
/*  DIS-1600  Advanced(?) CP-1600 Disassembler.                             */
/*  By Joseph Zbiciak                                                       */
/* ------------------------------------------------------------------------ */
/*                   Copyright (c) 2006, Joseph Zbiciak                     */
/* ======================================================================== */

#include "dasm/dis1600.h"


/* ======================================================================== */
/*  EXEC_ROUTINE_SYM -- Names of EXEC routines known to us.                 */
/*                      These names aren't necessarily the names Mattel     */
/*                      used.  These are just names meaningful to me.       */
/* ======================================================================== */
struct defsym_t exec_routine_sym[] =
{
    { "X_RESET",                0x1000,     1,          1   },
    { "X_RET_R5",               0x1003,     1,          1   },
    { "X_ISR",                  0x1004,     1,          1   },
    { "X_ISRRET",               0x1014,     1,          1   },
    { "X_CHK_KBD_OR_CBL",       0x101D,     1,          1   },
    { "X_INIT",                 0x1026,     1,          1   },
    { "X_READ_ROM_HDR",         0x10AB,     1,          1   },
    { "X_DEF_ISR",              0x1126,     1,          1   },

    { "X_RAND1",                0x167D,     1,          1   },
    { "X_RAND2",                0x169E,     1,          1   },

    { "X_INIT_MOB",             0x16B2,     1,          1   },
    { "X_INIT_MOBS",            0x16B4,     1,          1   },

    { "X_FILL_ZERO",            0x1738,     1,          1   },
    { "X_FILL_MEM",             0x1741,     1,          1   },
    { "X_PACK_BYTES",           0x174F,     1,          1   },
    { "X_PACK_BYTES.1",         0x1750,     1,          1   },
    { "X_UNPK_BYTES",           0x1757,     1,          1   },
    { "X_UNPK_BYTES.1",         0x1758,     1,          1   },

    { "X_GETNUM",               0x18FF,     1,          1   },
    { "X_PRNUM_LFT",            0x189E,     1,          1   },
    { "X_PRNUM_ZRO",            0x18AD,     1,          1   },
    { "X_PRNUM_RGT",            0x18C5,     1,          1   },

    { "X_PRINT_R1",             0x1867,     1,          1   },
    { "X_PRPAD_R1",             0x186C,     1,          1   },
    { "X_PRINT_R5",             0x187B,     1,          1   },
    { "X_PRPAD_R5",             0x1871,     1,          1   },

    { "X_EXT_SIGN_LO",          0x1668,     1,          1   },
    { "X_EXT_SIGN_HI",          0x1669,     1,          1   },
    { "X_CLAMP",                0x1670,     1,          1   },
    { "X_SET_BIT",              0x16DB,     1,          1   },
    { "X_CLR_BIT",              0x16E6,     1,          1   },
    { "X_POW2",                 0x1745,     1,          1   },

    { "X_TIMER_STOP",           0x1838,     1,          1   },
    { "X_TIMER_START",          0x1844,     1,          1   },

    { "X_PLAY_NOTE",            0x1ABD,     1,          1   },
    { "X_HUSH",                 0x1AA8,     1,          1   },
    { "X_PLAY_MUS1",            0x1B27,     1,          1   },
    { "X_PLAY_MUS2",            0x1B5D,     1,          1   },
    { "X_PLAY_MUS3",            0x1B95,     1,          1   },
    { "X_PLAY_SFX1",            0x1BBB,     1,          1   },
    { "X_PLAY_SFX2",            0x1BBE,     1,          1   },

    { "X_SFX_OK",               0x1EAD,     1,          1   },
    { "X_STOP_SFX",             0x1EB4,     1,          1   },
    { "X_PLAY_RAZZ1",           0x1EBA,     1,          1   },
    { "X_PLAY_RAZZ2",           0x1EBD,     1,          1   },
    { "X_PLAY_RAZZ3",           0x1EC1,     1,          1   },
    { "X_PLAY_RAZZ4",           0x1EC4,     1,          1   },
    { "X_PLAY_RAZZ5",           0x1EC5,     1,          1   },
    { "X_PLAY_CHEER1",          0x1ED5,     1,          1   },
    { "X_PLAY_CHEER2",          0x1ED6,     1,          1   },
    { "X_PLAY_WHST1",           0x1F1B,     1,          1   },
    { "X_PLAY_WHST2",           0x1F1E,     1,          1   },
    { "X_PLAY_WHST3",           0x1F22,     1,          1   },

    { "X_MPY",                  0x1DDC,     1,          1   },
    { "X_DIV",                  0x1DFB,     1,          1   },
    { "X_DIVR",                 0x1DF8,     1,          1   },
    { "X_SQRT",                 0x1E23,     1,          1   },
    { "X_SQUARE",               0x1DDB,     1,          1   },

    { "X_DO_GRAM_INIT",         0x1F2F,     1,          1   },
    { "X_NEW_GRAM_INIT",        0x1F35,     1,          1   },

    { NULL,                     0,          0,          0   }
};


/* ======================================================================== */
/*  SETUP_EXEC_ROUTINE_SYM -- Mark down EXEC routines we know.              */
/* ======================================================================== */
void setup_exec_routine_sym(void)
{
    int i;

    for (i = 0; exec_routine_sym[i].name; i++)
        maybe_defsym(exec_routine_sym[i].name, exec_routine_sym[i].addr);
}


/* ======================================================================== */
/* ======================================================================== */
/*  EXEC DECODERS                                                           */
/* ======================================================================== */
/* ======================================================================== */

typedef struct exec_hdr_dsc_t
{
    const char *cmt;
    int len;
} exec_hdr_dsc_t;


static const exec_hdr_dsc_t exec_hdr_dsc[] =
{
    {   "Ptr: MOB graphic images",          2   },
    {   "Ptr: EXEC timer table",            2   },
    {   "Ptr: Start of game",               2   },
    {   "Ptr: Backgnd gfx list",            2   },
    {   "Ptr: GRAM init sequence",          2   },
    {   "Ptr: Date/Title",                  2   },
    {   "Key-click / flags",                1   },
    {   "Border extension",                 1   },
    {   "Color Stack / FGBG",               1   },
    {   "Color Stack init (0, 1)",          2   },
    {   "Color Stack init (2, 3)",          2   },
    {   "Border color init\n",              1   }
};

uint32_t used_mob_pics[ 8];
uint32_t used_gfx_pics[16];

/* ======================================================================== */
/*  DECODE_GRAM_INIT                                                        */
/* ======================================================================== */
int decode_gram_init(uint32_t addr)
{
    int i, num_pic, changed = 0;
    char buf[64];
    int pic_num = 0;
    int rpt, xmr, ymr, inv, rot, crd;
    const char *src;

    num_pic = GET_WORD(addr);

    /* -------------------------------------------------------------------- */
    /*  Skip bogus init lists.                                              */
    /* -------------------------------------------------------------------- */
    if (IS_EMPTY(addr) || IS_INTERP(addr) || num_pic == 0 || num_pic > 64)
        return 0;

    /* -------------------------------------------------------------------- */
    /*  List starts with # of GRAM cards to initialize.  Must be 1..64.     */
    /* -------------------------------------------------------------------- */
    snprintf(buf, sizeof(buf), ".GRAM_INIT_%.4X", addr);
    if (!no_default_symbols)
        maybe_defsym(buf, addr);

    instr[addr].cmt_len = CMT_LONG;
    changed += mark_interp(addr++, FLAG_DATA, 1, "# of GRAM cards to init");

    /* -------------------------------------------------------------------- */
    /*  Iterate over the entries.  Some entries have repeat counts, so we   */
    /*  need to keep careful tally.                                         */
    /* -------------------------------------------------------------------- */
    while (pic_num < num_pic)
    {
        int w0, w1;
        int s_pic = -1, e_pic = -1;

        /* ---------------------------------------------------------------- */
        /*  Assume a single-decle record at first.                          */
        /* ---------------------------------------------------------------- */
        w0 = GET_WORD(addr++);

        xmr = w0 & 0x200;
        ymr = w0 & 0x100;
        inv = w0 & 0x080;

        /* ---------------------------------------------------------------- */
        /*  If bit 0 == 0, it's a 1-decle record.  This specifies a cart    */
        /*  to GRAM transfer of 1 pic.  Only XM/YM/INV flags may be used.   */
        /* ---------------------------------------------------------------- */
        if ((w0 & 1) == 0)
        {
            crd = 0x3F & (w0 >> 1);

            snprintf(buf, sizeof(buf), "#%.2X   :  CART #%.2X    %c%c%c-",
                    pic_num, crd,
                    xmr ? 'X' : '-',
                    ymr ? 'Y' : '-',
                    inv ? 'I' : '-');

            if (pic_num == num_pic - 1)
                strcat(buf, "\n");

            s_pic = e_pic = crd;

            instr[addr - 1].cmt_len = CMT_LONG;
            changed += mark_interp(addr-1, FLAG_DATA, 1, strdup(buf));

        } else
        {
            /* ------------------------------------------------------------ */
            /*  All other record types have 2 words.                        */
            /* ------------------------------------------------------------ */
            w1 = GET_WORD(addr++);

            /* ------------------------------------------------------------ */
            /*  If the 3 LSBs of the second word are 0, this is an extended */
            /*  picture copy.  It could have rpt > 0, source a card from    */
            /*  GRAM or GROM, specify rotation, specify a picture # > 63,   */
            /*  or any combo of the above.                                  */
            /* ------------------------------------------------------------ */
            if ((w1 & 7) == 0)
            {
                rpt = (w1 >> 7) & 7;                /* rpt - 1 in bits 9:7  */
                rot = w1 & 0x008;
                src = w1 & 0x010 ? "GROM" :
                      w1 & 0x040 ? "GRAM" : "CART";

                /* Get the source card #.  If the source is GRAM, truncate  */
                crd = (0xC0 & (w1 << 1)) | (0x3F & (w0 >> 1));
                if (w1 & 0x40)
                    crd &= 0x3F;

                snprintf(buf, sizeof(buf),
                        "#%.2X-%.2X:  %4s #%.2X-%.2X %c%c%c%c",
                        pic_num, pic_num + rpt, src, crd, crd + rpt,
                        xmr ? 'X' : '-',
                        ymr ? 'Y' : '-',
                        inv ? 'I' : '-',
                        rot ? 'R' : '-');

                if ((w1 & 0x50) == 0)
                {
                    s_pic = crd;
                    e_pic = crd + rpt;
                }

                pic_num += rpt;
            } else
            /* ------------------------------------------------------------ */
            /*  If the 3 LSBs are 100, then it's a 4x4 tile that gets       */
            /*  expanded to 8x8 through replication.                        */
            /* ------------------------------------------------------------ */
            if ((w1 & 7) == 4)
            {
                snprintf(buf, sizeof(buf), "#%.2X   :  4x4 tile", pic_num);
            } else
            /* ------------------------------------------------------------ */
            /*  If the 2 LSBs are 10, then the remaining bits drive a       */
            /*  generator algorithm.                                        */
            /* ------------------------------------------------------------ */
            if ((w1 & 3) == 2)
            {
                snprintf(buf, sizeof(buf), "#%.2X   :  Algo #1", pic_num);
            } else
            /* ------------------------------------------------------------ */
            /*  Otherwise, those bits drive a different generator algo.     */
            /* ------------------------------------------------------------ */
            {
                snprintf(buf, sizeof(buf), "#%.2X   :  Algo #2", pic_num);
            }

            if (pic_num == num_pic - 1)
                strcat(buf, "\n");

            instr[addr - 2].cmt_len = CMT_LONG;
            changed += mark_interp(addr-2, FLAG_DATA, 2, strdup(buf));
        }

        /* ---------------------------------------------------------------- */
        /*  Keep track of which pics we've loaded from the bkgd grx list.   */
        /* ---------------------------------------------------------------- */
        if (s_pic >= 0)
            for (i = s_pic; i <= e_pic; i++)
                SET_BIT(used_gfx_pics, i);

        pic_num++;
    }

    return changed;
}

/* ======================================================================== */
/*  DECODE_TIMER_TABLE   -- Decode list of periodically-executed functions  */
/* ======================================================================== */
int decode_timer_table(uint32_t addr)
{
    int changed = 0, i;

    if (!no_default_symbols)
        maybe_defsym(".TIMER", addr);

    for (; GET_WORD(addr) != 0; addr += 4)
    {
        changed += mark_interp(addr, FLAG_DBDATA, 4, "Timer dispatch/interval");
        for (i = 0; i < 4; i++)
            instr[addr + i].cmt_len = CMT_LONG;

        changed += add_entry_point(GET_DWORD(addr));
    }

    changed += mark_interp(addr, FLAG_DBDATA, 2, "End of timer table\n");
    instr[addr].cmt_len = CMT_LONG;

    return changed;
}


/* ======================================================================== */
/*  MARK_CART_HEADER_PRE -- Mark the cartridge header as known-data.        */
/*                          These are the steps we do "early."  The post    */
/*                          routine goes and backfills based on final info  */
/* ======================================================================== */
int mark_cart_header_pre(void)
{
    int addr, i;
    int changed = 0;
    int after_title_addr = 0;

    /* -------------------------------------------------------------------- */
    /*  First, prep the header to be marked as 'data' of various sorts.     */
    /* -------------------------------------------------------------------- */
    for (addr = 0x5000; addr < 0x5014; addr++)
    {
        if ((instr[addr].flags & FLAG_INVOP) == 0)
            changed++;
        instr[addr].flags &= ~MASK_DATA;
    }

    /* -------------------------------------------------------------------- */
    /*  The first 12 words are all BIDECLEs.                                */
    /* -------------------------------------------------------------------- */
    for (addr = 0x5000; addr < 0x500C; addr++)
        instr[addr].flags = FLAG_DBDATA | FLAG_INVOP | FLAG_INTERP;

    /* -------------------------------------------------------------------- */
    /*  The next several are DECLEs.                                        */
    /* -------------------------------------------------------------------- */
    for (addr = 0x500C; addr < 0x5014; addr++)
        instr[addr].flags = FLAG_DATA | FLAG_INVOP | FLAG_INTERP;

    /* -------------------------------------------------------------------- */
    /*  Slap comments on these guys.                                        */
    /* -------------------------------------------------------------------- */
    for (addr = 0x5000, i = 0; addr < 0x5014; addr += exec_hdr_dsc[i++].len)
    {
        instr[addr].cmt = exec_hdr_dsc[i].cmt;
        instr[addr].len = exec_hdr_dsc[i].len;
        instr[addr].cmt_len = CMT_LONG;
    }

    /* -------------------------------------------------------------------- */
    /*  Find the title string and cart year, and mark those words.          */
    /* -------------------------------------------------------------------- */
    addr = GET_DWORD(0x500A);
    if (!IS_EMPTY(addr))
    {
        if (!no_default_symbols)
            maybe_defsym(".TITLE", addr);

        /* ---------------------------------------------------------------- */
        /*  First word is 'year'.                                           */
        /* ---------------------------------------------------------------- */
        changed += mark_interp(addr, FLAG_DATA, 1, "Cartridge year");

        /* ---------------------------------------------------------------- */
        /*  Remaining words up to NUL are the title string.                 */
        /* ---------------------------------------------------------------- */
        instr[++addr].cmt = "Title string";
        while (addr < 0xFFFF && GET_WORD(addr) != 0 && !IS_EMPTY(addr))
        {
            instr[addr].flags &= ~MASK_DATA;
            changed += mark_invalid(addr);
            instr[addr].flags |= FLAG_INVOP | FLAG_STRING;
            addr++;
        }

        changed += mark_interp(addr++, FLAG_DATA, 1, "\n");

        after_title_addr = addr;
    }

    /* -------------------------------------------------------------------- */
    /*  Find the game entry point and code-after-title (if enabled), and    */
    /*  mark those as entry points.                                         */
    /* -------------------------------------------------------------------- */
    if ((GET_WORD(0x500C) & 0x80) && !IS_EMPTY(after_title_addr))
    {
        if (!no_default_symbols)
            maybe_defsym(".TITLECODE", addr);

        changed += add_entry_point(after_title_addr);
    }

    addr = GET_DWORD(0x5004);

    if (!IS_EMPTY(addr))
    {
        if (!no_default_symbols)
            maybe_defsym(".START", addr);

        changed += add_entry_point(addr);
    }

    /* -------------------------------------------------------------------- */
    /*  Go interpret the timer table.  Mark timer processes as entry pts.   */
    /* -------------------------------------------------------------------- */
    addr = GET_DWORD(0x5002);
    changed += decode_timer_table(addr);

    /* -------------------------------------------------------------------- */
    /*  Go interpret the GRAM initialization sequence.  This is a separate  */
    /*  function, as it's possible to call it w/ a different init list      */
    /*  outside header interpretation.                                      */
    /* -------------------------------------------------------------------- */
    changed += decode_gram_init(GET_DWORD(0x5008));

    return changed;
}


/* ======================================================================== */
/*  DECODE_GFX_LIST                                                         */
/* ======================================================================== */
int decode_gfx_list(uint32_t addr)
{
    int changed = 0;
    int i;
    char buf[32];

    snprintf(buf, sizeof(buf), ".GFX_LIST_%.4X", addr);
    if (!no_default_symbols)
        maybe_defsym(buf, addr);

    for (i = 0; i < 256; i++, addr += 8)
    {
        if (GET_BIT(used_gfx_pics, i) &&
            (instr[addr].flags & (FLAG_CODE | FLAG_INTERP)) == 0)
        {
            snprintf(buf, sizeof(buf), "CART PIC #%.2X", i);
            changed += mark_interp(addr,     FLAG_DATA, 4, strdup(buf));
            changed += mark_interp(addr + 4, FLAG_DATA, 4, "");
        }
    }

    return changed;
}


/* ======================================================================== */
/*  MARK_CART_HEADER_POST -- Finish marking up data spec'd by header.       */
/* ======================================================================== */
int mark_cart_header_post(void)
{
    int changed = 0;

    changed += decode_gfx_list(GET_DWORD(0x5006));

    return changed;
}

/* ======================================================================== */
/*  DECODE_EXEC_MUSIC -- Try to decode music played by EXEC music engine    */
/* ======================================================================== */
int decode_exec_music(void)
{
    uint32_t addr;
    int changed = 0;

    for (addr = 0; addr < 0xFFFD; addr++)
    {
        uint32_t targ;

        if (!IS_JSR(addr) || IS_INTERP(addr + 3))
            continue;

        targ = instr[addr].br_target;

        if (targ != 0x1B27 && targ != 0x1B5D &&
            targ != 0x1B95 && targ != 0x1A94)
            continue;

        addr += 3;

        while (addr < 0x10000 && !IS_EMPTY(addr))
        {
            if (GET_WORD(addr) >= 0x3F0)
            {
                changed += mark_interp(addr++, FLAG_DATA, 2, "Note (long)");
            } else if (GET_WORD(addr) == 0)
            {
                changed += mark_interp(addr, FLAG_DATA, 1, "End of music");
                break;
            } else
                changed += mark_interp(addr, FLAG_DATA, 1, "Note (short)");

            addr++;
        }
    }

    return changed;
}
/* opcode spaces:

0010 1111 O  xx00101111   One word   (STUNIT)
0100 1111 T  xx01001111   Two words  (SETR, long version)
0111 0111 T  0001110111   Three words (UCALL)
1100 1111 O  xx11001111   One word   (FIN)
x000 1111 T  0000001111   Two words  (NFREQV / PAUSE long dly)
x011 0111 O  xxx0110111   One word   (EUSE)

xx01 0111 T  xxxx010111   Two words  (RPAUSE)
xxx0 0111 O  xxxx000111   One word   (RNFREQ / RNFREQV)
xxxx 0001 T  xxxxxx0001   Two words  (FREQV)
xxxx 0011 T  xxxxx00011   Two words  (PDJNZ / RSETR)
xxxx 0101 O  xxxx000101   One word   (VOL)
xxxx 1001 O  xxxxxx1001   One word   (Enable)
xxxx 1011 O  xxxx001011   One word   (ECHAR / PAUSE short dly / RFREQ / RFREQV)
xxxx 1101 O  xxxxx01101   One word   (NFREQ / SETR short ver)
xxxx xx10 O  xxxx000010   One word   (VOLV)
xxxx xx00 T  xxxxxxxx00   Two words  (FREQ/EFREQ)

*/

struct sfx_ops_t { uint8_t mask, sigbits, len; };

static const struct sfx_ops_t sfx_ops[] =
{
    { 0xFF, 0x2F, 1 },
    { 0xFF, 0x4F, 2 },
    { 0xFF, 0x77, 3 },
    { 0xFF, 0xCF, 1 },
    { 0x7F, 0x0F, 2 },
    { 0x7F, 0x37, 1 },
    { 0x3F, 0x17, 2 },
    { 0x1F, 0x07, 1 },
    { 0x0F, 0x01, 2 },
    { 0x0F, 0x03, 2 },
    { 0x0F, 0x05, 1 },
    { 0x0F, 0x09, 1 },
    { 0x0F, 0x0B, 1 },
    { 0x0F, 0x0D, 1 },
    { 0x03, 0x02, 1 },
    { 0x03, 0x00, 2 },

    { 0, 0, 1 }
};

/* ======================================================================== */
/*  DECODE_EXEC_SFX -- Try to decode music played by EXEC sound fx engine   */
/* ======================================================================== */
int decode_exec_sfx(void)
{
    uint32_t addr;
    int changed = 0;
    int i;
    uint32_t ucall = 0xFFFFFFFF;

    for (addr = 0; addr < 0xFFFD; addr++)
    {
        uint32_t targ;

        if (!IS_JSR(addr) || IS_INTERP(addr + 3))
            continue;

        targ = instr[addr].br_target;

        if (targ != 0x1BBB && targ != 0x1BBE)
            continue;

        addr += 3;

        if (targ == 0x1BBE)
            changed += mark_interp(addr++, FLAG_DATA, 1, "SFX prio");

        while (addr < 0xFFFF && !IS_EMPTY(addr))
        {
            uint32_t word = GET_WORD(addr);

            if (addr >= ucall)  /* Some SCODE UCALLs into itself.  Skiing. */
                break;

            for (i = 0; sfx_ops[i].mask; i++)
                if ((word & sfx_ops[i].mask) == sfx_ops[i].sigbits)
                    break;

            if (word == 0x77)  /* UCALL */
            {
                uint32_t temp = GET_DWORD(addr + 1);
                char buf[64];

                changed += add_entry_point(temp);
                add_comment(temp, "SLIB UCALL target");
                sprintf(buf, "UC_%.4X", temp);
                symtab_defsym(symtab, buf, temp << 3);

                if (temp > addr && temp < ucall)
                    ucall = temp;
            }

            if (!sfx_ops[i].mask)
                changed += mark_interp(addr, FLAG_DATA, 1, "Unknown");
            else if ((word & 0xFF) == 0xCF)
            {
                changed += mark_interp(addr, FLAG_DATA, 1, "SFX end\n");
                break;
            }
            else
                changed += mark_interp(addr, FLAG_DATA, sfx_ops[i].len,
                                        "SFX data");

            addr += sfx_ops[i].len;
        }
    }

    return changed;
}

/* ======================================================================== */
/*  DECODE_PRINT_CALLS -- Scan for message-printing routines; Mark strings  */
/* ======================================================================== */
int decode_print_calls(void)
{
    uint32_t addr;
    int changed = 0;

    for (addr = 0; addr < 0xFFFD; addr++)
    {
        uint32_t targ;

        if (!IS_JSR(addr) || IS_INTERP(addr + 3))
            continue;

        targ = instr[addr].br_target;

        /* String after JSR:  0x187B, 0x1871 */
        /* String at R1:      0x1867, 0x186C */

        if (targ != 0x187B && targ != 0x1871 &&
            targ != 0x1867 && targ != 0x186C)
            continue;

        addr += 3;

        if (targ == 0x187B || targ == 0x1871)
        {
            changed += mark_string(addr);
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Eventually:  Use R1 value to mark string printed.  Req's pass   */
        /*  that chases code forward and determines "easy to compute"       */
        /*  values of registers.                                            */
        /* ---------------------------------------------------------------- */
        continue;
    }

    return changed;
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
/* ------------------------------------------------------------------------ */
/*                   Copyright (c) 2006, Joseph Zbiciak                     */
/* ======================================================================== */
