/* ======================================================================== */
/*  Routines for managing source code in the debugger.                      */
/* ======================================================================== */

#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "source.h"
#include "debug/debug_tag.h"
#include "asm/typetags.h"

typedef struct smapping
{
    unsigned    file      : 12; /* up to 4095 files             */
    unsigned    line      : 20; /* up to 1M lines / file        */
    unsigned    flag      :  8; /* up to 8 flag bits            */
    unsigned    list_line : 24; /* up to 16M lines in listing   */
} smapping;

#define MAX_SOURCE_FILES (4095)

smapping smap_tbl[65536];

source_file_info *source_file;
int               source_files;
int               sf_alloc = 0;
smap_mode         smode = SMAP_SMART;
path_t           *as1600_search_path;
int               listing_handle;

#define FL_PREFER_LISTING (1)
#define SOURCEOFFSET      (32)      /* same meaning as in assembler */

void        set_source_map_mode(smap_mode mode);

/* ======================================================================== */
/*  GET_FILE_HANDLE      -- Get the text_file handle for a filename.        */
/* ======================================================================== */
LOCAL int get_file_handle(const char *name)
{
    LZFILE *f;
    int i;

    /* -------------------------------------------------------------------- */
    /*  First see if we've already got this file.                           */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < source_files; i++)
        if (!strcmp(source_file[i].name, name))
            return i + 1;

    /* -------------------------------------------------------------------- */
    /*  Only allow up to MAX_SOURCE_FILES files for now.                    */
    /* -------------------------------------------------------------------- */
    if (source_files == MAX_SOURCE_FILES)
        return 0;

    /* -------------------------------------------------------------------- */
    /*  Next see if we can open the file.                                   */
    /* -------------------------------------------------------------------- */
    f = path_fopen(as1600_search_path, name, "rb");

    if (!f)
        return 0;

    /* -------------------------------------------------------------------- */
    /*  Now allocate a slot for it and populate it.                         */
    /* -------------------------------------------------------------------- */
    if (source_files >= sf_alloc)
    {
        sf_alloc    += 16;
        source_file  = (source_file_info *)
                        realloc(source_file, sf_alloc *
                                                   sizeof(source_file_info));
    }

    jzp_printf("Reading %s...  ", name);

    source_file[source_files].name = strdup(name);
    source_file[source_files].text = load_text_file(f, 8);

    if (!source_file[source_files].text)
    {
        jzp_printf("failed\n"); jzp_flush();
        CONDFREE(source_file[source_files].name);
        return 0;
    }

    jzp_printf("%d lines.\n", source_file[source_files].text->lines);
    jzp_flush();
    lzoe_fclose(f);

    return ++source_files;
}



