/* ======================================================================== */
/*  DIS-1600  Advanced(?) CP-1600 Disassembler.                             */
/*  By Joseph Zbiciak                                                       */
/* ------------------------------------------------------------------------ */
/*                   Copyright (c) 2006, Joseph Zbiciak                     */
/* ======================================================================== */

#include "dasm/dis1600.h"

icartrom_t temp_icart, icart;
symtab_t   *symtab;

/* ======================================================================== */
/*  INSTRUCTION MNEMONICS                                                   */
/* ======================================================================== */
const char *mnemonic[] =
{
    "err!",
    "HLT",  "SDBD", "EIS",  "DIS",          "TCI",  "CLRC", "SETC",
    "JSR",  "JSRE", "JSRD",         "J",    "JE",   "JD",
    "SWAP", "SLL",  "RLC",  "SLLC", "SLR",  "SAR",  "RRC",  "SARC",
    "B",    "BC",   "BOV",  "BPL",  "BEQ",  "BLT",  "BLE",  "BUSC",
    "NOPP", "BNC",  "BNOV", "BMI",  "BNEQ", "BGE",  "BGT",  "BESC",
    "BEXT",
            "MVO",  "MVI",  "ADD",  "SUB",  "CMP",  "AND",  "XOR",
            "MVOI", "MVII", "ADDI", "SUBI", "CMPI", "ANDI", "XORI",
            "MVO@", "MVI@", "ADD@", "SUB@", "CMP@", "AND@", "XOR@",
                    "MOVR", "ADDR", "SUBR", "CMPR", "ANDR", "XORR",
            "INCR", "DECR", "COMR", "NEGR", "ADCR",         "RSWD",
    "NOP",  "SIN",  "GSWD",

    "PSHR", "PULR", "CLRR", "TSTR",

    "DECLE", "BIDECLE", "STRING",
    "SKIP"
};

mnm_t mnm_dir_2op[8] =
{
    M_err,  M_MVO,  M_MVI,  M_ADD,  M_SUB,  M_CMP,  M_AND,  M_XOR
};

mnm_t mnm_ind_2op[8] =
{
    M_err,  M_MVO_, M_MVI_, M_ADD_, M_SUB_, M_CMP_, M_AND_, M_XOR_
};

mnm_t mnm_imm_2op[8] =
{
    M_err,  M_MVOI, M_MVII, M_ADDI, M_SUBI, M_CMPI, M_ANDI, M_XORI
};

mnm_t mnm_reg_2op[8] =
{
    M_err,  M_err,  M_MOVR, M_ADDR, M_SUBR, M_CMPR, M_ANDR, M_XORR
};

mnm_t mnm_rot_1op[8] =
{
    M_SWAP, M_SLL,  M_RLC,  M_SLLC, M_SLR,  M_SAR,  M_RRC,  M_SARC
};

mnm_t mnm_reg_1op[8] =
{
    M_err,  M_INCR, M_DECR, M_COMR, M_NEGR, M_ADCR, M_err,  M_RSWD
};

mnm_t mnm_cond_br[16] =
{
    M_B,    M_BC,   M_BOV,  M_BPL,  M_BEQ,  M_BLT,  M_BLE,  M_BUSC,
    M_NOPP, M_BNC,  M_BNOV, M_BMI,  M_BNEQ, M_BGE,  M_BGT,  M_BESC
};

mnm_t mnm_impl_1op_a[4] = { M_HLT,  M_SDBD, M_EIS,  M_DIS  };
mnm_t mnm_impl_1op_b[4] = { M_err,  M_TCI,  M_CLRC, M_SETC };

mnm_t mnm_jsr[8] =
{
    M_JSR,  M_JSRE, M_JSRD, M_err,  M_J,    M_JE,   M_JD,   M_err
};


/* ======================================================================== */
/*  INSTRUCTION PRINTER TABLE (forward references)                          */
/* ======================================================================== */
ins_prt_t *const instr_printer[] =
{
/*  "err!"  */
    prt_err,

/*  "HLT",  "SDBD", "EIS",  "DIS",          "TCI",  "CLRC", "SETC", */
    prt_imp,prt_imp,prt_imp,prt_imp,        prt_imp,prt_imp,prt_imp,

/*  "JSR",  "JSRE", "JSRD",         "J",    "JE",   "JD",           */
    prt_jsr,prt_jsr,prt_jsr,        prt_cbr,prt_cbr,prt_cbr,

/*  "SWAP", "SLL",  "RLC",  "SLLC", "SLR",  "SAR",  "RRC",  "SARC", */
    prt_rot,prt_rot,prt_rot,prt_rot,prt_rot,prt_rot,prt_rot,prt_rot,

/*  "B",    "BC",   "BOV",  "BPL",  "BEQ",  "BLT",  "BLE",  "BUSC", */
    prt_cbr,prt_cbr,prt_cbr,prt_cbr,prt_cbr,prt_cbr,prt_cbr,prt_cbr,
/*  "NOPP", "BNC",  "BNOV", "BMI",  "BNEQ", "BGE",  "BGT",  "BESC", */
    prt_imp,prt_cbr,prt_cbr,prt_cbr,prt_cbr,prt_cbr,prt_cbr,prt_cbr,
/*  "BEXT",                                                         */
    prt_bxt,

/*          "MVO",  "MVI",  "ADD",  "SUB",  "CMP",  "AND",  "XOR",  */
            prt_jsr,prt_dir,prt_dir,prt_dir,prt_dir,prt_dir,prt_dir,
/*          "MVOI", "MVII", "ADDI", "SUBI", "CMPI", "ANDI", "XORI", */
            prt_imo,prt_imm,prt_imm,prt_imm,prt_imm,prt_imm,prt_imm,
/*          "MVO@", "MVI@", "ADD@", "SUB@", "CMP@", "AND@", "XOR@", */
            prt_2rg,prt_2rg,prt_2rg,prt_2rg,prt_2rg,prt_2rg,prt_2rg,
/*                  "MOVR", "ADDR", "SUBR", "CMPR", "ANDR", "XORR", */
                    prt_2rg,prt_2rg,prt_2rg,prt_2rg,prt_2rg,prt_2rg,
/*          "INCR", "DECR", "COMR", "NEGR", "ADCR",         "RSWD", */
            prt_1rg,prt_1rg,prt_1rg,prt_1rg,prt_1rg,        prt_1rg,
/*  "NOP",  "SIN",  "GSWD",                                         */
    prt_imp,prt_imp,prt_1rg,

/*  "PSHR", "PULR", "CLRR", "TSTR"                                  */
    prt_1rg,prt_plr,prt_1rg,prt_1rg,

/*  "DECLE", "BIDECLE", "STRING"                                    */
    prt_dcl, prt_bid,    prt_str,

/*  "SKIP"                                                          */
    prt_imp
};


/* ======================================================================== */
/*  Declare storage for the ROM image.                                      */
/* ======================================================================== */
dis_instr_t instr[65539];  /* Yes, an instr for every address.  why not? */


/* ======================================================================== */
/* ======================================================================== */
/*  DEFAULT SYMBOL TABLE                                                    */
/* ======================================================================== */
/* ======================================================================== */

struct defsym_t defsym[] =
{
    { ".PSG0.chn_a_lo",         0x01F0,     1,          1   },
    { ".PSG0.chn_b_lo",         0x01F1,     1,          1   },
    { ".PSG0.chn_c_lo",         0x01F2,     1,          1   },
    { ".PSG0.envlp_lo",         0x01F3,     1,          1   },
    { ".PSG0.chn_a_hi",         0x01F4,     1,          1   },
    { ".PSG0.chn_b_hi",         0x01F5,     1,          1   },
    { ".PSG0.chn_c_hi",         0x01F6,     1,          1   },
    { ".PSG0.envlp_hi",         0x01F7,     1,          1   },
    { ".PSG0.chan_enable",      0x01F8,     1,          1   },
    { ".PSG0.noise",            0x01F9,     1,          1   },
    { ".PSG0.envelope",         0x01FA,     1,          1   },
    { ".PSG0.chn_a_vol",        0x01FB,     1,          1   },
    { ".PSG0.chn_b_vol",        0x01FC,     1,          1   },
    { ".PSG0.chn_c_vol",        0x01FD,     1,          1   },
    { ".PSG0.rgt_hand",         0x01FE,     1,          1   },
    { ".PSG0.lft_hand",         0x01FF,     1,          1   },


    { ".PSG1.chn_a_lo",         0x00F0,     1,          1   },
    { ".PSG1.chn_b_lo",         0x00F1,     1,          1   },
    { ".PSG1.chn_c_lo",         0x00F2,     1,          1   },
    { ".PSG1.envlp_lo",         0x00F3,     1,          1   },
    { ".PSG1.chn_a_hi",         0x00F4,     1,          1   },
    { ".PSG1.chn_b_hi",         0x00F5,     1,          1   },
    { ".PSG1.chn_c_hi",         0x00F6,     1,          1   },
    { ".PSG1.envlp_hi",         0x00F7,     1,          1   },
    { ".PSG1.chan_enable",      0x00F8,     1,          1   },
    { ".PSG1.noise",            0x00F9,     1,          1   },
    { ".PSG1.envelope",         0x00FA,     1,          1   },
    { ".PSG1.chn_a_vol",        0x00FB,     1,          1   },
    { ".PSG1.chn_b_vol",        0x00FC,     1,          1   },
    { ".PSG1.chn_c_vol",        0x00FD,     1,          1   },
    { ".PSG1.rgt_hand",         0x00FE,     1,          1   },
    { ".PSG1.lft_hand",         0x00FF,     1,          1   },

    { ".ISRVEC",                0x0100,     2,          1   },
  /*{ ".RAM",                   0x0100,     0x00F0,     2   },*/ /* annoying */
    { ".BTAB",                  0x0200,     0x00F0,     2   },
  /*{ ".SYS",                   0x02F0,     0x0070,     2   },*/ /* annoying */
    { ".GRAM",                  0x3800,     0x0800,     3   },
    { ".GROM",                  0x3000,     0x0800,     3   },

    { ".STIC.X",                0x0000,     8,          1   },
    { ".STIC.Y",                0x0008,     8,          1   },
    { ".STIC.A",                0x0010,     8,          1   },
    { ".STIC.C",                0x0018,     8,          1   },
    { ".STIC.VIDEN",            0x0020,     1,          1   },
    { ".STIC.MODE",             0x0021,     1,          1   },
    { ".STIC.CS",               0x0028,     4,          1   },
    { ".STIC.BORD",             0x002C,     1,          1   },
    { ".STIC.HDLY",             0x0030,     1,          1   },
    { ".STIC.VDLY",             0x0031,     1,          1   },
    { ".STIC.EDGE",             0x0032,     1,          1   },

    { ".HEADER",                0x5000,     1,          1   },

    { ".ISRRET",                0x1014,     1,          1   },
    { ".EXEC",                  0x1000,     0x1000,     3   },

    { ".UART",                  0x00E0,     4,          1   },
    { ".ECSRAM",                0x4000,     0x0800,     3   },

    { ".IV.ALD",                0x0080,     1,          1   },
    { ".IV.FIFO",               0x0081,     1,          1   },

    { ".ECSCBL.POLL",           0xCF01,     1,          1   },
    { ".ECSCBL",                0xCF00,     0x100,      2   },

    { NULL,                     0,          0,          0   }
};


/* ======================================================================== */
/*  MAYBE_DEFSYM     -- Helper:  Define a symbol if it's not def'd yet      */
/* ======================================================================== */
void maybe_defsym(const char *sym, uint32_t addr)
{
    uint32_t unused;

    addr <<= 3;

    /* -------------------------------------------------------------------- */
    /*  Make sure there is no other symbol at this address and that this    */
    /*  name isn't already taken.                                           */
    /* -------------------------------------------------------------------- */
    if (symtab_getsym (symtab, addr, 0, 0) == NULL &&  /* name not found?   */
        symtab_getaddr(symtab, sym, &unused) != 0)     /* addr not found?   */
        symtab_defsym (symtab, sym, addr);             /* def it!           */
}

/* ======================================================================== */
/*  SETUP_DEFSYM     -- Set up the default symbol table.                    */
/* ======================================================================== */
void setup_defsym(void)
{
    int i, j;
    int addr;
    char *symbuf = NULL;
    size_t symbuf_sz = 0;
    const char *sym;

    for (i = 0; defsym[i].name; i++)
    {
        for (j = 0; j < defsym[i].len; j++)
        {
            addr = (defsym[i].addr + j);

            sym = defsym[i].name;
            if (defsym[i].len > 1)
            {
                size_t symlen = strlen(defsym[i].name) + defsym[i].width + 2;
                if (symlen > symbuf_sz)
                {
                    symbuf_sz = symlen * 2;
                    symbuf = (char *)realloc(symbuf, symlen * 2);
                    if (!symbuf)
                    {
                        fprintf(stderr, "Out of memory in setup_defsym()\n");
                        exit(1);
                    }
                }
                snprintf(symbuf, symbuf_sz, 
                         "%s.%.*X", defsym[i].name, defsym[i].width, j);
                sym = symbuf;
            }

            maybe_defsym(sym, addr);
        }
    }

    if (symbuf)
    {
        free(symbuf);
    }
}


/* ======================================================================== */
/* ======================================================================== */
/*  ANALYSIS PASSES AND DECODER FUNCTIONS                                   */
/* ======================================================================== */
/* ======================================================================== */
int hlt_is_invalid                = 1;
int rare_ops_are_invalid          = 1;
int allow_branch_target_wrap      = 0;
int allow_branch_to_bad_addr      = 0;
int suspicious_pc_math_is_invalid = 1;
int verbose                       = 0;
int skip_advanced_analysis        = 0;
int skip_mark_cart_header         = 0;
int skip_funky_branch_detect      = 0;
int skip_propagate_invalid        = 0;
int skip_kill_bad_branches        = 0;
int skip_brtrg_vs_sdbd            = 0;
int skip_find_jsr_data            = 0;
int skip_mark_args_invalid        = 0;
int skip_exec_sound_interp        = 0;
int skip_exec_print               = 0;
int dont_loop_analysis            = 0;

int allow_global_branches         = 0;
int no_exec_branches              = 0;

int debug_find_jsr_data           = 0;
int debug_show_instr_flags        = 0;
int no_default_symbols            = 0;
int no_exec_routine_symbols       = 0;
int forced_entry_points           = 0;
int forced_data_ranges            = 0;
int generic_labels                = 0;

