#ifndef MISC_PRINTER_H_
#define MISC_PRINTER_H_

/* ======================================================================== */
/*  PRINTER_T                                                               */
/*  Printing abstraction to allow printing to files or other output devs.   */
/* ======================================================================== */
typedef struct printer_t
{
    void (*fxn)(void *, const char *, ...);     /* Printing function */
    void *opq;                                  /* Opaque data for printer. */
} printer_t;

extern printer_t printer_to_stdout;
extern printer_t printer_to_stderr;

/* ======================================================================== */
/*  PRINTER_TO_FILE  -- Construct a printer_t that prints to a FILE*.       */
/* ======================================================================== */
printer_t printer_to_file(FILE *const f);

#endif

