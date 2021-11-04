/* FILE: avl.c */
/* routines for handling AVL trees... */

#include <stdio.h>
#include <stdlib.h>
#include "avl.h"

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

    Types used: (defined elsewhere)

        TRec      -- Data record held in tree
        void *      -- Pointer to data record

        --Note:  TRec/void * are never actually used in these routines.
                 Instead, the void * is stored in a void *, and any
                 actions on the TRec is via routines whose addresses
                 are passed to the AVL routines.  Also, all function
                 and type decls that refer to void * actually refer to void *.

    Exported functions:

        int AVL_AddNode(PAVLTree tree, void * rec)
                --> Adds a node containing 'rec' to tree pointed to by
                    '*root'.  Returns non-zero on error.

        int AVL_DelNode(PAVLTree tree, void * rec)
                --> Searches tree '*tree' for matching 'rec' and deletes
                    Returns non-zero on error

        int AVL_SearchTree(PAVLTree tree, void * rec, void ** node)
                --> Searches tree '*tree' for node matching 'rec' by key
                    using the current key comparison function, and places
                    result in '*node'.  Returns non-zero on error.

        int AVL_SearchWholeTree
            (PAVLTree tree, void * rec, void **node, int (*comp)(void*,void*))
                --> Performs an inorder traversal on '*tree' until it
                    finds 'rec', using 'comp' as its comparison function.
                    If found, it places the node pointer in '*node'.
                    Returns non-zero on error.

        int AVL_InitTree(PAVLTree tree)
                --> Initializes the AVLTree struct 'tree'

        int AVL_Traverse(PAVLTree tree, EAVLOrd order, int (*act)(void *))
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

        int AVL_SetTreeComp(PAVLTree tree, int (*comp)(void *, void *))
                --> Sets the compare function for '*tree' to 'comp()'
                    'comp' should be defined as 'int comp(void *,void *);'

    Internal functions:

        Rotate_Left     --> Implements LL and LR rotations (from book)
        Rotate_Right    --> Implements RR and RL rotations
        Traverse        --> implements recursive traversal of tree
        Search          --> implements nonrecursive binary search of tree
        SearchAll       --> implements recursive sequential search

        GetNode         --> Allocates memory for one AVLNode
        PutNode         --> Deallocates memory for one AVLNode
        FillPool        --> Mallocs a pool of nodes (for GetNode)

*/


/*
    These routines are written to be as generic as possible, and to treat
    the AVL Tree structure as a container for records of various types,
    each with its own function(s) for key comparison and tree searching.

    This approach mimics the "Container Class" idea that C++ embodies,
    without the necessity of actually using C++.  (I don't know C++.)

*/

/*
    Side note:  I realize that it is considered 'incorrect' to place
    punctuation that is associated with a word or words in quotes
    outside the quotation marks.  However, in the field of computing,
    where quoted expressions are meant to specify a literal expression,
    to be taken exactly, and where such punctuation is not correctly
    part of the literal expression, then the only recourses are to
    (a) wrongfully place the puctuation inside the quotes, thus making
    the literal string incorrect, (b) omit the punctuation, leading
    to confusing sentence structures, or (c) risk offending strict
    purists by placing the affected punctuation outside the quotation
    marks.  I have opted for (c) as it is the most reasonable in my
    own opinion.
*/

const char * CAVLEmsg[]=
{
    "avl.c:  No error.",
    "avl.c:  Operation attempted on empty tree.",
    "avl.c:  Key not found in tree.",
    "avl.c:  Key already in tree.",
    "avl.c:  No comparison function set.",
    "avl.c:  Warning: Comparison function changed on non-empty tree."
};

typedef enum { False=0, True=1 } Boolean;

/* this is where we store our current key search comparison function */
static int (*S_Comp)(void *,void *);



static PAVLNode    AVLNodePool=NULL;
#define     CLUMPSIZE   8192
#define     CLUMPCOUNT  (CLUMPSIZE/sizeof(TAVLNode))

static void PutNode(PAVLNode node);

static void
FillPool
(void)
{
    PAVLNode end;
    int i;

    end=(PAVLNode)malloc(CLUMPSIZE);

    if (!end)
    {
        fprintf(stderr,"avl.c:  Out of memory in FillPool!\n");
        exit(1);
    }

    for(i=CLUMPCOUNT-1;i>0;i--)
    {
        PutNode(end+i);
    }

    return;
}

static PAVLNode
GetNode
(void)
{
    PAVLNode node;

    if (AVLNodePool==NULL) FillPool();

    node=AVLNodePool;

    AVLNodePool=node->r;

    node->r=node->l=NULL;
    node->bal=Balanced;

    return node;
}

