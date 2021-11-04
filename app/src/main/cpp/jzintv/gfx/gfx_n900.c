#define BENCHMARK_GFX
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
 *  GFX_STIC_PALETTE -- The STIC palette.
 * ============================================================================
 *  The graphics subsystem provides an abstraction layer between the
 *  emulator and the graphics library being used.  Theoretically, this
 *  should allow easy porting to other graphics libraries.
 *
 *  This is a modified version of gfx.c for N900.  This may get merged back
 *  into gfx.c as #ifdefs someday
 * ============================================================================
 */

#include "sdl_jzintv.h"
#include <SDL_haa.h>
#include "config.h"
#include "periph/periph.h"
#include "gfx.h"
#include "gfx_scale.h"
#include "file/file.h"
#include "mvi/mvi.h"
#include "gif/gif_enc.h"

/*
 * ============================================================================
 *  GFX_PVT_T        -- Private internal state to gfx_t structure.
 * ============================================================================
 */
typedef struct gfx_pvt_t
{
    SDL_Surface *scr;               /*  Screen surface in HAA_Actor     */
    HAA_Actor   *actor;             /*  HAA_Actor handle                */
    SDL_Color   pal_on [32];        /*  Palette when video is enabled.  */
    SDL_Color   pal_off[32];        /*  Palette when video is blanked.  */
    int         vid_enable;         /*  Video enable flag.              */
    int         ofs_x, ofs_y;       /*  X/Y offsets for centering img.  */
    int         bpp;                /*  Actual color depth.             */
    int         flags;              /*  Flags for current display surf. */

    int         movie_init;         /*  Is movie structure initialized? */
    mvi_t       *movie;             /*  Pointer to mvi_t to reduce deps */

    gfx_scale_spec_t    scaler;
} gfx_pvt_t;

LOCAL void gfx_dtor(periph_t *const p);
LOCAL void gfx_tick(gfx_t *const gfx);

/*
 * ============================================================================
 *  GFX_STIC_PALETTE -- The STIC palette.
 * ============================================================================
 */
LOCAL uint8_t gfx_stic_palette[32][3] =
{
    /* -------------------------------------------------------------------- */
    /*  I generated these colors by directly eyeballing my television       */
    /*  while it was next to my computer monitor.  I then tweaked each      */
    /*  color until it was pretty close to my TV.  Bear in mind that        */
    /*  NTSC (said to mean "Never The Same Color") is highly susceptible    */
    /*  to Tint/Brightness/Contrast settings, so your mileage may vary      */
    /*  with this particular pallete setting.                               */
    /* -------------------------------------------------------------------- */
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x2D, 0xFF },
    { 0xFF, 0x3D, 0x10 },
    { 0xC9, 0xCF, 0xAB },
    { 0x38, 0x6B, 0x3F },
    { 0x00, 0xA7, 0x56 },
    { 0xFA, 0xEA, 0x50 },
    { 0xFF, 0xFC, 0xFF },
    { 0xBD, 0xAC, 0xC8 },
    { 0x24, 0xB8, 0xFF },
    { 0xFF, 0xB4, 0x1F },
    { 0x54, 0x6E, 0x00 },
    { 0xFF, 0x4E, 0x57 },
    { 0xA4, 0x96, 0xFF },
    { 0x75, 0xCC, 0x80 },
    { 0xB5, 0x1A, 0x58 },

    /* -------------------------------------------------------------------- */
    /*  This pink color is used for drawing rectangles around sprites.      */
    /*  It's a temporary hack.                                              */
    /* -------------------------------------------------------------------- */
    { 0xFF, 0x80, 0x80 },
    /* -------------------------------------------------------------------- */
    /*  Grey shades used for misc tasks (not currently used).               */
    /* -------------------------------------------------------------------- */
    { 0x11, 0x11, 0x11 },
    { 0x22, 0x22, 0x22 },
    { 0x33, 0x33, 0x33 },
    { 0x44, 0x44, 0x44 },
    { 0x55, 0x55, 0x55 },
    { 0x66, 0x66, 0x66 },
    { 0x77, 0x77, 0x77 },
    { 0x88, 0x88, 0x88 },
    { 0x99, 0x99, 0x99 },
    { 0xAA, 0xAA, 0xAA },
    { 0xBB, 0xBB, 0xBB },
    { 0xCC, 0xCC, 0xCC },
    { 0xDD, 0xDD, 0xDD },
    { 0xEE, 0xEE, 0xEE },
    { 0xFF, 0xFF, 0xFF },
};


