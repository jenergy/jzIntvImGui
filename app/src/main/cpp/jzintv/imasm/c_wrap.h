#ifndef C_WRAP_H_
#define C_WRAP_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ignore_flags
{
    char    skip_parse;
    char    skip_macro;
} ignore_flags_t;

struct parser_callbacks
{
    int (*getline)(char *buf, int len, ignore_flags_t *ignore, void *opaque);
    void *gl_opaque;

    const char* (*get_pos)(int *line, void *opaque);
    void *gp_opaque;

    int  (*get_eof)(void *opaque);
    void *ge_opaque;

    int  (*reexam )(char *buf, int len, ignore_flags_t *ignore, void *opaque);
    void *rx_opaque;

    void (*report_error)(const char *buf, void *opaque);
    void *re_opaque;
};

int  init_parser (struct parser_callbacks *pc);
void close_parser(void);
char *get_parsed_line(char **buf, int *maxlen, ignore_flags_t *ignore);

#ifdef __cplusplus
}
#endif

#endif
