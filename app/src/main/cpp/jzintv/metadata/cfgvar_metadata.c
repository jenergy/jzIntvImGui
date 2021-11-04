/* ======================================================================== */
/*  Routines to generate game_metadata_t from a a list of cfg_var_t.        */
/* ======================================================================== */

#include "config.h"
#include "misc/ll.h"
#include "misc/types.h"
#include "metadata/metadata.h"
#include "metadata/cfgvar_metadata.h"

/* ======================================================================== */
/* ------------------------------------------------------------------------ */
/*  CFG_VAR_T to GAME_METADATA_T routines                                   */
/* ------------------------------------------------------------------------ */
/* ======================================================================== */

/* ======================================================================== */
/*  GM_CFG_VAR_MATCH_EXPR    -- Return 1 if a var matches name and flags.   */
/* ======================================================================== */
typedef struct cfg_var_match_expr_t
{
    const char *var_name;   /* Name to look for                     */
    uint32_t    flag_all;   /* All flags must be present            */
    uint32_t    flag_any;   /* At least one flag must be present    */
} cfg_var_match_expr_t;

LOCAL int cfg_var_match_expr
(
    const cfg_var_t            *RESTRICT const var,
    const cfg_var_match_expr_t *RESTRICT const expr
)
{
    if ( expr->flag_all && (var->val.flag & expr->flag_all) != expr->flag_all )
        return 0;
    if ( expr->flag_any && (var->val.flag & expr->flag_any) == 0 )
        return 0;
    if ( strcmp( var->name, expr->var_name ) != 0 )
        return 0;

    return 1;
}

/* ======================================================================== */
/*  GM_COUNT_HLPR            -- Helper function for GM_COUNT_VAR            */
/* ======================================================================== */
typedef struct gm_count_hlpr_t
{
    const cfg_var_match_expr_t *match_expr;
    int                         count;
} gm_count_hlpr_t;

LOCAL void gm_count_hlpr(ll_t *l_var, void *state_vp)
{
    gm_count_hlpr_t *RESTRICT const state  = (gm_count_hlpr_t *)state_vp;
    cfg_var_t       *RESTRICT const var    = (cfg_var_t *)l_var;

    if ( cfg_var_match_expr( var, state->match_expr ) )
        state->count++;
}

/* ======================================================================== */
/*  GM_COUNT_VAR             -- Count the number of vars with a given name  */
/* ======================================================================== */
LOCAL INLINE int gm_count_var
(
    cfg_var_t                  *RESTRICT const vars,
    const cfg_var_match_expr_t *RESTRICT const match_expr
)
{
    gm_count_hlpr_t state = { match_expr, 0 };

    LL_ACTON( vars, gm_count_hlpr, (void*)&state );

    return state.count;
}

/* ======================================================================== */
/*  GM_SV2A_HLPR             -- Helper function for GM_STRING_VAR_TO_ARRAY  */
/* ======================================================================== */
typedef struct gm_sv2a_hlpr_t
{
    const cfg_var_match_expr_t *match_expr;
    const char                **array;
    int                         index, max_index;
} gm_sv2a_hlpr_t;

LOCAL void gm_sv2a_hlpr(ll_t *l_var, void *state_vp)
{
    cfg_var_t      *RESTRICT const var    = (cfg_var_t *)l_var;
    gm_sv2a_hlpr_t *RESTRICT const state  = (gm_sv2a_hlpr_t *)state_vp;

    /* Don't even bother filtering if we've seen all we expect to see.  */
    if ( state->index >= state->max_index )
        return;

    /* Only process variables that match the filter                     */
    if ( !cfg_var_match_expr( var, state->match_expr ) )
        return;

    state->array[ state->index++ ] = strdup( var->val.str_val );
}

