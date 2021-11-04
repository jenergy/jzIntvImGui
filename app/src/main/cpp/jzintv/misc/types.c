#include "config.h"
#include "misc/types.h"
#include "misc/printer.h"

/* ------------------------------------------------------------------------ */
/*  VAL_TRY_PARSE_DECNUM                                                    */
/*                                                                          */
/*  See if this variable has a numeric value that could be interpreted      */
/*  as decimal.  Only succeed if all characters are 0-9.                    */
/* ------------------------------------------------------------------------ */
void val_try_parse_decnum( val_strnum_t *const val )
{
    int num_dig;
    if ( !val || !val->str_val )
        return;
    
    num_dig = strspn(val->str_val, "0123456789");

    /* Are all the characters decimal digits? */
    if (num_dig > 0 && val->str_val[num_dig] == 0)
    {
        val->dec_val = atoi(val->str_val);
        val->flag   |= VAL_DECNUM;
    }
}

/* ------------------------------------------------------------------------ */
/*  VAL_TRY_PARSE_HEXNUM                                                    */
/*                                                                          */
/*  See if this variable has a numeric value that could be interpreted      */
/*  as decimal.  Only succeed if all characters are 0-9, A-F, a-f, with     */
/*  the minor exception that '$' is allowed as a prefix.                    */
/* ------------------------------------------------------------------------ */
void val_try_parse_hexnum( val_strnum_t *const val )
{
    char *first;
    int num_dig;

    if ( !val || !val->str_val )
        return;

    first = val->str_val;
    first += (*first == '$');

    num_dig = strspn(first, "0123456789abcdefABCDEF");

    /* Are all the characters hexadecimal digits? */
    if (num_dig > 0 && first[num_dig] == 0)
    {
        val->hex_val = strtoul(first, NULL, 16);
        val->flag   |= VAL_HEXNUM;
    }
}

/* ------------------------------------------------------------------------ */
/*  VAL_TRY_PARSE_DATE                                                      */
/*                                                                          */
/*  See if this variable has a date-like string in one of the following     */
/*  formats, or a subset.  Fields can be missing from the right.            */
/*                                                                          */
/*      YYYY/MM/DD HH:MM:SS +hh:mm                                          */
/*      YYYY-MM-DD HH:MM:SS +hhmm                                           */
/*      YYYY/MM/DD HH:MM:SS +hh:mm                                          */
/*      YYYY-MM-DD HH:MM:SS +hhmm                                           */
/*                                                                          */
/*  If so, populate the date_val structure and set the VAL_DATE flag.       */
/*  The VAL_DATE is not used for printing most places, but rather just      */
/*  provided as a convenience to date-consuming code.                       */
/* ------------------------------------------------------------------------ */
void val_try_parse_date( val_strnum_t *const val )
{
    int cvt1, y1, m1, d1, hh1, mm1, ss1, hhh1, mmm1;
    int cvt2, y2, m2, d2, hh2, mm2, ss2, hhh2, mmm2;
    char p1, p2;
    int y, m, d, hh, mm, ss, p, hhh, mmm;
    int utc_delta = UTC_DELTA_UNKNOWN;

    if ( !val || !val->str_val )
        return;

    y1 = m1 = d1 = p1 = 0;
    hh1 = mm1 = ss1 = hhh1 = mmm1 = -1;
    cvt1 = sscanf( val->str_val, "%d/%d/%d %d:%d:%d %c%d:%d", &y1, &m1, &d1,
                   &hh1, &mm1, &ss1, &p1, &hhh1, &mmm1);

    y2 = m2 = d2 = p2 = 0;
    hh2 = mm2 = ss2 = hhh2 = mmm2 = -1;
    cvt2 = sscanf( val->str_val, "%d-%d-%d %d:%d:%d %c%d:%d", &y2, &m2, &d2,
                   &hh2, &mm2, &ss2, &p2, &hhh2, &mmm2);

    if ( cvt1 == 0 && cvt2 == 0 )
        return;

    y   = cvt1 > cvt2 ? y1   : y2;
    m   = cvt1 > cvt2 ? m1   : m2;
    d   = cvt1 > cvt2 ? d1   : d2;
    hh  = cvt1 > cvt2 ? hh1  : hh2;
    mm  = cvt1 > cvt2 ? mm1  : mm2;
    ss  = cvt1 > cvt2 ? ss1  : ss2;
    p   = cvt1 > cvt2 ? p1   : p2;    // '+' or '-'
    hhh = cvt1 > cvt2 ? hhh1 : hhh2;
    mmm = cvt1 > cvt2 ? mmm1 : mmm2;

    if ( y > 0 && y < 100 )                 { y += 1900; }
    if ( y < 1901 || y > 1900 + 255 )       { y =     0; }
    if ( !y      || m < 0 || m > 12 )       { m =     0; }
    if ( !m      || d < 0 || d > 31 )       { d =     0; }
    if ( !d      || hh < 0 || hh > 23 )     { hh =   -1; }
    if ( hh < 0  || mm < 0 || mm > 59 )     { mm =   -1; }
    if ( mm < 0  || ss < 0 || ss > 60 )     { ss =   -1; }
    if ( ss < 0  || (p != '-' && p != '+')) { p =     0; }

    if (p && hhh >= 0 && mmm < 0)
    {
        mmm = hhh % 100;
        hhh /= 100;
    } 
    else if (!p || hhh < 0 || hhh > 12 || mmm < 0 || mmm > 59)
    { 
        p   = 0;
        hhh = -1;
        mmm = -1;
    }

    if ( p )
    {
        utc_delta  = 60 * (p == '-' ? -hhh : hhh);
        utc_delta += (p == '-' ? -mmm : mmm);
        if (utc_delta < -720 || utc_delta > 720)
            utc_delta = UTC_DELTA_UNKNOWN;
    }

    if ( y )
    {
        val->flag |= VAL_DATE;
        val->date_val.year      = y;
        val->date_val.month     = m;
        val->date_val.day       = d;
        val->date_val.hour      = hh;
        val->date_val.min       = mm;
        val->date_val.sec       = ss;
        val->date_val.utc_delta = utc_delta;
    }
}

