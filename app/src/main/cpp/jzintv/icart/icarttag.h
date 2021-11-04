#ifndef ICARTTAG_H_
#define ICARTTAG_H_ 1

#include "config.h"

/* ======================================================================== */
/*  ICT_TYPE_T   -- Defined cartridge tag types.                            */
/* ======================================================================== */
typedef enum ict_type_t
{
    /* -------------------------------------------------------------------- */
    /*  General Game Information tag ID #'s.                                */
    /* -------------------------------------------------------------------- */
    ICT_IGNORE          = 0x00,     ICT_RSVD_10     = 0x10,
    ICT_FULL_TITLE      = 0x01,     ICT_RSVD_11     = 0x11,
    ICT_PUBLISHER       = 0x02,     ICT_RSVD_12     = 0x12,
    ICT_CREDITS         = 0x03,     ICT_RSVD_13     = 0x13,
    ICT_INFOURL         = 0x04,     ICT_RSVD_14     = 0x14,
    ICT_RELEASE_DATE    = 0x05,     ICT_RSVD_15     = 0x15,
    ICT_COMPAT          = 0x06,     ICT_RSVD_16     = 0x16,
    ICT_BINDINGS        = 0x07,     ICT_RSVD_17     = 0x17,
    ICT_SHORT_TITLE     = 0x08,     ICT_RSVD_18     = 0x18,
    ICT_LICENSE         = 0x09,     ICT_RSVD_19     = 0x19,
    ICT_DESCRIPTION     = 0x0A,     ICT_RSVD_1A     = 0x1A,
    ICT_BUILD_DATE      = 0x0B,     ICT_RSVD_1B     = 0x1B,
    ICT_VERSION         = 0x0C,     ICT_RSVD_1C     = 0x1C,
    ICT_RSVD_0D         = 0x0D,     ICT_RSVD_1D     = 0x1D,
    ICT_RSVD_0E         = 0x0E,     ICT_RSVD_1E     = 0x1E,
    ICT_RSVD_0F         = 0x0F,     ICT_RSVD_1F     = 0x1F,

    /* -------------------------------------------------------------------- */
    /*  Debugging Information tag ID #'s.                                   */
    /* -------------------------------------------------------------------- */
    ICT_SYMTAB          = 0x20,     ICT_RSVD_30     = 0x30,
    ICT_MEMATTR         = 0x21,     ICT_RSVD_31     = 0x31,
    ICT_LINEMAP         = 0x22,     ICT_RSVD_32     = 0x32,
    ICT_RSVD_23         = 0x23,     ICT_RSVD_33     = 0x33,
    ICT_RSVD_24         = 0x24,     ICT_RSVD_34     = 0x34,
    ICT_RSVD_25         = 0x25,     ICT_RSVD_35     = 0x35,
    ICT_RSVD_26         = 0x26,     ICT_RSVD_36     = 0x36,
    ICT_RSVD_27         = 0x27,     ICT_RSVD_37     = 0x37,
    ICT_RSVD_28         = 0x28,     ICT_RSVD_38     = 0x38,
    ICT_RSVD_29         = 0x29,     ICT_RSVD_39     = 0x39,
    ICT_RSVD_2A         = 0x2A,     ICT_RSVD_3A     = 0x3A,
    ICT_RSVD_2B         = 0x2B,     ICT_RSVD_3B     = 0x3B,
    ICT_RSVD_2C         = 0x2C,     ICT_RSVD_3C     = 0x3C,
    ICT_RSVD_2D         = 0x2D,     ICT_RSVD_3D     = 0x3D,
    ICT_RSVD_2E         = 0x2E,     ICT_RSVD_3E     = 0x3E,
    ICT_RSVD_2F         = 0x2F,     ICT_RSVD_3F     = 0x3F,

    /* -------------------------------------------------------------------- */
    /*  RESERVED:  0x40-0xEF                                                */
    /* -------------------------------------------------------------------- */
    ICT_RSVD_40=0x40, ICT_RSVD_50=0x50, ICT_RSVD_60=0x60, ICT_RSVD_70=0x70,
    ICT_RSVD_41=0x41, ICT_RSVD_51=0x51, ICT_RSVD_61=0x61, ICT_RSVD_71=0x71,
    ICT_RSVD_42=0x42, ICT_RSVD_52=0x52, ICT_RSVD_62=0x62, ICT_RSVD_72=0x72,
    ICT_RSVD_43=0x43, ICT_RSVD_53=0x53, ICT_RSVD_63=0x63, ICT_RSVD_73=0x73,
    ICT_RSVD_44=0x44, ICT_RSVD_54=0x54, ICT_RSVD_64=0x64, ICT_RSVD_74=0x74,
    ICT_RSVD_45=0x45, ICT_RSVD_55=0x55, ICT_RSVD_65=0x65, ICT_RSVD_75=0x75,
    ICT_RSVD_46=0x46, ICT_RSVD_56=0x56, ICT_RSVD_66=0x66, ICT_RSVD_76=0x76,
    ICT_RSVD_47=0x47, ICT_RSVD_57=0x57, ICT_RSVD_67=0x67, ICT_RSVD_77=0x77,
    ICT_RSVD_48=0x48, ICT_RSVD_58=0x58, ICT_RSVD_68=0x68, ICT_RSVD_78=0x78,
    ICT_RSVD_49=0x49, ICT_RSVD_59=0x59, ICT_RSVD_69=0x69, ICT_RSVD_79=0x79,
    ICT_RSVD_4A=0x4A, ICT_RSVD_5A=0x5A, ICT_RSVD_6A=0x6A, ICT_RSVD_7A=0x7A,
    ICT_RSVD_4B=0x4B, ICT_RSVD_5B=0x5B, ICT_RSVD_6B=0x6B, ICT_RSVD_7B=0x7B,
    ICT_RSVD_4C=0x4C, ICT_RSVD_5C=0x5C, ICT_RSVD_6C=0x6C, ICT_RSVD_7C=0x7C,
    ICT_RSVD_4D=0x4D, ICT_RSVD_5D=0x5D, ICT_RSVD_6D=0x6D, ICT_RSVD_7D=0x7D,
    ICT_RSVD_4E=0x4E, ICT_RSVD_5E=0x5E, ICT_RSVD_6E=0x6E, ICT_RSVD_7E=0x7E,
    ICT_RSVD_4F=0x4F, ICT_RSVD_5F=0x5F, ICT_RSVD_6F=0x6F, ICT_RSVD_7F=0x7F,

    ICT_RSVD_80=0x80, ICT_RSVD_90=0x90, ICT_RSVD_A0=0xA0, ICT_RSVD_B0=0xB0,
    ICT_RSVD_81=0x81, ICT_RSVD_91=0x91, ICT_RSVD_A1=0xA1, ICT_RSVD_B1=0xB1,
    ICT_RSVD_82=0x82, ICT_RSVD_92=0x92, ICT_RSVD_A2=0xA2, ICT_RSVD_B2=0xB2,
    ICT_RSVD_83=0x83, ICT_RSVD_93=0x93, ICT_RSVD_A3=0xA3, ICT_RSVD_B3=0xB3,
    ICT_RSVD_84=0x84, ICT_RSVD_94=0x94, ICT_RSVD_A4=0xA4, ICT_RSVD_B4=0xB4,
    ICT_RSVD_85=0x85, ICT_RSVD_95=0x95, ICT_RSVD_A5=0xA5, ICT_RSVD_B5=0xB5,
    ICT_RSVD_86=0x86, ICT_RSVD_96=0x96, ICT_RSVD_A6=0xA6, ICT_RSVD_B6=0xB6,
    ICT_RSVD_87=0x87, ICT_RSVD_97=0x97, ICT_RSVD_A7=0xA7, ICT_RSVD_B7=0xB7,
    ICT_RSVD_88=0x88, ICT_RSVD_98=0x98, ICT_RSVD_A8=0xA8, ICT_RSVD_B8=0xB8,
    ICT_RSVD_89=0x89, ICT_RSVD_99=0x99, ICT_RSVD_A9=0xA9, ICT_RSVD_B9=0xB9,
    ICT_RSVD_8A=0x8A, ICT_RSVD_9A=0x9A, ICT_RSVD_AA=0xAA, ICT_RSVD_BA=0xBA,
    ICT_RSVD_8B=0x8B, ICT_RSVD_9B=0x9B, ICT_RSVD_AB=0xAB, ICT_RSVD_BB=0xBB,
    ICT_RSVD_8C=0x8C, ICT_RSVD_9C=0x9C, ICT_RSVD_AC=0xAC, ICT_RSVD_BC=0xBC,
    ICT_RSVD_8D=0x8D, ICT_RSVD_9D=0x9D, ICT_RSVD_AD=0xAD, ICT_RSVD_BD=0xBD,
    ICT_RSVD_8E=0x8E, ICT_RSVD_9E=0x9E, ICT_RSVD_AE=0xAE, ICT_RSVD_BE=0xBE,
    ICT_RSVD_8F=0x8F, ICT_RSVD_9F=0x9F, ICT_RSVD_AF=0xAF, ICT_RSVD_BF=0xBF,

    ICT_RSVD_C0=0xC0, ICT_RSVD_D0=0xD0, ICT_RSVD_E0=0xE0,
    ICT_RSVD_C1=0xC1, ICT_RSVD_D1=0xD1, ICT_RSVD_E1=0xE1,
    ICT_RSVD_C2=0xC2, ICT_RSVD_D2=0xD2, ICT_RSVD_E2=0xE2,
    ICT_RSVD_C3=0xC3, ICT_RSVD_D3=0xD3, ICT_RSVD_E3=0xE3,
    ICT_RSVD_C4=0xC4, ICT_RSVD_D4=0xD4, ICT_RSVD_E4=0xE4,
    ICT_RSVD_C5=0xC5, ICT_RSVD_D5=0xD5, ICT_RSVD_E5=0xE5,
    ICT_RSVD_C6=0xC6, ICT_RSVD_D6=0xD6, ICT_RSVD_E6=0xE6,
    ICT_RSVD_C7=0xC7, ICT_RSVD_D7=0xD7, ICT_RSVD_E7=0xE7,
    ICT_RSVD_C8=0xC8, ICT_RSVD_D8=0xD8, ICT_RSVD_E8=0xE8,
    ICT_RSVD_C9=0xC9, ICT_RSVD_D9=0xD9, ICT_RSVD_E9=0xE9,
    ICT_RSVD_CA=0xCA, ICT_RSVD_DA=0xDA, ICT_RSVD_EA=0xEA,
    ICT_RSVD_CB=0xCB, ICT_RSVD_DB=0xDB, ICT_RSVD_EB=0xEB,
    ICT_RSVD_CC=0xCC, ICT_RSVD_DC=0xDC, ICT_RSVD_EC=0xEC,
    ICT_RSVD_CD=0xCD, ICT_RSVD_DD=0xDD, ICT_RSVD_ED=0xED,
    ICT_RSVD_CE=0xCE, ICT_RSVD_DE=0xDE, ICT_RSVD_EE=0xEE,
    ICT_RSVD_CF=0xCF, ICT_RSVD_DF=0xDF, ICT_RSVD_EF=0xEF,

    /* -------------------------------------------------------------------- */
    /*  Extended Tags:  0xF0 - 0xFF                                         */
    /* -------------------------------------------------------------------- */
    ICT_EXTENDED    = 0xF0,     ICT_RSVD_F8     = 0xF8,
    ICT_RSVD_F1     = 0xF1,     ICT_RSVD_F9     = 0xF9,
    ICT_RSVD_F2     = 0xF2,     ICT_RSVD_FA     = 0xFA,
    ICT_RSVD_F3     = 0xF3,     ICT_RSVD_FB     = 0xFB,
    ICT_RSVD_F4     = 0xF4,     ICT_RSVD_FC     = 0xFC,
    ICT_RSVD_F5     = 0xF5,     ICT_RSVD_FD     = 0xFD,
    ICT_RSVD_F6     = 0xF6,     ICT_RSVD_FE     = 0xFE,
    ICT_RSVD_F7     = 0xF7,     ICT_RSVD_FF     = 0xFF,
} ict_type_t;