uint32_t entry_point  [MAX_ENTRY];
uint32_t data_range_lo[MAX_ENTRY];
uint32_t data_range_hi[MAX_ENTRY];
uint32_t generic_label[MAX_ENTRY];

/* ======================================================================== */
/*  REMOVE_SDBD           -- Remove the SDBD from an instruction.           */
/* ======================================================================== */
LOCAL int remove_sdbd(uint32_t addr)
{
    if (!(instr[addr].flags & FLAG_SDBD))
        return 0;

    if ((instr[addr].flags & FLAG_SDBD) && instr[addr].len == 3)
    {
        instr[addr].len = 2;
        instr[addr].op1.op &= 0xFF;
        instr[addr].flags &= ~FLAG_SDBD;
        return 1;
    }

    return 0;
}

/* ======================================================================== */
/*  ADD_COMMENT  -- Add / replace comment on an instruction                 */
/* ======================================================================== */
int add_comment(uint32_t addr, const char *cmt)
{
    instr[addr].cmt = cmt;
    return 0;
}

/* ======================================================================== */
/*  MARK_INVALID -- Mark an address as invalid.                             */
/* ======================================================================== */
int mark_invalid(uint32_t addr)
{
    int changed = 0;

    if (instr[addr].flags & FLAG_FORCED)
        return 0;

    changed = (instr[addr].flags & FLAG_INVOP) == 0;
    instr[addr].flags |= FLAG_INVOP;
    instr[addr].flags &= ~MASK_CODE;

    changed += remove_sdbd((addr + 1) & 0xFFFF);

    return changed;
}

/* ======================================================================== */
/*  MARK_VALID -- Mark an address as valid, w/ optional add'l flags.        */
/* ======================================================================== */
int mark_valid(uint32_t addr, uint32_t flags)
{
    uint32_t old_flags = instr[addr].flags;
    instr[addr].flags &= MASK_CODE;
    instr[addr].flags |= flags;
    return old_flags != instr[addr].flags;
}

/* ======================================================================== */
/*  MARK_INTERP -- Mark address range as interpreted, w/ opt flags & cmts.  */
/* ======================================================================== */
int mark_interp(uint32_t addr, uint32_t flags, int len, const char *cmt)
{
    int i;
    int changed = 0;

    for (i = 0; i < len; i++)
    {
        uint32_t old_flags = instr[addr + i].flags;

        if (flags & MASK_DATA)
            instr[addr + 1].flags &= ~MASK_DATA;

        instr[addr + i].flags &= ~MASK_CODE;
        instr[addr + i].flags |= flags | FLAG_INTERP | FLAG_INVOP;
        instr[addr + i].len    = len - i;
        instr[addr + i].cmt    = cmt;

//printf("addr=%.4X  flags %.8X to %.8X\n", addr, old_flags, instr[addr+i].flags);
        changed += ((old_flags ^ instr[addr + i].flags) & (FLAG_INTERP|FLAG_INVOP)) != 0;
    }

    return changed;
}

/* ======================================================================== */
/*  MARK_EMPTY -- Mark an address range as not holding "local code."        */
/* ======================================================================== */
void mark_empty(int addr_lo, int addr_hi)
{
    int addr;

    for (addr = addr_lo; addr <= addr_hi; addr++)
        instr[addr].flags |= FLAG_EMPTY;
}

/* ======================================================================== */
/*  MARK_STRING -- Given a starting address, mark locations until a NUL.    */
/* ======================================================================== */
int mark_string(int addr)
{
    int changes = 0;

    while (!IS_EMPTY(addr) && (instr[addr].flags & FLAG_CODE) == 0)
    {
        uint32_t old_flags = instr[addr].flags;

        if (GET_WORD(addr) == 0)
            break;

        instr[addr].flags = (old_flags & ~(MASK_CODE|MASK_DATA|FLAG_INTERP))
                            | FLAG_INVOP | FLAG_STRING;

        changes += old_flags != instr[addr].flags;

        addr++;
    }

    return changes;
}


/* ======================================================================== */
/*  MARK_BRANCH_TARGET -- Mark a location as a branch target.               */
/* ======================================================================== */
int mark_branch_target(int target, int target_of)
{
    int i;

    instr[target].flags |= FLAG_BRTRG;

    if (!instr[target].target_of)
    {
        instr[target].target_of = (int *)calloc(4, sizeof(int));
        instr[target].tg_of_max = 4;
        instr[target].tg_of_cnt = 0;
    }

    for (i = 0; i < instr[target].tg_of_cnt; i++)
        if (instr[target].target_of[i] == target_of)
            return 0;

    if (i >= instr[target].tg_of_max)
    {
        instr[target].tg_of_max <<= 1;
        instr[target].target_of = (int *)realloc(instr[target].target_of,
                                          instr[target].tg_of_max*sizeof(int));
    }

    instr[target].target_of[i] = target_of;
    instr[target].tg_of_cnt++;

    return 1;
}

/* ======================================================================== */
/*  ADD_ENTRY_POINT -- List a location as a ROM entry point.                */
/* ======================================================================== */
int add_entry_point(uint32_t addr)
{
    int i;

    for (i = 0; i < forced_entry_points; i++)
        if (entry_point[i] == addr)
            return 0;

    if (forced_entry_points == MAX_ENTRY)
    {
        fprintf(stderr, "Too many entry points (max %d)\n", MAX_ENTRY);
        exit(1);
    }

    entry_point[forced_entry_points++] = addr;

    VB_PRINTF(0, ("Added $%.4X as an entry point\n", addr));
    return 1;
}

/* ======================================================================== */
/*  ADD_DATA_RANGE  -- List range of locations as data.                     */
/* ======================================================================== */
LOCAL int add_data_range(uint32_t addr_lo, uint32_t addr_hi)
{
    if (addr_lo > 0xFFFF)
        return 0;

    if (addr_hi > 0xFFFF)
        addr_hi = addr_lo;

    if (forced_data_ranges == MAX_ENTRY)
    {
        fprintf(stderr, "Too many data ranges (max %d)\n", MAX_ENTRY);
        exit(1);
    }

    if ( addr_lo > addr_hi )
    {
        uint32_t t = addr_lo; addr_lo = addr_hi; addr_hi = t;
    }

    data_range_lo[forced_data_ranges  ] = addr_lo;
    data_range_hi[forced_data_ranges++] = addr_hi;

    VB_PRINTF(0, ("Added $%.4X - $%.4X as a data_range\n", addr_lo, addr_hi));
    return 1;
}

/* ======================================================================== */
/*  ADD_GENERIC_LABEL                                                       */
/* ======================================================================== */
LOCAL int add_generic_label(uint32_t addr)
{
    if (generic_labels == MAX_ENTRY)
    {
        fprintf(stderr, "Too many generic labels (max %d)\n", MAX_ENTRY);
        exit(1);
    }

    generic_label[generic_labels++] = addr;

    VB_PRINTF(0, ("Added generic label at $%.4X\n", addr));
    return 1;
}

/* ======================================================================== */
/*  DIS_JUMP -- decode J/JE/JD/JSR/JSRE/JSRD                                */
/* ======================================================================== */
LOCAL void dis_jump      (uint32_t addr)
{
    uint16_t w0, w1, w2;
    uint16_t btarg;
    int      ret_reg;
    int      int_mode;

    /* -------------------------------------------------------------------- */
    /*  Make sure all three words are in implemented memory and are 10-bit. */
    /* -------------------------------------------------------------------- */
    if (BAD_ARG10(addr + 1) || BAD_ARG10(addr + 2))
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Get all three words of the jump instruction.                        */
    /* -------------------------------------------------------------------- */
    w0 = GET_WORD(addr);
    w1 = GET_WORD(addr + 1);
    w2 = GET_WORD(addr + 2);

    assert(w0 == 0x0004);

    /* -------------------------------------------------------------------- */
    /*  Decode "interrupt mode".  This is in bits 0 and 1 of word 2.        */
    /*      00 means "No Change."                                           */
    /*      01 means "Jump and Enable Interrupts"                           */
    /*      10 means "Jump and Disable Interrupts"                          */
    /*      11 is an invalid opcode.                                        */
    /* -------------------------------------------------------------------- */
    int_mode = (w1 & 3);
    if (int_mode == 3)
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Decode return register.  This is in bits 8 and 9 of 2nd word.       */
    /*      00 means R4         10 means R6                                 */
    /*      01 means R5         11 means "Do not save return address."      */
    /* -------------------------------------------------------------------- */
    ret_reg = 4 + ((w1 & 0x300) >> 8);

    /* -------------------------------------------------------------------- */
    /*  Decode branch target.  Lower 10 bits come from third word.  Upper   */
    /*  6 bits come from bits 2 thru 7 of second word.                      */
    /* -------------------------------------------------------------------- */
    btarg = ((w1 & 0x0FC) << 8) | (w2 & 0x3FF);

    /* -------------------------------------------------------------------- */
    /*  Generate the decoded instruction.                                   */
    /* -------------------------------------------------------------------- */
    instr[addr].len       = 3;
    instr[addr].br_target = btarg;
    instr[addr].flags    |= FLAG_BRANCH;

    if (ret_reg < 7)  /* JSR/JSRE/JSRD */
    {
        instr[addr].op1.type  = OP_REG;
        instr[addr].op1.op    = ret_reg;
        instr[addr].op1.flags = OPF_DST;

        instr[addr].op2.type  = OP_BRTRG;
        instr[addr].op2.op    = btarg;
        instr[addr].op2.flags = OPF_SRC | OPF_ADDR;

        instr[addr].flags    |= FLAG_JSR;

        instr[addr].mnemonic  = mnm_jsr[int_mode];
    } else            /* J/JE/JD */
    {
        instr[addr].op1.type  = OP_BRTRG;
        instr[addr].op1.op    = btarg;
        instr[addr].op1.flags = OPF_SRC | OPF_ADDR;

        instr[addr].mnemonic  = mnm_jsr[int_mode + 4];
    }
}

/* ======================================================================== */
/*  DIS_IMPL_1OP_A -- decode HLT/SDBD/EIS/DIS                               */
/* ======================================================================== */
LOCAL void dis_impl_1op_a(uint32_t addr)
{
    uint32_t w0, w1;

    /* -------------------------------------------------------------------- */
    /*  Make sure it's a valid opcode.                                      */
    /* -------------------------------------------------------------------- */
    if ((w0 = GET_WORD(addr)) > 3)
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  If this is a HLT instruction, mark it invalid if asked to do so.    */
    /* -------------------------------------------------------------------- */
    if (hlt_is_invalid && w0 == 0x000)
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  If this is an SDBD, make sure the instruction it modifies is an     */
    /*  indirect mode or immediate mode instruction that isn't MVO.         */
    /* -------------------------------------------------------------------- */
    if (w0 == 0x001)
    {
        w1 = GET_WORD(addr + 1);

        /* ---------------------------------------------------------------- */
        /*  The '(w1 & 0x3C0) < 0x280' makes sure it's MVI, ADD, SUB, CMP   */
        /*  AND or XOR.  The '(w1 & 0x038) < 0x008' makes sure it's not a   */
        /*  direct-mode instruction.                                        */
        /* ---------------------------------------------------------------- */
        if (NOT_CODE(addr + 1) || (w1 & 0x3C0) < 0x280 || (w1 & 0x038) < 0x008)
        {
            mark_invalid(addr);
            return;
        }
        /* ---------------------------------------------------------------- */
        /*  Go ahead and mark the next instruction as being modified by     */
        /*  SDBD.  We may revise this later once we discover br targets.    */
        /* ---------------------------------------------------------------- */
        instr[0xFFFF & (addr + 1)].flags |= FLAG_SDBD;
    }

    /* -------------------------------------------------------------------- */
    /*  Copy in the mnemonic and set the length.                            */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic = mnm_impl_1op_a[w0];
    instr[addr].len = 1;
}

/* ======================================================================== */
/*  DIS_IMPL_1OP_B -- decode TCI/CLRC/SETC                                  */
/* ======================================================================== */
LOCAL void dis_impl_1op_b(uint32_t addr)
{
    uint32_t w0;

    /* -------------------------------------------------------------------- */
    /*  Nuke invalid ops, and TCI if we're treating rare ops as invalid.    */
    /* -------------------------------------------------------------------- */
    if ((w0 = GET_WORD(addr)) < 5 || w0 > 7 ||
        (rare_ops_are_invalid && w0 == 0x005))  /* TCI */
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Copy in the mnemonic and set the length.                            */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic = mnm_impl_1op_b[w0 - 4];
    instr[addr].len = 1;
}

/* ======================================================================== */
/*  DIS_NOP_SIN -- Decode NOP and SIN instructions.                         */
/* ======================================================================== */
LOCAL void dis_nop_sin   (uint32_t addr)
{
    uint32_t w0;

    /* -------------------------------------------------------------------- */
    /*  If LSB != 0, treat it as invalid opcode no matter what.  Also, if   */
    /*  we're treating "rare as invalid", treat all SIN as invalid.         */
    /* -------------------------------------------------------------------- */
    w0 = GET_WORD(addr);

    if ((w0 & 1) ||
        (rare_ops_are_invalid && (w0 & 3) >= 2))
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Copy in the mnemonic and set the length.                            */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic = w0 & 2 ? M_SIN : M_NOP;
    instr[addr].len = 1;
}

/* ======================================================================== */
/*  DIS_GSWD    -- Decode Get Status WorD instructions.                     */
/* ======================================================================== */
LOCAL void dis_gwsd      (uint32_t addr)
{
    /* -------------------------------------------------------------------- */
    /*  Dirt simple.  One mnemonic.  Bits 0..1 are the destination.         */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic  = M_GSWD;
    instr[addr].op1.type  = OP_REG;
    instr[addr].op1.op    = GET_WORD(addr) & 3;
    instr[addr].op1.flags = OPF_DST;
    instr[addr].len = 1;
}

/* ======================================================================== */
/*  DIS_REG_1OP -- Decode single-operand register instructions.             */
/* ======================================================================== */
LOCAL void dis_reg_1op   (uint32_t addr)
{
    uint32_t w0, op, dst;

    /* -------------------------------------------------------------------- */
    /*  Bits 0..2 are the destination (R0..R7).  Bits 3..5 are the opcode.  */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    op  = (w0 >> 3) & 7;
    dst =  w0       & 7;

    /* -------------------------------------------------------------------- */
    /*  Opcode values 000 and 011 are invalid -- these alias other opcode   */
    /*  spaces.  We only check for them here for debug purposes.            */
    /* -------------------------------------------------------------------- */
    if (op == 0x000 || op == 0x006)
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Build the decoded instruction.                                      */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic  = mnm_reg_1op[op];
    instr[addr].op1.type  = OP_REG;
    instr[addr].op1.op    = dst;
    instr[addr].op1.flags = OPF_SRCDST;
    instr[addr].len = 1;
}

