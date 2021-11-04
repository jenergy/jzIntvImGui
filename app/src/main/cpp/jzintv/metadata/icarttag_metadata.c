#include "config.h"
#include "icart/icartrom.h"
#include "icart/icarttag.h"
#include "metadata/metadata.h"
#include "metadata/icarttag_metadata.h"
#include "misc/ll.h"

/* ======================================================================== */
/*  While building up the game_metadata_t, keep linked lists of records     */
/*  rather than resizing flat arrays.  The finalizer can then turn the      */
/*  list back into an array.                                                */
/* ======================================================================== */
typedef struct string_ll
{
    ll_t    l;
    char    *string;
} string_ll;

typedef struct tmp_gm_t
{
    int         failed;
    string_ll   *authors;
    string_ll   *game_artists;
    string_ll   *composers;
    string_ll   *sfx_artists;
    string_ll   *voice_actors;
    string_ll   *doc_writers;
    string_ll   *conceptualizers;
    string_ll   *box_artists;
    string_ll   *more_infos;
    string_ll   *publishers;
    string_ll   *licenses;
    string_ll   *descriptions;
    string_ll   *misc;
    string_ll   *versions;

    game_date_t *release_dates;
    game_date_t *build_dates;

    game_metadata_t *gm;
} tmp_gm_t;

/* ======================================================================== */
/*  Convert a linked list of strings into an array.  Free the list.         */
/* ======================================================================== */
LOCAL int gm_ict_string_ll_to_array(string_ll **ll, const char ***dest)
{
    int len = LL_LENGTH(*ll);
    const char **a = NULL;
    int i = len - 1;
    string_ll *curr;

    if (!len) goto leave;

    a = CALLOC(const char *, len + 1);
    if (!a) return IC_OUT_OF_MEMORY;

    for (curr = *ll; curr; curr = (string_ll *)(curr->l.next), --i)
    {
        a[i] = curr->string;
        curr->string = NULL;
    }

leave:
    LL_FREE(*ll);
    *ll = NULL;

    *dest = a;

    return 0;
}

/* ======================================================================== */
/*  GM_ICT_ASCII     -- Add a string to a list.  Build in reverse order.    */
/* ======================================================================== */
LOCAL int gm_ict_ascii(tmp_gm_t *tmp_gm, string_ll **ll, const char *string)
{
    char *string_copy = strdup(string);
    string_ll *head;

    if (!string_copy)
    {
        tmp_gm->failed = 1;
        return IC_OUT_OF_MEMORY;
    }

    head = CALLOC(string_ll, 1);
    if (!head)
    {
        free((void *)string_copy);
        tmp_gm->failed = 1;
        return IC_OUT_OF_MEMORY;
    }

    head->l.next = (ll_t *)*ll;
    head->string  = string_copy;
    *ll = head;

    return 0;
}

/* ======================================================================== */
/*  Forward the various string tag types up to gm_ict_ascii...              */
/* ======================================================================== */
LOCAL int gm_ict_publisher(void *opaque, const ict_publisher_t *t)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    return gm_ict_ascii(tmp_gm, &(tmp_gm->publishers), t->name);
}
    
LOCAL int gm_ict_infourl(void *opaque, const ict_infourl_t *i)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    return gm_ict_ascii(tmp_gm, &(tmp_gm->more_infos), i->url);
}
    
LOCAL int gm_ict_license(void *opaque, const ict_license_t *l)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    return gm_ict_ascii(tmp_gm, &(tmp_gm->licenses), l->license);
}
    
LOCAL int gm_ict_description(void *opaque, const ict_desc_t *d)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    return gm_ict_ascii(tmp_gm, &(tmp_gm->descriptions), d->text);
}

LOCAL int gm_ict_version(void *opaque, const ict_version_t *d)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    return gm_ict_ascii(tmp_gm, &(tmp_gm->versions), d->text);
}

