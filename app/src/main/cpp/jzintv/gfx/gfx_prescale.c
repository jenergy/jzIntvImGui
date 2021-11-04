/* ======================================================================== */
/*  Graphics Prescaler                                                      */
/*                                                                          */
/*  The graphics prescaler allows scaling up the 160x200 bitmap to some     */
/*  other size prior to final scaling to display resolution.  This is       */
/*  where we'd apply transforms such as Scale2X or a variant.               */
/* ======================================================================== */

#include "config.h"
#include "periph/periph.h"
#include "gfx/gfx.h"
#include "gfx/gfx_prescale.h"
#include "scale/scale2x.h"
#include "scale/scale3x.h"

typedef struct gfx_prescaler_typ_pvt_t
{
    int     orig_x, orig_y;
    uint8_t *intermediate;   // for scale4x only
} gfx_prescaler_typ_pvt_t;

/* ======================================================================== */
/*  Prescalers                                                              */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  NULL prescaler                                                          */
/* ------------------------------------------------------------------------ */
LOCAL void *gfx_prescaler_null_init(int orig_x, int orig_y,
                                    int *RESTRICT new_x,
                                    int *RESTRICT new_y,
                                    int *RESTRICT need_inter_vid,
                                    gfx_dirtyrect_spec *RESTRICT dr_spec)
{
    *new_x          = orig_x;
    *new_y          = orig_y;
    *need_inter_vid = 0;

    dr_spec->active_first_x = 0;
    dr_spec->active_first_y = 4;
    dr_spec->active_last_x  = orig_x - 1;
    dr_spec->active_last_y  = orig_y - 5;

    dr_spec->x_step         = 8;
    dr_spec->y_step         = 16;

    dr_spec->pitch          = orig_x;

    dr_spec->bord_first_x   = 0;
    dr_spec->bord_first_y   = 0;
    dr_spec->bord_last_x    = orig_x - 1;
    dr_spec->bord_last_y    = orig_y - 1;

    return NULL;
}

LOCAL void gfx_prescaler_null(const uint8_t *RESTRICT src,
                                    uint8_t *RESTRICT dst,
                                    void *RESTRICT opaque)
{
    UNUSED(src);
    UNUSED(dst);
    UNUSED(opaque);
    return;
}

LOCAL void gfx_prescaler_null_dtor(void *u)
{
    UNUSED(u);
    return;
}

/* ------------------------------------------------------------------------ */
/*  SCALE2X                                                                 */
/* ------------------------------------------------------------------------ */
LOCAL void *gfx_prescaler_scale2x_init(int orig_x, int orig_y,
                                       int *RESTRICT new_x,
                                       int *RESTRICT new_y,
                                       int *RESTRICT need_inter_vid,
                                       gfx_dirtyrect_spec *RESTRICT dr_spec)
{
    gfx_prescaler_typ_pvt_t *pvt = CALLOC(gfx_prescaler_typ_pvt_t, 1);

    if (!pvt)
    {
        fprintf(stderr, "Out of memory in gfx_prescale\n");
        exit(1);
    }

    pvt->orig_x = orig_x;
    pvt->orig_y = orig_y;
    pvt->intermediate = NULL;

    *new_x = orig_x * 2;
    *new_y = orig_y * 2;
    *need_inter_vid = 1;

    dr_spec->active_first_x = 0;
    dr_spec->active_first_y = 8;
    dr_spec->active_last_x  = 2*orig_x - 1;
    dr_spec->active_last_y  = 2*orig_y - 9;

    dr_spec->x_step         = 16;
    dr_spec->y_step         = 32;

    dr_spec->pitch          = 2*orig_x;

    dr_spec->bord_first_x   = 0;
    dr_spec->bord_first_y   = 0;
    dr_spec->bord_last_x    = 2*orig_x - 1;
    dr_spec->bord_last_y    = 2*orig_y - 1;

    return pvt;
}

