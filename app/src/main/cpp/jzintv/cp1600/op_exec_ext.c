/* ======================================================================== */
/*  LTO Flash / JLP Extended ISA                                            */
/* ======================================================================== */

#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/op_decode.h"
#include "cp1600/op_exec_ext.h"

typedef void fnx_t(const uint16_t, const uint16_t,
                   uint16_t *const, uint16_t *const);

#define FN(x) LOCAL void fnx_##x( const uint16_t s1, const uint16_t s2, \
                                  uint16_t *const dl, uint16_t *const dh )

LOCAL INLINE uint16_t pack(uint8_t hi, uint8_t lo)
{
    return ((uint16_t)hi << 8) | lo;
}

LOCAL INLINE uint16_t swap(const uint16_t x)
{
    return (x >> 8) | (x << 8);
}

LOCAL INLINE int32_t as_i32(const uint16_t x)
{
    return (int32_t)(x ^ 0x8000u) - 0x8000;
}

LOCAL INLINE int32_t as_i32s(const uint16_t fx)
{
    const uint16_t x = swap(fx);
    return (int32_t)(x ^ 0x8000u) - 0x8000;
}

LOCAL INLINE int16_t as_i16(const uint16_t x)
{
    return (x & 0x7FFF) - (x & 0x8000);
}

LOCAL INLINE int16_t as_i16s(const uint16_t fx)
{
    const uint16_t x = swap(fx);
    return (x & 0x7FFF) - (x & 0x8000);
}

LOCAL INLINE uint32_t as_u32 (const uint16_t x) { return x;       }
LOCAL INLINE uint32_t as_u32s(const uint16_t x) { return swap(x); }
LOCAL INLINE uint16_t as_u16 (const uint16_t x) { return x;       }
LOCAL INLINE uint16_t as_u16s(const uint16_t x) { return swap(x); }

FN(add3)    {   UNUSED(dh); *dl =   s1 + s2;                    }
FN(nadd)    {   UNUSED(dh); *dl = -(s1 + s2);                   }
FN(sub3)    {   UNUSED(dh); *dl =   s1 - s2;                    }
FN(addfx)   {   UNUSED(dh); *dl = swap( swap(s1) + swap(s2));   }
FN(naddfx)  {   UNUSED(dh); *dl = swap(-swap(s1) - swap(s2));   }
FN(subfx)   {   UNUSED(dh); *dl = swap( swap(s1) - swap(s2));   }
FN(and3)    {   UNUSED(dh); *dl =    s1 & s2;                   }
FN(nand)    {   UNUSED(dh); *dl = ~( s1 & s2);                  }
FN(andn)    {   UNUSED(dh); *dl =   ~s1 & s2;                   }
FN(orn)     {   UNUSED(dh); *dl = ~(~s1 & s2);                  }
FN(or3)     {   UNUSED(dh); *dl =    s1 | s2;                   }
FN(nor)     {   UNUSED(dh); *dl = ~( s1 | s2);                  }
FN(xor3)    {   UNUSED(dh); *dl =    s1 ^ s2;                   }
FN(xnor)    {   UNUSED(dh); *dl = ~( s1 ^ s2);                  }

FN(shl3)
{
    const int s = s2 & 0xF;
    if (!s) { *dl = s1 << s; *dh = as_i32(s1) >> 15;        }
    else    { *dl = s1 << s; *dh = as_i32(s1) >> (16 - s);  }
}
FN(shlu)
{
    const int s = s2 & 0xF;
    if (!s) { *dl = s1 << s; *dh = 0;                       }
    else    { *dl = s1 << s; *dh = as_u32(s1) >> (16 - s);  }
}
FN(shr3)
{
    const int s = s2 & 0xF;
    if (!s) { *dl = as_i32(s1) >> s; *dh = 0;               }
    else    { *dl = as_i32(s1) >> s; *dh = s1 << (16 - s);  }
}
FN(shru)
{
    const int s = s2 & 0xF;
    if (!s) { *dl = as_u32(s1) >> s; *dh = 0;               }
    else    { *dl = as_u32(s1) >> s; *dh = s1 << (16 - s);  }
}

FN(bshlu)
{
    const int s  = (s2 & 0xF) < 8 ? s2 & 0xF : 8;
    const int ns = 8 - s;
    const uint8_t dlh = ((s1 >> 8) & 0xFF) << s;
    const uint8_t dll = (s1 & 0xFF) << s;
    const uint8_t dhh = ((s1 >> 8) & 0xFF) >> ns;
    const uint8_t dhl = (s1 & 0xFF) >> ns;

    *dl = pack(dlh, dll);
    *dh = pack(dhh, dhl);
}

FN(bshru)
{
    const int s  = (s2 & 0xF) < 8 ? s2 & 0xF : 8;
    const int ns = 8 - s;
    const uint8_t dlh = ((s1 >> 8) & 0xFF) >> s;
    const uint8_t dll = (s1 & 0xFF) >> s;
    const uint8_t dhh = ((s1 >> 8) & 0xFF) << ns;
    const uint8_t dhl = (s1 & 0xFF) << ns;

    *dl = pack(dlh, dll);
    *dh = pack(dhh, dhl);
}

FN(rol)
{
    const int s = s2 & 0xF, ns = 16 - s;
    UNUSED(dh);
    *dl = (((uint32_t)s1) << s) | (s1 >> ns);
}