/* ------------------------------------------------------------------------ */
/*  GAME_DATE_TO_STRING                                                     */
/*  Convert a game_date_t to a string.  The string is malloc'd.  Returns    */
/*  NULL on failure.                                                        */
/* ------------------------------------------------------------------------ */
char *game_date_to_string( const game_date_t *const date )
{
    char buf[80];
    const int y = date->year;
    const int m = date->month;
    const int d = date->day;
    const int hh = date->hour;
    const int mm = date->min;
    const int ss = date->sec;

    if ( !date || !date->year )
        return NULL;

    if (y && m && d && hh >= 0 && mm >= 0 && ss >= 0 &&
        date->utc_delta >= -720 && date->utc_delta <= 720 )
    {
        const int uc = date->utc_delta < 0 ? '-' : '+';
        const int ad = abs(date->utc_delta);
        const int uh = ad / 60;
        const int um = ad % 60;

        sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d %c%02d%02d",
                y, m, d, hh, mm, ss, uc, uh, um);
    } 
    else if (y && m && d && hh >= 0 && mm >= 0 && ss >= 0)
        sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
    else if (y && m && d && hh >= 0 && mm >= 0)
        sprintf(buf, "%04d-%02d-%02d %02d:%02d", y, m, d, hh, mm);
    else if (y && m && d && hh >= 0)
        sprintf(buf, "%04d-%02d-%02d %02d", y, m, d, hh);
    else if (y && m && d)
        sprintf(buf, "%04d-%02d-%02d", y, m, d);
    else if (y && m)
        sprintf(buf, "%04d-%02d", y, m);
    else 
        sprintf(buf, "%04d", y);

    return strdup( buf );
}

/* ------------------------------------------------------------------------ */
/*  GAME_DATE_TO_UINT8_T -- Convert date to serialized .ROM/.LUIGI format.  */
/*  Note: buf[] must have room for up to 8 bytes.                           */
/* ------------------------------------------------------------------------ */
int game_date_to_uint8_t(const game_date_t *const d, uint8_t *const data)
{
    int length = 0;

    if (d->year)        data[length++] = d->year - 1900;    else return length;
    if (d->month)       data[length++] = d->month;          else return length;
    if (d->day)         data[length++] = d->day;            else return length;
    if (d->hour >= 0)   data[length++] = d->hour;           else return length;
    if (d->min >= 0)    data[length++] = d->min;            else return length;
    if (d->sec >= 0)    data[length++] = d->sec;            else return length;
    if (d->utc_delta != UTC_DELTA_UNKNOWN)
    {
        const int ud = d->utc_delta;
        int uh, um;

        /* For -ve numbers, round toward -oo so -01:30 becomes      */
        /* -2, +30; that is, the minutes are always a +ve offset    */

        if (ud < 0) uh = -(((59 - ud)) / 60);   /* Round to -oo */
        else        uh = ud / 60;               /* Round to -oo */

        um = ud - uh * 60;                      /* Positive offset */

        if (uh >= -12 && uh <= 12)
        {
            data[length++] = uh;
            if (um)
                data[length++] = um;
        }
    }

    return length;
}

