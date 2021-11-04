
#include "config.h"
#include "mvi.h"

#ifndef NO_LZO
# include "minilzo/minilzo.h"
# define IF_LZO(x) x
#else
# define IF_LZO(x) ((void)0)
#endif

/* ======================================================================== */
/*  Movie format is little endian on all targets.  All multibyte fields     */
/*  get sent LSbyte first.  All bitmaps are ls-bit first.  Packed pixels    */
/*  fill LSnibble before MSnibble.                                          */
/*                                                                          */
/*  FILE HEADER:                                                            */
/*      4 bytes     Signature: 0x4A 0x5A 0x6A 0x7A                          */
/*      2 bytes     X resolution  (always 0xA0 0x00 (160) for now)          */
/*      2 bytes     Y resolution  (always 0xC8 0x00 (200) for now)          */
/*                                                                          */
/*  Note that the file header can optionally appear after any frame.        */
/*  This makes it easy to concatenate two movies together.  The decoder     */
/*  should look for a file header when decoding any frame.  It's up to      */
/*  the decoder how it handles two movies w/ different resolutions.         */
/*                                                                          */
/*  FRAME FORMAT:                                                           */
/*      4 bytes     Total frame length (incl. header)                       */
/*      3 bytes     Frame number                                            */
/*      1 byte      Flags                                                   */
/*                  Bit 0:  Frame sent (0 == skip, 1 == present)            */
/*                          Absent frames assumed to be repeat of prev      */
/*                  Bit 1:  Bounding boxes sent (0 == skip, 1 == present)   */
/*                          Absent bounding boxes assumed to be repeat.     */
/*                  Bit 2:  RLE compressed                                  */
/*                  Bit 3:  In-frame row-repeat map (1 == present)          */
/*                  Bit 4:  Frame-to-frame row delta map (1 == present)     */
/*                  Bit 5:  LZO compressed (0 == no, 1 == yes)              */
/*                  Bit 6..7:  Reserved                                     */
/*                                                                          */
/*      (Note:  Remainder of frame is LZO compressed if LZO = 1)            */
/*                                                                          */
/*      32 bytes    MOB bounding boxes in 8 4-byte records (if sent.)       */
/*                  1 byte each for x0, y0, x1, y1                          */
/*      N bytes     Row Delta Map (if sent.)                                */
/*      N bytes     Row Repeat Map (if sent.)                               */
/*      N bytes     Image data  (if sent.)                                  */
/*                                                                          */
/*  ROW DELTA MAP                                                           */
/*                                                                          */
/*  If present, the row-delta map indicates which rows are exact repeats    */
/*  of the previous frame.  This eliminates rows from the raster scan       */
/*  that get sent.  Row-delta compression happens before row-repeat, RLE or */
/*  copy-through.  The row-delta map is 1-bit-per-row bitmap that           */
/*  indicates which rows are copies of the ones from the previous frame.    */
/*  Rows are packed in LSB first, in a byte-oriented bitmap.  The           */
/*  bitmap length is always rounded to a whole number of bytes.             */
/*                                                                          */
/*  The length of the row-delta map is implied by the Y dimension of        */
/*  the screen.                                                             */
/*                                                                          */
/*  ROW REPEAT MAP                                                          */
/*                                                                          */
/*  If present, the row-repeat map indicates which rows are repeats         */
/*  of the rows above them.  This eliminates rows from the raster scan      */
/*  that get sent.  Row-repeat compression happens before RLE or            */
/*  copy-through.  The row-repeat map is 1-bit-per-row bitmap that          */
/*  indicates which rows are copies of the ones above it.  Copies are       */
/*  recursive, so the same row can get copied down multiple times.          */
/*  Rows are packed in LSB first, in a byte-oriented bitmap.  The           */
/*  bitmap length is always rounded to a whole number of bytes.             */
/*                                                                          */
/*  The length of the row-repeat map is implied by the Y dimension of       */
/*  the screen, minus the rows compressed away by the row-delta map.        */
/*                                                                          */
/*  IMAGE DATA FORMAT:                                                      */
/*      N bytes     Pixel data                                              */
/*                                                                          */
/*  RLE COMPRESSED IMAGE                                                    */
/*                                                                          */
/*  For RLE-compressed images, image is composed of "copy runs" and         */
/*  "fill runs".  All image pixels are contained in a run and so an         */
/*  image consists of "run records."  Runs are always even length.          */
/*  The RLE compressor scans from upper left to lower right in normal       */
/*  raster order, and concatenates rows.                                    */
/*                                                                          */
/*      COPY RUN:  2 pixels to 256 pixels, always even length.              */
/*                                                                          */
/*          Header byte:                                                    */
/*          Bit 0:      0 (indicates "copy run")                            */
/*          Bit 7..1:   Bits 7..1 of "run_length - 2".                      */
/*                                                                          */
/*          Data byte(s):                                                   */
/*          Bit 7..4:   Second pixel                                        */
/*          Bit 3..0:   First pixel                                         */
/*                                                                          */
/*      FILL RUN:  2 pixels to 526 pixels.                                  */
/*                                                                          */
/*          Header byte:                                                    */
/*          Bit 0:      1 (indicates "fill run")                            */
/*          Bit 3..1:   Run length (2..14), or 000 to indicate overflow.    */
/*          Bit 7..4:   Fill color.                                         */
/*                                                                          */
/*          If encoded run length = 000, a second header byte provides      */
/*          an 8-bit run length in the range 0..510, to which 16 gets       */
/*          added for the actual run length.  Thus, the value stored        */
/*          in the byte is (run_length - 16) / 2.                           */
/*                                                                          */
/*  UNCOMPRESSED IMAGE                                                      */
/*                                                                          */
/*  For uncompressed images, there's simply a x_dim * y_dim / 2 byte        */
/*  record of packed pixels.  Pixels are packed into bytes with the         */
/*  first pixel in bits 3..0, and the second pixel in bits 7..4.            */
/* ======================================================================== */


