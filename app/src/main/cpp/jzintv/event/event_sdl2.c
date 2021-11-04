/*
 * ============================================================================
 *  Title:    Event Handling Driver for SDL2
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

#define REMAP(x) (((x) & 0x1FF) | ((x) & (1ul << 30) ? 0x200 : 0))

/* ======================================================================== */
/*  SDK_KEYMAP   -- Maps SDLK values to EVENT values.                       */
/* ======================================================================== */
static const event_num_t sdl_keymap[] =
{
    [REMAP(SDLK_UNKNOWN)]               = EVENT_UNKNOWN,
    [REMAP(SDLK_BACKSPACE)]             = EVENT_BACKSPACE,
    [REMAP(SDLK_TAB)]                   = EVENT_TAB,
    [REMAP(SDLK_CLEAR)]                 = EVENT_CLEAR,
    [REMAP(SDLK_RETURN)]                = EVENT_RETURN,
    [REMAP(SDLK_PAUSE)]                 = EVENT_PAUSE,
    [REMAP(SDLK_ESCAPE)]                = EVENT_ESCAPE,
    [REMAP(SDLK_SPACE)]                 = EVENT_SPACE,
    [REMAP(SDLK_EXCLAIM)]               = EVENT_EXCLAIM,
    [REMAP(SDLK_QUOTEDBL)]              = EVENT_QUOTEDBL,
    [REMAP(SDLK_HASH)]                  = EVENT_HASH,
    [REMAP(SDLK_DOLLAR)]                = EVENT_DOLLAR,
    [REMAP(SDLK_AMPERSAND)]             = EVENT_AMPERSAND,
    [REMAP(SDLK_QUOTE)]                 = EVENT_QUOTE,
    [REMAP(SDLK_LEFTPAREN)]             = EVENT_LEFTPAREN,
    [REMAP(SDLK_RIGHTPAREN)]            = EVENT_RIGHTPAREN,
    [REMAP(SDLK_ASTERISK)]              = EVENT_ASTERISK,
    [REMAP(SDLK_PLUS)]                  = EVENT_PLUS,
    [REMAP(SDLK_COMMA)]                 = EVENT_COMMA,
    [REMAP(SDLK_MINUS)]                 = EVENT_MINUS,
    [REMAP(SDLK_PERIOD)]                = EVENT_PERIOD,
    [REMAP(SDLK_SLASH)]                 = EVENT_SLASH,
    [REMAP(SDLK_0)]                     = EVENT_0,
    [REMAP(SDLK_1)]                     = EVENT_1,
    [REMAP(SDLK_2)]                     = EVENT_2,
    [REMAP(SDLK_3)]                     = EVENT_3,
    [REMAP(SDLK_4)]                     = EVENT_4,
    [REMAP(SDLK_5)]                     = EVENT_5,
    [REMAP(SDLK_6)]                     = EVENT_6,
    [REMAP(SDLK_7)]                     = EVENT_7,
    [REMAP(SDLK_8)]                     = EVENT_8,
    [REMAP(SDLK_9)]                     = EVENT_9,
    [REMAP(SDLK_COLON)]                 = EVENT_COLON,
    [REMAP(SDLK_SEMICOLON)]             = EVENT_SEMICOLON,
    [REMAP(SDLK_LESS)]                  = EVENT_LESS,
    [REMAP(SDLK_EQUALS)]                = EVENT_EQUALS,
    [REMAP(SDLK_GREATER)]               = EVENT_GREATER,
    [REMAP(SDLK_QUESTION)]              = EVENT_QUESTION,
    [REMAP(SDLK_AT)]                    = EVENT_AT,
    [REMAP(SDLK_LEFTBRACKET)]           = EVENT_LEFTBRACKET,
    [REMAP(SDLK_BACKSLASH)]             = EVENT_BACKSLASH,
    [REMAP(SDLK_RIGHTBRACKET)]          = EVENT_RIGHTBRACKET,
    [REMAP(SDLK_CARET)]                 = EVENT_CARET,
    [REMAP(SDLK_UNDERSCORE)]            = EVENT_UNDERSCORE,
    [REMAP(SDLK_BACKQUOTE)]             = EVENT_BACKQUOTE,
    [REMAP(SDLK_a)]                     = EVENT_A,
    [REMAP(SDLK_b)]                     = EVENT_B,
    [REMAP(SDLK_c)]                     = EVENT_C,
    [REMAP(SDLK_d)]                     = EVENT_D,
    [REMAP(SDLK_e)]                     = EVENT_E,
    [REMAP(SDLK_f)]                     = EVENT_F,
    [REMAP(SDLK_g)]                     = EVENT_G,
    [REMAP(SDLK_h)]                     = EVENT_H,
    [REMAP(SDLK_i)]                     = EVENT_I,
    [REMAP(SDLK_j)]                     = EVENT_J,
    [REMAP(SDLK_k)]                     = EVENT_K,
    [REMAP(SDLK_l)]                     = EVENT_L,
    [REMAP(SDLK_m)]                     = EVENT_M,
    [REMAP(SDLK_n)]                     = EVENT_N,
    [REMAP(SDLK_o)]                     = EVENT_O,
    [REMAP(SDLK_p)]                     = EVENT_P,
    [REMAP(SDLK_q)]                     = EVENT_Q,
    [REMAP(SDLK_r)]                     = EVENT_R,
    [REMAP(SDLK_s)]                     = EVENT_S,
    [REMAP(SDLK_t)]                     = EVENT_T,
    [REMAP(SDLK_u)]                     = EVENT_U,
    [REMAP(SDLK_v)]                     = EVENT_V,
    [REMAP(SDLK_w)]                     = EVENT_W,
    [REMAP(SDLK_x)]                     = EVENT_X,
    [REMAP(SDLK_y)]                     = EVENT_Y,
    [REMAP(SDLK_z)]                     = EVENT_Z,
    [REMAP(SDLK_DELETE)]                = EVENT_DELETE,
    [REMAP(SDLK_KP_0)]                  = EVENT_KP_0,
    [REMAP(SDLK_KP_1)]                  = EVENT_KP_1,
    [REMAP(SDLK_KP_2)]                  = EVENT_KP_2,
    [REMAP(SDLK_KP_3)]                  = EVENT_KP_3,
    [REMAP(SDLK_KP_4)]                  = EVENT_KP_4,
    [REMAP(SDLK_KP_5)]                  = EVENT_KP_5,
    [REMAP(SDLK_KP_6)]                  = EVENT_KP_6,
    [REMAP(SDLK_KP_7)]                  = EVENT_KP_7,
    [REMAP(SDLK_KP_8)]                  = EVENT_KP_8,
    [REMAP(SDLK_KP_9)]                  = EVENT_KP_9,
    [REMAP(SDLK_KP_PERIOD)]             = EVENT_KP_PERIOD,
    [REMAP(SDLK_KP_DIVIDE)]             = EVENT_KP_DIVIDE,
    [REMAP(SDLK_KP_MULTIPLY)]           = EVENT_KP_MULTIPLY,
    [REMAP(SDLK_KP_MINUS)]              = EVENT_KP_MINUS,
    [REMAP(SDLK_KP_PLUS)]               = EVENT_KP_PLUS,
    [REMAP(SDLK_KP_ENTER)]              = EVENT_KP_ENTER,
    [REMAP(SDLK_KP_EQUALS)]             = EVENT_KP_EQUALS,
    [REMAP(SDLK_UP)]                    = EVENT_UP,
    [REMAP(SDLK_DOWN)]                  = EVENT_DOWN,
    [REMAP(SDLK_RIGHT)]                 = EVENT_RIGHT,
    [REMAP(SDLK_LEFT)]                  = EVENT_LEFT,
    [REMAP(SDLK_INSERT)]                = EVENT_INSERT,
    [REMAP(SDLK_HOME)]                  = EVENT_HOME,
    [REMAP(SDLK_END)]                   = EVENT_END,
    [REMAP(SDLK_PAGEUP)]                = EVENT_PAGEUP,
    [REMAP(SDLK_PAGEDOWN)]              = EVENT_PAGEDOWN,
    [REMAP(SDLK_F1)]                    = EVENT_F1,
    [REMAP(SDLK_F2)]                    = EVENT_F2,
    [REMAP(SDLK_F3)]                    = EVENT_F3,
    [REMAP(SDLK_F4)]                    = EVENT_F4,
    [REMAP(SDLK_F5)]                    = EVENT_F5,
    [REMAP(SDLK_F6)]                    = EVENT_F6,
    [REMAP(SDLK_F7)]                    = EVENT_F7,
    [REMAP(SDLK_F8)]                    = EVENT_F8,
    [REMAP(SDLK_F9)]                    = EVENT_F9,
    [REMAP(SDLK_F10)]                   = EVENT_F10,
    [REMAP(SDLK_F11)]                   = EVENT_F11,
    [REMAP(SDLK_F12)]                   = EVENT_F12,
    [REMAP(SDLK_F13)]                   = EVENT_F13,
    [REMAP(SDLK_F14)]                   = EVENT_F14,
    [REMAP(SDLK_F15)]                   = EVENT_F15,
    [REMAP(SDLK_NUMLOCKCLEAR)]          = EVENT_NUMLOCK,
    [REMAP(SDLK_CAPSLOCK)]              = EVENT_CAPSLOCK,
    [REMAP(SDLK_SCROLLLOCK)]            = EVENT_SCROLLOCK,
    [REMAP(SDLK_RSHIFT)]                = EVENT_RSHIFT,
    [REMAP(SDLK_LSHIFT)]                = EVENT_LSHIFT,
    [REMAP(SDLK_RCTRL)]                 = EVENT_RCTRL,
    [REMAP(SDLK_LCTRL)]                 = EVENT_LCTRL,
    [REMAP(SDLK_RALT)]                  = EVENT_RALT,
    [REMAP(SDLK_LALT)]                  = EVENT_LALT,
    [REMAP(SDLK_MODE)]                  = EVENT_MODE,
    [REMAP(SDLK_HELP)]                  = EVENT_HELP,
    [REMAP(SDLK_PRINTSCREEN)]           = EVENT_PRINT,
    [REMAP(SDLK_SYSREQ)]                = EVENT_SYSREQ,
    [REMAP(SDLK_MENU)]                  = EVENT_MENU,
    [REMAP(SDLK_POWER)]                 = EVENT_POWER,
    [REMAP(SDLK_UNDO)]                  = EVENT_UNDO,

    /* New for SDL2 */
    [REMAP(SDLK_LGUI)]                  = EVENT_LGUI,
    [REMAP(SDLK_RGUI)]                  = EVENT_RGUI,
    [REMAP(SDLK_F16)]                   = EVENT_F16,
    [REMAP(SDLK_F17)]                   = EVENT_F17,
    [REMAP(SDLK_F18)]                   = EVENT_F18,
    [REMAP(SDLK_F19)]                   = EVENT_F19,
    [REMAP(SDLK_F20)]                   = EVENT_F20,
    [REMAP(SDLK_F21)]                   = EVENT_F21,
    [REMAP(SDLK_F22)]                   = EVENT_F22,
    [REMAP(SDLK_F23)]                   = EVENT_F23,
    [REMAP(SDLK_F24)]                   = EVENT_F24,
    [REMAP(SDLK_SELECT)]                = EVENT_SELECT,
    [REMAP(SDLK_STOP)]                  = EVENT_STOP,
    [REMAP(SDLK_AGAIN)]                 = EVENT_AGAIN,
    [REMAP(SDLK_CUT)]                   = EVENT_CUT,
    [REMAP(SDLK_COPY)]                  = EVENT_COPY,
    [REMAP(SDLK_PASTE)]                 = EVENT_PASTE,
    [REMAP(SDLK_FIND)]                  = EVENT_FIND,
    [REMAP(SDLK_MUTE)]                  = EVENT_MUTE,
    [REMAP(SDLK_VOLUMEUP)]              = EVENT_VOLUMEUP,
    [REMAP(SDLK_VOLUMEDOWN)]            = EVENT_VOLUMEDOWN,
    [REMAP(SDLK_KP_COMMA)]              = EVENT_KP_COMMA,
    [REMAP(SDLK_KP_000)]                = EVENT_KP_000,
    [REMAP(SDLK_KP_00)]                 = EVENT_KP_00,
    [REMAP(SDLK_KP_EQUALSAS400)]        = EVENT_KP_EQUALSAS400,
    [REMAP(SDLK_CANCEL)]                = EVENT_CANCEL,
    [REMAP(SDLK_PRIOR)]                 = EVENT_PRIOR,
    [REMAP(SDLK_SEPARATOR)]             = EVENT_SEPARATOR,
    [REMAP(SDLK_OUT)]                   = EVENT_OUT,
    [REMAP(SDLK_OPER)]                  = EVENT_OPER,
    [REMAP(SDLK_CLEARAGAIN)]            = EVENT_CLEARAGAIN,
    [REMAP(SDLK_CRSEL)]                 = EVENT_CRSEL,
    [REMAP(SDLK_EXSEL)]                 = EVENT_EXSEL,
    [REMAP(SDLK_THOUSANDSSEPARATOR)]    = EVENT_THOUSANDSSEPARATOR,
    [REMAP(SDLK_DECIMALSEPARATOR)]      = EVENT_DECIMALSEPARATOR,
    [REMAP(SDLK_CURRENCYUNIT)]          = EVENT_CURRENCYUNIT,
    [REMAP(SDLK_CURRENCYSUBUNIT)]       = EVENT_CURRENCYSUBUNIT,
    [REMAP(SDLK_KP_LEFTPAREN)]          = EVENT_KP_LEFTPAREN,
    [REMAP(SDLK_KP_RIGHTPAREN)]         = EVENT_KP_RIGHTPAREN,
    [REMAP(SDLK_KP_LEFTBRACE)]          = EVENT_KP_LEFTBRACE,
    [REMAP(SDLK_KP_RIGHTBRACE)]         = EVENT_KP_RIGHTBRACE,
    [REMAP(SDLK_KP_TAB)]                = EVENT_KP_TAB,
    [REMAP(SDLK_KP_BACKSPACE)]          = EVENT_KP_BACKSPACE,
    [REMAP(SDLK_KP_A)]                  = EVENT_KP_A,
    [REMAP(SDLK_KP_B)]                  = EVENT_KP_B,
    [REMAP(SDLK_KP_C)]                  = EVENT_KP_C,
    [REMAP(SDLK_KP_D)]                  = EVENT_KP_D,
    [REMAP(SDLK_KP_E)]                  = EVENT_KP_E,
    [REMAP(SDLK_KP_F)]                  = EVENT_KP_F,
    [REMAP(SDLK_KP_XOR)]                = EVENT_KP_XOR,
    [REMAP(SDLK_KP_POWER)]              = EVENT_KP_POWER,
    [REMAP(SDLK_KP_PERCENT)]            = EVENT_KP_PERCENT,
    [REMAP(SDLK_KP_LESS)]               = EVENT_KP_LESS,
    [REMAP(SDLK_KP_GREATER)]            = EVENT_KP_GREATER,
    [REMAP(SDLK_KP_AMPERSAND)]          = EVENT_KP_AMPERSAND,
    [REMAP(SDLK_KP_DBLAMPERSAND)]       = EVENT_KP_DBLAMPERSAND,
    [REMAP(SDLK_KP_VERTICALBAR)]        = EVENT_KP_VERTICALBAR,
    [REMAP(SDLK_KP_DBLVERTICALBAR)]     = EVENT_KP_DBLVERTICALBAR,
    [REMAP(SDLK_KP_COLON)]              = EVENT_KP_COLON,
    [REMAP(SDLK_KP_HASH)]               = EVENT_KP_HASH,
    [REMAP(SDLK_KP_SPACE)]              = EVENT_KP_SPACE,
    [REMAP(SDLK_KP_AT)]                 = EVENT_KP_AT,
    [REMAP(SDLK_KP_EXCLAM)]             = EVENT_KP_EXCLAIM,
    [REMAP(SDLK_KP_MEMSTORE)]           = EVENT_KP_MEMSTORE,
    [REMAP(SDLK_KP_MEMRECALL)]          = EVENT_KP_MEMRECALL,
    [REMAP(SDLK_KP_MEMCLEAR)]           = EVENT_KP_MEMCLEAR,
    [REMAP(SDLK_KP_MEMADD)]             = EVENT_KP_MEMADD,
    [REMAP(SDLK_KP_MEMSUBTRACT)]        = EVENT_KP_MEMSUBTRACT,
    [REMAP(SDLK_KP_MEMMULTIPLY)]        = EVENT_KP_MEMMULTIPLY,
    [REMAP(SDLK_KP_MEMDIVIDE)]          = EVENT_KP_MEMDIVIDE,
    [REMAP(SDLK_KP_PLUSMINUS)]          = EVENT_KP_PLUSMINUS,
    [REMAP(SDLK_KP_CLEAR)]              = EVENT_KP_CLEAR,
    [REMAP(SDLK_KP_CLEARENTRY)]         = EVENT_KP_CLEARENTRY,
    [REMAP(SDLK_KP_BINARY)]             = EVENT_KP_BINARY,
    [REMAP(SDLK_KP_OCTAL)]              = EVENT_KP_OCTAL,
    [REMAP(SDLK_KP_DECIMAL)]            = EVENT_KP_DECIMAL,
    [REMAP(SDLK_KP_HEXADECIMAL)]        = EVENT_KP_HEXADECIMAL,
    [REMAP(SDLK_AUDIONEXT)]             = EVENT_AUDIONEXT,
    [REMAP(SDLK_AUDIOPREV)]             = EVENT_AUDIOPREV,
    [REMAP(SDLK_AUDIOSTOP)]             = EVENT_AUDIOSTOP,
    [REMAP(SDLK_AUDIOPLAY)]             = EVENT_AUDIOPLAY,
    [REMAP(SDLK_AUDIOMUTE)]             = EVENT_AUDIOMUTE,
    [REMAP(SDLK_MEDIASELECT)]           = EVENT_MEDIASELECT,
    [REMAP(SDLK_WWW)]                   = EVENT_WWW,
    [REMAP(SDLK_MAIL)]                  = EVENT_MAIL,
    [REMAP(SDLK_CALCULATOR)]            = EVENT_CALCULATOR,
    [REMAP(SDLK_COMPUTER)]              = EVENT_COMPUTER,
    [REMAP(SDLK_AC_SEARCH)]             = EVENT_AC_SEARCH,
    [REMAP(SDLK_AC_HOME)]               = EVENT_AC_HOME,
    [REMAP(SDLK_AC_BACK)]               = EVENT_AC_BACK,
    [REMAP(SDLK_AC_FORWARD)]            = EVENT_AC_FORWARD,
    [REMAP(SDLK_AC_STOP)]               = EVENT_AC_STOP,
    [REMAP(SDLK_AC_REFRESH)]            = EVENT_AC_REFRESH,
    [REMAP(SDLK_AC_BOOKMARKS)]          = EVENT_AC_BOOKMARKS,
    [REMAP(SDLK_BRIGHTNESSDOWN)]        = EVENT_BRIGHTNESSDOWN,
    [REMAP(SDLK_BRIGHTNESSUP)]          = EVENT_BRIGHTNESSUP,
    [REMAP(SDLK_DISPLAYSWITCH)]         = EVENT_DISPLAYSWITCH,
    [REMAP(SDLK_KBDILLUMTOGGLE)]        = EVENT_KBDILLUMTOGGLE,
    [REMAP(SDLK_KBDILLUMDOWN)]          = EVENT_KBDILLUMDOWN,
    [REMAP(SDLK_KBDILLUMUP)]            = EVENT_KBDILLUMUP,
    [REMAP(SDLK_EJECT)]                 = EVENT_EJECT,
    [REMAP(SDLK_SLEEP)]                 = EVENT_SLEEP,
};