/* Factored out and reused by scale4x */
LOCAL void perform_scale2x_8(int orig_x,
                             int orig_y,
                             const uint8_t *RESTRICT src,
                                   uint8_t *RESTRICT dst)
{
    int src_pitch = orig_x,
        dst_pitch = orig_x * 2;
    const uint8_t *src_prev = src;
    const uint8_t *src_curr = src;
    const uint8_t *src_next = src + src_pitch;
    int y;

    scale2x_8_def((scale2x_uint8 *)dst, (scale2x_uint8 *)(dst + dst_pitch),
                  (const scale2x_uint8 *)src_prev,
                  (const scale2x_uint8 *)src_curr,
                  (const scale2x_uint8 *)src_next, src_pitch);

    for (y = 2 ; y < orig_y; y++)
    {
        dst      += dst_pitch * 2;
        src_prev  = src_curr;
        src_curr  = src_next;
        src_next += src_pitch;

        scale2x_8_def((scale2x_uint8 *)dst, (scale2x_uint8 *)(dst + dst_pitch),
                      (const scale2x_uint8 *)src_prev,
                      (const scale2x_uint8 *)src_curr,
                      (const scale2x_uint8 *)src_next, src_pitch);
    }

    dst     += dst_pitch * 2;
    src_prev = src_curr;
    src_curr = src_next;

    scale2x_8_def((scale2x_uint8 *)dst, (scale2x_uint8 *)(dst + dst_pitch),
                  (const scale2x_uint8 *)src_prev,
                  (const scale2x_uint8 *)src_curr,
                  (const scale2x_uint8 *)src_next, src_pitch);
}

LOCAL void  gfx_prescaler_scale2x(const uint8_t *RESTRICT src,
                                        uint8_t *RESTRICT dst,
                                           void *RESTRICT opaque)
{
    gfx_prescaler_typ_pvt_t *pvt = (gfx_prescaler_typ_pvt_t *)opaque;
    int orig_x = pvt->orig_x, orig_y = pvt->orig_y;

    perform_scale2x_8(orig_x, orig_y, src, dst);

    return;
}


/* ------------------------------------------------------------------------ */
/*  SCALE3X                                                                 */
/* ------------------------------------------------------------------------ */
LOCAL void *gfx_prescaler_scale3x_init(int orig_x, int orig_y,
                                       int *RESTRICT new_x,
                                       int *RESTRICT new_y,
                                       int *RESTRICT need_inter_vid,
                                       gfx_dirtyrect_spec *RESTRICT dr_spec)
{
    gfx_prescaler_typ_pvt_t *pvt = CALLOC(gfx_prescaler_typ_pvt_t, 1);

    if (!pvt)
    {
        fprintf(stderr, "Out of memory in gfx_prescale\n");
        exit(1);
    }

    pvt->orig_x = orig_x;
    pvt->orig_y = orig_y;
    pvt->intermediate = NULL;

    *new_x = orig_x * 3;
    *new_y = orig_y * 3;
    *need_inter_vid = 1;

    dr_spec->active_first_x = 0;
    dr_spec->active_first_y = 8;
    dr_spec->active_last_x  = 3*orig_x - 1;
    dr_spec->active_last_y  = 3*orig_y - 9;

    dr_spec->x_step         = 16;
    dr_spec->y_step         = 32;

    dr_spec->pitch          = 3*orig_x;

    dr_spec->bord_first_x   = 0;
    dr_spec->bord_first_y   = 0;
    dr_spec->bord_last_x    = 3*orig_x - 1;
    dr_spec->bord_last_y    = 3*orig_y - 1;

    return pvt;
}

