// ======================================================================== //
//  LOCUTUS -- aka. LTO Flash!                                              //
//                                                                          //
//  This class models Locutus, and serves as a structured container for     //
//  a Locutus compatible cartridge.  This may be initialized by or          //
//  serialized to a LUIGI file.                                             //
//                                                                          //
//  Note:  This is not tightly integrated into jzIntv at all.  Rather,      //
//  some adaptor C code will bridge between this C++ and jzIntv.            //
//                                                                          //
//  Terms:                                                                  //
//   -- byte:       8 bits                                                  //
//   -- word:       16 bits                                                 //
//   -- para:       256 words (para is short for "paragraph")               //
//   -- page:       4K words (16 para)                                      //
//   -- chap:       A group of pages that map to the same 4K address span   //
//                                                                          //
// ======================================================================== //
#ifndef LOCUTUS_HPP_
#define LOCUTUS_HPP_

#include "locutus_types.hpp"
#include "crc_calc.hpp"
#include <algorithm>
#include <iterator>
#include <string>

static const int LOCUTUS_FEAT_JLP_ACCEL     = 0;
static const int LOCUTUS_FEAT_JLP_SAVEGAME  = 1;

class t_cpu_cache_if                // interface back to the CPU decode cache
{
    public:
        virtual void invalidate( uint16_t addr_lo, uint16_t addr_hi ) = 0;
        virtual ~t_cpu_cache_if();
};

static const uint32_t LOCUTUS_CAPACITY = 512 * 1024;

class t_locutus
{
  private:
    uint16_t        ram     [512 * 1024];
    uint16_t        mem_map [256][2];   // bits 10:0 form bits 18:8 of addr
    t_perm          mem_perm[256][2];
    uint16_t        pfl_map [16][16];   // bits 6:0 form bits 18:12 of addr
    t_perm          pfl_perm[16][16];
    uint8_t         bsel    [16];       // Bank-select table (used by JLP only)
    uint64_t        uid;

    t_cpu_cache_if* cpu_cache;
    t_metadata      metadata;

    std::bitset<128>              feature_flag;
    std::bitset<LOCUTUS_CAPACITY> initialized;

    std::string             jlp_savegame;       // JLP savegame filename
    std::vector<uint8_t>    jlp_sg_img;         // JLP flash image
    uint32_t                jlp_sg_bytes;
    uint16_t                jlp_sg_start;
    uint16_t                jlp_sg_end;
    uint16_t                jlp_sg_addr;
    uint16_t                jlp_sg_row;
    FILE*                   jlp_sg_file;

    crc::jlp16              jlp_crc16;
    uint16_t                jlp_regs[16];
    uint16_t               *jlp_xregs;

    mutable uint16_t        jlp_sleep;
    mutable crc::zip        jlp_rand;

    // These are set only if we try to load a scrambled LUIGI.
    uint8_t druid[16];      // Scrambler DRUID
    bool    is_scrambled;

    void do_banksw_write  ( uint16_t addr, uint16_t data );
    void do_pageflip_write( uint16_t addr, uint16_t data );

    void validate_addr( const char* loc, const uint32_t addr ) const
    {
        if ( addr > LOCUTUS_CAPACITY )
            throw loc + std::string(": Address out of range");
    }

    // -------------------------------------------------------------------- //
    //  Internal JLP Acceleration and JLP Flash support                     //
    // -------------------------------------------------------------------- //
    uint16_t do_jlp_accel_read ( const uint16_t addr ) const;
    void     do_jlp_accel_write( const uint16_t addr, const uint16_t data );

    bool jlp_sg_validate_args( const bool chk_addr = true ) const;
    void jlp_sg_ram_to_flash( void );
    void jlp_sg_flash_to_ram( void );
    void jlp_sg_erase_sector( void );

    void flip_jlp_accel( const bool new_jlp_accel_on );

    inline uint32_t next_rand( void ) const
    {
        jlp_rand.update( 0x4A5A6A7A, 32 );
        return jlp_rand.get();
    }

  public:

    t_locutus();

    // -------------------------------------------------------------------- //
    //  INTV-facing interface                                               //
    // -------------------------------------------------------------------- //
    uint16_t intv_read ( const uint16_t addr, const bool force = false ) const;
    void     intv_write( const uint16_t addr, const uint16_t data );
    void     intv_reset( void );

    inline void set_cpu_cache ( t_cpu_cache_if* cpu_cache_ )
    {
        cpu_cache = cpu_cache_;
    }

    inline void set_xregs( uint16_t *x )
    {
        jlp_xregs = x;
    }

    // -------------------------------------------------------------------- //
    //  JLP Acceleration and JLP Flash support interface                    //
    // -------------------------------------------------------------------- //
    void init_jlp_flash( const char* jlp_savegame_ );

