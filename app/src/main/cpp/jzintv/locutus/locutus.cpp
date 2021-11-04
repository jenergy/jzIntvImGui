// ======================================================================== //
//  LOCUTUS -- aka. LTO Flash!                                              //
//                                                                          //
//  This class models Locutus, and serves as a structured container for     //
//  a Locutus compatible cartridge.  This may be initialized by or          //
//  serialized to a LUIGI file.                                             //
//                                                                          //
//  Note:  This is not tightly integrated into jzIntv at all.  Rather,      //
//  some adaptor C code will bridge between this C++ and jzIntv.            //
// ======================================================================== //

#include "locutus.hpp"
#include <cstring>
#include <algorithm>
#include <iterator>

#define LOC_JLP_SG_OFS (224)    // Arbitrary; however must be mult of 8

using namespace std;
extern "C"
{
    extern int jlp_accel_on;
    extern int lto_isa_enabled;
}

// ------------------------------------------------------------------------ //
//  Constructor                                                             //
// ------------------------------------------------------------------------ //
t_locutus::t_locutus()
:   uid          ( 0          )
,   cpu_cache    ( nullptr    )
,   jlp_savegame (            )
,   jlp_sg_start ( 0          )
,   jlp_sg_end   ( 0          )
,   jlp_sg_addr  ( 0          )
,   jlp_sg_row   ( 0          )
,   jlp_sg_file  ( nullptr    )
,   jlp_crc16    ( 0          )
,   jlp_xregs    ( nullptr    )
,   jlp_sleep    ( 0          )
,   jlp_rand     ( 0x4A5A6A7A )
,   is_scrambled ( false      )
{
    memset( static_cast<void *>(&ram     [0]   ), 0, sizeof( ram      ) );
    memset( static_cast<void *>(&mem_map [0][0]), 0, sizeof( mem_map  ) );
    memset( static_cast<void *>(&pfl_map [0][0]), 0, sizeof( pfl_map  ) );
    memset( static_cast<void *>(druid          ), 0, sizeof( druid    ) );

    for (int i = 0; i < 256; i++)
        mem_perm[i][0].reset();

    for (int i = 0; i < 256; i++)
        mem_perm[i][1].reset();

    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++)
            pfl_perm[i][j].reset();

    for (int i = 0; i < 16; i++)
        bsel[i] = 0;

    for (int i = 0; i < 16; i++)
        jlp_regs[i] = 0;

    initialized.reset();
}

// ------------------------------------------------------------------------ //
//  GET_INITIALIZED_SPAN_LIST  Get the spans of initialized memory          //
// ------------------------------------------------------------------------ //
t_addr_list t_locutus::get_initialized_span_list( void ) const
{
    uint32_t    s_addr  = 0, addr;
    bool        in_span = false;
    t_addr_list init_spans;

    for (addr = 0; addr < 512 * 1024; addr++)
    {
        if ( !in_span && initialized[addr] )
        {
            s_addr  = addr;
            in_span = true;

        } else if ( in_span && !initialized[addr] )
        {
            init_spans.push_back( t_addr_span( s_addr, addr - 1 ) );
            in_span = false;
        }
    }

    if (in_span)
        init_spans.push_back( t_addr_span( s_addr, addr - 1 ) );

    return init_spans;
}


// ======================================================================== //
//  Intellivision Interface!                                                //
// ======================================================================== //


// ------------------------------------------------------------------------ //
//  INTV_READ                                                               //
// ------------------------------------------------------------------------ //
uint16_t t_locutus::intv_read( const uint16_t intv_addr,
                               const bool force ) const
{
    jlp_rand.update( intv_addr, 16 );

    if ( jlp_sleep > 0 ) { jlp_sleep--; return 0xFFFF; }

    if ( ( intv_addr <= 0x04FF                        ) ||
         ( intv_addr >= 0x1000 && intv_addr <= 0x1FFF ) ||
         ( intv_addr >= 0x3000 && intv_addr <= 0x47FF ) )
        return 0xFFFF;

    const uint8_t para = intv_addr >> 8;
    const t_perm  perm = get_mem_perm( para, false );

    if ( jlp_accel_on &&
         ( ( intv_addr >= 0x8000 && intv_addr <= 0x803F ) ||
           ( intv_addr >= 0x9F80 && intv_addr <= 0x9FFF ) ) )
        return do_jlp_accel_read( intv_addr );

    if ( force || perm[LOCUTUS_PERM_READ] )
    {
        const uint32_t map       = get_mem_map( para, false );
        const uint32_t locu_addr = ( map << 8 ) | ( intv_addr & 0xFF );

        return read( locu_addr );
    } else
    {
        return 0xFFFF;
    }
}

