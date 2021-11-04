#include "config.h"
#include "metadata/metadata.h"
#include "misc/jzprint.h"

/* ------------------------------------------------------------------------ */
/*  Helpers for print_metadata.  These are all 'best effort.'               */
/*  PRINT_STR_IF_NONNULL -- Prints string if not NULL.                      */
/*  PRINT_STR_ARRAY      -- Prints all strings in array if non-empty.       */
/*  PRINT_DATE_ARRAY     -- Prints all dates in array if non-empty.         */
/*  PRINT_COMPAT         -- Prints compatibility level.                     */
/*  PRINT_VAR_ARRAY      -- Prints all x=y values, splitting on '='.        */
/* ------------------------------------------------------------------------ */
LOCAL void print_str_if_nonnull(const char *const tag, const char *const value)
{
    if (!value) return;
    jzp_printf("%-24s %s\n", tag, value);
}

LOCAL void print_str_array(const char *const tag, const char **array)
{
    int i;

    if (!array) return;

    for (i = 0; array[i]; i++)
        print_str_if_nonnull(tag, array[i]);
}

LOCAL void print_date_array(const char *const tag, const game_date_t *dates)
{
    int i;

    if (!dates)
    {
        //jzp_printf("%s array empty\n", tag);
        return;
    }

    for (i = 0; dates[i].year; i++)
    {
        char *date_str = game_date_to_string(dates + i);
        print_str_if_nonnull(tag, date_str);
        CONDFREE(date_str);
    }
}

LOCAL void print_compat(const char *const tag, compat_level_t c)
{
    switch (c)
    {
        case CMP_INCOMPATIBLE:
            print_str_if_nonnull(tag, "Incompatible");
            break;
        case CMP_TOLERATES:
            print_str_if_nonnull(tag, "Tolerates");
            break;
        case CMP_ENHANCED:
            print_str_if_nonnull(tag, "Enhanced by");
            break;
        case CMP_REQUIRES:
            print_str_if_nonnull(tag, "Requires");
            break;
        case CMP_UNSPECIFIED:
        default:
            break;
    }
}

LOCAL void print_jlp_support(const game_metadata_t *const meta)
{
    char buf[32];

    if (meta->jlp_accel == JLP_UNSPECIFIED) return;

    jzp_printf("JLP Support:\n");

    switch (meta->jlp_accel)
    {
        case JLP_DISABLED:
            print_str_if_nonnull("  JLP Accel", "Disabled");
            break;
        case JLP_ACCEL_ON:
            print_str_if_nonnull("  JLP Accel", "Accel+RAM On; No Flash");
            break;
        case JLP_ACCEL_OFF:
            print_str_if_nonnull("  JLP Accel", 
                                 meta->jlp_flash ? "Accel+RAM Off"
                                                 : "Accel+RAM Off; No flash");
            break;
        case JLP_ACCEL_FLASH_ON:
            print_str_if_nonnull("  JLP Accel", 
                                 meta->jlp_flash ? "Accel+RAM On"
                                                 : "Accel+RAM On; No flash");
            break;
        case JLP_UNSPECIFIED:
        default:
            break;
    }

    snprintf(buf, sizeof(buf), "%d", meta->jlp_flash);
    print_str_if_nonnull("  JLP Flash Sectors", buf);
}

LOCAL void print_var_array(const char **meta)
{
    int i;

    for (i = 0; meta[i]; i++)
    {
        char buf[256];  /* Truncate vars longer than this. */
        char *dst = buf, *const end = buf + sizeof(buf) - 1;
        const char *src = meta[i];

        *dst++ = ' ';
        *dst++ = ' ';

        while (dst != end && *src != '=')
            *dst++ = *src++;

        *dst++ = 0;

        /* Handle the case where var name was too long for buf */
        while (*src && *src != '=')
            src++;

        if (*src == '=')
            print_str_if_nonnull(buf, src + 1);
    }
}

/* ------------------------------------------------------------------------ */
/*  PRINT_METADATA   -- Print game metadata.                                */
/* ------------------------------------------------------------------------ */
void print_metadata
(
    const game_metadata_t *const meta
)
{
    if (!meta)
        return;

    print_str_if_nonnull("Name",            meta->name);
    print_str_if_nonnull("Short Name",      meta->short_name);
    print_str_array     ("Author",          meta->authors);
    print_str_array     ("Game Art by",     meta->game_artists);
    print_str_array     ("Music by",        meta->composers);
    print_str_array     ("SFX by",          meta->sfx_artists);
    print_str_array     ("Voices by",       meta->voice_actors);
    print_str_array     ("Docs by",         meta->doc_writers);
    print_str_array     ("Concept by",      meta->conceptualizers);
    print_str_array     ("Box Art by",      meta->box_artists);
    print_str_array     ("More Info at",    meta->more_infos);
    print_str_array     ("Publisher",       meta->publishers);
    print_str_array     ("License",         meta->licenses);
    print_str_array     ("Description",     meta->descriptions);
    print_date_array    ("Build date",      meta->build_dates);
    print_date_array    ("Release date",    meta->release_dates);

    if (!meta->is_defaults)
    {
        jzp_printf("Compatibility:\n");
        print_compat("  ECS",                 meta->ecs_compat);
        print_compat("  Intellivoice",        meta->voice_compat);
        print_compat("  Intellivision II",    meta->intv2_compat);
        print_compat("  Keyboard Component",  meta->kc_compat);
        print_compat("  TutorVision",         meta->tv_compat);
    
        print_jlp_support(meta); 

        if (meta->lto_mapper)
            jzp_printf("\nLTO Mapper: %d\n\n", meta->lto_mapper);
    }
    if (meta->misc && *meta->misc)
    {
        jzp_printf("Additional metadata:\n");
        print_var_array(meta->misc);
    }
}
