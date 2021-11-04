/* ======================================================================== */
/*  TOBIT                                                       J. Zbiciak  */
/*                                                                          */
/*  Reads a file (or a pipe) containing an arbitrary file, producing a      */
/*  bit-dump.  The bit-dump file is written in the following format:        */
/*                                                                          */
/*  0 1 1 0 1 0 0 1   0 1 0 0 0 0 1 0             # 69 42  ib  00000000     */
/*                                                                          */
/*  eg. A list of bits followed by a comment block containint the HEX and   */
/*  ASCII rendering of the bits.  Non-printing characters are shown as      */
/*  dots.                                                                   */
/*                                                                          */
/*  Usage:  tobit [infile [outfile]]                                        */
/*                                                                          */
/*  The files 'infile' and 'outfile' default to 'stdin' and 'stdout',       */
/*  respectively.  If 'infile' is given as '-', then 'stdin' is used.       */
/*  This allows specifying an output file while taking input from a pipe.   */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ======================================================================== */
/*  MAIN -- Most of the action happens here.                                */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    const char hex[]="0123456789ABCDEF";
    char buf[16];
    char line[80];
    int i,j,k;
    unsigned addr = 0;
    FILE * fi=stdin, * fo=stdout;

    errno = 0;

    for (i=0;i<80;i++) line[i]=' ';

    /* ==================================================================== */
    /*  Check to see if the user is asking for help.                        */
    /* ==================================================================== */
    if (argc>1 && (!strcmp(argv[1],"-?")     ||
                   !strcmp(argv[1],"-h")     ||
                   !strcmp(argv[1],"--help")))
    {
        fprintf(stderr,
            "Usage: tobit [infile [outfile]]\n\n"
            "  'infile' and 'outfile' default to stdin and stdout.\n\n"
            "  Use 'tobit - outfile' to use stdin for infile while\n"
            "  still specifying an output file.\n\n");
        exit(1);
    }

    /* ==================================================================== */
    /*  Open the input file (if specified).                                 */
    /* ==================================================================== */
    if (argc>1 && argv[1][0] != '-')
    {
        fi=fopen(argv[1],"rb");

        if (!fi)
        {
            perror("fopen()");
            fprintf(stderr,"Couldn't open '%s' for reading.\n",argv[1]);
            exit(1);
        }
    }

    /* ==================================================================== */
    /*  Open the output file (if specified).                                */
    /* ==================================================================== */
    if (argc>2)
    {
        fo=fopen(argv[2],"w");

        if (!fo)
        {
            perror("fopen()");
            fprintf(stderr,"Couldn't open '%s' for reading.\n",argv[2]);
            exit(1);
        }
    }

    /* ==================================================================== */
    /*  Read the input file 16 bytes at a time, outputting the hex dump.    */
    /* ==================================================================== */
    while (!feof(fi))
    {
        if (!(i=fread(buf,1,4,fi))) break;

        memset(line, ' ', 79);
        line[52]='#';
        line[79]='\n';

        for (j = 0; j < i; j++)
        {
            for (k = 0; k < 8; k++)
                line[2 + j*12 + k + (k>3)] = '0' + ((buf[j] >> (7 - k)) & 1);

            line[54 + j*3] = hex[0xF & (buf[j] >> 4)];
            line[55 + j*3] = hex[0xF & (buf[j]     )];
            line[66 + j  ] = (buf[j] >= 32) && (buf[j] < 127) ? buf[j] : '.';
        }

        for (j = 0; j < 8; j++)
            line[78 - j] = hex[(addr >> (j * 4)) & 0xF];

        fwrite(line,1,80,fo);

        addr += 4;
    }

    return errno;
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