// ------------------------------------------------------------------------ //
//  DO_BANKSW_WRITE                                                         //
// ------------------------------------------------------------------------ //
void    t_locutus::do_banksw_write( const uint16_t addr,
                                    const uint16_t data )
{
    const uint16_t hchap_addr = ((addr & 0xF) << 12) | ((addr & 0x10) << 7);
    const uint8_t  para       = hchap_addr >> 8;

    if ( !get_mem_perm( para, false )[LOCUTUS_PERM_BANKSW] )
        return;

    // Intellicart bank switching:
    //
    //  Each register accepts an 8-bit value.  (The upper 8 bits of the
    //  value written are ignored.)  The lower 8 bits of the value written
    //  specify the upper 8 bits of the target address for that 2K range.
    //  The target address is combined with the CPU address in this manner:
    //
    //      target_addr = bank_select << 8;
    //      icart_addr  = (intv_addr & 0x07FF) + target_addr;

    for (int i = 0; i < 8; i++)
    {
        set_mem_map( para + i, false, (data + i) & 0xFF );
    }

    if (cpu_cache)
    {
        const uint16_t cpu_addr = para << 8;
        cpu_cache->invalidate( cpu_addr, cpu_addr + 2047 );
    }
}

// ------------------------------------------------------------------------ //
//  DO_PAGEFLIP_WRITE                                                       //
// ------------------------------------------------------------------------ //
void    t_locutus::do_pageflip_write( const uint16_t intv_addr,
                                      const uint16_t data )
{
    const uint8_t chap = intv_addr >> 12;
    const uint8_t page = data & 0xF;

    if ( chap == 0 )
        return;

    if ( !get_pageflip_perm( chap, page )[LOCUTUS_PERM_BANKSW] )
        return;

    t_perm         perm = get_pageflip_perm( chap, page );
    const uint16_t map  = get_pageflip_map ( chap, page ) << 4;
    const uint8_t  para = chap << 4;

    perm[LOCUTUS_PERM_BANKSW] = false;
    bsel[chap] = page;

    if ( chap != 4 )
    {
        for (int i = 0; i < 16; i++)
        {
            set_mem_map ( para + i, false, map + i );
            set_mem_perm( para + i, false, perm    );
        }

        if (cpu_cache)
            cpu_cache->invalidate( para << 8, (para << 8) + 4095 );

    } else
    {
        // on $4xxx, only flip $4800 - $4FFF
        for (int i = 8; i < 16; i++)
        {
            set_mem_map ( para + i, false, map + i );
            set_mem_perm( para + i, false, perm    );
        }

        if (cpu_cache)
            cpu_cache->invalidate( 0x4800, 0x4FFF );
    }
}


// ------------------------------------------------------------------------ //
//  INTV_WRITE                                                              //
// ------------------------------------------------------------------------ //
void    t_locutus::intv_write( const uint16_t intv_addr,
                               const uint16_t data )
{
    jlp_rand.update( data,      16 );
    jlp_rand.update( intv_addr, 16 );

    if ( jlp_sleep > 0 ) { jlp_sleep--; return; }

    if ( intv_addr >= 0x40 && intv_addr <= 0x5F)
    {
        do_banksw_write( intv_addr, data );
        return;
    }

    const uint8_t our_para = intv_addr >> 8;
    const t_perm  our_perm = get_mem_perm( our_para, false );

    // Handle PV register updates
    if ( intv_addr != 0x9F8D &&
            (our_perm[LOCUTUS_PERM_READ] || our_perm[LOCUTUS_PERM_WRITE]) )
    {
        do_jlp_accel_write( 0x9F8D, intv_read( intv_addr, true ) );
    }

    // Intercept JLP accelerator writes even before page-flip writes
    if ( jlp_accel_on &&
         ( ( intv_addr >= 0x8000 && intv_addr < 0x8040 ) ||
           ( intv_addr >= 0x9F80 && intv_addr < 0x9FFF ) ) )
    {
        do_jlp_accel_write( intv_addr, data );
        return;
    }

    // That includes the switch to turn the accelerators on
    if ( !jlp_accel_on && intv_addr == 0x8033 && data == 0x4A5A &&
        get_jlp_accel() != JLP_DISABLED )
    {
        flip_jlp_accel( true );
        return;
    }

    // Always handle page-flip writes before anything else (except JLP)
    if ( ( intv_addr & 0x0FFF) == 0xFFF &&
         ((intv_addr & 0xF000) | 0x0A50 ) == ( data & 0xFFF0 ) &&
         (!jlp_accel_on || (intv_addr != 0x8FFF && intv_addr != 0x9FFF)) )
    {
        do_pageflip_write( intv_addr, data );
        return;
    }

    // Feature flag 32 controls whether we respond to Locutus mapper writes
    // in the $1000 - $14FF address range.  If it's not set, we ignore the
    // write.  This should be controlled by the lto_mapper variable in the
    // .CFG file used to create the LUIGI.
    //
    // Also, ignore the write if the user marked the segment writeable.
    if ( feature_flag[32] && !our_perm[LOCUTUS_PERM_WRITE] )
    {
        const bool reset_copy = intv_addr & 0x100;

        //  Locutus native memory mapping update
        if ( intv_addr >= 0x1000 && intv_addr <= 0x11FF )
        {
            uint8_t upd_para = intv_addr & 0xFF;
            set_mem_map( upd_para, reset_copy, data );
            return;
        }

        //  Locutus native memory permission update
        if ( intv_addr >= 0x1200 && intv_addr <= 0x13FF )
        {
            uint8_t upd_para = intv_addr & 0xFF;
            t_perm  new_perm;

            new_perm[0] = data & (1 << 0);
            new_perm[1] = data & (1 << 1);
            new_perm[2] = data & (1 << 2);
            new_perm[3] = data & (1 << 3);

            set_mem_perm( upd_para, reset_copy, new_perm );
            return;
        }

        //  Locutus native page-flip table update
        if ( intv_addr >= 0x1400 && intv_addr <= 0x14FF )
        {
            const int i = (intv_addr >> 4) & 0xF;
            const int j = (intv_addr >> 0) & 0xF;

            const uint16_t m = (data & 0x07F0) >> 4; // memory map
            t_perm new_perm;

            new_perm[0] = data & (1 << 0);
            new_perm[1] = data & (1 << 1);
            new_perm[2] = data & (1 << 2);
            new_perm[3] = data & (1 << 3);

            set_pageflip_map ( i, j, m        );
            set_pageflip_perm( i, j, new_perm );

            return;
        }
    }

    //  Normal data write!  Who'd have thunk it?
    if ( our_perm[LOCUTUS_PERM_WRITE] )
    {
        const uint32_t map       = get_mem_map( our_para, false );
        const uint32_t locu_addr = ( map << 8 ) | (intv_addr & 0xFF);

        write( locu_addr, data );
    }
}