/* ======================================================================== */
/*  Update the release_date or build_date list.                             */
/* ======================================================================== */
LOCAL int gm_ict_push_date(tmp_gm_t *tmp_gm, game_date_t **da,
                           const game_date_t *d)
{
    int num_dates;

    if (!*da)
    {
        *da = CALLOC(game_date_t, 2);
        if (!*da)
        {
            tmp_gm->failed = 1;
            return IC_OUT_OF_MEMORY;
        }
        (*da)[0] = *d;
        return 0;
    }

    for (num_dates = 0; (*da)[num_dates].year; ++num_dates)
        ;

    *da = REALLOC(*da, game_date_t, num_dates + 2);

    if (!*da)
    {
        tmp_gm->failed = 1;
        return IC_OUT_OF_MEMORY;
    }

    (*da)[num_dates++] = *d;
    (*da)[num_dates].year = 0;
    (*da)[num_dates].month = 0;
    (*da)[num_dates].day = 0;

    return 0;
}

LOCAL int gm_ict_release_date(void *opaque, const game_date_t *d)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    /* Since we don't expect more than a couple dates, realloc every time.  */
    game_date_t **da = &tmp_gm->release_dates;
    return gm_ict_push_date(tmp_gm, da, d);
}

LOCAL int gm_ict_build_date(void *opaque, const game_date_t *d)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    /* Since we don't expect more than a couple dates, realloc every time.  */
    game_date_t **da = &tmp_gm->build_dates;
    return gm_ict_push_date(tmp_gm, da, d);
}
    
/* ======================================================================== */
/*  Unpack the gnarly CREDITS records                                       */
/* ======================================================================== */
LOCAL int gm_ict_credits(void *opaque, const ict_credits_t *c)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    string_ll **by_bit[8] =
    {
        &(tmp_gm->authors),
        &(tmp_gm->game_artists),
        &(tmp_gm->composers),
        &(tmp_gm->sfx_artists),
        &(tmp_gm->voice_actors),
        &(tmp_gm->doc_writers),
        &(tmp_gm->conceptualizers),
        &(tmp_gm->box_artists)
    };
    int b, i, bit, err;

    for (i = 0; c[i].name; ++i)
    {
        for (b = 0; b < 8; ++b)
        {
            bit = 1 << b;
            if (c[i].credbits.u.raw[0] & bit)
            {
                err = gm_ict_ascii(tmp_gm, by_bit[b], c[i].name);
                if (err) return err;
            }
        }
    }

    return 0;
}

/* ======================================================================== */
/*  Unpack the COMPAT bits.  Assume we're non-default if we see this rec.   */
/* ======================================================================== */
LOCAL int gm_ict_compat(void *opaque, const ict_compat_t *c)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    game_metadata_t *gm = tmp_gm->gm;

    gm->is_defaults = 0;
    
    gm->ecs_compat      = IC_TO_GM_COMPAT(c->u.s.ecs);
    gm->voice_compat    = IC_TO_GM_COMPAT(c->u.s.intellivoice);
    gm->intv2_compat    = IC_TO_GM_COMPAT(c->u.s.intellivision_2);
    gm->kc_compat       = IC_TO_GM_COMPAT(c->u.s.keyboard_component);
    gm->tv_compat       = IC_TO_GM_COMPAT(c->u.s.tutorvision);
    gm->lto_mapper      = c->u.s.lto_mapper;
    gm->jlp_accel       = (jlp_accel_t)c->u.s.jlp_accel;
    gm->jlp_flash       = c->u.s.jlp_flash_lo | (c->u.s.jlp_flash_hi << 8);

    return 0;
}

/* ======================================================================== */
/*  Now the two easiest ones: Full Title, Short Title.                      */
/* ======================================================================== */
LOCAL int gm_ict_title(tmp_gm_t *tmp_gm, const char **field, const char *name)
{
    char *name_copy = strdup(name);

    if (!name_copy)
    {
        tmp_gm->failed = 1;
        return IC_OUT_OF_MEMORY;
    }

    if (*field) free(remove_const((const void *)*field));
    *field = name_copy;
    return 0;
}

LOCAL int gm_ict_full_title(void *opaque, const ict_title_t *t)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    return gm_ict_title(tmp_gm, &(tmp_gm->gm->name), t->name);
}

LOCAL int gm_ict_short_title(void *opaque, const ict_title_t *t)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    return gm_ict_title(tmp_gm, &(tmp_gm->gm->short_name), t->name);
}

/* ======================================================================== */
/*  Start the visitor process.                                              */
/* ======================================================================== */
LOCAL int gm_ict_start(void *opaque)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;

    /* See if we've already used this visitor. */
    if (tmp_gm->failed)
        return IC_BAD_ARGS;

    return 0;
}

