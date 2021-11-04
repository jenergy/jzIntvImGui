/* ======================================================================== */
/*  Takes a .GMS file from INTVPC, and makes a playable .ROM from it.       */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "icart/icartrom.h"

/* ======================================================================== */
/*  These are errors that can be reported by the Intellicart routines.      */
/* ======================================================================== */
const char *rom_errors[] =
{
    "No Error",
    "Bad Arguments",
    "Bad ROM Header",
    "CRC-16 Error in ROM Segments",
    "Bad ROM Segment Address Range",
    "Bad ROM Fine-Address Range",
    "CRC-16 Error in Enable Tables",
    "Unknown Error"
};

/* ======================================================================== */
/*  Our Intellicart and GMS images.                                         */
/* ======================================================================== */
icartrom_t the_icart;
uint8_t  *rom_img;
uint8_t  *gms_img;


/* ======================================================================== */
/*  GMS Loader Memory Maps                                                  */
/*                                                                          */
/*      ICART ADDRS       CONTENTS                                          */
/*      $0000 - $0032     Data to copy to STIC                              */
/*      $0040 - $0049     CPU state (R0..R7, SWD, EIS)                      */
/*      $004A - $004C     Relocated code from $7000 - $7002 (if any)        */
/*      $004D             Copy of vector to write to $100 - $101            */
/*      $004E - $004F     Display mode, visibility                          */
/*      $00F0 - $035F     Data to copy to PSGs, RAM                         */
/*      $0500 - $07FF     Scratchpad memory                                 */
/*      $0800 - $0FFF     Bankswitched memory                               */
/*      $3800 - $3AFF     Data to copy to GRAM                              */
/*      $4000 - $47FF     Data to copy to ECS RAM (if any)                  */
/*      $4800 - $7FFF     Available for game state.                         */
/*      $9000 - $BFFF     Available for game state.                         */
/*      $C800 - $CFFF     GMS Loader Software                               */
/*      $D000 - $FFFF     Available for game state.                         */
/*                                                                          */
/*      INTY ADDRS        CONTENTS                                          */
/*      $0500 - $0CFF     GMS Loader Software                               */
/*      $0D00 - $0DFF     Scratchpad RAM (stack, etc.)                      */
/*      $0E00 - $0EFF     Read/Write/Bankswitch memory window               */
/*      $0F00 - $0FFF     Read/Write/Bankswitch memory window               */
/*      $4800 - $FFFF     Mapped as necessary for game ROM.                 */
/*      $7000 - $70FF     Always mapped, even if no game ROM.               */
/*                        $7000 - $7002 will hold JD $0500 initially.       */
/*                                                                          */
/*  GMS File Layout                                                         */
/*                                                                          */
/*  From Carl on the layout of the non-memory-dump info in the GMS file:    */
/*                                                                          */
/*    Hi Joe.  Here is the relevant portion of code that writes the GMS     */
/*    file:                                                                 */
/*                                                                          */
/*      DstBuf^.Write( AY8914_1_SaveRec, SizeOf( AY8914_1_SaveRec ) );      */
/*      DstBuf^.Write( AY8914_2_SaveRec, SizeOf( AY8914_2_SaveRec ) );      */
/*                                                                          */
/*      { save CPU state }                                                  */
/*      with CPU^ do begin                                                  */
/*        DstBuf^.Write( R, SizeOf( R ) );                                  */
/*        DstBuf^.Write( Status, SizeOf( Status ) );                        */
/*        DstBuf^.Write( Interrupts, SizeOf( Interrupts ) );                */
/*        DstBuf^.Write( SDBD, SizeOf( SDBD ) );                            */
/*        DstBuf^.Write( cycCount, SizeOf( cycCount ) );                    */
/*        DstBuf^.Write( IntrReq, SizeOf( IntrReq ) );                      */
/*        DstBuf^.Write( IntrVec, SizeOf( IntrVec ) );                      */
/*        DstBuf^.Write( IntrCyc, SizeOf( IntrCyc ) );                      */
/*      end;                                                                */
/*                                                                          */
/*      { save STIC state }                                                 */
/*      with STIC do begin                                                  */
/*        DstBuf^.Write( Mode, SizeOf( Mode ) );                            */
/*        DstBuf^.Write( Display, SizeOf( Display ) );                      */
/*      end;                                                                */
/*                                                                          */
/*  The GMS file is written in the following order:                         */
/*                                                                          */
/*      FILE OFFSET       DESCRIPTION                                       */
/*      $00000 - $1FFFF   RAM dump in little-endian byte order              */
/*      $20000 - $2001D   AY8914 #1 and #2 internal state (ignored)         */
/*      $2001E - $2002D   CP1600 R0-R7, little-endian byte order            */
/*      $2002E            Status word >> 4.                                 */
/*                          Bit 3 = Sign                                    */
/*                          Bit 2 = Zero                                    */
/*                          Bit 1 = Over                                    */
/*                          Bit 0 = Carry                                   */
/*      $2002F            Interrupt enable / Interruptible / Pending        */
/*                          Bit 0 = Interrupts enabled                      */
/*                          Bit 1 = Interrupt pending                       */
/*                          Bit 2 = Interruptible instruction               */
/*      $20030            SDBD state (0 == SDBD prefix active)              */
/*      $20031 - $20034   Total emu cycle count (Little endian, ignored)    */
/*      $20035 - $20036   Hardware INTRM vector (Little endian, ignored)    */
/*      $20037 - $20038   Interrupt period in cycles (Big endian, ignored)  */
/*      $20039            Mode:     0 = Colored Squares, 1 = FGBG           */
/*      $2003A            Display:  0 = Blanked, 1 = Visible                */
/*                                                                          */
/*  The table "gmsl_fixed_ranges" below describes the fixed file-offset     */
/*  ranges that are copied to the Intellicart address space.  The "pack"    */
/*  flag controls whether bytes are packed into words.  If it's non-zero,   */
/*  bytes are packed into word assuming a little-endian order.              */
/* ======================================================================== */
#define GMSL_CPUREGS   (0x0040)
#define GMSL_DISPMODE  (0x004E)
#define GMSL_SCRATCH   (0x0500)
#define GMSL_WINDOW    (0x0800)
#define GMSL_BOOTRELO  (0x004A)
#define GMSL_PROGRAM   (0xC800)
#define GMSL_SCRSIZE   (0x0800 - 0x0500)
#define GMSL_WINDSIZE  (0x1000 - 0x0800)