// ------------------------------------------------------------------------ //
//  INTV_RESET                                                              //
// ------------------------------------------------------------------------ //
void    t_locutus::intv_reset( void )
{
    for (int para = 0; para < 256; para++)
        set_mem_map ( para, false, get_mem_map( para, true ) );

    for (int para = 0; para < 256; para++)
        set_mem_perm( para, false, get_mem_perm( para, true ) );

    for (int chap = 0; chap < 16; chap++)
        bsel[chap] = 0;

    const auto jlp_accel = get_jlp_accel();
    jlp_accel_on = false;

    if ( jlp_accel == JLP_ACCEL_ON || jlp_accel == JLP_ACCEL_FLASH_ON )
        flip_jlp_accel( true );
    // Note: Do *nothing* if JLP_DISABLED or JLP_ACCEL_OFF

    const auto lto_mapper = get_enable_lto_mapper();
    lto_isa_enabled = lto_mapper || jlp_accel != JLP_DISABLED;
}

// ======================================================================== //
//  JLP Acceleration Support                                                //
// ======================================================================== //

// ------------------------------------------------------------------------ //
//  INIT_JLP_FLASH      Initialize JLP Flash support.                       //
// ------------------------------------------------------------------------ //
void t_locutus::init_jlp_flash( const char* jlp_savegame_ )
{
    // Close any open save-game file image
    if ( jlp_sg_file )
    {
        fclose( jlp_sg_file );
        jlp_sg_file = nullptr;
    }

    // Get JLP flash parameters
    const auto jlp_accel = get_jlp_accel();
    const auto jlp_flash = jlp_accel == JLP_DISABLED ? 0u : get_jlp_flash();

    // If flash is disabled, set it up that way.
    if ( !jlp_flash )
    {
        jlp_sg_start = 0;
        jlp_sg_end   = 0;
        jlp_sg_bytes = 0;

        jlp_sg_img.resize(0);
        return;
    }

    // Otherwise, initialize the JLP flash image
    jlp_sg_start = LOC_JLP_SG_OFS;
    jlp_sg_end   = LOC_JLP_SG_OFS + jlp_flash * 8 - 1;
    jlp_sg_bytes = 192 * 8 * jlp_flash;
    jlp_sg_addr  = 0xBADDu;
    jlp_sg_row   = 0xBADDu;

    jlp_sg_img.resize( jlp_sg_bytes, 0xFF );

    // If we were given a backing file, open it.
    if ( jlp_savegame_ )
        jlp_savegame = jlp_savegame_;
    else
        jlp_savegame.clear();

    const char *fname = jlp_savegame_;
    if ( fname &&
         ( (jlp_sg_file = fopen(fname, "r+b")) != nullptr ||
           (jlp_sg_file = fopen(fname, "w+b")) != nullptr ) )
    {
        // Try to read in the file image
        fseek( jlp_sg_file, 0, SEEK_SET );
        if ( fread( &(jlp_sg_img[0]), 1, jlp_sg_bytes, jlp_sg_file) !=
                jlp_sg_bytes )
        {
            // ignore errors here for now; they're OK
        }

        // Create a backup file if we can
        std::string jlp_savegame_bak = jlp_savegame + "~";
        const char *fname_bak = jlp_savegame_bak.c_str();

        FILE *f_bak = fopen( fname_bak, "wb" );

        if (f_bak)
        {
            fwrite( &(jlp_sg_img[0]), 1, jlp_sg_bytes, f_bak );
            fclose( f_bak );
        }

        // Write our initial image to the backing file as part of init
        fseek( jlp_sg_file, 0, SEEK_SET );
        if ( fwrite( &(jlp_sg_img[0]), 1, jlp_sg_bytes, jlp_sg_file )
                                                            != jlp_sg_bytes )
        {
            throw std::string(
                    "Locutus JLP: Unable to initialize backing file\n");
        }
        fflush( jlp_sg_file );
    }
}