/* ======================================================================== */
/*  Flag bitfields used by many of the tag types.                           */
/* ======================================================================== */
typedef struct ict_credbit_t
{
    union
    {
        uint8_t raw[1];
        struct
        {
            BFE(uint8_t author              : 1,
            BFE(uint8_t game_art            : 1,
            BFE(uint8_t music               : 1,
            BFE(uint8_t sound_effects       : 1,
            BFE(uint8_t voices              : 1,
            BFE(uint8_t documentation       : 1,
            BFE(uint8_t game_concept        : 1,
                uint8_t box_art             : 1)))))));
        } s;
    } u;
} ict_credbit_t;

enum
{
    ICT_COMPAT_DONTCARE = 0,
    ICT_COMPAT_SUPPORTS = 1,
    ICT_COMPAT_REQUIRES = 2,
    ICT_COMPAT_INCOMPAT = 3
};

/* ------------------------------------------------------------------------ */
/*  These translation macros require metadata.h to be included before you   */
/*  can use them.  This mapping isn't exact, but it's sufficiently close.   */
/*  For example, DONTCARE could mean either "TOLERATES" or "UNSPECIFIED".   */
/*  I chose a mapping such that IC => GM => IC gives you back the original. */
/* ------------------------------------------------------------------------ */
#define IC_TO_GM_COMPAT(x) \
    ( (x) == ICT_COMPAT_DONTCARE ? CMP_TOLERATES    \
    : (x) == ICT_COMPAT_SUPPORTS ? CMP_ENHANCED     \
    : (x) == ICT_COMPAT_REQUIRES ? CMP_REQUIRES     \
    : (x) == ICT_COMPAT_INCOMPAT ? CMP_INCOMPATIBLE \
    :                              CMP_UNSPECIFIED )

