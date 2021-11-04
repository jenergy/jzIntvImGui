#include "config.h"
#include "lzoe/lzoe.h"
#include "bincfg/bincfg.h"

//extern int bc_debug, bc__flex_debug;

int main(int argc, char *argv[])
{
    bc_cfgfile_t *cfg;
    LZFILE *f;

#if 0
    if (argc > 2 && argv[1][0] == '-')
    {
        bc_debug = 1;
        bc__flex_debug = 1;
        argc--;
        argv++;
    } else
    {
        bc_debug = bc__flex_debug = 0;
    }
#endif

    if (argc != 2)
    {
        fprintf(stderr, "bc_test cfgfile\n");
        exit(1);
    }

    f = lzoe_fopen(argv[1], "r");

    if (!f)
    {
        fprintf(stderr, "Couldn't open %s\n", argv[1]);
        exit(1);
    }

    cfg = bc_parse_cfg(f, NULL, argv[1]);
    if (!cfg)
    {
        fprintf(stderr, "no config returned from bc_parse_cfg!\n");
        exit(1);
    }

    if (bc_do_macros(cfg, 0) != 0)
        printf("WARNING:  macro parser wasn't happy.\n");

    bc_print_cfg(stdout, cfg);


    return 0;
}