/* ======================================================================== */
/*  PROCESS_SOURCE_MAP   -- Process a source-map file                       */
/*                                                                          */
/*  Directive lines:                                                        */
/*      CWD <string>         Source directory where AS1600 ran              */
/*      PATH <string>        Adds <string> to AS1600 search path            */
/*      LISTING <string>     Indicates <string> is the AS1600 listing file  */
/*      FILE <string>        Sets <string> as the current source file       */
/*                                                                          */
/*  Mapping lines:                                                          */
/*      <addr> <addr> <flags> <source_line> <listing_line>                  */
/*                                                                          */
/* ======================================================================== */
void process_source_map(const char *fname)
{
    LZFILE *f;
    char buf[2048], *s;
    char *cwd = NULL;
    int  curr = 0;
    uint32_t lo, hi, flags, src_line, list_line, addr, dflags;

    if ((f = lzoe_fopen(fname, "r")) == NULL)
    {
        perror("fopen()");
        fprintf(stderr, "Could not open '%s' for reading.\n", fname);
        return;
    }

    while (lzoe_fgets(buf, sizeof(buf), f) != NULL)
    {
        if ((s = strchr(buf, '\012')) != NULL) *s = 0;
        if ((s = strchr(buf, '\015')) != NULL) *s = 0;

        /* ---------------------------------------------------------------- */
        /*  Keywords....                                                    */
        /* ---------------------------------------------------------------- */
        if (!strncmp(buf, "CWD ", 4))
        {
            if (cwd) free(cwd);
            cwd = strdup(buf + 4);
            continue;
        }

        if (!strncmp(buf, "PATH ", 5))
        {
            s                  = make_absolute_path(cwd ? cwd : "", buf + 5);
            as1600_search_path = append_path(as1600_search_path, s);
            free(s);
            continue;
        }

        if (!strncmp(buf, "LISTING ", 8))
        {
            listing_handle = get_file_handle(buf + 8);
            continue;
        }

        if (!strncmp(buf, "FILE ", 5))
        {
            curr = get_file_handle(buf + 5);
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Ranges....                                                      */
        /* ---------------------------------------------------------------- */
        if (sscanf(buf, "%x %x %x %d %d", &lo, &hi, &flags,
                                          &src_line, &list_line) != 5
            || lo > hi || lo > 0xFFFF)
        {
            fprintf(stderr, "Unhandled line in source map:\n>> %s\n", buf);
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  Go mark the range in the source map table.                      */
        /* ---------------------------------------------------------------- */
        for (addr = lo; addr <= hi; addr++)
        {
            smap_tbl[addr].file      = curr;
            smap_tbl[addr].flag      = flags;
            smap_tbl[addr].line      = src_line;
            smap_tbl[addr].list_line = list_line;
        }

        /* ---------------------------------------------------------------- */
        /*  Update the memory attribute flags in the debugger.              */
        /* ---------------------------------------------------------------- */
        dflags = ((flags & TYPE_CODE)   ? DEBUG_MA_CODE : 0)
               | ((flags & TYPE_DATA)   ? DEBUG_MA_DATA : 0)
               | ((flags & TYPE_DBDATA) ? DEBUG_MA_SDBD : 0)
               | ((flags & TYPE_STRING) ? DEBUG_MA_DATA : 0);

        debug_tag_range(lo, hi, dflags);

        /* ---------------------------------------------------------------- */
        /*  Heuristic for "smart mode":  If two consecutive ranges have     */
        /*  the same file / line, but different listing line, then prefer   */
        /*  the listing over the source when in "smart" mode.               */
        /* ---------------------------------------------------------------- */
        if (lo > 0)
        {
            addr = lo;
            while (smap_tbl[addr - 1].file == smap_tbl[lo].file &&
                   smap_tbl[addr - 1].line == smap_tbl[lo].line)
            {
                if (smap_tbl[addr - 1].list_line == smap_tbl[lo].list_line)
                    break;
                addr--;
            }

            if (addr != lo)
            {
                while (addr <= hi)
                    smap_tbl[addr++].flag |= FL_PREFER_LISTING;
            }
        }
    }

    CONDFREE(cwd);
    lzoe_fclose(f);

    /* -------------------------------------------------------------------- */
    /*  Now step over the listing file, if there was one, and trim off      */
    /*  the leading parts of the lines, keeping just the source.            */
    /* -------------------------------------------------------------------- */
    if (listing_handle > 0)
    {
        uint32_t i, j, k;

        text_file *text = source_file[listing_handle - 1].text;

        for (i = 0; i != text->lines; i++)
        {
            k = text->line[i];

            for (j = 0; j < SOURCEOFFSET; j++, k++)
            {
                if (text->body[k] == 0)
                    break;

                if (text->body[k] == '\t')
                    j += (7 - (j & 7));
            }

            text->line[i] = k;
        }
    }
}

/* ======================================================================== */
/*  SOURCE_FOR_ADDR      -- Try to find code for this address, if any       */
/*                          exists.  Pull from source or listing based on   */
/*                          the current mode.                               */
/* ======================================================================== */
const char *source_for_addr(uint32_t addr)
{
    int file  = smap_tbl[addr].file - 1;
    int sline = smap_tbl[addr].line - 1;
    int lline = smap_tbl[addr].list_line - 1;
    const char *src = NULL, *lst = NULL;

    if (file >= 0 && sline >= 0 && sline < (int)source_file[file].text->lines)
        src = &source_file[file].text->body[
                source_file[file].text->line[sline]];

    file = listing_handle - 1;
    if (file >= 0 && lline >= 0 && lline < (int)source_file[file].text->lines)
        lst = &source_file[file].text->body[
                source_file[file].text->line[lline]];

    if (smode == SMAP_SMART)
    {
        if (src && lst && (smap_tbl[addr].flag & FL_PREFER_LISTING) != 0)
            return lst;

        return src ? src : lst;
    }

    if (smode == SMAP_SOURCE)
        return src;

    return lst;
}

/* ======================================================================== */
/*  FILE_LINE_FOR_ADDR   -- Get the file and line associated with an addr.  */
/* ======================================================================== */
int file_line_for_addr(uint32_t addr, int *line)
{
    int sfile = smap_tbl[addr].file - 1;
    int sline = smap_tbl[addr].line - 1;
    int lfile = listing_handle - 1;
    int lline = smap_tbl[addr].list_line - 1;

    if (sfile < 0 || sline >= (int)source_file[sfile].text->lines)
        sline = -1;

    if (lfile < 0 || lline >= (int)source_file[lfile].text->lines)
        lline = -1;

    if (smode == SMAP_SMART)
    {
        if (sline < 0 ||
            (lline >= 0 && (smap_tbl[addr].flag & FL_PREFER_LISTING) != 0))
        {
            *line = lline;
            return lfile;
        }

        *line = sline;
        return sfile;
    }

    if (smode == SMAP_SOURCE)
    {
        *line = sline;
        return sfile;
    }

    *line = lline;
    return lfile;
}

/* ======================================================================== */
/*  SOURCE_FOR_FILE_LINE -- Get the source line associated with the file    */
/*                          handle and source line, if any.                 */
/* ======================================================================== */
const char *source_for_file_line(int file, int line)
{
    if (file < 0 || file + 1 > source_files || line < 0)
        return NULL;

    if (line >= (int)source_file[file].text->lines)
        return NULL;

    return &source_file[file].text->body[source_file[file].text->line[line]];
}
