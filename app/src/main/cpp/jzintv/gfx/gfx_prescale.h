/* ======================================================================== */
/*  Graphics Prescaler                                                      */
/*                                                                          */
/*  The graphics prescaler allows scaling up the 160x200 bitmap to some     */
/*  other size prior to final scaling to display resolution.  This is       */
/*  where we'd apply transforms such as Scale2X or a variant.               */
/* ======================================================================== */

#ifndef GFX_PRESCALE_H_
#define GFX_PRESCALE_H_


/* ------------------------------------------------------------------------ */
/*  Prototypical prescaler function:  Takes a source/dest bitmap, and a     */
/*  pointer to an opaque "private" structure that has all the config        */
/*  details for the prescaling operation.                                   */
/* ------------------------------------------------------------------------ */
typedef void gfx_prescaler_t
(
    const uint8_t *RESTRICT  src,
    uint8_t       *RESTRICT  dst,
    void          *RESTRICT  opaque
);

/* ------------------------------------------------------------------------ */
/*  Prototypical prescaler initializer function:  Takes the source X/Y      */
/*  dimensions, and returns the prescaled X/Y dimensions and a pointer to   */
/*  a private structure that has whatever further details it needs.         */
/* ------------------------------------------------------------------------ */
typedef void *gfx_prescaler_init_t
(
    int source_x,
    int source_y,
    int *RESTRICT prescaled_x,
    int *RESTRICT prescaled_y,
    int *RESTRICT neet_inter_vid,
    gfx_dirtyrect_spec *RESTRICT dr_spec
);

/* ------------------------------------------------------------------------ */
/*  Prototypical prescaler dirty rectangle oracle:  Fill in a struct that   */
/*  tells the dirty rectangle engine what it needs to do.                   */
/* ------------------------------------------------------------------------ */
typedef void gfx_prescale_dirtyrect_t
(
    int source_x,    int source_y,
    int prescaled_x, int prescaled_y,
    void *RESTRICT opaque,
    gfx_dirtyrect_spec *dirty_rect_spec
);

/* ------------------------------------------------------------------------ */
/*  Prototypical prescaler destructor.  Frees any memory stashed away in    */
/*  the private prescaler structure.                                        */
/* ------------------------------------------------------------------------ */
typedef void gfx_prescaler_dtor_t(void *RESTRICT opaque);


/* ------------------------------------------------------------------------ */
/*  Prescaler registry:  Where we register all of jzIntv's prescalers.      */
/* ------------------------------------------------------------------------ */
typedef struct gfx_prescaler_registry_t
{
    const char              *name;
    gfx_prescaler_t         *prescaler;
    gfx_prescaler_init_t    *prescaler_init;
    gfx_prescaler_dtor_t    *prescaler_dtor;
} gfx_prescaler_registry_t;



extern gfx_prescaler_registry_t gfx_prescaler_registry[];
extern int                      gfx_prescaler_registry_size;

#endif

