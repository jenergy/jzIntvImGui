/* ======================================================================== */
/*  GFX_SCALE -- scaling functions for building up the tiny jzIntv image    */
/*                                                                          */
/*  The scalers are broken up into two groups:  horizontal and vertical.    */
/*  The horizontal scalers generate a single row of output to a scratch     */
/*  buffer.  The vertical scalers then replicate the scratch buffer some    */
/*  number of times to a rectangle in the output surface.                   */
/*                                                                          */
/*  The horizontal scalers do not know anything about color depth.  Rather, */
/*  they work from a simple uint32_t palette, storing out uint32_t's for    */
/*  each pixel.  The palette can be set up for 8bpp, 16bpp or 32bpp as long */
/*  as it's replicated out to the fill size used  by the horizontal scaler. */
/*                                                                          */
/*  The hscale kernels assume the number of inputs are a multiple of 8.     */
/*  Hscale factors below 4 do not use a palette.                            */
/* ======================================================================== */

#include "config.h"
#include "periph/periph.h"
#include "gfx/gfx.h"
#include "gfx/gfx_scale.h"

#define R RESTRICT


#ifdef __GNUC__
#define ALIGN16 __attribute__((aligned(16)))
#else
#define ALIGN16
#endif


#ifdef BYTE_BE

LOCAL INLINE uint32_t pack8_32(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3)
{
    return ((uint32_t)p0 << 24) | ((uint32_t)p1 << 16) 
         | ((uint32_t)p2 << 8) | p3;
}

LOCAL INLINE uint32_t merge16_32(uint32_t p0, uint32_t p1)
{
    return (0xFFFF0000 & p0) | (0x0000FFFF & p1);
}

LOCAL INLINE uint32_t pack_AAAA(uint8_t p0)
{
    return p0 * 0x01010101;
}

LOCAL INLINE uint32_t pack_AAAB(uint8_t p0, uint8_t p1)
{
    return p0 * 0x01010100 + p1 * 0x00000001;
}

LOCAL INLINE uint32_t pack_AABB(uint8_t p0, uint8_t p1)
{
    return p0 * 0x01010000 + p1 * 0x00000101;
}

LOCAL INLINE uint32_t pack_ABBB(uint8_t p0, uint8_t p1)
{
    return p0 * 0x01000000 + p1 * 0x00010101;
}

LOCAL INLINE uint32_t pack_ABBC(uint8_t p0, uint8_t p1, uint8_t p2)
{
    return p0 * 0x01000000 + p1 * 0x00010100 + p2;
}

LOCAL INLINE uint32_t pack_AABC(uint8_t p0, uint8_t p1, uint8_t p2)
{
    return p0 * 0x01010000 + p1 * 0x00000100 + p2;
}

LOCAL INLINE uint32_t pack_ABCC(uint8_t p0, uint8_t p1, uint8_t p2)
{
    return p0 * 0x01000000 + p1 * 0x00010000 + p2 * 0x00000101;
}


#else /* BYTE_LE */

LOCAL INLINE uint32_t merge16_32(uint32_t p0, uint32_t p1)
{
    return (0xFFFF0000 & p1) | (0x0000FFFF & p0);
}

LOCAL INLINE uint32_t pack_AAAA(uint8_t p0)
{
    return p0 * 0x01010101;
}

LOCAL INLINE uint32_t pack_AAAB(uint8_t p0, uint8_t p1)
{
    return p0 * 0x00010101 + p1 * 0x01000000;
}

LOCAL INLINE uint32_t pack_AABB(uint8_t p0, uint8_t p1)
{
    return p0 * 0x00000101 + p1 * 0x01010000;
}

LOCAL INLINE uint32_t pack_ABBB(uint8_t p0, uint8_t p1)
{
    return p0 * 0x00000001 + p1 * 0x01010100;
}

LOCAL INLINE uint32_t pack_ABBC(uint8_t p0, uint8_t p1, uint8_t p2)
{
    return p0 * 0x00000001 + p1 * 0x00010100 + p2 * 0x01000000;
}

LOCAL INLINE uint32_t pack_AABC(uint8_t p0, uint8_t p1, uint8_t p2)
{
    return p0 * 0x00000101 + p1 * 0x00010000 + p2 * 0x01000000;
}

