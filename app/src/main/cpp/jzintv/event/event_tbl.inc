/* ======================================================================== */
/*  This file is meant to be #included after defining the macros EVT_DECL   */
/*  and EVT_DECL_A.  The A stands for Alias, and is used to declare         */
/*  alternate names for events that can have more than one name.            */
/*                                                                          */
/*  EVT_DECL takes two arguments: name, event_number.  The macro expands    */
/*  these as appropriate to build the corresponding table.  EVT_DECL_A      */
/*  takes the same arguments.                                               */
/*                                                                          */
/*  The trailing comma for each entry must be inside the macro.  This       */
/*  allows the macro to expand to an empty string if needed.                */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  Events that correspond to keyboard events.  The names derive from SDL   */
/*  keycode names.  On systems that use SDL, it's generally a 1:1 mapping;  */
/*  however some names are only in SDL1 and some are only in SDL2.          */
/* ------------------------------------------------------------------------ */
EVT_DECL  ("UNKNOWN",      EVENT_UNKNOWN     )
EVT_DECL  ("BACKSPACE",    EVENT_BACKSPACE   )
EVT_DECL  ("TAB",          EVENT_TAB         )
EVT_DECL  ("CLEAR",        EVENT_CLEAR       )
EVT_DECL  ("RETURN",       EVENT_RETURN      )
EVT_DECL  ("PAUSE",        EVENT_PAUSE       )
EVT_DECL  ("ESCAPE",       EVENT_ESCAPE      )
EVT_DECL  ("SPACE",        EVENT_SPACE       )
EVT_DECL  ("EXCLAIM",      EVENT_EXCLAIM     )
EVT_DECL  ("QUOTEDBL",     EVENT_QUOTEDBL    )
EVT_DECL  ("HASH",         EVENT_HASH        )
EVT_DECL  ("DOLLAR",       EVENT_DOLLAR      )
EVT_DECL  ("AMPERSAND",    EVENT_AMPERSAND   )
EVT_DECL  ("QUOTE",        EVENT_QUOTE       )
EVT_DECL  ("LEFTPAREN",    EVENT_LEFTPAREN   )
EVT_DECL  ("RIGHTPAREN",   EVENT_RIGHTPAREN  )
EVT_DECL  ("ASTERISK",     EVENT_ASTERISK    )
EVT_DECL  ("PLUS",         EVENT_PLUS        )
EVT_DECL  ("COMMA",        EVENT_COMMA       )
EVT_DECL  ("MINUS",        EVENT_MINUS       )
EVT_DECL  ("PERIOD",       EVENT_PERIOD      )
EVT_DECL  ("SLASH",        EVENT_SLASH       )
EVT_DECL  ("0",            EVENT_0           )
EVT_DECL  ("1",            EVENT_1           )
EVT_DECL  ("2",            EVENT_2           )
EVT_DECL  ("3",            EVENT_3           )
EVT_DECL  ("4",            EVENT_4           )
EVT_DECL  ("5",            EVENT_5           )
EVT_DECL  ("6",            EVENT_6           )
EVT_DECL  ("7",            EVENT_7           )
EVT_DECL  ("8",            EVENT_8           )
EVT_DECL  ("9",            EVENT_9           )
EVT_DECL  ("COLON",        EVENT_COLON       )
EVT_DECL  ("SEMICOLON",    EVENT_SEMICOLON   )
EVT_DECL  ("LESS",         EVENT_LESS        )
EVT_DECL  ("EQUALS",       EVENT_EQUALS      )
EVT_DECL  ("GREATER",      EVENT_GREATER     )
EVT_DECL  ("QUESTION",     EVENT_QUESTION    )
EVT_DECL  ("AT",           EVENT_AT          )
EVT_DECL  ("LEFTBRACKET",  EVENT_LEFTBRACKET )
EVT_DECL  ("BACKSLASH",    EVENT_BACKSLASH   )
EVT_DECL  ("RIGHTBRACKET", EVENT_RIGHTBRACKET)
EVT_DECL  ("CARET",        EVENT_CARET       )
EVT_DECL  ("UNDERSCORE",   EVENT_UNDERSCORE  )
EVT_DECL  ("BACKQUOTE",    EVENT_BACKQUOTE   )
EVT_DECL  ("A",            EVENT_A           )
EVT_DECL  ("B",            EVENT_B           )
EVT_DECL  ("C",            EVENT_C           )
EVT_DECL  ("D",            EVENT_D           )
EVT_DECL  ("E",            EVENT_E           )
EVT_DECL  ("F",            EVENT_F           )
EVT_DECL  ("G",            EVENT_G           )
EVT_DECL  ("H",            EVENT_H           )
EVT_DECL  ("I",            EVENT_I           )
EVT_DECL  ("J",            EVENT_J           )
EVT_DECL  ("K",            EVENT_K           )
EVT_DECL  ("L",            EVENT_L           )
EVT_DECL  ("M",            EVENT_M           )
EVT_DECL  ("N",            EVENT_N           )
EVT_DECL  ("O",            EVENT_O           )
EVT_DECL  ("P",            EVENT_P           )
EVT_DECL  ("Q",            EVENT_Q           )
EVT_DECL  ("R",            EVENT_R           )
EVT_DECL  ("S",            EVENT_S           )
EVT_DECL  ("T",            EVENT_T           )
EVT_DECL  ("U",            EVENT_U           )
EVT_DECL  ("V",            EVENT_V           )
EVT_DECL  ("W",            EVENT_W           )
EVT_DECL  ("X",            EVENT_X           )
EVT_DECL  ("Y",            EVENT_Y           )
EVT_DECL  ("Z",            EVENT_Z           )
EVT_DECL  ("DELETE",       EVENT_DELETE      )
/*  Note: KP0 - KP9 were the SDL1 names, while KP_0 - KP_9 are SDL2.        */
/*  jzIntv treats the SDL1 names as aliases, and treats SDL2 as canonical.  */
EVT_DECL_A("KP0",          EVENT_KP_0        )
EVT_DECL_A("KP1",          EVENT_KP_1        )
EVT_DECL_A("KP2",          EVENT_KP_2        )
EVT_DECL_A("KP3",          EVENT_KP_3        )
EVT_DECL_A("KP4",          EVENT_KP_4        )
EVT_DECL_A("KP5",          EVENT_KP_5        )
EVT_DECL_A("KP6",          EVENT_KP_6        )
EVT_DECL_A("KP7",          EVENT_KP_7        )
EVT_DECL_A("KP8",          EVENT_KP_8        )
EVT_DECL_A("KP9",          EVENT_KP_9        )
EVT_DECL  ("KP_0",         EVENT_KP_0        )
EVT_DECL  ("KP_1",         EVENT_KP_1        )
EVT_DECL  ("KP_2",         EVENT_KP_2        )
EVT_DECL  ("KP_3",         EVENT_KP_3        )
EVT_DECL  ("KP_4",         EVENT_KP_4        )
EVT_DECL  ("KP_5",         EVENT_KP_5        )
EVT_DECL  ("KP_6",         EVENT_KP_6        )
EVT_DECL  ("KP_7",         EVENT_KP_7        )
EVT_DECL  ("KP_8",         EVENT_KP_8        )
EVT_DECL  ("KP_9",         EVENT_KP_9        )
EVT_DECL  ("KP_PERIOD",    EVENT_KP_PERIOD   )
EVT_DECL  ("KP_DIVIDE",    EVENT_KP_DIVIDE   )
EVT_DECL  ("KP_MULTIPLY",  EVENT_KP_MULTIPLY )
EVT_DECL  ("KP_MINUS",     EVENT_KP_MINUS    )
EVT_DECL  ("KP_PLUS",      EVENT_KP_PLUS     )
EVT_DECL  ("KP_ENTER",     EVENT_KP_ENTER    )
EVT_DECL  ("KP_EQUALS",    EVENT_KP_EQUALS   )
EVT_DECL  ("UP",           EVENT_UP          )
EVT_DECL  ("DOWN",         EVENT_DOWN        )
EVT_DECL  ("RIGHT",        EVENT_RIGHT       )
EVT_DECL  ("LEFT",         EVENT_LEFT        )
EVT_DECL  ("INSERT",       EVENT_INSERT      )
EVT_DECL  ("HOME",         EVENT_HOME        )
EVT_DECL  ("END",          EVENT_END         )
EVT_DECL  ("PAGEUP",       EVENT_PAGEUP      )
EVT_DECL  ("PAGEDOWN",     EVENT_PAGEDOWN    )
EVT_DECL  ("F1",           EVENT_F1          )
EVT_DECL  ("F2",           EVENT_F2          )
EVT_DECL  ("F3",           EVENT_F3          )
EVT_DECL  ("F4",           EVENT_F4          )
EVT_DECL  ("F5",           EVENT_F5          )
EVT_DECL  ("F6",           EVENT_F6          )
EVT_DECL  ("F7",           EVENT_F7          )
EVT_DECL  ("F8",           EVENT_F8          )
EVT_DECL  ("F9",           EVENT_F9          )
EVT_DECL  ("F10",          EVENT_F10         )
EVT_DECL  ("F11",          EVENT_F11         )
EVT_DECL  ("F12",          EVENT_F12         )
EVT_DECL  ("F13",          EVENT_F13         )
EVT_DECL  ("F14",          EVENT_F14         )
EVT_DECL  ("F15",          EVENT_F15         )
EVT_DECL  ("F16",          EVENT_F16         )  /* SDL2 only */
EVT_DECL  ("F17",          EVENT_F17         )  /* SDL2 only */
EVT_DECL  ("F18",          EVENT_F18         )  /* SDL2 only */
EVT_DECL  ("F19",          EVENT_F19         )  /* SDL2 only */
EVT_DECL  ("F20",          EVENT_F20         )  /* SDL2 only */
EVT_DECL  ("F21",          EVENT_F21         )  /* SDL2 only */
EVT_DECL  ("F22",          EVENT_F22         )  /* SDL2 only */
EVT_DECL  ("F23",          EVENT_F23         )  /* SDL2 only */
EVT_DECL  ("F24",          EVENT_F24         )  /* SDL2 only */
EVT_DECL  ("NUMLOCK",      EVENT_NUMLOCK     )  /* SDL2's NUMLOCKCLEAR */
EVT_DECL  ("CAPSLOCK",     EVENT_CAPSLOCK    )
EVT_DECL  ("SCROLLOCK",    EVENT_SCROLLOCK   )
EVT_DECL  ("LSHIFT",       EVENT_LSHIFT      )
EVT_DECL  ("RSHIFT",       EVENT_RSHIFT      )
EVT_DECL  ("LCTRL",        EVENT_LCTRL       )
EVT_DECL  ("RCTRL",        EVENT_RCTRL       )
EVT_DECL  ("LALT",         EVENT_LALT        )
EVT_DECL  ("RALT",         EVENT_RALT        )
/*  The LMETA/RMETA/LSUPER/RSUPER/LGUI/RGUI situation is a mess between     */
/*  SDL1 and SDL2.  See "GUI Key Aliases" below for my attempt at backward  */
/*  compatibility for jzIntv between SDL1 and SDL2 builds.                  */
EVT_DECL  ("MODE",         EVENT_MODE        )
EVT_DECL_A("ALTGR",        EVENT_MODE        )  /* AltGR key (European). */
EVT_DECL  ("COMPOSE",      EVENT_COMPOSE     )
EVT_DECL  ("HELP",         EVENT_HELP        )
EVT_DECL  ("PRINT",        EVENT_PRINT       )
EVT_DECL  ("SYSREQ",       EVENT_SYSREQ      )
EVT_DECL  ("BREAK",        EVENT_BREAK       )
EVT_DECL  ("MENU",         EVENT_MENU        )
EVT_DECL  ("POWER",        EVENT_POWER       )
EVT_DECL  ("EURO",         EVENT_EURO        )  /* SDL1 only */
EVT_DECL  ("UNDO",         EVENT_UNDO        )

