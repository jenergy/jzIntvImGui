/* ======================================================================== */
/*  FROMHEX                                                     J. Zbiciak  */
/*                                                                          */
/*  Reads a file (or a pipe) containing a hex-dump, producing a binary      */
/*  file.  The hex-dump file should be in the following format:             */
/*                                                                          */
/*     AA BB CC DD EE ...  [# optional comment]                             */
/*                                                                          */
/*  eg. A list of hexidecimal bytes optionally followed by a comment.       */
/*  The hexadecimal numbers can be either upper or lower case.  Comments    */
/*  start with a # and end at the next newline.  Lines may be arbitrarily   */
/*  long.                                                                   */
/*                                                                          */
/*  Usage:  fromhex [infile [outfile]]                                      */
/*                                                                          */
/*  The files 'infile' and 'outfile' default to 'stdin' and 'stdout',       */
/*  respectively.  If 'infile' is given as '-', then 'stdin' is used.       */
/*  This allows specifying an output file while taking input from a pipe.   */
/* ======================================================================== */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LOCAL
#define LOCAL static
#endif

/* ======================================================================== */
/*  LINE_T -- Arbitrary-length line structure.                              */
/* ======================================================================== */

typedef struct line_t
{
    char * line;
    int    len;
    struct line_t * next;
} line_t;


/* ======================================================================== */
/*  GET_LINE_T  -- allocates a line_t                                       */
/* ======================================================================== */
#define POOL_FILL (16)
static line_t * line_t_pool           = NULL;

#ifdef MEM_POOL_STATS
int             line_t_alloc          = 0;
int             line_t_pool_depth     = 0;
int             line_t_pool_depth_max = 0;
#endif

LOCAL line_t * get_line_t(void)
{
    line_t *line_t_ptr = line_t_pool;

    if (!line_t_ptr)
    {
        int i;

        line_t_ptr = line_t_pool = (line_t*)malloc(sizeof(line_t) * POOL_FILL);

        if (!line_t_ptr)
        {
            perror("get_line_t(): malloc()");
            fprintf(stderr,"Could not allocate new line_t structure\n");
            exit(1);
        }

        for (i = 0; i < POOL_FILL-1; i++)
            line_t_pool[i].next = &line_t_pool[i+1];

        line_t_pool[POOL_FILL-1].next = 0;

#ifdef MEM_POOL_STATS
        line_t_pool_depth += POOL_FILL;
        if (line_t_pool_depth > line_t_pool_depth_max)
            line_t_pool_depth_max = line_t_pool_depth;
#endif
    }

    line_t_pool      = line_t_ptr->next;
    line_t_ptr->line = NULL;
    line_t_ptr->next = NULL;
    line_t_ptr->len  = 0;

#ifdef MEM_POOL_STATS
    line_t_alloc++;
    line_t_pool_depth--;
#endif

    return line_t_ptr;
}

/* ======================================================================== */
/*  PUT_LINE_T  -- deallocates a line_t                                     */
/* ======================================================================== */

LOCAL void put_line_t(line_t *line_t_ptr)
{
    if (line_t_ptr->line)
    {
        free(line_t_ptr->line);
        line_t_ptr->line = NULL;
    }

    line_t_ptr->next = line_t_pool;
    line_t_pool      = line_t_ptr;

#ifdef MEM_POOL_STATS
    line_t_alloc--;
    line_t_pool_depth++;
    if (line_t_pool_depth > line_t_pool_depth_max)
        line_t_pool_depth_max = line_t_pool_depth;
#endif
}


#define MAXCHUNK 128    /* Controls how much of a line is read at one time */