#define GM_TO_IC_COMPAT(x) \
    ( (x) == CMP_INCOMPATIBLE ? ICT_COMPAT_INCOMPAT \
    : (x) == CMP_TOLERATES    ? ICT_COMPAT_DONTCARE \
    : (x) == CMP_ENHANCED     ? ICT_COMPAT_SUPPORTS \
    : (x) == CMP_REQUIRES     ? ICT_COMPAT_REQUIRES \
    :                           ICT_COMPAT_DONTCARE)
    
typedef struct ict_compat_t
{
    union
    {
        uint8_t raw[5];
        struct
        {
            /* Byte 0 */
            BFE(uint8_t keyboard_component  : 2,
            BFE(uint8_t intellivoice        : 2,
            BFE(uint8_t rsvd_0_4            : 2,        /* Was 4CTRL    */
                uint8_t ecs                 : 2)));
                                           
            /* Byte 1 */                   
            BFE(uint8_t intellivision_2     : 2,
            BFE(uint8_t tutorvision         : 2,
            BFE(uint8_t rsvd_1_4            : 2,
                uint8_t rsvd_1_6            : 2)));
            
            /* Byte 2 */
            uint8_t rsvd_byte;

            /* Bytes 3, 4 are optional.  However, if byte 3 is present, */
            /* byte 4 must also be present. */

            /* Byte 3 */
            BFE(uint8_t jlp_flash_hi        : 2,
            BFE(uint8_t rsvd_3_2            : 3,
            BFE(uint8_t lto_mapper          : 1,
                uint8_t jlp_accel           : 2)));

            /* Byte 4 */
            uint8_t jlp_flash_lo;
        } s;
    } u;
} ict_compat_t;

