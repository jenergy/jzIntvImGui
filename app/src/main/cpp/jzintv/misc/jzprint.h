/* ======================================================================== */
/*  JZPRINT -- Wrapper around printf() to vector stdout as appropriate.     */
/* ======================================================================== */

#ifndef JZPRINT_H_
#define JZPRINT_H_

#include <stdarg.h>

extern int    jzp_silent;
extern FILE  *jzp_stdout;
extern int  (*jzp_vprintf)(void *arg, const char *fmt, va_list ap);
extern void  *jzp_vprintf_arg;
extern int    jzp_printf(const char *fmt, ...);
extern void   jzp_flush(void);
extern void   jzp_clear_and_eol(int cur_len);

extern void jzp_init
(
    int silent,
    FILE *fout,
    int  (*fn)(void *, const char *, va_list),
    void *fn_arg
);

/*  Return a printer_t that does jzp_printf().  Use a forward decl here to  */
/*  to avoid introducing a new compile dependency.                          */
struct printer_t;
struct printer_t *jzp_printer(void);

#endif

