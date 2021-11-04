//#define BENCHMARK_GFX
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

#include "sdl_jzintv.h"
#include "config.h"
#include "periph/periph.h"
#include "gfx/gfx.h"
#include "gfx/gfx_prescale.h"
#include "gfx/gfx_scale.h"
//#include "file/file.h"
#include "mvi/mvi.h"
#include "avi/avi.h"
#include "lzoe/lzoe.h"
#include "file/file.h"

#ifdef PLAT_MACOS
#include "gfx/gfx_sdl2_osx.h"
#endif

extern void update_screen_size();
extern void set_window(SDL_Window* w);
const double frame_delta = 0.0166;  /* Slightly faster than 60Hz.           */

/*
 * ============================================================================
 *  GFX_PVT_T        -- Private internal state to gfx_t structure.
 * ============================================================================
 */
typedef struct gfx_pvt_t
{
    SDL_Window      *wind;          /*  Main window.                        */
    SDL_Renderer    *rend;          /*  Renders texture onto surface.       */
    SDL_PixelFormat *pixf;          /*  Screen surface.                     */
    SDL_Texture     *text;          /*  Main surface texture.               */
    SDL_Color   pal_on [32];        /*  Palette when video is enabled.      */
    SDL_Color   pal_off[32];        /*  Palette when video is blanked.      */
    int         vid_enable;         /*  Video enable flag.                  */
    int         border_x, border_y; /*  X/Y border padding.                 */
    int         dim_x, dim_y;       /*  X/Y dimensions of window.           */
    int         ofs_x, ofs_y;       /*  X/Y offsets for centering img.      */
    int         bpp;                /*  Actual color depth.                 */
    int         flags;              /*  Flags for current display window.   */

    /* For GFX_DROP_EXTRA only: */
    double      last_frame;         /*  Wallclock time of next frame.       */

    uint8_t *RESTRICT inter_vid;    /*  Intermediate video after prescaler  */
    uint8_t *RESTRICT prev;         /*  previous frame for dirty-rect       */

    gfx_prescaler_t      *prescaler; /* Scale 160x200 to an intermediate    */
    gfx_prescaler_dtor_t *ps_dtor;   /* Destructor for prescaler, if any.   */
    void                 *ps_opaque; /* Prescaler opaque structure          */
    gfx_scale_spec_t     scaler;

    gfx_dirtyrect_spec  dr_spec;    /*  Dirty-rectangle control spec.       */


    uint32_t    *dirty_rows;        /*  dirty-row bitmap for scaler         */
    int         dirty_rows_sz;

    int         num_rects;
    SDL_Rect    *dirty_rects;
} gfx_pvt_t;

LOCAL void gfx_dtor(periph_t *const p);
LOCAL void gfx_tick(gfx_t *gfx);
LOCAL void gfx_find_dirty_rects(gfx_t *gfx);

/* ======================================================================== */
/*  GFX_SDL_ABORT    -- Abort due to SDL errors.                            */
/* ======================================================================== */
LOCAL void gfx_sdl_abort(const char *context)
{
    fprintf(stderr, "gfx: %s\ngfx/SDL Error:%s\n", context, SDL_GetError());
    exit(1);
}

/* ======================================================================== */
/*  GFX_SET_SCALER_PALETTE                                                  */
/* ======================================================================== */
LOCAL void gfx_set_scaler_palette
(
    gfx_scale_spec_t       *const scaler,
    const SDL_PixelFormat  *const pix_fmt,
    const SDL_Color        *const pal
)
{
    for (int i = 0; i < 32; i++)
        gfx_scale_set_palette(scaler, i,
            SDL_MapRGB(pix_fmt, pal[i].r, pal[i].g, pal[i].b));
}

/* ======================================================================== */
/*  GFX_TEARDOWN_SDL_DISPLAY                                                */
/*  Tears down the existing surface, renderer, texture, and window, if any. */
/* ======================================================================== */
LOCAL void gfx_teardown_sdl_display(gfx_t *gfx)
{
    if (gfx->pvt->text) SDL_DestroyTexture(gfx->pvt->text);
    if (gfx->pvt->pixf) SDL_FreeFormat(gfx->pvt->pixf);
    if (gfx->pvt->rend) SDL_DestroyRenderer(gfx->pvt->rend);
    if (gfx->pvt->wind) SDL_DestroyWindow(gfx->pvt->wind);

    gfx->pvt->text = NULL;
    gfx->pvt->pixf = NULL;
    gfx->pvt->rend = NULL;
    gfx->pvt->wind = NULL;
}

