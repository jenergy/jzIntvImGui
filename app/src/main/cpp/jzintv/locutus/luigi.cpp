// ======================================================================== //
//  LUIGI -- LTO Universal Intellivision Game Image                         //
// ======================================================================== //

#include "luigi.hpp"
#include "crc_calc.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <list>
#include <cassert>

typedef crc::kc::crc8_dowcrc t_crc_header;
typedef crc::cbh::crc32_4_r  t_crc_payload;

static const int LUIGI_BLKTYPE_SCRAMBLE  = 0x00;
static const int LUIGI_BLKTYPE_MEM_MAP   = 0x01;
static const int LUIGI_BLKTYPE_DATA_HUNK = 0x02;
static const int LUIGI_BLKTYPE_METADATA  = 0x03;
static const int LUIGI_BLKTYPE_EOF       = 0xFF;

using namespace std;

// ======================================================================== //
//  T_LUIGI_BLOCK    -- Encapsulates encoding/decoding LUIGI blocks         //
// ======================================================================== //
class t_luigi_block
{
    private:
        uint8_t     type;
        t_byte_vec  data;
    public:

        t_luigi_block( uint8_t type_ = 0xFF ) : type(type_) { }
        t_luigi_block( t_byte_vec::const_iterator& data,
                       t_byte_vec::const_iterator  end  );

        void set_data( const t_byte_vec& v )              { data = v;     }
        t_byte_vec&         raw_data()                    { return data;  }
        const t_byte_vec&   raw_data() const              { return data;  }
        uint8_t             get_type() const              { return type;  }
        void                set_type(const uint8_t type_) { type = type_; }
        void                encode( t_byte_vec& enc_data ) const;
};

// ------------------------------------------------------------------------ //
//  T_LUIGI_BLOCK CTOR                                                      //
//                                                                          //
//  Construct a t_luigi_block from a span in a file.  Updates the iterator  //
//  to point to the first byte after the span.  Throws an error string on   //
//  a decode error.                                                         //
// ------------------------------------------------------------------------ //
t_luigi_block::t_luigi_block( t_byte_vec::const_iterator& encoded,
                              t_byte_vec::const_iterator  end  )
{
    // EOF blocktype is not a full, proper block.  It's just one byte.
    // Everything after it gets *ignored*.
    if ( distance( encoded, end ) > 0 && encoded[0] == LUIGI_BLKTYPE_EOF )
    {
        type = LUIGI_BLKTYPE_EOF;
        encoded = end;
        return;
    }

    t_crc_header crc_header;

    if ( distance( encoded, end ) < 4 ) // must be at least 4 bytes long
        throw string("t_luigi_block:  Too little remaining encoded for header");

    // Check the header checksum
    crc_header.update( encoded[0] );
    crc_header.update( encoded[1] );
    crc_header.update( encoded[2] );

#ifndef FUZZ
    if ( encoded[3] != crc_header.get() )
    {
        char buf[10];
        sprintf(buf, "%.2X vs. %.2X", encoded[3], crc_header.get());
        throw string("t_luigi_block:  Checksum mismatch: ") + buf;
    }
#endif

    type = encoded[0];
    uint16_t length = encoded[1] | (encoded[2] << 8);

    encoded += 4;

    if (length == 0)
        return;

    if ( distance( encoded, end ) < length + 4 )
        throw string("t_luigi_block:  Too little encoded data remaining "
                     "for payload");

    uint32_t stored_crc = encoded[0]       | encoded[1] << 8 |
                          encoded[2] << 16 | encoded[3] << 24;

    encoded += 4;

    data.resize(length);

    const t_byte_vec::const_iterator payload_end = encoded + length;
    t_byte_vec::iterator data_i                  = data.begin();
    t_crc_payload crc_payload;

    while ( encoded != payload_end )
    {
        crc_payload.update( *data_i = *encoded );
        ++data_i;
        ++encoded;
    }

    // Ignore the CRC on a scramble block.
    if ( type == LUIGI_BLKTYPE_SCRAMBLE )
        return;

#ifndef FUZZ
    if ( crc_payload.get() != stored_crc )
    {
        char buf[24];
        sprintf(buf, "%.8X vs. %.8X", stored_crc, crc_payload.get());
        throw string("t_luigi_block:  Checksum mismatch: ") + buf;
    }
#endif
}

// ------------------------------------------------------------------------ //
//  T_LUIGI_BLOCK::ENCODE      -- Return the encoded buffer                 //
// ------------------------------------------------------------------------ //
void t_luigi_block::encode( t_byte_vec& enc_data ) const
{
    if (data.size() > 65535)
        throw string("t_luigi_block::encoded:  Hunk too big!");

    t_crc_header  crc_header;
    t_crc_payload crc_payload;
    uint16_t      length = data.size();

    enc_data.push_back( type          );    crc_header.update( type          );
    enc_data.push_back( length & 0xFF );    crc_header.update( length & 0xFF );
    enc_data.push_back( length >> 8   );    crc_header.update( length >> 8   );
    enc_data.push_back( crc_header.get() );

    if ( length == 0 )
        return;

    for ( t_byte_vec::const_iterator i = data.begin(); i != data.end(); ++i )
        crc_payload.update( *i );

    uint32_t crc_payload_val = crc_payload.get();

    enc_data.push_back( (crc_payload_val >>  0) & 0xFF );
    enc_data.push_back( (crc_payload_val >>  8) & 0xFF );
    enc_data.push_back( (crc_payload_val >> 16) & 0xFF );
    enc_data.push_back( (crc_payload_val >> 24) & 0xFF );

    enc_data.insert( enc_data.end(), data.begin(), data.end() );
}

