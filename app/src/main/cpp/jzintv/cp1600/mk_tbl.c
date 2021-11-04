/*
 * ============================================================================
 *  MK_TBL:         Makes a lookup table from a description file
 *
 *  Author:         J. Zbiciak
 *
 *
 * ============================================================================
 *
 *  Makes a redundantly coded table in "N" bits.
 *
 *  Takes a description of the form "0xxx1010xxx entry", and expands out all
 *  the 'x's.  Processes entries in order, so exceptions to a don't-care can
 *  be handled:
 *
 *    00xx, "entry 1"
 *    0000, "entry 2"
 *
 *  This causes 0000 to have "entry 2" and 0001, 0010, 0011 to have "entry 1".
 *
 *  The table description file should contain a single integer on the first
 *  non-comment line which corresponds to the number of bits that the table
 *  encodes.  The remaining lines should contain patterns as described above.
 *
 *  Usage:  make_tbl table.tbl table.c
 * ===========================================================================
 */


#include "config.h"

#define MAX_BITS  (20)
#define MAX_ENTRY (1024)
#define MAX_LINE  (1024)

/*
 * ===========================================================================
 *  RENDER_BITS     -- converts value into a string of 1's and 0's
 *
 *  Notes:  This renders into a static buffer, and so it can't be used for
 *          multiple %s fields in a printf().  Sorry.
 * ===========================================================================
 */
const char * render_bits(unsigned value, int bits)
{
    int i;
    static char string[32];
    char * s = string;

    for (i = bits-1; i >= 0; i--)
        *s++ = ( ((value >> i) & 1) + '0' );

    return string;
}

/*
 * ===========================================================================
 *  EXPAND_DC       -- expands the don't-cares in a description line.
 *
 *  This function isn't terribly efficient, but then it doesn't need to be.
 * ===========================================================================
 */
