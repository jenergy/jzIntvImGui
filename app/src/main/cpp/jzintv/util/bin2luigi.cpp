/* ======================================================================== */
/*  Takes a BIN (and optional CFG) and generates a .LUIGI from it.          */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
extern "C"
{
#    include "config.h"
#    include "lzoe/lzoe.h"
#    include "file/file.h"
}
#include "locutus/locutus.hpp"
#include "locutus/bin_to_loc.hpp"
#include "locutus/luigi.hpp"

#include <string>
using namespace std;

t_locutus locutus;

extern "C"
{
    // These are here just to satisfy the linker.
    int jlp_accel_on, lto_isa_enabled;
}

LOCAL void show_messages(const t_bin_to_loc& bin_to_loc)
{
    const t_string_vec& messages = bin_to_loc.get_messages();
    t_string_vec::const_iterator messages_i = messages.begin();

    int warnings = bin_to_loc.get_warnings();
    int errors   = bin_to_loc.get_errors();

    if ( warnings ) printf("%3d warnings\n", warnings);
    if ( errors   ) printf("%3d errors\n",   errors  );

    while ( messages_i != messages.end() )
    {
        puts( messages_i->c_str() );
        ++messages_i;
    }
}

const char *const bin_ext[] =
{
    ".bin", ".int", ".itv", ".BIN", ".INT", ".ITV", nullptr
};

LOCAL bool has_bin_extension( const char *ext )
{
    for ( int i = 0; bin_ext[i]; i++ )
        if ( stricmp( ext, bin_ext[i] ) == 0 )
            return true;

    return false;
}

LOCAL int extension_offset( const char *fname )
{
    const char *dot = strrchr( fname, '.' );

    if ( dot && has_bin_extension( dot ) )
        return dot - fname;

    return strlen( fname );
}

LOCAL char *alloc( int len )
{
    char *a = static_cast<char*>(calloc( len, 1 ));
    if (!a)
    {
        fprintf(stderr, "Out of memory allocating %d bytes\n", len);
        exit(1);
    }
    return a;
}

char *bin_fn, *cfg_fn, *luigi_fn;

/* ======================================================================== */
/*  MAIN                                                                    */
/*  This is the main program.  The action happens here.                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    int  ext_ofs;
    bool ok = true;

    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "usage: bin2luigi foo[.bin] [output.luigi]\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  First, see if the filename as-given exists.  If not, try various    */
    /*  extensions known to represent .BIN files.                           */
    /* -------------------------------------------------------------------- */
    if ( file_exists(argv[1]) )
    {
        bin_fn = argv[1];
        ext_ofs = extension_offset( bin_fn );
    } else
    {
        int i;

        ext_ofs = strlen( argv[1] );
        bin_fn = alloc( ext_ofs + 5 );
        strcpy( bin_fn, argv[1] );
        for ( i = 0; bin_ext[i]; i++ )
        {
            strcpy( bin_fn + ext_ofs, bin_ext[i] );
            if ( file_exists( bin_fn ) )
                break;
        }
        if ( !bin_ext[i] )
        {
            fprintf(stderr, "Could not find input file\n");
            exit(1);
        }
    }

    cfg_fn = alloc( ext_ofs + 5 );

    strcpy( cfg_fn, bin_fn );
    strcpy( cfg_fn + ext_ofs, ".cfg" );
    if ( !file_exists( cfg_fn ) )
        strcpy( cfg_fn + ext_ofs, ".CFG" );

    /* -------------------------------------------------------------------- */
    /*  If the user provided a LUIGI file name, use that.  Otherwise, make  */
    /*  one by appending .luigi to the given filename.                      */
    /* -------------------------------------------------------------------- */
    if ( argc == 3 )
    {
        luigi_fn = argv[2];
    } else
    {
        luigi_fn = alloc( ext_ofs + 7 );
        strcpy( luigi_fn, bin_fn );
        strcpy( luigi_fn + ext_ofs, ".luigi" );
    }

    /* -------------------------------------------------------------------- */
    /*  Use t_bin_to_loc to populate Locutus.                               */
    /* -------------------------------------------------------------------- */
    try
    {
        t_bin_to_loc bin_to_loc( bin_fn, cfg_fn, locutus, true );

        if ( !bin_to_loc.is_ok() || !bin_to_loc.process() )
        {
            fprintf(stderr, "Errors encountered during conversion\n");
            ok = false;
        }

        show_messages( bin_to_loc );

    } catch ( string& s )
    {
        fprintf(stderr, "caught exception: %s\n", s.c_str() );
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If all went well, serialize and write the file.                     */
    /* -------------------------------------------------------------------- */
    if ( ok )
    {
        try
        {
            t_byte_vec data = t_luigi::serialize( locutus );

            FILE *fo = fopen( luigi_fn, "wb" );

            if ( !fo )
            {
                perror("fopen()");
                fprintf(stderr, "Could not open %s for writing\n", luigi_fn);
                exit(1);
            }

            long long written = fwrite( static_cast<void *>(&data[0]),
                                        1, data.size(), fo );

            if ( written != static_cast<long long>(data.size()) )
            {
                perror("fwrite()");
                fprintf(stderr, "Error writing output (short write?)\n");
                exit(1);
            }
            fclose( fo );

        } catch ( string& s )
        {
            fprintf(stderr, "caught exception: %s\n", s.c_str() );
            exit(1);
        }
    }

    return ok ? 0 : 1;
}
