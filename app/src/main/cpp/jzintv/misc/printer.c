#include <stdio.h>
#include <stdarg.h>
#include "misc/printer.h"

/* ======================================================================== */
/*  Generic printer wrapper around fprintf(FILE *, ...)                     */
/* ======================================================================== */
static void fprintf_wrap(void *v_file, const char *fmt, ...)
{
    FILE *const file = (FILE *)v_file;
    va_list ap;
    va_start(ap, fmt);
    (void)vfprintf(file, fmt, ap);
    va_end(ap);
}

/* ======================================================================== */
/*  PRINTER_TO_FILE  -- Construct a printer_t that prints to a FILE*.       */
/* ======================================================================== */
printer_t printer_to_file(FILE *const f)
{
    printer_t p = { fprintf_wrap, (void *)f };
    return p;
}

/* ======================================================================== */
/*  Slightly less generic wrappers for stdout/stderr, due to the fact that  */
/*  neither 'stdout' nor 'stderr' is guaranteed to be a compile time        */
/*  constant, but we want printer_to_stdXXX to be init'd at compile time.   */
/* ======================================================================== */
static void stdxxx_wrap(void *v_std, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    FILE *const file = v_std == NULL ? stdout : stderr;
    (void)vfprintf(file, fmt, ap);
    va_end(ap);
}

/* ======================================================================== */
/*  Some prepopulated printer_t for stdout and stderr.                      */
/*  These unfortunately cannot easily be 'const' because they could be      */
/*  passed as opaque (void*) in some places, and that casts away const.     */
/*  Discriminate between stdout/stderr with NULL vs. non-NULL on opq.       */
/* ======================================================================== */
printer_t printer_to_stdout = { stdxxx_wrap, NULL       };
printer_t printer_to_stderr = { stdxxx_wrap, (void *)"" };
