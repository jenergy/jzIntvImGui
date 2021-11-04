/*
HEADER:     ;
TITLE:      Frankenstein Cross Assemblers;
VERSION:    2.0;
DESCRIPTION: "  Reconfigurable Cross-assembler producing Intel (TM)
            Hex format object records.  ";
KEYWORDS:   cross-assemblers, 1805, 2650, 6301, 6502, 6805, 6809,
            6811, tms7000, 8048, 8051, 8096, z8, z80;
SYSTEM:     UNIX, MS-Dos ;
FILENAME:   frasmain.c;
WARNINGS:   "This software is in the public domain.
            Any prior copyright claims are relinquished.

            This software is distributed with no warranty whatever.
            The author takes no responsibility for the consequences
            of its use.

            Yacc (or Bison) required to compile."  ;
SEE-ALSO:   base.doc, as*.doc (machine specific appendices) ,
            as*.1 (man pages);
AUTHORS:    Mark Zenier;
COMPILERS:  Microport Sys V/AT, ATT Yacc, Turbo C V1.5, Bison (CUG disk 285)
            (previous versions Xenix, Unisoft 68000 Version 7, Sun 3);
*/
/*
            description Main file
            usage       Unix, framework crossassembler
            history     September 25, 1987
            August 3, 1988    v 1.4
            September 14, 1990  v 1.5  Dosified
*/

#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "frasmdat.h"
#include "file/file.h"
#include "../imasm/c_wrap.h"
#include "icart/icartrom.h"
#include "as1600_types.h"
#include "protos.h"
#include "intermed.h"
#include "collect.h"

FILE *romoutf, *binoutf, *cfgoutf, *loutf;
char *cfgfn = NULL, *binfn  = NULL, *romfn    = NULL;
char *hexfn = NULL, *loutfn = NULL, *srcmapfn = NULL;
int errorcnt = 0, warncnt = 0;
int listflag = FALSE, hexflag = FALSE, hexvalid = FALSE;
static int debugmode = FALSE;
static FILE *symbf;
static char *symbfn;
static int  symbflag = FALSE;
char hexcva[17] = "0123456789ABCDEF";
FILE *smapf = NULL;
int  currseg, currpag;
int *(chtatab[NUM_CHTA]);
int  locctr;
ictype_t icart_type = ICART;
struct evalrel evalr[NUMPEXP];

//unsigned int memory_bitmap[65536 >> 5];
int show_map = FALSE;

time_t asm_time = 0;
struct tm asm_time_local;
struct tm asm_time_gmt;

path_t *as1600_search_path = NULL;

extern struct parser_callbacks imasm_hooks;

LOCAL void usage(void) 
{
    printf("AS1600 (%s)\n", svn_revision ? svn_revision : "revision unknown");
    printf(
"Usage:\n"
"   as1600 [flags] input.asm\n"
"\n"
"Flags:\n"
"\n"
"   -o <file>   --out-file=<file>     Output file name\n"
"   -l <file>   --list-file=<file>    Specify listing file\n"
"   -s <file>   --sym-file=<file>     Specify symbol dump file\n"
"   -j <file>   --src-map=<file>      Specify source-map file\n"
"   -i <path>   --include=<path>      Append <path> to include path\n"
"   -m          --show-map            Summarize memory map after assembly\n"
"   -3   -c     --cc3                 Assemble .ROMs for CC3\n"
"   -h   -?     --help                Show this usage info\n"
"   -v          --version             Show assembler version\n"
"   -e          --err-if-overwritten  Set ERR_IF_OVERWRITTEN to 1 initially\n"
"\n"
"The output file type is determined by the extension on the provided output\n"
"filename (.bin or .rom).  If you omit the extension, AS1600 will attempt to\n"
"output both formats.  Some features, such as page-flipping or bankswitching\n"
"only work in one format; AS1600 will warn if it detects a mismatch.\n"
"\n"
    );
}
   
static struct option longopts[] = 
{ 
    { "cc3",                0,  NULL,   '3' },
    { "include",            1,  NULL,   'i' },
    { "list-file",          1,  NULL,   'l' },
    { "out-file",           1,  NULL,   'o' },
    { "show-map",           0,  NULL,   'm' },
    { "sym-file",           1,  NULL,   's' },
    { "src-map",            1,  NULL,   'j' },
    { "version",            0,  NULL,   'v' },
    { "help",               0,  NULL,   '?' },
    { "err-if-overwritten", 0,  NULL,   'e' },
    { NULL,                 0,  NULL,   0   },
};

