/* ======================================================================== */
/*  CRC32 -- Wrapper program around file_crc32() that calculates and shows  */
/*           CRC values for files.                                          */
/* ======================================================================== */

#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "misc/crc32.h"
#include "misc/file_crc32.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s file [file [file [...]]]\n", argv[0]);
        exit(1);
    }

    while (0<--argc)
    {
        uint32_t crc = file_crc32(*++argv);
        printf("%.8X  %s\n", crc, *argv);
    }

    return 0;
}