// ------------------------------------------------------------------------ //
//  DO_JLP_ACCEL_READ                                                       //
// ------------------------------------------------------------------------ //
uint16_t t_locutus::do_jlp_accel_read( const uint16_t addr ) const
{
    if ( addr >= 0x8000 && addr < 0x8023 ) return 0;
    if ( addr == 0x8023 )                  return jlp_sg_start;
    if ( addr == 0x8024 )                  return jlp_sg_end;
    if ( addr >= 0x8025 && addr < 0x8040 ) return 0;
    if ( addr >= 0x9F80 && addr < 0x9F90 ) return jlp_regs[ addr & 0xF ];
    if ( addr >= 0x9F90 && addr < 0x9FA0  && jlp_xregs )
        return jlp_xregs[ addr & 0xF ];
    if ( addr >= 0x9FA0 && addr < 0x9FFD ) return 0xFFFF;
    if ( addr == 0x9FFD )                  return jlp_crc16.get();
    if ( addr == 0x9FFE )                  return next_rand() & 0xFFFF;
    if ( addr == 0x9FFF )                  return 0;


    return 0xFFFF;
}

// ------------------------------------------------------------------------ //
//  DO_JLP_ACCEL_WRITE                                                      //
// ------------------------------------------------------------------------ //
void t_locutus::do_jlp_accel_write( const uint16_t addr, const uint16_t data )
{
    // -------------------------------------------------------------------- //
    //  Check for mult/div writes                                           //
    //      $9F8(0,1):  s16($9F80) x s16($9F81) -> s32($9F8F:$9F8E)         //
    //      $9F8(2,3):  s16($9F82) x u16($9F83) -> s32($9F8F:$9F8E)         //
    //      $9F8(4,5):  u16($9F84) x s16($9F85) -> s32($9F8F:$9F8E)         //
    //      $9F8(6,7):  u16($9F86) x u16($9F87) -> u32($9F8F:$9F8E)         //
    //      $9F8(8,9):  s16($9F88) / s16($9F89) -> quot($9F8E), rem($9F8F)  //
    //      $9F8(A,B):  u16($9F8A) / u16($9F8B) -> quot($9F8E), rem($9F8F)  //
    // -------------------------------------------------------------------- //
    if ( addr >= 0x9F80 && addr <= 0x9F8F )
    {
        uint32_t prod, quot, rem;

        jlp_regs[ addr & 0xF ] = data;

        switch (addr & 0xF)
        {
            case 0: case 1:
                prod = uint32_t( int32_t(  int16_t( jlp_regs[0] ) ) *
                                 int32_t(  int16_t( jlp_regs[1] ) ) );
                jlp_regs[0xE] = prod;
                jlp_regs[0xF] = prod >> 16;
                break;

            case 2: case 3:
                prod = uint32_t( int32_t(  int16_t( jlp_regs[2] ) ) *
                                 int32_t( uint16_t( jlp_regs[3] ) ) );
                jlp_regs[0xE] = prod;
                jlp_regs[0xF] = prod >> 16;
                break;

            case 4: case 5:
                prod = uint32_t( int32_t( uint16_t( jlp_regs[4] ) ) *
                                 int32_t(  int16_t( jlp_regs[5] ) ) );
                jlp_regs[0xE] = prod;
                jlp_regs[0xF] = prod >> 16;
                break;

            case 6: case 7:
                prod = uint32_t( uint32_t( uint16_t( jlp_regs[6] ) ) *
                                 uint32_t( uint16_t( jlp_regs[7] ) ) );
                jlp_regs[0xE] = prod;
                jlp_regs[0xF] = prod >> 16;
                break;

            case 8: case 9:
                if ( jlp_regs[9] )
                {
                    quot = uint32_t( int16_t( jlp_regs[8] ) /
                                     int16_t( jlp_regs[9] ) );
                    rem  = uint32_t( int16_t( jlp_regs[8] ) %
                                     int16_t( jlp_regs[9] ) );
                    jlp_regs[0xE] = quot;
                    jlp_regs[0xF] = rem;
                }
                break;

            case 10: case 11:
                if ( jlp_regs[11] )
                {
                    quot = uint32_t( uint16_t( jlp_regs[10] ) /
                                     uint16_t( jlp_regs[11] ) );
                    rem  = uint32_t( uint16_t( jlp_regs[10] ) %
                                     uint16_t( jlp_regs[11] ) );
                    jlp_regs[0xE] = quot;
                    jlp_regs[0xF] = rem;
                }
                break;
        }
        return;
    }

    // -------------------------------------------------------------------- //
    //  Check for JLP X-Regs                                                //
    // -------------------------------------------------------------------- //
    if ( addr >= 0x9F90 && addr <= 0x9F9F && jlp_xregs )
    {
        jlp_xregs[ addr & 0xF ] = data;
        return;
    }

    // -------------------------------------------------------------------- //
    //  Check for Save-Game Arch V2 writes                                  //
    //                                                                      //
    //      $8025   JLP RAM address to operate on                           //
    //      $8026   Flash row to operate on                                 //
    //                                                                      //
    //      $802D   Copy JLP RAM to flash row (unlock: $C0DE)               //
    //      $802E   Copy JLP RAM to flash row (unlock: $DEC0)               //
    //      $802F   Copy JLP RAM to flash row (unlock: $BEEF)               //
    // -------------------------------------------------------------------- //
    if ( addr == 0x8025 ) { jlp_sg_addr = data; return; }
    if ( addr == 0x8026 ) { jlp_sg_row  = data; return; }

    if ( addr == 0x802D && data == 0xC0DE ) { jlp_sg_ram_to_flash(); return; }
    if ( addr == 0x802E && data == 0xDEC0 ) { jlp_sg_flash_to_ram(); return; }
    if ( addr == 0x802F && data == 0xBEEF ) { jlp_sg_erase_sector(); return; }

    // -------------------------------------------------------------------- //
    //  Check for CRC-16 accelerator writes                                 //
    // -------------------------------------------------------------------- //
    if ( addr == 0x9FFC ) jlp_crc16.update( data, 16 );
    if ( addr == 0x9FFD ) jlp_crc16.set( data );

    // -------------------------------------------------------------------- //
    //  Check for JLP Accelerator Disable                                   //
    // -------------------------------------------------------------------- //
    if ( addr == 0x8034 && data == 0x6A7A )
        flip_jlp_accel( false );
}

