/*
 * ============================================================================
 *  Title:    Common Graphics Subsystem Routines
 *  Author:   J. Zbiciak
 * ============================================================================
 *  GFX_SCRSHOT      -- Take a screenshot.
 *  GFX_MOVIEUPD     -- Update a MVI file.
 *  GFX_AVIUPD       -- Update an AVI file.
 *  GFX_RESYNC       -- Tell gfx we need to refresh everything.
 *  GFX_HIDDEN       -- Returns whether we think we're hidden.
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "gfx/gfx.h"
#include "mvi/mvi.h"
#include "avi/avi.h"
#include "gif/gif_enc.h"
#include "lzoe/lzoe.h"
#include "file/file.h"

extern void manage_screenshot_file(char *filename);

/* ======================================================================== */
/*  GFX_SCRSHOT      -- Write a 320x200 screen shot to a GIF file.          */
/* ======================================================================== */
void gfx_scrshot(const gfx_t *const gfx)
{
    const uint8_t *const scr = gfx->vid;
    const palette_t *const palette = &gfx->palette;
    static uint8_t scrshot_buf[320*200];
    static unique_filename_t shot_file_tmpl =
    {
        "shot", ".gif", NULL, 0, 4, 0
    };
    int i, len;

    /* -------------------------------------------------------------------- */
    /*  Open a unique file for the screenshot.                              */
    /* -------------------------------------------------------------------- */
    FILE *f = open_unique_filename(&shot_file_tmpl);

    if (!f)
    {
        fprintf(stderr, "Error:  Could not open '%s' for screen dump.\n",
                shot_file_tmpl.f_name);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Do the screen dump.  Write it as a nice GIF.  We need to pixel      */
    /*  double the image ahead of time.                                     */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 200*160; i++)
        scrshot_buf[i*2 + 0] = scrshot_buf[i*2 + 1] = scr[i];

    len = gif_write(f, scrshot_buf, 320, 200, (gif_pal_t)palette->color, 16);
    if (len > 0)
    {
        jzp_printf("\nWrote screen shot to '%s', %d bytes\n",
                   shot_file_tmpl.f_name, len);
    } else
    {
        jzp_printf("\nError writing screen shot to '%s'\n",
                   shot_file_tmpl.f_name);
    }
    jzp_flush();
    fclose(f);
    manage_screenshot_file(shot_file_tmpl.f_name);
    return;
}

/* ======================================================================== */
/*  GFX_MOVIEUPD     -- Start/Stop/Update a movie in progress               */
/* ======================================================================== */
void gfx_movieupd(gfx_t *const gfx)
{
    /* -------------------------------------------------------------------- */
    /*  Toggle current movie state if user requested.                       */
    /* -------------------------------------------------------------------- */
    if (gfx->scrshot & GFX_MVTOG)
    {
        static unique_filename_t mvi_file_tmpl = 
        {
            "mvi_", ".imv", NULL, 0, 4, 0
        };

        /* ---------------------------------------------------------------- */
        /*  Whatever happens, clear the toggle.                             */
        /* ---------------------------------------------------------------- */
        gfx->scrshot &= ~GFX_MVTOG;

        /* ---------------------------------------------------------------- */
        /*  Make sure movie subsystem initialized.  We only init this if    */
        /*  someone tries to take a movie.                                  */
        /* ---------------------------------------------------------------- */
        if (!gfx->movie_init)
        {
            if (!gfx->movie) gfx->movie = CALLOC(mvi_t, 1);
            if (!gfx->movie)
            {
                fprintf(stderr, "No memory for movie structure\n");
                return;
            }

            mvi_init(gfx->movie, 160, 200);
            gfx->movie_init = 1;
        }

        /* ---------------------------------------------------------------- */
        /*  If a movie's open, close it.                                    */
        /* ---------------------------------------------------------------- */
        if ((gfx->scrshot & GFX_MOVIE) != 0)
        {
            if (gfx->movie->f)
            {
                fclose(gfx->movie->f);
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
                       gfx->movie->fr,
                       gfx->movie->tot_bytes,
                       gfx->movie->tot_bytes / gfx->movie->fr,
#ifndef NO_LZO
                       gfx->movie->tot_lzosave,
#endif
                       gfx->movie->rpt_frames,
                       gfx->movie->rpt_rows,
                       (16032.*gfx->movie->fr) / gfx->movie->tot_bytes);
                jzp_flush();
            }

            gfx->scrshot &= ~GFX_MOVIE;
            gfx->movie->f  = NULL;
            gfx->movie->fr = 0;

            return;
        }

        /* ---------------------------------------------------------------- */
        /*  Otherwise, open a new movie.                                    */
        /* ---------------------------------------------------------------- */
        gfx->movie->f = open_unique_filename(&mvi_file_tmpl);
        if (!gfx->movie->f)
        {
            fprintf(stderr, "Error:  Could not open '%s' for movie.\n",
                    mvi_file_tmpl.f_name);
            return;
        }

        jzp_printf("\nStarted movie file '%s'\n", mvi_file_tmpl.f_name);
        jzp_flush();

        /* ---------------------------------------------------------------- */
        /*  Success:  Turn on the movie.                                    */
        /* ---------------------------------------------------------------- */
        gfx->scrshot |= GFX_MOVIE;
        gfx->movie->fr = 0;
    }

    if ((gfx->scrshot & GFX_RESET) == 0)
        mvi_wr_frame(gfx->movie, gfx->vid, gfx->bbox);
}

