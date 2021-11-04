#include "config.h"
#include "cheat/cheat.h"
#include "periph/periph.h"
#include "plat/plat_lib.h"
#include "cp1600/cp1600.h"

typedef enum { E_OPTIONAL = 0, E_REQUIRED } is_required_t;

LOCAL const char *cheat_parse_error = NULL;

/* ======================================================================== */
/*  CHEAT_PARSE_SKIP_WS  -- Returns pointer to next non-ws character.       */
/*                          Returns NULL if no whitespace was skipped,      */
/*                          and 'required' set.                             */
/* ======================================================================== */
LOCAL const char *cheat_parse_skip_ws(const char *s,
                                      const is_required_t is_required)
{
    if (is_required && (!*s || !isspace(*s)))
    {
        cheat_parse_error = "missing whitespace";
        return NULL;
    }
    while (*s && isspace(*s)) s++;
    return s;
}

/* ======================================================================== */
/*  CHEAT_PARSE_NEXT_CMD -- Skips whitespace and optional '|' between       */
/*                          commands.  Returns NULL if no characters        */
/*                          were skipped and a delimiter was required.      */
/* ======================================================================== */
LOCAL const char *cheat_parse_next_cmd
(
    const char *RESTRICT s,
    const is_required_t delimiter_is_required
)
{
    const char *os = s;

    s = cheat_parse_skip_ws(s, E_OPTIONAL);
    if (*s == '|') s++;
    s = cheat_parse_skip_ws(s, E_OPTIONAL);

    if (delimiter_is_required && s == os)
    {
        // If there's still more characters, then this is a parse error.
        // Otherwise, we're just at the end of string, and so no error.
        cheat_parse_error = *s ? "missing delimiter between commands" : NULL;
        return NULL;
    }

    return s;
}

/* ======================================================================== */
/*  CHEAT_PARSE_GET_HEX  -- Attempts to get a hex token and advance string  */
/*                          pointer.  Returns NULL on failure.              */
/* ======================================================================== */
LOCAL const char *cheat_parse_get_hex(const char *RESTRICT s,
                                      uint32_t *const RESTRICT val)
{
    if (!(s = cheat_parse_skip_ws(s, E_REQUIRED))) return NULL;
    if (*s == '$') s++;

    char *e = NULL;     // Because strtoul needs char**, not const char**.
    *val = strtoul(s, &e, 16);

    // If strtoul couldn't move us forward, there's a parse error.
    if (s == e)
    {
        cheat_parse_error = "unable to parse argument";
        return NULL;
    }

    return e;
}

/* ======================================================================== */
/*  CHEAT_PARSE_GET_CMD  -- Attempts to get a command character.            */
/* ======================================================================== */
LOCAL const char *cheat_parse_get_cmd
(
    const char *RESTRICT s,
    int        *const RESTRICT cmd,
    const is_required_t delimiter_is_required
)
{
    *cmd = 0;
    if (!(s = cheat_parse_next_cmd(s, delimiter_is_required))) return NULL;

    if (*s)
    {
        *cmd = toupper(*s);
        return s + 1;
    }

    // No actual error.  Just no more commands.
    cheat_parse_error = NULL;

    return NULL;
}

#define CHEAT_CMD_BUF_MIN (16)

/* ======================================================================== */
/*  CHEAT_PARSE  -- Parses a cheat command string and returns an array of   */
/*                  cheat_cmd_t to be hooked into a cheat_t.                */
/* ======================================================================== */
LOCAL cheat_cmd_t *cheat_parse(const int cheat_idx, const char *s)
{
    static cheat_cmd_t *cmd_buf = NULL;
    static int cmd_buf_size = 0;
    int cmd_cnt = 0;
    bool first = true;
    const char *last_s = s;

    if (!cmd_buf_size)
    {
        cmd_buf = CALLOC(cheat_cmd_t, CHEAT_CMD_BUF_MIN);
        cmd_buf_size = CHEAT_CMD_BUF_MIN;
    }

    memset(cmd_buf, 0, cmd_buf_size * sizeof(cheat_cmd_t));

    while (true)
    {
        int *cmd = &cmd_buf[cmd_cnt].cmd;
        uint32_t *arg = &cmd_buf[cmd_cnt].arg[0];

        cheat_parse_error = NULL;

        last_s = s;
        if (!(s = cheat_parse_get_cmd(s, cmd, first ? E_OPTIONAL : E_REQUIRED)))
        {
            if (cheat_parse_error)
                fprintf(stderr, "CHEAT%d:  Parse error: %s\n>> %s\n",
                        cheat_idx, cheat_parse_error, last_s);
            else if (first)
                fprintf(stderr, "CHEAT%d:  Empty cheat?\n>> %s\n",
                        cheat_idx, last_s);
            else
                break;  // Normal termination condition.

            return NULL;
        }
        first = false;

        last_s = s;
        if (!(s = cheat_parse_get_hex(s, &arg[0])))
        {
            fprintf(stderr, "CHEAT%d: Invalid 1st argument: %s\n>> %s\n",
                    cheat_idx, cheat_parse_error, last_s);
            return NULL;
        }

        last_s = s;
        if (!(s = cheat_parse_get_hex(s, &arg[1])))
        {
            fprintf(stderr,
                    "CHEAT%d: Invalid 2nd argument: %s\n>> %s\n", cheat_idx,
                    cheat_parse_error, last_s);
            return NULL;
        }

        cmd_cnt++;
        if (cmd_cnt == cmd_buf_size)
        {
            cmd_buf_size <<= 1;
            cmd_buf = REALLOC(cmd_buf, cheat_cmd_t, cmd_buf_size);
        }
    }

    cheat_cmd_t *result = CALLOC(cheat_cmd_t, cmd_cnt + 1);
    memcpy(result, cmd_buf, cmd_cnt * sizeof(cheat_cmd_t));

    return result;
}