/* ======================================================================== */
/*  GM_STRING_VAR_TO_ARRAY   -- Get all string vars with a particular name  */
/*                              to a const char**.                          */
/* ======================================================================== */
LOCAL void gm_string_var_to_array
(
    cfg_var_t   *RESTRICT   const vars,
    const char  *RESTRICT   const var_name,
    const char  ***RESTRICT const ptr_to_array
)
{
    const cfg_var_match_expr_t 
                            match_expr = { var_name, VAL_STRING, VAL_STRING };
    gm_sv2a_hlpr_t array_builder = { &match_expr, NULL, 0, 0 };

    array_builder.max_index = gm_count_var( vars, &match_expr );
    array_builder.index = 0;

    if ( !array_builder.max_index )
        return;

    if ( *ptr_to_array )
    {
        while ( (*ptr_to_array)[array_builder.index] )
        {
            array_builder.index++;
            array_builder.max_index++;
        }
    }

    array_builder.array = REALLOC( *ptr_to_array, const char *,
                                    array_builder.max_index + 1 );

    for ( int i = array_builder.index; i <= array_builder.max_index; ++i )
        array_builder.array[i] = NULL;

    LL_ACTON( vars, gm_sv2a_hlpr, (void*)&array_builder );

    *ptr_to_array = array_builder.array;
}

/* ======================================================================== */
/*  GM_DV2A_HLPR            -- Helper function for GM_DATE_VAR_TO_ARRAY     */
/* ======================================================================== */
typedef struct gm_dv2a_hlpr_t
{
    const cfg_var_match_expr_t *match_expr;
    game_date_t                *array;
    int                         index, max_index;
} gm_dv2a_hlpr_t;

LOCAL void gm_dv2a_hlpr(ll_t *l_var, void *state_vp)
{
    cfg_var_t      *RESTRICT const var    = (cfg_var_t *)l_var;
    gm_dv2a_hlpr_t *RESTRICT const state  = (gm_dv2a_hlpr_t *)state_vp;

    /* Don't even bother filtering if we've seen all we expect to see.  */
    if ( state->index >= state->max_index )
        return;

    /* Only process variables that match the filter                     */
    if ( !cfg_var_match_expr( var, state->match_expr ) )
        return;

    state->array[ state->index++ ] = var->val.date_val;
}

/* ======================================================================== */
/*  GM_DATE_VAR_TO_ARRAY     -- Get all date variables w/ a particular      */
/*                              name to a const game_date_t* array.         */
/* ======================================================================== */
LOCAL void gm_date_var_to_array
(
    cfg_var_t          *RESTRICT const vars,
    const char         *RESTRICT const var_name,
    const game_date_t **RESTRICT const ptr_to_array
)
{
    const cfg_var_match_expr_t match_expr =
        { var_name, VAL_DATE, VAL_DATE };
    gm_dv2a_hlpr_t array_builder = { &match_expr, NULL, 0, 0 };

    array_builder.max_index = gm_count_var( vars, &match_expr );
    array_builder.index = 0;

    if ( !array_builder.max_index )
        return;

    if ( *ptr_to_array )
    {
        while ( (*ptr_to_array)[array_builder.index].year )
        {
            array_builder.index++;
            array_builder.max_index++;
        }
    }

    array_builder.array = REALLOC( *ptr_to_array, game_date_t,
                                   array_builder.max_index + 1 );

    for ( int i = array_builder.index; i <= array_builder.max_index; ++i )
    {
        const game_date_t g = { 0 };
        array_builder.array[i] = g;
    }

    LL_ACTON( vars, gm_dv2a_hlpr, (void*)&array_builder );

    *ptr_to_array = array_builder.array;
}

/* ======================================================================== */
/*  GM_FIND_STRING_VAR   -- Return value of first matching string.          */
/*                          Allocates copy of value w/ strdup.              */
/* ======================================================================== */
LOCAL const char *gm_find_string_var
(
    cfg_var_t   *RESTRICT const vars,
    const char  *RESTRICT const var_name
)
{
    const cfg_var_match_expr_t match_expr =
        { var_name, VAL_STRING, VAL_STRING };
    cfg_var_t *var;

    for ( var = vars ; var ; var = (cfg_var_t *)var->l.next)
        if ( cfg_var_match_expr( var, &match_expr ) )
            return strdup( var->val.str_val );

    return NULL;
}

/* ======================================================================== */
/*  GM_FIND_INT_VAR      -- Return value of first matching integer,         */
/*                          optionally remapped by a remapping function.    */
/* ======================================================================== */
LOCAL int gm_find_int_var
(
    cfg_var_t   *RESTRICT const  vars,
    const char  *RESTRICT const  var_name,
    int         *RESTRICT const  val_ptr,
    int                        (*remap)(int)
)
{
    const cfg_var_match_expr_t match_expr =
        { var_name, 0, VAL_DECNUM | VAL_HEXNUM};
    cfg_var_t *var;

    for ( var = vars ; var ; var = (cfg_var_t *)var->l.next)
        if ( cfg_var_match_expr( var, &match_expr ) )
        {
            int val = VAL_HAS_DECNUM( var->val ) ? var->val.dec_val
                                                 : (int)var->val.hex_val;
            *val_ptr = remap ? remap(val) : val;
            return 1;
        }

    return 0;
}


