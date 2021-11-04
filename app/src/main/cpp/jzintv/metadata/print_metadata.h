#ifndef METADATA_H_
#define METADATA_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"
#include "misc/types.h"
#include "misc/game_metadata.h"
    
/* ------------------------------------------------------------------------ */
/*  PRINT_METADATA   -- Print game metadata.                                */
/* ------------------------------------------------------------------------ */
void print_metadata
(
    const game_metadata_t *const meta
);

#ifdef __cplusplus
}
#endif

#endif