// ======================================================================== //
//  T_LUIGI_DATA_HUNK           -- ROM data hunk with 8/10/16 bit data      //
// ======================================================================== //
class t_luigi_data_hunk
{
    private:
        uint32_t   hunk_addr;
        t_word_vec hunk_data;

        typedef enum { BLK08 = 8, BLK10 = 10, BLK16 = 16 } t_enc_type;

        inline int est_length( t_enc_type type, uint32_t len ) const
        {
            return           // raw payload cost          encoding overhead
                type == BLK08 ? len                    + (2*(1 + len / 0x3F))
              : type == BLK10 ? ((len + 2) >> 2) + len + (2*(1 + len / 0x80))
              :                 2*len                  + (   1 + len / 0x3E );
        }

        inline t_enc_type min_block_type( uint16_t word ) const
        {
            return word < 0x0100 ? BLK08
                 : word < 0x0400 ? BLK10
                 :                 BLK16;
        }

        typedef pair< t_enc_type, uint32_t > t_enc_span;
        typedef list< t_enc_span >           t_enc_plan;


    public:
        t_luigi_data_hunk( uint32_t addr = 0 ) : hunk_addr(addr) { }
        t_luigi_data_hunk( const t_luigi_block &block ) { decode(block);    }

        inline uint32_t     get_addr() const            { return hunk_addr; }
        inline void         set_addr(uint32_t addr)     { hunk_addr = addr; }

        t_word_vec&         raw_data()                  { return hunk_data; }
        const t_word_vec&   raw_data() const            { return hunk_data; }

        void decode( const t_luigi_block& block );
        void encode(       t_luigi_block& block ) const;

        void copy_to_locutus  ( t_locutus&       locutus ) const;
        void copy_from_locutus( const t_locutus& locutus,
                                const uint32_t addr,
                                const uint32_t len );
};

// ------------------------------------------------------------------------ //
//  T_LUIGI_DATA_HUNK::DECODE                                               //
//                                                                          //
//  Decode a t_luigi_block into this data_hunk, replacing what was there    //
//  previously.                                                             //
// ------------------------------------------------------------------------ //
void t_luigi_data_hunk::decode( const t_luigi_block &block )
{
    const t_byte_vec& enc_data = block.raw_data();

    if ( block.get_type() != LUIGI_BLKTYPE_DATA_HUNK )
        throw string("t_luigi_data_hunk: Attempt to decode wrong block type");

    if ( enc_data.size() < 3 )
        throw string("t_luigi_data_hunk:  Data block is too short");

    hunk_addr = enc_data[0] | (enc_data[1] << 8) | (enc_data[2] << 16);
    hunk_data.resize(0);

    if (hunk_addr >= 0x80000)
        throw string("t_luigi_data_hunk:  Address out of range (above 512K)");

    t_byte_vec::const_iterator cur_byte_i = enc_data.begin() + 3;
    t_byte_vec::const_iterator end_byte_i = enc_data.end();

    while (cur_byte_i != end_byte_i)
    {
        uint8_t block_type = *cur_byte_i;
        ++cur_byte_i;

        if ( block_type == 0x00 || block_type == 0xFE || block_type == 0xFF )
        {
            char buf[3];
            sprintf(buf, "%.2X", block_type);
            throw string("t_luigi_data_hunk:  Unexpected byte in input: ") +
                  buf;
        }

        // 8 bit hunk_data run
        if ( block_type >= 0x01 && block_type <= 0x3F )
        {
            int num_elem  = block_type;
            int num_bytes = num_elem + 1;

            if ( distance( cur_byte_i, end_byte_i ) < num_bytes )
                throw string("t_luigi_data_hunk:  "
                             "Too few bytes remaining for 8 bit run");

            // First N-1 elements are straight unsigned bytes
            while ( num_elem-- > 1 )
            {
                hunk_data.push_back( *cur_byte_i );
                ++cur_byte_i;
            }

            // Last element is always a 16-bit word, LSB-first
            hunk_data.push_back( cur_byte_i[0] | (cur_byte_i[1] << 8) );
            cur_byte_i += 2;

            continue;
        }

        // 10 bit hunk_data run
        if ( block_type >= 0x40 && block_type <= 0xBF )
        {
            int num_elem  = block_type - 0x3F;
            int num_bytes = ((num_elem + 2) >> 2) + num_elem + 1;

            if ( distance( cur_byte_i, end_byte_i ) < num_bytes )
                throw string("t_luigi_data_hunk:  "
                             "Too few bytes remaining for 10 bit run");

            // First N-1 elements are 10-bit values packed 2/8
            uint32_t uppers = 0x10000;

            while ( num_elem-- > 1 )
            {
                if ( uppers & 0x10000 )
                {
                    uppers = 0x0100 | *cur_byte_i;
                    ++cur_byte_i;
                }

                uppers <<= 2;

                hunk_data.push_back( (0x300 & uppers) | *cur_byte_i );

                ++cur_byte_i;
            }

            // Last element is always a 16-bit word, LSB first
            hunk_data.push_back( cur_byte_i[0] | (cur_byte_i[1] << 8) );
            cur_byte_i += 2;

            continue;
        }

        // 16 bit hunk_data run
        if ( block_type >= 0xC0 && block_type <= 0xFD )
        {
            int num_elem  = block_type - 0xBF;
            int num_bytes = num_elem * 2;

            if ( distance( cur_byte_i, end_byte_i ) < num_bytes )
                throw string("t_luigi_data_hunk:  "
                             "Too few bytes remaining for 16 bit run");

            while ( num_elem-- > 0 )
            {
                hunk_data.push_back( cur_byte_i[0] | (cur_byte_i[1] << 8) );
                cur_byte_i += 2;
            }
        }
    }
}