typedef struct gmsl_range_t
{
    int      ofs_lo, ofs_hi;   /* File offset in GMS file.                */
    uint16_t dst_addr;         /* Address range in Intellicart addr space */
    short    pack;             /* Whether to pack the data.               */
    int      flags;            /* Intellicart flags.                      */
} gmsl_range_t;

gmsl_range_t gmsl_fixed_ranges[] =
{
    { 0x0000*2, 0x0032*2 + 1,  0x0000          , 1, 0 },
    { 0x00F0*2, 0x035F*2 + 1,  0x00F0          , 1, 0 },
    { 0x3800*2, 0x3AFF*2 + 1,  0x3800          , 1, 0 },
    { 0x7000*2, 0x7002*2 + 1,  GMSL_BOOTRELO   , 1, 0 },
    { 0x2001E,  0x2002D,       GMSL_CPUREGS    , 1, 0 },
    { 0x2002E,  0x2002F,       GMSL_CPUREGS + 8, 0, 0 },
    { 0x20039,  0x2003A,       GMSL_DISPMODE   , 0, 0 },
    { 0, 0, 0, 0, 0 },
};

gmsl_range_t gmsl_game_ranges[] =
{
    { 0x4000*2, 0x47FF*2 + 1, 0x4000, 1, 0                            },
    { 0x4800*2, 0x7FFF*2 + 1, 0x4800, 1, ICARTROM_READ                },
    { 0x9000*2, 0xBFFF*2 + 1, 0x9000, 1, ICARTROM_READ                },
    { 0xD000*2, 0xFFFF*2 + 1, 0xD000, 1, ICARTROM_READ                },
    { 0, 0, 0, 0, 0 },
};