/* ------------------------------------------------------------------------ */
/*  UINT8_T_TO_GAME_DATE -- Deserialize a serialized game date.             */
/*  Returns 0 on success, non-zero on failure.                              */
/* ------------------------------------------------------------------------ */
int uint8_t_to_game_date(game_date_t *const d, const uint8_t *const data,
                         const int length)
{
    if (length < 1 || length > 8)
        return -1;

    int utc_delta = UTC_DELTA_UNKNOWN;
    int ud_byte = 0;

    if (length < 1 || length > 8)
        return -1;

    /* Not a full validation of dates, but a reasonable sanity check. */
    if (length > 1 && data[1] > 12)
        return -1;

    if (length > 2 && data[2] > 31)
        return -1;

    if (length > 3 && data[3] > 23)
        return -1;

    if (length > 4 && data[4] > 59)
        return -1;

    if (length > 5 && data[5] > 60)
        return -1;

    if (length > 6)
    {
        ud_byte = (data[6] ^ 0x80) - 0x80;
        if (ud_byte < -12 || ud_byte > 12)
            return -1;
    }

    if (length > 7 && data[7] > 59)
        return -1;

    d->year  = data[0] + 1900;
    d->month = length > 1 ? data[1] : 0;
    d->day   = length > 2 ? data[2] : 0;
    d->hour  = length > 3 ? data[3] : -1;
    d->min   = length > 4 ? data[4] : -1;
    d->sec   = length > 5 ? data[5] : -1;

    if (length > 6)
    {
        utc_delta = 60 * ud_byte;
        if (length > 7)
            utc_delta += data[7];
    }

    d->utc_delta = utc_delta;

    return 0;
}

/* ------------------------------------------------------------------------ */
/*  VAL_ADD_DATE_STRING                                                     */
/*  If a val has a date but no string, generate the string if possible.     */
/* ------------------------------------------------------------------------ */
void val_add_date_string( val_strnum_t *const val )
{
    if ( !val || !VAL_HAS_DATE( *val ) || VAL_HAS_STRING( *val ) )
        return;

    char *date_string = game_date_to_string( &(val->date_val) );

    if ( date_string )
    {
        val->flag  |= VAL_STRING;
        val->str_val = date_string;
    }
}

/* ------------------------------------------------------------------------ */
/*  FREE_CFG_VAR         -- Free a single CFG_VAR.                          */
/*  FREE_CFG_VAR_LIST    -- Free a list of CFG_VAR_T.                       */
/* ------------------------------------------------------------------------ */
void free_cfg_var( cfg_var_t *var )
{
    CONDFREE(var->name);
    CONDFREE(var->val.str_val);
    free(var);
}

void free_cfg_var_list( cfg_var_t *head )
{
    cfg_var_t *prev = head;

    while ( head )
    {
        prev = head;
        head = (cfg_var_t *)head->l.next;
        free_cfg_var( prev );
    }
}