/* ======================================================================== */
/*  DIS_ROT_1OP -- Decode rotates and shifts.                               */
/* ======================================================================== */
LOCAL void dis_rot_1op   (uint32_t addr)
{
    uint32_t w0, op, reg, amt;

    /* -------------------------------------------------------------------- */
    /*  Bits 0..1 are the destination (R0 thru R3).  Bit 2 determines       */
    /*  whether we shift by 1 or by 2.  Bits 3..5 are the opcode.           */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    reg = (w0 >> 0) & 3;        /* R0 thru R3   */
    amt = (w0 >> 2) & 1;        /* ,1 or ,2     */
    op  = (w0 >> 3) & 7;        /* which opcode */

    /* -------------------------------------------------------------------- */
    /*  Build the instruction.                                              */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic  = mnm_rot_1op[op];

    instr[addr].op1.type  = OP_REG;
    instr[addr].op1.op    = reg;
    instr[addr].op1.flags = OPF_SRCDST;

    instr[addr].op2.type  = OP_IMMED;
    instr[addr].op2.op    = amt + 1;
    instr[addr].op2.flags = OPF_SRC;

    instr[addr].len = 1;
}


/* ======================================================================== */
/*  DIS_REG_2OP -- Decode MOVR/ADDR/SUBR/CMPR/ANDR/XORR.                    */
/* ======================================================================== */
LOCAL void dis_reg_2op   (uint32_t addr)
{
    uint32_t w0, src, dst, op;
    mnm_t m;

    /* -------------------------------------------------------------------- */
    /*  Bits 0..2 are the source, bits 3..5 are the dest, and bits 6..8     */
    /*  are the opcode.                                                     */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    dst = (w0 >> 0) & 7;
    src = (w0 >> 3) & 7;
    op  = (w0 >> 6) & 7;

    /* -------------------------------------------------------------------- */
    /*  Opcodes 000 and 001 are illegal -- they alias other opcode spaces.  */
    /* -------------------------------------------------------------------- */
    if (op < 2)
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Build the instruction.                                              */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic  = m = mnm_reg_2op[op];

    instr[addr].op1.type  = OP_REG;
    instr[addr].op1.op    = src;
    instr[addr].op1.flags = OPF_SRC;

    instr[addr].op2.type  = OP_REG;
    instr[addr].op2.op    = dst;
    instr[addr].op2.flags = m == M_MOVR ? OPF_DST :
                            m == M_CMPR ? OPF_SRC : OPF_SRCDST;

    if (m == M_MOVR && src == dst)
        instr[addr].mnemonic = M_TSTR;

    if (m == M_XORR && src == dst)
        instr[addr].mnemonic = M_CLRR;

    instr[addr].len      = 1;
}

