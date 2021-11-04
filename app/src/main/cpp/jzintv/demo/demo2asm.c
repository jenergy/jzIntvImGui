/* ======================================================================== */
/*  DEMO to ASM conversion                                      J. Zbiciak  */
/*                                                                          */
/*  This reads a .dmo file recorded by jzIntv, and produces an assembly     */
/*  file intended for a demo player running on an Intellivision.  The goal  */
/*  here is to make "demo carts" from existing games that do not contain    */
/*  the games themselves.  This theoretically should simplify the process   */
/*  of making demo carts.                                                   */
/*                                                                          */
/*  Since we try to optimize the encoded demo, the process becomes very     */
/*  memory intensive.  We suck the entire recorded demo into memory,        */
/*  recreating the entire BTAB, STIC and PSG state for each frame.  On      */
/*  the commandline, the user can specify which frame ranges should get     */
/*  written to the resulting assembly file.                                 */
/*                                                                          */
/*  Ultimately, this tool should be able to optimize across multiple demo   */
/*  recordings.  Pipe dreams... pipe dreams.                                */
/*                                                                          */
/*  The assembly format output by this converter will evolve alongside the  */
/*  player.  Make sure that the player and the converter are matched!       */
/* ======================================================================== */

#include "config.h"

/* ======================================================================== */
/*  Utility functions for pulling apart frames.                             */
/* ======================================================================== */
static inline uint32_t get_32(uint8_t **buf)
{
    uint32_t word;

    word = ((*buf)[3] << 24) |
           ((*buf)[2] << 16) |
           ((*buf)[1] <<  8) |
           ((*buf)[0] <<  0);

    *buf += 4;

    return word;
}

static inline uint16_t get_16(uint8_t **buf)
{
    uint16_t word;

    word = ((*buf)[1] <<  8) |
           ((*buf)[0] <<  0);

    *buf += 2;

    return word;
}

static inline uint8_t get_8(uint8_t **buf)
{
    return *(*buf)++;
}

#define GET_32(buf) get_32(&buf)
#define GET_16(buf) get_16(&buf)
#define GET_8(buf)  get_8 (&buf)

/* ======================================================================== */
/*  Frame format:                                                           */
/*                                                                          */
/*      4 bytes     0x2A3A4A5A  Frame header                                */
/*      4 bytes     Bitmap of changed STIC registers                        */
/*      8 bytes     Bitmap of changed GRAM cards                            */
/*      30 bytes    Bitmap of changed BTAB cards                            */
/*      2 bytes     Bitmap of changed PSG0 registers                        */
/*      2 bytes     Bitmap of changed PSG1 registers                        */
/*      N bytes     STIC register values (2 bytes each)                     */
/*      N bytes     GRAM tiles (8 bytes each)                               */
/*      N bytes     BTAB cards (2 bytes each)                               */
/*      N bytes     PSG0 registers (1 byte each)                            */
/*      N bytes     PSG1 registers (1 byte each)                            */
/*                                                                          */
/* ======================================================================== */

#define HDR_SZ (4+4+8+30+2+2)

typedef struct frame_t
{
    struct frame_t *next, *prev;

    uint32_t stic_chg;
    uint32_t gram_chg[2];
    uint32_t btab_chg[8];
    uint32_t psg0_chg;
    uint32_t psg1_chg;

    uint16_t stic[32 ];
    uint16_t gram[64 ];  // tile_db indices, not images.
    uint16_t btab[240];
    uint8_t  psg0[14 ];
    uint8_t  psg1[14 ];
} frame_t;

/* ======================================================================== */
/*  TILE_DB          -- We keep a database of all GRAM images we've seen.   */
/*                      Rather than store each GRAM bitmap explicitly, we   */
/*                      store a database index.  The GRAM_HASH maintains    */
/*                      this database.                                      */
/* ======================================================================== */

typedef struct tile_t
{
    uint8_t tile[8];
    struct tile_t *hs_next; /* chained hashing.                             */
} tile_t;

#define MAX_TILES (4096)    /* 16Kwords worth of tiles.                     */
#define HASH_SZ   (139)

tile_t *tile_hash[HASH_SZ];
tile_t  tile_db[MAX_TILES];
int num_tiles = 0;

int tile_to_id(uint8_t *tile)
{
    uint32_t r0, r1;
    int hash;
    int i;
    tile_t *t;

    r0 = tile[0] ^ (tile[1] << 7) ^ (tile[2] << 14) ^ (tile[3] << 21);
    r1 = tile[4] ^ (tile[5] << 7) ^ (tile[6] << 14) ^ (tile[7] << 21);

    hash = (r0 * 33 + r1 * 31) % HASH_SZ;

    t = tile_hash[hash];

    while (t)
    {
        if (!memcmp(tile, t->tile, 8)) // match?
            return t - tile_db;

        t = t->hs_next;
    }

    if (num_tiles == MAX_TILES)
    {
        fprintf(stderr, "GRAM tile database overflow!\n");
        exit(1);
    }

    t               = &tile_db[num_tiles++];
    t->hs_next      = tile_hash[hash];
    tile_hash[hash] = t;

    memcpy(t->tile, tile, 8);

    return t - tile_db;
}

