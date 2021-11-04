/* ======================================================================== */
/*  TOHEX                                                       J. Zbiciak  */
/*                                                                          */
/*  Reads a file (or a pipe) containing an arbitrary file, producing a      */
/*  hex-dump.  The hex-dump file is written in the following format:        */
/*                                                                          */
/*  23 69 6E 63 6C 75 64 65   20 3C 73 74 64 69 6F 2E  #  #include <stdio.  */
/*  68 3E 0A                                           #  h>.               */
/*                                                                          */
/*  eg. A list of hexidecimal bytes followed by a comment block containing  */
/*  the ASCII rendering of the characters.  Non-printing characters are     */
/*  shown as dots.                                                          */
/*                                                                          */
/*  Usage:  tohex [infile [outfile]]                                        */
/*                                                                          */
/*  The files 'infile' and 'outfile' default to 'stdin' and 'stdout',       */
/*  respectively.  If 'infile' is given as '-', then 'stdin' is used.       */
/*  This allows specifying an output file while taking input from a pipe.   */
/* ======================================================================== */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
            "Usage: tohex [infile [outfile]]\n\n"
            "  'infile' and 'outfile' default to stdin and stdout.\n\n"
            "  Use 'tohex - outfile' to use stdin for infile while\n"
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
        if (!(i=fread(buf,1,16,fi))) break;

        line[52]='#';
        line[79]='\n';

        for (j=i;j<16;j++)
        {
            k=1+j*3+2*(j>7);
            line[k  ] =' ';
            line[k+1] =' ';
            line[k+2] =' ';
            line[54+j]=' ';
        }

        for (j=0;j<i;j++)
        {
            k=1+j*3+2*(j>7);
            line[k  ] =hex[(buf[j]>>4)&0x0f];
            line[k+1] =hex[buf[j]&0x0f];
            line[54+j]=(buf[j]>=32)&&(buf[j]<127)?buf[j]:'.';
        }

        for (j = 0; j < 8; j++)
            line[78 - j] = hex[(addr >> (j * 4)) & 0xF];

        fwrite(line,1,80,fo);

        addr += 16;
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