/* ======================================================================== */
/*  READ_LONG_LINE -- Read an arbitrarily long line.                        */
/*                                                                          */
/*  Reads an arbitrary length line, dynamically allocating memory to hold   */
/*  it. Works by reading input file in chunks up to MAXCHUNK bytes long     */
/*  up to a newline.  Chunks are then coalesced into a single line.         */
/* ======================================================================== */
LOCAL line_t * read_long_line(FILE * f)
{
    line_t * final_chunk;
    line_t * chunk;
    line_t * head_chunk=NULL;
    line_t * tail_chunk=NULL;
    char * s;
    size_t len;

    /* ==================================================================== */
    /*  Check for EOF.  If we're at the end, return a NULL line_t*.         */
    /* ==================================================================== */
    if (feof(f))
        return NULL;

    /* ==================================================================== */
    /*  Now read as many chunks as we can until we reach a newline.         */
    /* ==================================================================== */
    do
    {
        /* ================================================================ */
        /*  Allocate a "chunk" structure.  A very long line will exist as   */
        /*  multiple "chunks" that we will later paste together into a      */
        /*  single C-style string.                                          */
        /* ================================================================ */
        chunk = get_line_t();

        /* ================================================================ */
        /*  Allocate the actual line-buffer within the chunk.               */
        /* ================================================================ */
        chunk->line = (char *) malloc(MAXCHUNK);

        if (!chunk->line)
        {
            perror("read_long_line(): malloc()");
            fprintf(stderr,"Could not allocate new chunk structure\n");
            exit(1);
        }

        /* ================================================================ */
        /*  Append the chunk to the linked list of chunks.                  */
        /* ================================================================ */
        if (tail_chunk)
        {
            tail_chunk->next=chunk;
            tail_chunk=chunk;
        } else
        {
            head_chunk=tail_chunk=chunk;
        }

        /* ================================================================ */
        /*  Now read in up to either (a) MAXCHUNK characters, (b) EOL, or   */
        /*  (c) EOF.  If we are at EOF, stop now.                           */
        /* ================================================================ */
        if (!fgets(chunk->line,MAXCHUNK-1,f))
            break;  /* EOF */

        /* ================================================================ */
        /*  Normalize newlines                                              */
        /* ================================================================ */
        for (s = chunk->line; *s && s < chunk->line + MAXCHUNK - 1; s++)
            if (*s == '\n' || *s == '\r')
            {
                s[0] = '\n';
                s[1] = 0;
                break;
            }

        /* ================================================================ */
        /*  Make sure chunk is NUL-terminated and figure out its length.    */
        /* ================================================================ */
        chunk->line[MAXCHUNK-1] = 0;
        chunk->len = strlen(chunk->line);

        /* ================================================================ */
        /*  Is this the last chunk of the line?                             */
        /* ================================================================ */
        if (chunk->line[chunk->len-1] == '\n')
        {
            /* ============================================================ */
            /*  Newline -> Found it!  Now get rid of it!                    */
            /* ============================================================ */
            chunk->line[--chunk->len] = 0;

            /* ============================================================ */
            /*  Break out to stop grabbing more chunks.                     */
            /* ============================================================ */
            break;
        }

    } while (!feof(f));

    /* ==================================================================== */
    /*  Special case: no chunks.  Return NULL.                              */
    /* ==================================================================== */
    if (!head_chunk)
    {
        return NULL;
    }

    /* ==================================================================== */
    /*  Special case: zero-length chunk -> EOF.  Return NULL.               */
    /* ==================================================================== */
    if (head_chunk->len==0 && feof(f))
    {
        put_line_t(head_chunk);
        return NULL;
    }

    /* ==================================================================== */
    /*  Special case: only one chunk, just resize allocated region to size  */
    /*  of string and return.                                               */
    /* ==================================================================== */
    if (head_chunk==tail_chunk)
    {
        chunk=head_chunk;
        if (chunk->len+1 < MAXCHUNK)
            chunk->line = (char *)realloc(chunk->line, chunk->len + 1);
        chunk->line[chunk->len] = 0;
        return chunk;
    }

    /* ==================================================================== */
    /*  Ok. This string has multiple chunks that we need to put together.   */
    /*  Determine total length of string by stepping through list of        */
    /*  chunks, tallying the lengths of the individual chunks.              */
    /* ==================================================================== */
    chunk = head_chunk;
    len = 0;

    while (chunk)
    {
        len  += chunk->len;
        chunk = chunk->next;
    }

    /* ==================================================================== */
    /*  Ok, now that we know the total length of the line, we can realloc   */
    /*  the first chunk to the total length of the string, and concatenate  */
    /*  the remaining chunks after it.  We can then free these remaining    */
    /*  chunks as we go.                                                    */
    /* ==================================================================== */

    final_chunk       = head_chunk;
    final_chunk->line = (char *)realloc(final_chunk->line, len+1);
    s                 = final_chunk->line + final_chunk->len;
    if (!final_chunk->line)
    {
        perror("read_long_line(): realloc()");
        fprintf(stderr,"Could not reallocate line buffer of %ld bytes\n",
                (long)len+1);
        exit(1);
    }


    head_chunk        = head_chunk->next;
    while (head_chunk)
    {
        strcpy(s, head_chunk->line);
        s         += head_chunk->len;
        chunk      = head_chunk;
        head_chunk = head_chunk->next;
        put_line_t(chunk);
    }

    final_chunk->next = NULL;
    final_chunk->len  = len;

    /* ==================================================================== */
    /*  Done.  Return the final chunk.  It contains the fully concatenated  */
    /*  string.                                                             */
    /* ==================================================================== */

    return final_chunk;
}



