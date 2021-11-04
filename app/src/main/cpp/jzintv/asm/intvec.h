#ifndef INTVEC_H_
#define INTVEC_H_

/* Very, very loosely modeled after a cutdown of std::vector<int> */

typedef struct intvec_t
{
    int len;
    int alloc;
    int *data;
} intvec_t;

/* new and delete are by value, as we never new/delete struct intvec_t itself */
intvec_t *intvec_new(void);
void intvec_delete(intvec_t *const RESTRICT);

void intvec_push(intvec_t *const RESTRICT, const int val);
void intvec_resize(intvec_t *const RESTRICT, const int len);

/* Appends src to dst */
void intvec_concat(intvec_t *const RESTRICT dst, 
                   const intvec_t *const RESTRICT src);

#endif