// ------------------------------------------------------------------------ //
//  T_LUIGI_DATA_HUNK::ENCODE    -- Encode a data hunk into a t_luigi_block //
// ------------------------------------------------------------------------ //
void t_luigi_data_hunk::encode( t_luigi_block &block ) const
{
    t_enc_plan enc_plan;

    // -------------------------------------------------------------------- //
    //  Bottoms up span construction:                                       //
    //                                                                      //
    //  1.  Mark each word with the narrowest block type that contains it.  //
    //                                                                      //
    //  2.  Create spans through straight run-length encoding output of     //
    //      the first pass.                                                 //
    //       -- Minor exception:  If the word that terminates a span is     //
    //          wider than the span, absorb it into the span in this step.  //
    //                                                                      //
    //  3.  Merge spans together if we estimate the merged block will be    //
    //      shorter than the two spans considered separately.  We use a     //
    //      crude estimate of encoding overhead due to maximum span length  //
    //      at this stage.                                                  //
    //                                                                      //
    //  4.  Repeat step 3 until plan converges.                             //
    //                                                                      //
    //  5.  Break "too long" spans into legal length spans.                 //
    //                                                                      //
    //  Due to the estimates in step 3, it's possible this will not give    //
    //  an optimal plan.  But, it should be pretty good, nonetheless.       //
    //                                                                      //
    //  (Technical note:  If you ensure est_length produces an accurate     //
    //  estimate, I think you could recast this as a dynamic programming    //
    //  problem and get a guaranteed optimal solution, as I think the       //
    //  problem then has the required property of optimal substructure.)    //
    // -------------------------------------------------------------------- //


    // -------------------------------------------------------------------- //
    //  Steps 1 & 2:  Initialize span list from simple RLE'd spans.         //
    // -------------------------------------------------------------------- //
    {
        t_enc_span curr_span( min_block_type( hunk_data[0] ), 0 );  // initial span
        t_word_vec::const_iterator curr_data_i = hunk_data.begin();

        while ( curr_data_i != hunk_data.end() )
        {
            t_enc_type blk_type = min_block_type( *curr_data_i );

            if ( blk_type == curr_span.first )
                curr_span.second++;         // extend current span if same type
            else
            {
                // If the current block is type 8 or 10, absorb exactly one
                // word from the next block if it's a *wider* type.  Otherwise,
                // just terminate current span, add it to the plan, and start
                // a new one.
                if ( curr_span.first < blk_type )
                {
                    curr_span.second++;
                    enc_plan.push_back( curr_span );
                    curr_span = t_enc_span( blk_type, 0 );
                } else
                {
                    if ( curr_span.second > 0 )
                        enc_plan.push_back( curr_span );
                    curr_span = t_enc_span( blk_type, 1 );
                }
            }

            ++curr_data_i;
        }

        // handle the straggler at the end...
        if ( curr_span.second )
            enc_plan.push_back( curr_span );
    }

    // -------------------------------------------------------------------- //
    //  Steps 3 & 4:  Coalesce spans if it improves encoding length.        //
    // -------------------------------------------------------------------- //
    {
        bool changed = true;

        while (changed)     // step 4:  loop while anything changes
        {
            // curr_data_i tracks where we are among the spans, so we can
            // examine the last element in each span to enforce merging rules.
            changed = false;

            t_enc_plan::iterator prev = enc_plan.begin();
            t_enc_plan::iterator curr = prev;
            ++curr;

            if ( curr == enc_plan.end() )
                break;  // only one span -- nothing more to do!

            t_word_vec::const_iterator curr_data_i = hunk_data.begin();
            curr_data_i += prev->second;

            t_enc_plan::iterator next = curr;
            ++next;

            while ( curr != enc_plan.end() )
            {
                uint16_t prev_last_data = curr_data_i[ -1 ];

                int prev_est_length = est_length( prev->first, prev->second );
                int curr_est_length = est_length( curr->first, curr->second );

                //  Decide whether to merge prev and curr, where prev is
                //  wider than curr.
                //
                //  Conditions:
                //   -- Prev is wider than or equal to curr
                //   -- Last word of prev fits within block width of prev
                //   -- est_length of combined spans is less than
                //      est_length(prev) plus est_length(curr)
                if ( prev->first >= curr->first &&
                     min_block_type( prev_last_data ) <= prev->first  &&
                     prev_est_length + curr_est_length >=
                        est_length( prev->first, prev->second + curr->second ) )
                {
                    prev->second += curr->second;
                    curr_data_i  += curr->second;
                    enc_plan.erase( curr, next );

                    curr = next;
                    if ( next != enc_plan.end() )
                        ++next;

                    changed = true;
                    continue;
                }

                //  Decide whether to merge prev and curr, where prev is
                //  narrower than curr.
                //
                //  Conditions:
                //   -- Prev is narrower than curr
                //   -- Last word of prev fits within block width of curr
                //   -- est_length of combined spans is less than
                //      est_length(prev) plus est_length(curr)
                if ( prev->first <= curr->first &&
                     min_block_type( prev_last_data ) <= curr->first  &&
                     prev_est_length + curr_est_length >
                        est_length( curr->first, prev->second + curr->second ) )
                {
                    prev->first   = curr->first;
                    prev->second += curr->second;
                    curr_data_i  += curr->second;
                    enc_plan.erase( curr, next );

                    curr = next;
                    if ( next != enc_plan.end() )
                        ++next;

                    changed = true;
                    continue;
                }

                // Move to next span, as this span didn't merge.
                curr_data_i += curr->second;
                prev = curr;
                curr = next;

                if ( next != enc_plan.end() )
                    next++;
            }
        }
    }

    // -------------------------------------------------------------------- //
    //  Step 5:  Split runs that are too long.                              //
    // -------------------------------------------------------------------- //
    for ( t_enc_plan::iterator curr = enc_plan.begin() ;
          curr != enc_plan.end() ; ++curr )
    {
        unsigned max_len = curr->first == BLK08 ? 63
                         : curr->first == BLK10 ? 128
                         :                        62;

        while ( curr->second > max_len )
        {
            t_enc_span split( curr->first, max_len );
            curr->second -= max_len;

            enc_plan.insert( curr, split );
        }
    }


    // -------------------------------------------------------------------- //
    //  Now that we have an encoding plan, encode the actual hunk_data      //
    //  into the t_luigi_block.                                             //
    // -------------------------------------------------------------------- //
    {
        t_byte_vec& enc_data = block.raw_data();

        if ( enc_data.size() != 0 )
        {
            throw string("t_luigi_data_hunk: adding hunk_data onto "
                         "non-empty t_luigi_block");
        }

        block.set_type( LUIGI_BLKTYPE_DATA_HUNK );

        // Encode the hunk address first
        enc_data.push_back( (hunk_addr >>  0) & 0xFF );
        enc_data.push_back( (hunk_addr >>  8) & 0xFF );
        enc_data.push_back( (hunk_addr >> 16) & 0xFF );

        // Then encode the data
        t_word_vec::const_iterator curr_data_i = hunk_data.begin();

        for ( t_enc_plan::const_iterator curr_span_i = enc_plan.begin();
               curr_span_i != enc_plan.end(); ++curr_span_i )
        {
            t_enc_type blk_type = curr_span_i->first;
            int        blk_len  = curr_span_i->second;

            switch ( blk_type )
            {
                case BLK08:
                {
                    uint16_t word;
                    enc_data.push_back( blk_len + 0x00 );

                    for ( int i = 0; i < blk_len - 1; i++ )
                    {
                        word = *curr_data_i;
                        ++curr_data_i;

                        if ( word > 0xFF )
                            throw string("t_luigi_data_hunk: non-byte "
                                         "hunk_data in byte span");

                        enc_data.push_back( word );
                    }

                    word = *curr_data_i;
                    ++curr_data_i;


                    enc_data.push_back( word & 0xFF );
                    enc_data.push_back( word >> 8   );
                    break;
                }

                case BLK10:
                {
                    enc_data.push_back( blk_len + 0x3F );

                    uint8_t uppers     = 0;
                    int     uppers_idx = 0;

                    for ( int i = 0; i < blk_len - 1; i++ )
                    {
                        if ( (i & 3) == 0 )
                        {
                            uppers_idx = enc_data.size();
                            uppers     = 0;
                            enc_data.push_back( 0 );
                        }

                        uint16_t word = *curr_data_i;
                        ++curr_data_i;

                        if ( word > 0x3FF )
                            throw string("t_luigi_data_hunk: non-decle "
                                         "hunk_data in decle span");

                        uppers |= (word & 0x300) >> (2 + 2*(i & 3));

                        enc_data.push_back( word & 0xFF );

                        if ( (i & 3) == 3 && uppers_idx > 2 )
                            enc_data[uppers_idx] = uppers;
                    }

                    if ( uppers_idx > 2 )
                        enc_data[uppers_idx] = uppers;

                    uint16_t word = *curr_data_i;
                    ++curr_data_i;

                    enc_data.push_back( word & 0xFF );
                    enc_data.push_back( word >> 8   );

                    break;
                }

                case BLK16:
                {
                    enc_data.push_back( blk_len + 0xBF );

                    for ( int i = 0; i < blk_len; i++ )
                    {
                        uint16_t word = *curr_data_i;
                        ++curr_data_i;

                        enc_data.push_back( word & 0xFF );
                        enc_data.push_back( word >>   8 );
                    }
                    break;
                }

                default:
                {
                    throw string("t_luigi_data_hunk: invalid span type");
                }
            }
        }
    }
}