/* ======================================================================== */
/*  For now let's start small.  Read in the demo file, populate a bunch     */
/*  of frames, and see if we can, sloppily, play back GRAM, BTAB and STIC.  */
/* ======================================================================== */

int gr_chg_hist[64];
int bt_chg_hist[240];


/* ======================================================================== */
/*  READ_DEMO_FILE   -- Load up all the frames from a demo file and         */
/*                      decompress them into our linked list structure.     */
/*                                                                          */
/*  This isn't elegant code at all.  It's the worst sort of case-n-paste.   */
/* ======================================================================== */
frame_t *read_demo_file(char *fname)
{
    frame_t *head = NULL;
    frame_t *curr = NULL;
    frame_t *prev = NULL;
    FILE *f;

    uint8_t hdr[HDR_SZ], *buf;
    uint8_t stic_tmp[32*2];
    uint8_t gram_tmp[512];
    uint8_t btab_tmp[240*2];
    uint8_t psg0_tmp[14];
    uint8_t psg1_tmp[14];
    uint32_t sig;
    int r;
    int i, j;
    int stic_cnt, gram_cnt, btab_cnt, psg0_cnt, psg1_cnt;
    int frame_no = 0;

    /* -------------------------------------------------------------------- */
    /*  Open up the file and prepare to parse!                              */
    /* -------------------------------------------------------------------- */
    if (!(f = fopen(fname, "rb")))
    {
        perror("fopen()");
        fprintf(stderr, "Could not open '%s' for reading\n", fname);
        exit(1);
    }

    while ((r = fread(hdr, 1, HDR_SZ, f)) == HDR_SZ)
    {
        /* ---------------------------------------------------------------- */
        /*  Allocate structure and prepare to read the header.              */
        /* ---------------------------------------------------------------- */
        curr = calloc(sizeof(frame_t), 1);
        if (!curr)
        {
            perror("calloc()");
            fprintf(stderr, "Out of memory in read_demo_file\n");
            exit(1);
        }

        curr->prev = prev;
        if (prev)
            prev->next = curr;
        if (!head)
            head = curr;

        buf = hdr;

        sig = GET_32(buf);

        if (sig != 0x2A3A4A5A)
        {
            fprintf(stderr,
                    "Expected frame signature, got %.8X instead\n"
                    "File offset:  %llu\n"
                    "Frame number: %d\n",
                    sig, (uint64_t)ftell(f), frame_no);
            exit(1);
        }

        /* ---------------------------------------------------------------- */
        /*      4 bytes     Bitmap of changed STIC registers                */
        /*      8 bytes     Bitmap of changed GRAM cards                    */
        /*      30 bytes    Bitmap of changed BTAB cards                    */
        /*      2 bytes     Bitmap of changed PSG0 registers                */
        /*      2 bytes     Bitmap of changed PSG1 registers                */
        /* ---------------------------------------------------------------- */
        curr->stic_chg    = GET_32(buf);
        curr->gram_chg[0] = GET_32(buf);
        curr->gram_chg[1] = GET_32(buf);
        curr->btab_chg[0] = GET_32(buf);
        curr->btab_chg[1] = GET_32(buf);
        curr->btab_chg[2] = GET_32(buf);
        curr->btab_chg[3] = GET_32(buf);
        curr->btab_chg[4] = GET_32(buf);
        curr->btab_chg[5] = GET_32(buf);
        curr->btab_chg[6] = GET_32(buf);
        curr->btab_chg[7] = GET_16(buf); /* note GET_16... */
        curr->psg0_chg    = GET_16(buf);
        curr->psg1_chg    = GET_16(buf);


        /* ---------------------------------------------------------------- */
        /*  Now read all the elements, if they were sent.                   */
        /* ---------------------------------------------------------------- */
        stic_cnt = gram_cnt = btab_cnt = psg0_cnt = psg1_cnt = 0;

        for (i = 0; i < 32; i++)
        {
            if ((curr->stic_chg    >> i) & 1) stic_cnt++;
            if ((curr->gram_chg[0] >> i) & 1) gram_cnt++;
            if ((curr->gram_chg[1] >> i) & 1) gram_cnt++;
            if ((curr->btab_chg[0] >> i) & 1) btab_cnt++;
            if ((curr->btab_chg[1] >> i) & 1) btab_cnt++;
            if ((curr->btab_chg[2] >> i) & 1) btab_cnt++;
            if ((curr->btab_chg[3] >> i) & 1) btab_cnt++;
            if ((curr->btab_chg[4] >> i) & 1) btab_cnt++;
            if ((curr->btab_chg[5] >> i) & 1) btab_cnt++;
            if ((curr->btab_chg[6] >> i) & 1) btab_cnt++;
            if ((curr->btab_chg[7] >> i) & 1) btab_cnt++;
            if ((curr->psg0_chg    >> i) & 1) psg0_cnt++;
            if ((curr->psg1_chg    >> i) & 1) psg1_cnt++;
        }

        if (stic_cnt &&
            (r = fread(stic_tmp, 2, stic_cnt, f)) != stic_cnt)
        {
            fprintf(stderr,
                    "Short read getting STIC regs from frame.\n"
                    "File offset:  %llu\n", (uint64_t)ftell(f));
            exit(1);
        }

        if (gram_cnt &&
            (r = fread(gram_tmp, 8, gram_cnt, f)) != gram_cnt)
        {
            fprintf(stderr,
                    "Short read getting GRAM tiles from frame.\n"
                    "File offset:  %llu\n", (uint64_t)ftell(f));
            exit(1);
        }

        if (btab_cnt &&
            (r = fread(btab_tmp, 2, btab_cnt, f)) != btab_cnt)
        {
            fprintf(stderr,
                    "Short read getting BTAB cards from frame.\n"
                    "File offset:  %llu\n", (uint64_t)ftell(f));
            exit(1);
        }

        if (psg0_cnt &&
            (r = fread(psg0_tmp, 1, psg0_cnt, f)) != psg0_cnt)
        {
            fprintf(stderr,
                    "Short read getting PSG0 registers from frame.\n"
                    "File offset:  %llu\n", (uint64_t)ftell(f));
            exit(1);
        }

        if (psg1_cnt &&
            (r = fread(psg1_tmp, 1, psg1_cnt, f)) != psg1_cnt)
        {
            fprintf(stderr,
                    "Short read getting PSG1 registers from frame.\n"
                    "File offset:  %llu\n", (uint64_t)ftell(f));
            exit(1);
        }

        /* ---------------------------------------------------------------- */
        /*  Unpack all the elements into the frame structure.               */
        /* ---------------------------------------------------------------- */
        if (prev)
        {
            memcpy(curr->stic, prev->stic, sizeof(curr->stic));
            memcpy(curr->gram, prev->gram, sizeof(curr->gram));
            memcpy(curr->btab, prev->btab, sizeof(curr->btab));
            memcpy(curr->psg0, prev->psg0, sizeof(curr->psg0));
            memcpy(curr->psg1, prev->psg1, sizeof(curr->psg1));
        }

        buf = stic_tmp;
        for (i = 0; i < 32; i++)
            if (1 & (curr->stic_chg >> i))
                curr->stic[i] = GET_16(buf);

        buf = gram_tmp;
        for (i = 0; i < 64; i++)
            if (1 & (curr->gram_chg[i >> 5] >> (i & 31)))
            {
                curr->gram[i] = tile_to_id(buf);
                buf += 8;
            }

        buf = btab_tmp;
        for (i = 0; i < 240; i++)
            if (1 & (curr->btab_chg[i >> 5] >> (i & 31)))
                curr->btab[i] = GET_16(buf);

        buf = psg0_tmp;
        for (i = 0; i < 14; i++)
            if (1 & (curr->psg0_chg >> i))
                curr->psg0[i] = GET_8(buf);

        buf = psg1_tmp;
        for (i = 0; i < 14; i++)
            if (1 & (curr->psg1_chg >> i))
                curr->psg1[i] = GET_8(buf);

        prev = curr;
        frame_no++;

        /* ---------------------------------------------------------------- */
        /*  Stats keeping.                                                  */
        /* ---------------------------------------------------------------- */
        gr_chg_hist[gram_cnt]++;
        bt_chg_hist[btab_cnt]++;
    }

    /* -------------------------------------------------------------------- */
    /*  And that's it.  Seriously!                                          */
    /* -------------------------------------------------------------------- */
    return head;
}


main(int argc, char *argv[])
{
    int i, j, k;

    read_demo_file(argv[1]);
    printf("did it!\n");
    printf("%d unique GRAM tiles\n", num_tiles);

    for (i = 0; i < num_tiles; i++)
    {
        printf("tile %d\n", i);
        for (j = 0; j < 8; j++)
        {
            for (k = 0; k < 8; k++)
                putchar((tile_db[i].tile[j] << k) & 0x80 ? '#': '.');
            putchar('\n');
        }
    }

    for (i = 0; i < 64; i++)
        printf("%4d", gr_chg_hist[i]);

    return 0;
}