/* ======================================================================== */
/*  CHEAT_ADD    -- Adds a cheat to cheat_t.                                */
/* ======================================================================== */
int cheat_add(cheat_t *const RESTRICT cheat, const char *const s)
{
    int cheat_idx = cheat_count(cheat);
    if (cheat_idx == NUM_CHEATS)
    {
        fprintf(stderr, "cheat: Too many cheats.  Max: %d\n", NUM_CHEATS);
        return -1;
    }

    cheat_cmd_t *cmds = cheat_parse(cheat_idx, s);

    if (!cmds)
    {
        fprintf(stderr, "cheat: Failed to add CHEAT%d\n", cheat_idx);
        return -1;
    }

    cheat->cheat[cheat_idx] = cmds;
    return 0;
}

/* ======================================================================== */
/*  CHEAT_EXEC   -- Executes a cheat code.                                  */
/* ======================================================================== */
LOCAL void cheat_exec(cheat_t *const RESTRICT cheat, const int idx)
{
    fprintf(stderr, "CHEAT%d invoked.\n", idx);
    for (const cheat_cmd_t *RESTRICT cmd = cheat->cheat[idx]; cmd->cmd; cmd++)
    {
        if (cmd->cmd != 'P' && cmd->cmd != 'E')
        {
            fprintf(stderr, "CHEAT%d: unknown command '%c %X %X'\n", idx,
                       cmd->cmd, cmd->arg[0], cmd->arg[1]);
            continue;
        }

        if ((cmd->arg[0] & 0xFFFF) != cmd->arg[0] ||
            (cmd->arg[1] & 0xFFFF) != cmd->arg[1])
        {
            fprintf(stderr,
                    "CHEAT%d: invalid arguments for command '%c %X %X'\n",
                    idx, cmd->cmd, cmd->arg[0], cmd->arg[1]);
            continue;
        }

        periph_wr_t *const put = cmd->cmd == 'P' ? periph_poke : periph_write;

        put(AS_PERIPH(cheat->periph.bus), AS_PERIPH(cheat->periph.bus),
            cmd->arg[0], cmd->arg[1]);
        cp1600_invalidate(cheat->cpu, cmd->arg[0], cmd->arg[1]);
    }
}

/* ======================================================================== */
/*  CHEAT_TICK   -- Handles any cheats that have been requested.            */
/* ======================================================================== */
LOCAL uint32_t cheat_tick(periph_t *const periph, const uint32_t len)
{
    cheat_t *const cheat = PERIPH_AS(cheat_t, periph);

    if (!cheat->request)
        return len;

    for (int i = 0; i < NUM_CHEATS; ++i)
    {
        uint32_t mask = 1u << i;

        if (!(cheat->request & mask))
            continue;

        cheat->request &= ~mask;

        if (!cheat->cheat[i])
        {
            fprintf(stderr, "CHEAT%d invoked, but not defined.\n", i);
            continue;
        }

        cheat_exec(cheat, i);
    }

    return len;
}
/* ======================================================================== */
/*  CHEAT_DTOR   -- Cleans up the cheat structure.                          */
/* ======================================================================== */
LOCAL void cheat_dtor(periph_t *const periph)
{
    cheat_t *const cheat = PERIPH_AS(cheat_t, periph);

    for (int i = 0; i < NUM_CHEATS; ++i)
        CONDFREE(cheat->cheat[i]);

    memset(cheat, 0, sizeof(cheat_t));
}

/* ======================================================================== */
/*  CHEAT_COUNT  -- Returns number of active cheats.                        */
/* ======================================================================== */
int cheat_count(const cheat_t *const cheat)
{
    int i;

    for (i = 0; i < NUM_CHEATS; i++)
        if (!cheat->cheat[i])
            break;

    return i;
}

/* ======================================================================== */
/*  CHEAT_INIT   -- Initializes the cheat peripheral if it isn't already.   */
/* ======================================================================== */
int cheat_init(cheat_t *const cheat, cp1600_t *const cpu)
{
    /* -------------------------------------------------------------------- */
    /*  The cheat peripheral doesn't have most peripheral behaviors, so it  */
    /*  can simply zero out most of the periph structure.                   */
    /* -------------------------------------------------------------------- */
    memset(&cheat->periph, 0, sizeof(periph_t));

    /* -------------------------------------------------------------------- */
    /*  The cheat peripheral doesn't need to be 'ticked' very often.        */
    /*  It just needs to be ticked every frame or so.                       */
    /* -------------------------------------------------------------------- */
    cheat->periph.tick      = cheat_tick;
    cheat->periph.min_tick  = 5000;
    cheat->periph.max_tick  = 20000;
    cheat->periph.dtor      = cheat_dtor;

    /* -------------------------------------------------------------------- */
    /*  The cheat peripheral needs a hook back to the CPU to invalidate     */
    /*  caches.  We might need it if we upgrade cheat with other actions.   */
    /* -------------------------------------------------------------------- */
    cheat->cpu              = cpu;

    return 0;
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
/*         Copyright (c) 2019-+Inf, Joseph Zbiciak, Patrick Nadeau          */
/* ======================================================================== */