FN(ror)
{
    const int s = s2 & 0xF, ns = 16 - s;
    UNUSED(dh);
    *dl = (s1 >> s) | (((uint32_t)s1) << ns);
}

FN(mpyss)
{
    const int32_t p = as_i32(s1) * as_i32(s2);
    *dl = p & 0xFFFF;
    *dh = (p >> 16) & 0xFFFF;
}

FN(mpysu)
{
    const int32_t p = as_i32(s1) * as_u32(s2);
    *dl = p & 0xFFFF;
    *dh = (p >> 16) & 0xFFFF;
}

FN(mpyus)
{
    const int32_t p = as_u32(s1) * as_i32(s2);
    *dl = p & 0xFFFF;
    *dh = (p >> 16) & 0xFFFF;
}

FN(mpyuu)
{
    const uint32_t p = as_u32(s1) * as_u32(s2);
    *dl = p & 0xFFFF;
    *dh = (p >> 16) & 0xFFFF;
}

FN(mpy16) { UNUSED(dh); *dl = s1 * s2; }

FN(mpyfxss)
{
    const int32_t p = as_i32s(s1) * as_i32s(s2);
    *dl = (p & 0xFF00) | ((p >> 16) & 0x00FF);
    UNUSED(dh);
}

FN(mpyfxsu)
{
    const int32_t p = as_i32s(s1) * as_u32s(s2);
    *dl = (p & 0xFF00) | ((p >> 16) & 0x00FF);
    UNUSED(dh);
}

FN(mpyfxus)
{
    const int32_t p = as_u32s(s1) * as_i32s(s2);
    *dl = (p & 0xFF00) | ((p >> 16) & 0x00FF);
    UNUSED(dh);
}

FN(mpyfxuu)
{
    const uint32_t p = as_u32s(s1) * as_u32s(s2);
    *dl = (p & 0xFF00) | ((p >> 16) & 0x00FF);
    UNUSED(dh);
}

FN(divs)
{
    const int16_t i1 = as_i16(s1), i2 = as_i16(s2);
    if (s2) { *dl = i1 / i2; *dh = i1 % i2; }
    else    { *dl = 0x7FFF;  *dh = 0x7FFF;  }
}

FN(divu)
{
    if (s2) { *dl = s1 / s2; *dh = s1 % s2; }
    else    { *dl = 0xFFFF;  *dh = 0xFFFF;  }
}

FN(divfxs)
{
    const int32_t ss1 = as_i32(swap(s1));
    const int32_t i1 = SLS32(ss1, 8), i2 = as_i32(swap(s2));
    *dl = swap(0x7FFF);
    *dh = swap(0x7FFF);
    if (s2)
    {
        const int32_t q = i1 / i2;
        const int32_t r = i1 % i2;
        if (q >= -0x8000 && q <= 0x7FFF)
        {
            *dl = swap(q);
            *dh = swap(r);
        }
    }
}

FN(divfxu)
{
    const uint32_t i1 = as_u32(swap(s1)) << 8, i2 = as_u32(swap(s2));
    *dl = swap(0xFFFF);
    *dh = swap(0xFFFF);
    if (s2)
    {
        const uint32_t q = i1 / i2;
        const uint32_t r = i1 % i2;
        if (q <= 0xFFFF)
        {
            *dl = swap(q);
            *dh = swap(r);
        }
    }
}

// DIV32SS/DIV32UU need to be handled specially

FN(bitcntl)
{
    uint16_t value = s1;
    int to_count = (s2 & 0xF) + 1;
    int bits = 0;
    UNUSED(dh);
    while (to_count > 0)
    {
        if (value & 0x8000)
            bits++;
        value <<= 1;
        to_count--;
    }
    *dl = bits;
}

FN(bitcntr)
{
    uint16_t value = s1;
    int to_count = (s2 & 0xF) + 1;
    int bits = 0;
    UNUSED(dh);
    while (to_count > 0)
    {
        if (value & 1)
            bits++;
        value >>= 1;
        to_count--;
    }
    *dl = bits;
}

FN(bitrevl)
{
    uint16_t value_in  = s1;
    uint16_t value_out = 0;
    int to_reverse = (s2 & 0xF) + 1;
    UNUSED(dh);
    while (to_reverse > 0)
    {
        value_out >>= 1;
        if (value_in & 0x8000)
            value_out |= 0x8000;
        value_in <<= 1;
        to_reverse--;
    }
    *dl = value_out;
}

FN(bitrevr)
{
    uint16_t value_in  = s1;
    uint16_t value_out = 0;
    int to_reverse = (s2 & 0xF) + 1;
    UNUSED(dh);
    while (to_reverse > 0)
    {
        value_out <<= 1;
        if (value_in & 1)
            value_out |= 1;
        value_in >>= 1;
        to_reverse--;
    }
    *dl = value_out;
}

FN(lmo)
{
    int i;
    UNUSED(dh);

    for (i = s2 & 0xF; i >= 0; i--)
    {
        if (s1 & (1u << i))
        {
            *dl = i;
            return;
        }
    }

    *dl = 0xFFFF;
    return;
}

FN(lmz)
{
    int i;
    UNUSED(dh);

    for (i = s2 & 0xF; i >= 0; i--)
    {
        if ((s1 & (1u << i)) == 0)
        {
            *dl = i;
            return;
        }
    }

    *dl = 0xFFFF;
    return;
}

FN(rmo)
{
    int i;
    UNUSED(dh);

    for (i = s2 & 0xF; i <= 15; i++)
    {
        if (s1 & (1u << i))
        {
            *dl = i;
            return;
        }
    }

    *dl = 0xFFFF;
    return;
}

