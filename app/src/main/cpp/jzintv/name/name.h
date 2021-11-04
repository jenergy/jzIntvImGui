
typedef struct cart_name_t
{
    uint32_t    crc32;
    short       year;
    char        ecs, ivc;
    const char *name;
    const char *short_name;
} cart_name_t;

struct game_metadata_t;  /* forward decl. */

int find_cart_metadata(uint32_t crc32, struct game_metadata_t *meta);

extern const cart_name_t *name_list;
