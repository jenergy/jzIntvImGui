/* ======================================================================== */
/*  INTELLICART ROM MetaData Tag routines.              J. Zbiciak, 2017    */
/*                                                                          */
/*  These routines manipulate metadata tags that might be appended to an    */
/*  Intellicart ROM file.  Routines are provided for generating a linked    */
/*  list of decoded tags and for converting a list of tags into an          */
/*  encoded image.                                                          */
/* ======================================================================== */

#include "config.h"
#include "icart/icartrom.h"
#include "icart/icarttag.h"
#include "metadata/metadata.h"
#include "misc/crc16.h"

typedef struct cursor_t
{
    const uint8_t *data;
    int           length;
    int           offset;
    uint16_t      crc16;
} cursor_t;

typedef struct hunk_t
{
    uint8_t      *data;
    int           type;
    int           length;
    int           alloc;
} hunk_t;

/* ======================================================================== */
/*  ICT_NEXT_BYTE                                                           */
/* ======================================================================== */
LOCAL int ict_next_byte(cursor_t *const cursor)
{
    if (cursor->offset >= cursor->length)
        return -1;

    const uint8_t data = cursor->data[cursor->offset++];

    cursor->crc16 = crc16_update(cursor->crc16, data);

    return data;
}

/* ======================================================================== */
/*  ICT_START_CHECKSUM                                                      */
/* ======================================================================== */
LOCAL void ict_start_checksum(cursor_t *const cursor)
{
    cursor->crc16 = 0xFFFF;
}

/* ======================================================================== */
/*  ICT_CHECK_CHECKSUM  -- 0 means OK, non-zero means error.                */
/* ======================================================================== */
LOCAL int ict_check_checksum(cursor_t *const cursor)
{
    if (cursor->offset + 2 > cursor->length)
        return -1;

    const uint16_t expected_crc16 = cursor->crc16;
    const uint16_t stored_crc16   = (cursor->data[cursor->offset] << 8) 
                                  | (cursor->data[cursor->offset + 1]);

    cursor->offset += 2;

    return (stored_crc16 == expected_crc16) ? 0 : -1;
}

/* ======================================================================== */
/*  ICT_GET_LENGTH                                                          */
/* ======================================================================== */
LOCAL int ict_get_length(cursor_t *const cursor)
{
    const int len_byte_0 = ict_next_byte(cursor);

    if (len_byte_0 < 0)
        return -1;

    const int num_bytes  = (len_byte_0 >> 6) & 3;
    const int len_byte_1 = num_bytes > 0 ? ict_next_byte(cursor) : 0;
    const int len_byte_2 = num_bytes > 1 ? ict_next_byte(cursor) : 0;
    const int len_byte_3 = num_bytes > 2 ? ict_next_byte(cursor) : 0;

    if (len_byte_1 < 0 || len_byte_2 < 0 || len_byte_3 < 0)
        return -1;
    
    return (len_byte_0 & 0x3F) 
         | (len_byte_1 << 6) | (len_byte_2 << 14) | (len_byte_3 << 22);
}


/* ======================================================================== */
/*  ICT_PUBLISHER                                                           */
/* ======================================================================== */
LOCAL ict_publisher_t *ict_publisher_decode(const hunk_t *const hunk)
{
    /* Abort on the cases we can't handle because they look wrong. */
    if (hunk->length >  1 && hunk->data[0] != 0xFF) return NULL;
    if (hunk->length == 1 && hunk->data[0] == 0xFF) return NULL;

    /* OK, try to get a proper publisher record. */
    ict_publisher_t *const p = CALLOC(ict_publisher_t, 1);

    /* If length == 1, then it must be one of the canned publisher names. */
    if (hunk->length == 1)
    {
        if (hunk->data[0] < IC_PUBLISHER_COUNT)
            p->name = strdup(ic_publisher_names[hunk->data[0]]);
        else
        {
            char buf[64];
            snprintf(buf, 64, "Unknown Publisher %d", hunk->data[0]);
            p->name = strdup(buf);
        }
    } else
    {
        p->name = strdup((const char *)(hunk->data + 1));
    }

    return p;
}

LOCAL void ict_publisher_dtor(ict_publisher_t *const p)
{
    if (p)
        CONDFREE(p->name);

    CONDFREE_k(p);
}

LOCAL int ict_lookup_publisher(const char *const name)
{
    for (int i = 0; i < IC_PUBLISHER_COUNT; ++i)
    {
        if (!stricmp(ic_publisher_names[i], name))
            return i;
    }

    return -1;
}

