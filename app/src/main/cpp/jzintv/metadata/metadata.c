#include "config.h"
#include "metadata/metadata.h"

/* ------------------------------------------------------------------------ */
/*  GAME_METADATA_SET_COMPAT_TO_UNSPEC                                      */
/*  Set the compatibility defaults in a game_metadata structure             */
/* ------------------------------------------------------------------------ */
void game_metadata_set_compat_to_unspec( game_metadata_t *const game_metadata )
{
    if ( !game_metadata ) return;

    game_metadata->ecs_compat    = CMP_UNSPECIFIED;
    game_metadata->voice_compat  = CMP_UNSPECIFIED;
    game_metadata->intv2_compat  = CMP_UNSPECIFIED;
    game_metadata->kc_compat     = CMP_UNSPECIFIED;
    game_metadata->tv_compat     = CMP_UNSPECIFIED;
    game_metadata->jlp_accel     = JLP_UNSPECIFIED;
    game_metadata->jlp_flash     = -1;
    game_metadata->lto_mapper    = -1;
    game_metadata->is_defaults   = 1;
}

/* ------------------------------------------------------------------------ */
/*  GAME_METADATA_SET_UNSPEC_COMPAT_TO_DEFAULTS                             */
/*  Return 1 if all compat fields were unspecified, 0 otherwise.            */
/* ------------------------------------------------------------------------ */
int game_metadata_set_unspec_compat_to_defaults
(
    game_metadata_t *const game_metadata
)
{
    if ( !game_metadata ) return 0;
    int is_defaults = game_metadata->is_defaults;

#define REPLACE_IF(x,y,z)               \
    if (game_metadata->x == (y))        \
    {                                   \
        game_metadata->x = (z);         \
    } else                              \
    {                                   \
        is_defaults = 0;                \
    }

    REPLACE_IF( ecs_compat,   CMP_UNSPECIFIED, CMP_TOLERATES );
    REPLACE_IF( voice_compat, CMP_UNSPECIFIED, CMP_TOLERATES );
    REPLACE_IF( intv2_compat, CMP_UNSPECIFIED, CMP_TOLERATES );
    REPLACE_IF( kc_compat,    CMP_UNSPECIFIED, CMP_TOLERATES );
    REPLACE_IF( tv_compat,    CMP_UNSPECIFIED, CMP_TOLERATES );
    REPLACE_IF( lto_mapper,   -1,              0             );

    if ( game_metadata->jlp_accel == JLP_UNSPECIFIED &&
         game_metadata->jlp_flash == -1 )
    {
        game_metadata->jlp_accel = JLP_DISABLED;
        game_metadata->jlp_flash = 0;
    }
    else
    {
        is_defaults = 0;

        if ( game_metadata->jlp_accel == JLP_UNSPECIFIED )
            game_metadata->jlp_accel = JLP_DISABLED;

        if ( game_metadata->jlp_flash < 0 )
        {
            if ( game_metadata->jlp_accel & 2 ) game_metadata->jlp_flash = 4;
            else                                game_metadata->jlp_flash = 0;
        }

        /* The name JLP_ACCEL_OFF kinda sucks. */
        if ( game_metadata->jlp_flash > 0 )
            game_metadata->jlp_accel =
                game_metadata->jlp_accel == JLP_DISABLED ? JLP_ACCEL_OFF
                                                         : JLP_ACCEL_FLASH_ON;
    }

#undef REPLACE_IF
    game_metadata->is_defaults = is_defaults;
    return is_defaults;
}