/* ------------------------------------------------------------------------ */
/*  Some additional miscellaneous keyboard events SDL2 provides that        */
/*  SDL1 apparently did not.  SDL1 builds will not generate these.          */
/* ------------------------------------------------------------------------ */
EVT_DECL  ("SELECT",             EVENT_SELECT            )
EVT_DECL  ("STOP",               EVENT_STOP              )
EVT_DECL  ("AGAIN",              EVENT_AGAIN             )
EVT_DECL  ("CUT",                EVENT_CUT               )
EVT_DECL  ("COPY",               EVENT_COPY              )
EVT_DECL  ("PASTE",              EVENT_PASTE             )
EVT_DECL  ("FIND",               EVENT_FIND              )
EVT_DECL  ("MUTE",               EVENT_MUTE              )
EVT_DECL  ("VOLUMEUP",           EVENT_VOLUMEUP          )
EVT_DECL  ("VOLUMEDOWN",         EVENT_VOLUMEDOWN        )
EVT_DECL  ("KP_COMMA",           EVENT_KP_COMMA          )
EVT_DECL  ("KP_000",             EVENT_KP_000            )
EVT_DECL  ("KP_00",              EVENT_KP_00             )
EVT_DECL  ("KP_EQUALSAS400",     EVENT_KP_EQUALSAS400    )
EVT_DECL  ("CANCEL",             EVENT_CANCEL            )
EVT_DECL  ("PRIOR",              EVENT_PRIOR             )
EVT_DECL  ("SEPARATOR",          EVENT_SEPARATOR         )
EVT_DECL  ("OUT",                EVENT_OUT               )
EVT_DECL  ("OPER",               EVENT_OPER              )
EVT_DECL  ("CLEARAGAIN",         EVENT_CLEARAGAIN        )
EVT_DECL  ("CRSEL",              EVENT_CRSEL             )
EVT_DECL  ("EXSEL",              EVENT_EXSEL             )
EVT_DECL  ("THOUSANDSSEPARATOR", EVENT_THOUSANDSSEPARATOR)
EVT_DECL  ("DECIMALSEPARATOR",   EVENT_DECIMALSEPARATOR  )
EVT_DECL  ("CURRENCYUNIT",       EVENT_CURRENCYUNIT      )
EVT_DECL  ("CURRENCYSUBUNIT",    EVENT_CURRENCYSUBUNIT   )
EVT_DECL  ("KP_LEFTPAREN",       EVENT_KP_LEFTPAREN      )
EVT_DECL  ("KP_RIGHTPAREN",      EVENT_KP_RIGHTPAREN     )
EVT_DECL  ("KP_LEFTBRACE",       EVENT_KP_LEFTBRACE      )
EVT_DECL  ("KP_RIGHTBRACE",      EVENT_KP_RIGHTBRACE     )
EVT_DECL  ("KP_TAB",             EVENT_KP_TAB            )
EVT_DECL  ("KP_BACKSPACE",       EVENT_KP_BACKSPACE      )
EVT_DECL  ("KP_A",               EVENT_KP_A              )
EVT_DECL  ("KP_B",               EVENT_KP_B              )
EVT_DECL  ("KP_C",               EVENT_KP_C              )
EVT_DECL  ("KP_D",               EVENT_KP_D              )
EVT_DECL  ("KP_E",               EVENT_KP_E              )
EVT_DECL  ("KP_F",               EVENT_KP_F              )
EVT_DECL  ("KP_XOR",             EVENT_KP_XOR            )
EVT_DECL  ("KP_POWER",           EVENT_KP_POWER          )
EVT_DECL  ("KP_PERCENT",         EVENT_KP_PERCENT        )
EVT_DECL  ("KP_LESS",            EVENT_KP_LESS           )
EVT_DECL  ("KP_GREATER",         EVENT_KP_GREATER        )
EVT_DECL  ("KP_AMPERSAND",       EVENT_KP_AMPERSAND      )
EVT_DECL  ("KP_DBLAMPERSAND",    EVENT_KP_DBLAMPERSAND   )
EVT_DECL  ("KP_VERTICALBAR",     EVENT_KP_VERTICALBAR    )
EVT_DECL  ("KP_DBLVERTICALBAR",  EVENT_KP_DBLVERTICALBAR )
EVT_DECL  ("KP_COLON",           EVENT_KP_COLON          )
EVT_DECL  ("KP_HASH",            EVENT_KP_HASH           )
EVT_DECL  ("KP_SPACE",           EVENT_KP_SPACE          )
EVT_DECL  ("KP_AT",              EVENT_KP_AT             )
EVT_DECL  ("KP_EXCLAIM",         EVENT_KP_EXCLAIM        )
EVT_DECL  ("KP_MEMSTORE",        EVENT_KP_MEMSTORE       )
EVT_DECL  ("KP_MEMRECALL",       EVENT_KP_MEMRECALL      )
EVT_DECL  ("KP_MEMCLEAR",        EVENT_KP_MEMCLEAR       )
EVT_DECL  ("KP_MEMADD",          EVENT_KP_MEMADD         )
EVT_DECL  ("KP_MEMSUBTRACT",     EVENT_KP_MEMSUBTRACT    )
EVT_DECL  ("KP_MEMMULTIPLY",     EVENT_KP_MEMMULTIPLY    )
EVT_DECL  ("KP_MEMDIVIDE",       EVENT_KP_MEMDIVIDE      )
EVT_DECL  ("KP_PLUSMINUS",       EVENT_KP_PLUSMINUS      )
EVT_DECL  ("KP_CLEAR",           EVENT_KP_CLEAR          )
EVT_DECL  ("KP_CLEARENTRY",      EVENT_KP_CLEARENTRY     )
EVT_DECL  ("KP_BINARY",          EVENT_KP_BINARY         )
EVT_DECL  ("KP_OCTAL",           EVENT_KP_OCTAL          )
EVT_DECL  ("KP_DECIMAL",         EVENT_KP_DECIMAL        )
EVT_DECL  ("KP_HEXADECIMAL",     EVENT_KP_HEXADECIMAL    )
EVT_DECL  ("AUDIONEXT",          EVENT_AUDIONEXT         )
EVT_DECL  ("AUDIOPREV",          EVENT_AUDIOPREV         )
EVT_DECL  ("AUDIOSTOP",          EVENT_AUDIOSTOP         )
EVT_DECL  ("AUDIOPLAY",          EVENT_AUDIOPLAY         )
EVT_DECL  ("AUDIOMUTE",          EVENT_AUDIOMUTE         )
EVT_DECL  ("MEDIASELECT",        EVENT_MEDIASELECT       )
EVT_DECL  ("WWW",                EVENT_WWW               )
EVT_DECL  ("MAIL",               EVENT_MAIL              )
EVT_DECL  ("CALCULATOR",         EVENT_CALCULATOR        )
EVT_DECL  ("COMPUTER",           EVENT_COMPUTER          )
EVT_DECL  ("AC_SEARCH",          EVENT_AC_SEARCH         )
EVT_DECL  ("AC_HOME",            EVENT_AC_HOME           )
EVT_DECL  ("AC_BACK",            EVENT_AC_BACK           )
EVT_DECL  ("AC_FORWARD",         EVENT_AC_FORWARD        )
EVT_DECL  ("AC_STOP",            EVENT_AC_STOP           )
EVT_DECL  ("AC_REFRESH",         EVENT_AC_REFRESH        )
EVT_DECL  ("AC_BOOKMARKS",       EVENT_AC_BOOKMARKS      )
EVT_DECL  ("BRIGHTNESSDOWN",     EVENT_BRIGHTNESSDOWN    )
EVT_DECL  ("BRIGHTNESSUP",       EVENT_BRIGHTNESSUP      )
EVT_DECL  ("DISPLAYSWITCH",      EVENT_DISPLAYSWITCH     )
EVT_DECL  ("KBDILLUMTOGGLE",     EVENT_KBDILLUMTOGGLE    )
EVT_DECL  ("KBDILLUMDOWN",       EVENT_KBDILLUMDOWN      )
EVT_DECL  ("KBDILLUMUP",         EVENT_KBDILLUMUP        )
EVT_DECL  ("EJECT",              EVENT_EJECT             )
EVT_DECL  ("SLEEP",              EVENT_SLEEP             )

