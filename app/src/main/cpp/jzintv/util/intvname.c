/* ======================================================================== */
/*  INTVNAME  -- Attempt to determine the name of a .BIN or .ROM file.      */
/*                                                                          */
/*  Usage:                                                                  */
/*      intvname foo.bin                                                    */
/*                                                                          */
/*  Outputs the name of the .BIN or .ROM on stdout, or nothing if it can    */
/*  not determine the name.  If the game's year is known, it will be        */
/*  output on the second line of output.                                    */
/*                                                                          */
/*  First looks in CRC database to see if file's CRC is in the database.    */
/*  If present, it outputs the name from the database.  Otherwise, it       */
/*  looks for a cartridge header at $5000 and tries to interpret that       */
/*  using some heuristics.                                                  */
/*                                                                          */
/* ======================================================================== */
/*                 Copyright (c) 2002-2006, Joseph Zbiciak                  */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "misc/crc32.h"
#include "misc/file_crc32.h"
#include "icart/icartrom.h"
#include "icart/icartbin.h"
#include "icart/icartfile.h"
#include "metadata/metadata.h"
#include "metadata/cfgvar_metadata.h"
#include "locutus/locutus_adapt.h"
#include "bincfg/bincfg.h"
#include "name/name.h"

icartrom_t          icart;
t_locutus_wrap      locutus;
game_metadata_t*    metadata;
bc_cfgfile_t*       bincfg;

int jlp_accel_on, lto_isa_enabled;
int debug = 0;

#define GET_BIT(bv,i,b) do {                                    \
                            int _ = (i);                        \
                            b = ((bv)[_ >> 5] >> (_ & 31)) & 1; \
                        } while(0)

#define MAX_NAME (64)

/* ======================================================================== */
/*  USAGE            -- Just give usage info and exit.                      */
/* ======================================================================== */
LOCAL void usage(void)
{
    fprintf(stderr,
                                                                          "\n"
    "INTVNAME"                                                            "\n"
    "Copyright 2016, Joseph Zbiciak"                                      "\n"
                                                                          "\n"
    "Usage: \n"
    "    intvname foo.bin\n"
    "    intvname foo.rom\n"
    "    intvname foo.luigi\n"
    "\n"
                                                                          "\n"
    "This program is free software; you can redistribute it and/or modify""\n"
    "it under the terms of the GNU General Public License as published by""\n"
    "the Free Software Foundation; either version 2 of the License, or"   "\n"
    "(at your option) any later version."                                 "\n"
                                                                          "\n"
    "This program is distributed in the hope that it will be useful,"     "\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of"      "\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU"   "\n"
    "General Public License for more details."                            "\n"
                                                                          "\n"
"You should have received a copy of the GNU General Public License along" "\n"
"with this program; if not, write to the Free Software Foundation, Inc.," "\n"
"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA."             "\n"
                                                                          "\n"
    );

    exit(0);
}

enum
{
    FT_UNKNOWN = -1,
    FT_BINCFG,
    FT_ICARTROM,
    FT_LUIGI
};

/* ======================================================================== */
/*  CP1600_INVALIDATE    -- Not used; stub to keep Locutus happy.           */
/* ======================================================================== */
void cp1600_invalidate( cp1600_t* const cp1600,
                        const uint32_t ah, const uint32_t al )
{
    UNUSED( cp1600 );
    UNUSED( ah );
    UNUSED( al );
}

