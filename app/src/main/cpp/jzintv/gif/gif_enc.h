/* ======================================================================== */
/*  GIF Encoder routines.                                                   */
/*                                                                          */
/*  These are intended to support single-frame and multi-frame GIFs.        */
/*  Single frame support is for screen shots, multi-frame support is for    */
/*  converting movies to animated GIFs.                                     */
/*                                                                          */
/*  This GIF encoder doesn't trust that the decoder honors the aspect       */
/*  ratio stored in the GIF.  We just set it to 0.                          */
/*                                                                          */
/*  None of this code's thread-safe/reentrant.  BFD.                        */
/* ======================================================================== */
#ifndef GIF_ENC_H_
#define GIF_ENC_H_

typedef struct gif_t
{
    FILE    *f;
    int     x_dim, y_dim;
    int     trans, n_cols;
    uint8_t  *vid, *pal;
} gif_t;

extern int gif_best_stat[6];

typedef const uint8_t(*gif_pal_t)[3];

/* ======================================================================== */
/*  GIF_START -- Starts a single or multi-frame GIF.                        */
/* ======================================================================== */
int gif_start
(
    gif_t         *gif,
    FILE          *f,           /* file to attach to GIF.                   */
    int            x_dim,       /* source image X dimension                 */
    int            y_dim,       /* source image Y dimension                 */
    const uint8_t  pal[][3],    /* palette to use for GIF.                  */
    int            n_cols,      /* number of colors in GIF.                 */
    int            multi        /* 0: Single image, 1: Multiple image       */
);

/* ======================================================================== */
/*  GIF_FINISH -- Finishes off a GIF, terminating it and freeing memory.    */
/* ======================================================================== */
int gif_finish(gif_t *gif);


/* ======================================================================== */
/*  GIF_WR_FRAME_S -- Writes single-frame image to GIF.                     */
/* ======================================================================== */
int gif_wr_frame_s
(
    gif_t         *gif,
    const uint8_t *vid
);

/* ======================================================================== */
/*  GIF_WRITE       -- Wrapper around gif_start/gif_wr_frame_s.             */
/* ======================================================================== */
int gif_write
(
    FILE          *f,
    const uint8_t *vid,
    int            x_dim,
    int            y_dim,
    const uint8_t  pal[][3],
    int            n_cols
);


/* ======================================================================== */
/*  GIF_WR_FRAME_M -- Writes next frame to a multi-frame GIF.               */
/*                    Attempts to optimize image.                           */
/* ======================================================================== */
int gif_wr_frame_m
(
    gif_t         *gif,
    const uint8_t *vid,
    int            delay,
    int            mode
);

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
/*                   Copyright (c) 2005, Joseph Zbiciak                     */
/* ======================================================================== */
