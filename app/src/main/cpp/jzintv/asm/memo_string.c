/* ======================================================================== */
/*  String Memoization                                                      */
/*                                                                          */
/*  Rather than strdup'ing strings all over the place because we don't      */
/*  know where they'll be needed or when they'll be freed.  We can free     */
/*  the whole lot at the end if it matters.                                 */
/*                                                                          */
/*  Naturally, it only works for read-only strings.  It does allocate its   */
/*  own private copy.                                                       */
/* ======================================================================== */

#include "config.h"
#include "memo_string.h"

#define ARENA_SIZE (1 << 19)

typedef struct string_arena string_arena;

struct string_arena
{
    string_arena *next;
    char         *arena;
    size_t       remain;
};

string_arena *arenas = NULL;

#define HASH_SIZE (65535)
#define HASH_MULT (1023)

typedef struct hash_ent hash_ent;

struct hash_ent
{
    hash_ent *next;
    char     *string;
};

hash_ent *hash_table[HASH_SIZE];

static unsigned hash_string(const char *string)
{
    unsigned int hash = 0;

    while (*string)
    {
        hash *= HASH_MULT;
        hash += *string++;
    }

    return hash % HASH_SIZE;
}

static string_arena *get_arena(size_t len)
{
    string_arena *arena;

    if (len > ARENA_SIZE)
    {
        arena         = (string_arena *)malloc(sizeof(string_arena));
        arena->arena  = (char *)malloc(len);
        arena->remain = len;

        if (!arenas)
        {
            arenas = arena;
            arena->next   = 0;
        } else
        {
            arena->next  = arenas->next;
            arenas->next = arena;
        }
        return arena;
    }

    if (!arenas || len > arenas->remain)
    {
        arena         = (string_arena *)malloc(sizeof(string_arena));
        arena->arena  = (char *)malloc(ARENA_SIZE);
        assert(arena->arena);
        arena->remain = ARENA_SIZE;
        arena->next   = arenas;
        arenas        = arena;
        return arena;
    }

    return arenas;
}

char *arena_dup(const char *string)
{
    size_t          len = strlen(string) + 1;
    string_arena *arena = get_arena(len);
    char          *dest = arena->arena + arena->remain - len;

    memcpy(dest, string, len);
    arena->remain -= len;

    return dest;
}

const char *memoize_string(const char *string)
{
    unsigned int hash = hash_string(string);
    hash_ent **curr = &hash_table[hash];

    while (*curr)
    {
        if (!strcmp((*curr)->string, string))
            return (*curr)->string;

        curr = &((*curr)->next);
    }

    *curr           = (hash_ent *)malloc(sizeof(hash_ent));
    (*curr)->string = arena_dup(string);
    (*curr)->next   = NULL;

    return (*curr)->string;
}

void reset_memoize_string(void)
{
    unsigned int h;

    for (h = 0; h != HASH_SIZE; h++)
    {
        hash_ent *prev, *curr;

        curr = hash_table[h];

        while (curr)
        {
            prev = curr;
            curr = curr->next;
            free(prev);
        }

        hash_table[h] = NULL;
    }

    {
        string_arena *prev, *curr;

        curr = arenas;

        while (curr)
        {
            prev = curr;
            curr = curr->next;
            free(prev->arena);
            free(prev);
        }
    }
}
