
/*
HEADER:     ;
TITLE:      Frankenstein Cross Assemblers;
VERSION:    2.0;
DESCRIPTION: "  Reconfigurable Cross-assembler producing Intel (TM)
        Hex format object records.  ";
FILENAME:   frasmdat.h;
SEE-ALSO:   ;
AUTHORS:    Mark Zenier;
*/

/*
    description structures and data used in parser and output phases
    history     September 15, 1987
            August 3, 1988   Global
            September 14, 1990   6 char portable var
*/

#include <ctype.h>
#define PRINTCTRL(char) ((char)+'@')

#ifdef USEINDEX
#define strchr index
#endif

#ifdef NOSTRING
extern char * strncpy();
extern char * strchr();
extern int strcmp();
extern int strlen();
#else
#include <string.h>
#endif

#define local

#define TRUE 1
#define FALSE 0

#define hexch(cv) (hexcva[(cv)&0xf])
extern char hexcva[];

struct slidx { int first, last; struct symel *sym; };


/* symbol table element */
struct symel
{
    const char  *symstr;
    int         tok;
    int         seg;
    unsigned    flags;
    int         value;
    struct      symel *nextsym;
    int         symnum;
};

#define SSG_UNUSED  ( 0)
#define SSG_UNDEF   (-1)
#define SSG_ABS     ( 8)
#define SSG_RESV    (-2)
#define SSG_EQU     ( 2)
#define SSG_SET     ( 3)

#define SFLAG_NONE  (     0)
#define SFLAG_QUIET (1 << 0)
#define SFLAG_ARRAY (1 << 1)

#define SYMNULL (struct symel *) NULL

/* opcode symbol table element */

struct opsym
{
    const char *opstr;
    int         token;
    int         numsyn;
    int         subsyn;
};

struct opsynt
{
    int         syntaxgrp;
    int         elcnt;
    int         gentabsub;
};

struct igel
{
    int         selmask;
    int         criteria;
    const char *genstr;
};

#define PPEXPRLEN 1024

struct evalrel
{
    int     seg;
    int     value;
    char    exprstr[PPEXPRLEN];
};

#define INBUFFSZ (1 << 20)

extern int nextsymnum;
extern struct symel **symbindex;

extern struct opsym optab[];
extern int    gnumopcode;
extern struct opsynt ostab[];
extern struct igel igtab[];
extern int    ophashlnk[];

#define NUMPEXP 6
extern struct evalrel evalr[NUMPEXP];

#define PESTKDEPTH (1 << 12)
struct evstkel
{
    int v;
    int s;
};

extern struct evstkel   estk[PESTKDEPTH], *estkm1p;

extern int  currseg;
extern int  locctr;
extern int  currpag;

extern LZFILE *yyin;
extern int  listflag;
extern int hexvalid, hexflag;
extern FILE *romoutf, *binoutf, *cfgoutf, *loutf;
extern char *loutfn;
extern int listlineno;
extern int errorcnt, warncnt;


#define IFSTKDEPTH 256
extern int  ifstkpt;
enum If_Mode { If_Active, If_Skip, If_Err };
extern enum If_Mode elseifstk[IFSTKDEPTH], endifstk[IFSTKDEPTH];
extern int expmacstk[IFSTKDEPTH];

extern int  frarptact, frarptcnt, frarptskip;

#define FILESTKDPTH 32
extern int currfstk;
#define nextfstk (currfstk+1)
struct fstkel
{
    const char  *fnm;
    LZFILE      *fpt;
    int         line;
};
extern struct fstkel infilestk[FILESTKDPTH];

extern int lnumstk[FILESTKDPTH];

enum readacts
{
    Nra_normal,
    Nra_new,
    Nra_end
} ;

extern enum readacts nextreadact;


#ifndef macintosh
#include <stdlib.h>
#endif

extern struct symel * endsymbol;
extern char ignosyn[] ;
extern char ignosel[] ;

#define NUM_CHTA 6
extern int chtnxalph, *chtcpoint, *chtnpoint ;
extern int *(chtatab[NUM_CHTA]);

#define CF_END      -2
#define CF_INVALID  -1
#define CF_UNDEF    0
#define CF_CHAR     1
#define CF_NUMBER   2

#include "asm/typetags.h"

/* Values for CLASSIFY operator */
#define CLASS_ABS       (-1)
#define CLASS_SET       (-2)
#define CLASS_EQU       (-3)
#define CLASS_STRING    (-4)
#define CLASS_FEATURE   (-5)
#define CLASS_RESV      (-6)
#define CLASS_EMPTY     (-7)
#define CLASS_UNUSED    (-8)
#define CLASS_UNKNOWN   (-9)
#define CLASS_UNDEF     (-10000)

extern path_t * as1600_search_path;

/* The moment we started the assembler. */
extern time_t asm_time;
extern struct tm asm_time_local;
extern struct tm asm_time_gmt;

#include "intvec.h"
