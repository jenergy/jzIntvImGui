#include <stdio.h>
#ifndef LOCUTUS_TYPES_HPP_
#define LOCUTUS_TYPES_HPP_

#include <list>
#include <vector>
#include <map>
#include <utility>
#include <bitset>
#include <stdint.h>
#include <cstring>
#include <assert.h>
#include <iterator>
#include <algorithm>
#include "metadata/metadata.h"

static const int LOCUTUS_PERM_READ   = 0;
static const int LOCUTUS_PERM_WRITE  = 1;
static const int LOCUTUS_PERM_NARROW = 2;   // allow writes to 8 LSBs only
static const int LOCUTUS_PERM_BANKSW = 3;   // Bankswitch / Pageflip enable

typedef std::bitset< 4 >                        t_perm;
typedef std::pair< uint32_t, uint32_t >         t_addr_span;
typedef std::list< t_addr_span >                t_addr_list;
typedef std::vector< uint8_t >                  t_byte_vec;
typedef std::vector< uint16_t >                 t_word_vec;
typedef std::vector< std::string >              t_string_vec;
typedef std::vector< game_date_t >              t_game_date_vec;
typedef std::pair< std::string, std::string >   t_var;
typedef std::vector< t_var >                    t_var_vec;

static const int PAGE_NONE = 0xFF;


// ------------------------------------------------------------------------ //
//  T_BIN_HUNK                                                              //
// ------------------------------------------------------------------------ //
struct t_bin_hunk
{
    uint32_t    s_addr, e_addr;     // Locutus addresses
    uint8_t     page;               // ECS page (0xFF if none)
    bool        mapped;             // True == map to INTV space at s_addr
    t_word_vec  data;               // actual ROM data
    t_perm      perm;               // permissions

    t_bin_hunk( const uint32_t      s_addr_,
                const uint32_t      e_addr_,
                const uint8_t       page_,
                const bool          mapped_,
                const t_word_vec    data_,
                const t_perm        perm_ = 0 )
    :
        s_addr( s_addr_ ), e_addr( e_addr_ ), page( page_ ), mapped( mapped_ ),
        data( data_ ), perm( perm_ )
    { }

    t_bin_hunk( const uint32_t      s_addr_,
                const uint32_t      e_addr_,
                const uint8_t       page_,
                const bool          mapped_,
                const uint16_t*     data_,
                const t_perm        perm_ = 0 )
    :
        s_addr( s_addr_ ), e_addr( e_addr_ ), page( page_ ), mapped( mapped_ ),
        data( ), perm( perm_ )
    {
        data.resize( e_addr_ - s_addr_ + 1, 0 );

        if ( data_ )
            std::memcpy( static_cast<void*>(&data[0]), 
                         static_cast<const void *>(&data_[0]),
                         sizeof( uint16_t ) * data.size() );
    }


    inline bool operator < ( const t_bin_hunk& rhs ) const
    {
        return page <  rhs.page                         ? true
            :  page == rhs.page && s_addr < rhs.s_addr  ? true
            :                                             false;
    }

    static bool page_then_addr( const t_bin_hunk& lhs, const t_bin_hunk& rhs )
    {
        return lhs.page <  rhs.page                             ? true
            :  lhs.page == rhs.page && lhs.s_addr < rhs.s_addr  ? true
            :                                                     false;
    }

    static bool addr_then_page( const t_bin_hunk& lhs, const t_bin_hunk& rhs )
    {
        return lhs.s_addr < rhs.s_addr                        ? true
            :  lhs.s_addr == rhs.s_addr &&lhs.page < rhs.page ? true
            :                                                   false;
    }

    inline bool can_merge_with( const t_bin_hunk& rhs ) const
    {
        return e_addr == rhs.s_addr - 1
            && page   == rhs.page
            && mapped == rhs.mapped;
    }

    inline void merge_with( const t_bin_hunk& rhs )
    {
        assert( can_merge_with( rhs ) );

        e_addr = rhs.e_addr;
        data.insert( data.end(), rhs.data.begin(), rhs.data.end() );
    }