/* -------------------------------------------------------------------- */
/*  Aliases for punctuation event names.                                */
/* -------------------------------------------------------------------- */
EVT_DECL_A(" ",  EVENT_SPACE       )
EVT_DECL_A("!",  EVENT_EXCLAIM     )
EVT_DECL_A("\"", EVENT_QUOTEDBL    )
EVT_DECL_A("#",  EVENT_HASH        )
EVT_DECL_A("$",  EVENT_DOLLAR      )
EVT_DECL_A("&",  EVENT_AMPERSAND   )
EVT_DECL_A("'",  EVENT_QUOTE       )
EVT_DECL_A("(",  EVENT_LEFTPAREN   )
EVT_DECL_A(")",  EVENT_RIGHTPAREN  )
EVT_DECL_A("*",  EVENT_ASTERISK    )
EVT_DECL_A("+",  EVENT_PLUS        )
EVT_DECL_A(",",  EVENT_COMMA       )
EVT_DECL_A("-",  EVENT_MINUS       )
EVT_DECL_A(".",  EVENT_PERIOD      )
EVT_DECL_A("/",  EVENT_SLASH       )
EVT_DECL_A(":",  EVENT_COLON       )
EVT_DECL_A(";",  EVENT_SEMICOLON   )
EVT_DECL_A("<",  EVENT_LESS        )
EVT_DECL_A("=",  EVENT_EQUALS      )
EVT_DECL_A(">",  EVENT_GREATER     )
EVT_DECL_A("?",  EVENT_QUESTION    )
EVT_DECL_A("@",  EVENT_AT          )
EVT_DECL_A("[",  EVENT_LEFTBRACKET )
EVT_DECL_A("\\", EVENT_BACKSLASH   )
EVT_DECL_A("]",  EVENT_RIGHTBRACKET)
EVT_DECL_A("^",  EVENT_CARET       )
EVT_DECL_A("_",  EVENT_UNDERSCORE  )
EVT_DECL_A("`",  EVENT_BACKQUOTE   )

