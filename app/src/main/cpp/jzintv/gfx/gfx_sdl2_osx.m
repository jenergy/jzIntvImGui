/* ======================================================================== */
/*  SDL2 Bug Workaround for OS/X with Metal render driver backend.          */
/*                                                                          */
/*  SDL2 does not set a colorspace by default, leaving the colorspace       */
/*  unmanaged.  This little hack sets the colorspace to device-independent  */
/*  sRGB if no colorspace is set.                                           */
/*                                                                          */
/*  It'd be cool if SDL let us play with DisplayP3, but I don't have any    */
/*  extended-range palettes to throw at it either...                        */
/* ======================================================================== */
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include "gfx/gfx_sdl2_osx.h"

bool gfx_set_srgb_colorspace(void *layer_vp)
{
    CAMetalLayer *layer = (CAMetalLayer *)layer_vp;
    if (layer.colorspace == nil)
    {
        layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        return true;
    }
    return false;
}