static void
PutNode
(PAVLNode node)
{
    node->r=AVLNodePool;
    AVLNodePool=node;
    return;
}


static void
Rotate_Left
(PPAVLNode pp)
{
    PAVLNode p,c,g;

    p=*pp; /* simplifies and clarifies the coding... */

    c=p->l;
    if (c->bal == LeftHigh)
    {
        /* LL */
        p->l  =c->r;
        c->r  =p;
        p->bal=Balanced;
        p     =c;
    } else
    {
        /* LR */
        g   =c->r;
        c->r=g->l;
        g->l=c;
        p->l=g->r;
        g->r=p;
        switch(g->bal)
        {
        case LeftHigh:
            p->bal=RightHigh;
            c->bal=Balanced;
            break;
        case Balanced:
            p->bal=Balanced;
            c->bal=Balanced;
            break;
        case RightHigh:
            p->bal=Balanced;
            c->bal=LeftHigh;
            break;
        default:
            fprintf(stderr,"avl.c:  Invalid balance factor %d in "
                           "Rotate_Left, aborting.\n",g->bal);
            exit(1);
        }
        p=g;  /* I am my own grand-pa. */
    }
    p->bal=Balanced;
    *pp=p;  /* See, if I had (*pp)-> all over up above, */
            /* that would have been real messy, huh?    */
}

static void
Rotate_Right
(PPAVLNode pp)
{
    PAVLNode p,c,g;

    p=*pp; /* simplifies and clarifies the coding... */

    c=p->r;
    if (c->bal == RightHigh)
    {
        /* RR */
        p->r  =c->l;
        c->l  =p;
        p->bal=Balanced;
        p     =c;
    } else
    {
        /* RL */
        g   =c->l;
        c->l=g->r;
        g->r=c;
        p->r=g->l;
        g->l=p;
        switch(g->bal)
        {
        case RightHigh:
            p->bal=LeftHigh;
            c->bal=Balanced;
            break;
        case Balanced:
            p->bal=Balanced;
            c->bal=Balanced;
            break;
        case LeftHigh:
            p->bal=Balanced;
            c->bal=RightHigh;
            break;
        default:
            fprintf(stderr, "avl.c:  Invalid balance factor %d in "
                            "Rotate_Right, aborting.\n",g->bal);

            exit(1);
        }
        p=g;  /* I am my own grand-pa. */
    }
    p->bal=Balanced;
    *pp=p;  /* See, if I had (*pp)-> all over up above, */
            /* that would have been real messy, huh?    */
}

static Boolean
insert
(PPAVLNode pp, void * rec, Boolean *Unbal)
{
    int i;
    Boolean in_tree;
    PAVLNode p=*pp;

    if (p==NULL)
    {
        *Unbal=True;
        p=GetNode();
        p->rec=rec;
        p->bal=Balanced;
        *pp=p;
        return False;
    }

    i=S_Comp(p->rec,rec);
    if (i==0)
    {
        *Unbal=False;
        return True;
    } else if (i<0)
    {
        in_tree=insert(&(p->l),rec,Unbal);
        if (in_tree!=False) return True;
        if (*Unbal!=False)
        {
            switch(p->bal)
            {
            case RightHigh:
                p->bal=Balanced;
                *Unbal=False;
                break;
            case Balanced:
                p->bal=LeftHigh;
                break;
            case LeftHigh:
                Rotate_Left(pp);
                *Unbal=False;
            }
        }
    } else if (i>0)
    {
        in_tree=insert(&(p->r),rec,Unbal);
        if (in_tree!=False) return True;
        if (*Unbal!=False)
        {
            switch(p->bal)
            {
            case LeftHigh:
                p->bal=Balanced;
                *Unbal=False;
                break;
            case Balanced:
                p->bal=RightHigh;
                break;
            case RightHigh:
                Rotate_Right(pp);
                *Unbal=False;
            }
        }
    }
    return False;
}

int
AVL_AddNode
(PAVLTree tree, void * rec)
{
    Boolean Unbal=False;
    Boolean in_tree;

    if ((S_Comp=tree->comp)==NULL) return EAVL_NOCOMP;
    if (tree->root==NULL)
    {
        tree->root=GetNode();
        tree->root->rec=rec;
        return EAVL_NOERR;
    }

    S_Comp=tree->comp;
    in_tree=insert(&tree->root, rec, &Unbal);

    if (in_tree!=False)
    {
        return EAVL_DUPREC;
    }

    return EAVL_NOERR;
}

