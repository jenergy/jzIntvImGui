
#include "config.h"
#include "mvi/mvi.h"
#include "gif/gif_enc.h"
#include "gif/lzw_enc.h"

#ifdef HAS_LINK
#include <unistd.h>
#endif


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

    /* for debug */
    { 0xFF, 0x80, 0x80 },
};

uint8_t curr[MVI_MAX_X * MVI_MAX_Y];
uint8_t lbuf[MVI_MAX_X * 2][3];
uint8_t bbox[8][4];


int main(int argc, char *argv[])
{
    FILE *fi, *fo;
    int fr;
    int i, j, flag, len;
    char *fname, *fprev;
    int mode = 0;

    if (argc == 4 && argv[1][0] == '-' && argv[1][1] == '\0')
    {
        mode = 1;
        argc--;
        argv++;
    }

    if (argc != 3)
    {
        fprintf(stderr, "%s [-] input.imv output.gif\n", argv[0]);
        exit(1);
    }

    mvi_init(&movie, MVI_MAX_X, MVI_MAX_Y);

    len   = strlen(argv[2]) + 16;
    fname = (char *)malloc(len);
    fprev = (char *)malloc(len);

    fi = fopen(argv[1], "rb");
    if (!fi)
    {
        perror("fopen()");
        fprintf(stderr, "Could not open %s for reading\n", argv[1]);
        exit(1);
    }


    memset(movie.vid, 16, MVI_MAX_X * MVI_MAX_Y);
    movie.f = fi;
    fr = 0;

    printf("Expanding IMV to PPM files...\n"); fflush(stdout);
    while ((flag = mvi_rd_frame(&movie, curr, bbox)) >= 0)
    {
        char *ftmp;

        ftmp  = fname;
        fname = fprev;
        fprev = ftmp;

        snprintf(fname, len, "%s_%.05d.ppm", argv[2], fr);

#ifdef HAS_LINK
        if (fr > 0 && (flag & MVI_FR_SAME) == MVI_FR_SAME)
        {
            if (link(fprev, fname) == 0)
            {
                fr++;
                continue;
            }
            /* If link failed, pretend like we didn't try... */
        }
#endif

        if (!(fo = fopen(fname, "wb")))
        {
            perror("fopen()");
            fprintf(stderr, "Could not open '%s' for writing.\n", fname);
            exit(1);
        }

        printf("\r%s ...", fname); fflush(stdout);


        if (mode == 0 || movie.x_dim != 160 || movie.y_dim != 200)
        {
            fprintf(fo, "P6\n%d %d 255\n", movie.x_dim, movie.y_dim);

            for (i = 0; i < movie.y_dim; i++)
            {
                for (j = 0; j < movie.x_dim; j++)
                {
                    lbuf[j][0] = palette[curr[j + i*movie.x_dim]][0];
                    lbuf[j][1] = palette[curr[j + i*movie.x_dim]][1];
                    lbuf[j][2] = palette[curr[j + i*movie.x_dim]][2];
                }

                fwrite(lbuf, 3, movie.x_dim, fo);
            }
        } else
        {
            fprintf(fo, "P6\n352 288 255\n");

            /* assume curr[16] is border color */
            for (i = 0; i < 352; i++)
            {
                lbuf[i][0] = palette[curr[16]][0];
                lbuf[i][1] = palette[curr[16]][1];
                lbuf[i][2] = palette[curr[16]][2];
            }

            for (i = 0; i < 44; i++)
                fwrite(lbuf, 3, 352, fo);

            for (i = 0; i < movie.y_dim; i++)
            {
                for (j = 0; j < movie.x_dim; j++)
                {
                    lbuf[2*j+16][0] = palette[curr[j + i*movie.x_dim]][0];
                    lbuf[2*j+17][0] = palette[curr[j + i*movie.x_dim]][0];

                    lbuf[2*j+16][1] = palette[curr[j + i*movie.x_dim]][1];
                    lbuf[2*j+17][1] = palette[curr[j + i*movie.x_dim]][1];

                    lbuf[2*j+16][2] = palette[curr[j + i*movie.x_dim]][2];
                    lbuf[2*j+17][2] = palette[curr[j + i*movie.x_dim]][2];
                }

                fwrite(lbuf, 3, 352, fo);
            }

            /* assume curr[16] is border color */
            for (i = 0; i < 352; i++)
            {
                lbuf[i][0] = palette[curr[16]][0];
                lbuf[i][1] = palette[curr[16]][1];
                lbuf[i][2] = palette[curr[16]][2];
            }

            for (i = 0; i < 44; i++)
                fwrite(lbuf, 3, 352, fo);
        }

        fclose(fo);

        fr++;
    }

    printf("\nDone!\n");

    return 0;
}