LOCAL INLINE uint32_t pack_ABCC(uint8_t p0, uint8_t p1, uint8_t p2)
{
    return p0 * 0x00000001 + p1 * 0x00000100 + p2 * 0x01010000;
}

#endif


TARGET_CLONES(LOCAL void gfx_hscale_1_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    memcpy((void *)hs_buf_ptr, (const void *)src, cnt);
}

TARGET_CLONES(LOCAL void gfx_hscale_1_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++];
        p1 = src[i++];
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABC(p0, p1, p2);
        p3 = src[i++];
        p4 = src[i++]; hs_buf_ptr[j++] = pack_ABCC(p2, p3, p4);
        p5 = src[i++];
        p6 = src[i++];
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBC(p5, p6, p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_2_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++];
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p0, p1);
        p2 = src[i++];
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
        p4 = src[i++];
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
        p6 = src[i++];
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_2_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;


    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++];
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
        p3 = src[i++];
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
        p5 = src[i++];
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBC(p4, p5, p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_3_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++];
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p1, p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p2, p3);
        p4 = src[i++];
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p5, p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_3_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++];
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p1, p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p2, p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p5, p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_4_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_4_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p0, p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_5_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p0, p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p1, p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p4, p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p5, p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_5_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p0, p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p2, p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p5, p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_6_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_6_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p4, p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_7_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_7_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_8_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_8_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_9_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_9_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_10_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_10_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_11_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_11_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_12_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_12_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_13_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_13_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_14_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_14_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_15_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_15_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p6, p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_16_0x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_16_5x_np(uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt))
{
    int i, j;
    uint8_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
                       hs_buf_ptr[j++] = pack_AAAA(p0);
        p1 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p0, p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
                       hs_buf_ptr[j++] = pack_AAAA(p1);
        p2 = src[i++]; hs_buf_ptr[j++] = pack_ABBB(p1, p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
                       hs_buf_ptr[j++] = pack_AAAA(p2);
        p3 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p2, p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
                       hs_buf_ptr[j++] = pack_AAAA(p3);
        p4 = src[i++]; hs_buf_ptr[j++] = pack_AABB(p3, p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
                       hs_buf_ptr[j++] = pack_AAAA(p4);
        p5 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p4, p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
                       hs_buf_ptr[j++] = pack_AAAA(p5);
        p6 = src[i++]; hs_buf_ptr[j++] = pack_AAAB(p5, p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
                       hs_buf_ptr[j++] = pack_AAAA(p6);
        p7 = src[i++]; hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
                       hs_buf_ptr[j++] = pack_AAAA(p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_1_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_1_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_2_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_2_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_3_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_3_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_4_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_4_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_5_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_5_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_6_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_6_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_7_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_7_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_8_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_8_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_9_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_9_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_10_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_10_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_11_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_11_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_12_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_12_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_13_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_13_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_14_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_14_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_15_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_15_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_16_0x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_16_5x_p (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_1_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]];
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
        p2 = pal[src[i++]];
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
        p4 = pal[src[i++]];
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
        p6 = pal[src[i++]];
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_1_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]];
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]];
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_2_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_3_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_3_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_4_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_5_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_5_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_6_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_7_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_7_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_8_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_9_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_9_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_10_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_11_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_11_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_12_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_13_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_13_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_14_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_15_0x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

TARGET_CLONES(LOCAL void gfx_hscale_15_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p2, p3);
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p6, p7);
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}


TARGET_CLONES(LOCAL void gfx_hscale_16_5x_16 (uint32_t *R hs_buf_ptr,
                              const uint8_t  *R src,
                              int cnt,
                              const uint32_t *R pal))
{
    int i, j;
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;

    i = j = 0;
    cnt >>= 3;
    assert(cnt >= 20);
    while (cnt-- != 0)
    {
        p0 = pal[src[i++]]; hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
                            hs_buf_ptr[j++] = p0;
        p1 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p0, p1);
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
                            hs_buf_ptr[j++] = p1;
        p2 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p1, p2);
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
                            hs_buf_ptr[j++] = p2;
        p3 = pal[src[i++]]; hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
                            hs_buf_ptr[j++] = p3;
        p4 = pal[src[i++]]; hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
                            hs_buf_ptr[j++] = p4;
        p5 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p4, p5);
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
                            hs_buf_ptr[j++] = p5;
        p6 = pal[src[i++]]; hs_buf_ptr[j++] = merge16_32(p5, p6);
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
                            hs_buf_ptr[j++] = p6;
        p7 = pal[src[i++]]; hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
                            hs_buf_ptr[j++] = p7;
    }
}

