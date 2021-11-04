/* ======================================================================== */
/*  Print the metadata in an Intellicart ROM.                               */
/* ======================================================================== */
#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "icart/icartrom.h"
#include "icart/icarttag.h"


int main(int argc, char *argv[])
{
    LZFILE *f;
    size_t flen, r;
    uint8_t *data;
    int visitor_error = 0;
    int decode_result = 0;

    if (argc != 2)
    {
        fprintf(stderr, "rom_metadata foo.rom\n");
        exit(1);
    }

    f = lzoe_fopen(argv[1], "rb");
    if (!f)
    {
        perror("fopen");
        fprintf(stderr, "Could not open %s\n", argv[1]);
        exit(1);
    }

    flen = file_length(f);

    if (flen < 53)
    {
        fprintf(stderr, "Short file? %d bytes\n", (int)flen);
        exit(1);
    }

    data = CALLOC(uint8_t, flen);
    if (!data)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    if ((r = lzoe_fread(data, 1, flen, f)) != flen)
    {
        fprintf(stderr, "Tried to read %d bytes, got %d\n", (int)flen, (int)r);
        exit(1);
    }
    lzoe_fclose(f);

    decode_result = icarttag_decode(data, flen, 0, -1, &ict_printer,
                                    &visitor_error);

    if (decode_result < 0)
        fprintf(stderr, "Decoder error %d\n", decode_result);
    else
        printf("Processed %d of %d bytes\n", (int)decode_result, (int)flen);

    if (visitor_error)
        printf("Visitor error: %d\n", visitor_error);

    free(data);
    return 0;
}
