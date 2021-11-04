#include <stdio.h>

#include <list>
#include <vector>
#include <map>
#include <utility>
#include <bitset>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <iterator>
#include <algorithm>

#include "locutus/locutus_types.hpp"

extern "C"
{
#   include "misc/ll.h"
#   include "misc/types.h"
#   include "metadata/metadata.h"
#   include "metadata/cfgvar_metadata.h"
}

// These need to match LUIGI specification
enum : uint8_t
{
    METATAG_NAME        = 0x00,
    METATAG_SHORT_NAME  = 0x01,
    METATAG_AUTHOR      = 0x02,
    METATAG_PUBLISHER   = 0x03,
    METATAG_RLS_DATE    = 0x04,
    METATAG_LICENSE     = 0x05,
    METATAG_DESCRIPTION = 0x06,
    METATAG_MISC        = 0x07,
    METATAG_GAME_ARTIST = 0x08,
    METATAG_MUSIC_BY    = 0x09,
    METATAG_SFX_BY      = 0x0A,
    METATAG_VOICES_BY   = 0x0B,
    METATAG_DOCS_BY     = 0x0C,
    METATAG_CONCEPT_BY  = 0x0D,
    METATAG_BOX_ARTIST  = 0x0E,
    METATAG_MORE_INFO   = 0x0F
};

// ------------------------------------------------------------------------ //
//  STRING_VEC_TO_ARRAY                                                     //
// ------------------------------------------------------------------------ //
LOCAL const char **string_vec_to_array( const t_string_vec& vec )
{
    if ( !vec.size() ) return nullptr;

    const char **array =
        static_cast<const char**>(calloc( sizeof(char *), vec.size() + 1));

    if ( !array )
        return nullptr;

    int idx = 0;

    for ( const auto& str : vec )
        array[idx++] = strdup( str.c_str() );

    return array;
}

// ------------------------------------------------------------------------ //
//  GAME_DATE_VEC_TO_ARRAY                                                  //
// ------------------------------------------------------------------------ //
LOCAL game_date_t *game_date_vec_to_array( const t_game_date_vec& vec )
{
    if ( !vec.size() ) return nullptr;

    game_date_t *array =
        static_cast<game_date_t *>(calloc(sizeof(game_date_t), vec.size() + 1));

    if ( !array )
        return nullptr;

    int idx = 0;

    for ( const auto& game_date : vec )
        array[idx++] = game_date;

    return array;
}

// ------------------------------------------------------------------------ //
//  STRING_ARRAY_TO_VEC                                                     //
// ------------------------------------------------------------------------ //
LOCAL t_string_vec string_array_to_vec( const char **array )
{
    t_string_vec vec;

    if ( array )
    {
        while ( *array )
            vec.push_back( *array++ );
    }

    return vec;
}

// ------------------------------------------------------------------------ //
//  GAME_DATE_ARRAY_TO_VEC                                                  //
// ------------------------------------------------------------------------ //
LOCAL t_game_date_vec game_date_array_to_vec( const game_date_t* array )
{
    t_game_date_vec vec;

    if ( array )
    {
        while ( array->year )
            vec.push_back( *array++ );
    }

    return vec;
}

// ------------------------------------------------------------------------ //
//  T_METADATA::PACK_STRING                                                 //
// ------------------------------------------------------------------------ //
void t_metadata::pack_string( uint8_t tag, t_byte_vec& v,
                              const std::string& s ) const
{
    auto si = s.cbegin();
    auto se = s.cend();
    if ( std::distance( si, se ) > 255 )
        se = si + 255;

    const auto len =  std::distance( si, se );
    v.reserve( v.size() + len + 2 );
    v.push_back( tag );
    v.push_back( uint8_t( len ) );
    std::copy( si, se, std::back_inserter( v ) );
}

// ------------------------------------------------------------------------ //
//  T_METADATA::UNPACK_STRING                                               //
// ------------------------------------------------------------------------ //
template <typename T>
void t_metadata::unpack_string( std::string& str, T& si, uint8_t len )
{
    str.reserve( len );
    std::copy( si, si + len, std::back_inserter( str ) );

    si += len;
}