/* ------------------------------------------------------------------------ */
/*  NEXT_CODEPT_LENGTH   -- Returns the length in bytes of the next         */
/*                          code-point, assuming UTF-8 encoding.            */
/*                                                                          */
/*  Byte sequences that resemble UTF-8 but aren't valid UTF-8 return a      */
/*  negative length, with the negative value representing the /apparent/    */
/*  length of the code point if it were to be valid.                        */
/*                                                                          */
/*  The caller can decide how to handle the apparently-b0rken code point.   */
/* ------------------------------------------------------------------------ */
LOCAL int next_codept_length(const unsigned char *const u)
{
    /* -------------------------------------------------------------------- */
    /*  NULL pointer or empty string?  Return 0.                            */
    /* -------------------------------------------------------------------- */
    if (!u || !u[0])
        return 0;

    /* -------------------------------------------------------------------- */
    /*  Extract up to 4 bytes.  Also, precompute some predicates.           */
    /* -------------------------------------------------------------------- */
    const unsigned char b0 = u[0];
    const unsigned char b1 = b0 ? u[1] : 0;
    const unsigned char b2 = b1 ? u[2] : 0;
    const unsigned char b3 = b2 ? u[3] : 0;

    const int vc1 = b1 >= 0x80 && b1 <= 0xBF;   /* b1 is valid cont. byte   */
    const int vc2 = b2 >= 0x80 && b2 <= 0xBF;   /* b2 is valid cont. byte   */
    const int vc3 = b3 >= 0x80 && b3 <= 0xBF;   /* b3 is valid cont. byte   */

    /* -------------------------------------------------------------------- */
    /*  Table 3-7.  Well-Formed UTF-8 Byte Sequences, Unicode Std. 11.0.    */
    /*                                                                      */
    /*      Code Points           1st Byte  2nd Byte  3rd Byte  4th Byte    */
    /*  1.  U+0000..U+007F         00..7F                                   */
    /*  2.  U+0080..U+07FF         C2..DF    80..BF                         */
    /*  3.  U+0800..U+0FFF         E0       *A0..BF    80..BF               */
    /*  4.  U+1000..U+CFFF         E1..EC    80..BF    80..BF               */
    /*  5.  U+D000..U+D7FF         ED        80..9F*   80..BF               */
    /*  6.  U+E000..U+FFFF         EE..EF    80..BF    80..BF               */
    /*  7.  U+10000..U+3FFFF       F0       *90..BF    80..BF    80..BF     */
    /*  8.  U+40000..U+FFFFF       F1..F3    80..BF    80..BF    80..BF     */
    /*  9.  U+100000..U+10FFFF     F4        80..8F*   80..BF    80..BF     */
    /*                                                                      */
    /*  * Denotes a partial trailing byte range.                            */
    /* -------------------------------------------------------------------- */
    if (b0 > 0 && b0 <= 0x7F)                                          /* 1 */
        return 1;                                                     
                                                                      
    if (b0 >= 0xC2 && b0 <= 0xDF && vc1)                               /* 2 */
        return 2;                                                     
                                                                      
    if ((b0 == 0xE0 && b1 >= 0xA0 && b1 <= 0xBF && vc2) ||             /* 3 */
        (b0 >= 0xE1 && b0 <= 0xEC &&        vc1 && vc2) ||             /* 4 */
        (b0 == 0xED && b1 >= 0x80 && b1 <= 0x9F && vc2) ||             /* 5 */
        (b0 >= 0xEE && b0 <= 0xEF &&        vc1 && vc2))               /* 6 */
        return 3;                                                     
                                                                      
    if ((b0 == 0xF0 && b1 >= 0x90 && b1 <= 0xBF && vc2 && vc3) ||      /* 7 */
        (b0 >= 0xF1 && b0 <= 0xF3 &&        vc1 && vc2 && vc3) ||      /* 8 */
        (b0 == 0xF4 && b1 >= 0x80 && b1 <= 0x8F && vc2 && vc3))        /* 9 */
        return 4;

    /* -------------------------------------------------------------------- */
    /*  If we made it to here, then it's malformed UTF-8.                   */
    /*  Return a -ve value that represents its /apparent/ length, negated.  */
    /*  Shorten the -ve length estimate if we see a terminating NUL sooner. */
    /* -------------------------------------------------------------------- */
    if ((b0 >= 0x80 && b0 <= 0xBF) || !b1) return -1;
    if ((b0 >= 0xC0 && b0 <= 0xDF) || !b2) return -2;
    if ((b0 >= 0xE0 && b0 <= 0xEF) || !b3) return -3;
    if (b0 >= 0xF0                       ) return -4;  /* Don't go above 4. */
    return -1;                                  /* Should not be reachable. */
}

