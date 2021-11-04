
#include "locutus_types.hpp"
#include "locutus.hpp"
#include "loc_to_bin.hpp"
#include "luigi.hpp"

#include <stdio.h>
#include <cstdlib>

using namespace std;
int jlp_accel_on, lto_isa_enabled;

void show_messages(const t_loc_to_bin& loc_to_bin)
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

int main()
{
    t_locutus locutus;

    // Make a simple ROM segment at $5xxx
    for (int i = 0; i < 4096; i++)
        locutus.write( 0x5000 + i, i );

    t_perm read_only(1);

    for (int para = 0x50; para < 0x60; para++)
    {
        locutus.set_mem_map ( para, true, para      );
        locutus.set_mem_perm( para, true, read_only );
    }

    // Make a preloaded segment at $6xxx
    for (int i = 0; i < 4096; i++)
        locutus.write( 0x6000 + i, 0x6000 + i );

    for (int para = 0x60; para < 0x70; para++)
        locutus.set_mem_map ( para, true, para );

    const char *f_bin_path = "test.bin";
    const char *f_cfg_path = "test.cfg";

    // Make a page-flipped range at $F000 pages 0 and 1
    t_perm flip_only(0x8), flip_read(0x9);

    for (int chap = 0xF; chap < 0x10; chap++)
    {
        for (int page = 0; page < 2; page++)
            locutus.set_pageflip( chap, page,
                                  (0x7F000 - 0x1000*page) >> 12, flip_read );

        for (int page = 2; page < 16; page++)
            locutus.set_pageflip_perm( chap, page, flip_only );
    }

    for (int para = 0xF0; para < 0x100; para++)
    {
        locutus.set_mem_map ( para, true, (0x70000 + (para << 8)) >> 8 );
        locutus.set_mem_perm( para, true, read_only );
    }

    for (int i = 0x7E000; i < 0x80000; ++i)
        locutus.write( i, i ^ (i >> 8) );

    // Make an Intellicart bankswitched RAM 8 section from $AA00 - $BB00
    t_perm ram8_banksw( 0xF );
    for ( int para = 0xAA; para <= 0xBB ; ++para )
        locutus.set_mem_perm( para, true, ram8_banksw );


    // Make an Intellicart bankswitched WOM 8 section from $CC00 - $CD00
    t_perm wom8_banksw( 0xE );
    for ( int para = 0xCC; para <= 0xCD ; ++para )
        locutus.set_mem_perm( para, true, wom8_banksw );


    t_loc_to_bin loc_to_bin( locutus, f_bin_path, f_cfg_path, true );

    if ( !loc_to_bin.is_ok() )
    {
        fprintf(stderr, "Error constructing loc_to_bin\n%d\n%d\n",
                loc_to_bin.get_f_bin_err(),
                loc_to_bin.get_f_cfg_err() );
        show_messages( loc_to_bin );
        exit(1);
    }

    if ( !loc_to_bin.process() )
    {
        fprintf(stderr, "Error processing loc_to_bin\n");
        show_messages( loc_to_bin );
        exit(1);
    }

    show_messages( loc_to_bin );


    // Also try to LUIGI the file...
    t_byte_vec luigi_data;
    try
    {
        luigi_data = t_luigi::serialize  ( locutus    );
    } catch ( string& s )
    {
        fflush(stdout);
        fprintf( stderr, "Caught during serialize: %s\n", s.c_str() );
        exit(1);
    }


    // Write it out
    FILE *f_luigi = fopen("test.luigi", "wb");

    if (!f_luigi)
    {
        perror("fopen: ");
        exit(1);
    }

    fwrite( (const void *)&(luigi_data[0]), 1, luigi_data.size(), f_luigi );
    fclose(f_luigi);

    // Deserialize and write a second BIN+CFG
    t_locutus  locutus2;
    try
    {
        t_luigi::deserialize( locutus2, luigi_data );
    } catch ( string& s )
    {
        fflush(stdout);
        fprintf( stderr, "Caught during deserialize: %s\n", s.c_str() );
        exit(1);
    }

    t_loc_to_bin loc_to_bin2( locutus2, "test2.bin", "test2.cfg", true );

    if ( !loc_to_bin2.is_ok() )
    {
        fprintf(stderr, "Error constructing loc_to_bin2\n%d\n%d\n",
                loc_to_bin2.get_f_bin_err(),
                loc_to_bin2.get_f_cfg_err() );
        show_messages( loc_to_bin2 );
        exit(1);
    }

    if ( !loc_to_bin2.process() )
    {
        fprintf(stderr, "Error processing loc_to_bin2\n");
        show_messages( loc_to_bin2 );
        exit(1);
    }

    show_messages( loc_to_bin2 );




    return 0;
}
