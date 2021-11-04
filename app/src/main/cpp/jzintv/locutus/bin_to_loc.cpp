// ======================================================================== //
//  Utilities:                                                              //
//                                                                          //
//      class t_loc_to_bin_impl                                             //
//      class t_bin_to_loc_impl                                             //
//                                                                          //
//  You figure it out.  ;-)                                                 //
// ======================================================================== //

#include "config.h"
extern "C"
{
#   include "lzoe/lzoe.h"
#   include "bincfg/bincfg.h"
#   include "misc/file_crc32.h"
#   include "metadata/metadata.h"
#   include "metadata/cfgvar_metadata.h"
#   include <errno.h>
}

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

using namespace std;

#include "locutus_types.hpp"
#include "locutus.hpp"
#include "bin_to_loc.hpp"

// ======================================================================== //
//  CLASS T_BIN_TO_LOC_IMPL                                                 //
// ======================================================================== //
class t_bin_to_loc_impl
{
  private:
    const char* const   bin_fn;
    const char* const   cfg_fn;
    t_locutus&          locutus;
    t_metadata&         metadata;

    LZFILE* const       f_cfg;
    bc_cfgfile_t* const bincfg;

    int                 warnings;
    int                 errors;

    t_string_vec        messages;

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

    void add_segment( const uint16_t intv_s_addr, const uint32_t locu_s_addr,
                      const uint16_t span_len,    const t_perm   perm,
                      const uint16_t *data,       const int      page );

    void process_cfg_vars( );

  public:
    t_bin_to_loc_impl
    (
        const char* const   bin_fn_,
        const char* const   cfg_fn_,
        t_locutus&          locutus_,
        const bool          do_macros = true
    )
    :   bin_fn  ( bin_fn_  ),
        cfg_fn  ( cfg_fn_  ),
        locutus ( locutus_ ), metadata( locutus_.get_metadata() ),
        f_cfg   ( cfg_fn_ ? lzoe_fopen( cfg_fn_, "r" ) : nullptr ),
        bincfg  ( bc_parse_cfg( f_cfg, bin_fn_, f_cfg ? cfg_fn_ : nullptr ) ),
        warnings( 0 ), errors( 0 ),
        messages( )
    {
        // ---------------------------------------------------------------- //
        //  If we opened a config file, close it.                           //
        // ---------------------------------------------------------------- //
        if ( f_cfg )
            lzoe_fclose( f_cfg );

#ifndef BC_NODOMACRO
        // ---------------------------------------------------------------- //
        //  Apply any statically safe macros.                               //
        // ---------------------------------------------------------------- //
        if ( do_macros )
            bc_do_macros( bincfg, 0 );
#endif
        // ---------------------------------------------------------------- //
        //  Populate the config with corresponding BIN data.                //
        // ---------------------------------------------------------------- //
        if ( bc_read_data( bincfg ) )
            record_error( string("Error reading data for CFG file.") );

        // ---------------------------------------------------------------- //
        //  Look for configuration variables that affect the LUIGI file.    //
        // ---------------------------------------------------------------- //
        process_cfg_vars();

        // ---------------------------------------------------------------- //
        //  Set our UID to { original BIN CRC, original CFG CRC } or        //
        //  { original BIN CRC, 0 } if no CFG provided.                     //
        // ---------------------------------------------------------------- //
        const uint64_t cfg_crc =
            f_cfg && cfg_fn ? uint64_t( file_crc32( cfg_fn ) ) << 32 : 0;

        locutus.set_uid( cfg_crc | file_crc32( bin_fn ) );
    }

    ~t_bin_to_loc_impl()
    {
#ifndef BC_NOFREE
        // ---------------------------------------------------------------- //
        //  Discard the parsed config                                       //
        // ---------------------------------------------------------------- //
        bc_free_cfg( bincfg );
#endif
    }

    bool                process();
    bool                is_ok()             const   { return !errors;       }
    int                 get_errors()        const   { return errors;        }
    int                 get_warnings()      const   { return warnings;      }
    const t_string_vec& get_messages()      const   { return messages;      }
};