// ------------------------------------------------------------------------ //
//  JLP_SG_VALIDATE_ARGS    Ensure JLP SG command arguments are valid       //
// ------------------------------------------------------------------------ //
bool t_locutus::jlp_sg_validate_args( const bool chk_addr ) const
{
    if ( jlp_sg_row < jlp_sg_start || jlp_sg_row > jlp_sg_end )
        return false;

    if ( chk_addr && (jlp_sg_addr < 0x8040 || jlp_sg_addr + 95 > 0x9F7F) )
        return false;

    return true;
}

// ------------------------------------------------------------------------ //
//  JLP_SG_RAM_TO_FLASH     Copy RAM to a row in flash                      //
// ------------------------------------------------------------------------ //
void t_locutus::jlp_sg_ram_to_flash( void )
{
    jlp_sleep = 400 + next_rand() % 200;

    if ( !jlp_sg_validate_args() )
        return;

    const uint32_t row_idx = (jlp_sg_row - jlp_sg_start) * 192;

    // Refuse to write if row isn't empty
    for ( int i = 0; i < 192; i++ )
        if ( jlp_sg_img[ row_idx + i ] != 0xFF )
            return;

    // Copy the data from RAM to flash in little-endian
    for ( int i = 0, a = 0; i < 192; i += 2, a++ )
    {
        const uint16_t intv_addr = jlp_sg_addr + a;
        const uint16_t para      = intv_addr >> 8;
        const uint32_t map       = get_mem_map( para, false );
        const uint32_t locu_addr = ( map << 8 ) | ( intv_addr & 0xFF );
        const uint16_t data      = read( locu_addr );

        jlp_sg_img[ row_idx + i + 0 ] = data & 0xFF;
        jlp_sg_img[ row_idx + i + 1 ] = data >> 8;
    }

    // Update backing file
    if ( jlp_sg_file )
    {
        fseek( jlp_sg_file, row_idx, SEEK_SET );
        fwrite( &(jlp_sg_img[row_idx]), 2, 96, jlp_sg_file );
        fflush( jlp_sg_file );
    }
}

// ------------------------------------------------------------------------ //
//  JLP_SG_FLASH_TO_RAM     Copy a row in flash to RAM                      //
// ------------------------------------------------------------------------ //
void t_locutus::jlp_sg_flash_to_ram( void )
{
    jlp_sleep = 10 + next_rand() % 20;

    if ( !jlp_sg_validate_args() )
        return;

    const uint32_t row_idx = (jlp_sg_row - jlp_sg_start) * 192;

    // Copy the data from flash to RAM in little-endian
    for ( int i = 0, a = 0; i < 192; i += 2, a++ )
    {
        const uint16_t intv_addr = jlp_sg_addr + a;
        const uint16_t para      = intv_addr >> 8;
        const uint32_t map       = get_mem_map( para, false );
        const uint32_t locu_addr = ( map << 8 ) | ( intv_addr & 0xFF );
        const uint16_t data_lo   = jlp_sg_img[ row_idx + i + 0 ];
        const uint16_t data_hi   = jlp_sg_img[ row_idx + i + 1 ];
        const uint16_t data      = data_lo | (data_hi << 8);

        write( locu_addr, data );
    }
}

