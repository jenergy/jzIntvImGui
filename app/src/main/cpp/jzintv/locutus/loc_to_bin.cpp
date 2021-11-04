// ======================================================================== //
//  Utilities:                                                              //
//                                                                          //
//      class t_loc_to_bin_impl                                             //
//                                                                          //
//  You figure it out.  ;-)                                                 //
// ======================================================================== //

#include "config.h"
extern "C"
{
#   include "misc/printer.h"
#   include "misc/types.h"
#   include "metadata/metadata.h"
#   include "metadata/cfgvar_metadata.h"
}

#include <errno.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <memory>
#include <functional>

using namespace std;

#include "locutus_types.hpp"
#include "locutus.hpp"
#include "loc_to_bin.hpp"


// ------------------------------------------------------------------------ //
//  CLASS T_LOC_TO_BIN_IMPL                                                 //
// ------------------------------------------------------------------------ //
class t_loc_to_bin_impl
{
  private:
    const t_locutus&    locutus;
    FILE* const         f_bin;          const int f_bin_err;
    FILE* const         f_cfg;          const int f_cfg_err;
    const bool          self_close;

    uint32_t            file_offset;

    int                 warnings;
    int                 errors;

    t_hunk_vec          mappings;           // [mapping]
    t_hunk_vec          preloads;           // [preload]
    t_memattr_vec       memattrs;           // [memattr]
    t_banksw_vec        bankswitches;       // [bankswitch]
    unique_ptr<cfg_var_t, decltype(free_cfg_var_list)*> cfg_vars;   // [vars]

    t_string_vec        messages;

    void scan_and_classify_locutus    ();
    void decode_features_and_metadata ();
    void write_mappings               ();
    void write_preloads               ();
    void write_memattrs               ();
    void write_bankswitches           ();
    void write_vars                   ();

    void record_error( string error )
    {
        messages.push_back( string("ERROR: ") + error );
        ++errors;
    }

    void record_warning( string warning )
    {
        messages.push_back( string("WARNING: ") + warning );
        ++warnings;
    }

    template <typename T>
    void merge_adjacent( T& vec )
    {
        if ( vec.size() < 2 )
            return;

        auto i = vec.size();
        while ( --i != 0 )
            if ( vec[i - 1].can_merge_with( vec[i] ) )
            {
                vec[i - 1].merge_with( vec[i] );
                vec.erase( vec.begin() + i );
            }
    }

    template <typename T>
    void split_at_4k( T& vec )
    {
        if ( vec.size() < 1 )
            return;

        auto i = vec.size();
        while ( i != 0 )
        {
            // Does it cross a 4K page boundary?
            if ( ((vec[i - 1].s_addr ^ vec[i - 1].e_addr) & ~0xFFF) == 0 )
            {
                // No: move to next lower entry
                i--;
                continue;
            }

            // Yes:  Split at the 4K boundary
            vec.insert( vec.begin() + i,
                        vec[i - 1].split_at( vec[i - 1].e_addr & ~0xFFF ) );
        }
    }

    static std::string quote_string( const std::string& s );

  public:
    t_loc_to_bin_impl
    (
        const t_locutus& locutus_,
        FILE* const      f_bin_,
        FILE* const      f_cfg_,
        const bool       self_close_ = false
    ) :
        locutus     ( locutus_      ),
        f_bin       ( f_bin_        ), f_bin_err( 0 ),
        f_cfg       ( f_cfg_        ), f_cfg_err( 0 ),
        self_close  ( self_close_   ),
        file_offset ( 0             ),
        warnings    ( 0 ), errors   ( 0 ),
        mappings    ( ),
        preloads    ( ),
        memattrs    ( ),
        bankswitches( ),
        cfg_vars    ( nullptr, free_cfg_var_list ),
        messages    ( )
    {
    }

    t_loc_to_bin_impl
    (
        const t_locutus&  locutus_,
        const char* const f_bin_path,
        const char* const f_cfg_path,
        const bool        self_close_ = true
    ) :
        locutus     ( locutus_                  ),
        f_bin       ( fopen( f_bin_path, "wb" ) ), f_bin_err( errno ),
        f_cfg       ( fopen( f_cfg_path, "w"  ) ), f_cfg_err( errno ),
        self_close  ( self_close_               ),
        file_offset ( 0                         ),
        warnings    ( 0 ), errors   ( 0 ),
        mappings    ( ),
        preloads    ( ),
        memattrs    ( ),
        bankswitches( ),
        cfg_vars    ( nullptr, free_cfg_var_list ),
        messages    ( )
    {
    }