/* ======================================================================== */
/*  "Extended" horizontal scale modes that stack gfx_hscale_2_0x_xx, or     */
/*  gfx_hscale_4_0x_xx on other scalers to get scale factors up to 66x.     */
/* ======================================================================== */
LOCAL uint32_t iscale_buf[4096] ALIGN16;

#define HS_TWICE_NP(sf,sa,sb) \
    TARGET_CLONES(                                                          \
    LOCAL void gfx_hscale_##sf##_0x_np(uint32_t *R hs_buf_ptr,              \
                                        const uint8_t  *R src,              \
                                        int cnt))                           \
    {                                                                       \
        gfx_hscale_##sa##_0x_np(iscale_buf, src, cnt);                      \
        gfx_hscale_##sb##x_np(hs_buf_ptr, (uint8_t*)iscale_buf, cnt * 2);   \
    }

#define HS_TWICE(m,sf,sa,sb) \
    TARGET_CLONES(                                                           \
    LOCAL void gfx_hscale_##sf##_0x_##m(uint32_t *R hs_buf_ptr,              \
                                        const uint8_t  *R src,               \
                                        int cnt,                             \
                                        const uint32_t *R pal))              \
    {                                                                        \
        gfx_hscale_##sa##_0x_np(iscale_buf, src, cnt);                       \
        gfx_hscale_##sb##x_##m(hs_buf_ptr, (uint8_t*)iscale_buf, cnt*2,pal); \
    }

HS_TWICE_NP(17, 2,  8_5)     HS_TWICE_NP(34, 4,  8_5)
HS_TWICE_NP(18, 2,  9_0)     HS_TWICE_NP(36, 4,  9_0)
HS_TWICE_NP(19, 2,  9_5)     HS_TWICE_NP(38, 4,  9_5)
HS_TWICE_NP(20, 2, 10_0)     HS_TWICE_NP(40, 4, 10_0)
HS_TWICE_NP(21, 2, 10_5)     HS_TWICE_NP(42, 4, 10_5)
HS_TWICE_NP(22, 2, 11_0)     HS_TWICE_NP(44, 4, 11_0)
HS_TWICE_NP(23, 2, 11_5)     HS_TWICE_NP(46, 4, 11_5)
HS_TWICE_NP(24, 2, 12_0)     HS_TWICE_NP(48, 4, 12_0)
HS_TWICE_NP(25, 2, 12_5)     HS_TWICE_NP(50, 4, 12_5)
HS_TWICE_NP(26, 2, 13_0)     HS_TWICE_NP(52, 4, 13_0)
HS_TWICE_NP(27, 2, 13_5)     HS_TWICE_NP(54, 4, 13_5)
HS_TWICE_NP(28, 2, 14_0)     HS_TWICE_NP(56, 4, 14_0)
HS_TWICE_NP(29, 2, 14_5)     HS_TWICE_NP(58, 4, 14_5)
HS_TWICE_NP(30, 2, 15_0)     HS_TWICE_NP(60, 4, 15_0)
HS_TWICE_NP(31, 2, 15_5)     HS_TWICE_NP(62, 4, 15_5)
HS_TWICE_NP(32, 2, 16_0)     HS_TWICE_NP(64, 4, 16_0)
HS_TWICE_NP(33, 2, 16_5)     HS_TWICE_NP(66, 4, 16_5)