/* ======================================================================== */
/*  GFX_AVIUPD       -- Start/Stop/Update an AVI in progress                */
/* ======================================================================== */
void gfx_aviupd(gfx_t *const gfx)
{
    avi_writer_t *avi = gfx->avi;

    /* -------------------------------------------------------------------- */
    /*  Toggle current AVI state if user requested.                         */
    /* -------------------------------------------------------------------- */
    if ((gfx->scrshot & (GFX_AVTOG | GFX_RESET)) == GFX_AVTOG)
    {
        /* ---------------------------------------------------------------- */
        /*  Whatever happens, clear the toggle.                             */
        /* ---------------------------------------------------------------- */
        gfx->scrshot &= ~GFX_AVTOG;

        /* ---------------------------------------------------------------- */
        /*  If a AVI's open, close it.                                      */
        /* ---------------------------------------------------------------- */
        if ((gfx->scrshot & GFX_AVI) != 0)
        {
            if (avi_is_active(avi))
            {
                const avi_info_t *info = avi_info(avi);
                avi_end_video(avi);     // does not invalidate 'info'

                jzp_printf("\nDone writing AVI\n"
                       "    Total frames:        %10d\n",
                       info->total_frames);
                jzp_flush();
            }

            gfx->scrshot &= ~GFX_AVI;
            return;
        }

        /* ---------------------------------------------------------------- */
        /*  Open a unique file for the AVI.                                 */
        /* ---------------------------------------------------------------- */
        static unique_filename_t avi_file_tmpl =
        {
            "avi_", ".avi", NULL, 0, 4, 0
        };
        FILE *avi_file;

        avi_file = open_unique_filename(&avi_file_tmpl);

        if (!avi_file)
        {
            fprintf(stderr, "Error:  Could not open '%s' for AVI.\n",
                    avi_file_tmpl.f_name);
            return;
        }

        jzp_printf("\nStarted AVI file '%s'\n", avi_file_tmpl.f_name);
        jzp_flush();

        /* ---------------------------------------------------------------- */
        /*  Success:  Turn on the movie.                                    */
        /* ---------------------------------------------------------------- */
        gfx->scrshot |= GFX_AVI;
        avi_start_video(avi, avi_file, gfx->fps, gfx->audio_rate, 1,
                        gfx->periph.now);
        avi_set_palette(avi, &gfx->palette, 16, 0 );
        avi_set_palette(avi, &util_palette, 16, 16 );
    }

    if ((gfx->scrshot & GFX_RESET) == 0)
        avi_record_video(gfx->avi, gfx->vid + 4*160, gfx->vid[0]);
}

/* ======================================================================== */
/*  GFX_RESYNC   -- Resynchronize GFX after a load.                         */
/* ======================================================================== */
void gfx_resync(gfx_t *const gfx)
{
    gfx->dirty   = 3;
    gfx->b_dirty = 3;
}

/* ======================================================================== */
/*  GFX_HIDDEN       -- Returns true if the graphics window is hidden.      */
/* ======================================================================== */
bool gfx_hidden(const gfx_t *const gfx)
{
    return gfx->hidden;
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