/* ------------------------------------------------------------------------ */
/*  String quoting/escaping rules:                                          */
/*                                                                          */
/*  1.  If a string contains any of these characters: ; [ ] $ = - ,         */
/*      or a space character, it must be quoted.                            */
/*                                                                          */
/*  2.  If a string contains a lone double quote, it must be quoted.        */
/*      The double quote must be escaped with a backslash.                  */
/*                                                                          */
/*  3.  If a string contains characters with the  values 0x09, 0x0A, or     */
/*      0x0D, the string must be quoted and the character must be escaped.  */
/*      These three  characters map strictly as follows:                    */
/*                                                                          */
/*          0x09 => \t, 0x0A => \n, 0x0D => \r.                             */
/*                                                                          */
/*  4.  If a string contains a valid UTF-8 encoded character, it is         */
/*      /not/ quoted, and is passed through unmodified.                     */
/*                                                                          */
/*  5.  If a string contains any other character with a value below         */
/*      0x20 or a value above 0x7E, the string must be quoted, and the      */
/*      character must be escaped.  The character will be escaped with      */
/*      a hexadecimal escape.  0x00 => \x00.  0x7E => \x7E.                 */
/*                                                                          */
/*  6.  If the string gets quoted, any backslashes must be escaped with     */
/*      a backslash.  e.g.  foo-bar\baz => "foo-bar\\baz".                  */
/*                                                                          */
/*  String unquoting/unescaping rules:                                      */
/*                                                                          */
/*  1.  If the string does not begin *and* end with double-quotes, it is    */
/*      returned as-is.                                                     */
/*                                                                          */
/*  2.  A backslash followed by 'x' and two hexadecimal digits gets         */
/*      replaced by a character whose value is represented by the digits.   */
/*                                                                          */
/*  3.  A backslash followed by three octal digits gets replaced by a       */
/*      character whose value is represented by the digits, masked to 8     */
/*      bits. (e.g. \777 and \377 both map to 0xFF.)                        */
/*                                                                          */
/*  4.  A backslash followed by 't', 'n', or 'r' gets replaced by 0x09,     */
/*      0x0A, ar 0x0D, respectively.                                        */
/*                                                                          */
/*  5.  A backslash followed by any other character gets replaced with      */
/*      the character that follows the backslash.                           */
/* ------------------------------------------------------------------------ */

static const char *must_quote =
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
    " ;[]$=-,\\\"'\x7F"
    "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
    "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
    "\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
    "\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF"
    "\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF"
    "\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF"
    "\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF"
    "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF";

/* ------------------------------------------------------------------------ */
/*  Common implementation for escaping, and quoting-with-escapes.           */
/* ------------------------------------------------------------------------ */
static const char *cfg_escquote_impl( const char *const str, 
                                      const int add_quotes )
{
    static unsigned char *buf = NULL;
    static unsigned buf_sz = 0;
    const unsigned char *si;
    unsigned char *so;
    size_t req_sz = 3;

    size_t idx = strcspn( str, must_quote );
    if (!str[idx])
        return str;

    for (si = (const unsigned char *)str; *si; si++)
    {
        const unsigned char c = *si;
        int l;

        if (c == '"' || c == '\\' || c == 0x09 || c == 0x0A || c == 0x0D)
            req_sz += 2;
        else if ((l = next_codept_length(si)) > 1) 
        {
            req_sz += l;
            si += l - 1;
        } 
        else if (c < 0x20 || c > 0x7E || l < 0)
            req_sz += 4;
        else
            req_sz += 1;
    }

    if (buf_sz < req_sz)
    {
        buf_sz = req_sz * 2;
        buf = (unsigned char *)realloc( (void *)buf, buf_sz );
        if (!buf)
        {
            buf_sz = 0;
            buf = NULL;
            return str;
        }
    }

    buf[0] = '"';
    for (si = (const unsigned char *)str, so = buf + !!add_quotes; *si; si++)
    {
        const unsigned char c = *si;
        int l;

        if (c == '"' || c == '\\' || c == 0x09 || c == 0x0A || c == 0x0D) 
        {
            *so++ = '\\';
            *so++ = c == 0x09 ? 't'
                  : c == 0x0A ? 'n'
                  : c == 0x0D ? 'r'
                  :             c;
        } else if ((l = next_codept_length(si)) > 1) 
        {
            while (l-- > 0)
                *so++ = *si++;
            si--;
        } else if (c < 0x20 || c > 0x7E || l < 0)
        {
            *so++ = '\\';
            *so++ = 'x';
            *so++ = "0123456789ABCDEF"[0xF & (c >> 4)];
            *so++ = "0123456789ABCDEF"[0xF & (c >> 0)];
        } else
        {
            *so++ = c;
        }
    }
    if (add_quotes)
        *so++ = '"';
    *so++ = 0;

    return (char *)buf;
}

