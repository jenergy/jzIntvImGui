// ======================================================================== //
//  Adapt the Locutus class to the periph_t "class" in jzIntv               //
// ======================================================================== //

#include "locutus.hpp"
#include "luigi.hpp"

#include "locutus_adapt.h"

#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>

using namespace std;

class t_cpu_cache : public t_cpu_cache_if
{
  private:
    cp1600_t *const cp1600;

  public:
    t_cpu_cache( cp1600_t *cp1600_ ) : cp1600( cp1600_ ) { }

    virtual void invalidate( uint16_t addr_lo, uint16_t addr_hi )
    {
        if ( cp1600 )
            cp1600_invalidate( cp1600, addr_lo, addr_hi );
    }

    ~t_cpu_cache();
};

// Anchor the vtable
t_cpu_cache::~t_cpu_cache() { }

struct t_locutus_priv
{
    t_locutus    locutus;
    t_cpu_cache  cpu_cache;

    t_locutus_priv( cp1600_t *cp1600 )
    :   cpu_cache( cp1600 )
    { }
};


LOCAL uint32_t locutus_read( periph_t *const periph, periph_t *,
                             uint32_t addr, uint32_t )
{
    t_locutus_wrap *wrap = reinterpret_cast<t_locutus_wrap *>(periph);

    try
    {
        return wrap->locutus_priv->locutus.intv_read( addr );
    } catch ( string &s )
    {
        cerr << "EXCEPTION: " << s << endl;
        exit(1);
    }
}

LOCAL void locutus_write( periph_t *const periph, periph_t *,
                          uint32_t addr, uint32_t data )
{
    t_locutus_wrap *wrap = reinterpret_cast<t_locutus_wrap *>(periph);

    try
    {
        return wrap->locutus_priv->locutus.intv_write( addr, data );
    } catch ( string &s )
    {
        cerr << "EXCEPTION: " << s << endl;
        exit(1);
    }
}

LOCAL void locutus_reset( periph_t *const periph )
{
    t_locutus_wrap *wrap = reinterpret_cast<t_locutus_wrap *>(periph);

    try
    {
        wrap->locutus_priv->locutus.intv_reset();
    } catch ( string &s )
    {
        cerr << "EXCEPTION: " << s << endl;
        exit(1);
    }
}

LOCAL void locutus_dtor( periph_t *const periph )
{
    t_locutus_wrap *wrap = reinterpret_cast<t_locutus_wrap *>(periph);

    delete wrap->locutus_priv;
    wrap->locutus_priv = nullptr;
}

extern "C" int make_locutus
(
    t_locutus_wrap  *loc_wrap,      /*  pointer to a Locutus wrapper    */
    const char      *luigi_file,    /*  LUIGI file to load into Locutus */
    cp1600_t        *cp1600,
    int             silent,
    const char      *savegame
)
{
    t_locutus_priv *priv     = new t_locutus_priv( cp1600 );
    loc_wrap->locutus_priv   = priv;

    ifstream ifs(luigi_file, ios::binary | ios::ate);

    if ( !ifs.is_open() )
    {
        if (!silent)
            cerr << "could not open " << luigi_file << endl;
        return -1;
    }

    ifstream::pos_type pos = ifs.tellg();

    t_byte_vec luigi_data(pos);

    ifs.seekg(0, ios::beg);
    ifs.read(reinterpret_cast<char *>(&luigi_data[0]), pos);

    try
    {
        t_luigi::deserialize( priv->locutus, luigi_data );
    } catch ( string &s )
    {
        if (!silent)
            cerr << "EXCEPTION: " << s << endl;
        return -1;
    }

    if ( (!silent || cp1600) && priv->locutus.was_scrambled() )
    {
        const uint8_t* druid = priv->locutus.get_scramble_druid();
        char buf[50];
        sprintf(buf, 
                "%.2X%.2X:%.2X%.2X:%.2X%.2X:%.2X%.2X:"
                "%.2X%.2X:%.2X%.2X:%.2X%.2X:%.2X%.2X",
                druid[15], druid[14], druid[13], druid[12],
                druid[11], druid[10], druid[ 9], druid[ 8], 
                druid[ 7], druid[ 6], druid[ 5], druid[ 4], 
                druid[ 3], druid[ 2], druid[ 1], druid[ 0]);
        cerr << "This LUIGI is scrambled for DRUID " << buf << "\n";
        return -1;
    }

    priv->locutus.set_xregs( &cp1600->xr[0] );
    priv->locutus.init_jlp_flash( savegame );
    priv->locutus.intv_reset();

    loc_wrap->periph.read       = locutus_read;
    loc_wrap->periph.write      = locutus_write;
    loc_wrap->periph.peek       = locutus_read;
    loc_wrap->periph.poke       = locutus_write;
    loc_wrap->periph.reset      = locutus_reset;
    loc_wrap->periph.dtor       = locutus_dtor;

    loc_wrap->periph.tick       = nullptr;
    loc_wrap->periph.min_tick   = ~0U;
    loc_wrap->periph.max_tick   = ~0U;

    loc_wrap->periph.addr_base  = 0;
    loc_wrap->periph.addr_mask  = ~0U;
    loc_wrap->periph.ser_init   = nullptr; // no support for serializer()

    return 0;
}

extern "C" game_metadata_t* get_locutus_metadata
(
    t_locutus_wrap *loc_wrap
)
{
    t_locutus_priv* priv = loc_wrap->locutus_priv;
    const t_metadata& metadata = priv->locutus.get_metadata();
    return metadata.to_game_metadata();
}

extern "C" int get_locutus_compat_ecs  ( t_locutus_wrap *loc_wrap )
{
    return int( loc_wrap->locutus_priv->locutus.get_compat_ecs() );
}

extern "C" int get_locutus_compat_voice( t_locutus_wrap *loc_wrap )
{
    return int( loc_wrap->locutus_priv->locutus.get_compat_voice() );
}

extern "C" int get_locutus_compat_intv2( t_locutus_wrap *loc_wrap )
{
    return int( loc_wrap->locutus_priv->locutus.get_compat_intv2() );
}

extern "C" int get_locutus_compat_kc( t_locutus_wrap *loc_wrap )
{
    return int( loc_wrap->locutus_priv->locutus.get_compat_kc() );
}

extern "C" uint64_t get_locutus_uid( t_locutus_wrap *loc_wrap )
{
    return loc_wrap->locutus_priv->locutus.get_uid();
}

extern "C" int get_locutus_was_scrambled( t_locutus_wrap *loc_wrap )
{
    return loc_wrap->locutus_priv->locutus.was_scrambled();
}

extern "C" const uint8_t* get_locutus_scramble_druid( t_locutus_wrap *loc_wrap )
{
    return loc_wrap->locutus_priv->locutus.get_scramble_druid();
}