/* ======================================================================== */
/*  MAIN -- Most of the action happens here.                                */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    line_t *line;
    char buf[1024];
    char *s, *o;
    int c;
    FILE *fi = stdin, *fo = stdout;

    /* ==================================================================== */
    /*  Check to see if the user is asking for help.                        */
    /* ==================================================================== */
    if (argc>1 && (!strcmp(argv[1],"-?")     ||
                   !strcmp(argv[1],"-h")     ||
                   !strcmp(argv[1],"--help")))
    {
        fprintf(stderr,
            "Usage: fromhex [infile [outfile]]\n\n"
            "  'infile' and 'outfile' default to stdin and stdout.\n\n"
            "  Use 'fromhex - outfile' to use stdin for infile while\n"
            "  still specifying an output file.\n\n");
        exit(1);
    }

    /* ==================================================================== */
    /*  Open the input file (if specified).                                 */
    /* ==================================================================== */
    if (argc>1 && argv[1][0] != '-')
    {
        fi=fopen(argv[1],"r");
        if (!fi)
        {
            perror("fopen()");
            fprintf(stderr,"Couldn't open '%s' for reading\n",argv[1]);
            exit(1);
        }
    }

    /* ==================================================================== */
    /*  Open the output file (if specified).                                */
    /* ==================================================================== */
    if (argc>2)
    {
        fo=fopen(argv[2],"wb");
        if (!fo)
        {
            perror("fopen()");
            fprintf(stderr,"Couldn't open '%s' for writing\n",argv[2]);
            exit(1);
        }
    }

    /* ==================================================================== */
    /*  Read lines and de-hexify them.                                      */
    /* ==================================================================== */
    o = buf;
    while ( (line = read_long_line(fi)) != NULL)
    {
        s = line->line;

        while (s && *s)
        {
            while (*s==' ') s++;
            if (*s=='\n' || *s=='#' || *s==0 || !sscanf(s,"%x",&c)) break;
            while (*s && *s!=' ') s++;

            *o++ = c;
            if ((size_t)(o - buf) >= sizeof(buf))
            {
                fwrite(buf,1,o-buf,fo);
                o = buf;
            }
        }

        put_line_t(line);
    }

    if (o - buf > 0)
        fwrite(buf,1,o-buf,fo);

    fclose(fi);
    fclose(fo);

#ifdef MEM_POOL_STATS
    if (getenv("MEM_POOL_STATS"))
    {
        fprintf(stderr,"Outstanding line_t's:  %10d\n", line_t_alloc);
        fprintf(stderr,"Final pool depth:      %10d\n", line_t_pool_depth);
        fprintf(stderr,"Peak pool depth:       %10d\n", line_t_pool_depth_max);
    }
#endif

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
