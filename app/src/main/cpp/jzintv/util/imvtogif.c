
#include "config.h"
#include "mvi/mvi.h"
#include "gif/gif_enc.h"
#include "gif/lzw_enc.h"


mvi_t movie;

/*
 * ============================================================================
 *  GFX_STIC_PALETTE -- The STIC palette.
 * ============================================================================
 */
uint8_t palette[17][3] =
{
    /* -------------------------------------------------------------------- */
    /*  I generated these colors by directly eyeballing my television       */
    /*  while it was next to my computer monitor.  I then tweaked each      */
    /*  color until it was pretty close to my TV.  Bear in mind that        */
    /*  NTSC (said to mean "Never The Same Color") is highly susceptible    */
    /*  to Tint/Brightness/Contrast settings, so your mileage may vary      */
    /*  with this particular pallete setting.                               */
    /* -------------------------------------------------------------------- */
#if 1
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
#else
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x16, 0xFE },
    { 0xFE, 0x5A, 0x02 },
    { 0xC0, 0xD8, 0x63 },
    { 0x00, 0xB7, 0x00 },
    { 0x00, 0xE6, 0x18 },
    { 0xF0, 0xFF, 0x56 },
    { 0xFD, 0xFD, 0xFF },
    { 0xBF, 0xC3, 0xCA },
    { 0x00, 0xC8, 0xF0 },
    { 0xFC, 0xCA, 0x23 },
    { 0x20, 0x80, 0x00 },
    { 0xFF, 0x5E, 0xA8 },
    { 0xA0, 0x90, 0xFF },
    { 0x90, 0xFF, 0x60 },
    { 0xC0, 0x10, 0x7A },
#endif

    /* for debug */
    { 0xFF, 0x80, 0x80 },
};

uint8_t curr[MVI_MAX_X * MVI_MAX_Y];
uint8_t prev[MVI_MAX_X * MVI_MAX_Y];
uint8_t scaled[MVI_MAX_X * MVI_MAX_Y * 2];

const char * typedesc[6] =
{
    "a. Cropped image, orig palette, no trans pixels",
    "b. Cropped image, orig palette, trans pixels",
    "c. Cropped image, orig palette, \"wildcard\" trans/no-trans",
    "d. Cropped image, new palette, no trans pixels",
    "e. Cropped image, new palette, trans pixels",
    "f. Cropped image, new palette, \"wildcard\" trans/no-trans"
};

int color_histo[65536];
int color_votes[16];
int color_map[16];

uint8_t map_palette[16][3];

const char *prog;

