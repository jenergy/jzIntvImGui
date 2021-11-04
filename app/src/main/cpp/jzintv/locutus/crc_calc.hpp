// ======================================================================== //
//  CRC computation class template                                          //
//                                                                          //
//  ZIP CRC:                                                                //
//      crc_calc<0xEDB88320, 32, CRC_RIGHT_SHIFTING>                        //
//       -- initialize to ~0U                                               //
//       -- invert at end                                                   //
//                                                                          //
//  Intellicart / CC3 CRC:                                                  //
//      crc_calc<0x1021, 16, CRC_LEFT_SHIFTING>                             //
//       -- initialize to 0                                                 //
//       -- no inversion at end                                             //
//                                                                          //
//  JLP CRC (CRC-24/6.2 from Castagnoli, Brauer, and Herrmann):             //
//      crc_calc<0xBD80DE, 24, CRC_RIGHT_SHIFTING>                          //
//       -- initialize to 0                                                 //
//       -- no inversion at end                                             //
//                                                                          //
// ======================================================================== //
#ifndef CRC_HPP_
#define CRC_HPP_

#include <stdint.h>

enum crc_direction { CRC_LEFT_SHIFTING, CRC_RIGHT_SHIFTING };

// Dummy version -- can't be instantiated
template <uint32_t POLY, uint32_t FIELD, crc_direction DIR>
class crc_calc { private: crc_calc() { }; ~crc_calc() { }; };



// Left-shifting CRCs, left-justified in internal accumulator
template <uint32_t POLY, uint32_t FIELD>
class crc_calc<POLY, FIELD, CRC_LEFT_SHIFTING>
{
    private:
        uint32_t crc_val;

        template <uint32_t v0>
            struct crc_ls
            {
                static const uint32_t POLY_ = POLY << (32-FIELD);
                static const uint32_t
                    v1 = (((v0>>31) & 1) != 0 ? (v0 << 1) ^ POLY_ : (v0 << 1)),
                    v2 = (((v1>>31) & 1) != 0 ? (v1 << 1) ^ POLY_ : (v1 << 1)),
                    v3 = (((v2>>31) & 1) != 0 ? (v2 << 1) ^ POLY_ : (v2 << 1)),
                    v4 = (((v3>>31) & 1) != 0 ? (v3 << 1) ^ POLY_ : (v3 << 1)),
                    v5 = (((v4>>31) & 1) != 0 ? (v4 << 1) ^ POLY_ : (v4 << 1)),
                    v6 = (((v5>>31) & 1) != 0 ? (v5 << 1) ^ POLY_ : (v5 << 1)),
                    v7 = (((v6>>31) & 1) != 0 ? (v6 << 1) ^ POLY_ : (v6 << 1)),
                    v8 = (((v7>>31) & 1) != 0 ? (v7 << 1) ^ POLY_ : (v7 << 1));
            };

        static const uint32_t lu[256];

    public:
        crc_calc(uint32_t init_crc = 0)
            : crc_val(init_crc << (32 - FIELD)) { } ;
        ~crc_calc() { };

        void update(uint32_t data, int bits = 8)
        {
            int i;

            data   <<= 32 - bits;
            crc_val ^= data;

            while (bits >= 8)
            {
                crc_val = (crc_val << 8) ^ lu[(crc_val >> 24)];
                bits -= 8;
            }

            if (bits > 0)
                for (i = 0; i < bits; i++)
                {
                    bool xor_poly = (crc_val & 0x80000000) != 0;

                    crc_val <<= 1;

                    if (xor_poly)
                        crc_val ^= (POLY << (32 - FIELD));
                }
        }

        void block(const uint8_t *data, uint32_t count)
        {
            uint32_t lcrc = crc_val;

            while (count > 4)
            {
                lcrc ^= data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
                lcrc  = (lcrc << 8) ^ lu[lcrc >> 24];
                lcrc  = (lcrc << 8) ^ lu[lcrc >> 24];
                lcrc  = (lcrc << 8) ^ lu[lcrc >> 24];
                lcrc  = (lcrc << 8) ^ lu[lcrc >> 24];
                count -= 4;
                data  += 4;
            }

            while (count-- != 0)
                lcrc = (lcrc << 8) ^ lu[(lcrc >> 24) ^ *data++];

            crc_val = lcrc;
        }

        template<typename T>
        void block(const T *data, uint32_t count, int bits_per_elem = 8)
        {
            while (count-- != 0)
            {
                update(static_cast<uint32_t>(*data), bits_per_elem);
                ++data;
            }
        }