/* ------------------------------------------------------------------------ */
/*  Common implementation for unescaping, and unquoting-with-unescaping.    */
/* ------------------------------------------------------------------------ */
static const char *cfg_unescquote_impl( const char *const str, 
                                        const int remove_quotes )
{
    static unsigned char *buf = NULL;
    static unsigned buf_sz = 0;
    const unsigned char *si;
    unsigned char *so;
    const size_t len = strlen(str);
    const size_t req_sz = len + 1;

    /* -------------------------------------------------------------------- */
    /*  Ensure our output buffer is big enough.                             */
    /* -------------------------------------------------------------------- */
    if (buf_sz < req_sz)
    {
        buf_sz = req_sz * 2;
        buf = (unsigned char *)realloc( (void *)buf, buf_sz );
        if (!buf)
        {
            buf_sz = 0;
            buf = NULL;
            return str;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Unescape all escapes.                                               */
    /* -------------------------------------------------------------------- */
    for (si = (const unsigned char *)str + !!remove_quotes, so = buf; *si; )
    {
        const unsigned char c0 = si[0];

        if (c0 != '\\')
        {
            *so++ = c0;
            si += 1;
        } else
        {
            const unsigned char c1 = si[1];
            const unsigned char c2 = c1 ? si[2] : 0;
            const unsigned char c3 = c2 ? si[3] : 0;
            switch (c1)
            {
                case 't': *so++ = 0x09; si += 2; break;
                case 'n': *so++ = 0x0A; si += 2; break;
                case 'r': *so++ = 0x0D; si += 2; break;
                case 'x':
                {
                    const char *const hexdig = "0123456789ABCDEF";
                    const char *const dig0 = strchr(hexdig, toupper(c2));
                    const char *const dig1 = strchr(hexdig, toupper(c3));

                    if (dig0 && dig1)
                    {
                        *so++ = ((dig0 - hexdig) & 0x0F) << 4
                              | ((dig1 - hexdig) & 0x0F);
                        si += 4;
                    } else
                    {
                        *so++ = c1;
                        si += 2;
                    }
                    break;
                }
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                {
                    const char *const octdig = "01234567";
                    const char *const dig0 = strchr(octdig, toupper(c1));
                    const char *const dig1 = strchr(octdig, toupper(c2));
                    const char *const dig2 = strchr(octdig, toupper(c3));

                    if (dig0 && dig1 && dig2)
                    {
                        *so++ = ((dig0 - octdig) & 3) << 6
                              | ((dig1 - octdig) & 7) << 3
                              | ((dig2 - octdig) & 7);
                        si += 4;
                    } else
                    {
                        *so++ = c1;
                        si += 2;
                    }
                    break;
                }
                case '\\':
                {
                    *so++ = '\\';
                    si += 2;
                    break;
                }
                default:
                {
                    *so++ = c1;
                    si += 1;
                    break;
                }
            }
        }
    }

    if (remove_quotes)
        so[-1] = 0; /* Trim off closing quote. */
    else
        so[0] = 0;  /* Terminate string. */
                    
    return (char *)buf;
}

/* ------------------------------------------------------------------------ */
/*  CFG_ESCAPE_STR       -- Add escape character sequences as needed.       */
/*                          Used for strings in vars and other CFG stuff.   */
/*                          Non-reentrant.  Shares buffer w/cfg_quote_str.  */
/* ------------------------------------------------------------------------ */
const char *cfg_escape_str( const char *const str )
{
    return cfg_escquote_impl(str, 0);
}

/* ------------------------------------------------------------------------ */
/*  CFG_QUOTE_STR        -- Quote a string if necessary.  Non-reentrant.    */
/*                          Add escape character sequences as needed.       */
/*                          Used for strings in vars and other CFG stuff.   */
/* ------------------------------------------------------------------------ */
const char *cfg_quote_str( const char *const str )
{
    return cfg_escquote_impl(str, 1);
}

/* ------------------------------------------------------------------------ */
/*  CFG_UNESCAPE_STR     -- Convert escape character sequences as needed.   */
/*                          Used for strings in vars and other CFG stuff.   */
/*                          Non-reentrant. Shares buffer w/cfg_unquote_str. */
/* ------------------------------------------------------------------------ */
const char *cfg_unescape_str( const char *const str )
{
    return cfg_unescquote_impl(str, 1);
}

/* ------------------------------------------------------------------------ */
/*  CFG_UNQUOTE_STR      -- Remove quotes from a string.  Non-reentrant.    */
/*                          Convert escape character sequences as needed.   */
/*                          Used for strings in vars and other CFG stuff.   */
/* ------------------------------------------------------------------------ */
const char *cfg_unquote_str( const char *const str )
{
    const size_t len = strlen(str);

    if (len < 2 || str[0] != '"' || str[len - 1] != '"')
        return str;

    return cfg_unescquote_impl(str, 1);
}

/* ------------------------------------------------------------------------ */
/*  STRIP_BAD_UTF8       -- Replaces bad UTF-8 characters with '?'.         */
/*                          Destination may safely overlap source.          */
/*                          Output is never longer than the input.          */
/* ------------------------------------------------------------------------ */
void strip_bad_utf8( char *const dst, const char *const src )
{
    /* This reuses the UTF-8 code put together for config variables. */
    int i = 0, j = 0;

    while (src[i])
    {
        int l = next_codept_length((const uint8_t*)&src[i]);

        /* ---------------------------------------------------------------- */
        /*  Negative lengths are "bad" codepoints.  Skip them in the input  */
        /*  and output a single '?' in the output in their place.           */
        /* ---------------------------------------------------------------- */
        if (l < 0)
        {
            dst[j++] = '?';
            while (l++ < 0)
                if (!src[i++])
                    break;
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Otherwise, copy it over...                                      */
        /* ---------------------------------------------------------------- */
        while (l-- > 0)
            dst[j++] = src[i++];
    }

    dst[j] = 0;
}
            

/* ------------------------------------------------------------------------ */
/*  PRINT_CFG_VAR        -- Print <name> = <value> tuple.                   */
/*  PRINT_CFG_VAR_LIST   -- Print a list of cfg_vars                        */
/* ------------------------------------------------------------------------ */
void print_cfg_var
(
    cfg_var_t *RESTRICT const var,
    printer_t *RESTRICT const p
)
{
    if ( VAL_HAS_DECNUM( var->val ) )
    {
        p->fxn(p->opq, "%s = %d\015\012", var->name, var->val.dec_val);
    } else if ( VAL_HAS_HEXNUM( var->val ) )
    {
        p->fxn(p->opq, "%s = $%.4X\015\012", var->name, var->val.hex_val);
    } else
    {
        p->fxn(p->opq, "%s = %s\015\012", var->name,
               cfg_quote_str( var->val.str_val ) );
    }
}


void print_cfg_var_list
(
    cfg_var_t *RESTRICT const head,
    printer_t *RESTRICT const p
)
{
    cfg_var_t *curr = head;

    while ( curr )
    {
        print_cfg_var( curr, p );
        curr = (cfg_var_t *)curr->l.next;
    }
}

/* ------------------------------------------------------------------------ */
/*  CONS_CFG_VAR_DEC     -- Construct a decimal config variable.            */
/*  CONS_CFG_VAR_HEX     -- Construct a hexadecimal config variable.        */
/*  CONS_CFG_VAR_STRING  -- Construct a string config variable.             */
/* ------------------------------------------------------------------------ */
cfg_var_t *cons_cfg_var_dec
(
    const char *RESTRICT const name,
    const int32_t              value
)
{
    char buf[32];
    cfg_var_t *var = CALLOC(cfg_var_t, 1);
    char *name_str;
    char *val_str;

    if (!var)
        return NULL;

    name_str = strdup(name);
    if (!name_str)
    {
        free(var);
        return NULL;
    }
    var->name = name_str;

    sprintf(buf, "%d", value);
    val_str = strdup(buf);

    var->val.flag = VAL_DECNUM | (val_str ? VAL_STRING : 0);
    var->val.dec_val = value;
    var->val.str_val = val_str;

    return var;
}

cfg_var_t *cons_cfg_var_hex
(
    const char *RESTRICT const name,
    const uint32_t             value
)
{
    char buf[32];
    cfg_var_t *var = CALLOC(cfg_var_t, 1);
    char *name_str;
    char *val_str;

    if (!var)
        return NULL;

    name_str = strdup(name);
    if (!name_str)
    {
        free(var);
        return NULL;
    }
    var->name = name_str;

    sprintf(buf, "%x", value);
    val_str = strdup(buf);

    var->val.flag = VAL_HEXNUM | (val_str ? VAL_STRING : 0);
    var->val.hex_val = value;
    var->val.str_val = val_str;

    return var;
}

cfg_var_t *cons_cfg_var_string
(
    const char *RESTRICT const name,
    const char *RESTRICT const value
)
{
    cfg_var_t *var = CALLOC(cfg_var_t, 1);
    char *name_str = NULL;
    char *val_str  = NULL;

    if (!var)
        return NULL;

    name_str = strdup(name);
    if (!name_str)
    {
        free(var);
        return NULL;
    }
    var->name = name_str;

    val_str = strdup(value);
    if (!val_str)
    {
        free(var);
        free(name_str);
        return NULL;
    }

    var->val.flag = VAL_STRING;
    var->val.str_val = val_str;

    return var;
}

cfg_var_t *cons_cfg_var_date
(
    const char        *RESTRICT const name,
    const game_date_t *RESTRICT const value
)
{
    cfg_var_t *var = CALLOC(cfg_var_t, 1);
    char *name_str = NULL;

    if (!var)
        return NULL;

    name_str = strdup(name);
    if (!name_str)
    {
        free(var);
        return NULL;
    }

    var->name         = name_str;
    var->val.flag     = VAL_DATE;
    var->val.date_val = *value;

    val_add_date_string( &(var->val) );

    return var;
}

/* ------------------------------------------------------------------------ */
/*  CONS_CFG_VAR_KV_STR  -- Construct a config variable from a key=value    */
/*                          string (such as get packed in LUIGIs).          */
/*                                                                          */
/*  This function has to guess at the value type, whether it's a date,      */
/*  number, or generic string.  It's meant for reconstituting cfg_var_t's   */
/*  that were serialized under a "misc" category, as they had no other      */
/*  category at the time of serialization.                                  */
/* ------------------------------------------------------------------------ */
cfg_var_t *cons_cfg_var_kv_str
(
    const char *RESTRICT kv_str
)
{
    const char *div = NULL;
    char *name_str  = NULL;
    char *val_str   = NULL;
    cfg_var_t *var  = NULL;

    div = strchr(kv_str, '=');
    if (!div)
        goto fail;

    /* CALLOC ensures nul-termination here. */
    if (!(name_str = CALLOC(char, div - kv_str + 1)))
        goto fail;

    memcpy(name_str, kv_str, div - kv_str);     /* do not copy the '=' */

    if (!(val_str = strdup(div + 1)))
        goto fail;

    if (!(var = CALLOC(cfg_var_t, 1)))
        goto fail;

    var->name        = name_str;
    var->val.str_val = val_str;
    var->val.flag    = VAL_STRING;

    val_try_parse_decnum( &(var->val) );
    val_try_parse_hexnum( &(var->val) );
    val_try_parse_date  ( &(var->val) );

    return var;

fail:
    CONDFREE(var);
    CONDFREE(val_str);
    CONDFREE(name_str);

    return NULL;
}


/* ------------------------------------------------------------------------ */
/*  APPEND_CFG_VAR       -- Appends a new config var to a list.             */
/*                          Returns the appended var.                       */
/*                                                                          */
/*  Intended to be used as follows:                                         */
/*                                                                          */
/*      var = append_cfg_var( &head, cons_cfg_var_XXX( "name", val ) );     */
/*                                                                          */
/*  If the cons failes, 'var' will be NULL and the list remains unchanged.  */
/* ------------------------------------------------------------------------ */
cfg_var_t *append_cfg_var
(
    cfg_var_t *RESTRICT *const head,
    cfg_var_t           *RESTRICT const var
)
{
    cfg_var_t *RESTRICT *curr = head;

    if (!var)
        return NULL;

    while (*curr)
        curr = (cfg_var_t **)&((*curr)->l.next);
    
    *curr = var;
    return var;
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
/* ------------------------------------------------------------------------ */
/*                 Copyright (c) 2003-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
