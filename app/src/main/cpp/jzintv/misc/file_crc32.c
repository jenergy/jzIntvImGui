#include "config.h"
#include "misc/crc32.h"
#include "misc/file_crc32.h"
#include "lzoe/lzoe.h"

/* ======================================================================== */
/*  FILE_CRC32   -- Return the CRC-32 for a file.                           */
/* ======================================================================== */
uint32_t file_crc32(const char *fname)
{
    LZFILE *f;
    uint32_t crc = 0xFFFFFFFFU;
    int c;

    if (fname == NULL)
        f = (LZFILE *)stdin;  // so hack, much wow.
    else if (!(f = lzoe_fopen(fname, "rb")))
        return crc;

    while ((c = lzoe_fgetc(f)) != EOF)
        crc = crc32_update(crc, c & 0xFF);

    if (fname)
        lzoe_fclose(f);

    return crc ^ 0xFFFFFFFFU;
}