static int
Traverse
(PAVLNode node, EAVLOrd order, int (*act)(void *))
/* Internal recursive subfunction of AVL_Traverse */
{
    int i;

Traverse_Top: /* for elimination of tail-recursion */

    if (node==NULL) return 0;

    switch (order)
    {
    case PreOrder:
        i=act(node->rec);
        if (i) return i;
        if (node->l!=NULL) Traverse(node->l,order,act);
        if (node->r!=NULL) { node=node->r; goto Traverse_Top; }
        break;
    case InOrder:
        if (node->l!=NULL) Traverse(node->l,order,act);
        i=act(node->rec);
        if (i) return i;
        if (node->r!=NULL) { node=node->r; goto Traverse_Top; }
        break;
    case PostOrder:
        if (node->l!=NULL) Traverse(node->l,order,act);
        if (node->r!=NULL) Traverse(node->r,order,act);
        i=act(node->rec);
        if (i) return i;
        break;
    }

    return EAVL_NOERR;
}

int
AVL_Traverse
(PAVLTree tree, EAVLOrd order, int (*act)(void *))
{
    if (tree==NULL || tree->root==NULL || act==NULL)  return 0;

    return Traverse(tree->root,order,act);
}


static PAVLNode
Search
(PAVLNode node, void * rec)
{
    int i;

    if (S_Comp==NULL || node==NULL || rec==NULL) return NULL;

    while (node!=NULL)
    {
        if ((i=S_Comp(node->rec,rec))==0) return node;
        if (i<0)
        {
            if (node->l!=NULL) node=node->l; else return NULL;
        } else if (i>0)
        {
            if (node->r!=NULL) node=node->r; else return NULL;
        }
    }
    return NULL;
}

int
AVL_SearchTree
(PAVLTree tree, void * rec, void ** node)
{
    PAVLNode n;

    *node=NULL;

    if (tree==NULL || tree->root==NULL) return EAVL_NULLTREE;
    if (tree->comp==NULL) return EAVL_NOCOMP;

    S_Comp=tree->comp;
    n=Search(tree->root, rec);

    if (n==NULL) return EAVL_NOTFOUND;

    *node=n->rec;

    return EAVL_NOERR;
}

static PAVLNode
SearchAll
(PAVLNode node, void * rec)
{
    int i;
    PAVLNode n;

Search_Top:

    if (S_Comp==NULL || node==NULL || rec==NULL) return NULL;

    if (node->l!=NULL) {n=SearchAll(node->l,rec); if (n!=NULL) return n;}
    if ((i=S_Comp(node->rec,rec))==0) return node;
    if (node->r!=NULL) {node=node->r; goto Search_Top; }

    return NULL;
}


int
AVL_SearchWholeTree
(PAVLTree tree, void * rec, PPAVLNode node, int (*comp)(void *,void *))
{
    PAVLNode n;

    *node=NULL;

    if (tree==NULL || tree->root==NULL) return EAVL_NULLTREE;
    if (comp==NULL) return EAVL_NOCOMP;

    S_Comp=comp;
    n=SearchAll(tree->root, rec);

    if (n==NULL) return EAVL_NOTFOUND;

    *node= (PAVLNode)n->rec;

    return EAVL_NOERR;
}

static void
KillTree
(PAVLNode node)
{
    PAVLNode n;
    if (node==NULL) return;

    while (node!=NULL)
    {
        if (node->rec!=NULL) free(node->rec);
        if (node->l!=NULL) KillTree(node->l);
        n=node;
        node=node->r;  /* Tail-recursion elimination rulez */
        PutNode(n);
    }
}

static void
KillJustTree
(PAVLNode node)
{
    PAVLNode n;
    if (node==NULL) return;

    while (node!=NULL)
    {
        if (node->l!=NULL) KillJustTree(node->l);
        n=node;
        node=node->r;  /* Tail-recursion elimination rulez */
        PutNode(n);
    }
}

int
AVL_KillTree
(PAVLTree tree)
{
    if (tree==NULL) return EAVL_NULLTREE;

    if (tree->root!=NULL) KillTree(tree->root);
/*  free(tree); */
    return EAVL_NOERR;
}

int
AVL_KillJustTree
(PAVLTree tree)
{
    if (tree==NULL) return EAVL_NULLTREE;

    if (tree->root!=NULL) KillJustTree(tree->root);
/*  free(tree); */
    return EAVL_NOERR;
}

int
AVL_SetTreeComp
(PAVLTree tree, int (*comp)(void *,void *))
{
    if (tree==NULL) return EAVL_NULLTREE;

    tree->comp=comp;
    if (tree->root!=NULL) return EAVL_COMPCHG;
    return EAVL_NOERR;
}

