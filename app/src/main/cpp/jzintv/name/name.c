#include "config.h"
#include "metadata/metadata.h"
#include "name.h"

int find_cart_metadata(uint32_t crc32, game_metadata_t *meta)
{
    int i, found = 0;

    for (i = 0; name_list[i].name; i++)
    {
        if (name_list[i].crc32 == crc32)
        {
            found = 1;
            break;
        }
    }

    if (found && meta)
    {
        if (!meta->name)
            meta->name = strdup(name_list[i].name);

        if (!meta->short_name)
            meta->short_name = strdup(name_list[i].name);

        if (meta->is_defaults || meta->ecs_compat == CMP_UNSPECIFIED)
            meta->ecs_compat = name_list[i].ecs ? CMP_REQUIRES
                                                : CMP_UNSPECIFIED;

        if (meta->is_defaults || meta->voice_compat == CMP_UNSPECIFIED)
            meta->voice_compat = name_list[i].ivc ? CMP_REQUIRES
                                                  : CMP_UNSPECIFIED;

        /* Only bother adding a release date if metadata has none. */
        if (!meta->release_dates)
        {
            game_date_t *dates  = CALLOC(game_date_t, 2);
            dates[0].year       = name_list[i].year;
            meta->release_dates = dates;
        }
    }

    return found;
}