    bool is_ok() const
    {
        return !errors &&
               f_bin && !ferror( f_bin ) &&
               f_cfg && !ferror( f_cfg );
    }

    bool process()
    {
        if ( !is_ok() )
            return false;

        // Read through Locutus and classify everything
        scan_and_classify_locutus();

        // Canonicalize by sorting ranges.
        //
        // For mappings, we sort by page first to ensure we coalesce all
        // fragments associated with a paged ROM.  Later we will re-sort
        // by address.
        sort( mappings.begin(), mappings.end(), t_bin_hunk::page_then_addr );
        sort( preloads.begin(), preloads.end(), t_bin_hunk::page_then_addr );
        sort( memattrs.begin(),     memattrs.end()      );
        sort( bankswitches.begin(), bankswitches.end()  );

        // Merge things together where possible
        merge_adjacent( mappings     );
        merge_adjacent( preloads     );
        merge_adjacent( memattrs     );
        merge_adjacent( bankswitches );

        // Split mappings and preloads at 4K address boundaries.
        // Not strictly necessary, but improves readability.
        split_at_4k( mappings );
        split_at_4k( preloads );

        // Re-sort mappings and preloads by address first.
        sort( mappings.begin(), mappings.end(), t_bin_hunk::addr_then_page );
        sort( preloads.begin(), preloads.end(), t_bin_hunk::addr_then_page );

        // Build up variable table
        decode_features_and_metadata();

        // Now write it all out
        if ( is_ok() ) write_mappings    ();
        if ( is_ok() ) write_preloads    ();
        if ( is_ok() ) write_memattrs    ();
        if ( is_ok() ) write_bankswitches();
        if ( is_ok() ) write_vars        ();

        return is_ok();
    }

    int                 get_f_bin_err()     const   { return f_bin_err;     }
    int                 get_f_cfg_err()     const   { return f_cfg_err;     }
    uint32_t            get_file_offset()   const   { return file_offset;   }
    int                 get_errors()        const   { return errors;        }
    int                 get_warnings()      const   { return warnings;      }
    const t_string_vec& get_messages()      const   { return messages;      }