int
AVL_InitTree
(PAVLTree tree)
{
    if (tree==NULL) return EAVL_NULLTREE;

    tree->comp=NULL;
    tree->root=NULL;
    return EAVL_NOERR;
}

static Boolean
avl_delete
(PPAVLNode pp, void * rec, Boolean *Unbal, Boolean *Found, void ** PPrec)
{
    int i;
    Boolean in_tree;
    PAVLNode p=*pp;

    if (p==NULL)
    {
        *Unbal=False;
        return *Found;
    }
    i=S_Comp(p->rec,rec);
    if (*Found==True)
    {
        if ((i<0 && p->l==NULL)||(i>0 && p->r==NULL))
        {
            void * t;
            t=*PPrec;
            *PPrec=p->rec;
            p->rec=t;
            i=0;
        }
    }

    if (i==0)
    {
        *Found=True;
        /* Do delete */
        if (p->l==NULL && p->r==NULL) /* Case:  No kids */
        {
            *pp=NULL;
            *Unbal=True;
            free(p->rec);
            PutNode(p);
            return True;
        } else if (p->l==NULL || p->r==NULL) /* Case:  One kid */
        {
            *Unbal=True;
            if (p->l==NULL)
            {
                *pp=p->r;  /* replace lame duck node w/ r child */
            } else
            {
                *pp=p->l;  /* replace lame duck node w/ l child */
            }
            PutNode(p);
            free(p->rec);
            return True;
        } else /* Case:  Two kids... do swap-a-roo */
        {
            PPrec=&(p->rec);
            if (p->bal==LeftHigh)
            {
                /*avl_delete(&(p->l),rec,Unbal,Found,PPrec);*/
                i=-1;
            } else
            {
                /*avl_delete(&(p->r),rec,Unbal,Found,PPrec);*/
                i=1;
            }
        }
    /*  return True;*/
    }
    if (i<0)
    {
        in_tree=avl_delete(&(p->l),rec,Unbal,Found,PPrec);
        if (in_tree==False) { return False; }
        if (*Unbal!=False && p!=NULL)
        {
            switch(p->bal)
            {
            case LeftHigh:
                p->bal=Balanced;
                *Unbal=False;
                break;
            case Balanced:
                p->bal=RightHigh;
                break;
            case RightHigh:
                Rotate_Right(pp);
                *Unbal=False;
            }
        }
        return True;
    } else if (i>0)
    {
        in_tree=avl_delete(&(p->r),rec,Unbal,Found,PPrec);
        if (in_tree==False) { return False; }
        if (*Unbal!=False && p!=NULL)
        {
            switch(p->bal)
            {
            case RightHigh:
                p->bal=Balanced;
                *Unbal=False;
                break;
            case Balanced:
                p->bal=LeftHigh;
                break;
            case LeftHigh:
                Rotate_Left(pp);
                *Unbal=False;
            }
        }
        return True;
    }
    return False;
}

int
AVL_DelNode
(PAVLTree tree, void * rec)
{
/*
    My algorithm for delete:  (rough sketch)

        1.  Recurse into tree until node to delete is found
        2.  Delete:
            If node has no children, delete it and return "Unbalanced"
            If node has one child, replace node with child, return "Unbal."
            If node has two children, continue.
            A.  If left is higher, recurse to find right-most left child, else
                recurse to find left-most right child.
            B.  Swap child with node, and goto step two, deleting the node
                from the position of the child.
        3.  If "Unbalanced" rebalance, similar to how "insert" works.
 */
    Boolean Unbal,Found,F;
    void **p;

    p=NULL;
    Found=Unbal=False;

    F=avl_delete(&(tree->root),rec,&Unbal,&Found,p);

    if (F==False && Found==False) return EAVL_NOTFOUND;

    return EAVL_NOERR;
}

static void
dump
(PAVLNode node,int level,int (*act)(void *))
{
    int i;
    char Bal[]="LRB";

    if (node==NULL) return;
    dump(node->l,level+1,act);
    for(i=0;i<level;i++) printf("  .");
    printf("(%p,%p,%c)[%p,%p]",
            (void*)node, node->rec, Bal[node->bal],
            (void*)node->l, (void*)node->r);
    if (act!=NULL) act(node->rec);
    printf("\n");
    dump(node->r,level+1,act);
}

void
AVL_DumpTreeInfo
(PAVLTree tree,int (*act)(void *))
{
    printf("--start dump\n");
    printf("tree  ==%p\n",(void*)tree);
    if (tree==NULL) return;
    printf("->root==%p\n",(void*)tree->root);
    printf("->comp==%p\n\n",(void*)tree->comp);
    if (tree->root!=NULL) dump(tree->root,0,act);
    printf("--end dump\n");
    return;
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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