        void set(uint32_t new_crc)
        {
            crc_val = new_crc << (32 - FIELD);
        }

        crc_calc& operator=(const crc_calc& c)
        {
            crc_val = c.get() << (32 - FIELD);
            return *this;
        }

        crc_calc& operator=(const uint32_t new_crc)
        {
            crc_val = new_crc << (32 - FIELD);
            return *this;
        }

        uint32_t get(void)      const { return crc_val >> (32 - FIELD); }
        operator unsigned int() const { return crc_val >> (32 - FIELD); }

        void invert(void)
        {
            crc_val ^= (~0U) << (32 - FIELD);
        }
};

#define CRC_LS_16_(B) \
    crc_ls<0x00000000U + ((B) << 28)>::v8, \
    crc_ls<0x01000000U + ((B) << 28)>::v8, \
    crc_ls<0x02000000U + ((B) << 28)>::v8, \
    crc_ls<0x03000000U + ((B) << 28)>::v8, \
    crc_ls<0x04000000U + ((B) << 28)>::v8, \
    crc_ls<0x05000000U + ((B) << 28)>::v8, \
    crc_ls<0x06000000U + ((B) << 28)>::v8, \
    crc_ls<0x07000000U + ((B) << 28)>::v8, \
    crc_ls<0x08000000U + ((B) << 28)>::v8, \
    crc_ls<0x09000000U + ((B) << 28)>::v8, \
    crc_ls<0x0A000000U + ((B) << 28)>::v8, \
    crc_ls<0x0B000000U + ((B) << 28)>::v8, \
    crc_ls<0x0C000000U + ((B) << 28)>::v8, \
    crc_ls<0x0D000000U + ((B) << 28)>::v8, \
    crc_ls<0x0E000000U + ((B) << 28)>::v8, \
    crc_ls<0x0F000000U + ((B) << 28)>::v8

template <uint32_t POLY, uint32_t FIELD>
const uint32_t crc_calc<POLY, FIELD, CRC_LEFT_SHIFTING>::lu[256] =
{
    CRC_LS_16_(0x0U),
    CRC_LS_16_(0x1U),
    CRC_LS_16_(0x2U),
    CRC_LS_16_(0x3U),
    CRC_LS_16_(0x4U),
    CRC_LS_16_(0x5U),
    CRC_LS_16_(0x6U),
    CRC_LS_16_(0x7U),
    CRC_LS_16_(0x8U),
    CRC_LS_16_(0x9U),
    CRC_LS_16_(0xAU),
    CRC_LS_16_(0xBU),
    CRC_LS_16_(0xCU),
    CRC_LS_16_(0xDU),
    CRC_LS_16_(0xEU),
    CRC_LS_16_(0xFU)
};

#undef CRC_LS_16_


// Right-shifting CRCs, right-justified in internal accumulator
template <uint32_t POLY, uint32_t FIELD>
class crc_calc<POLY, FIELD, CRC_RIGHT_SHIFTING>
{
    private:
        uint32_t crc_val;

        template <uint32_t v0>
            struct crc_rs
            {
                static const uint32_t
                    v1 = ((v0 & 1) != 0 ? (v0 >> 1) ^ POLY : (v0 >> 1)),
                    v2 = ((v1 & 1) != 0 ? (v1 >> 1) ^ POLY : (v1 >> 1)),
                    v3 = ((v2 & 1) != 0 ? (v2 >> 1) ^ POLY : (v2 >> 1)),
                    v4 = ((v3 & 1) != 0 ? (v3 >> 1) ^ POLY : (v3 >> 1)),
                    v5 = ((v4 & 1) != 0 ? (v4 >> 1) ^ POLY : (v4 >> 1)),
                    v6 = ((v5 & 1) != 0 ? (v5 >> 1) ^ POLY : (v5 >> 1)),
                    v7 = ((v6 & 1) != 0 ? (v6 >> 1) ^ POLY : (v6 >> 1)),
                    v8 = ((v7 & 1) != 0 ? (v7 >> 1) ^ POLY : (v7 >> 1));
            };

        static const uint32_t lu[256];

    public:
        crc_calc(uint32_t init_crc = 0) : crc_val(init_crc) { } ;
        ~crc_calc() { };

        void update(uint32_t data, int bits = 8)
        {
            int i;

            crc_val ^= data;

            while (bits >= 8)
            {
                crc_val = (crc_val >> 8) ^ lu[(crc_val & 0xFF)];
                bits -= 8;
            }

            if (bits > 0)
                for (i = 0; i < bits; i++)
                {
                    bool xor_poly = (crc_val & 1) != 0;

                    crc_val >>= 1;

                    if (xor_poly)
                        crc_val ^= POLY;
                }
        }