FN(rmz)
{
    int i;
    UNUSED(dh);

    for (i = s2 & 0xF; i <= 15; i++)
    {
        if ((s1 & (1u << i)) == 0)
        {
            *dl = i;
            return;
        }
    }

    *dl = 0xFFFF;
    return;
}

FN(subabs)
{
    const int diff = as_i16(s1) - as_i16(s2);
    const int absdiff = diff < 0 ? -diff : diff;
    UNUSED(dh);
    *dl = absdiff & 0xFFFF;
}

FN(subabsu)
{
    const int diff = as_u16(s1) - as_u16(s2);
    const int absdiff = diff < 0 ? -diff : diff;
    UNUSED(dh);
    *dl = absdiff & 0xFFFF;
}

FN(subabsfx)
{
    const int diff = as_i16(swap(s1)) - as_i16(swap(s2));
    const int absdiff = diff < 0 ? -diff : diff;
    UNUSED(dh);
    *dl = swap(absdiff & 0xFFFF);
}

FN(subabsfxu)
{
    const int diff = as_u16(swap(s1)) - as_u16(swap(s2));
    const int absdiff = diff < 0 ? -diff : diff;
    UNUSED(dh);
    *dl = swap(absdiff & 0xFFFF);
}

FN(dist)
{
    const uint32_t u1 = abs(as_i32(s1));
    const uint32_t u2 = abs(as_i32(s2));
    const uint32_t mx = u1 > u2 ? u1 : u2;
    const uint32_t mn = u1 > u2 ? u2 : u1;
    UNUSED(dh);
    *dl = (mx * 123 + mn * 51) >> 7;
}

FN(distu)
{
    const uint32_t u1 = as_u32(s1);
    const uint32_t u2 = as_u32(s2);
    const uint32_t mx = u1 > u2 ? u1 : u2;
    const uint32_t mn = u1 > u2 ? u2 : u1;
    UNUSED(dh);
    *dl = (mx * 123 + mn * 51) >> 7;
}

FN(distfx)
{
    const uint32_t u1 = abs(as_i32(swap(s1)));
    const uint32_t u2 = abs(as_i32(swap(s2)));
    const uint32_t mx = u1 > u2 ? u1 : u2;
    const uint32_t mn = u1 > u2 ? u2 : u1;
    UNUSED(dh);
    *dl = swap((mx * 123 + mn * 51) >> 7);
}

FN(distfxu)
{
    const uint32_t u1 = as_u32(swap(s1));
    const uint32_t u2 = as_u32(swap(s2));
    const uint32_t mx = u1 > u2 ? u1 : u2;
    const uint32_t mn = u1 > u2 ? u2 : u1;
    UNUSED(dh);
    *dl = swap((mx * 123 + mn * 51) >> 7);
}

FN(min)     {   UNUSED(dh); *dl = as_i16 (s1) < as_i16 (s2) ? s1 : s2;    }
FN(minfx)   {   UNUSED(dh); *dl = as_i16s(s1) < as_i16s(s2) ? s1 : s2;    }
FN(minu)    {   UNUSED(dh); *dl = as_u16 (s1) < as_u16 (s2) ? s1 : s2;    }
FN(minfxu)  {   UNUSED(dh); *dl = as_u16s(s1) < as_u16s(s2) ? s1 : s2;    }
FN(max)     {   UNUSED(dh); *dl = as_i16 (s1) > as_i16 (s2) ? s1 : s2;    }
FN(maxfx)   {   UNUSED(dh); *dl = as_i16s(s1) > as_i16s(s2) ? s1 : s2;    }
FN(maxu)    {   UNUSED(dh); *dl = as_u16 (s1) > as_u16 (s2) ? s1 : s2;    }
FN(maxfxu)  {   UNUSED(dh); *dl = as_u16s(s1) > as_u16s(s2) ? s1 : s2;    }

FN(addcirc)
{
    const uint16_t mask = (1u << (s2 & 0xF)) - 1;
    const uint16_t sum  = (*dl + s1) & mask;
    const uint16_t keep = *dl & ~mask;
    UNUSED(dh);
    *dl = keep | sum;
}

FN(subcirc)
{
    const uint16_t mask = (1u << (s2 & 0xF)) - 1;
    const uint16_t sum  = (*dl - s1) & mask;
    const uint16_t keep = *dl & ~mask;
    UNUSED(dh);
    *dl = keep | sum;
}

FN(repack)
{
    *dl = pack(s1 & 0xFF, s2 & 0xFF);
    *dh = pack(s1 >> 8, s2 >> 8);
}

FN(packl)   {   UNUSED(dh); *dl = pack(s1 & 0xFF, s2 & 0xFF);   }
FN(packh)   {   UNUSED(dh); *dl = pack(s1 >> 8,   s2 >> 8);     }
FN(packlh)  {   UNUSED(dh); *dl = pack(s1 & 0xFF, s2 >> 8);     }

FN(btog)    {   UNUSED(dh); *dl = s1 ^  (1u << (s2 & 0xF));     }
FN(bset)    {   UNUSED(dh); *dl = s1 |  (1u << (s2 & 0xF));     }
FN(bclr)    {   UNUSED(dh); *dl = s1 & ~(1u << (s2 & 0xF));     }