HS_TWICE(p,  17, 2,  8_5)    HS_TWICE(p,  34, 4,  8_5)
HS_TWICE(p,  18, 2,  9_0)    HS_TWICE(p,  36, 4,  9_0)
HS_TWICE(p,  19, 2,  9_5)    HS_TWICE(p,  38, 4,  9_5)
HS_TWICE(p,  20, 2, 10_0)    HS_TWICE(p,  40, 4, 10_0)
HS_TWICE(p,  21, 2, 10_5)    HS_TWICE(p,  42, 4, 10_5)
HS_TWICE(p,  22, 2, 11_0)    HS_TWICE(p,  44, 4, 11_0)
HS_TWICE(p,  23, 2, 11_5)    HS_TWICE(p,  46, 4, 11_5)
HS_TWICE(p,  24, 2, 12_0)    HS_TWICE(p,  48, 4, 12_0)
HS_TWICE(p,  25, 2, 12_5)    HS_TWICE(p,  50, 4, 12_5)
HS_TWICE(p,  26, 2, 13_0)    HS_TWICE(p,  52, 4, 13_0)
HS_TWICE(p,  27, 2, 13_5)    HS_TWICE(p,  54, 4, 13_5)
HS_TWICE(p,  28, 2, 14_0)    HS_TWICE(p,  56, 4, 14_0)
HS_TWICE(p,  29, 2, 14_5)    HS_TWICE(p,  58, 4, 14_5)
HS_TWICE(p,  30, 2, 15_0)    HS_TWICE(p,  60, 4, 15_0)
HS_TWICE(p,  31, 2, 15_5)    HS_TWICE(p,  62, 4, 15_5)
HS_TWICE(p,  32, 2, 16_0)    HS_TWICE(p,  64, 4, 16_0)
HS_TWICE(p,  33, 2, 16_5)    HS_TWICE(p,  66, 4, 16_5)

#define gfx_hscale_2_0x_16  gfx_hscale_1_0x_p
#define gfx_hscale_4_0x_16  gfx_hscale_2_0x_p
#define gfx_hscale_6_0x_16  gfx_hscale_3_0x_p
#define gfx_hscale_8_0x_16  gfx_hscale_4_0x_p
#define gfx_hscale_10_0x_16 gfx_hscale_5_0x_p
#define gfx_hscale_12_0x_16 gfx_hscale_6_0x_p
#define gfx_hscale_14_0x_16 gfx_hscale_7_0x_p
#define gfx_hscale_16_0x_16 gfx_hscale_8_0x_p

HS_TWICE(16, 17, 2,  8_5)    HS_TWICE(16, 34, 4,  8_5)
HS_TWICE(16, 18, 2,  9_0)    HS_TWICE(16, 36, 4,  9_0)
HS_TWICE(16, 19, 2,  9_5)    HS_TWICE(16, 38, 4,  9_5)
HS_TWICE(16, 20, 2, 10_0)    HS_TWICE(16, 40, 4, 10_0)
HS_TWICE(16, 21, 2, 10_5)    HS_TWICE(16, 42, 4, 10_5)
HS_TWICE(16, 22, 2, 11_0)    HS_TWICE(16, 44, 4, 11_0)
HS_TWICE(16, 23, 2, 11_5)    HS_TWICE(16, 46, 4, 11_5)
HS_TWICE(16, 24, 2, 12_0)    HS_TWICE(16, 48, 4, 12_0)
HS_TWICE(16, 25, 2, 12_5)    HS_TWICE(16, 50, 4, 12_5)
HS_TWICE(16, 26, 2, 13_0)    HS_TWICE(16, 52, 4, 13_0)
HS_TWICE(16, 27, 2, 13_5)    HS_TWICE(16, 54, 4, 13_5)
HS_TWICE(16, 28, 2, 14_0)    HS_TWICE(16, 56, 4, 14_0)
HS_TWICE(16, 29, 2, 14_5)    HS_TWICE(16, 58, 4, 14_5)
HS_TWICE(16, 30, 2, 15_0)    HS_TWICE(16, 60, 4, 15_0)
HS_TWICE(16, 31, 2, 15_5)    HS_TWICE(16, 62, 4, 15_5)
HS_TWICE(16, 32, 2, 16_0)    HS_TWICE(16, 64, 4, 16_0)
HS_TWICE(16, 33, 2, 16_5)    HS_TWICE(16, 66, 4, 16_5)



/* ======================================================================== */
/*  Lookup tables for all this fun...                                       */
/* ======================================================================== */