int asm_main(int argc, char *argv[])
/*
    description top driver routine for framework cross assembler
                set the cpu type if implemented in parser
                process the command line parameters
                setup the tables
                call the first pass parser
                print the symbol table
                call the second pass
                close down and delete the outputs if any errors
    return      exit(2) for error, exit(0) for OK
*/
{
    extern char *optarg;
    extern int optind;
    int grv;
    int cart_type = 0;
    int err_if_overwrite = 0;

    /* Set the "current assembly time" to the time we started, not some
       later time, and keep it fixed throughout. */
    asm_time       = time(0);
    asm_time_local = *localtime(&asm_time);
    asm_time_gmt   = *gmtime(&asm_time);

    /* -------------------------------------------------------------------- */
    /*  Figure out out our executable's path.  If none, don't set one.      */
    /* -------------------------------------------------------------------- */
    if (!exe_path)
        exe_path = get_exe_dir(argv[0]);

    grv = cpumatch(argv[0]);

    while( (grv = getopt_long(argc, argv, "mdj:o:l:s:p:i:c3v?he",
                              longopts, NULL)) != EOF)
    {
        switch(grv)
        {
        case 'i':
            as1600_search_path = append_path(as1600_search_path, optarg);
            break;
        case 'o':
            hexfn = optarg;
            hexflag = hexvalid = TRUE;
            break;

        case 'j':
            srcmapfn = optarg;
            break;

        case 'l':
            loutfn = optarg;
            listflag = TRUE;
            break;

        case 'd':
            debugmode = TRUE;
            break;

        case 's':
            symbflag = TRUE;
            symbfn = optarg;
            break;

        case 'm':
            show_map = TRUE;
            break;

        case 'p':
            if( ! cpumatch(optarg) )
            {
                fprintf(stderr, "%s: no match on CPU type %s, default used\n",
                    argv[0], optarg);
            }
            break;

        case '3':
        case 'c':
            cart_type++;
            if (cart_type == 1) icart_type = CC3_STD;
            if (cart_type == 2) icart_type = CC3_ADV;
            break;

        case 'v':
            if (svn_revision)
                printf("AS1600 built from %s\n", svn_revision);
            else
                printf("AS1600 revision unknown\n");
            exit(0);

        case 'e':
            err_if_overwrite = 1;
            break;

        case '?':
        case 'h':
            usage();
            exit(0);
        }
    }

    if(optind < argc)
    {
        if(strcmp(argv[optind], "-") == 0)
        {
            yyin = lzoe_filep( stdin );
        }
        else
        {
            if( !(yyin = lzoe_fopen(argv[optind], "r")) )
            {
                fprintf(stderr,
                    "%s: cannot open input file %s\n",
                    argv[0], argv[optind]);
                exit(1);
            }
        }
    }
    else
    {
        fprintf(stderr, "%s: no input file\n", argv[0]);
        exit(1);
    }

    as1600_search_path = append_path(as1600_search_path, ".");

    as1600_search_path = parse_path_string(as1600_search_path,
                                            getenv("AS1600_PATH"));

    if(listflag)
    {
        if(strcmp(argv[optind], loutfn) == 0)
        {
            fprintf(stderr, "%s: list file overwrites input %s\n",
                argv[0], loutfn);
            listflag = FALSE;
        }
        else if( (loutf = fopen(loutfn, "w")) == (FILE *) NULL)
        {
            fprintf(stderr, "%s: cannot open list file %s\n",
                argv[0], loutfn);
            listflag = FALSE;
        }
    }

    if( ! listflag)
    {
        loutf = stdout;
    }

    if (srcmapfn)
    {
        if(strcmp(argv[optind], srcmapfn) == 0)
        {
            fprintf(stderr, "%s: source map file overwrites input %s\n",
                    argv[0], srcmapfn);
            srcmapfn = NULL;
        }
        else if( (smapf = fopen(srcmapfn, "w")) == (FILE *) NULL)
        {
            fprintf(stderr, "%s: cannot open source map file %s\n",
                argv[0], srcmapfn);
            srcmapfn = NULL;
        }
    }

    intermed_start_pass_1();

    setophash();
    setreserved();
    elseifstk[0] = endifstk[0] = If_Err;
    expmacstk[0] = TRUE;
    emit_entering_file(argv[optind]);
    infilestk[0].fpt  = yyin;
    infilestk[0].fnm  = argv[optind];
    infilestk[0].line = 0;
    currfstk = 0;
    currseg = 0;
    currpag = -1;

    init_parser(&imasm_hooks);
    collect_init();
    if (err_if_overwrite)
        collect_set_overwrite_flags(1, -1);

    yyparse();

    close_parser();

    if(ifstkpt != 0)
        fraerror("active IF at end of file");

    if(frarptact != FALSE || frarptcnt != -1 || frarptskip != FALSE)
        fraerror("active REPEAT at end of file");

    buildsymbolindex();
    if(listflag)
        printsymbols();

    if(symbflag)
    {
        if(strcmp(argv[optind], symbfn) == 0)
        {
            fprintf(stderr, "%s: symbol file overwrites input %s\n",
                argv[0], symbfn);
        }
        else if( (symbf = fopen(symbfn, "w")) == (FILE *) NULL)
        {
            fprintf(stderr, "%s: cannot open symbol file %s\n",
                argv[0], symbfn);
        }
        else
        {
            filesymbols();
            fclose(symbf);
        }
    }


    intermed_start_pass_2();

    if(errorcnt > 0)
        hexflag = FALSE;

    if(hexflag)
    {
        int hexfn_len = strlen(hexfn);
        char *s = hexfn + hexfn_len;
        char *ext, *tail1, *tail2;

        /* Only examine the last path component */
        tail1 = strrchr(hexfn, '/');
        if (!tail1) tail1 = hexfn;
        tail2 = strrchr(tail1, '\\');
        if (!tail2) tail2 = tail1;

        ext = strrchr(tail2, '.');  /* look for an extension */

        if (ext == NULL || ext == hexfn)
        {   /* no extension */
            binfn = (char *)malloc(hexfn_len + 5);
            cfgfn = (char *)malloc(hexfn_len + 5);
            romfn = (char *)malloc(hexfn_len + 5);
            if (!binfn || !cfgfn || !romfn)
            {
                fprintf(stderr, "%s: Out of memory\n", argv[0]);
                exit(1);
            }
            snprintf(binfn, hexfn_len+5, "%s.bin", hexfn);
            snprintf(cfgfn, hexfn_len+5, "%s.cfg", hexfn);
            snprintf(romfn, hexfn_len+5, "%s.rom", hexfn);
            if (!cart_type) icart_type = ICART;
        } else if (stricmp(ext, ".rom")==0)
        {
            if (!cart_type) icart_type = ICART;
            romfn      = hexfn;
        } else if (stricmp(ext, ".cc3")==0)
        {
            if (!cart_type) icart_type = CC3_STD;
            romfn      = hexfn;
        } else if (stricmp(ext, ".bin")==0)
        {
            binfn = hexfn;
            cfgfn = strdup(hexfn);
            if (!cfgfn)
            {
                fprintf(stderr, "%s: Out of memory\n", argv[0]);
                exit(1);
            }

            s = cfgfn + hexfn_len - 4;
            strcpy(s, ".cfg");
        } else
        {
            fprintf(stderr, "%s: Unknown output file extension '%s'\n",
                    argv[0], ext);
            hexflag = FALSE;
        }

        {
            char *tmp[3] = {binfn, cfgfn, romfn};
            FILE **ftmp[3] = {&binoutf, &cfgoutf, &romoutf};
            int ck;

            for (ck = 0; ck < 3; ck++)
                if (tmp[ck] && strcmp(argv[optind], tmp[ck]) == 0)
                {
                    fprintf(stderr, "%s: binary output overwrites input %s\n",
                            argv[0], tmp[ck]);
                    hexflag = FALSE;
                    break;
                }

            if (hexflag)
                for (ck = 0; ck < 3; ck++)
                {
                    if (!tmp[ck])
                        continue;
                    *ftmp[ck] = fopen(tmp[ck], "wb");
                    if (!*ftmp[ck])
                    {
                        fprintf(stderr, "%s: cannot open output file %s\n",
                                argv[0], tmp[ck]);
                        hexflag = FALSE;
                        break;
                    }
                }
        }

        if (hexflag == FALSE)
        {
            fprintf(stderr, "%s:  Warning:  output file not written due to "
                            "previous errors\n", argv[0]);
        }
    }


    currfstk = 0;
    outphase();

    if(errorcnt > 0)
        hexvalid = FALSE;

    fprintf(loutf, " ERROR SUMMARY - ERRORS DETECTED %d\n", errorcnt);
    fprintf(loutf, "               -  WARNINGS       %d\n", warncnt);
    listlineno += 2;

    if(listflag)
    {
        fprintf(stderr, " ERROR SUMMARY - ERRORS DETECTED %d\n",
            errorcnt);
        fprintf(stderr, "               -  WARNINGS       %d\n",
            warncnt);
    }

    if(listflag)
        fclose(loutf);

    if(srcmapfn)
        fclose(smapf);

    if(hexflag)
    {
        if (binoutf) fclose(binoutf);
        if (cfgoutf) fclose(cfgoutf);
        if (romoutf) fclose(romoutf);
        if (!hexvalid)
        {
            if (binfn) unlink(binfn);
            if (cfgfn) unlink(cfgfn);
            if (romfn) unlink(romfn);
        }
    }

    if (show_map)
        collect_show_map();

    intermed_finish(debugmode);

    exit(errorcnt > 0 ? 2 : 0);
}