/* ======================================================================== */
/*  ICT_CREDITS                                                             */
/* ======================================================================== */
LOCAL ict_credits_t *ict_credits_decode(const hunk_t *const hunk)
{
    /* -------------------------------------------------------------------- */
    /*  First, pass over the data and count the number of credit records    */
    /*  that are present in this hunk.                                      */
    /* -------------------------------------------------------------------- */
    int num_credits = 0;

    for (int i = 0; i < hunk->length; i++)
    {
        if (i + 1 >= hunk->length)  /* Malformed: Abort! */
            return NULL;

        num_credits++;

        /* Is it one of the database names? */
        if (hunk->data[i + 1] >= 0x80)
        {
            /* Yes:  Skip the byte and continue */
            i++;
            continue;
        }

        /* Otherwise, advance by the length of the ASCIIZ name. */
        i += strlen((const char *)(hunk->data + i + 1)) + 1;
    }

    /* -------------------------------------------------------------------- */
    /*  None here?  WTF?                                                    */
    /* -------------------------------------------------------------------- */
    if (!num_credits)
        return NULL;

    /* -------------------------------------------------------------------- */
    /*  Now we know how many credit records there are.  Allocate space      */
    /*  and then fill it.                                                   */
    /* -------------------------------------------------------------------- */
    ict_credits_t *const c = CALLOC(ict_credits_t, num_credits + 1);

    /* -------------------------------------------------------------------- */
    /*  Now pass over the data and populate all of the credit records.      */
    /* -------------------------------------------------------------------- */
    for (int i = 0, cur_credit = 0; i < hunk->length; i++, cur_credit++)
    {
        /* Decode the credit bits with a bitfield overlay. */
        c[cur_credit].credbits.u.raw[0] = hunk->data[i];

        /* Is it one of the database names? */
        if (hunk->data[i + 1] >= 0x80)
        {
            const int author_idx = hunk->data[i + 1] - 0x80;
            c[cur_credit].name = strdup(ic_author_list[author_idx]);
            /* Skip the byte and continue */
            i++;
            continue;
        }

        /* For UTF-8 names, we may have a stuffing byte ahead of the name.  */
        /* Only unstuff it is a valid stuffing scenario: Next byte is 0x01  */
        /* or next byte has its MSB set.                                    */
        if (hunk->data[i + 1] == 0x01 && 
            (hunk->data[i + 2] == 0x01 || (hunk->data[i + 2] & 0x80)))
            i++;

        /* Copy out the UTF-8Z name, and advance by that length. */
        c[cur_credit].name = strdup((const char *)(hunk->data + i + 1));
        i += strlen((const char *)(hunk->data + i + 1)) + 1;
    }

    return c;
}

LOCAL void ict_credits_dtor(ict_credits_t *const c)
{
    int i;

    if (!c) return;

    for (i = 0; c[i].name; ++i)
        CONDFREE(c[i].name);

    CONDFREE_k(c);
}

/* ======================================================================== */
/*  ICT_DATE                                                                */
/* ======================================================================== */
LOCAL game_date_t *ict_date_decode(const hunk_t *const hunk)
{
    game_date_t *const d = CALLOC(game_date_t, 1);

    if (!d)
        return NULL;

    if (uint8_t_to_game_date(d, hunk->data, hunk->length) != 0)
    {
        free(d);
        return NULL;
    }

    return d;
}

LOCAL void ict_date_dtor(game_date_t *const d)
{
    CONDFREE_k(d);
}

/* ======================================================================== */
/*  ICT_COMPAT                                                              */
/* ======================================================================== */
LOCAL ict_compat_t *ict_compat_decode(const hunk_t *const hunk)
{
    /* -------------------------------------------------------------------- */
    /*  Ignore too-short hunks.  Accept too-long hunks, but only parse the  */
    /*  bytes we know about.                                                */
    /* -------------------------------------------------------------------- */
    if (hunk->length < 3)
        return NULL;

    ict_compat_t *const c = CALLOC(ict_compat_t, 1);
    if (!c)
        return NULL;

    c->u.raw[0] = hunk->data[0];
    c->u.raw[1] = hunk->data[1];
    c->u.raw[2] = hunk->data[2];
    if (hunk->length > 3) c->u.raw[3] = hunk->data[3];
    if (hunk->length > 4) c->u.raw[4] = hunk->data[4];

    return c;
}

LOCAL void ict_compat_dtor(ict_compat_t *const c)
{
    CONDFREE_k(c);
}