/* ======================================================================== */
/*  GFX_SETUP_SDL_DISPLAY:  Do all the dirty SDL dirty work for setting up  */
/*                          the display.  This gets called during init, or  */
/*                          when toggling between full-screen and windowed  */
/* ======================================================================== */
LOCAL int gfx_setup_sdl_display
(
    gfx_t *gfx, uint32_t gfx_flags, int quiet
)
{
    /* Target width / height of the border.  Actual border may be larger.   */
    const int bord_x = gfx->pvt->border_x;
    const int bord_y = gfx->pvt->border_y;

    /* Actual dims of the scaler output, and thus our texture dimensions.   */
    const int text_x = gfx->pvt->scaler.actual_x;
    const int text_y = gfx->pvt->scaler.actual_y;

    /* Our desired window size / physical display mode.                     */
    const int tgt_wind_x = text_x + bord_x;
    const int tgt_wind_y = text_y + bord_y;
    //const unsigned tgt_wind_bpp = gfx->pvt->scaler.bpp;
    const unsigned tgt_wind_bpp = 32;  /* For now, force to 32bpp. */

    /* The actual window size / physical display mode.                      */
    int wind_x = tgt_wind_x, wind_y = tgt_wind_y;

    /* -------------------------------------------------------------------- */
    /*  Force dirty-rectangles off.  It's not clear we can support them in  */
    /*  SDL2:  Streaming textures aren't guaranteed to persist between      */
    /*  locks.  Likewise, the renderer backdrop is also not guaranteed to   */
    /*  persist after SDL_RenderPresent().                                  */
    /* -------------------------------------------------------------------- */
    gfx_flags &= ~GFX_DRECTS;

    /* -------------------------------------------------------------------- */
    /*  Set up the SDL video flags from our flags.                          */
    /* -------------------------------------------------------------------- */
    if (tgt_wind_bpp == 32)
        gfx_flags &= ~GFX_HWPAL;    /*  No hardware palette in 32-bpp.      */

    if ((gfx_flags & GFX_DRECTS) != 0)
        gfx_flags &= ~GFX_DBLBUF;   /*  No double-buffering w/dirty rects.  */

    const bool request_fullscreen = gfx_flags & GFX_FULLSC;
    uint32_t wind_flags =
        SDL_WINDOW_INPUT_FOCUS      /*  Should we or shouldn't we?          */
      | SDL_WINDOW_SHOWN
      | (request_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);

    /*  The code currently is not "high-DPI" aware, and so does the wrong   */
    /*  thing if you turn on High DPI mode.  We don't need it anyway.       */
    //| SDL_WINDOW_ALLOW_HIGHDPI

    const bool software_surface = gfx_flags & GFX_SWSURF;
    const bool disable_vsync    = gfx_flags & GFX_ASYNCB;
    uint32_t rend_flags =
        (software_surface ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED)
      | (disable_vsync    ? 0 : SDL_RENDERER_PRESENTVSYNC);

    /* -------------------------------------------------------------------- */
    /*  Try to allocate a screen surface at the desired size, etc.          */
    /* -------------------------------------------------------------------- */
    if (!quiet)
    {
        jzp_printf("gfx:  Searching for video modes near %dx%dx%d, %s:\n",
           tgt_wind_x, tgt_wind_y, tgt_wind_bpp,
           request_fullscreen ? "Full screen" : "Windowed");

        jzp_flush();
    }

    /* -------------------------------------------------------------------- */
    /*  Work in progress for SDL2.                                          */
    /*  For now, just work with display 0 and let SDL2 pick window size.    */
    /* -------------------------------------------------------------------- */
#if 0
    if (!request_fullscreen)
    {
        /* In windowed mode, just go with the requested dimensions. */
        wind_x = tgt_wind_x;
        wind_y = tgt_wind_y;
    } else
    {
        const int curr_disp = 0;
        const int num_modes = SDL_GetNumDisplayModes(curr_disp);
        SDL_DisplayMode disp_mode;

        /* ---------------------------------------------------------------- */
        /*  SDL_GetDisplayMode returns a list sorted largest to smallest.   */
        /*  Find the smallest mode >= the size requested.                   */
        /* ---------------------------------------------------------------- */
        const int tgt_wind_area = tgt_wind_x * tgt_wind_y;
        int best = -1, area_diff, best_area_diff = INT_MAX;

        for (int i = 0; i < num_modes; ++i)
        {
            if (SDL_GetDisplayMode(curr_disp, i, &disp_mode))
            {
                jzp_printf("gfx:  Warning, SDL2 returned %s when querying "
                           "disp mode %d of %d\n", SDL_GetError(), i,
                           num_modes);
                break;
            }

            if (SDL_BITSPERPIXEL(disp_mode.format) != tgt_wind_bpp)
                continue;

            if (!quiet)
                jzp_printf("gfx:  Considering %dx%d... ",
                           disp_mode.w, disp_mode.h);
            if (disp_mode.w >= tgt_wind_x && disp_mode.h >= tgt_wind_y)
            {
                area_diff = disp_mode.w * disp_mode.h - tgt_wind_area;

                if (best_area_diff > area_diff)
                {
                    best_area_diff = area_diff;
                    best = i;

                    if (!quiet)
                        jzp_printf("New best fit.  Diff = %d\n", area_diff);

                    if (!best_area_diff)
                        break;
                } else
                    if (!quiet)
                        jzp_printf("Poorer fit.    Diff = %d\n", area_diff);
            } else
            {
                if (!quiet)
                    jzp_printf("Too small.\n");
            }
        }

        /* No suitable mode available. */
        if (best == -1)
            gfx_sdl_abort("No suitable video mode.");

        SDL_GetDisplayMode(curr_disp, best, &disp_mode);
        wind_x = disp_mode.w;
        wind_y = disp_mode.h;
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  Destroy the old window/surface/texture/renderer, if any.            */
    /* -------------------------------------------------------------------- */
    gfx_teardown_sdl_display(gfx);

    /* -------------------------------------------------------------------- */
    /*  Set up the new window/surface/texture/renderer.                     */
    /* -------------------------------------------------------------------- */
    SDL_Window *const wind =
        SDL_CreateWindow("jzintv",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            tgt_wind_x, tgt_wind_y, wind_flags);

    if (!wind) gfx_sdl_abort("Could not create window");

    set_window(wind);
    update_screen_size();
    const uint32_t wind_pix_fmt = SDL_GetWindowPixelFormat(wind);
    jzp_printf("gfx:  Window pix format: %s\n",
        SDL_GetPixelFormatName(wind_pix_fmt));

    SDL_Renderer *rend = SDL_CreateRenderer(wind, -1, rend_flags);

    if (!rend && rend_flags != SDL_RENDERER_SOFTWARE)
    {
        jzp_printf("gfx: Could not create renderer with requested flags: %s\n"
                   "     Trying again with software renderer, no VSync.\n",
                   SDL_GetError());

        /* Try again with software renderer. */
        rend_flags = SDL_RENDERER_SOFTWARE;
        rend = SDL_CreateRenderer(wind, -1, rend_flags);
        if (!rend) gfx_sdl_abort("Could not create renderer");
    }

    /* Note: We only keep the surface around for the pixel format pointer.  */
    SDL_PixelFormat *pixf = SDL_AllocFormat(wind_pix_fmt);

    SDL_Texture *text =
        SDL_CreateTexture(rend, wind_pix_fmt, SDL_TEXTUREACCESS_STREAMING,
                          text_x, text_y);

    uint32_t text_pix_fmt;
    SDL_QueryTexture(text, &text_pix_fmt, NULL, NULL, NULL);

    /* Sanity check. */
    if (text_pix_fmt != wind_pix_fmt)
    {
        jzp_printf("gfx: Texture pixel format doesn't match window's format. "
                   "%u vs %u\n", text_pix_fmt, wind_pix_fmt);
        exit(-1);
    }

    /* Snapshot the final wind/rend flags, in case SDL2 overrode us. */
    SDL_RendererInfo rend_info;
    SDL_GetRendererInfo(rend, &rend_info);
    const uint32_t act_wind_flags = SDL_GetWindowFlags(wind);
    const uint32_t act_rend_flags = rend_info.flags;

    gfx->pvt->wind  = wind;
    gfx->pvt->rend  = rend;
    gfx->pvt->pixf  = pixf;
    gfx->pvt->text  = text;
    gfx->pvt->dim_x = wind_x;
    gfx->pvt->dim_y = wind_y;
    gfx->pvt->ofs_x = ((wind_x - text_x) >> 1) & (~3);
    gfx->pvt->ofs_y =  (wind_y - text_y) >> 1;
    gfx->pvt->bpp   = SDL_BYTESPERPIXEL(text_pix_fmt) * 8;
    gfx->pvt->flags = gfx_flags;

    gfx->pvt->last_frame = get_time();

    if (!quiet)
    {
        jzp_printf("gfx:  Selected:  %dx%dx%d with:\n"
           "gfx:      VSync: %s, Rend: %s, Windowed: %s\n"
           "gfx:      Video Driver: '%s', Render Driver: '%s'\n",
           wind_x, wind_y, gfx->pvt->bpp,
           act_rend_flags & SDL_RENDERER_PRESENTVSYNC ? "Yes"      : "No",
           act_rend_flags & SDL_RENDERER_SOFTWARE     ? "Software" : "Hardware",
           act_wind_flags & SDL_WINDOW_FULLSCREEN     ? "No"       : "Yes",
           SDL_GetCurrentVideoDriver(), rend_info.name);
    }

#if defined(PLAT_MACOS) && defined(USE_SDL2)
    /* -------------------------------------------------------------------- */
    /*  SDL2 2.0.12 and prior do not set a colorspace.  This occasionally   */
    /*  causes problems with dragging a window between desktops on OS/X.    */
    /* -------------------------------------------------------------------- */
    void *metal_layer = SDL_RenderGetMetalLayer(rend);
    if (metal_layer &&
        gfx_set_srgb_colorspace(metal_layer))
    {
        jzp_printf("gfx:  Manually set sRGB colorspace on Metal layer.\n");
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  TEMPORARY: Verify that the surface's format is as we expect.  This  */
    /*  is just a temporary bit of paranoia to ensure that scr->pixels      */
    /*  is in the format I _think_ it's in.                                 */
    /* -------------------------------------------------------------------- */
#if 0
    if ((tgt_wind_bpp == 8 && (SDL_BITSPERPIXEL(text_pix_fmt)  !=  8   ||
                               SDL_BYTESPERPIXEL(text_pix_fmt) !=  1))   ||
        (tgt_wind_bpp ==16 && (SDL_BITSPERPIXEL(text_pix_fmt)  != 16   ||
                               SDL_BYTESPERPIXEL(text_pix_fmt) !=  2))   ||
        (tgt_wind_bpp ==32 && (SDL_BITSPERPIXEL(text_pix_fmt)  != 32   ||
                               SDL_BYTESPERPIXEL(text_pix_fmt) !=  4)))
    {
        fprintf(stderr,"gfx panic: BitsPerPixel = %d, BytesPerPixel = %d\n",
                SDL_BITSPERPIXEL(text_pix_fmt),
                SDL_BYTESPERPIXEL(text_pix_fmt));
        return -1;
    }
#else
    if (tgt_wind_bpp ==32 && SDL_BYTESPERPIXEL(text_pix_fmt) != 4)
    {
        fprintf(stderr,"gfx panic: BitsPerPixel = %d, BytesPerPixel = %d\n",
                SDL_BITSPERPIXEL(text_pix_fmt),
                SDL_BYTESPERPIXEL(text_pix_fmt));
        return -1;
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  New surface will may need palette initialization.                   */
    /* -------------------------------------------------------------------- */
    if (gfx->pvt->bpp != 32)
    {
        fprintf(stderr, "gfx panic: SDL2 is 32bpp only for now.\n");
        return -1;
    } else
    {
        gfx_set_scaler_palette(&gfx->pvt->scaler,
                                gfx->pvt->pixf,
                                gfx->pvt->vid_enable ? gfx->pvt->pal_on
                                                     : gfx->pvt->pal_off);
    }

    /* -------------------------------------------------------------------- */
    /*  Start the display off as "completely dirty."                        */
    /* -------------------------------------------------------------------- */
    gfx->dirty = 3;
    gfx->b_dirty = 3;

    /* -------------------------------------------------------------------- */
    /*  Hide the mouse if full screen.                                      */
    /* -------------------------------------------------------------------- */
    SDL_ShowCursor(
        SDL_GetNumVideoDisplays() == 1 &&
        (act_wind_flags & SDL_WINDOW_FULLSCREEN) ? SDL_DISABLE : SDL_ENABLE);

    SDL_PumpEvents();
    SDL_ShowWindow(gfx->pvt->wind);
    SDL_RaiseWindow(gfx->pvt->wind);

    if (!(act_wind_flags & SDL_WINDOW_FULLSCREEN))
        SDL_SetWindowBordered(gfx->pvt->wind, SDL_TRUE);

    return 0;
}

#ifdef BENCHMARK_GFX
LOCAL int dr_hist[244];   /* histogram of number of dirty rects   */
LOCAL int drw_hist[21];   /* histogram of dirty rectangle widths  */

LOCAL void gfx_dr_hist_dump(void);
#endif

/* ======================================================================== */
/*  GFX_CHECK        -- Validates gfx parameters                            */
/* ======================================================================== */
int gfx_check(int desire_x, int desire_y, int desire_bpp, int prescaler)
{
    int i;

    desire_bpp = 32;  // TODO:  Support other bit depths eventually?

    if (desire_x < 320)
    {
        fprintf(stderr, "Minimum X resolution is 320\n");
        return -1;
    }

    if (desire_y < 200)
    {
        fprintf(stderr, "Minimum Y resolution is 200\n");
        return -1;
    }

    if (!(desire_bpp == 8 || desire_bpp == 16 ||
          desire_bpp == 24 || desire_bpp == 32))
    {
        fprintf(stderr, "Bits per pixel must be 8, 16, 24 or 32\n");
        return -1;
    }

    if (prescaler < 0 || prescaler > gfx_prescaler_registry_size)
    {
        if (prescaler > gfx_prescaler_registry_size)
        {
            fprintf(stderr, "gfx:  Prescaler number %d out of range\n",
                    prescaler);
        }
        fprintf(stderr, "Supported prescalers:\n");

        for (i = 0; i < gfx_prescaler_registry_size; i++)
            jzp_printf("    %d: %s\n", i, gfx_prescaler_registry[i].name);

        return -1;
    }

    return 0;
}

/* ======================================================================== */
/*  GFX_FLIP         -- Copy the texture to the display.                    */
/* ======================================================================== */

extern void manage_onscreen_controls(SDL_Renderer* renderer);
extern void update_jzintv_rendering_rect(const SDL_Rect* rect);
LOCAL int gfx_flip(const gfx_t *const gfx)
{
    const gfx_pvt_t *const pvt = gfx->pvt;
    SDL_Renderer *const rend = pvt->rend;
    SDL_Texture *const text = pvt->text;
    const SDL_Color bord_color = pvt->pal_on[gfx->b_color];
    const SDL_Rect dest = {
        .x = pvt->ofs_x, .y = pvt->ofs_y,
        .w = pvt->scaler.actual_x, .h = pvt->scaler.actual_y
    };

    update_jzintv_rendering_rect(&dest);

    /* -------------------------------------------------------------------- */
    /*  The docs for SDL_RenderPresent indicate that the back-buffer        */
    /*  contents may not be preserved between calls to Present.  Thus, we   */
    /*  need to clear the backdrop to our border color before copying the   */
    /*  texture to the display.                                             */
    /* -------------------------------------------------------------------- */
    // I love black
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderClear(rend);
    if (pvt->vid_enable || gfx->debug_blank)
        SDL_RenderCopy(rend, text, NULL,
                       gfx->scrshot & GFX_RESET ? NULL : &dest);

    manage_onscreen_controls(rend);

    SDL_RenderPresent(rend);

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
    int  inter_x = 160, inter_y = 200;
    int  i, need_inter_vid = 0;
    void *prescaler_opaque;
    gfx_dirtyrect_spec dr_spec;

    /* -------------------------------------------------------------------- */
    /*  Set up prescaler (ie. Scale2X/3X/4X or similar)                     */
    /* -------------------------------------------------------------------- */
    if (prescaler > 0)
    {
        jzp_printf("gfx:  Configuring prescaler %s\n",
                    gfx_prescaler_registry[prescaler].name);
    }

    prescaler_opaque = gfx_prescaler_registry[prescaler].prescaler_init
                       (
                            160,      200,
                            &inter_x, &inter_y, &need_inter_vid,
                            &dr_spec
                       );

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

    if (gfx->pvt)
    {
        int dr_count, dr_x_dim, dr_y_dim;

        dr_x_dim = (dr_spec.active_last_x - dr_spec.active_first_x + 1);
        dr_y_dim = (dr_spec.active_last_y - dr_spec.active_first_y + 1);

        dr_count = ((dr_x_dim + dr_spec.x_step - 1) / dr_spec.x_step) *
                   ((dr_y_dim + dr_spec.y_step - 1) / dr_spec.y_step);

        jzp_printf("active x: %d, %d, %d active y: %d %d, %d\n",
            dr_spec.active_first_x, dr_spec.active_last_x, dr_spec.x_step,
            dr_spec.active_first_y, dr_spec.active_last_y, dr_spec.y_step);

        if (need_inter_vid)
            gfx->pvt->inter_vid = CALLOC(uint8_t, inter_x * inter_y);
        else
            gfx->pvt->inter_vid = gfx->vid;

        gfx->pvt->prescaler = gfx_prescaler_registry[prescaler].prescaler;
        gfx->pvt->ps_opaque = prescaler_opaque;
        gfx->pvt->ps_dtor   = gfx_prescaler_registry[prescaler].prescaler_dtor;

        gfx->pvt->prev          = CALLOC(uint8_t,   inter_x * inter_y);
        gfx->pvt->dirty_rects   = CALLOC(SDL_Rect, dr_count);

        gfx->pvt->dirty_rows    = CALLOC(uint32_t,  ((inter_y+31) >> 5));
        gfx->pvt->dirty_rows_sz = 4 * ((inter_y+31) >> 5);

        gfx->pvt->dr_spec       = dr_spec;

        gfx->pvt->border_x      = border_x;
        gfx->pvt->border_y      = border_y;
    }

    if (!gfx->vid || !gfx->pvt || !gfx->pvt->prev || !gfx->pvt->dirty_rows ||
        !gfx->pvt->dirty_rects || !gfx->pvt->inter_vid)
    {

        fprintf(stderr, "gfx:  Panic:  Could not allocate memory.\n");

        goto die;
    }

    /* -------------------------------------------------------------------- */
    /*  Configure the scaler.                                               */
    /* -------------------------------------------------------------------- */
    //if (desire_bpp == 24)
        desire_bpp = 32;  // TODO: Support other bit-depths someday?

    if (gfx_scale_init_spec(&(gfx->pvt->scaler),
                             inter_x,  inter_y,
                             desire_x, desire_y, desire_bpp))
    {
        fprintf(stderr,
                "Could not configure scaler for %d x %d @ %d bpp\n",
                desire_x, desire_y, desire_bpp);
        goto die;
    }

    /* -------------------------------------------------------------------- */
    /*  Set up our color palette.  We start with video blanked.             */
    /* -------------------------------------------------------------------- */
    gfx->palette = *palette;
    for (i = 0; i < 16; i++)
    {
        gfx->pvt->pal_on [i].r = palette->color[i][0];
        gfx->pvt->pal_on [i].g = palette->color[i][1];
        gfx->pvt->pal_on [i].b = palette->color[i][2];
        gfx->pvt->pal_off[i].r = palette->color[i][0] >> 1;
        gfx->pvt->pal_off[i].g = palette->color[i][1] >> 1;
        gfx->pvt->pal_off[i].b = palette->color[i][2] >> 1;
    }
    for (i = 16; i < 32; i++)
    {
        gfx->pvt->pal_on [i].r = util_palette.color[i - 16][0];
        gfx->pvt->pal_on [i].g = util_palette.color[i - 16][1];
        gfx->pvt->pal_on [i].b = util_palette.color[i - 16][2];
        gfx->pvt->pal_off[i].r = util_palette.color[i - 16][0];
        gfx->pvt->pal_off[i].g = util_palette.color[i - 16][1];
        gfx->pvt->pal_off[i].b = util_palette.color[i - 16][2];
    }
    gfx->pvt->vid_enable = 0;
    gfx->dirty      = 3;
    gfx->b_dirty    = 3;

    gfx->fps        = pal_mode ? 50 : 60;
    gfx->avi        = avi;
    gfx->audio_rate = audio_rate;  // ugh

    /* -------------------------------------------------------------------- */
    /*  Set up initial graphics mode.                                       */
    /* -------------------------------------------------------------------- */
    if (gfx_setup_sdl_display(gfx, flags, !verbose) < 0)
        gfx_sdl_abort("Could not initialize video display");

    /* -------------------------------------------------------------------- */
    /*  Ok, see if we succeeded in setting our initial video mode, and do   */
    /*  some minor tidying.                                                 */
    /* -------------------------------------------------------------------- */
    gfx_flip(gfx);

    /* -------------------------------------------------------------------- */
    /*  Set up the gfx_t's internal structures.                             */
    /* -------------------------------------------------------------------- */
    gfx->periph.read        = NULL;
    gfx->periph.write       = NULL;
    gfx->periph.peek        = NULL;
    gfx->periph.poke        = NULL;
    gfx->periph.tick        = NULL;  /* STIC ticks us directly */
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
    if (gfx->pvt)
    {
        CONDFREE(gfx->pvt->dirty_rows);
        CONDFREE(gfx->pvt->dirty_rects);
        CONDFREE(gfx->pvt->prev);
        if (gfx->pvt->inter_vid != gfx->vid) CONDFREE(gfx->pvt->inter_vid);
    }
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

    if (gfx->pvt)
    {
        gfx_teardown_sdl_display(gfx);


        /* destruct the prescaler;
           prescaler should also free opaque struct if needed */
        if (gfx->pvt->ps_dtor)
            gfx->pvt->ps_dtor(gfx->pvt->ps_opaque);

        /* destruct the scaler */
        gfx_scale_dtor(&(gfx->pvt->scaler));

        CONDFREE(gfx->pvt->dirty_rows);
        CONDFREE(gfx->pvt->dirty_rects);
        CONDFREE(gfx->pvt->prev);
        if (gfx->pvt->inter_vid != gfx->vid) CONDFREE(gfx->pvt->inter_vid);
    }
    CONDFREE(gfx->pvt);
    CONDFREE(gfx->vid);
}

/* ======================================================================== */
/*  GFX_TOGGLE_WINDOWED -- Try to toggle windowed vs. full-screen.          */
/* ======================================================================== */
bool gfx_toggle_windowed(gfx_t *gfx, int quiet)
{
    gfx_pvt_t *const pvt = gfx->pvt;
    const uint32_t new_flags = pvt->flags ^ GFX_FULLSC;
    const uint32_t wind_flags =
        new_flags & GFX_FULLSC ? SDL_WINDOW_FULLSCREEN : 0;
    const bool new_fullsc = new_flags != 0;
    const int toggle = gfx->toggle;
    bool flipped = false;
    gfx->toggle = 0;

    switch (toggle)
    {
        case GFX_WIND_TOG: break;
        case GFX_WIND_ON:  if (new_fullsc == true ) return false; else break;
        case GFX_WIND_OFF: if (new_fullsc == false) return false; else break;
    }

    if (!quiet)
        jzp_printf("\n");

    if (SDL_SetWindowFullscreen(pvt->wind, wind_flags) == 0)
    {
        flipped = true;
        gfx->req_pause = true;
        pvt->flags = new_flags;

        SDL_ShowCursor(
            SDL_GetNumVideoDisplays() == 1 &&
            new_fullsc ? SDL_DISABLE : SDL_ENABLE);

        SDL_PumpEvents();
        SDL_ShowWindow(pvt->wind);
        SDL_RaiseWindow(pvt->wind);
        if (!new_fullsc)    /* Needed on w32 SDL2, apparently? */
            SDL_SetWindowBordered(pvt->wind, SDL_TRUE);

        const int text_x = pvt->scaler.actual_x;
        const int text_y = pvt->scaler.actual_y;
        int wind_x, wind_y;

        SDL_GetWindowSize(pvt->wind, &wind_x, &wind_y);

        pvt->dim_x = wind_x;
        pvt->dim_y = wind_y;
        pvt->ofs_x = ((wind_x - text_x) >> 1) & (~3);
        pvt->ofs_y =  (wind_y - text_y) >> 1;
    }

    gfx->b_dirty |= 2;
    gfx->dirty   |= 2;
    gfx->drop_frame = 0;
    return flipped;
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
    SDL_Window *const wind = gfx->pvt->wind;
    SDL_SetWindowTitle(wind, title);
    return 0;
}

/* ======================================================================== */
/*  GFX_REFRESH      -- Core graphics refresh.                              */
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
#if 0
        jzp_printf("Dropped %d frames.\n", gfx->dropped_frames);
        jzp_flush();
#endif
        gfx->tot_dropped_frames += gfx->dropped_frames;
        gfx->dropped_frames = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Update the palette if there's been a change in blanking state or    */
    /*  border color.                                                       */
    /* -------------------------------------------------------------------- */
    if ((gfx->pvt->vid_enable & 2) || gfx->b_dirty)
    {
        if (gfx->pvt->vid_enable & 2)
        {
            gfx->pvt->vid_enable &= 1;
            gfx->pvt->vid_enable ^= 1;
            gfx->b_dirty = 3;
        }

        gfx_set_scaler_palette(&gfx->pvt->scaler,
                                gfx->pvt->pixf,
                                gfx->pvt->vid_enable ? gfx->pvt->pal_on
                                                     : gfx->pvt->pal_off);

        gfx->dirty |= 2;
    }

    /* -------------------------------------------------------------------- */
    /*  If dirty-rectangle disabled, force a dirty frame to a full flip.    */
    /* -------------------------------------------------------------------- */
    if ((gfx->pvt->flags & GFX_DRECTS) == 0 &&
        (gfx->dirty || gfx->b_dirty))
    {
        gfx->dirty |= 3;
    }

    /* -------------------------------------------------------------------- */
    /*  Run the prescaler if any part of the frame is dirty.                */
    /* -------------------------------------------------------------------- */
    if (gfx->dirty)
        gfx->pvt->prescaler(gfx->vid, gfx->pvt->inter_vid, gfx->pvt->ps_opaque);

    /* -------------------------------------------------------------------- */
    /*  Push whole frame if dirty == 2, else do dirty-rectangle update.     */
    /* -------------------------------------------------------------------- */
    if (gfx->dirty >= 2)
    {
        memset(gfx->pvt->dirty_rows, 0xFF, gfx->pvt->dirty_rows_sz);
        gfx_tick(gfx);
        gfx_flip(gfx);
    } else if (gfx->dirty || gfx->b_dirty)
    {
        gfx_find_dirty_rects(gfx);

        if (gfx->pvt->num_rects > 0)
            gfx_tick(gfx);

        if (gfx->pvt->num_rects > 0 || gfx->b_dirty)
            gfx_flip(gfx);
    }

    gfx->dirty = 0;
    gfx->b_dirty = 0;
}

#ifdef BENCHMARK_GFX
LOCAL double bm_max = 0, bm_min = 1e30, bm_tot = 0;
LOCAL int bm_cnt = 0;
#endif
/* ======================================================================== */
/*  GFX_STIC_TICK    -- Get ticked directly by STIC to fix gfx pipeline.    */
/* ======================================================================== */
void gfx_stic_tick(gfx_t *const gfx)
{
#ifdef BENCHMARK_GFX
    double start, end, diff;

    start = get_time();
#endif


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
    /*  Toggle full-screen/windowed if requested.  Pause for a short time   */
    /*  if we do toggle between windowed and full-screen.                   */
    /* -------------------------------------------------------------------- */
    if (gfx->toggle)
        gfx_toggle_windowed(gfx, 0);

    /* -------------------------------------------------------------------- */
    /*  If we've been asked to drop 'extra' frames (ie. limit to max 60Hz   */
    /*  according to wall-clock), do so.                                    */
    /* -------------------------------------------------------------------- */
    if (gfx->dirty && !gfx->drop_frame &&
        (gfx->pvt->flags & GFX_SKIP_EXTRA) != 0)
    {
        const double now  = get_time();
        const double elapsed = now - gfx->pvt->last_frame;

        if (elapsed < frame_delta)
        {
            gfx->drop_frame = 1;
            gfx->dropped_frames--;  /* Don't count this dropped frame */
        }
    }

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

    /* -------------------------------------------------------------------- */
    /*  If rate-limiting our display, record the time /after/ the flip, as  */
    /*  some OSes *cough*OSX*cough* wait for vertical retrace, while other  */
    /*  OSes *cough*Linux*cough* do not.                                    */
    /* -------------------------------------------------------------------- */
    if (gfx->pvt->flags & GFX_SKIP_EXTRA)
    {
        gfx->pvt->last_frame = get_time();
    }

#ifdef BENCHMARK_GFX
    end = get_time();
    diff = end - start;
    if (diff > bm_max) bm_max = diff;
    if (diff < bm_min) bm_min = diff;
    bm_tot += diff;

    if (++bm_cnt == 120)
    {
        jzp_printf("gfx_tick: min = %8.3f max = %8.3f avg = %8.3f\n",
                   bm_min * 1000., bm_max * 1000., bm_tot * 1000. / 120);
        bm_max = bm_tot = 0;
        bm_cnt = 0;
        bm_min = 1e30;
    }
#endif
}

/* ======================================================================== */
/*  GFX_TICK         -- Services a gfx_t tick in any graphics format        */
/* ======================================================================== */
LOCAL void gfx_tick(gfx_t *gfx)
{
    void    *pixels;
    int      pitch;

    if (SDL_LockTexture(gfx->pvt->text, NULL, &pixels, &pitch))
        gfx_sdl_abort("Could not lock texture");

    gfx_scale
    (
        &gfx->pvt->scaler,
        gfx->pvt->inter_vid,
        (uint8_t *)pixels, pitch,
        gfx->pvt->dirty_rows
    );

    SDL_UnlockTexture(gfx->pvt->text);
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
    if (gfx->b_color != b_color)
    {
        gfx->b_color = b_color;
        gfx->b_dirty = 1;
    }

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

    if (dirty)     { gfx->dirty   |= 1; }
    if (dirty & 2) { gfx->b_dirty |= 2; }
}

/* ======================================================================== */
/*  GFX_FIND_DIRTY_RECTS -- Finds dirty rectangles in the current image.    */
/*                                                                          */
/*  Current algorithm just divides the display into 240 8x16 tiles aligned  */
/*  with the STIC's cards.  A tile is considered either clean or dirty      */
/*  in its entirety for now.  A tile can be merged with tiles to its        */
/*  right if they're contiguous, or there's a gap of at most one tile.      */
/*                                                                          */
/*  The algorithm is also responsible for copying the new image into the    */
/*  reference image, and constructing a bitmap of which rows need to be     */
/*  expanded by the scaler code.                                            */
/* ======================================================================== */
LOCAL void gfx_find_dirty_rects(gfx_t *gfx)
{
    int x, y, xx, yy, i, j, t;
    int nr = 0, row_start;
    uint32_t *RESTRICT old_pix = (uint32_t *)(void *)gfx->pvt->prev;
    uint32_t *RESTRICT new_pix = (uint32_t *)(void *)gfx->pvt->inter_vid;
    uint32_t is_dirty;
    SDL_Rect *rect = gfx->pvt->dirty_rects;

    int wpitch = gfx->pvt->dr_spec.pitch >> 2;
    int y0 = gfx->pvt->dr_spec.active_first_y;
    int y1 = gfx->pvt->dr_spec.active_last_y + 1;
    int ys = gfx->pvt->dr_spec.y_step;

    int x0 = (gfx->pvt->dr_spec.active_first_x >> 3);
    int x1 = (gfx->pvt->dr_spec.active_last_x  >> 3) + 1;
    int xs = (gfx->pvt->dr_spec.x_step         >> 3);

    int bo = (gfx->pvt->dr_spec.bord_first_x >> 2) +
             (gfx->pvt->dr_spec.bord_first_y * wpitch);

    /* -------------------------------------------------------------------- */
    /*  Set our merge threshold based on whether we're allowed to include   */
    /*  a clean rectangle between two dirty rectangles when coalescing.     */
    /* -------------------------------------------------------------------- */
    t = gfx->pvt->flags & GFX_DRCMRG ? 1 : 0;

    /* -------------------------------------------------------------------- */
    /*  Initally mark all rows clean.                                       */
    /* -------------------------------------------------------------------- */
    memset((void *)gfx->pvt->dirty_rows, 0, gfx->pvt->dirty_rows_sz);

    /* -------------------------------------------------------------------- */
    /*  Scan the source image tile-row-wise looking for differences.        */
    /* -------------------------------------------------------------------- */
    for (y = y0; y < y1; y += ys)
    {
        row_start = nr;

        /* ---------------------------------------------------------------- */
        /*  Find dirty rectangles in this row of cards.                     */
        /* ---------------------------------------------------------------- */
        for (x  = x0; x < x1; x += xs)
        {
            is_dirty = 0;
            switch (xs)
            {
                case 1:
                {
                    for (yy = y; yy < y + ys; yy++)
                        is_dirty  |= (old_pix[yy * wpitch + x*2 + 0] !=
                                      new_pix[yy * wpitch + x*2 + 0])
                                  |  (old_pix[yy * wpitch + x*2 + 1] !=
                                      new_pix[yy * wpitch + x*2 + 1]);
                    break;
                }

                case 2:
                {
                    for (yy = y; yy < y + ys; yy++)
                        is_dirty  |= (old_pix[yy * wpitch + x*2 + 0] !=
                                      new_pix[yy * wpitch + x*2 + 0])
                                  |  (old_pix[yy * wpitch + x*2 + 1] !=
                                      new_pix[yy * wpitch + x*2 + 1])
                                  |  (old_pix[yy * wpitch + x*2 + 2] !=
                                      new_pix[yy * wpitch + x*2 + 2])
                                  |  (old_pix[yy * wpitch + x*2 + 3] !=
                                      new_pix[yy * wpitch + x*2 + 3]);
                    break;
                }

                case 3:
                {
                    for (yy = y; yy < y + ys; yy++)
                        is_dirty  |= (old_pix[yy * wpitch + x*2 + 0] !=
                                      new_pix[yy * wpitch + x*2 + 0])
                                  |  (old_pix[yy * wpitch + x*2 + 1] !=
                                      new_pix[yy * wpitch + x*2 + 1])
                                  |  (old_pix[yy * wpitch + x*2 + 2] !=
                                      new_pix[yy * wpitch + x*2 + 2])
                                  |  (old_pix[yy * wpitch + x*2 + 3] !=
                                      new_pix[yy * wpitch + x*2 + 3])
                                  |  (old_pix[yy * wpitch + x*2 + 4] !=
                                      new_pix[yy * wpitch + x*2 + 4])
                                  |  (old_pix[yy * wpitch + x*2 + 5] !=
                                      new_pix[yy * wpitch + x*2 + 5]);
                    break;
                }

                case 4:
                {
                    for (yy = y; yy < y + ys; yy++)
                        is_dirty  |= (old_pix[yy * wpitch + x*2 + 0] !=
                                      new_pix[yy * wpitch + x*2 + 0])
                                  |  (old_pix[yy * wpitch + x*2 + 1] !=
                                      new_pix[yy * wpitch + x*2 + 1])
                                  |  (old_pix[yy * wpitch + x*2 + 2] !=
                                      new_pix[yy * wpitch + x*2 + 2])
                                  |  (old_pix[yy * wpitch + x*2 + 3] !=
                                      new_pix[yy * wpitch + x*2 + 3])
                                  |  (old_pix[yy * wpitch + x*2 + 4] !=
                                      new_pix[yy * wpitch + x*2 + 4])
                                  |  (old_pix[yy * wpitch + x*2 + 5] !=
                                      new_pix[yy * wpitch + x*2 + 5])
                                  |  (old_pix[yy * wpitch + x*2 + 6] !=
                                      new_pix[yy * wpitch + x*2 + 6])
                                  |  (old_pix[yy * wpitch + x*2 + 7] !=
                                      new_pix[yy * wpitch + x*2 + 7]);
                    break;
                }

                default:
                {
                    for (yy = y; yy < y + ys; yy++)
                        for (xx = x; xx < x + xs; xx++)
                            is_dirty |= (old_pix[yy * wpitch + xx*2 + 0] !=
                                         new_pix[yy * wpitch + xx*2 + 0])
                                     |  (old_pix[yy * wpitch + xx*2 + 1] !=
                                         new_pix[yy * wpitch + xx*2 + 1]);

                    break;
                }
            }

            if (is_dirty)
            {
                rect[nr].x = x;
                rect[nr].y = y;
                rect[nr].w = xs;
                rect[nr].h = ys;
                nr++;
            }
/*fprintf(stderr, "%3d %3d %3d\n", x, y, nr); */
        }

        /* ---------------------------------------------------------------- */
        /*  While it's still hot in the cache, copy "new" to "old"          */
        /* ---------------------------------------------------------------- */
        memcpy((void *)&old_pix[y * wpitch],
               (void *)&new_pix[y * wpitch],
               sizeof(uint32_t) * wpitch * ys);

        /* ---------------------------------------------------------------- */
        /*  Mark these rows as dirty in the dirty_row bitmap                */
        /* ---------------------------------------------------------------- */
        if (nr > row_start)
            for (yy = y; yy < y + ys; yy++)
                gfx->pvt->dirty_rows[yy >> 5] |= 1u << (yy & 31);

        /* ---------------------------------------------------------------- */
        /*  Coalesce rectangles if they're adjacent or separated by at      */
        /*  most one clean rectangle.                                       */
        /* ---------------------------------------------------------------- */
        if (nr - row_start < 2)
            continue;

        for (i = row_start, j = row_start + 1; j < nr; j++)
        {
            if (rect[i].x + rect[i].w + t >= rect[j].x)
            {
                rect[i].w = rect[j].x - rect[i].x + rect[j].w;
                continue;
            } else
            {
                rect[++i] = rect[j];
            }
        }

        nr = i + 1;
    }

    /* -------------------------------------------------------------------- */
    /*  If border areas changed color, update those too.                    */
    /*  XXX:  This needs to get fixed when I fix scaler's border handler.   */
    /* -------------------------------------------------------------------- */
    if (old_pix[bo] != new_pix[bo])
    {
        int x0l, x0h, y0l, y0h;     /* upper rectangle */
        int x1l, x1h, y1l, y1h;     /* lower rectangle */

        old_pix[bo] =  new_pix[bo];

        x0l = x1l = gfx->pvt->dr_spec.bord_first_x >> 3;    /* in dwords */
        x0h = x1h = gfx->pvt->dr_spec.bord_last_x  >> 3;    /* in dwords */

        y0l = gfx->pvt->dr_spec.bord_first_y;               /* in pixels */
        y0h = gfx->pvt->dr_spec.active_first_y - 1;         /* in pixels */

        y1l = gfx->pvt->dr_spec.active_last_y + 1;          /* in pixels */
        y1h = gfx->pvt->dr_spec.bord_last_y;                /* in pixels */

        rect[nr].x = x0l;
        rect[nr].y = y0l;
        rect[nr].w = x0h - x0l + 1;
        rect[nr].h = y0h - y0l + 1;
        nr++;

        rect[nr].x = x1l;
        rect[nr].y = y1l;
        rect[nr].w = x1h - x1l + 1;
        rect[nr].h = y1h - y1l + 1;
        nr++;

        for (yy = y0l; yy <= y0h; yy++)
            gfx->pvt->dirty_rows[yy >> 5] |= 1u << (yy & 31);

        for (yy = y1l; yy <= y1h; yy++)
            gfx->pvt->dirty_rows[yy >> 5] |= 1u << (yy & 31);
    }

    /* -------------------------------------------------------------------- */
    /*  Convert the rectangles to display coordinates.  Ick.                */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < nr; i++)
    {
        int w, h;
#ifdef BENCHMARK_GFX
        drw_hist[rect[i].w]++;
#endif
        x = rect[i].x * 8;
        y = rect[i].y;
        w = rect[i].w * 8;
        h = rect[i].h;

        rect[i].x  = gfx->pvt->scaler.scaled_x[x];
        rect[i].y  = gfx->pvt->scaler.scaled_y[y];
        rect[i].w  = gfx->pvt->scaler.scaled_x[x + w] - rect[i].x;
        rect[i].h  = gfx->pvt->scaler.scaled_y[y + h] - rect[i].y;

        rect[i].x += gfx->pvt->ofs_x;
        rect[i].y += gfx->pvt->ofs_y;
    }

    gfx->pvt->num_rects = nr;

#ifdef BENCHMARK_GFX
    dr_hist[nr]++;
#endif

    return;
}

#ifdef BENCHMARK_GFX
LOCAL void gfx_dr_hist_dump(void)
{
    int i;

    jzp_printf("Dirty rectangle counts:\n");
    for (i = 0; i <= 244; i++)
        if (dr_hist[i])
            jzp_printf("%4d: %7d\n", i, dr_hist[i]);

    jzp_printf("Dirty rectangle width counts:\n");
    for (i = 0; i <= 20; i++)
        if (drw_hist[i])
            jzp_printf("%4d: %7d\n", i, drw_hist[i]);
}
#endif

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
/*          Copyright (c) 1998-2020, Joseph Zbiciak, John Tanner            */
/* ======================================================================== */