/* ======================================================================== */
/*  The GMS Loader code which executes on the Inty.  This unpacks the RAM   */
/*  and CPU register information that came from the GMS file.               */
/* ======================================================================== */
uint16_t gms_loader[] =
{
                             /*   GMSPROGRAM  PROC                          */
    0x02B8, 0xC80A,          /*               MVII    #GMSMAIN,   R0        */
    0x0240, 0x0100,          /*               MVO     R0,     $100          */
    0x0040,                  /*               SWAP    R0                    */
    0x0240, 0x0101,          /*               MVO     R0,     $101          */
    0x0002,                  /*               EIS                           */
    0x0017,                  /*               DECR    PC                    */
                             /*               ENDP                          */
                             /*                                             */
    0x00AF,                  /*   STUB        JR      R5                    */
                             /*                                             */
                             /*   GMSMAIN     PROC                          */
    0x0003,                  /*               DIS                           */
    0x02BE, 0x0600,          /*               MVII    #STACK, R6            */
    0x02B8, 0xC809,          /*               MVII    #STUB,  R0            */
    0x0240, 0x0100,          /*               MVO     R0,     $100          */
    0x0040,                  /*               SWAP    R0                    */
    0x0240, 0x0101,          /*               MVO     R0,     $101          */
    0x0002,                  /*               EIS                           */
                             /*                                             */
    0x0004, 0x01C8, 0x0085,  /*               CALL    COPYRAM               */
    0x0800, 0x4000,          /*               DECLE   $0800,  $4000         */
    0x0020, 0x0000,          /*               DECLE   $0020,  $0000         */
    0x000B, 0x0028,          /*               DECLE   $000B,  $0028         */
    0x0010, 0x00F0,          /*               DECLE   $0010,  $00F0         */
    0x025E, 0x0102,          /*               DECLE   $025E,  $0102         */
    0x0200, 0x3800,          /*               DECLE   $0200,  $3800         */
    0x0000,                  /*               DECLE   0                     */
                             /*                                             */
    0x02B9, 0x0008,          /*               MVII    #WBANK, R1            */
    0x02BA, 0x0000,          /*               MVII    #CBANK, R2            */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK            */
                             /*                                             */
    0x0280, 0x084A,          /*               MVI     REGS.b0,  R0          */
    0x0283, 0x084B,          /*               MVI     REGS.b1,  R3          */
    0x0284, 0x084C,          /*               MVI     REGS.b2,  R4          */
                             /*                                             */
    0x02B9, 0x0008,          /*               MVII    #WBANK, R1            */
    0x02BA, 0x0070,          /*               MVII    #$70,   R2            */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK            */
                             /*                                             */
    0x0240, 0x0800,          /*               MVO     R0, WINDOW+0          */
    0x0243, 0x0801,          /*               MVO     R3, WINDOW+1          */
    0x0244, 0x0802,          /*               MVO     R4, WINDOW+2          */
                             /*                                             */
                             /*                                             */
    0x02B9, 0x0008,          /*               MVII    #WBANK, R1            */
    0x02BA, 0x0000,          /*               MVII    #CBANK, R2            */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK            */
                             /*                                             */
                             /*                                             */
    0x02B9, 0xC84E,          /*               MVII    #GMSLAUNCH, R1        */
    0x0241, 0x0100,          /*               MVO     R1,     $100          */
    0x0041,                  /*               SWAP    R1                    */
    0x0241, 0x0101,          /*               MVO     R1,     $101          */
    0x0017,                  /*               DECR    PC                    */
                             /*               ENDP                          */
                             /*                                             */
                             /*   GMSLAUNCH   PROC                          */
    0x0003,                  /*               DIS                           */
    0x0280, 0x084F,          /*               MVI     REGS.visb,  R0        */
    0x0080,                  /*               TSTR    R0                    */
    0x0204, 0x0002,          /*               BEQ     @@notvis              */
    0x0240, 0x0020,          /*               MVO     R0,     $20           */
                             /*                                             */
    0x0280, 0x0021,          /*               MVI     $21,        R0        */
    0x0280, 0x084E,          /*               MVI     REGS.mode,  R0        */
    0x0080,                  /*               TSTR    R0                    */
    0x0204, 0x0002,          /*               BEQ     @@colstk              */
    0x0240, 0x0021,          /*               MVO     R0,         $21       */
                             /*                                             */
    0x0280, 0x084D,          /*               MVI     REGS.isrv,  R0        */
    0x0240, 0x0100,          /*               MVO     R0,         $100      */
    0x0040,                  /*               SWAP    R0                    */
    0x0240, 0x0101,          /*               MVO     R0,         $101      */
                             /*                                             */
    0x02BD, 0x0841,          /*               MVII    #REGS.1,    R5        */
    0x02A9,                  /*               MVI@    R5,         R1        */
    0x02AA,                  /*               MVI@    R5,         R2        */
    0x02AB,                  /*               MVI@    R5,         R3        */
    0x02AC,                  /*               MVI@    R5,         R4        */
    0x000D,                  /*               INCR    R5                    */
    0x02AE,                  /*               MVI@    R5,         R6        */
    0x0285, 0x0845,          /*               MVI     REGS.5,     R5        */
                             /*                                             */
    0x0280, 0x0849,          /*               MVI     REGS.intr,  R0        */
    0x03B8, 0x0001,          /*               ANDI    #1,         R0        */
    0x0204, 0x0007,          /*               BEQ     @@do_eis              */
                             /*                                             */
    0x0280, 0x0848,          /*               MVI     REGS.stat,  R0        */
    0x0038,                  /*               RSWD    R0                    */
    0x0280, 0x0840,          /*               MVI     REGS.0,     R0        */
    0x0287, 0x0847,          /*               MVI     REGS.7,     R7        */
                             /*                                             */
    0x0280, 0x0848,          /*     @@do_eis: MVI     REGS.stat,  R0        */
    0x0038,                  /*               RSWD    R0                    */
    0x0280, 0x0840,          /*               MVI     REGS.0,     R0        */
    0x0002,                  /*               EIS                           */
    0x0287, 0x0847,          /*               MVI     REGS.7,     R7        */
                             /*               ENDP                          */
                             /*                                             */
                             /*   COPYRAM     PROC                          */
    0x0200, 0x0023,          /*               B       @@first               */
                             /*                                             */
    0x02AC,                  /*     @@o_loop: MVI@    R5,     R4            */
    0x0275,                  /*               PSHR    R5                    */
                             /*                                             */
    0x02B9, 0x0008,          /*     @@m_loop: MVII    #WBANK, R1            */
    0x00A2,                  /*               MOVR    R4,     R2            */
    0x0042,                  /*               SWAP    R2                    */
    0x03BA, 0x00FF,          /*               ANDI    #$FF,   R2            */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK            */
                             /*                                             */
    0x00A5,                  /*               MOVR    R4,     R5            */
    0x03BD, 0x00FF,          /*               ANDI    #$FF,   R5            */
    0x02FD, 0x0800,          /*               ADDI    #WINDOW,R5            */
                             /*                                             */
    0x00AA,                  /*               MOVR    R5,     R2            */
    0x00DA,                  /*               ADDR    R3,     R2            */
    0x037A, 0x1000,          /*               CMPI    #WINDOW + 2048, R2    */
    0x0209, 0x0002,          /*               BNC     @@cnt_ok              */
    0x02BA, 0x0FFF,          /*               MVII    #WINDOW+2048-1, R2    */
    0x012A,                  /*     @@cnt_ok: SUBR    R5,     R2            */
                             /*                                             */
    0x0113,                  /*               SUBR    R2,     R3            */
                             /*                                             */
    0x02A9,                  /*     @@i_loop: MVI@    R5,     R1            */
    0x0261,                  /*               MVO@    R1,     R4            */
    0x0012,                  /*               DECR    R2                    */
    0x022C, 0x0004,          /*               BNEQ    @@i_loop              */
                             /*                                             */
    0x009B,                  /*               TSTR    R3                    */
    0x022C, 0x001F,          /*               BNEQ    @@m_loop              */
                             /*                                             */
    0x02B5,                  /*               PULR    R5                    */
    0x02AB,                  /*      @@first: MVI@    R5,     R3            */
    0x009B,                  /*               TSTR    R3                    */
    0x022C, 0x0026,          /*               BNEQ    @@o_loop              */
    0x00AF,                  /*               JR      R5                    */
                             /*               ENDP                          */
                             /*                                             */
                             /*   IC_SETBANK  PROC                          */
    0x0275,                  /*               PSHR    R5                    */
    0x008D,                  /*               MOVR    R1,     R5            */
    0x00ED,                  /*               ADDR    R5,     R5            */
    0x03BD, 0x0010,          /*               ANDI    #$10,   R5            */
    0x0065,                  /*               SLR     R1,     2             */
    0x0065,                  /*               SLR     R1,     2             */
    0x03B9, 0x000F,          /*               ANDI    #$0F,   R1            */
    0x00E9,                  /*               ADDR    R5,     R1            */
    0x02F9, 0x0040,          /*               ADDI    #$40,   R1            */
    0x024A,                  /*               MVO@    R2,     R1            */
    0x02B7,                  /*               PULR    PC                    */
                             /*               ENDP                          */
};