// ------------------------------------------------------------------------ //
//  T_LUIGI_DATA_HUNK::COPY_TO_LOCUTUS                                      //
// ------------------------------------------------------------------------ //
void t_luigi_data_hunk::copy_to_locutus( t_locutus& locutus ) const
{
    uint32_t curr_addr = hunk_addr;
    t_word_vec::const_iterator hunk_data_i = hunk_data.begin();
    t_word_vec::const_iterator hunk_data_e = hunk_data.end();

    while ( hunk_data_i != hunk_data_e )
    {
        locutus.write( curr_addr, *hunk_data_i );
        ++hunk_data_i;
        ++curr_addr;
    }
}

// ------------------------------------------------------------------------ //
//  T_LUIGI_DATA_HUNK::COPY_FROM_LOCUTUS                                    //
// ------------------------------------------------------------------------ //
void t_luigi_data_hunk::copy_from_locutus( const t_locutus& locutus,
                                           const uint32_t addr,
                                           const uint32_t words )
{
    // for now, only support copying into empty hunks.
    if ( hunk_data.size() != 0 )
        throw string("t_luigi_data_hunk: copying into non-empty hunk");

    // Check length:  For now, limit to 32000 words, which should easily
    // fit in 64K bytes encoded.  Realistically, we should limit ourselves
    // to 4K word chunks to limit decoder complexity on Locutus.
    if ( words >= 0x80000 )
        throw string("t_luigi_data_hunk: hunk too big");

    // Check address range:
    if ( addr > 0x080000 || addr + words > 0x080000 )
        throw string("t_luigi_data_hunk: address span goes beyond 512K");

    hunk_addr = addr;
    hunk_data.resize( words );

    t_word_vec::iterator hunk_data_i = hunk_data.begin();

    for ( uint32_t curr_addr = addr; curr_addr != addr + words;
          ++curr_addr, ++hunk_data_i )
        *hunk_data_i = locutus.read( curr_addr );
}