/* ======================================================================== */
/*  Integer remapping helpers for GM_METADATA_FROM_CONFIG below             */
/* ======================================================================== */
LOCAL int gm_remap_int_compat( int val )
{
    return val < 0 ? 0
         : val > 3 ? 3
         :           val;
}

LOCAL int gm_remap_int_ecs( int val )
{
    return val == 0 ? CMP_TOLERATES : CMP_REQUIRES;
}

LOCAL int gm_remap_int_voice( int val )
{
    return val == 0 ? CMP_TOLERATES : CMP_ENHANCED;
}

LOCAL int gm_remap_int_intv2( int val )
{
    return val == 0 ? CMP_INCOMPATIBLE : CMP_TOLERATES;
}

LOCAL int gm_remap_int_jlp_flash( int val )
{
    return val < 0             ? 0
         : val > JLP_FLASH_MAX ? JLP_FLASH_MAX
         :                       val;
}

#define GM_STRING(key,val) \
    { (key), &(local_gm.val), NULL, NULL, NULL, NULL }
#define GM_STRING_ARRAY(key,val) \
    { (key), NULL, &(local_gm.val), NULL, NULL, NULL }
#define GM_DATE_ARRAY(key,val) \
    { (key), NULL, NULL, &(local_gm.val), NULL, NULL }