LOCAL gfx_hscale_np_t *const gfx_hscale_np[] =
{
    gfx_hscale_1_0x_np,
    gfx_hscale_1_5x_np,
    gfx_hscale_2_0x_np,
    gfx_hscale_2_5x_np,
    gfx_hscale_3_0x_np,
    gfx_hscale_3_5x_np,
    gfx_hscale_4_0x_np,
    gfx_hscale_4_5x_np,
    gfx_hscale_5_0x_np,
    gfx_hscale_5_5x_np,
    gfx_hscale_6_0x_np,
    gfx_hscale_6_5x_np,
    gfx_hscale_7_0x_np,
    gfx_hscale_7_5x_np,
    gfx_hscale_8_0x_np,
    gfx_hscale_8_5x_np,
    gfx_hscale_9_0x_np,
    gfx_hscale_9_5x_np,
    gfx_hscale_10_0x_np,
    gfx_hscale_10_5x_np,
    gfx_hscale_11_0x_np,
    gfx_hscale_11_5x_np,
    gfx_hscale_12_0x_np,
    gfx_hscale_12_5x_np,
    gfx_hscale_13_0x_np,
    gfx_hscale_13_5x_np,
    gfx_hscale_14_0x_np,
    gfx_hscale_14_5x_np,
    gfx_hscale_15_0x_np,
    gfx_hscale_15_5x_np,
    gfx_hscale_16_0x_np,
    gfx_hscale_16_5x_np,

    gfx_hscale_17_0x_np, NULL,
    gfx_hscale_18_0x_np, NULL,
    gfx_hscale_19_0x_np, NULL,
    gfx_hscale_20_0x_np, NULL,

    gfx_hscale_21_0x_np, NULL,
    gfx_hscale_22_0x_np, NULL,
    gfx_hscale_23_0x_np, NULL,
    gfx_hscale_24_0x_np, NULL,

    gfx_hscale_25_0x_np, NULL,
    gfx_hscale_26_0x_np, NULL,
    gfx_hscale_27_0x_np, NULL,
    gfx_hscale_28_0x_np, NULL,

    gfx_hscale_29_0x_np, NULL,
    gfx_hscale_30_0x_np, NULL,
    gfx_hscale_31_0x_np, NULL,
    gfx_hscale_32_0x_np, NULL,

    gfx_hscale_33_0x_np, NULL,


    gfx_hscale_34_0x_np, NULL, NULL, NULL,
    gfx_hscale_36_0x_np, NULL, NULL, NULL,
    gfx_hscale_38_0x_np, NULL, NULL, NULL,
    gfx_hscale_40_0x_np, NULL, NULL, NULL,

    gfx_hscale_42_0x_np, NULL, NULL, NULL,
    gfx_hscale_44_0x_np, NULL, NULL, NULL,
    gfx_hscale_46_0x_np, NULL, NULL, NULL,
    gfx_hscale_48_0x_np, NULL, NULL, NULL,

    gfx_hscale_50_0x_np, NULL, NULL, NULL,
    gfx_hscale_52_0x_np, NULL, NULL, NULL,
    gfx_hscale_54_0x_np, NULL, NULL, NULL,
    gfx_hscale_56_0x_np, NULL, NULL, NULL,

    gfx_hscale_58_0x_np, NULL, NULL, NULL,
    gfx_hscale_60_0x_np, NULL, NULL, NULL,
    gfx_hscale_62_0x_np, NULL, NULL, NULL,
    gfx_hscale_64_0x_np, NULL, NULL, NULL,

    gfx_hscale_66_0x_np, NULL, NULL, NULL
};