#define FLG_FRSENT ( 1)
#define FLG_BBSENT ( 2)
#define FLG_RLECMP ( 4)
#define FLG_RPTMAP ( 8)
#define FLG_DLTMAP (16)
#define FLG_LZOCMP (32)

LOCAL uint8_t *enc_vid = NULL, *enc_buf = NULL, *enc_vid2;
#ifndef NO_LZO
LOCAL uint8_t *lzo_wrk = NULL, *lzo_tmp = NULL;
#endif
LOCAL uint32_t rowrpt[MVI_MAX_Y >> 5];
LOCAL uint32_t rowdlt[MVI_MAX_Y >> 5];

#define ENC_BUF_SZ (MVI_MAX_X * MVI_MAX_Y + 128)


/* ======================================================================== */
/*  MVI_INIT:  Initialize movie tracking structure and scratch areas        */
/* ======================================================================== */
void mvi_init(mvi_t *movie, int x_dim, int y_dim)
{
    int i, j;

    if (x_dim > MVI_MAX_X || y_dim > MVI_MAX_Y)
    {
        fprintf(stderr, "MVI_INIT: %dx%d movie dimensions too large\n",
                x_dim, y_dim);
        exit(1);
    }

    movie->x_dim = x_dim;
    movie->y_dim = y_dim;

    for (i = 0; i < 8; i++)
        for (j = 0; j < 4; j++)
            movie->bbox[i][j] = 0;

    movie->vid = CALLOC(uint8_t, x_dim*y_dim); /* 1 byte/pixel, just like gfx */
    movie->fr  = 0;
    movie->f   = NULL;

    if (!enc_buf ) enc_buf  = CALLOC(uint8_t, ENC_BUF_SZ);
    if (!enc_vid ) enc_vid  = CALLOC(uint8_t, MVI_MAX_X * MVI_MAX_Y);
    if (!enc_vid2) enc_vid2 = CALLOC(uint8_t, MVI_MAX_X * MVI_MAX_Y);

#ifndef NO_LZO
    if (!lzo_wrk) lzo_wrk = CALLOC(uint8_t, LZO1X_MEM_COMPRESS);
    if (!lzo_tmp) lzo_tmp = CALLOC(uint8_t, ENC_BUF_SZ);
#endif

    if (!enc_buf || !enc_vid || !enc_vid2 || !movie->vid)
    {
        perror("MVI_INIT");
        exit(1);
    }
}

