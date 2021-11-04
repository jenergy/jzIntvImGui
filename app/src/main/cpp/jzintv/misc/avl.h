
/* FILE: avl.h */
/* definitions for AVL tree routines... */
#ifndef AVL_H_
#define AVL_H_

/*
    Types defined: (defined in avl.h)

        TAVLTree  -- Struct containing root ptr, current compare func, etc.
        PAVLTree  -- Pointer to AVLTree structure

        TAVLNode  -- Struct containing the data record, ptrs to chldrn, and
                     the balance factor
        PAVLNode  -- Pointer to AVLNode structure
        PPAVLNode -- Pointer to pointer to AVLNode structure

        EAVLBal   -- Enum of balance factor values

        EAVLOrd   -- Enum of traversal orders

        EAVLErr   -- Enum of AVL routine error codes
        CAVLEmsg  -- Constant array of error messages corres. w/ EAVLErr

    Exported functions:

        int AVL_AddNode(PAVLTree tree, PRec rec)
                --> Adds a node containing 'rec' to tree pointed to by
                    '*root'.  Returns non-zero on error.

        int AVL_DelNode(PAVLTree tree, PRec rec)
                --> Searches tree '*tree' for key matching 'rec' and deletes
                    Returns non-zero on error

        int AVL_SearchTree(PAVLTree tree, PRec rec, PPAVLNode node)
                --> Searches tree '*tree' for node matching 'rec' by key
                    using the current key comparison function, and places
                    result in '*node'.  Returns non-zero on error.

        int AVL_SearchWholeTree
            (PAVLTree tree, PRec rec, PPAVLNode node, int (*comp)(PRec,PRec))
                --> Performs an inorder traversal on '*tree' until it
                    finds 'rec', using 'comp' as its comparison function.
                    If found, it places the node pointer in '*node'.
                    Returns non-zero on error.

        int AVL_InitTree(PAVLTree tree)
                --> Initializes the AVLTree struct 'tree'

        int AVL_Traverse(PAVLTree tree, EAVLOrd order, int (*act)(PRec))
                --> Traverses tree 'tree' in order 'order', performing
                    'act' on each record.  If 'act' returns non-zero,
                    that value is returned to the caller.  Otherwise
                    zero is returned.

        int AVL_KillTree(PAVLTree tree)
                --> Deallocates all records and nodes within '*tree'

        int AVL_KillJustTree(PAVLTree tree)
                --> Deallocates all nodes within '*tree', leaving the
                    records intact (that way, the AVL tree can be used
                    as a container for records that may be stored in
                    another structure as well.)

        int AVL_SetTreeComp(PAVLTree tree, int (*comp)(PRec, PRec))
                --> Sets the compare function for '*tree' to 'comp()'
                    'comp' should be defined as 'int comp(PRec,PRec);'

*/

typedef enum { LeftHigh, RightHigh, Balanced } EAVLBal;

typedef struct tAVLNode * PAVLNode;

typedef struct tAVLNode {
    PAVLNode    l;
    PAVLNode    r;
    EAVLBal     bal;
    void *      rec;  /* No AVL functions ever dereference rec. */
} TAVLNode;

typedef PAVLNode * PPAVLNode;

typedef struct tAVLTree {
    PAVLNode    root;
    int         (*comp)(void *,void *);
} TAVLTree;

typedef struct tAVLTree * PAVLTree;

typedef enum { InOrder, PreOrder, PostOrder } EAVLOrd;

typedef enum {
  EAVL_NOERR=0, EAVL_NULLTREE, EAVL_NOTFOUND, EAVL_DUPREC, EAVL_NOCOMP,
  EAVL_COMPCHG
} EAVLErr;

extern const char * CAVLEmsg[];   /* Error message array in avl.c */

typedef int (*PAVLCompFxn)(void *, void*);
typedef int (*PAVLActFxn) (void *);

int     AVL_AddNode     (PAVLTree tree, void * rec);
int     AVL_Traverse    (PAVLTree tree, EAVLOrd order, PAVLActFxn act);
int     AVL_SearchTree  (PAVLTree tree, void * rec, void ** node);
int     AVL_SearchWholeTree
    (PAVLTree tree, void * rec, PPAVLNode node, PAVLCompFxn comp);
int     AVL_KillTree    (PAVLTree tree);
int     AVL_KillJustTree
                        (PAVLTree tree);
int     AVL_SetTreeComp (PAVLTree tree, PAVLCompFxn comp);
int     AVL_InitTree    (PAVLTree tree);
int     AVL_DelNode     (PAVLTree tree, void * rec);
void    AVL_DumpTreeInfo(PAVLTree tree, PAVLActFxn act);

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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
