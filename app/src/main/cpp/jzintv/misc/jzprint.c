/* ======================================================================== */
/*  JZPRINT -- Wrapper around printf() to vector stdout as appropriate.     */
/* ======================================================================== */


#include "config.h"
#include "misc/printer.h"
#include <stdio.h>
#include <stdarg.h>

int    jzp_silent = 0;
FILE  *jzp_stdout = NULL;
int  (*jzp_vprintf)(void *arg, const char *fmt, va_list ap) = NULL;
void  *jzp_vprintf_arg = NULL;

LOCAL int jzp_printf_impl(const char *fmt, va_list ap)
{
    int retval = 0;

    if (jzp_silent)
        return strlen(fmt); /* non-zero and plausible */

    if (jzp_stdout)
        retval = vfprintf(jzp_stdout, fmt, ap);

    if (jzp_vprintf)
        retval = jzp_vprintf(jzp_vprintf_arg, fmt, ap);

    if (!retval)
        retval = strlen(fmt);

    return retval;
}

int jzp_printf(const char *fmt, ...)
{
    va_list ap;
    int retval;
    va_start(ap, fmt);
    retval = jzp_printf_impl(fmt, ap);
    va_end(ap);
    return retval;
}


void jzp_flush(void)
{
    if (!jzp_silent && jzp_stdout)
        fflush(jzp_stdout);
}

void jzp_clear_and_eol(int cur_len)
{
    int w = get_disp_width();
    jzp_printf("%*s", cur_len < w ? w - cur_len - 1: 0, "\n");
}

void jzp_init
(
    int silent,
    FILE *fout,
    int  (*fn)(void *, const char *, va_list),
    void *fn_arg
)
{
    jzp_silent  = silent;
    jzp_stdout  = fout;
    jzp_vprintf = fn;
    jzp_vprintf_arg = fn_arg;
}

LOCAL void jzp_printf_wrap(void *unused, const char *fmt, ...)
{
    va_list ap;
    UNUSED(unused);
    va_start(ap, fmt);
    jzp_printf_impl(fmt, ap);
    va_end(ap);
}

LOCAL printer_t jzp_printer_adaptor = { jzp_printf_wrap, NULL };

struct printer_t *jzp_printer(void)
{
    return &jzp_printer_adaptor;
}