    // After split, e_addr = split_addr - 1, and we return a new hunk starting
    // at split_addr ending at the old e_addr.  That is:
    //  this => [ s_addr, split_addr )
    //  new  => [ split_addr, e_addr ]
    t_bin_hunk split_at( const uint32_t split_addr )
    {
        assert( split_addr > s_addr && split_addr <= e_addr );

        t_bin_hunk new_hunk( split_addr, e_addr, page, mapped,
                             &data[split_addr - s_addr] );

        e_addr = split_addr - 1;
        data.resize( e_addr - s_addr + 1 );

        return new_hunk;
    }
};

typedef std::vector< t_bin_hunk > t_hunk_vec;

// ------------------------------------------------------------------------ //
//  T_MEMATTR_SPAN                                                          //
// ------------------------------------------------------------------------ //
static const uint8_t MEMATTR_BAD = 0;
static const uint8_t MEMATTR_ROM = 1;
static const uint8_t MEMATTR_WOM = 2;
static const uint8_t MEMATTR_RAM = 3;

struct t_memattr_span
{
    uint16_t        s_addr, e_addr;
    uint8_t         type, width;

    t_memattr_span( const uint16_t s_addr_, const uint16_t e_addr_,
                    const uint8_t type_,    const uint8_t width_ )
    : s_addr( s_addr_ ), e_addr( e_addr_ ), type( type_ ), width( width_ )
    { }

    inline bool operator < ( const t_memattr_span& rhs ) const
    {
        return s_addr < rhs.s_addr ? true : false;
    }

    inline bool can_merge_with( const t_memattr_span& rhs ) const
    {
        return e_addr == rhs.s_addr - 1
            && type   == rhs.type
            && width  == rhs.width;
    }

    inline void merge_with( const t_memattr_span& rhs )
    {
        assert( can_merge_with( rhs ) );

        e_addr = rhs.e_addr;
    }
};

typedef std::vector< t_memattr_span > t_memattr_vec;

// ------------------------------------------------------------------------ //
//  T_BANKSW_SPAN                                                           //
// ------------------------------------------------------------------------ //
struct t_banksw_span
{
    uint16_t s_addr, e_addr;

    t_banksw_span( const uint16_t s_addr_, const uint16_t e_addr_ )
    : s_addr( s_addr_ ), e_addr( e_addr_ )
    { }

    inline bool operator < ( const t_banksw_span& rhs ) const
    {
        return s_addr < rhs.s_addr ? true : false;
    }

    inline bool can_merge_with( const t_banksw_span& rhs ) const
    {
        return e_addr == rhs.s_addr - 1;
    }

    inline void merge_with( const t_banksw_span& rhs )
    {
        assert( can_merge_with( rhs ) );

        e_addr = rhs.e_addr;
    }
};

typedef std::vector< t_banksw_span  > t_banksw_vec;

// ------------------------------------------------------------------------ //
//  T_METADATA                                                              //
// ------------------------------------------------------------------------ //
struct t_metadata
{
private:
    void pack_string( uint8_t tag, t_byte_vec& v, const std::string& s ) const;

    template <typename T>
    void unpack_string( std::string& str, T& si, uint8_t len );

    template <typename T>
    void unpack_string( t_string_vec& vec, T& si, uint8_t len );

public:

    std::string                 name;
    std::string                 short_name;
    t_string_vec                authors;
    t_string_vec                game_artists;
    t_string_vec                composers;
    t_string_vec                sfx_artists;
    t_string_vec                voice_actors;
    t_string_vec                doc_writers;
    t_string_vec                conceptualizers;
    t_string_vec                box_artists;
    t_string_vec                more_infos;
    t_string_vec                publishers;
    t_game_date_vec             release_dates;
    t_string_vec                licenses;
    t_string_vec                descriptions;
    t_string_vec                misc;

    compat_level_t              ecs_compat;
    compat_level_t              voice_compat;
    compat_level_t              intv2_compat;
    compat_level_t              kc_compat;
    compat_level_t              tv_compat;
    bool                        lto_mapper;
    jlp_accel_t                 jlp_accel;
    int                         jlp_flash;
    bool                        is_defaults;

    game_metadata_t* to_game_metadata() const;
    void from_game_metadata( const game_metadata_t* );

    bool empty( ) const;
    void clear( );

    t_byte_vec serialize( ) const;
    void deserialize( const t_byte_vec& serial );
};

#endif
