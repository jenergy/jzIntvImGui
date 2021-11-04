/* ======================================================================== */
/*  SERIALIZER                                                              */
/*                                                                          */
/*  Writes out the current state of jzIntv as a bunch of key/value pairs.   */
/*  Key/value pair lists are hung off a hierarchy of namespaces to allow    */
/*  short, simple names to be used for keys and to simplify disambiguating  */
/*  multiple instances of a given object (e.g. two PSGs, etc.).             */
/* ======================================================================== */

#include "config.h"

#ifdef NO_SERIALIZER

int no_serializer;

#else

#include "serializer/serializer.h"

static ser_hier_t *ser_hier = NULL;

/* ======================================================================== */
/*  SER_REGISTER:  Register key/value pair that will be serialized.         */
/* ======================================================================== */
void ser_register
(
    ser_hier_t *hier,       /*  What namespace to put this under.           */
    const char *name,       /*  What name to give to the object.            */
    void       *object,     /*  Pointer to the object itself.               */
    ser_type_t  type,       /*  8/16/32 bit, scalar, string, array,etc.     */
    int         length,     /*  Length of object if an array.               */
    uint32_t    flags       /*  mandatory/optional/informative/init         */
)
{
    ser_list_t *new_rec, *node, *prev;

    /* -------------------------------------------------------------------- */
    /*  First scan the list to make sure we have no dupes at this level.    */
    /*  If we find a dupe, error-out for now.  It's a programming error.    */
    /* -------------------------------------------------------------------- */
    prev = NULL;
    node = hier->obj_list;

    while (node)
    {
        if (!strcmp(node->name, name))
        {
            fprintf(stderr, "Error: Duplicate key '%s' in serializer\n", name);
            exit(1);
        }
        prev = node;
        node = node->next;
    }

    /* -------------------------------------------------------------------- */
    /*  Now append the new name.                                            */
    /* -------------------------------------------------------------------- */
    new_rec = CALLOC(ser_list_t, 1);

    new_rec->name   = strdup(name);
    new_rec->object = object;
    new_rec->type   = type;
    new_rec->length = length;
    new_rec->flags  = flags;
    new_rec->next   = NULL;

    if (prev) prev->next     = new_rec;
    else      hier->obj_list = new_rec;

    /* -------------------------------------------------------------------- */
    /*  If this key has its INIT flag set, set it in the hierarchies that   */
    /*  contain it.  If it does not have its INIT flag set, set the NONINIT */
    /*  flag in the hierarchy.                                              */
    /* -------------------------------------------------------------------- */
    flags &= SER_INIT;
    if (!flags)
        flags = SER_NONINIT;

    while (hier)
    {
        hier->flags |= flags;
        hier = hier->parent;
    }

    return;
}

/* ======================================================================== */
/*  SER_NEW_HIERARCHY:  Add a new namespace to the hierarchy.               */
/* ======================================================================== */
ser_hier_t *ser_new_hierarchy
(
    ser_hier_t  *parent_hier,
    const char  *name
)
{
    ser_hier_t *new_hier, *node, *prev;

    /* -------------------------------------------------------------------- */
    /*  First scan the list to make sure we have no dupes at this level.    */
    /*  If we find a dupe, error-out for now.  It's a programming error.    */
    /* -------------------------------------------------------------------- */
    prev = NULL;
    node = parent_hier ? parent_hier->hier_list : ser_hier;

    while (node)
    {
        if (!strcmp(node->name, name))
        {
            fprintf(stderr, "Error: Duplicate hier '%s' in serializer\n", name);
            exit(1);
        }
        prev = node;
        node = node->next;
    }

    /* -------------------------------------------------------------------- */
    /*  Now create and append the new hierarchy.                            */
    /* -------------------------------------------------------------------- */
    new_hier = CALLOC(ser_hier_t, 1);

    new_hier->name   = strdup(name);
    new_hier->parent = parent_hier;
    new_hier->next   = NULL;
    new_hier->flags  = 0;

    if (prev)               prev->next             = new_hier;
    else if (parent_hier)   parent_hier->hier_list = new_hier;
    else                    ser_hier               = new_hier;

    return new_hier;
}

/* ======================================================================== */
/*  SER_GET_INT                                                             */
/* ======================================================================== */
uint64_t ser_get_int(void *object, ser_type_t type, void **next)
{
    uint64_t value;
    union
    {
        uint8_t  *pu8;   int8_t  *ps8;
        uint16_t *pu16;  int16_t *ps16;
        uint32_t *pu32;  int32_t *ps32;
        uint64_t *pu64;  int64_t *ps64;
        void    *v;
    } ptr;

    ptr.v = object;

    switch (type)
    {
        case ser_u8 : value = *ptr.pu8 ++; break;
        case ser_u16: value = *ptr.pu16++; break;
        case ser_u32: value = *ptr.pu32++; break;
        case ser_u64: value = *ptr.pu64++; break;
        case ser_s8 : value = *ptr.ps8 ++; break;
        case ser_s16: value = *ptr.ps16++; break;
        case ser_s32: value = *ptr.ps32++; break;
        case ser_s64: value = *ptr.ps64++; break;
        default:      fprintf(stderr, "ser_get_int called w/ bad type %d\n",
                              (int)type);
                      exit(1);
    }

    if (next) *next = ptr.v;

    return value;
}

static char ser_int_buf[20];