#define GM_INT(key,val,remap) \
    { (key), NULL, NULL, NULL, (int *)&(local_gm.val), gm_remap_int_##remap }

typedef struct metadata_map_t
{
    const char           *name;
    const char          **ptr_to_string;
    const char         ***ptr_to_string_array;
    const game_date_t   **ptr_to_date_array;
    int                  *ptr_to_int;   /* Also compat_level, jlp_accel */
    int                  (*remap_int)(int);
} metadata_map_t;

/* Sigh, static to make old-fashioned C happy. */
LOCAL game_metadata_t local_gm;

LOCAL const metadata_map_t metadata_map[] =
{
    GM_STRING       ( "name",               name                        ),
    GM_STRING       ( "short_name",         short_name                  ),
    GM_STRING_ARRAY ( "author",             authors                     ),
    GM_STRING_ARRAY ( "game_art_by",        game_artists                ),
    GM_STRING_ARRAY ( "music_by",           composers                   ),
    GM_STRING_ARRAY ( "sfx_by",             sfx_artists                 ),
    GM_STRING_ARRAY ( "voices_by",          voice_actors                ),
    GM_STRING_ARRAY ( "docs_by",            doc_writers                 ),
    GM_STRING_ARRAY ( "concept_by",         conceptualizers             ),
    GM_STRING_ARRAY ( "box_art_by",         box_artists                 ),
    GM_STRING_ARRAY ( "more_info_at",       more_infos                  ),
    GM_STRING_ARRAY ( "publisher",          publishers                  ),
    GM_STRING_ARRAY ( "license",            licenses                    ),
    GM_STRING_ARRAY ( "desc",               descriptions                ),
    GM_STRING_ARRAY ( "description",        descriptions                ),
    GM_STRING_ARRAY ( "version",            versions                    ),
    GM_DATE_ARRAY   ( "year",               release_dates               ),
    GM_DATE_ARRAY   ( "release_date",       release_dates               ),
    GM_DATE_ARRAY   ( "build_date",         build_dates                 ),
    GM_INT          ( "ecs",                ecs_compat,     ecs         ),
    GM_INT          ( "ecs_compat",         ecs_compat,     compat      ),
    GM_INT          ( "voice",              voice_compat,   voice       ),
    GM_INT          ( "voice_compat",       voice_compat,   compat      ),
    GM_INT          ( "intv2",              intv2_compat,   intv2       ),
    GM_INT          ( "intv2_compat",       intv2_compat,   compat      ),
    GM_INT          ( "kc_compat",          kc_compat,      compat      ),
    GM_INT          ( "tv_compat",          tv_compat,      compat      ),
    GM_INT          ( "lto_mapper",         lto_mapper,     compat      ),
    GM_INT          ( "jlp",                jlp_accel,      compat      ),
    GM_INT          ( "jlp_accel",          jlp_accel,      compat      ),
    GM_INT          ( "jlpflash",           jlp_flash,      jlp_flash   ),
    GM_INT          ( "jlp_flash",          jlp_flash,      jlp_flash   ),
};

#define METADATA_MAP_SIZE ((int)(sizeof(metadata_map)/sizeof(metadata_map[0])))

/* ======================================================================== */
/*  GM_IS_MISC_VAR           -- Returns true if not in metadata_map         */
/* ======================================================================== */
LOCAL int gm_is_misc_var( const char *name )
{
    int i;

    for ( i = 0; i < METADATA_MAP_SIZE; i++ )
        if ( !strcmp( name, metadata_map[i].name ) )
            return 0;

    return 1;
}

/* ======================================================================== */
/*  GAME_METADATA_FROM_CFGVARS   -- Get all metadata from list of cfg vars  */
/* ======================================================================== */
game_metadata_t *game_metadata_from_cfgvars
(
    cfg_var_t *RESTRICT const vars
)
{
    cfg_var_t *var;
    int i, misc_count = 0;
    game_metadata_t *metadata = CALLOC( game_metadata_t, 1 );

    if (!metadata)
        return NULL;

    memset( (void *)&local_gm, 0, sizeof( local_gm ) );
    game_metadata_set_compat_to_unspec( &local_gm );

    /* -------------------------------------------------------------------- */
    /*  This should be a compile-time assertion...                          */
    /*  We're abusing the fact that compat_level_t and jlp_accel_t are      */
    /*  int under the hood.                                                 */
    /* -------------------------------------------------------------------- */
    assert(sizeof(int) == sizeof(compat_level_t));
    assert(sizeof(int) == sizeof(jlp_accel_t));

    /* -------------------------------------------------------------------- */
    /*  Yeah, this is a slow, brute-force approach.  The number of vars     */
    /*  involved is really too small to justify getting fancier.            */
    /* -------------------------------------------------------------------- */
    for ( i = 0; i < METADATA_MAP_SIZE ; i++ )
    {
        const metadata_map_t *RESTRICT const mm = &metadata_map[i];

        if ( mm->ptr_to_string )
        {
            if ( !*mm->ptr_to_string )
                *mm->ptr_to_string = gm_find_string_var( vars, mm->name );
            continue;
        }

        if ( mm->ptr_to_int )
        {
            gm_find_int_var( vars, mm->name, mm->ptr_to_int, mm->remap_int );
            continue;
        }

        if ( mm->ptr_to_string_array )
        {
            gm_string_var_to_array( vars, mm->name, mm->ptr_to_string_array );
            continue;
        }

        if ( mm->ptr_to_date_array )
        {
            gm_date_var_to_array( vars, mm->name, mm->ptr_to_date_array );
            continue;
        }

        assert( 0 && "Unhandled metadata_map entry" );
    }

    /* -------------------------------------------------------------------- */
    /*  Now collect up all the vars not handled as metadata and throw them  */
    /*  into the "misc" bucket.  Another dreadfully slow, brute-force bit   */
    /*  of code.  Again... number of vars involved too small.               */
    /*                                                                      */
    /*  To paraphrase Zoidberg:  This code is bad and I should feel bad.    */
    /*                                                                      */
    /*  To do this right (and the loop above, too), we'd throw everything   */
    /*  into a hash and iterate over the list of vars once.                 */
    /* -------------------------------------------------------------------- */
    for ( var = vars; var; var = (cfg_var_t *)(var->l.next) )
        if ( var->val.str_val && gm_is_misc_var( var->name ) )
            misc_count++;

    if ( misc_count )
        local_gm.misc = CALLOC( const char *, misc_count + 1 );

    if ( local_gm.misc )
    {
        int idx = 0;
        for ( var = vars; var; var = (cfg_var_t *)(var->l.next) )
        {
            int name_len, val_len;
            char *str;

            if ( !var->val.str_val || !gm_is_misc_var( var->name ) )
                continue;

            name_len = strlen( var->name );
            val_len  = strlen( var->val.str_val );

            str = CALLOC( char, name_len + val_len + 2 );
            if ( !str )
                break;

            strcpy( str, var->name );
            str[name_len] = '=';
            strcpy( str + name_len + 1, var->val.str_val );

            local_gm.misc[ idx++ ] = str;
        }
    }

    game_metadata_set_unspec_compat_to_defaults( &local_gm );

    memcpy( (void *)metadata, (void *)&local_gm, sizeof( local_gm ) );
    memset( (void *)&local_gm, 0, sizeof( local_gm ) );

    return metadata;
}

/* ======================================================================== */
/* ------------------------------------------------------------------------ */
/*  GAME_METADATA_T to CFG_VAR_T routines                                   */
/* ------------------------------------------------------------------------ */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  Remap compatibility flags to corresponding cfg_vars.                    */
/* ------------------------------------------------------------------------ */
typedef struct compat_remap_t
{
    const char *name;
    int         value;
    int         explicit_only;
} compat_remap_t;

static const compat_remap_t remap_ecs[5] =
{
    {   "ecs_compat",       1,      1   },      /* UNSPECIFIED  */
    {   "ecs",              0,      1   },      /* INCOMPATIBLE */
    {   "ecs_compat",       1,      1   },      /* TOLERATES    */
    {   "ecs_compat",       2,      0   },      /* ENHANCED     */
    {   "ecs",              1,      0   },      /* REQUIRES     */
};

static const compat_remap_t remap_voice[5] =
{
    {   "voice_compat",     1,      1   },      /* UNSPECIFIED  */
    {   "voice_compat",     0,      0   },      /* INCOMPATIBLE */
    {   "voice",            0,      1   },      /* TOLERATES    */
    {   "voice",            1,      0   },      /* ENHANCED     */
    {   "voice_compat",     3,      0   },      /* REQUIRES     */
};

static const compat_remap_t remap_intv2[5] =
{
    {   "intv2",            1,      1   },      /* UNSPECIFIED  */
    {   "intv2",            0,      0   },      /* INCOMPATIBLE */
    {   "intv2",            1,      1   },      /* TOLERATES    */
    {   "intv2_compat",     2,      0   },      /* ENHANCED     */
    {   "intv2_compat",     3,      0   },      /* REQUIRES     */
};

static const compat_remap_t remap_kc[5] =
{
    {   "kc_compat",        1,      1   },      /* UNSPECIFIED  */
    {   "kc_compat",        0,      0   },      /* INCOMPATIBLE */
    {   "kc_compat",        1,      1   },      /* TOLERATES    */
    {   "kc_compat",        2,      0   },      /* ENHANCED     */
    {   "kc_compat",        3,      0   },      /* REQUIRES     */
};

static const compat_remap_t remap_tv[5] =
{
    {   "tv_compat",        1,      1   },      /* UNSPECIFIED  */
    {   "tv_compat",        0,      0   },      /* INCOMPATIBLE */
    {   "tv_compat",        1,      1   },      /* TOLERATES    */
    {   "tv_compat",        2,      0   },      /* ENHANCED     */
    {   "tv_compat",        3,      0   },      /* REQUIRES     */
};

LOCAL int compat_remap
(
    cfg_var_t           **RESTRICT       vars,
    const compat_remap_t *RESTRICT const remap,
    const int                            explicit_flags,
    compat_level_t                       compat
)
{
    const compat_remap_t *RESTRICT const remap_entry = remap + (int)compat + 1;
    const int do_it = remap_entry->explicit_only == 0 || explicit_flags;

    if (do_it)
        return
            append_cfg_var( vars,
                cons_cfg_var_dec( remap_entry->name,
                                  remap_entry->value ) ) == NULL;

    return 0;
}

/* ------------------------------------------------------------------------ */
/*  Append an array of strings as cfg_vars                                  */
/* ------------------------------------------------------------------------ */
LOCAL int append_cfg_vars
(
    cfg_var_t  **RESTRICT vars,
    const char  *RESTRICT name,
    const char **RESTRICT values
)
{
    int i;

    if (!values) return 0;

    for (i = 0; values[i]; ++i)
    {
        if (!append_cfg_var( vars,
                             cons_cfg_var_string( name, values[i] ) ) )
            return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------ */
/*  CFGVARS_FROM_GAME_METADATA                                              */
/*  Create a list of configuration variables from game_metadata_t.          */
/*  Currently quadratic in the number of variables, which is fine, given    */
/*  there shouldn't be more than a dozen or so.                             */
/* ------------------------------------------------------------------------ */
cfg_var_t *cfgvars_from_game_metadata(game_metadata_t *RESTRICT const m)
{
    const int ef = !m->is_defaults;
    cfg_var_t *vars = NULL;

    if (compat_remap(&vars, remap_ecs,   ef, m->ecs_compat  )) goto fail;
    if (compat_remap(&vars, remap_voice, ef, m->voice_compat)) goto fail;
    if (compat_remap(&vars, remap_intv2, ef, m->intv2_compat)) goto fail;
    if (compat_remap(&vars, remap_kc,    ef, m->kc_compat   )) goto fail;
    if (compat_remap(&vars, remap_tv,    ef, m->tv_compat   )) goto fail;

    if (m->lto_mapper > 0 || ef)
    {
        if (!append_cfg_var( &vars,
                             cons_cfg_var_dec( "lto_mapper", m->lto_mapper )))
            goto fail;
    }

    if (m->jlp_accel > 0 || ef)
    {
        if (!append_cfg_var( &vars,
                             cons_cfg_var_dec( "jlp_accel", m->jlp_accel )))
            goto fail;
    }

    if (m->jlp_flash > 0 || ef)
    {
        if (!append_cfg_var( &vars,
                             cons_cfg_var_dec( "jlp_flash", m->jlp_flash )))
            goto fail;
    }

    if ( m->name &&
         !append_cfg_var( &vars, cons_cfg_var_string( "name", m->name )))
        goto fail;

    if ( m->short_name &&
         !append_cfg_var( &vars, cons_cfg_var_string( "short_name",
                                                      m->short_name )))
        goto fail;
                            
    if (append_cfg_vars(&vars, "author",       m->authors        )) goto fail;
    if (append_cfg_vars(&vars, "game_art_by",  m->game_artists   )) goto fail;
    if (append_cfg_vars(&vars, "music_by",     m->composers      )) goto fail;
    if (append_cfg_vars(&vars, "sfx_by",       m->sfx_artists    )) goto fail;
    if (append_cfg_vars(&vars, "voices_by",    m->voice_actors   )) goto fail;
    if (append_cfg_vars(&vars, "docs_by",      m->doc_writers    )) goto fail;
    if (append_cfg_vars(&vars, "concept_by",   m->conceptualizers)) goto fail;
    if (append_cfg_vars(&vars, "box_art_by",   m->box_artists    )) goto fail;
    if (append_cfg_vars(&vars, "more_info_at", m->more_infos     )) goto fail;
    if (append_cfg_vars(&vars, "publisher",    m->publishers     )) goto fail;

    if (m->release_dates)
    {
        int i;
        for (i = 0; m->release_dates[i].year; ++i)
        {
            if (!append_cfg_var(&vars,
                                cons_cfg_var_date("release_date",
                                                  &(m->release_dates[i]))))
                goto fail;
        }
    }

    if (m->build_dates)
    {
        int i;
        for (i = 0; m->build_dates[i].year; ++i)
        {
            if (!append_cfg_var(&vars,
                                cons_cfg_var_date("build_date",
                                                  &(m->build_dates[i]))))
                goto fail;
        }
    }

    if (append_cfg_vars(&vars, "license",      m->licenses       )) goto fail;
    if (append_cfg_vars(&vars, "description",  m->descriptions   )) goto fail;
    if (append_cfg_vars(&vars, "version",      m->versions       )) goto fail;

    if (m->misc)
    {
        int i;
        for (i = 0; m->misc[i]; ++i)
        {
            char *kv = strdup(m->misc[i]);
            char *v;
            if (!kv) goto fail;

            v = strchr(kv, '=');
            if (!v)
            {
                free(kv);
                continue;
            }

            *v++ = 0;
            if (!append_cfg_var(&vars, cons_cfg_var_string(kv, v)))
            {
                free(kv);
                goto fail;
            }

            free(kv);
        }
    }

    return vars;

fail:
    free_cfg_var_list( vars );
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
/*                 Copyright (c) 2016-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