/* ======================================================================== */
/*  COPY_OVER_FIXED -- Scan the gmsl_fixed_ranges array and copy the        */
/*                     ranges from the GMS file to the Intellicart.         */
/* ======================================================================== */
LOCAL int copy_over_fixed(uint8_t *img, int gms_len, icartrom_t *ic)
{
    int i, pack, err;
    int lo, hi, j, len;
    uint16_t addr, data;

    /* -------------------------------------------------------------------- */
    /*  Copy data over from each of the ranges.                             */
    /* -------------------------------------------------------------------- */
    for (i = 0; gmsl_fixed_ranges[i].ofs_lo < gmsl_fixed_ranges[i].ofs_hi; i++)
    {
        lo   = gmsl_fixed_ranges[i].ofs_lo;
        hi   = gmsl_fixed_ranges[i].ofs_hi;
        addr = gmsl_fixed_ranges[i].dst_addr;
        pack = gmsl_fixed_ranges[i].pack != 0;
        len  = hi - lo + 1;

        /* ---------------------------------------------------------------- */
        /*  Make sure we stay inside the file we read.                      */
        /* ---------------------------------------------------------------- */
        if (lo > gms_len || hi > gms_len)
        {
            fprintf(stderr, "ERROR:  Short GMS file\n");
            return -1;
        }

        /* ---------------------------------------------------------------- */
        /*  Odd ranges not allowed on packed ranges.                        */
        /* ---------------------------------------------------------------- */
        if (pack && (len & 1))
        {
            fprintf(stderr, "INTERNAL ERROR:  Odd packed range size\n");
            return -1;
        }

        /* ---------------------------------------------------------------- */
        /*  Mark the segment as a "preload" segment in the Intellicart.     */
        /*  We don't use ADDSEG to copy the data over, since it's not yet   */
        /*  packed in uint16_t's.                                            */
        /* ---------------------------------------------------------------- */
        if ((err = icartrom_addseg(ic, NULL, addr, len >> 1,
                   ICARTROM_PRELOAD | gmsl_fixed_ranges[i].flags, 0)) != 0)
        {
            fprintf(stderr, "Error '%s' setting icartrom range\n",
                    rom_errors[err]);
            return -1;
        }

        /* ---------------------------------------------------------------- */
        /*  Copy the data over, packing it if needed.                       */
        /* ---------------------------------------------------------------- */
        for (j = lo; j <= hi; j += (1 + pack))
        {
            if (pack)
            {
                data = ( img[j    ]       & 0x00FF) |
                       ((img[j + 1] << 8) & 0xFF00);
            } else
            {
                data = img[j];
            }
            ic->image[0xFFFF & addr++] = data;
        }
    }

    return 0;
}

