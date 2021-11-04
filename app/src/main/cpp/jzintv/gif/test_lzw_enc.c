/* A really simple GIF encoder, intended to test lzw_enc. */

#include "config.h"
#include "gif/lzw_enc.h"


uint8_t gif_header[] =
{
    /* signature */
    'G','I','F','8','7','a',

    /* image dimensions (160x200 */
    160, 0,
    200, 0,

    0xF7,       /* GlobalColor, 8-bpp CLUT, 8-bpp image */
    0,          /* Background color is index 0 */
    0,          /* Aspect ratio = 0 */

};


/* local descriptor */
uint8_t gif_local[] =
{
    0x2C,       /* signature */
    0, 0, 0, 0, /* Anchor at upper left */
    160, 0,     /* width 160 */
    200, 0,     /* height 200 */
    0,          /* no local colors */
};


uint8_t gif_file[65536];

uint8_t image[200*160];

int main(void)
{
    FILE *f;
    int x, y, i;
    uint8_t *gif_ptr = gif_file;

    for (y = 0; y < 200; y++)
        for (x = 0; x < 160; x++)
            image[x + y*160] = (x ^ y) & 1 ? 1 : 0;

    /* copy over GIF header */
    memcpy(gif_ptr, gif_header, sizeof(gif_header));
    gif_ptr += sizeof(gif_header);

    /* put in global color table, greyscale */
    for (i = 0; i < 256; i++)
    {
        *gif_ptr++ = i & 1 ? 255 : 0;
        *gif_ptr++ = i & 1 ? 255 : 0;
        *gif_ptr++ = i & 1 ? 255 : 0;
    }



    /* put local header */
    memcpy(gif_ptr, gif_local, sizeof(gif_local));
    gif_ptr += sizeof(gif_local);

    /* now compress the image */
    i = lzw_encode(image, gif_ptr, 200*160, 65536-10-13-768-2);

    if (i < 0)
    {
        printf("compression overflow?\n");
        exit(1);
    }

    gif_ptr += i;
    *gif_ptr++ = 0x3B;  /* image terminator */

    f = fopen("test.gif", "wb");
    fwrite(gif_file, 1, gif_ptr - gif_file, f);
    fclose(f);

    return 0;
}
