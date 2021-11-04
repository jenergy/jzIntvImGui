#include <stdio.h> /* for NULL */

#define QQ(x) #x
#define Q(x) QQ(x)

const char *svn_revision =
#if defined(JZINTV_SVN_REV)
#   if (JZINTV_SVN_REV + 0) > 1
        "SVN Revision " Q(JZINTV_SVN_REV)
#       if JZINTV_SVN_DTY == 1
            " (1 modified file)"
#       endif
#       if (JZINTV_SVN_DTY + 0) > 1
            " (" Q(JZINTV_SVN_DTY) " modified files)"
#       endif
    ;
#   else
        NULL;
#   endif
#else
    NULL;
#endif