/* ------------------------------------------------------------------------ */
/*  DEFAULT_GAME_METADATA                                                   */
/* ------------------------------------------------------------------------ */
game_metadata_t *default_game_metadata( void )
{
    game_metadata_t *const game_metadata = CALLOC( game_metadata_t, 1 );

    if (!game_metadata)
        return NULL;

    game_metadata->ecs_compat    = CMP_TOLERATES;
    game_metadata->voice_compat  = CMP_TOLERATES;
    game_metadata->intv2_compat  = CMP_TOLERATES;
    game_metadata->kc_compat     = CMP_TOLERATES;
    game_metadata->tv_compat     = CMP_TOLERATES;
    game_metadata->jlp_accel     = JLP_DISABLED;
    game_metadata->jlp_flash     = 0;
    game_metadata->lto_mapper    = 0;
    game_metadata->is_defaults   = 1;

    return game_metadata;
}

/* ------------------------------------------------------------------------ */
/*  FREE_STRING_ARRAY   Helper for metadata_free                            */
/* ------------------------------------------------------------------------ */
LOCAL INLINE void free_string_array( const char **const a )
{
    if ( !a ) return;

    for (int i = 0; a[i]; ++i)
        CONDFREE_k( a[i] );

    CONDFREE_k(a);
}

/* ------------------------------------------------------------------------ */
/*  FREE_GAME_METADATA                                                      */
/*  Free the game_metadata structure and its contents                       */
/* ------------------------------------------------------------------------ */
void free_game_metadata( game_metadata_t *const game_metadata )
{
    if ( !game_metadata ) return;
    CONDFREE( game_metadata->name );
    CONDFREE( game_metadata->short_name );
    CONDFREE( game_metadata->release_dates );
    CONDFREE( game_metadata->build_dates );
    free_string_array( game_metadata->authors         );
    free_string_array( game_metadata->game_artists    );
    free_string_array( game_metadata->composers       );
    free_string_array( game_metadata->sfx_artists     );
    free_string_array( game_metadata->voice_actors    );
    free_string_array( game_metadata->doc_writers     );
    free_string_array( game_metadata->conceptualizers );
    free_string_array( game_metadata->box_artists     );
    free_string_array( game_metadata->more_infos      );
    free_string_array( game_metadata->publishers      );
    free_string_array( game_metadata->licenses        );
    free_string_array( game_metadata->descriptions    );
    free_string_array( game_metadata->misc            );
    free_string_array( game_metadata->versions        );

    CONDFREE_k( game_metadata );
}

/* ------------------------------------------------------------------------ */
/*  COPY_STRING_ARRAY       Helper for merge_game_metadata.                 */
/* ------------------------------------------------------------------------ */
LOCAL const char **copy_string_array(const char **const src)
{
    if (!src) return NULL;

    int len;
    for (len = 0; src[len]; ++len)
        ;

    const char **const dst = CALLOC(const char *, len + 1);

    for (int i = 0, j = 0; i < len; ++i)
    {
        const char *const copy = strdup(src[i]);
        if (copy)
            dst[j++] = copy;
    }

    return dst;
}

/* ------------------------------------------------------------------------ */
/*  MERGE_STRING_ARRAYS     Helper for merge_game_metadata.                 */
/* ------------------------------------------------------------------------ */
LOCAL const char **merge_string_arrays(const char **const src1,
                                       const char **const src2)
{
    const char **dst;

    if (!src1) return copy_string_array(src2);
    if (!src2) return copy_string_array(src1);

    int len1;
    for (len1 = 0; src1[len1]; ++len1)
        ;

    int len2;
    for (len2 = 0; src2[len2]; ++len2)
        ;

    const int len_dst_init = len1 + len2;

    dst = CALLOC(const char *, len_dst_init + 1);

    int len_dst_final = 0;
    for (int i = 0; i < len1; ++i)
    {
        int is_dupe = 0;
        for (int j = 0; j < len_dst_final; j++)
            if (!stricmp(dst[j], src1[i]))
            {
                is_dupe = 1;
                break;
            }

        if (!is_dupe)
        {
            const char *const copy = strdup(src1[i]);
            if (copy)
                dst[len_dst_final++] = copy;
        }
    }

    for (int i = 0; i < len2; ++i)
    {
        int is_dupe = 0;
        for (int j = 0; j < len_dst_final; j++)
            if (!stricmp(dst[j], src2[i]))
            {
                is_dupe = 1;
                break;
            }

        if (!is_dupe)
        {
            const char *const copy = strdup(src2[i]);
            if (copy)
                dst[len_dst_final++] = copy;
        }
    }

    assert(len_dst_final <= len_dst_init);

    if (len_dst_final < len_dst_init)
        dst = REALLOC(dst, const char *, len_dst_final + 1);

    return dst;
}

