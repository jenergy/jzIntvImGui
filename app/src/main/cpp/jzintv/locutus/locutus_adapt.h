#ifndef LOCUTUS_ADAPT_H_
#define LOCUTUS_ADAPT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "metadata/metadata.h"

typedef struct t_locutus_priv t_locutus_priv;

typedef struct t_locutus_wrap
{
    periph_t        periph;         /*  Peripheral structure.           */
    t_locutus_priv *locutus_priv;   /*  Actual Locutus object ptr.      */
} t_locutus_wrap;


int make_locutus
(
    t_locutus_wrap  *loc_wrap,      /*  pointer to a Locutus wrapper    */
    const char      *luigi_file,    /*  LUIGI file to load into Locutus */
    cp1600_t        *cp1600,
    int             silent,
    const char      *savegame
);

game_metadata_t* get_locutus_metadata( t_locutus_wrap *loc_wrap );

/* Are these needed any longer now that game_metadata_t carries this? */
int get_locutus_compat_ecs  ( t_locutus_wrap *loc_wrap );
int get_locutus_compat_voice( t_locutus_wrap *loc_wrap );
int get_locutus_compat_intv2( t_locutus_wrap *loc_wrap );
int get_locutus_compat_kc   ( t_locutus_wrap *loc_wrap );


uint64_t get_locutus_uid( t_locutus_wrap *loc_wrap );

int get_locutus_was_scrambled( t_locutus_wrap *loc_wrap );
const uint8_t* get_locutus_scramble_druid( t_locutus_wrap *loc_wrap );

#ifdef __cplusplus
}
#endif

#endif