/* ======================================================================== */
/*  Stop the visitor process.                                               */
/* ======================================================================== */
LOCAL int gm_ict_stop(void *opaque)
{
    tmp_gm_t *tmp_gm = (tmp_gm_t *)opaque;
    int err;

    /* See if we've failed along the way. */
    if (tmp_gm->failed)
        return IC_BAD_ARGS;

    /* We succeeded, so mark this structure as "no longer active." */
    tmp_gm->failed = -1;

    /* Now slurp out the linked lists into arrays. */
#   define SLURP(f) \
    do {                                                                    \
        err = gm_ict_string_ll_to_array(&(tmp_gm->f), &(tmp_gm->gm->f));    \
        if (err)                                                            \
        {                                                                   \
            tmp_gm->failed = 1;                                             \
            return err;                                                     \
        }                                                                   \
    } while (0) 

    SLURP(authors);
    SLURP(game_artists);
    SLURP(composers);
    SLURP(sfx_artists);
    SLURP(voice_actors);
    SLURP(doc_writers);
    SLURP(conceptualizers);
    SLURP(box_artists);
    SLURP(more_infos);
    SLURP(publishers);
    SLURP(licenses);
    SLURP(descriptions);
    SLURP(misc);
    SLURP(versions);

    tmp_gm->gm->release_dates = (const game_date_t *)tmp_gm->release_dates;
    tmp_gm->release_dates = NULL;

    tmp_gm->gm->build_dates = (const game_date_t *)tmp_gm->build_dates;
    tmp_gm->build_dates = NULL;
    
#   undef SLURP
    return 0;
}

const icarttag_visitor_t gm_ict_template =
{
    NULL,
    gm_ict_start,
    gm_ict_stop,
    gm_ict_full_title,
    gm_ict_publisher,
    gm_ict_credits,
    gm_ict_infourl,
    gm_ict_release_date,
    gm_ict_compat,
    gm_ict_short_title,
    gm_ict_license,
    gm_ict_description,
    gm_ict_build_date,
    gm_ict_version
};

/* ======================================================================== */
/*  GET_GAME_METADATA_ICARTTAG_VISITOR                                      */
/*                                                                          */
/*  Returns an icarttag_visitor_t that has an empty game_metadata_t object  */
/*  attached.  This object is suitable for passing to icarttag_decode.      */
/* ======================================================================== */
icarttag_visitor_t *get_game_metadata_icarttag_visitor(void)
{
    game_metadata_t *gm = default_game_metadata( );
    tmp_gm_t *tmp_gm = NULL;
    icarttag_visitor_t *ict_v = NULL;

    if (!gm) goto failed;

    tmp_gm = CALLOC(tmp_gm_t, 1);
    
    if (!tmp_gm) goto failed;

    game_metadata_set_compat_to_unspec( gm );
    tmp_gm->gm = gm;

    ict_v = CALLOC(icarttag_visitor_t, 1);

    if (!ict_v) goto failed;

    *ict_v = gm_ict_template;
    ict_v->opaque = (void *)tmp_gm;

    return ict_v;

failed:
    if (gm) free_game_metadata(gm);
    if (tmp_gm) free(tmp_gm);
    return NULL;
}

/* ======================================================================== */
/*  PUT_GAME_METADATA_ICARTTAG_VISITOR                                      */
/*                                                                          */
/*  Frees the icarttag_visitor_t from get_game_metadata_icarttag_visitor.   */
/*  Returns the game_metadata_t that was constructed, if any.               */
/* ======================================================================== */
struct game_metadata_t *
    put_game_metadata_icarttag_visitor(struct icarttag_visitor_t *ict_v)
{
    tmp_gm_t *tmp_gm = NULL;
    game_metadata_t *gm = NULL;

    if (!ict_v) goto failed;

    tmp_gm = (tmp_gm_t *)ict_v->opaque;

    if (!tmp_gm) goto failed;

    gm = tmp_gm->gm;

    if (tmp_gm->failed != -1) goto failed;
    if (!gm) goto failed;

    game_metadata_set_unspec_compat_to_defaults( gm );

    free(tmp_gm);
    free(ict_v);
    return gm;

failed:
    if (gm)     free_game_metadata(gm);
    if (tmp_gm) free(tmp_gm);
    if (ict_v)  free(ict_v);

    return NULL;
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