// ======================================================================== //
//  T_LUIGI_MEM_MAP_TABLE       -- Memory mapping tables                    //
// ======================================================================== //
class t_luigi_mem_map_table
{
    private:
        uint16_t    mem_map [256];    // bits 7:0 added to bits 11:8 of addr
        t_perm      mem_perm[256];
        uint16_t    pfl_map [16][16]; // bits 11:0 form bits 19:12 of address
        t_perm      pfl_perm[16][16];

        void reset()
        {
            memset( static_cast<void *>(mem_map), 0, sizeof( mem_map ) );
            memset( static_cast<void *>(pfl_map), 0, sizeof( pfl_map ) );

            for (int i = 0; i < 256; i++)
                mem_perm[i].reset();

            for (int i = 0; i < 16; i++)
                for (int j = 0; j < 16; j++)
                    pfl_perm[i][j].reset();
        }

    public:
        t_luigi_mem_map_table() { reset(); }
        t_luigi_mem_map_table( const t_luigi_block &block ) { decode(block); }

        void decode( const t_luigi_block &block );
        void encode(       t_luigi_block &block ) const;

        inline uint16_t get_mem_map( const uint8_t para ) const
        {
            return mem_map[para];
        }

        inline void set_mem_map( const uint8_t  para,
                                 const uint16_t map )
        {
            mem_map[para] = map;
        }

        inline t_perm get_mem_perm( const uint8_t para ) const
        {
            return mem_perm[para];
        }

        inline void set_mem_perm( const uint8_t  para,
                                  const t_perm   perm )
        {
            mem_perm[para] = perm;
        }

        inline uint16_t get_pageflip_map( const uint8_t chap,
                                          const uint8_t page ) const
        {
            return pfl_map[chap][page];
        }

        inline void set_pageflip_map( const uint8_t  chap,
                                      const uint8_t  page,
                                      const uint16_t map )
        {
            pfl_map[chap][page] = map;
        }

        inline t_perm get_pageflip_perm( const uint8_t chap,
                                         const uint8_t page ) const
        {
            return pfl_perm[chap][page];
        }

        inline void set_pageflip_perm( const uint8_t  chap,
                                       const uint8_t  page,
                                       const t_perm   perm )
        {
            pfl_perm[chap][page] = perm;
        }

        inline void set_pageflip     ( const uint8_t  chap,
                                       const uint8_t  page,
                                       const uint16_t map,
                                       const t_perm   perm )
        {
            set_pageflip_map ( chap, page, map  );
            set_pageflip_perm( chap, page, perm );
        }

        void copy_to_locutus  (       t_locutus& locutus ) const;
        void copy_from_locutus( const t_locutus& locutus );
};


