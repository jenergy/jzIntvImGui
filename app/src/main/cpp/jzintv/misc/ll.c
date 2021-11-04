/* ======================================================================== */
/*  LL   -- Singly linked list routines.                                    */
/*                                                                          */
/*  LL_INSERT     -- Inserts an element at the head of a linked list.       */
/*  LL_REVERSE    -- Reverses a LL_T linked list.                           */
/*  LL_CONCAT     -- Concatenate one list onto another.                     */
/*  LL_ACTON      -- Performs an action on each element of a linked list.   */
/*  LL_FREE       -- Frees a linked list.                                   */
/* ======================================================================== */

#include "config.h"
#include "misc/ll.h"

/* ======================================================================== */
/*  LL_INSERT     -- Inserts an element at the head of a linked list.       */
/* ======================================================================== */
ll_t *ll_insert_
(
    ll_t *const RESTRICT head,
    ll_t *const RESTRICT elem
)
{
    if (!elem)
        return head;

    elem->next = head;
    return elem;
}


/* ======================================================================== */
/*  LL_REVERSE    -- Reverses a LLIST_T linked list.                        */
/* ======================================================================== */
ll_t *ll_reverse_
(
    ll_t *RESTRICT head
)
{
    ll_t *p, *q, *r;

    p = NULL;
    q = head;

    if (!q || !q->next)
        return head;

    while (q)
    {
        r       = q->next;
        q->next = p;
        p       = q;
        q       = r;
    }

    return p;
}

/* ======================================================================== */
/*  LL_CONCAT     -- Concatenate one list onto another.                     */
/* ======================================================================== */
ll_t *ll_concat_
(
    ll_t *RESTRICT head,
    ll_t *RESTRICT const     list
)
{
    ll_t *p, **pp;

    p  = head;
    pp = &p;

    while (*pp)
        pp = &((*pp)->next);

    *pp = list; /* this may or may not modify "p" */

    return p;   /* return new head of list.  It will change if it was NULL */
}

/* ======================================================================== */
/*  LL_ACTON      -- Performs an action on each element of a linked list.   */
/*                   It is explicitly safe to free the list node in the     */
/*                   action, provided the caller to LL_ACTON agrees.        */
/* ======================================================================== */
void  ll_acton_(ll_t *RESTRICT list, void (act)(ll_t *, void *), void *opq)
{
    ll_t *prev;

    while (list)
    {
        prev = list;
        list = list->next;
        act(prev, opq);
    }
}

/* ======================================================================== */
/*  LL_LENGTH     -- Returns the length of a linked list                    */
/* ======================================================================== */
int   ll_length_(ll_t *list)
{
    int length = 0;

    while (list)
    {
        length++;
        list = list->next;
    }

    return length;
}

/* ======================================================================== */
/*  LL_FREE       -- Frees a linked list.                                   */
/* ======================================================================== */
void  ll_free_(ll_t *list)
{
    ll_t *prev;

    while (list)
    {
        prev = list;
        list = list->next;
        free(prev);
    }
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
/*                 Copyright (c) 2003-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