        void block(const uint8_t *data, uint32_t count)
        {
            uint32_t lcrc = crc_val;

            while (count > 4)
            {
                lcrc ^= data[3]<<24 | data[2]<<16 | data[1]<<8 | data[0];
                lcrc  = (lcrc >> 8) ^ lu[0xFF & lcrc];
                lcrc  = (lcrc >> 8) ^ lu[0xFF & lcrc];
                lcrc  = (lcrc >> 8) ^ lu[0xFF & lcrc];
                lcrc  = (lcrc >> 8) ^ lu[0xFF & lcrc];
                count -= 4;
                data  += 4;
            }

            while (count-- != 0)
                lcrc = (lcrc >> 8) ^ lu[(0xFF & lcrc) ^ *data++];

            crc_val = lcrc;
        }

        template<typename T>
        void block(const T *data, uint32_t count, int bits_per_elem = 8)
        {
            while (count-- != 0)
            {
                update(static_cast<uint32_t>(*data), bits_per_elem);
                ++data;
            }
        }

        void set(uint32_t new_crc)
        {
            crc_val = new_crc;
        }

        crc_calc& operator=(const crc_calc& c)
        {
            crc_val = c.get();
            return *this;
        }

        crc_calc& operator=(const uint32_t new_crc)
        {
            crc_val = new_crc;
            return *this;
        }

        uint32_t get(void) const        { return crc_val; }
        operator unsigned int() const   { return crc_val; }

        void invert(void)
        {
            crc_val ^= (~0U) >> (32 - FIELD);
        }
};

#define CRC_RS_16_(B) \
    crc_rs<0x00000000 + ((B) << 4)>::v8, \
    crc_rs<0x00000001 + ((B) << 4)>::v8, \
    crc_rs<0x00000002 + ((B) << 4)>::v8, \
    crc_rs<0x00000003 + ((B) << 4)>::v8, \
    crc_rs<0x00000004 + ((B) << 4)>::v8, \
    crc_rs<0x00000005 + ((B) << 4)>::v8, \
    crc_rs<0x00000006 + ((B) << 4)>::v8, \
    crc_rs<0x00000007 + ((B) << 4)>::v8, \
    crc_rs<0x00000008 + ((B) << 4)>::v8, \
    crc_rs<0x00000009 + ((B) << 4)>::v8, \
    crc_rs<0x0000000A + ((B) << 4)>::v8, \
    crc_rs<0x0000000B + ((B) << 4)>::v8, \
    crc_rs<0x0000000C + ((B) << 4)>::v8, \
    crc_rs<0x0000000D + ((B) << 4)>::v8, \
    crc_rs<0x0000000E + ((B) << 4)>::v8, \
    crc_rs<0x0000000F + ((B) << 4)>::v8

template <uint32_t POLY, uint32_t FIELD>
const uint32_t crc_calc<POLY, FIELD, CRC_RIGHT_SHIFTING>::lu[256] =
{
    CRC_RS_16_(0x0),
    CRC_RS_16_(0x1),
    CRC_RS_16_(0x2),
    CRC_RS_16_(0x3),
    CRC_RS_16_(0x4),
    CRC_RS_16_(0x5),
    CRC_RS_16_(0x6),
    CRC_RS_16_(0x7),
    CRC_RS_16_(0x8),
    CRC_RS_16_(0x9),
    CRC_RS_16_(0xA),
    CRC_RS_16_(0xB),
    CRC_RS_16_(0xC),
    CRC_RS_16_(0xD),
    CRC_RS_16_(0xE),
    CRC_RS_16_(0xF)
};

#undef CRC_RS_16_

namespace crc
{
    // Some common polynomials (for me, at least)
    typedef crc_calc<0xEDB88320, 32, CRC_RIGHT_SHIFTING> zip;
    typedef crc_calc<0x00001021, 16, CRC_LEFT_SHIFTING>  icart;
    typedef crc_calc<0x0000AD52, 16, CRC_RIGHT_SHIFTING> jlp16;

    // These polynomials come from the paper "Optimization of
    // Cyclic Redundancy Check Codes with 24 and 32 Parity Bits"
    // by Castagnoli, Brauer, and Hermann.
    //
    // Most of these codes do not produce a complete field; however
    // some have superior error detection/correction characteristics
    // on short blocks.  See the paper for details.
    namespace cbh
    {
        // 24-bit fields
        typedef crc_calc<0x005D6DCB, 24, CRC_LEFT_SHIFTING>  crc24_6_1_l;
        typedef crc_calc<0x00D3B6BA, 24, CRC_RIGHT_SHIFTING> crc24_6_1_r;