FN(cmpltu)    { UNUSED(dh); *dl  = as_u16(s1)  <  as_u16(s2)  ? 0xFFFFu : 0; }
FN(cmpleu)    { UNUSED(dh); *dl  = as_u16(s1)  <= as_u16(s2)  ? 0xFFFFu : 0; }
FN(cmpltua)   { UNUSED(dh); *dl &= as_u16(s1)  <  as_u16(s2)  ? 0xFFFFu : 0; }
FN(cmpleua)   { UNUSED(dh); *dl &= as_u16(s1)  <= as_u16(s2)  ? 0xFFFFu : 0; }

FN(cmpltfxu)  { UNUSED(dh); *dl  = as_u16s(s1) <  as_u16s(s2) ? 0xFFFFu : 0; }
FN(cmplefxu)  { UNUSED(dh); *dl  = as_u16s(s1) <= as_u16s(s2) ? 0xFFFFu : 0; }
FN(cmpltfxua) { UNUSED(dh); *dl &= as_u16s(s1) <  as_u16s(s2) ? 0xFFFFu : 0; }
FN(cmplefxua) { UNUSED(dh); *dl &= as_u16s(s1) <= as_u16s(s2) ? 0xFFFFu : 0; }

FN(cmplt)     { UNUSED(dh); *dl  = as_i16(s1)  <  as_i16(s2)  ? 0xFFFFu : 0; }
FN(cmple)     { UNUSED(dh); *dl  = as_i16(s1)  <= as_i16(s2)  ? 0xFFFFu : 0; }
FN(cmplta)    { UNUSED(dh); *dl &= as_i16(s1)  <  as_i16(s2)  ? 0xFFFFu : 0; }
FN(cmplea)    { UNUSED(dh); *dl &= as_i16(s1)  <= as_i16(s2)  ? 0xFFFFu : 0; }

FN(cmpltfx)   { UNUSED(dh); *dl  = as_i16s(s1) <  as_i16s(s2) ? 0xFFFFu : 0; }
FN(cmplefx)   { UNUSED(dh); *dl  = as_i16s(s1) <= as_i16s(s2) ? 0xFFFFu : 0; }
FN(cmpltfxa)  { UNUSED(dh); *dl &= as_i16s(s1) <  as_i16s(s2) ? 0xFFFFu : 0; }
FN(cmplefxa)  { UNUSED(dh); *dl &= as_i16s(s1) <= as_i16s(s2) ? 0xFFFFu : 0; }

FN(cmpeq)     { UNUSED(dh); *dl  = s1 == s2 ? 0xFFFFu : 0; }
FN(cmpne)     { UNUSED(dh); *dl  = s1 != s2 ? 0xFFFFu : 0; }
FN(cmpeqa)    { UNUSED(dh); *dl &= s1 == s2 ? 0xFFFFu : 0; }
FN(cmpnea)    { UNUSED(dh); *dl &= s1 != s2 ? 0xFFFFu : 0; }

FN(bound)
{
    const int16_t i1  = as_i16(s1), i2 = as_i16(s2), id = as_i16(*dl);
    const int16_t mn = i1 < i2 ? i1 : i2;
    const int16_t mx = i1 > i2 ? i1 : i2;
    const int16_t cl = mn > id ? mn : id;
    const int16_t ch = cl > mx ? mx : cl;
    *dl = ch;
    UNUSED(dh);
}

FN(boundu)
{
    const uint16_t i1  = as_u16(s1), i2 = as_u16(s2), id = as_u16(*dl);
    const uint16_t mn = i1 < i2 ? i1 : i2;
    const uint16_t mx = i1 > i2 ? i1 : i2;
    const uint16_t cl = mn > id ? mn : id;
    const uint16_t ch = cl > mx ? mx : cl;
    *dl = ch;
    UNUSED(dh);
}

FN(boundfx)
{
    const int16_t i1  = as_i16s(s1), i2 = as_i16s(s2), id = as_i16s(*dl);
    const int16_t mn = i1 < i2 ? i1 : i2;
    const int16_t mx = i1 > i2 ? i1 : i2;
    const int16_t cl = mn > id ? mn : id;
    const int16_t ch = cl > mx ? mx : cl;
    *dl = swap(ch);
    UNUSED(dh);
}

FN(boundfxu)
{
    const uint16_t i1  = as_u16s(s1), i2 = as_u16s(s2), id = as_u16s(*dl);
    const uint16_t mn = i1 < i2 ? i1 : i2;
    const uint16_t mx = i1 > i2 ? i1 : i2;
    const uint16_t cl = mn > id ? mn : id;
    const uint16_t ch = cl > mx ? mx : cl;
    *dl = swap(ch);
    UNUSED(dh);
}

FN(aal) { UNUSED(*dh); *dl = (((( s1     & 0xFF) - 0x20) & 0x1FF) << 3) + s2; }
FN(aah) { UNUSED(*dh); *dl = (((((s1>>8) & 0xFF) - 0x20) & 0x1FF) << 3) + s2; }

FN(sumsq)
{
    const int64_t l1 = as_i32(s1), l2 = as_i32(s2);
    const int64_t p = l1 * l1 + l2 * l2;

    *dl = 0xFFFF;
    *dh = 0xFFFF;

    if (p < 0xFFFFFFFFll)
    {
        *dl = p & 0xFFFF;
        *dh = (p >> 16) & 0xFFFF;
    }
}