/* ======================================================================== */
/*  FILE_TYPE        -- Attempt to determine what type of file this is.     */
/* ======================================================================== */
static int file_type( const char *fname )
{
    LZFILE *file = lzoe_fopen( fname, "rb" );
    long len = file ? file_length( file ) : -1;
    uint8_t buf[4];

    /* -------------------------------------------------------------------- */
    /*  If it's really short, someone's being a joker.  Say it's unknown.   */
    /* -------------------------------------------------------------------- */
    if ( len < 4 )
        goto fail;

    /* -------------------------------------------------------------------- */
    /*  Try to read 4 bytes from file.  If it fails, throw our hands up.    */
    /* -------------------------------------------------------------------- */
    if (lzoe_fseek( file, 0, SEEK_SET ))    goto fail;
    if (lzoe_fread( buf, 1, 4, file ) != 4) goto fail;
    if (lzoe_fseek( file, 0, SEEK_SET ))    goto fail;

    lzoe_fclose( file );
    file = NULL;

    /* -------------------------------------------------------------------- */
    /*  If file length is at least 53, and we see an Intellicart or CC3     */
    /*  baud byte, and the ROM segment count looks consistent, then try     */
    /*  to load it as an Intellicart/CC3.  If that succeeds, then return    */
    /*  Intellicart/CC3.                                                    */
    /* -------------------------------------------------------------------- */
    if (len >= 53 &&
        (buf[0] == 0xA8 || buf[0] == 0x41 || buf[0] == 0x61) &&
        ((buf[1] ^ buf[2]) & 0xFF) == 0xFF)
    {
        /* Strong chance it's an Intellicart/CC3.  Try loading/decoding it. */
        if (icartrom_readfile(fname, NULL, 1) > 0)
            return FT_ICARTROM;
    }

    /* -------------------------------------------------------------------- */
    /*  If the first for chars are LTO\0 or LTO\1, and it's at least 1319   */
    /*  bytes, assume it's a LUIGI file.                                    */
    /* -------------------------------------------------------------------- */
    if ( len >= 1319 &&
         (memcmp("LTO\0", buf, 4) == 0 || memcmp("LTO\1", buf, 4) == 0 ) )
        return FT_LUIGI;

    /* -------------------------------------------------------------------- */
    /*  Otherwise, assume it's a BIN+CFG file.                              */
    /* -------------------------------------------------------------------- */
    return FT_BINCFG;

    /* -------------------------------------------------------------------- */
    /*  Detection failed.  Close file and return FT_UNKNOWN.                */
    /* -------------------------------------------------------------------- */
fail:
    if ( file )
        lzoe_fclose( file );
    return FT_UNKNOWN;
}

typedef int (*load_file_fn)(const char *game_fn);
typedef int (*get_word_fn)(uint16_t addr);

/* Read metadata outside cartridge ROM info */
typedef const char *(*get_name_fn)( void );
typedef const char *(*get_short_name_fn)( void );
typedef int         (*get_year_fn)( void );
typedef uint32_t    (*get_alt_crc_fn)( void );

/* ======================================================================== */
/*  IC_LOAD_FILE     -- Load an Intellicart/CC3 .ROM file                   */
/* ======================================================================== */
static int ic_load_file(const char *rom_fn)
{
    icartrom_init(&icart);
    if ( icartrom_readfile(rom_fn, &icart, 1) < 0 )
        return -1;

    return 0;
}

/* ======================================================================== */
/*  IC_GET_WORD      -- Read a word from the Intellicart ROM.               */
/*                      Returns -1 if the memory isn't mapped, or is        */
/*                      bankswitched or writeable memory.                   */
/* ======================================================================== */
static int ic_get_word(uint16_t addr)
{
    int bit;
    int seg = addr >> 8;

    GET_BIT(icart.preload,  seg, bit); if (!bit) return -1;
    GET_BIT(icart.dobanksw, seg, bit); if ( bit) return -1;
    GET_BIT(icart.writable, seg, bit); if ( bit) return -1;

    return icart.image[addr];
}

/* ======================================================================== */
/*  IC_GET_NAME          -- Get name from metadata                          */
/*  IC_GET_SHORT_NAME    -- Get short name from metadata                    */
/*  IC_GET_NAME          -- Get year from metadata                          */
/* ======================================================================== */
static const char *ic_get_name( void )
{
    return icart.metadata ? icart.metadata->name : NULL;
}
static const char *ic_get_short_name( void )
{
    return icart.metadata ? icart.metadata->short_name : NULL;
}
static int ic_get_year( void )
{
    return icart.metadata && icart.metadata->release_dates
            ? icart.metadata->release_dates[0].year
            : -1;
}