/* ======================================================================== */
/*  COPY_OVER_GAME -- Scans for game ranges and copies them to the icart    */
/* ======================================================================== */
LOCAL int copy_over_game(uint8_t *img, int gms_len, icartrom_t *ic)
{
    int i;
    int lo, hi, rhi, j;
    uint16_t addr, addr_lo;

    /* -------------------------------------------------------------------- */
    /*  Copy data over from each of the ranges.                             */
    /* -------------------------------------------------------------------- */
    for (i = 0; gmsl_game_ranges[i].ofs_lo < gmsl_game_ranges[i].ofs_hi; i++)
    {
        lo   = gmsl_game_ranges[i].ofs_lo;
        hi   = gmsl_game_ranges[i].ofs_hi;
        addr = gmsl_game_ranges[i].dst_addr;

        /* ---------------------------------------------------------------- */
        /*  Make sure we stay inside the file we read.                      */
        /* ---------------------------------------------------------------- */
        if (lo > gms_len || hi > gms_len)
            return -1;

        /* ---------------------------------------------------------------- */
        /*  Process the lo-hi range in 256-word increments.  If we see      */
        /*  any bytes other than $FF, we process this range.                */
        /* ---------------------------------------------------------------- */
        for (rhi = lo + 512; lo < hi; lo += 512, rhi += 512, addr += 256)
        {
            for (j = lo; j < rhi; j++)
            {
                if (img[j] != 0xFF)
                    break;
            }
            if (j == rhi) continue;

            if (icartrom_addseg(ic, NULL, addr, 256,
                                ICARTROM_PRELOAD|gmsl_game_ranges[i].flags, 0))
                return -1;

            for (j = lo, addr_lo = addr; j < rhi; j += 2, addr_lo++)
            {
                ic->image[addr_lo] = ( img[j + 0]       & 0x00FF) |
                                     ((img[j + 1] << 8) & 0xFF00);
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Special processing:  Try to detect Chess/Triple Challenge w/ RAM    */
    /* -------------------------------------------------------------------- */
    for (addr = 0xD000; addr < 0xD400; addr++)
    {
        if (img[addr * 2 + 1])
            break;
    }
    if (addr == 0xD400)
        icartrom_addseg(ic, NULL, 0xD000, 1024, ICARTROM_WRITE, 0);

    for (addr = 0xD400; addr < 0xD800; addr++)
    {
        if (img[addr * 2 + 1])
            break;
    }
    if (addr == 0xD800)
        icartrom_addseg(ic, NULL, 0xD400, 1024, ICARTROM_WRITE, 0);


    return 0;
}

/* ======================================================================== */
/*  MAIN                                                                    */
/*  This is the main program.  The action happens here.                     */
/* ======================================================================== */
int main(int argc, char *argv[])
{
    FILE *f;
    int err;
    long len;
    uint32_t rom_size;
    char *ifn, *ofn, *s;

    /* -------------------------------------------------------------------- */
    /*  Parse arguments and open the GMS file.                              */
    /* -------------------------------------------------------------------- */
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "usage: gms2rom foo.gms [bar.rom]\n");
        exit(1);
    }

    ifn = argv[1];

    if (argc == 3)
    {
        ofn = argv[2];
    } else
    {
        if ((ofn = (char *)malloc(strlen(ifn) + 5)) == NULL)
        {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
        strcpy(ofn, ifn);

        if (((s = strstr(ofn, ".gms")) == NULL &&
             (s = strstr(ofn, ".GMS")) == NULL) ||
             s[4] != '\0')
        {
            s = ofn + strlen(ifn);
        }

        strcpy(s, ".rom");
    }


    /* -------------------------------------------------------------------- */
    /*  Determine the GMS filesize and load it in.                          */
    /* -------------------------------------------------------------------- */
    f = fopen(ifn, "rb");

    if (!f)
    {
        perror("fopen()");
        fprintf(stderr, "Couldn't open '%s' for reading\n", argv[1]);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    if ((len = ftell(f)) < 0)
    {
        fprintf(stderr, "Error seeking\n");
        exit(1);
    }
    rewind(f);

    if ((gms_img = (uint8_t *)malloc(len)) == NULL)
    {
        fprintf(stderr, "Out of memory. Buy more.\n");
        exit(1);
    }

    if (fread(gms_img, 1, len, f) != (size_t)len)
    {
        perror("fread()");
        fprintf(stderr, "Error reading GMS file!\n");
        exit(1);
    }
    fclose(f);

    if (len < 0x2003A)
    {
        fprintf(stderr, "ERROR:  GMS file is too short!\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Now initialize the_icart and prepare to put game-state in it.       */
    /* -------------------------------------------------------------------- */
    icartrom_init(&the_icart);
    if ((err = icartrom_addseg(&the_icart, gms_loader, GMSL_PROGRAM,
                               sizeof(gms_loader) / sizeof(uint16_t),
                               ICARTROM_PRELOAD | ICARTROM_READ, 0)) != 0)
    {
        fprintf(stderr, "Unable to initialize the_icart: %s\n",
                rom_errors[err]);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  If SDBD flag is set, rewind PC by 1.  If we don't find an SDBD      */
    /*  opcode there, we're in trouble!!                                    */
    /* -------------------------------------------------------------------- */
    if (gms_img[0x20030] != 1)
    {
        int pc;

        pc = ((gms_img[0x2002D] << 8) & 0xFF00) |
             ((gms_img[0x2002C] << 0) & 0x00FF);

        if (gms_img[pc*2 + 0] != 0x0001 ||
            gms_img[pc*2 + 1] != 0x0000)
        {
            fprintf(stderr, "Error:  SDBD mode set, but SDBD not found\n");
            exit(1);
        }

        pc--;
        gms_img[0x2002D] = 0xFF & (pc >> 8);
        gms_img[0x2002C] = 0xFF & pc;
        gms_img[0x20030] = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Adjust the flags word so we can just do RSWD on the target....      */
    /* -------------------------------------------------------------------- */
    gms_img[0x2002E] <<= 4;

    /* -------------------------------------------------------------------- */
    /*  Extract the fixed offset ranges in the GMS file and put them in     */
    /*  the Intellicart address map.  This captures the CPU status.         */
    /* -------------------------------------------------------------------- */
    if (copy_over_fixed(gms_img, len, &the_icart))
    {
        fprintf(stderr, "Error copying fixed ranges to Intellicart\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Extract the game ranges in the GMS file and put them in the cart    */
    /*  image.  These get inserted into both the Intellicart AND the        */
    /*  Intellivision's memory map.                                         */
    /* -------------------------------------------------------------------- */
    if (copy_over_game(gms_img, len, &the_icart))
    {
        fprintf(stderr, "Error copying game ranges to Intellicart\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Set up our bank-switch window.                                      */
    /* -------------------------------------------------------------------- */
    if ((err = icartrom_addseg(&the_icart, NULL, GMSL_WINDOW, GMSL_WINDSIZE,
                        ICARTROM_WRITE|ICARTROM_READ|ICARTROM_BANKSW, 0)) != 0)
    {
        fprintf(stderr, "Error '%s' setting up bankswitch window\n",
                rom_errors[err]);
        exit(1);
    }
    /* -------------------------------------------------------------------- */
    /*  Set up our scratch-pad area.                                        */
    /* -------------------------------------------------------------------- */
    if ((err = icartrom_addseg(&the_icart, NULL, GMSL_SCRATCH, GMSL_SCRSIZE,
                          ICARTROM_WRITE | ICARTROM_READ, 0)) != 0)
    {
        fprintf(stderr, "Error '%s' setting up bankswitch window\n",
                rom_errors[err]);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Copy the ISR vector at $100/$101 to reg-save area.                  */
    /* -------------------------------------------------------------------- */
    the_icart.image[0x004D] = ( the_icart.image[0x0100]       & 0x00FF) |
                              ((the_icart.image[0x0101] << 8) & 0xFF00);

    /* -------------------------------------------------------------------- */
    /*  Now do one final patch-up:  Add "JD GMSL_PROGRAM" at $7000.         */
    /* -------------------------------------------------------------------- */
    the_icart.image[0x7000] = 0x0004;  /* J / JSR opcode */
    the_icart.image[0x7001] = 0x0302 | ((GMSL_PROGRAM >> 8) & 0xFC);
    the_icart.image[0x7002] = GMSL_PROGRAM & 0x3FF;
    if ((err = icartrom_addseg(&the_icart, NULL, 0x7000, 3,
                              ICARTROM_PRELOAD | ICARTROM_READ, 0)) != 0)
    {
        fprintf(stderr, "Error '%s' patching in JD GMSL_PROGRAM\n",
                rom_errors[err]);
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*  Lastly:  Generate the ROM image.                                    */
    /* -------------------------------------------------------------------- */
    if ((rom_img = icartrom_genrom(&the_icart, &rom_size, ICART)) == NULL)
    {
        fprintf(stderr, "Error generating .ROM file\n");
        exit(1);
    }


    f = fopen(ofn, "wb");
    if (f) { fwrite(rom_img, 1, rom_size, f); fclose(f); }
    else
    {
        fprintf(stderr, "Could not open output file '%s'.\n", ofn);
        exit(1);
    }

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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
