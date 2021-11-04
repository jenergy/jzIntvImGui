#include "intvec.h"

/* frasmain */
int asm_main(int argc, char *argv[]);
void frafatal(char * str);
void frawarn(char * str);
void fraerror(const char * str);
void fracherror(char *str, char *start, char *beyond);
void prtequvalue(char *fstr, int lv);
void prtcomment(char *fstr, const char *msg);
void printsymbols(void);
void filesymbols(void);

/* fraosub */
void outphase(void);
void outeval(void);
void flushlisthex(void);
void listhex(void);
void listouthex(void);
void outhexblock(void);
void flushhex(void);
void frp2undef(struct symel *symp);
void frp2warn(const char *str);
void frp2error(const char *str);
void flushsourceline(void);
void sm_outpath(void);
void sm_outrange(int lo, int hi);
void sm_flush(void);

int format_time_string(const struct tm *const t, 
                       const struct tm *const gmt,
                       const char *const fmt, 
                       char *const bufbeg, 
                       const int space);
intvec_t *unpack_time_exprs(const struct tm *const t, 
                            const struct tm *const gmt,
                            const char *const fmt);

/* frapsub */

char *savestring(const char *stx, int len);
void clrexpr(void);
int exprnode(int swact, int left, int op, int right, int value,
             struct symel *symbol);
struct symel *allocsym(void);
int syhash(const char *str);
struct symel *symbentry(const char *str, int toktyp);
struct symel *symbentryidx(const char *str, int toktyp, int hasidx, unsigned idx);
void reservedsym(const char *str, int tok, int value);
void buildsymbolindex(void);
void setophash(void);
int findop(char *str);
int opcodehash(const char *str);
const char *findgen(int op, int syntax, int crit);
void goutch(char ch);
void gout2hex(int inv);
void goutxnum(unsigned int num);
int geninstr(const char *str);
int chtcreate(void);
int chtcfind(int *chtab, char **sourcepnt, int **tabpnt, int *numret);
int chtran(char **sourceptr);
int genstring(char *str);
void pevalexpr(int sub, int exn);
/*void polout(char ch);*/
void polnumout(unsigned int inv);
int pepolcon(int esub);
unsigned long rotl16(unsigned long val, int amt);
unsigned long rotl32(unsigned long val, int amt);

/* fryylex */

void frarptdel(void);
int frarptnxt(void);
void frarptadd(char *buf);
void frarptbreak(void);
void frarptpush(int iters);
void frarptendr(void);
void frarptreset(void);
struct ignore_flags;
int fra_next_line(char *buf, int maxlen, struct ignore_flags *ignore, void *u);
const char *fra_get_pos(int *line, void *u);
int  fra_get_eof(void *u);
void fra_rpt_err(const char *buf, void *u);
int frareadrec(void);
int yylex(void);
int yyerror(char *str);

/* as1600.y */
int lexintercept(void);
void setreserved(void);
int cpumatch(char *str);
int yyparse(void);