// ------------------------------------------------------------------------ //
//  JLP_SG_ERASE_SECTOR     Erase a sector (8 rows)                         //
// ------------------------------------------------------------------------ //
void t_locutus::jlp_sg_erase_sector( void )
{
    jlp_sleep = 800 + next_rand() % 200;

    if ( !jlp_sg_validate_args( false ) )
        return;

    const uint32_t row_idx = ((jlp_sg_row - jlp_sg_start) & ~7) * 192;

    memset( &(jlp_sg_img[row_idx]), 0xFF, 192 * 8 );

    // Update backing file
    if ( jlp_sg_file )
    {
        fseek( jlp_sg_file, row_idx, SEEK_SET );
        fwrite( &(jlp_sg_img[row_idx]), 2, 96 * 8, jlp_sg_file );
        fflush( jlp_sg_file );
    }
}

// ------------------------------------------------------------------------ //
//  FLIP_JLP_ACCEL      Flip the JLP Accelerators on/off.                   //
// ------------------------------------------------------------------------ //
void t_locutus::flip_jlp_accel( const bool new_jlp_accel_on )
{
//  printf("Locutus: flip_jlp_accel %s\n", new_jlp_accel_on ? "ON" : "off");
//  fflush(stdout);
    // Locutus ignores attempts to switch into the already-active mode.
    // This is important, as updates to the various memory mapper structures
    // that happened since the last flip will be retained.
    if ( new_jlp_accel_on == !!jlp_accel_on )
        return;

    jlp_accel_on = new_jlp_accel_on;

    // Turning JLP Accelerators + RAM on:
    //
    //  -- Map $8040 - $9F7F through to Locutus $10040 - $11F7F
    //  -- Mark $8040 - $9F7F +R, +W, 16-bit, no page flip, no bankswitch
    //  -- JLP Accelerators become visible at $8000 - $803F, $9F80 - $9FFF
    //
    // Note that we don't actually modify the flip table, and we do remember
    // which page was last flipped in on $8xxx and $9xxx.
    if ( jlp_accel_on )
    {
        const t_perm perm = 0x3;  // +R, +W, 16-bit, no pageflip/bankswitch
        uint16_t jlp_map = 0x0100;

        for ( uint8_t para = 0x80 ; para <= 0x9F ; para++ )
        {
            set_mem_perm( para, false, perm );
            set_mem_map ( para, false, jlp_map++ );
        }
    } else
    // Turning JLP Accelerators + RAM off:
    //
    //  -- $8000 - $9FFF get mapped to most recently flipped-in page
    //  -- JLP Accelerators become invisible
    {
        for ( uint8_t chap = 8; chap <= 9; chap++ )
        {
            const uint8_t  page = bsel[chap];
            t_perm         perm = get_pageflip_perm( chap, page );
            const uint16_t map  = get_pageflip_map ( chap, page ) << 4;
            const uint8_t  para = chap << 4;

            perm[LOCUTUS_PERM_BANKSW] = false;

            for (int i = 0; i < 16; i++)
            {
                set_mem_map ( para + i, false, map + i );
                set_mem_perm( para + i, false, perm    );
            }
        }
    }

    if (cpu_cache)
        cpu_cache->invalidate( 0x8000, 0x9FFF );
}


// ------------------------------------------------------------------------ //
//  Feature Flags                                                           //
//                                                                          //
//  Bit(s)  Description                                                     //
//  ------- -----------------------------------------------------------     //
//  0..1    Intellivoice compatibility                                      //
//  2..3    ECS compatibility                                               //
//  4..5    Intellivision 2 compatibility                                   //
//  6..7    Keyboard Component compatibility                                //
//  8..15   Reserved                                                        //
//  16..17  JLP Accelerator Enable (MPY/DIV/etc at $9F80 - $9FFF)           //
//  18..21  Reserved                                                        //
//  22..31  JLP Flash Save-Game minimum required size in sectors            //
//  32      LTO Mapper                                                      //
//  33..62  Reserved                                                        //
//  63      Defaults flag:  0 = defaults, 1 = explicitly set by user        //
//  64..127 Reserved.                                                       //
//                                                                          //
//  Note: t_metadata and feature_flags must stay in sync.  If an outside    //
//  class directly modifies t_metadata or the feature flags, it must take   //
//  an additional step to bring them back in sync.                          //
//                                                                          //
//  The getters will assert() if they are out of sync at a get().           //
// ------------------------------------------------------------------------ //
void t_locutus::set_compat_voice( const compat_level_t voice_compat )
{
    const int voice = int( voice_compat );
    set_feature_flag(  0, voice     & 1 );
    set_feature_flag(  1, voice     & 2 );
    metadata.voice_compat = voice_compat;
}

