/*
 * ============================================================================
 *  Title:    Generic Game Loading Logic
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This code attempts to detect the type of the program file being loaded,
 *  returning the binary image of the file and a type indicator if the
 *  detection is successful.
 *
 *  It is also responsible for searching the game search path and for trying
 *  different extensions (.ROM, .BIN, .INT) on the provided filename.
 *  In a sense, this is similar to what DOS's COMMAND.COM does for a command
 *  name, searching for "FOO.COM", "FOO.EXE" and "FOO.BAT" along all the
 *  directories in the search path in response to the command "FOO."
 * ============================================================================
 *
 * ============================================================================
 */


#include "config.h"
#include "icart/icart_rom.h"
#include "event/event.h"
#include "file/file.h"
#include "cfg.h"


/* ======================================================================== */
/*  GEN_SEARCHLIST -- Generate the list of names to search for for a game.  */
/*                    Right now this just adds extensions.  In the future   */
/*                    it will chase down a search path.                     */
/* ======================================================================== */
LOCAL char **gen_searchlist(char *name)
{
    char **list;
    int len, i;

    /* -------------------------------------------------------------------- */
    /*  Allocate space for the search list.   For each name, we allocate    */
    /*  enough space for "name.ext" plus trailing NUL.                      */
    /* -------------------------------------------------------------------- */
    list = calloc(sizeof(char *), 8);
    if (!list)
        return NULL;

    len = strlen(name);

    list[0] = calloc(len + 5, 8);

    for (i = 1; i < 7; i++)
        list[i] = list[i - 1] + len;

    list[7] = NULL;

    /* -------------------------------------------------------------------- */
    /*  Ok, generate the names.                                             */
    /* -------------------------------------------------------------------- */
    snprintf(list[0], len + 5, "%s"    , name);
    snprintf(list[1], len + 5, "%s.rom", name);
    snprintf(list[2], len + 5, "%s.ROM", name);
    snprintf(list[3], len + 5, "%s.bin", name);
    snprintf(list[4], len + 5, "%s.BIN", name);
    snprintf(list[5], len + 5, "%s.int", name);  /* eew. */
    snprintf(list[6], len + 5, "%s.INT", name);  /* double-eew. */

    return list;
}


/* ======================================================================== */
/*  GEN_CFGLIST  -- Generate a list of plausible .CFG filenames.            */
/* ======================================================================== */
LOCAL char **gen_cfglist(char *bin_name)
{
    char **list, *temp;
    int len, cnt;

    /* -------------------------------------------------------------------- */
    /*  First, strip off any extensions that we recognize.                  */
    /* -------------------------------------------------------------------- */
    cnt = 2;

    if (stricmp(name + len - 4, ".bin") == 0 ||
        stricmp(name + len - 4, ".int") == 0 ||
        stricmp(name + len - 4, ".rom") == 0)
    {
        temp = strdup(bin_name);
        if (temp)
        {
            temp[len - 4] = 0;
            cnt = 4;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Allocate space for the search list.   For each name, we allocate    */
    /*  enough space for "name.ext" plus trailing NUL.                      */
    /* -------------------------------------------------------------------- */
    list = calloc(sizeof(char *), cnt + 1);
    if (!list)
        return NULL;

    len = strlen(name);

    list[0] = calloc(len + 5, cnt + 1);

    for (i = 1; i < cnt; i++)
        list[i] = list[i - 1] + len;

    list[cnt] = NULL;

    /* -------------------------------------------------------------------- */
    /*  Ok, generate the names.                                             */
    /* -------------------------------------------------------------------- */
    if (temp)
    {
        snprintf(list[0], len+5, "%s.cfg", temp);
        snprintf(list[1], len+5, "%s.CFG", temp);
        snprintf(list[2], len+5, "%s.cfg", name);
        snprintf(list[3], len+5, "%s.CFG", name);
        free(temp);
    } else
    {
        snprintf(list[0], len+5, "%s.cfg", name);
        snprintf(list[1], len+5, "%s.CFG", name);
    }

    return list;
}


/* ======================================================================== */
/*  READ_GAME                                                               */
/* ======================================================================== */
uint8_t *read_game
(
    const char *name,
    int        *type,
    char      **bin_name,
    char      **cfg_name
)
{
    uint8_t *img;
    char   **list, *found;
    int      i, err;
    FILE    *f;
    size_t   flen;

    /* -------------------------------------------------------------------- */
    /*  Sanity checks.                                                      */
    /* -------------------------------------------------------------------- */
    if (!name || !type)
        return NULL;

    /* -------------------------------------------------------------------- */
    /*  First, search for the file.                                         */
    /* -------------------------------------------------------------------- */
    if (!(list = gen_searchlist(name, &cnt)))
        return NULL;


    found = NULL;
    for (i = 0; list[i] != NULL; i++)
    {
        if (!found && file_exists(list[i]))
            found = strdup(list[i]);
        free(list[i]);
    }
    free(list);

    if (!found)
        return NULL;

    /* -------------------------------------------------------------------- */
    /*  Now that we have a filename, let's read in the file.                */
    /* -------------------------------------------------------------------- */
    if (!(f = fopen(found, "rb")))
        return NULL;

    fseek(f, 0, SEEK_END);
    flen = ftell(f);
    rewind(f);

    if (!(img = malloc(flen)))
    {
        fclose(f);
        return NULL;
    }

    fread(img, 1, flen, f);
    fclose(f);

    /* -------------------------------------------------------------------- */
    /*  Ok, first try to decode it as a .ROM file.  If that succeeds, stop. */
    /* -------------------------------------------------------------------- */
    err = icartrom_decode(NULL, img, flen, 0, 0);

    if (err > 0 && err <= flen)
    {
        *bin_name = found;
        *cfg_name = NULL;
        *type     = GAMETYPE_ROM;
        return img;
    }

    /* -------------------------------------------------------------------- */
    /*  It wasn't a .ROM.  Maybe it's a .BIN.  That can only be the case,   */
    /*  though, if the filesize is even.                                    */
    /* -------------------------------------------------------------------- */
    if (flen % 2 != 0)
    {
        *bin_name = NULL;
        *cfg_name = NULL;
        *type = GAMETYPE_BAD;
        free(img);
        free(found);
        return NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  We ran the gauntlet and survived, so assume it's .BIN.  Now look    */
    /*  for a .CFG file to go with it.                                      */
    /* -------------------------------------------------------------------- */
    *type     = GAMETYPE_BIN;
    *bin_name = found;
    *cfg_name = NULL;

    if (!(list = gen_cfglist(found)))
        return img;

    found = NULL;
    for (i = 0; list[i]; i++)
    {
        if (!found && file_exists(list[i]))
            found = strdup(list[i]);
        free(list[i]);
    }
    free(list);

    *cfg_name = found;

    return img;
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
/*                 Copyright (c) 1998-2004, Joseph Zbiciak                  */
/* ======================================================================== */
