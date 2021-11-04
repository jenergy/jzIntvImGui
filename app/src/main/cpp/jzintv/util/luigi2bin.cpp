/* ======================================================================== */
/*  Takes a .LUIGI and generates a BIN and CFG from it.                     */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
extern "C"
{
#   include "config.h"
#   include "lzoe/lzoe.h"
#   include "file/file.h"
}
#include "locutus/locutus.hpp"
#include "locutus/loc_to_bin.hpp"
#include "locutus/luigi.hpp"

#include <fstream>
#include <iostream>
#include <string>
using namespace std;

t_locutus locutus;
extern "C"
{
    // These are here just to satisfy the linker.
    int jlp_accel_on, lto_isa_enabled;
}

LOCAL void show_messages(const t_loc_to_bin& loc_to_bin)
{
    const t_string_vec& messages = loc_to_bin.get_messages();
    t_string_vec::const_iterator messages_i = messages.begin();

    int warnings = loc_to_bin.get_warnings();
    int errors   = loc_to_bin.get_errors();

    if ( warnings ) printf("%3d warnings\n", warnings);
    if ( errors   ) printf("%3d errors\n",   errors  );

    while ( messages_i != messages.end() )
    {
        puts( messages_i->c_str() );
        ++messages_i;
    }
}


/* ======================================================================== */
/*  MAIN                                                                    */
/*  This is the main program.  The action happens here.                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    char bin_fn[1024], cfg_fn[1024], rom_fn[1024];
    int fn_len;
    bool ok = true;

    if (argc != 2)
    {
        fprintf(stderr, "usage: luigi2bin foo[.luigi]\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Generate .BIN, .CFG, and .ROM filenames from argument filename.     */
    /*  If the argument lacks a .BIN extension, add one.                    */
    /* -------------------------------------------------------------------- */
    strncpy(rom_fn, argv[1], 1017);
    rom_fn[1017] = 0;

    fn_len = strlen(rom_fn);
    if (fn_len > 6 &&
        stricmp(rom_fn + fn_len - 6, ".luigi") != 0 &&
        !file_exists(rom_fn))
    {
        strcpy(rom_fn + fn_len, ".luigi");
        fn_len += 6;
        if (!file_exists(rom_fn))
        {
            fprintf(stderr, "Could not find '%s' or '%s'\n", argv[1], rom_fn);
            exit(1);
        }
    }

    strcpy(bin_fn, rom_fn);
    strcpy(cfg_fn, rom_fn);

    strcpy(bin_fn + fn_len - 6, ".bin");
    strcpy(cfg_fn + fn_len - 6, ".cfg");

    /* -------------------------------------------------------------------- */
    /*  Slurp in the LUIGI file and deserialize it.                         */
    /* -------------------------------------------------------------------- */
    ifstream ifs(rom_fn, ios::binary | ios::ate);

    if ( !ifs.is_open() )
    {
        perror("fopen()");
        fprintf(stderr, "Could not open %s for reading\n", rom_fn);
        exit(1);
    }

    ifstream::pos_type pos = ifs.tellg();

    t_byte_vec luigi_data(pos);

    ifs.seekg(0, ios::beg);
    ifs.read(reinterpret_cast<char *>(&luigi_data[0]), pos);

    try
    {
        t_luigi::deserialize( locutus, luigi_data );
    } catch ( string& s )
    {
        fflush(stdout);
        fprintf(stderr, "Caught exception: %s\n", s.c_str() );
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Now write that out to a BIN+CFG with t_loc_to_bin                   */
    /* -------------------------------------------------------------------- */
    try
    {
        t_loc_to_bin loc_to_bin( locutus, bin_fn, cfg_fn );

        if ( !loc_to_bin.is_ok() || !loc_to_bin.process() )
        {
            fprintf(stderr, "Errors encountered during processing\n");
            ok = false;
        }

        show_messages( loc_to_bin );
    } catch ( string& s )
    {
        fflush(stdout);
        fprintf(stderr, "Caught exception: %s\n", s.c_str() );
        exit(1);
    }

    return ok ? 0 : 1;
}