FN(sumsqu)
{
    const int64_t l1 = as_u32(s1), l2 = as_u32(s2);
    const int64_t p = l1 * l1 + l2 * l2;

    *dl = 0xFFFF;
    *dh = 0xFFFF;

    if (p < 0xFFFFFFFFll)
    {
        *dl = p & 0xFFFF;
        *dh = (p >> 16) & 0xFFFF;
    }
}

FN(sumsqfx)
{
    fnx_sumsqu( as_i16s(s1) < 0 ? -swap(s1) : swap(s1),
                as_i16s(s2) < 0 ? -swap(s2) : swap(s2), dl, dh );
}

FN(sumsqfxu)
{
    fnx_sumsqu( swap(s1), swap(s2), dl, dh );
}

LOCAL INLINE uint32_t isqrt_helper(uint32_t x)
{
    uint32_t guess = 0, bit;

    for (bit = 0x8000u; bit != 0; bit >>= 1)
    {
        guess |= bit;
        if ( guess * guess > x )
            guess ^= bit;
    }
    return guess;
}

FN(isqrt)   {   UNUSED(dh); UNUSED(s2); *dl = (uint16_t)isqrt_helper(s1); }
FN(isqrtfx)
{
    UNUSED(dh);
    UNUSED(s1);
    *dl = swap((uint16_t)isqrt_helper(256*swap(s2)));
}

FN(atan2)
{
    int16_t x = as_i16(s1), y = as_i16(s2);
    uint16_t ux, uy;
    uint16_t a = 0;

    // Flip around X axis
    if (y < 0)
    {
        y = -y;
        a ^= 0x1F;
    }
    uy = y;

    // Flip around Y axis
    if (x < 0)
    {
        x = -x;
        a ^= 0xF;
    }
    ux = x;

    // Flip around 45-degree line
    if (uy > ux)
    {
        uint16_t ut = ux;
        ux = uy;
        uy = ut;

        a ^= 0x7;
    }

    // Maximize precision for fxpt arith: slide leading 1 to top
    if (ux)
        while ((ux & 0x8000) == 0)
        {
            ux <<= 1;
            uy <<= 1;
        }

    // Are we above or below 22.5 degree line? Slope approx 0.41421
    uint16_t nx = (ux * 0x6A0A) >> 16;
    if (uy > nx)
    {
        // Above: toggle the bit
        a ^= 0x2;

        // Are we above or below the 33.75 degree line? Slope approx 0.66818
        nx = (ux * 0xAB0Eu) >> 16;
        if (uy > nx)
            a ^= 0x1;
    } else
    {
        // Are we above or below the 11.25 degree line? Slope approx 0.19891
        nx = (ux * 0x32EC) >> 16;
        if (uy > nx)
            a ^= 0x1;
    }

    // We now have an angle 0..31, with center points 11.25 degrees apart,
    // with the first center point at 6.125 degrees.  Collapse this to
    // 0..15, with the first center point at 0 deg.
    a = ((a + 1) >> 1) & 0xF;

    *dl = a;
    UNUSED(dh);
}

FN(atan2fx) { fnx_atan2(swap(s1),swap(s2),dl,dh); }

FN(i2bcd)
{
    uint32_t value = (as_u32(s1) << 16) | s2;

    if ( value >= 99999999 )
    {
        *dh = 0x9999;
        *dl = 0x9999;
        return;
    }

    const uint32_t digit7 = ((value / 10000000) % 10) << 12;
    const uint32_t digit6 = ((value / 1000000 ) % 10) <<  8;
    const uint32_t digit5 = ((value / 100000  ) % 10) <<  4;
    const uint32_t digit4 = ((value / 10000   ) % 10) <<  0;
    const uint32_t digit3 = ((value / 1000    ) % 10) << 12;
    const uint32_t digit2 = ((value / 100     ) % 10) <<  8;
    const uint32_t digit1 = ((value / 10      ) % 10) <<  4;
    const uint32_t digit0 = ((value / 1       ) % 10) <<  0;

    *dh = digit7 | digit6 | digit5 | digit4;
    *dl = digit3 | digit2 | digit1 | digit0;
}

FN(bcd2i)
{
    const uint32_t digit7 = ((s1 >> 12) & 0xF) * 10000000;
    const uint32_t digit6 = ((s1 >>  8) & 0xF) * 1000000;
    const uint32_t digit5 = ((s1 >>  4) & 0xF) * 100000;
    const uint32_t digit4 = ((s1 >>  0) & 0xF) * 10000;
    const uint32_t digit3 = ((s2 >> 12) & 0xF) * 1000;
    const uint32_t digit2 = ((s2 >>  8) & 0xF) * 100;
    const uint32_t digit1 = ((s2 >>  4) & 0xF) * 10;
    const uint32_t digit0 = ((s2 >>  0) & 0xF) * 1;

    const uint32_t rslt = digit7 + digit6 + digit5 + digit4
                       + digit3 + digit2 + digit1 + digit0;

    *dh = rslt >> 16;
    *dl = rslt & 0xFFFF;
}

/* Divide, rounding toward -oo */
LOCAL int div_10( int x )
{
    int q = 0;

    assert(x <=  33);
    assert(x >= -19);

    while (x > 9) { q++; x -= 10; }
    while (x < 0) { q--; x += 10; }

    return q;
}

