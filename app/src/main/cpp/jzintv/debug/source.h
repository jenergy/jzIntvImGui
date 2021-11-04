/* ======================================================================== */
/*  Routines for managing source code in the debugger.                      */
/* ======================================================================== */
#ifndef SOURCE_H_
#define SOURCE_H_

typedef struct source_file_info
{
    const char *name;
    text_file  *text;
} source_file_info;

extern source_file_info *source_file;
extern int               source_files;

typedef enum
{
    SMAP_SMART,     /* detect multi-line macros and pull from listing */
    SMAP_SOURCE,
    SMAP_LISTING
} smap_mode;

void        set_source_map_mode(smap_mode mode);

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
void process_source_map(const char *fname);

/* ======================================================================== */
/*  SOURCE_FOR_ADDR      -- Try to find code for this address, if any       */
/*                          exists.  Pull from source or listing based on   */
/*                          the current mode.                               */
/* ======================================================================== */
const char *source_for_addr(uint32_t addr);

/* ======================================================================== */
/*  FILE_LINE_FOR_ADDR   -- Get the file and line associated with an addr.  */
/* ======================================================================== */
int file_line_for_addr(uint32_t addr, int *line);

/* ======================================================================== */
/*  SOURCE_FOR_FILE_LINE -- Get the source line associated with the file    */
/*                          handle and source line, if any.                 */
/* ======================================================================== */
const char *source_for_file_line(int file, int line);

#endif