/* ======================================================================== */
/*  LOC_LOAD_FILE    -- Load a Locutus / LUIGI file.                        */
/* ======================================================================== */
static int loc_load_file(const char *luigi_fn)
{
    if ( make_locutus( &locutus, luigi_fn, NULL, 1, NULL ) )
        return -1;

    metadata = get_locutus_metadata( &locutus );
    return 0;
}

/* ======================================================================== */
/*  LOC_GET_WORD     -- Get a word from the LUIGI ROM.  Returns -1 if       */
/*                      if the memory is not mapped.  Does not have any     */
/*                      visibility into writeable or bankswitched memory.   */
/*                      Also, can't distinguish 0xFFFF from unmapped vs.    */
/*                      mapped with data == 0xFFFF.  Ok for our purposes.   */
/* ======================================================================== */
static int loc_get_word(uint16_t addr)
{
    int data = locutus.periph.read( &locutus.periph, NULL, addr, 0 );
    return data == 0xFFFF ? -1 : data;
}

/* ======================================================================== */
/*  LOC_GET_NAME         -- Get name from metadata                          */
/*  LOC_GET_SHORT_NAME   -- Get short name from metadata                    */
/*  LOC_GET_NAME         -- Get year from metadata                          */
/*  LOC_GET_ALT_CRC      -- Return lower 32-bits of UID as possible CRC     */
/* ======================================================================== */
static const char *loc_get_name( void )
{
    return metadata->name;
}
static const char *loc_get_short_name( void )
{
    return metadata->short_name;
}
static int loc_get_year( void )
{
    return metadata->release_dates ? metadata->release_dates[0].year : -1;
}
static uint32_t loc_get_alt_crc( void )
{
    return get_locutus_uid( &locutus ) & 0xFFFFFFFFu;
}