/*  The Cadillac of BCD add/subtract implementations.                   */
/*  Treats non-canonical digits A - F as values 10 - 15, consistently.  */
LOCAL void abcd_helper
(
    const uint16_t s1, const uint16_t s2,       // Inputs
    uint16_t *const dl, uint16_t *const dh,     // Outputs
    int ci_flag,                                // Examine carry in
    int co_flag,                                // Write carry out
    int addsub_flag                             // 1 == ADD, 0 == SUB
)
{
    //  Carry/borrow in is signed.  3 LSBs are significant,
    //  all others are ignored.
    //
    //              011     +3        111     -1
    //              010     +2        110     -2
    //              001     +1        101     -3
    //              000      0        100     -4
    //
    //  Maximum carry out +3 in arises from    F + F + 3 =  33
    //  Maximum borrow out -2 in arises from   0 - F - 4 = -19


    const int s1_d0 = (s1 >>  0) & 0xF, s2_d0 = (s2 >>  0) & 0xF;
    const int s1_d1 = (s1 >>  4) & 0xF, s2_d1 = (s2 >>  4) & 0xF;
    const int s1_d2 = (s1 >>  8) & 0xF, s2_d2 = (s2 >>  8) & 0xF;
    const int s1_d3 = (s1 >> 12) & 0xF, s2_d3 = (s2 >> 12) & 0xF;

    const int ci_d0  = ci_flag ? ((*dl & 7) ^ 4) - 4 : 0;
    const int raw_d0 = (addsub_flag ? s1_d0 + s2_d0 : s1_d0 - s2_d0) + ci_d0;
    const int co_d0  = div_10( raw_d0 );
    const int digit0 = raw_d0 - co_d0 * 10;

    const int ci_d1  = co_d0;
    const int raw_d1 = (addsub_flag ? s1_d1 + s2_d1 : s1_d1 - s2_d1) + ci_d1;
    const int co_d1  = div_10( raw_d1 );
    const int digit1 = raw_d1 - co_d1 * 10;

    const int ci_d2  = co_d1;
    const int raw_d2 = (addsub_flag ? s1_d2 + s2_d2 : s1_d2 - s2_d2) + ci_d2;
    const int co_d2  = div_10( raw_d2 );
    const int digit2 = raw_d2 - co_d2 * 10;

    const int ci_d3  = co_d2;
    const int raw_d3 = (addsub_flag ? s1_d3 + s2_d3 : s1_d3 - s2_d3) + ci_d3;
    const int co_d3  = div_10( raw_d3 );
    const int digit3 = raw_d3 - co_d3 * 10;

    *dl = (digit3 << 12) | (digit2 << 8) | (digit1 << 4) | digit0;

    if (co_flag)
        *dh = co_d3;
}

FN(abcd ) { abcd_helper(s1, s2, dl, dh, 0, 0, 1);  }
FN(abcdl) { abcd_helper(s1, s2, dl, dh, 0, 1, 1);  }
FN(abcdm) { abcd_helper(s1, s2, dl, dh, 1, 1, 1);  }
FN(abcdh) { abcd_helper(s1, s2, dl, dh, 1, 0, 1);  }

FN(sbcd ) { abcd_helper(s1, s2, dl, dh, 0, 0, 0);  }
FN(sbcdl) { abcd_helper(s1, s2, dl, dh, 0, 1, 0);  }
FN(sbcdm) { abcd_helper(s1, s2, dl, dh, 1, 1, 0);  }
FN(sbcdh) { abcd_helper(s1, s2, dl, dh, 1, 0, 0);  }

FN(adds)
{
    const int32_t rslt = as_i32(s1) + as_i32(s2);
    *dl = rslt & 0xFFFF;
    *dh = rslt >> 16;
}

FN(addu)
{
    const int32_t rslt = as_u32(s1) + as_u32(s2);
    *dl = rslt & 0xFFFF;
    *dh = rslt >> 16;
}

FN(addm)
{
    const int32_t rslt = as_u32(s1) + as_u32(s2) + as_u32(*dl);
    *dl = rslt & 0xFFFF;
    *dh = rslt >> 16;
}

FN(addh)
{
    const int32_t rslt = as_u32(s1) + as_u32(s2) + as_u32(*dl);
    *dl = rslt & 0xFFFF;
    UNUSED(dh);
}

FN(subs)
{
    const int32_t rslt = as_i32(s1) - as_i32(s2);
    *dl = rslt & 0xFFFF;
    *dh = rslt >> 16;
}

FN(subu)
{
    const int32_t rslt = as_u32(s1) - as_u32(s2);
    *dl = rslt & 0xFFFF;
    *dh = rslt >> 16;
}

FN(subm)
{
    const int32_t rslt = as_u32(s1) - as_u32(s2) + as_i32(*dl);
    *dl = rslt & 0xFFFF;
    *dh = rslt >> 16;
}

FN(subh)
{
    const int32_t rslt = as_u32(s1) - as_u32(s2) + as_i32(*dl);
    *dl = rslt & 0xFFFF;
    UNUSED(dh);
}

FN(dmov)   { *dh = s1;      *dl = s2;      }
FN(addsub) { *dh = s1 + s2; *dl = s1 - s2; }

