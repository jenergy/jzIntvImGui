/*
 * ============================================================================
 *  Title:    Graphics Interface Routines
 *  Author:   J. Zbiciak, J. Tanner
 * ============================================================================
 *  GFX_INIT         -- Initializes a gfx_t object.
 *  GFX_TICK         -- Services a gfx_t tick.
 *  GFX_VID_ENABLE   -- Alert gfx that video has been enabled or blanked
 *  GFX_SET_BORD     -- Set the border / offset parameters for the display
 * ============================================================================
 *  GFX_T            -- Graphics subsystem object.
 *  GFX_PVT_T        -- Private internal state to gfx_t structure.
 * ============================================================================
 *  The graphics subsystem provides an abstraction layer between the
 *  emulator and the graphics library being used.  Theoretically, this
 *  should allow easy porting to other graphics libraries.
 *
 *  TODO:
 *   -- Make use of dirty rectangle updating for speed.
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "gfx.h"
#include "gfx/palette.h"
//#include "file/file.h"
#include "mvi/mvi.h"
#include "avi/avi.h"
#include "lzoe/lzoe.h"
#include "file/file.h"

/*
 * ============================================================================
 *  GFX_PVT_T        -- Private internal state to gfx_t structure.
 * ============================================================================
 */
typedef struct gfx_pvt_t
{
    int         vid_enable;         /*  Video enable flag.                  */
} gfx_pvt_t;

LOCAL void gfx_dtor(periph_t *const p);

/* ======================================================================== */
/*  GFX_CHECK        -- Validates gfx parameters                            */
/* ======================================================================== */
int gfx_check(int desire_x, int desire_y, int desire_bpp, int prescaler)
{
    UNUSED(desire_x);
    UNUSED(desire_y);
    UNUSED(desire_bpp);
    UNUSED(prescaler);
    return 0;
}

/* ======================================================================== */
/*  GFX_INIT         -- Initializes a gfx_t object.                         */
/* ======================================================================== */
int gfx_init(gfx_t *gfx, int desire_x, int desire_y, int desire_bpp,
                         int flags,    int verbose,  int prescaler,
                         int border_x, int border_y, int pal_mode,
                         struct avi_writer_t *const avi, int audio_rate,
                         const palette_t *const palette)
{
    UNUSED(desire_x);
    UNUSED(desire_y);
    UNUSED(desire_bpp);
    UNUSED(flags);
    UNUSED(verbose);
    UNUSED(prescaler);
    UNUSED(border_x);
    UNUSED(border_y);
    UNUSED(pal_mode);

    /* -------------------------------------------------------------------- */
    /*  Sanity checks and cleanups.                                         */
    /* -------------------------------------------------------------------- */
    assert(gfx);
    memset((void*)gfx, 0, sizeof(gfx_t));

    /* -------------------------------------------------------------------- */
    /*  Allocate memory for the gfx_t.                                      */
    /* -------------------------------------------------------------------- */
    gfx->vid = CALLOC(uint8_t,   160 * 200);
    gfx->pvt = CALLOC(gfx_pvt_t, 1);

    if (!gfx->vid || !gfx->pvt)
    {

        fprintf(stderr, "gfx:  Panic:  Could not allocate memory.\n");

        goto die;
    }

    gfx->pvt->vid_enable = 0;
    gfx->dirty      = 3;
    gfx->b_dirty    = 3;
    gfx->palette    = *palette;

    gfx->fps        = pal_mode ? 50 : 60;
    gfx->avi        = avi;
    gfx->audio_rate = audio_rate;  // ugh

    gfx->hidden     = true;

    /* -------------------------------------------------------------------- */
    /*  Set up the gfx_t's internal structures.                             */
    /* -------------------------------------------------------------------- */
    gfx->periph.read        = NULL;
    gfx->periph.write       = NULL;
    gfx->periph.peek        = NULL;
    gfx->periph.poke        = NULL;
    gfx->periph.tick        = NULL;
    gfx->periph.min_tick    = 0;
    gfx->periph.max_tick    = INT_MAX;
    gfx->periph.addr_base   = 0;
    gfx->periph.addr_mask   = 0;
    gfx->periph.dtor        = gfx_dtor;

#ifdef BENCHMARK_GFX
    atexit(gfx_dr_hist_dump);
#endif

    return 0;

die:
    CONDFREE(gfx->pvt);
    CONDFREE(gfx->vid);
    return -1;
}

/* ======================================================================== */
/*  GFX_DTOR     -- Tear down the gfx_t                                     */
/* ======================================================================== */
LOCAL void gfx_dtor(periph_t *const p)
{
    gfx_t *const gfx = PERIPH_AS(gfx_t, p);

    if (gfx->movie)
    {
        if (gfx->movie->f)
            fclose(gfx->movie->f);

        CONDFREE(gfx->movie);
    }

    if (avi_is_active(gfx->avi))
        avi_end_video(gfx->avi);

    CONDFREE(gfx->pvt);
    CONDFREE(gfx->vid);
}

/* ======================================================================== */
/*  GFX_TOGGLE_WINDOWED -- Try to toggle windowed vs. full-screen.          */
/* ======================================================================== */
bool gfx_toggle_windowed(gfx_t *gfx, int quiet)
{
    UNUSED(gfx);
    UNUSED(quiet);
    return false;
}

