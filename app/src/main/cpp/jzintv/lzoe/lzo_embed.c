/* ======================================================================== */
/*  LZO_EMBED                                                               */
/*                                                                          */
/*  Use Mini-LZO to compress and obscure files that can then be embedded    */
/*  in another program.                                                     */
/*                                                                          */
/*  Usage:                                                                  */
/*      lzo_embed outfile_base file1 [file2 [file3 [...]]]                  */
/*                                                                          */
/*  The name "outfile_base" is the name of the desired output file, sans    */
/*  file suffixes.  lzo_embed will write two files with that prefix:  A     */
/*  .c file and a corresponding .h file.                                    */
/*                                                                          */
/*  Within the file outfile_base, lzo_embed will output an array of         */
/*  exported structures named "lzoe_[outfile_base]".  Each element of       */
/*  that array is a structure containing the following information:         */
/*                                                                          */
/*   -- A pointer to the embedded file's name                               */
/*   -- A pointer to the compressed, obscured data                          */
/*   -- The length of the compressed and decompressed output                */
/*   -- An 8 byte "obscuring key"                                           */
/*                                                                          */
/*  The obscuring key is not meant as any sort of hardened encryption.      */
/*  It's only meant to lightly obscure the compressed contents.  Anyone     */
/*  with sufficient motivation could extract the embedded file.             */
/*                                                                          */
/* ======================================================================== */
#ifdef NO_LZO
int main() { return 1; }
#else

#include "config.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "minilzo/minilzo.h"

#define DEFAULT_ENC_BUF_SZ (1l << 16)

LOCAL uint8_t   *lzo_wrk = NULL, *lzo_inp = NULL, *lzo_out = NULL;
LOCAL long       lzo_enc_buf_sz = DEFAULT_ENC_BUF_SZ;
LOCAL lzoe_info *info = NULL;
LOCAL int        lzoe_cnt;
LOCAL char      *cheader, *csource;
LOCAL char      *uc_name, *lc_name;


LOCAL void lzo_buf_alloc( long at_least )
{
    static int needed = 1;

    if (!lzo_wrk)
    {
        if (! (lzo_wrk = CALLOC(uint8_t, LZO1X_MEM_COMPRESS)) ) goto oom;
    }

    while (at_least > lzo_enc_buf_sz)
    {
        lzo_enc_buf_sz <<= 1;
        needed = 1;
    }

    if (needed)
    {

        lzo_inp = REALLOC(lzo_inp, uint8_t, lzo_enc_buf_sz);
        lzo_out = REALLOC(lzo_out, uint8_t, lzo_enc_buf_sz * 1088 / 1024);
        needed  = 0;

        if (!lzo_inp || !lzo_out)
            goto oom;
    }

    return;

oom:
    fprintf(stderr, "Out of memory\n");
    exit(1);
}

LOCAL lzoe_info *next_lzoe( void )
{
    info = REALLOC(info, lzoe_info, ++lzoe_cnt);
    return info + lzoe_cnt - 1;
}

LOCAL void push_lzoe_info( const char *fname, long orig_len, long comp_len,
                           const uint8_t *cdata )
{
    uint8_t *buf = CALLOC( uint8_t, comp_len );

    lzoe_info *new_info = next_lzoe();

    new_info->fname    = fname;
    new_info->orig_len = orig_len;
    new_info->comp_len = comp_len;
    new_info->cdata    = buf;

    if (!new_info->cdata)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    memcpy(buf, cdata, comp_len);
}

LOCAL void read_and_compress( int cnt, char *fnames[] )
{
    int i;

    /* Read and compress all the input files */
    for (i = 0; i < cnt; i++)
    {
        const char *fname = fnames[i];
        FILE *f;
        long  fl;
        int   r;
        lzo_uint lzo_len = 0;

        if (! (f = fopen( fname, "rb" ) ) )
        {
            perror("fopen()");
            fprintf(stderr, "Error opening %s for reading\n", fname);
            exit(1);
        }

        fseek(f, 0, SEEK_END);
        fl = ftell(f);
        rewind(f);
        lzo_buf_alloc( fl );

        if ( (long)fread( lzo_inp, 1, fl, f ) != fl )
        {
            perror("fread()");
            fprintf(stderr, "Error reading input file %s\n", fname);
            exit(1);
        }
        fclose( f );

        r = lzo1x_1_compress( lzo_inp, (lzo_uint)fl, lzo_out, &lzo_len,
                              (lzo_voidp)lzo_wrk );

        if (r == LZO_E_OK)
            push_lzoe_info( fname, fl, lzo_len, lzo_out );

        printf("Compressed %s from %ld bytes to %ld bytes.\n", fname,
               fl, (long)lzo_len );
    }
}