void t_locutus::set_compat_ecs  ( const compat_level_t ecs_compat )
{
    const int ecs = int( ecs_compat );
    set_feature_flag(  2, ecs       & 1 );
    set_feature_flag(  3, ecs       & 2 );
    metadata.ecs_compat = ecs_compat;
}

void t_locutus::set_compat_intv2( const compat_level_t intv2_compat )
{
    const int intv2 = int( intv2_compat );
    set_feature_flag(  4, intv2     & 1 );
    set_feature_flag(  5, intv2     & 2 );
    metadata.intv2_compat = intv2_compat;
}

void t_locutus::set_compat_kc( const compat_level_t kc_compat )
{
    const int kc = int( kc_compat );
    set_feature_flag(  6, kc        & 1 );
    set_feature_flag(  7, kc        & 2 );
    metadata.kc_compat = kc_compat;
}

void t_locutus::set_compat_tv( const compat_level_t tv_compat )
{
    const int tv = int( tv_compat );
    if ( tv_compat == CMP_TOLERATES ) {
        set_feature_flag(  8, 0 );
        set_feature_flag(  9, 0 );
        set_feature_flag( 10, 0 );
        set_feature_flag( 11, 0 );
    } else
    {
        set_feature_flag(  8, 1 );
        set_feature_flag(  9, 0 );
        set_feature_flag( 10, tv & 1 );
        set_feature_flag( 11, tv & 2 );
    }
    metadata.tv_compat = tv_compat;
}

void t_locutus::set_jlp_features( const jlp_accel_t jlp_accel,
                                  const unsigned jlp_flash )
{
    const int jlp_flag = int( jlp_accel );

    set_feature_flag( 16, jlp_flag & 1 );
    set_feature_flag( 17, jlp_flag & 2 );

    // Note:  jlp_flash ignored if (jlp_accel & 2) == 0
    set_feature_flag( 22, ( jlp_flash >> 0 ) & 1 );
    set_feature_flag( 23, ( jlp_flash >> 1 ) & 1 );
    set_feature_flag( 24, ( jlp_flash >> 2 ) & 1 );
    set_feature_flag( 25, ( jlp_flash >> 3 ) & 1 );
    set_feature_flag( 26, ( jlp_flash >> 4 ) & 1 );
    set_feature_flag( 27, ( jlp_flash >> 5 ) & 1 );
    set_feature_flag( 28, ( jlp_flash >> 6 ) & 1 );
    set_feature_flag( 29, ( jlp_flash >> 7 ) & 1 );
    set_feature_flag( 30, ( jlp_flash >> 8 ) & 1 );
    set_feature_flag( 31, ( jlp_flash >> 9 ) & 1 );

    metadata.jlp_accel = jlp_accel;
    metadata.jlp_flash = jlp_flash;
}

void t_locutus::set_enable_lto_mapper( const bool lto_mapper )
{
    set_feature_flag( 32, lto_mapper ? 1 : 0 );
    metadata.lto_mapper = lto_mapper;
}

void t_locutus::set_explicit_flags( const bool explicit_flags )
{
    set_feature_flag( 63, explicit_flags ? 1 : 0 );
    metadata.is_defaults = !explicit_flags;
}

compat_level_t t_locutus::get_compat_voice( void ) const
{
    int voice = ( get_feature_flag( 0 ) ? 1 : 0 )
              | ( get_feature_flag( 1 ) ? 2 : 0 );
    compat_level_t voice_compat = compat_level_t( voice );
    assert( voice_compat == metadata.voice_compat );
    return voice_compat;
}

compat_level_t t_locutus::get_compat_ecs  ( void ) const
{
    int ecs = ( get_feature_flag( 2 ) ? 1 : 0 )
            | ( get_feature_flag( 3 ) ? 2 : 0 );
    compat_level_t ecs_compat = compat_level_t( ecs );
    assert( ecs_compat == metadata.ecs_compat );
    return ecs_compat;
}

compat_level_t t_locutus::get_compat_intv2( void ) const
{
    int intv2 = ( get_feature_flag( 4 ) ? 1 : 0 )
              | ( get_feature_flag( 5 ) ? 2 : 0 );
    compat_level_t intv2_compat = compat_level_t( intv2 );
    assert( intv2_compat == metadata.intv2_compat );
    return intv2_compat;
}

compat_level_t t_locutus::get_compat_kc   ( void ) const
{
    int kc = ( get_feature_flag( 6 ) ? 1 : 0 )
           | ( get_feature_flag( 7 ) ? 2 : 0 );
    compat_level_t kc_compat = compat_level_t( kc );
    assert( kc_compat == metadata.kc_compat );
    return kc_compat;
}