// ------------------------------------------------------------------------ //
//  T_LUIGI_MEM_MAP_TABLE::DECODE                                           //
// ------------------------------------------------------------------------ //
void t_luigi_mem_map_table::decode( const t_luigi_block &block )
{
    if ( block.get_type() != LUIGI_BLKTYPE_MEM_MAP )
        throw string("t_luigi_mem_map_table: attempting to decode block "
                     "of wrong type");

    const t_byte_vec&          enc_data   = block.raw_data();
    t_byte_vec::const_iterator enc_data_i = enc_data.begin();

    if ( enc_data.size() != 1280 )
    {
        char buf[11];
        sprintf( buf, "%u",  unsigned(enc_data.size()) );
        throw string("t_luigi_mem_map_table: unexpected payload size ") + buf;
    }

    // -------------------------------------------------------------------- //
    //  Initial Memory Mapping table (512 bytes)                            //
    // -------------------------------------------------------------------- //
    for (int i = 0; i < 256; ++i, ++enc_data_i)
    {
        uint8_t b0 = *  enc_data_i;
        uint8_t b1 = *++enc_data_i;
        uint16_t w = b0 | (b1 << 8);

        if (w >= 0x800)
            throw string("t_luigi_mem_map_table: memory map "
                         "address beyond 512K");

        mem_map[i] = w;
    }

    // -------------------------------------------------------------------- //
    //  Initial Permissions table (256 bytes)                               //
    // -------------------------------------------------------------------- //
    for (int i = 0; i < 256; ++i, ++enc_data_i)
    {
        uint8_t perm_byte = *enc_data_i;
        t_perm  perm;

        if (perm_byte & 0xF0)
        {
            char buf[4];
            sprintf( buf, "%.2X", unsigned(perm_byte) );
            throw string("t_luigi_mem_map_table: invalid perm flags ") + buf;
        }

        perm[0] = perm_byte & (1 << 0);
        perm[1] = perm_byte & (1 << 1);
        perm[2] = perm_byte & (1 << 2);
        perm[3] = perm_byte & (1 << 3);

        mem_perm[i] = perm;
    }

    // -------------------------------------------------------------------- //
    //  Page Flip Table (512 bytes)                                         //
    // -------------------------------------------------------------------- //
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j, ++enc_data_i)
        {
            uint8_t b0 = *  enc_data_i;
            uint8_t b1 = *++enc_data_i;
            uint16_t w = b0 | (b1 << 8);
            uint16_t m = (w & 0xFFF0) >> 4; // memory map
            uint8_t  p =  w & 0x000F;       // permissions

            if (m >= 0x800)
                throw string("t_luigi_mem_map_table: memory map "
                             "address beyond 512K");

            pfl_map [i][j] = m;
            pfl_perm[i][j] = t_perm( p );
        }

    return;
}

// ------------------------------------------------------------------------ //
//  T_LUIGI_MEM_MAP_TABLE::ENCODE                                           //
// ------------------------------------------------------------------------ //
void t_luigi_mem_map_table::encode( t_luigi_block &block ) const
{
    t_byte_vec& enc_data = block.raw_data();

    if ( enc_data.size() != 0 )
        throw string("t_luigi_mem_map_table: adding data onto "
                     "non-empty t_luigi_block");

    block.set_type( LUIGI_BLKTYPE_MEM_MAP );

    enc_data.resize( 1280 );

    t_byte_vec::iterator enc_data_i = enc_data.begin();

    // -------------------------------------------------------------------- //
    //  Initial Memory Mapping table (512 bytes)                            //
    // -------------------------------------------------------------------- //
    for (int i = 0; i < 256; ++i, ++enc_data_i)
    {
        uint16_t w = mem_map[i];

        if ( w >= 0x800 )
        {
            char buf[60];
            sprintf( buf, "para %.2X => %.4X", i, w );
            throw string("t_luigi_mem_map_table: "
                         "mem map out of 512K range (a)  ") + buf;
        }

        *  enc_data_i = w & 0xFF;
        *++enc_data_i = w >> 8;
    }

    // -------------------------------------------------------------------- //
    //  Initial Permissions table (256 bytes)                               //
    // -------------------------------------------------------------------- //
    for (int i = 0; i < 256; ++i, ++enc_data_i)
        *enc_data_i = mem_perm[i].to_ulong();

    // -------------------------------------------------------------------- //
    //  Page Flip Table (512 bytes)                                         //
    // -------------------------------------------------------------------- //
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j, ++enc_data_i)
        {
            uint16_t m = pfl_map[i][j];

            if ( m > 0x800 )
            {
                char buf[60];
                sprintf( buf, "chap %.X page %.X => %.4X", i, j, m );
                throw string("t_luigi_mem_map_table: "
                             "mem map out of 512K range (b)  ") + buf;
            }

            uint16_t w = (0xFFF0 & (m << 4))
                       | (0x000F & pfl_perm[i][j].to_ulong());

            *  enc_data_i = w & 0xFF;
            *++enc_data_i = w >> 8;
        }

    return;
}

// ------------------------------------------------------------------------ //
//  T_LUIGI_MEM_MAP_TABLE::COPY_TO_LOCUTUS                                  //
// ------------------------------------------------------------------------ //
void t_luigi_mem_map_table::copy_to_locutus( t_locutus& locutus ) const
{
    for ( uint16_t para = 0x00; para < 0x100; ++para )
    {
        uint16_t mapping = get_mem_map( para );
        locutus.set_mem_map( para, false, mapping );
        locutus.set_mem_map( para, true,  mapping );
    }

    for ( uint16_t para = 0x00; para < 0x100; ++para )
    {
        t_perm   permissions = get_mem_perm( para );
        locutus.set_mem_perm( para, false, permissions );
        locutus.set_mem_perm( para, true,  permissions );
    }

    for ( uint8_t chap = 0; chap < 16; chap++ )
        for ( uint8_t page = 0; page < 16; page++ )
            locutus.set_pageflip_map( chap, page,
                                      get_pageflip_map( chap, page ) );

    for ( uint8_t chap = 0; chap < 16; chap++ )
        for ( uint8_t page = 0; page < 16; page++ )
            locutus.set_pageflip_perm( chap, page,
                                      get_pageflip_perm( chap, page ) );
}