void frafatal(char * str)
/*
    description Fatal error subroutine, shutdown and quit right now!
    parameters  message
    globals     if debug mode is true, save intermediate file
    return      exit(2)
*/
{
    fprintf(stderr, "Fatal error - %s\n",str);

    intermed_finish(debugmode);

    exit(2);
}

void frawarn(char * str)
/*
    description first pass - generate warning message by writing line
            to intermediate file
    parameters  message
    globals     the count of warnings
*/
{
    emit_warnerr(infilestk[currfstk].fnm, infilestk[currfstk].line,
                 WARNING, "%s", str);
}

void fraerror(const char * str)
/*
    description first pass - generate error message by writing line to
            intermediate file
    parameters  message
    globals     count of errors
*/
{
    emit_warnerr(infilestk[currfstk].fnm, infilestk[currfstk].line,
                 ERROR, "%s", str);
}

void fracherror(char *str, char *start, char *beyond)
/*
    description first pass - generate error message by writing line to
            intermediate file
    parameters  message
            pointer to bad character definition
            pointer after bad definition
    globals     count of errors
*/
{
    char bcbuff[8];
    int cnt;

    for(cnt=0; start < beyond && *start != '\0' && cnt < 7; cnt++)
    {
        bcbuff[cnt] = *start++;
    }
    bcbuff[cnt] = '\0';

    emit_warnerr(infilestk[currfstk].fnm, infilestk[currfstk].line,
                 ERROR, "%s '%s'", str, bcbuff);
}

