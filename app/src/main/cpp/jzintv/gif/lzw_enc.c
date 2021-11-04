/* ======================================================================== */
/*  LZW Encode                                                              */
/*                                                                          */
/*  This code compresses an input buffer using LZW compression, as defined  */
/*  by the GIF standard.  This includes dividing the compressed output      */
/*  into MAX_BLOCK_BYTES blocks.                                            */
/*                                                                          */
/*  My data structure is entirely uncreative.  I use an N-way tree to       */
/*  represent the current code table.  It's dirt simple to implement, but   */
/*  it's a memory pig.  Since the longest code is 12 bits, I use indices    */
/*  instead of pointers, and use a static table of codes.                   */
/* ======================================================================== */

#include "config.h"
#include "gif/lzw_enc.h"

#ifdef DEBUG
# define Dprintf(x) jzp_printf x
#else
# define Dprintf(x)
#endif

#define MAX_BLOCK_BYTES (255)


int lzw_encode(const uint8_t *i_buf, uint8_t *o_buf, int i_len, int max_o_len)
{
    static uint16_t *dict = NULL;
    static int      dict_size = 0;
    const uint8_t *i_end = i_buf + i_len;
    const uint8_t *i_ptr;
    uint8_t *o_end = o_buf + max_o_len - 1;
    uint8_t *o_ptr;
    uint8_t *last_len_byte;
    int i;
    int code_size;
    int max_sym = 0, dict_stride;
    uint32_t curr_word = 0;
    int curr_bits = 0;
    int code = 0, next_new_code, curr_size;
    int end_of_info, clear_code;
    int next_char = 0, next_code;

    /* -------------------------------------------------------------------- */
    /*  First, scan the buffer and determine the total dynamic range of     */
    /*  the input bytes.  We'll pick our starting code size based on that.  */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < i_len; i++)
        if (i_buf[i] > max_sym)
            max_sym = i_buf[i];
    dict_stride = max_sym + 1;
    Dprintf(("max_sym = %.2X\n", max_sym));

    /* -------------------------------------------------------------------- */
    /*  Compute and output the starting code-size.                          */
    /* -------------------------------------------------------------------- */
    for (code_size = 2; code_size < 8; code_size++)
        if ((1 << code_size) > max_sym)
            break;
    Dprintf(("code_size = %.2X\n", code_size));
    /* -------------------------------------------------------------------- */
    /*  Allocate the dictionary.  We store the tree in a 2-D array.  One    */
    /*  dimension is the code number, and the other is the codes it chains  */
    /*  to.  We size this to the maximum number of symbols in the input,    */
    /*  so that it's not too big.                                           */
    /* -------------------------------------------------------------------- */
    if (dict_size < dict_stride)
    {
        if (dict)
            free(dict);
        dict      = CALLOC(uint16_t, 4096 * dict_stride);
        dict_size = dict_stride;
    }

    /* -------------------------------------------------------------------- */
    /*  Output the code length, and prepare to compress.                    */
    /* -------------------------------------------------------------------- */
    o_ptr         = o_buf;
    *o_ptr++      = code_size;
    *o_ptr        = 0;
    last_len_byte = o_ptr++;  /* save room for first data block length byte */
    i_ptr         = i_buf;
    curr_size     = code_size + 1;
    curr_word     = 0;
    curr_bits     = 0;
    next_new_code = 0x1000;         /* trigger dictionary flush first go */
    clear_code    = (1 << code_size);
    end_of_info   = (1 << code_size) + 1;
    next_char     = *i_ptr++;

    /* -------------------------------------------------------------------- */
    /*  Compress!                                                           */
    /* -------------------------------------------------------------------- */
    while (i_ptr <= i_end && code != end_of_info)
    {
        Dprintf(("remaining: %10d\n", i_end - i_ptr));

        /* ---------------------------------------------------------------- */
        /*  If dictionary's full, send a clear code and flush dictionary.   */
        /*  Otherwise, patch the previous code+char into the dictionary.    */
        /* ---------------------------------------------------------------- */
        if (i_ptr != i_end && next_new_code == 0x1000)
        {
            Dprintf(("CLEAR %.3X %d\n", clear_code, curr_size));

            curr_word |= clear_code << curr_bits;
            curr_bits += curr_size;
            while (curr_bits > 8)
            {
                /* Handle packaging data into MBB-byte records */
                if (o_ptr - last_len_byte == MAX_BLOCK_BYTES)
                {
                    Dprintf(("last_len_byte=%.8X o_ptr=%.8X\n",
                              last_len_byte, o_ptr));

                    *last_len_byte = MAX_BLOCK_BYTES - 1;
                    last_len_byte  = o_ptr++;
                }
                if (o_ptr >= o_end)
                    goto overflow;

                *o_ptr++    = curr_word & 0xFF;
                curr_word >>= 8;
                curr_bits  -= 8;
            }

            curr_size = code_size + 1;
            next_new_code = (1 << code_size) + 2;
            memset(dict, 0, 4096*sizeof(uint16_t)*dict_stride);
        } else if (i_ptr != i_end)
        {
            Dprintf(("new code: %.3X = %.3X + %.2X\n", next_new_code,
                     code, next_char));

            dict[code*dict_stride + next_char] = next_new_code;
            if (next_new_code == (1 << curr_size))
                curr_size++;
            next_new_code++;
        }

        code = next_char;  /* Previous concat becomes new initial code */
        Dprintf(("next code: %.2X %c\n", code, code == end_of_info ? '*':' '));

        /* ---------------------------------------------------------------- */
        /*  Keep concatenating as long as we stay in the dictionary.        */
        /* ---------------------------------------------------------------- */
        if (i_ptr == i_end)
        {
            next_char = end_of_info;
            Dprintf(("--> next is EOI!\n"));
        } else
        {
            next_code = -1;
            while (next_code && i_ptr < i_end)
            {
                next_char = *i_ptr++;
                next_code = dict[code*dict_stride + next_char];
                Dprintf(("--> code: %.3X + %.2X = %.3X\n", code,
                         next_char, next_code));

                if (next_code)
                    code = next_code;
            }
            if (next_code && i_ptr == i_end)
                next_char = end_of_info;

            if (next_char == end_of_info)
            {
                Dprintf(("--> next is EOI! (b)\n"));
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Ok, no longer in the dictionary.  Emit the current code and the */
        /*  extra character.  We can stuff two codes in, since curr_bits    */
        /*  should never be more than 7, and curr_size should be no more    */
        /*  than 12.                                                        */
        /* ---------------------------------------------------------------- */
        curr_word |= code << curr_bits;
        curr_bits += curr_size;
        Dprintf(("SEND %.4X %d curr: %.8X %2d\n", code, curr_size,
                 curr_word, curr_bits));
        while (curr_bits >= 8)
        {
            /* Handle packaging data into 256-byte records */
            if (o_ptr - last_len_byte == MAX_BLOCK_BYTES)
            {
                Dprintf(("last_len_byte=%.8X o_ptr=%.8X\n", last_len_byte,
                          o_ptr));
                *last_len_byte = MAX_BLOCK_BYTES - 1;
                last_len_byte  = o_ptr++;
            }
            if (o_ptr >= o_end)
                goto overflow;

            *o_ptr++    = curr_word & 0xFF;
            curr_word >>= 8;
            curr_bits  -= 8;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  If we have any left-over bits (at most 7), go ahead and flush them. */
    /* -------------------------------------------------------------------- */
    while (curr_bits > 0)  /* flush it ALL out. */
    {
        /* Handle packaging data into 256-byte records */
        if (o_ptr - last_len_byte == MAX_BLOCK_BYTES)
        {
            Dprintf(("last_len_byte=%.8X o_ptr=%.8X\n", last_len_byte, o_ptr));
            *last_len_byte = MAX_BLOCK_BYTES - 1;
            last_len_byte  = o_ptr++;
        }
        if (o_ptr >= o_end)
            goto overflow;

        *o_ptr++    = curr_word & 0xFF;
        curr_word >>= 8;
        curr_bits  -= 8;
    }

    /* -------------------------------------------------------------------- */
    /*  Patch in the last length byte, and a 0-length record.  We are       */
    /*  guaranteed to have room here, since our overflow criterion above    */
    /*  is conservative by one character.                                   */
    /* -------------------------------------------------------------------- */
    *last_len_byte = o_ptr - last_len_byte - 1;
    if ( *last_len_byte )
        *o_ptr++ = 0;

    Dprintf(("encoded %d bytes\n", o_ptr - o_buf));

    return o_ptr - o_buf;

overflow:
    return -1;
}

/*#define DEBUG*/
#undef Dprintf
#ifdef DEBUG
# define Dprintf(x) jzp_printf x
#else
# define Dprintf(x)
#endif

int lzw_encode2(const uint8_t *i_buf, const uint8_t *i_buf_alt,
                uint8_t *o_buf, int i_len, int max_o_len)
{
    static uint16_t *dict = NULL;
    static int      dict_size = 0;
    int i_idx = 0;
    uint8_t *o_end = o_buf + max_o_len - 1;
    uint8_t *o_ptr;
    uint8_t *last_len_byte;
    int i;
    int code_size;
    int max_sym = 0, dict_stride;
    uint32_t curr_word = 0;
    int curr_bits = 0;
    int code = 0, next_new_code, curr_size;
    int end_of_info, clear_code;
    int next_char = 0, next_code;

    /* -------------------------------------------------------------------- */
    /*  First, scan the buffer and determine the total dynamic range of     */
    /*  the input bytes.  We'll pick our starting code size based on that.  */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < i_len; i++)
    {
        if (i_buf[i] > max_sym)
            max_sym = i_buf[i];
        if (i_buf_alt[i] > max_sym)
            max_sym = i_buf_alt[i];
    }

    dict_stride = max_sym + 1;
    Dprintf(("max_sym = %.2X\n", max_sym));

    /* -------------------------------------------------------------------- */
    /*  Compute and output the starting code-size.                          */
    /* -------------------------------------------------------------------- */
    for (code_size = 2; code_size < 8; code_size++)
        if ((1 << code_size) > max_sym)
            break;
    Dprintf(("code_size = %.2X\n", code_size));
    /* -------------------------------------------------------------------- */
    /*  Allocate the dictionary.  We store the tree in a 2-D array.  One    */
    /*  dimension is the code number, and the other is the codes it chains  */
    /*  to.  We size this to the maximum number of symbols in the input,    */
    /*  so that it's not too big.                                           */
    /* -------------------------------------------------------------------- */
    if (dict_size < dict_stride)
    {
        if (dict)
            free(dict);
        dict      = CALLOC(uint16_t, 4096 * dict_stride);
        dict_size = dict_stride;
    }

    /* -------------------------------------------------------------------- */
    /*  Output the code length, and prepare to compress.                    */
    /* -------------------------------------------------------------------- */
    o_ptr         = o_buf;
    *o_ptr++      = code_size;
    last_len_byte = o_ptr++;  /* save room for first data block length byte */
    curr_size     = code_size + 1;
    curr_word     = 0;
    curr_bits     = 0;
    next_new_code = 0x1000;         /* trigger dictionary flush first go */
    clear_code    = (1 << code_size);
    end_of_info   = (1 << code_size) + 1;
    next_char     = i_buf[i_idx++];

    /* -------------------------------------------------------------------- */
    /*  Compress!                                                           */
    /* -------------------------------------------------------------------- */
    while (i_idx <= i_len && code != end_of_info)
    {
        Dprintf(("remaining: %10d\n", i_len - i_idx));

        /* ---------------------------------------------------------------- */
        /*  If dictionary's full, send a clear code and flush dictionary.   */
        /*  Otherwise, patch the previous code+char into the dictionary.    */
        /* ---------------------------------------------------------------- */
        if ( i_idx != i_len && next_new_code == 0x1000 )
        {
            Dprintf(("CLEAR %.3X %d\n", clear_code, curr_size));

            curr_word |= clear_code << curr_bits;
            curr_bits += curr_size;
            while (curr_bits > 8)
            {
                /* Handle packaging data into MBB-byte records */
                if (o_ptr - last_len_byte == MAX_BLOCK_BYTES)
                {
                    Dprintf(("last_len_byte=%.8X o_ptr=%.8X\n",
                              last_len_byte, o_ptr));

                    *last_len_byte = MAX_BLOCK_BYTES - 1;
                    last_len_byte  = o_ptr++;
                }
                if (o_ptr >= o_end)
                    goto overflow;

                *o_ptr++    = curr_word & 0xFF;
                curr_word >>= 8;
                curr_bits  -= 8;
            }

            curr_size = code_size + 1;
            next_new_code = (1 << code_size) + 2;
            memset(dict, 0, 4096*sizeof(uint16_t)*dict_stride);
        } else if ( i_idx != i_len )
        {
            Dprintf(("new code: %.3X = %.3X + %.2X\n", next_new_code,
                     code, next_char));

            dict[code*dict_stride + next_char] = next_new_code;
            if (next_new_code == (1 << curr_size))
                curr_size++;
            next_new_code++;
        }

        code = next_char;  /* Previous concat becomes new initial code */
        Dprintf(("next code: %.2X %c\n", code, code == end_of_info ? '*':' '));

        /* ---------------------------------------------------------------- */
        /*  Keep concatenating as long as we stay in the dictionary.        */
        /* ---------------------------------------------------------------- */
        if (i_idx == i_len)
        {
            next_char = end_of_info;
            Dprintf(("--> next is EOI!\n"));
        } else
        {
            next_code = -1;
            while (next_code && i_idx < i_len)
            {
                int tmp;

                next_char = i_buf[i_idx];
                if ((tmp = dict[code*dict_stride + i_buf[i_idx]]) != 0)
                {
                    next_code = tmp;
                    Dprintf(("--> code: %.3X + %.2X(a) = %.3X\n", code,
                             next_char, next_code));
                } else
                if ((tmp = dict[code*dict_stride + i_buf_alt[i_idx]]) != 0)
                {
                    next_char = i_buf_alt[i_idx];
                    next_code = tmp;
                    Dprintf(("--> code: %.3X + %.2X(b) = %.3X\n", code,
                             next_char, next_code));
                } else
                {
                    next_code = 0;
                    Dprintf(("--> code: %.3X + %.2X(c) = %.3X\n", code,
                             next_char, next_code));
                }
                i_idx++;

                if (next_code)
                    code = next_code;
            }
            if (next_code && i_idx == i_len)
                next_char = end_of_info;

            if (next_char == end_of_info)
            {
                Dprintf(("--> next is EOI! (b)\n"));
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Ok, no longer in the dictionary.  Emit the current code and the */
        /*  extra character.  We can stuff two codes in, since curr_bits    */
        /*  should never be more than 7, and curr_size should be no more    */
        /*  than 12.                                                        */
        /* ---------------------------------------------------------------- */
        curr_word |= code << curr_bits;
        curr_bits += curr_size;
        Dprintf(("SEND %.4X %d curr: %.8X %2d\n", code, curr_size,
                 curr_word, curr_bits));
        while (curr_bits > 8)
        {
            /* Handle packaging data into MBB-byte records */
            if (o_ptr - last_len_byte == MAX_BLOCK_BYTES)
            {
                Dprintf(("last_len_byte=%.8X o_ptr=%.8X\n", last_len_byte,
                          o_ptr));
                *last_len_byte = MAX_BLOCK_BYTES - 1;
                last_len_byte  = o_ptr++;
            }
            if (o_ptr >= o_end)
                goto overflow;

            *o_ptr++    = curr_word & 0xFF;
            curr_word >>= 8;
            curr_bits  -= 8;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  If we have any left-over bits (at most 7), go ahead and flush them. */
    /* -------------------------------------------------------------------- */
    while (curr_bits > 0)  /* flush it ALL out. */
    {
        /* Handle packaging data into MBB-byte records */
        if (o_ptr - last_len_byte == MAX_BLOCK_BYTES)
        {
            Dprintf(("last_len_byte=%.8X o_ptr=%.8X\n", last_len_byte, o_ptr));
            *last_len_byte = MAX_BLOCK_BYTES - 1;
            last_len_byte  = o_ptr++;
        }
        if (o_ptr >= o_end)
            goto overflow;

        *o_ptr++    = curr_word & 0xFF;
        curr_word >>= 8;
        curr_bits  -= 8;
    }

    /* -------------------------------------------------------------------- */
    /*  Patch in the last length byte, and a 0-length record.  We are       */
    /*  guaranteed to have room here, since our overflow criterion above    */
    /*  is conservative by one character.                                   */
    /* -------------------------------------------------------------------- */
    *last_len_byte = o_ptr - last_len_byte - 1;
    *o_ptr++ = 0;

    Dprintf(("encoded %d bytes\n", o_ptr - o_buf));

    return o_ptr - o_buf;

overflow:
    return -1;
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
/*                   Copyright (c) 2005, Joseph Zbiciak                     */
/* ======================================================================== */
