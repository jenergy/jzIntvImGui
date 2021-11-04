/*
 * ============================================================================
 *  Title:    Event binding tables.
 *  Author:   J. Zbiciak, R. Reynolds (GP2X)
 * ============================================================================
 *  These tables specify the bindable events and the default bindings.
 * ============================================================================
 */

#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/emu_link.h"
#include "mem/mem.h"
#include "ecs/ecs.h"
#include "bincfg/bincfg.h"
#include "bincfg/legacy.h"
#include "icart/icart.h"
#include "pads/pads.h"
#include "pads/pads_cgc.h"
#include "pads/pads_intv2pc.h"
#include "avi/avi.h"
#include "gfx/gfx.h"
#include "gfx/palette.h"
#include "snd/snd.h"
#include "ay8910/ay8910.h"
#include "demo/demo.h"
#include "stic/stic.h"
#include "ivoice/ivoice.h"
#include "speed/speed.h"
#include "debug/debug_.h"
#include "event/event.h"
#include "joy/joy.h"
#include "serializer/serializer.h"
#include "jlp/jlp.h"
#include "locutus/locutus_adapt.h"
#include "cheat/cheat.h"
#include "mapping.h"
#include "cfg.h"
#include <errno.h>


#define W(word) ((uint32_t*)&intv.word)

