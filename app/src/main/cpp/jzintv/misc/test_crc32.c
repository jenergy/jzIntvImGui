#include <stdio.h>
#include "config.h"
#include "crc32.h"

main()
{
    uint32_t crc_8  = 0xFFFFFFFF;
    uint32_t crc_16 = 0xFFFFFFFF;
    uint32_t crc_32 = 0xFFFFFFFF;
    uint32_t word = 0;
    int c, i = 0;

    while ((c = getchar()) != EOF)
    {
        word = (word >> 8) | ((0xFF & c) << 24);
        crc_8 = crc32_update(crc_8, (word >> 24) & 0xFF);
        if ((i & 1) == 1)
            crc_16 = crc32_upd16 (crc_16, (word >> 16) & 0xFFFF);
        if ((i & 3) == 3)
            crc_32 = crc32_upd32 (crc_32, word);
        i++;
    }

    switch (i & 3)
    {
        case 0: break;
        case 1:
        {
            crc_16 = crc32_update(crc_16, (word >> 24) & 0xFF);
            crc_32 = crc32_update(crc_32, (word >> 24) & 0xFF);
            break;
        }
        case 2:
        {
            crc_32 = crc32_upd16 (crc_32, (word >> 16) & 0xFFFF);
            break;
        }
        case 3:
        {
            crc_16 = crc32_update(crc_16, (word >> 24) & 0xFF);
            crc_32 = crc32_upd16 (crc_32, (word >>  8) & 0xFFFF);
            crc_32 = crc32_update(crc_32, (word >> 24) & 0xFF);
            break;
        }
    }

    crc_8  = crc_8  ^ 0xFFFFFFFF;
    crc_16 = crc_16 ^ 0xFFFFFFFF;
    crc_32 = crc_32 ^ 0xFFFFFFFF;

    printf("CRC = %.8X, %.8X, %.8X\n", crc_8, crc_16, crc_32);

    return 0;
}