    ~t_loc_to_bin_impl()
    {
        if ( self_close )
        {
            if ( f_bin ) fclose( f_bin );
            if ( f_cfg ) fclose( f_cfg );
        }
    }
};

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::SCAN_AND_CLASSIFY_LOCUTUS                            //
// ------------------------------------------------------------------------ //
void t_loc_to_bin_impl::scan_and_classify_locutus()
{
    bitset< 512 * 1024 > preloaded;
    bitset< 65536      > memattred;

    preloaded.reset();

    // -------------------------------------------------------------------- //
    //  Scan INTV memory by chapter, page and offset to classify each       //
    //  memory-mapped ROM byte (either page-flipped or not).                //
    // -------------------------------------------------------------------- //
    for ( int chap = 0; chap < 0x10; ++chap )
    {
        bitset<16> flips;

        // ---------------------------------------------------------------- //
        //  Ensure this chapter is not page-flipped.                        //
        // ---------------------------------------------------------------- //
        for ( int page = 0; page < 0x10; ++page )
            flips[ page ] =
                locutus.get_pageflip_perm(chap, page)[ LOCUTUS_PERM_BANKSW ];


        // ---------------------------------------------------------------- //
        //  Not page-flipped:  Look for non-bankswitched ROM.               //
        // ---------------------------------------------------------------- //
        if ( flips.to_ulong() == 0x0000 )
        {
            const int s_para = chap << 4, e_para = s_para + 0xF;
            for ( int para = s_para; para <= e_para; ++para )
            {
                const t_perm perm = locutus.get_mem_perm( para, true );

                // -------------------------------------------------------- //
                //  Look for something ROM-like, not Intellicart-banksw.    //
                // -------------------------------------------------------- //
                if ( !perm[ LOCUTUS_PERM_READ   ] ||
                      perm[ LOCUTUS_PERM_WRITE  ] ||
                      perm[ LOCUTUS_PERM_BANKSW ] )
                     continue;

                // -------------------------------------------------------- //
                //  Readable para found.  Does it have initialized data?    //
                // -------------------------------------------------------- //
                const uint32_t  intv_base = para << 8;
                const uint32_t  locu_base =
                                        locutus.get_mem_map( para, true ) << 8;
                t_word_vec      data;
                int             s_addr  = -1;
                bool            in_span = false;

                for ( int ofs = 0; ofs < 0x100; ++ofs )
                {
                    const uint32_t intv_addr = intv_base + ofs;
                    const uint32_t locu_addr = locu_base + ofs;

                    if ( locutus.get_initialized( locu_addr ) )
                    {
                        if ( !in_span )
                        {
                            data.clear();
                            s_addr  = intv_addr;
                            in_span = true;
                        }
                    } else
                    {
                        if ( in_span )
                        {
                            mappings.push_back(
                                t_bin_hunk( s_addr, intv_addr - 1,
                                            PAGE_NONE, true, data ) );

                            data.clear();
                        }
                        in_span = false;
                    }

                    if ( in_span )
                    {
                        const uint16_t word = locutus.read( locu_addr );
                        data.push_back( host_to_be_16( word  ) );
                        preloaded.set( locu_addr );
                        memattred.set( intv_addr );
                    }
                }

                if ( in_span )
                {
                    mappings.push_back(
                        t_bin_hunk( s_addr, intv_base + 0xFF,
                                    PAGE_NONE, true, data ) );
                }
            }
            continue;  /* done with this chapter */
        }

        // ---------------------------------------------------------------- //
        //  Page-flipped:  Pull out each readable page into its own 4K      //
        //  ROM segment.  For compatbility, force to 4K, padding with FFs.  //
        // ---------------------------------------------------------------- //
        for ( int page = 0; page < 0x10; ++page )
        {
            const t_perm perm = locutus.get_pageflip_perm( chap, page );

            if ( perm[ LOCUTUS_PERM_WRITE ] )
            {
                char buf[100];
                sprintf( buf, "Pageflipped ROM at chapter %.1X, page %.1X "
                              "not yet supported.  Ignored.",
                              chap, page );
                record_error( string( buf ) );
                continue;
            }

            if ( !perm[ LOCUTUS_PERM_READ ] )
                continue;

            if ( perm[ LOCUTUS_PERM_NARROW ] )
            {
                char buf[100];
                sprintf( buf, "NARROW flag set on ROM at chapter %.1X, "
                              "page %.1X.  Ignored.",
                              chap, page );
                record_warning( string( buf ) );
                continue;
            }

            if ( !perm[ LOCUTUS_PERM_BANKSW ] )
            {
                char buf[100];
                sprintf(buf,
                    "Chapter %.2X page %.1X is readable, but does not have "
                    "page-flip bit set.  Perm=%.1X",
                    chap, page, static_cast<unsigned int>(perm.to_ulong()));

                record_warning( string( buf ) );
            }

            const uint32_t  intv_base = chap << 12;
            const uint32_t  locu_base =
                                locutus.get_pageflip_map( chap, page ) << 12;
            t_word_vec      data( 4096, 0xFFFF );
            assert( data.size() == 4096 && data[0] == 0xFFFF );

            for ( int ofs = 0; ofs < 4096; ++ofs )
            {
                const uint32_t intv_addr = intv_base + ofs;
                const uint32_t locu_addr = locu_base + ofs;
                if ( locutus.get_initialized( locu_addr ) )
                {
                    preloaded.set( locu_addr );
                    data[ofs] = host_to_be_16( locutus.read( locu_addr ) );
                }
                memattred.set( intv_addr );
            }

            mappings.push_back(
                t_bin_hunk( intv_base, intv_base + 0xFFF, page, true, data ) );
        }
    }

    // -------------------------------------------------------------------- //
    //  Now scan Locutus for segments that have not yet been preloaded by   //
    //  virtue of being memory-mapped in the first loop.                    //
    //                                                                      //
    //  Warn if any preload segments are above 64K, as the Intellicart      //
    //  does not support [preload] above 64K.                               //
    // -------------------------------------------------------------------- //
    {
        bool        in_span = false;
        int         locu_s_addr = 0;
        t_word_vec  data;
        bool        above_64k = false;

        for ( int locu_addr = 0; locu_addr < 512 * 1024; ++locu_addr )
        {
            if ( in_span &&
                 ( preloaded[ locu_addr ] ||
                   !locutus.get_initialized( locu_addr ) ) )
            {
                preloads.push_back(
                    t_bin_hunk( locu_s_addr, locu_addr - 1, PAGE_NONE,
                                false, data ) );
                in_span = false;

                if ( locu_addr > 0x10000 )
                    above_64k = true;

            } else if ( !in_span &&
                        !preloaded[ locu_addr ] &&
                        locutus.get_initialized( locu_addr ) )
            {
                in_span     = true;
                locu_s_addr = locu_addr;
                data.resize(0);
            }
            if ( in_span )
            {
                const uint16_t word = locutus.read( locu_addr );
                data.push_back( host_to_be_16( word ) );
                preloaded.set( locu_addr );
            }
        }

        if ( in_span )
        {
            preloads.push_back(
                t_bin_hunk( locu_s_addr, 512 * 1024 - 1, PAGE_NONE,
                            false, data ) );
            above_64k = true;
        }

        if ( above_64k )
            record_warning( string("preload section above 64K boundary") );
    }

    // -------------------------------------------------------------------- //
    //  Look for RAM, ROM and WOM spans.  Do not emit ROM spans if they     //
    //  have any preloaded bits, as those are covered under [mapping].      //
    // -------------------------------------------------------------------- //
    for ( int para = 0; para < 0x100; ++para )
    {
        const t_perm perm = locutus.get_mem_perm( para, true );

        if ( !perm[LOCUTUS_PERM_READ ] &&
             !perm[LOCUTUS_PERM_WRITE] )
            continue;

        const int width = perm[LOCUTUS_PERM_WRITE ] &&
                          perm[LOCUTUS_PERM_NARROW] ? 8 : 16;

        const int type  =
            perm[LOCUTUS_PERM_READ] &&  perm[LOCUTUS_PERM_WRITE] ? MEMATTR_RAM
        :   perm[LOCUTUS_PERM_READ] && !perm[LOCUTUS_PERM_WRITE] ? MEMATTR_ROM
        :  !perm[LOCUTUS_PERM_READ] &&  perm[LOCUTUS_PERM_WRITE] ? MEMATTR_WOM
        :                                                          MEMATTR_BAD;

        if ( type == MEMATTR_BAD )
            continue;


        // If it's RAM or WOM, just output the span
        if ( type != MEMATTR_ROM )
        {
            memattrs.push_back(
                    t_memattr_span( para << 8, (para << 8) + 0xFF,
                                    type, width ) );
            continue;
        }

        // If it's ROM, only output the spans that haven't been mapped by some
        // other means.
        bool in_span     = false;
        int  intv_s_addr = -1;
        for ( int ofs = 0; ofs < 0x100; ++ofs )
        {
            const int intv_addr = (para << 8) + ofs;

            if ( in_span && memattred[ intv_addr ] )
            {
                memattrs.push_back( t_memattr_span( intv_s_addr, intv_addr,
                                                    type, width ) );
                in_span = false;
            }
            else if ( !  in_span && !memattred[ intv_addr ] )
            {
                intv_s_addr = intv_addr;
                in_span = true;
            }
        }

        if ( in_span )
            memattrs.push_back(
                    t_memattr_span( intv_s_addr, (para << 8) + 0xFF,
                                    type, width ) );

    }

    // -------------------------------------------------------------------- //
    //  Look for [bankswitch] spans.                                        //
    // -------------------------------------------------------------------- //
    for ( int para = 0; para < 0x100; ++para )
    {
        const t_perm perm = locutus.get_mem_perm( para, true );

        if ( perm[LOCUTUS_PERM_BANKSW] )
            bankswitches.push_back(
                    t_banksw_span( para << 8, (para << 8) + 255 ) );

    }

}

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::DECODE_FEATURES_AND_METADATA                         //
// ------------------------------------------------------------------------ //
void t_loc_to_bin_impl::decode_features_and_metadata()
{
    game_metadata_t *gm = locutus.get_metadata().to_game_metadata();
    if (!gm)
        record_error("Unable to extract game metadata");
    
    cfg_vars.reset( cfgvars_from_game_metadata(gm) );
    free_game_metadata(gm);
}

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::WRITE_MAPPINGS                                       //
// ------------------------------------------------------------------------ //
void t_loc_to_bin_impl::write_mappings()
{
    if ( mappings.size() == 0 )
        return;

    fprintf( f_cfg, "[mapping]\015\012" );

    t_hunk_vec::const_iterator mappings_i = mappings.begin();

    while ( mappings_i != mappings.end() )
    {
        const uint32_t length = mappings_i->data.size();

        fprintf( f_cfg, "$%.4X - $%.4X = $%.4X",
                 file_offset, file_offset + length - 1, mappings_i->s_addr );

        if ( mappings_i->page == PAGE_NONE )
            fputs( "\015\012", f_cfg );
        else
            fprintf( f_cfg, " PAGE %1X\015\012", mappings_i->page );

        fwrite( static_cast<const void *>(&(mappings_i->data[0])),
                2, length, f_bin );

        file_offset += length;

        ++mappings_i;
    }
    fputs("\015\012", f_cfg);
}

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::WRITE_PRELOADS                                       //
// ------------------------------------------------------------------------ //
void t_loc_to_bin_impl::write_preloads()
{
    if ( preloads.size() == 0 )
        return;

    fprintf( f_cfg, "[preload]\015\012" );

    t_hunk_vec::const_iterator preloads_i = preloads.begin();

    while ( preloads_i != preloads.end() )
    {
        const uint32_t len = preloads_i->data.size();

        fprintf( f_cfg, "$%.4X - $%.4X = $%.4X\015\012",
                 file_offset, file_offset + len - 1, preloads_i->s_addr );

        fwrite( static_cast<const void *>(&(preloads_i->data[0])),
                2, len, f_bin );

        file_offset += preloads_i->data.size();

        ++preloads_i;
    }
    fputs("\015\012", f_cfg);
}

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::WRITE_MEMATTRS                                       //
// ------------------------------------------------------------------------ //
static const char *type_string[4] = { "BAD", "ROM", "WOM", "RAM" };

