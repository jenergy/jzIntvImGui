
/* ======================================================================== */
/*  SERIALIZER                                                              */
/*                                                                          */
/*  Writes out the current state of jzIntv as a bunch of key/value pairs.   */
/*  Key/value pair lists are hung off a hierarchy of namespaces to allow    */
/*  short, simple names to be used for keys and to simplify disambiguating  */
/*  multiple instances of a given object (e.g. two PSGs, etc.).             */
/* ======================================================================== */

#ifndef SERIALIZE_H_
#define SERIALIZE_H_ 1

#ifdef NO_SERIALIZER

typedef void ser_hier_t;

#else

typedef enum ser_type_t
{
    ser_u8 , ser_s8 , ser_u16, ser_s16,
    ser_u32, ser_s32, ser_u64, ser_s64,
    ser_string
} ser_type_t;

typedef struct ser_list_t ser_list_t;
typedef struct ser_hier_t ser_hier_t;

struct ser_list_t
{
    struct ser_list_t *next;
    const char *name;
    void       *object;
    ser_type_t  type;
    int         length;
    uint32_t    flags;
};


struct ser_hier_t
{
    ser_hier_t  *next;
    ser_hier_t  *parent;
    ser_hier_t  *hier_list;
    const char  *name;
    ser_list_t  *obj_list;
    uint32_t     flags;
};

#define SER_INIT    (0x0001)
#define SER_NONINIT (0x0002)
#define SER_HEX     (0x0004)
#define SER_INFO    (0x0008)
#define SER_MAND    (0x0010)
#define SER_SEEN    (0x0020)


/* ======================================================================== */
/*  SER_REGISTER:  Register key/value pair that will be serialized.         */
/* ======================================================================== */
void ser_register
(
    ser_hier_t  *hier,      /*  What namespace to put this under.           */
    const char  *name,      /*  What name to give to the object.            */
    void        *object,    /*  Pointer to the object itself.               */
    ser_type_t  type,       /*  8/16/32 bit, scalar, string, array,etc.     */
    int         length,     /*  Length of object if an array.               */
    uint32_t    flags       /*  mandatory/optional/informative/init         */
);

/* ======================================================================== */
/*  SER_NEW_HIERARCHY:  Add a new namespace to the hierarchy.               */
/* ======================================================================== */
ser_hier_t *ser_new_hierarchy
(
    ser_hier_t  *parent_hier,
    const char  *name
);

/* ======================================================================== */
/*  SER_GET_INT                                                             */
/* ======================================================================== */
uint64_t ser_get_int(void *object, ser_type_t type, void **next);

/* ======================================================================== */
/*  SER_INT_TO_STR                                                          */
/* ======================================================================== */
char *ser_int_to_str(uint64_t value, ser_type_t type, uint32_t flags, int fix);


/* ======================================================================== */
/*  SER_PRINT_ARRAY                                                         */
/* ======================================================================== */
void ser_print_array
(
    FILE       *f,
    ser_list_t *obj,
    int         indent
);

/* ======================================================================== */
/*  SER_PRINT_INT                                                           */
/* ======================================================================== */
void ser_print_int
(
    FILE       *f,
    ser_list_t *obj,
    int         indent
);

/* ======================================================================== */
/*  SER_PRINT_STRING                                                        */
/* ======================================================================== */
void ser_print_string
(
    FILE       *f,
    ser_list_t *obj,
    int         indent
);

/* ======================================================================== */
/*  SER_PRINT_HIERARCHY:  Do it!                                            */
/* ======================================================================== */
void ser_print_hierarchy(FILE *f, ser_hier_t *node, int init, int indent);


#endif
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
