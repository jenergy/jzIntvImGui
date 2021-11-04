#include "config.h"
#include "asm/intvec.h"

intvec_t *intvec_new(void)
{
    intvec_t *const iv = CALLOC(intvec_t, 1);;

    iv->len   = 0;
    iv->alloc = 1 << 10;
    iv->data  = CALLOC(int, (1 << 10));
    return iv;
}
    
void intvec_delete(intvec_t *const RESTRICT iv)
{
    if (iv)
    {
        if (iv->data) free(iv->data);
        free(iv);
    }
}

void intvec_push(intvec_t *const RESTRICT iv, const int val)
{
    if (iv->len >= iv->alloc)
    {
        iv->alloc <<= 1;
        iv->data = REALLOC(iv->data, int, iv->alloc);
        memset(iv->data + iv->len, 0, (iv->alloc - iv->len) * sizeof(int));
    }

    iv->data[iv->len++] = val;
}

void intvec_resize(intvec_t *const RESTRICT iv, const int len)
{
    if (iv->len > iv->alloc)
    {
        iv->alloc <<= 1;
        iv->data = REALLOC(iv->data, int, iv->alloc);
        memset(iv->data + iv->len, 0, (iv->alloc - iv->len) * sizeof(int));
    }

    iv->len = len;
}

void intvec_concat(intvec_t *const RESTRICT dst,
                   const intvec_t *const RESTRICT src)
{
    if (dst->len + src->len > dst->alloc) 
    {
        while (dst->len + src->len > dst->alloc) 
            dst->alloc <<= 1;

        dst->data = REALLOC(dst->data, int, dst->alloc);
        memset(dst->data + dst->len, 0, (dst->alloc - dst->len) * sizeof(int));
    }

    memcpy(dst->data + dst->len, src->data, src->len * sizeof(int));
    dst->len += src->len;
}