void t_loc_to_bin_impl::write_memattrs()
{
    if ( memattrs.size() == 0 )
        return;

    fprintf( f_cfg, "[memattr]\015\012" );

    t_memattr_vec::const_iterator memattrs_i = memattrs.begin();

    while ( memattrs_i != memattrs.end() )
    {
        fprintf( f_cfg, "$%.4X - $%.4X = %s %d\015\012",
                 memattrs_i->s_addr, memattrs_i->e_addr,
                 type_string[ memattrs_i->type ],
                 memattrs_i->width );

        ++memattrs_i;
    }
    fputs("\015\012", f_cfg);
}

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::WRITE_MEMATTRS                                       //
// ------------------------------------------------------------------------ //
void t_loc_to_bin_impl::write_bankswitches()
{
    if ( bankswitches.size() == 0 )
        return;

    fprintf( f_cfg, "[bankswitch]\015\012" );

    t_banksw_vec::const_iterator bankswitches_i = bankswitches.begin();

    while ( bankswitches_i != bankswitches.end() )
    {
        fprintf( f_cfg, "$%.4X - $%.4X\015\012",
                 bankswitches_i->s_addr, bankswitches_i->e_addr );

        ++bankswitches_i;
    }
    fputs("\015\012", f_cfg);
}

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::QUOTE_STRING                                         //
// ------------------------------------------------------------------------ //
std::string t_loc_to_bin_impl::quote_string( const std::string& str )
{
    std::string result;
    bool ok = true;

    for ( const auto c : str )
        if ( std::strchr( " \t\r\n;[]$=-,\"", c ) )
        {
            ok = false;
            break;
        }

    if ( ok )
    {
        result = str;
        return result;
    }

    // Far from optimal, but easy to write and not perf critical
    result = "\"";

    for ( const auto c : str )
    {
        if ( c == '"' )
            result += '\\';
        result += c;
    }

    result += '"';

    return result;
}