/* ======================================================================== */
/*  GFX_FORCE_WINDOWED -- Force display to be windowed mode; Returns 1 if   */
/*                        display was previously full-screen.               */
/* ======================================================================== */
int gfx_force_windowed(gfx_t *gfx, int quiet)
{
    UNUSED(gfx);
    UNUSED(quiet);
    return 0;
}

/* ======================================================================== */
/*  GFX_SET_TITLE    -- Sets the window title                               */
/* ======================================================================== */
int gfx_set_title(gfx_t *gfx, const char *title)
{
    UNUSED(gfx);
    UNUSED(title);
    return 0;
}

/* ======================================================================== */
/*  GFX_REFRESH      -- A whole lotta nuttin.                               */
/* ======================================================================== */
void gfx_refresh(gfx_t *const gfx)
{
    /* -------------------------------------------------------------------- */
    /*  Every ~0.5 second, force a dirty frame, in case there is a static   */
    /*  image.  On some systems (OS X in my case), the window will not      */
    /*  refresh properly unless we send *something* occasionally.           */
    /*                                                                      */
    /*  Where I saw it:  Dragging a window from the Retina display to an    */
    /*  external monitor caused the window to go all white.                 */
    /* -------------------------------------------------------------------- */
    if ((gfx->tot_frames++ & 31) == 0)
    {
        gfx->dirty |= 3;
        gfx->b_dirty |= 3;
    }

    /* -------------------------------------------------------------------- */
    /*  Don't bother if display isn't dirty or if we're iconified.          */
    /* -------------------------------------------------------------------- */
    if (!gfx->scrshot && (!gfx->dirty || gfx->hidden))
    {
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  DEBUG: Report blocks of dropped frames.                             */
    /* -------------------------------------------------------------------- */
    if (gfx->dropped_frames)
    {
        gfx->tot_dropped_frames += gfx->dropped_frames;
        gfx->dropped_frames = 0;
    }

    gfx->dirty = 0;
    gfx->b_dirty = 0;
}

/* ======================================================================== */
/*  GFX_STIC_TICK    -- Called directly from STIC emulation.                */
/* ======================================================================== */
void gfx_stic_tick(gfx_t *const gfx)
{
    gfx->tot_frames++;

    /* -------------------------------------------------------------------- */
    /*  Update a movie if one's active, or user requested toggle in movie   */
    /*  state.  We do this prior to dropping frames so that movies always   */
    /*  have a consistent frame rate.                                       */
    /* -------------------------------------------------------------------- */
    if (gfx->scrshot & (GFX_MOVIE | GFX_MVTOG))
        gfx_movieupd(gfx);

    /* -------------------------------------------------------------------- */
    /*  Update an AVI if one's active, or if user requested a toggle.       */
    /* -------------------------------------------------------------------- */
    if (gfx->scrshot & (GFX_AVI | GFX_AVTOG))
        gfx_aviupd(gfx);

    /* -------------------------------------------------------------------- */
    /*  Drop a frame if we need to.                                         */
    /* -------------------------------------------------------------------- */
    if (gfx->drop_frame)
    {
        gfx->drop_frame--;
        gfx->tot_frames++;
        if (gfx->dirty) gfx->dropped_frames++;
        return;
    }

    gfx_refresh(gfx);

    /* -------------------------------------------------------------------- */
    /*  If a screen-shot was requested, go write out a GIF file of the      */
    /*  screen right now.  Screen-shot GIFs are always 320x200.             */
    /* -------------------------------------------------------------------- */
    if (gfx->scrshot & GFX_SHOT)
    {
        gfx_scrshot(gfx);
        gfx->scrshot &= ~GFX_SHOT;
    }

    return;
}

/* ======================================================================== */
/*  GFX_VID_ENABLE   -- Alert gfx that video has been enabled or blanked    */
/* ======================================================================== */
void gfx_vid_enable(gfx_t *gfx, int enabled)
{
    /* -------------------------------------------------------------------- */
    /*  Force 'enabled' to be 0 or 1.                                       */
    /* -------------------------------------------------------------------- */
    enabled = enabled == VID_ENABLED;

    /* -------------------------------------------------------------------- */
    /*  If enabled state changed, schedule a palette update.                */
    /* -------------------------------------------------------------------- */
    if ((gfx->pvt->vid_enable ^ enabled) & 1)
    {
        gfx->pvt->vid_enable |= 2;
        gfx->dirty |= 2;
    } else
    {
        gfx->pvt->vid_enable = enabled;
    }
}

/* ======================================================================== */
/*  GFX_SET_BORD     -- Set the border color for the display                */
/* ======================================================================== */
void gfx_set_bord
(
    gfx_t *gfx,         /*  Graphics object.                        */
    int b_color
)
{
    int dirty = 0;

    /* -------------------------------------------------------------------- */
    /*  Set up the display parameters.                                      */
    /* -------------------------------------------------------------------- */
    if (gfx->b_color != b_color) { gfx->b_color = b_color; dirty = 3; }

    if (dirty)     { gfx->dirty   |= 1; }
    if (dirty & 2) { gfx->b_dirty |= 2; }
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
/*               Copyright (c) 1998-2020, Joseph Zbiciak.                   */
/* ======================================================================== */