LOCAL gfx_hscale_p_t *const gfx_hscale_p[] =
{
    gfx_hscale_1_0x_p,
    gfx_hscale_1_5x_p,
    gfx_hscale_2_0x_p,
    gfx_hscale_2_5x_p,
    gfx_hscale_3_0x_p,
    gfx_hscale_3_5x_p,
    gfx_hscale_4_0x_p,
    gfx_hscale_4_5x_p,
    gfx_hscale_5_0x_p,
    gfx_hscale_5_5x_p,
    gfx_hscale_6_0x_p,
    gfx_hscale_6_5x_p,
    gfx_hscale_7_0x_p,
    gfx_hscale_7_5x_p,
    gfx_hscale_8_0x_p,
    gfx_hscale_8_5x_p,
    gfx_hscale_9_0x_p,
    gfx_hscale_9_5x_p,
    gfx_hscale_10_0x_p,
    gfx_hscale_10_5x_p,
    gfx_hscale_11_0x_p,
    gfx_hscale_11_5x_p,
    gfx_hscale_12_0x_p,
    gfx_hscale_12_5x_p,
    gfx_hscale_13_0x_p,
    gfx_hscale_13_5x_p,
    gfx_hscale_14_0x_p,
    gfx_hscale_14_5x_p,
    gfx_hscale_15_0x_p,
    gfx_hscale_15_5x_p,
    gfx_hscale_16_0x_p,
    gfx_hscale_16_5x_p,

    gfx_hscale_17_0x_p, NULL,
    gfx_hscale_18_0x_p, NULL,
    gfx_hscale_19_0x_p, NULL,
    gfx_hscale_20_0x_p, NULL,

    gfx_hscale_21_0x_p, NULL,
    gfx_hscale_22_0x_p, NULL,
    gfx_hscale_23_0x_p, NULL,
    gfx_hscale_24_0x_p, NULL,

    gfx_hscale_25_0x_p, NULL,
    gfx_hscale_26_0x_p, NULL,
    gfx_hscale_27_0x_p, NULL,
    gfx_hscale_28_0x_p, NULL,

    gfx_hscale_29_0x_p, NULL,
    gfx_hscale_30_0x_p, NULL,
    gfx_hscale_31_0x_p, NULL,
    gfx_hscale_32_0x_p, NULL,

    gfx_hscale_33_0x_p, NULL,


    gfx_hscale_34_0x_p, NULL, NULL, NULL,
    gfx_hscale_36_0x_p, NULL, NULL, NULL,
    gfx_hscale_38_0x_p, NULL, NULL, NULL,
    gfx_hscale_40_0x_p, NULL, NULL, NULL,

    gfx_hscale_42_0x_p, NULL, NULL, NULL,
    gfx_hscale_44_0x_p, NULL, NULL, NULL,
    gfx_hscale_46_0x_p, NULL, NULL, NULL,
    gfx_hscale_48_0x_p, NULL, NULL, NULL,

    gfx_hscale_50_0x_p, NULL, NULL, NULL,
    gfx_hscale_52_0x_p, NULL, NULL, NULL,
    gfx_hscale_54_0x_p, NULL, NULL, NULL,
    gfx_hscale_56_0x_p, NULL, NULL, NULL,

    gfx_hscale_58_0x_p, NULL, NULL, NULL,
    gfx_hscale_60_0x_p, NULL, NULL, NULL,
    gfx_hscale_62_0x_p, NULL, NULL, NULL,
    gfx_hscale_64_0x_p, NULL, NULL, NULL,

    gfx_hscale_66_0x_p, NULL, NULL, NULL
};