static const int sdl_keymap_size = sizeof(sdl_keymap) / sizeof(sdl_keymap[0]);

/* ======================================================================== */
/*  EVENT_SDL_TRANSLATE_KEY  -- Translate an SDLKey to event_num_t.         */
/* ======================================================================== */
event_num_t event_sdl_translate_key(const SDL_Event *const event)
{
    const int remapped_key = REMAP(event->key.keysym.sym);
    const event_num_t primary =
        remapped_key < sdl_keymap_size ? sdl_keymap[remapped_key]
                                       : EVENT_UNKNOWN;
    if (primary != EVENT_UNKNOWN)
        return primary;

    if (event->key.keysym.scancode < 0x200)
        return EVENT_NUM_OFS(EVENT_SCANCODE_000, event->key.keysym.scancode);

    return EVENT_UNKNOWN;
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
        /*  Window events:  Only look at whether we're iconified or not,    */
        /*  and convert it to an up/down event on EVENT_HIDE.               */
        /* ---------------------------------------------------------------- */
        case SDL_WINDOWEVENT:
        {
            switch (event->window.event)
            {
                case SDL_WINDOWEVENT_MINIMIZED:
                    event_enqueue(evt_pvt, EV_DOWN, EVENT_HIDE);
                    break;

                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                case SDL_WINDOWEVENT_SHOWN:
                    event_enqueue(evt_pvt, EV_UP, EVENT_HIDE);
                    break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    event_enqueue(evt_pvt, EV_DOWN, EVENT_FOCUS_GAINED);
                    event_enqueue(evt_pvt, EV_UP,   EVENT_FOCUS_GAINED);
                    break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                    event_enqueue(evt_pvt, EV_DOWN, EVENT_FOCUS_LOST);
                    event_enqueue(evt_pvt, EV_UP,   EVENT_FOCUS_LOST);
                    break;

                default:
                    break;
            }
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
