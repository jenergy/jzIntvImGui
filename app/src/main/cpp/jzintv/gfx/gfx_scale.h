#ifndef GFX_SCALE_H_
#define GFX_SCALE_H_

typedef void gfx_hscale_np_t(uint32_t       *RESTRICT,
                             uint8_t  const *RESTRICT, int);
typedef void gfx_hscale_p_t (uint32_t       *RESTRICT,
                             uint8_t  const *RESTRICT, int,
                             uint32_t const *RESTRICT);

typedef struct gfx_scale_spec_t
{
    int         bpp;                /*  8, 16 or 32 */
    int         source_x, source_y;
    int         actual_x, actual_y;
    int         y_ratio, delta;

    int         *scaled_x;          /* Source X to Actual X lookup */
    int         *scaled_y;          /* Source Y to Actual Y lookup */

    union
    {
        gfx_hscale_np_t *np;
        gfx_hscale_p_t  *p;
    } hscale;                       /*  horizontal scaling function         */
    uint32_t     pal   [256];
} gfx_scale_spec_t;


int gfx_scale_init_spec
(
    gfx_scale_spec_t    *spec,
    int                  source_x,
    int                  source_y,
    int                  target_x,
    int                  target_y,
    int                  bpp
);

void gfx_scale_dtor(gfx_scale_spec_t *spec);

void gfx_scale
(
    const gfx_scale_spec_t *RESTRICT spec,
    const uint8_t          *RESTRICT src,
    uint8_t                *RESTRICT dst,
    int                              pitch,
    const uint32_t         *RESTRICT dirty_rows
);

void gfx_scale_set_palette
(
    gfx_scale_spec_t    *spec,
    int                  idx,
    uint32_t             color
);

#endif