/* ======================================================================== */
/*  SER_INT_TO_STR                                                          */
/* ======================================================================== */
char *ser_int_to_str(uint64_t value, ser_type_t type, uint32_t flags, int fix)
{
    const char *format;
    static char ffmt[10];

    if (flags & SER_HEX)
    {
        switch (type)
        {
            case ser_u8 : case ser_s8 :  format = "$%.2llX"; break;
            case ser_u16: case ser_s16:  format = "$%.4llX"; break;
            case ser_u32: case ser_s32:  format = "$%.8llX"; break;
            case ser_u64: case ser_s64:  format = "$%.16llX"; break;
            default: fprintf(stderr, "ser_int_to_str called w/ bad type %d\n",
                             (int)type);
                     exit(1);
        }
    } else if (fix)
    {
        format = ffmt;

        switch (type)
        {
            case ser_u8 : case ser_u16:
            case ser_u32: case ser_u64: snprintf(ffmt,9,"%%%dllu", fix); break;
            case ser_s8 : case ser_s16:
            case ser_s32: case ser_s64: snprintf(ffmt,9,"%%%dlld", fix); break;
            default: fprintf(stderr, "ser_int_to_str called w/ bad type %d\n",
                             (int)type);
                     exit(1);
        }
        ffmt[9] = 0;
    } else
    {
        switch (type)
        {
            case ser_u8 : case ser_u16:
            case ser_u32: case ser_u64: format = "%llu"; break;
            case ser_s8 : case ser_s16:
            case ser_s32: case ser_s64: format = "%lld"; break;
            default: fprintf(stderr, "ser_int_to_str called w/ bad type %d\n",
                             (int)type);
                     exit(1);
        }
    }

    snprintf(ser_int_buf, 19, format, value);
    ser_int_buf[19] = 0;

    return ser_int_buf;
}


/* ======================================================================== */
/*  SER_PRINT_ARRAY                                                         */
/* ======================================================================== */
void ser_print_array
(
    FILE        *f,
    ser_list_t  *obj,
    int         indent
)
{
    int      i, l, col;
    void    *p;
    uint64_t v;
    char    *s;

    fprintf(f, "%*s%s =\n%*s{\n%*s",
            indent, "", obj->name, indent, "", indent + 4, "");

    col = indent + 4;

    p = obj->object;
    for (i = 0; i < obj->length; i++)
    {
        v = ser_get_int   (p, obj->type, &p);
        s = ser_int_to_str(v, obj->type, obj->flags, 0);
        l = strlen(s);

        if (col + l > 80 && col > indent + 4)
        {
            fprintf(f, "\n%*s", indent + 4, "");
            col = indent + 4;
        } else if (col > indent + 4)
            fputc(' ', f);

        fputs(s, f);
        col += l + 2;

        if (i != obj->length - 1)
            fputc(',', f);
    }

    fprintf(f, "\n%*s}\n", indent, "");

    return;
}

/* ======================================================================== */
/*  SER_PRINT_INT                                                           */
/* ======================================================================== */
void ser_print_int
(
    FILE        *f,
    ser_list_t  *obj,
    int         indent
)
{
    fprintf(f, "%*s%s = %s;\n", indent, "", obj->name,
            ser_int_to_str(ser_get_int(obj->object, obj->type, NULL),
                           obj->type, obj->flags, 0));

    return;
}

/* ======================================================================== */
/*  SER_PRINT_STRING                                                        */
/* ======================================================================== */
void ser_print_string
(
    FILE        *f,
    ser_list_t  *obj,
    int         indent
)
{
    fprintf(f, "%*s%s = \"%s\";\n", indent, "", obj->name,
            obj->object ? (char *)obj->object : NULL);

    return;
}


/* ======================================================================== */
/*  SER_PRINT_HIERARCHY:  Do it!                                            */
/* ======================================================================== */
void ser_print_hierarchy(FILE *f, ser_hier_t *node, int init, int indent)
{
    ser_list_t *object;
    ser_hier_t *child;

    /* -------------------------------------------------------------------- */
    /*  Do this in two passes:  Catch all the initialization stuff first    */
    /*  and then capture all the rest.  This isn't necessary, but it's a    */
    /*  nicety.  This function's recursive.                                 */
    /* -------------------------------------------------------------------- */
    if (!node)
    {
        fprintf(f, "/* Initialization */\n");
        ser_print_hierarchy(f, ser_hier, 1, 0);

        fprintf(f, "\n/* Emulator state */\n");
        ser_print_hierarchy(f, ser_hier, 0, 0);

        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Do the actual printing.                                             */
    /* -------------------------------------------------------------------- */
    while (node)
    {
        int nl_flag = 1;

        if ((!node->obj_list && !node->hier_list) ||
           ( init && (node->flags & SER_INIT)    == 0) ||
           (!init && (node->flags & SER_NONINIT) == 0))
        {
            node = node->next;
            continue;
        }

        fprintf(f, "%*s[%s]\n%*s{\n", indent, "", node->name, indent, "");
        for (child = node->hier_list; child; child = child->next)
        {
            if ( init && (node->flags & SER_INIT)    == 0) continue;
            if (!init && (node->flags & SER_NONINIT) == 0) continue;

            if (nl_flag == 0)
                fprintf(f, "\n");
            nl_flag = 0;

            ser_print_hierarchy(f, child, init, indent + 4);
        }

        for (object = node->obj_list; object; object = object->next)
        {
            if ( init && (object->flags & SER_INIT) == 0) continue;
            if (!init && (object->flags & SER_INIT) != 0) continue;

            if (nl_flag == 0)
                fprintf(f, "\n");
            nl_flag = 1;

            if (object->type == ser_string)
                ser_print_string(f, object, indent + 4);
            else if (object->length > 1)
                ser_print_array(f, object, indent + 4);
            else
                ser_print_int(f, object, indent + 4);
        }

        fprintf(f, "%*s}\n", indent, "");

        node = node->next;
    }

    return;
}

#endif

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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