/*  01234567890123
**  ###  ####  ###
**  #  # #    #
**  ###  ###  #
**  #  # #    #
**  #  # ####  ###
*/

LOCAL const char* gfx_rec_bmp[5] =
{
   "###  ####  ###",
   "#  # #    #   ",
   "###  ###  #   ",
   "#  # #    #   ",
   "#  # ####  ###"
};




/* ======================================================================== */
/*  GFX_SDL_ABORT    -- Abort due to SDL errors.                            */
/* ======================================================================== */
LOCAL void gfx_sdl_abort(void)
{
    fprintf(stderr, "gfx/SDL Error:%s\n", SDL_GetError());
    exit(1);
}

/* ======================================================================== */
/*  GFX_SET_SCALER_PALETTE                                                  */
/* ======================================================================== */
LOCAL void gfx_set_scaler_palette
(
    SDL_Surface         *scr,
    gfx_scale_spec_t    *scaler,
    SDL_Color           pal[32]
)
{
    int i;
    uint32_t t;

    for (i = 0; i < 32; i++)
    {
        t = SDL_MapRGB(scr->format, pal[i].r, pal[i].g, pal[i].b);
        gfx_scale_set_palette(scaler, i, t);
    }
}

/* ======================================================================== */
/*  GFX_SETUP_SDL_SURFACE:  Do all the dirty SDL dirty work for setting up  */
/*                          the display.  This gets called during init, or  */
/*                          when toggling between full-screen and windowed  */
/* ======================================================================== */
LOCAL int gfx_setup_sdl_surface
(
    gfx_t *gfx, int flags, int quiet
)
{
    int i;
    int desire_x   = gfx->pvt->scaler.actual_x;
    int desire_y   = gfx->pvt->scaler.actual_y;
    int desire_bpp = 16;
    uint32_t sdl_flags = 0;
    SDL_Surface *scr, *scr_real;
    HAA_Actor *actor;

    /* -------------------------------------------------------------------- */
    /*  If we have an existing HAA actor, release it.                       */
    /* -------------------------------------------------------------------- */
    if (gfx->pvt->actor)
    {
        HAA_FreeActor(gfx->pvt->actor);
        gfx->pvt->actor = NULL;
        gfx->pvt->scr   = NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Set up the SDL video flags from our flags.                          */
    /* -------------------------------------------------------------------- */
    sdl_flags  =                      SDL_SWSURFACE;
    sdl_flags |= flags & GFX_FULLSC ? SDL_FULLSCREEN : 0;

    /* -------------------------------------------------------------------- */
    /*  Allocate a screen with the desired flags, and let SDL tell us the   */
    /*  size.  HAA will handle stretching our smaller window into the full  */
    /*  size window SDL gets us.                                            */
    /* -------------------------------------------------------------------- */

    scr_real = SDL_SetVideoMode(0, 0, 16, sdl_flags);

    if (!scr_real)
        return -1;
    HAA_SetVideoMode();

    actor = HAA_CreateActor(SDL_SWSURFACE, desire_x, desire_y, 16);

    if (actor && actor->surface)
    {
        gfx->pvt->actor = actor;
        gfx->pvt->scr   = actor->surface;
    }
    else
        return -1;

fprintf(stderr, "scr_real w,h = %d,%d\n", scr_real->w, scr_real->h);
    HAA_SetPosition  (actor, 0, 0); /* what is this all about? */
    HAA_SetScale     (actor, (double)scr_real->w / desire_x,
                             (double)scr_real->h / desire_y);
    HAA_SetRotationX (actor, HAA_X_AXIS, 0,0,0,0);
    HAA_SetRotationX (actor, HAA_Y_AXIS, 0,0,0,0);
    HAA_SetRotationX (actor, HAA_Z_AXIS, 0,0,0,0);
    HAA_SetGravity   (actor, HAA_GRAVITY_CENTER);
    HAA_Show         (actor);
    HAA_Commit       (actor);
    HAA_Flip         (actor);


    gfx->pvt->ofs_x = 0;
    gfx->pvt->ofs_y = 0;
    gfx->pvt->bpp   = 16;
    gfx->pvt->flags = flags;
    sdl_flags       = gfx->pvt->scr->flags;

    /* -------------------------------------------------------------------- */
    /*  TEMPORARY: Verify that the surface's format is as we expect.  This  */
    /*  is just a temporary bit of paranoia to ensure that scr->pixels      */
    /*  is in the format I _think_ it's in.                                 */
    /* -------------------------------------------------------------------- */
    if ((desire_bpp == 8 && (gfx->pvt->scr->format->BitsPerPixel  !=  8   ||
                             gfx->pvt->scr->format->BytesPerPixel !=  1))   ||
        (desire_bpp ==16 && (gfx->pvt->scr->format->BitsPerPixel  != 16   ||
                             gfx->pvt->scr->format->BytesPerPixel !=  2))   ||
        (desire_bpp ==32 && (gfx->pvt->scr->format->BitsPerPixel  != 32   ||
                             gfx->pvt->scr->format->BytesPerPixel !=  4)))
    {
        fprintf(stderr,"gfx panic: BitsPerPixel = %d, BytesPerPixel = %d\n",
                gfx->pvt->scr->format->BitsPerPixel,
                gfx->pvt->scr->format->BytesPerPixel);
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  New surface may need palette initialization.                        */
    /* -------------------------------------------------------------------- */
    gfx_set_scaler_palette( gfx->pvt->scr,
                           &gfx->pvt->scaler,
                            gfx->pvt->vid_enable ? gfx->pvt->pal_on
                                                 : gfx->pvt->pal_off);

    return 0;
}

/* ======================================================================== */
/*  GFX_INIT         -- Initializes a gfx_t object.                         */
/* ======================================================================== */
int gfx_init(gfx_t *gfx, int desire_x, int desire_y, int desire_bpp, int flags,
             int verbose)
{
    int i;

    /* -------------------------------------------------------------------- */
    /*  Hardcodes for N900                                                  */
    /* -------------------------------------------------------------------- */
    desire_x   = 800;
    desire_y   = 400;
    desire_bpp = 16;

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

    if (!gfx->vid && !gfx->pvt)
    {
        fprintf(stderr, "gfx panic:  Could not allocate memory.\n");
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Select the appropriate tick function based on our display res.      */
    /*  For now, only support 320x200x8bpp or 640x480x8bpp.                 */
    /* -------------------------------------------------------------------- */
    if (desire_bpp == 24)
        desire_bpp = 32;

    if (gfx_scale_init_spec(&(gfx->pvt->scaler), 160, 200,
                             desire_x, desire_y, desire_bpp))
    {
        fprintf(stderr,
                "Could not configure scaler for %d x %d @ %d bpp\n",
                desire_x, desire_y, desire_bpp);
    }

    /* -------------------------------------------------------------------- */
    /*  Set up our color palette.  We start with video blanked.             */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 16; i++)
    {
        gfx->pvt->pal_on [i].r = gfx_stic_palette[i][0];
        gfx->pvt->pal_on [i].g = gfx_stic_palette[i][1];
        gfx->pvt->pal_on [i].b = gfx_stic_palette[i][2];
        gfx->pvt->pal_off[i].r = gfx_stic_palette[i][0] >> 1;
        gfx->pvt->pal_off[i].g = gfx_stic_palette[i][1] >> 1;
        gfx->pvt->pal_off[i].b = gfx_stic_palette[i][2] >> 1;
    }
    for (i = 16; i < 32; i++)
    {
        gfx->pvt->pal_on [i].r = gfx_stic_palette[i][0];
        gfx->pvt->pal_on [i].g = gfx_stic_palette[i][1];
        gfx->pvt->pal_on [i].b = gfx_stic_palette[i][2];
        gfx->pvt->pal_off[i].r = gfx_stic_palette[i][0];
        gfx->pvt->pal_off[i].g = gfx_stic_palette[i][1];
        gfx->pvt->pal_off[i].b = gfx_stic_palette[i][2];
    }
    gfx->pvt->vid_enable = 0;

    /* -------------------------------------------------------------------- */
    /*  Set up initial graphics mode.                                       */
    /* -------------------------------------------------------------------- */
    if (gfx_setup_sdl_surface(gfx, flags, !verbose) < 0)
        gfx_sdl_abort();

    /* -------------------------------------------------------------------- */
    /*  Ok, see if we succeeded in setting our initial video mode, and do   */
    /*  some minor tidying.                                                 */
    /* -------------------------------------------------------------------- */
    if (!gfx->pvt->actor || HAA_Flip(gfx->pvt->actor) == -1)
        gfx_sdl_abort();

    /* -------------------------------------------------------------------- */
    /*  Hide the mouse.                                                     */
    /* -------------------------------------------------------------------- */
    SDL_ShowCursor(0);

    /* -------------------------------------------------------------------- */
    /*  Set up the gfx_t's internal structures.                             */
    /* -------------------------------------------------------------------- */
    gfx->periph.read        = NULL;
    gfx->periph.write       = NULL;
    gfx->periph.peek        = NULL;
    gfx->periph.poke        = NULL;
    gfx->periph.tick        = NULL;
    gfx->periph.min_tick    = 14934;
    gfx->periph.max_tick    = 14934;
    gfx->periph.addr_base   = 0;
    gfx->periph.addr_mask   = 0;
    gfx->periph.dtor        = gfx_dtor;

    return 0;
}

/* ======================================================================== */
/*  GFX_DTOR     -- Tear down the gfx_t                                     */
/* ======================================================================== */
LOCAL void gfx_dtor(periph_t *const p)
{
    gfx_t *const gfx = PERIPH_AS(gfx_t, p);

    if (gfx->pvt &&
        gfx->pvt->movie)
    {
        if (gfx->pvt->movie->f)
            fclose(gfx->pvt->movie->f);

        CONDFREE(gfx->pvt->movie);
    }

    CONDFREE(gfx->pvt);
    CONDFREE(gfx->vid);
}

/* ======================================================================== */
/*  GFX_TOGGLE_WINDOWED -- Try to toggle windowed vs. full-screen.          */
/* ======================================================================== */
void gfx_toggle_windowed(gfx_t *gfx, int quiet)
{
    if (!quiet)
        jzp_printf("\n");

    gfx->toggle = 0;
    if (gfx_setup_sdl_surface(gfx, gfx->pvt->flags ^ GFX_FULLSC, quiet) < 0)
        gfx_setup_sdl_surface(gfx, gfx->pvt->flags, quiet);
    else
    {
        int tmp;

        gfx->req_pause = true; /* Let monitor come up to speed w/ new res. */
        gfx->b_dirty = 2;
        gfx->dirty   = 1;

        tmp = gfx->scrshot;
        gfx->scrshot = 0;
        gfx->periph.tick(&gfx->periph, 0);
        gfx->scrshot = tmp;
    }
}

/* ======================================================================== */
/*  GFX_FORCE_WINDOWED -- Force display to be windowed mode; Returns 1 if   */
/*                        display was previously full-screen.               */
/* ======================================================================== */
int gfx_force_windowed(gfx_t *gfx, int quiet)
{
    if (gfx->pvt->flags & GFX_FULLSC)
    {
        gfx_toggle_windowed(gfx, quiet);
        return 1;
    }

    return 0;
}

/* ======================================================================== */
/*  GFX_SET_TITLE    -- Sets the window title                               */
/* ======================================================================== */
int gfx_set_title(gfx_t *gfx, const char *title)
{
    UNUSED(gfx);
    SDL_WM_SetCaption(title, title);
    return 0;
}
#ifdef BENCHMARK_GFX
LOCAL double bm_max = 0, bm_min = 1e30, bm_tot = 0;
LOCAL int bm_cnt = 0;
#endif

/* ======================================================================== */
/*  GFX_STIC_TICK    -- Services a gfx_t tick                               */
/* ======================================================================== */
void gfx_stic_tick(gfx_t *const gfx)
{
#ifdef BENCHMARK_GFX
    double start, end, diff;

    start = get_time();
#endif

    gfx->tot_frames++;

    /* -------------------------------------------------------------------- */
    /*  Update a movie if one's active, or user requested toggle in movie   */
    /*  state.  We do this prior to dropping frames so that movies always   */
    /*  have a consistent frame rate.                                       */
    /* -------------------------------------------------------------------- */
    if (gfx->scrshot & (GFX_MOVIE | GFX_MVTOG))
        gfx_movieupd(gfx);

    /* -------------------------------------------------------------------- */
    /*  Toggle full-screen/windowed if req'd.                               */
    /* -------------------------------------------------------------------- */
    if (gfx->toggle)
        gfx_toggle_windowed(gfx, 0);

    /* -------------------------------------------------------------------- */
    /*  Drop a frame if we need to.                                         */
    /* -------------------------------------------------------------------- */
    if (gfx->drop_frame)
    {
        gfx->drop_frame--;
        if (gfx->dirty) gfx->dropped_frames++;
        return;
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
#if 0
        jzp_printf("Dropped %d frames.\n", gfx->dropped_frames);
        jzp_flush();
#endif
        gfx->tot_dropped_frames += gfx->dropped_frames;
        gfx->dropped_frames = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Draw the frame to the screen surface.                               */
    /* -------------------------------------------------------------------- */
    gfx_tick(gfx);

    /* -------------------------------------------------------------------- */
    /*  Actually update the display.                                        */
    /* -------------------------------------------------------------------- */
    /*SDL_UpdateRect(gfx->pvt->scr, 0, 0, 0, 0);*/
  //SDL_Flip(gfx->pvt->scr  );
  //HAA_Flip(gfx->pvt->actor);

    /* -------------------------------------------------------------------- */
    /*  Update the palette if there's been a change in blanking state.      */
    /* -------------------------------------------------------------------- */
    if (gfx->pvt->vid_enable & 2)
    {
        gfx->pvt->vid_enable &= 1;
        gfx->pvt->vid_enable ^= 1;

        if (gfx->pvt->scaler.bpp == 8)
        {
            SDL_SetColors(gfx->pvt->scr,
                          gfx->pvt->vid_enable ? gfx->pvt->pal_on
                                               : gfx->pvt->pal_off, 0, 16);
        } else
        {
            gfx_set_scaler_palette( gfx->pvt->scr,
                                   &gfx->pvt->scaler,
                                    gfx->pvt->vid_enable ? gfx->pvt->pal_on
                                                         : gfx->pvt->pal_off);

        }
    }

    gfx->dirty = 0;

    /* -------------------------------------------------------------------- */
    /*  If a screen-shot was requested, go write out a PPM file of the      */
    /*  screen right now.  Screen-shot PPMs are always 320x200.             */
    /* -------------------------------------------------------------------- */
    if (gfx->scrshot & GFX_SHOT)
    {
        gfx_scrshot(gfx->vid);
        gfx->scrshot &= ~GFX_SHOT;
    }

#ifdef BENCHMARK_GFX
    end = get_time();
    diff = end - start;
    if (diff > bm_max) bm_max = diff;
    if (diff < bm_min) bm_min = diff;
    bm_tot += diff;

    if (++bm_cnt == 32)
    {
        jzp_printf("gfx_tick: min = %8.3f max = %8.3f avg = %8.3f\n",
                   bm_min * 1000., bm_max * 1000., bm_tot * 1000. / 32);
        bm_max = bm_tot = 0;
        bm_cnt = 0;
        bm_min = 1e30;
    }
#endif
}

/* ======================================================================== */
/*  GFX_TICK         -- Services a gfx_t tick in any graphics format        */
/* ======================================================================== */
LOCAL void gfx_tick(gfx_t *const gfx)
{
    uint8_t *scr;

    if (SDL_MUSTLOCK(gfx->pvt->scr))
        SDL_LockSurface(gfx->pvt->scr);

    scr = gfx->pvt->ofs_x + gfx->pvt->scr->pitch * gfx->pvt->ofs_y +
          (uint8_t *) gfx->pvt->scr->pixels;

    gfx_scale
    (
        &gfx->pvt->scaler,
        gfx->vid,
        scr,
        gfx->pvt->scr->pitch
    );

    if (SDL_MUSTLOCK(gfx->pvt->scr))
        SDL_UnlockSurface(gfx->pvt->scr);
}

/* ======================================================================== */
/*  GFX_VID_ENABLE   -- Alert gfx that video has been enabled or blanked    */
/* ======================================================================== */
void gfx_vid_enable(gfx_t *gfx, int enabled)
{
    /* -------------------------------------------------------------------- */
    /*  Force 'enabled' to be 0 or 1.                                       */
    /* -------------------------------------------------------------------- */
    enabled = enabled != 0;

    /* -------------------------------------------------------------------- */
    /*  If enabled state changed, schedule a palette update.                */
    /* -------------------------------------------------------------------- */
    if ((gfx->pvt->vid_enable ^ enabled) & 1)
    {
        gfx->pvt->vid_enable |= 2;
        gfx->dirty = 1;
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

    /* -------------------------------------------------------------------- */
    /*  If we're using the normal STIC blanking behavior, set our "off"     */
    /*  colors to the currently selected border color.  The alternate mode  */
    /*  (which is useful for debugging) sets the blanked colors to be       */
    /*  dimmed versions of the normal palette.                              */
    /* -------------------------------------------------------------------- */
    if (!gfx->debug_blank)
    {
        int i;

        for (i = 0; i < 16; i++)
            gfx->pvt->pal_off[i] = gfx->pvt->pal_on[b_color];
    }

    if (dirty)     { gfx->dirty   = 1; }
    if (dirty & 2) { gfx->b_dirty = 2; }
}

/* ======================================================================== */
/*  GFX_SCRSHOT      -- Write a 320x200 screen shot to a GIF file.          */
/* ======================================================================== */
LOCAL uint8_t scrshot_buf[320*200];
void gfx_scrshot(uint8_t *scr)
{
    static int last = -1;
    FILE * f;
    char f_name[32];
    int num = last, i, len;


    /* -------------------------------------------------------------------- */
    /*  Search for an unused screen-shot file name.                         */
    /* -------------------------------------------------------------------- */
    do
    {
        num = (num + 1) % 10000;

        snprintf(f_name, sizeof(f_name), "shot%.4d.gif", num);

        if (!file_exists(f_name))
            break;

    } while (num != last);

    /* -------------------------------------------------------------------- */
    /*  Warn the user if we wrapped all 10000 screen shots...               */
    /* -------------------------------------------------------------------- */
    if (num == last)
    {
        num = (num + 1) % 10000;
        snprintf(f_name, sizeof(f_name), "shot%.4d.gif", num);
        fprintf(stderr, "Warning:  Overwriting %s...\n", f_name);
    }

    /* -------------------------------------------------------------------- */
    /*  Update our 'last' pointer and open the file and dump the PPM.       */
    /* -------------------------------------------------------------------- */
    last = num;
    f    = fopen(f_name, "wb");

    if (!f)
    {
        fprintf(stderr, "Error:  Could not open '%s' for screen dump.\n",
                f_name);
        return;
    }


    /* -------------------------------------------------------------------- */
    /*  Do the screen dump.  Write it as a nice GIF.  We need to pixel      */
    /*  double the image ahead of time.                                     */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 200*160; i++)
        scrshot_buf[i*2 + 0] = scrshot_buf[i*2 + 1] = scr[i];

    len = gif_write(f, scrshot_buf, 320, 200, gfx_stic_palette, 16);
    if (len > 0)
    {
        jzp_printf("\nWrote screen shot to '%s', %d bytes\n", f_name, len);
    } else
    {
        jzp_printf("\nError writing screen shot to '%s'\n", f_name);
    }
    jzp_flush();
    fclose(f);

    return;
}

/* ======================================================================== */
/*  GFX_MOVIEUPD     -- Start/Stop/Update a movie in progress               */
/* ======================================================================== */
void gfx_movieupd(gfx_t *gfx)
{
    gfx_pvt_t *pvt = gfx->pvt;


    /* -------------------------------------------------------------------- */
    /*  Toggle current movie state if user requested.                       */
    /* -------------------------------------------------------------------- */
    if (gfx->scrshot & GFX_MVTOG)
    {
        static int last = -1;
        int num = last;
        char f_name[32];

        /* ---------------------------------------------------------------- */
        /*  Whatever happens, clear the toggle.                             */
        /* ---------------------------------------------------------------- */
        gfx->scrshot &= ~GFX_MVTOG;

        /* ---------------------------------------------------------------- */
        /*  Make sure movie subsystem initialized.  We only init this if    */
        /*  someone tries to take a movie.                                  */
        /* ---------------------------------------------------------------- */
        if (!pvt->movie_init)
        {
            if (!pvt->movie) pvt->movie = CALLOC(mvi_t, 1);
            if (!pvt->movie)
            {
                fprintf(stderr, "No memory for movie structure\n");
                return;
            }

            mvi_init(pvt->movie, 160, 200);
            pvt->movie_init = 1;
        }

        /* ---------------------------------------------------------------- */
        /*  If a movie's open, close it.                                    */
        /* ---------------------------------------------------------------- */
        if ((gfx->scrshot & GFX_MOVIE) != 0)
        {
            if (pvt->movie->f)
            {
                fclose(pvt->movie->f);
                jzp_printf("\nDone writing movie:\n"
                       "    Total frames:        %10d\n"
                       "    Total size:          %10d\n"
                       "    Bytes/frame:         %10d\n"
#ifndef NO_LZO
                       "    Bytes saved LZO:     %10d\n"
#endif
                       "    Dupe frames:         %10d\n"
                       "    Dupe rows:           %10d\n"
                       "    Compression ratio:   %8.2f:1\n",
                       pvt->movie->fr,
                       pvt->movie->tot_bytes,
                       pvt->movie->tot_bytes / pvt->movie->fr,
#ifndef NO_LZO
                       pvt->movie->tot_lzosave,
#endif
                       pvt->movie->rpt_frames,
                       pvt->movie->rpt_rows,
                       (16032.*pvt->movie->fr) / pvt->movie->tot_bytes);
                jzp_flush();
            }

            gfx->scrshot &= ~GFX_MOVIE;
            pvt->movie->f  = NULL;
            pvt->movie->fr = 0;

            return;
        }

        /* ---------------------------------------------------------------- */
        /*  Otherwise, open a new movie.                                    */
        /*  Search for an unused movie file name.                           */
        /* ---------------------------------------------------------------- */
        do
        {
            num = (num + 1) % 10000;

            snprintf(f_name, sizeof(f_name), "mvi_%.4d.imv", num);

            if (!file_exists(f_name))
                break;

        } while (num != last);

        /* ---------------------------------------------------------------- */
        /*  Warn the user if we wrapped all 10000 movie slots...            */
        /* ---------------------------------------------------------------- */
        if (num == last)
        {
            num = (num + 1) % 10000;
            snprintf(f_name, sizeof(f_name), "mvi_%.4d.imv", num);
            fprintf(stderr, "Warning:  Overwriting %s...\n", f_name);
        }

        /* ---------------------------------------------------------------- */
        /*  Update our 'last' pointer, and start the movie.                 */
        /* ---------------------------------------------------------------- */
        last = num;
        pvt->movie->f = fopen(f_name, "wb");

        if (!pvt->movie->f)
        {
            fprintf(stderr, "Error:  Could not open '%s' for movie.\n",
                    f_name);
            return;
        }

        jzp_printf("\nStarted movie file '%s'\n", f_name); jzp_flush();

        /* ---------------------------------------------------------------- */
        /*  Success:  Turn on the movie.                                    */
        /* ---------------------------------------------------------------- */
        gfx->scrshot |= GFX_MOVIE;
        pvt->movie->fr = 0;
    }

    if ((gfx->scrshot & GFX_RESET) == 0)
        mvi_wr_frame(pvt->movie, gfx->vid, gfx->bbox);
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
/*          Copyright (c) 1998-2006, Joseph Zbiciak, John Tanner            */
/* ======================================================================== */