        typedef crc_calc<0x007B01BD, 24, CRC_LEFT_SHIFTING>  crc24_6_2_l;
        typedef crc_calc<0x00BD80DE, 24, CRC_RIGHT_SHIFTING> crc24_6_2_r;

        typedef crc_calc<0x00323009, 24, CRC_LEFT_SHIFTING>  mp_crc24_6_1_l;
        typedef crc_calc<0x00900C4C, 24, CRC_RIGHT_SHIFTING> mp_crc24_6_1_r;

        typedef crc_calc<0x00401607, 24, CRC_LEFT_SHIFTING>  mp_crc24_6_2_l;
        typedef crc_calc<0x00E06802, 24, CRC_RIGHT_SHIFTING> mp_crc24_6_2_r;

        typedef crc_calc<0x0031FF19, 24, CRC_LEFT_SHIFTING>  crc24_5_1_l;
        typedef crc_calc<0x0098FF8C, 24, CRC_RIGHT_SHIFTING> crc24_5_1_r;

        typedef crc_calc<0x005BC4F5, 24, CRC_LEFT_SHIFTING>  crc24_5_2_l;
        typedef crc_calc<0x00AF23DA, 24, CRC_RIGHT_SHIFTING> crc24_5_2_r;

        typedef crc_calc<0x00328B63, 24, CRC_LEFT_SHIFTING>  crc24_4_l;
        typedef crc_calc<0x00C6D14C, 24, CRC_RIGHT_SHIFTING> crc24_4_r;

        // 32-bit fields
        typedef crc_calc<0xF1922815, 32, CRC_LEFT_SHIFTING>  crc32_8_l;
        typedef crc_calc<0xA814498F, 32, CRC_RIGHT_SHIFTING> crc32_8_r;

        typedef crc_calc<0x404098E2, 32, CRC_LEFT_SHIFTING>  mp_crc32_8_1_l;
        typedef crc_calc<0x47190202, 32, CRC_RIGHT_SHIFTING> mp_crc32_8_1_r;

        typedef crc_calc<0x0884C512, 32, CRC_LEFT_SHIFTING>  mp_crc32_8_2_l;
        typedef crc_calc<0x48A32110, 32, CRC_RIGHT_SHIFTING> mp_crc32_8_2_r;

        typedef crc_calc<0xF6ACFB13, 32, CRC_LEFT_SHIFTING>  crc32_6_l;
        typedef crc_calc<0xC8DF356F, 32, CRC_RIGHT_SHIFTING> crc32_6_r;

        typedef crc_calc<0xA833982B, 32, CRC_LEFT_SHIFTING>  crc32_5_1_l;
        typedef crc_calc<0xD419CC15, 32, CRC_RIGHT_SHIFTING> crc32_5_1_r;

        typedef crc_calc<0x572D7285, 32, CRC_LEFT_SHIFTING>  crc32_5_2_l;
        typedef crc_calc<0xA14EB4EA, 32, CRC_RIGHT_SHIFTING> crc32_5_2_r;

        typedef crc_calc<0x1EDC6F41, 32, CRC_LEFT_SHIFTING>  crc32_4_l;
        typedef crc_calc<0x82F63B78, 32, CRC_RIGHT_SHIFTING> crc32_4_r;

        typedef crc_calc<0x04C11DB7, 32, CRC_LEFT_SHIFTING>  crc32_ieee_802_l;
        typedef crc_calc<0xEDB88320, 32, CRC_RIGHT_SHIFTING> crc32_ieee_802_r;
    }

    // These polynomials come from the paper "Cyclic Redundancy Code (CRC)
    // Polynomial Selection For Embedded Networks" by Philip Koopman and
    // Tridib Chakravarty.

    namespace kc
    {
        // 8-bit CRCs
        typedef crc_calc<0x83, 8, CRC_RIGHT_SHIFTING> crc8_atm8;
        typedef crc_calc<0x97, 8, CRC_RIGHT_SHIFTING> crc8_c2;
        typedef crc_calc<0x98, 8, CRC_RIGHT_SHIFTING> crc8_dowcrc;
        typedef crc_calc<0x9C, 8, CRC_RIGHT_SHIFTING> crc8_darc8;
        typedef crc_calc<0xA6, 8, CRC_RIGHT_SHIFTING> crc8_0xa6;
        typedef crc_calc<0xCD, 8, CRC_RIGHT_SHIFTING> crc8_wcdma8;
    }
}

#endif