LOCAL void  gfx_prescaler_scale3x(const uint8_t *RESTRICT src,
                                        uint8_t *RESTRICT dst,
                                           void *RESTRICT opaque)
{
    gfx_prescaler_typ_pvt_t *pvt = (gfx_prescaler_typ_pvt_t *)opaque;
    int orig_x    = pvt->orig_x,
        orig_y    = pvt->orig_y,
        src_pitch = orig_x,
        dst_pitch = orig_x * 3;
    const uint8_t *src_prev = src;
    const uint8_t *src_curr = src;
    const uint8_t *src_next = src + src_pitch;
    int y;

    scale3x_8_def((scale3x_uint8 *)(dst                ),
                  (scale3x_uint8 *)(dst + dst_pitch * 2),
                  (scale3x_uint8 *)(dst + dst_pitch * 3),
                  (const scale3x_uint8 *)src_prev,
                  (const scale3x_uint8 *)src_curr,
                  (const scale3x_uint8 *)src_next, src_pitch);

    for (y = 3 ; y < orig_y; y++)
    {
        dst      += dst_pitch * 3;
        src_prev  = src_curr;
        src_curr  = src_next;
        src_next += src_pitch;

        scale3x_8_def((scale3x_uint8 *)(dst                ),
                      (scale3x_uint8 *)(dst + dst_pitch    ),
                      (scale3x_uint8 *)(dst + dst_pitch * 2),
                      (const scale3x_uint8 *)src_prev,
                      (const scale3x_uint8 *)src_curr,
                      (const scale3x_uint8 *)src_next, src_pitch);
    }

    dst     += dst_pitch * 3;
    src_prev = src_curr;
    src_curr = src_next;

    scale3x_8_def((scale3x_uint8 *)(dst                ),
                  (scale3x_uint8 *)(dst + dst_pitch * 2),
                  (scale3x_uint8 *)(dst + dst_pitch * 3),
                  (const scale3x_uint8 *)src_prev,
                  (const scale3x_uint8 *)src_curr,
                  (const scale3x_uint8 *)src_next, src_pitch);

    return;
}

/* ------------------------------------------------------------------------ */
/*  SCALE4X                                                                 */
/* ------------------------------------------------------------------------ */
LOCAL void *gfx_prescaler_scale4x_init(int orig_x, int orig_y,
                                       int *RESTRICT new_x,
                                       int *RESTRICT new_y,
                                       int *RESTRICT need_inter_vid,
                                       gfx_dirtyrect_spec *RESTRICT dr_spec)
{
    gfx_prescaler_typ_pvt_t *pvt = CALLOC(gfx_prescaler_typ_pvt_t, 1);
    uint8_t *intermediate = CALLOC(uint8_t, orig_x * 2 * orig_y * 2);

    if (!pvt || !intermediate)
    {
        fprintf(stderr, "Out of memory in gfx_prescale\n");
        exit(1);
    }

    pvt->orig_x       = orig_x;
    pvt->orig_y       = orig_y;
    pvt->intermediate = intermediate;

    *new_x = orig_x * 4;
    *new_y = orig_y * 4;
    *need_inter_vid = 1;

    dr_spec->active_first_x = 0;
    dr_spec->active_first_y = 8;
    dr_spec->active_last_x  = 4*orig_x - 1;
    dr_spec->active_last_y  = 4*orig_y - 9;

    dr_spec->x_step         = 16;
    dr_spec->y_step         = 32;

    dr_spec->pitch          = 4*orig_x;

    dr_spec->bord_first_x   = 0;
    dr_spec->bord_first_y   = 0;
    dr_spec->bord_last_x    = 4*orig_x - 1;
    dr_spec->bord_last_y    = 4*orig_y - 1;

    return pvt;
}

LOCAL void gfx_prescaler_scale4x(const uint8_t *RESTRICT src,
                                       uint8_t *RESTRICT dst,
                                          void *RESTRICT opaque)
{
    gfx_prescaler_typ_pvt_t *pvt = (gfx_prescaler_typ_pvt_t *)opaque;
    int orig_x = pvt->orig_x, orig_y = pvt->orig_y;
    uint8_t *mid = pvt->intermediate;

    perform_scale2x_8(orig_x,     orig_y,     src, mid);
    perform_scale2x_8(orig_x * 2, orig_y * 2, mid, dst);
}

