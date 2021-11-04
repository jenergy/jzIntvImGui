/*
 * ============================================================================
 *  Title:
 *  Author:   J. Zbiciak
 * ============================================================================
 *  GFX_INIT         -- Initializes a gfx_t object and gfx subsystem
 *  GFX_TICK         -- Services a gfx_t tick
 *  GFX_VID_ENABLE   -- Alert gfx that video has been enabled or blanked
 * ============================================================================
 *  GFX_PVT_T        -- Private internal state to gfx_t structure.
 *  GFX_T            -- Generic graphics object.  The graphics object is a
 *                      periph also so that screen updates, etc. can be
 *                      scheduled via the global tick mechanism.
 * ============================================================================
 *  The graphics subsystem provides an abstraction layer between the
 *  emulator and the graphics library being used.  Theoretically, this
 *  should allow easy porting to other graphics libraries.
 * ============================================================================
 */

#ifndef GFX_H_
#define GFX_H_

#include "gfx/palette.h"    // for palette_t definition

/* SDL-inspired flags */
#define GFX_DBLBUF (1 << 0)
#define GFX_SWSURF (1 << 1)
#define GFX_ASYNCB (1 << 2)
#define GFX_HWPAL  (1 << 3)
#define GFX_FULLSC (1 << 4)

/* Dirty-rectangle inspired flags */
#define GFX_DRECTS (1 << 5)     /* enable dirty rectangles */
#define GFX_DRCMRG (1 << 6)     /* A clean rect can merge between 2 dirty */

/* Other misc flags */
#define GFX_SKIP_EXTRA (1 << 7) /* Show at most 60 frames per wall-clock sec */

/* Internal */
#define GFX_FAILOK (1 << 8)     /* Return -1 if setting a mode fails. */

/*
 * ============================================================================
 *  GFX_PVT_T        -- Private internal state to gfx_t structure.
 *  GFX_T            -- Generic graphics object.  The graphics object is a
 *                      periph also so that screen updates, etc. can be
 *                      scheduled via the global tick mechanism.
 * ============================================================================
 */

struct avi_writer_t;    /* forward decl */
struct gfx_pvt_t;       /* forward decl */
struct mvi_t;           /* forward decl */

typedef struct gfx_t
{
    periph_t    periph;             /*  Yes, gfx_t is a peripheral.         */
    uint8_t    *vid;                /*  Display bitmap (160x200x8bpp).      */
    uint8_t     bbox[8][4];         /*  Bounding boxes for the 8 MOBs       */
    int         dirty;              /*  FLAG: Display needs update.         */
    uint32_t    drop_frame;         /*  while > 0 drop frames.              */
    uint32_t    dropped_frames;     /*  counts dropped frames.              */
    uint32_t    tot_frames;         /*  total frames                        */
    uint32_t    tot_dropped_frames; /*  total dropped frames                */

    uint32_t    hidden;             /*  Visibility flag (set by event_t)    */
    uint32_t    scrshot;            /*  Screen-shot/movie requested         */
    uint32_t    toggle;             /*  Toggle full-screen / windowed       */

    int         b_color, b_dirty;   /*  Border color and dirty flag.        */
    int         x_blank, y_blank;   /*  FLAG: Blank top row, left column.   */
    int         x_delay, y_delay;   /*  X/Y display delay.                  */
    bool        debug_blank;        /*  FLAG: If set, dim instead of blank  */

    bool        req_pause;          /*  We're requesting a 2-second pause.  */

    bool        movie_init;         /*  Is movie structure initialized?     */
    struct mvi_t *movie;            /*  Pointer to mvi_t to reduce deps     */

    struct avi_writer_t *avi;       /*  Ptr to avi_write_t to reduce deps.  */
    int         audio_rate;         /*  Ugh... only needed for AVI.         */
    int         fps;                /*  Frame rate.                         */

    palette_t   palette;            /*  Current graphics palette.           */
    struct gfx_pvt_t *pvt;          /*  Private data.                       */
} gfx_t;

/*
 * ============================================================================
 *  GFX_CHECK         -- Validates graphics parameters
 *  GFX_INIT          -- Initializes a gfx_t object and gfx subsystem
 *  GFX_VID_ENABLE    -- Alert gfx that video has been enabled or blanked
 * ============================================================================
 */
enum
{
    VID_DISABLED = 0,       /* Video disabled this frame                    */
    VID_ENABLED  = 1,       /* Video enabled this frame                     */
    VID_UNKNOWN  = 2        /* Unknown whether video is enabled this frame  */
};