/* ------------------------------------------------------------------------ */
/*  GUI Key Aliases.                                                        */
/*                                                                          */
/*  Under SDL1:                                                             */
/*   -- Windows:        Windows keys send xSUPER (alias xWIN, xGUI)         */
/*   -- MacOS:          Command keys send xMETA (alias xCMD, xGUI)          */
/*   -- Linux, others:  Windows keys send xMETA (alias xCMD, xGUI)          */
/*                                                                          */
/*  (Note: xFOO means LFOO, RFOO.)                                          */
/*                                                                          */
/*  Under SDL2, the keys listed above simply send LGUI, RGUI.  SDL2 does    */
/*  not generate xSUPER or xMETA.                                           */
/*                                                                          */
/*  jzIntv now treats the xGUI names as canonical, and treats the SDL1      */
/*  names xMETA, xSUPER, xWIN, xCMD as aliases where needed, according to   */
/*  the OS build and SDL version.                                           */
/*                                                                          */
/*  Maintenance note:  Make sure all non-alias EVENT_xx below are declared  */
/*  in the same order regardless of which conditional compilation branch    */
/*  is taken!  Use the special name "!BAD!" for events that won't get an    */
/*  event name asspciated with them.                                        */
/*                                                                          */
/*  If you change the following order, update the STATIC_ASSERT() in        */
/*  event/event_tbl.c:                                                      */
/*                                                                          */
/*      LMETA, RMETA, LSUPER, RSUPER, LGUI, RGUI                            */
/* ------------------------------------------------------------------------ */
#if defined(WIN32)
# if defined(USE_SDL2)
    /* Windows, SDL2 */
    EVT_DECL  ("!BAD!",  EVENT_LMETA)
    EVT_DECL  ("!BAD!",  EVENT_RMETA)
    EVT_DECL  ("!BAD!",  EVENT_LSUPER)
    EVT_DECL  ("!BAD!",  EVENT_RSUPER)

    EVT_DECL_A("LWIN",   EVENT_LGUI)
    EVT_DECL_A("RWIN",   EVENT_RGUI)
    EVT_DECL_A("LCMD",   EVENT_LGUI)
    EVT_DECL_A("RCMD",   EVENT_RGUI)
    EVT_DECL_A("LSUPER", EVENT_LGUI)
    EVT_DECL_A("RSUPER", EVENT_RGUI)
    EVT_DECL_A("LMETA",  EVENT_IGNORE)  /* Ignore these in a hackfile */
    EVT_DECL_A("RMETA",  EVENT_IGNORE)  /* Ignore these in a hackfile */

    EVT_DECL  ("LGUI",   EVENT_LGUI)
    EVT_DECL  ("RGUI",   EVENT_RGUI)
# else /* !defined(USE_SDL2) */
    /* Windows, SDL1 */
    EVT_DECL_A("LWIN",   EVENT_LSUPER)
    EVT_DECL_A("RWIN",   EVENT_RSUPER)
    EVT_DECL_A("LCMD",   EVENT_LMETA)
    EVT_DECL_A("RCMD",   EVENT_RMETA)
    EVT_DECL_A("LSUPER", EVENT_LSUPER)
    EVT_DECL_A("RSUPER", EVENT_RSUPER)
    EVT_DECL  ("LMETA",  EVENT_LMETA)
    EVT_DECL  ("RMETA",  EVENT_RMETA)

    EVT_DECL  ("LGUI",   EVENT_LSUPER)
    EVT_DECL  ("RGUI",   EVENT_RSUPER)

    EVT_DECL  ("!BAD!",  EVENT_LGUI)
    EVT_DECL  ("!BAD!",  EVENT_RGUI)
# endif /* USE_SDL2 */
#else /* !defined(WIN32) */
# if defined(USE_SDL2)
    /* Non-Windows, SDL2 */
    EVT_DECL  ("!BAD!",  EVENT_LMETA)
    EVT_DECL  ("!BAD!",  EVENT_RMETA)
    EVT_DECL  ("!BAD!",  EVENT_LSUPER)
    EVT_DECL  ("!BAD!",  EVENT_RSUPER)

    EVT_DECL_A("LWIN",   EVENT_LGUI)
    EVT_DECL_A("RWIN",   EVENT_RGUI)
    EVT_DECL_A("LCMD",   EVENT_LGUI)
    EVT_DECL_A("RCMD",   EVENT_RGUI)
    EVT_DECL_A("LSUPER", EVENT_IGNORE)  /* Ignore these in a hackfile */
    EVT_DECL_A("RSUPER", EVENT_IGNORE)  /* Ignore these in a hackfile */
    EVT_DECL_A("LMETA",  EVENT_LGUI)
    EVT_DECL_A("RMETA",  EVENT_RGUI)

    EVT_DECL  ("LGUI",   EVENT_LGUI)
    EVT_DECL  ("RGUI",   EVENT_RGUI)
# else /* Non-Windows, SDL1 */
    EVT_DECL_A("LWIN",   EVENT_LSUPER)
    EVT_DECL_A("RWIN",   EVENT_RSUPER)
    EVT_DECL_A("LCMD",   EVENT_LMETA)
    EVT_DECL_A("RCMD",   EVENT_RMETA)

    EVT_DECL_A("LMETA",  EVENT_LMETA)
    EVT_DECL_A("RMETA",  EVENT_RMETA)
    EVT_DECL  ("LGUI",   EVENT_LMETA)
    EVT_DECL  ("RGUI",   EVENT_RMETA)

    EVT_DECL  ("LSUPER", EVENT_LSUPER)
    EVT_DECL  ("RSUPER", EVENT_RSUPER)

    EVT_DECL  ("!BAD!",  EVENT_LGUI)
    EVT_DECL  ("!BAD!",  EVENT_RGUI)
