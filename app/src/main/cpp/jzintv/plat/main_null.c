/* This file exists because, on some platforms, sdl.h redefines 'main'. */
/* So, the null platform just needs to forward the call through, unless we
   are a pure back-end, in which case we have nothing here... */
#include "config.h"

#ifndef USE_AS_BACKEND
int main(int argc, char *argv[])
{
    return jzintv_entry_point(argc, argv);
}
#else
/* to silence compiler warnings/errors about empty compilation units */
extern void unused_function(void);
void unused_function(void) { }
#endif