/* ======================================================================== */
/*  DIS_COND_BR -- Decode all "internal" conditional branches, and NOPP.    */
/* ======================================================================== */
LOCAL void dis_cond_br   (uint32_t addr)
{
    uint32_t w0, cnd, ofs, dir, trg;

    /* -------------------------------------------------------------------- */
    /*  Make sure target word is in implemented memory.                     */
    /* -------------------------------------------------------------------- */
    if (IS_EMPTY(addr + 1))
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Bits 0..3 are the condition code.  Bit 5 determines branch target   */
    /*  direction.  Displacement is in second word.                         */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    cnd = (w0 >> 0) & 0xF;
    dir = (w0 >> 5) & 1;
    ofs = GET_WORD(addr + 1);
    trg = addr + 2 + (dir ? ~ofs : ofs);

    /* -------------------------------------------------------------------- */
    /*  If NOPP has a non-zero offset but positive direction, treat it as   */
    /*  a 'SKIP' instruction that's intended to skip a 1-word following     */
    /*  instruction.  If dir is non-zero, mark it invalid.                  */
    /* -------------------------------------------------------------------- */
    if (cnd == 0x8)
    {
        if (dir != 0)
        {
            mark_invalid(addr);
            return;
        }
        if (ofs != 0)   /* treat it as a 'SKIP' instruction instead. */
        {
            instr[addr].mnemonic = M_SKIP;
            instr[addr].len      = 1;
            return;
        }
        instr[addr].mnemonic = M_NOPP;
        instr[addr].len      = 2;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Check to make sure branch target doesn't wrap, unless user has      */
    /*  explicitly allowed it.                                              */
    /* -------------------------------------------------------------------- */
    if (cnd != 0x8 && !allow_branch_target_wrap && (trg & 0xFFFF0000))
    {
        mark_invalid(addr);
        return;
    }

    trg &= 0xFFFF;

    /* -------------------------------------------------------------------- */
    /*  Build the instruction.                                              */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic      = mnm_cond_br[cnd];

    if (cnd != 0x8)
    {
        instr[addr].op1.type  = OP_BRTRG;
        instr[addr].op1.op    = trg;
        instr[addr].op1.flags = OPF_SRC | OPF_ADDR;

        instr[addr].flags    |= FLAG_BRANCH;
        instr[addr].br_target = trg;

        /* ---------------------------------------------------------------- */
        /*  All branches except "B" are conditional branches.               */
        /* ---------------------------------------------------------------- */
        if (cnd != 0x0)
            instr[addr].flags |= FLAG_CONDBR;
    }

    instr[addr].len      = 2;
}

/* ======================================================================== */
/*  DIS_BEXT    -- Decode external-condition branches.                      */
/* ======================================================================== */
LOCAL void dis_bext      (uint32_t addr)
{
    uint32_t w0, cnd, ofs, dir, trg;

    /* -------------------------------------------------------------------- */
    /*  Treat BEXT as 'rare' opcode, since it shouldn't appear in any       */
    /*  Intellivision program.                                              */
    /* -------------------------------------------------------------------- */
    if (rare_ops_are_invalid || IS_EMPTY(addr + 1))
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Make sure target word is in implemented memory.                     */
    /* -------------------------------------------------------------------- */
    if (IS_EMPTY(addr + 1))
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Bits 0..3 are the condition code.  Bit 5 determines branch target   */
    /*  direction.  Displacement is in second word.                         */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    cnd = (w0 >> 0) & 0xF;
    dir = (w0 >> 5) & 1;
    ofs = GET_WORD(addr + 1);
    trg = addr + 2 + (dir ? ~ofs : ofs);

    /* -------------------------------------------------------------------- */
    /*  Check to make sure branch target doesn't wrap, unless user has      */
    /*  explicitly allowed it.                                              */
    /* -------------------------------------------------------------------- */
    if (!allow_branch_target_wrap && (trg & 0xFFFF0000))
    {
        mark_invalid(addr);
        return;
    }
    trg &= 0xFFFF;

    /* -------------------------------------------------------------------- */
    /*  Build the instruction.                                              */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic  = M_BEXT;

    instr[addr].op1.type  = OP_IMMED;
    instr[addr].op1.op    = cnd;
    instr[addr].op1.flags = OPF_SRC;

    instr[addr].op2.type  = OP_BRTRG;
    instr[addr].op2.op    = trg;
    instr[addr].op2.flags = OPF_SRC | OPF_ADDR;

    instr[addr].flags    |= FLAG_BRANCH | FLAG_CONDBR;
    instr[addr].br_target = trg;

    instr[addr].len      = 2;
}

/* ======================================================================== */
/*  DIS_IMM_2OP -- Decode immediate mode instructions                       */
/* ======================================================================== */
LOCAL void dis_imm_2op   (uint32_t addr)
{
    uint32_t w0, w1, w2, op, dst, imm;
    mnm_t    m;
    int      is_sdbd = 0;

    /* -------------------------------------------------------------------- */
    /*  Destination is in bits 0..2.  Opcode is in bits 6..8.               */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    op  = (w0 >> 6) & 7;
    dst = (w0 >> 0) & 7;

    /* -------------------------------------------------------------------- */
    /*  Chuck opcode 000, as it's illegal.  Also, if opcode is MVOI (001)   */
    /*  and we treat 'rare' as 'illegal', then chuck it as well.            */
    /* -------------------------------------------------------------------- */
    if (op == 0 || (rare_ops_are_invalid && op == 1))
    {
        mark_invalid(addr);
        if (instr[addr].flags & FLAG_SDBD)
            mark_invalid(0xFFFF & (addr - 1));
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Read our immediate constant.                                        */
    /*                                                                      */
    /*  We examine our own "FLAG_SDBD" to decide if our immediate constant  */
    /*  spans 1 or 2 words.  If FLAG_SDBD is set, but we fail validity      */
    /*  tests, we clear FLAG_SDBD and mark the SDBD as invalid code.        */
    /* -------------------------------------------------------------------- */
    w1  = GET_WORD(addr + 1);
    w2  = GET_WORD(addr + 2);
    imm = w1;    /* default is that word 1 is the immediate operand */

    if (IS_EMPTY(addr + 1))
    {
        mark_invalid(addr);
        if (instr[addr].flags & FLAG_SDBD)
            mark_invalid(0xFFFF & (addr - 1));
        return;
    }

    if (instr[addr].flags & FLAG_SDBD)
    {
        /* ---------------------------------------------------------------- */
        /*  SDBD is valid only if:                                          */
        /*   -- second word is in implemented memory.                       */
        /*   -- both words are empty in bits 8..15.                         */
        /*   -- instruction is not MVOI (opcode 001).                       */
        /* ---------------------------------------------------------------- */
        if (op == 1 || IS_EMPTY(addr + 2) || (w1 & 0xFF00) || (w2 & 0xFF00))
        {
            mark_invalid(0xFFFF & (addr - 1));
            instr[addr].flags &= ~FLAG_SDBD;
        } else
        {
            imm = (w1 & 0x00FF) | ((w2 << 8) & 0xFF00);
            is_sdbd = 1;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Build the instruction.                                              */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic = m = mnm_imm_2op[op];
    instr[addr].len = 2 + is_sdbd;

    if (m == M_MVOI)
    {
        instr[addr].op1.type  = OP_REG;
        instr[addr].op1.op    = dst;
        instr[addr].op1.flags = OPF_SRC;

        instr[addr].op2.type  = OP_IMMED;
        instr[addr].op2.op    = imm;
        instr[addr].op2.flags = OPF_DST;
    } else
    {
        instr[addr].op1.type  = OP_IMMED;
        instr[addr].op1.op    = imm;
        instr[addr].op1.flags = OPF_SRC;

        instr[addr].op2.type  = OP_REG;
        instr[addr].op2.op    = dst;
        instr[addr].op2.flags = m == M_MVII ? OPF_DST :
                                m == M_CMPI ? OPF_SRC : OPF_SRCDST;
    }
}

/* ======================================================================== */
/*  DIS_DIR_2OP -- Decode direct mode instructions                          */
/* ======================================================================== */
LOCAL void dis_dir_2op   (uint32_t addr)
{
    uint32_t w0, w1, op, dst;
    mnm_t    m;

    /* -------------------------------------------------------------------- */
    /*  Destination is in bits 0..2.  Opcode is in bits 6..8.               */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    w1  = GET_WORD(addr + 1); /* direct address */
    op  = (w0 >> 6) & 7;
    dst = (w0 >> 0) & 7;

    /* -------------------------------------------------------------------- */
    /*  SDBD is always illegal for direct-mode.                             */
    /* -------------------------------------------------------------------- */
    if (instr[addr].flags & FLAG_SDBD)
        mark_invalid(0xFFFF & (addr - 1));

    /* -------------------------------------------------------------------- */
    /*  Chuck opcode 000, as it's illegal.                                  */
    /*  Also, make sure our direct address is legal.                        */
    /* -------------------------------------------------------------------- */
    if (op == 0 || IS_EMPTY(addr + 1))
    {
        mark_invalid(addr);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Build the instruction.                                              */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic = m = mnm_dir_2op[op];
    instr[addr].len = 2;

    if (m == M_MVO)  /* MVO */
    {
        instr[addr].op1.type  = OP_REG;
        instr[addr].op1.op    = dst;
        instr[addr].op1.flags = OPF_SRC;

        instr[addr].op2.type  = OP_DIRADDR;
        instr[addr].op2.op    = w1;
        instr[addr].op2.flags = OPF_DST | OPF_ADDR;
    } else
    {
        instr[addr].op1.type  = OP_DIRADDR;
        instr[addr].op1.op    = w1;
        instr[addr].op1.flags = OPF_SRC | OPF_ADDR;

        instr[addr].op2.type  = OP_REG;
        instr[addr].op2.op    = dst;
        instr[addr].op2.flags = m == M_MVI ? OPF_DST :
                                m == M_CMP ? OPF_SRC : OPF_SRCDST;
    }
}

/* ======================================================================== */
/*  DIS_IND_2OP -- Decode indirect mode instructions                        */
/* ======================================================================== */
LOCAL void dis_ind_2op   (uint32_t addr)
{
    uint32_t w0, op, op1, op2;
    mnm_t    m;

    /* -------------------------------------------------------------------- */
    /*  Destination is in bits 0..2.  Source is in bits 3..5.  Opcode is    */
    /*  in bits 6..8.                                                       */
    /* -------------------------------------------------------------------- */
    w0  = GET_WORD(addr);
    op  = (w0 >> 6) & 7;
    op1 = (w0 >> 3) & 7;
    op2 = (w0 >> 0) & 7;

    /* -------------------------------------------------------------------- */
    /*  Chuck opcode 000, as it's illegal.                                  */
    /* -------------------------------------------------------------------- */
    if (op == 0)
    {
        mark_invalid(addr);
        if (instr[addr].flags & FLAG_SDBD)
            mark_invalid(0xFFFF & (addr - 1));
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  If FLAG_SDBD is set but we're MVO@, or we're reading via R6, nuke   */
    /*  FLAG_SDBD.  (SDBD is incompatible w/ MVO@ and stack accesses.)      */
    /* -------------------------------------------------------------------- */
    if ((instr[addr].flags & FLAG_SDBD) && (op == 1 || op1 == 6))
    {
        mark_invalid(0xFFFF & (addr - 1));
        instr[addr].flags &= ~FLAG_SDBD;
    }

    /* -------------------------------------------------------------------- */
    /*  Build the instruction.                                              */
    /* -------------------------------------------------------------------- */
    instr[addr].mnemonic = m = mnm_ind_2op[op];
    instr[addr].len = 1;

    if (m == M_MVO_)
    {
        instr[addr].op1.type  = OP_REG;
        instr[addr].op1.op    = op2;
        instr[addr].op1.flags = OPF_SRC;

        instr[addr].op2.type  = OP_REG;
        instr[addr].op2.op    = op1;
        instr[addr].op2.flags = OPF_DST | OPF_IND;

        if (op1 == 6)
            instr[addr].mnemonic = m = M_PSHR;
    } else
    {
        instr[addr].op1.type  = OP_REG;
        instr[addr].op1.op    = op1;
        instr[addr].op1.flags = OPF_SRC | OPF_IND;

        instr[addr].op2.type  = OP_REG;
        instr[addr].op2.op    = op2;
        instr[addr].op2.flags = m == M_MVI_ ? OPF_DST :
                                m == M_CMP_ ? OPF_SRC : OPF_SRCDST;

        if (op1 == 6 && m == M_MVI_)
            instr[addr].mnemonic = m = M_PULR;
    }
}

/* ======================================================================== */
/*  DECODE_INSTRS   -- Try to decode all instructions.  Mark obviously      */
/*                     invalid instructions as such.  If enabled, mark      */
/*                     "rare" instructions illegal too.                     */
/* ======================================================================== */
LOCAL void decode_instrs(uint32_t addr_lo, uint32_t addr_hi)
{
    uint32_t addr, w0;

    /* -------------------------------------------------------------------- */
    /*  Step by single addresses within range.  Although some instructions  */
    /*  are multi-word, we don't know where instruction boundaries are yet. */
    /*  Just decode all words as if they are instructions and then sort it  */
    /*  out more fully later.                                               */
    /* -------------------------------------------------------------------- */
    for (addr = addr_lo; addr <= addr_hi; addr++)
    {
        /* ---------------------------------------------------------------- */
        /*  Instructions that set bits 10..15 are invalid.  Skip them.      */
        /* ---------------------------------------------------------------- */
        if (GET_WORD(addr) & 0xFC00)
        {
            mark_invalid(addr);
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Skip over addresses we've previously identified as not code.    */
        /*  Examples are explicit data, and invalid opcodes.                */
        /* ---------------------------------------------------------------- */
        if (NOT_CODE(addr))
            continue;

        w0 = GET_WORD(addr);
        assert((w0 & 0xFC00) == 0);


        /* ---------------------------------------------------------------- */
        /*  Pull apart the opcode by opcode space.                          */
        /*                                                                  */
        /*                                                                  */
        /*      00 0000 0100    Absolute jump instructions                  */
        /*      00 0000 00oo    Implied 1-op arithmetic  (a)                */
        /*      00 0000 01oo    Implied 1-op arithmetic  (b)                */
        /*      00 0011 01oo    NOP, SIN instructions                       */
        /*      00 0011 00oo    GSWD -- Get Status WorD insn.               */
        /*      00 00oo oddd    Combined Src/Dst Register 1-op              */
        /*      00 01oo omrr    Rotate/Shift Register 1-op                  */
        /*      0o ooss sddd    Register  -> Register 2-op arith            */
        /*      10 00z0 cccc    Conditional branch (internal condition)     */
        /*      10 00z1 cccc    Conditional branch (external condition)     */
        /*      1o oo11 1ddd    Immediate -> Register 2-op arith            */
        /*      1o oo00 0ddd    Direct    -> Register 2-op arith            */
        /*      1o oomm mddd    Indirect  -> Register 2-op arith            */
        /*                                                                  */
        /* ---------------------------------------------------------------- */
        if      ((w0 & 0x3FF)==0x004) dis_jump      (addr); /* 00 0000 0100 */
        else if ((w0 & 0x3FC)==0x000) dis_impl_1op_a(addr); /* 00 0000 00oo */
        else if ((w0 & 0x3FC)==0x004) dis_impl_1op_b(addr); /* 00 0000 01oo */
        else if ((w0 & 0x3FC)==0x034) dis_nop_sin   (addr); /* 00 0011 01oo */
        else if ((w0 & 0x3FC)==0x030) dis_gwsd      (addr); /* 00 0011 00oo */
        else if ((w0 & 0x3C0)==0x000) dis_reg_1op   (addr); /* 00 00oo oddd */
        else if ((w0 & 0x3C0)==0x040) dis_rot_1op   (addr); /* 00 01oo omrr */
        else if ((w0 & 0x200)==0x000) dis_reg_2op   (addr); /* 0o ooss sddd */
        else if ((w0 & 0x3D0)==0x200) dis_cond_br   (addr); /* 10 00z0 cccc */
        else if ((w0 & 0x3D0)==0x210) dis_bext      (addr); /* 10 00z1 cccc */
        else if ((w0 & 0x238)==0x238) dis_imm_2op   (addr); /* 1o oo11 1ddd */
        else if ((w0 & 0x238)==0x200) dis_dir_2op   (addr); /* 1o oo00 0ddd */
        else                          dis_ind_2op   (addr); /* 1o oomm mddd */

    }

    for (addr = addr_lo; addr < addr_hi; addr++)
    {
        if (NOT_CODE(addr))
            instr[addr].len = 1;
    }
}

/* ======================================================================== */
/*  FIND_FUNKY_BRANCHES() -- Look for arithmetic on R7 and convert these    */
/*                           instructions to branches, *or* treat them as   */
/*                           invalid if they look invalid.                  */
/* ======================================================================== */
LOCAL int find_funky_branches(void)
{
    uint32_t addr;
    int changed = 0;

    /* -------------------------------------------------------------------- */
    /*  Examine all valid decoded instructions, and see if any of them      */
    /*  operate directly on the program counter as a destination.  Some     */
    /*  of these are highly suspicious and can be converted into invalid    */
    /*  opcodes.  Other instructions are useful -- INCR PC, DECR PC, ADCR   */
    /*  PC and ADDR PC are all common tricks.                               */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        /* ---------------------------------------------------------------- */
        /*  Skip code that's already identified as 'bad'.                   */
        /*  Skip code that's already identified as branches.                */
        /* ---------------------------------------------------------------- */
        if (NOT_CODE(addr) || IS_BRANCH(addr))
            continue;

        /* ---------------------------------------------------------------- */
        /*  Check the first operand.  Only 1op Reg format can set this to   */
        /*  "R7 is destination".                                            */
        /* ---------------------------------------------------------------- */
        if ( instr[addr].op1.type == OP_REG &&
             instr[addr].op1.op   == 7      &&
            (instr[addr].op1.flags & OPF_DST))
        {
            switch (instr[addr].mnemonic)
            {
                /* -------------------------------------------------------- */
                /*  ADCR looks like a short forward conditional branch.     */
                /* -------------------------------------------------------- */
                case M_ADCR:
                {
                    instr[addr].br_target = addr + 2;
                    instr[addr].flags    |= FLAG_BRANCH | FLAG_CONDBR;
                    break;
                }

                /* -------------------------------------------------------- */
                /*  INCR/DECR are unconditional branches.                   */
                /* -------------------------------------------------------- */
                case M_INCR:
                {
                    instr[addr].br_target = addr + 2;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }

                case M_DECR:
                {
                    instr[addr].br_target = addr;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }

                case M_COMR:
                {
                    if (suspicious_pc_math_is_invalid)
                    {
                        changed += mark_invalid(addr);
                    } else
                    {
                        instr[addr].br_target = addr ^ 0xFFFF;
                        instr[addr].flags    |= FLAG_BRANCH;
                    }
                    break;
                }

                case M_NEGR:
                {
                    if (suspicious_pc_math_is_invalid)
                    {
                        changed += mark_invalid(addr);
                    } else
                    {
                        instr[addr].br_target = (-addr) & 0xFFFF;
                        instr[addr].flags    |= FLAG_BRANCH;
                    }
                    break;
                }

                case M_RSWD:    /* Treat RSWD R7 as always illegal, period */
                {
                    changed += mark_invalid(addr);
                    break;
                }
                default:        /* Uhoh?! */
                {
                    fprintf(stderr,
                            "Instruction '%s' at $%.4X sets op1 as dst R7\n",
                            mnemonic[instr[addr].mnemonic], addr);
                    break;
                }
            }

            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Check the second operand.                                       */
        /* ---------------------------------------------------------------- */
        if ( instr[addr].op2.type == OP_REG &&
             instr[addr].op2.op   == 7      &&
            ((instr[addr].op2.flags & (OPF_DST | OPF_IND)) == OPF_DST))
        {
            /* ------------------------------------------------------------ */
            /*  In general, we permit MOV, ADD, SUB.  Anything else looks   */
            /*  pretty darn suspicious.  We can calculate branch targets    */
            /*  for ADDI, SUBI, MVII, but nothing else for now.             */
            /*                                                              */
            /*  Also, ADDI/SUBI that result in targets outside the 16-bit   */
            /*  range are suspicious.                                       */
            /* ------------------------------------------------------------ */

            switch (instr[addr].mnemonic)
            {
                /* -------------------------------------------------------- */
                /*  MVII is basically identical to J, only shorter.         */
                /*  In a 16-bit ROM, B is a better choice though.           */
                /* -------------------------------------------------------- */
                case M_MVII:
                {
                    instr[addr].br_target = instr[addr].op1.op;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }

                /* -------------------------------------------------------- */
                /*  These are still somewhat suspicious -- why not use "B"? */
                /* -------------------------------------------------------- */
                case M_ADDI:
                case M_SUBI:
                {
                    uint32_t targ;

                    if (instr[addr].mnemonic == M_ADDI)
                    {
                        targ = addr + instr[addr].len + instr[addr].op1.op;
                    } else
                    {
                        targ = addr + instr[addr].len - instr[addr].op1.op;
                    }

                    if (suspicious_pc_math_is_invalid && targ > 0xFFFF)
                    {
                        changed += mark_invalid(addr);
                        break;
                    }

                    instr[addr].br_target = targ & 0xFFFF;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }

                /* -------------------------------------------------------- */
                /*  These are very suspicious.                              */
                /* -------------------------------------------------------- */
                case M_ANDI:
                case M_XORI:
                {
                    uint32_t targ;

                    if (suspicious_pc_math_is_invalid)
                    {
                        changed += mark_invalid(addr);
                        break;
                    }

                    if (instr[addr].mnemonic == M_ANDI)
                    {
                        targ = (addr + instr[addr].len) & instr[addr].op1.op;
                    } else
                    {
                        targ = (addr + instr[addr].len) ^ instr[addr].op1.op;
                    }

                    instr[addr].br_target = targ & 0xFFFF;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }


                /* -------------------------------------------------------- */
                /*  These are fairly common.                                */
                /* -------------------------------------------------------- */
                case M_PULR: case M_MVI:  case M_MVI_:
                {
                    instr[addr].br_target = BTARG_UNK;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }

                /* -------------------------------------------------------- */
                /*  MOVR (aka. JR) is fairly common.  ADDR is also common   */
                /*  for switch-case constructions.  SUBR is less common.    */
                /*  Allow these unless the src is R6 or R7.                 */
                /* -------------------------------------------------------- */
                case M_MOVR: case M_ADDR: case M_SUBR:
                {
                    if (suspicious_pc_math_is_invalid &&
                        instr[addr].op1.op >= 6)
                    {
                        changed += mark_invalid(addr);
                        break;
                    }
                    instr[addr].br_target = BTARG_UNK;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }

                /* -------------------------------------------------------- */
                /*  Don't trust direct or indirect ADD/SUB to PC by default */
                /*  JZ, 2-1-2008:  Why??  Disabling that distrust.          */
                /* -------------------------------------------------------- */
                case M_ADD:  case M_SUB:
                case M_ADD_: case M_SUB_:
                {
                    /*
                    if (suspicious_pc_math_is_invalid)
                    {
                        changed += mark_invalid(addr);
                        break;
                    }
                    */
                    instr[addr].br_target = BTARG_UNK;
                    instr[addr].flags    |= FLAG_BRANCH;
                    break;
                }

                /* -------------------------------------------------------- */
                /*  Note that I shouldn't see MVO or CMP.  This is for      */
                /*  error checking purposes only.                           */
                /* -------------------------------------------------------- */
                case M_MVO:  case M_CMP:
                case M_MVO_: case M_CMP_:
                case M_MVOI: case M_CMPI:
                {
                    fprintf(stderr,
                            "WARNING: '%s' at $%.4x appears to write R7?\n",
                            mnemonic[instr[addr].mnemonic], addr);
                    break;
                }

                /* -------------------------------------------------------- */
                /*  Treat everything else as invalid/unknown target.        */
                /* -------------------------------------------------------- */
                default:
                {
                    if (suspicious_pc_math_is_invalid)
                    {
                        changed += mark_invalid(addr);
                    } else
                    {
                        instr[addr].br_target = BTARG_UNK;
                        instr[addr].flags    |= FLAG_BRANCH;
                    }
                    break;
                }
            }
            continue;
        }
    }


    return changed;
}

/* ======================================================================== */
/*  MARK_ARGS_INVALID -- Mark arguments to instructions as invalid.  This   */
/*                       allows us to catch branches into the middle of     */
/*                       instructions, and so on.                           */
/* ======================================================================== */
LOCAL int mark_args_invalid(void)
{
    int addr, step = 0, i, changes = 0;

    for (addr = 0; addr <= 0xFFFF; addr += step)
    {
        step = 1;
        if (NOT_CODE(addr) || !instr[addr].len)
            continue;

        step = instr[addr].len;

        for (i = 1; i < step; i++)
            changes += mark_invalid(addr + i);
    }

    return changes;
}

/* ======================================================================== */
/*  MARK_FORCED_ENTRY -- Mark forced entry points as valid and a brtrg.     */
/* ======================================================================== */
LOCAL int mark_forced_entry(void)
{
    int i, addr;
    int changed = 0;

    for (i = 0; i < forced_entry_points; i++)
    {
        addr = entry_point[i];

        if (IS_EMPTY(addr) || IS_BRTRG(addr))
            continue;

        changed += mark_valid(addr, FLAG_BRTRG | FLAG_CODE | FLAG_FORCED);
        changed += mark_branch_target(addr, 0x10000);
    }

    return changed;
}

/* ======================================================================== */
/*  MARK_FORCED_DATA  -- Mark forced data ranges as data, not code          */
/* ======================================================================== */
LOCAL int mark_forced_data(void)
{
    int i;
    int changed = 0;
    uint32_t addr;

    for (i = 0; i < forced_data_ranges; i++)
    {
        uint32_t addr_lo = data_range_lo[i];
        uint32_t addr_hi = data_range_hi[i] + 1;

        for ( addr = addr_lo ; addr != addr_hi ; addr++ )
        {
            if (IS_EMPTY(addr) || IS_BRTRG(addr))
                continue;

            changed += mark_invalid(addr);
        }
    }

    return changed;
}

/* ======================================================================== */
/*  PROPAGATE_INVOP  -- Scan code backwards, propagating "INVOP" attrs      */
/*                      back towards unconditional control transfers.       */
/*                      The idea is that you can't reach an invalid op by   */
/*                      "falling through" from a valid op.                  */
/* ======================================================================== */
LOCAL int propagate_invop(void)
{
    int addr, span = -1, last_br = 0x10000, changed = 0;
    int prev_was_invalid = 0;

    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (IS_EMPTY(addr))
        {
            span = addr + 1;
            prev_was_invalid = 1;
            continue;
        }

        if (NOT_CODE(addr))
        {
            prev_was_invalid = 1;
            continue;
        } else if (prev_was_invalid && span != -1 && span < addr)
        {
            while (span < addr)
                changed += mark_invalid(span++);
        }

        if (IS_BRANCH(addr) && !IS_CONDBR(addr))
        {
            if (!prev_was_invalid || IS_BRTRG(addr) || IS_JSR(last_br))
            {
                last_br = addr;
                span = addr + instr[addr].len;
            }
        }

        if (instr[addr].flags & FLAG_CODE) /* someone forced this on? */
            span = addr + instr[addr].len;

        if (IS_BRTRG(addr))
            span = addr + instr[addr].len;

        prev_was_invalid = 0;
        addr += instr[addr].len - 1;
    }

    return changed;
}


#if 0
LOCAL int propagate_invop_old(void)
{
    int addr, step = 0;
    int changed = 0;

    /* -------------------------------------------------------------------- */
    /*  This is tricky.  We scan for "truly invalid" instructions.  To      */
    /*  find these, though, during a *backwards* scan is tricky.  Why?      */
    /*  Instructions can be up to 3 words long.  In a 3-word-long instr,    */
    /*  the 2nd and 3rd words might look invalid even though the 3-word     */
    /*  combo is perfectly valid.  Thus, when we step backwards looking     */
    /*  for starting points, we do a 'greedy gobble', looking for the       */
    /*  longest instruction that ends at our current position, and step to  */
    /*  there.                                                              */
    /* -------------------------------------------------------------------- */

    /*  Find first piece of ROM we know about.  */
    for (addr = 0xFFFF; addr > 0x0002; addr--)
    {
        if (!IS_EMPTY(addr))
            break;
    }

    for (; addr > 0x0002; addr -= step)
    {
        step = 1;
        /* ---------------------------------------------------------------- */
        /*  Only analyse ROM we know about.                                 */
        /* ---------------------------------------------------------------- */
        if (IS_EMPTY(addr))
        {
            continue;
        }
//printf("addr %.4X\n", addr);

        /* ---------------------------------------------------------------- */
        /*  If this word is 'ok', try stepping to a previous instruction.   */
        /* ---------------------------------------------------------------- */
        if (MAYBE_CODE(addr))
        {
            if      (MAYBE_CODE(addr - 3) && instr[addr-3].len == 3) step = 3;
            else if (MAYBE_CODE(addr - 2) && instr[addr-2].len == 2) step = 2;
            else                                                     step = 1;
            continue;
        }

//printf("is: %.4X\n", addr);
        /* ---------------------------------------------------------------- */
        /*  Ok, we're at an invalid instruction.  Now chase this to         */
        /*  earlier addresses until we hit an unconditional branch.         */
        /* ---------------------------------------------------------------- */
        while (addr >= 2 && !IS_EMPTY(addr))
        {

            /* len 1 branches are "INCR PC", "DECR PC", etc. */
            if (MAYBE_CODE(addr) &&
                IS_BRANCH (addr) &&
                instr[addr].len == 1)
            {
//printf("a\n");
                break;
            }

            /* len 2 branches are "B", "Bcond", and some PC arith. */
            if (MAYBE_CODE(addr - 1) &&
                IS_BRANCH (addr - 1) &&
                !IS_CONDBR(addr - 1) &&
                instr[addr - 1].len == 2)
            {
//printf("b\n");
                break;
            }

            /* len 3 branches are J/JSR, and some PC arith.  */
            if (MAYBE_CODE(addr - 2) &&
                IS_BRANCH (addr - 2) &&
                instr[addr - 2].len == 3)
            {
//printf("c\n");
                break;
            }

            changed += mark_invalid(addr);

            addr--;
            step = 0;
        }
//printf("ie: %.4X\n", addr);
    }

    return changed;
}
#endif


/* ======================================================================== */
/*  KILL_BAD_BRANCHES -- Look for branches to invalid opcodes, and mark     */
/*                       those branches as invalid instructions.            */
/* ======================================================================== */
LOCAL int kill_bad_branches(void)
{
    uint32_t addr, targ;
    int changed = 0;

    /* -------------------------------------------------------------------- */
    /*  Scan the target-of arrays, removing bad targeting.                  */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        int i, j;

        if (NOT_CODE(addr) || !IS_BRTRG(addr))
            continue;

        if (!instr[addr].target_of)
        {
            fprintf(stderr,
                    "WARNING: $%.4X marked as BRTRG, but has no target_of\n",
                    addr);
            continue;
        }

        for (i = j = 0; i < instr[addr].tg_of_cnt; i++)
        {
            int target_of = instr[addr].target_of[i];
            if (target_of != 0x10000 &&
                NOT_CODE(target_of))
            {
                continue;
            }
            instr[addr].target_of[j++] = target_of;
        }
        instr[addr].tg_of_cnt = j;
        if (!j)
        {
            instr[addr].flags &= ~FLAG_BRTRG;
            free(instr[addr].target_of);
            instr[addr].tg_of_cnt = 0;
            instr[addr].tg_of_max = 0;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Scan for branches, updating branch targeting information.           */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        /* ---------------------------------------------------------------- */
        /*  Skip already-invalid instructions.                              */
        /* ---------------------------------------------------------------- */
        if (NOT_CODE(addr))
            continue;

        /* ---------------------------------------------------------------- */
        /*  See if this is a branch to a known target.                      */
        /* ---------------------------------------------------------------- */
        if ((instr[addr].flags & FLAG_BRANCH) &&
            (targ = instr[addr].br_target) <= 0xFFFF)
        {
            /* ------------------------------------------------------------ */
            /*  If so, see if it's a branch into ROM we know about.         */
            /* ------------------------------------------------------------ */
            if (IS_EMPTY(targ))
                continue;

            /* ------------------------------------------------------------ */
            /*  If the target is known, but not valid code, mark us inval.  */
            /* ------------------------------------------------------------ */
            if (NOT_CODE(targ))
            {
                changed += mark_invalid(addr);
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  Mark the target instruction as a BRTRG.                     */
            /* ------------------------------------------------------------ */
            mark_branch_target(targ, addr);

        }
    }

    /* -------------------------------------------------------------------- */
    /*  Scan again, and see if any branches are to areas of the memory      */
    /*  map that we *know* can't be valid branch targets.  Skip this pass   */
    /*  if we allow for these targets.                                      */
    /* -------------------------------------------------------------------- */
    if (allow_branch_to_bad_addr)
        return changed;

    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (NOT_CODE(addr))
            continue;

        if (IS_BRANCH(addr)
            && (targ = instr[addr].br_target) <= 0xFFFF
            && IS_EMPTY(targ)
            && (targ != 0xCF01)) /* hack for ECScable */
        {
            if ((!allow_global_branches && (targ < 0x1000||targ >= 0x2000)) ||
                (                  targ < 0x0400) ||
                (targ >= 0x0500 && targ < 0x1000) ||
                (targ >= 0x3000 && targ < 0x4800) ||
                (targ >= 0x1015 && targ < 0x2000 && no_exec_branches))
            {
                changed += mark_invalid(addr);
            }
        }
    }

    return changed;
}

/* ======================================================================== */
/*  BRTRG_VS_SDBD    -- Scan for branches that target instructions w/       */
/*                      FLAG_SDBD set.  Decide whether branch or instr      */
/*                      needs changing.                                     */
/* ======================================================================== */
LOCAL int brtrg_vs_sdbd(void)
{
    uint32_t addr, targ;
    int changed = 0;

    /* -------------------------------------------------------------------- */
    /*  For each potential branch target, do the following:                 */
    /*                                                                      */
    /*   -- If target's exact address is unknown, skip it.                  */
    /*   -- If the target is not modified by SDBD, ignore it.               */
    /*   -- If the target is modified by SDBD, but is indirect mode,        */
    /*      remove FLAG_SDBD and move on.                                   */
    /*   -- If the target is an immediate-mode instruction modified by      */
    /*      SDBD:                                                           */
    /*       -- If its third word is INVALID and the target is itself       */
    /*          not a "branch" (eg. ADDI #foo, PC), mark this branch        */
    /*          as an invalid instruction.                                  */
    /*       -- Otherwise, remove FLAG_SDBD from instruction and redecode.  */
    /* -------------------------------------------------------------------- */

    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        /* ---------------------------------------------------------------- */
        /*  Only handle stuff that still looks like code.                   */
        /* ---------------------------------------------------------------- */
        if (NOT_CODE(addr))
            continue;

        if (IS_BRANCH(addr))
        {
            targ = instr[addr].br_target;

            /* ------------------------------------------------------------ */
            /*  Only deal w/ known targets to SDBD-affected instructions.   */
            /* ------------------------------------------------------------ */
            if (targ > 0xFFFF ||                         /* unknown target  */
                (instr[targ].flags & FLAG_SDBD) == 0)    /* not SDBD target */
                continue;

            /* ------------------------------------------------------------ */
            /*  Skip situations where the user has forced instructions to   */
            /*  be valid.                                                   */
            /* ------------------------------------------------------------ */
            if ((instr[targ].flags & FLAG_FORCED) != 0 ||
                (instr[addr].flags & FLAG_FORCED) != 0)
                continue;

            /* ------------------------------------------------------------ */
            /*  Handle direct mode by just nuking 'SDBD' flag.              */
            /* ------------------------------------------------------------ */
            if (instr[targ].op1.type == OP_REG)          /* indirect mode   */
            {
                instr[targ].flags &= ~FLAG_SDBD;
                mark_invalid((targ - 1) & 0xFFFF);
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  The other case we handle is immediate-mode, so skip out on  */
            /*  anything else 'weird'.                                      */
            /* ------------------------------------------------------------ */
            if (instr[targ].op1.type != OP_IMMED)       /* not immedate     */
                continue;

            /* ------------------------------------------------------------ */
            /*  Ok, if we remove SDBD, do we expose bad code?               */
            /* ------------------------------------------------------------ */
            if (NOT_CODE(targ + 2) && !IS_BRANCH(targ))
            {
                /* -------------------------------------------------------- */
                /*  Yes:  Mark the branch invalid.                          */
                /* -------------------------------------------------------- */
                changed += mark_invalid(addr);
            } else
            {
                /* -------------------------------------------------------- */
                /*  No:  Remove SDBD from the target and mark the SDBD as   */
                /*  invalid.                                                */
                /* -------------------------------------------------------- */
                instr[targ].flags &= ~(FLAG_SDBD);
                changed += mark_invalid((targ - 1) & 0xFFFF);
                changed++;
                decode_instrs(targ, targ + 2);
            }
        }
    }

    return changed;
}

/* ======================================================================== */
/*  FIND_JSR_DATA    -- Look for data after JSRs by examining the JSR's     */
/*                      target code, looking for indirect reads via the     */
/*                      return address register.                            */
/* ======================================================================== */
LOCAL int find_jsr_data(void)
{
    int addr, targ, reg;
    int ind_addr;
    int changed = 0;

    /* -------------------------------------------------------------------- */
    /*  Scan looking for JSR R4 and JSR R5.                                 */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (NOT_CODE(addr))
            continue;

        if (!IS_JSR(addr))
            continue;

        if (instr[addr + 3].flags & FLAG_INTERP)
            continue;

        reg = instr[addr].op1.op;
        if (reg != 4 && reg != 5)
            continue;

        /* ---------------------------------------------------------------- */
        /*  Found one.  Look at the first few instructions of the target,   */
        /*  looking for indirect reads via the target.  We stop when we     */
        /*  find an invalid instruction, control transfer, or other instr   */
        /*  we don't understand.                                            */
        /* ---------------------------------------------------------------- */
        targ = instr[addr].op2.op;
        ind_addr = addr + 3;

        if (debug_find_jsr_data)
            printf("fjd: JSR R%d, %.4X;  ind_addr=%.4X\n", reg, targ, ind_addr);

        while (MAYBE_CODE(targ))
        {
            if (debug_find_jsr_data > 1)
                printf("fjd: target %.4X, flags %.4X\n",
                       targ, instr[targ].flags);
            /* ------------------------------------------------------------ */
            /*  If we see SDBD, just move to the next instruction.          */
            /* ------------------------------------------------------------ */
            if (instr[targ].mnemonic == M_SDBD)
            {
                if (debug_find_jsr_data)
                    printf("fjd: SDBD %.4X\n", targ);
                targ++;
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  If we see a JSR or a conditional branch, stop, since we     */
            /*  aren't smart enough to handle arbitrary control flow.       */
            /* ------------------------------------------------------------ */
            if (IS_JSR(targ) || IS_CONDBR(targ))
            {
                if (debug_find_jsr_data)
                    printf("fjd: saw %s at %.4X\n",
                            IS_JSR(targ)?"JSR":"CONDBR", targ);
                break;
            }

            /* ------------------------------------------------------------ */
            /*  If we see an unconditional forward branch, follow it.       */
            /* ------------------------------------------------------------ */
            if (IS_BRANCH(targ))

            {
                if (instr[targ].br_target <= 0xFFFF &&
                    instr[targ].br_target >  targ)
                {
                    if (debug_find_jsr_data)
                        printf("fjd: taking BR %.4X at %.4X\n",
                                instr[targ].br_target, targ);
                    targ = instr[targ].br_target;
                    continue;
                } else
                {
                    if (debug_find_jsr_data)
                        printf("fjd: not taking branch at %.4X\n", targ);
                    break;
                }
            }


            /* ------------------------------------------------------------ */
            /*  If we see an indirect read via R5, go ahead and mark        */
            /*  another location as invalid.  If we are modified by SDBD,   */
            /*  mark two locations, and mark them as "BIDECLE".             */
            /* ------------------------------------------------------------ */
            if (instr[targ].mnemonic >= M_MVI_ &&
                instr[targ].mnemonic <= M_XOR_ &&
                instr[targ].op1.op == reg      &&
                (instr[targ].op1.flags&(OPF_SRC|OPF_IND)) ==(OPF_SRC|OPF_IND))
            {
                uint32_t flag = FLAG_INVOP;
                int      i;
                i = 1;
                if (instr[targ].flags & FLAG_SDBD)
                {
                    i = 2;
                    flag |= FLAG_DBDATA;
                }
                if (debug_find_jsr_data)
                    printf("fjd: saw ind read of %d at %.4X\n", i, targ);

                targ += instr[targ].len;
                if (instr[targ].len == 0)
                    targ++;

                while (i-- > 0)
                {
                    if (debug_find_jsr_data)
                        printf("fjd: marking %.4X invop\n", ind_addr);

                    instr[ind_addr].flags &= ~(MASK_DATA | FLAG_INTERP);
                    changed += mark_invalid(ind_addr);
                    instr[ind_addr].flags |= flag;
                    ind_addr++;
                }
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  Make sure we don't see any instructions that write to this  */
            /*  register, or use it as an 'indirect' in a manner we haven't */
            /*  already grokked.                                            */
            /* ------------------------------------------------------------ */
            if (( instr[targ].op1.type != OP_REG ||
                  instr[targ].op1.op   != reg    ||
                 (instr[targ].op1.flags & (OPF_DST|OPF_IND)) == 0) &&
                ( instr[targ].op2.type != OP_REG ||
                  instr[targ].op2.op   != reg    ||
                 (instr[targ].op2.flags & (OPF_DST|OPF_IND)) == 0) &&
                 instr[targ].len)
            {
                targ += instr[targ].len;
                continue;
            }

            if (debug_find_jsr_data)
                printf("fjd: instr at %.4X broke it\n", targ);

            break;
        }
    }

    return changed;
}

/* ======================================================================== */
/*  MARK_DATA        -- Scan for locations marked as "INVALID" and convert  */
/*                      those to DECLE or BIDECLE directives.               */
/* ======================================================================== */
LOCAL void mark_data(void)
{
    uint32_t addr, addr2, span, dtype;

    /* -------------------------------------------------------------------- */
    /*  Convert all INVOP to DATA if they're not already marked as some     */
    /*  form of 'DATA'.                                                     */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (instr[addr].flags & FLAG_INVOP)
            instr[addr].flags &= ~MASK_CODE;

        if ((instr[addr].flags & (FLAG_INVOP|MASK_DATA)) == FLAG_INVOP)
            instr[addr].flags |= FLAG_DATA;
    }

    /* -------------------------------------------------------------------- */
    /*  Convert BIDECLEs back to DECLEs if they aren't valid BIDECLEs.      */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if ((instr[addr].flags & MASK_DATA) == FLAG_DBDATA &&
            ((GET_WORD(addr) & 0xFF00) != 0 ||
             (instr[addr+1].flags & MASK_DATA) != FLAG_DBDATA))
        {
            instr[addr].flags &= ~MASK_DATA;
            instr[addr].flags |= FLAG_DATA;
        }

        /* always step in pairs when we see successful BIDECLEs. */
        if ((instr[addr].flags & MASK_DATA) == FLAG_DBDATA)
            addr++;
    }

    /* -------------------------------------------------------------------- */
    /*  Convert STRING back to DECLE if we reach a non-printing character.  */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if ((instr[addr].flags & MASK_DATA) == FLAG_STRING &&
            (GET_WORD(addr) > 0xFF || !isprint(GET_WORD(addr))))
        {
            instr[addr].flags &= ~MASK_DATA;
            instr[addr].flags |= FLAG_DATA;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Scan forward for spans of INVOP | (DATA & !CODE).                   */
    /* -------------------------------------------------------------------- */
    addr = 0;
    while (addr <= 0xFFFF)
    {
        if (IS_EMPTY(addr) || MAYBE_CODE(addr))
        {
            addr++;
            continue;
        }

        dtype = instr[addr].flags & (MASK_DATA | FLAG_INTERP);

        for (addr2 = addr + 1; addr2 <= 0xFFFF; addr2++)
            if (IS_EMPTY(addr2) || instr[addr2].label ||
                (instr[addr2].flags & (MASK_DATA | FLAG_INTERP)) != dtype)
                break;

        switch (dtype & ~FLAG_INTERP)
        {
            case FLAG_DBDATA:
                if ((addr2 & 1) != (addr & 1))
                    addr2--;
                if (addr2 == addr)
                {
                    addr2++;
                    goto data;
                }

                while (addr < addr2)
                {
                    span = addr2 - addr < 4 ? addr2 - addr : 4;
                    instr[addr].mnemonic = M_BIDECLE;
                    if (!IS_INTERP(addr))
                        instr[addr].len  = span;
                    addr += 2;
                }
                break;

            case FLAG_DATA:
            data:
                while (addr < addr2)
                {
                    span = addr2 - addr < 4 ? addr2 - addr : 4;
                    instr[addr].mnemonic = M_DECLE;
                    if (!IS_INTERP(addr))
                        instr[addr].len  = span;
                    addr++;
                }
                break;


            case FLAG_STRING:
                while (addr < addr2)
                {
                    span = addr2 - addr < 20 ? addr2 - addr : 20;
                    instr[addr].mnemonic = M_STRING;
                    if (!IS_INTERP(addr))
                        instr[addr].len  = span;
                    addr++;
                }
                break;
            default:
                fprintf(stderr, "INTERNAL ERROR: Unknown data type %d at "
                        "%.4X\n", dtype, addr);
                exit(1);
        }

    }
}

/* ======================================================================== */
/* ======================================================================== */
/*  TOP LEVEL DRIVING CODE                                                  */
/* ======================================================================== */
/* ======================================================================== */

/* ======================================================================== */
/*  GENERATE_LABELS  -- Scan for all hard memory reference and generate /   */
/*                      look up labels for them.                            */
/* ======================================================================== */
LOCAL void generate_labels(void)
{
    int addr;
    int targ;
    const char *lbl;

    /* -------------------------------------------------------------------- */
    /*  Scan for *all* memory references.  References that point within     */
    /*  our readable, preloaded pages are "local".  All others are          */
    /*  considered "global".                                                */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (NOT_CODE(addr))
            continue;

        /* ---------------------------------------------------------------- */
        /*  Instructions can have only one address operand.  We look at     */
        /*  the two operands first, and then at the branch target (if this  */
        /*  is marked as a branch).                                         */
        /* ---------------------------------------------------------------- */
        if      (instr[addr].op1.flags & OPF_ADDR) targ=instr[addr].op1.op;
        else if (instr[addr].op2.flags & OPF_ADDR) targ=instr[addr].op2.op;
        else if (IS_BRANCH(addr) &&
                 instr[addr].br_target <= 0xFFFF)  targ=instr[addr].br_target;
        else                                       continue;

        /* ---------------------------------------------------------------- */
        /*  Avoid oddball occurrences.                                      */
        /* ---------------------------------------------------------------- */
        if (targ < 0 || targ > 0xFFFF)
            continue;

        /* ---------------------------------------------------------------- */
        /*  Look up the label.  If one doesn't exist, symtab() will make    */
        /*  one up for us with the prefix we specify.                       */
        /* ---------------------------------------------------------------- */
        lbl  = symtab_getsym(symtab, targ << 3, IS_EMPTY(targ) ? 'G' : 'L', 0);
        if (lbl && !instr[targ].label)
            instr[targ].label = strdup(lbl);
    }

    /* -------------------------------------------------------------------- */
    /*  Scan for branch targets that were discovered other ways and give    */
    /*  those labels.                                                       */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (instr[addr].label || !IS_BRTRG(addr) || NOT_CODE(addr))
            continue;

        lbl  = symtab_getsym(symtab, addr << 3, IS_EMPTY(addr) ? 'G' : 'L', 0);
        if (lbl)
            instr[addr].label = strdup(lbl);
    }

    /* -------------------------------------------------------------------- */
    /*  Next, scan every address that still lacks a label, and ask the      */
    /*  symbol table if there's a known label for that address.             */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (instr[addr].label)
            continue;

        lbl  = symtab_getsym(symtab, addr << 3, 0, 0);

        if (lbl)
            instr[addr].label = strdup(lbl);
    }
}


/* ======================================================================== */
/*  GENERATE_TEXT    -- Scan all instructions and generate disassembly.     */
/* ======================================================================== */
LOCAL void generate_text(void)
{
    int addr;

    /* -------------------------------------------------------------------- */
    /*  Go decode all the instructions.                                     */
    /* -------------------------------------------------------------------- */
    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (IS_EMPTY(addr))
            continue;

        instr[addr].fmtline = instr_printer[instr[addr].mnemonic](addr);
    }
}


/* ======================================================================== */
/*  DO_DISASM -- Top level driver that disassembles everything              */
/* ======================================================================== */
LOCAL void do_disasm(void)
{
    int addr, p, sa = -1, ea = -1;
    int tot_changes = 0, changes, c;

    /* -------------------------------------------------------------------- */
    /*  Scan for preload pages that are also marked readable and not        */
    /*  bankswitched.                                                       */
    /* -------------------------------------------------------------------- */
    VB_PRINTF(0, ("Initial decode pass...\n"));

    for (addr = 0; addr <= 0xFF00; addr += 256)
    {
        p = addr >> 8;

        if ((GET_BIT(icart.preload,  p) != 0) &&
            (GET_BIT(icart.readable, p) != 0) &&
            (GET_BIT(icart.dobanksw, p) == 0))
        {
            if (sa == -1)
                sa = addr;

            ea = addr + 255;
            decode_instrs(addr, addr + 255);
        } else
        {
            mark_empty(addr, addr + 255);
            if (sa >= 0)
                VB_PRINTF(1, (">   $%.4X - $%.4X\n", sa, ea));

            sa = ea = -1;
        }
    }

    if (sa >= 0)
        VB_PRINTF(1, (">   $%.4X - $%.4X\n", sa, ea));

    sa = ea = -1;

    /* -------------------------------------------------------------------- */
    /*  If we have any generic labels, generate them.                       */
    /* -------------------------------------------------------------------- */
    if ( generic_labels )
    {
        int i;
        for (i = 0; i < generic_labels; i++)
        {
            const uint32_t targ = generic_label[i];
            symtab_getsym(symtab, targ << 3, IS_EMPTY(targ) ? 'G' : 'L', 0);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Now, if enabled, do various "analysis passes".                      */
    /* -------------------------------------------------------------------- */
    if (!skip_advanced_analysis)
    {
        VB_PRINTF(0, ("Performing basic analysis.\n"));
        tot_changes = c =0;

        if (!skip_mark_cart_header)
        {
            VB_PRINTF(1, ("> Marking the standard cartridge header (pre)\n"));
            tot_changes += mark_cart_header_pre();
        }

        if (!skip_funky_branch_detect)
        {
            VB_PRINTF(1, ("> Identifying arithmetic branches...\n"));
            tot_changes += find_funky_branches();
        }

again:
        do
        {
            changes = 0;

            if (forced_data_ranges)
            {
                VB_PRINTF(1, ("> Marking forced data ranges...\n"));
                changes += c = mark_forced_data();
                VB_PRINTF(2, (">>  %d words marked invalid\n", c));
            }

            if (forced_entry_points)
            {
                VB_PRINTF(1, ("> Marking forced entry points as branch targets...\n"));
                changes += c = mark_forced_entry();
                VB_PRINTF(2, (">>  %d words marked valid\n", c));
            }


            if (!skip_kill_bad_branches)
            {
                VB_PRINTF(1, ("> Marking invalid branches...\n"));
                changes += c = kill_bad_branches();
                VB_PRINTF(2, (">>  %d words marked invalid\n", c));
            }

            if (!skip_brtrg_vs_sdbd)
            {
                VB_PRINTF(1, ("> Analysing SDBD/branch interaction...\n"));
                changes += c = brtrg_vs_sdbd();
                VB_PRINTF(2, (">>  %d words marked invalid\n", c));
            }

            if (!skip_find_jsr_data)
            {
                VB_PRINTF(1, ("> Finding data after JSR instructions...\n"));
                changes += c = find_jsr_data();
                VB_PRINTF(2, (">>  %d words marked invalid\n", c));
            }

            if (!skip_exec_print)
            {
                VB_PRINTF(1, ("> Finding data after EXEC print calls...\n"));
                changes += c = decode_print_calls();
                VB_PRINTF(2, (">>  %d words marked invalid\n", c));
            }

            if (!skip_mark_args_invalid)
            {
                VB_PRINTF(1, ("> Marking instruction arguments invalid...\n"));
                changes += c = mark_args_invalid();
                VB_PRINTF(2, (">>  %d words marked invalid\n", c));
            }

            if (!skip_exec_sound_interp)
            {
                VB_PRINTF(1, ("> Looking for EXEC music...\n"));
                changes += c = decode_exec_music();
                VB_PRINTF(2, (">>  %d words marked as data\n", c));

                VB_PRINTF(1, ("> Looking for EXEC sound effects...\n"));
                changes += c = decode_exec_sfx();
                VB_PRINTF(2, (">>  %d words marked as data\n", c));
            }


            if (!skip_propagate_invalid)
            {
                VB_PRINTF(1, ("> Propagating invalid opcodes...\n"));
                changes += c = propagate_invop();
                VB_PRINTF(2, (">>  %d words marked invalid\n", c));
            }


            tot_changes += changes;

            if (dont_loop_analysis)
                break;

            if (changes > 0)
                VB_PRINTF(0, ("%6d changes:  Repeating analysis passes.\n",
                             changes));

        } while (changes > 0);


        changes = 0;
        if (!skip_mark_cart_header)
        {
            VB_PRINTF(1, ("> Marking the standard cartridge header (post)\n"));
            tot_changes += changes = mark_cart_header_post();
        }
        if (changes && !dont_loop_analysis)
            goto again;

        VB_PRINTF(0, ("%6d words marked invalid during analysis\n",
                  tot_changes));
    }

    /* -------------------------------------------------------------------- */
    /*  Generate labels as appropriate.                                     */
    /* -------------------------------------------------------------------- */
    VB_PRINTF(0, ("Generating labels...\n"));
    generate_labels();

    /* -------------------------------------------------------------------- */
    /*  Convert invalid and data areas to DECLE instructions.               */
    /* -------------------------------------------------------------------- */
    VB_PRINTF(0, ("Marking DECLE/BIDECLE/STRING.\n"));
    mark_data();

    /* -------------------------------------------------------------------- */
    /*  Generate the disassembled text for each valid instruction.          */
    /* -------------------------------------------------------------------- */
    VB_PRINTF(0, ("Generating disassembled text...\n"));
    generate_text();
}

/* ======================================================================== */
/*  WRITE_DISASM -- this is a mess, but it's Good Enough For Now(TM).       */
/* ======================================================================== */
LOCAL void write_disasm(FILE *f)
{
    uint32_t next_addr, addr = 0, skip = 0, last_was_invop = 0;
    int i;

    addr = 0;
    while (addr <= 0xFFFF)
    {
        int ilen;

        if (IS_EMPTY(addr))
        {
            skip++;
            addr++;
            continue;
        }

        if (instr[addr].len == 0) instr[addr].len = 1;
        if (skip)
            fprintf(f, "\n        ORG     $%.4X\n", addr);

        if (instr[addr].label)
            fprintf(f, "%s:\n", instr[addr].label);
        else if (MAYBE_CODE(addr) && last_was_invop)
            fprintf(f, "\n");

        ilen = instr[addr].cmt_len == CMT_LONG ? 24 :
               instr[addr].cmt_len == CMT_MED  ? 32 : 40;

        fprintf(f, "        %-*s; %.4X  ", ilen, instr[addr].fmtline, addr);
        if (!instr[addr].cmt)
        {
            if (instr[addr].len <= 4)
            {
                for (i = 0; i < instr[addr].len; i++)
                    if (debug_show_instr_flags)
                        fprintf(f, " %.4X", instr[addr + i].flags);
                    else
                        fprintf(f, " %.4X", GET_WORD(addr + i));
            }
        } else
        {
            fputs(instr[addr].cmt, f);
        }
        fprintf(f, "\n");

        next_addr = addr + instr[addr].len;

        /*
        if (IS_BRANCH(addr) && !IS_JSR(addr) &&
            !IS_BRANCH(addr + instr[addr].len))
            fprintf(f, "\n");
        */
        if (IS_BRANCH(addr) &&
            ((IS_JSR(addr) && !IS_JSR(next_addr) && MAYBE_CODE(next_addr))||
             (!IS_JSR(addr) && (!IS_BRANCH(next_addr)||IS_JSR(next_addr)))))
            fprintf(f, "\n");

        if (!IS_BRANCH(addr) && IS_JSR(next_addr) && MAYBE_CODE(addr))
            fprintf(f, "\n");

        last_was_invop = NOT_CODE(addr);
        addr = next_addr;
        skip = 0;
    }
}

/* ======================================================================== */
/*  WRITE_CROSSREF   -- Write a list of who branches to whom.               */
/* ======================================================================== */
LOCAL void write_crossref(FILE *f)
{
    int addr;
    int i;

    fprintf
    (
        f,
        ";; ===================================="
        "==================================== ;;\n"
        ";;  Branch cross-reference\n"
        ";; ------------------------------------"
        "------------------------------------ ;;\n"
        ";;  Target      Target of"
    );

    for (addr = 0; addr <= 0xFFFF; addr++)
    {
        if (NOT_CODE(addr) || !IS_BRTRG(addr))
            continue;

        if (!instr[addr].target_of)
        {
            fprintf(stderr,
                    "Warning: $%.4X is a branch target w/out target_of\n",
                    addr);
            continue;
        }

        for (i = 0; i < instr[addr].tg_of_cnt; i++)
        {
            if ((i & 7) == 0)
                fprintf(f, "\n;;  $%.4X      ", addr);

            if (instr[addr].target_of[i] == 0x10000)
                fprintf(f, " <entry>");
            else
                fprintf(f, " $%.4X  ", instr[addr].target_of[i]);
        }
    }


    fprintf
    (
        f,
        "\n;; ===================================="
        "==================================== ;;\n"
    );
}


/* ======================================================================== */
/* ======================================================================== */
/*  INSTRUCTION PRINTER FUNCTIONS                                           */
/* ======================================================================== */
/* ======================================================================== */
static char prt_buf[512], prt_buf2[256];

/* ------------------------------------------------------------------------ */
/*  PRT_ERR -- print out an internal error.                                 */
/* ------------------------------------------------------------------------ */
char *prt_err(uint32_t addr)     /* Internal error printer */
{
    snprintf(prt_buf, sizeof(prt_buf),"error! flags=%.8X ", instr[addr].flags);
    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_IMP -- instruction with implied operands                            */
/* ------------------------------------------------------------------------ */
char *prt_imp(uint32_t addr)     /* Implied operands     */
{
    return strdup(mnemonic[instr[addr].mnemonic]);
}

/* ------------------------------------------------------------------------ */
/*  PRT_JSR -- JSR-type instructions:  OPC REG, LABEL                       */
/* ------------------------------------------------------------------------ */
char *prt_jsr(uint32_t addr)     /* JSR reg, label       */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s R%d,     %s",
            mnemonic[instr[addr].mnemonic],
            instr[addr].op1.op,
            symtab_getsym(symtab, instr[addr].op2.op<<3, 'X', 0));

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_ROT -- Shift/Rotate instructions:  OPC  REG, [1|2]                  */
/* ------------------------------------------------------------------------ */
char *prt_rot(uint32_t addr)     /* ROT reg, [1|2]       */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s R%d,     %d",
            mnemonic[instr[addr].mnemonic],
            instr[addr].op1.op, instr[addr].op2.op);

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_CBR -- Conditional Branch instructions.                             */
/* ------------------------------------------------------------------------ */
char *prt_cbr(uint32_t addr)     /* BRANCH label         */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s %s",
            mnemonic[instr[addr].mnemonic],
            symtab_getsym(symtab, instr[addr].op1.op<<3, 'X', 0));

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_BXT -- Branch External instructions                                 */
/* ------------------------------------------------------------------------ */
char *prt_bxt(uint32_t addr)     /* BEXT cond, label     */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s $%.1X,     %s",
            mnemonic[instr[addr].mnemonic],
            instr[addr].op1.op,
            symtab_getsym(symtab, instr[addr].op2.op<<3, 'X', 0));

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_DIR -- Direct-mode instructions                                     */
/* ------------------------------------------------------------------------ */
char *prt_dir(uint32_t addr)     /* OP label, reg        */
{
    snprintf(prt_buf2, sizeof(prt_buf2), "%s,",
            symtab_getsym(symtab, instr[addr].op1.op<<3, 'X', 0));

    snprintf(prt_buf, sizeof(prt_buf), "%-7s %-8sR%d",
            mnemonic[instr[addr].mnemonic],
            prt_buf2, instr[addr].op2.op);

    return strdup(prt_buf);
}


/* ------------------------------------------------------------------------ */
/*  PRT_IMM -- Immediate-mode instructions                                  */
/* ------------------------------------------------------------------------ */
char *prt_imm(uint32_t addr)     /* OP #imm, reg         */
{
    if (instr[addr].op1.flags & OPF_ADDR)
    {
        snprintf(prt_buf2, sizeof(prt_buf2), "#%s,",
                symtab_getsym(symtab, instr[addr].op1.op<<3, 'X', 0));
    } else
    {
        snprintf(prt_buf2, sizeof(prt_buf2), "#$%.4X,", instr[addr].op1.op);
    }

    snprintf(prt_buf, sizeof(prt_buf), "%-7s %-8sR%d",
            mnemonic[instr[addr].mnemonic],
            prt_buf2, instr[addr].op2.op);

    return strdup(prt_buf);
}


/* ------------------------------------------------------------------------ */
/*  PRT_IMO -- MVOI instructions.                                           */
/* ------------------------------------------------------------------------ */
char *prt_imo(uint32_t addr)     /* OP reg, #imm         */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s R%d,     #$%.4X",
            mnemonic[instr[addr].mnemonic],
            instr[addr].op1.op, instr[addr].op2.op);

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_2RG -- Register-to-register instructions                            */
/* ------------------------------------------------------------------------ */
char *prt_2rg(uint32_t addr)     /* OP reg, reg          */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s R%d,     R%d",
            mnemonic[instr[addr].mnemonic],
            instr[addr].op1.op, instr[addr].op2.op);

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_1RG -- Single-register instructions                                 */
/* ------------------------------------------------------------------------ */
char *prt_1rg(uint32_t addr)     /* OP reg               */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s R%d",
            mnemonic[instr[addr].mnemonic], instr[addr].op1.op);

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_PLR -- PULR Rx.                                                     */
/* ------------------------------------------------------------------------ */
char *prt_plr(uint32_t addr)     /* OP reg               */
{
    snprintf(prt_buf, sizeof(prt_buf), "%-7s R%d",
            mnemonic[instr[addr].mnemonic], instr[addr].op2.op);

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_DCL -- DECLE directives.                                            */
/* ------------------------------------------------------------------------ */
char *prt_dcl(uint32_t addr)     /* DECLE [up to four words] */
{
    if (instr[addr].len > 4)
    {
        fprintf(stderr, "ERROR: DECLE w/ len > 4 at address %.4X\n", addr);
        exit(1);
    }
    if (instr[addr].len <= 0)
    {
        fprintf(stderr, "ERROR: DECLE w/ len <= 0 at address %.4X\n", addr);
        exit(1);
    }

    switch (instr[addr].len)
    {
        case 1:
            snprintf(prt_buf, sizeof(prt_buf), "DECLE   $%.4X", GET_WORD(addr));
            break;
        case 2:
            snprintf(prt_buf, sizeof(prt_buf), "DECLE   $%.4X,  $%.4X",
                    GET_WORD(addr    ), GET_WORD(addr + 1));
            break;
        case 3:
            snprintf(prt_buf, sizeof(prt_buf), "DECLE   $%.4X,  $%.4X,  $%.4X",
                    GET_WORD(addr    ), GET_WORD(addr + 1),
                    GET_WORD(addr + 2));
            break;
        case 4:
            snprintf(prt_buf, sizeof(prt_buf), "DECLE   $%.4X,  $%.4X,  $%.4X,  $%.4X",
                    GET_WORD(addr    ), GET_WORD(addr + 1),
                    GET_WORD(addr + 2), GET_WORD(addr + 3));
            break;
    }

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_BID -- BIDECLE directives.                                          */
/* ------------------------------------------------------------------------ */
char *prt_bid(uint32_t addr)     /* BIDECLE [up to two bidecles] */
{
    /* implementation note:  BIDECLE relies on op1, op2 having been set up  */
    /* rather than reading from the memory image.                           */
    switch (instr[addr].len)
    {
        case 2:
            snprintf(prt_buf, sizeof(prt_buf), "BIDECLE $%.4X",
                    ( GET_WORD(addr    )       & 0x00FF) |
                    ((GET_WORD(addr + 1) << 8) & 0xFF00));
            break;

        case 4:
            snprintf(prt_buf, sizeof(prt_buf), "BIDECLE $%.4X,  $%.4X",
                    ( GET_WORD(addr    )       & 0x00FF) |
                    ((GET_WORD(addr + 1) << 8) & 0xFF00),
                    ( GET_WORD(addr + 2)       & 0x00FF) |
                    ((GET_WORD(addr + 3) << 8) & 0xFF00));
            break;

        default:
            fprintf(stderr, "ERROR: BIDECLE w/ len != 2 or 4 at addr %.4X\n",
                    addr);
            exit(1);
    }

    return strdup(prt_buf);
}

/* ------------------------------------------------------------------------ */
/*  PRT_STR -- STRING directives.                                           */
/* ------------------------------------------------------------------------ */
char *prt_str(uint32_t addr)     /* STRING [up to 20 characters] */
{
    char strbuf[21];
    int i;

    memset(strbuf, 0, 21);

    for (i = 0; i < instr[addr].len; i++)
        strbuf[i] = GET_WORD(addr + i);

    snprintf(prt_buf, sizeof(prt_buf), "STRING  \"%s\"", strbuf);
    return strdup(prt_buf);
}




/* ======================================================================== */
/* ======================================================================== */
/*  MAIN PROGRAM, GENERIC CRAPOLA(TM)                                       */
/* ======================================================================== */
/* ======================================================================== */


/* ======================================================================== */
/*  MERGE_ICARTS     -- Given two icartrom_t's, copy the second into the    */
/*                      first.  The "replace" argument controls whether     */
/*                      the second ROM is allowed to replace segments in    */
/*                      the first.                                          */
/* ======================================================================== */
LOCAL void merge_icarts(icartrom_t *dst, icartrom_t *src, int replace)
{
    uint32_t a, p, attr_src, attr_dst;

    /* -------------------------------------------------------------------- */
    /*  Look through 256-word pages of 'src' for preload hunks to copy to   */
    /*  'dst'.  Preload is orthogonal to memory attribute settings.         */
    /* -------------------------------------------------------------------- */
    for (p = 0; p < 256; p++)
    {
        if (!GET_BIT(src->preload, p))
            continue;

        a = p << 8;

        if (!replace)
        {
            if (GET_BIT(dst->preload, p))
            {
                fprintf(stderr, "ERROR:  [preload] hunk conflict at "
                                "$%.4X - $%.4x\n"
                                "        Use '-r' to override\n",
                                a, a + 255);
                exit(1);
            }
        }

        icartrom_addseg(dst, &src->image[a], a, 256, 0, 0);
    }

    /* -------------------------------------------------------------------- */
    /*  Now look through 256-word segments of 'src' for various memory      */
    /*  attributes and try to set them in 'dst'.  We allow non-empty flags  */
    /*  in 'src' to merge into 'dst' in the following circumstances:        */
    /*                                                                      */
    /*   -- The 'replace' flag is set, or                                   */
    /*   -- The corresponding 'dst' flags are empty, or                     */
    /*   -- The corresponding 'dst' flags are equal to the 'src' flags.     */
    /*                                                                      */
    /* -------------------------------------------------------------------- */
    for (p = 0; p < 256; p++)
    {
        a = p << 8;

        attr_src = 0;
        if (GET_BIT(src->readable, p)) attr_src |= ICARTROM_READ;
        if (GET_BIT(src->writable, p)) attr_src |= ICARTROM_WRITE;
        if (GET_BIT(src->narrow,   p)) attr_src |= ICARTROM_NARROW;
        if (GET_BIT(src->dobanksw, p)) attr_src |= ICARTROM_BANKSW;

        attr_dst = 0;
        if (GET_BIT(dst->readable, p)) attr_dst |= ICARTROM_READ;
        if (GET_BIT(dst->writable, p)) attr_dst |= ICARTROM_WRITE;
        if (GET_BIT(dst->narrow,   p)) attr_dst |= ICARTROM_NARROW;
        if (GET_BIT(dst->dobanksw, p)) attr_dst |= ICARTROM_BANKSW;

        if (!attr_src)
            continue;

        if (replace || !attr_dst || attr_dst == attr_src)
        {
            icartrom_addseg(dst, NULL, a, 256, attr_src, 0);
        } else
        {
            fprintf(stderr,
                    "INTERNAL ERROR:  Cannot merge incompatible attributes on "
                    "$%.4X - $%.4X\n", a, a + 255);
            exit(1);
        }
    }
}



static struct option long_opts[] =
{
    {   "entry",                                2,      NULL,       'e'     },
    {   "entry-point",                          2,      NULL,       'e'     },
    {   "data",                                 2,      NULL,       'd'     },
    {   "data-range",                           2,      NULL,       'd'     },
    {   "no-default-symbols",                   0,      NULL,       'S'     },
    {   "no-exec-routine-symbols",              0,      NULL,       'X'     },
    {   "allow-branch-target-wrap",             0,      NULL,       'b'     },
    {   "allow-branch-to-bad-addr",             0,      NULL,       'B'     },
    {   "allow-suspicious-pc-math",             0,      NULL,       'p'     },
    {   "allow-rare-opcodes",                   0,      NULL,       'r'     },
    {   "allow-hlt",                            0,      NULL,       'H'     },
    {   "disable-analysis",                     0,      NULL,       'a'     },
    {   "verbose",                              0,      NULL,       'v'     },
    {   "force-overwrite",                      0,      NULL,       'f'     },
    {   "disable-mark-cart-header",             0,      NULL,       '0'     },
    {   "disable-funky-branch-detect",          0,      NULL,       '1'     },
    {   "disable-kill-bad-branches",            0,      NULL,       '2'     },
    {   "disable-brtrg-vs-sdbd",                0,      NULL,       '3'     },
    {   "disable-find-jsr-data",                0,      NULL,       '4'     },
    {   "disable-invalid-propagation",          0,      NULL,       '5'     },
    {   "disable-exec-sound-interp",            0,      NULL,       '6'     },
    {   "disable-exec-print",                   0,      NULL,       '7'     },
    {   "dont-loop-analysis",                   0,      NULL,       'L'     },
    {   "help",                                 0,      NULL,       'h'     },
    {   "?",                                    0,      NULL,       '?'     },
    {   "license",                              0,      NULL,       'l'     },

    {   "debug-find-jsr-data",                  0,      NULL,       1       },
    {   "debug-show-instr-flags",               0,      NULL,       2       },

    {   NULL,                                   0,      NULL,       0       }
};

static const char *optchars = "g:d:e:?01234567BEGHLSXabdfhlprv";

/*extern char *optarg;*/
/*extern int  optind, opterr, optopt;*/
int force_overwrite = 0;


/* ======================================================================== */
/*  USAGE            -- Just give usage info and exit.                      */
/* ======================================================================== */
LOCAL void usage(void)
{
    fprintf(stderr,
                                                                           "\n"
"DIS-1600  Advanced(?) CP-1600 Disassembler"                               "\n"
"Copyright 2003, Joseph Zbiciak"                                           "\n"
                                                                           "\n"
"USAGE:"                                                                   "\n"
"     dis1600 [flags] file0.rom [file1.rom [file2.rom [...]]] out.asm"     "\n"
                                                                           "\n"
" Input files can be .ROM or .BIN.  For .BIN files, DIS-1600 will use the" "\n"
" .CFG file if present, or assume default map if absent.  If multiple"     "\n"
" input files are given, they will be merged in the order specified."      "\n"
                                                                           "\n"
" Branch analysis flags:"                                                  "\n"
"  -p  --allow-suspicious-pc-math  Odd math on R7 is legal"                "\n"
"  -b  --allow-branch-target-wrap  Branch targets <$0000 or >$FFFF are ok" "\n"
"  -B  --allow-branch-to-bad-addr  Branches into STIC, PSG, etc. are ok"   "\n"
"  -G  --allow-global-branches     Allow branch outside given ROM and EXEC""\n"
"  -E  --no-exec-branches          No branches to $1015-$1FFF"             "\n"
"  -e<ADDR>  --entry-point <ADDR>  Add <ADDR> as an entry point"           "\n"
"  -d<ADDR>-<ADDR>                 Add <ADDR>-<ADDR> as a data range"      "\n"
"            --data-range <ADDR>-<ADDR>"                                   "\n"
"  -g<ADDR>  --data-label <ADDR>   Add generic label at <ADDR>"            "\n"
"                                  For hex <ADDR>, use 0xABCD or '$ABCD'." "\n"
                                                                           "\n"
" Opcode analysis flags:"                                                  "\n"
"  -r  --allow-rare-opcodes        Permit SIN, TCI, MVOI, others."         "\n"
"  -H  --allow-hlt                 Permit HLT opcode"                      "\n"
                                                                           "\n"
" Analysis pass control flags:"                                            "\n"
"  -L  --dont-loop-analysis        Only iterate analysis passes once."     "\n"
"  -a  --disable-analysis          Disable all advanced analysis passes"   "\n"
                                                                           "\n"
" Individual analysis phase controls:"                                     "\n"
"  -0  --disable-mark-cart-header      Don't try to interpret cart header" "\n"
"  -1  --disable-funky-branch-detect   Don't detect non-JSR/J/B branches"  "\n"
"  -2  --disable-kill-bad-branches     Don't kill branches to invalid ops" "\n"
"  -3  --disable-brtrg-vs-sdbd         Branch targets don't affect SDBD"   "\n"
"  -4  --disable-find-jsr-data         Don't try to find data after JSR"   "\n"
"  -5  --disable-invalid-propagation   Don't propagate invalid instrs"     "\n"
"  -6  --disable-exec-sound-interp     Don't interpret EXEC music/sfx"     "\n"
"  -7  --disable-exec-print            Don't look for EXEC print calls"    "\n"
                                                                           "\n"
" Miscellaneous flags:"                                                    "\n"
"  -S  --no-default-symbols        Do not pre-seed symbol table"           "\n"
"  -X  --no-exec-routine-symbols   Do not include EXEC routine names"      "\n"
"  -v  --verbose                   Output details during analysis"         "\n"
"  -f  --force-overwrite           Allow output to overwrite existing file""\n"
"  -l  --license                   License information"                    "\n"
"  -?  --?                         This usage info"                        "\n"
"  -h  --help                      This usage info"                        "\n"
                                                                           "\n"
"SDK-1600 Website:"                                                        "\n"
"    http://SDK-1600.spatula-city.org/"                                    "\n"
                                                                           "\n"
    );

    exit(0);
}

/* ======================================================================== */
/*  LICENSE          -- Just give license/authorship info and exit.         */
/* ======================================================================== */
LOCAL void license(void)
{
    fprintf(stderr,
                                                                          "\n"
" DIS-1600  Advanced(?) CP-1600 Disassembler"                             "\n"
" Copyright 2003, Joseph Zbiciak"                                         "\n"
                                                                          "\n"
" LICENSE:"                                                               "\n"
                                                                          "\n"
"   This program is free software; you can redistribute it and/or modify" "\n"
"   it under the terms of the GNU General Public License as published by" "\n"
"   the Free Software Foundation; either version 2 of the License, or"    "\n"
"   (at your option) any later version."                                  "\n"
                                                                          "\n"
"   This program is distributed in the hope that it will be useful,"      "\n"
"   but WITHOUT ANY WARRANTY; without even the implied warranty of"       "\n"
"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU"    "\n"
"   General Public License for more details."                             "\n"
                                                                          "\n"
"   You should have received a copy of the GNU General Public License along\n"
"   with this program; if not, write to the Free Software Foundation, Inc.,\n"
"   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA."          "\n"
                                                                          "\n"
" Run \"dis1600 --help\" for usage information."                          "\n"
                                                                          "\n"
    );

    exit(0);
}


/* ======================================================================== */
/*  MAIN             -- In The Beginning, there was MAIN, and C was with    */
/*                      CONST and VOID, and Darkness was on the face of     */
/*                      the Programmer.                                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    int c, option_idx = 0, value, value2;
    char *output_file;
    int i;
    FILE *f;

    symtab = symtab_create();
    for (i = 65536; i < 65539; i++)
        instr[i].flags |= FLAG_INVOP | FLAG_EMPTY;


    /* -------------------------------------------------------------------- */
    /*  Parse command-line arguments.                                       */
    /* -------------------------------------------------------------------- */
    while ((c = getopt_long(argc, argv, optchars, long_opts, &option_idx))
            != EOF)
    {
        value = 1;
        value2 = -1;
        if (optarg)
        {
            char *s;

            if (!sscanf(optarg, "0x%x", &value) &&
                !sscanf(optarg, "0X%x", &value) &&
                !sscanf(optarg, "$%x",  &value))
                value = atoi(optarg);

            value2 = value;

            s = strchr(optarg, '-');

            if ( s && s[1] &&
                !sscanf(s+1, "0x%x", &value2) &&
                !sscanf(s+1, "0X%x", &value2) &&
                !sscanf(s+1, "$%x",  &value2))
                value2 = atoi(s+1);
        }

        switch (c)
        {
            case 'G': allow_global_branches         = 1;    break;
            case 'E': no_exec_branches              = 1;    break;
            case 'b': allow_branch_target_wrap      = 1;    break;
            case 'B': allow_branch_to_bad_addr      = 1;    break;
            case 'p': suspicious_pc_math_is_invalid = 0;    break;
            case 'r': rare_ops_are_invalid          = 0;    break;
            case 'H': hlt_is_invalid                = 0;    break;
            case 'f': force_overwrite               = 1;    break;
            case 'S': no_default_symbols            = 1;    break;
            case 'X': no_exec_routine_symbols       = 1;    break;
            case 'v': verbose++;                            break;
            case 'e': add_entry_point(value);               break;
            case 'd': add_data_range(value,value2);         break;
            case 'g': add_generic_label(value);             break;

            case 'a': skip_advanced_analysis        = 1;    break;
            case '0': skip_mark_cart_header         = 1;    break;
            case '1': skip_funky_branch_detect      = 1;    break;
            case '2': skip_kill_bad_branches        = 1;    break;
            case '3': skip_brtrg_vs_sdbd            = 1;    break;
            case '4': skip_find_jsr_data            = 1;    break;
            case '5': skip_propagate_invalid        = 1;    break;
            case '6': skip_exec_sound_interp        = 1;    break;
            case '7': skip_exec_print               = 1;    break;
            case 'L': dont_loop_analysis            = 1;    break;



            case 'h': case '?': usage();                    break;
            case 'l': license();                            break;


            case 1:   debug_find_jsr_data++;                break;
            case 2:   debug_show_instr_flags        = 1;    break;

            default:
            {
                fprintf(stderr, "Unrecognized argument: '%c'\n", c);
                fprintf(stderr, "Use \"%s -h\" for usage info\n", argv[0]);
                exit(1);
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Must have at least three additional arguments:  Two input files     */
    /*  and an output file.                                                 */
    /* -------------------------------------------------------------------- */
    if (optind + 1 > argc)
    {
        fprintf(stderr, "ERROR:  Must provide at least one input file and "
                        "an output file\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Initialize our main Intellicart image.  We work by merging all      */
    /*  others into this one.                                               */
    /* -------------------------------------------------------------------- */
    icartrom_init(&icart);

    /* -------------------------------------------------------------------- */
    /*  Remaining arguments are .ROM image filenames.  The first N-1 names  */
    /*  are input files.  The last name is the output file.  The output     */
    /*  file is not allowed to be the same as any of the input files names  */
    /*  unless the overwrite flag is used.                                  */
    /* -------------------------------------------------------------------- */
    output_file = argv[argc - 1];
    if (!force_overwrite && (f = fopen(output_file, "r")))
    {
        fprintf(stderr, "ERROR:  Output file '%s' exists.\n"
                        "        Use \"-f\" flag to force overwrite.\n",
                        output_file);
        exit(1);
    }
    if ((f = fopen(output_file, "w")) == NULL)
    {
        fprintf(stderr, "ERROR:  Unable to open output file '%s'.\n",
                        output_file);
        exit(1);
    }

    for (i = optind; i < argc - 1; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Check for accidental over write of output file.                 */
        /* ---------------------------------------------------------------- */
        if (!force_overwrite && !strcmp(argv[i], output_file))
        {
            fprintf(stderr, "ERROR:  Input file '%s' overwrites output file\n"
                            "        Use \"-f\" flag to force overwrite.\n",
                            argv[i]);
            exit(1);
        }

        /* ---------------------------------------------------------------- */
        /*  Re-initialize the temporary Intellicart.                        */
        /* ---------------------------------------------------------------- */
        icartrom_init(&temp_icart);

        /* ---------------------------------------------------------------- */
        /*  Read in the requested file.                                     */
        /* ---------------------------------------------------------------- */
        icart_readfile(argv[i], &temp_icart, 1);

        /* ---------------------------------------------------------------- */
        /*  Merge this image into our total image.                          */
        /* ---------------------------------------------------------------- */
        merge_icarts(&icart, &temp_icart, 1);
    }


    /* -------------------------------------------------------------------- */
    /*  Set up default symbols.                                             */
    /* -------------------------------------------------------------------- */
    if (!no_exec_routine_symbols)
        setup_exec_routine_sym();

    if (!no_default_symbols)
        setup_defsym();

    /* -------------------------------------------------------------------- */
    /*  Do the disassembly.                                                 */
    /* -------------------------------------------------------------------- */
    do_disasm();

    /* -------------------------------------------------------------------- */
    /*  Write the output file.                                              */
    /* -------------------------------------------------------------------- */
    write_disasm(f);

    /* -------------------------------------------------------------------- */
    /*  Write branch cross-reference table.                                 */
    /* -------------------------------------------------------------------- */
    write_crossref(f);

    fclose(f);

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
/* ------------------------------------------------------------------------ */
/*                   Copyright (c) 2006, Joseph Zbiciak                     */
/* ======================================================================== */