static void usage( void )
{
    fprintf(stderr, "%s [flags] input.imv output.gif\n", prog);
    fprintf(stderr, "Flags:\n"
            "    -d##   Set minimum delay to ##ms, default 50ms.\n"
            "    -D##   Assume GIF decode delay of ##ms, default 3.33ms.\n"
            "    -s     Stretch horizontallly 2x.\n"
            "    -f     Flat images (no transparency or optimization).\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    FILE *fi, *fo;
    uint8_t bbox[8][4];
    int fr, out_fr = 0;
    int flag;
    int i, j;
    gif_t gif;
    int prev_gif_time, curr_gif_time, delay;
    int ret, wrote = 0, skip = 0;
    int early = 0;
    int same = 0;
    int n_cols = 16;
    int mode = 0;
    int min_delay = 15, stretch = 0, dec_delay = 1;

    prog = argv[0];

    while ( argc >= 4 && argv[1][0] == '-' )
    {
        if      ( argv[1][1] == '\0' ) mode = 1;
        else if ( argv[1][1] == 'f'  ) mode = 1;
        else if ( argv[1][1] == 'd'  ) min_delay = atof(&argv[1][2])*3/10;
        else if ( argv[1][1] == 'D'  ) dec_delay = atof(&argv[1][2])*3/10;
        else if ( argv[1][1] == 's'  ) stretch = 1;
        else
        {
            fprintf(stderr, "%s: unexpected flag %s\n", prog, argv[1] );
            usage();
        }

        argc--;
        argv++;
    }

    if (argc != 3) usage();

    mvi_init(&movie, MVI_MAX_X, MVI_MAX_Y);

    fi = fopen(argv[1], "rb");
    if (!fi)
    {
        perror("fopen()");
        fprintf(stderr, "Could not open %s for reading\n", argv[1]);
        exit(1);
    }

    fo = fopen(argv[2], "wb");
    if (!fo)
    {
        perror("fopen()");
        fprintf(stderr, "Could not open %s for writing\n", argv[2]);
        exit(1);
    }


    memset(movie.vid, 16, MVI_MAX_X * MVI_MAX_Y);
    movie.f = fi;
    fr = 0;
    prev_gif_time = curr_gif_time = 0;

    printf("Pass 1:  Color optimization...\n"); fflush(stdout);
    while ((flag = mvi_rd_frame(&movie, curr, bbox)) >= 0)
    {
        int mask = 0;
        if ((flag & MVI_FR_SAME) != 0)
            continue;

        for (i = 0; i < movie.x_dim * movie.y_dim; i++)
            mask |= 1u << curr[i];

        j = 0;
        for (i = 0; i < 16; i++)
            j += (mask >> i) & 1;

        if (j <= 2)
            color_histo[mask & 0xFFFF] += 80;
        if (j <= 3)
            color_histo[mask & 0xFFFF] += 30;
        if (j <= 4)
            color_histo[mask & 0xFFFF] += 20;
        if (j <= 7)
            color_histo[mask & 0xFFFF] += 10;
        if (j <= 8)
            color_histo[mask & 0xFFFF] += 10;

        color_histo[mask & 0xFFFF] += 1;
    }

    for (i = 0; i < 65536; i++)
    {
        for (j = 0; j < 16; j++)
            if (i & (1u << j))
                color_votes[j] += color_histo[i];
    }

#if 1
    for (i = 0; i < 16; i++)
    {
        int max = 0, max_v = -1;

        for (j = 0; j < 16; j++)
        {
            if (color_votes[j] > max_v)
            {
                max_v = color_votes[j];
                max   = j;
            }
        }

        if (max_v == 0 && i < n_cols) n_cols = i;

        color_map[max]    = i;
        color_votes[max]  = -1;
        map_palette[i][0] = palette[max][0];
        map_palette[i][1] = palette[max][1];
        map_palette[i][2] = palette[max][2];
        printf("   Remap %2d to %2d (%7d votes)\n", max, i, max_v);
    }
#else
    for (i = 0; i < 16; i++)
    {
        int min = 0, min_v = INT_MAX;

        for (j = 0; j < 16; j++)
        {
            if (color_votes[j] > 0 && color_votes[j] < min_v)
            {
                min_v = color_votes[j];
                min   = j;
            }
        }
        if (min_v == INT_MAX)
            for (j = 0; j < 16; j++)
            {
                if (color_votes[j] == 0)
                {
                    min   = j;
                    break;
                }
            }


        color_map[min]    = i;
        color_votes[min]  = -1;
        map_palette[i][0] = palette[min][0];
        map_palette[i][1] = palette[min][1];
        map_palette[i][2] = palette[min][2];
        printf("   Remap %2d to %2d (%7d votes)\n", min, i, min_v);
    }
#endif


    /* reset the movie */
    fseek(movie.f, 0, SEEK_SET);
    memset(movie.vid, 0xFF, movie.x_dim * movie.y_dim);

    /* get to the first frame of the movie */
    while ((flag = mvi_rd_frame(&movie, prev, bbox)) >= 0)
    {
        if ((flag & MVI_FR_SAME) == 0)
            break;
    }

    ret = gif_start(&gif, fo, stretch ? 2 * movie.x_dim : movie.x_dim,
                              movie.y_dim, (gif_pal_t)map_palette, n_cols,1);
    if (ret < 0)
    {
        fprintf(stderr, "Error starting GIF file %s\n", argv[2]);
        exit(1);
    }
    wrote += ret;


    printf("Pass 2:  Image compression...\n"); fflush(stdout);
    while ((flag = mvi_rd_frame(&movie, curr, bbox)) >= 0)
    {
        curr_gif_time += 5;
        fr++;

        if ((flag & MVI_FR_SAME) && !early)    { skip++; same++;    continue; }
        if ( min_delay > 0 && curr_gif_time - prev_gif_time < min_delay )
        {
            skip++;
            early = 1;
            continue;
        }

        early = 0;

        delay = (curr_gif_time - prev_gif_time) / 3;

        prev_gif_time += 3*delay + dec_delay;
/*printf("skip = %d, delay = %d, curr_gif_time = %d, prev_gif_time = %d\n", skip, delay, curr_gif_time, prev_gif_time);*/

        for (i = 0; i < movie.x_dim * movie.y_dim; i++)
            prev[i] = color_map[prev[i]];

        if ( !stretch )
        {
            ret = gif_wr_frame_m(&gif, prev, delay, mode);
        } else
        {
            for (i = 0; i < movie.x_dim * movie.y_dim; i++)
                scaled[ i*2 + 0 ] = scaled[ i*2 + 1 ] = prev[i];

            ret = gif_wr_frame_m(&gif, scaled, delay, mode);
        }

        if (ret == 0)
        {
            memcpy(prev, curr, movie.x_dim * movie.y_dim);
            prev_gif_time -= 3*delay + dec_delay;
            skip++;
            continue;
        }
        if (ret < 0)
        {
            fprintf(stderr, "Error writing frame %d of GIF file %s\n",
                    fr - 1, argv[2]);
            exit(1);
        }
        wrote += ret;

        memcpy(prev, curr, movie.x_dim * movie.y_dim);
        skip = 0;
        out_fr++;
/*if (out_fr == 100) break;*/
    }

    /* write the last frame. */
    curr_gif_time += 5;
    delay = (curr_gif_time - prev_gif_time) / 3;

    for (i = 0; i < movie.x_dim * movie.y_dim; i++)
        prev[i] = color_map[prev[i]];

    if ( !stretch )
    {
        ret = gif_wr_frame_m(&gif, prev, delay, mode);
    } else
    {
        for (i = 0; i < movie.x_dim * movie.y_dim; i++)
            scaled[ i*2 + 0 ] = scaled[ i*2 + 1 ] = prev[i];

        ret = gif_wr_frame_m(&gif, scaled, delay, mode);
    }
    if (ret < 0)
    {
        fprintf(stderr, "Error writing frame %d of GIF file %s\n",
                fr - 1, argv[2]);
        exit(1);
    }
    wrote += ret;

    ret = gif_finish(&gif);
    if (ret < 0)
    {
        fprintf(stderr, "Error terminating GIF file %s\n", argv[2]);
        exit(1);
    }
    wrote += ret;
    fclose(fo);

    printf("Decoded %d source frames (%d dupes, %d dropped)\n",
            fr, same, fr - out_fr - same);
    printf("Encoded %d unique frames\n", out_fr);
    printf("Encoded %d bytes (%d bytes/source frame, %d bytes/unique frame)\n",
            wrote, wrote / fr, wrote / out_fr);
    printf("GIF frame type breakdown:\n");
    for (i = 0; i < 6; i++)
        printf("%-65s%10d\n", typedesc[i], gif_best_stat[i]);

    return 0;
}