template <typename T>
void t_metadata::unpack_string( t_string_vec& vec, T& si, uint8_t len )
{
    vec.push_back( std::string("") );
    unpack_string( vec.back(), si, len );
}

// ------------------------------------------------------------------------ //
//  T_METADATA::EMPTY                                                       //
// ------------------------------------------------------------------------ //
bool t_metadata::empty( ) const
{
    return name.empty()                 &&
           short_name.empty()           &&
           authors.empty()              &&
           game_artists.empty()         &&
           composers.empty()            &&
           sfx_artists.empty()          &&
           voice_actors.empty()         &&
           doc_writers.empty()          &&
           conceptualizers.empty()      &&
           box_artists.empty()          &&
           more_infos.empty()           &&
           publishers.empty()           &&
           release_dates.empty()        &&
           licenses.empty()             &&
           descriptions.empty()         &&
           misc.empty();
}

// ------------------------------------------------------------------------ //
//  T_METADATA::CLEAR                                                       //
// ------------------------------------------------------------------------ //
void t_metadata::clear()
{
    name.clear();
    short_name.clear();
    authors.clear();
    game_artists.clear();
    composers.clear();
    sfx_artists.clear();
    voice_actors.clear();
    doc_writers.clear();
    conceptualizers.clear();
    box_artists.clear();
    more_infos.clear();
    publishers.clear();
    release_dates.clear();
    licenses.clear();
    descriptions.clear();
    misc.clear();
}

// ------------------------------------------------------------------------ //
//  T_METADATA::SERIALIZE                                                   //
// ------------------------------------------------------------------------ //
t_byte_vec t_metadata::serialize( ) const
{
    t_byte_vec serial;

    if ( name.length() > 0 )
        pack_string( METATAG_NAME, serial, name );

    if ( short_name.length() > 0 )
        pack_string( METATAG_SHORT_NAME, serial, short_name );

    for ( const auto& author : authors )
        pack_string( METATAG_AUTHOR, serial, author );

    for ( const auto& name_credit : game_artists )
        pack_string( METATAG_GAME_ARTIST, serial, name_credit );

    for ( const auto& name_credit : composers )
        pack_string( METATAG_MUSIC_BY, serial, name_credit );

    for ( const auto& name_credit : sfx_artists )
        pack_string( METATAG_SFX_BY, serial, name_credit );

    for ( const auto& name_credit : voice_actors )
        pack_string( METATAG_VOICES_BY, serial, name_credit );

    for ( const auto& name_credit : doc_writers )
        pack_string( METATAG_DOCS_BY, serial, name_credit );

    for ( const auto& name_credit : conceptualizers )
        pack_string( METATAG_CONCEPT_BY, serial, name_credit );

    for ( const auto& name_credit : box_artists )
        pack_string( METATAG_BOX_ARTIST, serial, name_credit );

    for ( const auto& info : more_infos )
        pack_string( METATAG_MORE_INFO, serial, info );

    for ( const auto& publisher : publishers )
        pack_string( METATAG_PUBLISHER, serial, publisher );

    if ( release_dates.size() )
    {
        /* Reserve worst-case storage. */
        serial.reserve( serial.size() + release_dates.size() * 10 );

        for ( const auto& date : release_dates )
        {
            if ( !date.year )
                continue;

            serial.push_back( METATAG_RLS_DATE );

            uint8_t data[8];
            const auto length = game_date_to_uint8_t(&date, data);
            serial.push_back( length & 0xFF );

            for (auto i = 0; i < length; i++)
                serial.push_back(data[i]);
        }
    }

    for ( const auto& license : licenses )
        pack_string( METATAG_LICENSE, serial, license );

    for ( const auto& description : descriptions )
        pack_string( METATAG_DESCRIPTION, serial, description );

    for ( const auto& item : misc )
        pack_string( METATAG_MISC, serial, item );

    return serial;
}

