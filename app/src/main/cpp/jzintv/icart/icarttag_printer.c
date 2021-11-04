/* ======================================================================== */
/*  INTELLICART ROM MetaData Tag routines.              J. Zbiciak, 2017    */
/* ======================================================================== */
#include "config.h"
#include "misc/types.h"
#include "icart/icarttag.h"

/* ======================================================================== */
/*  An ICARTTAG printing visitor, for debug purposes.                       */
/* ======================================================================== */
LOCAL int ictp_start(void *opaque)
{
    UNUSED(opaque);
    printf(">> ---------------------------------------------------- >>\n");
    return 0;
}

LOCAL int ictp_stop(void *opaque)
{
    UNUSED(opaque);
    printf("<< ---------------------------------------------------- <<\n");
    return 0;
}

LOCAL int ictp_ascii(void *opaque, const char *value, const char *key,
                     int indent)
{
    UNUSED(opaque);
    printf("%*s%-20s : %s\n", indent, "", key, cfg_escape_str(value));
    return 0;
}

LOCAL int ictp_full_title(void *opaque, const ict_title_t *title)
{
    return ictp_ascii(opaque, title->name, "Title", 4);
}

LOCAL int ictp_publisher(void *opaque, const ict_publisher_t *publisher)
{
    return ictp_ascii(opaque, publisher->name, "Publisher", 4);
}

static const char *credit_names[8] =
{
    "Programming",
    "Game Artwork",
    "Music",
    "Sound Effects",
    "Voice Acting",
    "Documentation",
    "Game Concept / Design",
    "Package Artwork"
};

LOCAL int ictp_credits(void *opaque, const ict_credits_t *credits)
{
    int i, j;
    printf("    Credits:\n");
    for (j = 0; j < 8; j++)
    {
        const char *cn = credit_names[j];
        for (i = 0; credits[i].name; ++i)
        {
            if (((credits[i].credbits.u.raw[0] >> j) & 1) == 0) continue;
            ictp_ascii(opaque, credits[i].name, cn, 8);
        }
    }
    return 0;
}

LOCAL int ictp_infourl(void *opaque, const ict_infourl_t *info)
{
    return ictp_ascii(opaque, info->url, "More Info At", 4);
}

LOCAL int ictp_release_date(void *opaque, const game_date_t *date)
{
    char *date_str = game_date_to_string(date);
    ictp_ascii(opaque, date_str ? date_str : "???", "Release Date", 4);
    free(date_str);
    return 0;
}

LOCAL int ictp_build_date(void *opaque, const game_date_t *date)
{
    char *date_str = game_date_to_string(date);
    ictp_ascii(opaque, date_str ? date_str : "???", "Build Date", 4);
    free(date_str);
    return 0;
}

static const char *compat_names[4] =
{
    "DONTCARE",
    "SUPPORTS",
    "REQUIRES",
    "INCOMPATIBLE"
};

static const char *jlp_names[4] =
{
    "DISABLED",
    "ACCEL ON; -FLASH",
    "ACCEL OFF; +FLASH",
    "ACCEL ON; +FLASH"
};

LOCAL int ictp_compat(void *opaque, const ict_compat_t *compat)
{
    printf("    Compatibility / Config:\n");

#   define COMPAT(f,n) \
        if (compat->u.s.f != ICT_COMPAT_DONTCARE)   \
            ictp_ascii(opaque, compat_names[compat->u.s.f], n, 8)

    COMPAT(keyboard_component,  "Keyboard Component");
    COMPAT(intellivoice,        "Intellivoice");
    COMPAT(rsvd_0_4,            "RSVD[0][5:4]");
    COMPAT(ecs,                 "ECS");
    COMPAT(intellivision_2,     "Intellivision II");
    COMPAT(tutorvision,         "TutorVision");
    COMPAT(rsvd_1_4,            "RSVD[1][5:4]");
    COMPAT(rsvd_1_6,            "RSVD[1][7:6]");

    if (compat->u.s.rsvd_byte)
    {
        char buf[32];
        sprintf(buf, "%02X", compat->u.s.rsvd_byte);
        ictp_ascii(opaque, buf, "RSVD[2][7:0]", 8);
    }

    if (compat->u.s.lto_mapper)
        ictp_ascii(opaque, "ENABLED", "LTO Mapper", 8);

    COMPAT(rsvd_3_2,            "RSVD[3][3:2]");

    if (compat->u.s.jlp_accel)
        ictp_ascii(opaque, jlp_names[compat->u.s.jlp_accel], "JLP Mode", 8);

    if (compat->u.s.jlp_accel >= 2)
    {
        char buf[32];
        int jlp_flash = (compat->u.s.jlp_flash_hi << 8) | 
                        (compat->u.s.jlp_flash_lo);

        sprintf(buf, "%d sectors", jlp_flash);
        ictp_ascii(opaque, buf, "JLP Flash", 8);
    }

    return 0;
#   undef COMPAT
}

LOCAL int ictp_short_title(void *opaque, const ict_title_t *title)
{
    return ictp_ascii(opaque, title->name, "Shortened Title", 4);
}

LOCAL int ictp_license(void *opaque, const ict_license_t *lic)
{
    return ictp_ascii(opaque, lic->license, "License", 4);
}

LOCAL int ictp_desc(void *opaque, const ict_desc_t *desc)
{
    return ictp_ascii(opaque, desc->text, "Description", 4);
}

LOCAL int ictp_version(void *opaque, const ict_version_t *ver)
{
    return ictp_ascii(opaque, ver->text, "Version", 4);
}

const icarttag_visitor_t ict_printer =
{
    NULL,
    ictp_start,
    ictp_stop,
    ictp_full_title,
    ictp_publisher,
    ictp_credits,
    ictp_infourl,
    ictp_release_date,
    ictp_compat,
    ictp_short_title,
    ictp_license,
    ictp_desc,
    ictp_build_date,
    ictp_version
};

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