/* ------------------------------------------------------------------------ */
/*  jzIntv internal event action table.  Keyboard and joystick inputs may   */
/*  be bound to any of these actions.  This table also ties the actions     */
/*  to the actual bits that the event action fiddles with.                  */
/*                                                                          */
/*  Notes on mnemonics:                                                     */
/*      PD0L        Left controller pad on base unit.                       */
/*      PD0R        Right controller pad on base unit.                      */
/*      PD1L        Left controller pad on ECS unit.                        */
/*      PD1R        Right controller pad on ECS unit.                       */
/*      PDxx_KP     Left Key Pad                                            */
/*      PDxx_A      Left Action Button  ([T]op, [L]eft, [R]ight)            */
/*      PDxx_D      Right Disc                                              */
/*      N, NE, etc  Compass directions.                                     */
/*                                                                          */
/*  The bit patterns at the right are AND and OR masks.  The first two      */
/*  are the AND masks for release/press.  The second two are OR masks       */
/*  for release/press.                                                      */
/* ------------------------------------------------------------------------ */
cfg_evtact_t  cfg_event_action[] =
{
    /* -------------------------------------------------------------------- */
    /*  Miscellaneous.                                                      */
    /* -------------------------------------------------------------------- */
    { "QUIT",       W(do_exit           ),  { 1,   1,  },   { 1,   1   } },
    { "RESET",      W(do_reset          ),  { 0,   ~0U },   { 0,   ~0U } },
    { "DUMP",       W(do_dump           ),  { ~0U, 0   },   { 0,   1   } },
    { "LOAD",       W(do_load           ),  { ~0U, 0   },   { 0,   1   } },
    { "RELOAD",     W(do_reload         ),  { ~0U, 0   },   { 0,   1   } }, 
    { "MOVIE",      W(gfx.scrshot       ),  { ~0U, ~0U },   { GFX_MVTOG, 0} },
    { "AVI",        W(gfx.scrshot       ),  { ~0U, ~0U },   { GFX_AVTOG, 0} },
    { "SHOT",       W(gfx.scrshot       ),  { ~0U, ~0U },   { GFX_SHOT,  0} },
    { "GRAMSHOT",   W(stic.debug_flags  ),  { ~0U, ~0U },   { STIC_GRAMSHOT,0}},
    { "HIDE",       W(gfx.hidden        ),  { 0,   1   },   { 0,   1   } },
    { "WTOG",       W(gfx.toggle        ),  { 0,   0   },   { GFX_WIND_TOG,0}},
    { "WINDOW",     W(gfx.toggle        ),  { 0,   0   },   { GFX_WIND_ON, 0}},
    { "FULLSC",     W(gfx.toggle        ),  { 0,   0   },   { GFX_WIND_OFF,0}},
    { "BREAK",      W(debug.step_count  ),  { ~0U, 0   },   { 0,   0   } },
    { "VOLUP",      W(snd.change_vol    ),  { ~0U, 0   },   { 0,   1   } },
    { "VOLDN",      W(snd.change_vol    ),  { ~0U, 0   },   { 0,   2   } },
    { "NA",         NULL,                   { 0,   0   },   { 0,   0   } },

    /* -------------------------------------------------------------------- */
    /*  A rich set of pause actions, so we can tie them to things such as   */
    /*  a window gaining/losing focus, or getting hidden/unhidden.          */
    /* -------------------------------------------------------------------- */
    { "PAUSE",      W(do_pause),  { ~0U, 0   },   { 0,         PAUSE_TOG } },
    { "PAUSE_ON",   W(do_pause),  { ~0U, 0   },   { 0,         PAUSE_ON  } },
    { "PAUSE_OFF",  W(do_pause),  { ~0U, 0   },   { 0,         PAUSE_OFF } },
    { "PAUSE_HOLD", W(do_pause),  { 0,   0   },   { PAUSE_OFF, PAUSE_ON  } },

    /* -------------------------------------------------------------------- */
    /*  Input map selection.                                                */
    /* -------------------------------------------------------------------- */
#   define CHG_EVT_MAP(name, push, pop)                         \
        {                                                       \
            name, W(chg_evt_map),                               \
            { EV_MAP_##pop == EV_MAP_NOP ? ~0U : 0, 0 },        \
            { EV_MAP_##pop, EV_MAP_##push }                     \
        } 

    CHG_EVT_MAP("KBD0",    SET_0, NOP),
    CHG_EVT_MAP("KBD1",    SET_1, NOP),
    CHG_EVT_MAP("KBD2",    SET_2, NOP),
    CHG_EVT_MAP("KBD3",    SET_3, NOP),
    CHG_EVT_MAP("KBDn",    NEXT,  NOP),
    CHG_EVT_MAP("KBDp",    PREV,  NOP),

    CHG_EVT_MAP("SETMAP0", SET_0, NOP),
    CHG_EVT_MAP("SETMAP1", SET_1, NOP),
    CHG_EVT_MAP("SETMAP2", SET_2, NOP),
    CHG_EVT_MAP("SETMAP3", SET_3, NOP),
    CHG_EVT_MAP("NEXTMAP", NEXT,  NOP),
    CHG_EVT_MAP("PREVMAP", PREV,  NOP),

    CHG_EVT_MAP("SHF10",   SET_1, SET_0),
    CHG_EVT_MAP("SHF20",   SET_2, SET_0),
    CHG_EVT_MAP("SHF30",   SET_3, SET_0),
    CHG_EVT_MAP("SHF01",   SET_0, SET_1),
    CHG_EVT_MAP("SHF21",   SET_2, SET_1),
    CHG_EVT_MAP("SHF31",   SET_3, SET_1),
    CHG_EVT_MAP("SHF02",   SET_0, SET_2),
    CHG_EVT_MAP("SHF12",   SET_1, SET_2),
    CHG_EVT_MAP("SHF32",   SET_3, SET_2),
    CHG_EVT_MAP("SHF03",   SET_0, SET_3),
    CHG_EVT_MAP("SHF13",   SET_1, SET_3),
    CHG_EVT_MAP("SHF23",   SET_2, SET_3),

    CHG_EVT_MAP("PSH0",    PSH_0, POP),
    CHG_EVT_MAP("PSH1",    PSH_1, POP),
    CHG_EVT_MAP("PSH2",    PSH_2, POP),
    CHG_EVT_MAP("PSH3",    PSH_3, POP),

    CHG_EVT_MAP("POP",     POP,   NOP),
    CHG_EVT_MAP("POP_UP",  NOP,   POP),  /* Pop on release */

#   undef CHG_EVT_MAP

    /* -------------------------------------------------------------------- */
    /*  Cheat events.                                                       */
    /* -------------------------------------------------------------------- */
    { "CHEAT0",     W(cheat.request),       { ~0U, ~0U },   { 0x01, 0  } },
    { "CHEAT1",     W(cheat.request),       { ~0U, ~0U },   { 0x02, 0  } },
    { "CHEAT2",     W(cheat.request),       { ~0U, ~0U },   { 0x04, 0  } },
    { "CHEAT3",     W(cheat.request),       { ~0U, ~0U },   { 0x08, 0  } },
    { "CHEAT4",     W(cheat.request),       { ~0U, ~0U },   { 0x10, 0  } },
    { "CHEAT5",     W(cheat.request),       { ~0U, ~0U },   { 0x20, 0  } },
    { "CHEAT6",     W(cheat.request),       { ~0U, ~0U },   { 0x40, 0  } },
    { "CHEAT7",     W(cheat.request),       { ~0U, ~0U },   { 0x80, 0  } },

    /* -------------------------------------------------------------------- */
    /*  PAD0: Left-hand controller keypad                                   */
    /* -------------------------------------------------------------------- */
    { "PD0L_KP1",   W(pad0.l[1]         ),  { 0,    ~0U  }, { 0,    0x81 } },
    { "PD0L_KP2",   W(pad0.l[2]         ),  { 0,    ~0U  }, { 0,    0x41 } },
    { "PD0L_KP3",   W(pad0.l[3]         ),  { 0,    ~0U  }, { 0,    0x21 } },
    { "PD0L_KP4",   W(pad0.l[4]         ),  { 0,    ~0U  }, { 0,    0x82 } },
    { "PD0L_KP5",   W(pad0.l[5]         ),  { 0,    ~0U  }, { 0,    0x42 } },
    { "PD0L_KP6",   W(pad0.l[6]         ),  { 0,    ~0U  }, { 0,    0x22 } },
    { "PD0L_KP7",   W(pad0.l[7]         ),  { 0,    ~0U  }, { 0,    0x84 } },
    { "PD0L_KP8",   W(pad0.l[8]         ),  { 0,    ~0U  }, { 0,    0x44 } },
    { "PD0L_KP9",   W(pad0.l[9]         ),  { 0,    ~0U  }, { 0,    0x24 } },
    { "PD0L_KPC",   W(pad0.l[10]        ),  { 0,    ~0U  }, { 0,    0x88 } },
    { "PD0L_KP0",   W(pad0.l[0]         ),  { 0,    ~0U  }, { 0,    0x48 } },
    { "PD0L_KPE",   W(pad0.l[11]        ),  { 0,    ~0U  }, { 0,    0x28 } },

    /* -------------------------------------------------------------------- */
    /*  PAD0: Right-hand controller keypad                                  */
    /* -------------------------------------------------------------------- */
    { "PD0R_KP1",   W(pad0.r[1]         ),  { 0,    ~0U  }, { 0,    0x81 } },
    { "PD0R_KP2",   W(pad0.r[2]         ),  { 0,    ~0U  }, { 0,    0x41 } },
    { "PD0R_KP3",   W(pad0.r[3]         ),  { 0,    ~0U  }, { 0,    0x21 } },
    { "PD0R_KP4",   W(pad0.r[4]         ),  { 0,    ~0U  }, { 0,    0x82 } },
    { "PD0R_KP5",   W(pad0.r[5]         ),  { 0,    ~0U  }, { 0,    0x42 } },
    { "PD0R_KP6",   W(pad0.r[6]         ),  { 0,    ~0U  }, { 0,    0x22 } },
    { "PD0R_KP7",   W(pad0.r[7]         ),  { 0,    ~0U  }, { 0,    0x84 } },
    { "PD0R_KP8",   W(pad0.r[8]         ),  { 0,    ~0U  }, { 0,    0x44 } },
    { "PD0R_KP9",   W(pad0.r[9]         ),  { 0,    ~0U  }, { 0,    0x24 } },
    { "PD0R_KPC",   W(pad0.r[10]        ),  { 0,    ~0U  }, { 0,    0x88 } },
    { "PD0R_KP0",   W(pad0.r[0]         ),  { 0,    ~0U  }, { 0,    0x48 } },
    { "PD0R_KPE",   W(pad0.r[11]        ),  { 0,    ~0U  }, { 0,    0x28 } },

    /* -------------------------------------------------------------------- */
    /*  PAD0: Action buttons.                                               */
    /* -------------------------------------------------------------------- */
    { "PD0L_A_T",   W(pad0.l[12]        ), { 0,    ~0U  }, { 0,    0xA0 } },
    { "PD0L_A_L",   W(pad0.l[13]        ), { 0,    ~0U  }, { 0,    0x60 } },
    { "PD0L_A_R",   W(pad0.l[14]        ), { 0,    ~0U  }, { 0,    0xC0 } },

    { "PD0R_A_T",   W(pad0.r[12]        ), { 0,    ~0U  }, { 0,    0xA0 } },
    { "PD0R_A_L",   W(pad0.r[13]        ), { 0,    ~0U  }, { 0,    0x60 } },
    { "PD0R_A_R",   W(pad0.r[14]        ), { 0,    ~0U  }, { 0,    0xC0 } },

    /* -------------------------------------------------------------------- */
    /*  PAD0: The Controller DISC via Keyboard etc.                         */
    /* -------------------------------------------------------------------- */
    { "PD0L_D_E",   W(pad0.l[15]        ), { ~1,   ~0U  }, { 0,    1  } },
    { "PD0L_D_ENE", W(pad0.l[15]        ), { ~3,   ~0U  }, { 0,    3  } },
    { "PD0L_D_NE",  W(pad0.l[15]        ), { ~2,   ~0U  }, { 0,    2  } },
    { "PD0L_D_NNE", W(pad0.l[15]        ), { ~6,   ~0U  }, { 0,    6  } },
    { "PD0L_D_N",   W(pad0.l[15]        ), { ~4,   ~0U  }, { 0,    4  } },
    { "PD0L_D_NNW", W(pad0.l[15]        ), { ~12,  ~0U  }, { 0,    12 } },
    { "PD0L_D_NW",  W(pad0.l[15]        ), { ~8,   ~0U  }, { 0,    8  } },
    { "PD0L_D_WNW", W(pad0.l[15]        ), { ~24,  ~0U  }, { 0,    24 } },
    { "PD0L_D_W",   W(pad0.l[15]        ), { ~16,  ~0U  }, { 0,    16 } },
    { "PD0L_D_WSW", W(pad0.l[15]        ), { ~48,  ~0U  }, { 0,    48 } },
    { "PD0L_D_SW",  W(pad0.l[15]        ), { ~32,  ~0U  }, { 0,    32 } },
    { "PD0L_D_SSW", W(pad0.l[15]        ), { ~96,  ~0U  }, { 0,    96 } },
    { "PD0L_D_S",   W(pad0.l[15]        ), { ~64,  ~0U  }, { 0,    64 } },
    { "PD0L_D_SSE", W(pad0.l[15]        ), { ~192, ~0U  }, { 0,    192} },
    { "PD0L_D_SE",  W(pad0.l[15]        ), { ~128, ~0U  }, { 0,    128} },
    { "PD0L_D_ESE", W(pad0.l[15]        ), { ~129, ~0U  }, { 0,    129} },

    { "PD0R_D_E",   W(pad0.r[15]        ), { ~1,   ~0U  }, { 0,    1  } },
    { "PD0R_D_ENE", W(pad0.r[15]        ), { ~3,   ~0U  }, { 0,    3  } },
    { "PD0R_D_NE",  W(pad0.r[15]        ), { ~2,   ~0U  }, { 0,    2  } },
    { "PD0R_D_NNE", W(pad0.r[15]        ), { ~6,   ~0U  }, { 0,    6  } },
    { "PD0R_D_N",   W(pad0.r[15]        ), { ~4,   ~0U  }, { 0,    4  } },
    { "PD0R_D_NNW", W(pad0.r[15]        ), { ~12,  ~0U  }, { 0,    12 } },
    { "PD0R_D_NW",  W(pad0.r[15]        ), { ~8,   ~0U  }, { 0,    8  } },
    { "PD0R_D_WNW", W(pad0.r[15]        ), { ~24,  ~0U  }, { 0,    24 } },
    { "PD0R_D_W",   W(pad0.r[15]        ), { ~16,  ~0U  }, { 0,    16 } },
    { "PD0R_D_WSW", W(pad0.r[15]        ), { ~48,  ~0U  }, { 0,    48 } },
    { "PD0R_D_SW",  W(pad0.r[15]        ), { ~32,  ~0U  }, { 0,    32 } },
    { "PD0R_D_SSW", W(pad0.r[15]        ), { ~96,  ~0U  }, { 0,    96 } },
    { "PD0R_D_S",   W(pad0.r[15]        ), { ~64,  ~0U  }, { 0,    64 } },
    { "PD0R_D_SSE", W(pad0.r[15]        ), { ~192, ~0U  }, { 0,    192} },
    { "PD0R_D_SE",  W(pad0.r[15]        ), { ~128, ~0U  }, { 0,    128} },
    { "PD0R_D_ESE", W(pad0.r[15]        ), { ~129, ~0U  }, { 0,    129} },

    /* -------------------------------------------------------------------- */
    /*  PAD0: The Controller DISC via Joystick                              */
    /* -------------------------------------------------------------------- */
    { "PD0L_J_E",   W(pad0.l[16]        ), { 0,    0    }, { 0,    1   }},
    { "PD0L_J_ENE", W(pad0.l[16]        ), { 0,    0    }, { 0,    3   }},
    { "PD0L_J_NE",  W(pad0.l[16]        ), { 0,    0    }, { 0,    2   }},
    { "PD0L_J_NNE", W(pad0.l[16]        ), { 0,    0    }, { 0,    6   }},
    { "PD0L_J_N",   W(pad0.l[16]        ), { 0,    0    }, { 0,    4   }},
    { "PD0L_J_NNW", W(pad0.l[16]        ), { 0,    0    }, { 0,    12  }},
    { "PD0L_J_NW",  W(pad0.l[16]        ), { 0,    0    }, { 0,    8   }},
    { "PD0L_J_WNW", W(pad0.l[16]        ), { 0,    0    }, { 0,    24  }},
    { "PD0L_J_W",   W(pad0.l[16]        ), { 0,    0    }, { 0,    16  }},
    { "PD0L_J_WSW", W(pad0.l[16]        ), { 0,    0    }, { 0,    48  }},
    { "PD0L_J_SW",  W(pad0.l[16]        ), { 0,    0    }, { 0,    32  }},
    { "PD0L_J_SSW", W(pad0.l[16]        ), { 0,    0    }, { 0,    96  }},
    { "PD0L_J_S",   W(pad0.l[16]        ), { 0,    0    }, { 0,    64  }},
    { "PD0L_J_SSE", W(pad0.l[16]        ), { 0,    0    }, { 0,    192 }},
    { "PD0L_J_SE",  W(pad0.l[16]        ), { 0,    0    }, { 0,    128 }},
    { "PD0L_J_ESE", W(pad0.l[16]        ), { 0,    0    }, { 0,    129 }},

    { "PD0R_J_E",   W(pad0.r[16]        ), { 0,    0    }, { 0,    1   }},
    { "PD0R_J_ENE", W(pad0.r[16]        ), { 0,    0    }, { 0,    3   }},
    { "PD0R_J_NE",  W(pad0.r[16]        ), { 0,    0    }, { 0,    2   }},
    { "PD0R_J_NNE", W(pad0.r[16]        ), { 0,    0    }, { 0,    6   }},
    { "PD0R_J_N",   W(pad0.r[16]        ), { 0,    0    }, { 0,    4   }},
    { "PD0R_J_NNW", W(pad0.r[16]        ), { 0,    0    }, { 0,    12  }},
    { "PD0R_J_NW",  W(pad0.r[16]        ), { 0,    0    }, { 0,    8   }},
    { "PD0R_J_WNW", W(pad0.r[16]        ), { 0,    0    }, { 0,    24  }},
    { "PD0R_J_W",   W(pad0.r[16]        ), { 0,    0    }, { 0,    16  }},
    { "PD0R_J_WSW", W(pad0.r[16]        ), { 0,    0    }, { 0,    48  }},
    { "PD0R_J_SW",  W(pad0.r[16]        ), { 0,    0    }, { 0,    32  }},
    { "PD0R_J_SSW", W(pad0.r[16]        ), { 0,    0    }, { 0,    96  }},
    { "PD0R_J_S",   W(pad0.r[16]        ), { 0,    0    }, { 0,    64  }},
    { "PD0R_J_SSE", W(pad0.r[16]        ), { 0,    0    }, { 0,    192 }},
    { "PD0R_J_SE",  W(pad0.r[16]        ), { 0,    0    }, { 0,    128 }},
    { "PD0R_J_ESE", W(pad0.r[16]        ), { 0,    0    }, { 0,    129 }},

    /* -------------------------------------------------------------------- */
    /*  PAD0 "Raw" bit-level input bits for left and right.                 */
    /* -------------------------------------------------------------------- */
    { "PD0L_BIT_0", W(pad0.l[17]        ), { ~0x01, ~0U  }, { 0,   0x01 } },
    { "PD0L_BIT_1", W(pad0.l[17]        ), { ~0x02, ~0U  }, { 0,   0x02 } },
    { "PD0L_BIT_2", W(pad0.l[17]        ), { ~0x04, ~0U  }, { 0,   0x04 } },
    { "PD0L_BIT_3", W(pad0.l[17]        ), { ~0x08, ~0U  }, { 0,   0x08 } },
    { "PD0L_BIT_4", W(pad0.l[17]        ), { ~0x10, ~0U  }, { 0,   0x10 } },
    { "PD0L_BIT_5", W(pad0.l[17]        ), { ~0x20, ~0U  }, { 0,   0x20 } },
    { "PD0L_BIT_6", W(pad0.l[17]        ), { ~0x40, ~0U  }, { 0,   0x40 } },
    { "PD0L_BIT_7", W(pad0.l[17]        ), { ~0x80, ~0U  }, { 0,   0x80 } },

    { "PD0R_BIT_0", W(pad0.r[17]        ), { ~0x01, ~0U  }, { 0,   0x01 } },
    { "PD0R_BIT_1", W(pad0.r[17]        ), { ~0x02, ~0U  }, { 0,   0x02 } },
    { "PD0R_BIT_2", W(pad0.r[17]        ), { ~0x04, ~0U  }, { 0,   0x04 } },
    { "PD0R_BIT_3", W(pad0.r[17]        ), { ~0x08, ~0U  }, { 0,   0x08 } },
    { "PD0R_BIT_4", W(pad0.r[17]        ), { ~0x10, ~0U  }, { 0,   0x10 } },
    { "PD0R_BIT_5", W(pad0.r[17]        ), { ~0x20, ~0U  }, { 0,   0x20 } },
    { "PD0R_BIT_6", W(pad0.r[17]        ), { ~0x40, ~0U  }, { 0,   0x40 } },
    { "PD0R_BIT_7", W(pad0.r[17]        ), { ~0x80, ~0U  }, { 0,   0x80 } },

    /* -------------------------------------------------------------------- */
    /*  PAD1: Left-hand controller keypad                                   */
    /* -------------------------------------------------------------------- */
    { "PD1L_KP1",   W(pad1.l[1]         ),  { 0,    ~0U  }, { 0,    0x81 } },
    { "PD1L_KP2",   W(pad1.l[2]         ),  { 0,    ~0U  }, { 0,    0x41 } },
    { "PD1L_KP3",   W(pad1.l[3]         ),  { 0,    ~0U  }, { 0,    0x21 } },
    { "PD1L_KP4",   W(pad1.l[4]         ),  { 0,    ~0U  }, { 0,    0x82 } },
    { "PD1L_KP5",   W(pad1.l[5]         ),  { 0,    ~0U  }, { 0,    0x42 } },
    { "PD1L_KP6",   W(pad1.l[6]         ),  { 0,    ~0U  }, { 0,    0x22 } },
    { "PD1L_KP7",   W(pad1.l[7]         ),  { 0,    ~0U  }, { 0,    0x84 } },
    { "PD1L_KP8",   W(pad1.l[8]         ),  { 0,    ~0U  }, { 0,    0x44 } },
    { "PD1L_KP9",   W(pad1.l[9]         ),  { 0,    ~0U  }, { 0,    0x24 } },
    { "PD1L_KPC",   W(pad1.l[10]        ),  { 0,    ~0U  }, { 0,    0x88 } },
    { "PD1L_KP0",   W(pad1.l[0]         ),  { 0,    ~0U  }, { 0,    0x48 } },
    { "PD1L_KPE",   W(pad1.l[11]        ),  { 0,    ~0U  }, { 0,    0x28 } },

    /* -------------------------------------------------------------------- */
    /*  PAD1: Right-hand controller keypad                                  */
    /* -------------------------------------------------------------------- */
    { "PD1R_KP1",   W(pad1.r[1]         ),  { 0,    ~0U  }, { 0,    0x81 } },
    { "PD1R_KP2",   W(pad1.r[2]         ),  { 0,    ~0U  }, { 0,    0x41 } },
    { "PD1R_KP3",   W(pad1.r[3]         ),  { 0,    ~0U  }, { 0,    0x21 } },
    { "PD1R_KP4",   W(pad1.r[4]         ),  { 0,    ~0U  }, { 0,    0x82 } },
    { "PD1R_KP5",   W(pad1.r[5]         ),  { 0,    ~0U  }, { 0,    0x42 } },
    { "PD1R_KP6",   W(pad1.r[6]         ),  { 0,    ~0U  }, { 0,    0x22 } },
    { "PD1R_KP7",   W(pad1.r[7]         ),  { 0,    ~0U  }, { 0,    0x84 } },
    { "PD1R_KP8",   W(pad1.r[8]         ),  { 0,    ~0U  }, { 0,    0x44 } },
    { "PD1R_KP9",   W(pad1.r[9]         ),  { 0,    ~0U  }, { 0,    0x24 } },
    { "PD1R_KPC",   W(pad1.r[10]        ),  { 0,    ~0U  }, { 0,    0x88 } },
    { "PD1R_KP0",   W(pad1.r[0]         ),  { 0,    ~0U  }, { 0,    0x48 } },
    { "PD1R_KPE",   W(pad1.r[11]        ),  { 0,    ~0U  }, { 0,    0x28 } },

    /* -------------------------------------------------------------------- */
    /*  PAD1: Action buttons.                                               */
    /* -------------------------------------------------------------------- */
    { "PD1L_A_T",   W(pad1.l[12]        ), { 0,    ~0U  }, { 0,    0xA0 } },
    { "PD1L_A_L",   W(pad1.l[13]        ), { 0,    ~0U  }, { 0,    0x60 } },
    { "PD1L_A_R",   W(pad1.l[14]        ), { 0,    ~0U  }, { 0,    0xC0 } },

    { "PD1R_A_T",   W(pad1.r[12]        ), { 0,    ~0U  }, { 0,    0xA0 } },
    { "PD1R_A_L",   W(pad1.r[13]        ), { 0,    ~0U  }, { 0,    0x60 } },
    { "PD1R_A_R",   W(pad1.r[14]        ), { 0,    ~0U  }, { 0,    0xC0 } },

    /* -------------------------------------------------------------------- */
    /*  PAD1: The Controller DISC.                                          */
    /* -------------------------------------------------------------------- */
    { "PD1L_D_E",   W(pad1.l[15]        ), { ~1,   ~0U  }, { 0,    1  } },
    { "PD1L_D_NE",  W(pad1.l[15]        ), { ~2,   ~0U  }, { 0,    2  } },
    { "PD1L_D_N",   W(pad1.l[15]        ), { ~4,   ~0U  }, { 0,    4  } },
    { "PD1L_D_NW",  W(pad1.l[15]        ), { ~8,   ~0U  }, { 0,    8  } },
    { "PD1L_D_W",   W(pad1.l[15]        ), { ~16,  ~0U  }, { 0,    16 } },
    { "PD1L_D_SW",  W(pad1.l[15]        ), { ~32,  ~0U  }, { 0,    32 } },
    { "PD1L_D_S",   W(pad1.l[15]        ), { ~64,  ~0U  }, { 0,    64 } },
    { "PD1L_D_SE",  W(pad1.l[15]        ), { ~128, ~0U  }, { 0,    128} },

    { "PD1R_D_E",   W(pad1.r[15]        ), { ~1,   ~0U  }, { 0,    1  } },
    { "PD1R_D_NE",  W(pad1.r[15]        ), { ~2,   ~0U  }, { 0,    2  } },
    { "PD1R_D_N",   W(pad1.r[15]        ), { ~4,   ~0U  }, { 0,    4  } },
    { "PD1R_D_NW",  W(pad1.r[15]        ), { ~8,   ~0U  }, { 0,    8  } },
    { "PD1R_D_W",   W(pad1.r[15]        ), { ~16,  ~0U  }, { 0,    16 } },
    { "PD1R_D_SW",  W(pad1.r[15]        ), { ~32,  ~0U  }, { 0,    32 } },
    { "PD1R_D_S",   W(pad1.r[15]        ), { ~64,  ~0U  }, { 0,    64 } },
    { "PD1R_D_SE",  W(pad1.r[15]        ), { ~128, ~0U  }, { 0,    128} },

    /* -------------------------------------------------------------------- */
    /*  PAD1: The Controller DISC via Joystick                              */
    /* -------------------------------------------------------------------- */
    { "PD1L_J_E",   W(pad1.l[16]        ), { 0,    0    }, { 0,    1   }},
    { "PD1L_J_ENE", W(pad1.l[16]        ), { 0,    0    }, { 0,    3   }},
    { "PD1L_J_NE",  W(pad1.l[16]        ), { 0,    0    }, { 0,    2   }},
    { "PD1L_J_NNE", W(pad1.l[16]        ), { 0,    0    }, { 0,    6   }},
    { "PD1L_J_N",   W(pad1.l[16]        ), { 0,    0    }, { 0,    4   }},
    { "PD1L_J_NNW", W(pad1.l[16]        ), { 0,    0    }, { 0,    12  }},
    { "PD1L_J_NW",  W(pad1.l[16]        ), { 0,    0    }, { 0,    8   }},
    { "PD1L_J_WNW", W(pad1.l[16]        ), { 0,    0    }, { 0,    24  }},
    { "PD1L_J_W",   W(pad1.l[16]        ), { 0,    0    }, { 0,    16  }},
    { "PD1L_J_WSW", W(pad1.l[16]        ), { 0,    0    }, { 0,    48  }},
    { "PD1L_J_SW",  W(pad1.l[16]        ), { 0,    0    }, { 0,    32  }},
    { "PD1L_J_SSW", W(pad1.l[16]        ), { 0,    0    }, { 0,    96  }},
    { "PD1L_J_S",   W(pad1.l[16]        ), { 0,    0    }, { 0,    64  }},
    { "PD1L_J_SSE", W(pad1.l[16]        ), { 0,    0    }, { 0,    192 }},
    { "PD1L_J_SE",  W(pad1.l[16]        ), { 0,    0    }, { 0,    128 }},
    { "PD1L_J_ESE", W(pad1.l[16]        ), { 0,    0    }, { 0,    129 }},

    { "PD1R_J_E",   W(pad1.r[16]        ), { 0,    0    }, { 0,    1   }},
    { "PD1R_J_ENE", W(pad1.r[16]        ), { 0,    0    }, { 0,    3   }},
    { "PD1R_J_NE",  W(pad1.r[16]        ), { 0,    0    }, { 0,    2   }},
    { "PD1R_J_NNE", W(pad1.r[16]        ), { 0,    0    }, { 0,    6   }},
    { "PD1R_J_N",   W(pad1.r[16]        ), { 0,    0    }, { 0,    4   }},
    { "PD1R_J_NNW", W(pad1.r[16]        ), { 0,    0    }, { 0,    12  }},
    { "PD1R_J_NW",  W(pad1.r[16]        ), { 0,    0    }, { 0,    8   }},
    { "PD1R_J_WNW", W(pad1.r[16]        ), { 0,    0    }, { 0,    24  }},
    { "PD1R_J_W",   W(pad1.r[16]        ), { 0,    0    }, { 0,    16  }},
    { "PD1R_J_WSW", W(pad1.r[16]        ), { 0,    0    }, { 0,    48  }},
    { "PD1R_J_SW",  W(pad1.r[16]        ), { 0,    0    }, { 0,    32  }},
    { "PD1R_J_SSW", W(pad1.r[16]        ), { 0,    0    }, { 0,    96  }},
    { "PD1R_J_S",   W(pad1.r[16]        ), { 0,    0    }, { 0,    64  }},
    { "PD1R_J_SSE", W(pad1.r[16]        ), { 0,    0    }, { 0,    192 }},
    { "PD1R_J_SE",  W(pad1.r[16]        ), { 0,    0    }, { 0,    128 }},
    { "PD1R_J_ESE", W(pad1.r[16]        ), { 0,    0    }, { 0,    129 }},

    /* -------------------------------------------------------------------- */
    /*  PAD1 "Raw" bit-level input bits for left and right.                 */
    /* -------------------------------------------------------------------- */
    { "PD1L_BIT_0", W(pad1.l[17]        ), { ~0x01, ~0U  }, { 0,   0x01 } },
    { "PD1L_BIT_1", W(pad1.l[17]        ), { ~0x02, ~0U  }, { 0,   0x02 } },
    { "PD1L_BIT_2", W(pad1.l[17]        ), { ~0x04, ~0U  }, { 0,   0x04 } },
    { "PD1L_BIT_3", W(pad1.l[17]        ), { ~0x08, ~0U  }, { 0,   0x08 } },
    { "PD1L_BIT_4", W(pad1.l[17]        ), { ~0x10, ~0U  }, { 0,   0x10 } },
    { "PD1L_BIT_5", W(pad1.l[17]        ), { ~0x20, ~0U  }, { 0,   0x20 } },
    { "PD1L_BIT_6", W(pad1.l[17]        ), { ~0x40, ~0U  }, { 0,   0x40 } },
    { "PD1L_BIT_7", W(pad1.l[17]        ), { ~0x80, ~0U  }, { 0,   0x80 } },

    { "PD1R_BIT_0", W(pad1.r[17]        ), { ~0x01, ~0U  }, { 0,   0x01 } },
    { "PD1R_BIT_1", W(pad1.r[17]        ), { ~0x02, ~0U  }, { 0,   0x02 } },
    { "PD1R_BIT_2", W(pad1.r[17]        ), { ~0x04, ~0U  }, { 0,   0x04 } },
    { "PD1R_BIT_3", W(pad1.r[17]        ), { ~0x08, ~0U  }, { 0,   0x08 } },
    { "PD1R_BIT_4", W(pad1.r[17]        ), { ~0x10, ~0U  }, { 0,   0x10 } },
    { "PD1R_BIT_5", W(pad1.r[17]        ), { ~0x20, ~0U  }, { 0,   0x20 } },
    { "PD1R_BIT_6", W(pad1.r[17]        ), { ~0x40, ~0U  }, { 0,   0x40 } },
    { "PD1R_BIT_7", W(pad1.r[17]        ), { ~0x80, ~0U  }, { 0,   0x80 } },

    /*
00FFh|                  00FEh bits
bits |   0       1    2  3  4    5        6      7
-----+------------------------------------------------
  0  | left,   comma, n, v, x, space,   [n/a], [n/a]
  1  | period, m,     b, c, z, down,    [n/a], [n/a]
  2  | scolon, k,     h, f, s, up,      [n/a], [n/a]
  3  | p,      i,     y, r, w, q,       [n/a], [n/a]
  4  | esc,    9,     7, 5, 3, 1,       [n/a], [n/a]
  5  | 0,      8,     6, 4, 2, right,   [n/a], [n/a]
  6  | enter,  o,     u, t, e, ctl,     [n/a], [n/a]
  7  | [n/a],  l,     j, g, d, a,       shift, [n/a]
  */

    /* -------------------------------------------------------------------- */
    /*  ECS Keyboard                                                        */
    /* -------------------------------------------------------------------- */

    /* bit 0 */
    { "KEYB_LEFT",  W(pad1.k[ 0]        ), { ~  1, ~0U  }, { 0,      1} },
    { "KEYB_PERIOD",W(pad1.k[ 0]        ), { ~  2, ~0U  }, { 0,      2} },
    { "KEYB_SEMI",  W(pad1.k[ 0]        ), { ~  4, ~0U  }, { 0,      4} },
    { "KEYB_P",     W(pad1.k[ 0]        ), { ~  8, ~0U  }, { 0,      8} },
    { "KEYB_ESC",   W(pad1.k[ 0]        ), { ~ 16, ~0U  }, { 0,     16} },
    { "KEYB_0",     W(pad1.k[ 0]        ), { ~ 32, ~0U  }, { 0,     32} },
    { "KEYB_ENTER", W(pad1.k[ 0]        ), { ~ 64, ~0U  }, { 0,     64} },

    /* bit 1 */
    { "KEYB_COMMA", W(pad1.k[ 1]        ), { ~  1, ~0U  }, { 0,      1} },
    { "KEYB_M",     W(pad1.k[ 1]        ), { ~  2, ~0U  }, { 0,      2} },
    { "KEYB_K",     W(pad1.k[ 1]        ), { ~  4, ~0U  }, { 0,      4} },
    { "KEYB_I",     W(pad1.k[ 1]        ), { ~  8, ~0U  }, { 0,      8} },
    { "KEYB_9",     W(pad1.k[ 1]        ), { ~ 16, ~0U  }, { 0,     16} },
    { "KEYB_8",     W(pad1.k[ 1]        ), { ~ 32, ~0U  }, { 0,     32} },
    { "KEYB_O",     W(pad1.k[ 1]        ), { ~ 64, ~0U  }, { 0,     64} },
    { "KEYB_L",     W(pad1.k[ 1]        ), { ~128, ~0U  }, { 0,    128} },

    /* bit 2 */
    { "KEYB_N",     W(pad1.k[ 2]        ), { ~  1, ~0U  }, { 0,      1} },
    { "KEYB_B",     W(pad1.k[ 2]        ), { ~  2, ~0U  }, { 0,      2} },
    { "KEYB_H",     W(pad1.k[ 2]        ), { ~  4, ~0U  }, { 0,      4} },
    { "KEYB_Y",     W(pad1.k[ 2]        ), { ~  8, ~0U  }, { 0,      8} },
    { "KEYB_7",     W(pad1.k[ 2]        ), { ~ 16, ~0U  }, { 0,     16} },
    { "KEYB_6",     W(pad1.k[ 2]        ), { ~ 32, ~0U  }, { 0,     32} },
    { "KEYB_U",     W(pad1.k[ 2]        ), { ~ 64, ~0U  }, { 0,     64} },
    { "KEYB_J",     W(pad1.k[ 2]        ), { ~128, ~0U  }, { 0,    128} },

    /* bit 3 */
    { "KEYB_V",     W(pad1.k[ 3]        ), { ~  1, ~0U  }, { 0,      1} },
    { "KEYB_C",     W(pad1.k[ 3]        ), { ~  2, ~0U  }, { 0,      2} },
    { "KEYB_F",     W(pad1.k[ 3]        ), { ~  4, ~0U  }, { 0,      4} },
    { "KEYB_R",     W(pad1.k[ 3]        ), { ~  8, ~0U  }, { 0,      8} },
    { "KEYB_5",     W(pad1.k[ 3]        ), { ~ 16, ~0U  }, { 0,     16} },
    { "KEYB_4",     W(pad1.k[ 3]        ), { ~ 32, ~0U  }, { 0,     32} },
    { "KEYB_T",     W(pad1.k[ 3]        ), { ~ 64, ~0U  }, { 0,     64} },
    { "KEYB_G",     W(pad1.k[ 3]        ), { ~128, ~0U  }, { 0,    128} },

    /* bit 4 */
    { "KEYB_X",     W(pad1.k[ 4]        ), { ~  1, ~0U  }, { 0,      1} },
    { "KEYB_Z",     W(pad1.k[ 4]        ), { ~  2, ~0U  }, { 0,      2} },
    { "KEYB_S",     W(pad1.k[ 4]        ), { ~  4, ~0U  }, { 0,      4} },
    { "KEYB_W",     W(pad1.k[ 4]        ), { ~  8, ~0U  }, { 0,      8} },
    { "KEYB_3",     W(pad1.k[ 4]        ), { ~ 16, ~0U  }, { 0,     16} },
    { "KEYB_2",     W(pad1.k[ 4]        ), { ~ 32, ~0U  }, { 0,     32} },
    { "KEYB_E",     W(pad1.k[ 4]        ), { ~ 64, ~0U  }, { 0,     64} },
    { "KEYB_D",     W(pad1.k[ 4]        ), { ~128, ~0U  }, { 0,    128} },

    /* bit 5 */
    { "KEYB_SPACE", W(pad1.k[ 5]        ), { ~  1, ~0U  }, { 0,      1} },
    { "KEYB_DOWN",  W(pad1.k[ 5]        ), { ~  2, ~0U  }, { 0,      2} },
    { "KEYB_UP",    W(pad1.k[ 5]        ), { ~  4, ~0U  }, { 0,      4} },
    { "KEYB_Q",     W(pad1.k[ 5]        ), { ~  8, ~0U  }, { 0,      8} },
    { "KEYB_1",     W(pad1.k[ 5]        ), { ~ 16, ~0U  }, { 0,     16} },
    { "KEYB_RIGHT", W(pad1.k[ 5]        ), { ~ 32, ~0U  }, { 0,     32} },
    { "KEYB_CTRL",  W(pad1.k[ 5]        ), { ~ 64, ~0U  }, { 0,     64} },
    { "KEYB_A",     W(pad1.k[ 5]        ), { ~128, ~0U  }, { 0,    128} },

    /* bit 6 */
    { "KEYB_SHIFT", W(pad1.k[ 6]        ), { ~128, ~0U  }, { 0,    128} },

    /* -------------------------------------------------------------------- */
    /*  ECS Keyboard "Shifted" Keys.                                        */
    /* -------------------------------------------------------------------- */
    { "KEYB_EQUAL", W(pad1.k[ 5]), { ~( 16 << 8), ~0U  }, { 0, (16 << 8)} },
    { "KEYB_QUOTE", W(pad1.k[ 4]), { ~( 32 << 8), ~0U  }, { 0, (32 << 8)} },
    { "KEYB_HASH",  W(pad1.k[ 4]), { ~( 16 << 8), ~0U  }, { 0, (16 << 8)} },
    { "KEYB_DOLLAR",W(pad1.k[ 3]), { ~( 32 << 8), ~0U  }, { 0, (32 << 8)} },
    { "KEYB_PLUS",  W(pad1.k[ 3]), { ~( 16 << 8), ~0U  }, { 0, (16 << 8)} },
    { "KEYB_MINUS", W(pad1.k[ 2]), { ~( 32 << 8), ~0U  }, { 0, (32 << 8)} },
    { "KEYB_SLASH", W(pad1.k[ 2]), { ~( 16 << 8), ~0U  }, { 0, (16 << 8)} },
    { "KEYB_STAR",  W(pad1.k[ 1]), { ~( 32 << 8), ~0U  }, { 0, (32 << 8)} },
    { "KEYB_LPAREN",W(pad1.k[ 1]), { ~( 16 << 8), ~0U  }, { 0, (16 << 8)} },
    { "KEYB_RPAREN",W(pad1.k[ 0]), { ~( 32 << 8), ~0U  }, { 0, (32 << 8)} },
    { "KEYB_CARET", W(pad1.k[ 5]), { ~(  4 << 8), ~0U  }, { 0, ( 4 << 8)} },
    { "KEYB_QUEST", W(pad1.k[ 5]), { ~(  2 << 8), ~0U  }, { 0, ( 2 << 8)} },
    { "KEYB_PCT",   W(pad1.k[ 0]), { ~(  1 << 8), ~0U  }, { 0, ( 1 << 8)} },
    { "KEYB_SQUOTE",W(pad1.k[ 5]), { ~( 32 << 8), ~0U  }, { 0, (32 << 8)} },
    { "KEYB_COLON", W(pad1.k[ 0]), { ~(  4 << 8), ~0U  }, { 0, ( 4 << 8)} },
    { "KEYB_GREATER",W(pad1.k[0]), { ~(  2 << 8), ~0U  }, { 0, ( 2 << 8)} },
    { "KEYB_LESS",  W(pad1.k[ 1]), { ~(  1 << 8), ~0U  }, { 0, ( 1 << 8)} },

    /* -------------------------------------------------------------------- */
    /*  Synthesizer keyboard bindings.                                      */
    /* -------------------------------------------------------------------- */
    { "SYNTH_00",   W(pad1.k[ 0]), { ~(  1 << 16), ~0U  }, { 0, (  1 << 16)} },
    { "SYNTH_01",   W(pad1.k[ 0]), { ~(  2 << 16), ~0U  }, { 0, (  2 << 16)} },
    { "SYNTH_02",   W(pad1.k[ 0]), { ~(  4 << 16), ~0U  }, { 0, (  4 << 16)} },
    { "SYNTH_03",   W(pad1.k[ 0]), { ~(  8 << 16), ~0U  }, { 0, (  8 << 16)} },
    { "SYNTH_04",   W(pad1.k[ 0]), { ~( 16 << 16), ~0U  }, { 0, ( 16 << 16)} },
    { "SYNTH_05",   W(pad1.k[ 0]), { ~( 32 << 16), ~0U  }, { 0, ( 32 << 16)} },
    { "SYNTH_06",   W(pad1.k[ 0]), { ~( 64 << 16), ~0U  }, { 0, ( 64 << 16)} },
    { "SYNTH_07",   W(pad1.k[ 0]), { ~(128 << 16), ~0U  }, { 0, (128 << 16)} },
    { "SYNTH_08",   W(pad1.k[ 1]), { ~(  1 << 16), ~0U  }, { 0, (  1 << 16)} },
    { "SYNTH_09",   W(pad1.k[ 1]), { ~(  2 << 16), ~0U  }, { 0, (  2 << 16)} },
    { "SYNTH_10",   W(pad1.k[ 1]), { ~(  4 << 16), ~0U  }, { 0, (  4 << 16)} },
    { "SYNTH_11",   W(pad1.k[ 1]), { ~(  8 << 16), ~0U  }, { 0, (  8 << 16)} },
    { "SYNTH_12",   W(pad1.k[ 1]), { ~( 16 << 16), ~0U  }, { 0, ( 16 << 16)} },
    { "SYNTH_13",   W(pad1.k[ 1]), { ~( 32 << 16), ~0U  }, { 0, ( 32 << 16)} },
    { "SYNTH_14",   W(pad1.k[ 1]), { ~( 64 << 16), ~0U  }, { 0, ( 64 << 16)} },
    { "SYNTH_15",   W(pad1.k[ 1]), { ~(128 << 16), ~0U  }, { 0, (128 << 16)} },
    { "SYNTH_16",   W(pad1.k[ 2]), { ~(  1 << 16), ~0U  }, { 0, (  1 << 16)} },
    { "SYNTH_17",   W(pad1.k[ 2]), { ~(  2 << 16), ~0U  }, { 0, (  2 << 16)} },
    { "SYNTH_18",   W(pad1.k[ 2]), { ~(  4 << 16), ~0U  }, { 0, (  4 << 16)} },
    { "SYNTH_19",   W(pad1.k[ 2]), { ~(  8 << 16), ~0U  }, { 0, (  8 << 16)} },
    { "SYNTH_20",   W(pad1.k[ 2]), { ~( 16 << 16), ~0U  }, { 0, ( 16 << 16)} },
    { "SYNTH_21",   W(pad1.k[ 2]), { ~( 32 << 16), ~0U  }, { 0, ( 32 << 16)} },
    { "SYNTH_22",   W(pad1.k[ 2]), { ~( 64 << 16), ~0U  }, { 0, ( 64 << 16)} },
    { "SYNTH_23",   W(pad1.k[ 2]), { ~(128 << 16), ~0U  }, { 0, (128 << 16)} },
    { "SYNTH_24",   W(pad1.k[ 3]), { ~(  1 << 16), ~0U  }, { 0, (  1 << 16)} },
    { "SYNTH_25",   W(pad1.k[ 3]), { ~(  2 << 16), ~0U  }, { 0, (  2 << 16)} },
    { "SYNTH_26",   W(pad1.k[ 3]), { ~(  4 << 16), ~0U  }, { 0, (  4 << 16)} },
    { "SYNTH_27",   W(pad1.k[ 3]), { ~(  8 << 16), ~0U  }, { 0, (  8 << 16)} },
    { "SYNTH_28",   W(pad1.k[ 3]), { ~( 16 << 16), ~0U  }, { 0, ( 16 << 16)} },
    { "SYNTH_29",   W(pad1.k[ 3]), { ~( 32 << 16), ~0U  }, { 0, ( 32 << 16)} },
    { "SYNTH_30",   W(pad1.k[ 3]), { ~( 64 << 16), ~0U  }, { 0, ( 64 << 16)} },
    { "SYNTH_31",   W(pad1.k[ 3]), { ~(128 << 16), ~0U  }, { 0, (128 << 16)} },
    { "SYNTH_32",   W(pad1.k[ 4]), { ~(  1 << 16), ~0U  }, { 0, (  1 << 16)} },
    { "SYNTH_33",   W(pad1.k[ 4]), { ~(  2 << 16), ~0U  }, { 0, (  2 << 16)} },
    { "SYNTH_34",   W(pad1.k[ 4]), { ~(  4 << 16), ~0U  }, { 0, (  4 << 16)} },
    { "SYNTH_35",   W(pad1.k[ 4]), { ~(  8 << 16), ~0U  }, { 0, (  8 << 16)} },
    { "SYNTH_36",   W(pad1.k[ 4]), { ~( 16 << 16), ~0U  }, { 0, ( 16 << 16)} },
    { "SYNTH_37",   W(pad1.k[ 4]), { ~( 32 << 16), ~0U  }, { 0, ( 32 << 16)} },
    { "SYNTH_38",   W(pad1.k[ 4]), { ~( 64 << 16), ~0U  }, { 0, ( 64 << 16)} },
    { "SYNTH_39",   W(pad1.k[ 4]), { ~(128 << 16), ~0U  }, { 0, (128 << 16)} },
    { "SYNTH_40",   W(pad1.k[ 5]), { ~(  1 << 16), ~0U  }, { 0, (  1 << 16)} },
    { "SYNTH_41",   W(pad1.k[ 5]), { ~(  2 << 16), ~0U  }, { 0, (  2 << 16)} },
    { "SYNTH_42",   W(pad1.k[ 5]), { ~(  4 << 16), ~0U  }, { 0, (  4 << 16)} },
    { "SYNTH_43",   W(pad1.k[ 5]), { ~(  8 << 16), ~0U  }, { 0, (  8 << 16)} },
    { "SYNTH_44",   W(pad1.k[ 5]), { ~( 16 << 16), ~0U  }, { 0, ( 16 << 16)} },
    { "SYNTH_45",   W(pad1.k[ 5]), { ~( 32 << 16), ~0U  }, { 0, ( 32 << 16)} },
    { "SYNTH_46",   W(pad1.k[ 5]), { ~( 64 << 16), ~0U  }, { 0, ( 64 << 16)} },
    { "SYNTH_47",   W(pad1.k[ 5]), { ~(128 << 16), ~0U  }, { 0, (128 << 16)} },
    { "SYNTH_48",   W(pad1.k[ 6]), { ~(  1 << 16), ~0U  }, { 0, (  1 << 16)} },

     /* Press 1+9 on controller 0 on the master component */
    { "IPAUSE0",    W(pad0.l[1]         ),  { 0,    ~0U  }, { 0,    0xA5 } },

    /* Press 1+9 on controller 1 on the master component */
    { "IPAUSE1",    W(pad0.r[1]         ),  { 0,    ~0U  }, { 0,    0xA5 } },

};

int cfg_event_action_cnt = (int)(sizeof(cfg_event_action)/sizeof(cfg_evtact_t));

/* ------------------------------------------------------------------------ */
/*  Default key bindings table.                                             */
/*                                                                          */
/*  I really need to make sure there are rows in here for all possible      */
/*  key inputs, so that when I process a config file, I can just update     */
/*  this table directly.  Otherwise, I need to duplicate this table in      */
/*  order to change it.                                                     */
/*                                                                          */
/*  Column 1 is the default "primarily 1 player" setup.                     */
/*  Column 2 is the alternate "primarily 2 player" setup.                   */
/*  Column 3 is the ECS Keyboard setup.                                     */
/*  Column 4 is the Meta/Win/Cmd-key modified setup.                        */
/* ------------------------------------------------------------------------ */

cfg_kbd_t  cfg_key_bind[] =
{
/* ------------------------------------------------------------------------ */
/*  Miscellaneous.                                                          */
/* ------------------------------------------------------------------------ */
{ "QUIT",   {   "QUIT",         "QUIT",         "QUIT",         "QUIT"      }},
{ "F1",     {   "QUIT",         "QUIT",         "QUIT",         "QUIT"      }},
{ "ESCAPE", {   "NA",           "NA",           "KEYB_ESC",     "NA"        }},
#ifdef PLAT_MACOS
{ "F3",     {   "WTOG",         "WTOG",         "WTOG",         "WTOG"      }},
{ "LCMD",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
{ "RCMD",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
{ "LGUI",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
{ "RGUI",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
#else
{ "F9",     {   "WTOG",         "WTOG",         "WTOG",         "WTOG"      }},
#endif
#ifdef WIN32
{ "LWIN",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
{ "RWIN",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
{ "LGUI",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
{ "RGUI",   {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},
#endif
{ "F10",    {   "MOVIE",        "MOVIE",        "MOVIE",        "MOVIE"     }},
{ "F11",    {   "SHOT",         "SHOT",         "SHOT",         "SHOT"      }},
{ "F12",    {   "RESET",        "RESET",        "RESET",        "RESET"     }},
{ "HIDE",   {   "HIDE",         "HIDE",         "HIDE",         "HIDE"      }},
//{ "F2",     {   "HIDE",         "HIDE",         "HIDE",         "HIDE"      }},
//{ "F2",     {   "DUMP",         "DUMP",         "DUMP",         "DUMP"      }},
//{ "F3",     {   "LOAD",         "LOAD",         "LOAD",         "LOAD"      }},
{ "F4",     {   "BREAK",        "BREAK",        "BREAK",        "BREAK"     }},
{ "F5",     {   "KBD0",         "KBD0",         "KBD0",         "KBD0"      }},
{ "F6",     {   "KBD1",         "KBD1",         "KBD1",         "KBD1"      }},
{ "F7",     {   "KBD2",         "KBD2",         "KBD2",         "KBD2"      }},
/*{ "F8",   {   "KBD3",         "KBD3",         "KBD3",         "KBD3"      }},*/
{ "PAUSE",  {   "PAUSE",        "PAUSE",        "PAUSE",        "PAUSE"     }},
{ "PAGEUP", {   "VOLUP",        "VOLUP",        "VOLUP",        "VOLUP"     }},
{ "PAGEDOWN",{  "VOLDN",        "VOLDN",        "VOLDN",        "VOLDN"     }},
{ "F8",     {   "PSH3",         "PSH3",         "PSH3",         "POP_UP"    }},

{ "FOCUS_LOST", /* Force windowed mode if we lose focus. */
            {   "WINDOW",       "WINDOW",       "WINDOW",       "WINDOW"    }},

/* ------------------------------------------------------------------------ */
/*  The numeric keypad.                                                     */
/* ------------------------------------------------------------------------ */
{ "KP_7",   {   "PD0L_KP1",     "PD0L_KP1",     "KEYB_1",       "NA"        }},
{ "KP_8",   {   "PD0L_KP2",     "PD0L_KP2",     "KEYB_2",       "NA"        }},
{ "KP_9",   {   "PD0L_KP3",     "PD0L_KP3",     "KEYB_3",       "NA"        }},
{ "KP_4",   {   "PD0L_KP4",     "PD0L_KP4",     "KEYB_4",       "NA"        }},
{ "KP_5",   {   "PD0L_KP5",     "PD0L_KP5",     "KEYB_5",       "NA"        }},
{ "KP_6",   {   "PD0L_KP6",     "PD0L_KP6",     "KEYB_6",       "NA"        }},
{ "KP_1",   {   "PD0L_KP7",     "PD0L_KP7",     "KEYB_7",       "NA"        }},
{ "KP_2",   {   "PD0L_KP8",     "PD0L_KP8",     "KEYB_8",       "NA"        }},
{ "KP_3",   {   "PD0L_KP9",     "PD0L_KP9",     "KEYB_9",       "NA"        }},
{ "KP_0",   {   "PD0L_KPC",     "PD0L_KPC",     "KEYB_0",       "NA"        }},
{ "KP_PERIOD",{ "PD0L_KP0",     "PD0L_KP0",     "KEYB_PERIOD",  "NA"        }},
{ "KP_ENTER", { "PD0L_KPE",     "PD0L_KPE",     "KEYB_ENTER",   "NA"        }},

/* ------------------------------------------------------------------------ */
/*  The number keys.                                                        */
/* ------------------------------------------------------------------------ */
{ "1",      {   "PD0R_KP1",     "PD0L_KP1",     "KEYB_1",       "NA"        }},
{ "2",      {   "PD0R_KP2",     "PD0L_KP2",     "KEYB_2",       "NA"        }},
{ "3",      {   "PD0R_KP3",     "PD0L_KP3",     "KEYB_3",       "NA"        }},
{ "4",      {   "PD0R_KP4",     "PD0L_KP4",     "KEYB_4",       "NA"        }},
{ "5",      {   "PD0R_KP5",     "PD0L_KP5",     "KEYB_5",       "NA"        }},
{ "6",      {   "PD0R_KP6",     "PD0L_KP6",     "KEYB_6",       "NA"        }},
{ "7",      {   "PD0R_KP7",     "PD0L_KP7",     "KEYB_7",       "NA"        }},
{ "8",      {   "PD0R_KP8",     "PD0L_KP8",     "KEYB_8",       "NA"        }},
{ "9",      {   "PD0R_KP9",     "PD0L_KP9",     "KEYB_9",       "NA"        }},
{ "-",      {   "PD0R_KPC",     "PD0L_KPC",     "KEYB_MINUS",   "NA"        }},
{ "0",      {   "PD0R_KP0",     "PD0L_KP0",     "KEYB_0",       "NA"        }},
{ "=",      {   "PD0R_KPE",     "PD0L_KPE",     "KEYB_EQUAL",   "NA"        }},

/* ------------------------------------------------------------------------ */
/*  Action buttons.                                                         */
/* ------------------------------------------------------------------------ */
{ "RSHIFT", {   "PD0L_A_T",     "PD0L_A_T",     "KEYB_SHIFT",   "NA"        }},
{ "RALT",   {   "PD0L_A_L",     "PD0L_A_L",     "NA",           "NA"        }},
{ "RCTRL",  {   "PD0L_A_R",     "PD0L_A_R",     "KEYB_CTRL",    "NA"        }},

{ "LSHIFT", {   "PD0R_A_T",     "PD0L_A_T",     "KEYB_SHIFT",   "NA"        }},
{ "LALT",   {   "PD0R_A_L",     "PD0L_A_L",     "NA",           "NA"        }},
{ "LCTRL",  {   "PD0R_A_R",     "PD0L_A_R",     "KEYB_CTRL",    "NA"        }},

/* ------------------------------------------------------------------------ */
/*  Movement keys.                                                          */
/* ------------------------------------------------------------------------ */
{ "RIGHT",  {   "PD0L_D_E",     "PD0L_D_E",     "KEYB_RIGHT",   "NA"        }},
{ "UP",     {   "PD0L_D_N",     "PD0L_D_N",     "KEYB_UP",      "NA"        }},
{ "LEFT",   {   "PD0L_D_W",     "PD0L_D_W",     "KEYB_LEFT",    "NA"        }},
{ "DOWN",   {   "PD0L_D_S",     "PD0L_D_S",     "KEYB_DOWN",    "NA"        }},

{ "K",      {   "PD0L_D_E",     "PD0L_D_E",     "KEYB_K",       "NA"        }},
{ "O",      {   "PD0L_D_NE",    "PD0L_D_NE",    "KEYB_O",       "NA"        }},
{ "I",      {   "PD0L_D_N",     "PD0L_D_N",     "KEYB_I",       "NA"        }},
{ "U",      {   "PD0L_D_NW",    "PD0L_D_NW",    "KEYB_U",       "NA"        }},
{ "J",      {   "PD0L_D_W",     "PD0L_D_W",     "KEYB_J",       "NA"        }},
{ "N",      {   "PD0L_D_SW",    "PD0L_D_SW",    "KEYB_N",       "NA"        }},
{ "M",      {   "PD0L_D_S",     "PD0L_D_S",     "KEYB_M",       "MOVIE"     }},
{ ",",      {   "PD0L_D_SE",    "PD0L_D_SE",    "KEYB_COMMA",   "NA"        }},

{ "D",      {   "PD0R_D_E",     "PD0L_D_E",     "KEYB_D",       "NA"        }},
{ "R",      {   "PD0R_D_NE",    "PD0L_D_NE",    "KEYB_R",       "RESET"     }},
{ "E",      {   "PD0R_D_N",     "PD0L_D_N",     "KEYB_E",       "NA"        }},
{ "W",      {   "PD0R_D_NW",    "PD0L_D_NW",    "KEYB_W",       "WTOG"      }},
{ "S",      {   "PD0R_D_W",     "PD0L_D_W",     "KEYB_S",       "SHOT"      }},
{ "Z",      {   "PD0R_D_SW",    "PD0L_D_SW",    "KEYB_Z",       "RELOAD"    }},
{ "X",      {   "PD0R_D_S",     "PD0L_D_S",     "KEYB_X",       "NA"        }},
{ "C",      {   "PD0R_D_SE",    "PD0L_D_SE",    "KEYB_C",       "BREAK"     }},

    /*
00FFh|                  00FEh bits
bits |   0       1    2  3  4    5        6      7
-----+------------------------------------------------
  0  | left,   comma, n, v, x, space,   [n/a], [n/a]
  1  | period, m,     b, c, z, down,    [n/a], [n/a]
  2  | scolon, k,     h, f, s, up,      [n/a], [n/a]
  3  | p,      i,     y, r, w, q,       [n/a], [n/a]
  4  | esc,    9,     7, 5, 3, 1,       [n/a], [n/a]
  5  | 0,      8,     6, 4, 2, right,   [n/a], [n/a]
  6  | enter,  o,     u, t, e, ctl,     [n/a], [n/a]
  7  | [n/a],  l,     j, g, d, a,       shift, [n/a]
  */

/* ------------------------------------------------------------------------ */
/*  ECS Keyboard remaining keys.                                            */
/* ------------------------------------------------------------------------ */
{ "Q",      {   "NA",           "NA",           "KEYB_Q",       "QUIT"      }},
{ "T",      {   "NA",           "NA",           "KEYB_T",       "NA"        }},
{ "Y",      {   "NA",           "NA",           "KEYB_Y",       "NA"        }},
{ "P",      {   "NA",           "NA",           "KEYB_P",       "PAUSE"     }},

{ "A",      {   "NA",           "NA",           "KEYB_A",       "AVI"       }},
{ "F",      {   "NA",           "NA",           "KEYB_F",       "WTOG"      }},
{ "G",      {   "NA",           "NA",           "KEYB_G",       "GRAMSHOT"  }},
{ "H",      {   "NA",           "NA",           "KEYB_H",       "NA"        }},
{ "L",      {   "NA",           "NA",           "KEYB_L",       "NA"        }},

{ "V",      {   "NA",           "NA",           "KEYB_V",       "AVI"       }},
#ifdef N900
{ "B",      {   "QUIT",         "QUIT",         "QUIT",         "QUIT"      }},
#else
{ "B",      {   "NA",           "NA",           "KEYB_B",       "NA"        }},
#endif
{ ".",      {   "NA",           "NA",           "KEYB_PERIOD",  "NA"        }},
{ ";",      {   "NA",           "NA",           "KEYB_SEMI",    "NA"        }},

{ "SPACE",  {   "NA",           "NA",           "KEYB_SPACE",   "NA"        }},
{ "RETURN", {   "NA",           "NA",           "KEYB_ENTER",   "NA"        }},
{"BACKSPACE",{  "NA",           "NA",           "KEYB_LEFT",    "NA"        }},

{ "QUOTEDBL",{  "NA",           "NA",           "KEYB_QUOTE",   "NA"        }},
{ "QUOTE",  {   "NA",           "NA",           "KEYB_QUOTE",   "NA"        }},
{ "CARET",  {   "NA",           "NA",           "KEYB_CARET",   "NA"        }},
{ "HASH",   {   "NA",           "NA",           "KEYB_HASH",    "NA"        }},
{ "PLUS",   {   "NA",           "NA",           "KEYB_PLUS",    "NA"        }},
{ "SLASH",  {   "NA",           "NA",           "KEYB_SLASH",   "NA"        }},
{ "DOLLAR", {   "NA",           "NA",           "KEYB_DOLLAR",  "NA"        }},
{ "ASTERISK",{  "NA",           "NA",           "KEYB_STAR",    "NA"        }},
{ "LEFTPAREN",{ "NA",           "NA",           "KEYB_LPAREN",  "NA"        }},
{ "RIGHTPAREN",{"NA",           "NA",           "KEYB_RPAREN",  "NA"        }},
{ "QUESTION",{  "NA",           "NA",           "KEYB_QUEST",   "NA"        }},
{ "COLON",  {   "NA",           "NA",           "KEYB_COLON",   "NA"        }},
{ "GREATER",{   "NA",           "NA",           "KEYB_GREATER", "NA"        }},
{ "LESS",   {   "NA",           "NA",           "KEYB_LESS",    "NA"        }},

/* ------------------------------------------------------------------------ */
/*  Default Joystick 0 mapping.                                             */
/* ------------------------------------------------------------------------ */
{ "JS0_E",    { "PD0L_J_E",     "PD0L_J_E",     "PD0L_J_E",     "PD0L_J_E"  }},
{ "JS0_ENE",  { "PD0L_J_ENE",   "PD0L_J_ENE",   "PD0L_J_ENE",   "PD0L_J_ENE"}},
{ "JS0_NE",   { "PD0L_J_NE",    "PD0L_J_NE",    "PD0L_J_NE",    "PD0L_J_NE" }},
{ "JS0_NNE",  { "PD0L_J_NNE",   "PD0L_J_NNE",   "PD0L_J_NNE",   "PD0L_J_NNE"}},
{ "JS0_N",    { "PD0L_J_N",     "PD0L_J_N",     "PD0L_J_N",     "PD0L_J_N"  }},
{ "JS0_NNW",  { "PD0L_J_NNW",   "PD0L_J_NNW",   "PD0L_J_NNW",   "PD0L_J_NNW"}},
{ "JS0_NW",   { "PD0L_J_NW",    "PD0L_J_NW",    "PD0L_J_NW",    "PD0L_J_NW" }},
{ "JS0_WNW",  { "PD0L_J_WNW",   "PD0L_J_WNW",   "PD0L_J_WNW",   "PD0L_J_WNW"}},
{ "JS0_W",    { "PD0L_J_W",     "PD0L_J_W",     "PD0L_J_W",     "PD0L_J_W"  }},
{ "JS0_WSW",  { "PD0L_J_WSW",   "PD0L_J_WSW",   "PD0L_J_WSW",   "PD0L_J_WSW"}},
{ "JS0_SW",   { "PD0L_J_SW",    "PD0L_J_SW",    "PD0L_J_SW",    "PD0L_J_SW" }},
{ "JS0_SSW",  { "PD0L_J_SSW",   "PD0L_J_SSW",   "PD0L_J_SSW",   "PD0L_J_SSW"}},
{ "JS0_S",    { "PD0L_J_S",     "PD0L_J_S",     "PD0L_J_S",     "PD0L_J_S"  }},
{ "JS0_SSE",  { "PD0L_J_SSE",   "PD0L_J_SSE",   "PD0L_J_SSE",   "PD0L_J_SSE"}},
{ "JS0_SE",   { "PD0L_J_SE",    "PD0L_J_SE",    "PD0L_J_SE",    "PD0L_J_SE" }},
{ "JS0_ESE",  { "PD0L_J_ESE",   "PD0L_J_ESE",   "PD0L_J_ESE",   "PD0L_J_ESE"}},
{ "JS1_E",    { "PD0R_J_E",     "PD0R_J_E",     "PD0R_J_E",     "PD0R_J_E"  }},
{ "JS1_ENE",  { "PD0R_J_ENE",   "PD0R_J_ENE",   "PD0R_J_ENE",   "PD0R_J_ENE"}},
{ "JS1_NE",   { "PD0R_J_NE",    "PD0R_J_NE",    "PD0R_J_NE",    "PD0R_J_NE" }},
{ "JS1_NNE",  { "PD0R_J_NNE",   "PD0R_J_NNE",   "PD0R_J_NNE",   "PD0R_J_NNE"}},
{ "JS1_N",    { "PD0R_J_N",     "PD0R_J_N",     "PD0R_J_N",     "PD0R_J_N"  }},
{ "JS1_NNW",  { "PD0R_J_NNW",   "PD0R_J_NNW",   "PD0R_J_NNW",   "PD0R_J_NNW"}},
{ "JS1_NW",   { "PD0R_J_NW",    "PD0R_J_NW",    "PD0R_J_NW",    "PD0R_J_NW" }},
{ "JS1_WNW",  { "PD0R_J_WNW",   "PD0R_J_WNW",   "PD0R_J_WNW",   "PD0R_J_WNW"}},
{ "JS1_W",    { "PD0R_J_W",     "PD0R_J_W",     "PD0R_J_W",     "PD0R_J_W"  }},
{ "JS1_WSW",  { "PD0R_J_WSW",   "PD0R_J_WSW",   "PD0R_J_WSW",   "PD0R_J_WSW"}},
{ "JS1_SW",   { "PD0R_J_SW",    "PD0R_J_SW",    "PD0R_J_SW",    "PD0R_J_SW" }},
{ "JS1_SSW",  { "PD0R_J_SSW",   "PD0R_J_SSW",   "PD0R_J_SSW",   "PD0R_J_SSW"}},
{ "JS1_S",    { "PD0R_J_S",     "PD0R_J_S",     "PD0R_J_S",     "PD0R_J_S"  }},
{ "JS1_SSE",  { "PD0R_J_SSE",   "PD0R_J_SSE",   "PD0R_J_SSE",   "PD0R_J_SSE"}},
{ "JS1_SE",   { "PD0R_J_SE",    "PD0R_J_SE",    "PD0R_J_SE",    "PD0R_J_SE" }},
{ "JS1_ESE",  { "PD0R_J_ESE",   "PD0R_J_ESE",   "PD0R_J_ESE",   "PD0R_J_ESE"}},

#ifndef GP2X
{"JS0_BTN_00",{ "PD0L_A_T",     "PD0L_A_T",     "PD0L_A_T",     "PD0L_A_T"  }},
{"JS0_BTN_01",{ "PD0L_A_L",     "PD0L_A_L",     "PD0L_A_L",     "PD0L_A_L"  }},
{"JS0_BTN_02",{ "PD0L_A_R",     "PD0L_A_R",     "PD0L_A_R",     "PD0L_A_R"  }},
{"JS0_BTN_03",{ "PD0L_A_T",     "PD0L_A_T",     "PD0L_A_T",     "PD0L_A_T"  }},
{"JS0_BTN_04",{ "PD0L_A_L",     "PD0L_A_L",     "PD0L_A_L",     "PD0L_A_L"  }},
{"JS0_BTN_05",{ "PD0L_A_R",     "PD0L_A_R",     "PD0L_A_R",     "PD0L_A_R"  }},
{"JS0_BTN_06",{ "PD0L_A_T",     "PD0L_A_T",     "PD0L_A_T",     "PD0L_A_T"  }},
{"JS0_BTN_07",{ "PD0L_A_L",     "PD0L_A_L",     "PD0L_A_L",     "PD0L_A_L"  }},
{"JS0_BTN_08",{ "PD0L_A_R",     "PD0L_A_R",     "PD0L_A_R",     "PD0L_A_R"  }},

#else /* GP2X specific mappings. */

/* Directional controller */
{"JS0_BTN_00",{ "PD0L_D_N" ,    "PD0L_D_N" ,    "PD0L_D_N" ,    "PD0L_D_N"  }},
{"JS0_BTN_01",{ "PD0L_D_NW",    "PD0L_D_NW",    "PD0L_D_NW",    "PD0L_D_NW" }},
{"JS0_BTN_02",{ "PD0L_D_W" ,    "PD0L_D_W" ,    "PD0L_D_W" ,    "PD0L_D_W"  }},
{"JS0_BTN_03",{ "PD0L_D_SW",    "PD0L_D_SW",    "PD0L_D_SW",    "PD0L_D_SW" }},
{"JS0_BTN_04",{ "PD0L_D_S" ,    "PD0L_D_S" ,    "PD0L_D_S" ,    "PD0L_D_S"  }},
{"JS0_BTN_05",{ "PD0L_D_SE",    "PD0L_D_SE",    "PD0L_D_SE",    "PD0L_D_SE" }},
{"JS0_BTN_06",{ "PD0L_D_E" ,    "PD0L_D_E" ,    "PD0L_D_E" ,    "PD0L_D_E"  }},
{"JS0_BTN_07",{ "PD0L_D_NE",    "PD0L_D_NE",    "PD0L_D_NE",    "PD0L_D_NE" }},

/* Others:  Action, Volume, Quit, etc. */
/* GP2X button mapping:
    JS0_BTN_08  # start button
    JS0_BTN_09  # select button
    JS0_BTN_10  # button L
    JS0_BTN_11  # button R
    JS0_BTN_12  # button A
    JS0_BTN_13  # button B
    JS0_BTN_14  # button Y
    JS0_BTN_15  # button X
    JS0_BTN_16  # volume up
    JS0_BTN_17  # volume down
    JS0_BTN_18  # stick press, used for shifting to map 1
*/

{"JS0_BTN_08",{ "PD0R_KPE",   "QUIT"    ,   "PD0R_KPE",   "QUIT"       }},
{"JS0_BTN_09",{ "PD0R_KPC",   "RESET"   ,   "PD0R_KPC",   "RESET"      }},
{"JS0_BTN_10",{ "PD0L_D_W",   "PD0R_KP1",   "PD0L_D_W",   "PD0R_KP1"   }},
{"JS0_BTN_11",{ "PD0L_D_E",   "PD0R_KP2",   "PD0L_D_E",   "PD0R_KP2"   }},
{"JS0_BTN_12",{ "PD0L_A_T",   "PD0R_KP4",   "PD0L_A_T",   "PD0R_KP4"   }},
{"JS0_BTN_13",{ "PD0L_A_R",   "PD0R_KP5",   "PD0L_A_R",   "PD0R_KP5"   }},
{"JS0_BTN_14",{ "PD0L_A_T",   "PD0R_KP3",   "PD0L_A_T",   "PD0R_KP3"   }},
{"JS0_BTN_15",{ "PD0L_A_L",   "PD0R_KP6",   "PD0L_A_L",   "PD0R_KP6"   }},
{"JS0_BTN_16",{ "VOLUP"   ,   "PD0R_KP8",   "VOLUP"   ,   "PD0R_KP8"   }},
{"JS0_BTN_17",{ "VOLDN"   ,   "PD0R_KP7",   "VOLDN"   ,   "PD0R_KP7"   }},
{"JS0_BTN_18",{ "SHF10"   ,   "SHF10"   ,   "SHF10"   ,   "SHF10"      }},
#endif

#ifndef WII
{"JS0_HAT0_E", {"PD0R_KP6",     "PD0R_KP6",     "PD0R_KP6",     "PD0R_KP6"  }},
{"JS0_HAT0_NE",{"PD0R_KP3",     "PD0R_KP3",     "PD0R_KP3",     "PD0R_KP3"  }},
{"JS0_HAT0_N", {"PD0R_KP2",     "PD0R_KP2",     "PD0R_KP2",     "PD0R_KP2"  }},
{"JS0_HAT0_NW",{"PD0R_KP1",     "PD0R_KP1",     "PD0R_KP1",     "PD0R_KP1"  }},
{"JS0_HAT0_W", {"PD0R_KP4",     "PD0R_KP4",     "PD0R_KP4",     "PD0R_KP4"  }},
{"JS0_HAT0_SW",{"PD0R_KP7",     "PD0R_KP7",     "PD0R_KP7",     "PD0R_KP7"  }},
{"JS0_HAT0_S", {"PD0R_KP8",     "PD0R_KP8",     "PD0R_KP8",     "PD0R_KP8"  }},
{"JS0_HAT0_SE",{"PD0R_KP9",     "PD0R_KP9",     "PD0R_KP9",     "PD0R_KP9"  }},
#else /* WII Specific Bindings */

{"JS0_BTN_09", {"PD0L_KPE",     "QUIT"    ,     "PD0R_KPE",     "QUIT"      }},
{"JS0_BTN_10", {"PD0L_KPC",     "RESET"   ,     "PD0R_KPC",     "RESET"     }},
{"JS0_BTN_11", {"PD0L_KP5",     "PD0R_KP1",     "PD0L_D_W",     "PD0R_KP1"  }},
{"JS0_BTN_12", {"PD0L_KP0",     "PD0R_KP2",     "PD0L_D_E",     "PD0R_KP2"  }},
{"JS0_HAT0_E", {"PD0L_KP6",     "PD0R_KP6",     "PD0R_KP6",     "PD0R_KP6"  }},
{"JS0_HAT0_NE",{"PD0L_KP3",     "PD0R_KP3",     "PD0R_KP3",     "PD0R_KP3"  }},
{"JS0_HAT0_N", {"PD0L_KP2",     "PD0R_KP2",     "PD0R_KP2",     "PD0R_KP2"  }},
{"JS0_HAT0_NW",{"PD0L_KP1",     "PD0R_KP1",     "PD0R_KP1",     "PD0R_KP1"  }},
{"JS0_HAT0_W", {"PD0L_KP4",     "PD0R_KP4",     "PD0R_KP4",     "PD0R_KP4"  }},
{"JS0_HAT0_SW",{"PD0L_KP7",     "PD0R_KP7",     "PD0R_KP7",     "PD0R_KP7"  }},
{"JS0_HAT0_S", {"PD0L_KP8",     "PD0R_KP8",     "PD0R_KP8",     "PD0R_KP8"  }},
{"JS0_HAT0_SE",{"PD0L_KP9",     "PD0R_KP9",     "PD0R_KP9",     "PD0R_KP9"  }},
{"JS1_BTN_00", {"PD0R_A_T",     "PD0L_A_T",     "PD0L_A_T",     "PD0L_A_T"  }},
{"JS1_BTN_01", {"PD0R_A_L",     "PD0L_A_L",     "PD0L_A_L",     "PD0L_A_L"  }},
{"JS1_BTN_02", {"PD0R_A_R",     "PD0L_A_R",     "PD0L_A_R",     "PD0L_A_R"  }},
{"JS1_BTN_09", {"PD0R_KPE",     "QUIT"    ,     "PD0R_KPE",     "QUIT"      }},
{"JS1_BTN_10", {"PD0R_KPC",     "RESET"   ,     "PD0R_KPC",     "RESET"     }},
{"JS1_BTN_11", {"PD0R_KP5",     "PD0R_KP1",     "PD0L_D_W",     "PD0R_KP1"  }},
{"JS1_BTN_12", {"PD0R_KP0",     "PD0R_KP2",     "PD0L_D_E",     "PD0R_KP2"  }},
{"JS1_E",      {"PD0R_J_E",     "PD0L_J_E",     "PD0L_J_E",     "PD0L_J_E"  }},
{"JS1_ENE",    {"PD0R_J_ENE",   "PD0L_J_ENE",   "PD0L_J_ENE",   "PD0L_J_ENE"}},
{"JS1_NE",     {"PD0R_J_NE",    "PD0L_J_NE",    "PD0L_J_NE",    "PD0L_J_NE" }},
{"JS1_NNE",    {"PD0R_J_NNE",   "PD0L_J_NNE",   "PD0L_J_NNE",   "PD0L_J_NNE"}},
{"JS1_N",      {"PD0R_J_N",     "PD0L_J_N",     "PD0L_J_N",     "PD0L_J_N"  }},
{"JS1_NNW",    {"PD0R_J_NNW",   "PD0L_J_NNW",   "PD0L_J_NNW",   "PD0L_J_NNW"}},
{"JS1_NW",     {"PD0R_J_NW",    "PD0L_J_NW",    "PD0L_J_NW",    "PD0L_J_NW" }},
{"JS1_WNW",    {"PD0R_J_WNW",   "PD0L_J_WNW",   "PD0L_J_WNW",   "PD0L_J_WNW"}},
{"JS1_W",      {"PD0R_J_W",     "PD0L_J_W",     "PD0L_J_W",     "PD0L_J_W"  }},
{"JS1_WSW",    {"PD0R_J_WSW",   "PD0L_J_WSW",   "PD0L_J_WSW",   "PD0L_J_WSW"}},
{"JS1_SW",     {"PD0R_J_SW",    "PD0L_J_SW",    "PD0L_J_SW",    "PD0L_J_SW" }},
{"JS1_SSW",    {"PD0R_J_SSW",   "PD0L_J_SSW",   "PD0L_J_SSW",   "PD0L_J_SSW"}},
{"JS1_S",      {"PD0R_J_S",     "PD0L_J_S",     "PD0L_J_S",     "PD0L_J_S"  }},
{"JS1_SSE",    {"PD0R_J_SSE",   "PD0L_J_SSE",   "PD0L_J_SSE",   "PD0L_J_SSE"}},
{"JS1_SE",     {"PD0R_J_SE",    "PD0L_J_SE",    "PD0L_J_SE",    "PD0L_J_SE" }},
{"JS1_ESE",    {"PD0R_J_ESE",   "PD0L_J_ESE",   "PD0L_J_ESE",   "PD0L_J_ESE"}},
{"JS1_HAT0_E", {"PD0R_KP6",     "PD0R_KP6",     "PD0R_KP6",     "PD0R_KP6"  }},
{"JS1_HAT0_NE",{"PD0R_KP3",     "PD0R_KP3",     "PD0R_KP3",     "PD0R_KP3"  }},
{"JS1_HAT0_N", {"PD0R_KP2",     "PD0R_KP2",     "PD0R_KP2",     "PD0R_KP2"  }},
{"JS1_HAT0_NW",{"PD0R_KP1",     "PD0R_KP1",     "PD0R_KP1",     "PD0R_KP1"  }},
{"JS1_HAT0_W", {"PD0R_KP4",     "PD0R_KP4",     "PD0R_KP4",     "PD0R_KP4"  }},
{"JS1_HAT0_SW",{"PD0R_KP7",     "PD0R_KP7",     "PD0R_KP7",     "PD0R_KP7"  }},
{"JS1_HAT0_S", {"PD0R_KP8",     "PD0R_KP8",     "PD0R_KP8",     "PD0R_KP8"  }},
{"JS1_HAT0_SE",{"PD0R_KP9",     "PD0R_KP9",     "PD0R_KP9",     "PD0R_KP9"  }},

/* Press 1+9 on controller 0 on the master component */
{ "JS0_BTN_13",{"IPAUSE0",      "IPAUSE0",      "IPAUSE0",      "IPAUSE0"   }},
/* Press 1+9 on controller 1 on the master component */
{ "JS1_BTN_13",{"IPAUSE1",      "IPAUSE1",      "IPAUSE1",      "IPAUSE1"   }},
#endif
#ifdef N900
{ "Q",      {   "PD0R_KP1",     "PD0L_KP1",     "KEYB_1",       "NA"        }},
{ "W",      {   "PD0R_KP2",     "PD0L_KP2",     "KEYB_2",       "NA"        }},
{ "E",      {   "PD0R_KP3",     "PD0L_KP3",     "KEYB_3",       "NA"        }},
{ "R",      {   "PD0R_KP4",     "PD0L_KP4",     "KEYB_4",       "NA"        }},
{ "T",      {   "PD0R_KP5",     "PD0L_KP5",     "KEYB_5",       "NA"        }},
{ "Y",      {   "PD0R_KP6",     "PD0L_KP6",     "KEYB_6",       "NA"        }},
{ "U",      {   "PD0R_KP7",     "PD0L_KP7",     "KEYB_7",       "NA"        }},
{ "I",      {   "PD0R_KP8",     "PD0L_KP8",     "KEYB_8",       "NA"        }},
{ "O",      {   "PD0R_KP9",     "PD0L_KP9",     "KEYB_9",       "NA"        }},
{ "COMMA",  {   "PD0R_KPC",     "PD0L_KPC",     "KEYB_MINUS",   "NA"        }},
{ "P",      {   "PD0R_KP0",     "PD0L_KP0",     "KEYB_0",       "NA"        }},
{ "F",      {   "BREAK",        "BREAK",        "BREAK",        "BREAK"     }},
{ "G",      {   "KBD0",         "KBD0",         "KBD0",         "KBD0"      }},
{ "H",      {   "KBD1",         "KBD1",         "KBD1",         "KBD1"      }},
{ "J",      {   "RESET",        "RESET",        "RESET",        "RESET"     }},
#endif
{ NULL,     {   NULL,           NULL,           NULL,           NULL        }},
};

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
