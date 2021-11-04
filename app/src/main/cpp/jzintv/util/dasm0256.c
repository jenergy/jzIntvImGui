/* ======================================================================== */
/*  DASM0256 -- Disassembler for the SP-0256 RESROM.                        */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "misc/avl.h"
#include "symtab.h"
#include "bitmem.h"

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

typedef struct dasm0256_t
{
    struct dasm0256_t *next;
    uint32_t addr;      /* Address of decoded instr.                */
    int      len;       /* Number of bits in this instr.            */
    char     mnc[16];   /* Pointer to instruction mnemonic          */
    char     op1[32];   /* Operand                                  */
    int      bf;        /* Bitfield description for encoding        */
    int      mode;      /* Mode that was in effect at disasm time   */
} dasm0256_t;

typedef struct bf_desc_t
{
    uint8_t     len;
    uint8_t     flag;   /* Bit 0 == Rev, 1 == Byte-Align, 2 == Mnemonic */
    const char *name;
} bf_desc_t;

int bf_desc_ofs[16 * 4];

#define BF_BITREV 1
#define BF_ALIGNB 2
#define BF_MNEMON 4

bf_desc_t bf_desc[] =
{
    /*  Opcode 0000, Mode 00    */
    {   8,  4,  "RTS/PAGE"      },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 0000, Mode 01    */
    {   8,  4,  "RTS/PAGE"      },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 0000, Mode 10    */
    {   8,  4,  "RTS/PAGE"      },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 0000, Mode 11    */
    {   8,  4,  "RTS/PAGE"      },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },


    /*  Opcode 0001, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOADALL.00"    },
    {   5,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   8,  1,  "B0"            },
    {   8,  1,  "F0"            },
    {   8,  1,  "B1"            },
    {   8,  1,  "F1"            },
    {   8,  1,  "B2"            },
    {   8,  1,  "F2"            },
    {   8,  1,  "B3"            },
    {   8,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   8,  1,  "Ampl Interp"   },
    {   8,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0001, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOADALL.01"    },
    {   5,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   8,  1,  "B0"            },
    {   8,  1,  "F0"            },
    {   8,  1,  "B1"            },
    {   8,  1,  "F1"            },
    {   8,  1,  "B2"            },
    {   8,  1,  "F2"            },
    {   8,  1,  "B3"            },
    {   8,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   8,  1,  "Ampl Interp"   },
    {   8,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0001, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOADALL.10"    },
    {   5,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   8,  1,  "B0"            },
    {   8,  1,  "F0"            },
    {   8,  1,  "B1"            },
    {   8,  1,  "F1"            },
    {   8,  1,  "B2"            },
    {   8,  1,  "F2"            },
    {   8,  1,  "B3"            },
    {   8,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   8,  1,  "Ampl Interp"   },
    {   8,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0001, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOADALL.11"    },
    {   5,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   8,  1,  "B0"            },
    {   8,  1,  "F0"            },
    {   8,  1,  "B1"            },
    {   8,  1,  "F1"            },
    {   8,  1,  "B2"            },
    {   8,  1,  "F2"            },
    {   8,  1,  "B3"            },
    {   8,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   8,  1,  "Ampl Interp"   },
    {   8,  1,  "Period Interp" },
    {   0,  0,  NULL            },


    /*  Opcode 0010, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_2"        },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   3,  1,  "B0 [S=0]"      },
    {   5,  1,  "F0"            },
    {   3,  1,  "B1 [S=0]"      },
    {   5,  1,  "F1"            },
    {   3,  1,  "B2 [S=0]"      },
    {   5,  1,  "F2"            },
    {   4,  1,  "B3 [S=0]"      },
    {   6,  1,  "F3"            },
    {   7,  1,  "B4"            },
    {   6,  1,  "F4"            },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0010, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_2.01"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   3,  1,  "B0 [S=0]"      },
    {   5,  1,  "F0"            },
    {   3,  1,  "B1 [S=0]"      },
    {   5,  1,  "F1"            },
    {   3,  1,  "B2 [S=0]"      },
    {   5,  1,  "F2"            },
    {   4,  1,  "B3 [S=0]"      },
    {   6,  1,  "F3"            },
    {   7,  1,  "B4"            },
    {   6,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0010, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_2.10"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "B0 [S=0]"      },
    {   6,  1,  "F0"            },
    {   6,  1,  "B1 [S=0]"      },
    {   6,  1,  "F1"            },
    {   6,  1,  "B2 [S=0]"      },
    {   6,  1,  "F2"            },
    {   6,  1,  "B3 [S=0]"      },
    {   7,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0010, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_2.11"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "B0 [S=0]"      },
    {   6,  1,  "F0"            },
    {   6,  1,  "B1 [S=0]"      },
    {   6,  1,  "F1"            },
    {   6,  1,  "B2 [S=0]"      },
    {   6,  1,  "F2"            },
    {   6,  1,  "B3 [S=0]"      },
    {   7,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },



    /*  Opcode 0011, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_3.00"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   5,  1,  "F0 5 MSBs"     },
    {   5,  1,  "F1 5 MSBs"     },
    {   5,  1,  "F2 5 MSBs"     },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0011, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_3.01"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   5,  1,  "F0 5 MSBs"     },
    {   5,  1,  "F1 5 MSBs"     },
    {   5,  1,  "F2 5 MSBs"     },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0011, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_3.10"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   6,  1,  "F0 6 MSBs"     },
    {   6,  1,  "F1 6 MSBs"     },
    {   6,  1,  "F2 6 MSBs"     },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },

    /*  Opcode 0011, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_3.11"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   6,  1,  "F0 6 MSBs"     },
    {   6,  1,  "F1 6 MSBs"     },
    {   6,  1,  "F2 6 MSBs"     },
    {   5,  1,  "Ampl Interp"   },
    {   5,  1,  "Period Interp" },
    {   0,  0,  NULL            },


    /*  Opcode 0100, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_4.00"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   4,  1,  "B3 [S=0]"      },
    {   6,  1,  "F3"            },
    {   7,  1,  "B4"            },
    {   6,  1,  "F4"            },
    {   0,  0,  NULL            },

    /*  Opcode 0100, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_4.01"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   4,  1,  "B3 [S=0]"      },
    {   6,  1,  "F3"            },
    {   7,  1,  "B4"            },
    {   6,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   0,  0,  NULL            },

    /*  Opcode 0100, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_4.10"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "B3 [S=0]"      },
    {   7,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   0,  0,  NULL            },

    /*  Opcode 0100, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_4.11"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "B3 [S=0]"      },
    {   7,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   0,  0,  NULL            },


    /*  Opcode 0101, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_5.00"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   5,  1,  "F0 5 MSBs"     },
    {   5,  1,  "F1 5 MSBs"     },
    {   5,  1,  "F2 5 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 0101, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_5.01"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   5,  1,  "F0 5 MSBs"     },
    {   5,  1,  "F1 5 MSBs"     },
    {   5,  1,  "F2 5 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 0101, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_5.10"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "F0 6 MSBs"     },
    {   6,  1,  "F1 6 MSBs"     },
    {   6,  1,  "F2 6 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 0101, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_5.11"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "F0 6 MSBs"     },
    {   6,  1,  "F1 6 MSBs"     },
    {   6,  1,  "F2 6 MSBs"     },
    {   0,  0,  NULL            },


    /*  Opcode 0110, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "OPCODE_6"      },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   6,  1,  "F3 6 MSBs"     },
    {   6,  1,  "F4 6 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 0110, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "OPCODE_6"      },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   6,  1,  "F3 6 MSBs"     },
    {   6,  1,  "F4 6 MSBs"     },
    {   8,  1,  "F5 8 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 0110, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "OPCODE_6"      },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   7,  1,  "F3 7 MSBs"     },
    {   8,  1,  "F4 8 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 0110, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "OPCODE_6"      },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   7,  1,  "F3 7 MSBs"     },
    {   8,  1,  "F4 8 MSBs"     },
    {   8,  1,  "F5 8 MSBs"     },
    {   0,  0,  NULL            },


    /*  Opcode 0111, Mode 00    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JMP"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 0111, Mode 01    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JMP"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 0111, Mode 10    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JMP"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 0111, Mode 11    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JMP"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },


    /*  Opcode 1000, Mode 00    */
    {   2,  0,  "Rpt MSBs"      },
    {   2,  0,  "Mode"          },
    {   4,  4,  "SETMODE"       },
    {   0,  0,  NULL            },

    /*  Opcode 1000, Mode 01    */
    {   2,  0,  "Rpt MSBs"      },
    {   2,  0,  "Mode"          },
    {   4,  4,  "SETMODE"       },
    {   0,  0,  NULL            },

    /*  Opcode 1000, Mode 10    */
    {   2,  0,  "Rpt MSBs"      },
    {   2,  0,  "Mode"          },
    {   4,  4,  "SETMODE"       },
    {   0,  0,  NULL            },

    /*  Opcode 1000, Mode 11    */
    {   2,  0,  "Rpt MSBs"      },
    {   2,  0,  "Mode"          },
    {   4,  4,  "SETMODE"       },
    {   0,  0,  NULL            },


    /*  Opcode 1001, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_9.00"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   3,  1,  "+/- B0 4 MSBs" },
    {   3,  1,  "+/- F0 5 MSBs" },
    {   3,  1,  "+/- B1 4 MSBs" },
    {   3,  1,  "+/- F1 5 MSBs" },
    {   3,  1,  "+/- B2 4 MSBs" },
    {   3,  1,  "+/- F2 5 MSBs" },
    {   3,  1,  "+/- B3 5 MSBs" },
    {   4,  1,  "+/- F3 6 MSBs" },
    {   4,  1,  "+/- B4 6 MSBs" },
    {   4,  1,  "+/- F4 6 MSBs" },
    {   0,  0,  NULL            },

    /*  Opcode 1001, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_9.01"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   3,  1,  "+/- B0 4 MSBs" },
    {   3,  1,  "+/- F0 5 MSBs" },
    {   3,  1,  "+/- B1 4 MSBs" },
    {   3,  1,  "+/- F1 5 MSBs" },
    {   3,  1,  "+/- B2 4 MSBs" },
    {   3,  1,  "+/- F2 5 MSBs" },
    {   3,  1,  "+/- B3 5 MSBs" },
    {   4,  1,  "+/- F3 6 MSBs" },
    {   4,  1,  "+/- B4 6 MSBs" },
    {   4,  1,  "+/- F4 6 MSBs" },
    {   5,  1,  "+/- B5 8 MSBs" },
    {   5,  1,  "+/- F5 8 MSBs" },
    {   0,  0,  NULL            },

    /*  Opcode 1001, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_9.10"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   4,  1,  "+/- B0 7 MSBs" },
    {   4,  1,  "+/- F0 6 MSBs" },
    {   4,  1,  "+/- B1 7 MSBs" },
    {   4,  1,  "+/- F1 6 MSBs" },
    {   4,  1,  "+/- B2 7 MSBs" },
    {   4,  1,  "+/- F2 6 MSBs" },
    {   4,  1,  "+/- B3 7 MSBs" },
    {   5,  1,  "+/- F3 7 MSBs" },
    {   5,  1,  "+/- B4 8 MSBs" },
    {   5,  1,  "+/- F4 8 MSBs" },
    {   0,  0,  NULL            },

    /*  Opcode 1001, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_9.11"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   4,  1,  "+/- B0 7 MSBs" },
    {   4,  1,  "+/- F0 6 MSBs" },
    {   4,  1,  "+/- B1 7 MSBs" },
    {   4,  1,  "+/- F1 6 MSBs" },
    {   4,  1,  "+/- B2 7 MSBs" },
    {   4,  1,  "+/- F2 6 MSBs" },
    {   4,  1,  "+/- B3 7 MSBs" },
    {   5,  1,  "+/- F3 7 MSBs" },
    {   5,  1,  "+/- B4 8 MSBs" },
    {   5,  1,  "+/- F4 8 MSBs" },
    {   5,  1,  "+/- B5 8 MSBs" },
    {   5,  1,  "+/- F5 8 MSBs" },
    {   0,  0,  NULL            },


    /*  Opcode 1010, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_A.00"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   5,  1,  "F0 5 MSBs"     },
    {   5,  1,  "F1 5 MSBs"     },
    {   5,  1,  "F2 5 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 1010, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_A.01"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   5,  1,  "F0 5 MSBs"     },
    {   5,  1,  "F1 5 MSBs"     },
    {   5,  1,  "F2 5 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 1010, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_A.10"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   6,  1,  "F0 6 MSBs"     },
    {   6,  1,  "F1 6 MSBs"     },
    {   6,  1,  "F2 6 MSBs"     },
    {   0,  0,  NULL            },

    /*  Opcode 1010, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "SETMSB_A.11"   },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   6,  1,  "F0 6 MSBs"     },
    {   6,  1,  "F1 6 MSBs"     },
    {   6,  1,  "F2 6 MSBs"     },
    {   0,  0,  NULL            },


    /*  Opcode 1011, Mode 00    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JSR"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 1011, Mode 01    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JSR"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 1011, Mode 10    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JSR"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 1011, Mode 11    */
    {   4,  0,  "BTrg MSBs"     },
    {   4,  4,  "JSR"           },
    {   8,  0,  "BTrg LSBs"     },
    {   0,  2,  "Align"         },
    {   0,  0,  NULL            },

    /*  Opcode 1100, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_C.00"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   3,  1,  "B0 [S=0]"      },
    {   5,  1,  "F0"            },
    {   3,  1,  "B1 [S=0]"      },
    {   5,  1,  "F1"            },
    {   3,  1,  "B2 [S=0]"      },
    {   5,  1,  "F2"            },
    {   4,  1,  "B3 [S=0]"      },
    {   6,  1,  "F3"            },
    {   7,  1,  "B4"            },
    {   6,  1,  "F4"            },
    {   0,  0,  NULL            },

    /*  Opcode 1100, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_C.01"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   3,  1,  "B0 [S=0]"      },
    {   5,  1,  "F0"            },
    {   3,  1,  "B1 [S=0]"      },
    {   5,  1,  "F1"            },
    {   3,  1,  "B2 [S=0]"      },
    {   5,  1,  "F2"            },
    {   4,  1,  "B3 [S=0]"      },
    {   6,  1,  "F3"            },
    {   7,  1,  "B4"            },
    {   6,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   0,  0,  NULL            },

    /*  Opcode 1100, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_C.10"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "B0 [S=0]"      },
    {   6,  1,  "F0"            },
    {   6,  1,  "B1 [S=0]"      },
    {   6,  1,  "F1"            },
    {   6,  1,  "B2 [S=0]"      },
    {   6,  1,  "F2"            },
    {   6,  1,  "B3 [S=0]"      },
    {   7,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   0,  0,  NULL            },

    /*  Opcode 1100, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_C.11"     },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   6,  1,  "B0 [S=0]"      },
    {   6,  1,  "F0"            },
    {   6,  1,  "B1 [S=0]"      },
    {   6,  1,  "F1"            },
    {   6,  1,  "B2 [S=0]"      },
    {   6,  1,  "F2"            },
    {   6,  1,  "B3 [S=0]"      },
    {   7,  1,  "F3"            },
    {   8,  1,  "B4"            },
    {   8,  1,  "F4"            },
    {   8,  1,  "B5"            },
    {   8,  1,  "F5"            },
    {   0,  0,  NULL            },


    /*  Opcode 1101, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_D.00"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   3,  1,  "+/- B3 5 MSBs" },
    {   4,  1,  "+/- F3 6 MSBs" },
    {   4,  1,  "+/- B4 7 MSBs" },
    {   4,  1,  "+/- F4 6 MSBs" },
    {   0,  0,  NULL            },

    /*  Opcode 1101, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_D.01"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   3,  1,  "+/- B3 5 MSBs" },
    {   4,  1,  "+/- F3 6 MSBs" },
    {   4,  1,  "+/- B4 7 MSBs" },
    {   4,  1,  "+/- F4 6 MSBs" },
    {   5,  1,  "+/- B5 8 MSBs" },
    {   5,  1,  "+/- F5 8 MSBs" },
    {   0,  0,  NULL            },

    /*  Opcode 1101, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_D.10"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   4,  1,  "+/- B3 7 MSBs" },
    {   5,  1,  "+/- F3 7 MSBs" },
    {   5,  1,  "+/- B4 8 MSBs" },
    {   5,  1,  "+/- F4 8 MSBs" },
    {   0,  0,  NULL            },

    /*  Opcode 1101, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "DELTA_D.11"    },
    {   4,  1,  "+/- AmpMant"   },
    {   5,  1,  "+/- Period"    },
    {   4,  1,  "+/- B3 7 MSBs" },
    {   5,  1,  "+/- F3 7 MSBs" },
    {   5,  1,  "+/- B4 8 MSBs" },
    {   5,  1,  "+/- F4 8 MSBs" },
    {   5,  1,  "+/- B5 8 MSBs" },
    {   5,  1,  "+/- F5 8 MSBs" },
    {   0,  0,  NULL            },


    /*  Opcode 1110, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_E"        },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   0,  0,  NULL            },

    /*  Opcode 1110, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_E"        },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   0,  0,  NULL            },

    /*  Opcode 1110, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_E"        },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   0,  0,  NULL            },

    /*  Opcode 1110, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "LOAD_E"        },
    {   3,  1,  "AmpMant"       },
    {   3,  1,  "AmpExp"        },
    {   8,  1,  "Period"        },
    {   0,  0,  NULL            },


    /*  Opcode 1111, Mode 00    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "PAUSE"         },
    {   0,  0,  NULL            },

    /*  Opcode 1111, Mode 01    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "PAUSE"         },
    {   0,  0,  NULL            },

    /*  Opcode 1111, Mode 10    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "PAUSE"         },
    {   0,  0,  NULL            },

    /*  Opcode 1111, Mode 11    */
    {   4,  1,  "Repeat"        },
    {   4,  4,  "PAUSE"         },
    {   0,  0,  NULL            },

    /*  Sentinel Value!         */
    {   255, 255,   NULL        },
};