enum
{
    ICT_MEMATTR_CODE    = 0x10,
    ICT_MEMATTR_DATA    = 0x20,
    ICT_MEMATTR_DBDATA  = 0x40,
    ICT_MEMATTR_STRING  = 0x80
};

/* ======================================================================== */
/*  Structures for the tag list and for each of the tag types.              */
/* ======================================================================== */
typedef struct ict_title_t
{
    char        *name;
} ict_title_t;

typedef struct ict_publisher_t
{
    char        *name;
} ict_publisher_t;

typedef struct ict_credits_t
{
    char            *name;      /* Name associated with this credit         */
    ict_credbit_t   credbits;  /* Credit bits associated with this name     */
} ict_credits_t;


typedef struct ict_infourl_t
{
    char        *url;           /* The URL (http, mailto, etc.)             */
} ict_infourl_t;

typedef struct ict_license_t
{
    char        *license;
} ict_license_t;

typedef struct ict_desc_t
{
    char        *text;
} ict_desc_t;

typedef struct ict_version_t
{
    char        *text;
} ict_version_t;

/* Use game_date_t instead of our own custom ict_date_t.  Forward decl: */
struct game_date_t;

#if 0 /* Let's not support these just yet. */
typedef struct ict_symtab_t
{
    PAVLTree    by_name;        /* Symbol table sorted by name.             */
    PAVLTree    by_addr;        /* Symbol table sorted by addr.             */
} ict_symtab_t;

typedef struct ict_memattr_t
{
    uint16_t    addr_lo;        /* starting address                         */
    uint16_t    addr_hi;        /* ending address (inclusive range)         */
    uint8_t     flags;          /* flags associated with span               */
} ict_memattr_t;