// ------------------------------------------------------------------------ //
//  T_LUIGI_MEM_MAP_TABLE::COPY_FROM_LOCUTUS                                //
// ------------------------------------------------------------------------ //
void t_luigi_mem_map_table::copy_from_locutus( const t_locutus& locutus )
{
    for ( uint16_t para = 0x00; para < 0x100; ++para )
    {
        uint16_t mapping = locutus.get_mem_map( para, true ); // "reset" map
        set_mem_map( para, mapping );
    }

    for ( uint16_t para = 0x00; para < 0x100; ++para )
    {
        t_perm   permissions = locutus.get_mem_perm( para, true );
        set_mem_perm( para, permissions );
    }

    for ( uint8_t chap = 0; chap < 16; chap++ )
        for ( uint8_t page = 0; page < 16; page++ )
            set_pageflip_map( chap, page,
                              locutus.get_pageflip_map( chap, page ) );

    for ( uint8_t chap = 0; chap < 16; chap++ )
        for ( uint8_t page = 0; page < 16; page++ )
            set_pageflip_perm( chap, page,
                               locutus.get_pageflip_perm( chap, page ) );
}


// ======================================================================== //
//  T_LUIGI_SCRAMBLER           -- Scramble key setting -- not implemented  //
// ======================================================================== //
class t_luigi_scrambler
{
    private:
        uint8_t     druid[16];

    public:
        t_luigi_scrambler() : druid() { }
        t_luigi_scrambler( const t_luigi_block &block ) { decode(block); }

        void decode( const t_luigi_block& block )
        {
            const auto& data = block.raw_data();
            if ( data.size() < 16 )
            {
                throw string("t_luigi:  scrambler data too short!");
            }
            std::copy( std::begin( data ), std::begin( data ) + 16,
                       std::begin( druid ) );
        };

        void copy_to_locutus( t_locutus& locutus ) const
        {
            locutus.set_scramble_druid( true, druid );
        }
};


// ======================================================================== //
//  T_LUIGI::DESERIALIZE                                                    //
// ======================================================================== //

void t_luigi::deserialize( t_locutus &locutus, const t_byte_vec &data )
{
    if ( data.size() < 16 )
        throw string("t_luigi:  data too short!");

    // -------------------------------------------------------------------- //
    //  Check magic number.  Currently only support version 0 & 1 files.    //
    // -------------------------------------------------------------------- //
    const int ver = data[3];

    if ( data[0] != 'L' || data[1] != 'T' || data[2] != 'O' || ver > 1 )
        throw string("t_luigi:  invalid magic number");

    // -------------------------------------------------------------------- //
    //  Set parameters based on file version.                               //
    // -------------------------------------------------------------------- //
    const int hdr_len  = ver ? 32  : 16;
    const int num_feat = ver ? 128 : 64;
    const int max_feat = 128;

    // -------------------------------------------------------------------- //
    //  Check header checksum                                               //
    // -------------------------------------------------------------------- //
    uint8_t      crc_header_stored   = data[ hdr_len - 1 ];
    t_crc_header crc_header_computed;

    for (int i = 0; i < hdr_len - 1; i++)
        crc_header_computed.update( data[i] );

#ifndef FUZZ
    if ( crc_header_stored != crc_header_computed.get() )
    {
        char buf[10];
        sprintf(buf, "%.2X vs. %.2X", crc_header_stored,
                                      crc_header_computed.get());
        throw string("t_luigi:  header CRC mismatch: ") + buf;
    }
#endif

    // -------------------------------------------------------------------- //
    //  Populate feature flags.                                             //
    // -------------------------------------------------------------------- //
    for (int feat_num = 0; feat_num < num_feat; feat_num++)
    {
        const int  feat_idx = feat_num >> 3;
        const int  feat_bit = feat_num & 7;
        const bool feat_val = (data[feat_idx + 4] >> feat_bit) & 1;

        locutus.set_feature_flag( feat_num, feat_val );
    }

    for (int feat_num = num_feat; feat_num < max_feat; feat_num++)
        locutus.set_feature_flag( feat_num, false );

    locutus.copy_feature_flag_to_metadata();

    // -------------------------------------------------------------------- //
    //  If this is a version 1 file, extract the UID from the file.         //
    // -------------------------------------------------------------------- //
    uint64_t uid = 0;
    if ( ver )
    {
        uint32_t u0, u1;
        u0 = data[20] | (data[21]<<8) | (data[22]<<16) | (data[23]<<24);
        u1 = data[24] | (data[25]<<8) | (data[26]<<16) | (data[27]<<24);

        uid = u0 | (uint64_t( u1 ) << 32);
    }
    locutus.set_uid( uid );

    // -------------------------------------------------------------------- //
    //  Now start processing blocks of data, turning them into something.   //
    // -------------------------------------------------------------------- //
    t_byte_vec::const_iterator data_i = data.begin() + hdr_len;
    t_byte_vec::const_iterator data_e = data.end();

    while (data_i != data_e)
    {
        // Pull off the next block
        t_luigi_block block( data_i, data_e );      // Advances data_i. Ugly.

        // Apply it to Locutus
        switch ( block.get_type() )
        {
            case LUIGI_BLKTYPE_SCRAMBLE:
            {
                // Obtain some limited information from the scramble block.
                t_luigi_scrambler scrambler( block );
                scrambler.copy_to_locutus( locutus );
                data_i = data_e;                    // Stop parsing here.
                break;
            }

            case LUIGI_BLKTYPE_MEM_MAP:
            {
                t_luigi_mem_map_table mem_map_table( block );
                mem_map_table.copy_to_locutus( locutus );
                break;
            }

            case LUIGI_BLKTYPE_DATA_HUNK:
            {
                t_luigi_data_hunk data_hunk( block );
                data_hunk.copy_to_locutus( locutus );
                break;
            }

            case LUIGI_BLKTYPE_METADATA:
            {
                t_metadata& metadata = locutus.get_metadata();
                metadata.deserialize( block.raw_data() );
                break;
            }

            case LUIGI_BLKTYPE_EOF:
            {
                // Empty block; ignore
                break;
            }

            default:
            {
                char buf[4];
                sprintf( buf, "%.2X", block.get_type() );
                throw string("t_luigi: unknown block type ") + buf;
            }
        }
    }
}

