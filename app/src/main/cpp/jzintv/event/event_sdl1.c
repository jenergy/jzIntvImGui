/*
 * ============================================================================
 *  Title:    Event Handling Driver for SDL1
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This is the platform-specific driver for SDL 1.x.
 * ============================================================================
 */

#include "config.h"
#include "sdl_jzintv.h"
#include "event/event_tbl.h"
#include "event/event_plat.h"
#include "event/event_sdl_pvt.h"

/* ======================================================================== */
/*  SDK_KEYMAP   -- Maps SDLK values to EVENT values.                       */
/* ======================================================================== */
static const event_num_t sdl_keymap[] =
{
    [SDLK_UNKNOWN]              = EVENT_UNKNOWN,
    [SDLK_BACKSPACE]            = EVENT_BACKSPACE,
    [SDLK_TAB]                  = EVENT_TAB,
    [SDLK_CLEAR]                = EVENT_CLEAR,
    [SDLK_RETURN]               = EVENT_RETURN,
    [SDLK_PAUSE]                = EVENT_PAUSE,
    [SDLK_ESCAPE]               = EVENT_ESCAPE,
    [SDLK_SPACE]                = EVENT_SPACE,
    [SDLK_EXCLAIM]              = EVENT_EXCLAIM,
    [SDLK_QUOTEDBL]             = EVENT_QUOTEDBL,
    [SDLK_HASH]                 = EVENT_HASH,
    [SDLK_DOLLAR]               = EVENT_DOLLAR,
    [SDLK_AMPERSAND]            = EVENT_AMPERSAND,
    [SDLK_QUOTE]                = EVENT_QUOTE,
    [SDLK_LEFTPAREN]            = EVENT_LEFTPAREN,
    [SDLK_RIGHTPAREN]           = EVENT_RIGHTPAREN,
    [SDLK_ASTERISK]             = EVENT_ASTERISK,
    [SDLK_PLUS]                 = EVENT_PLUS,
    [SDLK_COMMA]                = EVENT_COMMA,
    [SDLK_MINUS]                = EVENT_MINUS,
    [SDLK_PERIOD]               = EVENT_PERIOD,
    [SDLK_SLASH]                = EVENT_SLASH,
    [SDLK_0]                    = EVENT_0,
    [SDLK_1]                    = EVENT_1,
    [SDLK_2]                    = EVENT_2,
    [SDLK_3]                    = EVENT_3,
    [SDLK_4]                    = EVENT_4,
    [SDLK_5]                    = EVENT_5,
    [SDLK_6]                    = EVENT_6,
    [SDLK_7]                    = EVENT_7,
    [SDLK_8]                    = EVENT_8,
    [SDLK_9]                    = EVENT_9,
    [SDLK_COLON]                = EVENT_COLON,
    [SDLK_SEMICOLON]            = EVENT_SEMICOLON,
    [SDLK_LESS]                 = EVENT_LESS,
    [SDLK_EQUALS]               = EVENT_EQUALS,
    [SDLK_GREATER]              = EVENT_GREATER,
    [SDLK_QUESTION]             = EVENT_QUESTION,
    [SDLK_AT]                   = EVENT_AT,
    [SDLK_LEFTBRACKET]          = EVENT_LEFTBRACKET,
    [SDLK_BACKSLASH]            = EVENT_BACKSLASH,
    [SDLK_RIGHTBRACKET]         = EVENT_RIGHTBRACKET,
    [SDLK_CARET]                = EVENT_CARET,
    [SDLK_UNDERSCORE]           = EVENT_UNDERSCORE,
    [SDLK_BACKQUOTE]            = EVENT_BACKQUOTE,
    [SDLK_a]                    = EVENT_A,
    [SDLK_b]                    = EVENT_B,
    [SDLK_c]                    = EVENT_C,
    [SDLK_d]                    = EVENT_D,
    [SDLK_e]                    = EVENT_E,
    [SDLK_f]                    = EVENT_F,
    [SDLK_g]                    = EVENT_G,
    [SDLK_h]                    = EVENT_H,
    [SDLK_i]                    = EVENT_I,
    [SDLK_j]                    = EVENT_J,
    [SDLK_k]                    = EVENT_K,
    [SDLK_l]                    = EVENT_L,
    [SDLK_m]                    = EVENT_M,
    [SDLK_n]                    = EVENT_N,
    [SDLK_o]                    = EVENT_O,
    [SDLK_p]                    = EVENT_P,
    [SDLK_q]                    = EVENT_Q,
    [SDLK_r]                    = EVENT_R,
    [SDLK_s]                    = EVENT_S,
    [SDLK_t]                    = EVENT_T,
    [SDLK_u]                    = EVENT_U,
    [SDLK_v]                    = EVENT_V,
    [SDLK_w]                    = EVENT_W,
    [SDLK_x]                    = EVENT_X,
    [SDLK_y]                    = EVENT_Y,
    [SDLK_z]                    = EVENT_Z,
    [SDLK_DELETE]               = EVENT_DELETE,
#if !defined(__EMSCRIPTEN__)
    [SDLK_WORLD_0]              = EVENT_SCANCODE_000,
    [SDLK_WORLD_1]              = EVENT_SCANCODE_001,
    [SDLK_WORLD_2]              = EVENT_SCANCODE_002,
    [SDLK_WORLD_3]              = EVENT_SCANCODE_003,
    [SDLK_WORLD_4]              = EVENT_SCANCODE_004,
    [SDLK_WORLD_5]              = EVENT_SCANCODE_005,
    [SDLK_WORLD_6]              = EVENT_SCANCODE_006,
    [SDLK_WORLD_7]              = EVENT_SCANCODE_007,
    [SDLK_WORLD_8]              = EVENT_SCANCODE_008,
    [SDLK_WORLD_9]              = EVENT_SCANCODE_009,
    [SDLK_WORLD_10]             = EVENT_SCANCODE_00A,
    [SDLK_WORLD_11]             = EVENT_SCANCODE_00B,
    [SDLK_WORLD_12]             = EVENT_SCANCODE_00C,
    [SDLK_WORLD_13]             = EVENT_SCANCODE_00D,
    [SDLK_WORLD_14]             = EVENT_SCANCODE_00E,
    [SDLK_WORLD_15]             = EVENT_SCANCODE_00F,
    [SDLK_WORLD_16]             = EVENT_SCANCODE_010,
    [SDLK_WORLD_17]             = EVENT_SCANCODE_011,
    [SDLK_WORLD_18]             = EVENT_SCANCODE_012,
    [SDLK_WORLD_19]             = EVENT_SCANCODE_013,
    [SDLK_WORLD_20]             = EVENT_SCANCODE_014,
    [SDLK_WORLD_21]             = EVENT_SCANCODE_015,
    [SDLK_WORLD_22]             = EVENT_SCANCODE_016,
    [SDLK_WORLD_23]             = EVENT_SCANCODE_017,
    [SDLK_WORLD_24]             = EVENT_SCANCODE_018,
    [SDLK_WORLD_25]             = EVENT_SCANCODE_019,
    [SDLK_WORLD_26]             = EVENT_SCANCODE_01A,
    [SDLK_WORLD_27]             = EVENT_SCANCODE_01B,
    [SDLK_WORLD_28]             = EVENT_SCANCODE_01C,
    [SDLK_WORLD_29]             = EVENT_SCANCODE_01D,
    [SDLK_WORLD_30]             = EVENT_SCANCODE_01E,
    [SDLK_WORLD_31]             = EVENT_SCANCODE_01F,
    [SDLK_WORLD_32]             = EVENT_SCANCODE_020,
    [SDLK_WORLD_33]             = EVENT_SCANCODE_021,
    [SDLK_WORLD_34]             = EVENT_SCANCODE_022,
    [SDLK_WORLD_35]             = EVENT_SCANCODE_023,
    [SDLK_WORLD_36]             = EVENT_SCANCODE_024,
    [SDLK_WORLD_37]             = EVENT_SCANCODE_025,
    [SDLK_WORLD_38]             = EVENT_SCANCODE_026,
    [SDLK_WORLD_39]             = EVENT_SCANCODE_027,
    [SDLK_WORLD_40]             = EVENT_SCANCODE_028,
    [SDLK_WORLD_41]             = EVENT_SCANCODE_029,
    [SDLK_WORLD_42]             = EVENT_SCANCODE_02A,
    [SDLK_WORLD_43]             = EVENT_SCANCODE_02B,
    [SDLK_WORLD_44]             = EVENT_SCANCODE_02C,
    [SDLK_WORLD_45]             = EVENT_SCANCODE_02D,
    [SDLK_WORLD_46]             = EVENT_SCANCODE_02E,
    [SDLK_WORLD_47]             = EVENT_SCANCODE_02F,
    [SDLK_WORLD_48]             = EVENT_SCANCODE_030,
    [SDLK_WORLD_49]             = EVENT_SCANCODE_031,
    [SDLK_WORLD_50]             = EVENT_SCANCODE_032,
    [SDLK_WORLD_51]             = EVENT_SCANCODE_033,
    [SDLK_WORLD_52]             = EVENT_SCANCODE_034,
    [SDLK_WORLD_53]             = EVENT_SCANCODE_035,
    [SDLK_WORLD_54]             = EVENT_SCANCODE_036,
    [SDLK_WORLD_55]             = EVENT_SCANCODE_037,
    [SDLK_WORLD_56]             = EVENT_SCANCODE_038,
    [SDLK_WORLD_57]             = EVENT_SCANCODE_039,
    [SDLK_WORLD_58]             = EVENT_SCANCODE_03A,
    [SDLK_WORLD_59]             = EVENT_SCANCODE_03B,
    [SDLK_WORLD_60]             = EVENT_SCANCODE_03C,
    [SDLK_WORLD_61]             = EVENT_SCANCODE_03D,
    [SDLK_WORLD_62]             = EVENT_SCANCODE_03E,
    [SDLK_WORLD_63]             = EVENT_SCANCODE_03F,
    [SDLK_WORLD_64]             = EVENT_SCANCODE_040,
    [SDLK_WORLD_65]             = EVENT_SCANCODE_041,
    [SDLK_WORLD_66]             = EVENT_SCANCODE_042,
    [SDLK_WORLD_67]             = EVENT_SCANCODE_043,
    [SDLK_WORLD_68]             = EVENT_SCANCODE_044,
    [SDLK_WORLD_69]             = EVENT_SCANCODE_045,
    [SDLK_WORLD_70]             = EVENT_SCANCODE_046,
    [SDLK_WORLD_71]             = EVENT_SCANCODE_047,
    [SDLK_WORLD_72]             = EVENT_SCANCODE_048,
    [SDLK_WORLD_73]             = EVENT_SCANCODE_049,
    [SDLK_WORLD_74]             = EVENT_SCANCODE_04A,
    [SDLK_WORLD_75]             = EVENT_SCANCODE_04B,
    [SDLK_WORLD_76]             = EVENT_SCANCODE_04C,
    [SDLK_WORLD_77]             = EVENT_SCANCODE_04D,
    [SDLK_WORLD_78]             = EVENT_SCANCODE_04E,
    [SDLK_WORLD_79]             = EVENT_SCANCODE_04F,
    [SDLK_WORLD_80]             = EVENT_SCANCODE_050,
    [SDLK_WORLD_81]             = EVENT_SCANCODE_051,
    [SDLK_WORLD_82]             = EVENT_SCANCODE_052,
    [SDLK_WORLD_83]             = EVENT_SCANCODE_053,
    [SDLK_WORLD_84]             = EVENT_SCANCODE_054,
    [SDLK_WORLD_85]             = EVENT_SCANCODE_055,
    [SDLK_WORLD_86]             = EVENT_SCANCODE_056,
    [SDLK_WORLD_87]             = EVENT_SCANCODE_057,
    [SDLK_WORLD_88]             = EVENT_SCANCODE_058,
    [SDLK_WORLD_89]             = EVENT_SCANCODE_059,
    [SDLK_WORLD_90]             = EVENT_SCANCODE_05A,
    [SDLK_WORLD_91]             = EVENT_SCANCODE_05B,
    [SDLK_WORLD_92]             = EVENT_SCANCODE_05C,
    [SDLK_WORLD_93]             = EVENT_SCANCODE_05D,
    [SDLK_WORLD_94]             = EVENT_SCANCODE_05E,
    [SDLK_WORLD_95]             = EVENT_SCANCODE_05F,
#endif
    [SDLK_KP0]                  = EVENT_KP_0,
    [SDLK_KP1]                  = EVENT_KP_1,
    [SDLK_KP2]                  = EVENT_KP_2,
    [SDLK_KP3]                  = EVENT_KP_3,
    [SDLK_KP4]                  = EVENT_KP_4,
    [SDLK_KP5]                  = EVENT_KP_5,
    [SDLK_KP6]                  = EVENT_KP_6,
    [SDLK_KP7]                  = EVENT_KP_7,
    [SDLK_KP8]                  = EVENT_KP_8,
    [SDLK_KP9]                  = EVENT_KP_9,
    [SDLK_KP_PERIOD]            = EVENT_KP_PERIOD,
    [SDLK_KP_DIVIDE]            = EVENT_KP_DIVIDE,
    [SDLK_KP_MULTIPLY]          = EVENT_KP_MULTIPLY,
    [SDLK_KP_MINUS]             = EVENT_KP_MINUS,
    [SDLK_KP_PLUS]              = EVENT_KP_PLUS,
    [SDLK_KP_ENTER]             = EVENT_KP_ENTER,
    [SDLK_KP_EQUALS]            = EVENT_KP_EQUALS,
    [SDLK_UP]                   = EVENT_UP,
    [SDLK_DOWN]                 = EVENT_DOWN,
    [SDLK_RIGHT]                = EVENT_RIGHT,
    [SDLK_LEFT]                 = EVENT_LEFT,
    [SDLK_INSERT]               = EVENT_INSERT,
    [SDLK_HOME]                 = EVENT_HOME,
    [SDLK_END]                  = EVENT_END,
    [SDLK_PAGEUP]               = EVENT_PAGEUP,
    [SDLK_PAGEDOWN]             = EVENT_PAGEDOWN,
    [SDLK_F1]                   = EVENT_F1,
    [SDLK_F2]                   = EVENT_F2,
    [SDLK_F3]                   = EVENT_F3,
    [SDLK_F4]                   = EVENT_F4,
    [SDLK_F5]                   = EVENT_F5,
    [SDLK_F6]                   = EVENT_F6,
    [SDLK_F7]                   = EVENT_F7,
    [SDLK_F8]                   = EVENT_F8,
    [SDLK_F9]                   = EVENT_F9,
    [SDLK_F10]                  = EVENT_F10,
    [SDLK_F11]                  = EVENT_F11,
    [SDLK_F12]                  = EVENT_F12,
    [SDLK_F13]                  = EVENT_F13,
    [SDLK_F14]                  = EVENT_F14,
    [SDLK_F15]                  = EVENT_F15,
    [SDLK_NUMLOCK]              = EVENT_NUMLOCK,
    [SDLK_CAPSLOCK]             = EVENT_CAPSLOCK,
    [SDLK_SCROLLOCK]            = EVENT_SCROLLOCK,
    [SDLK_RSHIFT]               = EVENT_RSHIFT,
    [SDLK_LSHIFT]               = EVENT_LSHIFT,
    [SDLK_RCTRL]                = EVENT_RCTRL,
    [SDLK_LCTRL]                = EVENT_LCTRL,
    [SDLK_RALT]                 = EVENT_RALT,
    [SDLK_LALT]                 = EVENT_LALT,
    [SDLK_RMETA]                = EVENT_RMETA,
    [SDLK_LMETA]                = EVENT_LMETA,
#if !defined(__EMSCRIPTEN__)
    [SDLK_LSUPER]               = EVENT_LSUPER,
    [SDLK_RSUPER]               = EVENT_RSUPER,
    [SDLK_EURO]                 = EVENT_EURO,
#endif
    [SDLK_MODE]                 = EVENT_MODE,
    [SDLK_COMPOSE]              = EVENT_COMPOSE,
    [SDLK_HELP]                 = EVENT_HELP,
    [SDLK_PRINT]                = EVENT_PRINT,
    [SDLK_SYSREQ]               = EVENT_SYSREQ,
    [SDLK_BREAK]                = EVENT_BREAK,
    [SDLK_MENU]                 = EVENT_MENU,
    [SDLK_POWER]                = EVENT_POWER,
    [SDLK_UNDO]                 = EVENT_UNDO,
};