LOCAL void make_names_from_base( const char *const base_name )
{
    int i, blen;

    blen    = strlen(base_name);
    cheader = CALLOC(char, blen + 2);
    csource = CALLOC(char, blen + 2);
    uc_name = CALLOC(char, blen);
    lc_name = CALLOC(char, blen);

    if (!cheader || !csource || !uc_name || !lc_name)
    {
        fprintf( stderr, "Out of memory\n" );
        exit(1);
    }

    memcpy(cheader, base_name, blen); memcpy(cheader + blen, ".h", 3);
    memcpy(csource, base_name, blen); memcpy(csource + blen, ".c", 3);

    for (i = 0; i < blen; i++)
    {
        int uc = base_name[i], lc;

        if      (!isalpha( uc ) && !isdigit( uc )) uc = lc = '_';
        else    { lc = tolower(uc); uc = toupper(uc); }

        uc_name[i] = uc;
        lc_name[i] = lc;
    }

    uc_name[ blen ] = lc_name[ blen ] = 0;
}

LOCAL void write_cheader( void )
{
    FILE *fh = fopen( cheader, "w" );

    if (!fh)
    {
        perror( "fopen()" );
        fprintf(stderr, "Error opening %s for writing\n", cheader);
        exit(1);
    }

    fprintf( fh,
             "#ifndef LZOE_%s\n"
             "#define LZOE_%s 1\n"
             "\n"
             "#include \"lzoe/lzoe.h\"\n"
             "\n"
             "extern const lzoe_info lzoe_%s[%d];\n"
             "#endif\n",
             uc_name, uc_name, lc_name, lzoe_cnt + 1 );

    fclose( fh );
}


LOCAL void write_csource( void )
{
    FILE *fc = fopen( csource, "w" );
    int i, j;

    if (!fc)
    {
        perror( "fopen()" );
        fprintf(stderr, "Error opening %s for writing\n", cheader);
        exit(1);
    }

    fprintf( fc, "#include \"config.h\"\n"
                 "#include \"lzoe/lzoe.h\"\n\n" );

    for (i = 0; i < lzoe_cnt; i++)
    {
        fprintf( fc, "static const uint8_t cdata_%04d[] =\n{\n", i );

        for (j = 0; j < info[i].comp_len; j++)
        {
            if ((j & 7) == 0) fprintf(fc, "    ");

            fprintf( fc, "0x%02X", info[i].cdata[j] );

            if (j == info[i].comp_len - 1) fprintf( fc, "\n};\n\n" );
            else if ((j & 7) == 7)         fprintf( fc, ",\n"      );
            else                           fprintf( fc, ", "       );
        }
    }

    fprintf( fc, "const lzoe_info lzoe_%s[%d] =\n{\n", lc_name, lzoe_cnt + 1);

    for (i = 0; i < lzoe_cnt; i++)
    {
        fprintf( fc, "    { \"%s\", %ld, %ld, cdata_%04d },\n",
                 info[i].fname, info[i].orig_len, info[i].comp_len, i
               );
    }

    fprintf(fc, "    { NULL, 0, 0, NULL }\n};\n");
    fclose( fc );
}


int main( int argc, char *argv[] )
{
    if (argc < 3)
    {
        fprintf(stderr,
                "Usage: lzo_embed outfile_base file1 [file2 [file3 [...]]]\n");
        exit(1);
    }

    make_names_from_base( argv[1] );
    read_and_compress( argc - 2, argv + 2 );
    write_cheader( );
    write_csource( );

    return 0;
}


#endif