/* ------------------------------------------------------------------------ */
/*  COPY_DATE_ARRAY         Helper for merge_game_metadata                  */
/* ------------------------------------------------------------------------ */
LOCAL const game_date_t* copy_date_array(const game_date_t *const src)
{
    if (!src) return NULL;

    int len = 0;
    for (len = 0; src[len].year; ++len)
        ;

    game_date_t *const dst = CALLOC(game_date_t, len + 1);

    if (dst)
        memcpy(dst, src, sizeof(game_date_t) * (len + 1));

    return dst;
}

/* ------------------------------------------------------------------------ */
/*  MERGE_DATE_ARRAYS       Helper for merge_game_metadata                  */
/* ------------------------------------------------------------------------ */
LOCAL const game_date_t* merge_date_arrays(const game_date_t *const src1,
                                           const game_date_t *const src2)
{

    if (!src1) return copy_date_array(src2);
    if (!src2) return copy_date_array(src1);

    int len1 = 0;
    for (len1 = 0; src1[len1].year; ++len1)
        ;

    int len2 = 0;
    for (len2 = 0; src2[len2].year; ++len2)
        ;

    const int len_dst_init = len1 + len2;

    game_date_t *dst = CALLOC(game_date_t, len_dst_init + 1);

    int len_dst_final = 0;
    for (int i = 0; i < len1; ++i)
    {
        int is_dupe = 0;
        for (int j = 0; j < len_dst_final; j++)
            if (DATES_EQUAL(&dst[j], &src1[i]))
            {
                is_dupe = 1;
                break;
            }

        if (!is_dupe)
            dst[len_dst_final++] = src1[i];
    }

    for (int i = 0; i < len2; ++i)
    {
        int is_dupe = 0;
        for (int j = 0; j < len_dst_final; j++)
            if (DATES_EQUAL(&dst[j], &src2[i]))
            {
                is_dupe = 1;
                break;
            }

        if (!is_dupe)
            dst[len_dst_final++] = src2[i];
    }

    assert(len_dst_final <= len_dst_init);

    if (len_dst_final < len_dst_init)
        dst = REALLOC(dst, game_date_t, len_dst_final + 1);

    return dst;
}