/* ======================================================================== */
/*  EVENT_SDL_TRANSLATE_KEY  -- Translate an SDLKey to event_num_t.         */
/* ======================================================================== */
event_num_t event_sdl_translate_key(const SDL_Event *const event)
{
    const SDLKey key = event->key.keysym.sym;

    if (key >= SDLK_LAST)
        return EVENT_UNKNOWN;

    return sdl_keymap[key];
}
/* ======================================================================== */
/*  EVENT_SDL_UNHANDLED_EVENT -- Hook for SDL-version specific events.      */
/*  Returns true if the event ended up queuing a COMBO, false otherwise.    */
/* ======================================================================== */
bool event_sdl_unhandled_event(evt_pvt_t *const evt_pvt,
                               const SDL_Event *const event)
{
    switch (event->type)
    {
        /* ---------------------------------------------------------------- */
        /*  Activation events:  Only look at whether we're iconified or     */
        /*  not, and convert it to an up/down event on EVENT_HIDE.          */
        /* ---------------------------------------------------------------- */
        case SDL_ACTIVEEVENT:
        {
            if (event->active.state & SDL_APPACTIVE)
                event_enqueue(evt_pvt, event->active.gain ? EV_UP : EV_DOWN,
                            EVENT_HIDE);
            break;
        }
        default:
            break;
    }
    return false;
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
/*                 Copyright (c) 2020-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