// ======================================================================== //
//  T_LUIGI::SERIALIZE                                                      //
// ======================================================================== //
t_byte_vec t_luigi::serialize( const t_locutus& locutus )
{
    //  create a header
    //  copy out the feature flags into the header
    //  create a mem map table and copy the mem map into that
    //  serialize the mem map
    //  scan the memory space for initialized spans
    //   -- merge spans together that have 15 or fewer uninitialized
    //      words between them (optional?)
    //  create data_hunks for each island of initialized data
    //  serialize the data_hunks
    //  call it a day!

    t_byte_vec enc_data;

    // -------------------------------------------------------------------- //
    //  Encode the header.  For now, only make Ver 1 headers.               //
    // -------------------------------------------------------------------- //
    enc_data.push_back('L');
    enc_data.push_back('T');
    enc_data.push_back('O');
    enc_data.push_back( 1 );

    for (int i = 0; i < 128; i += 8)
    {
        uint8_t flags = uint8_t( locutus.get_feature_flag( i + 0 ) ) << 0
                      | uint8_t( locutus.get_feature_flag( i + 1 ) ) << 1
                      | uint8_t( locutus.get_feature_flag( i + 2 ) ) << 2
                      | uint8_t( locutus.get_feature_flag( i + 3 ) ) << 3
                      | uint8_t( locutus.get_feature_flag( i + 4 ) ) << 4
                      | uint8_t( locutus.get_feature_flag( i + 5 ) ) << 5
                      | uint8_t( locutus.get_feature_flag( i + 6 ) ) << 6
                      | uint8_t( locutus.get_feature_flag( i + 7 ) ) << 7;

        enc_data.push_back( flags );
    }

    const uint64_t uid = locutus.get_uid();

    enc_data.push_back( ( uid >>  0 ) & 0xFF );
    enc_data.push_back( ( uid >>  8 ) & 0xFF );
    enc_data.push_back( ( uid >> 16 ) & 0xFF );
    enc_data.push_back( ( uid >> 24 ) & 0xFF );
    enc_data.push_back( ( uid >> 32 ) & 0xFF );
    enc_data.push_back( ( uid >> 40 ) & 0xFF );
    enc_data.push_back( ( uid >> 48 ) & 0xFF );
    enc_data.push_back( ( uid >> 56 ) & 0xFF );

    enc_data.push_back( 0 );
    enc_data.push_back( 0 );
    enc_data.push_back( 0 );

    t_crc_header crc_header;

    for ( t_byte_vec::const_iterator enc_data_i = enc_data.begin() ;
          enc_data_i != enc_data.end() ; ++enc_data_i )
        crc_header.update( *enc_data_i );

    enc_data.push_back( crc_header.get() );

    // -------------------------------------------------------------------- //
    //  Encode the metadata, if we have any.                                //
    // -------------------------------------------------------------------- //
    const t_metadata& metadata = locutus.get_metadata();
    if ( !metadata.empty() )
    {
        t_luigi_block metadata_block( LUIGI_BLKTYPE_METADATA );
        metadata_block.set_data( metadata.serialize() );
        metadata_block.encode( enc_data );
    }

    // -------------------------------------------------------------------- //
    //  Encode the memory map tables                                        //
    // -------------------------------------------------------------------- //
    t_luigi_mem_map_table mem_map_table;
    t_luigi_block         mem_map_table_block( LUIGI_BLKTYPE_MEM_MAP );

    mem_map_table.copy_from_locutus( locutus );
    mem_map_table.encode( mem_map_table_block );
    mem_map_table_block.encode( enc_data );

    // -------------------------------------------------------------------- //
    //  Encode the data hunks                                               //
    // -------------------------------------------------------------------- //
    t_addr_list addr_list = locutus.get_initialized_span_list();

    for ( t_addr_list::const_iterator addr_list_i = addr_list.begin();
          addr_list_i != addr_list.end(); ++addr_list_i )
    {
        uint32_t s_addr = addr_list_i->first,
                 e_addr = addr_list_i->second,
                 len    = e_addr - s_addr + 1;

        // Break up large hunks into max 8K hunks
        while ( len > 0 )
        {
            uint32_t hunk_len = len > 8192 ? 8192 : len;

            t_luigi_data_hunk data_hunk( s_addr );
            t_luigi_block     data_hunk_block( LUIGI_BLKTYPE_DATA_HUNK );

            data_hunk.copy_from_locutus( locutus, s_addr, hunk_len );
            data_hunk.encode( data_hunk_block );
            data_hunk_block.encode( enc_data );

            len    -= hunk_len;
            s_addr += hunk_len;
        }
        assert( s_addr - 1 == e_addr );
    }

    // -------------------------------------------------------------------- //
    //  Add an EOF byte at the end.                                         //
    // -------------------------------------------------------------------- //
    enc_data.push_back( LUIGI_BLKTYPE_EOF );

    return enc_data;
}