/* ------------------------------------------------------------------------ */
/*  SCALEXX destructor                                                      */
/* ------------------------------------------------------------------------ */

LOCAL void gfx_prescaler_scalexx_dtor(void *opaque)
{
    gfx_prescaler_typ_pvt_t *pvt = (gfx_prescaler_typ_pvt_t *)opaque;

    if (pvt)
    {
        CONDFREE(pvt->intermediate);
        free(pvt);
    }

    return;
}

/* ------------------------------------------------------------------------ */
/*  Rotate-180 Prescaler                                                    */
/* ------------------------------------------------------------------------ */
LOCAL void *gfx_prescaler_rot180_init(int orig_x, int orig_y,
                                      int *RESTRICT new_x,
                                      int *RESTRICT new_y,
                                      int *RESTRICT need_inter_vid,
                                      gfx_dirtyrect_spec *RESTRICT dr_spec)
{
    gfx_prescaler_typ_pvt_t *pvt = CALLOC(gfx_prescaler_typ_pvt_t, 1);

    if (!pvt)
    {
        fprintf(stderr, "Out of memory in gfx_prescale\n");
        exit(1);
    }

    pvt->orig_x = orig_x;
    pvt->orig_y = orig_y;
    pvt->intermediate = NULL;

    *new_x = orig_x;
    *new_y = orig_y;
    *need_inter_vid = 1;

    dr_spec->active_first_x = 0;
    dr_spec->active_first_y = 4;
    dr_spec->active_last_x  = orig_x - 1;
    dr_spec->active_last_y  = orig_y - 5;

    dr_spec->x_step         = 8;
    dr_spec->y_step         = 16;

    dr_spec->pitch          = orig_x;

    dr_spec->bord_first_x   = 0;
    dr_spec->bord_first_y   = 0;
    dr_spec->bord_last_x    = orig_x - 1;
    dr_spec->bord_last_y    = orig_y - 1;

    return pvt;
}

LOCAL void gfx_prescaler_rot180(const uint8_t *RESTRICT src,
                                      uint8_t *RESTRICT dst,
                                         void *RESTRICT opaque)
{
    gfx_prescaler_typ_pvt_t *pvt = (gfx_prescaler_typ_pvt_t *)opaque;
    const uint8_t *RESTRICT ps = src;
    uint8_t       *RESTRICT pd = dst + pvt->orig_x * pvt->orig_y - 1;
    int i = pvt->orig_x * pvt->orig_y;

    while (i-- > 0)
        *pd-- = *ps++;

    return;
}

LOCAL void gfx_prescaler_rot180_dtor(void *u)
{
    UNUSED(u);
    return;
}

/* ======================================================================== */
/*  Prescaler Registry                                                      */
/* ======================================================================== */
gfx_prescaler_registry_t gfx_prescaler_registry[] =
{
    {   "None",     gfx_prescaler_null,
                    gfx_prescaler_null_init,
                    gfx_prescaler_null_dtor
    },

    {   "Scale2x",  gfx_prescaler_scale2x,
                    gfx_prescaler_scale2x_init,
                    gfx_prescaler_scalexx_dtor
    },

    {   "Scale3x",  gfx_prescaler_scale3x,
                    gfx_prescaler_scale3x_init,
                    gfx_prescaler_scalexx_dtor
    },

    {   "Scale4x",  gfx_prescaler_scale4x,
                    gfx_prescaler_scale4x_init,
                    gfx_prescaler_scalexx_dtor
    },

    {   "Rot180",   gfx_prescaler_rot180,
                    gfx_prescaler_rot180_init,
                    gfx_prescaler_rot180_dtor
    },
};


int gfx_prescaler_registry_size = sizeof(gfx_prescaler_registry) /
                                  sizeof(gfx_prescaler_registry_t);

