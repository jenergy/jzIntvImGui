/*
 * ============================================================================
 *  Title:    File I/O Routines
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module contains routines for reading/writing files, including ROM
 *  images, CFG files, etc.
 *
 *  Currently, these routines operate on LZFILE*'s rather than on filenames,
 *  since I'd like these to be able to work in structured files someday.
 *  (eg. so I can read a ROM image out of an archive, or such.)
 * ============================================================================
 *  FILE_READ_ROM16      -- Reads a 16-bit big-endian ROM image.
 *  FILE_READ_ROM8P2     -- Reads a 10-bit ROM image in 8 plus 2 format
 *  FILE_READ_ROM10      -- Reads an 8-bit ROM image (eg. GROM).
 *  FILE_PARSE_CFG       -- Parses a CFG file and returns a linked list of
 *                          configuration actions to be handled by the
 *                          machine configuration engine.
 * ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "lzoe/lzoe.h"
#include "file.h"

char *exe_path;


/* ======================================================================== */
/*  FILE_READ_ROM16  -- Reads a 16-bit ROM image up to 64K x 16.            */
/*                                                                          */
/*                      Leaves file pointer pointing at end of ROM image    */
/*                      if read is successful.  Returns 0 on success, -1    */
/*                      of failure.                                         */
/* ======================================================================== */
int         file_read_rom16     (LZFILE *f, int len, uint16_t img[])
{
    int r;

    /* -------------------------------------------------------------------- */
    /*  Sanity check:  To all the arguments make sense?                     */
    /* -------------------------------------------------------------------- */
    if (!f || !img || len < 0)
    {
        fprintf(stderr, "file_read_rom16:  Bad parameters!\n"
                        "                  %p, %10d, %p\n",
                        (void *)f, len, (void *)img);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Read in the ROM image.                                              */
    /* -------------------------------------------------------------------- */
    len = lzoe_fread((void*) img, 2, len, f);


    /* -------------------------------------------------------------------- */
    /*  Bring the ROM image into the host endian.                           */
    /* -------------------------------------------------------------------- */
    for (r = 0; r < len; r++)
        img[r] = be_to_host_16(img[r]);

    return len;
}

/* ======================================================================== */
/*  FILE_READ_ROM8P2 -- Reads a 10-bit ROM image up to 64K x 16 in packed   */
/*                      8 plus 2 format.  The first 'len' bytes are         */
/*                      the 8 LSB's of the ROM's decles.  The next          */
/*                      'len / 4' bytes hold the 2 MSBs, packed in little-  */
/*                      endian order.  This format is used by the VOL1,     */
/*                      VOL2 resource files, and is included for            */
/*                      completeness.                                       */
/*                                                                          */
/*                      Leaves file pointer pointing at end of ROM image    */
/*                      if read is successful.  Returns 0 on success, -1    */
/*                      of failure.                                         */
/* ======================================================================== */
int         file_read_rom8p2    (LZFILE *f, int len, uint16_t img[])
{
    int r, blk8sz, blk2sz, blk8, blk2, shl;
    uint8_t *tmp;

    /* -------------------------------------------------------------------- */
    /*  Sanity check:  To all the arguments make sense?                     */
    /* -------------------------------------------------------------------- */
    if (!f || !img || len < 0)
    {
        fprintf(stderr, "file_read_rom8p2:  Bad parameters!\n"
                        "                   %p, %10d, %p\n",
                        (void *)f, len, (void *)img);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Calculate the sizes of the 8-bit and 2-bit sections, being careful  */
    /*  to round the decle count up to handle non-multiple-of-4 images.     */
    /* -------------------------------------------------------------------- */
    blk8sz = len;
    blk2sz = (len + 3) >> 2;

    /* -------------------------------------------------------------------- */
    /*  Read in the ROM image to a temporary storage buffer for unpacking.  */
    /* -------------------------------------------------------------------- */
    tmp = CALLOC(uint8_t, blk8sz + blk2sz);

    if (!tmp)
    {
        fprintf(stderr, "file_read_rom8p2:  Out of memory.\n");
        exit(1);
    }

    r = lzoe_fread(tmp, 1, blk8sz + blk2sz, f);

    if (r != blk8sz + blk2sz)
    {
        fprintf(stderr, "file_read_rom8p2:  Error reading ROM image.\n");
        perror("fread()");

        free(tmp);
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Unpack the ROM image into the user's buffer.                        */
    /* -------------------------------------------------------------------- */
    for (blk8 = 0, blk2 = blk8sz; blk8 < blk8sz; blk8++)
    {
        shl = 8 - ((blk8 & 3) << 1);

        img[blk8] = tmp[blk8] | (0x0300 & (tmp[blk2] << shl));

        if ((blk8 & 3) == 3) blk2++;
    }

    free(tmp);

    return len;
}


/* ======================================================================== */
/*  FILE_READ_ROM8   -- Reads an 8-bit ROM image up to 64K x 16.            */
/*                                                                          */
/*                      Leaves file pointer pointing at end of ROM image    */
/*                      if read is successful.  Returns 0 on success, -1    */
/*                      of failure.                                         */
/* ======================================================================== */
int         file_read_rom8      (LZFILE *f, int len, uint16_t img[])
{
    int r;
    uint16_t packed;

    /* -------------------------------------------------------------------- */
    /*  Sanity check:  To all the arguments make sense?                     */
    /* -------------------------------------------------------------------- */
    if (!f || !img || len < 0)
    {
        fprintf(stderr, "file_read_rom8:  Bad parameters!\n"
                        "                 %p, %10d, %p\n",
                        (void *)f, len, (void *)img);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Read in the ROM image.                                              */
    /* -------------------------------------------------------------------- */
    len = lzoe_fread((void*) img, 1, len, f);
    if (len < 1) return len;

    /* -------------------------------------------------------------------- */
    /*  Unpack the ROM image.                                               */
    /* -------------------------------------------------------------------- */
    len = len + (len % 2);      /* Round length up to an even value         */

    for (r = len - 2; r >= 0; r -= 2)
    {
        packed = host_to_le_16(img[r >> 1]);

        img[r + 1] = packed >> 8;
        img[r + 0] = packed & 0xFF;
    }

    return len;
}

/* ======================================================================== */
/*  FILE_LENGTH     -- Returns the length of an open file                   */
/* ======================================================================== */
long file_length(LZFILE *f)
{
    long here, end;

    here = lzoe_ftell(f); lzoe_fseek(f, 0,    SEEK_END);
    end  = lzoe_ftell(f); lzoe_fseek(f, here, SEEK_SET);

    return end;
}

/* ======================================================================== */
/*  FILE_EXISTS     -- Determines if a given file exists.                   */
/* ======================================================================== */
int file_exists
(
    const char *pathname
)
{
#ifndef NO_LZO
    return lzoe_exists(pathname);
#else
    /* -------------------------------------------------------------------- */
    /*  NOTE: access() isn't portable, so fall back to fopen() if needed.   */
    /* -------------------------------------------------------------------- */
#ifndef NO_ACCESS
    return access(pathname, R_OK|F_OK) != -1;
#else
    FILE *f = fopen(pathname, "r");

    if (f)
        fclose(f);

    return f != NULL;
#endif
#endif
}

/* ======================================================================== */
/*  IS_ABSOLUTE_PATH -- Returns non-zero if the path is absolute.           */
/* ======================================================================== */
int is_absolute_path(const char *fname)
{
    if (fname[0] == PATH_SEP)
        return 1;

    if ( has_lzoe_prefix( fname ) )
        return 1;

#ifdef WIN32
    /* Look for a drive letter */
    if (isalpha(fname[0]) && fname[1] == ':' && fname[2] == PATH_SEP)
        return 1;
#endif

#if defined(__AMIGAOS4__) || defined(WII)
    /* Look for a prefix of the form "VOL:".  Allow everything but the
     * path separator to appear before a ":".  */
    {
        const char *s;

        s = fname;
        while (*s)
        {
            if (*s == PATH_SEP)
                break;

            if (*s == ':')
                return 1;

            s++;
        }
    }
#endif

    return 0;
}

/* ======================================================================== */
/*  PATH_FOPEN   -- Wrapper on fopen() that searches down a path.           */
/*                  Warning:  Don't use this with mode = "w" or "wb".       */
/* ======================================================================== */
LZFILE *path_fopen(path_t *path, const char *fname, const char *mode)
{
    int f_len, b_len;
    char *buf;
    LZFILE *f;

    /* -------------------------------------------------------------------- */
    /*  If the path is empty, or the filename specifies an absolute path,   */
    /*  just do a bare fopen.                                               */
    /* -------------------------------------------------------------------- */
    if (!path || is_absolute_path(fname))
        return lzoe_fopen(fname, mode);

    /* -------------------------------------------------------------------- */
    /*  Dynamically allocate string buffer to avoid overflows.              */
    /* -------------------------------------------------------------------- */
    f_len = strlen(fname);
    b_len = f_len * 2 + 2;
    buf   = CALLOC(char, b_len);

    /* -------------------------------------------------------------------- */
    /*  Check all the path elements.                                        */
    /* -------------------------------------------------------------------- */
    while (path)
    {
        if (b_len < f_len + path->p_len + 2)
        {
            b_len = 2 * (f_len + path->p_len) + 2;
            buf   = REALLOC(buf, char, b_len);
        }

        strcpy(buf, path->name);
        buf[path->p_len] = PATH_SEP;
        strcpy(buf + path->p_len + 1, fname);

        if ((f = lzoe_fopen(buf, mode)) != NULL)
        {
            free(buf);
            return f;
        }

        path = path->next;
    }

    /* -------------------------------------------------------------------- */
    /*  Didn't find it?  Give up.                                           */
    /* -------------------------------------------------------------------- */
    free(buf);

    return NULL;
}

/* ======================================================================== */
/*  EXISTS_IN_PATH -- Looks for file along the given path, returns the      */
/*                    full path if it finds it and it's readable.           */
/* ======================================================================== */
char *exists_in_path(path_t *path, const char *fname)
{
    int f_len, b_len;
    char *buf;

    /* -------------------------------------------------------------------- */
    /*  If the path is empty, just look in CWD.                             */
    /* -------------------------------------------------------------------- */
    if (!path || is_absolute_path(fname))
    {
        if (file_exists(fname))
            return strdup(fname);
        else
            return NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Dynamically allocate string buffer to avoid overflows.              */
    /* -------------------------------------------------------------------- */
    f_len = strlen(fname);
    b_len = f_len * 2 + 2;
    buf   = CALLOC(char, b_len);

    /* -------------------------------------------------------------------- */
    /*  Check all the path elements.                                        */
    /* -------------------------------------------------------------------- */
    while (path)
    {
        if (b_len < f_len + path->p_len + 2)
        {
            b_len = 2 * (f_len + path->p_len) + 2;
            buf   = REALLOC(buf, char, b_len);
        }

        strcpy(buf, path->name);
        buf[path->p_len] = PATH_SEP;
        strcpy(buf + path->p_len + 1, fname);

        if (file_exists(buf))
            return buf;

        path = path->next;
    }

    /* -------------------------------------------------------------------- */
    /*  Didn't find it?  Give up.                                           */
    /* -------------------------------------------------------------------- */
    free(buf);

    return NULL;
}

/* ======================================================================== */
/*  APPEND_PATH  -- Given an existing path, add a new path on the end.      */
/*                  Sure, this will be slow on ridiculously long paths.     */
/* ======================================================================== */
path_t *append_path(path_t *path, const char *fname)
{
    path_t *head = path, **node;
    char *local_fname;

    if (exe_path && fname[0] == '=')
    {
        int l;
        char *n;

        l = strlen(exe_path) + strlen(fname) + 1;

        if (!(n = CALLOC(char, l + 1)))
        {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }

        snprintf(n, l+1, "%s%c%s", exe_path, PATH_SEP, fname + 1);

        local_fname = n;
    } else
    {
        local_fname = strdup(fname);
    }

    for (node = &head; *node; node = &(*node)->next)
        if (!strcmp((*node)->name, local_fname))
        {
            free(local_fname);
            return head;
        }

    *node = CALLOC(path_t, 1);
    (*node)->p_len = strlen(local_fname);
    (*node)->name  = local_fname;

    return head;
}

/* ======================================================================== */
/*  PARSE_PATH_STRING                                                       */
/* ======================================================================== */
path_t *parse_path_string(path_t *path, const char *pstr)
{
    char *str, *p;

    if (!pstr || !strlen(pstr))
        return path;

    /* get writeable local copy for strtok */
    str = strdup(pstr);

    p = strtok(str, PATH_COMPONENT_SEP);
    while (p)
    {
        path = append_path(path, p);
        p = strtok(NULL, PATH_COMPONENT_SEP);
    }

    free(str);  /* dump writeable local copy */
    return path;
}

/* ======================================================================== */
/*  FREE_PATH                                                               */
/* ======================================================================== */
void free_path(path_t *path)
{
    path_t *next;

    if (!path)
        return;

    for (; path; path = next)
    {
        next = path->next;
        CONDFREE(path->name);
        free(path);
    }
}

/* ======================================================================== */
/*  DUMP_SEARCH_PATH                                                        */
/* ======================================================================== */
void dump_search_path(path_t *path)
{
    fprintf(stderr, "\nSearch path:\n");

    while (path)
    {
        fprintf(stderr, "  %s\n", path->name);
        path = path->next;
    }

    fprintf(stderr, "\n");
}

/* ======================================================================== */
/*  MAKE_ABSOLUTE_PATH                                                      */
/*                                                                          */
/*  Given a notion of "current working directory", try to make an absolute  */
/*  a path string.  Always returns a freshly allocated string that must be  */
/*  freed by the caller.                                                    */
/* ======================================================================== */
char *make_absolute_path(const char *cwd, const char *path)
{
    char *new_path;
    int  c_len = strlen(cwd);
    int  p_len = strlen(path);

    if (!is_absolute_path(path))
    {
        new_path = (char *)malloc(c_len + p_len + 2);

        memcpy(new_path,             cwd,  c_len);
        memcpy(new_path + c_len + 1, path, p_len);

        new_path[c_len            ] = PATH_SEP;
        new_path[c_len + p_len + 1] = 0;
    } else
    {
        new_path = strdup(path);
    }

    return new_path;
}

#ifdef PLAT_MACOS
#   include <mach-o/dyld.h>
#endif

#define MAX_CWD_PATH (65535)

#if defined(HAS_READLINK) && defined(HAS_LSTAT)
/* ======================================================================== */
/*  RESOLVE_LINK         -- If path is a symbolic link, return the real     */
/*                          path.  Otherwise just return the path as-is.    */
/*                          Allocates fresh storage for the return value.   */
/*                          Returns NULL on error.                          */
/* ======================================================================== */
LOCAL char *resolve_link( const char *path )
{
    char *rl_buf;
    struct stat s;
    ssize_t rl_len;

    if ( lstat( path, &s ) != 0 )
        return NULL;

    // Not a link, so just return a copy as-is.
    if ( !S_ISLNK(s.st_mode) )
        return strdup(path);

    // Readlink needs us to pass it a buffer
    rl_buf = CALLOC(char, MAX_CWD_PATH);
    if ( !rl_buf )
        return NULL;

    rl_len = readlink(path, rl_buf, MAX_CWD_PATH);

    if ( rl_len < 0 )
    {
        free(rl_buf);
        return NULL;
    }

    // readlink explicitly does NOT terminate with a NUL. ?!?
    rl_buf[rl_len] = 0;

    // Return a path trimmed to size.  If realloc fails, it'll return NULL,
    // so we have that going for us...
    return REALLOC(rl_buf, char, rl_len + 1);
}
#endif

/* ======================================================================== */
/*  GET_EXE_DIR          -- Get directory containing this executable.       */
/*                          Returns pointer to allocated storage.  Will     */
/*                          return NULL if it can't determine directory.    */
/* ======================================================================== */
char *get_exe_dir(const char *const argv0)
{
    char *old_exe_path;
    char *new_exe_path;
    char *s;
#ifdef WIN32
    const int argv0_len = strlen(argv0);
#endif

    /* -------------------------------------------------------------------- */
    /*  Disable exe_path for now, as we reuse the PATH routines above that  */
    /*  do magic things if it's set.                                        */
    /* -------------------------------------------------------------------- */
    old_exe_path = exe_path;
    exe_path = NULL;

    /* -------------------------------------------------------------------- */
    /*  MacOS X: _NSGetExecutablePath supposedly gets *a* path to our       */
    /*  executable.  Then, readlink() should get us the executable itself.  */
    /* -------------------------------------------------------------------- */
#ifdef PLAT_MACOS
    {
        char *gep_buf;
        uint32_t gep_buf_size = MAX_CWD_PATH - 1;

        gep_buf = CALLOC(char, MAX_CWD_PATH);
        if (!gep_buf)
            goto macosx_fail;

        // Try to get the executable path
        if ( _NSGetExecutablePath(gep_buf, &gep_buf_size) != 0 )
            goto macosx_fail;

        // Documentation doesn't say whether path is NUL terminated, to add
        // a NUL terminator just to be sure.
        gep_buf[gep_buf_size] = 0;

        // Resolve the symlinks (if any)
        new_exe_path = resolve_link(gep_buf);

        if (new_exe_path)
        {
            free(gep_buf);
            goto got_exe_path;
        }

macosx_fail:
        CONDFREE(gep_buf);
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  If the system has a /proc/self/exe or similar symlink to the exe,   */
    /*  then try to use that.                                               */
    /* -------------------------------------------------------------------- */
#ifdef PROC_SELF_EXE
    new_exe_path = resolve_link(PROC_SELF_EXE);
    if ( new_exe_path )
        goto got_exe_path;
#endif

    /* -------------------------------------------------------------------- */
    /*  Classical heuristic:  If argv[0] is set, try the following:         */
    /*                                                                      */
    /*   -- If it starts with PATH_SEP, assume it's an absolute path to     */
    /*      the executable.  (If it's Windows, look for drive letter & ':') */
    /*                                                                      */
    /*   -- If it contains PATH_SEP, assume it's a relative path to the     */
    /*      executable.  Concatenate it to CWD to get full path.            */
    /*                                                                      */
    /*   -- Otherwise, examine $PATH looking for PATH/argv0.                */
    /* -------------------------------------------------------------------- */
    if (argv0[0] == PATH_SEP)
    {
        new_exe_path = strdup(argv0);
        goto got_exe_path;
    }

#ifdef WIN32
    if (argv0_len > 3 && isalpha(argv0[0]) && argv0[1] == ':')
    {
        /* ---------------------------------------------------------------- */
        /*  Common case: C:\path\to\jzintv.exe                              */
        /* ---------------------------------------------------------------- */
        if (argv0[2] == PATH_SEP)
        {
            new_exe_path = strdup(argv0);
            goto got_exe_path;
        }

        /* ---------------------------------------------------------------- */
        /*  Ugh: C:foo.exe is relative to CWD on a specific drive that      */
        /*  isn't necessarily the current drive.  Let's just fail for now,  */
        /*  as remaining code doesn't expect this abomination.              */
        /* ---------------------------------------------------------------- */
        goto fail;
    }
#endif

#ifndef NO_GETCWD
    {
        char *cwd_buf = NULL;
        if (strchr(argv0, PATH_SEP))
        {
            cwd_buf = CALLOC(char, MAX_CWD_PATH);
            if (!cwd_buf)
                goto cwd_fail;

            char *cwd = getcwd(cwd_buf, MAX_CWD_PATH - 1);
            if (!cwd)
                goto cwd_fail;

            new_exe_path = make_absolute_path(cwd, argv0);
            free(cwd_buf);
            goto got_exe_path;
        }
cwd_fail:
        CONDFREE(cwd_buf);
    }
#endif

#ifndef NO_PATH_CHECK
    {
        path_t *path = NULL;
        const char *pstr = getenv("PATH");
        char *found = NULL;

        if (!pstr)
            goto path_fail;

        path = parse_path_string(NULL, pstr);
        if (!path)
            goto path_fail;

        found = exists_in_path(path, argv0);
        if (found)
        {
            free_path(path);
            new_exe_path = found;
            goto got_exe_path;
        }
path_fail:
        free_path(path);
        CONDFREE(found);
    }
#endif

fail:
    /* -------------------------------------------------------------------- */
    /*  Done borrowing exe_path; restore it.                                */
    /* -------------------------------------------------------------------- */
    exe_path = old_exe_path;

#ifdef WIN32
    /* -------------------------------------------------------------------- */
    /*  If the argv0 filename didn't end in ".exe", add it and try again.   */
    /* -------------------------------------------------------------------- */
    if (argv0_len > 5 && stricmp(argv0 + argv0_len - 4, ".exe") != 0)
    {
        char *with_exe = malloc(argv0_len + 5);
        if (with_exe)
        {
            memcpy(with_exe, argv0, argv0_len);
            memcpy(with_exe + argv0_len, ".exe", 5);
            new_exe_path = get_exe_dir(with_exe);
            free(with_exe);
            return new_exe_path;
        }
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  Return failure.  Bummer, dude.                                      */
    /* -------------------------------------------------------------------- */
    return NULL;

got_exe_path:
    /* -------------------------------------------------------------------- */
    /*  Done borrowing exe_path; restore it.                                */
    /* -------------------------------------------------------------------- */
    exe_path = old_exe_path;

    /* -------------------------------------------------------------------- */
    /*  Strip off the executable name, keeping only the path.               */
    /*  Possible late failure mode: We can't find PATH_SEP.                 */
    /* -------------------------------------------------------------------- */
    s = strrchr(new_exe_path, PATH_SEP);
    if (!s)
    {
        free(new_exe_path);
        goto fail;
    }
    *s = 0;

    return new_exe_path;
}

/* ======================================================================== */
/*  LOAD_TEXT_FILE                                                          */
/*                                                                          */
/*  Loads a text file into a line-oriented structure.  Attempts to deal     */
/*  with DOS (CR+LF), Mac (CR) and UNIX (LF) newline styles.  Strips off    */
/*  newlines in resulting structure.                                        */
/*                                                                          */
/*  The text_file structure itself contains a pointer to the file body,     */
/*  along with an array of offsets into that file.  Using offsets saves     */
/*  RAM on machines where sizeof(char *) > sizeof(uint32_t).                */
/* ======================================================================== */
text_file *load_text_file(LZFILE *f, int ts)
{
    long len;
    char *body;
    size_t r;
    text_file *tf;
    uint32_t *l_ptr, idx, l_start;
    int lines = 0, l_alloc, got_line = 0;

    /* -------------------------------------------------------------------- */
    /*  Allocate one large buffer for the entire file.                      */
    /* -------------------------------------------------------------------- */
    len = file_length(f);

    if (len > 1 << 30 || len < 1)
        return NULL;

    if (!(body = (char *)malloc(len)))
        return NULL;

    /* -------------------------------------------------------------------- */
    /*  Slurp in the entire file.                                           */
    /* -------------------------------------------------------------------- */
    r = lzoe_fread(body, 1, len, f);
    if ((int)r < len)
    {
        free(body);
        return NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Convert tabs to spaces if asked to do so.                           */
    /* -------------------------------------------------------------------- */
    if (ts > 0)
    {
        int new_len = len;
        int col = 0;
        char *os, *ns;

        /* Initial scan:  Count up how much white space we'll add. */
        if (ts > 1)
            for (idx = 0; idx < (uint32_t)len; idx++)
            {
                int ch = body[idx];

                if (ch == '\t')
                {
                    int tab;

                    tab      = ts - (col % ts);
                    col     += tab;
                    new_len += tab - 1;
                } else
                {
                    col++;
                }

                if (ch == '\012' || ch == '\015')
                    col = 0;
            }

        /* Next, if the file got larger, reallocate the buffer and slide    */
        /* it to the end so we can do this in-place.                        */
        if (new_len > len)
        {
            body = REALLOC(body, char, new_len);
            if (!body)
                return NULL;

            os = body + new_len - len;
            ns = body;
            memmove(os, ns, len);
        } else
        {
            os = ns = body;
        }

        /* Now expand-in-place. */
        col = 0;
        for (idx = 0; idx < (uint32_t)len; idx++)
        {
            int ch = *os++;

            if (ch == '\t')
            {
                int tab;

                tab  = ts - (col % ts);
                col += tab;

                while (tab-- > 0)
                    *ns++ = ' ';
            } else
            {
                col++;
                *ns++ = ch;
            }

            if (ch == '\012' || ch == '\015')
                col = 0;

            assert(ns <= os);
        }

        len = new_len;
    }

    /* -------------------------------------------------------------------- */
    /*  Start with an initial line-pointer buffer.                          */
    /* -------------------------------------------------------------------- */
    l_alloc = 4096;
    l_ptr   = (uint32_t *)malloc(sizeof(uint32_t) * l_alloc);


    /* -------------------------------------------------------------------- */
    /*  Find the line breaks and poke in NULs.                              */
    /* -------------------------------------------------------------------- */
    l_start = 0;
    idx     = 0;
    while (idx < (uint32_t)len)
    {
        if (body[idx] == '\012')    /* LF:  Assume it's UNIX    */
        {
            body[idx] = 0;
            got_line  = 1;
        }

        if (body[idx] == '\015')    /* CR:  Could be DOS or Mac */
        {
            body[idx] = 0;
            if (body[idx + 1] == '\012')    /* Skip LF immediately after CR */
                idx++;
            got_line = 1;
        }

        idx++;

        if (got_line)
        {
            if (lines >= l_alloc)
            {
                l_alloc <<= 1;
                l_ptr   = REALLOC(l_ptr, uint32_t, l_alloc);
            }
            l_ptr[lines++] = l_start;
            l_start  = idx;
            got_line = 0;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Finally, construct the text_file structure.                         */
    /* -------------------------------------------------------------------- */
    tf = CALLOC(text_file, 1);
    tf->body  = body;
    tf->line  = l_ptr;
    tf->lines = lines;

    return tf;
}

/* ======================================================================== */
/*  OPEN_UNIQUE_FILENAME                                                    */
/*  Given a specification, generate a unique filename and open the file.    */
/* ======================================================================== */
FILE *open_unique_filename(unique_filename_t *spec)
{
    uint32_t wrap_thresh = 1;
    int attempts = 0, i;
    FILE *f = NULL;

    if (!spec->f_name)
    {
        /* Assume the index could never be more than ~10 digits */
        uint32_t alloc = strlen(spec->prefix) + strlen(spec->suffix) + 12;
        spec->f_name = CALLOC(char, alloc);
        spec->alloc  = alloc;
    }

    if (spec->num_digits < 1)
        spec->num_digits = 4;

    for (i = 0; i < spec->num_digits; i++)
        wrap_thresh *= 10;

    do
    {
        spec->last_idx++;

        if (spec->last_idx >= wrap_thresh)
        {
            spec->num_digits++;
            wrap_thresh *= 10;
        }

        snprintf(spec->f_name, spec->alloc, "%s%.*d%s",
                 spec->prefix, spec->num_digits, spec->last_idx, spec->suffix);

        if (!file_exists(spec->f_name))
        {
            attempts++;
            f = fopen(spec->f_name, "wb");
        }
    } while (!f && attempts < 10 && spec->last_idx != 0);

    return f;
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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
