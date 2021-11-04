#ifndef LZOE_H_
#define LZOE_H_

#ifdef NO_LZO
#   define LZFILE               FILE
#   define has_lzoe_prefix(x)   (0)
#   define lzoe_fopen           fopen
#   define lzoe_fgets           fgets
#   define lzoe_fread           fread
#   define lzoe_fseek           fseek
#   define lzoe_ftell           ftell
#   define lzoe_rewind          rewind
#   define lzoe_fclose          fclose
#   define lzoe_exists          file_exists
#   define lzoe_register(x,y)   ((void)(x), (void)(y))
#   define lzoe_filep(f)        (f)
#   define lzoe_fgetc           fgetc
#   define lzoe_feof            feof
#else

typedef struct lzoe_info
{
    const char   *fname;
    long          orig_len, comp_len;
    const uint8_t *cdata;
} lzoe_info;

typedef struct lzoe_file
{
    const lzoe_info *info;
    uint8_t         *ddata;
    long             offset;
} lzoe_file;

typedef struct LZFILE
{
    FILE        *f;
    lzoe_file   *l;
} LZFILE;

typedef struct lzoe_directory
{
    const lzoe_info *table;
    const char      *prefix;
    int              pfx_len;
} lzoe_directory;

#define MAX_LZOE_OPEN   (32)
#define LZOE_PREFIX     "$LZOE$"

extern int      has_lzoe_prefix( const char *fname );
extern LZFILE  *lzoe_fopen ( const char *fname, const char *mode );
extern char    *lzoe_fgets ( char *s, int size, LZFILE *f );
extern size_t   lzoe_fread ( void *buf, size_t size, size_t nmemb, LZFILE *f );
extern int      lzoe_fseek ( LZFILE *f, long offset, int whence );
extern long     lzoe_ftell ( LZFILE *f );
extern int      lzoe_fclose( LZFILE *f );
extern int      lzoe_exists( const char *fname );
extern LZFILE  *lzoe_filep ( FILE *f );
extern int      lzoe_fgetc ( LZFILE *f );
extern int      lzoe_feof  ( LZFILE *f );


#define lzoe_rewind(f) ((void) lzoe_fseek( f, 0L, SEEK_SET ))

extern void     lzoe_register( const char *prefix, const lzoe_info *table );

#endif

#endif