LOCAL void init_bf_desc(void)
{
    int i, j;

    bf_desc_ofs[0] = 0;

    for (i = 1, j = 1; (bf_desc[i].flag & 128) == 0 && j < 64; i++)
        if (bf_desc[i].name == NULL)
            bf_desc_ofs[j++] = i + 1;

    if (j != 64)
    {
        fprintf(stderr, "bf_desc table error!  i=%d j=%d\n", i, j);
        exit(1);
    }

#if 0
    for (i = 0; i < 64; i++)
    {
        printf("Opcode %.1X, Mode %d%d:\n", i >> 2, (i >> 1) & 1, i & 1);
        for (j = bf_desc_ofs[i]; bf_desc[j].name; j++)
        {
            printf("   %3d: %3d, %3d, %s\n",
                    j, bf_desc[j].len, bf_desc[j].flag, bf_desc[j].name);
        }
    }
#endif
}


#define MAKE_CODE(m,a,l)  bitmem_setattr((m), (a), (l), AT_CODE)
#define MAKE_OPER(m,a,l)  bitmem_setattr((m), (a), (l), AT_OPER)
#define MAKE_DATA(m,a,l)  bitmem_setattr((m), (a), (l), AT_DATA), \
                          bitmem_setattr((m), (a),  1,  AT_DATAB)