LOCAL fnx_t *const fnx_tbl[][2] =
{
    {   fnx_add3,       fnx_nadd        },
    {   fnx_addfx,      fnx_naddfx      },
    {   fnx_sub3,       fnx_sub3        },
    {   fnx_subfx,      fnx_subfx       },
    {   fnx_and3,       fnx_nand        },
    {   fnx_andn,       fnx_orn         },
    {   fnx_or3,        fnx_nor         },
    {   fnx_xor3,       fnx_xnor        },

    {   fnx_shl3,       fnx_shl3        },
    {   fnx_shlu,       fnx_shlu        },
    {   fnx_shr3,       fnx_shr3        },
    {   fnx_shru,       fnx_shru        },
    {   fnx_bshlu,      fnx_bshlu       },
    {   fnx_bshru,      fnx_bshru       },
    {   fnx_rol,        fnx_rol         },
    {   fnx_ror,        fnx_ror         },

    {   fnx_bitcntl,    fnx_bitcntl     },
    {   fnx_bitcntr,    fnx_bitcntr     },
    {   fnx_bitrevl,    fnx_bitrevl     },
    {   fnx_bitrevr,    fnx_bitrevr     },
    {   fnx_lmo,        fnx_lmo         },
    {   fnx_lmz,        fnx_lmz         },
    {   fnx_rmo,        fnx_rmo         },
    {   fnx_rmz,        fnx_rmz         },

    {   fnx_repack,     fnx_repack      },
    {   fnx_packl,      fnx_packl       },
    {   fnx_packh,      fnx_packh       },
    {   fnx_packlh,     fnx_packlh      },
    {   fnx_btog,       fnx_btog        },
    {   fnx_bset,       fnx_bset        },
    {   fnx_bclr,       fnx_bclr        },
    {   fnx_cmpeq,      fnx_cmpne       },

    {   fnx_cmpltu,     fnx_cmpltu      },
    {   fnx_cmpltfxu,   fnx_cmpltfxu    },
    {   fnx_cmpleu,     fnx_cmpleu      },
    {   fnx_cmplefxu,   fnx_cmplefxu    },
    {   fnx_cmpltua,    fnx_cmpltua     },
    {   fnx_cmpltfxua,  fnx_cmpltfxua   },
    {   fnx_cmpleua,    fnx_cmpleua     },
    {   fnx_cmplefxua,  fnx_cmplefxua   },

    {   fnx_cmplt,      fnx_cmplt       },
    {   fnx_cmpltfx,    fnx_cmpltfx     },
    {   fnx_cmple,      fnx_cmple       },
    {   fnx_cmplefx,    fnx_cmplefx     },
    {   fnx_cmplta,     fnx_cmplta      },
    {   fnx_cmpltfxa,   fnx_cmpltfxa    },
    {   fnx_cmplea,     fnx_cmplea      },
    {   fnx_cmplefxa,   fnx_cmplefxa    },

    {   fnx_min,        fnx_minu        },
    {   fnx_minfx,      fnx_minfxu      },
    {   fnx_max,        fnx_maxu        },
    {   fnx_maxfx,      fnx_maxfxu      },
    {   fnx_bound,      fnx_boundu      },
    {   fnx_boundfx,    fnx_boundfxu    },
    {   fnx_addcirc,    fnx_addcirc     },
    {   fnx_subcirc,    fnx_subcirc     },

    {   fnx_atan2,      fnx_atan2       },
    {   fnx_atan2fx,    fnx_atan2fx     },
    {   fnx_subabs,     fnx_subabsu     },
    {   fnx_subabsfx,   fnx_subabsfxu   },
    {   fnx_dist,       fnx_distu       },
    {   fnx_distfx,     fnx_distfxu     },
    {   fnx_sumsq,      fnx_sumsqu      },
    {   fnx_sumsqfx,    fnx_sumsqfxu    },

    {   fnx_mpyss,      fnx_mpyuu       },
    {   fnx_mpyfxss,    fnx_mpyfxuu     },
    {   fnx_mpysu,      fnx_mpysu       },
    {   fnx_mpyfxsu,    fnx_mpyfxsu     },
    {   fnx_mpyus,      fnx_mpyus       },
    {   fnx_mpyfxus,    fnx_mpyfxus     },
    {   fnx_mpy16,      fnx_mpy16       },
    {   fnx_isqrt,      fnx_isqrtfx     },

    {   fnx_aal,        fnx_aal         },
    {   fnx_aah,        fnx_aah         },
    {   fnx_divs,       fnx_divs        },
    {   fnx_divfxs,     fnx_divfxs      },
    {   fnx_divu,       fnx_divu        },
    {   fnx_divfxu,     fnx_divfxu      },
#define OPX_DIV32S  0x4E
#define OPX_DIV32U  0x4F
    {   NULL,           NULL            },  // DIV32S   0x4E
    {   NULL,           NULL            },  // DIV32U   0x4F

    {   fnx_adds,       fnx_addu        },
    {   fnx_addh,       fnx_addm        },
    {   fnx_subs,       fnx_subs        },
    {   fnx_subu,       fnx_subu        },
    {   fnx_subm,       fnx_subm        },
    {   fnx_subh,       fnx_subh        },
    {   fnx_dmov,       fnx_dmov        },
    {   fnx_addsub,     fnx_addsub      },

    {   fnx_abcd,       fnx_abcdl       },
    {   fnx_abcdh,      fnx_abcdm       },
    {   fnx_sbcd,       fnx_sbcd        },
    {   fnx_sbcdl,      fnx_sbcdl       },
    {   fnx_sbcdm,      fnx_sbcdm       },
    {   fnx_sbcdh,      fnx_sbcdh       },
    {   fnx_i2bcd,      fnx_i2bcd       },
    {   fnx_bcd2i,      fnx_bcd2i       },

    {   fnx_cmpeqa,     fnx_cmpnea      },
};

#define OPX_MAX (sizeof(fnx_tbl) / sizeof(fnx_tbl[0]))