// ------------------------------------------------------------------------ //
//  T_METADATA::DESERIALIZE                                                 //
// ------------------------------------------------------------------------ //
void t_metadata::deserialize( const t_byte_vec& serial )
{

    clear();

    auto si = serial.cbegin();
    auto se = serial.cend();

    while ( si != se )
    {
        const auto remain = std::distance( si, se );

        assert( remain >= 2);

        const uint8_t tag = *si++;
        const uint8_t len = *si++;

        assert( remain >= 2 + len );

        switch ( tag )
        {
            case METATAG_NAME:
                unpack_string( name, si, len );
                break;
            case METATAG_SHORT_NAME:
                unpack_string( short_name, si, len );
                break;
            case METATAG_AUTHOR:
                unpack_string( authors, si, len );
                break;
            case METATAG_GAME_ARTIST:
                unpack_string( game_artists, si, len );
                break;
            case METATAG_MUSIC_BY:
                unpack_string( composers, si, len );
                break;
            case METATAG_SFX_BY:
                unpack_string( sfx_artists, si, len );
                break;
            case METATAG_VOICES_BY:
                unpack_string( voice_actors, si, len );
                break;
            case METATAG_DOCS_BY:
                unpack_string( doc_writers, si, len );
                break;
            case METATAG_CONCEPT_BY:
                unpack_string( conceptualizers, si, len );
                break;
            case METATAG_BOX_ARTIST:
                unpack_string( box_artists, si, len );
                break;
            case METATAG_MORE_INFO:
                unpack_string( more_infos, si, len );
                break;
            case METATAG_PUBLISHER:
                unpack_string( publishers, si, len );
                break;
            case METATAG_RLS_DATE:
            {
                const auto data = &*si;
                game_date_t game_date;

                if (uint8_t_to_game_date(&game_date, data, len) == 0)
                    release_dates.push_back( game_date );

                si += len;
                break;
            }
            case METATAG_LICENSE:
                unpack_string( licenses, si, len );
                break;
            case METATAG_DESCRIPTION:
                unpack_string( descriptions, si, len );
                break;
            case METATAG_MISC:
                unpack_string( misc, si, len );
                break;
            default:
                // Silently skip unknown tags
                si += len;
        }
    }
}

// ------------------------------------------------------------------------ //
//  T_METADATA::FROM_GAME_METADATA                                          //
//                                                                          //
//  TODO:  Rewrite the code to not need this at all, and just hold a        //
//         pointer to the game_metadata structure.  Requires rewriting all  //
//         the serialization/deserialization code, though.                  //
// ------------------------------------------------------------------------ //
void t_metadata::from_game_metadata( const game_metadata_t* game_metadata )
{
    clear();

    if ( game_metadata->name )
        name = game_metadata->name;

    if ( game_metadata->short_name )
        short_name = game_metadata->short_name;

    authors         = string_array_to_vec( game_metadata->authors           );
    game_artists    = string_array_to_vec( game_metadata->game_artists      );
    composers       = string_array_to_vec( game_metadata->composers         );
    sfx_artists     = string_array_to_vec( game_metadata->sfx_artists       );
    voice_actors    = string_array_to_vec( game_metadata->voice_actors      );
    doc_writers     = string_array_to_vec( game_metadata->doc_writers       );
    conceptualizers = string_array_to_vec( game_metadata->conceptualizers   );
    box_artists     = string_array_to_vec( game_metadata->box_artists       );
    more_infos      = string_array_to_vec( game_metadata->more_infos        );
    release_dates   = game_date_array_to_vec( game_metadata->release_dates  );
    publishers      = string_array_to_vec( game_metadata->publishers        );
    licenses        = string_array_to_vec( game_metadata->licenses          );
    descriptions    = string_array_to_vec( game_metadata->descriptions      );
    misc            = string_array_to_vec( game_metadata->misc              );

    ecs_compat      = game_metadata->ecs_compat;
    voice_compat    = game_metadata->voice_compat;
    intv2_compat    = game_metadata->intv2_compat;
    kc_compat       = game_metadata->kc_compat;
    tv_compat       = game_metadata->tv_compat;
    lto_mapper      = game_metadata->lto_mapper;
    jlp_accel       = game_metadata->jlp_accel;
    jlp_flash       = game_metadata->jlp_flash;
    is_defaults     = game_metadata->is_defaults;

    // -------------------------------------------------------------------- //
    //  Ugly:  LUIGI doesn't have explicit tags for build_date or version,  //
    //  so for now pack these as 'misc' tags.  That's fine for a rom2luigi  //
    //  then luigi2bin round-trip; however, ultimately these should become  //
    //  tags.  Unfortunately, the early versions of the LUIGI decoder will  //
    //  abort() if they see a tag they don't understand.                    //
    // -------------------------------------------------------------------- //
    if (game_metadata->build_dates)
    {
        for (auto i = 0; game_metadata->build_dates[i].year; ++i)
        {
            char *dstr = game_date_to_string(&game_metadata->build_dates[i]);
            misc.push_back(std::string("build_date=") + dstr);
            free(dstr);
        }
    }

    if (game_metadata->versions)
        for (auto i = 0; game_metadata->versions[i]; ++i)
            misc.push_back(std::string("version=") +
                           game_metadata->versions[i]);
}