compat_level_t t_locutus::get_compat_tv   ( void ) const
{
    int ex = ( get_feature_flag( 8 ) ? 1 : 0 )
           | ( get_feature_flag( 9 ) ? 2 : 0 );
    int tv = ( get_feature_flag( 10 ) ? 1 : 0 )
           | ( get_feature_flag( 11 ) ? 2 : 0 );

    compat_level_t tv_compat = ex ? compat_level_t( tv ) : CMP_TOLERATES;
    assert( tv_compat == metadata.tv_compat );
    return tv_compat;
}

jlp_accel_t t_locutus::get_jlp_accel( void ) const
{
    int jlp_accel_val = ( get_feature_flag( 16 ) ? 1 : 0 )
                      | ( get_feature_flag( 17 ) ? 2 : 0 );
    jlp_accel_t jlp_accel = jlp_accel_t( jlp_accel_val );
    assert( jlp_accel == metadata.jlp_accel );
    return jlp_accel;
}

unsigned t_locutus::get_jlp_flash( void ) const
{
    // Note:  jlp_flash ignored if (jlp_accel & 2) == 0
    int jlp_flash = ( get_feature_flag( 22 ) ?   1 : 0 )
                  | ( get_feature_flag( 23 ) ?   2 : 0 )
                  | ( get_feature_flag( 24 ) ?   4 : 0 )
                  | ( get_feature_flag( 25 ) ?   8 : 0 )
                  | ( get_feature_flag( 26 ) ?  16 : 0 )
                  | ( get_feature_flag( 27 ) ?  32 : 0 )
                  | ( get_feature_flag( 28 ) ?  64 : 0 )
                  | ( get_feature_flag( 29 ) ? 128 : 0 )
                  | ( get_feature_flag( 30 ) ? 256 : 0 )
                  | ( get_feature_flag( 31 ) ? 512 : 0 );
    assert( jlp_flash == metadata.jlp_flash );
    return jlp_flash;
}

bool t_locutus::get_enable_lto_mapper( void ) const
{
    bool lto_mapper = get_feature_flag( 32 );
    assert( lto_mapper == metadata.lto_mapper );
    return lto_mapper;
}

bool t_locutus::get_explicit_flags( void ) const
{
    return get_feature_flag( 63 );
}

void t_locutus::copy_feature_flag_to_metadata( )
{
    int voice = ( get_feature_flag( 0 ) ? 1 : 0 )
              | ( get_feature_flag( 1 ) ? 2 : 0 );
    metadata.voice_compat = compat_level_t( voice );

    int ecs = ( get_feature_flag( 2 ) ? 1 : 0 )
            | ( get_feature_flag( 3 ) ? 2 : 0 );
    metadata.ecs_compat = compat_level_t( ecs );

    int intv2 = ( get_feature_flag( 4 ) ? 1 : 0 )
              | ( get_feature_flag( 5 ) ? 2 : 0 );
    metadata.intv2_compat = compat_level_t( intv2 );

    int kc = ( get_feature_flag( 6 ) ? 1 : 0 )
           | ( get_feature_flag( 7 ) ? 2 : 0 );
    metadata.kc_compat = compat_level_t( kc );

    int ex = ( get_feature_flag( 8 ) ? 1 : 0 )
           | ( get_feature_flag( 9 ) ? 2 : 0 );
    int tv = ( get_feature_flag( 10 ) ? 1 : 0 )
           | ( get_feature_flag( 11 ) ? 2 : 0 );
    metadata.tv_compat = ex ? compat_level_t( tv ) : CMP_TOLERATES;

    int jlp_accel_val = ( get_feature_flag( 16 ) ? 1 : 0 )
                      | ( get_feature_flag( 17 ) ? 2 : 0 );
    metadata.jlp_accel = jlp_accel_t( jlp_accel_val );

    // Note:  jlp_flash ignored if (jlp_accel & 2) == 0
    int jlp_flash = ( get_feature_flag( 22 ) ?   1 : 0 )
                  | ( get_feature_flag( 23 ) ?   2 : 0 )
                  | ( get_feature_flag( 24 ) ?   4 : 0 )
                  | ( get_feature_flag( 25 ) ?   8 : 0 )
                  | ( get_feature_flag( 26 ) ?  16 : 0 )
                  | ( get_feature_flag( 27 ) ?  32 : 0 )
                  | ( get_feature_flag( 28 ) ?  64 : 0 )
                  | ( get_feature_flag( 29 ) ? 128 : 0 )
                  | ( get_feature_flag( 30 ) ? 256 : 0 )
                  | ( get_feature_flag( 31 ) ? 512 : 0 );
    metadata.jlp_flash = jlp_flash;

    metadata.lto_mapper = get_feature_flag( 32 );
    metadata.is_defaults = !get_explicit_flags();
}

// ------------------------------------------------------------------------ //
//  T_CPU_CACHE_IF::~T_CPU_CACHE_IF                                         //
//  Anchor this class' VTable!                                              //
// ------------------------------------------------------------------------ //
t_cpu_cache_if::~t_cpu_cache_if()
{
    // Nothing.
}