# endif /* defined(USE_SDL2) */
#endif /* defined(WIN32) */

/* ------------------------------------------------------------------------ */
/*  The QUIT event, which corresponds to an OS termination request.         */
/* ------------------------------------------------------------------------ */
EVT_DECL  ("QUIT", EVENT_QUIT)

/* ------------------------------------------------------------------------ */
/*  The HIDE event is lets us know when we're iconified or not.             */
/*  A "HIDE DOWN" event means we're iconified, and a "HIDE UP" event means  */
/*  we're visible.  jzIntv disables graphics updates while it's iconified.  */
/* ------------------------------------------------------------------------ */
EVT_DECL  ("HIDE", EVENT_HIDE)

/* ------------------------------------------------------------------------ */
/*  The FOCUS events lets us know when our window has gained or lost focus. */
/*  Sends both a down and an up event for each.                             */
/* ------------------------------------------------------------------------ */
EVT_DECL  ("FOCUS_GAINED",  EVENT_FOCUS_GAINED)
EVT_DECL  ("FOCUS_LOST",    EVENT_FOCUS_LOST)

/* ------------------------------------------------------------------------ */
/*  "Scancode" events.  These are arbitrarily defined, and mainly intended  */
/*  to help with porting jzIntv to devices that report events jzIntv has    */
/*  not anticipated yet.  For now, define 512 scancode events.  Use with    */
/*  event_diag.rom to discover what events are getting reported.            */
/*                                                                          */
/*  The scancode events *must* be declared in numerical order.              */
/* ------------------------------------------------------------------------ */
#define EVT_DECL__SCANCODE(y,x) \
    EVT_DECL  ("SCANCODE_" #y #x, EVENT_SCANCODE_##y##x)

#define EVT_DECL__SCANCODE_X(y) \
    EVT_DECL__SCANCODE(y, 0)  EVT_DECL__SCANCODE(y, 1)  \
    EVT_DECL__SCANCODE(y, 2)  EVT_DECL__SCANCODE(y, 3)  \
    EVT_DECL__SCANCODE(y, 4)  EVT_DECL__SCANCODE(y, 5)  \
    EVT_DECL__SCANCODE(y, 6)  EVT_DECL__SCANCODE(y, 7)  \
    EVT_DECL__SCANCODE(y, 8)  EVT_DECL__SCANCODE(y, 9)  \
    EVT_DECL__SCANCODE(y, A)  EVT_DECL__SCANCODE(y, B)  \
    EVT_DECL__SCANCODE(y, C)  EVT_DECL__SCANCODE(y, D)  \
    EVT_DECL__SCANCODE(y, E)  EVT_DECL__SCANCODE(y, F)

    EVT_DECL__SCANCODE_X(00)  EVT_DECL__SCANCODE_X(01)
    EVT_DECL__SCANCODE_X(02)  EVT_DECL__SCANCODE_X(03)
    EVT_DECL__SCANCODE_X(04)  EVT_DECL__SCANCODE_X(05)
    EVT_DECL__SCANCODE_X(06)  EVT_DECL__SCANCODE_X(07)
    EVT_DECL__SCANCODE_X(08)  EVT_DECL__SCANCODE_X(09)
    EVT_DECL__SCANCODE_X(0A)  EVT_DECL__SCANCODE_X(0B)
    EVT_DECL__SCANCODE_X(0C)  EVT_DECL__SCANCODE_X(0D)
    EVT_DECL__SCANCODE_X(0E)  EVT_DECL__SCANCODE_X(0F)
    EVT_DECL__SCANCODE_X(10)  EVT_DECL__SCANCODE_X(11)
    EVT_DECL__SCANCODE_X(12)  EVT_DECL__SCANCODE_X(13)
    EVT_DECL__SCANCODE_X(14)  EVT_DECL__SCANCODE_X(15)
    EVT_DECL__SCANCODE_X(16)  EVT_DECL__SCANCODE_X(17)
    EVT_DECL__SCANCODE_X(18)  EVT_DECL__SCANCODE_X(19)
    EVT_DECL__SCANCODE_X(1A)  EVT_DECL__SCANCODE_X(1B)
    EVT_DECL__SCANCODE_X(1C)  EVT_DECL__SCANCODE_X(1D)
    EVT_DECL__SCANCODE_X(1E)  EVT_DECL__SCANCODE_X(1F)

/* ------------------------------------------------------------------------ */
/*  If you need to add key-like events to support some other framework      */
/*  that don't map cleanly to the events above, add them here.              */
/* ------------------------------------------------------------------------ */
/* your additional framework-specific events here */


/* ------------------------------------------------------------------------ */
/*  The joystick buttons (up to 32).                                        */
/*                                                                          */
/*  Joystick button events were moved up here to make them eligible for     */
/*  COMBO presses.  The remaining joystick events are after COMBO, and not  */
/*  eligible to form COMBOs.                                                */
/*                                                                          */
/*  The button events *must* be declared in numerical order.                */
/* ------------------------------------------------------------------------ */
#define EVT_DECL__JOY_BTN(n) \
    EVT_DECL  (#n "_BTN_00", EVENT_##n##_BTN_00) \
    EVT_DECL  (#n "_BTN_01", EVENT_##n##_BTN_01) \
    EVT_DECL  (#n "_BTN_02", EVENT_##n##_BTN_02) \
    EVT_DECL  (#n "_BTN_03", EVENT_##n##_BTN_03) \
    EVT_DECL  (#n "_BTN_04", EVENT_##n##_BTN_04) \
    EVT_DECL  (#n "_BTN_05", EVENT_##n##_BTN_05) \
    EVT_DECL  (#n "_BTN_06", EVENT_##n##_BTN_06) \
    EVT_DECL  (#n "_BTN_07", EVENT_##n##_BTN_07) \
    EVT_DECL  (#n "_BTN_08", EVENT_##n##_BTN_08) \
    EVT_DECL  (#n "_BTN_09", EVENT_##n##_BTN_09) \
    EVT_DECL  (#n "_BTN_10", EVENT_##n##_BTN_10) \
    EVT_DECL  (#n "_BTN_11", EVENT_##n##_BTN_11) \
    EVT_DECL  (#n "_BTN_12", EVENT_##n##_BTN_12) \
    EVT_DECL  (#n "_BTN_13", EVENT_##n##_BTN_13) \
    EVT_DECL  (#n "_BTN_14", EVENT_##n##_BTN_14) \
    EVT_DECL  (#n "_BTN_15", EVENT_##n##_BTN_15) \
    EVT_DECL  (#n "_BTN_16", EVENT_##n##_BTN_16) \
    EVT_DECL  (#n "_BTN_17", EVENT_##n##_BTN_17) \
    EVT_DECL  (#n "_BTN_18", EVENT_##n##_BTN_18) \
    EVT_DECL  (#n "_BTN_19", EVENT_##n##_BTN_19) \
    EVT_DECL  (#n "_BTN_20", EVENT_##n##_BTN_20) \
    EVT_DECL  (#n "_BTN_21", EVENT_##n##_BTN_21) \
    EVT_DECL  (#n "_BTN_22", EVENT_##n##_BTN_22) \
    EVT_DECL  (#n "_BTN_23", EVENT_##n##_BTN_23) \
    EVT_DECL  (#n "_BTN_24", EVENT_##n##_BTN_24) \
    EVT_DECL  (#n "_BTN_25", EVENT_##n##_BTN_25) \
    EVT_DECL  (#n "_BTN_26", EVENT_##n##_BTN_26) \
    EVT_DECL  (#n "_BTN_27", EVENT_##n##_BTN_27) \
    EVT_DECL  (#n "_BTN_28", EVENT_##n##_BTN_28) \
    EVT_DECL  (#n "_BTN_29", EVENT_##n##_BTN_29) \
    EVT_DECL  (#n "_BTN_30", EVENT_##n##_BTN_30) \
    EVT_DECL  (#n "_BTN_31", EVENT_##n##_BTN_31)

EVT_DECL__JOY_BTN(JS0) EVT_DECL__JOY_BTN(JS1) EVT_DECL__JOY_BTN(JS2)
EVT_DECL__JOY_BTN(JS3) EVT_DECL__JOY_BTN(JS4) EVT_DECL__JOY_BTN(JS5)
EVT_DECL__JOY_BTN(JS6) EVT_DECL__JOY_BTN(JS7) EVT_DECL__JOY_BTN(JS8)
EVT_DECL__JOY_BTN(JS9)

/* -------------------------------------------------------------------- */
/*  COMBO events -- these are synthesized out of two other events.      */
/* -------------------------------------------------------------------- */
EVT_DECL  ("COMBO0",  EVENT_COMBO0 )
EVT_DECL  ("COMBO1",  EVENT_COMBO1 )
EVT_DECL  ("COMBO2",  EVENT_COMBO2 )
EVT_DECL  ("COMBO3",  EVENT_COMBO3 )
EVT_DECL  ("COMBO4",  EVENT_COMBO4 )
EVT_DECL  ("COMBO5",  EVENT_COMBO5 )
EVT_DECL  ("COMBO6",  EVENT_COMBO6 )
EVT_DECL  ("COMBO7",  EVENT_COMBO7 )
EVT_DECL  ("COMBO8",  EVENT_COMBO8 )
EVT_DECL  ("COMBO9",  EVENT_COMBO9 )
EVT_DECL  ("COMBO10", EVENT_COMBO10)
EVT_DECL  ("COMBO11", EVENT_COMBO11)
EVT_DECL  ("COMBO12", EVENT_COMBO12)
EVT_DECL  ("COMBO13", EVENT_COMBO13)
EVT_DECL  ("COMBO14", EVENT_COMBO14)
EVT_DECL  ("COMBO15", EVENT_COMBO15)
EVT_DECL  ("COMBO16", EVENT_COMBO16)
EVT_DECL  ("COMBO17", EVENT_COMBO17)
EVT_DECL  ("COMBO18", EVENT_COMBO18)
EVT_DECL  ("COMBO19", EVENT_COMBO19)
EVT_DECL  ("COMBO20", EVENT_COMBO20)
EVT_DECL  ("COMBO21", EVENT_COMBO21)
EVT_DECL  ("COMBO22", EVENT_COMBO22)
EVT_DECL  ("COMBO23", EVENT_COMBO23)
EVT_DECL  ("COMBO24", EVENT_COMBO24)
EVT_DECL  ("COMBO25", EVENT_COMBO25)
EVT_DECL  ("COMBO26", EVENT_COMBO26)
EVT_DECL  ("COMBO27", EVENT_COMBO27)
EVT_DECL  ("COMBO28", EVENT_COMBO28)
EVT_DECL  ("COMBO29", EVENT_COMBO29)
EVT_DECL  ("COMBO30", EVENT_COMBO30)
EVT_DECL  ("COMBO31", EVENT_COMBO31)
EVT_DECL  ("COMBO32", EVENT_COMBO32)
EVT_DECL  ("COMBO33", EVENT_COMBO33)
EVT_DECL  ("COMBO34", EVENT_COMBO34)
EVT_DECL  ("COMBO35", EVENT_COMBO35)
EVT_DECL  ("COMBO36", EVENT_COMBO36)
EVT_DECL  ("COMBO37", EVENT_COMBO37)
EVT_DECL  ("COMBO38", EVENT_COMBO38)
EVT_DECL  ("COMBO39", EVENT_COMBO39)
EVT_DECL  ("COMBO40", EVENT_COMBO40)
EVT_DECL  ("COMBO41", EVENT_COMBO41)
EVT_DECL  ("COMBO42", EVENT_COMBO42)
EVT_DECL  ("COMBO43", EVENT_COMBO43)
EVT_DECL  ("COMBO44", EVENT_COMBO44)
EVT_DECL  ("COMBO45", EVENT_COMBO45)
EVT_DECL  ("COMBO46", EVENT_COMBO46)
EVT_DECL  ("COMBO47", EVENT_COMBO47)
EVT_DECL  ("COMBO48", EVENT_COMBO48)
EVT_DECL  ("COMBO49", EVENT_COMBO49)
EVT_DECL  ("COMBO50", EVENT_COMBO50)
EVT_DECL  ("COMBO51", EVENT_COMBO51)
EVT_DECL  ("COMBO52", EVENT_COMBO52)
EVT_DECL  ("COMBO53", EVENT_COMBO53)
EVT_DECL  ("COMBO54", EVENT_COMBO54)
EVT_DECL  ("COMBO55", EVENT_COMBO55)
EVT_DECL  ("COMBO56", EVENT_COMBO56)
EVT_DECL  ("COMBO57", EVENT_COMBO57)
EVT_DECL  ("COMBO58", EVENT_COMBO58)
EVT_DECL  ("COMBO59", EVENT_COMBO59)
EVT_DECL  ("COMBO60", EVENT_COMBO60)
EVT_DECL  ("COMBO61", EVENT_COMBO61)
EVT_DECL  ("COMBO62", EVENT_COMBO62)
EVT_DECL  ("COMBO63", EVENT_COMBO63)

/* ------------------------------------------------------------------------ */
/*  Aliases for the 16 joystick directions that we resolve on stick 'A'.    */
/*  The direction events must be declared in CCW order starting at E.       */
/* ------------------------------------------------------------------------ */
#define EVT_DECL__JOY_DIR_A(n) \
    EVT_DECL_A(#n "_E",   EVENT_##n##A_E  ) \
    EVT_DECL_A(#n "_ENE", EVENT_##n##A_ENE) \
    EVT_DECL_A(#n "_NE",  EVENT_##n##A_NE ) \
    EVT_DECL_A(#n "_NNE", EVENT_##n##A_NNE) \
    EVT_DECL_A(#n "_N",   EVENT_##n##A_N  ) \
    EVT_DECL_A(#n "_NNW", EVENT_##n##A_NNW) \
    EVT_DECL_A(#n "_NW",  EVENT_##n##A_NW ) \
    EVT_DECL_A(#n "_WNW", EVENT_##n##A_WNW) \
    EVT_DECL_A(#n "_W",   EVENT_##n##A_W  ) \
    EVT_DECL_A(#n "_WSW", EVENT_##n##A_WSW) \
    EVT_DECL_A(#n "_SW",  EVENT_##n##A_SW ) \
    EVT_DECL_A(#n "_SSW", EVENT_##n##A_SSW) \
    EVT_DECL_A(#n "_S",   EVENT_##n##A_S  ) \
    EVT_DECL_A(#n "_SSE", EVENT_##n##A_SSE) \
    EVT_DECL_A(#n "_SE",  EVENT_##n##A_SE ) \
    EVT_DECL_A(#n "_ESE", EVENT_##n##A_ESE)

/* ------------------------------------------------------------------------ */ 
/*  The 16 joystick directions that we resolve on each joystick stick.      */ 
/*  The direction events must be declared in CCW order starting at E.       */
/*  The sticks must be declared in numerical order.                         */
/* ------------------------------------------------------------------------ */ 
#define EVT_DECL__JOY_DIR_X(n) \
    EVT_DECL  (#n "_E",   EVENT_##n##_E  ) \
    EVT_DECL  (#n "_ENE", EVENT_##n##_ENE) \
    EVT_DECL  (#n "_NE",  EVENT_##n##_NE ) \
    EVT_DECL  (#n "_NNE", EVENT_##n##_NNE) \
    EVT_DECL  (#n "_N",   EVENT_##n##_N  ) \
    EVT_DECL  (#n "_NNW", EVENT_##n##_NNW) \
    EVT_DECL  (#n "_NW",  EVENT_##n##_NW ) \
    EVT_DECL  (#n "_WNW", EVENT_##n##_WNW) \
    EVT_DECL  (#n "_W",   EVENT_##n##_W  ) \
    EVT_DECL  (#n "_WSW", EVENT_##n##_WSW) \
    EVT_DECL  (#n "_SW",  EVENT_##n##_SW ) \
    EVT_DECL  (#n "_SSW", EVENT_##n##_SSW) \
    EVT_DECL  (#n "_S",   EVENT_##n##_S  ) \
    EVT_DECL  (#n "_SSE", EVENT_##n##_SSE) \
    EVT_DECL  (#n "_SE",  EVENT_##n##_SE ) \
    EVT_DECL  (#n "_ESE", EVENT_##n##_ESE)

#define EVT_DECL__JOY_DIR(n)   \
    EVT_DECL__JOY_DIR_A(n)     \
    EVT_DECL__JOY_DIR_X(n##A)  \
    EVT_DECL__JOY_DIR_X(n##B)  \
    EVT_DECL__JOY_DIR_X(n##C)  \
    EVT_DECL__JOY_DIR_X(n##D)  \
    EVT_DECL__JOY_DIR_X(n##E)  \
    EVT_DECL__JOY_DIR_X(n##F)  \
    EVT_DECL__JOY_DIR_X(n##G)  \
    EVT_DECL__JOY_DIR_X(n##H)  \
    EVT_DECL__JOY_DIR_X(n##I)  \
    EVT_DECL__JOY_DIR_X(n##J)

/* ------------------------------------------------------------------------ */
/*  The 8 joystick hat dirs on 4 hats that we resolve, on each joystick.    */
/*  The direction events must be declared in CCW order starting at E.       */
/*  The hats must be declared in numerical order.                           */
/* ------------------------------------------------------------------------ */
#define EVT_DECL__JOY_HAT(n) \
    EVT_DECL  (#n "_HAT0_E",  EVENT_##n##_HAT0_E ) \
    EVT_DECL  (#n "_HAT0_NE", EVENT_##n##_HAT0_NE) \
    EVT_DECL  (#n "_HAT0_N",  EVENT_##n##_HAT0_N ) \
    EVT_DECL  (#n "_HAT0_NW", EVENT_##n##_HAT0_NW) \
    EVT_DECL  (#n "_HAT0_W",  EVENT_##n##_HAT0_W ) \
    EVT_DECL  (#n "_HAT0_SW", EVENT_##n##_HAT0_SW) \
    EVT_DECL  (#n "_HAT0_S",  EVENT_##n##_HAT0_S ) \
    EVT_DECL  (#n "_HAT0_SE", EVENT_##n##_HAT0_SE) \
    EVT_DECL  (#n "_HAT1_E",  EVENT_##n##_HAT1_E ) \
    EVT_DECL  (#n "_HAT1_NE", EVENT_##n##_HAT1_NE) \
    EVT_DECL  (#n "_HAT1_N",  EVENT_##n##_HAT1_N ) \
    EVT_DECL  (#n "_HAT1_NW", EVENT_##n##_HAT1_NW) \
    EVT_DECL  (#n "_HAT1_W",  EVENT_##n##_HAT1_W ) \
    EVT_DECL  (#n "_HAT1_SW", EVENT_##n##_HAT1_SW) \
    EVT_DECL  (#n "_HAT1_S",  EVENT_##n##_HAT1_S ) \
    EVT_DECL  (#n "_HAT1_SE", EVENT_##n##_HAT1_SE) \
    EVT_DECL  (#n "_HAT2_E",  EVENT_##n##_HAT2_E ) \
    EVT_DECL  (#n "_HAT2_NE", EVENT_##n##_HAT2_NE) \
    EVT_DECL  (#n "_HAT2_N",  EVENT_##n##_HAT2_N ) \
    EVT_DECL  (#n "_HAT2_NW", EVENT_##n##_HAT2_NW) \
    EVT_DECL  (#n "_HAT2_W",  EVENT_##n##_HAT2_W ) \
    EVT_DECL  (#n "_HAT2_SW", EVENT_##n##_HAT2_SW) \
    EVT_DECL  (#n "_HAT2_S",  EVENT_##n##_HAT2_S ) \
    EVT_DECL  (#n "_HAT2_SE", EVENT_##n##_HAT2_SE) \
    EVT_DECL  (#n "_HAT3_E",  EVENT_##n##_HAT3_E ) \
    EVT_DECL  (#n "_HAT3_NE", EVENT_##n##_HAT3_NE) \
    EVT_DECL  (#n "_HAT3_N",  EVENT_##n##_HAT3_N ) \
    EVT_DECL  (#n "_HAT3_NW", EVENT_##n##_HAT3_NW) \
    EVT_DECL  (#n "_HAT3_W",  EVENT_##n##_HAT3_W ) \
    EVT_DECL  (#n "_HAT3_SW", EVENT_##n##_HAT3_SW) \
    EVT_DECL  (#n "_HAT3_S",  EVENT_##n##_HAT3_S ) \
    EVT_DECL  (#n "_HAT3_SE", EVENT_##n##_HAT3_SE) \
    EVT_DECL  (#n "_HAT4_E",  EVENT_##n##_HAT4_E ) \
    EVT_DECL  (#n "_HAT4_NE", EVENT_##n##_HAT4_NE) \
    EVT_DECL  (#n "_HAT4_N",  EVENT_##n##_HAT4_N ) \
    EVT_DECL  (#n "_HAT4_NW", EVENT_##n##_HAT4_NW) \
    EVT_DECL  (#n "_HAT4_W",  EVENT_##n##_HAT4_W ) \
    EVT_DECL  (#n "_HAT4_SW", EVENT_##n##_HAT4_SW) \
    EVT_DECL  (#n "_HAT4_S",  EVENT_##n##_HAT4_S ) \
    EVT_DECL  (#n "_HAT4_SE", EVENT_##n##_HAT4_SE) \
    EVT_DECL  (#n "_HAT5_E",  EVENT_##n##_HAT5_E ) \
    EVT_DECL  (#n "_HAT5_NE", EVENT_##n##_HAT5_NE) \
    EVT_DECL  (#n "_HAT5_N",  EVENT_##n##_HAT5_N ) \
    EVT_DECL  (#n "_HAT5_NW", EVENT_##n##_HAT5_NW) \
    EVT_DECL  (#n "_HAT5_W",  EVENT_##n##_HAT5_W ) \
    EVT_DECL  (#n "_HAT5_SW", EVENT_##n##_HAT5_SW) \
    EVT_DECL  (#n "_HAT5_S",  EVENT_##n##_HAT5_S ) \
    EVT_DECL  (#n "_HAT5_SE", EVENT_##n##_HAT5_SE) \
    EVT_DECL  (#n "_HAT6_E",  EVENT_##n##_HAT6_E ) \
    EVT_DECL  (#n "_HAT6_NE", EVENT_##n##_HAT6_NE) \
    EVT_DECL  (#n "_HAT6_N",  EVENT_##n##_HAT6_N ) \
    EVT_DECL  (#n "_HAT6_NW", EVENT_##n##_HAT6_NW) \
    EVT_DECL  (#n "_HAT6_W",  EVENT_##n##_HAT6_W ) \
    EVT_DECL  (#n "_HAT6_SW", EVENT_##n##_HAT6_SW) \
    EVT_DECL  (#n "_HAT6_S",  EVENT_##n##_HAT6_S ) \
    EVT_DECL  (#n "_HAT6_SE", EVENT_##n##_HAT6_SE) \
    EVT_DECL  (#n "_HAT7_E",  EVENT_##n##_HAT7_E ) \
    EVT_DECL  (#n "_HAT7_NE", EVENT_##n##_HAT7_NE) \
    EVT_DECL  (#n "_HAT7_N",  EVENT_##n##_HAT7_N ) \
    EVT_DECL  (#n "_HAT7_NW", EVENT_##n##_HAT7_NW) \
    EVT_DECL  (#n "_HAT7_W",  EVENT_##n##_HAT7_W ) \
    EVT_DECL  (#n "_HAT7_SW", EVENT_##n##_HAT7_SW) \
    EVT_DECL  (#n "_HAT7_S",  EVENT_##n##_HAT7_S ) \
    EVT_DECL  (#n "_HAT7_SE", EVENT_##n##_HAT7_SE) \
    EVT_DECL  (#n "_HAT8_E",  EVENT_##n##_HAT8_E ) \
    EVT_DECL  (#n "_HAT8_NE", EVENT_##n##_HAT8_NE) \
    EVT_DECL  (#n "_HAT8_N",  EVENT_##n##_HAT8_N ) \
    EVT_DECL  (#n "_HAT8_NW", EVENT_##n##_HAT8_NW) \
    EVT_DECL  (#n "_HAT8_W",  EVENT_##n##_HAT8_W ) \
    EVT_DECL  (#n "_HAT8_SW", EVENT_##n##_HAT8_SW) \
    EVT_DECL  (#n "_HAT8_S",  EVENT_##n##_HAT8_S ) \
    EVT_DECL  (#n "_HAT8_SE", EVENT_##n##_HAT8_SE) \
    EVT_DECL  (#n "_HAT9_E",  EVENT_##n##_HAT9_E ) \
    EVT_DECL  (#n "_HAT9_NE", EVENT_##n##_HAT9_NE) \
    EVT_DECL  (#n "_HAT9_N",  EVENT_##n##_HAT9_N ) \
    EVT_DECL  (#n "_HAT9_NW", EVENT_##n##_HAT9_NW) \
    EVT_DECL  (#n "_HAT9_W",  EVENT_##n##_HAT9_W ) \
    EVT_DECL  (#n "_HAT9_SW", EVENT_##n##_HAT9_SW) \
    EVT_DECL  (#n "_HAT9_S",  EVENT_##n##_HAT9_S ) \
    EVT_DECL  (#n "_HAT9_SE", EVENT_##n##_HAT9_SE)

EVT_DECL__JOY_DIR(JS0)  EVT_DECL__JOY_HAT(JS0)
EVT_DECL__JOY_DIR(JS1)  EVT_DECL__JOY_HAT(JS1)
EVT_DECL__JOY_DIR(JS2)  EVT_DECL__JOY_HAT(JS2)
EVT_DECL__JOY_DIR(JS3)  EVT_DECL__JOY_HAT(JS3)
EVT_DECL__JOY_DIR(JS4)  EVT_DECL__JOY_HAT(JS4)
EVT_DECL__JOY_DIR(JS5)  EVT_DECL__JOY_HAT(JS5)
EVT_DECL__JOY_DIR(JS6)  EVT_DECL__JOY_HAT(JS6)
EVT_DECL__JOY_DIR(JS7)  EVT_DECL__JOY_HAT(JS7)
EVT_DECL__JOY_DIR(JS8)  EVT_DECL__JOY_HAT(JS8)
EVT_DECL__JOY_DIR(JS9)  EVT_DECL__JOY_HAT(JS9)

/* -------------------------------------------------------------------- */
/*  Mouse events -- experimental.                                       */
/* -------------------------------------------------------------------- */
EVT_DECL__JOY_DIR_X(MOUSE)  EVT_DECL__JOY_BTN(MOUSE)

#undef EVT_DECL__SCANCODE
#undef EVT_DECL__SCANCODE_X
#undef EVT_DECL__JOY_BTN
#undef EVT_DECL__JOY_DIR_A
#undef EVT_DECL__JOY_DIR_X
#undef EVT_DECL__JOY_DIR
#undef EVT_DECL__JOY_HAT