typedef struct ict_linemap_t
{
    int         line_no;
    uint16_t    addr;
} ict_linemap_t;

typedef struct ict_unknown_t
{
    uint8_t     *data;
    uint32_t    len;
} ict_unknown_t;

struct ict_extended_t
{
    char                *name;
    ict_unknown_t       body;
};
#endif

/* ======================================================================== */
/*  ICARTTAG parser                                                         */
/*  Built on the gawdawful visitor pattern, because I enjoy pain.           */
/* ======================================================================== */
typedef int ict_startstop_fxn(void *opaque);
typedef int ict_title_fxn    (void *opaque, const ict_title_t *         );
typedef int ict_publisher_fxn(void *opaque, const ict_publisher_t *     );
typedef int ict_credits_fxn  (void *opaque, const ict_credits_t *       );
typedef int ict_infourl_fxn  (void *opaque, const ict_infourl_t *       );
typedef int ict_date_fxn     (void *opaque, const struct game_date_t *  );
typedef int ict_compat_fxn   (void *opaque, const ict_compat_t *        );
typedef int ict_license_fxn  (void *opaque, const ict_license_t *       );
typedef int ict_desc_fxn     (void *opaque, const ict_desc_t *          );
typedef int ict_version_fxn  (void *opaque, const ict_version_t *       );

typedef struct icarttag_visitor_t
{
    /*  The opaque pointer gets passed to every 'visit' call.  It's a bit   */
    /*  like a 'this' pointer in C++.                                       */
    void                *opaque;

    /*  The start fxn is called before parsing any tags.  The stop fxn is   */
    /*  called after tag parsing completes.                                 */
    ict_startstop_fxn   *start, *stop;

    /*  Each of these is called for each instance of its corresponding tag. */
    /*  The tag provided will get deallocated after the visit.  The callee  */
    /*  should make copies of anything it intends to keep.  NULL pointers   */
    /*  are legal here, and indicate "no visit requested."                  */
    ict_title_fxn       *visit_full_title;
    ict_publisher_fxn   *visit_publisher;
    ict_credits_fxn     *visit_credits;
    ict_infourl_fxn     *visit_infourl;
    ict_date_fxn        *visit_release_date;
    ict_compat_fxn      *visit_compat;
    ict_title_fxn       *visit_short_title;
    ict_license_fxn     *visit_license;
    ict_desc_fxn        *visit_desc;
    ict_date_fxn        *visit_build_date;
    ict_version_fxn     *visit_version;
} icarttag_visitor_t;

enum icarttag_errors    /* -1 thru -7 are icartrom_errors */
{
    IC_CRC_ERROR_TAG    = -8,
    IC_OUT_OF_MEMORY    = -9,
    IC_TAG_PARSING_ERR  = -10,
    IC_VISITOR_ERROR    = -11
};

/* returns number of bytes processed on success, or -ve value on failure.   */
int icarttag_decode
(
    const uint8_t *const rom_img,           /* First byte of ROM file.      */
    const int            length,            /* Total size of .ROM file.     */
    const int            ignore_crc,        /* Disables CRC checks if set.  */
    const int            tag_ofs,           /* Offset of tags, if known.    */
                                            /* Pass in -1 if not known.     */
    const icarttag_visitor_t *const visitor,/* Visiting class               */
    int *const           visitor_error      /* For reporting visitor errors */
);
    
/* ======================================================================== */
/*  An ICARTTAG printing visitor, for debug purposes.                       */
/* ======================================================================== */
extern const icarttag_visitor_t ict_printer;

/* ======================================================================== */
/*  ICARTTAG encoder                                                        */
/*  For now just accept a game metadata structure to encode.  Returns an    */
/*  allocated chunk of memory with the encoded tags.  Good enough for now.  */
/* ======================================================================== */
struct game_metadata_t;     /* forward decl. */
uint8_t *icarttag_encode
(
    const struct game_metadata_t *const metadata,
    size_t                       *const encoded_length,
    int                          *const error_code
);
 
/* ======================================================================== */
/*  Misc data                                                               */
/* ======================================================================== */
extern const char *ic_author_list[128];
extern const char *ic_publisher_names[15];

enum
{
    IC_PUBLISHER_COUNT = 15
};

#endif