LOCAL gfx_hscale_p_t *const gfx_hscale_16[] =
{
    gfx_hscale_1_0x_16,
    gfx_hscale_1_5x_16,
    gfx_hscale_1_0x_p,
    gfx_hscale_2_5x_16,
    gfx_hscale_3_0x_16,
    gfx_hscale_3_5x_16,
    gfx_hscale_2_0x_p,
    gfx_hscale_4_5x_16,
    gfx_hscale_5_0x_16,
    gfx_hscale_5_5x_16,
    gfx_hscale_3_0x_p,
    gfx_hscale_6_5x_16,
    gfx_hscale_7_0x_16,
    gfx_hscale_7_5x_16,
    gfx_hscale_4_0x_p,
    gfx_hscale_8_5x_16,
    gfx_hscale_9_0x_16,
    gfx_hscale_9_5x_16,
    gfx_hscale_5_0x_p,
    gfx_hscale_10_5x_16,
    gfx_hscale_11_0x_16,
    gfx_hscale_11_5x_16,
    gfx_hscale_6_0x_p,
    gfx_hscale_12_5x_16,
    gfx_hscale_13_0x_16,
    gfx_hscale_13_5x_16,
    gfx_hscale_7_0x_p,
    gfx_hscale_14_5x_16,
    gfx_hscale_15_0x_16,
    gfx_hscale_15_5x_16,
    gfx_hscale_8_0x_p,
    gfx_hscale_16_5x_16,

    gfx_hscale_17_0x_16, NULL,
    gfx_hscale_18_0x_16, NULL,
    gfx_hscale_19_0x_16, NULL,
    gfx_hscale_20_0x_16, NULL,

    gfx_hscale_21_0x_16, NULL,
    gfx_hscale_22_0x_16, NULL,
    gfx_hscale_23_0x_16, NULL,
    gfx_hscale_24_0x_16, NULL,

    gfx_hscale_25_0x_16, NULL,
    gfx_hscale_26_0x_16, NULL,
    gfx_hscale_27_0x_16, NULL,
    gfx_hscale_28_0x_16, NULL,

    gfx_hscale_29_0x_16, NULL,
    gfx_hscale_30_0x_16, NULL,
    gfx_hscale_31_0x_16, NULL,
    gfx_hscale_32_0x_16, NULL,

    gfx_hscale_33_0x_16, NULL,


    gfx_hscale_34_0x_16, NULL, NULL, NULL,
    gfx_hscale_36_0x_16, NULL, NULL, NULL,
    gfx_hscale_38_0x_16, NULL, NULL, NULL,
    gfx_hscale_40_0x_16, NULL, NULL, NULL,

    gfx_hscale_42_0x_16, NULL, NULL, NULL,
    gfx_hscale_44_0x_16, NULL, NULL, NULL,
    gfx_hscale_46_0x_16, NULL, NULL, NULL,
    gfx_hscale_48_0x_16, NULL, NULL, NULL,

    gfx_hscale_50_0x_16, NULL, NULL, NULL,
    gfx_hscale_52_0x_16, NULL, NULL, NULL,
    gfx_hscale_54_0x_16, NULL, NULL, NULL,
    gfx_hscale_56_0x_16, NULL, NULL, NULL,

    gfx_hscale_58_0x_16, NULL, NULL, NULL,
    gfx_hscale_60_0x_16, NULL, NULL, NULL,
    gfx_hscale_62_0x_16, NULL, NULL, NULL,
    gfx_hscale_64_0x_16, NULL, NULL, NULL,

    gfx_hscale_66_0x_16, NULL, NULL, NULL
};

/* ======================================================================== */
/*  GFX_SCALE_INIT_SPEC                                                     */
/* ======================================================================== */
int gfx_scale_init_spec
(
    gfx_scale_spec_t    *spec,
    int                  source_x,
    int                  source_y,
    int                  target_x,
    int                  target_y,
    int                  bpp
)
{
    int actual_x, actual_y;
    int x_ratio_2x;
    int error, i, dest_y;

    /* -------------------------------------------------------------------- */
    /*  Compute the approx ratio of target/source, rounding down to the     */
    /*  next lowest 0.5.                                                    */
    /* -------------------------------------------------------------------- */
    x_ratio_2x = 2*target_x / source_x;

    if (x_ratio_2x < 2)
        x_ratio_2x = 2;

    if      (x_ratio_2x > 132) x_ratio_2x = 132;
    else if (x_ratio_2x > 66 ) x_ratio_2x &= ~3;
    else if (x_ratio_2x > 33 ) x_ratio_2x &= ~1;

    /* -------------------------------------------------------------------- */
    /*  Now, determine the actual X/Y dimensions.  Because the ratio gets   */
    /*  rounded down, the actual X will be less than or equal to the target */
    /*  value.  Contract the Y dimension by the same amount as the X to     */
    /*  keep the same aspect ratio.                                         */
    /* -------------------------------------------------------------------- */
    spec->source_x = source_x;
    spec->source_y = source_y;
    spec->actual_x = actual_x = source_x * x_ratio_2x / 2;
    spec->actual_y = actual_y = target_y * actual_x / target_x;

    spec->y_ratio = actual_y / source_y;
    spec->delta   = actual_y - spec->y_ratio * source_y;

    /* -------------------------------------------------------------------- */
    /*  Set up the horizontal scaler.  For BPP=8, we use the "np" versions  */
    /*  and for BPP=16 or 32, we use the "p" versions.                      */
    /* -------------------------------------------------------------------- */
    if (bpp != 8 && bpp != 16 && bpp != 32)
        return -1;

    spec->bpp = bpp;

    if (bpp == 8)  spec->hscale.np = gfx_hscale_np[x_ratio_2x - 2];
    if (bpp == 16) spec->hscale.p  = gfx_hscale_16[x_ratio_2x - 2];
    if (bpp == 32) spec->hscale.p  = gfx_hscale_p [x_ratio_2x - 2];

    /* -------------------------------------------------------------------- */
    /*  Allocate and initialize the X/Y coordinate lookup arrays.           */
    /* -------------------------------------------------------------------- */
    spec->scaled_x = CALLOC(int, source_x + 1);
    spec->scaled_y = CALLOC(int, source_y + 1);

    for (i = 0; i <= source_x; i++)
        spec->scaled_x[i] = i * x_ratio_2x / 2;

    error  = spec->source_y >> 1;
    for (i = dest_y = 0; i <= source_y; i++)
    {
        spec->scaled_y[i] = dest_y;

        dest_y += spec->y_ratio;

        error -= spec->delta;
        if (error < 0)
        {
            error += spec->source_y;
            dest_y++;
        }
    }

    return 0;
}