/* ======================================================================== */
/*  ICARTTAG_DECODE                                                         */
/* ======================================================================== */
int icarttag_decode                         /* 0 on success, -1 on failure  */
(
    const uint8_t *const rom_img,           /* First byte of ROM file.      */
    const int            length,            /* Total size of .ROM file.     */
    const int            ignore_crc,        /* Disables CRC checks if set.  */
    const int            tag_ofs,           /* Offset of tags, if known.    */
                                            /* Pass in -1 if not known.     */
    const icarttag_visitor_t *const visitor,/* Visiting class               */
    int *const           visitor_error
)
{
#   define VISIT(m) (visitor && visitor->m ? \
                     (v_err = visitor->m(visitor->opaque)) : 0)
#   define VISIT_TAG(m,t) (visitor && visitor->visit_##m ? \
                           (v_err = visitor->visit_##m(visitor->opaque, (t))) \
                           : 0)
#   define LEAVE(r) do { result = (r); goto leave; } while (0)

    /* -------------------------------------------------------------------- */
    /*  Some of the visited metadata types consist solely of a struct       */
    /*  wrapping a single char pointer.  This helper makes it cheap to      */
    /*  copy hunk.data to a tag variable.                                   */
    /* -------------------------------------------------------------------- */
#   define HUNK_DATA_TAG(t) t tag_data = { (char *)hunk.data }; \
                            t* tag = &tag_data

    cursor_t cursor = { rom_img, length, 0, 0 };
    hunk_t hunk = { NULL, 0, 0, 0 };
    int result = 0, v_err = 0;

    /* -------------------------------------------------------------------- */
    /*  Fire up the visitor...                                              */
    /* -------------------------------------------------------------------- */
    if (VISIT(start))
        LEAVE(IC_VISITOR_ERROR);

    /* -------------------------------------------------------------------- */
    /*  Have icartrom_decode determine our starting offset, if needed.      */
    /*  This should only happen if we're called directly.  Usually, we are  */
    /*  called from icartrom_decode, and passed tag_ofs already.            */
    /* -------------------------------------------------------------------- */
    if (tag_ofs < 0)
    {
        cursor.offset = icartrom_decode(NULL, rom_img, length, ignore_crc, 1);
        if (cursor.offset < 0)          /* Error occurred during decode.    */
            LEAVE(cursor.offset);
    } else
    {
        cursor.offset = tag_ofs;
    }

    /* -------------------------------------------------------------------- */
    /*  Now step through all the tags...                                    */
    /* -------------------------------------------------------------------- */
    while (cursor.offset < cursor.length)
    {
        /* ---------------------------------------------------------------- */
        /*  Pull each tag out into a staging buffer, and verify checksum.   */
        /* ---------------------------------------------------------------- */
        ict_start_checksum(&cursor);

        hunk.length = ict_get_length(&cursor);
        if (hunk.length < 0 || hunk.length + cursor.offset + 2 > cursor.length)
            LEAVE(IC_SHORT_FILE);

        hunk.type = ict_next_byte(&cursor);
        if (hunk.type < 0)
            LEAVE(IC_SHORT_FILE);

        /* ---------------------------------------------------------------- */
        /*  Skip all IGNORE hunks entirely.                                 */
        /* ---------------------------------------------------------------- */
        if (hunk.type == ICT_IGNORE)
        {
            cursor.offset += hunk.length + 2;
            continue;
        }
    
        /* ---------------------------------------------------------------- */
        /*  Slurp data into the hunk, allocating more space if needed.      */
        /* ---------------------------------------------------------------- */
        if (hunk.length + 1 > hunk.alloc)
        {
            CONDFREE(hunk.data);
            hunk.alloc = 2 * (hunk.length + 1);
            hunk.data = CALLOC(uint8_t, hunk.alloc);
            if (!hunk.data)
                LEAVE(IC_OUT_OF_MEMORY);
        }

        for (int i = 0; i < hunk.length; i++)
            hunk.data[i] = ict_next_byte(&cursor);

        /* ---------------------------------------------------------------- */
        /*  Zero out data after hunk, to turn ASCII strings to ASCIIZ.      */
        /* ---------------------------------------------------------------- */
        memset(hunk.data + hunk.length, 0, hunk.alloc - hunk.length);

        /* ---------------------------------------------------------------- */
        /*  Validate CRC, unless we've been asked to ignore it.             */
        /* ---------------------------------------------------------------- */
        if (!ignore_crc && ict_check_checksum(&cursor))
            LEAVE(IC_CRC_ERROR_TAG);

        /* ---------------------------------------------------------------- */
        /*  Process the tag based on its type.                              */
        /* ---------------------------------------------------------------- */
        switch (hunk.type)
        {
            case ICT_FULL_TITLE:
            {
                HUNK_DATA_TAG(ict_title_t);
                if (VISIT_TAG(full_title, tag))
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_PUBLISHER:
            {
                ict_publisher_t *tag = ict_publisher_decode(&hunk);
                if (!tag) LEAVE(IC_TAG_PARSING_ERR);
                VISIT_TAG(publisher, tag);
                ict_publisher_dtor(tag);
                if (v_err)
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_CREDITS:
            {
                ict_credits_t *tag = ict_credits_decode(&hunk);
                if (!tag) LEAVE(IC_TAG_PARSING_ERR);
                VISIT_TAG(credits, tag);
                ict_credits_dtor(tag);
                if (v_err)
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_INFOURL:
            {
                HUNK_DATA_TAG(ict_infourl_t);
                if (VISIT_TAG(infourl, tag))
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_RELEASE_DATE:
            {
                game_date_t *tag = ict_date_decode(&hunk);
                if (!tag) LEAVE(IC_TAG_PARSING_ERR);
                VISIT_TAG(release_date, tag);
                ict_date_dtor(tag);
                if (v_err)
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_COMPAT:
            {
                ict_compat_t *tag = ict_compat_decode(&hunk);
                if (!tag) LEAVE(IC_TAG_PARSING_ERR);
                VISIT_TAG(compat, tag);
                ict_compat_dtor(tag);
                if (v_err)
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_BINDINGS:
            {
                /* Nope, not supporting. Bad idea. */
                break;
            }

            case ICT_SHORT_TITLE:
            {
                HUNK_DATA_TAG(ict_title_t);
                if (VISIT_TAG(short_title, tag))
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_LICENSE:
            {
                HUNK_DATA_TAG(ict_license_t);
                if (VISIT_TAG(license, tag))
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_DESCRIPTION:
            {
                HUNK_DATA_TAG(ict_desc_t);
                if (VISIT_TAG(desc, tag))
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_BUILD_DATE:
            {
                game_date_t *tag = ict_date_decode(&hunk);
                if (!tag) LEAVE(IC_TAG_PARSING_ERR);
                VISIT_TAG(build_date, tag);
                ict_date_dtor(tag);
                if (v_err)
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_VERSION:
            {
                HUNK_DATA_TAG(ict_version_t);
                if (VISIT_TAG(version, tag))
                    LEAVE(IC_VISITOR_ERROR);
                break;
            }

            case ICT_IGNORE:    /* shouldn't happen */
            default:
            {
                /* Ignore unknown tags. */
                break;
            }
        }
    }

    LEAVE(0);
    assert(0 || "Unreachable code reached\n");

leave:
    /* -------------------------------------------------------------------- */
    /*  Only call visitor->stop if there was no visitor error.              */
    /*  This is an ugly policy tradeoff, really.                            */
    /* -------------------------------------------------------------------- */
    if (!v_err)
        VISIT(stop);

    /* -------------------------------------------------------------------- */
    /*  Report the last visitor error back up to the user, if we can.       */
    /* -------------------------------------------------------------------- */
    if (visitor_error)
        *visitor_error = v_err;

    CONDFREE(hunk.data);
    return result ? result : cursor.offset;

#   undef LEAVE
#   undef VISIT
#   undef VISIT_TAG
#   undef HUNK_DATA_TAG
}

/* ======================================================================== */
/*  ICT_PREFIX_ASCII_TAG    -- Encode an ASCII tag with optional prefix     */
/* ======================================================================== */
LOCAL hunk_t ict_prefix_ascii_tag
(
    const int         tag,
    const char *const asciiz,
    int               prefix
)
{
    const int asciiz_length = asciiz ? strlen(asciiz) : 0;
    const int hunk_length = asciiz_length + (prefix >= 0);
    hunk_t hunk = { NULL, 0, 0, 0 };

    hunk.data = CALLOC(uint8_t, hunk_length);
    if (!hunk.data)
        return hunk;
    hunk.length = hunk_length;
    hunk.alloc  = hunk_length;
    hunk.type   = tag;

    if (prefix >= 0)
    {
        hunk.data[0] = prefix;
        if (asciiz_length)
            memcpy((void *)&(hunk.data[1]), (const void *)asciiz,
                    asciiz_length);
    } else
    {
        memcpy((void *)hunk.data, (const void *)asciiz, asciiz_length);
    }

    return hunk;
}

/* ======================================================================== */
/*  ICT_ASCII_TAG    -- Encode an ASCII tag hunk                            */
/* ======================================================================== */
LOCAL hunk_t ict_ascii_tag
(
    const int         tag,
    const char *const asciiz
)
{
    return ict_prefix_ascii_tag(tag, asciiz, -1);
}
        
/* ======================================================================== */
/*  ICT_ENCODE_COMPAT_TAG                                                   */
/* ======================================================================== */
LOCAL hunk_t ict_encode_compat_tag
(
    const game_metadata_t *const metadata
)
{
    game_metadata_t metacopy;
    hunk_t hunk = { NULL, 0, 0, 0 };
    const int num_spec = (metadata->ecs_compat != CMP_UNSPECIFIED)
                       + (metadata->voice_compat != CMP_UNSPECIFIED)
                       + (metadata->intv2_compat != CMP_UNSPECIFIED)
                       + (metadata->kc_compat != CMP_UNSPECIFIED)
                       + (metadata->tv_compat != CMP_UNSPECIFIED)
                       + (metadata->lto_mapper > 0)
                       + (metadata->jlp_accel != JLP_UNSPECIFIED)
                       + (metadata->jlp_flash > 0);

    /* If all config bits are unspecified, refuse to encode it. */
    if (num_spec == 0)
        return hunk;

    hunk.data   = CALLOC(uint8_t, 5);

    if (!hunk.data)
        return hunk;

    hunk.alloc  = 5;
    hunk.length = 3;    /* Assume no JLP or LTO Mapper to start */
    hunk.type   = ICT_COMPAT;

    metacopy = *metadata;
    game_metadata_set_unspec_compat_to_defaults( &metacopy );

    ict_compat_t *const compat = (ict_compat_t *)hunk.data;

    compat->u.s.keyboard_component = GM_TO_IC_COMPAT(metacopy.kc_compat);
    compat->u.s.intellivoice       = GM_TO_IC_COMPAT(metacopy.voice_compat);
    compat->u.s.ecs                = GM_TO_IC_COMPAT(metacopy.ecs_compat);
    compat->u.s.intellivision_2    = GM_TO_IC_COMPAT(metacopy.intv2_compat);
    compat->u.s.tutorvision        = GM_TO_IC_COMPAT(metacopy.tv_compat);

    /* If any JLP or LTO features are enabled, encode 4th byte. */
    if (metacopy.jlp_accel > JLP_DISABLED || metacopy.lto_mapper > 0)
    {
        hunk.length = 4;
        compat->u.s.lto_mapper = metacopy.lto_mapper > 0;
        compat->u.s.jlp_accel = 
            metacopy.jlp_accel >= JLP_DISABLED ? metacopy.jlp_accel 
                                               : JLP_DISABLED;
    }

    /* If JLP Flash storage is enabled, encode 5th byte. */
    if (metacopy.jlp_accel == JLP_ACCEL_OFF ||
        metacopy.jlp_accel == JLP_ACCEL_FLASH_ON)
    {
        assert(hunk.length == 4);
        hunk.length = 5;
        compat->u.s.jlp_flash_hi = (metacopy.jlp_flash >> 8) & 0x3;
        compat->u.s.jlp_flash_lo = metacopy.jlp_flash & 0xFF;
    }

    return hunk;
}

typedef struct name_list_t
{
    struct name_list_t *next;
    const char         *name;
    int16_t             known;
    uint8_t             flags;
} name_list_t;

/* ======================================================================== */
/*  ICT_LOOKUP_KNOWN_NAME                                                   */
/* ======================================================================== */
LOCAL int ict_lookup_known_name(const char *const name)
{
    for (int i = 0; i < 128; i++)
    {
        if (!stricmp(name, ic_author_list[i]))
            return i;
    }

    return -1;
}

/* ======================================================================== */
/*  ICT_ENCODE_CREDITS_TAG                                                  */
/* ======================================================================== */
LOCAL hunk_t ict_encode_credits_tag
(
    const game_metadata_t *const metadata,
    int *const error_return
)
{
    hunk_t hunk = { NULL, 0, 0, 0 };
    name_list_t *head = NULL;
    int tot_reqd_alloc = 0;
    const char **by_bit[8] =
    {
        metadata->authors,
        metadata->game_artists,
        metadata->composers,
        metadata->sfx_artists,
        metadata->voice_actors,
        metadata->doc_writers,
        metadata->conceptualizers,
        metadata->box_artists
    };

    /* -------------------------------------------------------------------- */
    /*  Default to "no error."                                              */
    /* -------------------------------------------------------------------- */
    if (error_return) *error_return = 0;

    /* -------------------------------------------------------------------- */
    /*  Step through each list, accumulating the unique names, and tagging  */
    /*  each name with the lists it appeared on.  For each unique name,     */
    /*  look it up in the database of known names.  All name comparisons    */
    /*  are case-insensitive.  I should consider making them punctuation    */
    /*  insensitive also, and add a table of aliases.                       */
    /* -------------------------------------------------------------------- */
    for (int i = 0, bit = 1; i < 8; i++, bit <<= 1)
    {
        if (!by_bit[i]) continue;

        for (int j = 0; by_bit[i][j]; j++)
        {
            name_list_t **p = &head;
            const char *const name = by_bit[i][j];
            int found_it = 0;

            while (*p)
            {
                if (!stricmp(name, (*p)->name))
                {
                    (*p)->flags |= bit;
                    found_it = 1;
                    break;
                }
                p = &((*p)->next);
            }

            if (found_it) continue;

            *p = CALLOC(name_list_t, 1);
            if (!*p)
                goto error_unwind;

            (*p)->next = NULL;
            (*p)->name = name;
            (*p)->known = ict_lookup_known_name(name);
            (*p)->flags = bit;

            tot_reqd_alloc += 1;  /* for the flags byte   */
            tot_reqd_alloc += 1;  /* for the author byte or terminating NUL */
            if ((*p)->known < 0)  /* for the body of the name if not in DB */
            {
                tot_reqd_alloc += strlen(name);
                /* Account for stuffing byte, if needed. */
                if (name[0] == 0x01 || (name[0] & 0x80))
                    tot_reqd_alloc++;
            }
        }
    }
    
    /* -------------------------------------------------------------------- */
    /*  Nothing to encode?  Leave!  We also have nothing to free...         */
    /* -------------------------------------------------------------------- */
    if (head == NULL)
    {
        assert(tot_reqd_alloc == 0);
        return hunk;
    }

    hunk.data   = CALLOC(uint8_t, tot_reqd_alloc);
    hunk.alloc  = tot_reqd_alloc;
    hunk.length = 0;
    hunk.type   = ICT_CREDITS;

    /* -------------------------------------------------------------------- */
    /*  Double-entry bookkeeping:  Walk the list and tally up length as we  */
    /*  build the encoded structure.  Also tear down our allocations.       */
    /* -------------------------------------------------------------------- */
    {
        name_list_t *prev = NULL, *curr = head;

        while (curr)
        {
            hunk.data[hunk.length++] = curr->flags;
            if (curr->known >= 0)
            {
                assert(curr->known < 128);
                hunk.data[hunk.length++] = curr->known | 0x80;
            } else
            {
                /* Include NUL in ASCIIZ */
                const int len = strlen(curr->name) + 1;

                /* Escape UTF-8 and 0x01 at start of a name. */
                if ((curr->name[0] & 0x80) || curr->name[0] == 0x01)
                    hunk.data[hunk.length++] = 0x01;

                memcpy((void *)&hunk.data[hunk.length],
                        curr->name, len);

                hunk.length += len;
            }

            prev = curr;
            curr = curr->next;
            free(prev);
        }

        head = NULL;
    }

    assert(hunk.length == hunk.alloc);

    return hunk;

error_unwind:
    {
        name_list_t *prev = NULL, *curr = head;
        while (curr)
        {
            prev = curr;
            curr = curr->next;
            free(prev);
        }
    }
    if (error_return) *error_return = IC_OUT_OF_MEMORY;
    return hunk;
}

/* ======================================================================== */
/*  ICT_APPEND_TAG   -- Appends hunk containing encoded tag to the output,  */
/*                      and empties the hunk.                               */
/* ======================================================================== */
LOCAL int ict_append_tag
(
    hunk_t *const output,
    hunk_t *const input
)
{
#   define LEAVE(e) do { result = (e); goto leave; } while(0)
    int result = 0;
    int lenlen = 0;
    uint8_t enc_length[4];

    /* -------------------------------------------------------------------- */
    /*  Encode the hunk length.                                             */
    /* -------------------------------------------------------------------- */
    enc_length[lenlen++] = input->length & 0x3F;

    if (input->length > 0x3F)
        enc_length[lenlen++] = (input->length >> 6) & 0xFF;

    if (input->length > 0x3FFF)
        enc_length[lenlen++] = (input->length >> 14) & 0xFF;

    if (input->length > 0x3FFFFF)
        enc_length[lenlen++] = (input->length >> 22) & 0xFF;

    enc_length[0] |= (lenlen - 1) << 6;

    /* -------------------------------------------------------------------- */
    /*  Ensure we have enough memory for this.                              */
    /* -------------------------------------------------------------------- */
    const int space_reqd = output->length + lenlen + 1 + input->length + 2;
    if (space_reqd > output->alloc)
    {
        int alloc_size = output->alloc ? output->alloc : 1024;
        while (alloc_size < space_reqd && alloc_size < INT_MAX / 2)
        {
            alloc_size *= 2;
        }

        if (alloc_size < space_reqd)
            LEAVE(IC_OUT_OF_MEMORY);

        output->data = REALLOC(output->data, uint8_t, alloc_size);

        if (!output->data)
            LEAVE(IC_OUT_OF_MEMORY);

        output->alloc = alloc_size;
        memset(output->data + output->length, 0,
               output->alloc - output->length);
    }

    /* -------------------------------------------------------------------- */
    /*  Copy in the new hunk.                                               */
    /* -------------------------------------------------------------------- */
    const int start_of_block = output->length;  /* start of checksum block  */

    for (int i = 0; i < lenlen; i++)
        output->data[output->length++] = enc_length[i];

    output->data[output->length++] = input->type & 0xFF;

    memcpy((void *)(output->data + output->length),
           (const void *)input->data,
           input->length);

    output->length += input->length;

    const uint16_t crc16 =
        crc16_block(0xFFFF,
                    (const uint8_t *)(output->data + start_of_block),
                    output->length - start_of_block);

    output->data[output->length++] = crc16 >> 8;
    output->data[output->length++] = crc16 & 0xFF;

leave:
    /* -------------------------------------------------------------------- */
    /*  Empty out the hunk, and return pass/fail.                           */
    /* -------------------------------------------------------------------- */
    free(input->data);
    input->length = 0;
    input->alloc = 0;

    return result;

#   undef LEAVE
}

/* ======================================================================== */
/*  ICARTTAG_ENCODE  -- Encode game_metadata into a series of ICART tags    */
/* ======================================================================== */
uint8_t *icarttag_encode
(
    const game_metadata_t *const metadata,
    size_t                *const encoded_length,
    int                   *const error_code
)
{
    hunk_t encoded = { NULL, 0, 0, 0 };
    int result = 0;
#   define LEAVE(x) do { result = (x); goto leave; } while (0);

    /* -------------------------------------------------------------------- */
    /*  Allow blindly calling us with NULL metadata.                        */
    /* -------------------------------------------------------------------- */
    if (!metadata)
    {
        if (encoded_length)
            *encoded_length = 0;
        if (error_code)
            *error_code = 0;
        return NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Encode tags in numerical order of tag ID.                           */
    /* -------------------------------------------------------------------- */

    /* -------------------------------------------------------------------- */
    /*  0x01: ICT_FULL_TITLE                                                */
    /* -------------------------------------------------------------------- */
    if (metadata->name)
    {
        hunk_t hunk = ict_ascii_tag(ICT_FULL_TITLE, metadata->name);
        const int err = ict_append_tag(&encoded, &hunk);
        if (err) LEAVE(err);
    }

    /* -------------------------------------------------------------------- */
    /*  0x02: ICT_PUBLISHER                                                 */
    /* -------------------------------------------------------------------- */
    if (metadata->publishers)
    {
        for (int i = 0; metadata->publishers[i]; ++i)
        {
            const int pnum = ict_lookup_publisher(metadata->publishers[i]);
            const int prefix = pnum >= 0 ? pnum : 0xFF;
            const char *const name = pnum < 0 ? metadata->publishers[i] : NULL;

            hunk_t hunk = ict_prefix_ascii_tag(ICT_PUBLISHER, name, prefix);
            const int err = ict_append_tag(&encoded, &hunk);
            if (err) LEAVE(err);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  0x03: ICT_CREDITS                                                   */
    /* -------------------------------------------------------------------- */
    {
        int err = 0;
        hunk_t hunk = ict_encode_credits_tag(metadata, &err);
        if (!err && hunk.length)
            err = ict_append_tag(&encoded, &hunk);
        if (err) LEAVE(err);
    }

    /* -------------------------------------------------------------------- */
    /*  0x04: ICT_INFOURL                                                   */
    /* -------------------------------------------------------------------- */
    if (metadata->more_infos)
    {
        for (int i = 0; metadata->more_infos[i]; ++i)
        {
            hunk_t hunk = ict_ascii_tag(ICT_INFOURL, metadata->more_infos[i]);
            const int err = ict_append_tag(&encoded, &hunk);

            if (err) LEAVE(err);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  0x05: ICT_RELEASE_DATE                                              */
    /* -------------------------------------------------------------------- */
    if (metadata->release_dates)
    {
        for (int i = 0; metadata->release_dates[i].year; ++i)
        {
            hunk_t hunk = { NULL, 0, 0, 0 };

            hunk.data = CALLOC(uint8_t, 16);
            if (!hunk.data)
                LEAVE(IC_OUT_OF_MEMORY);

            hunk.alloc  = 16;
            hunk.type   = ICT_RELEASE_DATE;
            hunk.length = game_date_to_uint8_t(&metadata->release_dates[i],
                                               hunk.data);

            if (hunk.length > 0)
            {
                const int err = ict_append_tag(&encoded, &hunk);
                if (err) LEAVE(err);
            }
            else
                free(hunk.data);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  0x06: ICT_COMPAT                                                    */
    /* -------------------------------------------------------------------- */
    if (!metadata->is_defaults)
    {
        hunk_t hunk = ict_encode_compat_tag(metadata);
        if (hunk.length)
        {
            const int err = ict_append_tag(&encoded, &hunk);
            if (err) LEAVE(err);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  0x07: ICT_BINDINGS                                                  */
    /* -------------------------------------------------------------------- */

    /* Not supported! */

    /* -------------------------------------------------------------------- */
    /*  0x08: ICT_SHORT_TITLE                                               */
    /* -------------------------------------------------------------------- */
    if (metadata->short_name)
    {
        hunk_t hunk = ict_ascii_tag(ICT_SHORT_TITLE, metadata->short_name);
        const int err = ict_append_tag(&encoded, &hunk);

        if (err) LEAVE(err);
    }

    /* -------------------------------------------------------------------- */
    /*  0x09: ICT_LICENSE                                                   */
    /* -------------------------------------------------------------------- */
    if (metadata->licenses)
    {
        for (int i = 0; metadata->licenses[i]; ++i)
        {
            hunk_t hunk = ict_ascii_tag(ICT_LICENSE, metadata->licenses[i]);
            const int err = ict_append_tag(&encoded, &hunk);

            if (err) LEAVE(err);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  0x0A: ICT_DESCRIPTION                                               */
    /* -------------------------------------------------------------------- */
    if (metadata->descriptions)
    {
        for (int i = 0; metadata->descriptions[i]; ++i)
        {
            hunk_t hunk = ict_ascii_tag(ICT_DESCRIPTION,
                                         metadata->descriptions[i]);
            const int err = ict_append_tag(&encoded, &hunk);

            if (err) LEAVE(err);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  0x0B: ICT_BUILD_DATE                                                */
    /* -------------------------------------------------------------------- */
    if (metadata->build_dates)
    {
        for (int i = 0; metadata->build_dates[i].year; ++i)
        {
            hunk_t hunk = { NULL, 0, 0, 0 };
            int err;

            hunk.data = CALLOC(uint8_t, 16);
            if (!hunk.data)
                LEAVE(IC_OUT_OF_MEMORY);

            hunk.alloc  = 16;
            hunk.type   = ICT_BUILD_DATE;
            hunk.length = game_date_to_uint8_t(&metadata->build_dates[i],
                                               hunk.data);

            if (hunk.length > 0)
            {
                err = ict_append_tag(&encoded, &hunk);
                if (err) LEAVE(err);
            }
            else
                free(hunk.data);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  0x0C: ICT_VERSION                                                   */
    /* -------------------------------------------------------------------- */
    if (metadata->versions)
    {
        for (int i = 0; metadata->versions[i]; ++i)
        {
            hunk_t hunk = ict_ascii_tag(ICT_VERSION,
                                         metadata->versions[i]);
            const int err = ict_append_tag(&encoded, &hunk);

            if (err) LEAVE(err);
        }
    }

leave:
    /* -------------------------------------------------------------------- */
    /*  Clean up and return what we have.  If there was any error at all,   */
    /*  discard everything we built.                                        */
    /* -------------------------------------------------------------------- */
    if (result == 0)
    {
        encoded.data = REALLOC(encoded.data, uint8_t, encoded.length);
        if (!encoded.data)
            result = IC_OUT_OF_MEMORY;

        encoded.alloc = encoded.length;
    }

    if (result != 0)
    {
        free(encoded.data);
        encoded.data = NULL;
        encoded.length = 0;
        encoded.alloc = 0;
    }

    if (encoded_length)
        *encoded_length = encoded.length;

    if (error_code)
        *error_code = result;

    return encoded.data;
#   undef LEAVE
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
/*                 Copyright (c) 2017-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