// ------------------------------------------------------------------------ //
//  T_LOC_TO_BIN_IMPL::WRITE_VARS                                           //
// ------------------------------------------------------------------------ //
void t_loc_to_bin_impl::write_vars()
{
    if ( cfg_vars )
    {
        printer_t p_cfg = printer_to_file(f_cfg);
        fprintf( f_cfg, "[vars]\015\012" );
        print_cfg_var_list( cfg_vars.get(), &p_cfg );
    }
}

// ------------------------------------------------------------------------ //
//  Forwarding from T_LOC_TO_BIN to T_LOC_TO_BIN_IMPL                       //
// ------------------------------------------------------------------------ //

t_loc_to_bin::t_loc_to_bin
(
    const t_locutus&    locutus,
    FILE *const         f_bin,
    FILE *const         f_cfg,
    const bool          self_close
)
{
    impl = new t_loc_to_bin_impl( locutus, f_bin, f_cfg, self_close );
}

t_loc_to_bin::t_loc_to_bin
(
    const t_locutus&    locutus,
    const char* const   f_bin_path,
    const char* const   f_cfg_path,
    const bool          self_close
)
{
    impl = new t_loc_to_bin_impl( locutus, f_bin_path, f_cfg_path, self_close );
}

t_loc_to_bin::~t_loc_to_bin()
{
    delete impl;
}

bool t_loc_to_bin::is_ok()         const    { return impl->is_ok();         }
bool t_loc_to_bin::process()                { return impl->process();       }
int  t_loc_to_bin::get_f_bin_err() const    { return impl->get_f_bin_err(); }
int  t_loc_to_bin::get_f_cfg_err() const    { return impl->get_f_cfg_err(); }
int  t_loc_to_bin::get_errors()    const    { return impl->get_errors();    }
int  t_loc_to_bin::get_warnings()  const    { return impl->get_warnings();  }

const t_string_vec& t_loc_to_bin::get_messages() const
{
    return impl->get_messages();
}

uint32_t t_loc_to_bin::get_file_offset() const
{
    return impl->get_file_offset();
}