#define MAKE_RDATA(m,a,l) bitmem_setattr((m), (a), (l), AT_DATA|AT_BITREV),\
                          bitmem_setattr((m), (a),  1,  AT_DATAB)
#define MAKE_BTRG(m,a,l)  bitmem_setattr((m), (a), (l), AT_BTRG)

#define IS_CODE(m,a,l)  ((bitmem_getattr((m), (a), (l)) & AT_CODE ) != 0)
#define IS_OPER(m,a,l)  ((bitmem_getattr((m), (a), (l)) & AT_OPER ) != 0)
#define IS_DATA(m,a,l)  ((bitmem_getattr((m), (a), (l)) & AT_DATA ) != 0)
#define IS_DATAB(m,a,l) ((bitmem_getattr((m), (a), (l)) & AT_DATAB) != 0 || \
                         (bitmem_getattr((m), (a), (l)) & AT_DATA ) == 0)
#define IS_BTRG(m,a,l)  ((bitmem_getattr((m), (a), (l)) & AT_BTRG ) != 0)
#define IS_LOCAL(m,a,l) ((bitmem_getattr((m), (a), (l)) & AT_LOCAL) != 0)
#define IS_REV(m,a,l)   ((bitmem_getattr((m), (a), (l)) & AT_BITREV)!= 0)