void expand_dc(char *tbl[], char *pattern, char *entry, int bits)
{
    int len, num_dc;
    char *s;
    int dc_tmp[MAX_BITS], dc_pos[MAX_BITS];
    int j;
    unsigned pat = 0, msk = 0, idx, i;


    /* -------------------------------------------------------------------- */
    /*  Scan the pattern buffer looking for "don't cares", and note their   */
    /*  positions.  Also, build a bit mask for the bits that aren't DC's.   */
    /* -------------------------------------------------------------------- */
    for (len = num_dc = 0, s = pattern; *s=='1' || *s=='0' || *s=='x'; s++)
    {
        pat <<= 1;
        pat  |= *s == '1';

        msk <<= 1;
        if (*s == 'x')
        {
            dc_tmp[num_dc++] = len;
        } else
        {
            msk |= 1;
        }
        len++;

        if (len > bits || num_dc > bits)
        {
            fprintf(stderr,"Oversized pattern encountered in expand_dc()\n");
            exit(1);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  At this point, the dc_tmp array contains a list of all the don't-   */
    /*  care positions.  Re-order the array so that we can quickly fill in  */
    /*  the don't-care positions in the next loop.  This essentially does   */
    /*  an endian swap bitwise on the recorded don't-care positions.        */
    /* -------------------------------------------------------------------- */

    for (j = 0; j < num_dc; j++)
    {
        dc_pos[num_dc - j - 1] = len - dc_tmp[j] - 1;
    }

    /* -------------------------------------------------------------------- */
    /*  Now, fill in the table by expanding all of the don't-cares.         */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < (1U<<num_dc); i++)
    {
        idx = pat & msk;

        for (j = 0; j < num_dc; j++)
        {
            idx |= (i << (dc_pos[j] - j)) & (1 << dc_pos[j]);
        }

        tbl[idx] = entry;
    }
}

/*
 * ===========================================================================
 *  GRAB_QUOTED_STRING  -- Reads a quoted string out of an input line.
 *
 *  Reads a quoted string from an input line.  Accepts a pointer to the
 *  first character (which must be the opening quote), and returns a pointer
 *  to the last character (which is the closing quote), or NULL on failure.
 * ===========================================================================
 */
char * grab_quoted_string
(
    char *in,
    char *out,
    int   max_len
)
{
    char *s, *es;
    int   p;

    s  = in;
    es = out;

    /* -------------------------------------------------------------------- */
    /*  Initial sanity check.  Do not proceed if this fails.                */
    /* -------------------------------------------------------------------- */
    if (!s || !es || *s++ != '"')
        return NULL;

    /* -------------------------------------------------------------------- */
    /*  Keep processing until we hit the closing quote.  If we don't get    */
    /*  the closing quote, or if we hit a syntax error in the string, or    */
    /*  if the string is too long, we fail and return NULL.                 */
    /* -------------------------------------------------------------------- */
    while (*s && *s != '"')
    {
        p = *s++;

        if (p == '\\')
        {
            p = *s++;

            switch (p)
            {
                case  0  : return NULL; break;
                case 'a' : p = '\a'; break;
                case 'b' : p = '\b'; break;
                case 'n' : p = '\n'; break;
                case 'r' : p = '\r'; break;
                case 't' : p = '\t'; break;
                case 'v' : p = '\v'; break;
                case 'x' :
                {
                    int x1, x2;

                    if ( !(x1 = *s++) || !(x2 = *s++) ) return NULL;
                    x1 = HEXINT(toupper(x1));
                    x2 = HEXINT(toupper(x2));
                    if ( x1 < 0 || x2 < 0 ) return NULL;

                    p = (x1 << 4) | x2;

                    break;
                }
                case '0' : case '1' : case '2' : case '3' : case '4' :
                case '5' : case '6' : case '7' :
                {
                    int o1, o2, o3;

                    o1 = p;

                    if ( !(o2 = *s++) || !(o3 = *s++) ) return NULL;

                    o1 -= '0'; o2 -= '0'; o3 -= '0';

                    if ( o2 < 0 || o3 < 0 || o2 > 7 || o3 > 7 ) return NULL;

                    p = (o1 << 6) | (o2 << 3) | o3;

                    break;
                }
                default:
                    break;
                    /* just return whatever character that was escaped */
            }
        }

        *es++ = p;

        if (es - out >= max_len)
            return NULL; /* entry too long */
    }

    *es = 0;

    if (*s != '"')
        return NULL;

    return s;
}

/*
 * ===========================================================================
 *  DECODE_BITS_LINE    -- Decodes a line looking for # of bits designation
 *
 *  Format:
 *
 *  [space] [decimal number][, "default entry"] [space] [# comment]
 *
 *  Return:
 *   --  0 if the line was complete
 *   --  1 if the line was empty/comments
 *   -- -1 if the line was invalid
 * ===========================================================================
 */
int
decode_bits_line
(
    char  *input,
    int   *bits,
    char  *entry,
    int   max_entry
)
{
    char *s;
    int p,val;

    *bits = 0;

    /* -------------------------------------------------------------------- */
    /*  Skip leading whitespace and find first char of the bit pattern.     */
    /* -------------------------------------------------------------------- */
    s = input;
    while (*s && isspace(*s)) s++;

    /* -------------------------------------------------------------------- */
    /*  If we stopped at a # or NUL, just return that this is a comment.    */
    /* -------------------------------------------------------------------- */
    if (*s == '#' || !*s) return 1;

    /* -------------------------------------------------------------------- */
    /*  See if this is an integer, and if so decode it into our bit-length. */
    /*  If it isn't an integer, report an error.                            */
    /* -------------------------------------------------------------------- */
    if (!isdigit(*s))
    {
        return -1;
    }

    val = 0;
    while (*s && isdigit(*s))
    {
        p = *s++;

        val = val * 10 + p - '0';
    }

    *bits = val;

    /* -------------------------------------------------------------------- */
    /*  Next, look for any whitespace, skipping it.  If we run into end of  */
    /*  string, stop here, returning what we have.  Otherwise, look for the */
    /*  whitespace-comma-whitespace-quote sequence that preceeds a string.  */
    /* -------------------------------------------------------------------- */
    while (*s && isspace(*s)) s++;
    if (!*s || *s == '\n' || *s == '#')
    {
        strncpy(entry,"NULL,",max_entry);
        return 0;
    }
    if (*s++ != ',') return -1;

    while (*s && isspace(*s)) s++;
    if (*s != '"') return -1;

    /* -------------------------------------------------------------------- */
    /*  Now, copy the quoted string.  It should end with another quote.     */
    /*  Handle C-style escapes for \b, \x##, \###, \n, \t, \", etc..        */
    /* -------------------------------------------------------------------- */
    s = grab_quoted_string(s, entry, max_entry);

    if (!s) return -1;


    /* -------------------------------------------------------------------- */
    /*  We should be ok -- we'll just silently ignore the rest of the line. */
    /*  Return SUCCESS!                                                     */
    /* -------------------------------------------------------------------- */
    return 0;

}

/*
 * ===========================================================================
 *  DECODE_DC_LINE  -- Decodes a line, returning the pattern and entry
 *
 *  Format:
 *
 *  [space] [10x]* [space],[space] "[C-style string]" [space] # [space] [cmt]
 *  .prolog [space] "[C-style string]" [space] # [space] [cmt]
 *  .epilog [space] "[C-style string]" [space] # [space] [cmt]
 *
 *  Return:
 *   --  0 if the line was complete
 *   --  1 if the line was empty/comments
 *   --  2 if the line was a .prolog directive
 *   --  3 if the line was a .epilog directive
 *   -- -1 if the line was invalid
 * ===========================================================================
 */
int
decode_dc_line
(
    char  *input,
    char  *pattern,
    int    max_bits,
    char  *entry,
    int    max_entry
)
{
    char *s, *ps;
    int p;
    int directive = 0;

    /* -------------------------------------------------------------------- */
    /*  Skip leading whitespace and find first char of the bit pattern.     */
    /* -------------------------------------------------------------------- */
    s = input;
    while (*s && isspace(*s)) s++;


    /* -------------------------------------------------------------------- */
    /*  If we stopped at a # or NUL, just return that this is a comment.    */
    /* -------------------------------------------------------------------- */
    if (*s == '#' || !*s) return 1;

    /* -------------------------------------------------------------------- */
    /*  Detect if this was a prolog/epilog directive.                       */
    /* -------------------------------------------------------------------- */
    if (!strncmp(s, ".prolog", 7)) directive = 2; else
    if (!strncmp(s, ".epilog", 7)) directive = 3;

    if (directive)
        s += 7;

    /* -------------------------------------------------------------------- */
    /*  Copy pattern into our pattern buffer.  Do not allow the pattern     */
    /*  length to exceed "bits."  Allowed characters are [01A-Za-z._-].     */
    /*  Characters other than 0 and 1 are treated as "don't-cares" and are  */
    /*  converted into 'x' in the pattern buffer.                           */
    /* -------------------------------------------------------------------- */
    if (!directive)
    {
        ps = pattern;
        while (*s && (strchr("01._-",*s) || isalpha(*s)))
        {
            p = *s++;
            if (p != '0' && p != '1') p = 'x';
            *ps++ = p;

            if (ps - pattern > max_bits)
            {
                return -1; /* pattern too long */
            }
        }

        if (ps == pattern)
            return -1;  /* we didn't find a pattern! */

        *ps++ = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Next, look for any whitespace, skipping it.  The character after    */
    /*  the (optional) whitespace must be a comma if this isn't a           */
    /*  directive.  After the comma is some more (optional) whitespace.     */
    /*  It must be followed by a double quote (which starts the entry.)     */
    /* -------------------------------------------------------------------- */
    if (!directive)
    {
        while (*s && isspace(*s)) s++;
        if (*s++ != ',') return -1;
    }

    while (*s && isspace(*s)) s++;
    if (*s != '"') return -1;

    /* -------------------------------------------------------------------- */
    /*  Now, copy the quoted string.  It should end with another quote.     */
    /*  Handle C-style escapes for \b, \x##, \###, \n, \t, \", etc..        */
    /* -------------------------------------------------------------------- */
    s = grab_quoted_string(s, entry, max_entry);

    if (!s) return -1;


    /* -------------------------------------------------------------------- */
    /*  We should be ok -- we'll just silently ignore the rest of the line. */
    /*  Return SUCCESS!                                                     */
    /* -------------------------------------------------------------------- */
    return directive;
}

/*
 * ===========================================================================
 *  GETLINE  -- Reads a line of input.  Handles 'Unix-style' files on
 *              non-Unix platforms (eg. DOS, Mac).  Note:  this only works
 *              on files, not redirected input.
 * ===========================================================================
 */
char *getline(char *buf, size_t len, FILE *f)
{
    long r_len;
    long keep_len;
    long seek_back;
    char *s;

    /* -------------------------------------------------------------------- */
    /*  Fill up the read buffer to the top.  We'll go looking for an EOL    */
    /*  in a second.  (This fread() is the reason why we don't work on      */
    /*  pipes/streams.)  Abort on error.  (Caller can use 'errno' to find   */
    /*  out what happened.)                                                 */
    /* -------------------------------------------------------------------- */
    r_len = fread(buf, 1, len - 1, f);      /* Fill the buffer.             */
    if (r_len <= 0) return NULL;            /* Abort on error or EOF.       */
    buf[r_len] = '\0';                      /* Guarantee NUL termination.   */

    /* -------------------------------------------------------------------- */
    /*  Now scan the string looking for 'LF' or 'CR' as our EOL marker.     */
    /* -------------------------------------------------------------------- */
    s = buf;
    while (*s)
    {
        /* ---------------------------------------------------------------- */
        /*  LF == ASCII code 10.  CR == ASCII code 13.                      */
        /* ---------------------------------------------------------------- */
        if (*s == (char)10 || *s == (char)13) break;

        s++;
    }

    /* -------------------------------------------------------------------- */
    /*  If we hit the NUL terminator, then we didn't see an EOL.  Just      */
    /*  return the whole darn string!                                       */
    /* -------------------------------------------------------------------- */
    if (!*s)
        return buf;

    /* -------------------------------------------------------------------- */
    /*  Figure out how many characters we're keeping, including 1           */
    /*  character for newline.                                              */
    /* -------------------------------------------------------------------- */
    keep_len = s - buf + 1;

    /* -------------------------------------------------------------------- */
    /*  Next, figure out how many characters we need to seek back because   */
    /*  we read too far.  Note:  If we have a CR + LF in the input, we      */
    /*  need to consume both the CR and LF, but pass only one character     */
    /*  for newline back to the application.  Fun, fun.                     */
    /* -------------------------------------------------------------------- */
    seek_back = r_len - keep_len;
    if (*s == (char)13 && s[1] == (char)10) /* CR + LF */
    {
        s[1] = '\0';
        seek_back--;
    }

    /* -------------------------------------------------------------------- */
    /*  Now, push the unused characters back on the input.                  */
    /* -------------------------------------------------------------------- */
    fseek(f, -seek_back, SEEK_CUR);

    /* -------------------------------------------------------------------- */
    /*  Lastly, replace the EOL marker with the platform's expected         */
    /*  newline designator (good old '\n'), NUL-terminate it, and return    */
    /*  it to the caller.                                                   */
    /* -------------------------------------------------------------------- */
    *s++ = '\n';
    *s   = '\0';

    return buf;
}

/*
 * ===========================================================================
 *  MAIN -- Uhm... uh... uhm... yeah!  Huh-huh.
 * ===========================================================================
 */
int main(int argc, char *argv[])
{
    int bits, i;
    FILE *i_file, *o_file;
    char **tbl;
    char pattern[MAX_BITS  + 2];
    char entry  [MAX_ENTRY + 1];
    char d_entry[MAX_ENTRY + 1];
    char buf    [MAX_LINE  + 1];
    char *prolog_string = NULL;
    char *epilog_string = NULL;

    /* -------------------------------------------------------------------- */
    /*  Check to see that we've been given the appropriate number of args.  */
    /* -------------------------------------------------------------------- */
    if (argc < 3)
    {
        fprintf(stderr,"Usage:  mk_tbl table.tbl table.c\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Next, try to open our input file for reading.                       */
    /* -------------------------------------------------------------------- */
    i_file = fopen(argv[1], "rb");
    if ( !i_file )
    {
        perror(argv[1]);
        fprintf(stderr,"Unable to read input file '%s'\n", argv[1]);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Read the table size in bits from the file.  Complain if we can't    */
    /*  find it, or if its value is out of range.                           */
    /* -------------------------------------------------------------------- */
    while (getline(buf, sizeof(buf)-1, i_file) &&
            (i = decode_bits_line(buf, &bits, d_entry, MAX_ENTRY)) )
    {

        if (i < 0)
        {
            fprintf(stderr,"Syntax error in input. "
                           "Table size in bits expected.\n");
            fprintf(stderr,"--> %s\n",buf);
            exit(1);
        }

    }

    if (bits <= 0 || bits > MAX_BITS)
    {
        fprintf(stderr,"Bit size %d is out of range\n", bits);
        exit(1);
    }


    /* -------------------------------------------------------------------- */
    /*  Now that we have the table size, allocate the table and set all of  */
    /*  the entries to the default value.                                   */
    /* -------------------------------------------------------------------- */
    tbl = malloc(sizeof(char *) << bits);
    if (!tbl)
    {
        fprintf(stderr,"Out of memory allocating look-up table.\n"
                       "Try a smaller table size.\n");
        exit(1);
    }

    for (i = 0; i < (1 << bits); i++)
        tbl[i] = d_entry;

    while (getline(buf, sizeof(buf)-1, i_file))
    {
        if (! (i = decode_dc_line(buf, pattern, bits, entry, MAX_ENTRY)) )
        {
            expand_dc(tbl, pattern, strdup(entry), bits);
        }

        if (i == 2)
        {
            if (prolog_string)
                free(prolog_string);
            prolog_string = strdup(entry);
        }

        if (i == 3)
        {
            if (epilog_string)
                free(epilog_string);
            epilog_string = strdup(entry);
        }

        if (i == -1)
        {
            fprintf(stderr,"Syntax error in pattern line.\n");
            fprintf(stderr,"--> %s\n",buf);
            exit(1);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Close our input file, because we're done with it.                   */
    /* -------------------------------------------------------------------- */
    fclose(i_file);

    /* -------------------------------------------------------------------- */
    /*  Next try to open our output file, now that we've successfully read  */
    /*  in all of the patterns from our input file.                         */
    /* -------------------------------------------------------------------- */
    o_file = fopen(argv[2], "w");
    if ( !o_file )
    {
        perror(argv[2]);
        fprintf(stderr,"Unable to write output file '%s'\n", argv[2]);
        exit(1);
    }


    /* -------------------------------------------------------------------- */
    /*  Write all of the entries in our table out to the output file.       */
    /*  Note, we rely on the user to provide all the syntactic requirements */
    /*  such as commas.  We do provide newlines, though.                    */
    /* -------------------------------------------------------------------- */
    if (prolog_string)
        fprintf(o_file,"%s\n", prolog_string);

    for (i = 0; i < (1<<bits) ; i++)
    {
        fprintf(o_file,"/*%*s*/\t %s\n", bits, render_bits(i,bits), tbl[i]);
    }

    if (epilog_string)
        fprintf(o_file,"%s\n", epilog_string);

    /* -------------------------------------------------------------------- */
    /*  Close our output file, because we're done with it too.              */
    /* -------------------------------------------------------------------- */
    fclose(o_file);

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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