// ------------------------------------------------------------------------ //
//  T_BIN_TO_LOC_IMPL::PROCESS_CFG_VARS                                     //
// ------------------------------------------------------------------------ //
void t_bin_to_loc_impl::process_cfg_vars()
{
    game_metadata_t* game_metadata =
        (!bincfg || !bincfg->vars)
             ? default_game_metadata()
             : game_metadata_from_cfgvars( bincfg->vars );

    metadata.from_game_metadata( game_metadata );

    free_game_metadata( game_metadata );

    // -------------------------------------------------------------------- //
    //  Update feature flags in Locutus to indicate what needs to be        //
    //  enabled / disabled.                                                 //
    // -------------------------------------------------------------------- //
    locutus.set_compat_voice( metadata.voice_compat );
    locutus.set_compat_ecs  ( metadata.ecs_compat   );
    locutus.set_compat_intv2( metadata.intv2_compat );
    locutus.set_compat_kc   ( metadata.kc_compat    );
    locutus.set_compat_tv   ( metadata.tv_compat    );
    locutus.set_jlp_features( metadata.jlp_accel, metadata.jlp_flash );
    locutus.set_enable_lto_mapper( metadata.lto_mapper );
    locutus.set_explicit_flags( !metadata.is_defaults );
}

// ------------------------------------------------------------------------ //
//  T_BIN_TO_LOC_IMPL::PROCESS                                              //
//                                                                          //
//  This is mostly ganked from icart/icartbin.c, but upgraded to support    //
//  paged ROM segments.                                                     //
// ------------------------------------------------------------------------ //
bool t_bin_to_loc_impl::process()
{
    bc_memspan_t* span;
    t_hunk_vec paged_hunks;

    // -------------------------------------------------------------------- //
    //  Traverse the memspan list, calling "add_segment" on each non-paged  //
    //  segment, and queuing up the paged segments for a second pass.       //
    // -------------------------------------------------------------------- //
    for ( span = bincfg->span ; span ;
          span = reinterpret_cast<bc_memspan_t *>(span->l.next) )
    {
        const int span_len = span->e_addr - span->s_addr + 1;

        // ---------------------------------------------------------------- //
        //  If this span goes outside the address map, record the error     //
        //  and ignore it.                                                  //
        // ---------------------------------------------------------------- //
        if ( span->e_addr >= 0x10000 )
        {
            char buf[128];

            sprintf( buf,
                    "Segment at $%.4X - $%.4X goes outside valid address "
                    "range.", span->s_addr, span->e_addr );

            record_error( string(buf) );
            continue;
        }

        // ---------------------------------------------------------------- //
        //  If this span has an ECS page associated with it, queue it.  We  //
        //  will handle this in a second pass so that we can assign the     //
        //  preload addresses in a canonical order.                         //
        // ---------------------------------------------------------------- //
        if ( (span->flags & BC_SPAN_EP) != 0 )
        {
            // ------------------------------------------------------------ //
            //  Pageflipped, but no valid page?                             //
            //  That's a paddlin'                                           //
            // ------------------------------------------------------------ //
            if ( span->epage == BC_SPAN_NOPAGE )
            {
                char buf[120];

                sprintf( buf,
                        "Invalid segment is marked 'paged' but without a "
                        "valid page number at $%.4X - $%.4X",
                        span->s_addr, span->e_addr );

                record_error( string(buf) );
                continue;
            }

            // ------------------------------------------------------------ //
            //  Pageflipped, but page number greater than 15?               //
            //  That's a paddlin'                                           //
            // ------------------------------------------------------------ //
            if ( span->epage > 15 )
            {
                char buf[120];

                sprintf( buf,
                        "Invalid page number %.2X for page at $%.4X - $%.4X",
                        span->epage, span->s_addr, span->e_addr );

                record_error( string(buf) );
                continue;
            }

            // ------------------------------------------------------------ //
            //  Bankswitched and pageflipped at the same time?              //
            //  That's a paddlin'.                                          //
            // ------------------------------------------------------------ //
            if ( ( span->flags & BC_SPAN_B ) != 0 )
            {
                char buf[120];

                sprintf( buf,
                        "Invalid segment is both bankswitched "
                        "and paged at $%.4X - $%.4X PAGE %.1X",
                        span->s_addr, span->e_addr, span->epage );

                record_error( string(buf) );
                continue;
            }

            // ------------------------------------------------------------ //
            //  Bankswitched but neither readable nor writeable?            //
            //  That's a paddlin'.                                          //
            // ------------------------------------------------------------ //
            if ( ( span->flags & (BC_SPAN_R|BC_SPAN_W) ) == 0 )
            {
                char buf[120];

                sprintf( buf,
                        "Invalid segment is paged but neither readable "
                        "nor writeable at $%.4X - $%.4X PAGE %.1X",
                        span->s_addr, span->e_addr, span->epage );

                record_error( string(buf) );
                continue;
            }

            // ------------------------------------------------------------ //
            //  Preload flag set, but no span data?                         //
            //  Oh you better believe that's a paddlin'!                    //
            // ------------------------------------------------------------ //
            if ( ( span->flags & BC_SPAN_PL ) != 0 && !span->data )
            {
                char buf[120];

                sprintf( buf,
                        "Skipping paged segment marked preload but missing "
                        "data at $%.4X - $%.4X PAGE %.1X",
                        span->s_addr, span->e_addr, span->epage );

                record_error( string(buf) );
                continue;
            }

            t_perm perm;
            if ( span->flags & BC_SPAN_R ) perm.set( LOCUTUS_PERM_READ   );
            if ( span->flags & BC_SPAN_W ) perm.set( LOCUTUS_PERM_WRITE  );
            if ( span->flags & BC_SPAN_N ) perm.set( LOCUTUS_PERM_NARROW );
            if ( span->flags & BC_SPAN_B ) perm.set( LOCUTUS_PERM_BANKSW );

            paged_hunks.push_back(
                t_bin_hunk( span->s_addr, span->e_addr, span->epage, true,
                            span->data, perm ) );

            continue;
        }

        // ---------------------------------------------------------------- //
        //  An actual non-paged chunk.  Imagine that.                       //
        // ---------------------------------------------------------------- //
        t_perm perm;

        if ( span->flags & BC_SPAN_R ) perm.set( LOCUTUS_PERM_READ   );
        if ( span->flags & BC_SPAN_W ) perm.set( LOCUTUS_PERM_WRITE  );
        if ( span->flags & BC_SPAN_N ) perm.set( LOCUTUS_PERM_NARROW );
        if ( span->flags & BC_SPAN_B ) perm.set( LOCUTUS_PERM_BANKSW );

        bool has_data = ( span->flags & ( BC_SPAN_PL | BC_SPAN_PK ) ) != 0;

        if ( has_data && !span->data )
        {
            char buf[120];

            sprintf( buf,
                    "Skipping segment marked preload, but missing data at "
                    "$%.4X - $%.4X",
                    span->s_addr, span->e_addr );

            record_error( string( buf ) );
            continue;
        }

        add_segment
        (
            span->s_addr,  // address in Intellivision memory map
            span->s_addr,  // address in Locutus memory map
            span_len,      // length of span in words
            perm,          // span permissions
            span->data,    // data associated with span, if any
            PAGE_NONE      // not a paged ROM
        );
    }

    // -------------------------------------------------------------------- //
    //  Handle paged ROM segments in two passes:                            //
    //                                                                      //
    //   -- first collect up pages and round them out to 4K                 //
    //   -- then step through pages in canonical order, adding segments.    //
    //                                                                      //
    //  Two passes are necessary, as a single page might arrive in more     //
    //  than one fragment, or as something smaller than 4K.                 //
    //                                                                      //
    //  For the special case of $4xxx, we still send 4K fragments for       //
    //  $4800 - $4FFF, even though $4000 - $47FF gets ignored.  Fixing      //
    //  that requires changing the Locutus firmware.                        //
    //                                                                      //
    //  Allocate paged ROM chunks at the end of Locutus' address space,     //
    //  growing downward from 0x80000.  TODO:  For very large programs,     //
    //  detect whether the paged segments collide with anything.            //
    // -------------------------------------------------------------------- //
    if ( paged_hunks.size() > 0 )
    {
        uint16_t* data[16][16];
        t_perm    perm[16][16];

        for ( int i = 0 ; i < 16; i++ )
            for ( int j = 0 ; j < 16; j++ )
                data[i][j] = nullptr;

        for
        (
            t_hunk_vec::const_iterator paged_span_i = paged_hunks.begin() ;
            paged_span_i != paged_hunks.end() ;
            ++paged_span_i
        )
        {
            const uint16_t page   = paged_span_i->page;
            const uint32_t s_addr = paged_span_i->s_addr;
            const uint32_t e_addr = paged_span_i->e_addr;

            /*printf( "pass 1: %.4X - %.4X PAGE %.4X \n",
                    s_addr, e_addr, page ); fflush(stdout);*/

            for ( uint32_t addr = s_addr, dofs = 0 ;
                  addr <= e_addr ; addr++, dofs++ )
            {
                const int chap = addr >> 12;
                const int cofs = addr & 0x0FFF;

                if ( !data[ chap ][ page ] )
                {
                    perm[ chap ][ page ] = paged_span_i->perm ;
                    perm[ chap ][ page ][ LOCUTUS_PERM_BANKSW ] = true;
                    data[ chap ][ page ] =
                        static_cast<uint16_t*>(malloc( 4096 * 2 ));
                    memset( static_cast<void*>(data[ chap ][ page ]),
                            0xFF, 4096 * 2 );
                }

                data[ chap ][ page ][ cofs ] = paged_span_i->data[ dofs ];
            }
        }

        uint32_t locu_addr = 0x80000;
        t_perm p_pf_only( 0x8 );     // page-flip only.

        for ( int chap = 0xF ; chap >= 0x0 ; --chap )
        {
            bool has_data = false;

            for ( int page = 0xF ; page >= 0x0 ; --page )
                if ( data[ chap ][ page ] )
                {
                    has_data = true;
                    break;
                }

            if (!has_data) continue;

            for ( int page = 0xF ; page >= 0x0 ; --page )
            {
                const uint16_t s_addr = (chap << 12);
                const uint16_t s_len  = 0x1000;

                if ( !data[ chap ][ page ] )
                {
                    add_segment( s_addr, 0, s_len, p_pf_only, nullptr, page );
                    continue;
                }

                // Error out if there isn't enough room.
                //
                // TODO: This should /actually/ error out if the paged
                // segment would overlap:
                //  -- JLP RAM (0x10040 - 0x11F7F) if JLP is enabled,
                //  -- Intellicart RAM if any thing uses BANKSW memory, or
                //  -- Non-paged segments otherwise.
                if (locu_addr < s_len)
                {
                    record_error( "Paged segment overflow" );
                    break;
                }

                locu_addr -= s_len;

                /*fprintf(stderr,  "pass 2: %.4X - %.4X PAGE %.4X => %.8X\n",
                        s_addr, s_addr + s_len - 1, page, locu_addr );*/

                add_segment
                (
                    s_addr,                 // address in INTV memory map
                    locu_addr,              // address in Locutus memory map
                    s_len,                  // length of paged_span_i in words
                    perm[ chap ][ page ],   // paged_span_i permissions
                    data[ chap ][ page ],   // data assoc with paged_span_i
                    page                    // paged ROM
                );

                free( data[ chap ][ page ] );
                data[ chap ][ page ] = nullptr;
            }
        }
    }

    return errors == 0;
}