/* ======================================================================== */
/*  MVI_WR_FRAME:  Encode and write a movie frame to the movie file.        */
/* ======================================================================== */
void mvi_wr_frame(mvi_t *movie, uint8_t *vid, uint8_t bbox[8][4])
{
    int send_bbox = 0, send_frame = 0, send_rle = 0;
    int send_rowrpt = 0, lzo_comp = 0;
    int enc_height_rd = movie->y_dim, enc_height = movie->y_dim;
    int send_rowdlt = 0, rowdlt_ok = 0;
    int yi, yo;
    uint8_t header_byte = 0;
    int rle_max_len;
    uint8_t *enc_hdr;
    uint8_t *enc_ptr = enc_buf;
    uint8_t *evid = vid;
    int i, j;

    /* -------------------------------------------------------------------- */
    /*  If this is frame #0, send a file header.                            */
    /* -------------------------------------------------------------------- */
    if (movie->fr == 0)
    {
        /* signature */
        *enc_ptr++ = 0x4A;
        *enc_ptr++ = 0x5A;
        *enc_ptr++ = 0x6A;
        *enc_ptr++ = 0x7A;
        /* dimensions */
        *enc_ptr++ = (movie->x_dim >> 0) & 0xFF;
        *enc_ptr++ = (movie->x_dim >> 8) & 0xFF;
        *enc_ptr++ = (movie->y_dim >> 0) & 0xFF;
        *enc_ptr++ = (movie->y_dim >> 8) & 0xFF;

        movie->rpt_frames = 0;
        movie->rpt_rows   = 0;
        movie->tot_bytes  = 0;
#ifndef NO_LZO
        movie->tot_lzosave= 0;
#endif
        memset(movie->vid, 0xFF, movie->x_dim * movie->y_dim); /* force 1st */
    }

    enc_hdr  = enc_ptr;
    enc_ptr += 8;

    /* -------------------------------------------------------------------- */
    /*  Decide whether to send bounding box and frame.                      */
    /* -------------------------------------------------------------------- */
    send_frame = memcmp(movie->vid, vid, movie->x_dim * movie->y_dim);
    send_bbox  = memcmp((void*)movie->bbox, bbox, sizeof(movie->bbox));

    if (!send_frame) movie->rpt_frames++;

    if (send_frame &&                             /* only on frames we send */
        movie->fr > 0 &&                          /* never the first frame  */
        ((movie->fr - movie->rpt_frames) & 15) != 0)  /* force no 1 ever 16 */
        rowdlt_ok = 1;


    /* -------------------------------------------------------------------- */
    /*  If we're sending the bounding boxes, plug them in now.              */
    /* -------------------------------------------------------------------- */
    if (send_bbox)
    {
        for (i = 0; i < 8; i++)
            for (j = 0; j < 4; j++)
                *enc_ptr++ = bbox[i][j];

        memcpy(movie->bbox, bbox, sizeof(movie->bbox));
    }

    /* -------------------------------------------------------------------- */
    /*  If we can send a row-delta map, go find which rows are repeats      */
    /*  from the previous frame.                                            */
    /* -------------------------------------------------------------------- */
    if (send_frame && rowdlt_ok)
    {
        memset(rowdlt, 0, sizeof(rowdlt));      /* zero out repeat bitmap.  */
        for (yi = yo = 0; yi < movie->y_dim; yi++)
        {
            if (memcmp(movie->vid + (yi*movie->x_dim),
                              vid + (yi*movie->x_dim), movie->x_dim) == 0)
            {
                rowdlt[yi >> 5] |= 1u << (yi & 0x1F);
                enc_height_rd--;
                movie->rpt_rows++;
            } else
            {
                memcpy(enc_vid2 + yo * movie->x_dim,
                           vid  + yi * movie->x_dim, movie->x_dim);
                yo++;
            }
        }
        assert( !send_rowdlt || yo <  yi );
        assert(!(send_rowdlt && yo == yi));
        assert(yo > 0);
        assert(enc_height_rd > 0);
        assert(yo == enc_height_rd);

        /* ---------------------------------------------------------------- */
        /*  Only send rowdelta if at least 2 rows are repeats.              */
        /* ---------------------------------------------------------------- */
        if (enc_height - enc_height_rd > 2)
        {
            send_rowdlt = 1;
            evid        = enc_vid2;
            enc_height  = enc_height_rd;
/*printf("Frame %-6d %.8X %.8X %.8X %.8X %.8X\n", movie->fr, rowdlt[0], rowdlt[1], rowdlt[2], rowdlt[3], rowdlt[4]);*/

            for (i = 0; i < movie->y_dim; i += 8)
            {
                j = 8 * ((i >> 3) & 3);
                *enc_ptr++ = (rowdlt[i >> 5] >> j) & 0xFF;
            }
        } else
        {
            send_rowdlt   = 0;
            enc_height_rd = movie->y_dim;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  At this point we don't need the previous frame any more, so update  */
    /* -------------------------------------------------------------------- */
    if (send_frame)
        memcpy(movie->vid, vid, movie->x_dim * movie->y_dim);

    /* -------------------------------------------------------------------- */
    /*  If we're sending the frame, copy it to our encoding buffer.  Look   */
    /*  for redundant rows as we go.                                        */
    /* -------------------------------------------------------------------- */
    if (send_frame && enc_height_rd > 1)
    {
        memset(rowrpt, 0, sizeof(rowrpt));      /* zero out repeat bitmap.  */
        memcpy(enc_vid, evid, movie->x_dim);    /* copy first row           */
        for (yi = 1, yo = 1; yi < enc_height_rd; yi++)
        {
            if (!memcmp(evid + (yi-1) * movie->x_dim,
                        evid +  yi    * movie->x_dim, movie->x_dim))
            {
                rowrpt[yi >> 5] |= 1u << (yi & 0x1F);

                send_rowrpt = 1;
                enc_height--;
                movie->rpt_rows++;
            } else
            {
                memcpy(enc_vid + yo * movie->x_dim,
                          evid + yi * movie->x_dim, movie->x_dim);
                yo++;
            }
        }
        assert( !send_rowrpt || yo <  yi );
        assert(!(send_rowrpt && yo == yi));

        if (send_rowrpt)
        {
            for (i = 0; i < enc_height_rd; i += 8)
            {
                j = 8 * ((i >> 3) & 3);
                *enc_ptr++ = (rowrpt[i >> 5] >> j) & 0xFF;
            }
        }
    } else if (send_frame && enc_height_rd <= 1)
    {
        memcpy(enc_vid, enc_vid2, movie->x_dim * enc_height);
    }

/*if (send_frame) printf("frame %.6X enc_height=%-3d enc_height_rd=%-3d\n", movie->fr, enc_height, enc_height_rd);*/

    /* -------------------------------------------------------------------- */
    /*  If we're sending an image, try to RLE-compress it.  If RLE "blows   */
    /*  up" (expands the data) we'll send uncompressed.                     */
    /* -------------------------------------------------------------------- */
    if (send_frame)
    {
        uint8_t *rle_ptr = enc_ptr;
        uint8_t *vid_ptr = enc_vid;
        uint8_t *rle_end;
        uint8_t *vid_end;
        uint8_t *cpstart = enc_vid;
        uint8_t p0, p1, p2, p3;

        rle_max_len = (movie->x_dim * enc_height) >> 1;

        vid_end     = enc_vid + movie->x_dim * enc_height;
        rle_end     = enc_ptr + rle_max_len - 1;

        /* ---------------------------------------------------------------- */
        /*  Keep encoding RLE until we either run out of pixels or we       */
        /*  overflow the encoding buffer.  We'll send uncompressed if RLE   */
        /*  expands the image.                                              */
        /* ---------------------------------------------------------------- */
        while (rle_ptr < rle_end && vid_ptr < vid_end)
        {
            int fl_len;
            int fl_cur;
            int remain = vid_end - vid_ptr;

            /* ------------------------------------------------------------ */
            /*  Grab the next two pixels.                                   */
            /*  If they're not equal append to a copy run.                  */
            /* ------------------------------------------------------------ */
            p0 = vid_ptr[0] & 0xF;
            p1 = vid_ptr[1] & 0xF;

            if (p0 != p1)
            {
                vid_ptr += 2;
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  See how long the run is.                                    */
            /* ------------------------------------------------------------ */
            for (i = 2; i < remain; i += 2)
            {
                p2 = vid_ptr[i+0] & 0xF;
                p3 = vid_ptr[i+1] & 0xF;

                if (p2 != p0 || p3 != p0)
                    break;
            }

            fl_len = i >> 1;  /* always even number, so div-by-2 */

            /* ------------------------------------------------------------ */
            /*  If it's not a long enough run, just append them to the      */
            /*  copy run and continue.                                      */
            /* ------------------------------------------------------------ */
            if (fl_len == 1 && remain > 2)
            {
                vid_ptr += fl_len << 1;
                continue;
            }

            /* ------------------------------------------------------------ */
            /*  Flush any existing copy run first.                          */
            /* ------------------------------------------------------------ */
            if (cpstart < vid_ptr)
            {
                int cp_len = vid_ptr - cpstart;
                int cp_cur;

                assert((cp_len & 1) == 0);

                /* -------------------------------------------------------- */
                /*  Don't let copy run overflow buffer!                     */
                /* -------------------------------------------------------- */
                if (rle_ptr + 2 + (cp_len>>8) + (cp_len>>1) >= rle_end)
                    goto no_rle;

                /* -------------------------------------------------------- */
                /*  Output the copy run.                                    */
                /* -------------------------------------------------------- */
                while (cp_len > 0)
                {
                    int p;
                    cp_cur  = cp_len > 256 ? 256 : cp_len;
                    cp_len -= cp_cur;

                    *rle_ptr++ = cp_cur - 2;
                    for (j = 0; j < cp_cur; j += 2)
                    {
                        p = (cpstart[0] & 0xF) | ((cpstart[1]&0xF)<<4);
                        *rle_ptr++ = p;
                        cpstart   += 2;
                    }
                }
                assert(cpstart == vid_ptr);
            }

            /* ------------------------------------------------------------ */
            /*  Advance the input video pointer beyond fill run.  That      */
            /*  location also marks the beginning of the next copy run.     */
            /* ------------------------------------------------------------ */
            vid_ptr += fl_len << 1;
            cpstart  = vid_ptr;

            /* ------------------------------------------------------------ */
            /*  Now output the RLE run.                                     */
            /* ------------------------------------------------------------ */
            while (fl_len > 0)
            {
                fl_cur  = fl_len > 263 ? 263 : fl_len;
                fl_len -= fl_cur;

                if (fl_cur < 8)
                {
                    if (rle_ptr >= rle_end)
                        goto no_rle;
                    *rle_ptr++ = 1 | (fl_cur << 1) | (p0 << 4);
                } else
                {
                    if (rle_ptr + 1 >= rle_end)
                        goto no_rle;
                    *rle_ptr++ = 1 | (p0 << 4);
                    *rle_ptr++ = fl_cur - 8;
                }
            }
        }

        if (rle_ptr >= rle_end)
            goto no_rle;

        /* ---------------------------------------------------------------- */
        /*  Flush any copy run at end.                                      */
        /* ---------------------------------------------------------------- */
        if (cpstart < vid_ptr)
        {
            int cp_len = vid_ptr - cpstart;
            int cp_cur;

            assert((cp_len & 1) == 0);

            /* ------------------------------------------------------------ */
            /*  Don't let copy run overflow buffer!                         */
            /* ------------------------------------------------------------ */
            if (rle_ptr + 2 + (cp_len>>8) + (cp_len>>1) >= rle_end)
                goto no_rle;

            /* ------------------------------------------------------------ */
            /*  Output the copy run.                                        */
            /* ------------------------------------------------------------ */
            while (cp_len > 0)
            {
                int p;
                cp_cur  = cp_len > 256 ? 256 : cp_len;
                cp_len -= cp_cur;

                *rle_ptr++ = cp_cur - 2;
                for (j = 0; j < cp_cur; j += 2)
                {
                    p = (cpstart[0] & 0xF) | ((cpstart[1]&0xF)<<4);
                    *rle_ptr++ = p;
                    cpstart   += 2;
                }
            }
            assert(cpstart == vid_ptr);
        }

        /* ---------------------------------------------------------------- */
        /*  If we make it to here w/out overflowing the RLE buf, send RLE.  */
        /* ---------------------------------------------------------------- */
        if (rle_ptr <= rle_end)
        {
            send_rle = 1;
            enc_ptr  = rle_ptr;
        }
    } /* end of RLE encode */

no_rle:

    /* -------------------------------------------------------------------- */
    /*  If we're sending the frame and RLE didn't work out, send packed.    */
    /* -------------------------------------------------------------------- */
    if (send_frame && !send_rle)
    {
        int total = movie->x_dim * enc_height;

        for (i = 0; i < total; i += 2)
        {
            *enc_ptr++ =  (enc_vid[i + 0] & 0xF)
                       | ((enc_vid[i + 1] & 0xF) << 4);
        }
    }

#ifndef NO_LZO
    /* -------------------------------------------------------------------- */
    /*  Try to compress the encoded frame with LZO.                         */
    /* -------------------------------------------------------------------- */
    if (lzo_wrk && lzo_tmp && enc_ptr > enc_hdr + 8)
    {
        int r;
        lzo_uint lzo_len = 0, data_len = enc_ptr - enc_hdr - 8;

        r = lzo1x_1_compress(enc_hdr + 8, data_len,
                             lzo_tmp, &lzo_len, (lzo_voidp)lzo_wrk);

        if (r == LZO_E_OK && lzo_len < data_len && lzo_len > 0)
        {
            memcpy(enc_hdr + 8, lzo_tmp, lzo_len);
            enc_ptr = enc_hdr + 8 + lzo_len;
            lzo_comp = 1;
            movie->tot_lzosave += data_len - lzo_len;
        }
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  Generate the header byte.                                           */
    /* -------------------------------------------------------------------- */
    if ( send_frame ) header_byte |= FLG_FRSENT;
    if ( send_bbox  ) header_byte |= FLG_BBSENT;
    if ( send_rowrpt) header_byte |= FLG_RPTMAP;
    if ( send_rle   ) header_byte |= FLG_RLECMP;
    if ( send_rowdlt) header_byte |= FLG_DLTMAP;
    if ( lzo_comp   ) header_byte |= FLG_LZOCMP;

    /* -------------------------------------------------------------------- */
    /*  Prepend the header:  Frame # and flag byte.                         */
    /* -------------------------------------------------------------------- */
    enc_hdr[0] = ((enc_ptr - enc_hdr) >>  0) & 0xFF;
    enc_hdr[1] = ((enc_ptr - enc_hdr) >>  8) & 0xFF;
    enc_hdr[2] = ((enc_ptr - enc_hdr) >> 16) & 0xFF;
    enc_hdr[3] = ((enc_ptr - enc_hdr) >> 24) & 0xFF;
    enc_hdr[4] = (movie->fr >>  0) & 0xFF;
    enc_hdr[5] = (movie->fr >>  8) & 0xFF;
    enc_hdr[6] = (movie->fr >> 16) & 0xFF;
    enc_hdr[7] = header_byte;

    /* -------------------------------------------------------------------- */
    /*  Send it all in one big fwrite.                                      */
    /* -------------------------------------------------------------------- */
    fwrite(enc_buf, 1, enc_ptr - enc_buf, movie->f);
    fflush(movie->f);
    movie->tot_bytes += enc_ptr - enc_buf;

    movie->fr++;  /* increment frame count. */
}


/* ======================================================================== */
/*  MVI_RD_FRAME -- Reads a frame from a movie file and decodes it.         */
/* ======================================================================== */
int  mvi_rd_frame(mvi_t *movie, uint8_t *vid, uint8_t bbox[8][4])
{
    int got_frame  = 0;
    int got_bbox   = 0;
    int got_rowrpt = 0;
    int got_rle    = 0;
    int got_rowdlt = 0;
    int lzo_comp   = 0;
    uint8_t *dec_ptr = enc_buf, *dec_end;
    uint8_t *dec_vid;
    uint8_t *dvid;
    size_t tot_rd;
    uint32_t flags;
    int i, j;
    int enc_height, enc_height_rd;
    uint32_t fr_len;

    /* -------------------------------------------------------------------- */
    /*  Sanity check.                                                       */
    /* -------------------------------------------------------------------- */
    if (!movie->f) return -1; /* no current file? */

again:
    /* -------------------------------------------------------------------- */
    /*  Read 8 bytes.  It's either a frame header or a file header.         */
    /* -------------------------------------------------------------------- */
    tot_rd = fread(enc_buf, 1, 8, movie->f);
    if (tot_rd < 8)
        return -1;      /* end of file or other error */


    /* -------------------------------------------------------------------- */
    /*  Check for a file header on this frame.                              */
    /* -------------------------------------------------------------------- */
    if (tot_rd == 8 &&
        enc_buf[0] == 0x4A && enc_buf[1] == 0x5A &&
        enc_buf[2] == 0x6A && enc_buf[3] == 0x7A)
    {
        int new_x_dim, new_y_dim;

        new_x_dim = (enc_buf[5] << 8) | enc_buf[4];
        new_y_dim = (enc_buf[7] << 8) | enc_buf[6];

        if (new_x_dim != movie->x_dim ||
            new_y_dim != movie->y_dim)
        {
            movie->x_dim = new_x_dim;
            movie->y_dim = new_y_dim;
            if (new_x_dim > MVI_MAX_X || new_y_dim > MVI_MAX_Y)
            {
                fprintf(stderr, "MVI_RD: Movie size %dx%d too large!",
                        new_x_dim, new_y_dim);
                return -1;
            }
        }

        goto again;  /* loop until we get a frame header */
    }

    /* -------------------------------------------------------------------- */
    /*  Pull apart the frame header.                                        */
    /* -------------------------------------------------------------------- */
    fr_len = (enc_buf[0] <<  0) |
             (enc_buf[1] <<  8) |
             (enc_buf[2] << 16) |
             (enc_buf[3] << 24);

    if (fr_len > ENC_BUF_SZ)
    {
        fprintf(stderr,
                "MVI_RD: Frame size too large: %d bytes (%.8X bytes)\n",
                fr_len, fr_len);
        exit(1);
    }

    movie->last_fr = movie->fr;

    movie->fr = (enc_buf[4] <<  0) |
                (enc_buf[5] <<  8) |
                (enc_buf[6] << 16);

    flags = enc_buf[7];


    got_frame  = flags & FLG_FRSENT;
    got_bbox   = flags & FLG_BBSENT;
    got_rowrpt = flags & FLG_RPTMAP;
    got_rle    = flags & FLG_RLECMP;
    got_rowdlt = flags & FLG_DLTMAP;
    lzo_comp   = flags & FLG_LZOCMP;

    /* -------------------------------------------------------------------- */
    /*  Now read the rest of the frame.                                     */
    /* -------------------------------------------------------------------- */
    tot_rd += fread(enc_buf, 1, fr_len - 8, movie->f);
    dec_end = enc_buf + fr_len - 8;
    dec_ptr = enc_buf;

    if (tot_rd != fr_len)
    {
        fprintf(stderr,
                "MVI_RD: Short read on frame %d (%d vs %d bytes)\n"
                "        Previous frame was frame %d\n",
                movie->fr, (int)tot_rd, fr_len, movie->last_fr);
        exit(1);
    }


    /* -------------------------------------------------------------------- */
    /*  Sanity check flags.                                                 */
    /* -------------------------------------------------------------------- */
    if ((flags & 0xC0) != 0 ||
        (got_rle    != 0 && got_frame == 0) ||
        (got_rowdlt != 0 && got_frame == 0) ||
        (got_rowrpt != 0 && got_frame == 0))
    {
        fprintf(stderr, "MVI_RD: Bogus flags: %.2X\n", flags);
        return -1;
    }

    if (flags != 0 && (movie->x_dim == 0 || movie->y_dim == 0))
    {
        fprintf(stderr, "MVI_RD: Movie data before valid file header!\n");
        return -1;
    }


    /* -------------------------------------------------------------------- */
    /*  If the frame is LZO compressed, decompress it before proceeding.    */
    /* -------------------------------------------------------------------- */
#ifdef NO_LZO
    if (lzo_comp)
    {
        fprintf(stderr, "MVI_RD: This movie is LZO-compressed.  LZO "
                        "compression support is not compiled in.\n");
        return -1;
    }
#else
    if (lzo_comp)
    {
        int r;
        lzo_uint lzo_len = 0;

        if (!lzo_tmp)
        {
            fprintf(stderr, "MVI_RD: This movie is LZO-compressed, but "
                            "there is insufficient memory for decompresion\n");
            return -1;
        }

        r = lzo1x_decompress(dec_ptr, fr_len - 8, lzo_tmp,
                             &lzo_len,
                             (lzo_voidp)NULL);

        if (r == LZO_E_OK)
        {
            memcpy(dec_ptr, lzo_tmp, lzo_len);
            fr_len  = 8 + lzo_len;
            dec_end = dec_ptr + lzo_len;
        } else
        {
            fprintf(stderr, "MVI_RD:  LZO error: %d\n", r);
            return -1;
        }
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  If we have an updated set of bounding boxes, read them in.          */
    /* -------------------------------------------------------------------- */
    if (got_bbox)
    {
        for (i = 0; i < 8; i++)
            for (j = 0; j < 4; j++)
                movie->bbox[i][j] = *dec_ptr++;
    }

    /* -------------------------------------------------------------------- */
    /*  If we have a row-delta map, read that in.                           */
    /* -------------------------------------------------------------------- */
    memset(rowdlt, 0, sizeof(rowdlt));
    enc_height_rd = movie->y_dim;

    if (got_rowdlt)
    {
        for (i = 0; i < movie->y_dim; i += 8)
            rowdlt[i >> 5] |= *dec_ptr++ << (8*((i>>3) & 3));

        if (dec_ptr >= dec_end)
        {
            fprintf(stderr, "MVI_RD:  Error reading row-repeat tbl\n");
            return -1;
        }

        for (i = 0; i < movie->y_dim; i += 32)
        {
            uint32_t tmp = rowdlt[i >> 5];

            while (tmp)
            {
                tmp &= tmp - 1;
                enc_height_rd--;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  If we have a row-repeat map, read that in.                          */
    /* -------------------------------------------------------------------- */
    memset(rowrpt, 0, sizeof(rowrpt));
    enc_height = enc_height_rd;

    if (got_rowrpt)
    {
        for (i = 0; i < enc_height_rd; i += 8)
            rowrpt[i >> 5] |= *dec_ptr++ << (8*((i>>3) & 3));

        if (dec_ptr >= dec_end)
        {
            fprintf(stderr, "MVI_RD:  Error reading row-repeat tbl\n");
            return -1;
        }

        for (i = 0; i < enc_height_rd; i += 32)
        {
            uint32_t tmp = rowrpt[i >> 5];

            while (tmp)
            {
                tmp &= tmp - 1;
                enc_height--;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Now decode the image (prior to unpacking the row repeat map).       */
    /* -------------------------------------------------------------------- */
    dec_vid = got_rowrpt ? enc_vid  :
              got_rowdlt ? enc_vid2 : vid;

    if (got_frame == 0)
    {
        memcpy(vid, movie->vid, movie->x_dim * movie->y_dim);
    } else if (!got_rle)
    {
        for (i = 0; i < movie->x_dim * enc_height; i += 2)
        {
            *dec_vid++ = 0xF &  *dec_ptr;
            *dec_vid++ = 0xF & (*dec_ptr >> 4);
            dec_ptr++;
        }
    } else /* RLE data */
    {
        int run, len, p;
        int remain = movie->x_dim * enc_height;

        /* ---------------------------------------------------------------- */
        /*  Expand all the runs in the encoded image.  The ending comes     */
        /*  implicitly when no images pixels remain.                        */
        /* ---------------------------------------------------------------- */
        while (remain > 0)
        {
            if (dec_ptr >= dec_end)
            {
                fprintf(stderr, "MVI_RD:  Error in RLE data!\n");
                return -1;
            }

            run = *dec_ptr++;

            /* ------------------------------------------------------------ */
            /*  Handle copy runs by unpacking and copying the pixels to     */
            /*  the output.                                                 */
            /* ------------------------------------------------------------ */
            if ((run & 1) == 0) /* copy run? */
            {
                len = run + 2;
                if (len > remain || (dec_ptr + (len>>1)) > dec_end)
                {
                    fprintf(stderr, "MVI_RD:  Error in RLE data!\n");
                    return -1;
                }
                remain -= len;

                for (i = 0; i < len; i += 2)
                {
                    *dec_vid++ = 0xF &  *dec_ptr;
                    *dec_vid++ = 0xF & (*dec_ptr >> 4);
                    dec_ptr++;
                }

                continue;
            }

            /* ------------------------------------------------------------ */
            /*  Handle fill runs by writing the color value over and over.  */
            /* ------------------------------------------------------------ */
            len = run & 0xE;
            if (!len) len = 16 + (*dec_ptr++ << 1);

            if (len > remain || dec_ptr > dec_end)
            {
                fprintf(stderr, "MVI_RD:  Error in RLE data!\n");
                return -1;
            }
            remain -= len;

            p = (run & 0xF0) >> 4;

            len >>= 1;

            while (len-->0) { *dec_vid++ = p; *dec_vid++ = p; }
        }

        if (remain != 0)
        {
            fprintf(stderr, "MVI_RD:  Error in RLE data!\n");
            return -1;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  If we have a row-repeat map, expand vertically-compressed image.    */
    /* -------------------------------------------------------------------- */
    if (got_rowrpt)
    {
        int yo, yi, xd = movie->x_dim;

        dvid = got_rowdlt ? enc_vid2 : vid;

        yi = enc_height - 1;
        for (yo = enc_height_rd - 1; yo >= 0; yo--)
        {
            memcpy(dvid + yo*xd, enc_vid + yi*xd, xd);

            if ((1 & (rowrpt[yo >> 5] >> (yo & 0x1F))) == 0) /* not a rpt */
                yi--;
        }

        assert(yi == -1);
    }

    /* -------------------------------------------------------------------- */
    /*  If we have a row-delta map, expand temporally-compressed image.     */
    /* -------------------------------------------------------------------- */
    if (got_rowdlt)
    {
        int yo, yi, xd = movie->x_dim;

        yi = enc_height_rd - 1;
        for (yo = movie->y_dim - 1; yo >= 0; yo--)
        {
            if ((1 & (rowdlt[yo >> 5] >> (yo & 0x1F))) == 0) /* not a rpt */
            {
                memcpy(vid + yo*xd, enc_vid2 + yi*xd, xd);
                yi--;
            } else  /* copy from previous frame */
            {
                memcpy(vid + yo*xd, movie->vid + yo*xd, xd);
            }
        }

        assert(yi == -1);
    }

    /* -------------------------------------------------------------------- */
    /*  Remember each frame we decode.                                      */
    /* -------------------------------------------------------------------- */
    if (got_frame)
        memcpy(movie->vid, vid, movie->x_dim * movie->y_dim);


    /* -------------------------------------------------------------------- */
    /*  Copy over current bounding boxes.                                   */
    /* -------------------------------------------------------------------- */
    memcpy(bbox, movie->bbox, sizeof(movie->bbox));

    return (got_frame ? 0 : MVI_FR_SAME) |
           (got_bbox  ? 0 : MVI_BB_SAME);
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
/*                 Copyright (c) 1998-2005, Joseph Zbiciak                  */
/* ======================================================================== */

