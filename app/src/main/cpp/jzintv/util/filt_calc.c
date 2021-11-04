/* accepts two bitreversed numbers, returns the poles of the filter. */
#define BITREV

#include <stdio.h>
#include <math.h>

int iq_rom[128] =
{
    0,      9,      12,     25,     33,     41,     49,     57,
    65,     73,     81,     89,     97,     105,    113,    121,
    129,    137,    145,    153,    161,    169,    177,    185,
    193,    201,    209,    217,    225,    233,    241,    249,
    257,    265,    273,    281,    289,    297,    301,    305,
    309,    313,    317,    321,    325,    329,    333,    337,
    341,    345,    349,    353,    357,    361,    365,    369,
    373,    377,    381,    385,    389,    393,    397,    401,
    405,    409,    413,    417,    421,    425,    427,    429,
    431,    433,    435,    437,    439,    441,    443,    445,
    447,    449,    451,    453,    455,    457,    459,    461,
    463,    465,    467,    469,    471,    473,    475,    477,
    479,    481,    482,    483,    484,    485,    486,    487,
    488,    489,    490,    491,    492,    493,    494,    495,
    496,    497,    498,    499,    500,    501,    502,    503,
    504,    505,    506,    507,    508,    509,    510,    511
};

typedef struct cplx
{
    double r, i;
    double a, m;
} cplx;

int calc_poles(double Ft, double Bt, cplx *pole, double Fs,
               double *Fk, double *BW)
{
    double discrim;
    double root;
    int    sign = 0;

    discrim = Ft * Ft + Bt;
    if (discrim < 0) { sign = 1; discrim = -discrim; }
    root    = sqrt(discrim);

    if (sign)
    {
        pole[0].r =  (pole[1].r = Ft);
        pole[0].i = -(pole[1].i = root);

        pole[0].m = pole[1].m = sqrt(Ft*Ft + discrim);
        pole[0].a = atan2(pole[0].i, pole[0].r);
        pole[1].a = atan2(pole[1].i, pole[1].r);

        Fk[0] =  Fs * pole[1].a / (2.0*M_PI);
        BW[0] = -Fs * log(Bt * Bt) / M_PI;
        BW[1] = 0;
    } else
    {
        int i;

        pole[0].i = pole[1].i = 0;
        pole[0].r = Ft + root;
        pole[1].r = Ft - root;

        pole[0].m = fabs(Ft + root);
        pole[1].m = fabs(Ft - root);
        pole[0].a = pole[0].r > 0 ? 0 : M_PI;
        pole[1].a = pole[1].r > 0 ? 0 : M_PI;

        for (i = 0; i < 2; i++)
        {
            if (pole[i].r < 0)
            {
                Fk[i] = Fs / 2.0;
                BW[i] = -2 * Fs * log(-pole[i].r);
            } else
            {
                Fk[i] = 0;
                BW[i] = -2 * Fs * log(pole[i].r);
            }
        }
    }

    return sign;
}

void show_poles(cplx *pole, int num_poles)
{
    int x, y, c, p;
    double bb_x0, bb_x1, bb_y0, bb_y1;
    double sq_x0, sq_x1, sq_y0, sq_y1;
    int num_inside, ws;

    for (p = 0; p < num_poles; p++)
        printf("%2d: [%10f,%10f]   %s\n",
                p + 1, pole[p].r, pole[p].i,
                pole[p].m >= 1.0 ? "Unstable" : "Stable");

    for (y = -12; y <= +12; y++)
    {
        bb_y0 = y / 10.0 - 0.05;
        bb_y1 = y / 10.0 + 0.05;

        sq_y0 = bb_y0 * bb_y0;
        sq_y1 = bb_y1 * bb_y1;

        ws = 0;

        for (x = -32; x <= +32; x++)
        {
            bb_x0 = x / 20.0 - 0.025;
            bb_x1 = x / 20.0 + 0.025;

            sq_x0 = bb_x0 * bb_x0;
            sq_x1 = bb_x1 * bb_x1;

            c = ' ';

            if (y == 0) c = '-';
            if (x == 0) c = y == 0 ? '+' : '|';

            num_inside  = (sq_x0 + sq_y0) < 1.0;
            num_inside += (sq_x0 + sq_y1) < 1.0;
            num_inside += (sq_x1 + sq_y0) < 1.0;
            num_inside += (sq_x1 + sq_y1) < 1.0;

            if (num_inside == 2 || num_inside == 3) c = '*';

            for (p = 0; p < num_poles; p++)
            {
                if (pole[p].r >= bb_x0 && pole[p].r <= bb_x1 &&
                    pole[p].i >= bb_y0 && pole[p].i <= bb_y1)
                {
                    c = '1' + p;
                }
            }

            if (c == ' ') ws++;
            else
            {
                printf("%*s%c", ws, "", c);
                ws = 0;
            }
        }
        putchar('\n');
    }

}