// ------------------------------------------------------------------------ //
//  T_METADATA::TO_GAME_METADATA                                            //
// ------------------------------------------------------------------------ //
game_metadata_t *t_metadata::to_game_metadata( ) const
{
    game_metadata_t *game_metadata =
        static_cast<game_metadata_t *>(calloc( sizeof(game_metadata_t), 1 ));

    if ( !game_metadata )
        return nullptr;

    if ( name.length() )
        game_metadata->name = strdup( name.c_str() );

    if ( short_name.length() )
        game_metadata->short_name = strdup( short_name.c_str() );

    game_metadata->authors         = string_vec_to_array( authors           );
    game_metadata->game_artists    = string_vec_to_array( game_artists      );
    game_metadata->composers       = string_vec_to_array( composers         );
    game_metadata->sfx_artists     = string_vec_to_array( sfx_artists       );
    game_metadata->voice_actors    = string_vec_to_array( voice_actors      );
    game_metadata->doc_writers     = string_vec_to_array( doc_writers       );
    game_metadata->conceptualizers = string_vec_to_array( conceptualizers   );
    game_metadata->box_artists     = string_vec_to_array( box_artists       );
    game_metadata->more_infos      = string_vec_to_array( more_infos        );
    game_metadata->release_dates   = game_date_vec_to_array( release_dates  );
    game_metadata->publishers      = string_vec_to_array( publishers        );
    game_metadata->licenses        = string_vec_to_array( licenses          );
    game_metadata->descriptions    = string_vec_to_array( descriptions      );
    game_metadata->misc            = nullptr;

    game_metadata->ecs_compat      = ecs_compat;
    game_metadata->voice_compat    = voice_compat;
    game_metadata->intv2_compat    = intv2_compat;
    game_metadata->kc_compat       = kc_compat;
    game_metadata->tv_compat       = tv_compat;
    game_metadata->lto_mapper      = lto_mapper;
    game_metadata->jlp_accel       = jlp_accel;
    game_metadata->jlp_flash       = jlp_flash;
    game_metadata->is_defaults     = is_defaults;

    // Pull apart 'misc' array into a cfg_var_t list that we can run through
    // the cfgvar_metadata infrastructure.  Merge any metadata that results
    // with our partial metadata above.
    if ( misc.size() )
    {
        cfg_var_t *cv = nullptr;

        for ( const auto& misc_str : misc )
            append_cfg_var( &cv, cons_cfg_var_kv_str( misc_str.c_str() ) );

        auto misc_gm = game_metadata_from_cfgvars( cv );

        if ( misc_gm )
        {
            auto merged_gm = merge_game_metadata( game_metadata, misc_gm );
            if ( merged_gm )
            {
                free_game_metadata( game_metadata );
                game_metadata = merged_gm;
            }
            free_game_metadata( misc_gm );
        }

        free_cfg_var_list( cv );
    }
 
    return game_metadata;
}