/* ======================================================================== */
/*  GFX_SCALE_DTOR                                                          */
/* ======================================================================== */
void gfx_scale_dtor(gfx_scale_spec_t *spec)
{
    if (!spec) return;

    CONDFREE(spec->scaled_x);
    CONDFREE(spec->scaled_y);

    spec->scaled_x = NULL;
    spec->scaled_y = NULL;
}

/* ======================================================================== */
/*  GFX_SCALE_SET_PALETTE                                                   */
/* ======================================================================== */
void gfx_scale_set_palette
(
    gfx_scale_spec_t    *spec,
    int                  idx,
    uint32_t             color
)
{
    if (spec->bpp == 16)
        spec->pal[idx] = (color & 0xFFFF) * 0x00010001;
    else
        spec->pal[idx] = color;
}


LOCAL uint32_t hscale_buf[8192] ALIGN16;

/* ======================================================================== */
/*  GFX_SCALE                                                               */
/* ======================================================================== */
void gfx_scale
(
    const gfx_scale_spec_t *RESTRICT spec,
    const uint8_t          *RESTRICT src,
    uint8_t                *RESTRICT dst,
    int                              pitch,
    const uint32_t         *RESTRICT dirty_rows
)
{
    int error, i, repeats, bytes;

    if (spec->bpp == 8)
    {
        error = spec->source_y >> 1;

        for (i = 0; i < spec->source_y; i++)
        {
            repeats = spec->y_ratio;

            error -= spec->delta;
            if (error < 0)
            {
                error += spec->source_y;
                repeats++;
            }

            if ((dirty_rows[i >> 5] & (1u << (i & 31))) == 0)
            {
                dst += repeats * pitch;
                src += spec->source_x;
                continue;
            }

            spec->hscale.np(hscale_buf, src, spec->source_x);

            while (repeats-- > 0)
            {
                memcpy(dst, hscale_buf, spec->actual_x);
                dst += pitch;
            }
            src += spec->source_x;
        }
    } else
    {
        bytes = spec->actual_x * (spec->bpp == 16 ? 2 : 4);
        error = spec->source_y >> 1;

        for (i = 0; i < spec->source_y; i++)
        {
            repeats = spec->y_ratio;

            error -= spec->delta;
            if (error < 0)
            {
                error += spec->source_y;
                repeats++;
            }

            if ((dirty_rows[i >> 5] & (1u << (i & 31))) == 0)
            {
                dst += repeats * pitch;
                src += spec->source_x;
                continue;
            }

            spec->hscale.p(hscale_buf, src, spec->source_x, spec->pal);

            while (repeats-- > 0)
            {
                memcpy(dst, hscale_buf, bytes);
                dst += pitch;
            }
            src += spec->source_x;
        }
    }
}