int      gfx_check       (int x, int y, int bpp, int prescale);
int      gfx_init        (gfx_t   *gfx,
                          int desired_x, int desired_y, int desired_bpp,
                          int flags, int verbose, int prescaler, 
                          int bord_x, int bord_y, int pal_mode,
                          struct avi_writer_t *const avi, int audio_rate,
                          const palette_t *const palette);
void     gfx_vid_enable  (gfx_t   *gfx, int enabled);

/*
 * ============================================================================
 *  GFX_SET_BORD     -- Set the border / offset parameters for the display
 * ============================================================================
 */
void gfx_set_bord
(
    gfx_t *gfx,         /*  Graphics object.                        */
    int bord
);

/*
 * ============================================================================
 *  GFX_SET_TITLE    -- Sets the window title
 * ============================================================================
 */
int gfx_set_title(gfx_t *gfx, const char *title);

#define GFX_SHOT    (1 << 0)
#define GFX_MVTOG   (1 << 1)
#define GFX_MOVIE   (1 << 2)
#define GFX_RESET   (1 << 3)
#define GFX_AVTOG   (1 << 4)
#define GFX_AVI     (1 << 5)

#define GFX_WIND_TOG  (1)   /* Toggle windowed / fullscreen */
#define GFX_WIND_ON   (2)   /* Go to windowed mode.         */
#define GFX_WIND_OFF  (3)   /* Go to fullscreen mode.       */

/* ======================================================================== */
/*  GFX_TOGGLE_WINDOWED -- Try to toggle windowed vs. full-screen.          */
/* ======================================================================== */
bool gfx_toggle_windowed(gfx_t *gfx, int quiet);

/* ======================================================================== */
/*  GFX_FORCE_WINDOWED -- Force display to be windowed mode; Returns 1 if   */
/*                        display was previously full-screen.               */
/* ======================================================================== */
int gfx_force_windowed(gfx_t *gfx, int quiet);

/* ======================================================================== */
/*  GFX_DIRTYRECT_SPEC_T    Details that drive the dirty rectangle routine  */
/* ======================================================================== */
typedef struct gfx_dirtyrect_spec
{
    int active_first_x, active_last_x, x_step;
    int active_first_y, active_last_y, y_step;

    int pitch;

    int bord_first_x, bord_last_x;
    int bord_first_y, bord_last_y;
} gfx_dirtyrect_spec;

/* ======================================================================== */
/*  GFX_RESYNC   -- Resynchronize GFX internal state after a load.          */
/* ======================================================================== */
void gfx_resync(gfx_t *const gfx);

/* ======================================================================== */
/*  GFX_MOVIE_IS_ACTIVE  -- Return whether we're recording an AVI/GIF, or   */
/*                          have a pending screenshot.                      */
/* ======================================================================== */
#define gfx_movie_is_active(g) \
    (((g)->scrshot & \
      (GFX_MOVIE | GFX_AVI | GFX_SHOT | GFX_MVTOG | GFX_AVTOG)) != 0)

/* ======================================================================== */
/*  GFX_HIDDEN       -- Returns true if the graphics window is hidden.      */
/* ======================================================================== */
bool gfx_hidden(const gfx_t *const gfx);

/* ======================================================================== */
/*  GFX_STIC_TICK    -- Called directly by STIC to sync video pipeline.     */
/* ======================================================================== */
void gfx_stic_tick(gfx_t *const gfx);

/* ======================================================================== */
/*  GFX_REFRESH      -- Called when we just need to regenerate the video.   */
/* ======================================================================== */
void gfx_refresh(gfx_t *const gfx);

/* ======================================================================== */
/*  GFX_SCRSHOT      -- Write a 320x200 screen shot to a GIF file.          */
/* ======================================================================== */
void gfx_scrshot(const gfx_t *const gfx);

/* ======================================================================== */
/*  GFX_MOVIEUPD     -- Start/Stop/Update a movie in progress               */
/* ======================================================================== */
void gfx_movieupd(gfx_t *const gfx);

/* ======================================================================== */
/*  GFX_AVIUPD       -- Start/Stop/Update an AVI in progress                */
/* ======================================================================== */
void gfx_aviupd(gfx_t *const gfx);

#endif /* GFX_H_ */

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
/*                    Copyright (c) 2006, Joseph Zbiciak                    */
/* ======================================================================== */