    // -------------------------------------------------------------------- //
    //  General purpose interface                                           //
    //                                                                      //
    //  For mem_map, mem_perm, the "reset" argument selects between the     //
    //  currently active values (reset = false) or the values that will be  //
    //  loaded at reset (reset = true).                                     //
    // -------------------------------------------------------------------- //
    inline uint16_t read( const uint32_t addr ) const
    {
        validate_addr( "t_locutus::read", addr );
        return ram[addr];
    }

    inline void     write( const uint32_t addr, const uint16_t data )
    {
        validate_addr( "t_locutus::write", addr );
        ram[addr] = data;
        initialized[addr] = true;
    }

    inline uint16_t get_mem_map ( const uint8_t para, const bool reset ) const
    {
        return mem_map[para][reset];
    }

    inline void     set_mem_map ( const uint8_t  para, const bool reset,
                                  const uint16_t mapping )
    {
        mem_map[para][reset] = mapping & 0x07FF;
    }

    inline t_perm   get_mem_perm( const uint8_t  para, const bool reset ) const
    {
        return mem_perm[para][reset];
    }

    inline void     set_mem_perm( const uint8_t  para, const bool reset,
                                  const t_perm   perm )
    {
        mem_perm[para][reset] = perm;
    }

    inline uint16_t get_pageflip_map ( const uint8_t chap,
                                       const uint8_t page ) const
    {
        return pfl_map[chap][page];
    }

    inline t_perm   get_pageflip_perm( const uint8_t  chap,
                                       const uint8_t  page ) const
    {
        return pfl_perm[chap][page];
    }

    inline void     set_pageflip_map ( const uint8_t  chap,
                                       const uint8_t  page,
                                       const uint16_t map  )
    {
        pfl_map[chap][page] = map & 0x007F;
    }

    inline void     set_pageflip_perm( const uint8_t  chap,
                                       const uint8_t  page,
                                       const t_perm   perm )
    {
        pfl_perm[chap][page] = perm;
    }

    inline void     set_pageflip     ( const uint8_t  chap,
                                       const uint8_t  page,
                                       const uint16_t map,
                                       const t_perm   perm )
    {
        set_pageflip_map ( chap, page, map  );
        set_pageflip_perm( chap, page, perm );
    }

    inline bool     get_initialized  ( const uint32_t addr ) const
    {
        validate_addr( "t_locutus::get_initialized", addr );
        return initialized[ addr ];
    }

    inline void     set_initialized  ( const uint32_t addr, const bool ini )
    {
        validate_addr( "t_locutus::set_initialized", addr );
        initialized[ addr ] = ini;
    }

    inline bool get_feature_flag( const int feat ) const
    {
        return feat < 128 ? feature_flag[feat] : false;
    }

    inline void set_feature_flag( const int feat, const bool value )
    {
        if ( feat < 128 ) feature_flag[feat] = value;
    }

    inline void set_uid( const uint64_t uid_ )
    {
        uid = uid_;
    }

    inline uint64_t get_uid( void ) const
    {
        return uid;
    }

    t_addr_list get_initialized_span_list( void ) const;

    t_metadata&       get_metadata( void )       { return metadata; }
    const t_metadata& get_metadata( void ) const { return metadata; }

    void set_metadata( const t_metadata& m )
    {
        metadata = m;
    }

    bool was_scrambled() const
    {
        return is_scrambled;
    }

    const uint8_t* get_scramble_druid() const
    {
        return is_scrambled ? druid : nullptr;
    }

    // -------------------------------------------------------------------- //
    //  Higher level feature-flag interface                                 //
    // -------------------------------------------------------------------- //
    void set_compat_ecs( const compat_level_t ecs_compat );
    compat_level_t get_compat_ecs( void ) const;

    void set_compat_voice( const compat_level_t voice_compat );
    compat_level_t get_compat_voice( void ) const;

    void set_compat_intv2( const compat_level_t intv2_compat );
    compat_level_t get_compat_intv2( void ) const;

    void set_compat_kc( const compat_level_t kc_compat );
    compat_level_t get_compat_kc( void ) const;

    void set_compat_tv( const compat_level_t tv_compat );
    compat_level_t get_compat_tv( void ) const;

    void set_jlp_features( const jlp_accel_t jlp_accel,
                           const unsigned jlp_flash );
    jlp_accel_t get_jlp_accel( void ) const;
    unsigned    get_jlp_flash( void ) const;

    void set_enable_lto_mapper( const bool lto_mapper );
    bool get_enable_lto_mapper( void ) const;

    void set_explicit_flags( const bool explicit_flags );
    bool get_explicit_flags( void ) const;

    // -------------------------------------------------------------------- //
    //  Used only by LUIGI loader                                           //
    // -------------------------------------------------------------------- //
    void copy_feature_flag_to_metadata( void );
    void set_scramble_druid( const bool is_scrambled_,
                             const uint8_t* druid_ = nullptr )
    {
        is_scrambled = is_scrambled_ && druid_;
        if ( is_scrambled )
            std::copy( druid_, druid_ + sizeof( druid ), std::begin( druid ) );
        else
            std::fill( std::begin( druid ), std::end( druid ), 0 );
    }
};

#endif