LOCAL INLINE void fnx_div32s(const uint16_t s1lo, const uint16_t s1hi,
                             const uint16_t s2,
                             uint16_t *const dl, uint16_t *const dh)
{
    const int32_t ss1hi = as_i32(s1hi);
    const int32_t i1 = SLS32(ss1hi, 16) + s1lo;
    const int32_t i2 = as_i32(s2);

    *dl = 0x7FFFu;
    *dh = 0x7FFFu;

    if (i2)
    {
        const int32_t q = i1 / i2;
        const int32_t r = i1 % i2;
        if (q >= -0x8000 && q <= 0x7FFF)
        {
            *dl = q;
            *dh = r;
        }
    }
}

LOCAL INLINE void fnx_div32u(const uint16_t s1lo, const uint16_t s1hi,
                             const uint16_t s2,
                             uint16_t *const dl,  uint16_t *const dh)
{
    const uint32_t u1 = (as_u32(s1hi) << 16) + s1lo;
    const uint32_t u2 = as_u32(s2);

    *dl = 0xFFFFu;
    *dh = 0xFFFFu;

    if (u2)
    {
        const uint32_t q = u1 / u2;
        const uint32_t r = u1 % u2;
        if (q <= 0xFFFFu)
        {
            *dl = q;
            *dh = r;
        }
    }
}

#define S2_BAD  (0)
#define S2_REG  (1)
#define S2_PCST (2)
#define S2_NCST (3)

#define S1_REG  (0)
#define S1_XREG (1)

/* ======================================================================== */
/*  FN_EXT_ISA      Execute the extended ISA.                               */
/* ======================================================================== */
int fn_ext_isa(const instr_t *instr, cp1600_t *cp1600)
{
    const uint16_t opcode = instr->opcode.decoded.imm0 >> 8;
    const uint8_t  amode  = instr->opcode.decoded.amode;
    const uint8_t  s_bit  = instr->opcode.decoded.amode & 1;
    const uint8_t  s1type = (instr->opcode.decoded.amode >> 3) & 1;
    const uint8_t  s2type = (instr->opcode.decoded.amode >> 1) & 3;
    const uint8_t  reg0   = instr->opcode.decoded.reg0;
    const uint8_t  reg1   = instr->opcode.decoded.reg1;
    const uint16_t imm1   = instr->opcode.decoded.imm1;
    const uint8_t  xdst   = instr->opcode.decoded.xreg0;
    uint16_t   src1, src1h, src2, *dst_hi, *dst_lo;

    cp1600->r[7]++;
    cp1600->intr = 0;

    /* -------------------------------------------------------------------- */
    /*  Validity checks.  If these fail, the instruction behaves as a NOP.  */
    /* -------------------------------------------------------------------- */
    if (amode >= 0x20 || amode < 0x02 || opcode >= OPX_MAX || s2type == S2_BAD)
        goto leave;

    /* -------------------------------------------------------------------- */
    /*  Bind pointers for dst_hi/dst_lo                                     */
    /* -------------------------------------------------------------------- */
    dst_lo = &( cp1600->xr[xdst] );
    dst_hi = &( cp1600->xr[15 & (1 + xdst)] );

    /* -------------------------------------------------------------------- */
    /*  Update the PV register.                                             */
    /* -------------------------------------------------------------------- */
    CP1600_WR(cp1600, 0x9F8D, *dst_lo);

    /* -------------------------------------------------------------------- */
    /*  Get initial values for src1, src2.                                  */
    /* -------------------------------------------------------------------- */
    src1 = s1type == S1_REG ? cp1600->r[reg0]  : cp1600->xr[reg0];
    src2 = s2type == S2_REG ? cp1600->xr[reg1] : imm1;

    /* -------------------------------------------------------------------- */
    /*  DIV32S / DIV32U get special handling.                               */
    /* -------------------------------------------------------------------- */
    if (opcode == OPX_DIV32S || opcode == OPX_DIV32U)               goto div32;

    /* -------------------------------------------------------------------- */
    /*  Swap src1/src2 if S bit is set in opcode.                           */
    /* -------------------------------------------------------------------- */
    if (s_bit)
    {
        int temp = src1;
        src1 = src2;
        src2 = temp;
    }

    /* -------------------------------------------------------------------- */
    /*  Dispatch to the particular instruction.                             */
    /* -------------------------------------------------------------------- */
    fnx_tbl[opcode][s_bit](src1, src2, dst_lo, dst_hi);

leave:
    cp1600->r[7]++;
    return 9;   /* Does this take 9 or 10 cycles? */

    /* -------------------------------------------------------------------- */
    /*  Handle DIV32S/DIV32U specially. They have a 32-bit src1 argument    */
    /*  and so for now that argument must come from a register.  If that    */
    /*  argument is an R-register, then deposit it in the upper 16 bits.    */
    /*  If that argument is an X-register, fetch a 32-bit register pair.    */
    /* -------------------------------------------------------------------- */
div32:
    if (s_bit) goto leave;

    if (s1type == S1_REG)
    {
        src1h = src1;
        src1  = 0;
    }
    else
    {
        src1h = cp1600->xr[15 & (1 + reg0)];
    }

    if (opcode == OPX_DIV32S)
        fnx_div32s(src1, src1h, src2, dst_lo, dst_hi);
    else
        fnx_div32u(src1, src1h, src2, dst_lo, dst_hi);

    cp1600->r[7]++;
    return 9;
}