static symtab_t *symtab = NULL;

/* ------------------------------------------------------------------------ */
/*  BITFIELD         -- Render a bitfield into a string.                    */
/* ------------------------------------------------------------------------ */
LOCAL void bitfield(char *str, uint32_t bits, int len)
{
    str[len] = 0;
    while (len)
    {
        str[--len] = '0' + (bits & 1);
        bits >>= 1;
    }
}

/* ------------------------------------------------------------------------ */
/*  DASM0256_READSYM -- Process a .SYM file which defines our starting      */
/*                      symbol table.  Sets our origin, and memory attribs  */
/*                      too.                                                */
/* ------------------------------------------------------------------------ */
static const char *sym_cmd[] =
{
    "code",
    "data",
    "org",
    "oper",
    "sym",
    0
};

#define REQ1_MAY2 if (nargs < 2 || v1 == -1 || (nargs == 3 && v2 == -1)) \
                  fprintf(stderr, "ERROR: %s requires 1 or 2 numeric args\n",\
                          w[0]), exit(1)

#define REQ1_ONLY if (nargs != 2 || v1 == -1) \
                  fprintf(stderr,"ERROR: %s requires exactly 1 numeric arg\n",\
                          w[0]), exit(1)

#define REQ2_SYMB if (nargs != 3 || v2 == -1) \
                  fprintf(stderr,"ERROR: %s requires 2 args, and the second "\
                          "args must be numeric\n", w[0]), exit(1)