/* ======================================================================== */
/*  BC_LOAD_FILE     -- Load a BIN + CFG file.                              */
/* ======================================================================== */
static int bc_load_file(const char *bin_fn)
{
    /* Ah, this sh!t-show. */
    int len = strlen( bin_fn );
    char *p_cfg_fn = CALLOC( char, (len + 5) * 2 );
    char *cfg1_fn = p_cfg_fn;
    char *cfg2_fn = p_cfg_fn ? p_cfg_fn + len + 5 : NULL;
    char *ext;
    char *cfg_fn = NULL;
    LZFILE *f_cfg = NULL;

    /* -------------------------------------------------------------------- */
    /*  Try two things: Replacing the extension with .cfg and just tacking  */
    /*  .cfg on the end.  We assume bin_fn exists.                          */
    /* -------------------------------------------------------------------- */
    if ( p_cfg_fn )
    {
        strcpy( cfg1_fn, bin_fn );
        ext = strrchr( cfg1_fn, '.' );
        if ( ext )
            strcpy( ext, ".cfg" );
        else
            cfg1_fn = NULL;

        strcpy( cfg2_fn, bin_fn );
        strcpy( cfg2_fn + len, ".cfg" );

        if ( cfg1_fn && file_exists( cfg1_fn ) )
            cfg_fn = strdup( cfg1_fn );
        else if ( cfg2_fn && file_exists( cfg2_fn ) )
            cfg_fn = strdup( cfg2_fn );

        free( p_cfg_fn );
    }

    /* -------------------------------------------------------------------- */
    /*  If we successfully found a config file, try to open it.             */
    /* -------------------------------------------------------------------- */
    if ( cfg_fn )
    {
        f_cfg = lzoe_fopen( cfg_fn, "r" );
        if ( !f_cfg )
            cfg_fn = NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Now try to read the CFG or generate a default CFG.                  */
    /* -------------------------------------------------------------------- */
    bincfg = bc_parse_cfg( f_cfg, bin_fn, cfg_fn );
    if ( f_cfg )
        lzoe_fclose( f_cfg );

    CONDFREE( cfg_fn );

    if ( !bincfg || !bincfg->span )
        return -1;

#ifndef BC_NODOMACRO
    /* -------------------------------------------------------------------- */
    /*  Apply any statically safe macros.  Ignore errors.                   */
    /* -------------------------------------------------------------------- */
    bc_do_macros( bincfg, 0 );
#endif

    /* -------------------------------------------------------------------- */
    /*  Populate the config with corresponding BIN.                         */
    /* -------------------------------------------------------------------- */
    if ( bc_read_data( bincfg ) )
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Reuse the Locutus logic for examining metadata from bincfg.         */
    /* -------------------------------------------------------------------- */
    metadata = bincfg->metadata;
    return 0;
}

/* ======================================================================== */
/*  BC_GET_WORD      -- Get a word from the BINCFG.  Returns -1 if          */
/*                      if the memory is not mapped, is bankswitched or is  */
/*                      writeable.                                          */
/* ======================================================================== */
static int bc_get_word(uint16_t addr)
{
    bc_memspan_t *span, *target_span = NULL;

    /* -------------------------------------------------------------------- */
    /*  Traverse the span list and take the last span that contains addr.   */
    /* -------------------------------------------------------------------- */
    for ( span = bincfg->span; span; span = (bc_memspan_t *)span->l.next )
    {
        /* ---------------------------------------------------------------- */
        /*  Only look for spans that are not ECS-paged or are in PAGE 0.    */
        /* ---------------------------------------------------------------- */
        if ( span->epage != 0 && span->epage != BC_SPAN_NOPAGE )
            continue;

        /* ---------------------------------------------------------------- */
        /*  If span doesn't overlap addr, skip it.                          */
        /* ---------------------------------------------------------------- */
        if ( span->s_addr > addr || span->e_addr < addr )
            continue;

        /* ---------------------------------------------------------------- */
        /*  If span is bankswitched or writeable, skip it.                  */
        /* ---------------------------------------------------------------- */
        if ( span->flags & (BC_SPAN_B | BC_SPAN_W) )
            continue;

        /* ---------------------------------------------------------------- */
        /*  If the span has no data, skip it.                               */
        /* ---------------------------------------------------------------- */
        if ( !span->data )
            continue;

        /* ---------------------------------------------------------------- */
        /*  It's a keeper!                                                  */
        /* ---------------------------------------------------------------- */
        target_span = span;
    }

    /* -------------------------------------------------------------------- */
    /*  No candidate span?  Leave.                                          */
    /* -------------------------------------------------------------------- */
    if ( !target_span )
        return -1;

    return target_span->data[ addr - target_span->s_addr ];
}

static load_file_fn      load_file;
static get_word_fn       get_word;
static get_name_fn       get_name;
static get_short_name_fn get_short_name;
static get_year_fn       get_year;
static get_alt_crc_fn    get_alt_crc;

/* ======================================================================== */
/*  GET_SDBD_WORD    -- Read an SDBD word from memory.                      */
/*                      Returns -1 if the memory isn't mapped, or is        */
/*                      bankswitched or writeable memory.                   */
/* ======================================================================== */
static int get_sdbd_word(uint16_t addr)
{
    int lo = get_word(addr    );
    int hi = get_word(addr + 1);

    if (lo < 0) return -1;
    if (hi < 0) return -1;

    return (lo & 0xFF) | ((hi << 8) & 0xFF00);
}


char name_buf[MAX_NAME];

/* ======================================================================== */
/*  MAIN             -- In The Beginning, there was MAIN, and C was without */
/*                      CONST and VOID, and Darkness was on the face of     */
/*                      the Programmer.                                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    uint32_t crc;
    int i, name_addr, decle, type;
    int         md_year = -1,          db_year = -1,    ri_year = -1;
    const char *md_name = NULL,       *db_name = NULL, *ri_name = NULL;
    const char *md_short_name = NULL, *db_short_name = NULL;
    game_metadata_t *db_meta;
    int         db_found = 0;

    if (argc != 2)
        usage();

    /* -------------------------------------------------------------------- */
    /*  Check the file type.                                                */
    /* -------------------------------------------------------------------- */
    type = file_type( argv[1] );

    switch ( type )
    {
        case FT_BINCFG:
            load_file      = bc_load_file;
            get_word       = bc_get_word;
            get_name       = loc_get_name;
            get_short_name = loc_get_short_name;
            get_year       = loc_get_year;
            get_alt_crc    = NULL;
            break;

        case FT_ICARTROM:
            load_file      = ic_load_file;
            get_word       = ic_get_word;
            get_name       = ic_get_name;
            get_short_name = ic_get_short_name;
            get_year       = ic_get_year;
            get_alt_crc    = NULL;
            break;

        case FT_LUIGI:
            load_file      = loc_load_file;
            get_word       = loc_get_word;
            get_name       = loc_get_name;
            get_short_name = loc_get_short_name;
            get_year       = loc_get_year;
            get_alt_crc    = loc_get_alt_crc;
            break;

        default:
            goto try_crc;
    }

    /* -------------------------------------------------------------------- */
    /*  Read in the game image.                                             */
    /* -------------------------------------------------------------------- */
    if ( load_file( argv[1] ) )
        goto try_crc;

    /* -------------------------------------------------------------------- */
    /*  See if it has any metadata.                                         */
    /* -------------------------------------------------------------------- */
    md_name       = get_name();
    md_short_name = get_short_name();
    md_year       = get_year();

    /* -------------------------------------------------------------------- */
    /*  If we didn't find some pieces of metadata, look in the ROM.         */
    /* -------------------------------------------------------------------- */
    if ((name_addr = get_sdbd_word(0x500A)) < 0)
    {
        if (debug) printf("no header\n");
        goto try_crc;
    }

    ri_year = 1900 + get_word(name_addr);

    if (ri_year < 1977 || ri_year > 2050)
    {
        if (debug) printf("bad year %d\n", ri_year);
        ri_year = -1;
        goto try_crc;
    }

    for (i = 0; i < MAX_NAME - 1; i++)
    {
        decle = get_word(name_addr + i + 1);

        name_buf[i] = decle;

        if (!decle)
            break;

        if (decle < 0x20 || decle > 0x7E || !isprint(decle))
            exit(0); /* not a viable name string */
    }
    name_buf[i] = 0;

    if (i < 2) /* too short? */
    {
        if (debug) printf("too short\n");
        goto try_crc;
    }

    ri_name = name_buf;

    /* -------------------------------------------------------------------- */
    /*  Try to get the file's CRC-32 value.                                 */
    /* -------------------------------------------------------------------- */
try_crc:
    crc = file_crc32(argv[1]);

    /* -------------------------------------------------------------------- */
    /*  Look it up in the database.                                         */
    /*  If we don't find it in the DB with the default CRC, but we have an  */
    /*  alternate way to get a CRC (e.g. from LUIGI's UID field), try that. */
    /* -------------------------------------------------------------------- */
    db_meta = default_game_metadata();
    game_metadata_set_compat_to_unspec( db_meta );
    db_found = find_cart_metadata(crc, db_meta);
    if (!db_found && get_alt_crc)
    {
        crc = get_alt_crc();
        db_found = find_cart_metadata(crc, db_meta);
    }

    if (db_found)
    {
        db_name = strdup(db_meta->name);
        db_short_name = strdup(db_meta->short_name);
        if (db_meta->release_dates)
            db_year = db_meta->release_dates[0].year;
    }

    free_game_metadata(db_meta);
    db_meta = NULL;

    /* -------------------------------------------------------------------- */
    /*  Exit if we found nothing at all.                                    */
    /* -------------------------------------------------------------------- */
    if ( !md_name && !db_name && !ri_name )
        exit(0);

    /* -------------------------------------------------------------------- */
    /*  Now print our best guess among all three name sources.              */
    /*  Priority:  Metadata > Database > ROM Image.                         */
    /* -------------------------------------------------------------------- */
    puts( cfg_escape_str( md_name ? md_name : db_name ? db_name : ri_name ) );
    printf("%d\n", md_year > 0 ? md_year : db_year > 0 ? db_year : ri_year );

    if ( md_short_name || db_short_name )
        puts( cfg_escape_str( md_short_name ? md_short_name : db_short_name ) );

    return 0;
}

/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License along */
/*  with this program; if not, write to the Free Software Foundation, Inc., */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             */
/* ======================================================================== */
/*                 Copyright (c) 2014-2014, Joseph Zbiciak                  */
/* ======================================================================== */