/* ------------------------------------------------------------------------ */
/*  MERGE_GAME_METADATA                                                     */
/*                                                                          */
/*  Given two game_metadata structures, merge them into one that has data   */
/*  from them both.                                                         */
/*                                                                          */
/*  For "name" and "short_name", first argument takes precedence when both  */
/*  are set.                                                                */
/*                                                                          */
/*  For string-arrays and date arrays, all dupes are eliminated.            */
/*                                                                          */
/*  For "compat" entries, we us the following hierarchy:                    */
/*                                                                          */
/*   -- If both have is_defaults = 1, first argument wins.                  */
/*   -- If exactly one has is_defaults = 0, its settings win.               */
/*   -- If both have is_defaults = 0, then things get fun.                  */
/*                                                                          */
/*                    |   UNS  INC  TOL  ENH  REQ  second                   */
/*            --------+-----------------------------------                  */
/*               UNS  |   UNS  INC  TOL  ENH  REQ                           */
/*               INC  |   INC  INC  INC  INC  uns                           */
/*               TOL  |   TOL  INC  TOL  ENH  REQ                           */
/*               ENH  |   ENH  INC  ENH  ENH  REQ                           */
/*               REQ  |   REQ  uns  REQ  REQ  REQ                           */
/*             first  |                                                     */
/*                                                                          */
/*   -- For jlp_accel:  The greater value takes precedence.                 */
/*   -- For jlp_flash:  The greater value takes precedence.                 */
/*   -- For lto_mapper:  The greater value takes precedence.                */
/*                                                                          */
/*  A new game_metadata structure gets created; neither of the source args  */
/*  is modified.                                                            */
/* ------------------------------------------------------------------------ */
game_metadata_t *merge_game_metadata
(
    const game_metadata_t *const src1,
    const game_metadata_t *const src2
)
{
#   define UNS CMP_UNSPECIFIED
#   define INC CMP_INCOMPATIBLE
#   define TOL CMP_TOLERATES
#   define ENH CMP_ENHANCED
#   define REQ CMP_REQUIRES

    static const compat_level_t cmp_map[5][5] =
    {
        {   UNS, INC, TOL, ENH, REQ   },
        {   INC, INC, INC, INC, UNS   },
        {   TOL, INC, TOL, ENH, REQ   },
        {   ENH, INC, ENH, ENH, REQ   },
        {   REQ, UNS, REQ, REQ, REQ   },
    };

#   undef UNS
#   undef INC
#   undef TOL
#   undef ENH
#   undef REQ
#   define GT_MAP(a) dst->a = src1->a > src2->a ? src1->a : src2->a;
#   define CMP_MAP(a) dst->a = (cmp_map[(int)src1->a + 1][(int)src2->a + 1])
#   define MSA(a) dst->a = merge_string_arrays(src1->a, src2->a)
#   define MDA(a) dst->a = merge_date_arrays(src1->a, src2->a)

    game_metadata_t *dst = CALLOC(game_metadata_t, 1);
    const game_metadata_t *csrc = NULL;

    if (!dst) return NULL;

    if (src1->name)
        dst->name = strdup(src1->name);
    else if (!src1->name && src2->name)
        dst->name = strdup(src2->name);

    if (src1->short_name)
        dst->short_name = strdup(src1->short_name);
    else if (!src1->short_name && src2->short_name)
        dst->short_name = strdup(src2->short_name);

    MSA(authors);
    MSA(game_artists);
    MSA(composers);
    MSA(sfx_artists);
    MSA(voice_actors);
    MSA(doc_writers);
    MSA(conceptualizers);
    MSA(box_artists);
    MSA(more_infos);
    MSA(publishers);
    MDA(release_dates);
    MSA(licenses);
    MSA(descriptions);
    MSA(misc);
    MDA(build_dates);
    MSA(versions);

    game_metadata_set_compat_to_unspec( dst );

    if      ( src1->is_defaults &&  src2->is_defaults) dst->is_defaults = 1;
    else if (!src1->is_defaults &&  src2->is_defaults) csrc = src1;
    else if ( src1->is_defaults && !src2->is_defaults) csrc = src2;
    else if (!src1->is_defaults && !src2->is_defaults)
    {
        CMP_MAP(ecs_compat);
        CMP_MAP(voice_compat);
        CMP_MAP(intv2_compat);
        CMP_MAP(kc_compat);
        CMP_MAP(tv_compat);
        dst->is_defaults = 0;
    }

    if (csrc)
    {
        dst->ecs_compat   = csrc->ecs_compat;
        dst->voice_compat = csrc->voice_compat;
        dst->intv2_compat = csrc->intv2_compat;
        dst->kc_compat    = csrc->kc_compat;
        dst->tv_compat    = csrc->tv_compat;
        dst->is_defaults  = 0;
    }

    GT_MAP(lto_mapper);
    GT_MAP(jlp_accel);
    GT_MAP(jlp_flash);

    return dst;

#   undef GT_MAP
#   undef CMP_MAP
#   undef MSA
#   undef MDA
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
/*                  Copyright (c) 2016, Joseph Zbiciak                      */
/* ======================================================================== */