#define SYMPERLINE 2

void printsymbols(void)
/*
    description print the symbols on the listing file, 3 symbols
            across.  Only the first 15 characters are printed
            though all are significant.  Reserved symbols are
            not assigned symbol numbers and thus are not printed.
    globals     the symbol index array and the symbol table elements.
*/
{
    int syn, npl = 0;
    struct symel *syp;

    for(syn = 1; syn <nextsymnum; syn++)
    {
        if(npl >= SYMPERLINE)
        {
            fputc('\n', loutf);
            listlineno++;
            npl = 0;
        }

        syp = symbindex[syn];

        if (syp -> flags & SFLAG_QUIET)
            continue;

        if(syp -> seg != SSG_UNDEF)
            fprintf(loutf, "%8.8X %-25.25s  ", 0xFFFFFFFF & syp -> value,
                syp -> symstr);
        else
            fprintf(loutf, "???????? %-25.25s  ", syp -> symstr);
        npl++;
    }

    if(npl > 0)
    {
        listlineno++;
        fputc('\n', loutf);
    }

    fputc('\f', loutf);
}


void filesymbols(void)
/*
    description print the symbols to the symbol table file
    globals     the symbol index array and the symbol table elements.
*/
{
    int syn;
    struct symel *syp;

    for(syn = 1; syn <nextsymnum; syn++)
    {
        syp = symbindex[syn];

        if (syp -> flags & SFLAG_QUIET)
            continue;

        if(syp -> seg != SSG_UNDEF)
            fprintf(symbf, "%8.8X %s\n",syp -> value,
                syp -> symstr);
        else
            fprintf(symbf, "???????? %s\n", syp -> symstr);
    }
}