LOCAL void dasm0256_readsym(FILE *f, uint32_t *addr_lo, bitmem_t *mem)
{
    char buf[1024], *s1, *s2;
    char *w[3] = { NULL, NULL, NULL };
    int  v1 = -1, v2 = -1;
    int  ws, is, i, tl, cmd, nargs;

    if (!mem && addr_lo)
    {
        *addr_lo = 0;                               /* Default origin is 0. */
    }

    /* -------------------------------------------------------------------- */
    /*  Rewind the symbol file.                                             */
    /* -------------------------------------------------------------------- */
    rewind(f);

    /* -------------------------------------------------------------------- */
    /*  Process the commands. All commands are of the same general format:  */
    /*      word [word [word]]                                              */
    /*  The second and third words can be hexadecimal numbers.              */
    /* -------------------------------------------------------------------- */
    while (fgets(buf, 1024, f))
    {
        /* ---------------------------------------------------------------- */
        /*  First, compress away redundant whitespace.                      */
        /*  Make first word lowercase, and turn whitespace into spaces.     */
        /* ---------------------------------------------------------------- */
        ws = 1;
        tl = 1;
        for (s1 = s2 = buf; *s1; s1++)
        {
            if (*s1 == ';') break;  /* Stop at ; for comments. */
            is = isspace(*s1);
            if (ws && is) continue;

            *s2++ = is ? ' ' : tl ? tolower(*s1) : *s1;
            ws = is;
            if (is) tl = 0;
        }
        *s2 = 0;

        /* ---------------------------------------------------------------- */
        /*  Do early abort if line comes up empty, or is a comment.         */
        /* ---------------------------------------------------------------- */
        if (s2 - buf < 2) continue;

        /* ---------------------------------------------------------------- */
        /*  Next, strtok out any words.                                     */
        /* ---------------------------------------------------------------- */
        w[1] = w[2] = NULL;
                  { w[0] = strtok(buf,  " "); nargs = 0; }
        if (w[0]) { w[1] = strtok(NULL, " "); nargs = 1; }
        if (w[1]) { w[2] = strtok(NULL, " "); nargs = 2; }
        if (w[2]) {                           nargs = 3; }

        if (nargs == 0) continue;  /* found nothing? */

        /* ---------------------------------------------------------------- */
        /*  Last, decode any hex values.                                    */
        /* ---------------------------------------------------------------- */
        if (w[1]) if (sscanf(w[1], "%x", &v1) == 0) v1 = -1;
        if (w[2]) if (sscanf(w[2], "%x", &v2) == 0) v2 = -1;

        /* ---------------------------------------------------------------- */
        /*  Identify which command it was.                                  */
        /* ---------------------------------------------------------------- */
        for (cmd = 0, i = -1; sym_cmd[cmd]; cmd++)
        {
            i = strcmp(sym_cmd[cmd], w[0]);
            if (i >= 0) break;
        }

        /* ---------------------------------------------------------------- */
        /*  Complain if we didn't find it.                                  */
        /* ---------------------------------------------------------------- */
        if (i != 0)
        {
            fprintf(stderr, "WARNING:  Unknown command '%s' in sym file\n",
                    w[0]);
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Process the command!                                            */
        /* ---------------------------------------------------------------- */
        switch (cmd)
        {
            case 0: /* CODE */
            {
                REQ1_MAY2;
                if (!mem) break;

                if (v2 == -1 || nargs == 2) v2 = v1;
                for (i = v1; i <= v2; i++) MAKE_CODE(mem, v1 << 3, 8);
                break;
            }
            case 1: /* DATA */
            {
                REQ1_MAY2;
                if (!mem) break;

                if (v2 == -1 || nargs == 2) v2 = v1;
                for (i = v1; i <= v2; i++) MAKE_DATA(mem, v1 << 3, 8);
                break;
            }
            case 3: /* OPER */
            {
                REQ1_MAY2;
                if (!mem) break;

                if (v2 == -1 || nargs == 2) v2 = v1;
                for (i = v1; i <= v2; i++) MAKE_OPER(mem, v1 << 3, 8);
                break;
            }
            case 2: /* ORG */
            {
                REQ1_ONLY;
                if (addr_lo)
                    *addr_lo = v1 << 3;     /*  Convert to bit-address. */
                break;
            }
            case 4: /* SYM */
            {
                REQ2_SYMB;

                symtab_defsym(symtab, w[1], v2 << 3);
                if (mem) MAKE_BTRG(mem, v2 << 3, 1);
                break;
            }

            default:
            {
                fprintf(stderr, "READSYM Internal error: cmd=%d\n", cmd);
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------------ */
/*  DASM0256_DECODE  -- decode the instruction at a given address.          */
/*                      Fills a dasm0256_t struct, and returns # of bits.   */
/* ------------------------------------------------------------------------ */
static int brtbl[16] = { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
                         0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF };

#define MNEMONIC(d,v)   strcpy((d)->mnc, (v));
#define OP1_NIB(d,v)    snprintf((d)->op1, 32, "$%.1X", (v))
#define OP1_MODE(d,v)   snprintf((d)->op1, 32, "RPT=$%.1X?, MODE=%d%d", \
                                brtbl[(v) & 0xC], ((v) & 2) >> 1, (v) & 1);
#define OP1_RNIB(d,v)   snprintf((d)->op1, 32, "RPT=$%.1X%.1X", \
                                brtbl[*mode & 0xC], brtbl[(v)])
#define OP1_PAGE(d,v)   snprintf((d)->op1, 32, "PAGE=$%.1Xxxx", \
                                ((v) & 0xF0) >> 4)
#define OP1_DEC(d,v)    snprintf((d)->op1, 32, "%d", (v))
#define OP1_BYTE(d,v)   snprintf((d)->op1, 32, "$%.2X", (v))
#define OP1_WORD(d,v)   snprintf((d)->op1, 32, "$%.4X", (v))
#define OP1_LOC(d,m,v)  snprintf((d)->op1, 32, "%s",\
                                symtab_getsym(symtab,(v)<<3,(m),0))

LOCAL int dasm0256_decode
(
    bitmem_t   *mem,        /* ROM image to decode from.        */
    uint32_t    addr,       /* Bit-Address to decode at.        */
    dasm0256_t *dasm,       /* Disassembly structure.           */
    int        *mode        /* Mode bits from Opcode 8.         */
)
{
    int len = 0;
    int i;
    int opcode;
    int immed4;
    int bf, bf_len;

    /* -------------------------------------------------------------------- */
    /*  Start out by clearing the structure and poking in a few values      */
    /*  that we know right off the bat.                                     */
    /* -------------------------------------------------------------------- */
    memset(dasm, 0, sizeof(dasm0256_t));

    immed4 = bitmem_read_fwd(mem, addr,     4); MAKE_OPER(mem, addr,     4);
    opcode = bitmem_read_fwd(mem, addr + 4, 4); MAKE_CODE(mem, addr + 4, 4);
    dasm->addr = addr;
    dasm->mode = *mode;
    dasm->bf   = bf = bf_desc_ofs[opcode * 4 + (*mode & 3)];

    /* -------------------------------------------------------------------- */
    /*  Next, step through the bit-field description for this opcode and    */
    /*  count up how large the instruction is, and find its mnemonic, etc.  */
    /* -------------------------------------------------------------------- */
    for (i = bf; bf_desc[i].name; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Get the bit-field length.  Handle byte-alignment if needed.     */
        /* ---------------------------------------------------------------- */
        if (bf_desc[i].flag & BF_ALIGNB)
        {
            bf_len = 7 & -(addr + len);
        } else
        {
            bf_len = bf_desc[i].len;
        }

        /* ---------------------------------------------------------------- */
        /*  Is this our mnemonic?  If so, grab the name.  Else, these are   */
        /*  operand bits, so mark them as such.                             */
        /* ---------------------------------------------------------------- */
        if (bf_desc[i].flag & BF_MNEMON)
        {
            MNEMONIC(dasm, bf_desc[i].name);
        } else
        {
            MAKE_OPER(mem, addr + len, bf_len);
        }

        len += bf_len;
    }

    /* -------------------------------------------------------------------- */
    /*  Record the final length of the instruction.                         */
    /* -------------------------------------------------------------------- */
    dasm->len = len;

    /* -------------------------------------------------------------------- */
    /*  Lastly, do some opcode-specific fixups, such as label-resolution    */
    /*  for branches, mode changes for SETMODE, etc.                        */
    /* -------------------------------------------------------------------- */
    switch (opcode)
    {
        case 0x00:  /* RTS / SETPAGE */
        {
            if (immed4 != 0)
            {
                *mode &= ~0xF0;
                *mode |= immed4 << 4;

                MNEMONIC(dasm,"SETPAGE");
                OP1_NIB(dasm,immed4);

                /* SETPAGE is not a control transfer, so no align happens */
                dasm->len = len = 8;
            } else
            {
                int maybe_btrg;

                MNEMONIC(dasm,"RTS");

                /* Mark instruction after as a branch target if it's not RTS */
                maybe_btrg = dasm->addr + dasm->len;
                if (bitmem_read_fwd(mem, maybe_btrg, 8) != 0)
                    MAKE_BTRG(mem, maybe_btrg, 1);

                *mode &= ~0xF0;
                *mode |= 0xF0 & ((dasm->addr + 8) >> 11);
            }

            break;
        }

        case 0x08:  /* SETMODE */
        {
            OP1_MODE(dasm, immed4);
            *mode = (*mode & ~0x0F) | immed4;
            break;
        }

        case 0x07:  /* JMP      */
        {
            int maybe_btrg;

            /* Mark instruction after JMP as a branch target if it's not RTS */
            MAKE_BTRG(mem, dasm->addr + dasm->len, 1);
            maybe_btrg = dasm->addr + dasm->len;
            if (bitmem_read_fwd(mem, maybe_btrg, 8) != 0)
                MAKE_BTRG(mem, maybe_btrg, 1);

            FALLTHROUGH_INTENDED;
        }
        case 0x0B:  /* JSR      */
        {
            int btrg;

            btrg  = (immed4 << 8) | bitmem_read_fwd(mem, addr + 8, 8);
            btrg |= (*mode & 0xF0) << 8;
            MAKE_BTRG(mem, btrg << 3, 1);
            OP1_LOC (dasm, (IS_LOCAL(mem, btrg<<3, 8) ? 'L' : 'G'), btrg);

            symtab_xref_addr(symtab, btrg << 3, dasm->addr);

            break;
        }

        default:
        {
            OP1_RNIB(dasm, immed4);
            break;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  If this wasn't opcode 8, clear the repeat MSBs from the mode.       */
    /*  If this wasn't opcode 0, clear the page set by SETPAGE.             */
    /* -------------------------------------------------------------------- */
    if (opcode != 0x08) *mode &= ~0x0C;
    if (opcode != 0x00)
        *mode = (*mode & ~0xF0) | (((dasm->addr + dasm->len) >> 11) & 0xF0);

    /* -------------------------------------------------------------------- */
    /*  All done.  Return our length.                                       */
    /* -------------------------------------------------------------------- */
    return len;
}


/* ------------------------------------------------------------------------ */
/*  DASM0256_DISPLAY -- Display a decoded instruction to a file.            */
/* ------------------------------------------------------------------------ */
LOCAL void dasm0256_display(FILE *f, dasm0256_t *decoded, bitmem_t *bitmem)
{
    const char *label;
    char buf[64];
    int  i, j, k, l, bf_len, tot_len = 0;
    uint32_t addr, addr_s;

    /* -------------------------------------------------------------------- */
    /*  First, find any labels that might've been assigned to this instr.   */
    /* -------------------------------------------------------------------- */
    label = symtab_getsym(symtab, decoded->addr,
                          IS_BTRG(bitmem, decoded->addr, 1) ? 'L' : 0, 0);

    if (label)
    {
        symtab_dref_addr(symtab, decoded->addr);
        fputs("##-------------------------------------"
              "---------------------------------------\n", f);

        i = 1;
        do
        {
            fprintf(f, "## %s:\n", label);
            label = symtab_getsym(symtab, decoded->addr, 0, i++);
        } while (label);
    } else
    {
        fputc('\n', f);
        buf[0] = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Now, display the mnemonic and other stuff to the user.              */
    /* -------------------------------------------------------------------- */
    fprintf(f, "# $%.4X.%X  %-16s%-16s\n",
            decoded->addr >> 3, decoded->addr & 7, decoded->mnc, decoded->op1);

    /* -------------------------------------------------------------------- */
    /*  Lastly, pull apart all of the bit fields to show the encoding.      */
    /* -------------------------------------------------------------------- */
    addr = decoded->addr;

    for (i = decoded->bf; bf_desc[i].name && tot_len < decoded->len; i++)
    {
        bf_len = bf_desc[i].len;

        if (bf_desc[i].flag & BF_ALIGNB)
            bf_len = 7 & -addr;

        tot_len += bf_len;

        if (tot_len > decoded->len) bf_len -= tot_len - decoded->len;

        for (j = 0; j < bf_len; j += 24)
        {
            fputs("    ", f);
            addr_s = addr;
            for (k = 0; k < 24; k += 4)
            {
                l = bf_len - (j + k);

                if (l > 4) l = 4;

                if (l < 0) { buf[0] = 0; l = 0; }
                else       bitfield(buf, bitmem_read_fwd(bitmem,addr,l),l);

                fprintf(f, " %-4.4s", buf);

                addr += l;
            }

            fprintf(f, "  # $%.4X.%X .. $%.4X.%X:  %s%s%s\n",
                    (addr_s) >> 3, (addr_s) & 7,
                    (addr-1) >> 3, (addr-1) & 7,
                    bf_desc[i].name,
                    bf_desc[i].flag & BF_BITREV ? " (br)" : "",
                    j > 0 ? " (cont)" : "");
        }
    }
}

bitmem_t   *bitmem = NULL;
dasm0256_t *decode_head = NULL, *decode_tail = NULL;
int main(int argc, char *argv[])
{
    uint32_t addr_lo = 0x1000 << 3, addr_hi = 0x1000 << 3;
    int i, l;
    FILE *rom, *sym;
    int len, mode = 0;
    dasm0256_t *decoded = NULL;

    /* -------------------------------------------------------------------- */
    /*  Initialize things, such as our bitfield descriptions and symbol     */
    /*  table.  Also do some argument parsing.                              */
    /* -------------------------------------------------------------------- */
    init_bf_desc();

    symtab = symtab_create();

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "%s rom.bin [symfile]\n", argv[0]);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If we have a sym file, do the first pass over it right now.         */
    /* -------------------------------------------------------------------- */
    if (argc == 3)
    {
        /* ---------------------------------------------------------------- */
        /*  Open our symbol file.  Complain and die if we can't.            */
        /* ---------------------------------------------------------------- */
        sym = fopen(argv[2], "r");
        if (!sym)
        {
            fprintf(stderr,"ERROR:  Could not read symbol file '%s'\n",
                    argv[2]);
            exit(1);
        }

        /* ---------------------------------------------------------------- */
        /*  Process it.  This will set our origin and define some symbols.  */
        /* ---------------------------------------------------------------- */
        dasm0256_readsym(sym, &addr_lo, NULL);

#if 0 /* This is disabled because it's obnoxious */
        /* ---------------------------------------------------------------- */
        /*  If our ROM's origin falls in the range 0000...01FE, define      */
        /*  the SP0256 set of external entry points as labels.              */
        /* ---------------------------------------------------------------- */
        if (addr_lo < 0x200)
        {
            for (i = 0; i < 0xFF; i++)
            {
                snprintf(labbuf, sizeof(labbuf), "_ext%.2X", i);
                symtab_defsym(symtab, labbuf, i << 4);
            }
        }
#endif

    } else
    {
        sym = NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Read in our ROM image.  Do this by figuring out how big it is,      */
    /*  and then telling bitmem to suck it in.                              */
    /* -------------------------------------------------------------------- */
    rom = fopen(argv[1], "rb");
    if (!rom)
    {
        fprintf(stderr, "Could not open ROM file '%s'\n", argv[1]);
        exit(1);
    }

    fseek(rom, 0, SEEK_END);
    len = ftell(rom) << 3;
    addr_hi = addr_lo + len;
    rewind(rom);

    bitmem = bitmem_create(addr_lo, len);
    bitmem_load(bitmem, rom);

    fclose(rom);

    /* -------------------------------------------------------------------- */
    /*  Do the second pass over the sym file, if we have one.               */
    /* -------------------------------------------------------------------- */
    if (sym)
    {
        dasm0256_readsym(sym, NULL, bitmem);
        fclose(sym);
    }

    /* -------------------------------------------------------------------- */
    /*  Decode the instructions.                                            */
    /* -------------------------------------------------------------------- */
    fprintf(stderr, "Pass 1:  Decoding instructions.\n");
    mode = (addr_lo >> 11) & 0xF0;
    for (i = 0; i < (int)(addr_hi - addr_lo); i += l)
    {
        decoded = CALLOC(dasm0256_t, 1);
        if (!decoded) { fprintf(stderr, "Out of memory\n"); exit(1); }

        l = dasm0256_decode(bitmem, i + addr_lo, decoded, &mode);

        if (!decode_tail)
        {
            decode_head = decode_tail = decoded;
        } else
        {
            decode_tail->next = decoded;
            decode_tail       = decoded;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Display the output.                                                 */
    /* -------------------------------------------------------------------- */
    fprintf(stderr, "Pass 2:  Displaying output.\n");


    printf("##-------------------------------------"
           "---------------------------------------\n");
    printf("## Disassembled output");
    printf("\n##-------------------------------------"
           "---------------------------------------\n\n");
    for (decoded = decode_head; decoded; decoded = decoded->next)
        dasm0256_display(stdout, decoded, bitmem);

    /* Dump the symbol table */
    symtab_dump_by_syms(symtab, stdout);
    symtab_dump_by_addr(symtab, stdout);
    symtab_dump_xrefs  (symtab, stdout);


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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