// ------------------------------------------------------------------------ //
//  T_BIN_TO_LOC_IMPL::ADD_SEGMENT                                          //
// ------------------------------------------------------------------------ //
void t_bin_to_loc_impl::add_segment
(
    const uint16_t intv_s_addr,
    const uint32_t locu_s_addr,
    const uint16_t span_len,
    const t_perm   perm,
    const uint16_t *data,
    const int      page
)
{
    // -------------------------------------------------------------------- //
    //  Check for overflow.  This should never trigger, but just in case..  //
    // -------------------------------------------------------------------- //
    if ( uint32_t(intv_s_addr) + uint32_t(span_len) > 0x10000 )
    {
        char buf[120];

        sprintf(buf, "Address overflow on span %.4X len %.4X",
                intv_s_addr, span_len);

        record_error( string(buf) );
        return;
    }

    // -------------------------------------------------------------------- //
    //  If there was a data payload, copy it into Locutus now.              //
    // -------------------------------------------------------------------- //
    if ( data )
        for ( int i = 0; i < span_len; i++ )
        {
            const uint32_t locu_addr = locu_s_addr + i;
            locutus.write( locu_addr, data[i] );
        }

    // -------------------------------------------------------------------- //
    //  If this was not paged, or if it was page 0 of a chapter, put it in  //
    //  the default after-reset memory map.                                 //
    // -------------------------------------------------------------------- //
    if ( page == 0 || page == PAGE_NONE )
    {
        const int paras    = 1 + ( ( span_len + (intv_s_addr&0xFF) - 1) >> 8 );
        const int para     = intv_s_addr >> 8;
        const int locu_map = locu_s_addr >> 8;
        t_perm    p_perm   = perm;

        // Ensure no Intellicart bankswitching for PAGE 0 pageflipped ROM.
        if ( page == 0 )
            p_perm[ LOCUTUS_PERM_BANKSW ] = false;

        for ( int i = 0; i < paras; ++i)
        {
            locutus.set_mem_perm( para + i, true, p_perm       );
            locutus.set_mem_map ( para + i, true, locu_map + i );
        }
    }

    // -------------------------------------------------------------------- //
    //  If this was paged, also put it in the page flip map.                //
    // -------------------------------------------------------------------- //
    if ( page != PAGE_NONE )
    {
        const int chaps    = (span_len + 4095) >> 12;
        const int chap     = intv_s_addr >> 12;
        const int locu_map = locu_s_addr >> 12;

        for ( int i = 0; i < chaps; ++i )
            locutus.set_pageflip( chap + i, page, locu_map + i, perm );
    }
    // -------------------------------------------------------------------- //
    //  Otherwise, configure page 0 of page-flip map to hold this page,     //
    //  with page-flip enable set to 0.                                     //
    // -------------------------------------------------------------------- //
    else
    {
        const int chaps    = (span_len + 4095) >> 12;
        const int chap     = intv_s_addr >> 12;
        const int locu_map = locu_s_addr >> 12;
        t_perm    p_perm   = perm;

        // Disable page-flipping for this mapping.
        p_perm[ LOCUTUS_PERM_BANKSW ] = false;

        for ( int i = 0; i < chaps; ++i )
            locutus.set_pageflip( chap + i, 0, locu_map + i, perm );
    }
}

// ------------------------------------------------------------------------ //
//  Forwarding from T_BIN_TO_LOC to T_BIN_TO_LOC_IMPL                       //
// ------------------------------------------------------------------------ //

t_bin_to_loc::t_bin_to_loc
(
    const char* const   bin_fn,
    const char* const   cfg_fn,
    t_locutus&          locutus,
    const bool          do_macros
)
{
    impl = new t_bin_to_loc_impl( bin_fn, cfg_fn, locutus, do_macros );
}

t_bin_to_loc::~t_bin_to_loc()
{
    delete impl;
}

bool t_bin_to_loc::is_ok()         const    { return impl->is_ok();         }
bool t_bin_to_loc::process()                { return impl->process();       }
int  t_bin_to_loc::get_errors()    const    { return impl->get_errors();    }
int  t_bin_to_loc::get_warnings()  const    { return impl->get_warnings();  }

const t_string_vec& t_bin_to_loc::get_messages() const
{
    return impl->get_messages();
}