double bitdecode(char *str)
{
    int val = 0, iq_val;
    int len = 0;
    char *s;
    double final;

#ifdef BITREV
    for (s = str; *s; s++, len++)
    {
        if (*s == '1') val |= 1u << len;
        else if (*s != '0')
        {
            fprintf(stderr, "Format error in '%s'\n", str);
            exit(1);
        }
    }

    if (len > 8)
    {
        fprintf(stderr, "Field too long (max 8): '%s'\n", str);
        exit(1);
    }

    /* Left justify */
    if (len < 8) val <<= 8 - len;
#else
    for (s = str; *s; s++, len++)
    {
        if (*s == '1') val |= 0x80 >> len;
        else if (*s != '0')
        {
            fprintf(stderr, "Format error in '%s'\n", str);
            exit(1);
        }
    }

    if (len > 8)
    {
        fprintf(stderr, "Field too long (max 8): '%s'\n", str);
        exit(1);
    }
#endif


#if 0
    iq_val = iq_rom[val & 0x7F];

    if (val & 0x80) final =  iq_val / 512.0;
    else            final = -iq_val / 512.0;
#else
    if (val & 0x80) final =  iq_rom[(-val) & 0x7F] / 512.0;
    else            final = -iq_rom[  val        ] / 512.0;
#endif

    return final;
}

void calc_impulse_resp(double Ft, double Bt, double *s, int cnt)
{
    double z0, z1, samp;
    int i;

    s[0] = samp = 1.0;
    z0 = z1 = 0.0;
    for (i = 1; i < cnt; i++)
    {
        samp = samp + 2.0 * Ft * z0 + Bt * z1;

        z1   = z0;
        z0   = samp;
        s[i] = samp > 1.0 ? 1.0 : samp < -1.0 ? -1.0 : samp;
        samp = 0.0;
    }
}

void show_impulse_resp(double *s, int cnt)
{
    int y, x, c0, c1, ws, uy, ly, my;

    for (y = 12; y >= -12; y--)
    {
        ws = 0;
        for (x = 0; x < cnt; x++)
        {
            c0 = y == 0 ? '-' : ' ';
            c1 = x % 10 == 0 ? '|' : y == 0 ? x % 2 == 0 ? '|' : '-' : ' ';

            if (x == 0)
            {
                uy = ly = my = floor(s[x]   * 10.0 + 0.5);
            } else  if (s[x] > s[x-1])
            {
                ly = floor(s[x-1] * 10.0 + 0.5);
                uy = floor(s[x]   * 10.0 + 0.5);
                my = uy;
            } else
            {
                uy = floor(s[x-1] * 10.0 + 0.5);
                ly = floor(s[x]   * 10.0 + 0.5);
                my = ly;
            }

            if (y > ly && y < uy)   c0 = '*';
            if (y == ly || y == uy) c0 = '*';

            if (y == my) c1 = '*';


            if (c0== ' ') ws++;
            else
            {
                printf("%*c", ws+1, c0);
                ws = 0;
            }

            if (c1== ' ') ws++;
            else
            {
                printf("%*c", ws+1, c1);
                ws = 0;
            }
        }
        putchar('\n');
    }
}

main(int argc, char *argv[])
{
    double Fs = 10000;
    double Ft, Bt, Fk[2], BW[2];
    cplx   pole[2];
    int    is_cplx;
    double impulse[80];

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s bitfield bitfield\n", argv[0]);
        exit(1);
    }

    Bt = bitdecode(argv[1]);
    Ft = bitdecode(argv[2]);

    printf("Ft: %10f    Bt: %10f\n", Ft, Bt);

    is_cplx = calc_poles(Ft, Bt, pole, Fs, Fk, BW);
    if (is_cplx) printf("Fk: %10.3fHz  BW: %10.3fHz\n", Fk[0], BW[0]);
    else
    {
        printf("Fk: %10.3fHz  BW: %10.3fHz\n",Fk[0],BW[0]);
        printf("Fk: %10.3fHz  BW: %10.3fHz\n",Fk[1],BW[1]);
    }
    show_poles(pole, 2);

    calc_impulse_resp(Ft, Bt, impulse, 39);
    show_impulse_resp(impulse, 39);

    return 0;
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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
